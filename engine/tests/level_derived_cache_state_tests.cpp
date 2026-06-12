#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "scene/level/LevelCollisionCacheState.hpp"
#include "scene/level/LevelDerivedCacheState.hpp"
#include "scene/level/LevelGridQuery.hpp"
#include "scene/level/LevelRenderCacheState.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectBounds;
using iggy::test::Failures;
using iggy::test::MapFromRows;

const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };

iggy::LevelRuntimeState RuntimeLevel(std::initializer_list<std::string_view> rows)
{
	iggy::LevelRuntimeState level;
	level.map = MapFromRows(std::vector<std::string_view>(rows));
	return level;
}

iggy::LevelTileRenderChunkCacheConfig RenderConfig(int chunkWidth = 2, int chunkHeight = 2, int layer = 4)
{
	return { chunkWidth, chunkHeight, { { WalkableMaterial, BlockedMaterial }, layer } };
}

iggy::LevelDerivedCacheBuildConfig Config(
	bool buildRender,
	bool buildCollision,
	iggy::LevelTileRenderChunkCacheConfig renderConfig = RenderConfig())
{
	iggy::LevelDerivedCacheBuildConfig config;
	config.buildRenderCache = buildRender;
	config.renderCacheConfig = renderConfig;
	config.buildCollisionCache = buildCollision;
	return config;
}

void ExpectCollisionObjectsMatch(
	const iggy::physics2d::CollisionWorld2D &actual,
	const iggy::physics2d::CollisionWorld2D &expected,
	const char *message)
{
	Expect(actual.objects().size() == expected.objects().size(), message);
	for (std::size_t index = 0; index < actual.objects().size() && index < expected.objects().size(); ++index) {
		Expect(actual.objects()[index].id == expected.objects()[index].id, message);
		Expect(actual.objects()[index].solid == expected.objects()[index].solid, message);
		Expect(actual.objects()[index].shape.type == expected.objects()[index].shape.type, message);
		ExpectBounds(actual.objects()[index].shape.bounds, expected.objects()[index].shape.bounds, message);
	}
}

void ExpectRuntimeLevelUnchanged(
	const iggy::LevelRuntimeState &actual,
	const iggy::LevelRuntimeState &expected,
	const char *message)
{
	Expect(actual.map.id == expected.map.id, message);
	Expect(actual.map.width == expected.map.width && actual.map.height == expected.map.height, message);
	Expect(actual.map.tiles.size() == expected.map.tiles.size(), message);
	for (std::size_t index = 0; index < actual.map.tiles.size() && index < expected.map.tiles.size(); ++index)
		Expect(actual.map.tiles[index].walkable == expected.map.tiles[index].walkable, message);
	Expect(actual.npcAgents.size() == expected.npcAgents.size(), message);
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

bool CommandsContainMaterial(const iggy::LevelRenderCacheState &cache, iggy::ResourceId material)
{
	for (const iggy::LevelTileRenderChunk &chunk : cache.tileChunks.chunks) {
		for (const iggy::render::RenderCommand2D &command : chunk.commands.commands) {
			if (command.materialId == material)
				return true;
		}
	}
	return false;
}

iggy::LevelDerivedCacheState BuildDerivedCaches(
	const iggy::LevelRuntimeState &level,
	bool buildRender,
	bool buildCollision,
	iggy::LevelTileRenderChunkCacheConfig renderConfig = RenderConfig())
{
	const iggy::LevelDerivedCacheBuildResult build = iggy::LevelDerivedCacheBuilder {}.build(level, Config(buildRender, buildCollision, renderConfig));
	Expect(build.built, "derived cache fixture should build");
	return build.state;
}

bool FileContains(const char *path, const char *needle)
{
	std::ifstream file(path);
	std::ostringstream contents;
	contents << file.rdbuf();
	return contents.str().find(needle) != std::string::npos;
}

void TestNoRequestedCachesBuildsEmptyDerivedState()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({ ".#" });

	const iggy::LevelDerivedCacheBuildResult result = iggy::LevelDerivedCacheBuilder {}.build(level, Config(false, false));

	Expect(result.built, "no requested caches should build successfully");
	Expect(!result.state.hasRenderCache, "no requested caches should leave render cache absent");
	Expect(!result.state.hasCollisionCache, "no requested caches should leave collision cache absent");
	Expect(!result.render.built, "no requested caches should not build render cache");
	Expect(!result.collision.built, "no requested caches should not build collision cache");
}

void TestCollisionOnlyBuildStoresCollisionCache()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({
		".#.",
	});
	const iggy::LevelCollisionCacheBuildResult collision = iggy::LevelCollisionCacheBuilder {}.build(level.map);

	const iggy::LevelDerivedCacheBuildResult result = iggy::LevelDerivedCacheBuilder {}.build(level, Config(false, true));

	Expect(result.built, "collision-only derived cache build should succeed");
	Expect(!result.state.hasRenderCache, "collision-only build should leave render cache absent");
	Expect(result.state.hasCollisionCache, "collision-only build should store collision cache");
	Expect(!result.render.built, "collision-only build should not invoke render cache build");
	Expect(result.collision.built, "collision-only build should preserve nested collision build");
	ExpectCollisionObjectsMatch(result.state.collision.world, collision.state.world, "collision-only derived cache should match collision cache builder output");
}

void TestRenderOnlyBuildStoresRenderCache()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({
		"..",
		"..",
	});
	const iggy::LevelTileRenderChunkCacheConfig renderConfig = RenderConfig(2, 2, 7);
	const iggy::LevelRenderCacheBuildResult render = iggy::LevelRenderCacheBuilder {}.build(level.map, renderConfig);

	const iggy::LevelDerivedCacheBuildResult result = iggy::LevelDerivedCacheBuilder {}.build(level, Config(true, false, renderConfig));

	Expect(render.built, "render-only expected render cache should build");
	Expect(result.built, "render-only derived cache build should succeed");
	Expect(result.state.hasRenderCache, "render-only build should store render cache");
	Expect(!result.state.hasCollisionCache, "render-only build should leave collision cache absent");
	Expect(result.render.built, "render-only build should preserve nested render build");
	Expect(!result.collision.built, "render-only build should not invoke collision cache build");
	Expect(result.state.render.tileChunkConfig.chunkWidth == renderConfig.chunkWidth && result.state.render.tileChunkConfig.chunkHeight == renderConfig.chunkHeight, "render-only build should preserve render config");
	Expect(result.state.render.tileChunkConfig.tileCommands.layer == 7, "render-only build should preserve render layer config");
	Expect(result.state.render.tileChunks.chunks.size() == render.state.tileChunks.chunks.size(), "render-only build should match render cache chunk count");
}

void TestBothRequestedCachesStoreBothOnSuccess()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({
		".#",
		"..",
	});
	const iggy::LevelTileRenderChunkCacheConfig renderConfig = RenderConfig(2, 2, 5);

	const iggy::LevelDerivedCacheBuildResult result = iggy::LevelDerivedCacheBuilder {}.build(level, Config(true, true, renderConfig));

	Expect(result.built, "both requested caches should build successfully");
	Expect(result.state.hasRenderCache, "both requested caches should store render cache");
	Expect(result.state.hasCollisionCache, "both requested caches should store collision cache");
	Expect(result.render.built, "both requested caches should preserve nested render build");
	Expect(result.collision.built, "both requested caches should preserve nested collision build");
	Expect(result.state.render.tileChunks.chunks.size() == 1, "both requested caches should store render chunks");
	Expect(result.state.collision.world.objects().size() == 1, "both requested caches should store collision objects");
	if (result.state.collision.world.objects().size() == 1)
		ExpectBounds(result.state.collision.world.objects()[0].shape.bounds, iggy::tileBounds({ 1, 0 }), "both requested collision cache should use blocked tile bounds");
}

void TestCollisionCacheMatchesBuilderForBlockedTiles()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({
		"#.#",
		".#.",
	});
	const iggy::LevelCollisionCacheBuildResult collision = iggy::LevelCollisionCacheBuilder {}.build(level.map);

	const iggy::LevelDerivedCacheBuildResult result = iggy::LevelDerivedCacheBuilder {}.build(level, Config(false, true));

	Expect(collision.built, "collision expected cache should build");
	Expect(result.built, "collision match derived cache should build");
	ExpectCollisionObjectsMatch(result.state.collision.world, collision.state.world, "derived collision cache should match direct collision cache builder output");
	Expect(result.state.collision.world.objects().size() == 3, "derived collision cache should preserve blocked tile row-major count");
}

void TestRenderCacheMatchesBuilderEnoughToProveDelegation()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({
		"..#",
		"...",
	});
	const iggy::LevelTileRenderChunkCacheConfig renderConfig = RenderConfig(2, 2, 6);
	const iggy::LevelRenderCacheBuildResult render = iggy::LevelRenderCacheBuilder {}.build(level.map, renderConfig);

	const iggy::LevelDerivedCacheBuildResult result = iggy::LevelDerivedCacheBuilder {}.build(level, Config(true, false, renderConfig));

	Expect(render.built, "render expected cache should build");
	Expect(result.built, "render delegation derived cache should build");
	Expect(result.state.render.tileChunks.chunks.size() == render.state.tileChunks.chunks.size(), "derived render cache should match direct render chunk count");
	if (!result.state.render.tileChunks.chunks.empty() && !render.state.tileChunks.chunks.empty()) {
		Expect(result.state.render.tileChunks.chunks[0].commands.commands.size() == render.state.tileChunks.chunks[0].commands.commands.size(), "derived render cache should match direct command count");
		Expect(result.state.render.tileChunks.chunks[0].commands.commands[0].materialId == render.state.tileChunks.chunks[0].commands.commands[0].materialId, "derived render cache should preserve delegated command material");
		Expect(result.state.render.tileChunks.chunks[0].commands.commands[0].layer == render.state.tileChunks.chunks[0].commands.commands[0].layer, "derived render cache should preserve delegated command layer");
	}
}

void TestRenderFailurePreservesDiagnosticsAndPublishesNoPartialState()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({
		".#",
	});
	const iggy::LevelTileRenderChunkCacheConfig invalidRenderConfig = RenderConfig(0, 2, 3);

	const iggy::LevelDerivedCacheBuildResult result = iggy::LevelDerivedCacheBuilder {}.build(level, Config(true, true, invalidRenderConfig));

	Expect(!result.built, "requested render failure should fail derived cache build");
	Expect(!result.state.hasRenderCache, "failed derived cache build should not publish render cache");
	Expect(!result.state.hasCollisionCache, "failed derived cache build should not publish collision cache even if collision succeeded");
	Expect(!result.render.built, "failed derived cache build should preserve nested render failure");
	Expect(result.render.tileChunkIssues.size() == 1, "failed derived cache build should preserve render diagnostics");
	if (result.render.tileChunkIssues.size() == 1)
		Expect(result.render.tileChunkIssues[0].code == iggy::LevelTileRenderChunkCacheIssueCode::InvalidChunkSize, "failed derived cache build should preserve invalid chunk size code");
	Expect(result.collision.built, "failed derived cache build should still preserve attempted collision build result");
}

void TestSuccessfulEmptyAndNonpositiveMapsFollowNestedBuilders()
{
	iggy::LevelRuntimeState level;
	level.map.width = -2;
	level.map.height = 3;
	level.map.tiles.push_back({ false });

	const iggy::LevelDerivedCacheBuildResult result = iggy::LevelDerivedCacheBuilder {}.build(level, Config(true, true, RenderConfig()));

	Expect(result.built, "nonpositive map should follow nested builders and build derived caches");
	Expect(result.state.hasRenderCache, "nonpositive map should still publish requested render cache");
	Expect(result.state.hasCollisionCache, "nonpositive map should still publish requested collision cache");
	Expect(result.state.render.tileChunks.chunks.empty(), "nonpositive map render cache should be empty");
	Expect(result.state.collision.world.objects().empty(), "nonpositive map collision cache should be empty");
}

void TestInputLevelRuntimeStateIsNotMutated()
{
	iggy::LevelRuntimeState level = RuntimeLevel({
		".#",
		"..",
	});
	level.map.id = iggy::ResourceId("level:test");
	level.npcAgents.push_back({ iggy::ResourceId("npc:one"), {} });
	const iggy::LevelRuntimeState before = level;

	const iggy::LevelDerivedCacheBuildResult result = iggy::LevelDerivedCacheBuilder {}.build(level, Config(true, true, RenderConfig()));

	Expect(result.built, "derived cache immutability setup should build");
	ExpectRuntimeLevelUnchanged(level, before, "derived cache build should not mutate LevelRuntimeState");
}

void TestRuntimeAndLevelRuntimeDoNotReferenceDerivedCacheState()
{
	Expect(!FileContains("engine/src/runtime/RuntimeSessionState.hpp", "LevelDerivedCacheState"), "RuntimeSessionState should not reference LevelDerivedCacheState yet");
	Expect(!FileContains("engine/src/scene/level/LevelRuntimeState.hpp", "LevelDerivedCacheState"), "LevelRuntimeState should not reference LevelDerivedCacheState");
}

void TestUpdateWithNoCachesPresentCopiesCurrentState()
{
	const iggy::LevelDerivedCacheState current;
	const iggy::LevelRuntimeState level = RuntimeLevel({ ".#" });
	const std::vector<iggy::TileCoord> changedTiles { { 1, 0 } };

	const iggy::LevelDerivedCacheUpdateResult result = iggy::LevelDerivedCacheUpdater {}.update(current, level, changedTiles);

	Expect(result.updated, "derived update with no present caches should succeed");
	Expect(!result.state.hasRenderCache, "derived update with no present caches should leave render absent");
	Expect(!result.state.hasCollisionCache, "derived update with no present caches should leave collision absent");
	Expect(!result.render.updated, "derived update with no render cache should not call render updater");
	Expect(!result.collision.updated, "derived update with no collision cache should not call collision updater");
	ExpectChangedTiles(result.changedTiles, changedTiles, "derived update should preserve changed tile diagnostics");
}

void TestRenderOnlyUpdateDelegatesToRenderUpdater()
{
	const iggy::LevelTileRenderChunkCacheConfig renderConfig = RenderConfig(2, 1, 8);
	const iggy::LevelRuntimeState initial = RuntimeLevel({ ".." });
	const iggy::LevelDerivedCacheState current = BuildDerivedCaches(initial, true, false, renderConfig);
	const iggy::LevelRuntimeState updated = RuntimeLevel({ ".#" });
	const std::vector<iggy::TileCoord> changedTiles { { 1, 0 } };

	const iggy::LevelDerivedCacheUpdateResult result = iggy::LevelDerivedCacheUpdater {}.update(current, updated, changedTiles);

	Expect(result.updated, "render-only derived update should succeed");
	Expect(result.state.hasRenderCache, "render-only update should preserve render cache presence");
	Expect(!result.state.hasCollisionCache, "render-only update should keep collision cache absent");
	Expect(result.render.updated, "render-only update should preserve nested render update success");
	Expect(result.render.dirtyChunks.queried, "render-only update should delegate dirty chunk query");
	Expect(result.render.tileChunkUpdate.rebuiltChunks.size() == 1, "render-only update should rebuild one dirty render chunk");
	Expect(CommandsContainMaterial(result.state.render, BlockedMaterial), "render-only update should reflect current blocked tile material");
	Expect(!result.collision.updated, "render-only update should not call collision updater");
	ExpectChangedTiles(result.changedTiles, changedTiles, "render-only update should preserve changed tile diagnostics");
}

void TestCollisionOnlyUpdateDelegatesToCollisionUpdater()
{
	const iggy::LevelRuntimeState initial = RuntimeLevel({ "..." });
	const iggy::LevelDerivedCacheState current = BuildDerivedCaches(initial, false, true);
	const iggy::LevelRuntimeState updated = RuntimeLevel({ ".#." });
	const std::vector<iggy::TileCoord> changedTiles { { 1, 0 } };

	const iggy::LevelDerivedCacheUpdateResult result = iggy::LevelDerivedCacheUpdater {}.update(current, updated, changedTiles);

	Expect(result.updated, "collision-only derived update should succeed");
	Expect(!result.state.hasRenderCache, "collision-only update should keep render cache absent");
	Expect(result.state.hasCollisionCache, "collision-only update should preserve collision cache presence");
	Expect(!result.render.updated, "collision-only update should not call render updater");
	Expect(result.collision.updated, "collision-only update should preserve nested collision update success");
	ExpectChangedTiles(result.collision.changedTiles, changedTiles, "collision-only update should pass changed tiles to nested collision updater");
	Expect(result.state.collision.world.objects().size() == 1, "collision-only update should add current blocked tile object");
	if (result.state.collision.world.objects().size() == 1)
		ExpectBounds(result.state.collision.world.objects()[0].shape.bounds, iggy::tileBounds({ 1, 0 }), "collision-only update should use current blocked tile bounds");
}

void TestBothCachesPresentUpdateBothCaches()
{
	const iggy::LevelTileRenderChunkCacheConfig renderConfig = RenderConfig(2, 1, 9);
	const iggy::LevelRuntimeState initial = RuntimeLevel({ ".." });
	const iggy::LevelDerivedCacheState current = BuildDerivedCaches(initial, true, true, renderConfig);
	const iggy::LevelRuntimeState updated = RuntimeLevel({ "#." });
	const std::vector<iggy::TileCoord> changedTiles { { 0, 0 } };

	const iggy::LevelDerivedCacheUpdateResult result = iggy::LevelDerivedCacheUpdater {}.update(current, updated, changedTiles);

	Expect(result.updated, "both-cache derived update should succeed");
	Expect(result.state.hasRenderCache, "both-cache update should preserve render cache presence");
	Expect(result.state.hasCollisionCache, "both-cache update should preserve collision cache presence");
	Expect(result.render.updated, "both-cache update should preserve nested render success");
	Expect(result.collision.updated, "both-cache update should preserve nested collision success");
	Expect(CommandsContainMaterial(result.state.render, BlockedMaterial), "both-cache update should refresh render cache");
	Expect(result.state.collision.world.objects().size() == 1, "both-cache update should refresh collision cache");
	if (result.state.collision.world.objects().size() == 1)
		ExpectBounds(result.state.collision.world.objects()[0].shape.bounds, iggy::tileBounds({ 0, 0 }), "both-cache update should refresh collision object bounds");
}

void TestEmptyChangedTilesNoOpsThroughNestedUpdaters()
{
	const iggy::LevelTileRenderChunkCacheConfig renderConfig = RenderConfig(2, 1, 10);
	const iggy::LevelRuntimeState level = RuntimeLevel({ ".#" });
	const iggy::LevelDerivedCacheState current = BuildDerivedCaches(level, true, true, renderConfig);

	const iggy::LevelDerivedCacheUpdateResult result = iggy::LevelDerivedCacheUpdater {}.update(current, level, {});

	Expect(result.updated, "empty changed tile derived update should succeed");
	Expect(result.render.updated, "empty changed tile derived update should preserve render no-op success");
	Expect(result.render.dirtyChunks.queried, "empty changed tile derived update should still expose render dirty query");
	Expect(result.render.tileChunkUpdate.rebuiltChunks.empty(), "empty changed tile derived update should rebuild no render chunks");
	Expect(result.collision.updated, "empty changed tile derived update should preserve collision no-op success");
	Expect(result.collision.changedTiles.empty(), "empty changed tile derived update should preserve nested collision diagnostics");
	ExpectCollisionObjectsMatch(result.state.collision.world, current.collision.world, "empty changed tile derived update should copy collision cache");
	Expect(result.state.render.tileChunks.chunks.size() == current.render.tileChunks.chunks.size(), "empty changed tile derived update should copy render cache");
}

void TestRenderUpdateFailureFailsOverallWithoutPartialState()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({ ".#" });
	iggy::LevelDerivedCacheState current;
	current.hasRenderCache = true;
	current.render.tileChunkConfig = RenderConfig(0, 1, 11);
	current.hasCollisionCache = true;
	current.collision = BuildDerivedCaches(level, false, true).collision;

	const iggy::LevelDerivedCacheUpdateResult result = iggy::LevelDerivedCacheUpdater {}.update(current, level, { { 1, 0 } });

	Expect(!result.updated, "render update failure should fail combined derived update");
	Expect(!result.state.hasRenderCache, "render update failure should publish no render cache");
	Expect(!result.state.hasCollisionCache, "render update failure should publish no collision cache");
	Expect(!result.render.updated, "render update failure should preserve nested render failure");
	Expect(result.render.dirtyChunks.issues.size() == 1, "render update failure should preserve dirty chunk diagnostics");
	Expect(!result.collision.updated, "render update failure should not publish later collision update");
}

void TestDuplicateAndOutOfBoundsChangedTilesArePreserved()
{
	const iggy::LevelRuntimeState initial = RuntimeLevel({ "..." });
	const iggy::LevelDerivedCacheState current = BuildDerivedCaches(initial, false, true);
	const iggy::LevelRuntimeState updated = RuntimeLevel({ "..#" });
	const std::vector<iggy::TileCoord> changedTiles { { 2, 0 }, { 2, 0 }, { -1, 4 }, { 99, 0 } };

	const iggy::LevelDerivedCacheUpdateResult result = iggy::LevelDerivedCacheUpdater {}.update(current, updated, changedTiles);

	Expect(result.updated, "duplicate and out-of-bounds changed tiles should still update derived cache");
	ExpectChangedTiles(result.changedTiles, changedTiles, "derived update should preserve duplicate/out-of-bounds changed tile diagnostics");
	ExpectChangedTiles(result.collision.changedTiles, changedTiles, "derived update should pass duplicate/out-of-bounds diagnostics to collision updater");
	Expect(result.state.collision.world.objects().size() == 1, "duplicate and out-of-bounds diagnostics should not affect rebuilt collision output");
}

void TestDerivedUpdateInputsAreNotMutated()
{
	const iggy::LevelTileRenderChunkCacheConfig renderConfig = RenderConfig(2, 1, 12);
	const iggy::LevelRuntimeState initial = RuntimeLevel({ ".#" });
	iggy::LevelDerivedCacheState current = BuildDerivedCaches(initial, true, true, renderConfig);
	const iggy::LevelDerivedCacheState currentBefore = current;
	iggy::LevelRuntimeState updated = RuntimeLevel({ "#." });
	updated.map.id = iggy::ResourceId("level:update");
	updated.npcAgents.push_back({ iggy::ResourceId("npc:one"), {} });
	const iggy::LevelRuntimeState levelBefore = updated;
	std::vector<iggy::TileCoord> changedTiles { { 0, 0 }, { 1, 0 } };
	const std::vector<iggy::TileCoord> changedTilesBefore = changedTiles;

	const iggy::LevelDerivedCacheUpdateResult result = iggy::LevelDerivedCacheUpdater {}.update(current, updated, changedTiles);

	Expect(result.updated, "derived cache immutability update should succeed");
	Expect(current.hasRenderCache == currentBefore.hasRenderCache, "derived cache update should not mutate current render flag");
	Expect(current.hasCollisionCache == currentBefore.hasCollisionCache, "derived cache update should not mutate current collision flag");
	Expect(current.render.tileChunks.chunks.size() == currentBefore.render.tileChunks.chunks.size(), "derived cache update should not mutate current render cache");
	ExpectCollisionObjectsMatch(current.collision.world, currentBefore.collision.world, "derived cache update should not mutate current collision cache");
	ExpectRuntimeLevelUnchanged(updated, levelBefore, "derived cache update should not mutate LevelRuntimeState");
	ExpectChangedTiles(changedTiles, changedTilesBefore, "derived cache update should not mutate changed tiles input");
}

} // namespace

int main()
{
	TestNoRequestedCachesBuildsEmptyDerivedState();
	TestCollisionOnlyBuildStoresCollisionCache();
	TestRenderOnlyBuildStoresRenderCache();
	TestBothRequestedCachesStoreBothOnSuccess();
	TestCollisionCacheMatchesBuilderForBlockedTiles();
	TestRenderCacheMatchesBuilderEnoughToProveDelegation();
	TestRenderFailurePreservesDiagnosticsAndPublishesNoPartialState();
	TestSuccessfulEmptyAndNonpositiveMapsFollowNestedBuilders();
	TestInputLevelRuntimeStateIsNotMutated();
	TestRuntimeAndLevelRuntimeDoNotReferenceDerivedCacheState();
	TestUpdateWithNoCachesPresentCopiesCurrentState();
	TestRenderOnlyUpdateDelegatesToRenderUpdater();
	TestCollisionOnlyUpdateDelegatesToCollisionUpdater();
	TestBothCachesPresentUpdateBothCaches();
	TestEmptyChangedTilesNoOpsThroughNestedUpdaters();
	TestRenderUpdateFailureFailsOverallWithoutPartialState();
	TestDuplicateAndOutOfBoundsChangedTilesArePreserved();
	TestDerivedUpdateInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
