#include <cstdlib>
#include <iostream>
#include <string_view>

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

bool HasValidationIssue(const iggy::LevelBlueprintValidationReport &report, iggy::LevelBlueprintIssueCode code)
{
	for (const iggy::LevelBlueprintIssue &issue : report.issues) {
		if (issue.code == code)
			return true;
	}
	return false;
}

bool HasSpawnIssue(const iggy::LevelRuntimeBuildResult &result, iggy::npc_ai::NpcSpawnBuildIssueCode code)
{
	for (const iggy::npc_ai::NpcSpawnBuildIssue &issue : result.spawnIssues) {
		if (issue.code == code)
			return true;
	}
	return false;
}

iggy::LevelBlueprint ValidBlueprint()
{
	iggy::LevelBlueprint blueprint;
	blueprint.id = iggy::ResourceId { "level:cellar_01" };
	blueprint.bounds = { 4, 3 };
	blueprint.tiles = {
		{ true },
		{ true },
		{ false },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ false },
		{ true },
		{ true },
		{ true },
	};
	blueprint.playerStarts.push_back({ 0, 0 });
	return blueprint;
}

void TestValidBlueprintBuildsTileMapAndNpcAgents()
{
	iggy::LevelBlueprint blueprint = ValidBlueprint();
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "enemy:skeleton" }, 2, 1, iggy::ResourceId { "spawn:skeleton_01" } });
	const iggy::LevelRuntimeBuildResult result = iggy::LevelRuntimeBuilder {}.build(blueprint);

	Expect(result.built, "valid runtime blueprint should build");
	Expect(result.validation.valid, "valid runtime blueprint should preserve valid validation report");
	Expect(result.tileMap.id == iggy::ResourceId { "level:cellar_01" }, "runtime tile map should preserve level id");
	Expect(result.npcAgents.size() == 1, "runtime build should include NPC agents");
	Expect(result.npcAgents[0].id == iggy::ResourceId { "spawn:skeleton_01" }, "runtime NPC agent should preserve spawn id");
	Expect(result.npcAgents[0].state.position == iggy::Vec2 { 2.5F, 1.5F }, "runtime NPC agent should use spawn tile center");
}

void TestInvalidBlueprintFailsWithValidationIssues()
{
	iggy::LevelBlueprint blueprint = ValidBlueprint();
	blueprint.playerStarts.clear();
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "enemy:skeleton" }, 2, 1, iggy::ResourceId { "spawn:skeleton_01" } });
	const iggy::LevelRuntimeBuildResult result = iggy::LevelRuntimeBuilder {}.build(blueprint);

	Expect(!result.built, "invalid runtime blueprint should not build");
	Expect(!result.validation.valid, "invalid runtime blueprint should preserve invalid validation report");
	Expect(HasValidationIssue(result.validation, iggy::LevelBlueprintIssueCode::MissingPlayerStart), "invalid runtime blueprint should report validation issue");
	Expect(result.tileMap.tiles.empty(), "invalid runtime blueprint should not produce tile map");
	Expect(result.npcAgents.empty(), "invalid runtime blueprint should not produce NPC agents");
}

void TestNonNpcSpawnSkippedWhileBuildSucceeds()
{
	iggy::LevelBlueprint blueprint = ValidBlueprint();
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "item:potion" }, 1, 1, iggy::ResourceId { "spawn:potion_01" } });
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "npc:villager" }, 3, 2, iggy::ResourceId { "spawn:villager_01" } });
	const iggy::LevelRuntimeBuildResult result = iggy::LevelRuntimeBuilder {}.build(blueprint);

	Expect(result.built, "valid runtime blueprint with non-NPC spawn should still build");
	Expect(result.npcAgents.size() == 1, "runtime build should skip non-NPC spawns");
	Expect(result.npcAgents[0].id == iggy::ResourceId { "spawn:villager_01" }, "runtime build should preserve remaining NPC spawn");
	Expect(HasSpawnIssue(result, iggy::npc_ai::NpcSpawnBuildIssueCode::NonNpcSpawnSkipped), "runtime build should surface skipped non-NPC issue");
}

void TestNoNpcSpawnsSucceedsWithEmptyBatch()
{
	const iggy::LevelBlueprint blueprint = ValidBlueprint();
	const iggy::LevelRuntimeBuildResult result = iggy::LevelRuntimeBuilder {}.build(blueprint);

	Expect(result.built, "valid runtime blueprint without NPCs should build");
	Expect(result.npcAgents.empty(), "valid runtime blueprint without NPCs should have empty NPC batch");
	Expect(result.spawnIssues.empty(), "valid runtime blueprint without spawns should have no spawn issues");
}

void TestTileMapOutputMatchesBlueprint()
{
	const iggy::LevelBlueprint blueprint = ValidBlueprint();
	const iggy::LevelRuntimeBuildResult result = iggy::LevelRuntimeBuilder {}.build(blueprint);

	Expect(result.tileMap.width == 4 && result.tileMap.height == 3, "runtime tile map should preserve dimensions");
	Expect(result.tileMap.tiles.size() == 12, "runtime tile map should preserve tile count");
	Expect(result.tileMap.tileAt(2, 0) != nullptr && !result.tileMap.tileAt(2, 0)->walkable, "runtime tile map should preserve blocked tile");
	Expect(result.tileMap.tileAt(1, 1) != nullptr && result.tileMap.tileAt(1, 1)->walkable, "runtime tile map should preserve walkable tile");
}

} // namespace

int main()
{
	TestValidBlueprintBuildsTileMapAndNpcAgents();
	TestInvalidBlueprintFailsWithValidationIssues();
	TestNonNpcSpawnSkippedWhileBuildSucceeds();
	TestNoNpcSpawnsSucceedsWithEmptyBatch();
	TestTileMapOutputMatchesBlueprint();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
