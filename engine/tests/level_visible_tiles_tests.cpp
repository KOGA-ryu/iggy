#include <cstdlib>
#include <vector>

#include "scene/camera/CameraView.hpp"
#include "scene/level/LevelVisibleTiles.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::SameTile;

iggy::LevelVisibleTilesResult Query(const iggy::LevelTileMap &map, iggy::Vec2 min, iggy::Vec2 max)
{
	return iggy::LevelVisibleTiles {}.query(map, { min, max });
}

void ExpectNoTiles(const iggy::LevelVisibleTilesResult &result, const char *message)
{
	Expect(!result.hasTiles && result.tiles.empty(), message);
}

void ExpectRange(const iggy::LevelVisibleTilesResult &result, int minX, int minY, int maxX, int maxY, const char *message)
{
	Expect(result.hasTiles && SameTile(result.minTile, minX, minY) && SameTile(result.maxTile, maxX, maxY), message);
}

void ExpectTiles(const std::vector<iggy::TileCoord> &actual, std::vector<iggy::TileCoord> expected, const char *message)
{
	if (actual.size() != expected.size()) {
		Expect(false, message);
		return;
	}

	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index] != expected[index]) {
			Expect(false, message);
			return;
		}
	}
	Expect(true, message);
}

void TestEmptyOrInvalidMapReturnsNoTiles()
{
	iggy::LevelTileMap empty;
	empty.width = 3;
	empty.height = 3;
	ExpectNoTiles(Query(empty, { 0.0F, 0.0F }, { 1.0F, 1.0F }), "map with no tiles should return no visible tiles");

	iggy::LevelTileMap invalid = iggy::test::MapFromRows({ "..." });
	invalid.width = 0;
	ExpectNoTiles(Query(invalid, { 0.0F, 0.0F }, { 1.0F, 1.0F }), "nonpositive map dimensions should return no visible tiles");
}

void TestBoundsEntirelyOutsideMapReturnsNoTiles()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"...",
		"...",
	});

	ExpectNoTiles(Query(map, { 4.0F, 0.0F }, { 5.0F, 1.0F }), "bounds to the right of map should return no visible tiles");
	ExpectNoTiles(Query(map, { -2.0F, 0.0F }, { -1.0F, 1.0F }), "bounds to the left of map should return no visible tiles");
}

void TestBoundsInsideOneTile()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"...",
		"...",
	});
	const iggy::LevelVisibleTilesResult result = Query(map, { 1.2F, 0.2F }, { 1.8F, 0.8F });

	ExpectRange(result, 1, 0, 1, 0, "single-tile bounds should report one tile range");
	ExpectTiles(result.tiles, { { 1, 0 } }, "single-tile bounds should return that tile");
}

void TestZeroSizePointBoundsInsideMap()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"...",
		"...",
	});
	const iggy::LevelVisibleTilesResult result = Query(map, { 2.0F, 1.0F }, { 2.0F, 1.0F });

	ExpectRange(result, 2, 1, 2, 1, "point bounds should report containing tile range");
	ExpectTiles(result.tiles, { { 2, 1 } }, "point bounds should return containing tile");
}

void TestMultipleTilesRowMajor()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"....",
		"....",
		"....",
	});
	const iggy::LevelVisibleTilesResult result = Query(map, { 1.2F, 0.2F }, { 3.8F, 2.2F });

	ExpectRange(result, 1, 0, 3, 2, "multi-tile bounds should report clamped min/max range");
	ExpectTiles(result.tiles, { { 1, 0 }, { 2, 0 }, { 3, 0 }, { 1, 1 }, { 2, 1 }, { 3, 1 }, { 1, 2 }, { 2, 2 }, { 3, 2 } }, "multi-tile bounds should return row-major tiles");
}

void TestExactMaxEdgeUsesHalfOpenSemantics()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"...",
		"...",
	});
	const iggy::LevelVisibleTilesResult result = Query(map, { 0.0F, 0.0F }, { 2.0F, 1.0F });

	ExpectRange(result, 0, 0, 1, 0, "exact max edge should exclude max-edge tile");
	ExpectTiles(result.tiles, { { 0, 0 }, { 1, 0 } }, "exact max edge should use half-open bounds");
}

void TestPartlyOutsideMapClampsToValidTiles()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"...",
		"...",
		"...",
	});
	const iggy::LevelVisibleTilesResult result = Query(map, { -1.5F, 1.2F }, { 2.4F, 4.0F });

	ExpectRange(result, 0, 1, 2, 2, "partly outside bounds should clamp range to map");
	ExpectTiles(result.tiles, { { 0, 1 }, { 1, 1 }, { 2, 1 }, { 0, 2 }, { 1, 2 }, { 2, 2 } }, "partly outside bounds should return clamped row-major tiles");
}

void TestInvertedBoundsNormalize()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"....",
		"....",
		"....",
	});
	const iggy::LevelVisibleTilesResult ordered = Query(map, { 1.2F, 0.2F }, { 3.0F, 2.0F });
	const iggy::LevelVisibleTilesResult inverted = Query(map, { 3.0F, 2.0F }, { 1.2F, 0.2F });

	ExpectRange(inverted, 1, 0, 2, 1, "inverted bounds should normalize to ordered range");
	ExpectTiles(inverted.tiles, ordered.tiles, "inverted bounds should match ordered tile list");
}

void TestNegativeWorldCoordinatesClampOrReject()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"...",
		"...",
	});
	const iggy::LevelVisibleTilesResult overlapping = Query(map, { -0.25F, -0.25F }, { 1.25F, 1.25F });
	const iggy::LevelVisibleTilesResult outside = Query(map, { -2.0F, -2.0F }, { -1.0F, -1.0F });

	ExpectRange(overlapping, 0, 0, 1, 1, "negative overlapping bounds should clamp to map");
	ExpectTiles(overlapping.tiles, { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } }, "negative overlapping bounds should return clamped tiles");
	ExpectNoTiles(outside, "negative bounds outside map should return no tiles");
}

void TestCameraViewBoundsFeedVisibleTiles()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"....",
		"....",
		"....",
	});
	const iggy::CameraViewResult view = iggy::CameraView {}.visibleWorldBounds({ { 1.5F, 1.0F } }, { { 2.0F, 2.0F }, 1.0F });
	const iggy::LevelVisibleTilesResult result = iggy::LevelVisibleTiles {}.query(map, view.bounds);

	ExpectRange(result, 0, 0, 2, 1, "camera view bounds should feed visible tile query");
	ExpectTiles(result.tiles, { { 0, 0 }, { 1, 0 }, { 2, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 } }, "camera view composition should return visible map tiles");
}

} // namespace

int main()
{
	TestEmptyOrInvalidMapReturnsNoTiles();
	TestBoundsEntirelyOutsideMapReturnsNoTiles();
	TestBoundsInsideOneTile();
	TestZeroSizePointBoundsInsideMap();
	TestMultipleTilesRowMajor();
	TestExactMaxEdgeUsesHalfOpenSemantics();
	TestPartlyOutsideMapClampsToValidTiles();
	TestInvertedBoundsNormalize();
	TestNegativeWorldCoordinatesClampOrReject();
	TestCameraViewBoundsFeedVisibleTiles();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
