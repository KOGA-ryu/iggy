#include <cstdlib>

#include "servers/physics2d/CollisionShape2D.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectBounds;
using iggy::test::Failures;

void TestDefaultShapeIsUnknownAndInvalid()
{
	const iggy::physics2d::CollisionShape2D shape;

	Expect(shape.type == iggy::physics2d::CollisionShape2DType::Unknown, "default collision shape should have Unknown type");
	ExpectBounds(shape.bounds, {}, "default collision shape should have default bounds");
	Expect(!iggy::physics2d::isValid(shape), "default collision shape should be invalid");
}

void TestMakeAabbShapePreservesTypeAndBounds()
{
	const iggy::Aabb2 bounds { { 1.0F, 2.0F }, { 3.0F, 4.0F } };
	const iggy::physics2d::CollisionShape2D shape = iggy::physics2d::makeAabbShape(bounds);

	Expect(shape.type == iggy::physics2d::CollisionShape2DType::Aabb, "makeAabbShape should set Aabb type");
	ExpectBounds(shape.bounds, bounds, "makeAabbShape should preserve supplied bounds");
}

void TestBoundsOfReturnsStoredBounds()
{
	const iggy::Aabb2 bounds { { -2.0F, 1.5F }, { 5.0F, 9.0F } };
	const iggy::physics2d::CollisionShape2D shape = iggy::physics2d::makeAabbShape(bounds);

	ExpectBounds(iggy::physics2d::boundsOf(shape), bounds, "boundsOf should return exact stored bounds");
}

void TestOrderedAabbIsValid()
{
	const iggy::physics2d::CollisionShape2D shape = iggy::physics2d::makeAabbShape({ { 0.0F, 1.0F }, { 2.0F, 3.0F } });

	Expect(iggy::physics2d::isValid(shape), "ordered AABB bounds should be valid");
}

void TestDegenerateAabbIsValid()
{
	const iggy::physics2d::CollisionShape2D point = iggy::physics2d::makeAabbShape({ { 2.0F, 3.0F }, { 2.0F, 3.0F } });
	const iggy::physics2d::CollisionShape2D verticalLine = iggy::physics2d::makeAabbShape({ { 2.0F, 1.0F }, { 2.0F, 5.0F } });
	const iggy::physics2d::CollisionShape2D horizontalLine = iggy::physics2d::makeAabbShape({ { 1.0F, 3.0F }, { 5.0F, 3.0F } });

	Expect(iggy::physics2d::isValid(point), "zero-width and zero-height AABB should be valid");
	Expect(iggy::physics2d::isValid(verticalLine), "zero-width AABB should be valid");
	Expect(iggy::physics2d::isValid(horizontalLine), "zero-height AABB should be valid");
}

void TestNegativeOrderedAabbIsValid()
{
	const iggy::physics2d::CollisionShape2D shape = iggy::physics2d::makeAabbShape({ { -3.0F, -2.0F }, { -1.0F, 0.0F } });

	Expect(iggy::physics2d::isValid(shape), "ordered negative-coordinate AABB should be valid");
}

void TestInvertedXBoundsAreInvalid()
{
	const iggy::physics2d::CollisionShape2D shape = iggy::physics2d::makeAabbShape({ { 3.0F, 1.0F }, { 2.0F, 4.0F } });

	Expect(!iggy::physics2d::isValid(shape), "inverted x bounds should be invalid");
}

void TestInvertedYBoundsAreInvalid()
{
	const iggy::physics2d::CollisionShape2D shape = iggy::physics2d::makeAabbShape({ { 1.0F, 5.0F }, { 2.0F, 4.0F } });

	Expect(!iggy::physics2d::isValid(shape), "inverted y bounds should be invalid");
}

void TestInvalidAabbBoundsAreReturnedWithoutNormalization()
{
	const iggy::Aabb2 inverted { { 3.0F, 5.0F }, { 2.0F, 4.0F } };
	const iggy::physics2d::CollisionShape2D shape = iggy::physics2d::makeAabbShape(inverted);

	Expect(!iggy::physics2d::isValid(shape), "inverted bounds setup should be invalid");
	ExpectBounds(iggy::physics2d::boundsOf(shape), inverted, "boundsOf should return inverted bounds without normalization");
}

void TestUnknownShapeWithStoredBoundsIsInvalid()
{
	iggy::physics2d::CollisionShape2D shape;
	shape.bounds = { { -4.0F, -5.0F }, { 6.0F, 7.0F } };

	Expect(!iggy::physics2d::isValid(shape), "unknown collision shape should be invalid even with stored bounds");
	ExpectBounds(iggy::physics2d::boundsOf(shape), { { -4.0F, -5.0F }, { 6.0F, 7.0F } }, "boundsOf should return stored bounds for unknown shape");
}

} // namespace

int main()
{
	TestDefaultShapeIsUnknownAndInvalid();
	TestMakeAabbShapePreservesTypeAndBounds();
	TestBoundsOfReturnsStoredBounds();
	TestOrderedAabbIsValid();
	TestDegenerateAabbIsValid();
	TestNegativeOrderedAabbIsValid();
	TestInvertedXBoundsAreInvalid();
	TestInvertedYBoundsAreInvalid();
	TestInvalidAabbBoundsAreReturnedWithoutNormalization();
	TestUnknownShapeWithStoredBoundsIsInvalid();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
