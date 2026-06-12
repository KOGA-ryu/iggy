#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

#include "runtime/RuntimeTickRunner.hpp"
#include "scene/level/LevelRuntimeBuilder.hpp"

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

iggy::LevelBlueprint BlueprintWithNpc()
{
	iggy::LevelBlueprint blueprint;
	blueprint.id = iggy::ResourceId { "level:cellar_01" };
	blueprint.bounds = { 5, 3 };
	blueprint.tiles = {
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
	};
	blueprint.playerStarts.push_back({ 0, 0 });
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "enemy:skeleton" }, 0, 1, iggy::ResourceId { "spawn:skeleton_01" } });
	return blueprint;
}

iggy::npc_ai::NpcAgentTickConfig Config(float maxDistance = 0.25F)
{
	iggy::npc_ai::NpcAgentTickConfig config;
	config.maxDistance = maxDistance;
	config.awareness = { 8.0F, 0 };
	return config;
}

iggy::LevelRuntimeState StateFromBuild(const iggy::LevelRuntimeBuildResult &build)
{
	return { build.tileMap, build.npcAgents };
}

iggy::runtime::RuntimeTickRunInput Input(iggy::LevelRuntimeState state, std::size_t tickCount)
{
	return { state, { 4.5F, 1.5F }, Config(), tickCount };
}

bool HasEvent(const iggy::npc_ai::NpcTickReport &report, iggy::npc_ai::NpcTickEventType type)
{
	for (const iggy::npc_ai::NpcTickEvent &event : report.events) {
		if (event.type == type)
			return true;
	}
	return false;
}

void TestZeroTicksStable()
{
	const iggy::LevelRuntimeState state = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	const iggy::runtime::RuntimeTickRunResult result = iggy::runtime::RuntimeTickRunner {}.run(Input(state, 0));

	Expect(result.finalState.npcAgents.size() == state.npcAgents.size(), "zero ticks should preserve NPC count");
	Expect(result.finalState.npcAgents[0].state.position == state.npcAgents[0].state.position, "zero ticks should preserve NPC position");
	Expect(result.reportsByTick.empty(), "zero ticks should produce no reports");
}

void TestOneTickMovesLikeRuntimeTick()
{
	const iggy::LevelRuntimeState state = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	const iggy::runtime::RuntimeTickRunResult result = iggy::runtime::RuntimeTickRunner {}.run(Input(state, 1));

	Expect(NearVec(result.finalState.npcAgents[0].state.position, { 0.75F, 1.5F }), "one tick runner should match RuntimeTick movement");
	Expect(result.reportsByTick.size() == 1, "one tick runner should have one report group");
	Expect(result.reportsByTick[0].size() == 1, "one tick runner should preserve one NPC report");
}

void TestMultipleTicksContinueFromPriorOutput()
{
	const iggy::LevelRuntimeState state = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	const iggy::runtime::RuntimeTickRunResult result = iggy::runtime::RuntimeTickRunner {}.run(Input(state, 3));

	Expect(NearVec(result.finalState.npcAgents[0].state.position, { 1.25F, 1.5F }), "multiple ticks should continue movement from prior output");
	Expect(result.reportsByTick.size() == 3, "multiple ticks should produce report group per tick");
}

void TestReportsGroupedByTickOrder()
{
	const iggy::LevelRuntimeState state = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	const iggy::runtime::RuntimeTickRunResult result = iggy::runtime::RuntimeTickRunner {}.run(Input(state, 2));

	Expect(result.reportsByTick.size() == 2, "reports should be grouped by tick");
	Expect(result.reportsByTick[0].size() == 1 && result.reportsByTick[1].size() == 1, "each tick should preserve NPC reports");
	Expect(result.reportsByTick[0][0].id == iggy::ResourceId { "spawn:skeleton_01" }, "first tick report should preserve NPC id");
	Expect(result.reportsByTick[1][0].id == iggy::ResourceId { "spawn:skeleton_01" }, "second tick report should preserve NPC id");
	Expect(HasEvent(result.reportsByTick[0][0].report, iggy::npc_ai::NpcTickEventType::PositionChanged), "first tick should report position change");
	Expect(HasEvent(result.reportsByTick[1][0].report, iggy::npc_ai::NpcTickEventType::PositionChanged), "second tick should report position change");
}

void TestInitialInputStateUnchanged()
{
	const iggy::LevelRuntimeState state = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	const iggy::Vec2 originalPosition = state.npcAgents[0].state.position;
	const iggy::runtime::RuntimeTickRunResult result = iggy::runtime::RuntimeTickRunner {}.run(Input(state, 2));

	Expect(state.npcAgents[0].state.position == originalPosition, "tick runner should not mutate initial state");
	Expect(!NearVec(result.finalState.npcAgents[0].state.position, originalPosition), "tick runner should return changed final state separately");
}

void TestMapPreserved()
{
	const iggy::LevelRuntimeState state = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	const iggy::runtime::RuntimeTickRunResult result = iggy::runtime::RuntimeTickRunner {}.run(Input(state, 3));

	Expect(result.finalState.map.id == iggy::ResourceId { "level:cellar_01" }, "tick runner should preserve map id");
	Expect(result.finalState.map.width == state.map.width && result.finalState.map.height == state.map.height, "tick runner should preserve map dimensions");
	Expect(result.finalState.map.tiles.size() == state.map.tiles.size(), "tick runner should preserve map tiles");
}

} // namespace

int main()
{
	TestZeroTicksStable();
	TestOneTickMovesLikeRuntimeTick();
	TestMultipleTicksContinueFromPriorOutput();
	TestReportsGroupedByTickOrder();
	TestInitialInputStateUnchanged();
	TestMapPreserved();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
