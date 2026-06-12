#include <cmath>
#include <cstdlib>

#include "modules/npc_ai/NpcAgentController.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;
using iggy::test::SameTile;

iggy::npc_ai::NpcAgentState AgentAt(iggy::Vec2 position)
{
	iggy::npc_ai::NpcAgentState state;
	state.position = position;
	state.homeTile = { 0, 1 };
	return state;
}

iggy::npc_ai::NpcAgentTickConfig Config(float maxDistance = 0.25F)
{
	iggy::npc_ai::NpcAgentTickConfig config;
	config.maxDistance = maxDistance;
	config.awareness = { 8.0F, 0 };
	return config;
}

void TestFirstTickSeesTargetAndMoves()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		".....",
		".....",
	});
	const iggy::npc_ai::NpcAgentTickResult result = iggy::npc_ai::NpcAgentController {}.tick(map, AgentAt({ 0.5F, 1.5F }), { 4.5F, 1.5F }, Config());

	Expect(result.brain.awarenessState.playerVisible, "first tick should see clear target");
	Expect(result.brain.intent.type == iggy::npc_ai::NpcIntentType::PursueVisibleTarget, "first tick should pursue visible target");
	Expect(result.brain.navigation.status == iggy::npc_ai::NpcNavigationStatus::Moving, "first tick should start moving");
	Expect(NearVec(result.state.position, { 0.75F, 1.5F }), "first tick should update agent position");
	Expect(result.state.awareness.playerVisible, "first tick should store updated awareness");
}

void TestSecondTickContinuesFromUpdatedState()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		".....",
		".....",
	});
	const iggy::npc_ai::NpcAgentController controller;
	const iggy::npc_ai::NpcAgentTickResult first = controller.tick(map, AgentAt({ 0.5F, 1.5F }), { 4.5F, 1.5F }, Config());
	const iggy::npc_ai::NpcAgentTickResult second = controller.tick(map, first.state, { 4.5F, 1.5F }, Config());

	Expect(NearVec(first.state.position, { 0.75F, 1.5F }), "first tick should advance to expected position");
	Expect(NearVec(second.state.position, { 1.0F, 1.5F }), "second tick should continue from updated position");
	Expect(second.state.followState.waypointIndex >= first.state.followState.waypointIndex, "second tick should carry follow state forward");
}

void TestPriorAwarenessPersistsIntoBlockedInvestigate()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		"..#..",
		".....",
	});
	iggy::npc_ai::NpcAgentState state = AgentAt({ 0.5F, 2.5F });
	state.awareness.alerted = true;
	state.awareness.alertTicksRemaining = 2;
	state.awareness.lastSeenTile = { 4, 1 };
	state.awareness.lastSeenPosition = { 4.5F, 1.5F };

	iggy::npc_ai::NpcAgentTickConfig config = Config();
	config.awareness = { 8.0F, 2 };
	const iggy::npc_ai::NpcAgentTickResult result = iggy::npc_ai::NpcAgentController {}.tick(map, state, { 4.5F, 1.5F }, config);

	Expect(result.brain.intent.type == iggy::npc_ai::NpcIntentType::InvestigateLastSeen, "persisted awareness should investigate blocked target");
	Expect(SameTile(result.brain.intent.targetTile, 4, 1), "investigate intent should use persisted last seen tile");
	Expect(result.brain.navigation.status == iggy::npc_ai::NpcNavigationStatus::Moving, "investigate intent should move when path exists");
	Expect(!NearVec(result.state.position, state.position), "investigate tick should update position");
	Expect(result.state.awareness.alerted, "investigate tick should keep updated alert state");
}

void TestAlertExpirationClearsAwarenessInState()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		"..#..",
		".....",
	});
	iggy::npc_ai::NpcAgentState state = AgentAt({ 0.5F, 1.5F });
	state.awareness.alerted = true;
	state.awareness.alertTicksRemaining = 0;
	state.awareness.lastSeenTile = { 4, 1 };

	const iggy::npc_ai::NpcAgentTickResult result = iggy::npc_ai::NpcAgentController {}.tick(map, state, { 4.5F, 1.5F }, Config());

	Expect(result.brain.awarenessEvent.type == iggy::npc_ai::AwarenessEventType::AlertExpired, "expired awareness should emit event");
	Expect(!result.state.awareness.alerted, "expired awareness should clear alert in returned state");
	Expect(result.state.awareness.alertTicksRemaining == 0, "expired awareness should have no remaining memory ticks");
}

void TestPathFailurePreservesPreviousPosition()
{
	const iggy::LevelTileMap map = MapFromRows({
		"..#..",
		"..#..",
		"..#..",
	});
	iggy::npc_ai::NpcAgentState state = AgentAt({ 0.5F, 1.5F });
	state.awareness.alerted = true;
	state.awareness.alertTicksRemaining = 2;
	state.awareness.lastSeenTile = { 4, 1 };
	state.awareness.lastSeenPosition = { 4.5F, 1.5F };

	iggy::npc_ai::NpcAgentTickConfig config = Config();
	config.awareness = { 8.0F, 2 };
	const iggy::npc_ai::NpcAgentTickResult result = iggy::npc_ai::NpcAgentController {}.tick(map, state, { 4.5F, 1.5F }, config);

	Expect(result.brain.intent.type == iggy::npc_ai::NpcIntentType::InvestigateLastSeen, "path failure should still preserve chosen intent");
	Expect(result.brain.navigation.status == iggy::npc_ai::NpcNavigationStatus::PathNotFound, "path failure should report navigation failure");
	Expect(result.state.position == state.position, "path failure should preserve previous position");
}

void TestIdleTickPreservesStateWhenNoMovementRequested()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		"..#..",
		".....",
	});
	iggy::npc_ai::NpcAgentState state = AgentAt({ 0.5F, 1.5F });
	state.homeTile = { 0, 1 };

	const iggy::npc_ai::NpcAgentTickResult result = iggy::npc_ai::NpcAgentController {}.tick(map, state, { 4.5F, 1.5F }, Config());

	Expect(result.brain.intent.type == iggy::npc_ai::NpcIntentType::Idle, "blocked target without awareness should idle");
	Expect(result.brain.navigation.status == iggy::npc_ai::NpcNavigationStatus::NoMovement, "idle tick should request no movement");
	Expect(result.state.position == state.position, "idle tick should preserve position");
	Expect(result.state.homeTile.x == state.homeTile.x && result.state.homeTile.y == state.homeTile.y, "idle tick should preserve home tile");
}

} // namespace

int main()
{
	TestFirstTickSeesTargetAndMoves();
	TestSecondTickContinuesFromUpdatedState();
	TestPriorAwarenessPersistsIntoBlockedInvestigate();
	TestAlertExpirationClearsAwarenessInState();
	TestPathFailurePreservesPreviousPosition();
	TestIdleTickPreservesStateWhenNoMovementRequested();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
