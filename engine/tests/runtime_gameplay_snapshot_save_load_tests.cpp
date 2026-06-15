#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeGameplaySnapshotChunkCodec.hpp"
#include "runtime/RuntimeGameplaySnapshotSaveLoad.hpp"
#include "runtime/RuntimeSaveChunkArchiveCodec.hpp"
#include "runtime/RuntimeSaveFileEnvelope.hpp"
#include "runtime/RuntimeSaveFileIO.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::runtime::RuntimeSaveChunkId InventoryChunkId = iggy::runtime::makeRuntimeSaveChunkId('G', 'I', 'N', 'V');

template <typename T, typename = void>
struct HasNpcActors : std::false_type {
};

template <typename T>
struct HasNpcActors<T, std::void_t<decltype(std::declval<T>().npcActors)>> : std::true_type {
};

template <typename T, typename = void>
struct HasNpcControls : std::false_type {
};

template <typename T>
struct HasNpcControls<T, std::void_t<decltype(std::declval<T>().npcControls)>> : std::true_type {
};

static_assert(!HasNpcActors<iggy::runtime::RuntimeSessionState>::value);
static_assert(!HasNpcControls<iggy::runtime::RuntimeSessionState>::value);
static_assert(!HasNpcActors<iggy::runtime::RuntimeSessionSnapshot>::value);
static_assert(!HasNpcControls<iggy::runtime::RuntimeSessionSnapshot>::value);

std::filesystem::path TempRoot()
{
	return std::filesystem::temp_directory_path() / "iggy_runtime_gameplay_snapshot_save_load_tests";
}

std::filesystem::path TempPath(const char *name)
{
	return TempRoot() / name;
}

void ResetTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
	std::filesystem::create_directories(TempRoot(), ignored);
}

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId(value);
}

iggy::LevelRuntimeState Level(std::vector<std::string_view> rows)
{
	iggy::LevelRuntimeState level;
	level.map = MapFromRows(rows);
	level.map.id = Id("level:gameplay-save-load");
	level.map.playerStart = { 1, 1 };
	return level;
}

iggy::runtime::RuntimeSessionSnapshot SessionSnapshot(bool hasPlayer)
{
	iggy::runtime::RuntimeSessionSnapshot snapshot;
	snapshot.level = Level({
		"........",
		"........",
		"........",
	});
	snapshot.tickIndex = 44;
	snapshot.hasPlayer = hasPlayer;
	if (hasPlayer)
		snapshot.player = PlayerAgent(Id("player:one"), { 1.5F, 1.5F }, { 1, 1 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::East);
	return snapshot;
}

iggy::runtime::RuntimeGameplaySnapshotRestoreConfig RestoreConfigNoCaches()
{
	iggy::runtime::RuntimeGameplaySnapshotRestoreConfig config;
	config.session.buildConfig.buildRenderCache = false;
	return config;
}

iggy::runtime::RuntimeGameplaySnapshotRestoreConfig BadRestoreConfig()
{
	iggy::runtime::RuntimeGameplaySnapshotRestoreConfig config = RestoreConfigNoCaches();
	config.session.buildConfig.buildDerivedCaches = true;
	config.session.buildConfig.derivedCacheConfig.buildRenderCache = true;
	config.session.buildConfig.derivedCacheConfig.renderCacheConfig.chunkWidth = 0;
	return config;
}

iggy::runtime::GameplayCommand2D Command(
	iggy::runtime::GameplayCommand2DType type,
	const char *actorId)
{
	iggy::runtime::GameplayCommand2D command;
	command.type = type;
	command.actorId = Id(actorId);
	command.targetPoint = { 2.25F, 1.75F };
	command.targetTile = { 2, 1 };
	command.targetId = Id("target:door");
	return command;
}

iggy::NpcActorState2D Actor(const char *id, iggy::Vec2 position, bool present = true)
{
	return { Id(id), Id("ai-profile:guard"), Id("faction:town"), position, Id("goal:watch"), present };
}

iggy::NpcActorControlState2D Control(
	const char *id,
	iggy::NpcObjective objective,
	iggy::NpcBehaviorState behavior,
	iggy::NpcMoveMode moveMode)
{
	iggy::NpcActorControlState2D control;
	control.npcId = Id(id);
	control.objective = objective;
	control.behavior = behavior;
	control.moveMode = moveMode;
	return control;
}

iggy::runtime::RuntimeGameplaySnapshot DefaultSnapshot()
{
	iggy::runtime::RuntimeGameplaySnapshot snapshot;
	snapshot.session = SessionSnapshot(false);
	return snapshot;
}

iggy::runtime::RuntimeGameplaySnapshot ExplicitSnapshot()
{
	iggy::runtime::RuntimeGameplaySnapshot snapshot;
	snapshot.session = SessionSnapshot(true);
	snapshot.commandQueue.frames = {
		{ {
			Command(iggy::runtime::GameplayCommand2DType::MoveToPoint, "player:one"),
			Command(iggy::runtime::GameplayCommand2DType::Interact, "player:one"),
		} },
	};
	snapshot.interaction.targets = iggy::InteractionTarget2DRegistry({
		{ Id("target:inspect"), iggy::InteractionTarget2DKind::Inspectable, { 1.5F, 0.5F }, 0.5F, true },
		{ Id("target:door"), iggy::InteractionTarget2DKind::Door, { 2.5F, 0.5F }, 0.75F, false },
	});
	snapshot.interaction.effects = iggy::InteractionEffectCatalog2D({
		{
			Id("target:inspect"),
			{
				iggy::inspectTextInteractionEffect(Id("target:inspect"), "A heavy latch."),
				iggy::toggleTargetInteractionEffect(Id("target:door"), true),
				iggy::emitInteractionEventEffect(Id("target:door"), Id("event:door-open")),
				iggy::pickupItemInteractionEffect(Id("target:inspect"), Id("drop:key")),
			},
		},
	});
	snapshot.inventory.inventory.stacks = {
		{ Id("item:potion"), 3 },
		{ Id("item:key"), 1 },
	};
	snapshot.inventory.drops.drops = {
		{ Id("drop:key"), Id("item:key"), 1, { 3.5F, 1.5F }, 0.35F, true },
	};
	snapshot.npcActors.actors = {
		Actor("npc:one", { 0.5F, 1.5F }),
		Actor("npc:two", { 1.5F, 1.5F }, false),
		Actor("unqualified", { 2.5F, 1.5F }),
	};
	snapshot.npcControls.entries = {
		Control("npc:one", iggy::moveToNpcObjective({ 4.5F, 1.5F }), iggy::seekingNpcBehaviorState({ 4.5F, 1.5F }), iggy::NpcMoveMode::Walk),
		Control("npc:two", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still),
		Control("unqualified", iggy::interactNpcObjective(Id("target:door")), iggy::interactingNpcBehaviorState(Id("target:door")), iggy::NpcMoveMode::Run),
	};
	return snapshot;
}

bool SameSession(const iggy::runtime::RuntimeSessionState &actual, const iggy::runtime::RuntimeSessionSnapshot &expected)
{
	return actual.level.map.id == expected.level.map.id
		&& actual.level.map.width == expected.level.map.width
		&& actual.level.map.height == expected.level.map.height
		&& actual.tickIndex == expected.tickIndex
		&& actual.hasPlayer == expected.hasPlayer
		&& (!actual.hasPlayer
			|| (actual.player.id == expected.player.id
				&& NearVec(actual.player.position, expected.player.position)
				&& actual.player.spawnTile == expected.player.spawnTile
				&& actual.player.movementStatus == expected.player.movementStatus
				&& actual.player.facing == expected.player.facing));
}

bool SameCommand(const iggy::runtime::GameplayCommand2D &actual, const iggy::runtime::GameplayCommand2D &expected)
{
	return actual.type == expected.type
		&& actual.actorId == expected.actorId
		&& NearVec(actual.targetPoint, expected.targetPoint)
		&& actual.targetTile == expected.targetTile
		&& actual.targetId == expected.targetId;
}

bool SameQueue(const iggy::runtime::RuntimeCommandQueueState &actual, const iggy::runtime::RuntimeCommandQueueState &expected)
{
	if (actual.frames.size() != expected.frames.size())
		return false;
	for (std::size_t frameIndex = 0; frameIndex < actual.frames.size(); ++frameIndex) {
		if (actual.frames[frameIndex].commands.size() != expected.frames[frameIndex].commands.size())
			return false;
		for (std::size_t commandIndex = 0; commandIndex < actual.frames[frameIndex].commands.size(); ++commandIndex) {
			if (!SameCommand(actual.frames[frameIndex].commands[commandIndex], expected.frames[frameIndex].commands[commandIndex]))
				return false;
		}
	}
	return true;
}

bool SameInteraction(const iggy::runtime::RuntimeInteractionState &actual, const iggy::runtime::RuntimeInteractionState &expected)
{
	return actual.targets.targets().size() == expected.targets.targets().size()
		&& actual.effects.entries().size() == expected.effects.entries().size()
		&& (actual.targets.targets().empty() || actual.targets.targets()[0].id == expected.targets.targets()[0].id)
		&& (actual.effects.entries().empty() || actual.effects.entries()[0].effects.size() == expected.effects.entries()[0].effects.size());
}

bool SameInventory(const iggy::runtime::RuntimeInventoryState &actual, const iggy::runtime::RuntimeInventoryState &expected)
{
	return actual.inventory.stacks.size() == expected.inventory.stacks.size()
		&& actual.drops.drops.size() == expected.drops.drops.size()
		&& (actual.inventory.stacks.empty() || actual.inventory.stacks[0].itemId == expected.inventory.stacks[0].itemId)
		&& (actual.drops.drops.empty() || actual.drops.drops[0].id == expected.drops.drops[0].id);
}

bool SameActors(const iggy::NpcActorState2DRegistry &actual, const iggy::NpcActorState2DRegistry &expected)
{
	if (actual.actors.size() != expected.actors.size())
		return false;
	for (std::size_t index = 0; index < actual.actors.size(); ++index) {
		if (actual.actors[index].npcId != expected.actors[index].npcId
			|| actual.actors[index].aiProfileId != expected.actors[index].aiProfileId
			|| actual.actors[index].factionId != expected.actors[index].factionId
			|| !NearVec(actual.actors[index].position, expected.actors[index].position)
			|| actual.actors[index].currentGoalId != expected.actors[index].currentGoalId
			|| actual.actors[index].present != expected.actors[index].present)
			return false;
	}
	return true;
}

bool SameControls(const iggy::NpcActorControlState2DRegistry &actual, const iggy::NpcActorControlState2DRegistry &expected)
{
	if (actual.entries.size() != expected.entries.size())
		return false;
	for (std::size_t index = 0; index < actual.entries.size(); ++index) {
		if (actual.entries[index].npcId != expected.entries[index].npcId
			|| actual.entries[index].objective.type != expected.entries[index].objective.type
			|| actual.entries[index].objective.targetId != expected.entries[index].objective.targetId
			|| actual.entries[index].behavior.type != expected.entries[index].behavior.type
			|| actual.entries[index].behavior.targetId != expected.entries[index].behavior.targetId
			|| actual.entries[index].moveMode != expected.entries[index].moveMode)
			return false;
	}
	return true;
}

void ExpectStateMatchesSnapshot(
	const iggy::runtime::RuntimeGameplayState &state,
	const iggy::runtime::RuntimeGameplaySnapshot &snapshot,
	const char *message)
{
	Expect(SameSession(state.session, snapshot.session), message);
	Expect(SameQueue(state.commandQueue, snapshot.commandQueue), message);
	Expect(SameInteraction(state.interaction, snapshot.interaction), message);
	Expect(SameInventory(state.inventory, snapshot.inventory), message);
	Expect(SameActors(state.npcActors, snapshot.npcActors), message);
	Expect(SameControls(state.npcControls, snapshot.npcControls), message);
}

void WriteBytes(const std::filesystem::path &path, const std::vector<std::uint8_t> &bytes)
{
	const iggy::runtime::RuntimeSaveFileWriteResult write = iggy::runtime::RuntimeSaveFileWriter {}.writeBytes(path, bytes);
	Expect(write.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "fixture should write bytes");
}

void WriteEnvelopePayload(const std::filesystem::path &path, const std::vector<std::uint8_t> &payload)
{
	const iggy::runtime::RuntimeSaveFileEnvelopeEncodeResult envelope =
		iggy::runtime::RuntimeSaveFileEnvelopeEncoder {}.encodePayload(payload);
	Expect(envelope.encoded, "fixture should encode envelope");
	WriteBytes(path, envelope.bytes);
}

void WriteArchive(const std::filesystem::path &path, const iggy::runtime::RuntimeSaveChunkArchive &archive)
{
	const iggy::runtime::RuntimeSaveChunkArchiveEncodeResult archiveBytes =
		iggy::runtime::RuntimeSaveChunkArchiveEncoder {}.encode(archive);
	Expect(archiveBytes.encoded, "fixture should encode archive");
	WriteEnvelopePayload(path, archiveBytes.bytes);
}

iggy::runtime::RuntimeSaveChunk *FindChunk(
	iggy::runtime::RuntimeSaveChunkArchive &archive,
	iggy::runtime::RuntimeSaveChunkId id)
{
	for (iggy::runtime::RuntimeSaveChunk &chunk : archive.chunks) {
		if (chunk.id == id)
			return &chunk;
	}
	return nullptr;
}

std::vector<std::uint8_t> ReadBytes(const std::filesystem::path &path)
{
	const iggy::runtime::RuntimeSaveFileReadResult read = iggy::runtime::RuntimeSaveFileReader {}.readBytes(path);
	Expect(read.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "fixture should read bytes");
	return read.bytes;
}

void TestSaveDefaultSnapshotWritesFileAndLoads()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("default.gplay");
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = DefaultSnapshot();

	const iggy::runtime::RuntimeGameplaySnapshotSaveResult save =
		iggy::runtime::RuntimeGameplaySnapshotSaver {}.save(path, snapshot);
	const iggy::runtime::RuntimeGameplaySnapshotLoadResult load =
		iggy::runtime::RuntimeGameplaySnapshotLoader {}.load(path);

	Expect(save.status == iggy::runtime::RuntimeGameplaySnapshotSaveStatus::Saved, "default gameplay snapshot should save");
	Expect(std::filesystem::exists(path), "default gameplay snapshot save should write file");
	Expect(load.status == iggy::runtime::RuntimeGameplaySnapshotLoadStatus::Loaded, "default gameplay snapshot should load");
	ExpectStateMatchesSnapshot(load.state, snapshot, "default gameplay snapshot should restore expected gameplay state");
}

void TestExplicitSnapshotRoundTripsAllGameplayChildren()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("explicit.gplay");
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = ExplicitSnapshot();

	const iggy::runtime::RuntimeGameplaySnapshotSaveResult save =
		iggy::runtime::RuntimeGameplaySnapshotSaver {}.save(path, snapshot);
	const iggy::runtime::RuntimeGameplaySnapshotLoadResult load =
		iggy::runtime::RuntimeGameplaySnapshotLoader {}.load(path, RestoreConfigNoCaches());

	Expect(save.status == iggy::runtime::RuntimeGameplaySnapshotSaveStatus::Saved, "explicit gameplay snapshot should save");
	Expect(save.validation.valid, "explicit gameplay snapshot save should preserve validation");
	Expect(load.status == iggy::runtime::RuntimeGameplaySnapshotLoadStatus::Loaded, "explicit gameplay snapshot should load");
	Expect(load.snapshotDecode.decoded, "explicit gameplay snapshot load should preserve gameplay chunk decode");
	Expect(load.restore.restored, "explicit gameplay snapshot load should preserve restore result");
	ExpectStateMatchesSnapshot(load.state, snapshot, "explicit gameplay snapshot should round-trip gameplay children");
}

void TestSaveInvalidSnapshotDoesNotOverwriteFile()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("invalid.gplay");
	const std::vector<std::uint8_t> original { 1, 2, 3, 4 };
	WriteBytes(path, original);
	iggy::runtime::RuntimeGameplaySnapshot snapshot = ExplicitSnapshot();
	snapshot.npcActors.actors[0].position = { 99.5F, 99.5F };

	const iggy::runtime::RuntimeGameplaySnapshotSaveResult save =
		iggy::runtime::RuntimeGameplaySnapshotSaver {}.save(path, snapshot);

	Expect(save.status == iggy::runtime::RuntimeGameplaySnapshotSaveStatus::SnapshotInvalid, "invalid gameplay snapshot should not save");
	Expect(!save.validation.valid, "invalid gameplay snapshot save should preserve validation diagnostics");
	Expect(ReadBytes(path) == original, "invalid gameplay snapshot save should not overwrite existing file");
}

void TestLoadMissingFileFails()
{
	ResetTempRoot();
	const iggy::runtime::RuntimeGameplaySnapshotLoadResult load =
		iggy::runtime::RuntimeGameplaySnapshotLoader {}.load(TempPath("missing.gplay"));

	Expect(load.status == iggy::runtime::RuntimeGameplaySnapshotLoadStatus::FileReadFailed, "missing gameplay snapshot file should fail read");
}

void TestLoadInvalidEnvelopeFails()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("bad-envelope.gplay");
	WriteBytes(path, { 1, 2, 3 });

	const iggy::runtime::RuntimeGameplaySnapshotLoadResult load =
		iggy::runtime::RuntimeGameplaySnapshotLoader {}.load(path);

	Expect(load.status == iggy::runtime::RuntimeGameplaySnapshotLoadStatus::EnvelopeDecodeFailed, "invalid envelope bytes should fail envelope decode");
}

void TestLoadInvalidArchiveFails()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("bad-archive.gplay");
	WriteEnvelopePayload(path, { 1, 2, 3 });

	const iggy::runtime::RuntimeGameplaySnapshotLoadResult load =
		iggy::runtime::RuntimeGameplaySnapshotLoader {}.load(path);

	Expect(load.status == iggy::runtime::RuntimeGameplaySnapshotLoadStatus::ArchiveDecodeFailed, "invalid archive payload should fail archive decode");
}

void TestLoadInvalidGameplayPayloadFailsSnapshotDecode()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("bad-gameplay-payload.gplay");
	iggy::runtime::RuntimeSaveChunkArchive archive =
		iggy::runtime::RuntimeGameplaySnapshotChunkEncoder {}.encode(ExplicitSnapshot()).archive;
	iggy::runtime::RuntimeSaveChunk *inventory = FindChunk(archive, InventoryChunkId);
	if (inventory != nullptr && !inventory->payload.empty())
		inventory->payload.pop_back();
	WriteArchive(path, archive);

	const iggy::runtime::RuntimeGameplaySnapshotLoadResult load =
		iggy::runtime::RuntimeGameplaySnapshotLoader {}.load(path);

	Expect(load.status == iggy::runtime::RuntimeGameplaySnapshotLoadStatus::SnapshotDecodeFailed, "truncated gameplay chunk payload should fail snapshot decode");
	Expect(!load.snapshotDecode.decoded, "truncated gameplay chunk payload should preserve failed snapshot decode");
}

void TestLoadDecodedSnapshotValidationFailureMapsToSnapshotInvalid()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("invalid-decoded-snapshot.gplay");
	iggy::runtime::RuntimeGameplaySnapshot snapshot = ExplicitSnapshot();
	snapshot.npcControls.entries.pop_back();
	WriteArchive(path, iggy::runtime::RuntimeGameplaySnapshotChunkEncoder {}.encode(snapshot).archive);

	const iggy::runtime::RuntimeGameplaySnapshotLoadResult load =
		iggy::runtime::RuntimeGameplaySnapshotLoader {}.load(path);

	Expect(load.status == iggy::runtime::RuntimeGameplaySnapshotLoadStatus::SnapshotInvalid, "decoded gameplay snapshot validation failure should map to SnapshotInvalid");
	Expect(!load.validation.valid, "snapshot validation failure should preserve gameplay validation diagnostics");
}

void TestRestoreFailureMapsToRestoreFailed()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("restore-failure.gplay");
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = ExplicitSnapshot();
	const iggy::runtime::RuntimeGameplaySnapshotSaveResult save =
		iggy::runtime::RuntimeGameplaySnapshotSaver {}.save(path, snapshot);

	const iggy::runtime::RuntimeGameplaySnapshotLoadResult load =
		iggy::runtime::RuntimeGameplaySnapshotLoader {}.load(path, BadRestoreConfig());

	Expect(save.status == iggy::runtime::RuntimeGameplaySnapshotSaveStatus::Saved, "restore failure fixture should save valid snapshot first");
	Expect(load.status == iggy::runtime::RuntimeGameplaySnapshotLoadStatus::RestoreFailed, "failed gameplay restore should map to RestoreFailed");
	Expect(!load.restore.restored, "restore failure should preserve nested restore result");
}

void TestSaveLoadDoNotMutateInputs()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("immutability.gplay");
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = ExplicitSnapshot();
	const iggy::runtime::RuntimeGameplaySnapshot before = snapshot;

	const iggy::runtime::RuntimeGameplaySnapshotSaveResult save =
		iggy::runtime::RuntimeGameplaySnapshotSaver {}.save(path, snapshot);
	const iggy::runtime::RuntimeGameplaySnapshotLoadResult load =
		iggy::runtime::RuntimeGameplaySnapshotLoader {}.load(path);

	Expect(save.status == iggy::runtime::RuntimeGameplaySnapshotSaveStatus::Saved, "immutability fixture should save");
	Expect(load.status == iggy::runtime::RuntimeGameplaySnapshotLoadStatus::Loaded, "immutability fixture should load");
	Expect(SameQueue(snapshot.commandQueue, before.commandQueue), "save/load should not mutate source command queue");
	Expect(SameActors(snapshot.npcActors, before.npcActors), "save/load should not mutate source NPC actors");
	Expect(SameControls(snapshot.npcControls, before.npcControls), "save/load should not mutate source NPC controls");
}

void TestCaptureSaveLoadAcceptance()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("captured.gplay");
	iggy::runtime::RuntimeGameplayState state;
	state.session.level = Level({
		"........",
		"........",
		"........",
	});
	state.session.tickIndex = 19;
	state.session.hasPlayer = true;
	state.session.player = PlayerAgent(Id("player:one"), { 1.5F, 1.5F }, { 1, 1 });
	state.commandQueue = ExplicitSnapshot().commandQueue;
	state.interaction = ExplicitSnapshot().interaction;
	state.inventory = ExplicitSnapshot().inventory;
	state.npcActors = ExplicitSnapshot().npcActors;
	state.npcControls = ExplicitSnapshot().npcControls;
	const iggy::runtime::RuntimeGameplaySnapshot snapshot =
		iggy::runtime::RuntimeGameplaySnapshotBuilder {}.capture(state);

	const iggy::runtime::RuntimeGameplaySnapshotSaveResult save =
		iggy::runtime::RuntimeGameplaySnapshotSaver {}.save(path, snapshot);
	const iggy::runtime::RuntimeGameplaySnapshotLoadResult load =
		iggy::runtime::RuntimeGameplaySnapshotLoader {}.load(path, RestoreConfigNoCaches());

	Expect(save.status == iggy::runtime::RuntimeGameplaySnapshotSaveStatus::Saved, "captured gameplay state should save as gameplay snapshot");
	Expect(load.status == iggy::runtime::RuntimeGameplaySnapshotLoadStatus::Loaded, "captured gameplay snapshot should load");
	ExpectStateMatchesSnapshot(load.state, snapshot, "captured gameplay snapshot should restore gameplay-owned children");
}

} // namespace

int main()
{
	TestSaveDefaultSnapshotWritesFileAndLoads();
	TestExplicitSnapshotRoundTripsAllGameplayChildren();
	TestSaveInvalidSnapshotDoesNotOverwriteFile();
	TestLoadMissingFileFails();
	TestLoadInvalidEnvelopeFails();
	TestLoadInvalidArchiveFails();
	TestLoadInvalidGameplayPayloadFailsSnapshotDecode();
	TestLoadDecodedSnapshotValidationFailureMapsToSnapshotInvalid();
	TestRestoreFailureMapsToRestoreFailed();
	TestSaveLoadDoNotMutateInputs();
	TestCaptureSaveLoadAcceptance();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
