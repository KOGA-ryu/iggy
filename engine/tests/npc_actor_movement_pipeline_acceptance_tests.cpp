#include <cstdlib>
#include <string_view>
#include <vector>

#include "scene/npc/NpcActorEscapeRouteTarget2D.hpp"
#include "scene/npc/NpcActorMovementFrameIntent2D.hpp"
#include "scene/npc/NpcActorNavigationRequest2D.hpp"
#include "scene/npc/NpcActorOccupancyQuery2D.hpp"
#include "scene/npc/NpcActorPathReport2D.hpp"
#include "scene/npc/NpcActorPathStep2D.hpp"
#include "scene/npc/NpcActorRouteTarget2D.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;
using iggy::test::SameTile;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::LevelTileMap MapFromRows(std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map = iggy::test::MapFromRows(rows);
	map.id = Id("level:acceptance");
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
			|| left.moveMode != right.moveMode) {
			return false;
		}
	}
	return true;
}

iggy::NpcActorFrameState2DProjectionResult ProjectFrames(
	const iggy::NpcActorState2DRegistry &actors,
	const iggy::NpcActorControlState2DRegistry &controls)
{
	return iggy::NpcActorFrameStateProjector2D {}.project(actors, controls);
}

iggy::NpcActorMovementFrameIntent2DResult ProjectMovementFrame(
	const iggy::NpcActorFrameState2DProjectionResult &frames)
{
	return iggy::NpcActorMovementFrameIntentProjector2D {}.project(frames);
}

iggy::NpcActorPathStep2D StepDirectMoveTo(
	const iggy::NpcActorMovementIntent2D &intent,
	const iggy::LevelTileMap &map,
	const iggy::NpcActorPathStep2DConfig &stepConfig = {})
{
	const iggy::NpcActorRouteTarget2D route =
		iggy::NpcActorRouteTargetProjector2D {}.project(intent);
	const iggy::NpcActorNavigationRequest2D navigation =
		iggy::NpcActorNavigationRequestBuilder2D {}.build(route, map);
	const iggy::NpcActorPathReport2D path =
		iggy::NpcActorPathReporter2D {}.findPath(navigation, map);
	return iggy::NpcActorPathStepper2D {}.step(path, stepConfig);
}

void TestSeekingActorRoadComposesThroughPathStep()
{
	const iggy::LevelTileMap map = MapFromRows({ "...." });
	const iggy::LevelTileMap mapBefore = map;
	const iggy::NpcActorState2DRegistry actors = ActorRegistry({
		Actor("npc:seeker", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = ControlRegistry({
		Control(
			"npc:seeker",
			iggy::moveToNpcObjective({ 3.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
	});
	const iggy::NpcActorState2DRegistry actorsBefore = actors;
	const iggy::NpcActorControlState2DRegistry controlsBefore = controls;

	const iggy::NpcActorFrameState2DProjectionResult frames = ProjectFrames(actors, controls);
	const iggy::NpcActorMovementFrameIntent2DResult intents = ProjectMovementFrame(frames);
	const iggy::NpcActorPathStep2D step = StepDirectMoveTo(intents.entries[0].intent, map);

	Expect(frames.entries.size() == 1 && frames.entries[0].hasControl, "seeking pipeline should join actor/control frame");
	Expect(intents.status == iggy::NpcActorMovementFrameIntent2DStatus::Projected, "seeking pipeline should project ready frame movement");
	Expect(intents.readyCount == 1, "seeking pipeline should have one ready intent");
	Expect(intents.entries[0].intent.type == iggy::NpcActorMovementIntent2DType::MoveTo, "seeking pipeline should produce MoveTo intent");
	Expect(step.status == iggy::NpcActorPathStep2DStatus::Proposed, "seeking pipeline should propose path step");
	Expect(step.requestsMovement, "seeking pipeline should request movement");
	Expect(step.npcId == Id("npc:seeker"), "seeking pipeline should preserve npc id");
	Expect(NearVec(step.oldPosition, { 0.5F, 0.5F }), "seeking pipeline should preserve actor start");
	Expect(NearVec(step.proposedPosition, { 1.5F, 0.5F }), "seeking pipeline should propose first bounded waypoint step");
	Expect(SameTile(step.oldTile, 0, 0), "seeking pipeline should derive old tile");
	Expect(SameTile(step.proposedTile, 1, 0), "seeking pipeline should derive proposed tile");
	Expect(SameActorRegistry(actors, actorsBefore), "seeking pipeline should not mutate actor registry");
	Expect(SameControlRegistry(controls, controlsBefore), "seeking pipeline should not mutate control registry");
	Expect(SameMap(map, mapBefore), "seeking pipeline should not mutate map");
}

void TestFleeingActorRoadComposesThroughEscapeRouteAndPathStep()
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
		Control(
			"npc:flee",
			iggy::fleeNpcObjective({ 0.5F, 1.5F }),
			iggy::fleeingNpcBehaviorState({ 0.5F, 1.5F }),
			iggy::NpcMoveMode::Run),
	});

	const iggy::NpcActorMovementFrameIntent2DResult intents =
		ProjectMovementFrame(ProjectFrames(actors, controls));
	const iggy::NpcActorEscapeRouteTarget2D escapeRoute =
		iggy::NpcActorEscapeRouteTargetProjector2D {}.project(intents.entries[0].intent, map);
	const iggy::NpcActorNavigationRequest2D navigation =
		iggy::NpcActorNavigationRequestBuilder2D {}.build(escapeRoute.route, map);
	const iggy::NpcActorPathReport2D path =
		iggy::NpcActorPathReporter2D {}.findPath(navigation, map);
	const iggy::NpcActorPathStep2D step =
		iggy::NpcActorPathStepper2D {}.step(path);

	Expect(intents.entries[0].intent.type == iggy::NpcActorMovementIntent2DType::MoveAwayFrom, "fleeing pipeline should preserve MoveAwayFrom intent");
	Expect(escapeRoute.status == iggy::NpcActorEscapeRouteTarget2DStatus::Ready, "fleeing pipeline should find an escape route target");
	Expect(escapeRoute.escape.ready(), "fleeing pipeline should preserve ready escape diagnostics");
	Expect(escapeRoute.route.ready(), "fleeing pipeline should preserve ready route diagnostics");
	Expect(NearVec(escapeRoute.escape.threatPosition, { 0.5F, 1.5F }), "fleeing pipeline should preserve threat/source position");
	Expect(NearVec(escapeRoute.route.targetPosition, escapeRoute.escape.escapePosition), "fleeing pipeline route should use selected escape position");
	Expect(navigation.ready(), "fleeing pipeline should validate navigation to selected escape target");
	Expect(path.hasPath(), "fleeing pipeline should find a path to selected escape target");
	Expect(step.status == iggy::NpcActorPathStep2DStatus::Proposed, "fleeing pipeline should propose path step");
	Expect(step.proposedPosition.x > step.oldPosition.x, "fleeing step should move away from left-side threat where practical");
	Expect(step.completedPath, "run step should complete the short local escape path");
}

void TestBlockedDestinationStopsBeforePathStepMovement()
{
	const iggy::LevelTileMap map = MapFromRows({
		".#",
	});
	const iggy::NpcActorState2DRegistry actors = ActorRegistry({
		Actor("npc:blocked", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = ControlRegistry({
		Control(
			"npc:blocked",
			iggy::moveToNpcObjective({ 1.5F, 0.5F }),
			iggy::seekingNpcBehaviorState({ 1.5F, 0.5F }),
			iggy::NpcMoveMode::Walk),
	});
	const iggy::NpcActorMovementFrameIntent2DResult intents =
		ProjectMovementFrame(ProjectFrames(actors, controls));
	const iggy::NpcActorRouteTarget2D route =
		iggy::NpcActorRouteTargetProjector2D {}.project(intents.entries[0].intent);
	const iggy::NpcActorNavigationRequest2D navigation =
		iggy::NpcActorNavigationRequestBuilder2D {}.build(route, map);
	const iggy::NpcActorPathReport2D path =
		iggy::NpcActorPathReporter2D {}.findPath(navigation, map);
	const iggy::NpcActorPathStep2D step =
		iggy::NpcActorPathStepper2D {}.step(path);

	Expect(route.ready(), "blocked destination setup should still produce a route target");
	Expect(navigation.status == iggy::NpcActorNavigationRequest2DStatus::NavigationRejected, "blocked destination should reject navigation request");
	Expect(path.status == iggy::NpcActorPathReport2DStatus::NoNavigationRequest, "rejected navigation should not run pathfinder");
	Expect(step.status == iggy::NpcActorPathStep2DStatus::NoPath, "blocked destination should produce NoPath path step");
	Expect(!step.requestsMovement, "blocked destination should not request movement");
}

void TestUnreachableDestinationStopsAtPathReport()
{
	const iggy::LevelTileMap map = MapFromRows({
		".#.",
		".#.",
		".#.",
	});
	const iggy::NpcActorState2DRegistry actors = ActorRegistry({
		Actor("npc:trapped", { 0.5F, 1.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = ControlRegistry({
		Control(
			"npc:trapped",
			iggy::moveToNpcObjective({ 2.5F, 1.5F }),
			iggy::seekingNpcBehaviorState({ 2.5F, 1.5F }),
			iggy::NpcMoveMode::Walk),
	});
	const iggy::NpcActorMovementFrameIntent2DResult intents =
		ProjectMovementFrame(ProjectFrames(actors, controls));
	const iggy::NpcActorRouteTarget2D route =
		iggy::NpcActorRouteTargetProjector2D {}.project(intents.entries[0].intent);
	const iggy::NpcActorNavigationRequest2D navigation =
		iggy::NpcActorNavigationRequestBuilder2D {}.build(route, map);
	const iggy::NpcActorPathReport2D path =
		iggy::NpcActorPathReporter2D {}.findPath(navigation, map);
	const iggy::NpcActorPathStep2D step =
		iggy::NpcActorPathStepper2D {}.step(path);

	Expect(navigation.ready(), "unreachable destination should pass static destination validation");
	Expect(path.status == iggy::NpcActorPathReport2DStatus::PathNotFound, "unreachable destination should fail at path report");
	Expect(path.path.status == iggy::navigation::NavigationPathStatus::NoPath, "unreachable destination should preserve NoPath diagnostic");
	Expect(step.status == iggy::NpcActorPathStep2DStatus::NoPath, "unreachable destination should produce NoPath path step");
	Expect(!step.requestsMovement, "unreachable destination should not request movement");
}

void TestMoveModeSpeedAffectsPathStepDistance()
{
	const iggy::LevelTileMap map = MapFromRows({ "...." });
	const iggy::NpcActorState2DRegistry actors = ActorRegistry({
		Actor("npc:walk", { 0.5F, 0.5F }),
		Actor("npc:run", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = ControlRegistry({
		Control("npc:walk", iggy::moveToNpcObjective({ 3.5F, 0.5F }), iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }), iggy::NpcMoveMode::Walk),
		Control("npc:run", iggy::moveToNpcObjective({ 3.5F, 0.5F }), iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }), iggy::NpcMoveMode::Run),
	});
	const iggy::NpcActorMovementFrameIntent2DResult intents =
		ProjectMovementFrame(ProjectFrames(actors, controls));
	const iggy::NpcActorPathStep2D walk = StepDirectMoveTo(intents.entries[0].intent, map);
	const iggy::NpcActorPathStep2D run = StepDirectMoveTo(intents.entries[1].intent, map);

	Expect(walk.status == iggy::NpcActorPathStep2DStatus::Proposed, "walk speed setup should propose movement");
	Expect(run.status == iggy::NpcActorPathStep2DStatus::Proposed, "run speed setup should propose movement");
	Expect(walk.maxDistance < run.maxDistance, "run should compute a larger max distance than walk");
	Expect(run.proposedPosition.x > walk.proposedPosition.x, "run should advance farther than walk on the same path");
	Expect(NearVec(walk.proposedPosition, { 1.5F, 0.5F }), "walk should advance one tile center");
	Expect(NearVec(run.proposedPosition, { 2.5F, 0.5F }), "run should advance two tile centers");
}

void TestOccupancyIsNotUsedByPurePathStepYet()
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
	const iggy::NpcActorMovementFrameIntent2DResult intents =
		ProjectMovementFrame(ProjectFrames(actors, controls));
	const iggy::NpcActorPathStep2D step = StepDirectMoveTo(intents.entries[0].intent, map);
	const iggy::NpcActorOccupancy2D occupancy =
		iggy::NpcActorOccupancyProjector2D {}.project(actors);
	const iggy::NpcActorOccupancyBlock2DResult block =
		iggy::npcActorTileBlockedFor(occupancy, Id("npc:mover"), step.proposedTile);

	Expect(block.blocked(), "acceptance setup should have another NPC occupying the proposed tile");
	Expect(block.status == iggy::NpcActorOccupancyBlock2DStatus::Blocked, "occupancy query should report the proposed tile blocked");
	Expect(step.status == iggy::NpcActorPathStep2DStatus::Proposed, "path step should still propose movement because occupancy filtering is not in this slice");
	Expect(step.requestsMovement, "path step should request movement despite occupancy until a dedicated filter layer is added");
	Expect(NearVec(step.proposedPosition, { 1.5F, 0.5F }), "occupancy acceptance should block the exact proposed tile for the next slice");
}

void TestIntermediateInputsAreNotMutated()
{
	const iggy::LevelTileMap map = MapFromRows({ "...." });
	const iggy::LevelTileMap mapBefore = map;
	const iggy::NpcActorState2DRegistry actors = ActorRegistry({
		Actor("npc:stable", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = ControlRegistry({
		Control("npc:stable", iggy::moveToNpcObjective({ 3.5F, 0.5F }), iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }), iggy::NpcMoveMode::Walk),
	});
	const iggy::NpcActorState2DRegistry actorsBefore = actors;
	const iggy::NpcActorControlState2DRegistry controlsBefore = controls;
	const iggy::NpcActorFrameState2DProjectionResult frames = ProjectFrames(actors, controls);
	const iggy::NpcActorFrameState2DProjectionResult framesBefore = frames;
	const iggy::NpcActorMovementFrameIntent2DResult intents = ProjectMovementFrame(frames);
	const iggy::NpcActorMovementFrameIntent2DResult intentsBefore = intents;
	const iggy::NpcActorRouteTarget2D route =
		iggy::NpcActorRouteTargetProjector2D {}.project(intents.entries[0].intent);
	const iggy::NpcActorRouteTarget2D routeBefore = route;
	const iggy::NpcActorNavigationRequest2D navigation =
		iggy::NpcActorNavigationRequestBuilder2D {}.build(route, map);
	const iggy::NpcActorNavigationRequest2D navigationBefore = navigation;
	const iggy::NpcActorPathReport2D path =
		iggy::NpcActorPathReporter2D {}.findPath(navigation, map);
	const iggy::NpcActorPathReport2D pathBefore = path;

	const iggy::NpcActorPathStep2D step =
		iggy::NpcActorPathStepper2D {}.step(path);

	Expect(step.status == iggy::NpcActorPathStep2DStatus::Proposed, "immutability setup should propose movement");
	Expect(SameActorRegistry(actors, actorsBefore), "movement pipeline should not mutate actor registry");
	Expect(SameControlRegistry(controls, controlsBefore), "movement pipeline should not mutate control registry");
	Expect(SameMap(map, mapBefore), "movement pipeline should not mutate map");
	Expect(frames.entries.size() == framesBefore.entries.size(), "movement pipeline should not mutate frame projection entries");
	Expect(intents.readyCount == intentsBefore.readyCount && intents.entries.size() == intentsBefore.entries.size(), "movement pipeline should not mutate movement frame intents");
	Expect(route.status == routeBefore.status && NearVec(route.targetPosition, routeBefore.targetPosition), "movement pipeline should not mutate route target");
	Expect(navigation.status == navigationBefore.status && navigation.requestsPath == navigationBefore.requestsPath, "movement pipeline should not mutate navigation request");
	Expect(path.status == pathBefore.status && path.path.waypoints.size() == pathBefore.path.waypoints.size(), "movement pipeline should not mutate path report");
}

} // namespace

int main()
{
	TestSeekingActorRoadComposesThroughPathStep();
	TestFleeingActorRoadComposesThroughEscapeRouteAndPathStep();
	TestBlockedDestinationStopsBeforePathStepMovement();
	TestUnreachableDestinationStopsAtPathReport();
	TestMoveModeSpeedAffectsPathStepDistance();
	TestOccupancyIsNotUsedByPurePathStepYet();
	TestIntermediateInputsAreNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
