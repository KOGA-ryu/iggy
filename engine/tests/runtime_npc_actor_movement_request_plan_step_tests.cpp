#include <cstdlib>
#include <string_view>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeGameplayFrameStep.hpp"
#include "runtime/RuntimeNpcActorMovementRequestPlanStep.hpp"
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

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::LevelTileMap Map(std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map = MapFromRows(rows);
	map.id = Id("level:runtime-request-plan");
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

iggy::runtime::RuntimeGameplayState GameplayState(const iggy::LevelTileMap &map)
{
	iggy::runtime::RuntimeGameplayState state;
	state.session.level.map = map;
	state.session.hasPlayer = true;
	state.session.player = PlayerAgent(
		Id("player:runtime-request-plan"),
		{ 0.0F, 0.0F },
		{ 0, 0 },
		iggy::PlayerMovementStatus::Idle,
		iggy::PlayerFacing2D::East);
	return state;
}

iggy::NpcActorState2D Actor(
	const char *npcId,
	iggy::Vec2 position,
	bool present = true)
{
	return {
		Id(npcId),
		Id("profile:runtime-request-plan"),
		Id("faction:runtime-request-plan"),
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

iggy::NpcActorControlState2D FleeingControl(
	const char *npcId,
	iggy::Vec2 threat,
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Run)
{
	return {
		Id(npcId),
		iggy::fleeNpcObjective(threat),
		iggy::fleeingNpcBehaviorState(threat),
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
	Expect(result.built, "runtime request planner actor registry fixture should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "runtime request planner control registry fixture should build");
	return result.registry;
}

iggy::runtime::RuntimeNpcActorMovementRequestPlanResult Plan(
	const iggy::runtime::RuntimeGameplayState &state,
	const iggy::LevelTileMap &map,
	const iggy::NpcActorMovementFramePlan2DConfig &config = {})
{
	return iggy::runtime::RuntimeNpcActorMovementRequestPlanStep {}.plan({ state, map, config });
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

bool SameState(
	const iggy::runtime::RuntimeGameplayState &actual,
	const iggy::runtime::RuntimeGameplayState &expected)
{
	return actual.session.hasPlayer == expected.session.hasPlayer
		&& actual.session.player.id == expected.session.player.id
		&& NearVec(actual.session.player.position, expected.session.player.position)
		&& SameActors(actual.npcActors, expected.npcActors)
		&& SameControls(actual.npcControls, expected.npcControls);
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

bool SameConfig(
	const iggy::NpcActorMovementFramePlan2DConfig &actual,
	const iggy::NpcActorMovementFramePlan2DConfig &expected)
{
	return actual.intent.allowAttackingApproach == expected.intent.allowAttackingApproach
		&& actual.intent.allowInteractingApproach == expected.intent.allowInteractingApproach
		&& actual.route.arrivalTolerance == expected.route.arrivalTolerance
		&& actual.route.hasEscapeDestination == expected.route.hasEscapeDestination
		&& NearVec(actual.route.escapeDestination, expected.route.escapeDestination)
		&& actual.escapeRoute.escape.searchRadius == expected.escapeRoute.escape.searchRadius
		&& actual.escapeRoute.escape.requireBetterThanCurrent == expected.escapeRoute.escape.requireBetterThanCurrent
		&& actual.escapeRoute.route.arrivalTolerance == expected.escapeRoute.route.arrivalTolerance
		&& actual.occupancy.includeAbsent == expected.occupancy.includeAbsent
		&& actual.pathStep.baseStepDistance == expected.pathStep.baseStepDistance
		&& actual.pathStep.arrivalTolerance == expected.pathStep.arrivalTolerance;
}

void TestEmptyStateProducesNoRequests()
{
	const iggy::LevelTileMap map = Map({ "..." });
	const iggy::LevelTileMap mapBefore = map;
	const iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	const iggy::runtime::RuntimeGameplayState stateBefore = state;

	const iggy::runtime::RuntimeNpcActorMovementRequestPlanResult result = Plan(state, map);

	Expect(!result.hasRequests(), "empty runtime request planner should have no requests");
	Expect(result.requestCount == 0 && result.preparedCount == 0, "empty runtime request planner should mirror empty counts");
	Expect(result.plan.status == iggy::NpcActorMovementFramePlan2DStatus::NoRequests, "empty runtime request planner should preserve nested plan status");
	Expect(SameState(result.state, state), "empty runtime request planner should copy state");
	Expect(SameMap(result.map, map), "empty runtime request planner should copy map");
	Expect(SameState(state, stateBefore), "empty runtime request planner should not mutate input state");
	Expect(SameMap(map, mapBefore), "empty runtime request planner should not mutate input map");
}

void TestSeekingStateProducesRequestsMatchingScenePlanner()
{
	const iggy::LevelTileMap map = Map({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:seeker", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:seeker", { 3.5F, 0.5F }) });
	const iggy::NpcActorMovementFramePlan2DResult scenePlan =
		iggy::NpcActorMovementFramePlanner2D {}.plan(state.npcActors, state.npcControls, map);

	const iggy::runtime::RuntimeNpcActorMovementRequestPlanResult result = Plan(state, map);

	Expect(result.hasRequests(), "seeking runtime request planner should have requests");
	Expect(result.requestCount == scenePlan.requestCount, "seeking runtime request planner should mirror scene request count");
	Expect(result.preparedCount == scenePlan.preparedCount, "seeking runtime request planner should mirror prepared count");
	Expect(SameRequests(result.requests, scenePlan.requests), "seeking runtime request planner should copy scene planner requests");
	Expect(SameRequests(result.requests, result.plan.requests), "seeking runtime request planner result requests should match nested plan requests");
	Expect(NearVec(result.requests[0].filter.step.proposedPosition, { 1.5F, 0.5F }), "seeking runtime request planner should preserve proposed position");
}

void TestFleeingStateProducesEscapeRouteRequest()
{
	const iggy::LevelTileMap map = Map({
		"...",
		"...",
		"...",
	});
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:flee", { 1.5F, 1.5F }) });
	state.npcControls = Controls({ FleeingControl("npc:flee", { 0.5F, 1.5F }) });

	const iggy::runtime::RuntimeNpcActorMovementRequestPlanResult result = Plan(state, map);

	Expect(result.requestCount == 1, "fleeing runtime request planner should produce one request");
	Expect(result.plan.entries[0].escapeRoute.status == iggy::NpcActorEscapeRouteTarget2DStatus::Ready, "fleeing runtime request planner should preserve escape diagnostics");
	Expect(result.requests[0].filter.step.npcId == Id("npc:flee"), "fleeing runtime request planner should preserve npc id");
	Expect(result.requests[0].filter.step.proposedPosition.x > 1.5F, "fleeing runtime request planner should move away from left threat");
}

void TestBlockedByNpcRequestIsPreserved()
{
	const iggy::LevelTileMap map = Map({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});
	state.npcControls = Controls({
		SeekingControl("npc:mover", { 3.5F, 0.5F }),
		IdleControl("npc:blocker"),
	});

	const iggy::runtime::RuntimeNpcActorMovementRequestPlanResult result = Plan(state, map);

	Expect(result.requestCount == 1 && result.blockedRequestCount == 1, "blocked runtime request planner should mirror blocked count");
	Expect(result.requests[0].filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "blocked runtime request planner should preserve blocked filter");
	Expect(result.requests[0].filter.blockingNpcId == Id("npc:blocker"), "blocked runtime request planner should preserve blocking npc id");
}

void TestIdleAndPathFailureProduceNoRequestsWithDiagnostics()
{
	const iggy::LevelTileMap idleMap = Map({ "..." });
	iggy::runtime::RuntimeGameplayState idleState = GameplayState(idleMap);
	idleState.npcActors = Actors({ Actor("npc:idle", { 0.5F, 0.5F }) });
	idleState.npcControls = Controls({ IdleControl("npc:idle") });
	const iggy::runtime::RuntimeNpcActorMovementRequestPlanResult idle = Plan(idleState, idleMap);

	const iggy::LevelTileMap pathMap = Map({ ".#." });
	iggy::runtime::RuntimeGameplayState pathState = GameplayState(pathMap);
	pathState.npcActors = Actors({ Actor("npc:path-fail", { 0.5F, 0.5F }) });
	pathState.npcControls = Controls({ SeekingControl("npc:path-fail", { 2.5F, 0.5F }) });
	const iggy::runtime::RuntimeNpcActorMovementRequestPlanResult path = Plan(pathState, pathMap);

	Expect(!idle.hasRequests() && idle.noMovementIntentCount == 1, "idle runtime request planner should produce no requests with no-movement diagnostics");
	Expect(idle.plan.entries[0].status == iggy::NpcActorMovementFramePlan2DEntryStatus::NoMovementIntent, "idle runtime request planner should preserve nested entry status");
	Expect(!path.hasRequests() && path.pathFailedCount == 1, "path failure runtime request planner should produce no requests with path diagnostics");
	Expect(path.plan.entries[0].status == iggy::NpcActorMovementFramePlan2DEntryStatus::PathNotFound, "path failure runtime request planner should preserve nested entry status");
}

void TestRequestsCanFeedRuntimeFrameButPlannerDoesNotApply()
{
	const iggy::LevelTileMap map = Map({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:runtime-feed", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:runtime-feed", { 3.5F, 0.5F }) });
	const iggy::runtime::RuntimeGameplayState stateBefore = state;

	const iggy::runtime::RuntimeNpcActorMovementRequestPlanResult planned = Plan(state, map);

	iggy::runtime::RuntimeGameplayFrameInput input;
	input.state = state;
	input.actorId = Id("player:runtime-request-plan");
	input.fallbackPlayerPosition = { 0.5F, 0.5F };
	input.playerCommandConfig = PlayerConfig();
	input.npcConfig = NpcConfig();
	input.npcMovementRequests = planned.requests;
	const iggy::runtime::RuntimeGameplayFrameResult frame =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(input);

	Expect(NearVec(planned.state.npcActors.actors[0].position, { 0.5F, 0.5F }), "runtime request planner step should not apply movement");
	Expect(SameState(state, stateBefore), "runtime request planner step should not mutate input state");
	Expect(NearVec(frame.state.npcActors.actors[0].position, planned.requests[0].filter.step.proposedPosition), "runtime frame should apply generated requests");
	Expect(frame.npcMovement.apply.movedCount == 1, "runtime frame should report generated request movement");
}

void TestInputsAndConfigAreNotMutated()
{
	const iggy::LevelTileMap map = Map({ "...." });
	const iggy::LevelTileMap mapBefore = map;
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:immutable", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:immutable", { 3.5F, 0.5F }) });
	const iggy::runtime::RuntimeGameplayState stateBefore = state;
	iggy::NpcActorMovementFramePlan2DConfig config;
	config.pathStep.baseStepDistance = 1.0F;
	config.occupancy.includeAbsent = true;
	const iggy::NpcActorMovementFramePlan2DConfig configBefore = config;

	(void)iggy::runtime::RuntimeNpcActorMovementRequestPlanStep {}.plan({ state, map, config });

	Expect(SameState(state, stateBefore), "runtime request planner should not mutate input state");
	Expect(SameMap(map, mapBefore), "runtime request planner should not mutate input map");
	Expect(SameConfig(config, configBefore), "runtime request planner should not mutate input config");
}

} // namespace

int main()
{
	TestEmptyStateProducesNoRequests();
	TestSeekingStateProducesRequestsMatchingScenePlanner();
	TestFleeingStateProducesEscapeRouteRequest();
	TestBlockedByNpcRequestIsPreserved();
	TestIdleAndPathFailureProduceNoRequestsWithDiagnostics();
	TestRequestsCanFeedRuntimeFrameButPlannerDoesNotApply();
	TestInputsAndConfigAreNotMutated();

	return Failures;
}
