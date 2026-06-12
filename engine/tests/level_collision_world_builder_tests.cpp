#include <cstdlib>
#include <vector>

#include "scene/level/LevelCollisionWorldBuilder.hpp"
#include "scene/level/LevelGridQuery.hpp"
#include "servers/physics2d/CharacterMove2D.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectBounds;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;

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

void ExpectMapUnchanged(const iggy::LevelTileMap &actual, const iggy::LevelTileMap &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.width == expected.width && actual.height == expected.height, message);
	Expect(actual.tiles.size() == expected.tiles.size(), message);
	for (std::size_t index = 0; index < actual.tiles.size() && index < expected.tiles.size(); ++index)
		Expect(actual.tiles[index].walkable == expected.tiles[index].walkable, message);
	Expect(actual.entitySpawns.size() == expected.entitySpawns.size(), message);
}

void TestEmptyMapBuildsEmptyWorld()
{
	const iggy::LevelTileMap map;

	const iggy::LevelCollisionWorldBuildResult result = iggy::LevelCollisionWorldBuilder {}.build(map);

	Expect(result.built, "empty level tile map should build an empty collision world");
	Expect(result.world.objects().empty(), "empty level tile map should produce no collision objects");
	Expect(result.issues.empty(), "empty level tile map should produce no collision issues");
}

void TestNonpositiveDimensionsBuildEmptyWorld()
{
	iggy::LevelTileMap map;
	map.width = -2;
	map.height = 3;
	map.tiles.push_back({ false });

	const iggy::LevelCollisionWorldBuildResult result = iggy::LevelCollisionWorldBuilder {}.build(map);

	Expect(result.built, "nonpositive map dimensions should build successfully");
	Expect(result.world.objects().empty(), "nonpositive map dimensions should produce no collision objects");
	Expect(result.issues.empty(), "nonpositive map dimensions should produce no collision issues");
}

void TestAllWalkableMapBuildsEmptyWorld()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});

	const iggy::LevelCollisionWorldBuildResult result = iggy::LevelCollisionWorldBuilder {}.build(map);

	Expect(result.built, "all-walkable map should build successfully");
	Expect(result.world.objects().empty(), "walkable tiles should not create collision objects");
	Expect(result.issues.empty(), "all-walkable map should produce no issues");
}

void TestSingleBlockedTileEmitsSolidAabbObject()
{
	const iggy::LevelTileMap map = MapFromRows({
		".#.",
	});

	const iggy::LevelCollisionWorldBuildResult result = iggy::LevelCollisionWorldBuilder {}.build(map);

	Expect(result.built, "single blocked tile map should build");
	Expect(result.world.objects().size() == 1, "single blocked tile should create one collision object");
	if (result.world.objects().size() == 1)
		ExpectObject(result.world.objects()[0], { 1, 0 }, "blocked tile object should preserve empty id, solid flag, and tile bounds");
}

void TestMultipleBlockedTilesPreserveRowMajorOrder()
{
	const iggy::LevelTileMap map = MapFromRows({
		"#.#",
		".#.",
	});

	const iggy::LevelCollisionWorldBuildResult result = iggy::LevelCollisionWorldBuilder {}.build(map);

	Expect(result.built, "multiple blocked tile map should build");
	Expect(result.world.objects().size() == 3, "three blocked tiles should create three collision objects");
	if (result.world.objects().size() == 3) {
		ExpectObject(result.world.objects()[0], { 0, 0 }, "first blocked object should be row-major tile 0,0");
		ExpectObject(result.world.objects()[1], { 2, 0 }, "second blocked object should be row-major tile 2,0");
		ExpectObject(result.world.objects()[2], { 1, 1 }, "third blocked object should be row-major tile 1,1");
	}
}

void TestMixedWalkableBlockedIncludesOnlyBlockedTiles()
{
	const iggy::LevelTileMap map = MapFromRows({
		".#.",
		"#..",
	});

	const iggy::LevelCollisionWorldBuildResult result = iggy::LevelCollisionWorldBuilder {}.build(map);

	Expect(result.built, "mixed map should build");
	Expect(result.world.objects().size() == 2, "mixed map should include only blocked tiles");
	if (result.world.objects().size() == 2) {
		ExpectObject(result.world.objects()[0], { 1, 0 }, "first mixed blocked tile should be included");
		ExpectObject(result.world.objects()[1], { 0, 1 }, "second mixed blocked tile should be included");
	}
}

void TestSparseMissingTileStorageIsSkipped()
{
	iggy::LevelTileMap map;
	map.width = 3;
	map.height = 1;
	map.tiles.push_back({ false });

	const iggy::LevelCollisionWorldBuildResult result = iggy::LevelCollisionWorldBuilder {}.build(map);

	Expect(result.built, "sparse tile map should build");
	Expect(result.world.objects().size() == 1, "sparse tile map should include only present blocked tile storage");
	if (result.world.objects().size() == 1)
		ExpectObject(result.world.objects()[0], { 0, 0 }, "sparse present blocked tile should create collision object");
}

void TestObjectIdsAreEmptyAndDeterministic()
{
	const iggy::LevelTileMap map = MapFromRows({
		"##",
	});

	const iggy::LevelCollisionWorldBuildResult result = iggy::LevelCollisionWorldBuilder {}.build(map);

	Expect(result.built, "blocked tile id test map should build");
	Expect(result.world.objects().size() == 2, "blocked tile id test map should create two objects");
	if (result.world.objects().size() == 2)
		Expect(result.world.objects()[0].id.empty() && result.world.objects()[1].id.empty(), "blocked tile collision object ids should remain empty");
}

void TestInputMapIsNotMutated()
{
	iggy::LevelTileMap map = MapFromRows({
		".#.",
		"#..",
	});
	map.id = iggy::ResourceId("level:test");
	const iggy::LevelTileMap before = map;

	const iggy::LevelCollisionWorldBuildResult result = iggy::LevelCollisionWorldBuilder {}.build(map);

	Expect(result.built, "non-mutating collision world build should succeed");
	ExpectMapUnchanged(map, before, "collision world build should not mutate tile map");
}

void TestCollisionWorldConstrainsCharacterMovement()
{
	const iggy::LevelTileMap map = MapFromRows({
		"..#",
	});
	const iggy::LevelCollisionWorldBuildResult world = iggy::LevelCollisionWorldBuilder {}.build(map);

	const iggy::physics2d::CharacterMove2DResult move = iggy::physics2d::CharacterMove2D {}.move(
		world.world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		{ 4.0F, 0.0F });

	Expect(world.built, "manual composition should build collision world");
	Expect(move.status == iggy::physics2d::CharacterMove2DStatus::Blocked, "blocked level tile should constrain character movement");
	Expect(NearVec(move.allowedDelta, { 1.0F, 0.0F }), "blocked level tile should limit allowed movement before tile bounds");
	ExpectBounds(move.motion.hit.objectBounds, iggy::tileBounds({ 2, 0 }), "manual composition hit should use blocked tile bounds");
}

} // namespace

int main()
{
	TestEmptyMapBuildsEmptyWorld();
	TestNonpositiveDimensionsBuildEmptyWorld();
	TestAllWalkableMapBuildsEmptyWorld();
	TestSingleBlockedTileEmitsSolidAabbObject();
	TestMultipleBlockedTilesPreserveRowMajorOrder();
	TestMixedWalkableBlockedIncludesOnlyBlockedTiles();
	TestSparseMissingTileStorageIsSkipped();
	TestObjectIdsAreEmptyAndDeterministic();
	TestInputMapIsNotMutated();
	TestCollisionWorldConstrainsCharacterMovement();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
