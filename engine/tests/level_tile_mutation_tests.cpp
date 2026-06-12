#include <cstdlib>
#include <vector>

#include "scene/level/LevelTileMutation.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::MapFromRows;

iggy::LevelTileEdit Edit(iggy::TileCoord tile, bool walkable)
{
	return { tile, walkable };
}

void ExpectTileWalkable(const iggy::LevelTileMap &map, iggy::TileCoord tile, bool walkable, const char *message)
{
	const iggy::LevelTile *tileData = map.tileAt(tile.x, tile.y);
	Expect(tileData != nullptr, message);
	if (tileData != nullptr)
		Expect(tileData->walkable == walkable, message);
}

void ExpectChangedTiles(
	const std::vector<iggy::TileCoord> &actual,
	const std::vector<iggy::TileCoord> &expected,
	const char *message)
{
	Expect(actual.size() == expected.size(), message);
	for (std::size_t index = 0; index < actual.size() && index < expected.size(); ++index)
		Expect(actual[index] == expected[index], message);
}

void ExpectMapSame(const iggy::LevelTileMap &actual, const iggy::LevelTileMap &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.width == expected.width && actual.height == expected.height, message);
	Expect(actual.tiles.size() == expected.tiles.size(), message);
	for (std::size_t index = 0; index < actual.tiles.size() && index < expected.tiles.size(); ++index)
		Expect(actual.tiles[index].walkable == expected.tiles[index].walkable, message);
	Expect(actual.entitySpawns.size() == expected.entitySpawns.size(), message);
	Expect(actual.playerStart.x == expected.playerStart.x && actual.playerStart.y == expected.playerStart.y, message);
}

void ExpectIssue(
	const iggy::LevelTileEditIssue &issue,
	iggy::LevelTileEditIssueCode code,
	std::size_t editIndex,
	iggy::LevelTileEdit edit,
	const char *message)
{
	Expect(issue.code == code, message);
	Expect(issue.editIndex == editIndex, message);
	Expect(issue.edit.tile == edit.tile, message);
	Expect(issue.edit.walkable == edit.walkable, message);
}

void TestEmptyEditsReturnUnchangedMap()
{
	const iggy::LevelTileMap map = MapFromRows({
		".#",
	});

	const iggy::LevelTileMutationResult result = iggy::LevelTileMutation {}.apply(map, {});

	Expect(!result.mutated, "empty edits should not mark map mutated");
	ExpectMapSame(result.map, map, "empty edits should copy input map");
	Expect(result.changedTiles.empty(), "empty edits should report no changed tiles");
	Expect(result.issues.empty(), "empty edits should report no issues");
}

void TestSingleInBoundsEditChangesWalkability()
{
	const iggy::LevelTileMap map = MapFromRows({
		"..",
	});

	const iggy::LevelTileMutationResult result = iggy::LevelTileMutation {}.apply(map, { Edit({ 1, 0 }, false) });

	Expect(result.mutated, "in-bounds edit should mark mutation when value changes");
	ExpectTileWalkable(result.map, { 1, 0 }, false, "in-bounds edit should update copied tile walkability");
	ExpectChangedTiles(result.changedTiles, { { 1, 0 } }, "in-bounds edit should report changed tile");
	Expect(result.issues.empty(), "in-bounds edit should not report issues");
}

void TestSameValueEditIsAcceptedNoOp()
{
	const iggy::LevelTileMap map = MapFromRows({
		".#",
	});

	const iggy::LevelTileMutationResult result = iggy::LevelTileMutation {}.apply(map, { Edit({ 1, 0 }, false) });

	Expect(!result.mutated, "same-value edit should not mark mutation");
	ExpectMapSame(result.map, map, "same-value edit should leave copied map unchanged");
	Expect(result.changedTiles.empty(), "same-value edit should not report changed tile");
	Expect(result.issues.empty(), "same-value edit should not report issue");
}

void TestMultipleEditsApplyInOrder()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
	});
	const std::vector<iggy::LevelTileEdit> edits {
		Edit({ 2, 0 }, false),
		Edit({ 0, 0 }, false),
	};

	const iggy::LevelTileMutationResult result = iggy::LevelTileMutation {}.apply(map, edits);

	Expect(result.mutated, "multiple edits should mark mutation");
	ExpectTileWalkable(result.map, { 2, 0 }, false, "first multiple edit should apply");
	ExpectTileWalkable(result.map, { 0, 0 }, false, "second multiple edit should apply");
	ExpectChangedTiles(result.changedTiles, { { 2, 0 }, { 0, 0 } }, "multiple edits should preserve changed tile order");
}

void TestDuplicateChangingEditsReportDuplicateChangedTiles()
{
	const iggy::LevelTileMap map = MapFromRows({
		".",
	});
	const std::vector<iggy::LevelTileEdit> edits {
		Edit({ 0, 0 }, false),
		Edit({ 0, 0 }, true),
		Edit({ 0, 0 }, true),
		Edit({ 0, 0 }, false),
	};

	const iggy::LevelTileMutationResult result = iggy::LevelTileMutation {}.apply(map, edits);

	Expect(result.mutated, "duplicate changing edits should mark mutation");
	ExpectTileWalkable(result.map, { 0, 0 }, false, "duplicate changing edits should leave final value from last changing edit");
	ExpectChangedTiles(result.changedTiles, { { 0, 0 }, { 0, 0 }, { 0, 0 } }, "duplicate changing edits should preserve accepted-change event order");
	Expect(result.issues.empty(), "duplicate changing edits should not report issues");
}

void TestOutOfBoundsEditReportsIssue()
{
	const iggy::LevelTileMap map = MapFromRows({
		".",
	});

	const iggy::LevelTileMutationResult result = iggy::LevelTileMutation {}.apply(map, { Edit({ 1, 0 }, false) });

	Expect(!result.mutated, "out-of-bounds edit should not mutate map");
	ExpectMapSame(result.map, map, "out-of-bounds edit should leave copied map unchanged");
	Expect(result.changedTiles.empty(), "out-of-bounds edit should not report changed tile");
	Expect(result.issues.size() == 1, "out-of-bounds edit should report one issue");
	if (result.issues.size() == 1)
		ExpectIssue(result.issues[0], iggy::LevelTileEditIssueCode::OutOfBounds, 0, Edit({ 1, 0 }, false), "out-of-bounds issue should preserve edit diagnostics");
}

void TestMissingSparseTileStorageReportsIssue()
{
	iggy::LevelTileMap map;
	map.width = 2;
	map.height = 1;
	map.tiles.push_back({ true });

	const iggy::LevelTileMutationResult result = iggy::LevelTileMutation {}.apply(map, { Edit({ 1, 0 }, false) });

	Expect(!result.mutated, "missing sparse tile storage should not mutate map");
	ExpectMapSame(result.map, map, "missing sparse tile storage should leave copied map unchanged");
	Expect(result.changedTiles.empty(), "missing sparse tile storage should not report changed tile");
	Expect(result.issues.size() == 1, "missing sparse tile storage should report one issue");
	if (result.issues.size() == 1)
		ExpectIssue(result.issues[0], iggy::LevelTileEditIssueCode::MissingTileStorage, 0, Edit({ 1, 0 }, false), "missing sparse tile issue should preserve edit diagnostics");
}

void TestMixedValidAndInvalidEditsApplyValidEdits()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
	});
	const std::vector<iggy::LevelTileEdit> edits {
		Edit({ 0, 0 }, false),
		Edit({ 5, 0 }, false),
		Edit({ 2, 0 }, false),
	};

	const iggy::LevelTileMutationResult result = iggy::LevelTileMutation {}.apply(map, edits);

	Expect(result.mutated, "mixed edits should mutate when valid edits change values");
	ExpectTileWalkable(result.map, { 0, 0 }, false, "mixed edits should apply first valid edit");
	ExpectTileWalkable(result.map, { 2, 0 }, false, "mixed edits should apply later valid edit after invalid issue");
	ExpectChangedTiles(result.changedTiles, { { 0, 0 }, { 2, 0 } }, "mixed edits should report accepted changed tiles only");
	Expect(result.issues.size() == 1, "mixed edits should report invalid issue without failing valid edits");
	if (result.issues.size() == 1)
		ExpectIssue(result.issues[0], iggy::LevelTileEditIssueCode::OutOfBounds, 1, Edit({ 5, 0 }, false), "mixed edit issue should preserve original edit index");
}

void TestInputMapAndEditsAreNotMutated()
{
	iggy::LevelTileMap map = MapFromRows({
		".#",
	});
	map.id = iggy::ResourceId("level:mutation");
	map.playerStart = { 1, 0 };
	map.entitySpawns.push_back({ iggy::ResourceId("npc:type"), 1, 0, iggy::ResourceId("spawn:one") });
	const iggy::LevelTileMap before = map;
	std::vector<iggy::LevelTileEdit> edits {
		Edit({ 0, 0 }, false),
		Edit({ 1, 0 }, true),
	};
	const std::vector<iggy::LevelTileEdit> editsBefore = edits;

	const iggy::LevelTileMutationResult result = iggy::LevelTileMutation {}.apply(map, edits);

	Expect(result.mutated, "immutability setup should mutate copied result");
	ExpectMapSame(map, before, "tile mutation should not mutate input map");
	Expect(edits.size() == editsBefore.size(), "tile mutation should not mutate edit vector size");
	for (std::size_t index = 0; index < edits.size() && index < editsBefore.size(); ++index) {
		Expect(edits[index].tile == editsBefore[index].tile, "tile mutation should not mutate edit tile");
		Expect(edits[index].walkable == editsBefore[index].walkable, "tile mutation should not mutate edit walkability");
	}
	Expect(result.map.id == before.id, "tile mutation should preserve map id in copied result");
	Expect(result.map.width == before.width && result.map.height == before.height, "tile mutation should preserve map dimensions in copied result");
	Expect(result.map.entitySpawns.size() == before.entitySpawns.size(), "tile mutation should preserve unrelated entity spawns in copied result");
}

} // namespace

int main()
{
	TestEmptyEditsReturnUnchangedMap();
	TestSingleInBoundsEditChangesWalkability();
	TestSameValueEditIsAcceptedNoOp();
	TestMultipleEditsApplyInOrder();
	TestDuplicateChangingEditsReportDuplicateChangedTiles();
	TestOutOfBoundsEditReportsIssue();
	TestMissingSparseTileStorageReportsIssue();
	TestMixedValidAndInvalidEditsApplyValidEdits();
	TestInputMapAndEditsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
