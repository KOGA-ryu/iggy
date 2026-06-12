#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

#include "scene/level/LevelNpcUpdateStep.hpp"

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

void TestEmptyNpcBatch()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::LevelNpcUpdateResult result = iggy::LevelNpcUpdateStep {}.update(map, {}, { 2.5F, 1.5F }, Config());

	Expect(result.npcAgents.empty(), "empty level NPC batch should return no agents");
	Expect(result.reports.empty(), "empty level NPC batch should return no reports");
}

void TestOneVisibleNpcMovesAndReportsPositionChange()
{
	const iggy::LevelTileMap map = MapFromRows({
		".....",
		".....",
		".....",
	});
	const std::vector<iggy::npc_ai::NpcAgentEntry> agents = {
		Entry("npc:guard_01", { 0.5F, 1.5F }),
	};
	const iggy::LevelNpcUpdateResult result = iggy::LevelNpcUpdateStep {}.update(map, agents, { 4.5F, 1.5F }, Config());

	Expect(result.npcAgents.size() == 1, "single visible NPC should return one updated agent");
	Expect(result.reports.size() == 1, "single visible NPC should return one report");
	Expect(NearVec(result.npcAgents[0].state.position, { 0.75F, 1.5F }), "visible NPC should move toward target");
	Expect(HasEvent(result.reports[0].report, iggy::npc_ai::NpcTickEventType::PositionChanged), "visible NPC report should include PositionChanged");
}

void TestMultipleNpcsPreserveIdAndOrder()
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
	const iggy::LevelNpcUpdateResult result = iggy::LevelNpcUpdateStep {}.update(map, agents, { 4.5F, 1.5F }, Config());

	Expect(result.npcAgents.size() == 3, "level NPC update should return all agents");
	Expect(result.reports.size() == 3, "level NPC update should return all reports");
	Expect(result.npcAgents[0].id == iggy::ResourceId { "npc:first" }, "first NPC id should preserve order");
	Expect(result.npcAgents[1].id == iggy::ResourceId { "npc:second" }, "second NPC id should preserve order");
	Expect(result.npcAgents[2].id == iggy::ResourceId { "npc:third" }, "third NPC id should preserve order");
	Expect(result.reports[0].id == iggy::ResourceId { "npc:first" }, "first report id should preserve order");
	Expect(result.reports[1].id == iggy::ResourceId { "npc:second" }, "second report id should preserve order");
	Expect(result.reports[2].id == iggy::ResourceId { "npc:third" }, "third report id should preserve order");
}

void TestMixedVisibleAndBlockedNpcsUpdateIndependently()
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
	const iggy::LevelNpcUpdateResult result = iggy::LevelNpcUpdateStep {}.update(map, agents, { 4.5F, 1.5F }, Config());

	Expect(NearVec(result.npcAgents[0].state.position, { 4.5F, 2.25F }), "visible NPC should move independently");
	Expect(result.npcAgents[1].state.position == agents[1].state.position, "blocked NPC should remain unchanged");
	Expect(HasEvent(result.reports[0].report, iggy::npc_ai::NpcTickEventType::PositionChanged), "visible NPC report should include PositionChanged");
	Expect(!HasEvent(result.reports[1].report, iggy::npc_ai::NpcTickEventType::PositionChanged), "blocked NPC report should not include PositionChanged");
}

void TestInputNpcBatchIsNotMutated()
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
	const iggy::LevelNpcUpdateResult result = iggy::LevelNpcUpdateStep {}.update(map, agents, { 4.5F, 1.5F }, Config());

	Expect(agents[0].state.position == originalPosition, "level NPC update should not mutate input agents");
	Expect(!NearVec(result.npcAgents[0].state.position, originalPosition), "level NPC update should return updated agents separately");
}

} // namespace

int main()
{
	TestEmptyNpcBatch();
	TestOneVisibleNpcMovesAndReportsPositionChange();
	TestMultipleNpcsPreserveIdAndOrder();
	TestMixedVisibleAndBlockedNpcsUpdateIndependently();
	TestInputNpcBatchIsNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
