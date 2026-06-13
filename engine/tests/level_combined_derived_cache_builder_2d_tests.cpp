#include <cstdlib>
#include <vector>

#include "scene/level/LevelCombinedDerivedCacheBuilder2D.hpp"
#include "scene/level/LevelCollisionCacheState.hpp"
#include "scene/level/LevelGridQuery.hpp"
#include "servers/physics2d/CollisionOverlap2D.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectBounds;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::LevelRuntimeState RuntimeLevel(std::initializer_list<std::string_view> rows)
{
	iggy::LevelRuntimeState level;
	level.map = MapFromRows(std::vector<std::string_view>(rows));
	return level;
}

iggy::LevelCollisionSourceBox2D SourceBox(
	const char *wallId,
	std::size_t wallIndex,
	iggy::Vec2 center,
	iggy::Vec2 size,
	float rotationRadians = 0.0F)
{
	return {
		Id(wallId),
		wallIndex,
		center,
		size,
		rotationRadians,
		Id("asset:wall"),
		Id("definition:wall"),
	};
}

iggy::LevelCollisionSource2D Source(std::vector<iggy::LevelCollisionSourceBox2D> boxes)
{
	return { boxes };
}

iggy::LevelCombinedDerivedCacheBuildConfig2D Config(bool buildRender = false, bool buildCollision = true)
{
	iggy::LevelCombinedDerivedCacheBuildConfig2D config;
	config.buildRenderCache = buildRender;
	config.buildCollisionCache = buildCollision;
	return config;
}

iggy::Aabb2 BoundsFor(iggy::Vec2 center, iggy::Vec2 size)
{
	const iggy::Vec2 half { size.x * 0.5F, size.y * 0.5F };
	return {
		{ center.x - half.x, center.y - half.y },
		{ center.x + half.x, center.y + half.y },
	};
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

bool SameSource(
	const iggy::LevelCollisionSource2D &actual,
	const iggy::LevelCollisionSource2D &expected)
{
	if (actual.boxes.size() != expected.boxes.size())
		return false;

	for (std::size_t index = 0; index < actual.boxes.size(); ++index) {
		const iggy::LevelCollisionSourceBox2D &left = actual.boxes[index];
		const iggy::LevelCollisionSourceBox2D &right = expected.boxes[index];
		if (left.sourceWallId != right.sourceWallId
			|| left.sourceWallIndex != right.sourceWallIndex
			|| !NearVec(left.center, right.center)
			|| !NearVec(left.size, right.size)
			|| left.rotationRadians != right.rotationRadians
			|| left.assetId != right.assetId
			|| left.definitionId != right.definitionId) {
			return false;
		}
	}

	return true;
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

void TestEmptySourceMatchesExistingTileOnlyCollisionBehavior()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({
		"#.#",
		".#.",
	});
	const iggy::LevelCollisionSource2D source;
	const iggy::LevelCollisionCacheBuildResult tileOnly = iggy::LevelCollisionCacheBuilder {}.build(level.map);

	const iggy::LevelCombinedDerivedCacheBuildResult2D result =
		iggy::LevelCombinedDerivedCacheBuilder2D {}.build(level, source, Config());

	Expect(tileOnly.built, "tile-only comparison cache should build");
	Expect(result.built, "combined derived cache with empty source should build");
	Expect(result.state.hasCollisionCache, "combined derived cache should publish collision cache");
	Expect(result.tileCollision.built, "combined result should preserve tile collision result");
	Expect(result.sourceCollision.built, "combined result should preserve empty source collision result");
	Expect(result.merge.built, "combined result should preserve merge result");
	ExpectCollisionObjectsMatch(result.state.collision.world, tileOnly.state.world, "empty source combined world should match existing tile-only collision behavior");
}

void TestSourceOnlyCollisionAppearsInFinalWorld()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({
		"...",
	});
	const iggy::LevelCollisionSource2D source = Source({
		SourceBox("source:wall", 7, { 4.0F, 1.0F }, { 2.0F, 2.0F }),
	});

	const iggy::LevelCombinedDerivedCacheBuildResult2D result =
		iggy::LevelCombinedDerivedCacheBuilder2D {}.build(level, source, Config());

	Expect(result.built, "source-only combined collision should build");
	Expect(result.state.collision.world.objects().size() == 1, "source-only combined collision should publish one source object");
	if (result.state.collision.world.objects().size() == 1) {
		Expect(result.state.collision.world.objects()[0].id == Id("source:wall"), "source-only collision object should preserve source id");
		ExpectBounds(result.state.collision.world.objects()[0].shape.bounds, BoundsFor({ 4.0F, 1.0F }, { 2.0F, 2.0F }), "source-only collision object should use source bounds");
	}

	const iggy::physics2d::CollisionOverlap2DResult overlap = iggy::physics2d::CollisionOverlap2D {}.queryAabb(
		result.state.collision.world,
		{ { 3.5F, 0.5F }, { 4.5F, 1.5F } });
	Expect(overlap.hits.size() == 1, "source-only collision should be queryable");
}

void TestTileAndSourceCollisionBothAppearInFinalWorld()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({
		".#.",
	});
	const iggy::LevelCollisionSource2D source = Source({
		SourceBox("source:wall", 0, { 5.0F, 0.0F }, { 2.0F, 2.0F }),
	});

	const iggy::LevelCombinedDerivedCacheBuildResult2D result =
		iggy::LevelCombinedDerivedCacheBuilder2D {}.build(level, source, Config());

	Expect(result.built, "tile plus source collision should build");
	Expect(result.state.collision.world.objects().size() == 2, "tile plus source should publish both objects");
	if (result.state.collision.world.objects().size() == 2) {
		Expect(result.state.collision.world.objects()[0].id.empty(), "tile collision object should remain first with empty generated id");
		ExpectBounds(result.state.collision.world.objects()[0].shape.bounds, iggy::tileBounds({ 1, 0 }), "first merged object should use tile bounds");
		Expect(result.state.collision.world.objects()[1].id == Id("source:wall"), "source collision object should follow tile objects");
		ExpectBounds(result.state.collision.world.objects()[1].shape.bounds, BoundsFor({ 5.0F, 0.0F }, { 2.0F, 2.0F }), "second merged object should use source bounds");
	}
}

void TestSourceWorldIssuesSurfaceWhileValidSourceBoxesStillMerge()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({
		"...",
	});
	const iggy::LevelCollisionSource2D source = Source({
		SourceBox("source:valid", 0, { -2.0F, 0.0F }, { 2.0F, 2.0F }),
		SourceBox("source:zero", 1, { 0.0F, 0.0F }, { 0.0F, 2.0F }),
		SourceBox("source:rotated", 2, { 2.0F, 0.0F }, { 2.0F, 2.0F }, 0.5F),
	});

	const iggy::LevelCombinedDerivedCacheBuildResult2D result =
		iggy::LevelCombinedDerivedCacheBuilder2D {}.build(level, source, Config());

	Expect(result.built, "source issues with valid subset should still build combined cache");
	Expect(result.sourceCollision.issues.size() == 2, "combined result should surface source-world issues");
	Expect(result.state.collision.world.objects().size() == 1, "combined cache should merge valid source subset");
	if (result.sourceCollision.issues.size() == 2) {
		Expect(result.sourceCollision.issues[0].code == iggy::LevelCollisionSourceWorldBuilder2DIssueCode::NonPositiveBoxSize, "first source issue should preserve non-positive size");
		Expect(result.sourceCollision.issues[1].code == iggy::LevelCollisionSourceWorldBuilder2DIssueCode::UnsupportedRotation, "second source issue should preserve unsupported rotation");
	}
	if (result.state.collision.world.objects().size() == 1)
		Expect(result.state.collision.world.objects()[0].id == Id("source:valid"), "combined cache should include valid source object despite source issues");
}

void TestDuplicateNonEmptyCollisionIdsFailDeterministically()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({
		"...",
	});
	const iggy::LevelCollisionSource2D source = Source({
		SourceBox("source:duplicate", 0, { 0.0F, 0.0F }, { 2.0F, 2.0F }),
		SourceBox("source:duplicate", 1, { 4.0F, 0.0F }, { 2.0F, 2.0F }),
	});

	const iggy::LevelCombinedDerivedCacheBuildResult2D result =
		iggy::LevelCombinedDerivedCacheBuilder2D {}.build(level, source, Config());

	Expect(!result.built, "duplicate source collision ids should fail combined cache build through merge");
	Expect(!result.state.hasCollisionCache, "duplicate source collision ids should publish no collision cache");
	Expect(result.sourceCollision.built, "duplicate source ids should still preserve source collision build");
	Expect(!result.merge.built, "duplicate source ids should preserve merge failure");
	Expect(result.merge.issues.size() == 1, "duplicate source ids should report one merge issue");
	if (result.merge.issues.size() == 1) {
		Expect(result.merge.issues[0].code == iggy::LevelCollisionWorldMerge2DIssueCode::DuplicateObjectId, "duplicate merge issue should use DuplicateObjectId");
		Expect(result.merge.issues[0].objectIndex == 1, "duplicate merge issue should preserve later duplicate index");
	}
}

void TestInputsAreNotMutated()
{
	iggy::LevelRuntimeState level = RuntimeLevel({
		".#",
		"..",
	});
	level.map.id = Id("level:test");
	level.npcAgents.push_back({ Id("npc:one"), {} });
	iggy::LevelCollisionSource2D source = Source({
		SourceBox("source:wall", 0, { 4.0F, 0.0F }, { 2.0F, 2.0F }),
	});
	const iggy::LevelRuntimeState beforeLevel = level;
	const iggy::LevelCollisionSource2D beforeSource = source;

	const iggy::LevelCombinedDerivedCacheBuildResult2D result =
		iggy::LevelCombinedDerivedCacheBuilder2D {}.build(level, source, Config());

	Expect(result.built, "combined derived cache immutability setup should build");
	ExpectRuntimeLevelUnchanged(level, beforeLevel, "combined derived cache builder should not mutate level runtime state");
	Expect(SameSource(source, beforeSource), "combined derived cache builder should not mutate collision source");
}

void TestExistingLevelDerivedCacheBuilderStillUsesTileOnlyCollision()
{
	const iggy::LevelRuntimeState level = RuntimeLevel({
		".#.",
	});
	const iggy::LevelDerivedCacheBuildConfig config { false, {}, true };

	const iggy::LevelDerivedCacheBuildResult result = iggy::LevelDerivedCacheBuilder {}.build(level, config);

	Expect(result.built, "existing derived cache builder should still build tile-only collision");
	Expect(result.state.hasCollisionCache, "existing derived cache builder should still publish collision cache");
	Expect(result.state.collision.world.objects().size() == 1, "existing derived cache builder should not include source collision");
	if (result.state.collision.world.objects().size() == 1)
		ExpectBounds(result.state.collision.world.objects()[0].shape.bounds, iggy::tileBounds({ 1, 0 }), "existing derived cache collision should still use tile bounds");
}

} // namespace

int main()
{
	TestEmptySourceMatchesExistingTileOnlyCollisionBehavior();
	TestSourceOnlyCollisionAppearsInFinalWorld();
	TestTileAndSourceCollisionBothAppearInFinalWorld();
	TestSourceWorldIssuesSurfaceWhileValidSourceBoxesStillMerge();
	TestDuplicateNonEmptyCollisionIdsFailDeterministically();
	TestInputsAreNotMutated();
	TestExistingLevelDerivedCacheBuilderStillUsesTileOnlyCollision();

	return Failures;
}
