#include <cstdlib>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeBinaryCodec.hpp"
#include "runtime/RuntimeGameplaySnapshot.hpp"
#include "runtime/RuntimeGameplaySnapshotChunkCodec.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::runtime::RuntimeSaveChunkId SessionChunkId = iggy::runtime::makeRuntimeSaveChunkId('S', 'E', 'S', 'S');
const iggy::runtime::RuntimeSaveChunkId LevelMapChunkId = iggy::runtime::makeRuntimeSaveChunkId('L', 'M', 'A', 'P');
const iggy::runtime::RuntimeSaveChunkId PlayerChunkId = iggy::runtime::makeRuntimeSaveChunkId('P', 'L', 'Y', 'R');
const iggy::runtime::RuntimeSaveChunkId NpcsChunkId = iggy::runtime::makeRuntimeSaveChunkId('N', 'P', 'C', 'S');
const iggy::runtime::RuntimeSaveChunkId CommandQueueChunkId = iggy::runtime::makeRuntimeSaveChunkId('G', 'Q', 'U', 'E');
const iggy::runtime::RuntimeSaveChunkId InteractionChunkId = iggy::runtime::makeRuntimeSaveChunkId('G', 'I', 'N', 'T');
const iggy::runtime::RuntimeSaveChunkId InventoryChunkId = iggy::runtime::makeRuntimeSaveChunkId('G', 'I', 'N', 'V');
const iggy::runtime::RuntimeSaveChunkId NpcActorsChunkId = iggy::runtime::makeRuntimeSaveChunkId('G', 'A', 'C', 'T');
const iggy::runtime::RuntimeSaveChunkId NpcControlsChunkId = iggy::runtime::makeRuntimeSaveChunkId('G', 'C', 'T', 'L');
const iggy::runtime::RuntimeSaveChunkId UnknownChunkId = iggy::runtime::makeRuntimeSaveChunkId('U', 'N', 'K', 'N');

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

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId(value);
}

void WriteString(iggy::runtime::RuntimeBinaryWriter &writer, std::string_view value)
{
	writer.writeU32LE(static_cast<std::uint32_t>(value.size()));
	writer.writeBytes(std::vector<std::uint8_t>(value.begin(), value.end()));
}

void WriteResourceId(iggy::runtime::RuntimeBinaryWriter &writer, const char *value)
{
	WriteString(writer, value);
}

void WriteVec2(iggy::runtime::RuntimeBinaryWriter &writer, iggy::Vec2 value)
{
	writer.writeF32LE(value.x);
	writer.writeF32LE(value.y);
}

void WriteValidObjective(iggy::runtime::RuntimeBinaryWriter &writer)
{
	writer.writeU32LE(1);
	WriteResourceId(writer, "");
	WriteVec2(writer, {});
}

void WriteValidBehavior(iggy::runtime::RuntimeBinaryWriter &writer)
{
	writer.writeU32LE(1);
	WriteResourceId(writer, "");
	WriteVec2(writer, {});
}

iggy::LevelRuntimeState Level(std::vector<std::string_view> rows)
{
	iggy::LevelRuntimeState level;
	level.map = MapFromRows(rows);
	level.map.id = Id("level:gameplay-codec");
	level.map.playerStart = { 1, 1 };
	return level;
}

iggy::runtime::RuntimeSessionSnapshot SessionSnapshot(bool hasPlayer = true)
{
	iggy::runtime::RuntimeSessionSnapshot snapshot;
	snapshot.level = Level({
		"........",
		"........",
		"........",
	});
	snapshot.tickIndex = 77;
	snapshot.hasPlayer = hasPlayer;
	if (hasPlayer)
		snapshot.player = PlayerAgent(Id("player:one"), { 1.5F, 1.5F }, { 1, 1 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::South);
	return snapshot;
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

iggy::runtime::RuntimeGameplaySnapshot EmptyValidSnapshot()
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
			Command(iggy::runtime::GameplayCommand2DType::None, "actor:none"),
			Command(iggy::runtime::GameplayCommand2DType::MoveToPoint, "actor:point"),
			Command(iggy::runtime::GameplayCommand2DType::MoveToTile, "actor:tile"),
		} },
		{ {
			Command(iggy::runtime::GameplayCommand2DType::Interact, "actor:interact"),
			Command(iggy::runtime::GameplayCommand2DType::Wait, "actor:wait"),
		} },
	};
	snapshot.interaction.targets = iggy::InteractionTarget2DRegistry({
		{ Id("target:unknown"), iggy::InteractionTarget2DKind::Unknown, { 0.5F, 0.5F }, 0.25F, true },
		{ Id("target:inspect"), iggy::InteractionTarget2DKind::Inspectable, { 1.5F, 0.5F }, 0.5F, true },
		{ Id("target:use"), iggy::InteractionTarget2DKind::Usable, { 2.5F, 0.5F }, 0.75F, false },
		{ Id("target:pickup"), iggy::InteractionTarget2DKind::Pickup, { 3.5F, 0.5F }, 1.0F, true },
		{ Id("target:talk"), iggy::InteractionTarget2DKind::Talk, { 4.5F, 0.5F }, 1.25F, true },
		{ Id("target:door"), iggy::InteractionTarget2DKind::Door, { 5.5F, 0.5F }, 1.5F, false },
	});
	snapshot.interaction.effects = iggy::InteractionEffectCatalog2D({
		{
			Id("target:inspect"),
			{
				iggy::InteractionEffect2D {},
				iggy::inspectTextInteractionEffect(Id("target:inspect"), "A labeled switch."),
				iggy::toggleTargetInteractionEffect(Id("target:door"), false),
				iggy::emitInteractionEventEffect(Id("target:use"), Id("event:alarm")),
				iggy::pickupItemInteractionEffect(Id("target:pickup"), Id("drop:potion")),
			},
		},
	});
	snapshot.inventory.inventory.stacks = {
		{ Id("item:potion"), 3 },
		{ Id("item:key"), 1 },
	};
	snapshot.inventory.drops.drops = {
		{ Id("drop:potion"), Id("item:potion"), 2, { 2.5F, 1.5F }, 0.35F, true },
		{ Id("drop:key"), Id("item:key"), 1, { 3.5F, 1.5F }, 0.45F, false },
	};
	snapshot.npcActors.actors = {
		Actor("npc:none", { 0.5F, 1.5F }),
		Actor("npc:still", { 1.5F, 1.5F }),
		Actor("npc:walk", { 2.5F, 1.5F }),
		Actor("npc:jog", { 3.5F, 1.5F }),
		Actor("npc:run", { 4.5F, 1.5F }, false),
		Actor("npc:sprint", { 5.5F, 1.5F }),
		Actor("unqualified", { 6.5F, 1.5F }),
	};
	snapshot.npcControls.entries = {
		Control("npc:none", iggy::noneNpcObjective(), iggy::noneNpcBehaviorState(), iggy::NpcMoveMode::None),
		Control("npc:still", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still),
		Control("npc:walk", iggy::moveToNpcObjective({ 6.5F, 1.5F }), iggy::seekingNpcBehaviorState({ 6.5F, 1.5F }), iggy::NpcMoveMode::Walk),
		Control("npc:jog", iggy::fleeNpcObjective({ 2.5F, 1.5F }), iggy::fleeingNpcBehaviorState({ 2.5F, 1.5F }), iggy::NpcMoveMode::Jog),
		Control("npc:run", iggy::attackNpcObjective(Id("target:enemy")), iggy::attackingNpcBehaviorState(Id("target:enemy")), iggy::NpcMoveMode::Run),
		Control("npc:sprint", iggy::interactNpcObjective(Id("target:door")), iggy::interactingNpcBehaviorState(Id("target:door")), iggy::NpcMoveMode::Sprint),
		Control("unqualified", iggy::guardNpcObjective(), iggy::waitingNpcBehaviorState(), iggy::NpcMoveMode::Walk),
	};
	return snapshot;
}

bool SameMap(const iggy::LevelTileMap &actual, const iggy::LevelTileMap &expected)
{
	if (actual.id != expected.id || actual.width != expected.width || actual.height != expected.height || actual.tiles.size() != expected.tiles.size())
		return false;
	for (std::size_t index = 0; index < actual.tiles.size(); ++index) {
		if (actual.tiles[index].walkable != expected.tiles[index].walkable)
			return false;
	}
	return true;
}

bool SameSession(const iggy::runtime::RuntimeSessionSnapshot &actual, const iggy::runtime::RuntimeSessionSnapshot &expected)
{
	return actual.tickIndex == expected.tickIndex
		&& actual.hasPlayer == expected.hasPlayer
		&& SameMap(actual.level.map, expected.level.map)
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
	if (actual.targets.targets().size() != expected.targets.targets().size() || actual.effects.entries().size() != expected.effects.entries().size())
		return false;
	for (std::size_t index = 0; index < actual.targets.targets().size(); ++index) {
		const iggy::InteractionTarget2D &left = actual.targets.targets()[index];
		const iggy::InteractionTarget2D &right = expected.targets.targets()[index];
		if (left.id != right.id || left.kind != right.kind || !NearVec(left.position, right.position) || left.radius != right.radius || left.enabled != right.enabled)
			return false;
	}
	for (std::size_t entryIndex = 0; entryIndex < actual.effects.entries().size(); ++entryIndex) {
		const iggy::InteractionEffectEntry2D &left = actual.effects.entries()[entryIndex];
		const iggy::InteractionEffectEntry2D &right = expected.effects.entries()[entryIndex];
		if (left.targetId != right.targetId || left.effects.size() != right.effects.size())
			return false;
		for (std::size_t effectIndex = 0; effectIndex < left.effects.size(); ++effectIndex) {
			const iggy::InteractionEffect2D &leftEffect = left.effects[effectIndex];
			const iggy::InteractionEffect2D &rightEffect = right.effects[effectIndex];
			if (leftEffect.type != rightEffect.type
				|| leftEffect.targetId != rightEffect.targetId
				|| leftEffect.eventId != rightEffect.eventId
				|| leftEffect.dropId != rightEffect.dropId
				|| leftEffect.text != rightEffect.text
				|| leftEffect.enabledValue != rightEffect.enabledValue)
				return false;
		}
	}
	return true;
}

bool SameInventory(const iggy::runtime::RuntimeInventoryState &actual, const iggy::runtime::RuntimeInventoryState &expected)
{
	if (actual.inventory.stacks.size() != expected.inventory.stacks.size() || actual.drops.drops.size() != expected.drops.drops.size())
		return false;
	for (std::size_t index = 0; index < actual.inventory.stacks.size(); ++index) {
		if (actual.inventory.stacks[index].itemId != expected.inventory.stacks[index].itemId
			|| actual.inventory.stacks[index].count != expected.inventory.stacks[index].count)
			return false;
	}
	for (std::size_t index = 0; index < actual.drops.drops.size(); ++index) {
		const iggy::LevelItemDrop2D &left = actual.drops.drops[index];
		const iggy::LevelItemDrop2D &right = expected.drops.drops[index];
		if (left.id != right.id || left.itemId != right.itemId || left.count != right.count || !NearVec(left.position, right.position) || left.pickupRadius != right.pickupRadius || left.enabled != right.enabled)
			return false;
	}
	return true;
}

bool SameActors(const iggy::NpcActorState2DRegistry &actual, const iggy::NpcActorState2DRegistry &expected)
{
	if (actual.actors.size() != expected.actors.size())
		return false;
	for (std::size_t index = 0; index < actual.actors.size(); ++index) {
		const iggy::NpcActorState2D &left = actual.actors[index];
		const iggy::NpcActorState2D &right = expected.actors[index];
		if (left.npcId != right.npcId
			|| left.aiProfileId != right.aiProfileId
			|| left.factionId != right.factionId
			|| !NearVec(left.position, right.position)
			|| left.currentGoalId != right.currentGoalId
			|| left.present != right.present)
			return false;
	}
	return true;
}

bool SameControls(const iggy::NpcActorControlState2DRegistry &actual, const iggy::NpcActorControlState2DRegistry &expected)
{
	if (actual.entries.size() != expected.entries.size())
		return false;
	for (std::size_t index = 0; index < actual.entries.size(); ++index) {
		const iggy::NpcActorControlState2D &left = actual.entries[index];
		const iggy::NpcActorControlState2D &right = expected.entries[index];
		if (left.npcId != right.npcId
			|| left.objective.type != right.objective.type
			|| left.objective.targetId != right.objective.targetId
			|| !NearVec(left.objective.targetPosition, right.objective.targetPosition)
			|| left.behavior.type != right.behavior.type
			|| left.behavior.targetId != right.behavior.targetId
			|| !NearVec(left.behavior.targetPosition, right.behavior.targetPosition)
			|| left.moveMode != right.moveMode)
			return false;
	}
	return true;
}

bool SameSnapshot(const iggy::runtime::RuntimeGameplaySnapshot &actual, const iggy::runtime::RuntimeGameplaySnapshot &expected)
{
	return SameSession(actual.session, expected.session)
		&& SameQueue(actual.commandQueue, expected.commandQueue)
		&& SameInteraction(actual.interaction, expected.interaction)
		&& SameInventory(actual.inventory, expected.inventory)
		&& SameActors(actual.npcActors, expected.npcActors)
		&& SameControls(actual.npcControls, expected.npcControls);
}

bool HasIssue(
	const iggy::runtime::RuntimeGameplaySnapshotDecodeResult &result,
	iggy::runtime::RuntimeGameplaySnapshotChunkIssueCode code,
	iggy::runtime::RuntimeSaveChunkId chunkId = {},
	std::size_t chunkIndex = 0)
{
	for (const iggy::runtime::RuntimeGameplaySnapshotChunkIssue &issue : result.issues) {
		if (issue.code == code && issue.chunkId == chunkId && issue.chunkIndex == chunkIndex)
			return true;
	}
	return false;
}

bool HasIssueForChunk(
	const iggy::runtime::RuntimeGameplaySnapshotDecodeResult &result,
	iggy::runtime::RuntimeGameplaySnapshotChunkIssueCode code,
	iggy::runtime::RuntimeSaveChunkId chunkId)
{
	for (const iggy::runtime::RuntimeGameplaySnapshotChunkIssue &issue : result.issues) {
		if (issue.code == code && issue.chunkId == chunkId)
			return true;
	}
	return false;
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

void RemoveChunk(iggy::runtime::RuntimeSaveChunkArchive &archive, iggy::runtime::RuntimeSaveChunkId id)
{
	for (auto it = archive.chunks.begin(); it != archive.chunks.end(); ++it) {
		if (it->id == id) {
			archive.chunks.erase(it);
			return;
		}
	}
}

void ExpectChunkOrder(
	const iggy::runtime::RuntimeSaveChunkArchive &archive,
	const std::vector<iggy::runtime::RuntimeSaveChunkId> &ids)
{
	Expect(archive.chunks.size() == ids.size(), "gameplay archive should have expected chunk count");
	if (archive.chunks.size() != ids.size())
		return;
	for (std::size_t index = 0; index < ids.size(); ++index)
		Expect(archive.chunks[index].id == ids[index], "gameplay archive should preserve session-first then gameplay chunk order");
}

std::vector<std::uint8_t> InvalidCommandTypePayload()
{
	iggy::runtime::RuntimeBinaryWriter writer;
	writer.writeU32LE(1);
	writer.writeU32LE(1);
	writer.writeU32LE(99);
	return writer.takeBytes();
}

std::vector<std::uint8_t> InvalidInteractionTargetKindPayload()
{
	iggy::runtime::RuntimeBinaryWriter writer;
	writer.writeU32LE(1);
	WriteResourceId(writer, "target:bad");
	writer.writeU32LE(99);
	return writer.takeBytes();
}

std::vector<std::uint8_t> InvalidInteractionEffectTypePayload()
{
	iggy::runtime::RuntimeBinaryWriter writer;
	writer.writeU32LE(0);
	writer.writeU32LE(1);
	WriteResourceId(writer, "target:bad");
	writer.writeU32LE(1);
	writer.writeU32LE(99);
	return writer.takeBytes();
}

std::vector<std::uint8_t> InvalidObjectiveTypePayload()
{
	iggy::runtime::RuntimeBinaryWriter writer;
	writer.writeU32LE(1);
	WriteResourceId(writer, "npc:bad");
	writer.writeU32LE(99);
	return writer.takeBytes();
}

std::vector<std::uint8_t> InvalidBehaviorTypePayload()
{
	iggy::runtime::RuntimeBinaryWriter writer;
	writer.writeU32LE(1);
	WriteResourceId(writer, "npc:bad");
	WriteValidObjective(writer);
	writer.writeU32LE(99);
	return writer.takeBytes();
}

std::vector<std::uint8_t> InvalidMoveModePayload()
{
	iggy::runtime::RuntimeBinaryWriter writer;
	writer.writeU32LE(1);
	WriteResourceId(writer, "npc:bad");
	WriteValidObjective(writer);
	WriteValidBehavior(writer);
	writer.writeU32LE(99);
	return writer.takeBytes();
}

iggy::runtime::RuntimeSessionBuildConfig BuildConfigNoCaches()
{
	iggy::runtime::RuntimeSessionBuildConfig config;
	config.buildRenderCache = false;
	return config;
}

void TestEmptyGameplaySnapshotRoundTrips()
{
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = EmptyValidSnapshot();
	const iggy::runtime::RuntimeGameplaySnapshotEncodeResult encoded =
		iggy::runtime::RuntimeGameplaySnapshotChunkEncoder {}.encode(snapshot);

	ExpectChunkOrder(encoded.archive, { SessionChunkId, LevelMapChunkId, NpcsChunkId, CommandQueueChunkId, InteractionChunkId, InventoryChunkId, NpcActorsChunkId, NpcControlsChunkId });

	const iggy::runtime::RuntimeGameplaySnapshotDecodeResult decoded =
		iggy::runtime::RuntimeGameplaySnapshotChunkDecoder {}.decode(encoded.archive);

	Expect(decoded.decoded, "empty valid gameplay snapshot should decode");
	Expect(decoded.issues.empty(), "empty valid gameplay snapshot should have no decode issues");
	Expect(SameSnapshot(decoded.snapshot, snapshot), "empty valid gameplay snapshot should round trip");
}

void TestExplicitGameplaySnapshotRoundTripsByValue()
{
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = ExplicitSnapshot();
	const iggy::runtime::RuntimeGameplaySnapshotEncodeResult encoded =
		iggy::runtime::RuntimeGameplaySnapshotChunkEncoder {}.encode(snapshot);

	ExpectChunkOrder(encoded.archive, { SessionChunkId, LevelMapChunkId, PlayerChunkId, NpcsChunkId, CommandQueueChunkId, InteractionChunkId, InventoryChunkId, NpcActorsChunkId, NpcControlsChunkId });

	const iggy::runtime::RuntimeGameplaySnapshotDecodeResult decoded =
		iggy::runtime::RuntimeGameplaySnapshotChunkDecoder {}.decode(encoded.archive);

	Expect(decoded.decoded, "explicit gameplay snapshot should decode");
	Expect(decoded.session.decoded, "gameplay decode should preserve nested session decode result");
	Expect(decoded.validation.valid, "gameplay decode should preserve gameplay validation result");
	Expect(SameSnapshot(decoded.snapshot, snapshot), "explicit gameplay snapshot should preserve all gameplay children by value");
	Expect(decoded.snapshot.npcActors.actors[6].npcId == Id("unqualified"), "unqualified NPC id should round trip distinctly");
	Expect(decoded.snapshot.npcControls.entries[5].moveMode == iggy::NpcMoveMode::Sprint, "all current move mode values should be representable");
}

void TestMissingGameplayChunkFailsDecode()
{
	iggy::runtime::RuntimeSaveChunkArchive archive =
		iggy::runtime::RuntimeGameplaySnapshotChunkEncoder {}.encode(ExplicitSnapshot()).archive;
	RemoveChunk(archive, NpcControlsChunkId);

	const iggy::runtime::RuntimeGameplaySnapshotDecodeResult result =
		iggy::runtime::RuntimeGameplaySnapshotChunkDecoder {}.decode(archive);

	Expect(!result.decoded, "missing gameplay singleton chunk should fail decode");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplaySnapshotChunkIssueCode::MissingRequiredChunk, NpcControlsChunkId), "missing GCTL should report missing issue");
}

void TestDuplicateGameplayChunkFailsDecode()
{
	iggy::runtime::RuntimeSaveChunkArchive archive =
		iggy::runtime::RuntimeGameplaySnapshotChunkEncoder {}.encode(ExplicitSnapshot()).archive;
	const iggy::runtime::RuntimeSaveChunk *chunk = iggy::runtime::findFirstChunk(archive, CommandQueueChunkId);
	if (chunk != nullptr)
		archive.chunks.push_back(*chunk);

	const iggy::runtime::RuntimeGameplaySnapshotDecodeResult result =
		iggy::runtime::RuntimeGameplaySnapshotChunkDecoder {}.decode(archive);

	Expect(!result.decoded, "duplicate gameplay singleton chunk should fail decode");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplaySnapshotChunkIssueCode::DuplicateSingletonChunk, CommandQueueChunkId, archive.chunks.size() - 1), "duplicate GQUE should report duplicate issue");
}

void TestUnsupportedGameplayChunkVersionFailsDecode()
{
	iggy::runtime::RuntimeSaveChunkArchive archive =
		iggy::runtime::RuntimeGameplaySnapshotChunkEncoder {}.encode(ExplicitSnapshot()).archive;
	iggy::runtime::RuntimeSaveChunk *chunk = FindChunk(archive, InventoryChunkId);
	if (chunk != nullptr)
		chunk->version = 2;

	const iggy::runtime::RuntimeGameplaySnapshotDecodeResult result =
		iggy::runtime::RuntimeGameplaySnapshotChunkDecoder {}.decode(archive);

	Expect(!result.decoded, "unsupported gameplay chunk version should fail decode");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplaySnapshotChunkIssueCode::UnsupportedChunkVersion, InventoryChunkId, 6), "unsupported GINV version should report issue");
}

void TestMalformedGameplayPayloadFailsDecode()
{
	iggy::runtime::RuntimeSaveChunkArchive archive =
		iggy::runtime::RuntimeGameplaySnapshotChunkEncoder {}.encode(ExplicitSnapshot()).archive;
	iggy::runtime::RuntimeSaveChunk *chunk = FindChunk(archive, InventoryChunkId);
	if (chunk != nullptr && !chunk->payload.empty())
		chunk->payload.pop_back();

	const iggy::runtime::RuntimeGameplaySnapshotDecodeResult result =
		iggy::runtime::RuntimeGameplaySnapshotChunkDecoder {}.decode(archive);

	Expect(!result.decoded, "truncated gameplay payload should fail decode");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplaySnapshotChunkIssueCode::MalformedChunkPayload, InventoryChunkId, 6), "truncated GINV should report malformed payload");
}

void ExpectInvalidEnum(
	iggy::runtime::RuntimeSaveChunkId chunkId,
	std::vector<std::uint8_t> payload,
	const char *message)
{
	iggy::runtime::RuntimeSaveChunkArchive archive =
		iggy::runtime::RuntimeGameplaySnapshotChunkEncoder {}.encode(ExplicitSnapshot()).archive;
	iggy::runtime::RuntimeSaveChunk *chunk = FindChunk(archive, chunkId);
	if (chunk != nullptr)
		chunk->payload = std::move(payload);

	const iggy::runtime::RuntimeGameplaySnapshotDecodeResult result =
		iggy::runtime::RuntimeGameplaySnapshotChunkDecoder {}.decode(archive);

	Expect(!result.decoded, message);
	Expect(HasIssueForChunk(result, iggy::runtime::RuntimeGameplaySnapshotChunkIssueCode::InvalidEnumValue, chunkId), message);
}

void TestInvalidGameplayEnumsFailDecode()
{
	ExpectInvalidEnum(CommandQueueChunkId, InvalidCommandTypePayload(), "invalid command type should fail decode");
	ExpectInvalidEnum(InteractionChunkId, InvalidInteractionTargetKindPayload(), "invalid interaction target kind should fail decode");
	ExpectInvalidEnum(InteractionChunkId, InvalidInteractionEffectTypePayload(), "invalid interaction effect type should fail decode");
	ExpectInvalidEnum(NpcControlsChunkId, InvalidObjectiveTypePayload(), "invalid objective type should fail decode");
	ExpectInvalidEnum(NpcControlsChunkId, InvalidBehaviorTypePayload(), "invalid behavior state type should fail decode");
	ExpectInvalidEnum(NpcControlsChunkId, InvalidMoveModePayload(), "invalid move mode should fail decode");
}

void TestNestedSessionDecodeFailureIsPreserved()
{
	iggy::runtime::RuntimeSaveChunkArchive archive =
		iggy::runtime::RuntimeGameplaySnapshotChunkEncoder {}.encode(ExplicitSnapshot()).archive;
	RemoveChunk(archive, SessionChunkId);

	const iggy::runtime::RuntimeGameplaySnapshotDecodeResult result =
		iggy::runtime::RuntimeGameplaySnapshotChunkDecoder {}.decode(archive);

	Expect(!result.decoded, "nested session decode failure should fail gameplay decode");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplaySnapshotChunkIssueCode::NestedSessionDecodeFailed), "nested session failure should report gameplay issue");
	Expect(!result.session.decoded, "nested session decode result should be preserved");
}

void TestGameplaySnapshotValidationFailureAfterDecodeFailsDecode()
{
	iggy::runtime::RuntimeGameplaySnapshot snapshot = ExplicitSnapshot();
	snapshot.npcActors.actors[0].position = { 99.5F, 99.5F };
	const iggy::runtime::RuntimeSaveChunkArchive archive =
		iggy::runtime::RuntimeGameplaySnapshotChunkEncoder {}.encode(snapshot).archive;

	const iggy::runtime::RuntimeGameplaySnapshotDecodeResult result =
		iggy::runtime::RuntimeGameplaySnapshotChunkDecoder {}.decode(archive);

	Expect(!result.decoded, "decoded invalid gameplay snapshot should fail validation");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplaySnapshotChunkIssueCode::SnapshotValidationFailed), "gameplay validation failure should report decode issue");
	Expect(!result.validation.valid, "gameplay validation failure should preserve validation result");
	Expect(!result.validation.issues.empty(), "gameplay validation failure should preserve validation diagnostics");
}

void TestMissingOrphanControlValidationFailureAfterDecodeFailsDecode()
{
	iggy::runtime::RuntimeGameplaySnapshot snapshot = ExplicitSnapshot();
	snapshot.npcControls.entries.pop_back();
	snapshot.npcControls.entries.push_back(Control("npc:orphan", iggy::waitNpcObjective(), iggy::waitingNpcBehaviorState(), iggy::NpcMoveMode::Still));
	const iggy::runtime::RuntimeSaveChunkArchive archive =
		iggy::runtime::RuntimeGameplaySnapshotChunkEncoder {}.encode(snapshot).archive;

	const iggy::runtime::RuntimeGameplaySnapshotDecodeResult result =
		iggy::runtime::RuntimeGameplaySnapshotChunkDecoder {}.decode(archive);

	Expect(!result.decoded, "missing/orphan NPC controls should fail gameplay snapshot validation");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplaySnapshotChunkIssueCode::SnapshotValidationFailed), "missing/orphan controls should map to snapshot validation failure");
	Expect(result.validation.npcFrameState.issues.size() == 2, "missing/orphan controls should preserve frame-state diagnostics");
}

void TestUnknownChunkIgnored()
{
	iggy::runtime::RuntimeSaveChunkArchive archive =
		iggy::runtime::RuntimeGameplaySnapshotChunkEncoder {}.encode(ExplicitSnapshot()).archive;
	archive.chunks.insert(archive.chunks.begin() + 2, { UnknownChunkId, 1, { 1, 2, 3 } });

	const iggy::runtime::RuntimeGameplaySnapshotDecodeResult result =
		iggy::runtime::RuntimeGameplaySnapshotChunkDecoder {}.decode(archive);

	Expect(result.decoded, "unknown unrelated chunk should be ignored");
	Expect(result.issues.empty(), "unknown unrelated chunk should not produce gameplay issues");
	Expect(SameSnapshot(result.snapshot, ExplicitSnapshot()), "unknown unrelated chunk should not alter snapshot");
}

void TestInvalidArchiveFailsBeforePayloadDecode()
{
	iggy::runtime::RuntimeSaveChunkArchive archive =
		iggy::runtime::RuntimeGameplaySnapshotChunkEncoder {}.encode(ExplicitSnapshot()).archive;
	archive.magic = { 'B', 'A', 'D', '!' };

	const iggy::runtime::RuntimeGameplaySnapshotDecodeResult result =
		iggy::runtime::RuntimeGameplaySnapshotChunkDecoder {}.decode(archive);

	Expect(!result.decoded, "invalid archive should fail gameplay decode");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplaySnapshotChunkIssueCode::InvalidArchive), "invalid archive should report issue");
}

void TestEncodeDecodeDoNotMutateInputs()
{
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = ExplicitSnapshot();
	const iggy::runtime::RuntimeGameplaySnapshot before = snapshot;
	const iggy::runtime::RuntimeGameplaySnapshotEncodeResult encoded =
		iggy::runtime::RuntimeGameplaySnapshotChunkEncoder {}.encode(snapshot);
	const iggy::runtime::RuntimeSaveChunkArchive archiveBefore = encoded.archive;

	(void)iggy::runtime::RuntimeGameplaySnapshotChunkDecoder {}.decode(encoded.archive);

	Expect(SameSnapshot(snapshot, before), "encoding should not mutate source snapshot");
	Expect(encoded.archive.chunks.size() == archiveBefore.chunks.size(), "decoding should not mutate archive chunk count");
	for (std::size_t index = 0; index < encoded.archive.chunks.size(); ++index) {
		Expect(encoded.archive.chunks[index].id == archiveBefore.chunks[index].id, "decoding should not mutate archive chunk ids");
		Expect(encoded.archive.chunks[index].payload == archiveBefore.chunks[index].payload, "decoding should not mutate archive payloads");
	}
}

void TestCaptureEncodeDecodeRestoreAcceptance()
{
	iggy::runtime::RuntimeGameplayState state;
	state.session.level = Level({
		"........",
		"........",
		"........",
	});
	state.session.tickIndex = 12;
	state.session.hasPlayer = true;
	state.session.player = PlayerAgent(Id("player:one"), { 1.5F, 1.5F }, { 1, 1 });
	state.commandQueue = ExplicitSnapshot().commandQueue;
	state.interaction = ExplicitSnapshot().interaction;
	state.inventory = ExplicitSnapshot().inventory;
	state.npcActors = ExplicitSnapshot().npcActors;
	state.npcControls = ExplicitSnapshot().npcControls;

	const iggy::runtime::RuntimeGameplaySnapshot snapshot =
		iggy::runtime::RuntimeGameplaySnapshotBuilder {}.capture(state);
	const iggy::runtime::RuntimeSaveChunkArchive archive =
		iggy::runtime::RuntimeGameplaySnapshotChunkEncoder {}.encode(snapshot).archive;
	const iggy::runtime::RuntimeGameplaySnapshotDecodeResult decoded =
		iggy::runtime::RuntimeGameplaySnapshotChunkDecoder {}.decode(archive);
	iggy::runtime::RuntimeGameplaySnapshotRestoreConfig restoreConfig;
	restoreConfig.session.buildConfig = BuildConfigNoCaches();
	const iggy::runtime::RuntimeGameplaySnapshotRestoreResult restored =
		iggy::runtime::RuntimeGameplaySnapshotRestorer {}.restore(decoded.snapshot, restoreConfig);

	Expect(decoded.decoded, "captured gameplay snapshot archive should decode");
	Expect(restored.restored, "decoded gameplay snapshot should restore");
	Expect(SameQueue(restored.state.commandQueue, state.commandQueue), "archive-level restore should preserve command queue");
	Expect(SameInteraction(restored.state.interaction, state.interaction), "archive-level restore should preserve interaction state");
	Expect(SameInventory(restored.state.inventory, state.inventory), "archive-level restore should preserve inventory state");
	Expect(SameActors(restored.state.npcActors, state.npcActors), "archive-level restore should preserve NPC actors");
	Expect(SameControls(restored.state.npcControls, state.npcControls), "archive-level restore should preserve NPC controls");
}

} // namespace

int main()
{
	TestEmptyGameplaySnapshotRoundTrips();
	TestExplicitGameplaySnapshotRoundTripsByValue();
	TestMissingGameplayChunkFailsDecode();
	TestDuplicateGameplayChunkFailsDecode();
	TestUnsupportedGameplayChunkVersionFailsDecode();
	TestMalformedGameplayPayloadFailsDecode();
	TestInvalidGameplayEnumsFailDecode();
	TestNestedSessionDecodeFailureIsPreserved();
	TestGameplaySnapshotValidationFailureAfterDecodeFailsDecode();
	TestMissingOrphanControlValidationFailureAfterDecodeFailsDecode();
	TestUnknownChunkIgnored();
	TestInvalidArchiveFailsBeforePayloadDecode();
	TestEncodeDecodeDoNotMutateInputs();
	TestCaptureEncodeDecodeRestoreAcceptance();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
