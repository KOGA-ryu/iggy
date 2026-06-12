#include <cstdlib>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/level/LevelRenderFrame2D.hpp"
#include "scene/level/LevelTileRenderChunkCache.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };
const iggy::ResourceId NpcMaterial { "material:npc" };

bool SameBounds(iggy::Aabb2 actual, iggy::Aabb2 expected)
{
	return NearVec(actual.min, expected.min) && NearVec(actual.max, expected.max);
}

iggy::npc_ai::NpcAgentEntry Agent(const char *id, iggy::Vec2 position)
{
	iggy::npc_ai::NpcAgentEntry agent;
	agent.id = iggy::ResourceId { id };
	agent.state.position = position;
	return agent;
}

iggy::LevelRuntimeState State(std::vector<std::string_view> rows, std::vector<iggy::npc_ai::NpcAgentEntry> agents = {})
{
	return { iggy::test::MapFromRows(rows), agents };
}

iggy::LevelRenderFrame2DConfig Config(bool includeNpcCommands = true)
{
	return {
		{ { 2.0F, 2.0F }, 1.0F },
		{ { WalkableMaterial, BlockedMaterial }, 1 },
		{ NpcMaterial, { 1.0F, 1.0F }, { 0.5F, 0.5F }, 8 },
		includeNpcCommands,
	};
}

iggy::LevelTileRenderChunkCache BuildCache(const iggy::LevelTileMap &map, int chunkWidth = 2, int chunkHeight = 2, int layer = 1)
{
	const iggy::LevelTileRenderChunkCacheBuildResult result = iggy::LevelTileRenderChunkCacheBuilder {}.build(map, { chunkWidth, chunkHeight, { { WalkableMaterial, BlockedMaterial }, layer } });
	return result.cache;
}

void ExpectCommand(const iggy::render::RenderCommand2D &command, iggy::Aabb2 bounds, const iggy::ResourceId &materialId, int layer, std::size_t order, const char *message)
{
	Expect(command.type == iggy::render::RenderCommand2DType::Quad && SameBounds(command.worldBounds, bounds) && command.materialId == materialId && command.layer == layer && command.order == order, message);
}

void TestEmptyStateComputesCameraViewAndNoCommands()
{
	const iggy::LevelRuntimeState state;
	const iggy::LevelRenderFrame2DResult result = iggy::LevelRenderFrame2D {}.build(state, { { 5.0F, 5.0F } }, { { { 4.0F, 2.0F }, 1.0F }, { { WalkableMaterial, BlockedMaterial }, 1 }, { NpcMaterial }, true });

	Expect(SameBounds(result.cameraView.bounds, { { 3.0F, 4.0F }, { 7.0F, 6.0F } }), "empty state should still compute camera view bounds");
	Expect(!result.visibleTiles.hasTiles && result.visibleTiles.tiles.empty(), "empty state should have no visible tiles");
	Expect(result.tileDrawList.items.empty(), "empty state should have no tile draw items");
	Expect(!result.usedTileChunkCache, "default empty state should use direct tile path");
	Expect(!result.visibleTileChunks.hasChunks && result.visibleTileChunks.chunkIndexes.empty(), "default direct path should not report visible tile chunks");
	Expect(result.tileChunkCommands.commands.commands.empty() && result.tileChunkCommands.usedChunkIndexes.empty(), "default direct path should not report tile chunk commands");
	Expect(result.commands.commands.empty(), "empty state should emit no commands");
}

void TestVisibleMapEmitsTileCommandsWithMaterialsAndOrder()
{
	const iggy::LevelRuntimeState state = State({
		"..",
		".#",
	});
	const iggy::LevelRenderFrame2DResult result = iggy::LevelRenderFrame2D {}.build(state, { { 1.0F, 1.0F } }, Config(false));

	Expect(!result.usedTileChunkCache, "default visible map render should use direct tile path");
	Expect(result.visibleTileChunks.chunkIndexes.empty(), "default visible map render should not run chunk visibility");
	Expect(result.tileChunkCommands.commands.commands.empty(), "default visible map render should not compose chunk commands");
	Expect(result.visibleTiles.hasTiles, "visible map should report visible tiles");
	Expect(result.tileDrawList.items.size() == 4, "visible 2x2 map should produce four tile draw items");
	Expect(result.commands.commands.size() == 4, "visible 2x2 map should produce four tile commands");
	if (result.commands.commands.size() == 4) {
		ExpectCommand(result.commands.commands[0], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, WalkableMaterial, 1, 0, "first tile command should use walkable material");
		ExpectCommand(result.commands.commands[1], { { 1.0F, 0.0F }, { 2.0F, 1.0F } }, WalkableMaterial, 1, 1, "second tile command should preserve order");
		ExpectCommand(result.commands.commands[2], { { 0.0F, 1.0F }, { 1.0F, 2.0F } }, WalkableMaterial, 1, 2, "third tile command should preserve row-major order");
		ExpectCommand(result.commands.commands[3], { { 1.0F, 1.0F }, { 2.0F, 2.0F } }, BlockedMaterial, 1, 3, "blocked tile command should use blocked material");
	}
}

void TestNpcCommandsAppendAfterTilesWithNormalizedOrder()
{
	const iggy::LevelRuntimeState state = State({
		"..",
		".#",
	}, {
		Agent("npc:one", { 0.5F, 0.5F }),
	});

	const iggy::LevelRenderFrame2DResult result = iggy::LevelRenderFrame2D {}.build(state, { { 1.0F, 1.0F } }, Config());

	Expect(result.commands.commands.size() == 5, "tile commands plus one NPC command should produce five commands");
	if (result.commands.commands.size() == 5) {
		Expect(result.commands.commands[3].materialId == BlockedMaterial && result.commands.commands[3].order == 3, "last tile command should keep tile order before NPC commands");
		ExpectCommand(result.commands.commands[4], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, NpcMaterial, 8, 4, "NPC command should append after tile commands with normalized order");
	}
}

void TestIncludeNpcCommandsFalseExcludesNpcCommands()
{
	const iggy::LevelRuntimeState state = State({
		"..",
		"..",
	}, {
		Agent("npc:one", { 0.5F, 0.5F }),
	});

	const iggy::LevelRenderFrame2DResult result = iggy::LevelRenderFrame2D {}.build(state, { { 1.0F, 1.0F } }, Config(false));

	Expect(result.commands.commands.size() == 4, "excluding NPC commands should leave only tile commands");
	if (result.commands.commands.size() == 4)
		Expect(result.commands.commands[3].materialId == WalkableMaterial && result.commands.commands[3].order == 3, "excluded NPC should not add a command after tiles");
}

void TestCachePathUsesVisibleChunksAndCommands()
{
	const iggy::LevelRuntimeState state = State({
		"....",
		"....",
		"....",
		"....",
	});
	const iggy::LevelTileRenderChunkCache cache = BuildCache(state.map, 2, 2, 4);
	iggy::LevelRenderFrame2DConfig config = Config(false);
	config.cameraView.viewportSize = { 2.0F, 2.0F };
	config.tileCommands.layer = 9;
	config.useTileChunkCache = true;
	config.tileChunkCache = &cache;

	const iggy::LevelRenderFrame2DResult result = iggy::LevelRenderFrame2D {}.build(state, { { 1.0F, 1.0F } }, config);

	Expect(result.usedTileChunkCache, "valid cache config should use chunk cache path");
	Expect(!result.visibleTiles.hasTiles && result.visibleTiles.tiles.empty(), "cache path should leave direct visible tiles empty");
	Expect(result.tileDrawList.items.empty(), "cache path should leave direct tile draw list empty");
	Expect(result.visibleTileChunks.chunkIndexes == std::vector<std::size_t> { 0 }, "cache path should query visible chunk indexes");
	Expect(result.tileChunkCommands.usedChunkIndexes == std::vector<std::size_t> { 0 }, "cache path should compose visible chunk commands");
	Expect(result.commands.commands.size() == 4, "cache path should emit visible chunk tile commands");
	if (result.commands.commands.size() == 4) {
		ExpectCommand(result.commands.commands[0], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, WalkableMaterial, 4, 0, "cache path should use cached tile command layer, not direct config layer");
		ExpectCommand(result.commands.commands[3], { { 1.0F, 1.0F }, { 2.0F, 2.0F } }, WalkableMaterial, 4, 3, "cache path should preserve chunk command order");
		Expect(!result.commands.commands[0].texture.hasSourceRect, "cache tile command should remain material-only");
	}
}

void TestCachePathAppendsNpcCommandsAfterChunkCommands()
{
	const iggy::LevelRuntimeState state = State({
		"..",
		"..",
	}, {
		Agent("npc:one", { 0.5F, 0.5F }),
	});
	const iggy::LevelTileRenderChunkCache cache = BuildCache(state.map, 2, 2);
	iggy::LevelRenderFrame2DConfig config = Config();
	config.useTileChunkCache = true;
	config.tileChunkCache = &cache;

	const iggy::LevelRenderFrame2DResult result = iggy::LevelRenderFrame2D {}.build(state, { { 1.0F, 1.0F } }, config);

	Expect(result.commands.commands.size() == 5, "cache tile commands plus one NPC should produce five commands");
	if (result.commands.commands.size() == 5) {
		Expect(result.commands.commands[3].materialId == WalkableMaterial && result.commands.commands[3].order == 3, "cache path last tile command should precede NPC");
		ExpectCommand(result.commands.commands[4], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, NpcMaterial, 8, 4, "cache path NPC command should append after chunk commands");
	}
}

void TestCachePathCanExcludeNpcCommands()
{
	const iggy::LevelRuntimeState state = State({
		"..",
		"..",
	}, {
		Agent("npc:one", { 0.5F, 0.5F }),
	});
	const iggy::LevelTileRenderChunkCache cache = BuildCache(state.map, 2, 2);
	iggy::LevelRenderFrame2DConfig config = Config(false);
	config.useTileChunkCache = true;
	config.tileChunkCache = &cache;

	const iggy::LevelRenderFrame2DResult result = iggy::LevelRenderFrame2D {}.build(state, { { 1.0F, 1.0F } }, config);

	Expect(result.usedTileChunkCache, "cache path should still be used when NPC commands are excluded");
	Expect(result.commands.commands.size() == 4, "cache path with NPCs excluded should emit only chunk tile commands");
	if (result.commands.commands.size() == 4)
		Expect(result.commands.commands[3].materialId == WalkableMaterial && result.commands.commands[3].order == 3, "excluded NPC should not add command after cache tiles");
}

void TestNullCacheFallsBackToDirectPath()
{
	const iggy::LevelRuntimeState state = State({
		"..",
		".#",
	});
	iggy::LevelRenderFrame2DConfig config = Config(false);
	config.useTileChunkCache = true;
	config.tileChunkCache = nullptr;

	const iggy::LevelRenderFrame2DResult result = iggy::LevelRenderFrame2D {}.build(state, { { 1.0F, 1.0F } }, config);

	Expect(!result.usedTileChunkCache, "null cache with flag set should fall back to direct path");
	Expect(result.visibleTiles.hasTiles, "fallback direct path should still compute visible tiles");
	Expect(result.tileDrawList.items.size() == 4, "fallback direct path should still build tile draw list");
	Expect(result.commands.commands.size() == 4, "fallback direct path should still emit tile commands");
}

void TestCachePathCameraChangesSelectedChunks()
{
	const iggy::LevelRuntimeState state = State({
		"....",
		"....",
		"....",
		"....",
	});
	const iggy::LevelTileRenderChunkCache cache = BuildCache(state.map, 2, 2);
	iggy::LevelRenderFrame2DConfig config = Config(false);
	config.cameraView.viewportSize = { 2.0F, 2.0F };
	config.useTileChunkCache = true;
	config.tileChunkCache = &cache;

	const iggy::LevelRenderFrame2DResult left = iggy::LevelRenderFrame2D {}.build(state, { { 1.0F, 1.0F } }, config);
	const iggy::LevelRenderFrame2DResult right = iggy::LevelRenderFrame2D {}.build(state, { { 3.0F, 1.0F } }, config);

	Expect(left.visibleTileChunks.chunkIndexes == std::vector<std::size_t> { 0 }, "left cache camera should select left chunk");
	Expect(right.visibleTileChunks.chunkIndexes == std::vector<std::size_t> { 1 }, "right cache camera should select right chunk");
	Expect(left.commands.commands.size() == 4 && right.commands.commands.size() == 4, "each cache camera should emit one chunk of commands");
	if (left.commands.commands.size() == 4 && right.commands.commands.size() == 4) {
		Expect(SameBounds(left.commands.commands[0].worldBounds, { { 0.0F, 0.0F }, { 1.0F, 1.0F } }), "left cache camera should emit left chunk commands");
		Expect(SameBounds(right.commands.commands[0].worldBounds, { { 2.0F, 0.0F }, { 3.0F, 1.0F } }), "right cache camera should emit right chunk commands");
	}
}

void TestNpcsAreNotCameraCulled()
{
	const iggy::LevelRuntimeState state = State({
		".",
	}, {
		Agent("npc:far", { 10.0F, 10.0F }),
	});

	const iggy::LevelRenderFrame2DResult result = iggy::LevelRenderFrame2D {}.build(state, { { 0.5F, 0.5F } }, Config());

	Expect(result.commands.commands.size() == 2, "far NPC should still emit a command after visible tile command");
	if (result.commands.commands.size() == 2)
		ExpectCommand(result.commands.commands[1], { { 9.5F, 9.5F }, { 10.5F, 10.5F } }, NpcMaterial, 8, 1, "NPC outside camera-visible tiles should not be culled in this builder");
}

void TestMapEmptyStillIncludesNpcCommands()
{
	iggy::LevelRuntimeState state;
	state.npcAgents.push_back(Agent("npc:one", { 2.0F, 3.0F }));

	const iggy::LevelRenderFrame2DResult result = iggy::LevelRenderFrame2D {}.build(state, { { 0.0F, 0.0F } }, Config());

	Expect(!result.visibleTiles.hasTiles && result.tileDrawList.items.empty(), "empty map should produce no visible tile draw data");
	Expect(result.commands.commands.size() == 1, "empty map with NPC should still emit NPC command");
	if (result.commands.commands.size() == 1)
		ExpectCommand(result.commands.commands[0], { { 1.5F, 2.5F }, { 2.5F, 3.5F } }, NpcMaterial, 8, 0, "NPC command should be normalized to order zero when there are no tile commands");
}

void TestTileAndNpcLayersArePreservedButNotSorted()
{
	iggy::LevelRenderFrame2DConfig config = Config();
	config.tileCommands.layer = 10;
	config.npcCommands.layer = -5;
	const iggy::LevelRuntimeState state = State({
		".",
	}, {
		Agent("npc:one", { 0.5F, 0.5F }),
	});

	const iggy::LevelRenderFrame2DResult result = iggy::LevelRenderFrame2D {}.build(state, { { 0.5F, 0.5F } }, config);

	Expect(result.commands.commands.size() == 2, "tile and NPC command setup should produce two commands");
	if (result.commands.commands.size() == 2) {
		Expect(result.commands.commands[0].layer == 10 && result.commands.commands[0].order == 0, "tile layer should be preserved first without sorting");
		Expect(result.commands.commands[1].layer == -5 && result.commands.commands[1].order == 1, "NPC layer should be preserved after tiles without sorting");
	}
}

void TestPresentationCameraChangesVisibleTileCoverage()
{
	const iggy::LevelRuntimeState state = State({
		"...",
	});
	iggy::LevelRenderFrame2DConfig config = Config(false);
	config.cameraView.viewportSize = { 1.0F, 1.0F };

	const iggy::LevelRenderFrame2DResult left = iggy::LevelRenderFrame2D {}.build(state, { { 0.5F, 0.5F } }, config);
	const iggy::LevelRenderFrame2DResult right = iggy::LevelRenderFrame2D {}.build(state, { { 2.5F, 0.5F } }, config);

	Expect(left.commands.commands.size() == 1 && right.commands.commands.size() == 1, "one-tile camera views should each emit one tile command");
	if (left.commands.commands.size() == 1 && right.commands.commands.size() == 1) {
		Expect(SameBounds(left.commands.commands[0].worldBounds, { { 0.0F, 0.0F }, { 1.0F, 1.0F } }), "left presentation camera should see left tile");
		Expect(SameBounds(right.commands.commands[0].worldBounds, { { 2.0F, 0.0F }, { 3.0F, 1.0F } }), "right presentation camera should see right tile");
	}
}

void TestInputRuntimeStateIsNotMutated()
{
	const iggy::LevelRuntimeState state = State({
		".#",
	}, {
		Agent("npc:one", { 0.5F, 0.5F }),
	});
	const iggy::Vec2 originalPosition = state.npcAgents[0].state.position;
	const int originalWidth = state.map.width;
	const std::size_t originalTileCount = state.map.tiles.size();

	const iggy::LevelRenderFrame2DResult result = iggy::LevelRenderFrame2D {}.build(state, { { 0.5F, 0.5F } }, Config());

	Expect(!result.commands.commands.empty(), "state mutation test should produce commands");
	Expect(state.map.width == originalWidth && state.map.tiles.size() == originalTileCount, "render frame build should not mutate map shape");
	Expect(NearVec(state.npcAgents[0].state.position, originalPosition), "render frame build should not mutate NPC position");
}

void TestCachePathDoesNotMutateStateOrCache()
{
	const iggy::LevelRuntimeState state = State({
		".#",
		"..",
	}, {
		Agent("npc:one", { 0.5F, 0.5F }),
	});
	iggy::LevelTileRenderChunkCache cache = BuildCache(state.map, 2, 2);
	const int originalWidth = state.map.width;
	const std::size_t originalTileCount = state.map.tiles.size();
	const iggy::Vec2 originalPosition = state.npcAgents[0].state.position;
	const std::size_t originalChunkCount = cache.chunks.size();
	const std::size_t originalCommandCount = cache.chunks[0].commands.commands.size();
	const iggy::ResourceId originalMaterial = cache.chunks[0].commands.commands[1].materialId;
	const std::size_t originalOrder = cache.chunks[0].commands.commands[1].order;
	iggy::LevelRenderFrame2DConfig config = Config();
	config.useTileChunkCache = true;
	config.tileChunkCache = &cache;

	const iggy::LevelRenderFrame2DResult result = iggy::LevelRenderFrame2D {}.build(state, { { 1.0F, 1.0F } }, config);

	Expect(result.usedTileChunkCache, "cache immutability setup should use cache path");
	Expect(state.map.width == originalWidth && state.map.tiles.size() == originalTileCount, "cache path should not mutate map shape");
	Expect(NearVec(state.npcAgents[0].state.position, originalPosition), "cache path should not mutate NPC position");
	Expect(cache.chunks.size() == originalChunkCount, "cache path should not mutate chunk count");
	if (!cache.chunks.empty() && cache.chunks[0].commands.commands.size() == originalCommandCount) {
		Expect(cache.chunks[0].commands.commands[1].materialId == originalMaterial, "cache path should not mutate cached command material");
		Expect(cache.chunks[0].commands.commands[1].order == originalOrder, "cache path should not mutate cached command order");
		Expect(!cache.chunks[0].commands.commands[1].texture.hasSourceRect, "cache path should not mutate cached texture payload");
	}
}

void TestPresentationCameraIsUsedWithoutCameraUpdateBehavior()
{
	const iggy::LevelRuntimeState state = State({
		"...",
	});
	iggy::LevelRenderFrame2DConfig config = Config(false);
	config.cameraView.viewportSize = { 1.0F, 1.0F };

	const iggy::LevelRenderFrame2DResult result = iggy::LevelRenderFrame2D {}.build(state, { { 1.5F, 0.5F } }, config);

	Expect(SameBounds(result.cameraView.bounds, { { 1.0F, 0.0F }, { 2.0F, 1.0F } }), "camera view should be computed directly from supplied presentation camera state");
	Expect(result.commands.commands.size() == 1, "presentation camera should drive visible tile coverage directly");
	if (result.commands.commands.size() == 1)
		Expect(SameBounds(result.commands.commands[0].worldBounds, { { 1.0F, 0.0F }, { 2.0F, 1.0F } }), "no rig/lookahead/shake update should alter the supplied presentation camera");
}

} // namespace

int main()
{
	TestEmptyStateComputesCameraViewAndNoCommands();
	TestVisibleMapEmitsTileCommandsWithMaterialsAndOrder();
	TestNpcCommandsAppendAfterTilesWithNormalizedOrder();
	TestIncludeNpcCommandsFalseExcludesNpcCommands();
	TestCachePathUsesVisibleChunksAndCommands();
	TestCachePathAppendsNpcCommandsAfterChunkCommands();
	TestCachePathCanExcludeNpcCommands();
	TestNullCacheFallsBackToDirectPath();
	TestCachePathCameraChangesSelectedChunks();
	TestNpcsAreNotCameraCulled();
	TestMapEmptyStillIncludesNpcCommands();
	TestTileAndNpcLayersArePreservedButNotSorted();
	TestPresentationCameraChangesVisibleTileCoverage();
	TestInputRuntimeStateIsNotMutated();
	TestCachePathDoesNotMutateStateOrCache();
	TestPresentationCameraIsUsedWithoutCameraUpdateBehavior();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
