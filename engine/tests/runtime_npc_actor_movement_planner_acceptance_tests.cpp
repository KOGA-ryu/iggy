#include <cstdlib>
#include <string_view>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeGameplayFrameReporter.hpp"
#include "runtime/RuntimeGameplayFrameRunner.hpp"
#include "runtime/RuntimeGameplayFrameStep.hpp"
#include "runtime/RuntimePolicyGameplayFrameReporter.hpp"
#include "runtime/RuntimePolicyGameplayFrameRunner.hpp"
#include "runtime/RuntimePolicyGameplayFrameStep.hpp"
#include "scene/npc/NpcActorMovementFramePlan2D.hpp"
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
struct HasNpcActorsField : std::false_type {
};

template <typename T>
struct HasNpcActorsField<T, std::void_t<decltype(&T::npcActors)>> : std::true_type {
};

template <typename T, typename = void>
struct HasNpcControlsField : std::false_type {
};

template <typename T>
struct HasNpcControlsField<T, std::void_t<decltype(&T::npcControls)>> : std::true_type {
};

static_assert(!HasNpcActorsField<iggy::runtime::RuntimeSessionState>::value);
static_assert(!HasNpcControlsField<iggy::runtime::RuntimeSessionState>::value);

const iggy::ResourceId RawPlayerId { "player:planner-runtime-raw" };
const iggy::ResourceId PolicyPlayerId { "player:planner-runtime-policy" };

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::LevelTileMap Map(std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map = MapFromRows(rows);
	map.id = Id("level:planner-runtime-acceptance");
	return map;
}

iggy::runtime::RuntimePlayerCommandExecutionConfig PlayerConfig()
{
	iggy::runtime::RuntimePlayerCommandExecutionConfig config;
	config.movement.maxStep = 1.0F;
	return config;
}

iggy::npc_ai::NpcAgentTickConfig NpcConfig()
{
	iggy::npc_ai::NpcAgentTickConfig config;
	config.maxDistance = 0.25F;
	config.awareness = { 8.0F, 0 };
	return config;
}

iggy::runtime::RuntimeSessionState SessionWithPlayer(
	const iggy::ResourceId &playerId,
	const iggy::LevelTileMap &map,
	iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.level.map = map;
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 77;
	session.hasPlayer = true;
	session.player = PlayerAgent(playerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
	return session;
}

iggy::NpcActorState2D Actor(
	const char *npcId,
	iggy::Vec2 position,
	bool present = true)
{
	return {
		Id(npcId),
		Id("profile:planner-runtime"),
		Id("faction:planner-runtime"),
		position,
		{},
		present,
	};
}

iggy::NpcActorControlState2D SeekingControl(
	const char *npcId,
	iggy::Vec2 target,
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Walk)
{
	return {
		Id(npcId),
		iggy::moveToNpcObjective(target),
		iggy::seekingNpcBehaviorState(target),
		moveMode,
	};
}

iggy::NpcActorControlState2D IdleControl(const char *npcId)
{
	return {
		Id(npcId),
		iggy::waitNpcObjective(),
		iggy::idleNpcBehaviorState(),
		iggy::NpcMoveMode::Still,
	};
}

iggy::NpcActorState2DRegistry Actors(std::vector<iggy::NpcActorState2D> actors)
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);
	Expect(result.built, "planner runtime actor fixture should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "planner runtime control fixture should build");
	return result.registry;
}

iggy::runtime::RuntimeGameplayState GameplayState(
	const iggy::LevelTileMap &map,
	const iggy::ResourceId &playerId,
	iggy::runtime::RuntimeInteractionState interaction = {},
	iggy::runtime::RuntimeInventoryState inventory = {})
{
	return { SessionWithPlayer(playerId, map), {}, interaction, inventory };
}

iggy::InteractionTarget2D Target(
	const char *id,
	iggy::InteractionTarget2DKind kind = iggy::InteractionTarget2DKind::Usable)
{
	return { Id(id), kind, { 0.0F, 0.0F }, 0.0F, true };
}

iggy::InteractionTarget2DRegistry TargetRegistry(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result =
		iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "planner runtime target registry fixture should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { Id(targetId), effects };
}

iggy::InteractionEffectCatalog2D EffectCatalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result =
		iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "planner runtime effect catalog fixture should build");
	return result.catalog;
}

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { Id(itemId), count };
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result =
		iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "planner runtime inventory fixture should build");
	return result.inventory;
}

iggy::LevelItemDrop2D Drop(const char *id, const char *itemId, std::uint32_t count = 1)
{
	return { Id(id), Id(itemId), count, { 0.0F, 0.0F }, 0.0F, true };
}

iggy::runtime::RuntimeInventoryState InventoryState(
	std::vector<iggy::InventoryItemStack2D> stacks,
	std::vector<iggy::LevelItemDrop2D> drops)
{
	return { Inventory(stacks), iggy::LevelItemDrop2DRegistry { drops } };
}

iggy::ItemDefinition2D Definition(const char *itemId, const char *displayName = "Item", std::uint32_t maxStackCount = 10)
{
	return { Id(itemId), displayName, maxStackCount, iggy::ItemDefinition2DKind::Material };
}

iggy::ItemDefinition2DCatalog ItemCatalog(std::vector<iggy::ItemDefinition2D> definitions)
{
	const iggy::ItemDefinition2DCatalogBuildResult result =
		iggy::ItemDefinition2DCatalogBuilder {}.build(definitions);
	Expect(result.built, "planner runtime item catalog fixture should build");
	return result.catalog;
}

iggy::NpcActorMovementFramePlan2DResult Plan(
	const iggy::runtime::RuntimeGameplayState &state,
	const iggy::LevelTileMap &map)
{
	return iggy::NpcActorMovementFramePlanner2D {}.plan(state.npcActors, state.npcControls, map);
}

bool SameMap(const iggy::LevelTileMap &actual, const iggy::LevelTileMap &expected)
{
	if (actual.id != expected.id
		|| actual.width != expected.width
		|| actual.height != expected.height
		|| actual.tiles.size() != expected.tiles.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.tiles.size(); ++index) {
		if (actual.tiles[index].walkable != expected.tiles[index].walkable) {
			return false;
		}
	}
	return true;
}

bool SameActors(
	const iggy::NpcActorState2DRegistry &actual,
	const iggy::NpcActorState2DRegistry &expected)
{
	if (actual.actors.size() != expected.actors.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.actors.size(); ++index) {
		const iggy::NpcActorState2D &left = actual.actors[index];
		const iggy::NpcActorState2D &right = expected.actors[index];
		if (left.npcId != right.npcId
			|| left.aiProfileId != right.aiProfileId
			|| left.factionId != right.factionId
			|| !NearVec(left.position, right.position)
			|| left.currentGoalId != right.currentGoalId
			|| left.present != right.present) {
			return false;
		}
	}
	return true;
}

bool SameControls(
	const iggy::NpcActorControlState2DRegistry &actual,
	const iggy::NpcActorControlState2DRegistry &expected)
{
	if (actual.entries.size() != expected.entries.size()) {
		return false;
	}
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
			|| left.moveMode != right.moveMode) {
			return false;
		}
	}
	return true;
}

bool SameFilter(
	const iggy::NpcActorPathStepOccupancyFilter2D &actual,
	const iggy::NpcActorPathStepOccupancyFilter2D &expected)
{
	return actual.step.npcId == expected.step.npcId
		&& NearVec(actual.step.oldPosition, expected.step.oldPosition)
		&& NearVec(actual.step.proposedPosition, expected.step.proposedPosition)
		&& actual.step.oldTile == expected.step.oldTile
		&& actual.step.proposedTile == expected.step.proposedTile
		&& actual.status == expected.status
		&& actual.requestsMovement == expected.requestsMovement
		&& actual.blockingNpcId == expected.blockingNpcId;
}

bool SameRequests(
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> &actual,
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> &expected)
{
	if (actual.size() != expected.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameFilter(actual[index].filter, expected[index].filter)) {
			return false;
		}
	}
	return true;
}

bool SameStacks(
	const std::vector<iggy::InventoryItemStack2D> &actual,
	const std::vector<iggy::InventoryItemStack2D> &expected)
{
	if (actual.size() != expected.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index].itemId != expected[index].itemId || actual[index].count != expected[index].count) {
			return false;
		}
	}
	return true;
}

void TestRawFrameUsesPlannerGeneratedRequests()
{
	const iggy::LevelTileMap map = Map({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map, RawPlayerId);
	state.npcActors = Actors({ Actor("npc:raw-planned", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:raw-planned", { 3.5F, 0.5F }) });
	const iggy::runtime::RuntimeGameplayState stateBefore = state;
	const iggy::LevelTileMap mapBefore = map;
	const iggy::NpcActorMovementFramePlan2DResult plan = Plan(state, map);
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requestsBefore = plan.requests;

	iggy::runtime::RuntimeGameplayFrameInput input;
	input.state = state;
	input.actorId = RawPlayerId;
	input.fallbackPlayerPosition = { 0.5F, 0.5F };
	input.playerCommandConfig = PlayerConfig();
	input.npcConfig = NpcConfig();
	input.npcMovementRequests = plan.requests;

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(input);
	const iggy::runtime::RuntimeGameplayFrameReport report =
		iggy::runtime::RuntimeGameplayFrameReporter {}.report(result);

	Expect(plan.requestCount == 1 && plan.preparedCount == 1, "raw frame planner fixture should generate one request");
	Expect(result.npcMovement.apply.requestCount == plan.requestCount, "raw frame should apply exactly planner request count");
	Expect(SameFilter(result.npcMovement.apply.entries[0].request.filter, plan.requests[0].filter), "raw frame nested apply should preserve planner request filter");
	Expect(result.npcMovement.report.movedCount == 1 && report.npcMovedCount == 1, "raw frame should report planned NPC movement");
	Expect(NearVec(result.state.npcActors.actors[0].position, plan.requests[0].filter.step.proposedPosition), "raw frame should update NPC actor to planner proposed position");
	Expect(SameControls(result.state.npcControls, state.npcControls), "raw frame should preserve NPC controls");
	Expect(SameActors(state.npcActors, stateBefore.npcActors), "raw frame planner/runtime should not mutate input actors");
	Expect(SameRequests(input.npcMovementRequests, requestsBefore), "raw frame runtime input requests should remain unchanged");
	Expect(SameMap(map, mapBefore), "raw frame planner/runtime should not mutate map");
}

void TestPolicyFrameUsesPlannerRequestsAndPreservesPickup()
{
	const iggy::LevelTileMap map = Map({ "...." });
	const iggy::InteractionTarget2D target = Target("target:policy-planner-pickup", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:policy-planner-pickup", "item:policy-planner-pickup");
	const iggy::runtime::RuntimeInteractionState interaction {
		TargetRegistry({ target }),
		EffectCatalog({ Entry("target:policy-planner-pickup", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	iggy::runtime::RuntimeGameplayState state =
		GameplayState(map, PolicyPlayerId, interaction, InventoryState({}, { drop }));
	state.npcActors = Actors({ Actor("npc:policy-planned", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:policy-planned", { 3.5F, 0.5F }) });
	const iggy::NpcActorMovementFramePlan2DResult plan = Plan(state, map);
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requestsBefore = plan.requests;

	iggy::runtime::RuntimePolicyGameplayFrameInput input;
	input.state = state;
	input.actorId = PolicyPlayerId;
	input.playerIntents = { iggy::playerInteractIntent(target.id) };
	input.fallbackPlayerPosition = { 0.5F, 0.5F };
	input.playerCommandConfig = PlayerConfig();
	input.npcConfig = NpcConfig();
	input.itemDefinitions = ItemCatalog({ Definition("item:policy-planner-pickup", "Policy Planner Pickup", 1) });
	input.npcMovementRequests = plan.requests;

	const iggy::runtime::RuntimePolicyGameplayFrameResult result =
		iggy::runtime::RuntimePolicyGameplayFrameStep {}.run(input);
	const iggy::runtime::RuntimePolicyGameplayFrameReport report =
		iggy::runtime::RuntimePolicyGameplayFrameReporter {}.report(result);

	Expect(plan.requestCount == 1, "policy frame planner fixture should generate one request");
	Expect(SameStacks(result.state.inventory.inventory.stacks, { Stack("item:policy-planner-pickup", 1) }), "policy frame should preserve policy pickup behavior with planner request");
	Expect(result.inventoryEvents.events.size() == 3, "policy frame should preserve inventory events with planner request");
	Expect(NearVec(result.state.npcActors.actors[0].position, plan.requests[0].filter.step.proposedPosition), "policy frame should update NPC actor to planner proposed position");
	Expect(report.npcMovedCount == 1 && report.npcMovementDirtyTileCount == 2, "policy frame report should project planner movement facts");
	Expect(SameControls(result.state.npcControls, state.npcControls), "policy frame should preserve NPC controls");
	Expect(SameRequests(input.npcMovementRequests, requestsBefore), "policy frame runtime input requests should remain unchanged");
	Expect(state.inventory.inventory.stacks.empty(), "policy frame should not mutate input inventory");
}

void TestRawRunnerUsesPlannerRequestsAcrossFrames()
{
	const iggy::LevelTileMap map = Map({ "...." });
	iggy::runtime::RuntimeGameplayState initial = GameplayState(map, RawPlayerId);
	initial.npcActors = Actors({ Actor("npc:raw-runner-planned", { 0.5F, 0.5F }) });
	initial.npcControls = Controls({ SeekingControl("npc:raw-runner-planned", { 3.5F, 0.5F }) });

	const iggy::NpcActorMovementFramePlan2DResult firstPlan = Plan(initial, map);
	iggy::runtime::RuntimeGameplayState expectedAfterFirst = initial;
	expectedAfterFirst.npcActors.actors[0].position = firstPlan.requests[0].filter.step.proposedPosition;
	const iggy::NpcActorMovementFramePlan2DResult secondPlan = Plan(expectedAfterFirst, map);

	iggy::runtime::RuntimeGameplayFrameRunnerFrame first;
	first.actorId = RawPlayerId;
	first.fallbackPlayerPosition = { 0.5F, 0.5F };
	first.playerCommandConfig = PlayerConfig();
	first.npcConfig = NpcConfig();
	first.npcMovementRequests = firstPlan.requests;
	iggy::runtime::RuntimeGameplayFrameRunnerFrame second = first;
	second.npcMovementRequests = secondPlan.requests;
	const std::vector<iggy::runtime::RuntimeGameplayFrameRunnerFrame> frames { first, second };

	const iggy::runtime::RuntimeGameplayFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayFrameRunner {}.run({ initial, frames });

	Expect(firstPlan.requestCount == 1 && secondPlan.requestCount == 1, "raw runner planner fixtures should generate one request per frame");
	Expect(NearVec(result.ticks[0].frame.state.npcActors.actors[0].position, firstPlan.requests[0].filter.step.proposedPosition), "raw runner first tick should apply first planner request");
	Expect(NearVec(result.finalState.npcActors.actors[0].position, secondPlan.requests[0].filter.step.proposedPosition), "raw runner should carry first movement and apply second planner request");
	Expect(result.npcMovedCount == 2 && result.npcMovementDirtyTileCount == 4, "raw runner should aggregate planner movement facts");
	Expect(result.npcMovementNeedsOccupancyRebuild && result.npcMovementNeedsRenderRefresh, "raw runner should aggregate planner refresh flags");
	Expect(SameControls(result.finalState.npcControls, initial.npcControls), "raw runner should preserve NPC controls");
	Expect(SameRequests(frames[0].npcMovementRequests, firstPlan.requests), "raw runner input frame requests should remain unchanged");
	Expect(NearVec(initial.npcActors.actors[0].position, { 0.5F, 0.5F }), "raw runner should not mutate input state");
}

void TestPolicyRunnerUsesPlannerRequestsAcrossFramesAndPreservesInventoryEvents()
{
	const iggy::LevelTileMap map = Map({ "...." });
	const iggy::InteractionTarget2D target = Target("target:policy-runner-planner-pickup", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:policy-runner-planner-pickup", "item:policy-runner-planner-pickup");
	const iggy::runtime::RuntimeInteractionState interaction {
		TargetRegistry({ target }),
		EffectCatalog({ Entry("target:policy-runner-planner-pickup", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	iggy::runtime::RuntimeGameplayState initial =
		GameplayState(map, PolicyPlayerId, interaction, InventoryState({}, { drop }));
	initial.npcActors = Actors({ Actor("npc:policy-runner-planned", { 0.5F, 0.5F }) });
	initial.npcControls = Controls({ SeekingControl("npc:policy-runner-planned", { 3.5F, 0.5F }) });

	const iggy::NpcActorMovementFramePlan2DResult firstPlan = Plan(initial, map);
	iggy::runtime::RuntimeGameplayState expectedAfterFirst = initial;
	expectedAfterFirst.npcActors.actors[0].position = firstPlan.requests[0].filter.step.proposedPosition;
	const iggy::NpcActorMovementFramePlan2DResult secondPlan = Plan(expectedAfterFirst, map);

	iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame first;
	first.actorId = PolicyPlayerId;
	first.playerIntents = { iggy::playerInteractIntent(target.id) };
	first.fallbackPlayerPosition = { 0.5F, 0.5F };
	first.playerCommandConfig = PlayerConfig();
	first.npcConfig = NpcConfig();
	first.npcMovementRequests = firstPlan.requests;
	iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame second = first;
	second.playerIntents = {};
	second.npcMovementRequests = secondPlan.requests;

	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult result =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run({
			initial,
			{ first, second },
			ItemCatalog({ Definition("item:policy-runner-planner-pickup", "Policy Runner Planner Pickup", 1) }),
		});

	Expect(result.ticks.size() == 2, "policy runner should produce two planner-request ticks");
	Expect(result.ticks[0].report.pickedUpCount == 1, "policy runner should preserve pickup in first planner-request frame");
	Expect(result.inventoryEvents.events.size() == 3, "policy runner should preserve policy inventory aggregation");
	Expect(NearVec(result.finalState.npcActors.actors[0].position, secondPlan.requests[0].filter.step.proposedPosition), "policy runner should carry first movement and apply second planner request");
	Expect(result.npcMovedCount == 2 && result.npcMovementDirtyTileCount == 4, "policy runner should aggregate planner movement facts");
	Expect(result.npcMovementNeedsAiMapQueryRefresh && result.npcMovementNeedsVisibilityRefresh, "policy runner should aggregate planner refresh flags");
	Expect(SameControls(result.finalState.npcControls, initial.npcControls), "policy runner should preserve NPC controls");
	Expect(SameRequests(first.npcMovementRequests, firstPlan.requests), "policy runner frame requests should remain unchanged");
}

void TestPlannerBlockedRequestFeedsRuntimeBlockedPath()
{
	const iggy::LevelTileMap map = Map({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map, RawPlayerId);
	state.npcActors = Actors({
		Actor("npc:blocked-planned", { 0.5F, 0.5F }),
		Actor("npc:blocker-planned", { 1.5F, 0.5F }),
	});
	state.npcControls = Controls({
		SeekingControl("npc:blocked-planned", { 3.5F, 0.5F }),
		IdleControl("npc:blocker-planned"),
	});
	const iggy::NpcActorMovementFramePlan2DResult plan = Plan(state, map);

	iggy::runtime::RuntimeGameplayFrameInput input;
	input.state = state;
	input.actorId = RawPlayerId;
	input.fallbackPlayerPosition = { 0.5F, 0.5F };
	input.playerCommandConfig = PlayerConfig();
	input.npcConfig = NpcConfig();
	input.npcMovementRequests = plan.requests;

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(input);
	const iggy::runtime::RuntimeGameplayFrameReport report =
		iggy::runtime::RuntimeGameplayFrameReporter {}.report(result);

	Expect(plan.requestCount == 1 && plan.blockedRequestCount == 1, "planner should create a blocked prepared request");
	Expect(plan.requests[0].filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "planner request should preserve blocked filter status");
	Expect(NearVec(result.state.npcActors.actors[0].position, { 0.5F, 0.5F }), "runtime should preserve blocked actor position");
	Expect(result.npcMovement.apply.blockedCount == 1 && report.npcBlockedMovementCount == 1, "runtime should report planner blocked request");
	Expect(report.npcMovedCount == 0 && !report.npcMovementNeedsOccupancyRebuild, "blocked planner request should not report movement refresh");
}

void TestPlannerNoRequestCasesFeedRuntimeNoOp()
{
	const iggy::LevelTileMap idleMap = Map({ "..." });
	iggy::runtime::RuntimeGameplayState idleState = GameplayState(idleMap, RawPlayerId);
	idleState.npcActors = Actors({ Actor("npc:idle-planned", { 0.5F, 0.5F }) });
	idleState.npcControls = Controls({ IdleControl("npc:idle-planned") });
	const iggy::NpcActorMovementFramePlan2DResult idlePlan = Plan(idleState, idleMap);

	iggy::runtime::RuntimeGameplayFrameInput idleInput;
	idleInput.state = idleState;
	idleInput.actorId = RawPlayerId;
	idleInput.fallbackPlayerPosition = { 0.5F, 0.5F };
	idleInput.playerCommandConfig = PlayerConfig();
	idleInput.npcConfig = NpcConfig();
	idleInput.npcMovementRequests = idlePlan.requests;
	const iggy::runtime::RuntimeGameplayFrameResult idleResult =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(idleInput);

	const iggy::LevelTileMap pathMap = Map({ ".#." });
	iggy::runtime::RuntimeGameplayState pathState = GameplayState(pathMap, RawPlayerId);
	pathState.npcActors = Actors({ Actor("npc:path-fail-planned", { 0.5F, 0.5F }) });
	pathState.npcControls = Controls({ SeekingControl("npc:path-fail-planned", { 2.5F, 0.5F }) });
	const iggy::NpcActorMovementFramePlan2DResult pathPlan = Plan(pathState, pathMap);
	iggy::runtime::RuntimeGameplayFrameInput pathInput = idleInput;
	pathInput.state = pathState;
	pathInput.npcMovementRequests = pathPlan.requests;
	const iggy::runtime::RuntimeGameplayFrameResult pathResult =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(pathInput);

	Expect(idlePlan.requestCount == 0 && idlePlan.noMovementIntentCount == 1, "idle planner case should produce no request");
	Expect(NearVec(idleResult.state.npcActors.actors[0].position, { 0.5F, 0.5F }), "runtime should preserve idle no-request actor");
	Expect(idleResult.npcMovement.report.movedCount == 0 && idleResult.npcMovement.report.entryCount == 0, "runtime should report no movement for idle no-request frame");
	Expect(pathPlan.requestCount == 0 && pathPlan.pathFailedCount == 1, "path failure planner case should produce no request");
	Expect(NearVec(pathResult.state.npcActors.actors[0].position, { 0.5F, 0.5F }), "runtime should preserve path-failure no-request actor");
	Expect(pathResult.npcMovement.report.movedCount == 0 && pathResult.npcMovement.report.entryCount == 0, "runtime should report no movement for path-failure no-request frame");
}

} // namespace

int main()
{
	TestRawFrameUsesPlannerGeneratedRequests();
	TestPolicyFrameUsesPlannerRequestsAndPreservesPickup();
	TestRawRunnerUsesPlannerRequestsAcrossFrames();
	TestPolicyRunnerUsesPlannerRequestsAcrossFramesAndPreservesInventoryEvents();
	TestPlannerBlockedRequestFeedsRuntimeBlockedPath();
	TestPlannerNoRequestCasesFeedRuntimeNoOp();

	return Failures;
}
