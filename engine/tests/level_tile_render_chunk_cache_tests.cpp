#include <cstdlib>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/level/LevelTileRenderChunkCache.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;
using iggy::test::SameTile;

const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };

bool SameBounds(iggy::Aabb2 actual, iggy::Aabb2 expected)
{
	return NearVec(actual.min, expected.min) && NearVec(actual.max, expected.max);
}

iggy::LevelTileRenderChunkCacheConfig Config(int chunkWidth, int chunkHeight, int layer = 2)
{
	return { chunkWidth, chunkHeight, { { WalkableMaterial, BlockedMaterial }, layer } };
}

iggy::LevelTileRenderChunkCacheBuildResult Build(const iggy::LevelTileMap &map, int chunkWidth, int chunkHeight)
{
	return iggy::LevelTileRenderChunkCacheBuilder {}.build(map, Config(chunkWidth, chunkHeight));
}

void ExpectChunkExtent(const iggy::LevelTileRenderChunk &chunk, int chunkX, int chunkY, iggy::TileCoord minTile, iggy::TileCoord maxTile, iggy::Aabb2 bounds, const char *message)
{
	Expect(chunk.coord.x == chunkX && chunk.coord.y == chunkY && SameTile(chunk.minTile, minTile.x, minTile.y) && SameTile(chunk.maxTile, maxTile.x, maxTile.y) && SameBounds(chunk.worldBounds, bounds), message);
}

void ExpectCommand(const iggy::render::RenderCommand2D &command, iggy::Aabb2 bounds, const iggy::ResourceId &materialId, int layer, std::size_t order, const char *message)
{
	Expect(command.type == iggy::render::RenderCommand2DType::Quad && SameBounds(command.worldBounds, bounds) && command.materialId == materialId && command.layer == layer && command.order == order, message);
}

void ExpectNoTexturePayload(const iggy::render::RenderCommand2D &command, const char *message)
{
	Expect(command.texture.textureId.empty() && !command.texture.hasSourceRect, message);
}

void TestInvalidChunkWidthFails()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({ "." });

	const iggy::LevelTileRenderChunkCacheBuildResult result = Build(map, 0, 1);

	Expect(!result.built, "invalid chunk width should fail build");
	Expect(result.cache.chunks.empty(), "invalid chunk width should return empty cache");
	Expect(result.issues.size() == 1, "invalid chunk width should report one issue");
	if (result.issues.size() == 1)
		Expect(result.issues[0].code == iggy::LevelTileRenderChunkCacheIssueCode::InvalidChunkSize && result.issues[0].chunkWidth == 0 && result.issues[0].chunkHeight == 1, "invalid chunk width issue should preserve config dimensions");
}

void TestInvalidChunkHeightFails()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({ "." });

	const iggy::LevelTileRenderChunkCacheBuildResult result = Build(map, 1, -2);

	Expect(!result.built, "invalid chunk height should fail build");
	Expect(result.cache.chunks.empty(), "invalid chunk height should return empty cache");
	Expect(result.issues.size() == 1, "invalid chunk height should report one issue");
	if (result.issues.size() == 1)
		Expect(result.issues[0].code == iggy::LevelTileRenderChunkCacheIssueCode::InvalidChunkSize && result.issues[0].chunkWidth == 1 && result.issues[0].chunkHeight == -2, "invalid chunk height issue should preserve config dimensions");
}

void TestEmptyMapBuildsEmptyCache()
{
	const iggy::LevelTileMap map;

	const iggy::LevelTileRenderChunkCacheBuildResult result = Build(map, 2, 2);

	Expect(result.built, "empty map should build successfully");
	Expect(result.issues.empty(), "empty map should not report issues");
	Expect(result.cache.chunks.empty(), "empty map should produce empty cache");
}

void TestOneByOneMapBuildsOneChunk()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({ "." });

	const iggy::LevelTileRenderChunkCacheBuildResult result = Build(map, 1, 1);

	Expect(result.built, "1x1 map should build");
	Expect(result.cache.chunks.size() == 1, "1x1 map with 1x1 chunks should produce one chunk");
	if (result.cache.chunks.size() == 1) {
		const iggy::LevelTileRenderChunk &chunk = result.cache.chunks[0];
		ExpectChunkExtent(chunk, 0, 0, { 0, 0 }, { 0, 0 }, { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, "single chunk should cover the only tile");
		Expect(chunk.commands.commands.size() == 1, "single chunk should contain one tile command");
		if (chunk.commands.commands.size() == 1)
			ExpectCommand(chunk.commands.commands[0], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, WalkableMaterial, 2, 0, "single tile command should match the only tile");
	}
}

void TestMapSmallerThanChunkBuildsOneEdgeChunk()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"..",
		"..",
	});

	const iggy::LevelTileRenderChunkCacheBuildResult result = Build(map, 4, 4);

	Expect(result.cache.chunks.size() == 1, "map smaller than chunk should produce one chunk");
	if (result.cache.chunks.size() == 1)
		ExpectChunkExtent(result.cache.chunks[0], 0, 0, { 0, 0 }, { 1, 1 }, { { 0.0F, 0.0F }, { 2.0F, 2.0F } }, "oversized chunk should clamp to map extents");
}

void TestThreeByTwoMapWithWidthTwoChunks()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"...",
		"...",
	});

	const iggy::LevelTileRenderChunkCacheBuildResult result = Build(map, 2, 2);

	Expect(result.cache.chunks.size() == 2, "3x2 map with 2x2 chunks should produce two chunks");
	if (result.cache.chunks.size() == 2) {
		ExpectChunkExtent(result.cache.chunks[0], 0, 0, { 0, 0 }, { 1, 1 }, { { 0.0F, 0.0F }, { 2.0F, 2.0F } }, "first chunk should cover x 0-1 y 0-1");
		ExpectChunkExtent(result.cache.chunks[1], 1, 0, { 2, 0 }, { 2, 1 }, { { 2.0F, 0.0F }, { 3.0F, 2.0F } }, "second chunk should cover x 2-2 y 0-1");
	}
}

void TestFourByThreeMapBuildsRowMajorChunks()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"....",
		"....",
		"....",
	});

	const iggy::LevelTileRenderChunkCacheBuildResult result = Build(map, 2, 2);

	Expect(result.cache.chunks.size() == 4, "4x3 map with 2x2 chunks should produce four chunks");
	if (result.cache.chunks.size() == 4) {
		ExpectChunkExtent(result.cache.chunks[0], 0, 0, { 0, 0 }, { 1, 1 }, { { 0.0F, 0.0F }, { 2.0F, 2.0F } }, "chunk 0 should be top-left");
		ExpectChunkExtent(result.cache.chunks[1], 1, 0, { 2, 0 }, { 3, 1 }, { { 2.0F, 0.0F }, { 4.0F, 2.0F } }, "chunk 1 should be top-right");
		ExpectChunkExtent(result.cache.chunks[2], 0, 1, { 0, 2 }, { 1, 2 }, { { 0.0F, 2.0F }, { 2.0F, 3.0F } }, "chunk 2 should be bottom-left edge");
		ExpectChunkExtent(result.cache.chunks[3], 1, 1, { 2, 2 }, { 3, 2 }, { { 2.0F, 2.0F }, { 4.0F, 3.0F } }, "chunk 3 should be bottom-right edge");
	}
}

void TestCommandsInsideChunkPreserveTileRowMajorOrder()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"..",
		"..",
	});

	const iggy::LevelTileRenderChunkCacheBuildResult result = Build(map, 2, 2);

	Expect(result.cache.chunks.size() == 1, "row-major command test should produce one chunk");
	if (result.cache.chunks.size() == 1) {
		const std::vector<iggy::render::RenderCommand2D> &commands = result.cache.chunks[0].commands.commands;
		Expect(commands.size() == 4, "2x2 chunk should produce four commands");
		if (commands.size() == 4) {
			ExpectCommand(commands[0], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, WalkableMaterial, 2, 0, "first command should be tile 0,0");
			ExpectCommand(commands[1], { { 1.0F, 0.0F }, { 2.0F, 1.0F } }, WalkableMaterial, 2, 1, "second command should be tile 1,0");
			ExpectCommand(commands[2], { { 0.0F, 1.0F }, { 1.0F, 2.0F } }, WalkableMaterial, 2, 2, "third command should be tile 0,1");
			ExpectCommand(commands[3], { { 1.0F, 1.0F }, { 2.0F, 2.0F } }, WalkableMaterial, 2, 3, "fourth command should be tile 1,1");
		}
	}
}

void TestBlockedMaterialsAndMaterialOnlyPayload()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		".#",
	});

	const iggy::LevelTileRenderChunkCacheBuildResult result = Build(map, 2, 1);

	Expect(result.cache.chunks.size() == 1, "blocked material test should produce one chunk");
	if (result.cache.chunks.size() == 1) {
		const std::vector<iggy::render::RenderCommand2D> &commands = result.cache.chunks[0].commands.commands;
		Expect(commands.size() == 2, "blocked material test should emit two commands");
		if (commands.size() == 2) {
			ExpectCommand(commands[0], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, WalkableMaterial, 2, 0, "walkable tile should use walkable material");
			ExpectCommand(commands[1], { { 1.0F, 0.0F }, { 2.0F, 1.0F } }, BlockedMaterial, 2, 1, "blocked tile should use blocked material");
			ExpectNoTexturePayload(commands[0], "walkable chunk command should remain material-only");
			ExpectNoTexturePayload(commands[1], "blocked chunk command should remain material-only");
		}
	}
}

void TestSparseTileStorageSkipsMissingTiles()
{
	iggy::LevelTileMap map;
	map.width = 3;
	map.height = 1;
	map.tiles.push_back({ true });
	map.tiles.push_back({ false });

	const iggy::LevelTileRenderChunkCacheBuildResult result = Build(map, 3, 1);

	Expect(result.cache.chunks.size() == 1, "sparse map should still produce chunk metadata");
	if (result.cache.chunks.size() == 1) {
		const std::vector<iggy::render::RenderCommand2D> &commands = result.cache.chunks[0].commands.commands;
		Expect(commands.size() == 2, "sparse map should skip missing tile storage");
		if (commands.size() == 2) {
			Expect(commands[0].materialId == WalkableMaterial, "present walkable sparse tile should emit walkable material");
			Expect(commands[1].materialId == BlockedMaterial, "present blocked sparse tile should emit blocked material");
		}
	}
}

void TestBuildDoesNotMutateMap()
{
	iggy::LevelTileMap map = iggy::test::MapFromRows({
		".#",
		"..",
	});
	const int originalWidth = map.width;
	const int originalHeight = map.height;
	const std::vector<iggy::LevelTile> originalTiles = map.tiles;

	const iggy::LevelTileRenderChunkCacheBuildResult result = Build(map, 1, 1);

	Expect(result.built, "immutability setup should build");
	Expect(map.width == originalWidth && map.height == originalHeight, "chunk build should not mutate map dimensions");
	Expect(map.tiles.size() == originalTiles.size(), "chunk build should not mutate tile count");
	if (map.tiles.size() == originalTiles.size()) {
		for (std::size_t index = 0; index < map.tiles.size(); ++index)
			Expect(map.tiles[index].walkable == originalTiles[index].walkable, "chunk build should not mutate tile walkability");
	}
}

} // namespace

int main()
{
	TestInvalidChunkWidthFails();
	TestInvalidChunkHeightFails();
	TestEmptyMapBuildsEmptyCache();
	TestOneByOneMapBuildsOneChunk();
	TestMapSmallerThanChunkBuildsOneEdgeChunk();
	TestThreeByTwoMapWithWidthTwoChunks();
	TestFourByThreeMapBuildsRowMajorChunks();
	TestCommandsInsideChunkPreserveTileRowMajorOrder();
	TestBlockedMaterialsAndMaterialOnlyPayload();
	TestSparseTileStorageSkipsMissingTiles();
	TestBuildDoesNotMutateMap();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
