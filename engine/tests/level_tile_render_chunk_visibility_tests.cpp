#include <cstdlib>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/camera/CameraView.hpp"
#include "scene/level/LevelTileRenderChunkCache.hpp"
#include "scene/level/LevelTileRenderChunkVisibility.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };

iggy::LevelTileRenderChunkCacheConfig Config(int chunkWidth = 2, int chunkHeight = 2)
{
	return { chunkWidth, chunkHeight, { { WalkableMaterial, BlockedMaterial }, 3 } };
}

iggy::LevelTileRenderChunkCache CacheFromRows(std::vector<std::string_view> rows, int chunkWidth = 2, int chunkHeight = 2)
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows(rows);
	const iggy::LevelTileRenderChunkCacheBuildResult build = iggy::LevelTileRenderChunkCacheBuilder {}.build(map, Config(chunkWidth, chunkHeight));
	return build.cache;
}

iggy::LevelTileRenderChunkVisibilityResult Query(const iggy::LevelTileRenderChunkCache &cache, iggy::Aabb2 bounds)
{
	return iggy::LevelTileRenderChunkVisibility {}.query(cache, bounds);
}

bool IndexesEqual(const std::vector<std::size_t> &actual, std::vector<std::size_t> expected)
{
	return actual == expected;
}

void TestEmptyCacheReturnsNoChunks()
{
	const iggy::LevelTileRenderChunkCache cache;

	const iggy::LevelTileRenderChunkVisibilityResult result = Query(cache, { { 0.0F, 0.0F }, { 1.0F, 1.0F } });

	Expect(!result.hasChunks, "empty cache should not report chunks");
	Expect(result.chunkIndexes.empty(), "empty cache should return empty indexes");
}

void TestOutsideBoundsReturnNoChunks()
{
	const iggy::LevelTileRenderChunkCache cache = CacheFromRows({
		"....",
		"....",
		"....",
		"....",
	});

	const iggy::LevelTileRenderChunkVisibilityResult result = Query(cache, { { 5.0F, 5.0F }, { 6.0F, 6.0F } });

	Expect(!result.hasChunks, "bounds outside all chunks should not report chunks");
	Expect(result.chunkIndexes.empty(), "bounds outside all chunks should return empty indexes");
}

void TestBoundsInsideOneChunkReturnThatIndex()
{
	const iggy::LevelTileRenderChunkCache cache = CacheFromRows({
		"....",
		"....",
		"....",
		"....",
	});

	const iggy::LevelTileRenderChunkVisibilityResult result = Query(cache, { { 0.25F, 0.25F }, { 1.0F, 1.0F } });

	Expect(result.hasChunks, "bounds inside one chunk should report chunks");
	Expect(IndexesEqual(result.chunkIndexes, { 0 }), "bounds inside first chunk should return index 0");
}

void TestBoundsOverlappingMultipleChunksReturnCacheOrder()
{
	const iggy::LevelTileRenderChunkCache cache = CacheFromRows({
		"....",
		"....",
		"....",
		"....",
	});

	const iggy::LevelTileRenderChunkVisibilityResult result = Query(cache, { { 1.5F, 1.5F }, { 3.5F, 3.5F } });

	Expect(result.hasChunks, "bounds crossing chunk seams should report chunks");
	Expect(IndexesEqual(result.chunkIndexes, { 0, 1, 2, 3 }), "overlapping multiple chunks should return indexes in cache order");
}

void TestExactMaxEdgeUsesHalfOpenSemantics()
{
	const iggy::LevelTileRenderChunkCache cache = CacheFromRows({
		"....",
		"....",
		"....",
		"....",
	});

	const iggy::LevelTileRenderChunkVisibilityResult result = Query(cache, { { 0.0F, 0.0F }, { 2.0F, 2.0F } });

	Expect(IndexesEqual(result.chunkIndexes, { 0 }), "half-open query ending at chunk max should not select adjacent chunks");
}

void TestBoundsStartingAtChunkMaxEdgeSkipPreviousChunk()
{
	const iggy::LevelTileRenderChunkCache cache = CacheFromRows({
		"....",
		"....",
		"....",
		"....",
	});

	const iggy::LevelTileRenderChunkVisibilityResult result = Query(cache, { { 2.0F, 0.25F }, { 3.0F, 1.0F } });

	Expect(IndexesEqual(result.chunkIndexes, { 1 }), "query starting at first chunk max x should select only the adjacent chunk");
}

void TestInvertedBoundsNormalize()
{
	const iggy::LevelTileRenderChunkCache cache = CacheFromRows({
		"....",
		"....",
		"....",
		"....",
	});

	const iggy::LevelTileRenderChunkVisibilityResult ordered = Query(cache, { { 2.25F, 0.25F }, { 3.25F, 1.25F } });
	const iggy::LevelTileRenderChunkVisibilityResult inverted = Query(cache, { { 3.25F, 1.25F }, { 2.25F, 0.25F } });

	Expect(IndexesEqual(ordered.chunkIndexes, { 1 }), "ordered bounds setup should select right chunk");
	Expect(IndexesEqual(inverted.chunkIndexes, ordered.chunkIndexes), "inverted bounds should normalize to the same result as ordered bounds");
}

void TestDegeneratePointInsideChunk()
{
	const iggy::LevelTileRenderChunkCache cache = CacheFromRows({
		"....",
		"....",
		"....",
		"....",
	});

	const iggy::LevelTileRenderChunkVisibilityResult result = Query(cache, { { 1.0F, 1.0F }, { 1.0F, 1.0F } });

	Expect(IndexesEqual(result.chunkIndexes, { 0 }), "point bounds inside a chunk should select that chunk");
}

void TestDegeneratePointOnSharedBoundarySelectsContainingChunks()
{
	const iggy::LevelTileRenderChunkCache cache = CacheFromRows({
		"....",
		"....",
		"....",
		"....",
	});

	const iggy::LevelTileRenderChunkVisibilityResult result = Query(cache, { { 2.0F, 2.0F }, { 2.0F, 2.0F } });

	Expect(IndexesEqual(result.chunkIndexes, { 0, 1, 2, 3 }), "point on shared chunk boundary should select containing chunks in cache order");
}

void TestCameraViewCompositionSelectsExpectedChunks()
{
	const iggy::LevelTileRenderChunkCache cache = CacheFromRows({
		"....",
		"....",
		"....",
		"....",
	});
	const iggy::CameraViewResult view = iggy::CameraView {}.visibleWorldBounds({ { 1.0F, 1.0F } }, { { 2.0F, 2.0F }, 1.0F });

	const iggy::LevelTileRenderChunkVisibilityResult result = Query(cache, view.bounds);

	Expect(IndexesEqual(result.chunkIndexes, { 0 }), "CameraView bounds should feed chunk visibility without production coupling");
}

void TestQueryDoesNotMutateCacheOrCommands()
{
	iggy::LevelTileRenderChunkCache cache = CacheFromRows({
		".#",
		"..",
	}, 1, 1);
	const std::size_t originalChunkCount = cache.chunks.size();
	const std::size_t commandCount = cache.chunks.empty() ? 0 : cache.chunks[0].commands.commands.size();
	const iggy::ResourceId material = cache.chunks.empty() || cache.chunks[0].commands.commands.empty() ? iggy::ResourceId {} : cache.chunks[0].commands.commands[0].materialId;
	const std::size_t order = cache.chunks.empty() || cache.chunks[0].commands.commands.empty() ? 0 : cache.chunks[0].commands.commands[0].order;

	const iggy::LevelTileRenderChunkVisibilityResult result = Query(cache, { { 0.0F, 0.0F }, { 2.0F, 2.0F } });

	Expect(result.hasChunks, "immutability query setup should select chunks");
	Expect(cache.chunks.size() == originalChunkCount, "visibility query should not mutate chunk count");
	if (!cache.chunks.empty()) {
		Expect(cache.chunks[0].commands.commands.size() == commandCount, "visibility query should not mutate command count");
		if (!cache.chunks[0].commands.commands.empty()) {
			Expect(cache.chunks[0].commands.commands[0].materialId == material, "visibility query should not mutate command material");
			Expect(cache.chunks[0].commands.commands[0].order == order, "visibility query should not mutate command order");
		}
	}
}

} // namespace

int main()
{
	TestEmptyCacheReturnsNoChunks();
	TestOutsideBoundsReturnNoChunks();
	TestBoundsInsideOneChunkReturnThatIndex();
	TestBoundsOverlappingMultipleChunksReturnCacheOrder();
	TestExactMaxEdgeUsesHalfOpenSemantics();
	TestBoundsStartingAtChunkMaxEdgeSkipPreviousChunk();
	TestInvertedBoundsNormalize();
	TestDegeneratePointInsideChunk();
	TestDegeneratePointOnSharedBoundarySelectsContainingChunks();
	TestCameraViewCompositionSelectsExpectedChunks();
	TestQueryDoesNotMutateCacheOrCommands();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
