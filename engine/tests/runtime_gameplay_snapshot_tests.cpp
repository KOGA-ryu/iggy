#include <cstdlib>
#include <string_view>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeGameplaySnapshot.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectPlayerAgent;
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

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::LevelRuntimeState Level(std::vector<std::string_view> rows)
{
	iggy::LevelRuntimeState level;
	level.map = MapFromRows(rows);
	level.map.id = Id("level:gameplay-snapshot");
	return level;
}

iggy::runtime::RuntimeSessionBuildConfig BuildConfigNoCaches()
{
	iggy::runtime::RuntimeSessionBuildConfig config;
	config.buildRenderCache = false;
	return config;
}

iggy::runtime::RuntimeGameplaySnapshotRestoreConfig RestoreConfig(
	iggy::runtime::RuntimeSessionBuildConfig buildConfig = BuildConfigNoCaches())
{
	iggy::runtime::RuntimeGameplaySnapshotRestoreConfig config;
	config.session.buildConfig = buildConfig;
	return config;
}

iggy::runtime::RuntimeSessionState Session()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = Level({
		"...",
		"...",
	});
	session.tickIndex = 23;
	session.hasPlayer = true;
	session.player = PlayerAgent(Id("player:one"), { 1.5F, 1.5F }, { 1, 1 });
	return session;
}

iggy::NpcActorState2D Actor(
	const char *npcId,
	iggy::Vec2 position,
	bool present = true)
{
	return {
		Id(npcId),
		Id("ai-profile:guard"),
		Id("faction:town"),
		position,
		Id("goal:patrol"),
		present,
	};
}

iggy::NpcActorControlState2D Control(
	const char *npcId,
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Walk)
{
	iggy::NpcActorControlState2D control;
	control.npcId = Id(npcId);
	control.behavior = iggy::idleNpcBehaviorState();
	control.objective = iggy::waitNpcObjective();
	control.moveMode = moveMode;
	return control;
}

iggy::runtime::GameplayCommand2D Command(
	const char *actorId,
	iggy::Vec2 target)
{
	return iggy::runtime::GameplayCommand2DFactory {}.moveToPoint(Id(actorId), target);
}

iggy::runtime::RuntimeGameplayState GameplayState()
{
	iggy::runtime::RuntimeGameplayState state;
	state.session = Session();
	state.commandQueue.frames = {
		{ { Command("player:one", { 2.0F, 2.0F }) } },
	};
	state.interaction.targets = iggy::InteractionTarget2DRegistry({
		{ Id("target:door"), iggy::InteractionTarget2DKind::Door, { 1.0F, 0.5F }, 0.5F, true },
	});
	state.inventory.inventory.stacks = {
		{ Id("item:potion"), 3 },
	};
	state.inventory.drops.drops = {
		{ Id("drop:potion"), Id("item:potion"), 1, { 2.5F, 0.5F }, 0.25F, true },
	};
	state.npcActors.actors = {
		Actor("npc:one", { 0.5F, 0.5F }),
		Actor("npc:two", { 2.5F, 1.5F }, false),
	};
	state.npcControls.entries = {
		Control("npc:one"),
		Control("npc:two", iggy::NpcMoveMode::Still),
	};
	return state;
}

bool SameLevel(const iggy::LevelRuntimeState &actual, const iggy::LevelRuntimeState &expected)
{
	if (actual.map.id != expected.map.id
		|| actual.map.width != expected.map.width
		|| actual.map.height != expected.map.height
		|| actual.map.tiles.size() != expected.map.tiles.size()
		|| actual.npcAgents.size() != expected.npcAgents.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.map.tiles.size(); ++index) {
		if (actual.map.tiles[index].walkable != expected.map.tiles[index].walkable) {
			return false;
		}
	}
	return true;
}

bool SameSessionSnapshot(
	const iggy::runtime::RuntimeSessionSnapshot &actual,
	const iggy::runtime::RuntimeSessionSnapshot &expected)
{
	return SameLevel(actual.level, expected.level)
		&& actual.tickIndex == expected.tickIndex
		&& actual.hasPlayer == expected.hasPlayer
		&& actual.player.id == expected.player.id
		&& NearVec(actual.player.position, expected.player.position)
		&& actual.player.spawnTile == expected.player.spawnTile
		&& actual.player.movementStatus == expected.player.movementStatus
		&& actual.player.facing == expected.player.facing;
}

bool SameSession(
	const iggy::runtime::RuntimeSessionState &actual,
	const iggy::runtime::RuntimeSessionState &expected)
{
	return SameLevel(actual.level, expected.level)
		&& actual.tickIndex == expected.tickIndex
		&& actual.hasPlayer == expected.hasPlayer
		&& actual.player.id == expected.player.id
		&& NearVec(actual.player.position, expected.player.position)
		&& actual.player.spawnTile == expected.player.spawnTile
		&& actual.player.movementStatus == expected.player.movementStatus
		&& actual.player.facing == expected.player.facing;
}

bool SameCommand(
	const iggy::runtime::GameplayCommand2D &actual,
	const iggy::runtime::GameplayCommand2D &expected)
{
	return actual.type == expected.type
		&& actual.actorId == expected.actorId
		&& NearVec(actual.targetPoint, expected.targetPoint)
		&& actual.targetTile == expected.targetTile
		&& actual.targetId == expected.targetId;
}

bool SameCommandQueue(
	const iggy::runtime::RuntimeCommandQueueState &actual,
	const iggy::runtime::RuntimeCommandQueueState &expected)
{
	if (actual.frames.size() != expected.frames.size()) {
		return false;
	}
	for (std::size_t frameIndex = 0; frameIndex < actual.frames.size(); ++frameIndex) {
		if (actual.frames[frameIndex].commands.size() != expected.frames[frameIndex].commands.size()) {
			return false;
		}
		for (std::size_t commandIndex = 0; commandIndex < actual.frames[frameIndex].commands.size(); ++commandIndex) {
			if (!SameCommand(actual.frames[frameIndex].commands[commandIndex], expected.frames[frameIndex].commands[commandIndex])) {
				return false;
			}
		}
	}
	return true;
}

bool SameInteraction(
	const iggy::runtime::RuntimeInteractionState &actual,
	const iggy::runtime::RuntimeInteractionState &expected)
{
	return actual.targets.targets().size() == expected.targets.targets().size()
		&& (actual.targets.targets().empty()
			|| actual.targets.targets()[0].id == expected.targets.targets()[0].id);
}

bool SameInventory(
	const iggy::runtime::RuntimeInventoryState &actual,
	const iggy::runtime::RuntimeInventoryState &expected)
{
	return actual.inventory.stacks.size() == expected.inventory.stacks.size()
		&& actual.drops.drops.size() == expected.drops.drops.size()
		&& (actual.inventory.stacks.empty()
			|| (actual.inventory.stacks[0].itemId == expected.inventory.stacks[0].itemId
				&& actual.inventory.stacks[0].count == expected.inventory.stacks[0].count))
		&& (actual.drops.drops.empty()
			|| (actual.drops.drops[0].id == expected.drops.drops[0].id
				&& actual.drops.drops[0].itemId == expected.drops.drops[0].itemId
				&& actual.drops.drops[0].count == expected.drops.drops[0].count
				&& NearVec(actual.drops.drops[0].position, expected.drops.drops[0].position)));
}

bool SameActor(const iggy::NpcActorState2D &actual, const iggy::NpcActorState2D &expected)
{
	return actual.npcId == expected.npcId
		&& actual.aiProfileId == expected.aiProfileId
		&& actual.factionId == expected.factionId
		&& NearVec(actual.position, expected.position)
		&& actual.currentGoalId == expected.currentGoalId
		&& actual.present == expected.present;
}

bool SameActors(
	const iggy::NpcActorState2DRegistry &actual,
	const iggy::NpcActorState2DRegistry &expected)
{
	if (actual.actors.size() != expected.actors.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.actors.size(); ++index) {
		if (!SameActor(actual.actors[index], expected.actors[index])) {
			return false;
		}
	}
	return true;
}

bool SameControl(
	const iggy::NpcActorControlState2D &actual,
	const iggy::NpcActorControlState2D &expected)
{
	return actual.npcId == expected.npcId
		&& actual.objective.type == expected.objective.type
		&& actual.behavior.type == expected.behavior.type
		&& actual.moveMode == expected.moveMode;
}

bool SameControls(
	const iggy::NpcActorControlState2DRegistry &actual,
	const iggy::NpcActorControlState2DRegistry &expected)
{
	if (actual.entries.size() != expected.entries.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.entries.size(); ++index) {
		if (!SameControl(actual.entries[index], expected.entries[index])) {
			return false;
		}
	}
	return true;
}

bool SameGameplayState(
	const iggy::runtime::RuntimeGameplayState &actual,
	const iggy::runtime::RuntimeGameplayState &expected)
{
	return SameSession(actual.session, expected.session)
		&& SameCommandQueue(actual.commandQueue, expected.commandQueue)
		&& SameInteraction(actual.interaction, expected.interaction)
		&& SameInventory(actual.inventory, expected.inventory)
		&& SameActors(actual.npcActors, expected.npcActors)
		&& SameControls(actual.npcControls, expected.npcControls);
}

void TestDefaultGameplayStateCapturesDefaultChildPackets()
{
	const iggy::runtime::RuntimeGameplayState state;

	const iggy::runtime::RuntimeGameplaySnapshot snapshot =
		iggy::runtime::RuntimeGameplaySnapshotBuilder {}.capture(state);

	Expect(snapshot.commandQueue.frames.empty(), "default gameplay snapshot should capture empty command queue");
	Expect(snapshot.interaction.targets.targets().empty(), "default gameplay snapshot should capture empty interaction state");
	Expect(snapshot.inventory.inventory.stacks.empty(), "default gameplay snapshot should capture empty inventory");
	Expect(snapshot.inventory.drops.drops.empty(), "default gameplay snapshot should capture empty drops");
	Expect(snapshot.npcActors.actors.empty(), "default gameplay snapshot should capture empty NPC actors");
	Expect(snapshot.npcControls.entries.empty(), "default gameplay snapshot should capture empty NPC controls");
}

void TestExplicitGameplayStateCapturesChildrenByValue()
{
	iggy::runtime::RuntimeGameplayState state = GameplayState();

	const iggy::runtime::RuntimeGameplaySnapshot snapshot =
		iggy::runtime::RuntimeGameplaySnapshotBuilder {}.capture(state);

	Expect(SameSessionSnapshot(snapshot.session, iggy::runtime::RuntimeSessionSnapshotBuilder {}.capture(state.session)), "gameplay snapshot should capture session snapshot");
	Expect(SameCommandQueue(snapshot.commandQueue, state.commandQueue), "gameplay snapshot should capture command queue");
	Expect(SameInteraction(snapshot.interaction, state.interaction), "gameplay snapshot should capture interaction state");
	Expect(SameInventory(snapshot.inventory, state.inventory), "gameplay snapshot should capture inventory state");
	Expect(SameActors(snapshot.npcActors, state.npcActors), "gameplay snapshot should capture NPC actors");
	Expect(SameControls(snapshot.npcControls, state.npcControls), "gameplay snapshot should capture NPC controls");

	state.npcActors.actors[0].position = { 9.5F, 9.5F };
	Expect(NearVec(snapshot.npcActors.actors[0].position, { 0.5F, 0.5F }), "gameplay snapshot should not alias source NPC actors");
}

void TestRestoreRehydratesGameplayStateWithCopiedGameplayChildren()
{
	const iggy::runtime::RuntimeGameplayState state = GameplayState();
	const iggy::runtime::RuntimeGameplaySnapshot snapshot =
		iggy::runtime::RuntimeGameplaySnapshotBuilder {}.capture(state);

	const iggy::runtime::RuntimeGameplaySnapshotRestoreResult result =
		iggy::runtime::RuntimeGameplaySnapshotRestorer {}.restore(snapshot, RestoreConfig());

	Expect(result.restored, "gameplay snapshot restore should succeed for valid session snapshot");
	Expect(result.session.restored, "gameplay snapshot restore should preserve nested session restore");
	Expect(SameGameplayState(result.state, state), "gameplay snapshot restore should rehydrate session and gameplay children");
}

void TestFailedSessionRestoreFailsGameplayRestore()
{
	const iggy::runtime::RuntimeGameplayState state = GameplayState();
	const iggy::runtime::RuntimeGameplaySnapshot snapshot =
		iggy::runtime::RuntimeGameplaySnapshotBuilder {}.capture(state);
	iggy::runtime::RuntimeSessionBuildConfig badBuildConfig = BuildConfigNoCaches();
	badBuildConfig.buildDerivedCaches = true;
	badBuildConfig.derivedCacheConfig.buildRenderCache = true;
	badBuildConfig.derivedCacheConfig.renderCacheConfig.chunkWidth = 0;

	const iggy::runtime::RuntimeGameplaySnapshotRestoreResult result =
		iggy::runtime::RuntimeGameplaySnapshotRestorer {}.restore(snapshot, RestoreConfig(badBuildConfig));

	Expect(!result.restored, "failed session restore should fail gameplay restore");
	Expect(!result.session.restored, "failed gameplay restore should preserve nested failed session restore");
	Expect(!result.state.session.hasPlayer, "failed gameplay restore should not publish partial gameplay state");
	Expect(result.state.npcActors.actors.empty(), "failed gameplay restore should leave gameplay state default");
}

void TestSnapshotCopiesAreIndependent()
{
	const iggy::runtime::RuntimeGameplayState original = GameplayState();
	iggy::runtime::RuntimeGameplaySnapshot snapshot =
		iggy::runtime::RuntimeGameplaySnapshotBuilder {}.capture(original);
	const iggy::runtime::RuntimeGameplaySnapshot snapshotBefore = snapshot;

	iggy::runtime::RuntimeGameplaySnapshotRestoreResult result =
		iggy::runtime::RuntimeGameplaySnapshotRestorer {}.restore(snapshot, RestoreConfig());

	Expect(result.restored, "copy independence restore should succeed");
	result.state.npcActors.actors[0].position = { 8.5F, 8.5F };
	result.state.commandQueue.frames.clear();
	snapshot.npcActors.actors[0].position = { 7.5F, 7.5F };

	Expect(NearVec(original.npcActors.actors[0].position, { 0.5F, 0.5F }), "mutating restore/snapshot should not mutate original");
	Expect(NearVec(snapshotBefore.npcActors.actors[0].position, { 0.5F, 0.5F }), "snapshot copy should preserve original snapshot value");
}

void TestInvalidLookingGameplayChildrenArePreservedWithoutValidation()
{
	iggy::runtime::RuntimeGameplayState state = GameplayState();
	state.commandQueue.frames.push_back({ { {} } });
	state.inventory.inventory.stacks.push_back({ {}, 0 });
	state.npcActors.actors.push_back({});
	state.npcControls.entries.push_back({});

	const iggy::runtime::RuntimeGameplaySnapshot snapshot =
		iggy::runtime::RuntimeGameplaySnapshotBuilder {}.capture(state);
	const iggy::runtime::RuntimeGameplaySnapshotRestoreResult result =
		iggy::runtime::RuntimeGameplaySnapshotRestorer {}.restore(snapshot, RestoreConfig());

	Expect(result.restored, "invalid-looking gameplay children should not fail capture/restore");
	Expect(result.state.commandQueue.frames.size() == state.commandQueue.frames.size(), "snapshot should preserve command queue without validation");
	Expect(result.state.inventory.inventory.stacks.size() == state.inventory.inventory.stacks.size(), "snapshot should preserve inventory without validation");
	Expect(result.state.npcActors.actors.size() == state.npcActors.actors.size(), "snapshot should preserve NPC actors without validation");
	Expect(result.state.npcControls.entries.size() == state.npcControls.entries.size(), "snapshot should preserve NPC controls without validation");
	Expect(result.state.npcActors.actors.back().npcId.empty(), "snapshot should preserve invalid-looking NPC actor");
	Expect(result.state.npcControls.entries.back().npcId.empty(), "snapshot should preserve invalid-looking NPC control");
}

} // namespace

int main()
{
	TestDefaultGameplayStateCapturesDefaultChildPackets();
	TestExplicitGameplayStateCapturesChildrenByValue();
	TestRestoreRehydratesGameplayStateWithCopiedGameplayChildren();
	TestFailedSessionRestoreFailsGameplayRestore();
	TestSnapshotCopiesAreIndependent();
	TestInvalidLookingGameplayChildrenArePreservedWithoutValidation();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
