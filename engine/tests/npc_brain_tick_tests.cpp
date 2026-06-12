#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

#include "modules/npc_ai/NpcBrainTick.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

bool Near(float actual, float expected, float tolerance = 0.0001F)
{
	return std::fabs(actual - expected) <= tolerance;
}

bool NearVec(iggy::Vec2 actual, iggy::Vec2 expected)
{
	return Near(actual.x, expected.x) && Near(actual.y, expected.y);
}

bool SameTile(iggy::line_of_sight::TileCoord actual, int x, int y)
{
	return actual.x == x && actual.y == y;
}

iggy::LevelTileMap MapFromRows(std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map;
	map.height = static_cast<int>(rows.size());
	map.width = rows.empty() ? 0 : static_cast<int>(rows.front().size());
	for (std::string_view row : rows) {
		for (char cell : row)
			map.tiles.push_back({ cell != '#' });
	}
	return map;
}

iggy::npc_ai::NpcBrainTickInput Input(iggy::Vec2 npcPosition, iggy::Vec2 playerPosition)
{
	iggy::npc_ai::NpcBrainTickInput input;
	input.npcPosition = npcPosition;
	input.playerPosition = playerPosition;
	input.homeTile = { 0, 1 };
	input.maxDistance = 0.25F;
	input.awarenessConfig = { 8.0F, 0 };
	return input;
}

void TestVisiblePlayerPursuesAndMoves()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		".....",
		".....",
	});
	const iggy::npc_ai::NpcBrainTickResult result = iggy::npc_ai::NpcBrainTick {}.tick(map, Input({ 0.5F, 1.5F }, { 4.5F, 1.5F }));

	Expect(result.awarenessState.playerVisible, "visible player should update awareness");
	Expect(result.intent.type == iggy::npc_ai::NpcIntentType::PursueVisibleTarget, "visible player should select pursue intent");
	Expect(result.movementPlan.type == iggy::npc_ai::NpcMovementPlanType::MoveTo, "pursue intent should create movement plan");
	Expect(result.navigation.status == iggy::npc_ai::NpcNavigationStatus::Moving, "visible player should start navigation movement");
	Expect(NearVec(result.nextPosition, { 0.75F, 1.5F }), "visible player tick should advance toward target");
}

void TestBlockedPlayerWithoutAwarenessDoesNotPursue()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		"..#..",
		".....",
	});
	const iggy::npc_ai::NpcBrainTickResult result = iggy::npc_ai::NpcBrainTick {}.tick(map, Input({ 0.5F, 1.5F }, { 4.5F, 1.5F }));

	Expect(!result.awarenessState.playerVisible, "blocked player should not be visible");
	Expect(result.intent.type == iggy::npc_ai::NpcIntentType::Idle, "blocked player without memory should idle");
	Expect(result.movementPlan.type == iggy::npc_ai::NpcMovementPlanType::None, "idle intent should not create movement plan");
	Expect(result.navigation.status == iggy::npc_ai::NpcNavigationStatus::NoMovement, "idle intent should produce no navigation movement");
	Expect(result.nextPosition == iggy::Vec2 { 0.5F, 1.5F }, "blocked player without memory should preserve position");
}

void TestAlertMemoryInvestigatesLastSeen()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		"..#..",
		".....",
	});
	iggy::npc_ai::NpcBrainTickInput input = Input({ 0.5F, 2.5F }, { 4.5F, 1.5F });
	input.awarenessState.alerted = true;
	input.awarenessState.alertTicksRemaining = 2;
	input.awarenessState.lastSeenTile = { 4, 1 };
	input.awarenessState.lastSeenPosition = { 4.5F, 1.5F };
	input.awarenessConfig = { 8.0F, 2 };

	const iggy::npc_ai::NpcBrainTickResult result = iggy::npc_ai::NpcBrainTick {}.tick(map, input);

	Expect(!result.awarenessState.playerVisible, "blocked remembered player should not be visible");
	Expect(result.awarenessState.alerted, "memory tick should keep NPC alerted");
	Expect(result.intent.type == iggy::npc_ai::NpcIntentType::InvestigateLastSeen, "alert memory should investigate");
	Expect(SameTile(result.intent.targetTile, 4, 1), "investigate intent should preserve last seen tile");
	Expect(result.navigation.status == iggy::npc_ai::NpcNavigationStatus::Moving, "investigate intent should move toward last seen tile");
	Expect(!NearVec(result.nextPosition, input.npcPosition), "investigate tick should advance position");
}

void TestExpiredMemoryIdles()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		"..#..",
		".....",
	});
	iggy::npc_ai::NpcBrainTickInput input = Input({ 0.5F, 1.5F }, { 4.5F, 1.5F });
	input.awarenessState.alerted = true;
	input.awarenessState.alertTicksRemaining = 0;
	input.awarenessState.lastSeenTile = { 4, 1 };

	const iggy::npc_ai::NpcBrainTickResult result = iggy::npc_ai::NpcBrainTick {}.tick(map, input);

	Expect(result.awarenessEvent.type == iggy::npc_ai::AwarenessEventType::AlertExpired, "expired memory should emit AlertExpired");
	Expect(!result.awarenessState.alerted, "expired memory should clear alert");
	Expect(result.intent.type == iggy::npc_ai::NpcIntentType::Idle, "expired memory should idle by default");
	Expect(result.navigation.status == iggy::npc_ai::NpcNavigationStatus::NoMovement, "idle after memory expiry should not move");
}

void TestExpiredMemoryCanReturnToPost()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		".....",
		".....",
	});
	iggy::npc_ai::NpcBrainTickInput input = Input({ 4.5F, 1.5F }, { 4.5F, 2.5F });
	input.awarenessConfig = { 0.25F, 0 };
	input.intentConfig = { true };
	input.homeTile = { 0, 1 };

	const iggy::npc_ai::NpcBrainTickResult result = iggy::npc_ai::NpcBrainTick {}.tick(map, input);

	Expect(result.intent.type == iggy::npc_ai::NpcIntentType::ReturnToPost, "configured unaware NPC should return to post");
	Expect(result.navigation.status == iggy::npc_ai::NpcNavigationStatus::Moving, "return-to-post intent should move when away from post");
	Expect(NearVec(result.nextPosition, { 4.25F, 1.5F }), "return-to-post tick should advance toward home tile");
}

void TestBlockedNavigationPreservesIntentPlanAndPosition()
{
	const iggy::LevelTileMap map = MapFromRows({
		"..#..",
		"..#..",
		"..#..",
	});
	iggy::npc_ai::NpcBrainTickInput input = Input({ 0.5F, 1.5F }, { 4.5F, 1.5F });
	input.awarenessState.alerted = true;
	input.awarenessState.alertTicksRemaining = 2;
	input.awarenessState.lastSeenTile = { 4, 1 };
	input.awarenessState.lastSeenPosition = { 4.5F, 1.5F };
	input.awarenessConfig = { 8.0F, 2 };

	const iggy::npc_ai::NpcBrainTickResult result = iggy::npc_ai::NpcBrainTick {}.tick(map, input);

	Expect(result.intent.type == iggy::npc_ai::NpcIntentType::InvestigateLastSeen, "alert memory should choose investigate before navigation fails");
	Expect(result.movementPlan.type == iggy::npc_ai::NpcMovementPlanType::MoveTo, "failed navigation should preserve movement plan");
	Expect(result.navigation.status == iggy::npc_ai::NpcNavigationStatus::PathNotFound, "unreachable last seen tile should report PathNotFound");
	Expect(result.nextPosition == input.npcPosition, "failed navigation should preserve current position");
}

} // namespace

int main()
{
	TestVisiblePlayerPursuesAndMoves();
	TestBlockedPlayerWithoutAwarenessDoesNotPursue();
	TestAlertMemoryInvestigatesLastSeen();
	TestExpiredMemoryIdles();
	TestExpiredMemoryCanReturnToPost();
	TestBlockedNavigationPreservesIntentPlanAndPosition();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
