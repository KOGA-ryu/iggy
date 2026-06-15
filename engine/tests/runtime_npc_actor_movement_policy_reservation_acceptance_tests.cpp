#include <cstdlib>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeGameplayFrameStep.hpp"
#include "runtime/RuntimeNpcActorMovementRequestPlanStep.hpp"
#include "runtime/RuntimePolicyGameplayFrameStep.hpp"
#include "scene/npc/NpcActorOccupancyQuery2D.hpp"
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
	map.id = Id("level:runtime-policy-reservation");
	map.playerStart = { 0, 0 };
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

iggy::NpcActorState2D Actor(const char *npcId, iggy::Vec2 position)
{
	return {
		Id(npcId),
		Id("profile:runtime-policy-reservation"),
		Id("faction:runtime-policy-reservation"),
		position,
		{},
		true,
	};
}

iggy::NpcActorControlState2D SeekingControl(const char *npcId, iggy::Vec2 target)
{
	return {
		Id(npcId),
		iggy::moveToNpcObjective(target),
		iggy::seekingNpcBehaviorState(target),
		iggy::NpcMoveMode::Walk,
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
	Expect(result.built, "policy reservation acceptance actor registry fixture should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "policy reservation acceptance control registry fixture should build");
	return result.registry;
}

iggy::runtime::RuntimeGameplayState GameplayState(const iggy::LevelTileMap &map)
{
	iggy::runtime::RuntimeGameplayState state;
	state.session.level.map = map;
	state.session.tickIndex = 17;
	state.session.hasPlayer = true;
	state.session.player = PlayerAgent(
		Id("player:runtime-policy-reservation"),
		{ 0.0F, 0.0F },
		{ 0, 0 },
		iggy::PlayerMovementStatus::Idle,
		iggy::PlayerFacing2D::East);
	return state;
}

iggy::runtime::RuntimeNpcActorMovementRequestPlanResult Plan(
	const iggy::runtime::RuntimeGameplayState &state,
	const iggy::LevelTileMap &map,
	const iggy::NpcActorMovementFramePlan2DConfig &config = {})
{
	return iggy::runtime::RuntimeNpcActorMovementRequestPlanStep {}.plan({ state, map, config });
}

iggy::runtime::RuntimeGameplayFrameResult RawFrame(
	const iggy::runtime::RuntimeGameplayState &state,
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> &requests)
{
	iggy::runtime::RuntimeGameplayFrameInput input;
	input.state = state;
	input.actorId = Id("player:runtime-policy-reservation");
	input.fallbackPlayerPosition = { 0.0F, 0.0F };
	input.playerCommandConfig = PlayerConfig();
	input.npcConfig = NpcConfig();
	input.npcMovementRequests = requests;
	return iggy::runtime::RuntimeGameplayFrameStep {}.run(input);
}

iggy::runtime::RuntimePolicyGameplayFrameResult PolicyFrame(
	const iggy::runtime::RuntimeGameplayState &state,
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> &requests)
{
	iggy::runtime::RuntimePolicyGameplayFrameInput input;
	input.state = state;
	input.actorId = Id("player:runtime-policy-reservation");
	input.fallbackPlayerPosition = { 0.0F, 0.0F };
	input.playerCommandConfig = PlayerConfig();
	input.npcConfig = NpcConfig();
	input.npcMovementRequests = requests;
	return iggy::runtime::RuntimePolicyGameplayFrameStep {}.run(input);
}

const iggy::NpcActorState2D *FindActor(const iggy::NpcActorState2DRegistry &registry, const char *npcId)
{
	for (const iggy::NpcActorState2D &actor : registry.actors) {
		if (actor.npcId == Id(npcId)) {
			return &actor;
		}
	}
	return nullptr;
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
			|| left.behavior.type != right.behavior.type
			|| left.moveMode != right.moveMode
			|| !NearVec(left.objective.targetPosition, right.objective.targetPosition)
			|| !NearVec(left.behavior.targetPosition, right.behavior.targetPosition)) {
			return false;
		}
	}
	return true;
}

void ExpectSessionChildrenPreserved(
	const iggy::runtime::RuntimeGameplayState &actual,
	const iggy::runtime::RuntimeGameplayState &input,
	const char *message)
{
	Expect(actual.session.hasPlayer == input.session.hasPlayer, message);
	Expect(actual.session.player.id == input.session.player.id, message);
	Expect(NearVec(actual.session.player.position, input.session.player.position), message);
	Expect(SameControls(actual.npcControls, input.npcControls), message);
	Expect(actual.interaction.targets.targets().size() == input.interaction.targets.targets().size(), message);
	Expect(actual.interaction.effects.entries().size() == input.interaction.effects.entries().size(), message);
	Expect(actual.inventory.inventory.stacks.size() == input.inventory.inventory.stacks.size(), message);
	Expect(actual.inventory.drops.drops.size() == input.inventory.drops.drops.size(), message);
}

iggy::runtime::RuntimeGameplayState TwoMoverState(const iggy::LevelTileMap &map)
{
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({
		Actor("npc:first", { 0.5F, 0.5F }),
		Actor("npc:second", { 2.5F, 0.5F }),
	});
	state.npcControls = Controls({
		SeekingControl("npc:first", { 1.5F, 0.5F }),
		SeekingControl("npc:second", { 1.5F, 0.5F }),
	});
	return state;
}

void TestRawFrameConsumesReservedCapacityOneRequests()
{
	const iggy::LevelTileMap map = Map({ "..." });
	const iggy::runtime::RuntimeGameplayState state = TwoMoverState(map);
	iggy::NpcActorMovementFramePlan2DConfig config;
	config.runReservation = true;

	const iggy::runtime::RuntimeNpcActorMovementRequestPlanResult plan = Plan(state, map, config);
	const iggy::runtime::RuntimeGameplayFrameResult frame = RawFrame(state, plan.requests);

	Expect(plan.preReservationRequestCount == 2, "raw capacity-one plan should see two candidate requests");
	Expect(plan.requestCount == 1, "raw capacity-one plan should publish one accepted request");
	Expect(plan.reservationAcceptedCount == 1 && plan.reservationRejectedCount == 1, "raw capacity-one plan should preserve reservation counts");
	Expect(NearVec(FindActor(frame.state.npcActors, "npc:first")->position, { 1.5F, 0.5F }), "raw capacity-one frame should move first NPC");
	Expect(NearVec(FindActor(frame.state.npcActors, "npc:second")->position, { 2.5F, 0.5F }), "raw capacity-one frame should leave rejected NPC unchanged");
	Expect(frame.npcMovement.apply.movedCount == 1, "raw capacity-one frame should apply one movement");
	ExpectSessionChildrenPreserved(frame.state, state, "raw capacity-one frame should preserve non-NPC-movement state");
}

void TestPolicyFrameConsumesReservedCapacityOneRequests()
{
	const iggy::LevelTileMap map = Map({ "..." });
	const iggy::runtime::RuntimeGameplayState state = TwoMoverState(map);
	iggy::NpcActorMovementFramePlan2DConfig config;
	config.runReservation = true;

	const iggy::runtime::RuntimeNpcActorMovementRequestPlanResult plan = Plan(state, map, config);
	const iggy::runtime::RuntimePolicyGameplayFrameResult frame = PolicyFrame(state, plan.requests);

	Expect(plan.requestCount == 1, "policy capacity-one plan should publish one accepted request");
	Expect(frame.npcMovement.apply.movedCount == 1, "policy capacity-one frame should apply one movement");
	Expect(NearVec(FindActor(frame.state.npcActors, "npc:first")->position, { 1.5F, 0.5F }), "policy capacity-one frame should move first NPC");
	Expect(NearVec(FindActor(frame.state.npcActors, "npc:second")->position, { 2.5F, 0.5F }), "policy capacity-one frame should leave rejected NPC unchanged");
	ExpectSessionChildrenPreserved(frame.state, state, "policy capacity-one frame should preserve non-NPC-movement state");
}

void TestRawFrameConsumesReservedCapacityTwoRequests()
{
	const iggy::LevelTileMap map = Map({ "..." });
	const iggy::runtime::RuntimeGameplayState state = TwoMoverState(map);
	iggy::NpcActorMovementFramePlan2DConfig config;
	config.runReservation = true;
	config.reservation.policy.maxOccupantsPerTile = 2;

	const iggy::runtime::RuntimeNpcActorMovementRequestPlanResult plan = Plan(state, map, config);
	const iggy::runtime::RuntimeGameplayFrameResult frame = RawFrame(state, plan.requests);
	const iggy::NpcActorOccupancy2D occupancy =
		iggy::NpcActorOccupancyProjector2D {}.project(frame.state.npcActors);
	const iggy::NpcActorOccupancyQuery2DResult occupants =
		iggy::npcActorOccupantsAt(occupancy, { 1, 0 });

	Expect(plan.requestCount == 2, "raw capacity-two plan should publish both accepted requests");
	Expect(plan.reservationAcceptedCount == 2 && plan.reservationRejectedCount == 0, "raw capacity-two plan should preserve reservation counts");
	Expect(frame.npcMovement.apply.movedCount == 2, "raw capacity-two frame should apply two movements");
	Expect(NearVec(FindActor(frame.state.npcActors, "npc:first")->position, { 1.5F, 0.5F }), "raw capacity-two frame should move first NPC");
	Expect(NearVec(FindActor(frame.state.npcActors, "npc:second")->position, { 1.5F, 0.5F }), "raw capacity-two frame should move second NPC onto shared tile");
	Expect(occupancy.hasIssues(), "raw capacity-two final occupancy should remain inspectable as duplicate occupancy");
	Expect(occupants.npcIds.size() == 2, "raw capacity-two occupancy query should expose both stacked NPCs");
}

void TestPolicyFrameConsumesReservedCapacityTwoRequests()
{
	const iggy::LevelTileMap map = Map({ "..." });
	const iggy::runtime::RuntimeGameplayState state = TwoMoverState(map);
	iggy::NpcActorMovementFramePlan2DConfig config;
	config.runReservation = true;
	config.reservation.policy.maxOccupantsPerTile = 2;

	const iggy::runtime::RuntimeNpcActorMovementRequestPlanResult plan = Plan(state, map, config);
	const iggy::runtime::RuntimePolicyGameplayFrameResult frame = PolicyFrame(state, plan.requests);
	const iggy::NpcActorOccupancy2D occupancy =
		iggy::NpcActorOccupancyProjector2D {}.project(frame.state.npcActors);
	const iggy::NpcActorOccupancyQuery2DResult occupants =
		iggy::npcActorOccupantsAt(occupancy, { 1, 0 });

	Expect(plan.requestCount == 2, "policy capacity-two plan should publish both accepted requests");
	Expect(frame.npcMovement.apply.movedCount == 2, "policy capacity-two frame should apply two movements");
	Expect(NearVec(FindActor(frame.state.npcActors, "npc:first")->position, { 1.5F, 0.5F }), "policy capacity-two frame should move first NPC");
	Expect(NearVec(FindActor(frame.state.npcActors, "npc:second")->position, { 1.5F, 0.5F }), "policy capacity-two frame should move second NPC onto shared tile");
	Expect(occupancy.hasIssues(), "policy capacity-two final occupancy should remain inspectable as duplicate occupancy");
	Expect(occupants.npcIds.size() == 2, "policy capacity-two occupancy query should expose both stacked NPCs");
}

void TestDefaultNoPolicyPathStillHardBlocks()
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

	const iggy::runtime::RuntimeNpcActorMovementRequestPlanResult plan = Plan(state, map);
	const iggy::runtime::RuntimeGameplayFrameResult frame = RawFrame(state, plan.requests);

	Expect(plan.requestCount == 1 && plan.blockedRequestCount == 1, "default planner path should preserve hard-block request diagnostics");
	Expect(plan.requests[0].filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "default planner request should be blocked by NPC");
	Expect(plan.requests[0].filter.blockingNpcId == Id("npc:blocker"), "default planner request should preserve blocker id");
	Expect(frame.npcMovement.apply.blockedCount == 1, "default raw runtime frame should report blocked movement");
	Expect(NearVec(FindActor(frame.state.npcActors, "npc:mover")->position, { 0.5F, 0.5F }), "default raw runtime frame should not move hard-blocked NPC");
	ExpectSessionChildrenPreserved(frame.state, state, "default hard-block frame should preserve non-NPC-movement state");
}

void TestInputsAreNotMutated()
{
	const iggy::LevelTileMap map = Map({ "..." });
	const iggy::runtime::RuntimeGameplayState state = TwoMoverState(map);
	const iggy::runtime::RuntimeGameplayState stateBefore = state;
	iggy::NpcActorMovementFramePlan2DConfig config;
	config.runReservation = true;
	config.reservation.policy.maxOccupantsPerTile = 2;

	const iggy::runtime::RuntimeNpcActorMovementRequestPlanResult plan = Plan(state, map, config);
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requestsBefore = plan.requests;
	(void)RawFrame(state, plan.requests);
	(void)PolicyFrame(state, plan.requests);

	Expect(NearVec(FindActor(state.npcActors, "npc:first")->position, FindActor(stateBefore.npcActors, "npc:first")->position), "policy reservation acceptance should not mutate first input actor");
	Expect(NearVec(FindActor(state.npcActors, "npc:second")->position, FindActor(stateBefore.npcActors, "npc:second")->position), "policy reservation acceptance should not mutate second input actor");
	Expect(SameControls(state.npcControls, stateBefore.npcControls), "policy reservation acceptance should not mutate input controls");
	Expect(plan.requests.size() == requestsBefore.size(), "policy reservation acceptance should not mutate request vector size");
	Expect(plan.requests[0].filter.step.npcId == requestsBefore[0].filter.step.npcId, "policy reservation acceptance should not mutate request payload");
}

} // namespace

int main()
{
	TestRawFrameConsumesReservedCapacityOneRequests();
	TestPolicyFrameConsumesReservedCapacityOneRequests();
	TestRawFrameConsumesReservedCapacityTwoRequests();
	TestPolicyFrameConsumesReservedCapacityTwoRequests();
	TestDefaultNoPolicyPathStillHardBlocks();
	TestInputsAreNotMutated();

	return Failures;
}
