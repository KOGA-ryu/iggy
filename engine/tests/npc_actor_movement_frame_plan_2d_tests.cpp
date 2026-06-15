#include <cstdlib>
#include <string_view>
#include <vector>

#include "scene/npc/NpcActorMovementFrameApply2D.hpp"
#include "scene/npc/NpcActorMovementFramePlan2D.hpp"
#include "scene/npc/NpcActorOccupancyQuery2D.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::LevelTileMap Map(std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map = iggy::test::MapFromRows(rows);
	map.id = Id("level:movement-frame-plan");
	return map;
}

iggy::NpcActorState2D Actor(
	const char *npcId,
	iggy::Vec2 position,
	bool present = true)
{
	return {
		Id(npcId),
		Id("profile:movement-frame-plan"),
		Id("faction:movement-frame-plan"),
		position,
		{},
		present,
	};
}

iggy::NpcActorControlState2D Control(
	const char *npcId,
	iggy::NpcObjective objective,
	iggy::NpcBehaviorState behavior,
	iggy::NpcMoveMode moveMode)
{
	return {
		Id(npcId),
		objective,
		behavior,
		moveMode,
	};
}

iggy::NpcActorState2DRegistry Actors(std::vector<iggy::NpcActorState2D> actors)
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);
	Expect(result.built, "actor fixture registry should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "control fixture registry should build");
	return result.registry;
}

iggy::NpcActorMovementFramePlan2DResult Plan(
	const iggy::NpcActorState2DRegistry &actors,
	const iggy::NpcActorControlState2DRegistry &controls,
	const iggy::LevelTileMap &map,
	const iggy::NpcActorMovementFramePlan2DConfig &config = {})
{
	return iggy::NpcActorMovementFramePlanner2D {}.plan(actors, controls, map, config);
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
	return actual.status == expected.status
		&& actual.requestsMovement == expected.requestsMovement
		&& actual.blockingNpcId == expected.blockingNpcId
		&& actual.step.npcId == expected.step.npcId
		&& NearVec(actual.step.oldPosition, expected.step.oldPosition)
		&& NearVec(actual.step.proposedPosition, expected.step.proposedPosition)
		&& actual.step.oldTile == expected.step.oldTile
		&& actual.step.proposedTile == expected.step.proposedTile;
}

void TestEmptyRegistriesProduceNoRequests()
{
	const iggy::LevelTileMap map = Map({ "..." });

	const iggy::NpcActorMovementFramePlan2DResult result = Plan({}, {}, map);

	Expect(result.status == iggy::NpcActorMovementFramePlan2DStatus::NoRequests, "empty planner input should produce NoRequests");
	Expect(!result.hasRequests(), "empty planner input should not have requests");
	Expect(result.entries.empty(), "empty planner input should have no entries");
	Expect(result.requests.empty(), "empty planner input should have no requests");
	Expect(result.frameState.entries.empty(), "empty planner input should preserve empty frame state");
	Expect(result.movementIntents.entries.empty(), "empty planner input should preserve empty movement intents");
	Expect(result.occupancy.entries.empty(), "empty planner input should build empty occupancy");
}

void TestSeekingActorPreparesAllowedRequest()
{
	const iggy::LevelTileMap map = Map({ "...." });
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:seeker", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = Controls({
		Control(
			"npc:seeker",
			iggy::moveToNpcObjective({ 3.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
	});

	const iggy::NpcActorMovementFramePlan2DResult result = Plan(actors, controls, map);

	Expect(result.status == iggy::NpcActorMovementFramePlan2DStatus::Planned, "seeking actor should plan a request");
	Expect(result.entryCount == 1 && result.requestCount == 1 && result.preparedCount == 1, "seeking actor should prepare exactly one request");
	Expect(result.entries[0].status == iggy::NpcActorMovementFramePlan2DEntryStatus::RequestPrepared, "seeking entry should be request prepared");
	Expect(result.entries[0].intent.type == iggy::NpcActorMovementIntent2DType::MoveTo, "seeking entry should preserve MoveTo intent");
	Expect(result.entries[0].route.status == iggy::NpcActorRouteTarget2DStatus::Ready, "seeking entry should preserve ready route");
	Expect(result.entries[0].navigation.status == iggy::NpcActorNavigationRequest2DStatus::Ready, "seeking entry should preserve ready navigation");
	Expect(result.entries[0].path.status == iggy::NpcActorPathReport2DStatus::PathFound, "seeking entry should preserve found path");
	Expect(result.entries[0].step.status == iggy::NpcActorPathStep2DStatus::Proposed, "seeking entry should preserve proposed step");
	Expect(result.entries[0].filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed, "seeking entry should preserve allowed filter");
	Expect(result.entries[0].requestIndex.has_value() && *result.entries[0].requestIndex == 0, "seeking entry should preserve request index");
	Expect(SameFilter(result.requests[0].filter, result.entries[0].filter), "prepared request should copy entry filter");
	Expect(NearVec(result.requests[0].filter.step.proposedPosition, { 1.5F, 0.5F }), "seeking request should use first bounded path step");
}

void TestFleeingActorPreparesRequestThroughEscapeRoute()
{
	const iggy::LevelTileMap map = Map({
		"...",
		"...",
		"...",
	});
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:flee", { 1.5F, 1.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = Controls({
		Control(
			"npc:flee",
			iggy::fleeNpcObjective({ 0.5F, 1.5F }),
			iggy::fleeingNpcBehaviorState({ 0.5F, 1.5F }),
			iggy::NpcMoveMode::Run),
	});

	const iggy::NpcActorMovementFramePlan2DResult result = Plan(actors, controls, map);

	Expect(result.requestCount == 1, "fleeing actor should prepare one request");
	Expect(result.entries[0].status == iggy::NpcActorMovementFramePlan2DEntryStatus::RequestPrepared, "fleeing entry should prepare request");
	Expect(result.entries[0].intent.type == iggy::NpcActorMovementIntent2DType::MoveAwayFrom, "fleeing entry should preserve MoveAwayFrom intent");
	Expect(result.entries[0].escapeRoute.status == iggy::NpcActorEscapeRouteTarget2DStatus::Ready, "fleeing entry should preserve ready escape route");
	Expect(result.entries[0].escapeRoute.escape.hasEscapeTarget, "fleeing entry should preserve escape target facts");
	Expect(result.entries[0].route.status == iggy::NpcActorRouteTarget2DStatus::Ready, "fleeing entry should preserve nested ready route");
	Expect(result.entries[0].filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed, "fleeing entry should preserve filter diagnostics");
	Expect(result.requests[0].filter.step.npcId == Id("npc:flee"), "fleeing request should preserve npc id");
	Expect(result.requests[0].filter.step.proposedPosition.x > 1.5F, "fleeing step should move away from left-side threat");
}

void TestOccupancyBlockedStepStillPreparesRequest()
{
	const iggy::LevelTileMap map = Map({ "...." });
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = Controls({
		Control(
			"npc:mover",
			iggy::moveToNpcObjective({ 3.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
		Control("npc:blocker", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still),
	});

	const iggy::NpcActorMovementFramePlan2DResult result = Plan(actors, controls, map);

	Expect(result.requestCount == 1, "blocked occupancy step should still prepare one request");
	Expect(result.blockedRequestCount == 1, "blocked occupancy step should increment blocked request count");
	Expect(result.entries[0].status == iggy::NpcActorMovementFramePlan2DEntryStatus::RequestPrepared, "blocked occupancy entry should be prepared");
	Expect(result.entries[0].filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "blocked occupancy entry should preserve blocked filter");
	Expect(result.entries[0].filter.blockingNpcId == Id("npc:blocker"), "blocked occupancy entry should preserve blocking npc id");
	Expect(SameFilter(result.requests[0].filter, result.entries[0].filter), "blocked prepared request should copy blocked filter");
}

void TestDefaultConfigStillUsesHardBlockFilter()
{
	const iggy::LevelTileMap map = Map({ "...." });
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = Controls({
		Control(
			"npc:mover",
			iggy::moveToNpcObjective({ 3.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
		Control("npc:blocker", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still),
	});

	const iggy::NpcActorMovementFramePlan2DResult result = Plan(actors, controls, map);

	Expect(!result.entries[0].usedOccupancyPolicy, "default planner config should not use occupancy policy");
	Expect(result.entries[0].policyFilter.status == iggy::NpcActorPathStepOccupancyPolicyFilter2DStatus::NoStepProposal, "default planner config should not populate policy diagnostics");
	Expect(result.entries[0].filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "default planner config should preserve old hard-block filter");
	Expect(result.requestCount == 1 && result.blockedRequestCount == 1, "default planner config should preserve blocked request counts");
}

void TestOccupancyPolicyCapacityAllowsPreviouslyBlockedStep()
{
	const iggy::LevelTileMap map = Map({ "...." });
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:occupant", { 1.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = Controls({
		Control(
			"npc:mover",
			iggy::moveToNpcObjective({ 3.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
		Control("npc:occupant", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still),
	});
	iggy::NpcActorMovementFramePlan2DConfig config;
	config.useOccupancyPolicy = true;
	config.occupancyPolicy.maxOccupantsPerTile = 2;

	const iggy::NpcActorMovementFramePlan2DResult result = Plan(actors, controls, map, config);

	Expect(result.requestCount == 1, "policy capacity should still produce caller-ready request");
	Expect(result.blockedRequestCount == 0, "policy capacity should not count shared tile as blocked");
	Expect(result.entries[0].usedOccupancyPolicy, "policy capacity should mark policy path");
	Expect(result.entries[0].policyFilter.status == iggy::NpcActorPathStepOccupancyPolicyFilter2DStatus::Allowed, "policy capacity should allow entering occupied tile");
	Expect(result.entries[0].policyFilter.policy.occupancyCount == 1, "policy capacity should preserve occupancy count");
	Expect(result.entries[0].policyFilter.policy.effectiveOccupancyCount == 1, "policy capacity should preserve effective occupancy count");
	Expect(result.entries[0].policyFilter.policy.capacity == 2, "policy capacity should preserve configured capacity");
	Expect(result.entries[0].filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed, "policy capacity should emit executor-compatible allowed filter");
}

void TestOccupancyPolicyDefaultCapacityMatchesHardBlock()
{
	const iggy::LevelTileMap map = Map({ "...." });
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = Controls({
		Control(
			"npc:mover",
			iggy::moveToNpcObjective({ 3.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
		Control("npc:blocker", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still),
	});
	iggy::NpcActorMovementFramePlan2DConfig config;
	config.useOccupancyPolicy = true;

	const iggy::NpcActorMovementFramePlan2DResult result = Plan(actors, controls, map, config);

	Expect(result.requestCount == 1 && result.blockedRequestCount == 1, "policy default capacity should match hard-block request counts");
	Expect(result.entries[0].policyFilter.status == iggy::NpcActorPathStepOccupancyPolicyFilter2DStatus::BlockedByNpc, "policy default capacity should block occupied tile");
	Expect(result.entries[0].policyFilter.blockingNpcId == Id("npc:blocker"), "policy default capacity should preserve blocker id");
	Expect(result.entries[0].policyFilter.policy.capacity == 1, "policy default capacity should preserve default capacity");
	Expect(result.entries[0].filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "policy default capacity should emit blocked executor-compatible filter");
}

void TestNavigationRejectedPreparesNoRequest()
{
	const iggy::LevelTileMap map = Map({ ".#." });
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:blocked-target", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = Controls({
		Control(
			"npc:blocked-target",
			iggy::moveToNpcObjective({ 1.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 1.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
	});

	const iggy::NpcActorMovementFramePlan2DResult result = Plan(actors, controls, map);

	Expect(result.status == iggy::NpcActorMovementFramePlan2DStatus::NoRequests, "navigation rejection should produce no requests");
	Expect(result.entries[0].status == iggy::NpcActorMovementFramePlan2DEntryStatus::NavigationRequestFailed, "blocked destination should fail at navigation");
	Expect(result.navigationFailedCount == 1, "blocked destination should increment navigation failure count");
	Expect(result.entries[0].navigation.status == iggy::NpcActorNavigationRequest2DStatus::NavigationRejected, "blocked destination should preserve rejected navigation");
}

void TestPathNotFoundPreparesNoRequest()
{
	const iggy::LevelTileMap map = Map({ ".#." });
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:no-path", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = Controls({
		Control(
			"npc:no-path",
			iggy::moveToNpcObjective({ 2.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 2.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
	});

	const iggy::NpcActorMovementFramePlan2DResult result = Plan(actors, controls, map);

	Expect(result.status == iggy::NpcActorMovementFramePlan2DStatus::NoRequests, "no-path route should produce no request");
	Expect(result.entries[0].status == iggy::NpcActorMovementFramePlan2DEntryStatus::PathNotFound, "unreachable target should fail at path stage");
	Expect(result.pathFailedCount == 1, "unreachable target should increment path failure count");
	Expect(result.entries[0].navigation.status == iggy::NpcActorNavigationRequest2DStatus::Ready, "unreachable target should preserve accepted navigation");
	Expect(result.entries[0].path.status == iggy::NpcActorPathReport2DStatus::PathNotFound, "unreachable target should preserve path failure diagnostics");
}

void TestIdleAndMissingControlPrepareNoRequests()
{
	const iggy::LevelTileMap map = Map({ "..." });
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:idle", { 0.5F, 0.5F }),
		Actor("npc:missing-control", { 1.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = Controls({
		Control("npc:idle", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still),
	});

	const iggy::NpcActorMovementFramePlan2DResult result = Plan(actors, controls, map);

	Expect(result.status == iggy::NpcActorMovementFramePlan2DStatus::NoRequests, "idle and missing control should produce no requests");
	Expect(result.entryCount == 2, "idle and missing control should preserve both frame entries");
	Expect(result.noMovementIntentCount == 2, "idle and missing control should count no movement intents");
	Expect(result.frameState.issues.size() == 1, "missing control issue should be preserved in frame state");
	Expect(result.movementIntents.missingControlCount == 1, "missing control should be preserved in movement intent diagnostics");
	Expect(result.entries[0].status == iggy::NpcActorMovementFramePlan2DEntryStatus::NoMovementIntent, "idle entry should stop at movement intent stage");
	Expect(result.entries[1].status == iggy::NpcActorMovementFramePlan2DEntryStatus::NoMovementIntent, "missing-control entry should stop at movement intent stage");
}

void TestRequestOrderFollowsMovementEntryOrder()
{
	const iggy::LevelTileMap map = Map({ "....." });
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:first", { 0.5F, 0.5F }),
		Actor("npc:idle-between", { 2.5F, 0.5F }),
		Actor("npc:second", { 4.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = Controls({
		Control(
			"npc:first",
			iggy::moveToNpcObjective({ 1.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 1.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
		Control("npc:idle-between", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still),
		Control(
			"npc:second",
			iggy::moveToNpcObjective({ 3.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
	});

	const iggy::NpcActorMovementFramePlan2DResult result = Plan(actors, controls, map);

	Expect(result.entryCount == 3 && result.requestCount == 2, "planner should preserve entries and prepare only moving requests");
	Expect(result.entries[0].requestPrepared && result.entries[2].requestPrepared, "first and third entries should prepare requests");
	Expect(*result.entries[0].requestIndex == 0 && *result.entries[2].requestIndex == 1, "request indexes should follow entry order");
	Expect(result.requests[0].filter.step.npcId == Id("npc:first"), "first request should come from first moving entry");
	Expect(result.requests[1].filter.step.npcId == Id("npc:second"), "second request should come from later moving entry");
}

void TestReservationRejectsSecondDifferentNpcSameDestination()
{
	const iggy::LevelTileMap map = Map({ "..." });
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:first", { 0.5F, 0.5F }),
		Actor("npc:second", { 2.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = Controls({
		Control(
			"npc:first",
			iggy::moveToNpcObjective({ 1.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 1.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
		Control(
			"npc:second",
			iggy::moveToNpcObjective({ 1.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 1.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
	});
	iggy::NpcActorMovementFramePlan2DConfig config;
	config.runReservation = true;

	const iggy::NpcActorMovementFramePlan2DResult result = Plan(actors, controls, map, config);
	const iggy::NpcActorMovementFrameApply2DResult applied =
		iggy::NpcActorMovementFrameApplier2D {}.apply(actors, result.requests);

	Expect(result.preReservationRequestCount == 2, "reservation planner should preserve pre-reservation requests");
	Expect(result.requestCount == 1, "default reservation should expose accepted-only final requests");
	Expect(result.reservationAcceptedCount == 1 && result.reservationRejectedCount == 1, "default reservation should preserve accept/reject counts");
	Expect(result.reservation.entries[1].status == iggy::NpcActorMovementReservationEntry2DStatus::ReservationBlocked, "default reservation should reject second same-tile npc");
	Expect(result.entries[0].requestIndex.has_value() && *result.entries[0].requestIndex == 0, "accepted entry should preserve final request index");
	Expect(!result.entries[1].requestIndex.has_value(), "reservation-rejected entry should not have final request index");
	Expect(result.entries[1].preReservationRequestIndex.has_value() && *result.entries[1].preReservationRequestIndex == 1, "reservation-rejected entry should preserve pre-reservation index");
	Expect(applied.movedCount == 1, "accepted-only planner requests should feed existing applier");
	Expect(NearVec(applied.registry.actors[0].position, { 1.5F, 0.5F }), "first reserved actor should move");
	Expect(NearVec(applied.registry.actors[1].position, { 2.5F, 0.5F }), "reservation-rejected actor should not move");
}

void TestReservationCapacityTwoAllowsTwoDifferentNpcsSameDestination()
{
	const iggy::LevelTileMap map = Map({ "..." });
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:first", { 0.5F, 0.5F }),
		Actor("npc:second", { 2.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = Controls({
		Control(
			"npc:first",
			iggy::moveToNpcObjective({ 1.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 1.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
		Control(
			"npc:second",
			iggy::moveToNpcObjective({ 1.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 1.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
	});
	iggy::NpcActorMovementFramePlan2DConfig config;
	config.runReservation = true;
	config.reservation.policy.maxOccupantsPerTile = 2;

	const iggy::NpcActorMovementFramePlan2DResult result = Plan(actors, controls, map, config);
	const iggy::NpcActorMovementFrameApply2DResult applied =
		iggy::NpcActorMovementFrameApplier2D {}.apply(actors, result.requests);
	const iggy::NpcActorOccupancy2D refreshed = iggy::NpcActorOccupancyProjector2D {}.project(applied.registry);

	Expect(result.preReservationRequestCount == 2 && result.requestCount == 2, "capacity reservation should expose both final requests");
	Expect(result.reservationAcceptedCount == 2 && result.reservationRejectedCount == 0, "capacity reservation should accept both requests");
	Expect(applied.movedCount == 2, "capacity reservation requests should move both actors");
	Expect(NearVec(applied.registry.actors[0].position, { 1.5F, 0.5F }), "first capacity actor should move");
	Expect(NearVec(applied.registry.actors[1].position, { 1.5F, 0.5F }), "second capacity actor should move");
	Expect(refreshed.hasIssues(), "capacity final registry should project duplicate occupancy inspectably");
	Expect(npcActorOccupantsAt(refreshed, { 1, 0 }).npcIds.size() == 2, "capacity final occupancy should contain both actors");
}

void TestReservationPreservesBlockedDiagnosticsInFinalRequests()
{
	const iggy::LevelTileMap map = Map({ "...." });
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = Controls({
		Control(
			"npc:mover",
			iggy::moveToNpcObjective({ 3.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
		Control("npc:blocker", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still),
	});
	iggy::NpcActorMovementFramePlan2DConfig config;
	config.runReservation = true;

	const iggy::NpcActorMovementFramePlan2DResult result = Plan(actors, controls, map, config);
	const iggy::NpcActorMovementFrameApply2DResult applied =
		iggy::NpcActorMovementFrameApplier2D {}.apply(actors, result.requests);

	Expect(result.preReservationRequestCount == 1 && result.requestCount == 1, "reservation should pass blocked diagnostics through as final request");
	Expect(result.reservationAcceptedCount == 1 && result.reservationRejectedCount == 0, "blocked non-movement request should not be reservation-rejected");
	Expect(result.requests[0].filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "final request should preserve blocked filter");
	Expect(applied.blockedCount == 1 && !applied.changed, "blocked final request should feed existing applier diagnostics without movement");
}

void TestReservationSameNpcDuplicateDeterministicFromRawRegistry()
{
	const iggy::LevelTileMap map = Map({ "..." });
	iggy::NpcActorState2DRegistry actors;
	actors.actors = {
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:mover", { 2.5F, 0.5F }),
	};
	iggy::NpcActorControlState2DRegistry controls;
	controls.entries = {
		Control(
			"npc:mover",
			iggy::moveToNpcObjective({ 1.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 1.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
	};
	iggy::NpcActorMovementFramePlan2DConfig config;
	config.runReservation = true;

	const iggy::NpcActorMovementFramePlan2DResult result = Plan(actors, controls, map, config);

	Expect(result.preReservationRequestCount == 2, "raw duplicate same-npc registry should produce two requests for deterministic reservation coverage");
	Expect(result.requestCount == 2, "same-npc duplicate reservations should remain accepted");
	Expect(result.reservationRejectedCount == 0, "same-npc duplicate reservations should not reject each other");
	Expect(result.entries[0].requestIndex.has_value() && *result.entries[0].requestIndex == 0, "first same-npc entry should keep final request index");
	Expect(result.entries[1].requestIndex.has_value() && *result.entries[1].requestIndex == 1, "second same-npc entry should keep final request index");
}

void TestGeneratedRequestsCanFeedFrameApplierWithoutPlannerApplying()
{
	const iggy::LevelTileMap map = Map({ "...." });
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:apply-check", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = Controls({
		Control(
			"npc:apply-check",
			iggy::moveToNpcObjective({ 3.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
	});

	const iggy::NpcActorMovementFramePlan2DResult result = Plan(actors, controls, map);
	const iggy::NpcActorMovementFrameApply2DResult applied =
		iggy::NpcActorMovementFrameApplier2D {}.apply(actors, result.requests);

	Expect(result.requestCount == 1, "planner should generate one applier request");
	Expect(NearVec(actors.actors[0].position, { 0.5F, 0.5F }), "planner should not apply movement itself");
	Expect(applied.changed && applied.movedCount == 1, "generated request should feed existing applier");
	Expect(NearVec(applied.registry.actors[0].position, result.requests[0].filter.step.proposedPosition), "applier should use planner request proposed position");
}

void TestInputsAreNotMutated()
{
	const iggy::LevelTileMap map = Map({ "...." });
	const iggy::LevelTileMap mapBefore = map;
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:immutable", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorState2DRegistry actorsBefore = actors;
	const iggy::NpcActorControlState2DRegistry controls = Controls({
		Control(
			"npc:immutable",
			iggy::moveToNpcObjective({ 3.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
	});
	const iggy::NpcActorControlState2DRegistry controlsBefore = controls;

	(void)Plan(actors, controls, map);

	Expect(SameActors(actors, actorsBefore), "planner should not mutate actor registry");
	Expect(SameControls(controls, controlsBefore), "planner should not mutate control registry");
	Expect(SameMap(map, mapBefore), "planner should not mutate level map");
}

} // namespace

int main()
{
	TestEmptyRegistriesProduceNoRequests();
	TestSeekingActorPreparesAllowedRequest();
	TestFleeingActorPreparesRequestThroughEscapeRoute();
	TestOccupancyBlockedStepStillPreparesRequest();
	TestDefaultConfigStillUsesHardBlockFilter();
	TestOccupancyPolicyCapacityAllowsPreviouslyBlockedStep();
	TestOccupancyPolicyDefaultCapacityMatchesHardBlock();
	TestNavigationRejectedPreparesNoRequest();
	TestPathNotFoundPreparesNoRequest();
	TestIdleAndMissingControlPrepareNoRequests();
	TestRequestOrderFollowsMovementEntryOrder();
	TestReservationRejectsSecondDifferentNpcSameDestination();
	TestReservationCapacityTwoAllowsTwoDifferentNpcsSameDestination();
	TestReservationPreservesBlockedDiagnosticsInFinalRequests();
	TestReservationSameNpcDuplicateDeterministicFromRawRegistry();
	TestGeneratedRequestsCanFeedFrameApplierWithoutPlannerApplying();
	TestInputsAreNotMutated();

	return Failures;
}
