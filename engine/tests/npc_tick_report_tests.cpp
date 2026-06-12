#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

#include "modules/npc_ai/NpcAgentController.hpp"
#include "modules/npc_ai/NpcTickReporter.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
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

bool HasEvent(const iggy::npc_ai::NpcTickReport &report, iggy::npc_ai::NpcTickEventType type)
{
	for (const iggy::npc_ai::NpcTickEvent &event : report.events) {
		if (event.type == type)
			return true;
	}
	return false;
}

iggy::npc_ai::NpcTickReport RunReport(const iggy::LevelTileMap &map, const iggy::npc_ai::NpcAgentState &state, iggy::Vec2 playerPosition, const iggy::npc_ai::NpcAgentTickConfig &config)
{
	const iggy::npc_ai::NpcAgentTickResult tick = iggy::npc_ai::NpcAgentController {}.tick(map, state, playerPosition, config);
	return iggy::npc_ai::NpcTickReporter {}.report(state, tick);
}

void TestVisibleTargetReportContainsFacts()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		".....",
		".....",
	});
	const iggy::npc_ai::NpcTickReport report = RunReport(map, AgentAt({ 0.5F, 1.5F }), { 4.5F, 1.5F }, Config());

	Expect(report.awarenessEvent.type == iggy::npc_ai::AwarenessEventType::PlayerSeen, "visible target report should preserve awareness event");
	Expect(report.intent.type == iggy::npc_ai::NpcIntentType::PursueVisibleTarget, "visible target report should preserve intent");
	Expect(report.navigation.status == iggy::npc_ai::NpcNavigationStatus::Moving, "visible target report should preserve navigation status");
	Expect(HasEvent(report, iggy::npc_ai::NpcTickEventType::TargetSeen), "visible target report should include TargetSeen event");
	Expect(HasEvent(report, iggy::npc_ai::NpcTickEventType::IntentSelected), "visible target report should include IntentSelected event");
	Expect(HasEvent(report, iggy::npc_ai::NpcTickEventType::NavigationAdvanced), "visible target report should include NavigationAdvanced event");
	Expect(HasEvent(report, iggy::npc_ai::NpcTickEventType::PositionChanged), "visible target report should include PositionChanged event");
}

void TestIdleTickHasNoPositionChange()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		"..#..",
		".....",
	});
	const iggy::npc_ai::NpcTickReport report = RunReport(map, AgentAt({ 0.5F, 1.5F }), { 4.5F, 1.5F }, Config());

	Expect(report.intent.type == iggy::npc_ai::NpcIntentType::Idle, "idle report should preserve idle intent");
	Expect(HasEvent(report, iggy::npc_ai::NpcTickEventType::IntentSelected), "idle report should include IntentSelected event");
	Expect(HasEvent(report, iggy::npc_ai::NpcTickEventType::NoMovement), "idle report should include NoMovement event");
	Expect(!HasEvent(report, iggy::npc_ai::NpcTickEventType::PositionChanged), "idle report should not include PositionChanged event");
	Expect(report.previousPosition == report.nextPosition, "idle report should preserve position fields");
}

void TestAlertExpirationIsReported()
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
	const iggy::npc_ai::NpcTickReport report = RunReport(map, state, { 4.5F, 1.5F }, Config());

	Expect(report.awarenessEvent.type == iggy::npc_ai::AwarenessEventType::AlertExpired, "alert expiration report should preserve awareness event");
	Expect(HasEvent(report, iggy::npc_ai::NpcTickEventType::AlertExpired), "alert expiration report should include AlertExpired event");
	Expect(HasEvent(report, iggy::npc_ai::NpcTickEventType::IntentSelected), "alert expiration report should include IntentSelected event");
}

void TestNavigationFailureIsReported()
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
	const iggy::npc_ai::NpcTickReport report = RunReport(map, state, { 4.5F, 1.5F }, config);

	Expect(report.navigation.status == iggy::npc_ai::NpcNavigationStatus::PathNotFound, "navigation failure report should preserve failure status");
	Expect(HasEvent(report, iggy::npc_ai::NpcTickEventType::NavigationFailed), "navigation failure report should include NavigationFailed event");
	Expect(!HasEvent(report, iggy::npc_ai::NpcTickEventType::PositionChanged), "navigation failure report should not include PositionChanged event");
	Expect(report.previousPosition == report.nextPosition, "navigation failure report should preserve position fields");
}

void TestArrivalIsReported()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::npc_ai::NpcTickReport report = RunReport(map, AgentAt({ 0.5F, 1.5F }), { 1.5F, 1.5F }, Config(4.0F));

	Expect(report.navigation.status == iggy::npc_ai::NpcNavigationStatus::Arrived, "arrival report should preserve arrival status");
	Expect(HasEvent(report, iggy::npc_ai::NpcTickEventType::NavigationArrived), "arrival report should include NavigationArrived event");
	Expect(HasEvent(report, iggy::npc_ai::NpcTickEventType::PositionChanged), "arrival report should include PositionChanged event when position changed");
	Expect(report.nextPosition == iggy::Vec2 { 1.5F, 1.5F }, "arrival report should preserve destination position");
}

} // namespace

int main()
{
	TestVisibleTargetReportContainsFacts();
	TestIdleTickHasNoPositionChange();
	TestAlertExpirationIsReported();
	TestNavigationFailureIsReported();
	TestArrivalIsReported();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
