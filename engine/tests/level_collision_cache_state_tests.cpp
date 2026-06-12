#include <cstdlib>

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

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
