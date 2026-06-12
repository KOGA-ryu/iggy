#include <cstdlib>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/level/LevelTileRenderDirtyChunks.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

bool SameChunk(iggy::LevelTileRenderChunkCoord actual, int x, int y)
{
	return actual.x == x && actual.y == y;
}

bool ChunksEqual(const std::vector<iggy::LevelTileRenderChunkCoord> &actual, std::vector<iggy::LevelTileRenderChunkCoord> expected)
{
	if (actual.size() != expected.size())
		return false;

	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameChunk(actual[index], expected[index].x, expected[index].y))
			return false;
	}
	return true;
}

iggy::LevelTileRenderDirtyChunksResult Query(std::vector<iggy::TileCoord> changedTiles, int chunkWidth, int chunkHeight)
{
	return iggy::LevelTileRenderDirtyChunks {}.query(changedTiles, iggy::LevelTileRenderDirtyChunksConfig { chunkWidth, chunkHeight });
}

void TestInvalidChunkWidthFails()
{
	const iggy::LevelTileRenderDirtyChunksResult result = Query({ { 0, 0 } }, 0, 2);

	Expect(!result.queried, "invalid chunk width should not query");
	Expect(result.chunks.empty(), "invalid chunk width should return no chunks");
	Expect(result.issues.size() == 1, "invalid chunk width should report one issue");
	if (result.issues.size() == 1)
		Expect(result.issues[0].code == iggy::LevelTileRenderDirtyChunksIssueCode::InvalidChunkSize && result.issues[0].chunkWidth == 0 && result.issues[0].chunkHeight == 2, "invalid chunk width issue should preserve dimensions");
}

void TestInvalidChunkHeightFails()
{
	const iggy::LevelTileRenderDirtyChunksResult result = Query({ { 0, 0 } }, 2, -1);

	Expect(!result.queried, "invalid chunk height should not query");
	Expect(result.chunks.empty(), "invalid chunk height should return no chunks");
	Expect(result.issues.size() == 1, "invalid chunk height should report one issue");
	if (result.issues.size() == 1)
		Expect(result.issues[0].code == iggy::LevelTileRenderDirtyChunksIssueCode::InvalidChunkSize && result.issues[0].chunkWidth == 2 && result.issues[0].chunkHeight == -1, "invalid chunk height issue should preserve dimensions");
}

void TestEmptyChangedTilesStillQueries()
{
	const iggy::LevelTileRenderDirtyChunksResult result = Query({}, 2, 2);

	Expect(result.queried, "valid empty changed tile list should still be queried");
	Expect(result.chunks.empty(), "valid empty changed tile list should return no dirty chunks");
	Expect(result.issues.empty(), "valid empty changed tile list should return no issues");
}

void TestOneChangedTileMapsToChunk()
{
	const iggy::LevelTileRenderDirtyChunksResult result = Query({ { 3, 4 } }, 2, 2);

	Expect(result.queried, "one changed tile should query");
	Expect(ChunksEqual(result.chunks, { { 1, 2 } }), "one changed tile should map to expected chunk");
}

void TestMultipleTilesInSameChunkProduceOneChunk()
{
	const iggy::LevelTileRenderDirtyChunksResult result = Query({ { 0, 0 }, { 1, 0 }, { 1, 1 } }, 2, 2);

	Expect(ChunksEqual(result.chunks, { { 0, 0 } }), "multiple changed tiles in same chunk should deduplicate");
}

void TestMultipleChunksPreserveFirstSeenOrder()
{
	const iggy::LevelTileRenderDirtyChunksResult result = Query({ { 4, 0 }, { 0, 0 }, { 2, 0 }, { 4, 1 } }, 2, 2);

	Expect(ChunksEqual(result.chunks, { { 2, 0 }, { 0, 0 }, { 1, 0 } }), "dirty chunks should preserve first-seen order");
}

void TestDuplicateChangedTilesDoNotDuplicateChunks()
{
	const iggy::LevelTileRenderDirtyChunksResult result = Query({ { 2, 2 }, { 2, 2 }, { 3, 3 } }, 2, 2);

	Expect(ChunksEqual(result.chunks, { { 1, 1 } }), "duplicate changed tiles should not duplicate dirty chunks");
}

void TestBoundaryTileMapsToNextChunk()
{
	const iggy::LevelTileRenderDirtyChunksResult result = Query({ { 2, 0 } }, 2, 2);

	Expect(ChunksEqual(result.chunks, { { 1, 0 } }), "tile on positive chunk boundary should map to next chunk");
}

void TestNegativeXUsesFloorDivision()
{
	const iggy::LevelTileRenderDirtyChunksResult result = Query({ { -1, 0 }, { -2, 0 }, { -3, 0 } }, 2, 2);

	Expect(ChunksEqual(result.chunks, { { -1, 0 }, { -2, 0 } }), "negative x tiles should use floor division and preserve first-seen dirty chunks");
}

void TestNegativeYUsesFloorDivision()
{
	const iggy::LevelTileRenderDirtyChunksResult result = Query({ { 0, -1 }, { 0, -2 }, { 0, -3 } }, 2, 2);

	Expect(ChunksEqual(result.chunks, { { 0, -1 }, { 0, -2 } }), "negative y tiles should use floor division and preserve first-seen dirty chunks");
}

void TestNonSquareChunksMapAxesIndependently()
{
	const iggy::LevelTileRenderDirtyChunksResult result = Query({ { 2, 1 }, { 3, 2 }, { 5, 3 } }, 3, 2);

	Expect(ChunksEqual(result.chunks, { { 0, 0 }, { 1, 1 } }), "non-square chunks should map x and y independently");
}

void TestCacheConfigOverloadMatchesDirtyConfigAndIgnoresTileCommands()
{
	const std::vector<iggy::TileCoord> changedTiles { { 0, 0 }, { 3, 2 } };
	const iggy::LevelTileRenderDirtyChunksResult direct = iggy::LevelTileRenderDirtyChunks {}.query(changedTiles, iggy::LevelTileRenderDirtyChunksConfig { 2, 2 });
	const iggy::LevelTileRenderChunkCacheConfig cacheConfig {
		2,
		2,
		{ { iggy::ResourceId { "material:unused_walkable" }, iggy::ResourceId { "material:unused_blocked" } }, 99 },
	};

	const iggy::LevelTileRenderDirtyChunksResult viaCacheConfig = iggy::LevelTileRenderDirtyChunks {}.query(changedTiles, cacheConfig);

	Expect(viaCacheConfig.queried, "cache config overload should query with valid dimensions");
	Expect(ChunksEqual(viaCacheConfig.chunks, direct.chunks), "cache config overload should match query-specific config result");
}

void TestInputChangedTilesAreNotMutated()
{
	const std::vector<iggy::TileCoord> changedTiles { { -1, 0 }, { 2, 3 } };
	const std::vector<iggy::TileCoord> original = changedTiles;

	const iggy::LevelTileRenderDirtyChunksResult result = iggy::LevelTileRenderDirtyChunks {}.query(changedTiles, iggy::LevelTileRenderDirtyChunksConfig { 2, 2 });

	Expect(result.queried, "immutability test should query");
	Expect(changedTiles == original, "dirty chunk query should not mutate changed tile input");
}

} // namespace

int main()
{
	TestInvalidChunkWidthFails();
	TestInvalidChunkHeightFails();
	TestEmptyChangedTilesStillQueries();
	TestOneChangedTileMapsToChunk();
	TestMultipleTilesInSameChunkProduceOneChunk();
	TestMultipleChunksPreserveFirstSeenOrder();
	TestDuplicateChangedTilesDoNotDuplicateChunks();
	TestBoundaryTileMapsToNextChunk();
	TestNegativeXUsesFloorDivision();
	TestNegativeYUsesFloorDivision();
	TestNonSquareChunksMapAxesIndependently();
	TestCacheConfigOverloadMatchesDirtyConfigAndIgnoresTileCommands();
	TestInputChangedTilesAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
