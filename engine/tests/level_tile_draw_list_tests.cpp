#include <cstdlib>
#include <vector>

#include "scene/camera/CameraView.hpp"
#include "scene/level/LevelTileDrawList.hpp"
#include "scene/level/LevelVisibleTiles.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;
using iggy::test::SameTile;

iggy::LevelTileDrawListResult Build(const iggy::LevelTileMap &map, std::vector<iggy::TileCoord> visibleTiles)
{
	return iggy::LevelTileDrawList {}.build(map, visibleTiles);
}

bool SameBounds(iggy::Aabb2 actual, iggy::Aabb2 expected)
{
	return NearVec(actual.min, expected.min) && NearVec(actual.max, expected.max);
}

void ExpectItem(const iggy::LevelTileDrawItem &item, iggy::TileCoord tile, iggy::Aabb2 bounds, bool walkable, std::size_t index, const char *message)
{
	Expect(SameTile(item.tile, tile.x, tile.y) && SameBounds(item.worldBounds, bounds) && item.walkable == walkable && item.tileIndex == index, message);
}

void TestEmptyVisibleTilesReturnEmptyItems()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"...",
	});
	const iggy::LevelTileDrawListResult result = Build(map, {});

	Expect(result.items.empty(), "empty visible tile input should return empty draw items");
}

void TestOneValidTileProducesDrawItem()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"...",
		"...",
	});
	const iggy::LevelTileDrawListResult result = Build(map, { { 1, 1 } });

	Expect(result.items.size() == 1, "one valid visible tile should produce one draw item");
	if (!result.items.empty())
		ExpectItem(result.items[0], { 1, 1 }, { { 1.0F, 1.0F }, { 2.0F, 2.0F } }, true, 4, "draw item should preserve tile facts");
}

void TestMultipleVisibleTilesPreserveInputOrder()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"...",
		"...",
	});
	const iggy::LevelTileDrawListResult result = Build(map, { { 2, 1 }, { 0, 0 }, { 1, 1 } });

	Expect(result.items.size() == 3, "three valid visible tiles should produce three items");
	if (result.items.size() == 3) {
		Expect(SameTile(result.items[0].tile, 2, 1), "first draw item should preserve first input tile");
		Expect(SameTile(result.items[1].tile, 0, 0), "second draw item should preserve second input tile");
		Expect(SameTile(result.items[2].tile, 1, 1), "third draw item should preserve third input tile");
	}
}

void TestWalkableAndBlockedTilesBothProduceItems()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		".#.",
	});
	const iggy::LevelTileDrawListResult result = Build(map, { { 0, 0 }, { 1, 0 } });

	Expect(result.items.size() == 2, "walkable and blocked tiles should both produce draw items");
	if (result.items.size() == 2) {
		Expect(result.items[0].walkable, "walkable tile should copy walkable true");
		Expect(!result.items[1].walkable, "blocked tile should copy walkable false");
	}
}

void TestOutOfBoundsCoordinatesAreSkipped()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"...",
	});
	const iggy::LevelTileDrawListResult result = Build(map, { { -1, 0 }, { 1, 0 }, { 3, 0 } });

	Expect(result.items.size() == 1, "out-of-bounds visible tiles should be skipped");
	if (result.items.size() == 1)
		Expect(SameTile(result.items[0].tile, 1, 0), "valid tile should remain after invalid skips");
}

void TestMissingTileStorageIsSkipped()
{
	iggy::LevelTileMap map;
	map.width = 3;
	map.height = 1;
	map.tiles.push_back({ true });

	const iggy::LevelTileDrawListResult result = Build(map, { { 0, 0 }, { 1, 0 } });

	Expect(result.items.size() == 1, "coordinates with missing tile storage should be skipped");
	if (result.items.size() == 1)
		ExpectItem(result.items[0], { 0, 0 }, { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, true, 0, "present tile storage should still produce draw item");
}

void TestDuplicateVisibleTilesArePreserved()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"..",
	});
	const iggy::LevelTileDrawListResult result = Build(map, { { 1, 0 }, { 1, 0 } });

	Expect(result.items.size() == 2, "duplicate visible tiles should produce duplicate draw items");
	if (result.items.size() == 2) {
		Expect(SameTile(result.items[0].tile, 1, 0), "first duplicate should preserve tile");
		Expect(SameTile(result.items[1].tile, 1, 0), "second duplicate should preserve tile");
	}
}

void TestCameraViewVisibleTilesBuildDrawList()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"....",
		".#..",
		"....",
	});
	const iggy::CameraViewResult view = iggy::CameraView {}.visibleWorldBounds({ { 1.5F, 1.0F } }, { { 2.0F, 2.0F }, 1.0F });
	const iggy::LevelVisibleTilesResult visible = iggy::LevelVisibleTiles {}.query(map, view.bounds);
	const iggy::LevelTileDrawListResult drawList = iggy::LevelTileDrawList {}.build(map, visible.tiles);

	Expect(visible.hasTiles, "composition should produce visible tiles");
	Expect(drawList.items.size() == 6, "composition should produce one draw item per visible tile");
	if (drawList.items.size() == 6) {
		ExpectItem(drawList.items[0], { 0, 0 }, { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, true, 0, "composition first draw item should match first visible tile");
		ExpectItem(drawList.items[4], { 1, 1 }, { { 1.0F, 1.0F }, { 2.0F, 2.0F } }, false, 5, "composition blocked tile should preserve walkable false");
		ExpectItem(drawList.items[5], { 2, 1 }, { { 2.0F, 1.0F }, { 3.0F, 2.0F } }, true, 6, "composition last draw item should preserve row-major order");
	}
}

} // namespace

int main()
{
	TestEmptyVisibleTilesReturnEmptyItems();
	TestOneValidTileProducesDrawItem();
	TestMultipleVisibleTilesPreserveInputOrder();
	TestWalkableAndBlockedTilesBothProduceItems();
	TestOutOfBoundsCoordinatesAreSkipped();
	TestMissingTileStorageIsSkipped();
	TestDuplicateVisibleTilesArePreserved();
	TestCameraViewVisibleTilesBuildDrawList();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
