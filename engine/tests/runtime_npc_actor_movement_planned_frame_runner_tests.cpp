#include <cstdlib>
#include <string_view>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeNpcActorMovementPlannedFrameRunner.hpp"
#include "runtime/RuntimeNpcActorMovementPlannedFrameStep.hpp"
#include "scene/npc/NpcActorOccupancy2D.hpp"
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
	map.id = Id("level:runtime-planned-runner");
	return map;
}

iggy::runtime::GameplayCommandFrame2D WaitFrame(const char *actorId)
{
	return {
		{ iggy::runtime::GameplayCommand2DFactory {}.wait(Id(actorId)) },
	};
}

iggy::InteractionTarget2DRegistry InteractionTargets()
{
	return iggy::InteractionTarget2DRegistry {
		{ {
			Id("target:runtime-planned-runner"),
			iggy::InteractionTarget2DKind::Usable,
			{ 1.0F, 2.0F },
			0.5F,
			true,
		} },
	};
}

iggy::InventoryState2D Inventory()
{
	return {
		{ { Id("item:runtime-planned-runner"), 2 } },
	};
}

iggy::LevelItemDrop2DRegistry Drops()
{
	return {
		{ { Id("drop:runtime-planned-runner"), Id("item:runtime-planned-runner"), 1, { 3.0F, 4.0F }, 0.25F, true } },
	};
}

iggy::runtime::RuntimeGameplayState GameplayState(const iggy::LevelTileMap &map)
{
	iggy::runtime::RuntimeGameplayState state;
	state.session.tickIndex = 133;
	state.session.level.map = map;
	state.session.hasPlayer = true;
	state.session.player = PlayerAgent(
		Id("player:runtime-planned-runner"),
		{ 0.0F, 0.0F },
		{ 0, 0 },
		iggy::PlayerMovementStatus::Idle,
		iggy::PlayerFacing2D::East);
	state.commandQueue.frames = { WaitFrame("player:queued") };
	state.interaction.targets = InteractionTargets();
	state.inventory.inventory = Inventory();
	state.inventory.drops = Drops();
	return state;
}

iggy::NpcActorState2D Actor(const char *npcId, iggy::Vec2 position)
{
	return {
		Id(npcId),
		Id("profile:runtime-planned-runner"),
		Id("faction:runtime-planned-runner"),
		position,
		Id("goal:runtime-planned-runner"),
		true,
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
	Expect(result.built, "runtime planned runner actor registry fixture should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "runtime planned runner control registry fixture should build");
	return result.registry;
}

iggy::runtime::RuntimeNpcActorMovementPlannedFrameRunnerFrame Frame(
	const iggy::LevelTileMap &map,
	const iggy::NpcActorMovementFramePlan2DConfig &config = {})
{
	return { map, config };
}

iggy::runtime::RuntimeNpcActorMovementPlannedFrameRunnerResult Run(
	const iggy::runtime::RuntimeGameplayState &state,
	std::vector<iggy::runtime::RuntimeNpcActorMovementPlannedFrameRunnerFrame> frames)
{
	return iggy::runtime::RuntimeNpcActorMovementPlannedFrameRunner {}.run({ state, frames });
}

const iggy::NpcActorState2D *FindActor(
	const iggy::NpcActorState2DRegistry &registry,
	const iggy::ResourceId &npcId)
{
	for (const iggy::NpcActorState2D &actor : registry.actors) {
		if (actor.npcId == npcId) {
			return &actor;
		}
	}
	return nullptr;
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
	return actual.session.tickIndex == expected.session.tickIndex
		&& actual.session.hasPlayer == expected.session.hasPlayer
		&& actual.session.player.id == expected.session.player.id
		&& NearVec(actual.session.player.position, expected.session.player.position)
		&& actual.commandQueue.frames.size() == expected.commandQueue.frames.size()
		&& actual.interaction.targets.targets().size() == expected.interaction.targets.targets().size()
		&& actual.inventory.inventory.stacks.size() == expected.inventory.inventory.stacks.size()
		&& actual.inventory.drops.drops.size() == expected.inventory.drops.drops.size()
		&& SameActors(actual.npcActors, expected.npcActors)
		&& SameControls(actual.npcControls, expected.npcControls);
}

bool SameConfig(
	const iggy::NpcActorMovementFramePlan2DConfig &actual,
	const iggy::NpcActorMovementFramePlan2DConfig &expected)
{
	return actual.useOccupancyPolicy == expected.useOccupancyPolicy
		&& actual.occupancyPolicy.maxOccupantsPerTile == expected.occupancyPolicy.maxOccupantsPerTile
		&& actual.runReservation == expected.runReservation
		&& actual.reservation.policy.maxOccupantsPerTile == expected.reservation.policy.maxOccupantsPerTile
		&& actual.pathStep.baseStepDistance == expected.pathStep.baseStepDistance;
}

std::size_t OccupantCountAt(
	const iggy::NpcActorState2DRegistry &registry,
	iggy::TileCoord tile)
{
	const iggy::NpcActorOccupancy2D occupancy =
		iggy::NpcActorOccupancyProjector2D {}.project(registry);
	for (const iggy::NpcActorOccupiedTile2D &occupied : occupancy.occupiedTiles) {
		if (occupied.tile == tile) {
			return occupied.npcIds.size();
		}
	}
	return 0;
}

void TestEmptyFrameListReturnsInitialState()
{
	const iggy::LevelTileMap map = Map({ "..." });
	const iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	const iggy::runtime::RuntimeGameplayState stateBefore = state;

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameRunnerResult result = Run(state, {});

	Expect(!result.hasFrames(), "empty planned runner should report no frames");
	Expect(result.frameCount == 0 && result.frameResults.empty(), "empty planned runner should have no frame results");
	Expect(result.plannedRequestCount == 0 && result.movedCount == 0, "empty planned runner should aggregate zero counts");
	Expect(!result.changed(), "empty planned runner should not change");
	Expect(SameState(result.state, state), "empty planned runner should return initial state");
	Expect(SameState(state, stateBefore), "empty planned runner should not mutate input state");
}

void TestSingleFrameSeekingMatchesDirectStep()
{
	const iggy::LevelTileMap map = Map({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:seeker", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:seeker", { 3.5F, 0.5F }) });

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameResult direct =
		iggy::runtime::RuntimeNpcActorMovementPlannedFrameStep {}.run({ state, map, {} });
	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameRunnerResult result =
		Run(state, { Frame(map) });

	Expect(result.frameCount == 1 && result.frameResults.size() == 1, "single planned runner should store one frame result");
	Expect(SameActors(result.state.npcActors, direct.state.npcActors), "single planned runner should match direct step actor state");
	Expect(result.plannedRequestCount == direct.plannedRequestCount, "single planned runner should match direct request count");
	Expect(result.movedCount == direct.movedCount, "single planned runner should match direct moved count");
	Expect(result.needsOccupancyRebuild && result.needsRenderRefresh, "single planned runner should aggregate refresh flags");
}

void TestMultiFrameSeekingCarriesUpdatedPosition()
{
	const iggy::LevelTileMap map = Map({ "....." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:walker", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:walker", { 4.5F, 0.5F }) });

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameRunnerResult result =
		Run(state, { Frame(map), Frame(map) });
	const iggy::NpcActorState2D *actor = FindActor(result.state.npcActors, Id("npc:walker"));

	Expect(result.frameCount == 2 && result.frameResults.size() == 2, "multi-frame planned runner should store both results");
	Expect(result.plannedRequestCount == 2 && result.movedCount == 2, "multi-frame planned runner should aggregate two moves");
	Expect(result.changedFrameCount == 2, "multi-frame planned runner should count changed frames");
	Expect(actor != nullptr && NearVec(actor->position, { 2.5F, 0.5F }), "multi-frame planned runner should plan second frame from carried position");
	Expect(NearVec(result.frameResults[1].inputState.npcActors.actors[0].position, { 1.5F, 0.5F }), "second planned frame should receive carried state");
}

void TestFleeingFramePreservesEscapeDiagnostics()
{
	const iggy::LevelTileMap map = Map({
		"...",
		"...",
		"...",
	});
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:flee", { 1.5F, 1.5F }) });
	state.npcControls = Controls({ FleeingControl("npc:flee", { 0.5F, 1.5F }) });

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameRunnerResult result =
		Run(state, { Frame(map) });
	const iggy::NpcActorState2D *actor = FindActor(result.state.npcActors, Id("npc:flee"));

	Expect(result.frameResults[0].plan.plan.entries[0].escapeRoute.status == iggy::NpcActorEscapeRouteTarget2DStatus::Ready, "planned runner should preserve escape route diagnostics");
	Expect(result.movedCount == 1, "planned runner should move fleeing actor once");
	Expect(actor != nullptr && actor->position.x > 1.5F, "planned runner should move fleeing actor away from threat");
}

void TestPathFailureStillRecordsFrameResult()
{
	const iggy::LevelTileMap map = Map({ ".#." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:path-fail", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:path-fail", { 2.5F, 0.5F }) });

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameRunnerResult result =
		Run(state, { Frame(map) });

	Expect(result.frameCount == 1 && result.frameResults.size() == 1, "path failure planned runner should still record a frame");
	Expect(result.plannedRequestCount == 0 && result.movedCount == 0, "path failure planned runner should aggregate no movement");
	Expect(result.frameResults[0].plan.pathFailedCount == 1, "path failure planned runner should preserve path diagnostics");
	Expect(SameActors(result.state.npcActors, state.npcActors), "path failure planned runner should preserve actor state");
}

void TestDefaultHardBlockAggregatesBlockedDiagnostics()
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

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameRunnerResult result =
		Run(state, { Frame(map) });

	Expect(result.plannedRequestCount == 1, "hard-block planned runner should preserve blocked prepared request");
	Expect(result.blockedMovementCount == 1, "hard-block planned runner should aggregate blocked movement");
	Expect(result.frameResults[0].movement.apply.entries[0].executor.postMove.blockingNpcId == Id("npc:blocker"), "hard-block planned runner should preserve blocker id");
	Expect(SameActors(result.state.npcActors, state.npcActors), "hard-block planned runner should not move actor");
}

void TestPolicyCapacityTwoAllowsStackedOccupancy()
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
	iggy::NpcActorMovementFramePlan2DConfig config;
	config.useOccupancyPolicy = true;
	config.occupancyPolicy.maxOccupantsPerTile = 2;

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameRunnerResult result =
		Run(state, { Frame(map, config) });

	Expect(result.movedCount == 1 && result.blockedMovementCount == 0, "policy planned runner should allow capacity-two occupied tile");
	Expect(result.frameResults[0].plan.plan.entries[0].policyFilter.status == iggy::NpcActorPathStepOccupancyPolicyFilter2DStatus::Allowed, "policy planned runner should preserve policy diagnostics");
	Expect(OccupantCountAt(result.state.npcActors, { 1, 0 }) == 2, "policy planned runner final occupancy should be stacked");
}

void TestReservationCapacityOneRejectsSecondSameDestination()
{
	const iggy::LevelTileMap map = Map({ "..." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({
		Actor("npc:first", { 0.5F, 0.5F }),
		Actor("npc:second", { 2.5F, 0.5F }),
	});
	state.npcControls = Controls({
		SeekingControl("npc:first", { 1.5F, 0.5F }),
		SeekingControl("npc:second", { 1.5F, 0.5F }),
	});
	iggy::NpcActorMovementFramePlan2DConfig config;
	config.runReservation = true;

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameRunnerResult result =
		Run(state, { Frame(map, config) });
	const iggy::NpcActorState2D *first = FindActor(result.state.npcActors, Id("npc:first"));
	const iggy::NpcActorState2D *second = FindActor(result.state.npcActors, Id("npc:second"));

	Expect(result.preReservationRequestCount == 2, "reservation planned runner should aggregate pre-reservation requests");
	Expect(result.reservationAcceptedCount == 1 && result.reservationRejectedCount == 1, "reservation planned runner should aggregate reservation counts");
	Expect(result.plannedRequestCount == 1 && result.movedCount == 1, "reservation planned runner should apply accepted request only");
	Expect(first != nullptr && NearVec(first->position, { 1.5F, 0.5F }), "reservation planned runner should move first actor");
	Expect(second != nullptr && NearVec(second->position, { 2.5F, 0.5F }), "reservation planned runner should leave second actor unchanged");
}

void TestReservationCapacityTwoAllowsBothSameDestination()
{
	const iggy::LevelTileMap map = Map({ "..." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({
		Actor("npc:first", { 0.5F, 0.5F }),
		Actor("npc:second", { 2.5F, 0.5F }),
	});
	state.npcControls = Controls({
		SeekingControl("npc:first", { 1.5F, 0.5F }),
		SeekingControl("npc:second", { 1.5F, 0.5F }),
	});
	iggy::NpcActorMovementFramePlan2DConfig config;
	config.runReservation = true;
	config.reservation.policy.maxOccupantsPerTile = 2;

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameRunnerResult result =
		Run(state, { Frame(map, config) });

	Expect(result.reservationAcceptedCount == 2 && result.reservationRejectedCount == 0, "capacity-two planned runner should accept both reservations");
	Expect(result.movedCount == 2, "capacity-two planned runner should move both actors");
	Expect(OccupantCountAt(result.state.npcActors, { 1, 0 }) == 2, "capacity-two planned runner final occupancy should be inspectably stacked");
}

void TestNonNpcGameplayChildrenAndInputsArePreserved()
{
	const iggy::LevelTileMap map = Map({ "...." });
	const iggy::LevelTileMap mapBefore = map;
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:immutable", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:immutable", { 3.5F, 0.5F }) });
	const iggy::runtime::RuntimeGameplayState stateBefore = state;
	iggy::NpcActorMovementFramePlan2DConfig config;
	config.useOccupancyPolicy = true;
	config.occupancyPolicy.maxOccupantsPerTile = 2;
	config.runReservation = true;
	config.reservation.policy.maxOccupantsPerTile = 2;
	const iggy::NpcActorMovementFramePlan2DConfig configBefore = config;
	const std::vector<iggy::runtime::RuntimeNpcActorMovementPlannedFrameRunnerFrame> frames = { Frame(map, config) };

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameRunnerResult result = Run(state, frames);

	Expect(result.state.session.tickIndex == state.session.tickIndex, "planned runner should preserve session");
	Expect(result.state.commandQueue.frames.size() == state.commandQueue.frames.size(), "planned runner should preserve command queue");
	Expect(result.state.interaction.targets.targets().size() == state.interaction.targets.targets().size(), "planned runner should preserve interaction state");
	Expect(result.state.inventory.inventory.stacks.size() == state.inventory.inventory.stacks.size(), "planned runner should preserve inventory state");
	Expect(SameControls(result.state.npcControls, state.npcControls), "planned runner should preserve npc controls");
	Expect(SameState(state, stateBefore), "planned runner should not mutate input state");
	Expect(SameMap(map, mapBefore), "planned runner should not mutate input map");
	Expect(SameConfig(config, configBefore), "planned runner should not mutate input config");
	Expect(frames.size() == 1 && SameMap(frames[0].map, mapBefore), "planned runner should not mutate input frames");
}

void TestManualLoopMatchesRunner()
{
	const iggy::LevelTileMap map = Map({ "....." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:manual", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:manual", { 4.5F, 0.5F }) });
	const std::vector<iggy::runtime::RuntimeNpcActorMovementPlannedFrameRunnerFrame> frames = {
		Frame(map),
		Frame(map),
	};

	iggy::runtime::RuntimeGameplayState manualState = state;
	std::size_t manualRequests = 0;
	std::size_t manualMoved = 0;
	std::size_t manualChanged = 0;
	iggy::runtime::RuntimeNpcActorMovementPlannedFrameStep step;
	for (const iggy::runtime::RuntimeNpcActorMovementPlannedFrameRunnerFrame &frame : frames) {
		const iggy::runtime::RuntimeNpcActorMovementPlannedFrameResult frameResult =
			step.run({ manualState, frame.map, frame.config });
		manualState = frameResult.state;
		manualRequests += frameResult.plannedRequestCount;
		manualMoved += frameResult.movedCount;
		if (frameResult.changed) {
			++manualChanged;
		}
	}

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameRunnerResult result =
		Run(state, frames);

	Expect(SameActors(result.state.npcActors, manualState.npcActors), "planned runner should match manual loop final actor registry");
	Expect(result.plannedRequestCount == manualRequests, "planned runner should match manual request aggregate");
	Expect(result.movedCount == manualMoved, "planned runner should match manual moved aggregate");
	Expect(result.changedFrameCount == manualChanged, "planned runner should match manual changed-frame aggregate");
}

} // namespace

int main()
{
	TestEmptyFrameListReturnsInitialState();
	TestSingleFrameSeekingMatchesDirectStep();
	TestMultiFrameSeekingCarriesUpdatedPosition();
	TestFleeingFramePreservesEscapeDiagnostics();
	TestPathFailureStillRecordsFrameResult();
	TestDefaultHardBlockAggregatesBlockedDiagnostics();
	TestPolicyCapacityTwoAllowsStackedOccupancy();
	TestReservationCapacityOneRejectsSecondSameDestination();
	TestReservationCapacityTwoAllowsBothSameDestination();
	TestNonNpcGameplayChildrenAndInputsArePreserved();
	TestManualLoopMatchesRunner();

	return Failures;
}
