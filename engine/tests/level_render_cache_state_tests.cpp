#include <cstdlib>
#include <string_view>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/level/LevelRenderCacheState.hpp"
#include "scene/level/LevelRenderFrame2D.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::SameBounds;

const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };
const iggy::ResourceId NpcMaterial { "material:npc" };

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

iggy::LevelTileMap Map(std::vector<std::string_view> rows)
{
	return iggy::test::MapFromRows(rows);
}

iggy::LevelTileRenderChunkCacheConfig ChunkConfig(int chunkWidth = 2, int chunkHeight = 2, int layer = 1)
{
	return { chunkWidth, chunkHeight, { { WalkableMaterial, BlockedMaterial }, layer } };
}

iggy::LevelRenderFrame2DConfig FrameConfig(const iggy::LevelRenderCacheState &cacheState)
{
	iggy::LevelRenderFrame2DConfig config {
		{ { 2.0F, 2.0F }, 1.0F },
		{ { WalkableMaterial, BlockedMaterial }, 99 },
		{ NpcMaterial, { 1.0F, 1.0F }, { 0.5F, 0.5F }, 8 },
		false,
	};
	config.useTileChunkCache = true;
	config.tileChunkCache = &cacheState.tileChunks;
	return config;
}

void ExpectCommand(const iggy::render::RenderCommand2D &command, iggy::Aabb2 bounds, const iggy::ResourceId &materialId, int layer, std::size_t order, const char *message)
{
	Expect(command.type == iggy::render::RenderCommand2DType::Quad && SameBounds(command.worldBounds, bounds) && command.materialId == materialId && command.layer == layer && command.order == order, message);
}

void TestBuildValidMapCreatesCacheState()
{
	const iggy::LevelTileMap map = Map({
		"....",
		"....",
	});
	const iggy::LevelTileRenderChunkCacheConfig config = ChunkConfig(2, 2, 4);

	const iggy::LevelRenderCacheBuildResult result = iggy::LevelRenderCacheBuilder {}.build(map, config);

	Expect(result.built, "valid render cache build should succeed");
	Expect(result.tileChunkIssues.empty(), "valid render cache build should expose no tile chunk issues");
	Expect(result.state.tileChunkConfig.chunkWidth == 2 && result.state.tileChunkConfig.chunkHeight == 2 && result.state.tileChunkConfig.tileCommands.layer == 4, "render cache state should preserve tile chunk config");
	Expect(result.state.tileChunks.chunks.size() == 2, "render cache state should store built tile chunk cache");
	if (result.state.tileChunks.chunks.size() == 2)
		Expect(SameChunkCoord(result.state.tileChunks.chunks[1].coord, 1, 0), "render cache build should preserve chunk cache order");
}

void TestBuildInvalidChunkSizeFailsWithIssues()
{
	const iggy::LevelTileMap map = Map({ "." });

	const iggy::LevelRenderCacheBuildResult result = iggy::LevelRenderCacheBuilder {}.build(map, ChunkConfig(0, 2));

	Expect(!result.built, "invalid render cache build should fail");
	Expect(result.state.tileChunks.chunks.empty(), "invalid render cache build should not produce tile chunks");
	Expect(result.tileChunkIssues.size() == 1, "invalid render cache build should expose tile chunk issue");
	if (result.tileChunkIssues.size() == 1)
		Expect(result.tileChunkIssues[0].code == iggy::LevelTileRenderChunkCacheIssueCode::InvalidChunkSize && result.tileChunkIssues[0].chunkWidth == 0 && result.tileChunkIssues[0].chunkHeight == 2, "invalid render cache build should preserve issue dimensions");
}

void TestUpdateEmptyChangedTilesCopiesState()
{
	const iggy::LevelTileMap map = Map({
		"..",
		"..",
	});
	const iggy::LevelRenderCacheState current = iggy::LevelRenderCacheBuilder {}.build(map, ChunkConfig()).state;

	const iggy::LevelRenderCacheUpdateResult result = iggy::LevelRenderCacheUpdater {}.update(current, map, {});

	Expect(result.updated, "empty changed tiles should be a successful no-op update");
	Expect(result.dirtyChunks.queried && result.dirtyChunks.chunks.empty(), "empty changed tiles should query to no dirty chunks");
	Expect(result.tileChunkUpdate.updated && result.tileChunkUpdate.rebuiltChunks.empty() && result.tileChunkUpdate.skippedChunks.empty(), "empty changed tiles should call tile chunk updater with no rebuilds/skips");
	Expect(result.state.tileChunkConfig.chunkWidth == current.tileChunkConfig.chunkWidth && result.state.tileChunkConfig.chunkHeight == current.tileChunkConfig.chunkHeight, "empty changed tile update should preserve config");
	Expect(result.state.tileChunks.chunks.size() == current.tileChunks.chunks.size(), "empty changed tile update should copy cache");
}

void TestUpdateChangedTileRebuildsExpectedChunk()
{
	const iggy::LevelTileMap oldMap = Map({
		"..",
		"..",
	});
	iggy::LevelTileMap updatedMap = oldMap;
	updatedMap.tiles[1].walkable = false;
	const iggy::LevelRenderCacheState current = iggy::LevelRenderCacheBuilder {}.build(oldMap, ChunkConfig()).state;

	const iggy::LevelRenderCacheUpdateResult result = iggy::LevelRenderCacheUpdater {}.update(current, updatedMap, { { 1, 0 } });

	Expect(result.updated, "changed tile should update render cache state");
	Expect(ChunkCoordsEqual(result.dirtyChunks.chunks, { { 0, 0 } }), "changed tile should map to expected dirty chunk");
	Expect(ChunkCoordsEqual(result.tileChunkUpdate.rebuiltChunks, { { 0, 0 } }), "changed tile should rebuild expected chunk");
	Expect(result.state.tileChunks.chunks.size() == 1, "changed tile update should keep cache shape");
	if (result.state.tileChunks.chunks.size() == 1 && result.state.tileChunks.chunks[0].commands.commands.size() == 4)
		ExpectCommand(result.state.tileChunks.chunks[0].commands.commands[1], { { 1.0F, 0.0F }, { 2.0F, 1.0F } }, BlockedMaterial, 1, 1, "updated cache state should reflect changed blocked tile");
}

void TestUpdateDuplicateChangedTilesRebuildsOnce()
{
	const iggy::LevelTileMap map = Map({
		"..",
		"..",
	});
	const iggy::LevelRenderCacheState current = iggy::LevelRenderCacheBuilder {}.build(map, ChunkConfig()).state;

	const iggy::LevelRenderCacheUpdateResult result = iggy::LevelRenderCacheUpdater {}.update(current, map, { { 0, 0 }, { 0, 0 }, { 1, 1 } });

	Expect(result.updated, "duplicate changed tiles should update successfully");
	Expect(ChunkCoordsEqual(result.dirtyChunks.chunks, { { 0, 0 } }), "duplicate changed tiles should deduplicate in dirty query");
	Expect(ChunkCoordsEqual(result.tileChunkUpdate.rebuiltChunks, { { 0, 0 } }), "duplicate changed tiles should rebuild one chunk");
}

void TestUpdateOutOfMapChangedTileRecordsSkippedChunk()
{
	const iggy::LevelTileMap map = Map({
		"..",
		"..",
	});
	const iggy::LevelRenderCacheState current = iggy::LevelRenderCacheBuilder {}.build(map, ChunkConfig()).state;

	const iggy::LevelRenderCacheUpdateResult result = iggy::LevelRenderCacheUpdater {}.update(current, map, { { 4, 0 } });

	Expect(result.updated, "out-of-map changed tile should still produce successful cache update result");
	Expect(ChunkCoordsEqual(result.dirtyChunks.chunks, { { 2, 0 } }), "out-of-map changed tile should map to dirty chunk coord");
	Expect(result.tileChunkUpdate.rebuiltChunks.empty(), "out-of-map dirty chunk should not rebuild");
	Expect(ChunkCoordsEqual(result.tileChunkUpdate.skippedChunks, { { 2, 0 } }), "out-of-map dirty chunk should be recorded as skipped");
}

void TestUpdateUsesCurrentStateMaterialConfig()
{
	const iggy::LevelTileMap oldMap = Map({
		"..",
		"..",
	});
	iggy::LevelTileMap updatedMap = oldMap;
	updatedMap.tiles[3].walkable = false;
	const iggy::LevelTileRenderChunkCacheConfig config = ChunkConfig(2, 2, 6);
	const iggy::LevelRenderCacheState current = iggy::LevelRenderCacheBuilder {}.build(oldMap, config).state;

	const iggy::LevelRenderCacheUpdateResult result = iggy::LevelRenderCacheUpdater {}.update(current, updatedMap, { { 1, 1 } });

	Expect(result.updated, "material config update setup should update");
	Expect(result.state.tileChunkConfig.tileCommands.layer == 6, "updated render cache state should preserve current config layer");
	if (!result.state.tileChunks.chunks.empty() && result.state.tileChunks.chunks[0].commands.commands.size() == 4)
		ExpectCommand(result.state.tileChunks.chunks[0].commands.commands[3], { { 1.0F, 1.0F }, { 2.0F, 2.0F } }, BlockedMaterial, 6, 3, "updated cache should use current state's material config");
}

void TestInputsAreNotMutated()
{
	const iggy::LevelTileMap map = Map({
		".#",
		"..",
	});
	const iggy::LevelRenderCacheState current = iggy::LevelRenderCacheBuilder {}.build(map, ChunkConfig()).state;
	const int originalWidth = map.width;
	const std::size_t originalTileCount = map.tiles.size();
	const std::size_t originalChunkCount = current.tileChunks.chunks.size();
	const iggy::ResourceId originalMaterial = current.tileChunks.chunks[0].commands.commands[1].materialId;
	const std::size_t originalOrder = current.tileChunks.chunks[0].commands.commands[1].order;

	const iggy::LevelRenderCacheUpdateResult result = iggy::LevelRenderCacheUpdater {}.update(current, map, { { 0, 0 } });

	Expect(result.updated, "immutability setup should update");
	Expect(map.width == originalWidth && map.tiles.size() == originalTileCount, "render cache update should not mutate map shape");
	Expect(current.tileChunks.chunks.size() == originalChunkCount, "render cache update should not mutate current cache chunk count");
	Expect(current.tileChunks.chunks[0].commands.commands[1].materialId == originalMaterial, "render cache update should not mutate current cache material");
	Expect(current.tileChunks.chunks[0].commands.commands[1].order == originalOrder, "render cache update should not mutate current cache command order");
}

void TestManualFrameCompositionUsesUpdatedCacheState()
{
	const iggy::LevelTileMap oldMap = Map({
		"..",
		"..",
	});
	iggy::LevelTileMap updatedMap = oldMap;
	updatedMap.tiles[3].walkable = false;
	const iggy::LevelRuntimeState runtimeState { updatedMap, {} };
	const iggy::LevelRenderCacheState initialCacheState = iggy::LevelRenderCacheBuilder {}.build(oldMap, ChunkConfig()).state;
	const iggy::LevelRenderFrame2DResult beforeUpdate = iggy::LevelRenderFrame2D {}.build(runtimeState, { { 1.0F, 1.0F } }, FrameConfig(initialCacheState));

	const iggy::LevelRenderCacheUpdateResult update = iggy::LevelRenderCacheUpdater {}.update(initialCacheState, updatedMap, { { 1, 1 } });
	const iggy::LevelRenderFrame2DResult afterUpdate = iggy::LevelRenderFrame2D {}.build(runtimeState, { { 1.0F, 1.0F } }, FrameConfig(update.state));

	Expect(beforeUpdate.usedTileChunkCache && afterUpdate.usedTileChunkCache, "manual composition should use cache path before and after update");
	Expect(beforeUpdate.commands.commands.size() == 4 && afterUpdate.commands.commands.size() == 4, "manual composition should render cached tile commands before and after update");
	if (beforeUpdate.commands.commands.size() == 4 && afterUpdate.commands.commands.size() == 4) {
		Expect(beforeUpdate.commands.commands[3].materialId == WalkableMaterial, "frame before cache update should use old cached walkable material");
		Expect(afterUpdate.commands.commands[3].materialId == BlockedMaterial, "frame after cache update should use updated cached blocked material");
	}
}

void TestDirtyQueryFailureStopsBeforeCacheUpdater()
{
	const iggy::LevelRenderCacheState current {
		ChunkConfig(0, 2),
		{},
	};
	const iggy::LevelTileMap map = Map({ "." });

	const iggy::LevelRenderCacheUpdateResult result = iggy::LevelRenderCacheUpdater {}.update(current, map, { { 0, 0 } });

	Expect(!result.updated, "dirty query failure should fail render cache update");
	Expect(!result.dirtyChunks.queried, "dirty query failure should expose unqueried dirty result");
	Expect(result.dirtyChunks.issues.size() == 1, "dirty query failure should expose dirty issue");
	Expect(!result.tileChunkUpdate.updated && result.tileChunkUpdate.issues.empty(), "dirty query failure should not call cache updater");
}

} // namespace

int main()
{
	TestBuildValidMapCreatesCacheState();
	TestBuildInvalidChunkSizeFailsWithIssues();
	TestUpdateEmptyChangedTilesCopiesState();
	TestUpdateChangedTileRebuildsExpectedChunk();
	TestUpdateDuplicateChangedTilesRebuildsOnce();
	TestUpdateOutOfMapChangedTileRecordsSkippedChunk();
	TestUpdateUsesCurrentStateMaterialConfig();
	TestInputsAreNotMutated();
	TestManualFrameCompositionUsesUpdatedCacheState();
	TestDirtyQueryFailureStopsBeforeCacheUpdater();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
