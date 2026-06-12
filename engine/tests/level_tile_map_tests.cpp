#include <cstdlib>
#include <iostream>
#include <string_view>

#include "scene/level/LevelTileMap.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

bool HasIssue(const iggy::LevelBlueprintValidationReport &report, iggy::LevelBlueprintIssueCode code)
{
	for (const iggy::LevelBlueprintIssue &issue : report.issues) {
		if (issue.code == code)
			return true;
	}
	return false;
}

iggy::LevelBlueprint ValidBlueprint()
{
	iggy::LevelBlueprint blueprint;
	blueprint.id = iggy::ResourceId { "level:cellar_01" };
	blueprint.bounds = { 3, 2 };
	blueprint.tiles = {
		{ true },
		{ false },
		{ true },
		{ true },
		{ true },
		{ false },
	};
	blueprint.playerStarts.push_back({ 1, 1 });
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "enemy:skeleton" }, 2, 1, iggy::ResourceId { "spawn:skeleton_01" } });
	return blueprint;
}

void TestBuildsRuntimeTileMap()
{
	const iggy::LevelTileMapBuildResult result = iggy::LevelTileMapBuilder {}.build(ValidBlueprint());

	Expect(result.built, "valid blueprint should build tile map");
	Expect(result.validation.valid, "valid blueprint build should keep validation report");
	Expect(result.tileMap.id == iggy::ResourceId { "level:cellar_01" }, "tile map should keep level id");
	Expect(result.tileMap.width == 3 && result.tileMap.height == 2, "tile map should keep dimensions");
	Expect(result.tileMap.tiles.size() == 6, "tile map should contain one tile per cell");
	Expect(result.tileMap.tileAt(1, 0) != nullptr && !result.tileMap.tileAt(1, 0)->walkable, "tile map should preserve tile walkability");
	Expect(result.tileMap.tileAt(3, 0) == nullptr, "tile lookup should reject out-of-bounds cell");
	Expect(result.tileMap.playerStart.x == 1 && result.tileMap.playerStart.y == 1, "tile map should preserve player start");
	Expect(result.tileMap.entitySpawns.size() == 1, "tile map should preserve entity spawns");
	Expect(result.tileMap.entitySpawns[0].id == iggy::ResourceId { "spawn:skeleton_01" }, "tile map should preserve entity ids");
}

void TestBuildsDefaultWalkableTiles()
{
	iggy::LevelBlueprint blueprint = ValidBlueprint();
	blueprint.tiles.clear();
	const iggy::LevelTileMapBuildResult result = iggy::LevelTileMapBuilder {}.build(blueprint);

	Expect(result.built, "blueprint without explicit tiles should build");
	Expect(result.tileMap.tiles.size() == 6, "default tile map should fill bounds");
	Expect(result.tileMap.tileAt(2, 1) != nullptr && result.tileMap.tileAt(2, 1)->walkable, "default tiles should be walkable");
}

void TestRejectsInvalidBlueprint()
{
	iggy::LevelBlueprint blueprint = ValidBlueprint();
	blueprint.playerStarts.clear();
	const iggy::LevelTileMapBuildResult result = iggy::LevelTileMapBuilder {}.build(blueprint);

	Expect(!result.built, "invalid blueprint should not build tile map");
	Expect(!result.validation.valid, "invalid build should return validation failure");
	Expect(HasIssue(result.validation, iggy::LevelBlueprintIssueCode::MissingPlayerStart), "invalid build should report validation issue");
	Expect(result.tileMap.tiles.empty(), "invalid build should not create runtime tiles");
}

} // namespace

int main()
{
	TestBuildsRuntimeTileMap();
	TestBuildsDefaultWalkableTiles();
	TestRejectsInvalidBlueprint();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
