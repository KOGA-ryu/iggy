#include <cstdlib>
#include <vector>

#include "servers/physics2d/CharacterMove2D.hpp"
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

void ExpectMotionHit(
	const iggy::physics2d::CollisionMotionQuery2DResult &motion,
	float expectedTravel,
	std::size_t expectedObjectIndex,
	const iggy::physics2d::CollisionObject2D &expectedObject,
	const char *message)
{
	Expect(motion.blocked, message);
	Expect(motion.hit.hit, message);
	Expect(Near(motion.safeTravel, expectedTravel), message);
	Expect(Near(motion.hit.travel, expectedTravel), message);
	Expect(motion.hit.objectIndex == expectedObjectIndex, message);
	ExpectObject(motion.hit.object, expectedObject, message);
	ExpectBounds(motion.hit.objectBounds, expectedObject.shape.bounds, message);
}

void ExpectWorldUnchanged(
	const iggy::physics2d::CollisionWorld2D &world,
	const std::vector<iggy::physics2d::CollisionObject2D> &before,
	const char *message)
{
	Expect(world.objects().size() == before.size(), message);
	for (std::size_t index = 0; index < before.size(); ++index)
		ExpectObject(world.objects()[index], before[index], message);
}

void TestEmptyWorldNonzeroDeltaMoves()
{
	const iggy::physics2d::CollisionWorld2D world;
	const iggy::Aabb2 start { { 0.0F, 0.0F }, { 1.0F, 1.0F } };
	const iggy::Vec2 delta { 2.0F, 3.0F };

	const iggy::physics2d::CharacterMove2DResult result = iggy::physics2d::CharacterMove2D {}.move(world, start, delta);

	Expect(result.status == iggy::physics2d::CharacterMove2DStatus::Moved, "empty world nonzero delta should move");
	ExpectBounds(result.startBounds, start, "empty world move should preserve normalized start bounds");
	ExpectBounds(result.finalBounds, { { 2.0F, 3.0F }, { 3.0F, 4.0F } }, "empty world move should translate final bounds by full delta");
	Expect(NearVec(result.requestedDelta, delta), "empty world move should preserve requested delta");
	Expect(NearVec(result.allowedDelta, delta), "empty world move should allow full delta");
	Expect(!result.motion.blocked, "empty world move should embed unblocked motion result");
}

void TestNoHitWorldMovesAndEmbedsMotion()
{
	const iggy::physics2d::CollisionWorld2D world = World({
		Object(iggy::ResourceId("wall"), { { 5.0F, 5.0F }, { 6.0F, 6.0F } }),
	});
	const iggy::Vec2 delta { 2.0F, 0.0F };

	const iggy::physics2d::CharacterMove2DResult result = iggy::physics2d::CharacterMove2D {}.move(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		delta);

	Expect(result.status == iggy::physics2d::CharacterMove2DStatus::Moved, "no-hit movement should move");
	Expect(NearVec(result.allowedDelta, delta), "no-hit movement should allow full delta");
	ExpectBounds(result.finalBounds, { { 2.0F, 0.0F }, { 3.0F, 1.0F } }, "no-hit movement should translate final bounds by full delta");
	Expect(!result.motion.blocked, "no-hit movement should embed unblocked motion");
	Expect(Near(result.motion.safeTravel, 1.0F), "no-hit movement should preserve motion safe travel");
	Expect(NearVec(result.motion.allowedDelta, delta), "no-hit movement should preserve motion allowed delta");
}

void TestBlockedMovementReturnsAllowedDeltaFinalBoundsAndHit()
{
	const iggy::physics2d::CollisionObject2D wall = Object(
		iggy::ResourceId("wall"),
		{ { 3.0F, 0.0F }, { 4.0F, 1.0F } });
	const iggy::physics2d::CollisionWorld2D world = World({ wall });

	const iggy::physics2d::CharacterMove2DResult result = iggy::physics2d::CharacterMove2D {}.move(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		{ 4.0F, 0.0F });

	Expect(result.status == iggy::physics2d::CharacterMove2DStatus::Blocked, "blocked movement should report blocked status");
	Expect(NearVec(result.allowedDelta, { 2.0F, 0.0F }), "blocked movement should preserve allowed delta from motion query");
	ExpectBounds(result.finalBounds, { { 2.0F, 0.0F }, { 3.0F, 1.0F } }, "blocked movement should translate final bounds by allowed delta");
	ExpectMotionHit(result.motion, 0.5F, 0, wall, "blocked movement should preserve motion hit metadata");
}

void TestPartialMovementThatHitsIsBlocked()
{
	const iggy::physics2d::CollisionObject2D wall = Object(
		iggy::ResourceId("wall"),
		{ { 5.0F, 0.0F }, { 6.0F, 1.0F } });
	const iggy::physics2d::CollisionWorld2D world = World({ wall });

	const iggy::physics2d::CharacterMove2DResult result = iggy::physics2d::CharacterMove2D {}.move(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		{ 8.0F, 0.0F });

	Expect(result.status == iggy::physics2d::CharacterMove2DStatus::Blocked, "partial movement ending in collision should be blocked");
	Expect(Near(result.motion.safeTravel, 0.5F), "partial blocked movement should preserve nonzero safe travel");
	Expect(NearVec(result.allowedDelta, { 4.0F, 0.0F }), "partial blocked movement should preserve nonzero allowed delta");
	ExpectBounds(result.finalBounds, { { 4.0F, 0.0F }, { 5.0F, 1.0F } }, "partial blocked movement should translate final bounds by allowed delta");
}

void TestStartingOverlappedBlocksWithUnchangedFinalBounds()
{
	const iggy::physics2d::CollisionObject2D wall = Object(
		iggy::ResourceId("wall"),
		{ { 0.5F, 0.5F }, { 2.0F, 2.0F } });
	const iggy::physics2d::CollisionWorld2D world = World({ wall });
	const iggy::Aabb2 start { { 0.0F, 0.0F }, { 1.0F, 1.0F } };

	const iggy::physics2d::CharacterMove2DResult result = iggy::physics2d::CharacterMove2D {}.move(world, start, { 4.0F, 0.0F });

	Expect(result.status == iggy::physics2d::CharacterMove2DStatus::Blocked, "starting overlap should block");
	Expect(NearVec(result.allowedDelta, { 0.0F, 0.0F }), "starting overlap should allow zero delta");
	ExpectBounds(result.finalBounds, start, "starting overlap should leave final bounds unchanged");
	ExpectMotionHit(result.motion, 0.0F, 0, wall, "starting overlap should preserve travel zero hit");
}

void TestStartingEdgeTouchingBlocksWithUnchangedFinalBounds()
{
	const iggy::physics2d::CollisionObject2D wall = Object(
		iggy::ResourceId("wall"),
		{ { 1.0F, 0.0F }, { 2.0F, 1.0F } });
	const iggy::physics2d::CollisionWorld2D world = World({ wall });
	const iggy::Aabb2 start { { 0.0F, 0.0F }, { 1.0F, 1.0F } };

	const iggy::physics2d::CharacterMove2DResult result = iggy::physics2d::CharacterMove2D {}.move(world, start, { 2.0F, 0.0F });

	Expect(result.status == iggy::physics2d::CharacterMove2DStatus::Blocked, "starting edge-touch should block");
	Expect(NearVec(result.allowedDelta, { 0.0F, 0.0F }), "starting edge-touch should allow zero delta");
	ExpectBounds(result.finalBounds, start, "starting edge-touch should leave final bounds unchanged");
	ExpectMotionHit(result.motion, 0.0F, 0, wall, "starting edge-touch should preserve travel zero hit");
}

void TestZeroDeltaWithoutOverlapIsNoMovement()
{
	const iggy::physics2d::CollisionWorld2D world = World({
		Object(iggy::ResourceId("wall"), { { 2.0F, 2.0F }, { 3.0F, 3.0F } }),
	});
	const iggy::Aabb2 start { { 0.0F, 0.0F }, { 1.0F, 1.0F } };

	const iggy::physics2d::CharacterMove2DResult result = iggy::physics2d::CharacterMove2D {}.move(world, start, { 0.0F, 0.0F });

	Expect(result.status == iggy::physics2d::CharacterMove2DStatus::NoMovement, "zero delta without overlap should be no movement");
	Expect(NearVec(result.allowedDelta, { 0.0F, 0.0F }), "zero delta without overlap should allow zero delta");
	ExpectBounds(result.finalBounds, start, "zero delta without overlap should leave final bounds unchanged");
	Expect(!result.motion.blocked, "zero delta without overlap should embed unblocked motion");
}

void TestZeroDeltaWithOverlapIsBlocked()
{
	const iggy::physics2d::CollisionObject2D wall = Object(
		iggy::ResourceId("wall"),
		{ { -0.5F, -0.5F }, { 0.5F, 0.5F } });
	const iggy::physics2d::CollisionWorld2D world = World({ wall });
	const iggy::Aabb2 start { { 0.0F, 0.0F }, { 1.0F, 1.0F } };

	const iggy::physics2d::CharacterMove2DResult result = iggy::physics2d::CharacterMove2D {}.move(world, start, { 0.0F, 0.0F });

	Expect(result.status == iggy::physics2d::CharacterMove2DStatus::Blocked, "zero delta with overlap should be blocked by motion query");
	Expect(NearVec(result.allowedDelta, { 0.0F, 0.0F }), "zero delta with overlap should allow zero delta");
	ExpectBounds(result.finalBounds, start, "zero delta with overlap should leave final bounds unchanged");
	ExpectMotionHit(result.motion, 0.0F, 0, wall, "zero delta with overlap should preserve motion hit");
}

void TestInvertedInputBoundsAreNormalized()
{
	const iggy::physics2d::CollisionWorld2D world;

	const iggy::physics2d::CharacterMove2DResult result = iggy::physics2d::CharacterMove2D {}.move(
		world,
		{ { 1.0F, 2.0F }, { -1.0F, -2.0F } },
		{ 3.0F, 4.0F });

	Expect(result.status == iggy::physics2d::CharacterMove2DStatus::Moved, "inverted input bounds should still move when unblocked");
	ExpectBounds(result.startBounds, { { -1.0F, -2.0F }, { 1.0F, 2.0F } }, "inverted input bounds should normalize start bounds");
	ExpectBounds(result.finalBounds, { { 2.0F, 2.0F }, { 4.0F, 6.0F } }, "inverted input bounds should normalize before final translation");
}

void TestSolidFalseObjectCanBlockAndMetadataIsPreserved()
{
	const iggy::physics2d::CollisionObject2D trigger = Object(
		iggy::ResourceId("trigger"),
		{ { 3.0F, 0.0F }, { 4.0F, 1.0F } },
		false);
	const iggy::physics2d::CollisionWorld2D world = World({ trigger });

	const iggy::physics2d::CharacterMove2DResult result = iggy::physics2d::CharacterMove2D {}.move(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		{ 4.0F, 0.0F });

	Expect(result.status == iggy::physics2d::CharacterMove2DStatus::Blocked, "solid=false object should block through motion query");
	ExpectMotionHit(result.motion, 0.5F, 0, trigger, "solid=false hit should preserve metadata");
	Expect(result.motion.hit.object.solid == false, "solid=false hit should preserve solid flag");
}

void TestWorldIsNotMutated()
{
	const iggy::physics2d::CollisionWorld2D world = World({
		Object(iggy::ResourceId("first"), { { 3.0F, 0.0F }, { 4.0F, 1.0F } }),
		Object(iggy::ResourceId("second"), { { 8.0F, 0.0F }, { 9.0F, 1.0F } }, false),
	});
	const std::vector<iggy::physics2d::CollisionObject2D> before = world.objects();

	const iggy::physics2d::CharacterMove2DResult result = iggy::physics2d::CharacterMove2D {}.move(
		world,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		{ 4.0F, 0.0F });

	Expect(result.status == iggy::physics2d::CharacterMove2DStatus::Blocked, "mutation check move should block");
	ExpectWorldUnchanged(world, before, "character move should not mutate world objects");
}

} // namespace

int main()
{
	TestEmptyWorldNonzeroDeltaMoves();
	TestNoHitWorldMovesAndEmbedsMotion();
	TestBlockedMovementReturnsAllowedDeltaFinalBoundsAndHit();
	TestPartialMovementThatHitsIsBlocked();
	TestStartingOverlappedBlocksWithUnchangedFinalBounds();
	TestStartingEdgeTouchingBlocksWithUnchangedFinalBounds();
	TestZeroDeltaWithoutOverlapIsNoMovement();
	TestZeroDeltaWithOverlapIsBlocked();
	TestInvertedInputBoundsAreNormalized();
	TestSolidFalseObjectCanBlockAndMetadataIsPreserved();
	TestWorldIsNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
