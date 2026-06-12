#include <cstdlib>
#include <vector>

#include "scene/level/LevelGridQuery.hpp"
#include "scene/level/LevelMutationCacheUpdateStep.hpp"
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

iggy::LevelTileRenderChunkCacheConfig RenderConfig(int chunkWidth = 2, int chunkHeight = 1, int layer = 5)
{
	return { chunkWidth, chunkHeight, { { WalkableMaterial, BlockedMaterial }, layer } };
}

iggy::LevelTileEdit Edit(iggy::TileCoord tile, bool walkable)
{
	return { tile, walkable };
}

iggy::LevelDerivedCacheState BuildCaches(
	const iggy::LevelRuntimeState &level,
	bool render,
	bool collision,
	iggy::LevelTileRenderChunkCacheConfig renderConfig = RenderConfig())
{
	iggy::LevelDerivedCacheBuildConfig config;
	config.buildRenderCache = render;
	config.renderCacheConfig = renderConfig;
	config.buildCollisionCache = collision;
	const iggy::LevelDerivedCacheBuildResult build = iggy::LevelDerivedCacheBuilder {}.build(level, config);
	Expect(build.built, "derived cache fixture should build");
	return build.state;
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

void ExpectRuntimeLevelSame(const iggy::LevelRuntimeState &actual, const iggy::LevelRuntimeState &expected, const char *message)
{
	ExpectMapSame(actual.map, expected.map, message);
	Expect(actual.npcAgents.size() == expected.npcAgents.size(), message);
}

void ExpectCachesSame(
	const iggy::LevelDerivedCacheState &actual,
	const iggy::LevelDerivedCacheState &expected,
	const char *message)
{
	Expect(actual.hasRenderCache == expected.hasRenderCache, message);
	Expect(actual.hasCollisionCache == expected.hasCollisionCache, message);
	Expect(actual.render.tileChunks.chunks.size() == expected.render.tileChunks.chunks.size(), message);
	Expect(actual.collision.world.objects().size() == expected.collision.world.objects().size(), message);
	for (std::size_t index = 0; index < actual.collision.world.objects().size() && index < expected.collision.world.objects().size(); ++index)
		ExpectBounds(actual.collision.world.objects()[index].shape.bounds, expected.collision.world.objects()[index].shape.bounds, message);
}

void TestEmptyEditsReturnNoMutation()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({ ".#" });
	const iggy::LevelDerivedCacheState caches = BuildCaches(level, true, true);

	const iggy::LevelMutationCacheUpdateResult result = iggy::LevelMutationCacheUpdateStep {}.apply(level, caches, {});

	Expect(result.status == iggy::LevelMutationCacheUpdateStatus::NoMutation, "empty edits should return NoMutation");
	Expect(!result.mutation.mutated, "empty edits should preserve mutation no-op diagnostics");
	Expect(result.mutation.changedTiles.empty(), "empty edits should report no changed tiles");
	Expect(!result.cacheUpdate.updated, "empty edits should not call derived cache updater");
	ExpectRuntimeLevelSame(result.level, level, "empty edits should preserve level");
	ExpectCachesSame(result.derivedCaches, caches, "empty edits should preserve caches");
}

void TestNoOpEditReturnsNoMutation()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({ ".#" });
	const iggy::LevelDerivedCacheState caches = BuildCaches(level, false, true);

	const iggy::LevelMutationCacheUpdateResult result = iggy::LevelMutationCacheUpdateStep {}.apply(level, caches, { Edit({ 1, 0 }, false) });

	Expect(result.status == iggy::LevelMutationCacheUpdateStatus::NoMutation, "same-value edit should return NoMutation");
	Expect(!result.mutation.mutated, "same-value edit should not mutate map");
	Expect(result.mutation.issues.empty(), "same-value edit should not report mutation issue");
	Expect(!result.cacheUpdate.updated, "same-value edit should not call cache updater");
	ExpectRuntimeLevelSame(result.level, level, "same-value edit should preserve level");
	ExpectCachesSame(result.derivedCaches, caches, "same-value edit should preserve caches");
}

void TestValidEditWithNoCachesReturnsMutationOnly()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({ "." });
	const iggy::LevelDerivedCacheState caches;

	const iggy::LevelMutationCacheUpdateResult result = iggy::LevelMutationCacheUpdateStep {}.apply(level, caches, { Edit({ 0, 0 }, false) });

	Expect(result.status == iggy::LevelMutationCacheUpdateStatus::MutationOnly, "valid edit without caches should return MutationOnly");
	Expect(result.mutation.mutated, "valid edit without caches should mutate copied map");
	Expect(result.cacheUpdate.updated, "valid edit without caches should still use derived cache updater");
	Expect(result.level.map.tileAt(0, 0) != nullptr && !result.level.map.tileAt(0, 0)->walkable, "valid edit without caches should update level map");
	Expect(!result.derivedCaches.hasRenderCache, "valid edit without caches should leave render cache absent");
	Expect(!result.derivedCaches.hasCollisionCache, "valid edit without caches should leave collision cache absent");
}

void TestValidEditUpdatesCollisionCache()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({ "..." });
	const iggy::LevelDerivedCacheState caches = BuildCaches(level, false, true);

	const iggy::LevelMutationCacheUpdateResult result = iggy::LevelMutationCacheUpdateStep {}.apply(level, caches, { Edit({ 1, 0 }, false) });

	Expect(result.status == iggy::LevelMutationCacheUpdateStatus::Updated, "collision cache edit should return Updated");
	Expect(result.derivedCaches.hasCollisionCache, "collision cache edit should preserve collision cache presence");
	Expect(result.derivedCaches.collision.world.objects().size() == 1, "collision cache edit should add blocked tile collision object");
	if (result.derivedCaches.collision.world.objects().size() == 1)
		ExpectBounds(result.derivedCaches.collision.world.objects()[0].shape.bounds, iggy::tileBounds({ 1, 0 }), "collision cache edit should refresh blocked tile bounds");
}

void TestValidEditUpdatesRenderCache()
{
	const iggy::LevelTileRenderChunkCacheConfig renderConfig = RenderConfig();
	const iggy::LevelRuntimeState level = RuntimeLevel({ ".." });
	const iggy::LevelDerivedCacheState caches = BuildCaches(level, true, false, renderConfig);

	const iggy::LevelMutationCacheUpdateResult result = iggy::LevelMutationCacheUpdateStep {}.apply(level, caches, { Edit({ 1, 0 }, false) });

	Expect(result.status == iggy::LevelMutationCacheUpdateStatus::Updated, "render cache edit should return Updated");
	Expect(result.derivedCaches.hasRenderCache, "render cache edit should preserve render cache presence");
	Expect(result.cacheUpdate.render.updated, "render cache edit should preserve nested render update result");
	Expect(result.cacheUpdate.render.tileChunkUpdate.rebuiltChunks.size() == 1, "render cache edit should rebuild dirty render chunk");
	Expect(CommandsContainMaterial(result.derivedCaches.render, BlockedMaterial), "render cache edit should refresh blocked material command");
}

void TestValidEditUpdatesBothCaches()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({ ".." });
	const iggy::LevelDerivedCacheState caches = BuildCaches(level, true, true);

	const iggy::LevelMutationCacheUpdateResult result = iggy::LevelMutationCacheUpdateStep {}.apply(level, caches, { Edit({ 0, 0 }, false) });

	Expect(result.status == iggy::LevelMutationCacheUpdateStatus::Updated, "both-cache edit should return Updated");
	Expect(result.derivedCaches.hasRenderCache, "both-cache edit should preserve render cache");
	Expect(result.derivedCaches.hasCollisionCache, "both-cache edit should preserve collision cache");
	Expect(CommandsContainMaterial(result.derivedCaches.render, BlockedMaterial), "both-cache edit should refresh render cache");
	Expect(result.derivedCaches.collision.world.objects().size() == 1, "both-cache edit should refresh collision cache");
}

void TestInvalidOutOfBoundsEditDoesNotUpdateCaches()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({ "." });
	const iggy::LevelDerivedCacheState caches = BuildCaches(level, false, true);

	const iggy::LevelMutationCacheUpdateResult result = iggy::LevelMutationCacheUpdateStep {}.apply(level, caches, { Edit({ 9, 0 }, false) });

	Expect(result.status == iggy::LevelMutationCacheUpdateStatus::NoMutation, "invalid-only edit should return NoMutation");
	Expect(result.mutation.issues.size() == 1, "invalid-only edit should preserve mutation issue");
	if (result.mutation.issues.size() == 1)
		Expect(result.mutation.issues[0].code == iggy::LevelTileEditIssueCode::OutOfBounds, "invalid-only edit should report out-of-bounds issue");
	Expect(!result.cacheUpdate.updated, "invalid-only edit should not call cache updater");
	ExpectCachesSame(result.derivedCaches, caches, "invalid-only edit should preserve caches");
}

void TestMixedValidInvalidEditsUpdateFromValidChangedTiles()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({ "..." });
	const iggy::LevelDerivedCacheState caches = BuildCaches(level, false, true);
	const std::vector<iggy::LevelTileEdit> edits {
		Edit({ 0, 0 }, false),
		Edit({ 9, 0 }, false),
		Edit({ 2, 0 }, false),
	};

	const iggy::LevelMutationCacheUpdateResult result = iggy::LevelMutationCacheUpdateStep {}.apply(level, caches, edits);

	Expect(result.status == iggy::LevelMutationCacheUpdateStatus::Updated, "mixed valid/invalid edits should update when valid edits changed map");
	Expect(result.mutation.issues.size() == 1, "mixed valid/invalid edits should preserve invalid issue");
	ExpectChangedTiles(result.mutation.changedTiles, { { 0, 0 }, { 2, 0 } }, "mixed valid/invalid edits should report accepted changed tiles only");
	ExpectChangedTiles(result.cacheUpdate.changedTiles, result.mutation.changedTiles, "cache update should use mutation changed tiles");
	Expect(result.derivedCaches.collision.world.objects().size() == 2, "mixed valid/invalid edits should refresh cache from valid changes");
}

void TestCacheUpdateFailurePreservesCurrentCaches()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({ ".." });
	iggy::LevelDerivedCacheState caches;
	caches.hasRenderCache = true;
	caches.render.tileChunkConfig = RenderConfig(0, 1, 6);
	caches.hasCollisionCache = true;
	caches.collision = BuildCaches(level, false, true).collision;
	const iggy::LevelDerivedCacheState cachesBefore = caches;

	const iggy::LevelMutationCacheUpdateResult result = iggy::LevelMutationCacheUpdateStep {}.apply(level, caches, { Edit({ 1, 0 }, false) });

	Expect(result.status == iggy::LevelMutationCacheUpdateStatus::CacheUpdateFailed, "cache update failure should return CacheUpdateFailed");
	Expect(result.level.map.tileAt(1, 0) != nullptr && !result.level.map.tileAt(1, 0)->walkable, "cache update failure should still publish updated authoritative level");
	Expect(!result.cacheUpdate.updated, "cache update failure should preserve failed cache update result");
	Expect(!result.cacheUpdate.render.updated, "cache update failure should preserve render failure");
	Expect(result.cacheUpdate.render.dirtyChunks.issues.size() == 1, "cache update failure should preserve render diagnostics");
	ExpectCachesSame(result.derivedCaches, cachesBefore, "cache update failure should preserve current caches unchanged");
}

void TestInputsAreNotMutated()
{
	iggy::LevelRuntimeState level = RuntimeLevel({ ".#" });
	level.map.id = iggy::ResourceId("level:mutation-cache");
	level.npcAgents.push_back({ iggy::ResourceId("npc:one"), {} });
	const iggy::LevelRuntimeState levelBefore = level;
	iggy::LevelDerivedCacheState caches = BuildCaches(level, true, true);
	const iggy::LevelDerivedCacheState cachesBefore = caches;
	std::vector<iggy::LevelTileEdit> edits {
		Edit({ 0, 0 }, false),
		Edit({ 1, 0 }, true),
	};
	const std::vector<iggy::LevelTileEdit> editsBefore = edits;

	const iggy::LevelMutationCacheUpdateResult result = iggy::LevelMutationCacheUpdateStep {}.apply(level, caches, edits);

	Expect(result.status == iggy::LevelMutationCacheUpdateStatus::Updated, "immutability setup should update");
	ExpectRuntimeLevelSame(level, levelBefore, "mutation cache step should not mutate input level");
	ExpectCachesSame(caches, cachesBefore, "mutation cache step should not mutate input caches");
	Expect(edits.size() == editsBefore.size(), "mutation cache step should not mutate edit vector size");
	for (std::size_t index = 0; index < edits.size() && index < editsBefore.size(); ++index) {
		Expect(edits[index].tile == editsBefore[index].tile, "mutation cache step should not mutate edit tile");
		Expect(edits[index].walkable == editsBefore[index].walkable, "mutation cache step should not mutate edit walkability");
	}
}

} // namespace

int main()
{
	TestEmptyEditsReturnNoMutation();
	TestNoOpEditReturnsNoMutation();
	TestValidEditWithNoCachesReturnsMutationOnly();
	TestValidEditUpdatesCollisionCache();
	TestValidEditUpdatesRenderCache();
	TestValidEditUpdatesBothCaches();
	TestInvalidOutOfBoundsEditDoesNotUpdateCaches();
	TestMixedValidInvalidEditsUpdateFromValidChangedTiles();
	TestCacheUpdateFailurePreservesCurrentCaches();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
