#include <cstdlib>
#include <vector>

#include "scene/level/LevelCollisionWorldMerge2D.hpp"
#include "servers/physics2d/CollisionOverlap2D.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectBounds;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::physics2d::CollisionObject2D Object(
	iggy::ResourceId id,
	iggy::Aabb2 bounds,
	bool solid = true)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), solid };
}

iggy::physics2d::CollisionWorld2D BuiltWorld(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult build = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(build.built, "test fixture collision world should build");
	return build.world;
}

void ExpectObject(
	const iggy::physics2d::CollisionObject2D &actual,
	const iggy::physics2d::CollisionObject2D &expected,
	const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.shape.type == expected.shape.type, message);
	ExpectBounds(actual.shape.bounds, expected.shape.bounds, message);
	Expect(actual.solid == expected.solid, message);
}

bool SameObjects(
	const std::vector<iggy::physics2d::CollisionObject2D> &actual,
	const std::vector<iggy::physics2d::CollisionObject2D> &expected)
{
	if (actual.size() != expected.size())
		return false;

	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index].id != expected[index].id
			|| actual[index].shape.type != expected[index].shape.type
			|| !iggy::test::SameBounds(actual[index].shape.bounds, expected[index].shape.bounds)
			|| actual[index].solid != expected[index].solid) {
			return false;
		}
	}

	return true;
}

void TestEmptyWorldsMergeToEmptyWorld()
{
	const iggy::physics2d::CollisionWorld2D primary;
	const iggy::physics2d::CollisionWorld2D secondary;

	const iggy::LevelCollisionWorldMerge2DResult result =
		iggy::LevelCollisionWorldMerge2D {}.merge(primary, secondary);

	Expect(result.built, "empty collision worlds should merge successfully");
	Expect(result.world.objects().empty(), "empty collision worlds should produce empty merged world");
	Expect(result.issues.empty(), "empty collision worlds should produce no merge issues");
	Expect(result.primaryObjectCount == 0, "empty primary count should be preserved");
	Expect(result.secondaryObjectCount == 0, "empty secondary count should be preserved");
	Expect(result.mergedObjectCount == 0, "empty merged count should be preserved");
}

void TestPrimaryOnlyObjectsAppearInOutput()
{
	const std::vector<iggy::physics2d::CollisionObject2D> primaryObjects {
		Object(Id("wall:primary_a"), { { 0.0F, 0.0F }, { 1.0F, 1.0F } }),
		Object(Id("wall:primary_b"), { { 2.0F, 0.0F }, { 3.0F, 1.0F } }, false),
	};
	const iggy::physics2d::CollisionWorld2D primary = BuiltWorld(primaryObjects);
	const iggy::physics2d::CollisionWorld2D secondary;

	const iggy::LevelCollisionWorldMerge2DResult result =
		iggy::LevelCollisionWorldMerge2D {}.merge(primary, secondary);

	Expect(result.built, "primary-only collision world should merge");
	Expect(result.world.objects().size() == primaryObjects.size(), "primary-only merge should preserve primary objects");
	for (std::size_t index = 0; index < result.world.objects().size() && index < primaryObjects.size(); ++index)
		ExpectObject(result.world.objects()[index], primaryObjects[index], "primary-only merge should preserve object fields and order");
}

void TestSecondaryOnlyObjectsAppearInOutput()
{
	const iggy::physics2d::CollisionWorld2D primary;
	const std::vector<iggy::physics2d::CollisionObject2D> secondaryObjects {
		Object(Id("wall:secondary"), { { -2.0F, -1.0F }, { -1.0F, 0.0F } }),
	};
	const iggy::physics2d::CollisionWorld2D secondary = BuiltWorld(secondaryObjects);

	const iggy::LevelCollisionWorldMerge2DResult result =
		iggy::LevelCollisionWorldMerge2D {}.merge(primary, secondary);

	Expect(result.built, "secondary-only collision world should merge");
	Expect(result.world.objects().size() == secondaryObjects.size(), "secondary-only merge should preserve secondary objects");
	if (result.world.objects().size() == 1)
		ExpectObject(result.world.objects()[0], secondaryObjects[0], "secondary-only merge should preserve object fields");
}

void TestBothWorldsPreservePrimaryThenSecondaryOrderAndCollisionBehavior()
{
	const std::vector<iggy::physics2d::CollisionObject2D> primaryObjects {
		Object(Id("wall:primary"), { { 0.0F, 0.0F }, { 2.0F, 2.0F } }),
	};
	const std::vector<iggy::physics2d::CollisionObject2D> secondaryObjects {
		Object(Id("wall:secondary_a"), { { 4.0F, 0.0F }, { 6.0F, 2.0F } }),
		Object(Id("wall:secondary_b"), { { 8.0F, 0.0F }, { 10.0F, 2.0F } }),
	};
	const iggy::physics2d::CollisionWorld2D primary = BuiltWorld(primaryObjects);
	const iggy::physics2d::CollisionWorld2D secondary = BuiltWorld(secondaryObjects);

	const iggy::LevelCollisionWorldMerge2DResult result =
		iggy::LevelCollisionWorldMerge2D {}.merge(primary, secondary);

	Expect(result.built, "primary and secondary worlds should merge");
	Expect(result.primaryObjectCount == 1, "merge should preserve primary object count");
	Expect(result.secondaryObjectCount == 2, "merge should preserve secondary object count");
	Expect(result.mergedObjectCount == 3, "merge should preserve merged object count");
	Expect(result.world.objects().size() == 3, "merged world should include all objects");
	if (result.world.objects().size() == 3) {
		ExpectObject(result.world.objects()[0], primaryObjects[0], "first merged object should be primary object");
		ExpectObject(result.world.objects()[1], secondaryObjects[0], "second merged object should be first secondary object");
		ExpectObject(result.world.objects()[2], secondaryObjects[1], "third merged object should be second secondary object");
	}

	const iggy::physics2d::CollisionOverlap2DResult primaryHit = iggy::physics2d::CollisionOverlap2D {}.queryAabb(
		result.world,
		{ { 1.0F, 1.0F }, { 1.5F, 1.5F } });
	const iggy::physics2d::CollisionOverlap2DResult secondaryHit = iggy::physics2d::CollisionOverlap2D {}.queryAabb(
		result.world,
		{ { 8.5F, 0.5F }, { 9.5F, 1.5F } });

	Expect(primaryHit.hits.size() == 1, "merged world should collide with primary object");
	Expect(secondaryHit.hits.size() == 1, "merged world should collide with secondary object");
	if (primaryHit.hits.size() == 1)
		Expect(primaryHit.hits[0].object.id == Id("wall:primary"), "primary hit should preserve object id");
	if (secondaryHit.hits.size() == 1)
		Expect(secondaryHit.hits[0].object.id == Id("wall:secondary_b"), "secondary hit should preserve object id");
}

void TestDuplicateNonEmptyIdsFailWithoutMergedWorld()
{
	const iggy::physics2d::CollisionWorld2D primary = BuiltWorld({
		Object(Id("wall:duplicate"), { { 0.0F, 0.0F }, { 1.0F, 1.0F } }),
	});
	const iggy::physics2d::CollisionWorld2D secondary = BuiltWorld({
		Object(Id("wall:duplicate"), { { 2.0F, 0.0F }, { 3.0F, 1.0F } }),
	});

	const iggy::LevelCollisionWorldMerge2DResult result =
		iggy::LevelCollisionWorldMerge2D {}.merge(primary, secondary);

	Expect(!result.built, "duplicate non-empty ids should fail merge");
	Expect(result.world.objects().empty(), "duplicate ids should not publish merged world");
	Expect(result.issues.size() == 1, "duplicate ids should report one issue for later duplicate");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::LevelCollisionWorldMerge2DIssueCode::DuplicateObjectId, "duplicate issue should use DuplicateObjectId");
		Expect(result.issues[0].objectIndex == 1, "duplicate issue should preserve merged object index");
		Expect(result.issues[0].object.id == Id("wall:duplicate"), "duplicate issue should preserve offending object");
	}
}

void TestEmptyIdsAreAllowedAndPreserved()
{
	const iggy::physics2d::CollisionWorld2D primary = BuiltWorld({
		Object({}, { { 0.0F, 0.0F }, { 1.0F, 1.0F } }),
	});
	const iggy::physics2d::CollisionWorld2D secondary = BuiltWorld({
		Object({}, { { 2.0F, 0.0F }, { 3.0F, 1.0F } }),
	});

	const iggy::LevelCollisionWorldMerge2DResult result =
		iggy::LevelCollisionWorldMerge2D {}.merge(primary, secondary);

	Expect(result.built, "empty ids should remain allowed for generated tile-style collision objects");
	Expect(result.world.objects().size() == 2, "empty id objects should both be preserved");
	Expect(result.issues.empty(), "empty ids should not produce duplicate issues");
}

void TestInvalidDirectWorldObjectMapsToWorldBuildFailed()
{
	const iggy::physics2d::CollisionWorld2D primary({
		Object(Id("wall:valid"), { { 0.0F, 0.0F }, { 1.0F, 1.0F } }),
	});
	const iggy::physics2d::CollisionWorld2D secondary({
		{ Id("wall:invalid"), iggy::physics2d::makeAabbShape({ { 3.0F, 0.0F }, { 2.0F, 1.0F } }), true },
	});

	const iggy::LevelCollisionWorldMerge2DResult result =
		iggy::LevelCollisionWorldMerge2D {}.merge(primary, secondary);

	Expect(!result.built, "invalid direct-world object should fail merged world build");
	Expect(result.world.objects().empty(), "world build failure should not publish merged world");
	Expect(result.issues.size() == 1, "world build failure should report issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::LevelCollisionWorldMerge2DIssueCode::WorldBuildFailed, "invalid direct object should map to WorldBuildFailed");
		Expect(result.issues[0].objectIndex == 1, "world build failure should preserve merged object index");
		Expect(result.issues[0].object.id == Id("wall:invalid"), "world build failure should preserve offending object");
	}
}

void TestInputsAreNotMutated()
{
	const std::vector<iggy::physics2d::CollisionObject2D> primaryObjects {
		Object(Id("wall:primary"), { { 0.0F, 0.0F }, { 1.0F, 1.0F } }),
	};
	const std::vector<iggy::physics2d::CollisionObject2D> secondaryObjects {
		Object(Id("wall:secondary"), { { 2.0F, 0.0F }, { 3.0F, 1.0F } }),
	};
	const iggy::physics2d::CollisionWorld2D primary = BuiltWorld(primaryObjects);
	const iggy::physics2d::CollisionWorld2D secondary = BuiltWorld(secondaryObjects);
	const std::vector<iggy::physics2d::CollisionObject2D> primaryBefore = primary.objects();
	const std::vector<iggy::physics2d::CollisionObject2D> secondaryBefore = secondary.objects();

	const iggy::LevelCollisionWorldMerge2DResult result =
		iggy::LevelCollisionWorldMerge2D {}.merge(primary, secondary);

	Expect(result.built, "immutability setup should merge successfully");
	Expect(SameObjects(primary.objects(), primaryBefore), "merge should not mutate primary world objects");
	Expect(SameObjects(secondary.objects(), secondaryBefore), "merge should not mutate secondary world objects");
}

} // namespace

int main()
{
	TestEmptyWorldsMergeToEmptyWorld();
	TestPrimaryOnlyObjectsAppearInOutput();
	TestSecondaryOnlyObjectsAppearInOutput();
	TestBothWorldsPreservePrimaryThenSecondaryOrderAndCollisionBehavior();
	TestDuplicateNonEmptyIdsFailWithoutMergedWorld();
	TestEmptyIdsAreAllowedAndPreserved();
	TestInvalidDirectWorldObjectMapsToWorldBuildFailed();
	TestInputsAreNotMutated();

	return Failures;
}
