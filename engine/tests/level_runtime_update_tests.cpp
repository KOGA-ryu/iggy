#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

#include "scene/level/LevelRuntimeBuilder.hpp"
#include "scene/level/LevelRuntimeUpdate.hpp"

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

bool HasEvent(const iggy::npc_ai::NpcTickReport &report, iggy::npc_ai::NpcTickEventType type)
{
	for (const iggy::npc_ai::NpcTickEvent &event : report.events) {
		if (event.type == type)
			return true;
	}
	return false;
}

void TestUpdateStateFromRuntimeBuilder()
{
	const iggy::LevelRuntimeBuildResult build = iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc());
	const iggy::LevelRuntimeState state = StateFromBuild(build);
	const iggy::LevelRuntimeUpdateResult result = iggy::LevelRuntimeUpdate {}.updateNpcAgents(state, { 4.5F, 1.5F }, Config());

	Expect(build.built, "test blueprint should build runtime state");
	Expect(result.state.npcAgents.size() == 1, "runtime update should preserve NPC count");
	Expect(result.npcReports.size() == 1, "runtime update should produce one NPC report");
	Expect(NearVec(result.state.npcAgents[0].state.position, { 0.75F, 1.5F }), "runtime update should move visible NPC");
}

void TestEmptyNpcStateRemainsStable()
{
	iggy::LevelRuntimeBuildResult build = iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc());
	iggy::LevelRuntimeState state = StateFromBuild(build);
	state.npcAgents.clear();
	const iggy::LevelRuntimeUpdateResult result = iggy::LevelRuntimeUpdate {}.updateNpcAgents(state, { 4.5F, 1.5F }, Config());

	Expect(result.state.npcAgents.empty(), "empty runtime NPC state should remain empty");
	Expect(result.npcReports.empty(), "empty runtime NPC state should produce no reports");
	Expect(result.state.map.id == state.map.id, "empty runtime update should preserve map id");
}

void TestVisibleNpcMovesInReturnedState()
{
	const iggy::LevelRuntimeState state = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	const iggy::LevelRuntimeUpdateResult result = iggy::LevelRuntimeUpdate {}.updateNpcAgents(state, { 4.5F, 1.5F }, Config());

	Expect(HasEvent(result.npcReports[0].report, iggy::npc_ai::NpcTickEventType::PositionChanged), "visible NPC update should report position change");
	Expect(NearVec(result.state.npcAgents[0].state.position, { 0.75F, 1.5F }), "visible NPC update should move in returned state");
}

void TestInputStateRemainsUnchanged()
{
	const iggy::LevelRuntimeState state = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	const iggy::Vec2 originalPosition = state.npcAgents[0].state.position;
	const iggy::LevelRuntimeUpdateResult result = iggy::LevelRuntimeUpdate {}.updateNpcAgents(state, { 4.5F, 1.5F }, Config());

	Expect(state.npcAgents[0].state.position == originalPosition, "runtime update should not mutate input state");
	Expect(!NearVec(result.state.npcAgents[0].state.position, originalPosition), "runtime update should return changed state separately");
}

void TestMapPropertiesPreservedAfterUpdate()
{
	const iggy::LevelRuntimeState state = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	const iggy::LevelRuntimeUpdateResult result = iggy::LevelRuntimeUpdate {}.updateNpcAgents(state, { 4.5F, 1.5F }, Config());

	Expect(result.state.map.id == iggy::ResourceId { "level:cellar_01" }, "runtime update should preserve map id");
	Expect(result.state.map.width == state.map.width && result.state.map.height == state.map.height, "runtime update should preserve map dimensions");
	Expect(result.state.map.tiles.size() == state.map.tiles.size(), "runtime update should preserve map tiles");
	Expect(result.state.map.tileAt(0, 1) != nullptr && result.state.map.tileAt(0, 1)->walkable, "runtime update should preserve map walkability");
}

void TestReportsPreserveNpcIdAndOrder()
{
	iggy::LevelBlueprint blueprint = BlueprintWithNpc();
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "npc:villager" }, 0, 2, iggy::ResourceId { "spawn:villager_01" } });
	const iggy::LevelRuntimeState state = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(blueprint));
	const iggy::LevelRuntimeUpdateResult result = iggy::LevelRuntimeUpdate {}.updateNpcAgents(state, { 4.5F, 1.5F }, Config());

	Expect(result.npcReports.size() == 2, "runtime update should preserve report count");
	Expect(result.npcReports[0].id == iggy::ResourceId { "spawn:skeleton_01" }, "first report should preserve NPC id");
	Expect(result.npcReports[1].id == iggy::ResourceId { "spawn:villager_01" }, "second report should preserve NPC id");
	Expect(result.state.npcAgents[0].id == iggy::ResourceId { "spawn:skeleton_01" }, "first updated NPC should preserve id");
	Expect(result.state.npcAgents[1].id == iggy::ResourceId { "spawn:villager_01" }, "second updated NPC should preserve id");
}

} // namespace

int main()
{
	TestUpdateStateFromRuntimeBuilder();
	TestEmptyNpcStateRemainsStable();
	TestVisibleNpcMovesInReturnedState();
	TestInputStateRemainsUnchanged();
	TestMapPropertiesPreservedAfterUpdate();
	TestReportsPreserveNpcIdAndOrder();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
