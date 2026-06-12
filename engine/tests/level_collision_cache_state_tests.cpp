#include <cstdlib>
#include <vector>

#include "scene/level/LevelCollisionCacheState.hpp"
#include "scene/level/LevelCollisionWorldBuilder.hpp"
#include "scene/level/LevelGridQuery.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectBounds;
using iggy::test::Failures;
using iggy::test::MapFromRows;

void ExpectObject(
	const iggy::physics2d::CollisionObject2D &object,
	iggy::TileCoord tile,
	const char *message)
{
	Expect(object.id.empty(), message);
	Expect(object.solid, message);
	Expect(object.shape.type == iggy::physics2d::CollisionShape2DType::Aabb, message);
	ExpectBounds(object.shape.bounds, iggy::tileBounds(tile), message);
}

void ExpectObjectsMatch(
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

void ExpectMapUnchanged(const iggy::LevelTileMap &actual, const iggy::LevelTileMap &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.width == expected.width && actual.height == expected.height, message);
	Expect(actual.tiles.size() == expected.tiles.size(), message);
	for (std::size_t index = 0; index < actual.tiles.size() && index < expected.tiles.size(); ++index)
		Expect(actual.tiles[index].walkable == expected.tiles[index].walkable, message);
	Expect(actual.entitySpawns.size() == expected.entitySpawns.size(), message);
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

iggy::LevelCollisionCacheState CacheFrom(const iggy::LevelTileMap &map)
{
	const iggy::LevelCollisionCacheBuildResult build = iggy::LevelCollisionCacheBuilder {}.build(map);
	Expect(build.built, "cache fixture should build");
	return build.state;
}

void TestEmptyMapBuildsEmptyCollisionCache()
{
	const iggy::LevelTileMap map;

	const iggy::LevelCollisionCacheBuildResult result = iggy::LevelCollisionCacheBuilder {}.build(map);

	Expect(result.built, "empty map should build collision cache");
	Expect(result.state.world.objects().empty(), "empty map collision cache should store empty world");
	Expect(result.collisionWorld.built, "empty map should preserve nested collision world success");
	Expect(result.collisionWorld.world.objects().empty(), "empty map nested collision world should be empty");
	Expect(result.collisionWorld.issues.empty(), "empty map should preserve no collision issues");
}

void TestNonpositiveMapBuildsEmptyCollisionCache()
{
	iggy::LevelTileMap map;
	map.width = -2;
	map.height = 3;
	map.tiles.push_back({ false });

	const iggy::LevelCollisionCacheBuildResult result = iggy::LevelCollisionCacheBuilder {}.build(map);

	Expect(result.built, "nonpositive map should build collision cache");
	Expect(result.state.world.objects().empty(), "nonpositive map collision cache should store empty world");
	Expect(result.collisionWorld.built, "nonpositive map should preserve nested collision world success");
	Expect(result.collisionWorld.issues.empty(), "nonpositive map should preserve no issues");
}

void TestAllWalkableMapBuildsEmptyCollisionCache()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});

	const iggy::LevelCollisionCacheBuildResult result = iggy::LevelCollisionCacheBuilder {}.build(map);

	Expect(result.built, "all-walkable map should build collision cache");
	Expect(result.state.world.objects().empty(), "walkable tiles should not create cached collision objects");
	Expect(result.collisionWorld.built, "all-walkable map should preserve nested collision world success");
	Expect(result.collisionWorld.issues.empty(), "all-walkable map should expose no nested issues");
}

void TestBlockedTileMapMatchesCollisionWorldBuilder()
{
	const iggy::LevelTileMap map = MapFromRows({
		".#.",
	});
	const iggy::LevelCollisionWorldBuildResult worldBuild = iggy::LevelCollisionWorldBuilder {}.build(map);

	const iggy::LevelCollisionCacheBuildResult result = iggy::LevelCollisionCacheBuilder {}.build(map);

	Expect(worldBuild.built, "blocked tile expected world fixture should build");
	Expect(result.built, "blocked tile map should build collision cache");
	ExpectObjectsMatch(result.state.world, worldBuild.world, "collision cache state should store world from LevelCollisionWorldBuilder");
	ExpectObjectsMatch(result.collisionWorld.world, worldBuild.world, "collision cache result should preserve nested world build result");
	Expect(result.collisionWorld.issues.empty(), "blocked tile map should expose no nested issues");
	if (result.state.world.objects().size() == 1)
		ExpectObject(result.state.world.objects()[0], { 1, 0 }, "blocked tile cache object should use tile bounds");
}

void TestMixedMapPreservesBlockedTileRowMajorOrder()
{
	const iggy::LevelTileMap map = MapFromRows({
		"#.#",
		".#.",
	});

	const iggy::LevelCollisionCacheBuildResult result = iggy::LevelCollisionCacheBuilder {}.build(map);

	Expect(result.built, "mixed blocked map should build collision cache");
	Expect(result.state.world.objects().size() == 3, "mixed blocked map should cache three blocked tile objects");
	if (result.state.world.objects().size() == 3) {
		ExpectObject(result.state.world.objects()[0], { 0, 0 }, "first cached collision object should be row-major tile 0,0");
		ExpectObject(result.state.world.objects()[1], { 2, 0 }, "second cached collision object should be row-major tile 2,0");
		ExpectObject(result.state.world.objects()[2], { 1, 1 }, "third cached collision object should be row-major tile 1,1");
	}
}

void TestSparseMissingTileStorageIsSkipped()
{
	iggy::LevelTileMap map;
	map.width = 3;
	map.height = 1;
	map.tiles.push_back({ false });

	const iggy::LevelCollisionCacheBuildResult result = iggy::LevelCollisionCacheBuilder {}.build(map);

	Expect(result.built, "sparse map should build collision cache");
	Expect(result.state.world.objects().size() == 1, "sparse map should cache only present blocked tile data");
	if (result.state.world.objects().size() == 1)
		ExpectObject(result.state.world.objects()[0], { 0, 0 }, "sparse present blocked tile should create cached collision object");
	Expect(result.collisionWorld.issues.empty(), "sparse map should expose no nested issues");
}

void TestNestedBuildResultFieldsArePreservedOnSuccess()
{
	const iggy::LevelTileMap map = MapFromRows({
		"##",
	});

	const iggy::LevelCollisionCacheBuildResult result = iggy::LevelCollisionCacheBuilder {}.build(map);

	Expect(result.built, "nested result preservation setup should build");
	Expect(result.collisionWorld.built, "cache build should preserve nested built flag");
	Expect(result.collisionWorld.issues.empty(), "cache build should preserve nested issue list");
	ExpectObjectsMatch(result.state.world, result.collisionWorld.world, "cache state world should match nested collision world output");
}

void TestInputMapIsNotMutated()
{
	iggy::LevelTileMap map = MapFromRows({
		".#.",
		"#..",
	});
	map.id = iggy::ResourceId("level:test");
	const iggy::LevelTileMap before = map;

	const iggy::LevelCollisionCacheBuildResult result = iggy::LevelCollisionCacheBuilder {}.build(map);

	Expect(result.built, "non-mutating collision cache build should succeed");
	ExpectMapUnchanged(map, before, "collision cache build should not mutate tile map");
}

void TestEmptyChangedTilesCopiesCurrentCache()
{
	const iggy::LevelTileMap map = MapFromRows({
		".#.",
		"#..",
	});
	const iggy::LevelCollisionCacheState current = CacheFrom(map);

	const iggy::LevelCollisionCacheUpdateResult result = iggy::LevelCollisionCacheUpdater {}.update(current, map, {});

	Expect(result.updated, "empty changed tiles should update as no-op");
	ExpectObjectsMatch(result.state.world, current.world, "empty changed tiles should copy current cache state");
	Expect(!result.rebuild.built, "empty changed tiles should not rebuild collision cache");
	Expect(result.changedTiles.empty(), "empty changed tiles should preserve empty diagnostics");
}

void TestChangedTilesRebuildFromCurrentMap()
{
	const iggy::LevelTileMap initial = MapFromRows({
		"...",
	});
	const iggy::LevelCollisionCacheState current = CacheFrom(initial);
	const iggy::LevelTileMap updated = MapFromRows({
		".#.",
	});
	const std::vector<iggy::TileCoord> changedTiles { { 1, 0 } };

	const iggy::LevelCollisionCacheUpdateResult result = iggy::LevelCollisionCacheUpdater {}.update(current, updated, changedTiles);

	Expect(result.updated, "changed tiles should rebuild collision cache");
	Expect(result.rebuild.built, "changed tiles should preserve rebuild success");
	ExpectChangedTiles(result.changedTiles, changedTiles, "changed tiles should be copied for diagnostics");
	Expect(result.state.world.objects().size() == 1, "rebuilt collision cache should include newly blocked tile");
	if (result.state.world.objects().size() == 1)
		ExpectObject(result.state.world.objects()[0], { 1, 0 }, "rebuilt collision cache should use current map blocked tile");
}

void TestRebuildRemovesFormerlyBlockedTile()
{
	const iggy::LevelTileMap initial = MapFromRows({
		".#.",
	});
	const iggy::LevelCollisionCacheState current = CacheFrom(initial);
	const iggy::LevelTileMap updated = MapFromRows({
		"...",
	});

	const iggy::LevelCollisionCacheUpdateResult result = iggy::LevelCollisionCacheUpdater {}.update(current, updated, { { 1, 0 } });

	Expect(result.updated, "walkable changed tile should rebuild collision cache");
	Expect(result.state.world.objects().empty(), "formerly blocked tile should be removed from rebuilt collision cache");
}

void TestRebuildAddsFormerlyWalkableTile()
{
	const iggy::LevelTileMap initial = MapFromRows({
		"...",
	});
	const iggy::LevelCollisionCacheState current = CacheFrom(initial);
	const iggy::LevelTileMap updated = MapFromRows({
		"#..",
	});

	const iggy::LevelCollisionCacheUpdateResult result = iggy::LevelCollisionCacheUpdater {}.update(current, updated, { { 0, 0 } });

	Expect(result.updated, "blocked changed tile should rebuild collision cache");
	Expect(result.state.world.objects().size() == 1, "formerly walkable tile should be added to rebuilt collision cache");
	if (result.state.world.objects().size() == 1)
		ExpectObject(result.state.world.objects()[0], { 0, 0 }, "new blocked tile should use tile bounds");
}

void TestDuplicateChangedTilesArePreserved()
{
	const iggy::LevelTileMap initial = MapFromRows({
		"...",
	});
	const iggy::LevelCollisionCacheState current = CacheFrom(initial);
	const iggy::LevelTileMap updated = MapFromRows({
		".#.",
	});
	const std::vector<iggy::TileCoord> changedTiles { { 1, 0 }, { 1, 0 }, { 1, 0 } };

	const iggy::LevelCollisionCacheUpdateResult result = iggy::LevelCollisionCacheUpdater {}.update(current, updated, changedTiles);

	Expect(result.updated, "duplicate changed tiles should still rebuild collision cache");
	ExpectChangedTiles(result.changedTiles, changedTiles, "duplicate changed tiles should be preserved for diagnostics");
	Expect(result.state.world.objects().size() == 1, "duplicate changed tiles should not duplicate rebuilt collision objects");
}

void TestOutOfBoundsChangedTilesArePreserved()
{
	const iggy::LevelTileMap initial = MapFromRows({
		"...",
	});
	const iggy::LevelCollisionCacheState current = CacheFrom(initial);
	const iggy::LevelTileMap updated = MapFromRows({
		"..#",
	});
	const std::vector<iggy::TileCoord> changedTiles { { -1, 0 }, { 99, 42 } };

	const iggy::LevelCollisionCacheUpdateResult result = iggy::LevelCollisionCacheUpdater {}.update(current, updated, changedTiles);

	Expect(result.updated, "out-of-bounds changed tiles should not prevent full collision cache rebuild");
	ExpectChangedTiles(result.changedTiles, changedTiles, "out-of-bounds changed tiles should be preserved for diagnostics");
	Expect(result.state.world.objects().size() == 1, "out-of-bounds diagnostics should not affect rebuilt output from current map");
	if (result.state.world.objects().size() == 1)
		ExpectObject(result.state.world.objects()[0], { 2, 0 }, "full rebuild should still use blocked tile from current map");
}

void TestUpdateInputsAreNotMutated()
{
	iggy::LevelTileMap initial = MapFromRows({
		".#.",
	});
	iggy::LevelCollisionCacheState current = CacheFrom(initial);
	const iggy::LevelCollisionCacheState currentBefore = current;
	iggy::LevelTileMap updated = MapFromRows({
		"#..",
	});
	updated.id = iggy::ResourceId("level:update");
	const iggy::LevelTileMap mapBefore = updated;
	std::vector<iggy::TileCoord> changedTiles { { 0, 0 }, { 1, 0 } };
	const std::vector<iggy::TileCoord> changedTilesBefore = changedTiles;

	const iggy::LevelCollisionCacheUpdateResult result = iggy::LevelCollisionCacheUpdater {}.update(current, updated, changedTiles);

	Expect(result.updated, "non-mutating collision cache update should succeed");
	ExpectObjectsMatch(current.world, currentBefore.world, "collision cache update should not mutate current cache");
	ExpectMapUnchanged(updated, mapBefore, "collision cache update should not mutate tile map");
	ExpectChangedTiles(changedTiles, changedTilesBefore, "collision cache update should not mutate changed tile diagnostics input");
}

} // namespace

int main()
{
	TestEmptyMapBuildsEmptyCollisionCache();
	TestNonpositiveMapBuildsEmptyCollisionCache();
	TestAllWalkableMapBuildsEmptyCollisionCache();
	TestBlockedTileMapMatchesCollisionWorldBuilder();
	TestMixedMapPreservesBlockedTileRowMajorOrder();
	TestSparseMissingTileStorageIsSkipped();
	TestNestedBuildResultFieldsArePreservedOnSuccess();
	TestInputMapIsNotMutated();
	TestEmptyChangedTilesCopiesCurrentCache();
	TestChangedTilesRebuildFromCurrentMap();
	TestRebuildRemovesFormerlyBlockedTile();
	TestRebuildAddsFormerlyWalkableTile();
	TestDuplicateChangedTilesArePreserved();
	TestOutOfBoundsChangedTilesArePreserved();
	TestUpdateInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
