#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

#include "modules/npc_ai/NpcAgentBatchUpdater.hpp"

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

iggy::npc_ai::NpcAgentEntry Entry(std::string id, iggy::Vec2 position)
{
	return { iggy::ResourceId { std::move(id) }, AgentAt(position) };
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

void TestEmptyBatch()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::npc_ai::NpcAgentBatchUpdateResult result = iggy::npc_ai::NpcAgentBatchUpdater {}.update(map, {}, { 2.5F, 1.5F }, Config());

	Expect(result.agents.empty(), "empty batch should return no updated agents");
	Expect(result.reports.empty(), "empty batch should return no reports");
}

void TestSingleVisibleAgentUpdatesStateAndReport()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		".....",
		".....",
	});
	const std::vector<iggy::npc_ai::NpcAgentEntry> agents = {
		Entry("npc:guard_01", { 0.5F, 1.5F }),
	};
	const iggy::npc_ai::NpcAgentBatchUpdateResult result = iggy::npc_ai::NpcAgentBatchUpdater {}.update(map, agents, { 4.5F, 1.5F }, Config());

	Expect(result.agents.size() == 1, "single-agent batch should return one updated agent");
	Expect(result.reports.size() == 1, "single-agent batch should return one report");
	Expect(result.agents[0].id == iggy::ResourceId { "npc:guard_01" }, "single-agent batch should preserve id");
	Expect(result.reports[0].id == iggy::ResourceId { "npc:guard_01" }, "single-agent report should preserve id");
	Expect(NearVec(result.agents[0].state.position, { 0.75F, 1.5F }), "visible agent should move toward target");
	Expect(HasEvent(result.reports[0].report, iggy::npc_ai::NpcTickEventType::TargetSeen), "visible agent report should include TargetSeen");
	Expect(HasEvent(result.reports[0].report, iggy::npc_ai::NpcTickEventType::PositionChanged), "visible agent report should include PositionChanged");
}

void TestMultipleAgentsPreserveOrder()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		".....",
		".....",
	});
	const std::vector<iggy::npc_ai::NpcAgentEntry> agents = {
		Entry("npc:first", { 0.5F, 1.5F }),
		Entry("npc:second", { 0.5F, 2.5F }),
		Entry("npc:third", { 1.5F, 0.5F }),
	};
	const iggy::npc_ai::NpcAgentBatchUpdateResult result = iggy::npc_ai::NpcAgentBatchUpdater {}.update(map, agents, { 4.5F, 1.5F }, Config());

	Expect(result.agents.size() == 3, "multi-agent batch should return all agents");
	Expect(result.reports.size() == 3, "multi-agent batch should return all reports");
	Expect(result.agents[0].id == iggy::ResourceId { "npc:first" }, "first updated agent should preserve order");
	Expect(result.agents[1].id == iggy::ResourceId { "npc:second" }, "second updated agent should preserve order");
	Expect(result.agents[2].id == iggy::ResourceId { "npc:third" }, "third updated agent should preserve order");
	Expect(result.reports[0].id == iggy::ResourceId { "npc:first" }, "first report should preserve order");
	Expect(result.reports[1].id == iggy::ResourceId { "npc:second" }, "second report should preserve order");
	Expect(result.reports[2].id == iggy::ResourceId { "npc:third" }, "third report should preserve order");
}

void TestMixedVisibleAndBlockedAgentsUpdateIndependently()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		"..#..",
		".....",
	});
	const std::vector<iggy::npc_ai::NpcAgentEntry> agents = {
		Entry("npc:visible", { 4.5F, 2.5F }),
		Entry("npc:blocked", { 0.5F, 1.5F }),
	};
	const iggy::npc_ai::NpcAgentBatchUpdateResult result = iggy::npc_ai::NpcAgentBatchUpdater {}.update(map, agents, { 4.5F, 1.5F }, Config());

	Expect(NearVec(result.agents[0].state.position, { 4.5F, 2.25F }), "visible agent should move independently");
	Expect(result.agents[1].state.position == agents[1].state.position, "blocked idle agent should remain unchanged");
	Expect(HasEvent(result.reports[0].report, iggy::npc_ai::NpcTickEventType::PositionChanged), "visible agent report should include PositionChanged");
	Expect(!HasEvent(result.reports[1].report, iggy::npc_ai::NpcTickEventType::PositionChanged), "blocked idle agent report should not include PositionChanged");
	Expect(result.reports[1].report.intent.type == iggy::npc_ai::NpcIntentType::Idle, "blocked idle agent should choose Idle");
}

void TestInputAgentsAreNotMutated()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		".....",
		".....",
	});
	std::vector<iggy::npc_ai::NpcAgentEntry> agents = {
		Entry("npc:guard_01", { 0.5F, 1.5F }),
	};
	const iggy::Vec2 originalPosition = agents[0].state.position;
	const iggy::npc_ai::NpcAgentBatchUpdateResult result = iggy::npc_ai::NpcAgentBatchUpdater {}.update(map, agents, { 4.5F, 1.5F }, Config());

	Expect(agents[0].state.position == originalPosition, "batch updater should not mutate input agent state");
	Expect(!NearVec(result.agents[0].state.position, originalPosition), "batch updater should return updated state separately");
}

} // namespace

int main()
{
	TestEmptyBatch();
	TestSingleVisibleAgentUpdatesStateAndReport();
	TestMultipleAgentsPreserveOrder();
	TestMixedVisibleAndBlockedAgentsUpdateIndependently();
	TestInputAgentsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
