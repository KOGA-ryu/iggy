#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

#include "runtime/RuntimeTick.hpp"
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

iggy::runtime::RuntimeTickInput Input(iggy::LevelRuntimeState state)
{
	return { state, { 4.5F, 1.5F }, Config() };
}

bool HasEvent(const iggy::npc_ai::NpcTickReport &report, iggy::npc_ai::NpcTickEventType type)
{
	for (const iggy::npc_ai::NpcTickEvent &event : report.events) {
		if (event.type == type)
			return true;
	}
	return false;
}

void TestEmptyStateStable()
{
	iggy::LevelRuntimeState state = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	state.npcAgents.clear();
	const iggy::runtime::RuntimeTickResult result = iggy::runtime::RuntimeTick {}.run(Input(state));

	Expect(result.state.npcAgents.empty(), "runtime tick with empty NPC state should keep empty NPCs");
	Expect(result.npcReports.empty(), "runtime tick with empty NPC state should produce no reports");
	Expect(result.state.map.id == state.map.id, "runtime tick with empty NPC state should preserve map");
}

void TestVisibleNpcMovesThroughRuntimeTick()
{
	const iggy::LevelRuntimeState state = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	const iggy::runtime::RuntimeTickResult result = iggy::runtime::RuntimeTick {}.run(Input(state));

	Expect(result.state.npcAgents.size() == 1, "runtime tick should preserve NPC count");
	Expect(NearVec(result.state.npcAgents[0].state.position, { 0.75F, 1.5F }), "runtime tick should move visible NPC");
}

void TestInputStateUnchanged()
{
	const iggy::LevelRuntimeState state = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	const iggy::Vec2 originalPosition = state.npcAgents[0].state.position;
	const iggy::runtime::RuntimeTickResult result = iggy::runtime::RuntimeTick {}.run(Input(state));

	Expect(state.npcAgents[0].state.position == originalPosition, "runtime tick should not mutate input state");
	Expect(!NearVec(result.state.npcAgents[0].state.position, originalPosition), "runtime tick should return updated state separately");
}

void TestReportsPreserved()
{
	const iggy::LevelRuntimeState state = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	const iggy::runtime::RuntimeTickResult result = iggy::runtime::RuntimeTick {}.run(Input(state));

	Expect(result.npcReports.size() == 1, "runtime tick should preserve NPC reports");
	Expect(result.npcReports[0].id == iggy::ResourceId { "spawn:skeleton_01" }, "runtime tick report should preserve NPC id");
	Expect(HasEvent(result.npcReports[0].report, iggy::npc_ai::NpcTickEventType::PositionChanged), "runtime tick report should preserve position change event");
}

void TestMapPreserved()
{
	const iggy::LevelRuntimeState state = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	const iggy::runtime::RuntimeTickResult result = iggy::runtime::RuntimeTick {}.run(Input(state));

	Expect(result.state.map.id == iggy::ResourceId { "level:cellar_01" }, "runtime tick should preserve map id");
	Expect(result.state.map.width == state.map.width && result.state.map.height == state.map.height, "runtime tick should preserve map dimensions");
	Expect(result.state.map.tiles.size() == state.map.tiles.size(), "runtime tick should preserve map tiles");
}

} // namespace

int main()
{
	TestEmptyStateStable();
	TestVisibleNpcMovesThroughRuntimeTick();
	TestInputStateUnchanged();
	TestReportsPreserved();
	TestMapPreserved();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
