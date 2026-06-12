#include <cstdlib>
#include <vector>

#include "servers/physics2d/CollisionMotionQuery2D.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectBounds;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;

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

void ExpectUnblocked(const iggy::physics2d::CollisionMotionQuery2DResult &result, iggy::Vec2 expectedDelta, const char *message)
{
	Expect(!result.blocked, message);
	Expect(Near(result.safeTravel, 1.0F), message);
	Expect(NearVec(result.allowedDelta, expectedDelta), message);
	Expect(!result.hit.hit, message);
}

void ExpectBlocked(
	const iggy::physics2d::CollisionMotionQuery2DResult &result,
	float expectedTravel,
	iggy::Vec2 expectedAllowedDelta,
	std::size_t expectedObjectIndex,
	const iggy::physics2d::CollisionObject2D &expectedObject,
	const char *message)
{
	Expect(result.blocked, message);
	Expect(Near(result.safeTravel, expectedTravel), message);
	Expect(NearVec(result.allowedDelta, expectedAllowedDelta), message);
	Expect(result.hit.hit, message);
	Expect(Near(result.hit.travel, expectedTravel), message);
	Expect(result.hit.objectIndex == expectedObjectIndex, message);
	ExpectObject(result.hit.object, expectedObject, message);
	ExpectBounds(result.hit.objectBounds, expectedObject.shape.bounds, message);
}

void TestEmptyWorldAllowsFullDelta()
{
	const iggy::physics2d::CollisionWorld2D world;
	const iggy::Vec2 delta { 3.0F, -2.0F };

	const iggy::physics2d::CollisionMotionQuery2DResult result = iggy::physics2d::CollisionMotionQuery2D {}.sweepAabb(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		delta);

	ExpectUnblocked(result, delta, "empty world should allow full requested delta");
}

void TestNoHitMovementAllowsFullDelta()
{
	const iggy::physics2d::CollisionWorld2D world = World({
		Object(iggy::ResourceId("wall"), { { 5.0F, 5.0F }, { 6.0F, 6.0F } }),
	});
	const iggy::Vec2 delta { 2.0F, 0.0F };

	const iggy::physics2d::CollisionMotionQuery2DResult result = iggy::physics2d::CollisionMotionQuery2D {}.sweepAabb(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		delta);

	ExpectUnblocked(result, delta, "non-hitting movement should allow full delta");
}

void TestHorizontalSweepHitsExpectedTravel()
{
	const iggy::physics2d::CollisionObject2D wall = Object(
		iggy::ResourceId("wall"),
		{ { 3.0F, 0.0F }, { 4.0F, 1.0F } });
	const iggy::physics2d::CollisionWorld2D world = World({ wall });

	const iggy::physics2d::CollisionMotionQuery2DResult result = iggy::physics2d::CollisionMotionQuery2D {}.sweepAabb(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		{ 4.0F, 0.0F });

	ExpectBlocked(result, 0.5F, { 2.0F, 0.0F }, 0, wall, "horizontal sweep should report expected travel and allowed delta");
}

void TestVerticalSweepHitsExpectedTravel()
{
	const iggy::physics2d::CollisionObject2D platform = Object(
		iggy::ResourceId("platform"),
		{ { 0.0F, 4.0F }, { 1.0F, 5.0F } });
	const iggy::physics2d::CollisionWorld2D world = World({ platform });

	const iggy::physics2d::CollisionMotionQuery2DResult result = iggy::physics2d::CollisionMotionQuery2D {}.sweepAabb(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		{ 0.0F, 6.0F });

	ExpectBlocked(result, 0.5F, { 0.0F, 3.0F }, 0, platform, "vertical sweep should report expected travel and allowed delta");
}

void TestNegativeDeltaSweepHitsExpectedTravel()
{
	const iggy::physics2d::CollisionObject2D wall = Object(
		iggy::ResourceId("wall"),
		{ { 1.0F, 0.0F }, { 2.0F, 1.0F } });
	const iggy::physics2d::CollisionWorld2D world = World({ wall });

	const iggy::physics2d::CollisionMotionQuery2DResult result = iggy::physics2d::CollisionMotionQuery2D {}.sweepAabb(
		world,
		{ { 4.0F, 0.0F }, { 5.0F, 1.0F } },
		{ -4.0F, 0.0F });

	ExpectBlocked(result, 0.5F, { -2.0F, 0.0F }, 0, wall, "negative delta sweep should report expected travel and allowed delta");
}

void TestStartingOverlappedBlocksAtZero()
{
	const iggy::physics2d::CollisionObject2D wall = Object(
		iggy::ResourceId("wall"),
		{ { 0.5F, 0.5F }, { 2.0F, 2.0F } });
	const iggy::physics2d::CollisionWorld2D world = World({ wall });

	const iggy::physics2d::CollisionMotionQuery2DResult result = iggy::physics2d::CollisionMotionQuery2D {}.sweepAabb(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		{ 3.0F, 0.0F });

	ExpectBlocked(result, 0.0F, { 0.0F, 0.0F }, 0, wall, "starting overlap should block at travel zero");
}

void TestStartingEdgeTouchingBlocksAtZero()
{
	const iggy::physics2d::CollisionObject2D wall = Object(
		iggy::ResourceId("wall"),
		{ { 1.0F, 0.0F }, { 2.0F, 1.0F } });
	const iggy::physics2d::CollisionWorld2D world = World({ wall });

	const iggy::physics2d::CollisionMotionQuery2DResult result = iggy::physics2d::CollisionMotionQuery2D {}.sweepAabb(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		{ 2.0F, 0.0F });

	ExpectBlocked(result, 0.0F, { 0.0F, 0.0F }, 0, wall, "starting edge-touch should block at travel zero");
}

void TestZeroDeltaWithOverlapBlocksAtZero()
{
	const iggy::physics2d::CollisionObject2D wall = Object(
		iggy::ResourceId("wall"),
		{ { -0.5F, -0.5F }, { 0.5F, 0.5F } });
	const iggy::physics2d::CollisionWorld2D world = World({ wall });

	const iggy::physics2d::CollisionMotionQuery2DResult result = iggy::physics2d::CollisionMotionQuery2D {}.sweepAabb(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		{ 0.0F, 0.0F });

	ExpectBlocked(result, 0.0F, { 0.0F, 0.0F }, 0, wall, "zero delta with overlap should block at travel zero");
}

void TestZeroDeltaWithoutOverlapDoesNotBlock()
{
	const iggy::physics2d::CollisionWorld2D world = World({
		Object(iggy::ResourceId("wall"), { { 2.0F, 2.0F }, { 3.0F, 3.0F } }),
	});

	const iggy::physics2d::CollisionMotionQuery2DResult result = iggy::physics2d::CollisionMotionQuery2D {}.sweepAabb(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		{ 0.0F, 0.0F });

	ExpectUnblocked(result, { 0.0F, 0.0F }, "zero delta without overlap should not block");
}

void TestMultipleObjectsChooseEarliestHit()
{
	const iggy::physics2d::CollisionObject2D later = Object(
		iggy::ResourceId("later"),
		{ { 5.0F, 0.0F }, { 6.0F, 1.0F } });
	const iggy::physics2d::CollisionObject2D earliest = Object(
		iggy::ResourceId("earliest"),
		{ { 3.0F, 0.0F }, { 4.0F, 1.0F } });
	const iggy::physics2d::CollisionWorld2D world = World({ later, earliest });

	const iggy::physics2d::CollisionMotionQuery2DResult result = iggy::physics2d::CollisionMotionQuery2D {}.sweepAabb(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		{ 4.0F, 0.0F });

	ExpectBlocked(result, 0.5F, { 2.0F, 0.0F }, 1, earliest, "sweep should choose earliest hit even when later in world order");
}

void TestSameTravelHitsPreserveWorldOrder()
{
	const iggy::physics2d::CollisionObject2D first = Object(
		iggy::ResourceId("first"),
		{ { 3.0F, -1.0F }, { 4.0F, 0.0F } });
	const iggy::physics2d::CollisionObject2D second = Object(
		iggy::ResourceId("second"),
		{ { 3.0F, 1.0F }, { 4.0F, 2.0F } });
	const iggy::physics2d::CollisionWorld2D world = World({ first, second });

	const iggy::physics2d::CollisionMotionQuery2DResult result = iggy::physics2d::CollisionMotionQuery2D {}.sweepAabb(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		{ 4.0F, 0.0F });

	ExpectBlocked(result, 0.5F, { 2.0F, 0.0F }, 0, first, "same-travel hits should preserve first world-order hit");
}

void TestInvertedMovingBoundsAreNormalized()
{
	const iggy::physics2d::CollisionObject2D wall = Object(
		iggy::ResourceId("wall"),
		{ { 3.0F, 0.0F }, { 4.0F, 1.0F } });
	const iggy::physics2d::CollisionWorld2D world = World({ wall });

	const iggy::physics2d::CollisionMotionQuery2DResult result = iggy::physics2d::CollisionMotionQuery2D {}.sweepAabb(
		world,
		{ { 1.0F, 1.0F }, { 0.0F, 0.0F } },
		{ 4.0F, 0.0F });

	ExpectBlocked(result, 0.5F, { 2.0F, 0.0F }, 0, wall, "inverted moving bounds should be normalized for sweep");
}

void TestSolidFalseObjectCanBlock()
{
	const iggy::physics2d::CollisionObject2D trigger = Object(
		iggy::ResourceId("trigger"),
		{ { 3.0F, 0.0F }, { 4.0F, 1.0F } },
		false);
	const iggy::physics2d::CollisionWorld2D world = World({ trigger });

	const iggy::physics2d::CollisionMotionQuery2DResult result = iggy::physics2d::CollisionMotionQuery2D {}.sweepAabb(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		{ 4.0F, 0.0F });

	ExpectBlocked(result, 0.5F, { 2.0F, 0.0F }, 0, trigger, "solid=false object should still block until filtering policy exists");
	Expect(result.hit.object.solid == false, "solid=false hit should preserve solid metadata");
}

void TestInvalidDirectWorldShapeIsSkipped()
{
	const iggy::physics2d::CollisionObject2D invalid {
		iggy::ResourceId("invalid"),
		iggy::physics2d::makeAabbShape({ { 3.0F, 0.0F }, { 2.0F, 1.0F } }),
		true,
	};
	const iggy::physics2d::CollisionObject2D valid = Object(
		iggy::ResourceId("valid"),
		{ { 5.0F, 0.0F }, { 6.0F, 1.0F } });
	const iggy::physics2d::CollisionWorld2D world({ invalid, valid });

	const iggy::physics2d::CollisionMotionQuery2DResult result = iggy::physics2d::CollisionMotionQuery2D {}.sweepAabb(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		{ 8.0F, 0.0F });

	ExpectBlocked(result, 0.5F, { 4.0F, 0.0F }, 1, valid, "invalid direct-world shape should be skipped defensively");
}

void TestQueryDoesNotMutateWorldOrObjects()
{
	const iggy::physics2d::CollisionWorld2D world = World({
		Object(iggy::ResourceId("first"), { { 3.0F, 0.0F }, { 4.0F, 1.0F } }),
		Object(iggy::ResourceId("second"), { { 8.0F, 0.0F }, { 9.0F, 1.0F } }, false),
	});
	const std::vector<iggy::physics2d::CollisionObject2D> before = world.objects();

	const iggy::physics2d::CollisionMotionQuery2DResult result = iggy::physics2d::CollisionMotionQuery2D {}.sweepAabb(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		{ 4.0F, 0.0F });

	Expect(result.blocked, "mutation check query should hit");
	Expect(world.objects().size() == before.size(), "motion query should not mutate world object count");
	for (std::size_t index = 0; index < before.size(); ++index)
		ExpectObject(world.objects()[index], before[index], "motion query should not mutate world objects");
}

} // namespace

int main()
{
	TestEmptyWorldAllowsFullDelta();
	TestNoHitMovementAllowsFullDelta();
	TestHorizontalSweepHitsExpectedTravel();
	TestVerticalSweepHitsExpectedTravel();
	TestNegativeDeltaSweepHitsExpectedTravel();
	TestStartingOverlappedBlocksAtZero();
	TestStartingEdgeTouchingBlocksAtZero();
	TestZeroDeltaWithOverlapBlocksAtZero();
	TestZeroDeltaWithoutOverlapDoesNotBlock();
	TestMultipleObjectsChooseEarliestHit();
	TestSameTravelHitsPreserveWorldOrder();
	TestInvertedMovingBoundsAreNormalized();
	TestSolidFalseObjectCanBlock();
	TestInvalidDirectWorldShapeIsSkipped();
	TestQueryDoesNotMutateWorldOrObjects();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
