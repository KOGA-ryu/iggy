#include <cstdlib>
#include <vector>

#include "servers/physics2d/CollisionOverlap2D.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectBounds;
using iggy::test::Failures;

iggy::physics2d::CollisionObject2D Object(
	iggy::ResourceId id,
	iggy::Aabb2 bounds,
	bool solid = true)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), solid };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult result = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(result.built, "test fixture world should build");
	return result.world;
}

void ExpectObject(const iggy::physics2d::CollisionObject2D &actual, const iggy::physics2d::CollisionObject2D &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.shape.type == expected.shape.type, message);
	ExpectBounds(actual.shape.bounds, expected.shape.bounds, message);
	Expect(actual.solid == expected.solid, message);
}

void TestEmptyWorldReturnsNoHits()
{
	const iggy::physics2d::CollisionWorld2D world;

	const iggy::physics2d::CollisionOverlap2DResult result = iggy::physics2d::CollisionOverlap2D {}.queryAabb(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } });

	Expect(result.hits.empty(), "empty world should return no overlap hits");
	Expect(!result.hasHits(), "empty world result should report no hits");
}

void TestNonOverlappingQueryReturnsNoHits()
{
	const iggy::physics2d::CollisionWorld2D world = World({
		Object(iggy::ResourceId("wall"), { { 0.0F, 0.0F }, { 1.0F, 1.0F } }),
	});

	const iggy::physics2d::CollisionOverlap2DResult result = iggy::physics2d::CollisionOverlap2D {}.queryAabb(
		world,
		{ { 2.0F, 2.0F }, { 3.0F, 3.0F } });

	Expect(result.hits.empty(), "non-overlapping query should return no hits");
	Expect(!result.hasHits(), "non-overlapping query result should report no hits");
}

void TestOneOverlappingObjectReturnsCopiedHit()
{
	const iggy::physics2d::CollisionObject2D object = Object(
		iggy::ResourceId("crate"),
		{ { 1.0F, 1.0F }, { 3.0F, 4.0F } },
		false);
	const iggy::physics2d::CollisionWorld2D world = World({ object });

	const iggy::physics2d::CollisionOverlap2DResult result = iggy::physics2d::CollisionOverlap2D {}.queryAabb(
		world,
		{ { 2.0F, 0.0F }, { 4.0F, 2.0F } });

	Expect(result.hasHits(), "overlapping query should report hits");
	Expect(result.hits.size() == 1, "overlapping one object should return one hit");
	Expect(result.hits[0].objectIndex == 0, "hit should preserve object index");
	ExpectObject(result.hits[0].object, object, "hit should copy object id, shape, bounds, and solid flag");
	ExpectBounds(result.hits[0].objectBounds, object.shape.bounds, "hit should preserve exact object bounds");
}

void TestMultipleOverlapsPreserveWorldOrderAndIndexes()
{
	const iggy::physics2d::CollisionObject2D first = Object(
		iggy::ResourceId("first"),
		{ { 0.0F, 0.0F }, { 2.0F, 2.0F } });
	const iggy::physics2d::CollisionObject2D skipped = Object(
		iggy::ResourceId("skipped"),
		{ { 5.0F, 5.0F }, { 6.0F, 6.0F } });
	const iggy::physics2d::CollisionObject2D third = Object(
		iggy::ResourceId("third"),
		{ { -1.0F, -1.0F }, { 0.5F, 0.5F } });
	const iggy::physics2d::CollisionWorld2D world = World({ first, skipped, third });

	const iggy::physics2d::CollisionOverlap2DResult result = iggy::physics2d::CollisionOverlap2D {}.queryAabb(
		world,
		{ { -0.25F, -0.25F }, { 1.0F, 1.0F } });

	Expect(result.hits.size() == 2, "query should hit two overlapping objects");
	Expect(result.hits[0].objectIndex == 0, "first hit should preserve first world index");
	Expect(result.hits[1].objectIndex == 2, "second hit should preserve original world index");
	ExpectObject(result.hits[0].object, first, "first hit should preserve first world object");
	ExpectObject(result.hits[1].object, third, "second hit should preserve third world object");
}

void TestSolidFalseObjectsAreIncluded()
{
	const iggy::physics2d::CollisionObject2D trigger = Object(
		iggy::ResourceId("trigger"),
		{ { 0.0F, 0.0F }, { 2.0F, 2.0F } },
		false);
	const iggy::physics2d::CollisionWorld2D world = World({ trigger });

	const iggy::physics2d::CollisionOverlap2DResult result = iggy::physics2d::CollisionOverlap2D {}.queryAabb(
		world,
		{ { 1.0F, 1.0F }, { 3.0F, 3.0F } });

	Expect(result.hits.size() == 1, "solid=false object should be included in overlap query");
	Expect(result.hits[0].object.solid == false, "hit should preserve solid=false metadata");
}

void TestEdgeTouchingAabbsOverlap()
{
	const iggy::physics2d::CollisionWorld2D world = World({
		Object(iggy::ResourceId("edge"), { { 1.0F, 1.0F }, { 2.0F, 2.0F } }),
	});

	const iggy::physics2d::CollisionOverlap2DResult result = iggy::physics2d::CollisionOverlap2D {}.queryAabb(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } });

	Expect(result.hits.size() == 1, "edge-touching AABBs should overlap inclusively");
	Expect(result.hits[0].objectIndex == 0, "edge-touching hit should preserve object index");
}

void TestInvertedQueryBoundsAreNormalized()
{
	const iggy::physics2d::CollisionObject2D object = Object(
		iggy::ResourceId("target"),
		{ { -1.0F, -2.0F }, { 1.0F, 2.0F } });
	const iggy::physics2d::CollisionWorld2D world = World({ object });

	const iggy::physics2d::CollisionOverlap2DResult result = iggy::physics2d::CollisionOverlap2D {}.queryAabb(
		world,
		{ { 2.0F, 3.0F }, { 0.0F, 0.0F } });

	Expect(result.hits.size() == 1, "inverted query bounds should be normalized and hit expected object");
	ExpectObject(result.hits[0].object, object, "inverted query hit should preserve object copy");
}

void TestInvalidObjectsAreSkippedInDirectWorld()
{
	const iggy::physics2d::CollisionObject2D invalid {
		iggy::ResourceId("invalid"),
		iggy::physics2d::makeAabbShape({ { 5.0F, 0.0F }, { 4.0F, 1.0F } }),
		true,
	};
	const iggy::physics2d::CollisionObject2D valid = Object(
		iggy::ResourceId("valid"),
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } });
	const iggy::physics2d::CollisionWorld2D world({ invalid, valid });
	const std::vector<iggy::physics2d::CollisionObject2D> before = world.objects();

	const iggy::physics2d::CollisionOverlap2DResult result = iggy::physics2d::CollisionOverlap2D {}.queryAabb(
		world,
		{ { 0.0F, 0.0F }, { 6.0F, 1.0F } });

	Expect(result.hits.size() == 1, "query should skip invalid direct-world objects");
	Expect(result.hits[0].objectIndex == 1, "valid hit should preserve original index after skipped invalid object");
	ExpectObject(result.hits[0].object, valid, "valid hit should preserve object after skipped invalid object");
	Expect(world.objects().size() == before.size(), "query should not mutate direct world object count");
	for (std::size_t index = 0; index < before.size(); ++index)
		ExpectObject(world.objects()[index], before[index], "query should not mutate direct world objects");
}

void TestQueryDoesNotMutateValidWorld()
{
	const iggy::physics2d::CollisionWorld2D world = World({
		Object(iggy::ResourceId("solid"), { { 0.0F, 0.0F }, { 1.0F, 1.0F } }),
		Object(iggy::ResourceId("trigger"), { { 2.0F, 0.0F }, { 3.0F, 1.0F } }, false),
	});
	const std::vector<iggy::physics2d::CollisionObject2D> before = world.objects();

	const iggy::physics2d::CollisionOverlap2DResult result = iggy::physics2d::CollisionOverlap2D {}.queryAabb(
		world,
		{ { -1.0F, -1.0F }, { 4.0F, 2.0F } });

	Expect(result.hits.size() == 2, "mutation check query should hit both valid objects");
	Expect(world.objects().size() == before.size(), "query should not mutate valid world object count");
	for (std::size_t index = 0; index < before.size(); ++index)
		ExpectObject(world.objects()[index], before[index], "query should not mutate valid world objects");
}

void TestHasHitsReflectsHitVector()
{
	iggy::physics2d::CollisionOverlap2DResult empty;
	Expect(!empty.hasHits(), "default overlap result should not have hits");

	iggy::physics2d::CollisionOverlap2DResult nonEmpty;
	nonEmpty.hits.push_back({});
	Expect(nonEmpty.hasHits(), "overlap result should report hits when hit vector is non-empty");
}

} // namespace

int main()
{
	TestEmptyWorldReturnsNoHits();
	TestNonOverlappingQueryReturnsNoHits();
	TestOneOverlappingObjectReturnsCopiedHit();
	TestMultipleOverlapsPreserveWorldOrderAndIndexes();
	TestSolidFalseObjectsAreIncluded();
	TestEdgeTouchingAabbsOverlap();
	TestInvertedQueryBoundsAreNormalized();
	TestInvalidObjectsAreSkippedInDirectWorld();
	TestQueryDoesNotMutateValidWorld();
	TestHasHitsReflectsHitVector();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
