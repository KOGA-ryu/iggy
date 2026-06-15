#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeGameplaySaveSlotStore.hpp"
#include "runtime/RuntimeSaveFileIO.hpp"
#include "runtime/RuntimeSaveSlotStore.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

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
	return std::filesystem::temp_directory_path() / "iggy_runtime_gameplay_save_slot_store_tests";
}

void ResetTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
	std::filesystem::create_directories(TempRoot(), ignored);
}

const char *DirectoryName(iggy::runtime::RuntimeSaveSlotKind kind)
{
	switch (kind) {
	case iggy::runtime::RuntimeSaveSlotKind::Manual:
		return "manual";
	case iggy::runtime::RuntimeSaveSlotKind::Auto:
		return "auto";
	case iggy::runtime::RuntimeSaveSlotKind::Quick:
		return "quick";
	}
	return "manual";
}

void CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind kind)
{
	std::filesystem::create_directories(TempRoot() / DirectoryName(kind));
}

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId(value);
}

iggy::runtime::RuntimeSaveSlotId Slot(
	iggy::runtime::RuntimeSaveSlotKind kind,
	std::string name)
{
	return { kind, std::move(name) };
}

iggy::runtime::RuntimeGameplaySaveSlotStoreConfig Config(std::filesystem::path base = TempRoot())
{
	iggy::runtime::RuntimeGameplaySaveSlotStoreConfig config;
	config.baseDirectory = std::move(base);
	config.restore.session.buildConfig.buildRenderCache = false;
	return config;
}

iggy::LevelRuntimeState Level(std::vector<std::string_view> rows)
{
	iggy::LevelRuntimeState level;
	level.map = MapFromRows(rows);
	level.map.id = Id("level:gameplay-slot-store");
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
	snapshot.tickIndex = 31;
	snapshot.hasPlayer = hasPlayer;
	if (hasPlayer)
		snapshot.player = PlayerAgent(Id("player:slot"), { 1.5F, 1.5F }, { 1, 1 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::West);
	return snapshot;
}

iggy::runtime::GameplayCommand2D Command()
{
	iggy::runtime::GameplayCommand2D command;
	command.type = iggy::runtime::GameplayCommand2DType::Interact;
	command.actorId = Id("player:slot");
	command.targetId = Id("target:door");
	command.targetPoint = { 2.5F, 1.5F };
	command.targetTile = { 2, 1 };
	return command;
}

iggy::NpcActorState2D Actor(const char *id, iggy::Vec2 position, bool present = true)
{
	return { Id(id), Id("ai-profile:guard"), Id("faction:town"), position, Id("goal:watch"), present };
}

iggy::NpcActorControlState2D Control(const char *id)
{
	iggy::NpcActorControlState2D control;
	control.npcId = Id(id);
	control.objective = iggy::moveToNpcObjective({ 4.5F, 1.5F });
	control.behavior = iggy::seekingNpcBehaviorState({ 4.5F, 1.5F });
	control.moveMode = iggy::NpcMoveMode::Walk;
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
	snapshot.commandQueue.frames = { { { Command() } } };
	snapshot.interaction.targets = iggy::InteractionTarget2DRegistry({
		{ Id("target:door"), iggy::InteractionTarget2DKind::Door, { 2.5F, 1.5F }, 0.75F, true },
	});
	snapshot.interaction.effects = iggy::InteractionEffectCatalog2D({
		{ Id("target:door"), { iggy::toggleTargetInteractionEffect(Id("target:door"), false) } },
	});
	snapshot.inventory.inventory.stacks = {
		{ Id("item:key"), 1 },
	};
	snapshot.inventory.drops.drops = {
		{ Id("drop:key"), Id("item:key"), 1, { 3.5F, 1.5F }, 0.35F, true },
	};
	snapshot.npcActors.actors = {
		Actor("npc:one", { 0.5F, 1.5F }),
		Actor("unqualified", { 1.5F, 1.5F }, false),
	};
	snapshot.npcControls.entries = {
		Control("npc:one"),
		Control("unqualified"),
	};
	return snapshot;
}

iggy::runtime::RuntimeGameplayState GameplayState()
{
	iggy::runtime::RuntimeGameplayState state;
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = ExplicitSnapshot();
	state.session.level = snapshot.session.level;
	state.session.tickIndex = snapshot.session.tickIndex;
	state.session.hasPlayer = snapshot.session.hasPlayer;
	state.session.player = snapshot.session.player;
	state.commandQueue = snapshot.commandQueue;
	state.interaction = snapshot.interaction;
	state.inventory = snapshot.inventory;
	state.npcActors = snapshot.npcActors;
	state.npcControls = snapshot.npcControls;
	return state;
}

bool SameSession(const iggy::runtime::RuntimeSessionState &state, const iggy::runtime::RuntimeSessionSnapshot &snapshot)
{
	return state.level.map.id == snapshot.level.map.id
		&& state.level.map.width == snapshot.level.map.width
		&& state.level.map.height == snapshot.level.map.height
		&& state.tickIndex == snapshot.tickIndex
		&& state.hasPlayer == snapshot.hasPlayer
		&& (!state.hasPlayer
			|| (state.player.id == snapshot.player.id
				&& NearVec(state.player.position, snapshot.player.position)
				&& state.player.spawnTile == snapshot.player.spawnTile));
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
			|| actual.entries[index].behavior.type != expected.entries[index].behavior.type
			|| actual.entries[index].moveMode != expected.entries[index].moveMode)
			return false;
	}
	return true;
}

void ExpectLoadedState(
	const iggy::runtime::RuntimeGameplayState &state,
	const iggy::runtime::RuntimeGameplaySnapshot &snapshot,
	const char *message)
{
	Expect(SameSession(state.session, snapshot.session), message);
	Expect(state.commandQueue.frames.size() == snapshot.commandQueue.frames.size(), message);
	Expect(state.interaction.targets.targets().size() == snapshot.interaction.targets.targets().size(), message);
	Expect(state.inventory.inventory.stacks.size() == snapshot.inventory.inventory.stacks.size(), message);
	Expect(SameActors(state.npcActors, snapshot.npcActors), message);
	Expect(SameControls(state.npcControls, snapshot.npcControls), message);
}

void WriteBytes(const std::filesystem::path &path, const std::vector<std::uint8_t> &bytes)
{
	const iggy::runtime::RuntimeSaveFileWriteResult write = iggy::runtime::RuntimeSaveFileWriter {}.writeBytes(path, bytes);
	Expect(write.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "fixture should write bytes");
}

std::vector<std::uint8_t> ReadBytes(const std::filesystem::path &path)
{
	const iggy::runtime::RuntimeSaveFileReadResult read = iggy::runtime::RuntimeSaveFileReader {}.readBytes(path);
	Expect(read.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "fixture should read bytes");
	return read.bytes;
}

void TestSaveDefaultSnapshotToSlot()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = DefaultSnapshot();

	const iggy::runtime::RuntimeGameplaySaveSlotSaveResult save = iggy::runtime::RuntimeGameplaySaveSlotStore {}.save(
		snapshot,
		Config(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1"));

	Expect(save.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::Saved, "default gameplay slot save should succeed");
	Expect(save.save.status == iggy::runtime::RuntimeGameplaySnapshotSaveStatus::Saved, "default gameplay slot save should preserve nested save status");
	Expect(save.path.path == TempRoot() / "manual" / "slot1.iggygameplay", "default gameplay slot save should use gameplay extension");
	Expect(std::filesystem::exists(save.path.path), "default gameplay slot save should write expected file");
}

void TestLoadSavedDefaultSlot()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = DefaultSnapshot();
	const iggy::runtime::RuntimeSaveSlotId slot = Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1");
	Expect(iggy::runtime::RuntimeGameplaySaveSlotStore {}.save(snapshot, Config(), slot).status == iggy::runtime::RuntimeGameplaySaveSlotStatus::Saved, "default gameplay slot load setup should save");

	const iggy::runtime::RuntimeGameplaySaveSlotLoadResult load =
		iggy::runtime::RuntimeGameplaySaveSlotStore {}.load(Config(), slot);

	Expect(load.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::Loaded, "default gameplay slot load should succeed");
	Expect(load.load.status == iggy::runtime::RuntimeGameplaySnapshotLoadStatus::Loaded, "default gameplay slot load should preserve nested load status");
	ExpectLoadedState(load.state, snapshot, "default gameplay slot load should restore expected gameplay state");
}

void TestExplicitSnapshotRoundTripsThroughSlot()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Auto);
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = ExplicitSnapshot();
	const iggy::runtime::RuntimeSaveSlotId slot = Slot(iggy::runtime::RuntimeSaveSlotKind::Auto, "auto_1");

	const iggy::runtime::RuntimeGameplaySaveSlotSaveResult save =
		iggy::runtime::RuntimeGameplaySaveSlotStore {}.save(snapshot, Config(), slot);
	const iggy::runtime::RuntimeGameplaySaveSlotLoadResult load =
		iggy::runtime::RuntimeGameplaySaveSlotStore {}.load(Config(), slot);

	Expect(save.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::Saved, "explicit gameplay slot save should succeed");
	Expect(save.path.path == TempRoot() / "auto" / "auto_1.iggygameplay", "explicit gameplay auto slot should use auto directory");
	Expect(load.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::Loaded, "explicit gameplay slot load should succeed");
	ExpectLoadedState(load.state, snapshot, "explicit gameplay slot load should round-trip gameplay children");
}

void TestSaveStateCapturesAndRoundTrips()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Quick);
	const iggy::runtime::RuntimeGameplayState state = GameplayState();
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = iggy::runtime::RuntimeGameplaySnapshotBuilder {}.capture(state);
	const iggy::runtime::RuntimeSaveSlotId slot = Slot(iggy::runtime::RuntimeSaveSlotKind::Quick, "quick-1");

	const iggy::runtime::RuntimeGameplaySaveSlotSaveResult save =
		iggy::runtime::RuntimeGameplaySaveSlotStore {}.saveState(state, Config(), slot);
	const iggy::runtime::RuntimeGameplaySaveSlotLoadResult load =
		iggy::runtime::RuntimeGameplaySaveSlotStore {}.load(Config(), slot);

	Expect(save.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::Saved, "saveState should capture and save gameplay state");
	Expect(save.path.path == TempRoot() / "quick" / "quick-1.iggygameplay", "saveState should preserve slot directory policy");
	Expect(load.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::Loaded, "saveState slot should load");
	ExpectLoadedState(load.state, snapshot, "saveState slot should restore captured gameplay state");
}

void TestInvalidSlotNamesRejected()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = ExplicitSnapshot();

	const iggy::runtime::RuntimeGameplaySaveSlotSaveResult empty = iggy::runtime::RuntimeGameplaySaveSlotStore {}.save(
		snapshot,
		Config(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, ""));
	const iggy::runtime::RuntimeGameplaySaveSlotSaveResult traversal = iggy::runtime::RuntimeGameplaySaveSlotStore {}.save(
		snapshot,
		Config(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "../bad"));

	Expect(empty.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::InvalidSlotPath, "empty gameplay slot name should be rejected");
	Expect(empty.path.status == iggy::runtime::RuntimeSaveSlotPathStatus::EmptySlotName, "empty gameplay slot name should preserve path diagnostic");
	Expect(traversal.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::InvalidSlotPath, "path traversal gameplay slot name should be rejected");
	Expect(traversal.path.status == iggy::runtime::RuntimeSaveSlotPathStatus::InvalidSlotName, "path traversal slot should preserve path diagnostic");
	Expect(std::filesystem::is_empty(TempRoot() / "manual"), "invalid gameplay slot names should not write files");
}

void TestEmptyBaseDirectoryRejected()
{
	const iggy::runtime::RuntimeGameplaySaveSlotSaveResult save =
		iggy::runtime::RuntimeGameplaySaveSlotStore {}.save(ExplicitSnapshot(), Config({}), Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1"));

	Expect(save.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::InvalidSlotPath, "empty gameplay slot base directory should be rejected");
	Expect(save.path.status == iggy::runtime::RuntimeSaveSlotPathStatus::EmptyBaseDirectory, "empty gameplay slot base should preserve path diagnostic");
}

void TestInvalidSnapshotSaveFailureDoesNotOverwrite()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	const iggy::runtime::RuntimeSaveSlotId slot = Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1");
	Expect(iggy::runtime::RuntimeGameplaySaveSlotStore {}.save(DefaultSnapshot(), Config(), slot).status == iggy::runtime::RuntimeGameplaySaveSlotStatus::Saved, "invalid snapshot overwrite setup should save");
	const std::filesystem::path path = TempRoot() / "manual" / "slot1.iggygameplay";
	const std::vector<std::uint8_t> before = ReadBytes(path);
	iggy::runtime::RuntimeGameplaySnapshot invalid = ExplicitSnapshot();
	invalid.npcActors.actors[0].position = { 99.5F, 99.5F };

	const iggy::runtime::RuntimeGameplaySaveSlotSaveResult save =
		iggy::runtime::RuntimeGameplaySaveSlotStore {}.save(invalid, Config(), slot);

	Expect(save.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::SaveFailed, "invalid gameplay snapshot should map to slot SaveFailed");
	Expect(save.save.status == iggy::runtime::RuntimeGameplaySnapshotSaveStatus::SnapshotInvalid, "invalid gameplay snapshot slot save should preserve nested status");
	Expect(ReadBytes(path) == before, "invalid gameplay snapshot slot save should not overwrite existing slot file");
}

void TestMissingSlotLoadFailure()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);

	const iggy::runtime::RuntimeGameplaySaveSlotLoadResult load =
		iggy::runtime::RuntimeGameplaySaveSlotStore {}.load(Config(), Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "missing"));

	Expect(load.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::LoadFailed, "missing gameplay slot should map to LoadFailed");
	Expect(load.load.status == iggy::runtime::RuntimeGameplaySnapshotLoadStatus::FileReadFailed, "missing gameplay slot should preserve nested file read failure");
}

void TestCorruptSlotLoadFailure()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	const std::filesystem::path path = TempRoot() / "manual" / "corrupt.iggygameplay";
	WriteBytes(path, { 1, 2, 3, 4 });

	const iggy::runtime::RuntimeGameplaySaveSlotLoadResult load =
		iggy::runtime::RuntimeGameplaySaveSlotStore {}.load(Config(), Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "corrupt"));

	Expect(load.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::LoadFailed, "corrupt gameplay slot should map to LoadFailed");
	Expect(load.load.status == iggy::runtime::RuntimeGameplaySnapshotLoadStatus::EnvelopeDecodeFailed, "corrupt gameplay slot should preserve nested decode diagnostics");
}

void TestCustomExtensionIsHonored()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	iggy::runtime::RuntimeGameplaySaveSlotStoreConfig config = Config();
	config.extension = ".customgame";

	const iggy::runtime::RuntimeGameplaySaveSlotSaveResult save =
		iggy::runtime::RuntimeGameplaySaveSlotStore {}.save(DefaultSnapshot(), config, Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1"));

	Expect(save.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::Saved, "custom gameplay slot extension should save");
	Expect(save.path.path == TempRoot() / "manual" / "slot1.customgame", "custom gameplay slot extension should be honored");
}

void TestInputSnapshotImmutability()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = ExplicitSnapshot();
	const iggy::runtime::RuntimeGameplaySnapshot before = snapshot;

	const iggy::runtime::RuntimeGameplaySaveSlotSaveResult save =
		iggy::runtime::RuntimeGameplaySaveSlotStore {}.save(snapshot, Config(), Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1"));

	Expect(save.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::Saved, "immutability fixture should save");
	Expect(SameActors(snapshot.npcActors, before.npcActors), "gameplay slot save should not mutate snapshot NPC actors");
	Expect(SameControls(snapshot.npcControls, before.npcControls), "gameplay slot save should not mutate snapshot NPC controls");
	Expect(snapshot.commandQueue.frames.size() == before.commandQueue.frames.size(), "gameplay slot save should not mutate command queue");
}

} // namespace

int main()
{
	TestSaveDefaultSnapshotToSlot();
	TestLoadSavedDefaultSlot();
	TestExplicitSnapshotRoundTripsThroughSlot();
	TestSaveStateCapturesAndRoundTrips();
	TestInvalidSlotNamesRejected();
	TestEmptyBaseDirectoryRejected();
	TestInvalidSnapshotSaveFailureDoesNotOverwrite();
	TestMissingSlotLoadFailure();
	TestCorruptSlotLoadFailure();
	TestCustomExtensionIsHonored();
	TestInputSnapshotImmutability();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
