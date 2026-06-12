#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

#include "modules/npc_ai/AwarenessSensor.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

bool SameTile(iggy::TileCoord actual, int x, int y)
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

void TestNpcSeesPlayerInClearLine()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		".....",
		".....",
	});
	iggy::npc_ai::AwarenessState state;
	const iggy::npc_ai::AwarenessEvent event = iggy::npc_ai::AwarenessSensor { { 8.0F, 0 } }.observe(map, { 0.5F, 1.5F }, { 4.5F, 1.5F }, state);

	Expect(event.type == iggy::npc_ai::AwarenessEventType::PlayerSeen, "clear line should emit PlayerSeen event");
	Expect(state.playerVisible, "clear line should mark player visible");
	Expect(state.alerted, "visible player should alert NPC");
}

void TestNpcDoesNotSeeThroughBlockedTile()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		"..#..",
		".....",
	});
	iggy::npc_ai::AwarenessState state;
	const iggy::npc_ai::AwarenessEvent event = iggy::npc_ai::AwarenessSensor { { 8.0F, 0 } }.observe(map, { 0.5F, 1.5F }, { 4.5F, 1.5F }, state);

	Expect(event.type == iggy::npc_ai::AwarenessEventType::None, "blocked line without prior awareness should emit no event");
	Expect(!state.playerVisible, "blocked line should not mark player visible");
	Expect(!state.alerted, "blocked line without memory should not alert NPC");
}

void TestNpcDoesNotSeeOutsideRange()
{
	const iggy::LevelTileMap map = MapFromRows({
		"..........",
		"..........",
		"..........",
	});
	iggy::npc_ai::AwarenessState state;
	const iggy::npc_ai::AwarenessEvent event = iggy::npc_ai::AwarenessSensor { { 3.0F, 0 } }.observe(map, { 0.5F, 1.5F }, { 5.5F, 1.5F }, state);

	Expect(event.type == iggy::npc_ai::AwarenessEventType::None, "out-of-range player should emit no event");
	Expect(!state.playerVisible, "out-of-range player should not be visible");
	Expect(!state.alerted, "out-of-range player without memory should not alert NPC");
}

void TestNpcRemembersLastSeenTileWhenVisible()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		".....",
		".....",
	});
	iggy::npc_ai::AwarenessState state;
	const iggy::npc_ai::AwarenessEvent event = iggy::npc_ai::AwarenessSensor { { 8.0F, 0 } }.observe(map, { 0.5F, 1.5F }, { 3.5F, 1.5F }, state);

	Expect(SameTile(state.lastSeenTile, 3, 1), "visible player should update last seen tile");
	Expect(state.lastSeenPosition == iggy::Vec2 { 3.5F, 1.5F }, "visible player should update last seen position");
	Expect(SameTile(event.lastSeenTile, 3, 1), "event should include last seen tile");
}

void TestNpcRemainsAlertedForMemoryTicksAfterLosingVisibility()
{
	iggy::LevelTileMap map = MapFromRows({
		".....",
		".....",
		".....",
	});
	iggy::npc_ai::AwarenessState state;
	iggy::npc_ai::AwarenessSensor sensor { { 8.0F, 2 } };

	const iggy::npc_ai::AwarenessEvent seen = sensor.observe(map, { 0.5F, 1.5F }, { 4.5F, 1.5F }, state);
	Expect(seen.type == iggy::npc_ai::AwarenessEventType::PlayerSeen, "initial visible player should emit PlayerSeen event");
	map.tiles[7].walkable = false;

	const iggy::npc_ai::AwarenessEvent lost = sensor.observe(map, { 0.5F, 1.5F }, { 4.5F, 1.5F }, state);
	Expect(lost.type == iggy::npc_ai::AwarenessEventType::PlayerLost, "losing visible player should emit PlayerLost event");
	Expect(!state.playerVisible, "blocked player should no longer be visible");
	Expect(state.alerted, "NPC should remain alerted on first memory tick");
	Expect(state.alertTicksRemaining == 1, "first memory tick should decrement remaining alert memory");
	Expect(SameTile(state.lastSeenTile, 4, 1), "losing visibility should preserve last seen tile");

	const iggy::npc_ai::AwarenessEvent memory = sensor.observe(map, { 0.5F, 1.5F }, { 4.5F, 1.5F }, state);
	Expect(memory.type == iggy::npc_ai::AwarenessEventType::None, "continued memory should not emit a new event");
	Expect(state.alerted, "NPC should remain alerted through configured memory ticks");
	Expect(state.alertTicksRemaining == 0, "second memory tick should consume alert memory");

	const iggy::npc_ai::AwarenessEvent expired = sensor.observe(map, { 0.5F, 1.5F }, { 4.5F, 1.5F }, state);
	Expect(expired.type == iggy::npc_ai::AwarenessEventType::AlertExpired, "expired memory should emit AlertExpired event");
	Expect(!state.alerted, "NPC should stop being alerted after memory expires");
}

} // namespace

int main()
{
	TestNpcSeesPlayerInClearLine();
	TestNpcDoesNotSeeThroughBlockedTile();
	TestNpcDoesNotSeeOutsideRange();
	TestNpcRemembersLastSeenTileWhenVisible();
	TestNpcRemainsAlertedForMemoryTicksAfterLosingVisibility();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
