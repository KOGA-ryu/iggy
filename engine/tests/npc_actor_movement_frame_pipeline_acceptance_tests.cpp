#include <cstdlib>
#include <string_view>
#include <vector>

#include "scene/npc/NpcActorEscapeRouteTarget2D.hpp"
#include "scene/npc/NpcActorMovementFrameApply2D.hpp"
#include "scene/npc/NpcActorMovementFrameIntent2D.hpp"
#include "scene/npc/NpcActorMovementFrameReport2D.hpp"
#include "scene/npc/NpcActorNavigationRequest2D.hpp"
#include "scene/npc/NpcActorOccupancy2D.hpp"
#include "scene/npc/NpcActorPathReport2D.hpp"
#include "scene/npc/NpcActorPathStep2D.hpp"
#include "scene/npc/NpcActorPathStepOccupancyFilter2D.hpp"
#include "scene/npc/NpcActorRouteTarget2D.hpp"
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

iggy::LevelTileMap MapFromRows(std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map = iggy::test::MapFromRows(rows);
	map.id = Id("level:movement-frame-acceptance");
	return map;
}

iggy::NpcActorState2D Actor(
	const char *npcId,
	iggy::Vec2 position,
	bool present = true)
{
	return {
		Id(npcId),
		Id("profile:guard"),
		Id("faction:town"),
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

iggy::NpcActorState2DRegistry ActorRegistry(std::vector<iggy::NpcActorState2D> actors)
{
	return iggy::NpcActorState2DRegistryBuilder {}.build(actors).registry;
}

iggy::NpcActorControlState2DRegistry ControlRegistry(std::vector<iggy::NpcActorControlState2D> controls)
{
	return iggy::NpcActorControlState2DRegistryBuilder {}.build(controls).registry;
}

bool SameMap(const iggy::LevelTileMap &actual, const iggy::LevelTileMap &expected)
{
	if (actual.id != expected.id
		|| actual.width != expected.width
		|| actual.height != expected.height
		|| actual.tiles.size() != expected.tiles.size()
		|| actual.entitySpawns.size() != expected.entitySpawns.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.tiles.size(); ++index) {
		if (actual.tiles[index].walkable != expected.tiles[index].walkable) {
			return false;
		}
	}
	return true;
}

bool SameActorRegistry(
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

bool SameControlRegistry(
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

bool AllRefreshFlags(const iggy::NpcActorMovementFrameReport2D &report)
{
	return report.needsOccupancyRebuild
		&& report.needsAiMapQueryRefresh
		&& report.needsInteractionRefresh
		&& report.needsRenderRefresh
		&& report.needsVisibilityRefresh;
}

bool NoRefreshFlags(const iggy::NpcActorMovementFrameReport2D &report)
{
	return !report.needsOccupancyRebuild
		&& !report.needsAiMapQueryRefresh
		&& !report.needsInteractionRefresh
		&& !report.needsRenderRefresh
		&& !report.needsVisibilityRefresh;
}

void ExpectEvents(
	const std::vector<iggy::NpcActorMovementFrameEvent2D> &actual,
	const std::vector<iggy::NpcActorMovementFrameEvent2D> &expected,
	const char *message)
{
	Expect(actual.size() == expected.size(), message);
	if (actual.size() != expected.size()) {
		return;
	}
	for (std::size_t index = 0; index < actual.size(); ++index) {
		Expect(actual[index] == expected[index], message);
	}
}

struct MovementRoad {
	iggy::NpcActorFrameState2DProjectionResult frameState;
	iggy::NpcActorMovementFrameIntent2DResult movement;
	iggy::NpcActorRouteTarget2D route;
	iggy::NpcActorEscapeRouteTarget2D escapeRoute;
	iggy::NpcActorNavigationRequest2D navigation;
	iggy::NpcActorPathReport2D path;
	iggy::NpcActorPathStep2D step;
	iggy::NpcActorOccupancy2D occupancy;
	iggy::NpcActorPathStepOccupancyFilter2D filter;
};

MovementRoad SeekingRoad(
	const iggy::NpcActorState2DRegistry &actors,
	const iggy::NpcActorControlState2DRegistry &controls,
	const iggy::LevelTileMap &map,
	std::size_t movementIndex,
	const iggy::NpcActorState2DRegistry &occupancyActors)
{
	MovementRoad road;
	road.frameState = iggy::NpcActorFrameStateProjector2D {}.project(actors, controls);
	road.movement = iggy::NpcActorMovementFrameIntentProjector2D {}.project(road.frameState);
	road.route = iggy::NpcActorRouteTargetProjector2D {}.project(road.movement.entries[movementIndex].intent);
	road.navigation = iggy::NpcActorNavigationRequestBuilder2D {}.build(road.route, map);
	road.path = iggy::NpcActorPathReporter2D {}.findPath(road.navigation, map);
	road.step = iggy::NpcActorPathStepper2D {}.step(road.path);
	road.occupancy = iggy::NpcActorOccupancyProjector2D {}.project(occupancyActors);
	road.filter = iggy::NpcActorPathStepOccupancyFilterProjector2D {}.filter(road.step, road.occupancy);
	return road;
}

MovementRoad SeekingRoad(
	const iggy::NpcActorState2DRegistry &actors,
	const iggy::NpcActorControlState2DRegistry &controls,
	const iggy::LevelTileMap &map,
	std::size_t movementIndex = 0)
{
	return SeekingRoad(actors, controls, map, movementIndex, actors);
}

MovementRoad FleeingRoad(
	const iggy::NpcActorState2DRegistry &actors,
	const iggy::NpcActorControlState2DRegistry &controls,
	const iggy::LevelTileMap &map,
	const iggy::NpcActorState2DRegistry &occupancyActors)
{
	MovementRoad road;
	road.frameState = iggy::NpcActorFrameStateProjector2D {}.project(actors, controls);
	road.movement = iggy::NpcActorMovementFrameIntentProjector2D {}.project(road.frameState);
	road.escapeRoute = iggy::NpcActorEscapeRouteTargetProjector2D {}.project(road.movement.entries[0].intent, map);
	road.navigation = iggy::NpcActorNavigationRequestBuilder2D {}.build(road.escapeRoute.route, map);
	road.path = iggy::NpcActorPathReporter2D {}.findPath(road.navigation, map);
	road.step = iggy::NpcActorPathStepper2D {}.step(road.path);
	road.occupancy = iggy::NpcActorOccupancyProjector2D {}.project(occupancyActors);
	road.filter = iggy::NpcActorPathStepOccupancyFilterProjector2D {}.filter(road.step, road.occupancy);
	return road;
}

iggy::NpcActorMovementFrameApply2DRequest Request(const iggy::NpcActorPathStepOccupancyFilter2D &filter)
{
	iggy::NpcActorMovementFrameApply2DRequest request;
	request.filter = filter;
	return request;
}

iggy::NpcActorMovementFrameReport2D ApplyAndReport(
	const iggy::NpcActorState2DRegistry &actors,
	const std::vector<iggy::NpcActorPathStepOccupancyFilter2D> &filters)
{
	std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests;
	for (const iggy::NpcActorPathStepOccupancyFilter2D &filter : filters) {
		requests.push_back(Request(filter));
	}

	const iggy::NpcActorMovementFrameApply2DResult apply =
		iggy::NpcActorMovementFrameApplier2D {}.apply(actors, requests);
	return iggy::NpcActorMovementFrameReporter2D {}.report(apply);
}

void TestAllowedSeekingMovementUpdatesFinalRegistryByValue()
{
	const iggy::LevelTileMap map = MapFromRows({ "...." });
	const iggy::LevelTileMap mapBefore = map;
	const iggy::NpcActorState2DRegistry actors = ActorRegistry({
		Actor("npc:mover", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = ControlRegistry({
		Control("npc:mover", iggy::moveToNpcObjective({ 3.5F, 0.5F }), iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }), iggy::NpcMoveMode::Walk),
	});
	const iggy::NpcActorState2DRegistry actorsBefore = actors;
	const iggy::NpcActorControlState2DRegistry controlsBefore = controls;

	const MovementRoad road = SeekingRoad(actors, controls, map);
	const iggy::NpcActorMovementFrameReport2D report = ApplyAndReport(actors, { road.filter });

	Expect(road.frameState.entries.size() == 1 && road.frameState.entries[0].hasControl, "allowed road should join actor/control");
	Expect(road.movement.readyCount == 1, "allowed road should produce ready movement intent");
	Expect(road.route.ready(), "allowed road should produce ready route target");
	Expect(road.navigation.ready(), "allowed road should validate navigation");
	Expect(road.path.hasPath(), "allowed road should find path");
	Expect(road.step.status == iggy::NpcActorPathStep2DStatus::Proposed, "allowed road should propose path step");
	Expect(road.filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed, "allowed road should pass occupancy filter");
	Expect(report.apply.status == iggy::NpcActorMovementFrameApply2DStatus::Applied, "allowed road should apply movement");
	Expect(report.movedCount == 1 && report.changedCount == 1, "allowed report should count moved and changed actor");
	Expect(report.blockedCount == 0 && report.rejectedCount == 0 && report.missingActorCount == 0, "allowed report should not count failures");
	Expect(NearVec(report.registry.actors[0].position, road.step.proposedPosition), "allowed report registry should move actor to proposed position");
	Expect(NearVec(actors.actors[0].position, { 0.5F, 0.5F }), "allowed road should leave input actor registry unchanged");
	Expect(report.dirtyTiles.size() == 2, "allowed report should aggregate old/new dirty tiles");
	Expect(report.dirtyTiles[0] == iggy::TileCoord { 0, 0 }, "allowed report should dirty old tile first");
	Expect(report.dirtyTiles[1] == iggy::TileCoord { 1, 0 }, "allowed report should dirty new tile second");
	Expect(AllRefreshFlags(report), "allowed report should mark all refresh flags");
	Expect(SameActorRegistry(actors, actorsBefore), "allowed road should not mutate actor registry");
	Expect(SameControlRegistry(controls, controlsBefore), "allowed road should not mutate control registry");
	Expect(SameMap(map, mapBefore), "allowed road should not mutate map");
}

void TestBlockedByNpcPreservesRegistryAndBlockerFacts()
{
	const iggy::LevelTileMap map = MapFromRows({ "...." });
	const iggy::NpcActorState2DRegistry actors = ActorRegistry({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = ControlRegistry({
		Control("npc:mover", iggy::moveToNpcObjective({ 3.5F, 0.5F }), iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }), iggy::NpcMoveMode::Walk),
		Control("npc:blocker", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still),
	});
	const iggy::NpcActorState2DRegistry actorsBefore = actors;

	const MovementRoad road = SeekingRoad(actors, controls, map, 0);
	const iggy::NpcActorMovementFrameReport2D report = ApplyAndReport(actors, { road.filter });

	Expect(road.step.status == iggy::NpcActorPathStep2DStatus::Proposed, "blocked road should still propose path step before occupancy filter");
	Expect(road.filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "blocked road should block at occupancy filter");
	Expect(road.filter.blockingNpcId == Id("npc:blocker"), "blocked filter should preserve blocker id");
	Expect(report.movedCount == 0 && report.blockedCount == 1, "blocked report should count one blocked movement");
	Expect(report.apply.entries[0].executor.postMove.status == iggy::NpcActorPostMoveReport2DStatus::Blocked, "blocked report should preserve blocked post-move status");
	Expect(report.apply.entries[0].executor.postMove.blockingNpcId == Id("npc:blocker"), "blocked report should preserve blocker through executor post-move");
	Expect(SameActorRegistry(report.registry, actorsBefore), "blocked road should preserve final registry");
	Expect(report.dirtyTiles.empty(), "blocked report should not aggregate dirty tiles");
	Expect(NoRefreshFlags(report), "blocked report should not request refreshes");
	ExpectEvents(
		report.events,
		{
			iggy::NpcActorMovementFrameEvent2D::MovementBlocked,
			iggy::NpcActorMovementFrameEvent2D::MovementFrameUnchanged,
		},
		"blocked report should preserve blocked and unchanged events");
}

void TestMixedFrameMovesOnlySuccessfulNpcAndReportsPhaseOrderedEvents()
{
	const iggy::LevelTileMap map = MapFromRows({ "....." });
	const iggy::NpcActorState2DRegistry actors = ActorRegistry({
		Actor("npc:move", { 0.5F, 0.5F }),
		Actor("npc:blocked", { 3.5F, 0.5F }),
		Actor("npc:blocker", { 2.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = ControlRegistry({
		Control("npc:move", iggy::moveToNpcObjective({ 1.5F, 0.5F }), iggy::seekingNpcBehaviorState({ 1.5F, 0.5F }), iggy::NpcMoveMode::Walk),
		Control("npc:blocked", iggy::moveToNpcObjective({ 0.5F, 0.5F }), iggy::seekingNpcBehaviorState({ 0.5F, 0.5F }), iggy::NpcMoveMode::Walk),
		Control("npc:blocker", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still),
	});

	const MovementRoad moveRoad = SeekingRoad(actors, controls, map, 0);
	const MovementRoad blockedRoad = SeekingRoad(actors, controls, map, 1);
	const iggy::NpcActorMovementFrameReport2D report = ApplyAndReport(actors, { moveRoad.filter, blockedRoad.filter });

	Expect(moveRoad.filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed, "mixed frame first actor should be allowed");
	Expect(blockedRoad.filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "mixed frame second actor should be blocked");
	Expect(report.movedCount == 1 && report.blockedCount == 1 && report.changedCount == 1, "mixed frame should count one move and one block");
	Expect(NearVec(report.registry.actors[0].position, moveRoad.step.proposedPosition), "mixed frame should move only successful actor");
	Expect(NearVec(report.registry.actors[1].position, actors.actors[1].position), "mixed frame should keep blocked actor position");
	Expect(NearVec(report.registry.actors[2].position, actors.actors[2].position), "mixed frame should keep blocking actor position");
	Expect(report.dirtyTiles.size() == 2, "mixed frame should aggregate dirty tiles from moved entry only");
	Expect(AllRefreshFlags(report), "mixed frame should request refresh because one actor moved");
	ExpectEvents(
		report.events,
		{
			iggy::NpcActorMovementFrameEvent2D::MovementApplied,
			iggy::NpcActorMovementFrameEvent2D::MovementBlocked,
			iggy::NpcActorMovementFrameEvent2D::DirtyTileObserved,
			iggy::NpcActorMovementFrameEvent2D::DirtyTileObserved,
			iggy::NpcActorMovementFrameEvent2D::RefreshNeeded,
			iggy::NpcActorMovementFrameEvent2D::RefreshNeeded,
			iggy::NpcActorMovementFrameEvent2D::RefreshNeeded,
			iggy::NpcActorMovementFrameEvent2D::RefreshNeeded,
			iggy::NpcActorMovementFrameEvent2D::RefreshNeeded,
			iggy::NpcActorMovementFrameEvent2D::MovementFrameChanged,
		},
		"mixed frame should preserve deterministic event phase order");
}

void TestFleeingMovementPathAppliesAndReportsPostMoveFacts()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
		"...",
	});
	const iggy::NpcActorState2DRegistry actors = ActorRegistry({
		Actor("npc:flee", { 1.5F, 1.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = ControlRegistry({
		Control("npc:flee", iggy::fleeNpcObjective({ 0.5F, 1.5F }), iggy::fleeingNpcBehaviorState({ 0.5F, 1.5F }), iggy::NpcMoveMode::Run),
	});

	const MovementRoad road = FleeingRoad(actors, controls, map, actors);
	const iggy::NpcActorMovementFrameReport2D report = ApplyAndReport(actors, { road.filter });

	Expect(road.escapeRoute.ready(), "fleeing road should produce ready escape route");
	Expect(road.filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed, "fleeing road should pass occupancy filter");
	Expect(road.step.proposedPosition.x > road.step.oldPosition.x, "fleeing step should move away from left-side threat where practical");
	Expect(report.movedCount == 1 && report.changedCount == 1, "fleeing report should count moved actor");
	Expect(report.registry.actors[0].position.x > actors.actors[0].position.x, "fleeing final registry should move actor away from threat");
	Expect(report.apply.entries[0].executor.postMove.status == iggy::NpcActorPostMoveReport2DStatus::Moved, "fleeing report should preserve moved post-move facts");
	Expect(AllRefreshFlags(report), "fleeing report should mark refresh flags");
}

void TestMissingActorStaleRequestReportsIssueAndContinues()
{
	const iggy::LevelTileMap map = MapFromRows({ "...." });
	const iggy::NpcActorState2DRegistry applyActors = ActorRegistry({
		Actor("npc:mover", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry applyControls = ControlRegistry({
		Control("npc:mover", iggy::moveToNpcObjective({ 3.5F, 0.5F }), iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }), iggy::NpcMoveMode::Walk),
	});
	const iggy::NpcActorState2DRegistry staleActors = ActorRegistry({
		Actor("npc:stale", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry staleControls = ControlRegistry({
		Control("npc:stale", iggy::moveToNpcObjective({ 3.5F, 0.5F }), iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }), iggy::NpcMoveMode::Walk),
	});

	const MovementRoad staleRoad = SeekingRoad(staleActors, staleControls, map);
	const MovementRoad validRoad = SeekingRoad(applyActors, applyControls, map);
	const iggy::NpcActorMovementFrameReport2D report = ApplyAndReport(applyActors, { staleRoad.filter, validRoad.filter });

	Expect(report.missingActorCount == 1, "stale request should count missing actor");
	Expect(report.movedCount == 1 && report.changedCount == 1, "valid request should continue after stale request");
	Expect(report.apply.entries[0].hasIssue, "stale request should preserve actor-not-found entry issue");
	Expect(report.apply.entries[0].issue == iggy::NpcActorMovementFrameApply2DIssueCode::ActorNotFound, "stale request should use ActorNotFound issue");
	Expect(report.events[0] == iggy::NpcActorMovementFrameEvent2D::ActorNotFound, "stale request should emit ActorNotFound event first");
	Expect(report.events[1] == iggy::NpcActorMovementFrameEvent2D::MovementApplied, "valid request should still emit applied event");
	Expect(NearVec(report.registry.actors[0].position, validRoad.step.proposedPosition), "valid request after stale should update registry");
}

void TestDuplicateSameNpcSequentialRequestsAndFailurePreserveCarriedState()
{
	const iggy::LevelTileMap map = MapFromRows({ "...." });
	const iggy::NpcActorState2DRegistry initialActors = ActorRegistry({
		Actor("npc:mover", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry firstControls = ControlRegistry({
		Control("npc:mover", iggy::moveToNpcObjective({ 3.5F, 0.5F }), iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }), iggy::NpcMoveMode::Walk),
	});
	const MovementRoad firstRoad = SeekingRoad(initialActors, firstControls, map);

	const iggy::NpcActorState2DRegistry carriedActors = ActorRegistry({
		Actor("npc:mover", firstRoad.step.proposedPosition),
	});
	const iggy::NpcActorControlState2DRegistry secondControls = ControlRegistry({
		Control("npc:mover", iggy::moveToNpcObjective({ 3.5F, 0.5F }), iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }), iggy::NpcMoveMode::Walk),
	});
	const MovementRoad secondRoad = SeekingRoad(carriedActors, secondControls, map);
	const iggy::NpcActorMovementFrameReport2D successReport = ApplyAndReport(initialActors, { firstRoad.filter, secondRoad.filter });

	const iggy::NpcActorState2DRegistry blockedOccupancy = ActorRegistry({
		Actor("npc:mover", firstRoad.step.proposedPosition),
		Actor("npc:blocker", secondRoad.step.proposedPosition),
	});
	const MovementRoad blockedSecondRoad = SeekingRoad(carriedActors, secondControls, map, 0, blockedOccupancy);
	const iggy::NpcActorMovementFrameReport2D blockedReport = ApplyAndReport(initialActors, { firstRoad.filter, blockedSecondRoad.filter });

	Expect(successReport.movedCount == 2 && successReport.changedCount == 2, "duplicate successful requests should both move");
	Expect(NearVec(successReport.registry.actors[0].position, secondRoad.step.proposedPosition), "later successful duplicate should win final position");
	Expect(NearVec(successReport.apply.entries[1].executor.actor.position, firstRoad.step.proposedPosition), "later duplicate should see carried registry position");
	Expect(blockedReport.movedCount == 1 && blockedReport.blockedCount == 1, "failed later duplicate should count move then block");
	Expect(NearVec(blockedReport.registry.actors[0].position, firstRoad.step.proposedPosition), "failed later duplicate should not erase earlier movement");
	Expect(blockedReport.apply.entries[1].executor.status == iggy::NpcActorMovementExecutor2DStatus::BlockedByNpc, "failed later duplicate should preserve blocked executor status");
}

} // namespace

int main()
{
	TestAllowedSeekingMovementUpdatesFinalRegistryByValue();
	TestBlockedByNpcPreservesRegistryAndBlockerFacts();
	TestMixedFrameMovesOnlySuccessfulNpcAndReportsPhaseOrderedEvents();
	TestFleeingMovementPathAppliesAndReportsPostMoveFacts();
	TestMissingActorStaleRequestReportsIssueAndContinues();
	TestDuplicateSameNpcSequentialRequestsAndFailurePreserveCarriedState();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
