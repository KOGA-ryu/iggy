#include <cstdlib>
#include <string_view>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeNpcActorMovementFrameStep.hpp"
#include "runtime/RuntimeNpcActorMovementPlannedFrameStep.hpp"
#include "runtime/RuntimeNpcActorMovementRequestPlanStep.hpp"
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
	map.id = Id("level:runtime-planned-frame");
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
			Id("target:runtime-planned-frame"),
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
		{ { Id("item:runtime-planned-frame"), 2 } },
	};
}

iggy::LevelItemDrop2DRegistry Drops()
{
	return {
		{ { Id("drop:runtime-planned-frame"), Id("item:runtime-planned-frame"), 1, { 3.0F, 4.0F }, 0.25F, true } },
	};
}

iggy::runtime::RuntimeGameplayState GameplayState(const iggy::LevelTileMap &map)
{
	iggy::runtime::RuntimeGameplayState state;
	state.session.tickIndex = 91;
	state.session.level.map = map;
	state.session.hasPlayer = true;
	state.session.player = PlayerAgent(
		Id("player:runtime-planned-frame"),
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

iggy::NpcActorState2D Actor(
	const char *npcId,
	iggy::Vec2 position,
	bool present = true)
{
	return {
		Id(npcId),
		Id("profile:runtime-planned-frame"),
		Id("faction:runtime-planned-frame"),
		position,
		Id("goal:runtime-planned-frame"),
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
	Expect(result.built, "runtime planned frame actor registry fixture should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "runtime planned frame control registry fixture should build");
	return result.registry;
}

iggy::runtime::RuntimeNpcActorMovementPlannedFrameResult RunPlanned(
	const iggy::runtime::RuntimeGameplayState &state,
	const iggy::LevelTileMap &map,
	const iggy::NpcActorMovementFramePlan2DConfig &config = {})
{
	return iggy::runtime::RuntimeNpcActorMovementPlannedFrameStep {}.run({ state, map, config });
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
		&& actual.pathStep.baseStepDistance == expected.pathStep.baseStepDistance
		&& actual.pathStep.arrivalTolerance == expected.pathStep.arrivalTolerance;
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

void TestEmptyStateRunsNoRequestNoOp()
{
	const iggy::LevelTileMap map = Map({ "..." });
	const iggy::LevelTileMap mapBefore = map;
	const iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	const iggy::runtime::RuntimeGameplayState stateBefore = state;

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameResult result = RunPlanned(state, map);

	Expect(result.status == iggy::runtime::RuntimeNpcActorMovementPlannedFrameStatus::NoRequests, "empty planned frame should report no requests");
	Expect(!result.ranMovementRequests(), "empty planned frame should not report movement requests");
	Expect(result.plan.requestCount == 0 && result.movement.apply.requestCount == 0, "empty planned frame should still run empty movement frame");
	Expect(result.movement.report.apply.entries.empty(), "empty planned frame should preserve no-op movement report");
	Expect(!result.changedState(), "empty planned frame should not change state");
	Expect(SameState(result.inputState, state), "empty planned frame should copy input state");
	Expect(SameState(result.state, state), "empty planned frame should return unchanged state");
	Expect(SameMap(result.map, map), "empty planned frame should copy map");
	Expect(SameState(state, stateBefore), "empty planned frame should not mutate input state");
	Expect(SameMap(map, mapBefore), "empty planned frame should not mutate input map");
}

void TestSeekingNpcPlansAndMovesOnlyNpcActors()
{
	const iggy::LevelTileMap map = Map({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:seeker", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:seeker", { 3.5F, 0.5F }) });
	const iggy::runtime::RuntimeGameplayState stateBefore = state;

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameResult result = RunPlanned(state, map);
	const iggy::NpcActorState2D *actor = FindActor(result.state.npcActors, Id("npc:seeker"));

	Expect(result.status == iggy::runtime::RuntimeNpcActorMovementPlannedFrameStatus::Ran, "seeking planned frame should report requests");
	Expect(result.plannedRequestCount == 1 && result.movedCount == 1, "seeking planned frame should mirror request and moved counts");
	Expect(actor != nullptr && NearVec(actor->position, { 1.5F, 0.5F }), "seeking planned frame should move actor to proposed step");
	Expect(SameControls(result.state.npcControls, state.npcControls), "seeking planned frame should preserve npc controls");
	Expect(result.state.session.tickIndex == state.session.tickIndex, "seeking planned frame should preserve session");
	Expect(result.state.commandQueue.frames.size() == state.commandQueue.frames.size(), "seeking planned frame should preserve command queue");
	Expect(result.state.interaction.targets.targets().size() == state.interaction.targets.targets().size(), "seeking planned frame should preserve interaction state");
	Expect(result.state.inventory.inventory.stacks.size() == state.inventory.inventory.stacks.size(), "seeking planned frame should preserve inventory state");
	Expect(SameState(state, stateBefore), "seeking planned frame should not mutate input state");
}

void TestFleeingNpcUsesEscapeRouteAndMovesAway()
{
	const iggy::LevelTileMap map = Map({
		"...",
		"...",
		"...",
	});
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:flee", { 1.5F, 1.5F }) });
	state.npcControls = Controls({ FleeingControl("npc:flee", { 0.5F, 1.5F }) });

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameResult result = RunPlanned(state, map);
	const iggy::NpcActorState2D *actor = FindActor(result.state.npcActors, Id("npc:flee"));

	Expect(result.plan.plan.entries[0].escapeRoute.status == iggy::NpcActorEscapeRouteTarget2DStatus::Ready, "fleeing planned frame should preserve escape route diagnostics");
	Expect(result.plannedRequestCount == 1 && result.movedCount == 1, "fleeing planned frame should plan and move once");
	Expect(actor != nullptr && actor->position.x > 1.5F, "fleeing planned frame should move away from left threat");
}

void TestBlockedDestinationProducesNoPlannedMovement()
{
	const iggy::LevelTileMap map = Map({ ".#." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:path-fail", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:path-fail", { 2.5F, 0.5F }) });

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameResult result = RunPlanned(state, map);
	const iggy::NpcActorState2D *actor = FindActor(result.state.npcActors, Id("npc:path-fail"));

	Expect(result.status == iggy::runtime::RuntimeNpcActorMovementPlannedFrameStatus::NoRequests, "path failure planned frame should have no requests");
	Expect(result.plan.pathFailedCount == 1, "path failure planned frame should preserve path diagnostics");
	Expect(result.movement.apply.requestCount == 0, "path failure planned frame should feed no requests to movement step");
	Expect(actor != nullptr && NearVec(actor->position, { 0.5F, 0.5F }), "path failure planned frame should leave actor unchanged");
}

void TestExistingHardBlockProducesBlockedDiagnostics()
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

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameResult result = RunPlanned(state, map);
	const iggy::NpcActorState2D *mover = FindActor(result.state.npcActors, Id("npc:mover"));

	Expect(result.plannedRequestCount == 1 && result.blockedCount == 1, "default hard-block planned frame should preserve blocked diagnostics");
	Expect(result.plan.blockedRequestCount == 1, "default hard-block planned frame should mirror blocked request count");
	Expect(result.movement.apply.entries[0].executor.postMove.blockingNpcId == Id("npc:blocker"), "default hard-block planned frame should preserve blocker id");
	Expect(mover != nullptr && NearVec(mover->position, { 0.5F, 0.5F }), "default hard-block planned frame should not move blocked actor");
}

void TestOccupancyPolicyCapacityTwoAllowsOccupiedTileMove()
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

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameResult result = RunPlanned(state, map, config);
	const iggy::NpcActorState2D *mover = FindActor(result.state.npcActors, Id("npc:mover"));

	Expect(result.plan.plan.entries[0].usedOccupancyPolicy, "policy planned frame should preserve policy usage diagnostics");
	Expect(result.plan.plan.entries[0].policyFilter.status == iggy::NpcActorPathStepOccupancyPolicyFilter2DStatus::Allowed, "policy planned frame should preserve allowed policy filter");
	Expect(result.movedCount == 1 && result.blockedCount == 0, "policy planned frame should move into occupied tile when capacity allows");
	Expect(mover != nullptr && NearVec(mover->position, { 1.5F, 0.5F }), "policy planned frame should place mover on occupied tile");
	Expect(OccupantCountAt(result.state.npcActors, { 1, 0 }) == 2, "policy planned frame final occupancy should be inspectably stacked");
}

void TestReservationCapacityOneMovesOnlyFirst()
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

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameResult result = RunPlanned(state, map, config);
	const iggy::NpcActorState2D *first = FindActor(result.state.npcActors, Id("npc:first"));
	const iggy::NpcActorState2D *second = FindActor(result.state.npcActors, Id("npc:second"));

	Expect(result.preReservationRequestCount == 2, "reservation planned frame should preserve pre-reservation count");
	Expect(result.reservationAcceptedCount == 1 && result.reservationRejectedCount == 1, "reservation planned frame should mirror accepted/rejected counts");
	Expect(result.plannedRequestCount == 1 && result.movedCount == 1, "reservation planned frame should apply only accepted request");
	Expect(first != nullptr && NearVec(first->position, { 1.5F, 0.5F }), "reservation planned frame should move first accepted actor");
	Expect(second != nullptr && NearVec(second->position, { 2.5F, 0.5F }), "reservation planned frame should leave rejected actor unchanged");
	Expect(result.plan.plan.reservation.entries[1].status == iggy::NpcActorMovementReservationEntry2DStatus::ReservationBlocked, "reservation planned frame should preserve rejection diagnostics");
}

void TestReservationCapacityTwoMovesBothAndStacksOccupancy()
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

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameResult result = RunPlanned(state, map, config);
	const iggy::NpcActorState2D *first = FindActor(result.state.npcActors, Id("npc:first"));
	const iggy::NpcActorState2D *second = FindActor(result.state.npcActors, Id("npc:second"));
	const iggy::NpcActorOccupancy2D occupancy =
		iggy::NpcActorOccupancyProjector2D {}.project(result.state.npcActors);

	Expect(result.reservationAcceptedCount == 2 && result.reservationRejectedCount == 0, "capacity-two planned frame should accept both reservations");
	Expect(result.plannedRequestCount == 2 && result.movedCount == 2, "capacity-two planned frame should apply both accepted requests");
	Expect(first != nullptr && NearVec(first->position, { 1.5F, 0.5F }), "capacity-two planned frame should move first actor");
	Expect(second != nullptr && NearVec(second->position, { 1.5F, 0.5F }), "capacity-two planned frame should move second actor");
	Expect(occupancy.hasIssues() && OccupantCountAt(result.state.npcActors, { 1, 0 }) == 2, "capacity-two planned frame should leave stacked occupancy inspectable");
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
	config.useOccupancyPolicy = true;
	config.occupancyPolicy.maxOccupantsPerTile = 2;
	config.runReservation = true;
	config.reservation.policy.maxOccupantsPerTile = 2;
	const iggy::NpcActorMovementFramePlan2DConfig configBefore = config;

	(void)RunPlanned(state, map, config);

	Expect(SameState(state, stateBefore), "planned frame should not mutate input state");
	Expect(SameMap(map, mapBefore), "planned frame should not mutate input map");
	Expect(SameConfig(config, configBefore), "planned frame should not mutate input config");
}

void TestManualCompositionMatchesHelper()
{
	const iggy::LevelTileMap map = Map({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:manual", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:manual", { 3.5F, 0.5F }) });
	iggy::NpcActorMovementFramePlan2DConfig config;
	config.useOccupancyPolicy = true;

	const iggy::runtime::RuntimeNpcActorMovementRequestPlanResult planned =
		iggy::runtime::RuntimeNpcActorMovementRequestPlanStep {}.plan({ state, map, config });
	const iggy::runtime::RuntimeNpcActorMovementFrameResult movement =
		iggy::runtime::RuntimeNpcActorMovementFrameStep {}.run({ state, planned.requests });
	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameResult composed =
		RunPlanned(state, map, config);

	Expect(SameActors(composed.state.npcActors, movement.state.npcActors), "planned frame helper should match manual final actor registry");
	Expect(composed.plannedRequestCount == planned.requestCount, "planned frame helper should match manual planned request count");
	Expect(composed.movedCount == movement.apply.movedCount, "planned frame helper should match manual moved count");
	Expect(composed.changed == movement.changed, "planned frame helper should match manual changed flag");
	Expect(composed.plan.requests.size() == planned.requests.size(), "planned frame helper should preserve nested plan requests");
}

} // namespace

int main()
{
	TestEmptyStateRunsNoRequestNoOp();
	TestSeekingNpcPlansAndMovesOnlyNpcActors();
	TestFleeingNpcUsesEscapeRouteAndMovesAway();
	TestBlockedDestinationProducesNoPlannedMovement();
	TestExistingHardBlockProducesBlockedDiagnostics();
	TestOccupancyPolicyCapacityTwoAllowsOccupiedTileMove();
	TestReservationCapacityOneMovesOnlyFirst();
	TestReservationCapacityTwoMovesBothAndStacksOccupancy();
	TestInputsAndConfigAreNotMutated();
	TestManualCompositionMatchesHelper();

	return Failures;
}
