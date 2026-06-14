#include <cstdlib>
#include <string_view>
#include <vector>

#include "scene/npc/NpcActorEscapeRouteTarget2D.hpp"
#include "scene/npc/NpcActorMovementFrameIntent2D.hpp"
#include "scene/npc/NpcActorNavigationRequest2D.hpp"
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
	map.id = Id("level:path-step-occupancy-acceptance");
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

iggy::NpcActorState2DRegistry RawActorRegistry(std::vector<iggy::NpcActorState2D> actors)
{
	iggy::NpcActorState2DRegistry registry;
	registry.actors = actors;
	return registry;
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

bool SameOccupancy(const iggy::NpcActorOccupancy2D &actual, const iggy::NpcActorOccupancy2D &expected)
{
	if (actual.status != expected.status
		|| actual.entries.size() != expected.entries.size()
		|| actual.occupiedTiles.size() != expected.occupiedTiles.size()
		|| actual.issues.size() != expected.issues.size()) {
		return false;
	}

	for (std::size_t index = 0; index < actual.entries.size(); ++index) {
		const iggy::NpcActorOccupancyEntry2D &left = actual.entries[index];
		const iggy::NpcActorOccupancyEntry2D &right = expected.entries[index];
		if (left.npcId != right.npcId
			|| left.tile != right.tile
			|| left.actorIndex != right.actorIndex
			|| left.actor.npcId != right.actor.npcId
			|| !NearVec(left.actor.position, right.actor.position)
			|| left.actor.present != right.actor.present) {
			return false;
		}
	}

	for (std::size_t index = 0; index < actual.occupiedTiles.size(); ++index) {
		const iggy::NpcActorOccupiedTile2D &left = actual.occupiedTiles[index];
		const iggy::NpcActorOccupiedTile2D &right = expected.occupiedTiles[index];
		if (left.tile != right.tile
			|| left.npcIds != right.npcIds
			|| left.actorIndexes != right.actorIndexes) {
			return false;
		}
	}

	return true;
}

bool SameStep(const iggy::NpcActorPathStep2D &actual, const iggy::NpcActorPathStep2D &expected)
{
	return actual.status == expected.status
		&& actual.npcId == expected.npcId
		&& NearVec(actual.oldPosition, expected.oldPosition)
		&& NearVec(actual.proposedPosition, expected.proposedPosition)
		&& actual.oldTile == expected.oldTile
		&& actual.proposedTile == expected.proposedTile
		&& actual.moveMode == expected.moveMode
		&& actual.requestsMovement == expected.requestsMovement
		&& actual.completedPath == expected.completedPath;
}

struct MovementPipeline {
	iggy::NpcActorFrameState2DProjectionResult frames;
	iggy::NpcActorMovementFrameIntent2DResult movement;
	iggy::NpcActorRouteTarget2D route;
	iggy::NpcActorNavigationRequest2D navigation;
	iggy::NpcActorPathReport2D path;
	iggy::NpcActorPathStep2D step;
};

MovementPipeline SeekingPipeline(
	const iggy::NpcActorState2DRegistry &actors,
	const iggy::NpcActorControlState2DRegistry &controls,
	const iggy::LevelTileMap &map,
	std::size_t movementIndex = 0)
{
	MovementPipeline pipeline;
	pipeline.frames = iggy::NpcActorFrameStateProjector2D {}.project(actors, controls);
	pipeline.movement = iggy::NpcActorMovementFrameIntentProjector2D {}.project(pipeline.frames);
	pipeline.route = iggy::NpcActorRouteTargetProjector2D {}.project(pipeline.movement.entries[movementIndex].intent);
	pipeline.navigation = iggy::NpcActorNavigationRequestBuilder2D {}.build(pipeline.route, map);
	pipeline.path = iggy::NpcActorPathReporter2D {}.findPath(pipeline.navigation, map);
	pipeline.step = iggy::NpcActorPathStepper2D {}.step(pipeline.path);
	return pipeline;
}

struct FleePipeline {
	iggy::NpcActorFrameState2DProjectionResult frames;
	iggy::NpcActorMovementFrameIntent2DResult movement;
	iggy::NpcActorEscapeRouteTarget2D escapeRoute;
	iggy::NpcActorNavigationRequest2D navigation;
	iggy::NpcActorPathReport2D path;
	iggy::NpcActorPathStep2D step;
};

FleePipeline FleeingPipeline(
	const iggy::NpcActorState2DRegistry &actors,
	const iggy::NpcActorControlState2DRegistry &controls,
	const iggy::LevelTileMap &map)
{
	FleePipeline pipeline;
	pipeline.frames = iggy::NpcActorFrameStateProjector2D {}.project(actors, controls);
	pipeline.movement = iggy::NpcActorMovementFrameIntentProjector2D {}.project(pipeline.frames);
	pipeline.escapeRoute = iggy::NpcActorEscapeRouteTargetProjector2D {}.project(pipeline.movement.entries[0].intent, map);
	pipeline.navigation = iggy::NpcActorNavigationRequestBuilder2D {}.build(pipeline.escapeRoute.route, map);
	pipeline.path = iggy::NpcActorPathReporter2D {}.findPath(pipeline.navigation, map);
	pipeline.step = iggy::NpcActorPathStepper2D {}.step(pipeline.path);
	return pipeline;
}

iggy::NpcActorPathStepOccupancyFilter2D Filter(
	const iggy::NpcActorPathStep2D &step,
	const iggy::NpcActorOccupancy2D &occupancy)
{
	return iggy::NpcActorPathStepOccupancyFilterProjector2D {}.filter(step, occupancy);
}

void TestAllowedFullRoad()
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

	const MovementPipeline pipeline = SeekingPipeline(actors, controls, map);
	const iggy::NpcActorOccupancy2D occupancy =
		iggy::NpcActorOccupancyProjector2D {}.project(actors);
	const iggy::NpcActorPathStepOccupancyFilter2D filter = Filter(pipeline.step, occupancy);

	Expect(pipeline.frames.entries.size() == 1, "allowed road should project one frame entry");
	Expect(pipeline.movement.readyCount == 1, "allowed road should produce one ready movement intent");
	Expect(pipeline.route.ready(), "allowed road should produce ready route target");
	Expect(pipeline.navigation.ready(), "allowed road should produce ready navigation request");
	Expect(pipeline.path.hasPath(), "allowed road should find path");
	Expect(pipeline.step.status == iggy::NpcActorPathStep2DStatus::Proposed, "allowed road should propose path step");
	Expect(filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed, "empty proposed tile should be allowed by filter");
	Expect(filter.allowed() && filter.requestsMovement, "allowed filter should request movement");
	Expect(filter.occupancy.status == iggy::NpcActorOccupancyBlock2DStatus::Empty, "allowed full road should preserve empty occupancy query");
	Expect(NearVec(filter.step.proposedPosition, { 1.5F, 0.5F }), "allowed full road should preserve proposed step position");
	Expect(SameActorRegistry(actors, actorsBefore), "allowed full road should not mutate actor registry");
	Expect(SameControlRegistry(controls, controlsBefore), "allowed full road should not mutate control registry");
	Expect(SameMap(map, mapBefore), "allowed full road should not mutate map");
}

void TestSelfOnlyOccupancyAllowed()
{
	const iggy::LevelTileMap map = MapFromRows({ "...." });
	const iggy::NpcActorState2DRegistry actors = ActorRegistry({
		Actor("npc:mover", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = ControlRegistry({
		Control("npc:mover", iggy::moveToNpcObjective({ 3.5F, 0.5F }), iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }), iggy::NpcMoveMode::Walk),
	});
	const MovementPipeline pipeline = SeekingPipeline(actors, controls, map);
	const iggy::NpcActorOccupancy2D occupancy = iggy::NpcActorOccupancyProjector2D {}.project(
		RawActorRegistry({ Actor("npc:mover", pipeline.step.proposedPosition) }));

	const iggy::NpcActorPathStepOccupancyFilter2D filter = Filter(pipeline.step, occupancy);

	Expect(pipeline.step.status == iggy::NpcActorPathStep2DStatus::Proposed, "self-only setup should still propose path step");
	Expect(filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed, "self-only proposed tile should be allowed");
	Expect(filter.occupancy.status == iggy::NpcActorOccupancyBlock2DStatus::OnlySelf, "self-only filter should preserve OnlySelf query result");
	Expect(filter.blockingNpcId.empty(), "self-only filter should not report blocking id");
}

void TestOtherNpcBlocksAfterPathStepProposes()
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
	const MovementPipeline pipeline = SeekingPipeline(actors, controls, map, 0);
	const iggy::NpcActorOccupancy2D occupancy =
		iggy::NpcActorOccupancyProjector2D {}.project(actors);
	const iggy::NpcActorPathStepOccupancyFilter2D filter = Filter(pipeline.step, occupancy);

	Expect(pipeline.step.status == iggy::NpcActorPathStep2DStatus::Proposed, "blocked road should still produce a path-step proposal");
	Expect(pipeline.step.requestsMovement, "blocked road path step should still request movement before occupancy filtering");
	Expect(filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "other NPC should block proposed tile");
	Expect(!filter.requestsMovement, "blocked filter should not request movement");
	Expect(filter.blockingNpcId == Id("npc:blocker"), "blocked filter should preserve blocking NPC id");
	Expect(filter.occupancy.occupancy.npcIds.size() == 1 && filter.occupancy.occupancy.npcIds[0] == Id("npc:blocker"), "blocked filter should preserve occupancy query details");
}

void TestFleeingRoadAllowedAndBlockedByOccupancy()
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
	const FleePipeline pipeline = FleeingPipeline(actors, controls, map);

	const iggy::NpcActorOccupancy2D emptyDestination =
		iggy::NpcActorOccupancyProjector2D {}.project(actors);
	const iggy::NpcActorPathStepOccupancyFilter2D allowed = Filter(pipeline.step, emptyDestination);

	const iggy::NpcActorOccupancy2D blockedDestination = iggy::NpcActorOccupancyProjector2D {}.project(
		RawActorRegistry({
			Actor("npc:flee", { 1.5F, 1.5F }),
			Actor("npc:blocker", pipeline.step.proposedPosition),
		}));
	const iggy::NpcActorPathStepOccupancyFilter2D blocked = Filter(pipeline.step, blockedDestination);

	Expect(pipeline.escapeRoute.ready(), "fleeing road should produce a ready escape route");
	Expect(pipeline.path.hasPath(), "fleeing road should find path to escape destination");
	Expect(pipeline.step.status == iggy::NpcActorPathStep2DStatus::Proposed, "fleeing road should propose movement before occupancy filtering");
	Expect(allowed.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed, "fleeing road should be allowed when escape tile is empty");
	Expect(blocked.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "fleeing road should be blocked when another NPC occupies escape tile");
	Expect(blocked.blockingNpcId == Id("npc:blocker"), "blocked fleeing road should preserve blocker id");
	Expect(NearVec(blocked.step.proposedPosition, pipeline.escapeRoute.escape.escapePosition), "blocked fleeing road should preserve selected escape step");
}

void TestDuplicateOccupancyRemainsInspectable()
{
	const iggy::LevelTileMap map = MapFromRows({ "...." });
	const iggy::NpcActorState2DRegistry actors = ActorRegistry({
		Actor("npc:mover", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = ControlRegistry({
		Control("npc:mover", iggy::moveToNpcObjective({ 3.5F, 0.5F }), iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }), iggy::NpcMoveMode::Walk),
	});
	const MovementPipeline pipeline = SeekingPipeline(actors, controls, map);

	const iggy::NpcActorOccupancy2D sameIdDuplicate = iggy::NpcActorOccupancyProjector2D {}.project(
		RawActorRegistry({
			Actor("npc:mover", { 1.25F, 0.25F }),
			Actor("npc:mover", { 1.75F, 0.75F }),
		}));
	const iggy::NpcActorOccupancy2D mixedDuplicate = iggy::NpcActorOccupancyProjector2D {}.project(
		RawActorRegistry({
			Actor("npc:mover", { 1.25F, 0.25F }),
			Actor("npc:blocker", { 1.75F, 0.75F }),
		}));

	const iggy::NpcActorPathStepOccupancyFilter2D sameId = Filter(pipeline.step, sameIdDuplicate);
	const iggy::NpcActorPathStepOccupancyFilter2D mixed = Filter(pipeline.step, mixedDuplicate);

	Expect(sameIdDuplicate.hasIssues(), "same-id duplicate occupancy should remain inspectable with issues");
	Expect(sameId.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed, "same-id duplicate occupancy should be allowed");
	Expect(sameId.occupancy.occupancy.npcIds.size() == 2, "same-id duplicate filter should preserve duplicate occupant facts");
	Expect(mixedDuplicate.hasIssues(), "mixed duplicate occupancy should remain inspectable with issues");
	Expect(mixed.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "mixed duplicate occupancy should block");
	Expect(mixed.blockingNpcId == Id("npc:blocker"), "mixed duplicate occupancy should preserve first different blocker id");
	Expect(mixed.occupancy.occupancy.npcIds.size() == 2, "mixed duplicate filter should preserve all occupant facts");
}

void TestInputsAreNotMutated()
{
	const iggy::LevelTileMap map = MapFromRows({ "...." });
	const iggy::LevelTileMap mapBefore = map;
	const iggy::NpcActorState2DRegistry actors = ActorRegistry({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});
	const iggy::NpcActorControlState2DRegistry controls = ControlRegistry({
		Control("npc:mover", iggy::moveToNpcObjective({ 3.5F, 0.5F }), iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }), iggy::NpcMoveMode::Walk),
		Control("npc:blocker", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still),
	});
	const iggy::NpcActorState2DRegistry actorsBefore = actors;
	const iggy::NpcActorControlState2DRegistry controlsBefore = controls;
	const MovementPipeline pipeline = SeekingPipeline(actors, controls, map);
	iggy::NpcActorPathStep2D step = pipeline.step;
	const iggy::NpcActorPathStep2D stepBefore = step;
	iggy::NpcActorOccupancy2D occupancy =
		iggy::NpcActorOccupancyProjector2D {}.project(actors);
	const iggy::NpcActorOccupancy2D occupancyBefore = occupancy;

	const iggy::NpcActorPathStepOccupancyFilter2D filter = Filter(step, occupancy);

	Expect(filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "immutability setup should block on occupied proposed tile");
	Expect(SameActorRegistry(actors, actorsBefore), "occupancy pipeline should not mutate actor registry");
	Expect(SameControlRegistry(controls, controlsBefore), "occupancy pipeline should not mutate control registry");
	Expect(SameMap(map, mapBefore), "occupancy pipeline should not mutate map");
	Expect(SameStep(step, stepBefore), "occupancy pipeline should not mutate path step");
	Expect(SameOccupancy(occupancy, occupancyBefore), "occupancy pipeline should not mutate occupancy projection");
}

} // namespace

int main()
{
	TestAllowedFullRoad();
	TestSelfOnlyOccupancyAllowed();
	TestOtherNpcBlocksAfterPathStepProposes();
	TestFleeingRoadAllowedAndBlockedByOccupancy();
	TestDuplicateOccupancyRemainsInspectable();
	TestInputsAreNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
