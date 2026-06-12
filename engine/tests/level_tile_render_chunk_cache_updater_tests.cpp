#include <cstdlib>
#include <string_view>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/level/LevelTileRenderChunkCacheUpdater.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::SameBounds;
using iggy::test::SameTile;

const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };
const iggy::ResourceId UpdatedWalkableMaterial { "material:updated_floor" };
const iggy::ResourceId UpdatedBlockedMaterial { "material:updated_wall" };

bool SameChunkCoord(iggy::LevelTileRenderChunkCoord actual, int x, int y)
{
	return actual.x == x && actual.y == y;
}

bool ChunkCoordsEqual(const std::vector<iggy::LevelTileRenderChunkCoord> &actual, std::vector<iggy::LevelTileRenderChunkCoord> expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameChunkCoord(actual[index], expected[index].x, expected[index].y))
			return false;
	}
	return true;
}

iggy::LevelTileRenderChunkCacheConfig Config(int chunkWidth = 2, int chunkHeight = 2, iggy::ResourceId walkable = WalkableMaterial, iggy::ResourceId blocked = BlockedMaterial, int layer = 2)
{
	return { chunkWidth, chunkHeight, { { walkable, blocked }, layer } };
}

iggy::LevelTileMap Map(std::vector<std::string_view> rows)
{
	return iggy::test::MapFromRows(rows);
}

iggy::LevelTileRenderChunkCache BuildCache(const iggy::LevelTileMap &map, const iggy::LevelTileRenderChunkCacheConfig &config = Config())
{
	return iggy::LevelTileRenderChunkCacheBuilder {}.build(map, config).cache;
}

iggy::LevelTileRenderChunkCacheUpdateResult Update(const iggy::LevelTileRenderChunkCache &current, const iggy::LevelTileMap &map, const iggy::LevelTileRenderChunkCacheConfig &config, std::vector<iggy::LevelTileRenderChunkCoord> dirty)
{
	return iggy::LevelTileRenderChunkCacheUpdater {}.update(current, map, config, dirty);
}

void ExpectCommand(const iggy::render::RenderCommand2D &command, iggy::Aabb2 bounds, const iggy::ResourceId &materialId, int layer, std::size_t order, const char *message)
{
	Expect(command.type == iggy::render::RenderCommand2DType::Quad && SameBounds(command.worldBounds, bounds) && command.materialId == materialId && command.layer == layer && command.order == order, message);
}

void ExpectChunksEquivalent(const iggy::LevelTileRenderChunk &actual, const iggy::LevelTileRenderChunk &expected, const char *message)
{
	Expect(SameChunkCoord(actual.coord, expected.coord.x, expected.coord.y) && SameTile(actual.minTile, expected.minTile.x, expected.minTile.y) && SameTile(actual.maxTile, expected.maxTile.x, expected.maxTile.y) && SameBounds(actual.worldBounds, expected.worldBounds) && actual.commands.commands.size() == expected.commands.commands.size(), message);
	if (actual.commands.commands.size() != expected.commands.commands.size())
		return;

	for (std::size_t index = 0; index < actual.commands.commands.size(); ++index) {
		const iggy::render::RenderCommand2D &actualCommand = actual.commands.commands[index];
		const iggy::render::RenderCommand2D &expectedCommand = expected.commands.commands[index];
		Expect(actualCommand.type == expectedCommand.type && SameBounds(actualCommand.worldBounds, expectedCommand.worldBounds) && actualCommand.materialId == expectedCommand.materialId && actualCommand.layer == expectedCommand.layer && actualCommand.order == expectedCommand.order && actualCommand.texture.hasSourceRect == expectedCommand.texture.hasSourceRect, "chunk command should match expected full rebuild command");
	}
}

void TestInvalidChunkWidthFails()
{
	const iggy::LevelTileMap map = Map({ "." });
	const iggy::LevelTileRenderChunkCache current = BuildCache(map, Config(1, 1));

	const iggy::LevelTileRenderChunkCacheUpdateResult result = Update(current, map, Config(0, 1), { { 0, 0 } });

	Expect(!result.updated, "invalid chunk width should fail update");
	Expect(result.cache.chunks.empty(), "invalid chunk width should return empty cache");
	Expect(result.issues.size() == 1, "invalid chunk width should report one issue");
	if (result.issues.size() == 1)
		Expect(result.issues[0].code == iggy::LevelTileRenderChunkCacheUpdateIssueCode::InvalidChunkSize && result.issues[0].chunkWidth == 0 && result.issues[0].chunkHeight == 1, "invalid chunk width issue should preserve dimensions");
}

void TestInvalidChunkHeightFails()
{
	const iggy::LevelTileMap map = Map({ "." });
	const iggy::LevelTileRenderChunkCache current = BuildCache(map, Config(1, 1));

	const iggy::LevelTileRenderChunkCacheUpdateResult result = Update(current, map, Config(1, -2), { { 0, 0 } });

	Expect(!result.updated, "invalid chunk height should fail update");
	Expect(result.cache.chunks.empty(), "invalid chunk height should return empty cache");
	Expect(result.issues.size() == 1, "invalid chunk height should report one issue");
	if (result.issues.size() == 1)
		Expect(result.issues[0].code == iggy::LevelTileRenderChunkCacheUpdateIssueCode::InvalidChunkSize && result.issues[0].chunkWidth == 1 && result.issues[0].chunkHeight == -2, "invalid chunk height issue should preserve dimensions");
}

void TestEmptyDirtyChunksReturnsCopiedCache()
{
	const iggy::LevelTileMap map = Map({
		"..",
		"..",
	});
	const iggy::LevelTileRenderChunkCache current = BuildCache(map);

	const iggy::LevelTileRenderChunkCacheUpdateResult result = Update(current, map, Config(), {});

	Expect(result.updated, "empty dirty chunks should still produce an updated result");
	Expect(result.rebuiltChunks.empty() && result.skippedChunks.empty(), "empty dirty chunks should rebuild and skip nothing");
	Expect(result.cache.chunks.size() == current.chunks.size(), "empty dirty chunks should copy current cache shape");
	if (!result.cache.chunks.empty())
		ExpectChunksEquivalent(result.cache.chunks[0], current.chunks[0], "empty dirty chunks should preserve existing chunk data");
}

void TestOneDirtyChunkRebuildsOnlyThatChunk()
{
	const iggy::LevelTileMap oldMap = Map({
		"....",
		"....",
	});
	iggy::LevelTileMap updatedMap = oldMap;
	updatedMap.tiles[1].walkable = false;
	const iggy::LevelTileRenderChunkCache current = BuildCache(oldMap);

	const iggy::LevelTileRenderChunkCacheUpdateResult result = Update(current, updatedMap, Config(), { { 0, 0 } });

	Expect(result.updated, "one dirty chunk should update");
	Expect(ChunkCoordsEqual(result.rebuiltChunks, { { 0, 0 } }), "one dirty chunk should record one rebuilt chunk");
	Expect(result.skippedChunks.empty(), "one valid dirty chunk should not skip");
	Expect(result.cache.chunks.size() == current.chunks.size(), "one dirty chunk update should preserve cache vector size");
	if (result.cache.chunks.size() == 2) {
		ExpectCommand(result.cache.chunks[0].commands.commands[1], { { 1.0F, 0.0F }, { 2.0F, 1.0F } }, BlockedMaterial, 2, 1, "rebuilt dirty chunk should reflect changed blocked tile");
		ExpectChunksEquivalent(result.cache.chunks[1], current.chunks[1], "non-dirty chunk should remain unchanged");
	}
}

void TestRebuiltChunkUsesCurrentMapAndMaterialConfig()
{
	const iggy::LevelTileMap oldMap = Map({
		"..",
		"..",
	});
	iggy::LevelTileMap updatedMap = oldMap;
	updatedMap.tiles[3].walkable = false;
	const iggy::LevelTileRenderChunkCache current = BuildCache(oldMap, Config(2, 2, WalkableMaterial, BlockedMaterial, 2));

	const iggy::LevelTileRenderChunkCacheUpdateResult result = Update(current, updatedMap, Config(2, 2, UpdatedWalkableMaterial, UpdatedBlockedMaterial, 7), { { 0, 0 } });

	Expect(ChunkCoordsEqual(result.rebuiltChunks, { { 0, 0 } }), "changed map/material setup should rebuild target chunk");
	Expect(result.cache.chunks.size() == 1, "changed map/material setup should keep one chunk");
	if (result.cache.chunks.size() == 1 && result.cache.chunks[0].commands.commands.size() == 4) {
		ExpectCommand(result.cache.chunks[0].commands.commands[0], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, UpdatedWalkableMaterial, 7, 0, "rebuilt chunk should use current walkable material config");
		ExpectCommand(result.cache.chunks[0].commands.commands[3], { { 1.0F, 1.0F }, { 2.0F, 2.0F } }, UpdatedBlockedMaterial, 7, 3, "rebuilt chunk should use current blocked material config");
		Expect(!result.cache.chunks[0].commands.commands[3].texture.hasSourceRect, "rebuilt tile command should remain material-only");
	}
}

void TestDuplicateDirtyChunksRebuildOnce()
{
	const iggy::LevelTileMap map = Map({
		"..",
		"..",
	});
	const iggy::LevelTileRenderChunkCache current = BuildCache(map);

	const iggy::LevelTileRenderChunkCacheUpdateResult result = Update(current, map, Config(), { { 0, 0 }, { 0, 0 } });

	Expect(ChunkCoordsEqual(result.rebuiltChunks, { { 0, 0 } }), "duplicate dirty chunks should rebuild once");
	Expect(result.skippedChunks.empty(), "duplicate dirty chunks should not skip valid chunk");
}

void TestOutOfMapDirtyChunksAreSkipped()
{
	const iggy::LevelTileMap map = Map({
		"..",
		"..",
	});
	const iggy::LevelTileRenderChunkCache current = BuildCache(map);

	const iggy::LevelTileRenderChunkCacheUpdateResult result = Update(current, map, Config(), { { -1, 0 }, { 2, 0 } });

	Expect(result.rebuiltChunks.empty(), "out-of-map dirty chunks should not rebuild");
	Expect(ChunkCoordsEqual(result.skippedChunks, { { -1, 0 }, { 2, 0 } }), "out-of-map dirty chunks should be skipped in order");
}

void TestValidMapChunkAbsentFromCurrentCacheIsSkipped()
{
	const iggy::LevelTileMap map = Map({
		"....",
		"....",
	});
	iggy::LevelTileRenderChunkCache incomplete = BuildCache(map);
	incomplete.chunks.resize(1);

	const iggy::LevelTileRenderChunkCacheUpdateResult result = Update(incomplete, map, Config(), { { 1, 0 } });

	Expect(result.rebuiltChunks.empty(), "absent current chunk should not be appended");
	Expect(ChunkCoordsEqual(result.skippedChunks, { { 1, 0 } }), "absent current chunk should be skipped");
	Expect(result.cache.chunks.size() == 1, "updater should preserve incomplete cache shape");
}

void TestSparseTileStorageDoesNotCrash()
{
	iggy::LevelTileMap map;
	map.width = 3;
	map.height = 1;
	map.tiles.push_back({ true });
	map.tiles.push_back({ false });
	const iggy::LevelTileRenderChunkCache current = BuildCache(map, Config(3, 1));

	const iggy::LevelTileRenderChunkCacheUpdateResult result = Update(current, map, Config(3, 1), { { 0, 0 } });

	Expect(ChunkCoordsEqual(result.rebuiltChunks, { { 0, 0 } }), "sparse map should still rebuild present chunk");
	Expect(result.cache.chunks.size() == 1, "sparse map should keep one chunk");
	if (result.cache.chunks.size() == 1)
		Expect(result.cache.chunks[0].commands.commands.size() == 2, "sparse map rebuild should skip missing tile storage");
}

void TestRebuiltChunkMatchesFullRebuild()
{
	iggy::LevelTileMap map = Map({
		"....",
		"..#.",
	});
	const iggy::LevelTileRenderChunkCache oldCache = BuildCache(Map({
		"....",
		"....",
	}));
	const iggy::LevelTileRenderChunkCache fullRebuild = BuildCache(map);

	const iggy::LevelTileRenderChunkCacheUpdateResult result = Update(oldCache, map, Config(), { { 1, 0 } });

	Expect(ChunkCoordsEqual(result.rebuiltChunks, { { 1, 0 } }), "full rebuild comparison should rebuild right chunk");
	Expect(result.cache.chunks.size() == fullRebuild.chunks.size(), "updated cache should preserve full rebuild chunk count");
	if (result.cache.chunks.size() == fullRebuild.chunks.size())
		ExpectChunksEquivalent(result.cache.chunks[1], fullRebuild.chunks[1], "dirty rebuilt chunk should match full rebuild for that chunk");
}

void TestInputsAreNotMutated()
{
	const iggy::LevelTileMap map = Map({
		".#",
		"..",
	});
	const iggy::LevelTileRenderChunkCache current = BuildCache(map);
	const int originalWidth = map.width;
	const std::size_t originalTileCount = map.tiles.size();
	const std::size_t originalChunkCount = current.chunks.size();
	const iggy::ResourceId originalMaterial = current.chunks[0].commands.commands[1].materialId;
	const std::size_t originalOrder = current.chunks[0].commands.commands[1].order;

	const iggy::LevelTileRenderChunkCacheUpdateResult result = Update(current, map, Config(), { { 0, 0 } });

	Expect(result.updated, "input immutability setup should update");
	Expect(map.width == originalWidth && map.tiles.size() == originalTileCount, "updater should not mutate map shape");
	Expect(current.chunks.size() == originalChunkCount, "updater should not mutate current cache chunk count");
	Expect(current.chunks[0].commands.commands[1].materialId == originalMaterial, "updater should not mutate current cache command material");
	Expect(current.chunks[0].commands.commands[1].order == originalOrder, "updater should not mutate current cache command order");
}

} // namespace

int main()
{
	TestInvalidChunkWidthFails();
	TestInvalidChunkHeightFails();
	TestEmptyDirtyChunksReturnsCopiedCache();
	TestOneDirtyChunkRebuildsOnlyThatChunk();
	TestRebuiltChunkUsesCurrentMapAndMaterialConfig();
	TestDuplicateDirtyChunksRebuildOnce();
	TestOutOfMapDirtyChunksAreSkipped();
	TestValidMapChunkAbsentFromCurrentCacheIsSkipped();
	TestSparseTileStorageDoesNotCrash();
	TestRebuiltChunkMatchesFullRebuild();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
