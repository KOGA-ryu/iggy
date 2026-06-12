#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

#include "core/math/Aabb2.hpp"
#include "core/math/Ray2.hpp"
#include "core/math/Rect2.hpp"
#include "core/math/Vec2.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

bool Near(float actual, float expected, float tolerance = 0.0001F)
{
	return std::fabs(actual - expected) <= tolerance;
}

void TestVec2Arithmetic()
{
	Expect((iggy::Vec2 { 2.0F, 3.0F } + iggy::Vec2 { 4.0F, 5.0F }) == iggy::Vec2 { 6.0F, 8.0F }, "Vec2 addition should add components");
	Expect((iggy::Vec2 { 8.0F, 5.0F } - iggy::Vec2 { 3.0F, 2.0F }) == iggy::Vec2 { 5.0F, 3.0F }, "Vec2 subtraction should subtract components");
	Expect((iggy::Vec2 { 2.0F, 3.0F } * 3.0F) == iggy::Vec2 { 6.0F, 9.0F }, "Vec2 scalar multiplication should scale components");
	Expect((iggy::Vec2 { 8.0F, 4.0F } / 2.0F) == iggy::Vec2 { 4.0F, 2.0F }, "Vec2 scalar division should divide components");
}

void TestVec2LengthDotAndNormalize()
{
	const iggy::Vec2 value { 3.0F, 4.0F };
	const iggy::Vec2 normal = value.normalized();

	Expect(value.lengthSquared() == 25.0F, "Vec2 lengthSquared should use dot product");
	Expect(value.length() == 5.0F, "Vec2 length should return magnitude");
	Expect(iggy::Dot(value, { 2.0F, 0.0F }) == 6.0F, "Vec2 dot should multiply and sum components");
	Expect(Near(normal.x, 0.6F) && Near(normal.y, 0.8F), "Vec2 normalized should return unit direction");
	Expect(iggy::Vec2 {}.normalized() == iggy::Vec2 {}, "zero Vec2 should normalize to zero");
}

void TestRect2ContainmentAndOverlap()
{
	const iggy::Rect2 rect { { 2.0F, 3.0F }, { 4.0F, 5.0F } };

	Expect(rect.min() == iggy::Vec2 { 2.0F, 3.0F }, "Rect2 min should be position");
	Expect(rect.max() == iggy::Vec2 { 6.0F, 8.0F }, "Rect2 max should be position plus size");
	Expect(rect.contains({ 4.0F, 4.0F }), "Rect2 should contain interior point");
	Expect(rect.contains({ 6.0F, 8.0F }), "Rect2 should contain max edge point");
	Expect(!rect.contains({ 6.1F, 8.0F }), "Rect2 should reject points outside bounds");
	Expect(rect.overlaps({ { 5.0F, 7.0F }, { 2.0F, 2.0F } }), "Rect2 should overlap intersecting rects");
	Expect(!rect.overlaps({ { 7.0F, 3.0F }, { 1.0F, 1.0F } }), "Rect2 should reject separated rects");
}

void TestAabb2ContainmentAndOverlap()
{
	const iggy::Aabb2 bounds { { -1.0F, -2.0F }, { 3.0F, 4.0F } };

	Expect(bounds.contains({ 0.0F, 0.0F }), "Aabb2 should contain interior point");
	Expect(bounds.contains({ -1.0F, 4.0F }), "Aabb2 should contain edge point");
	Expect(!bounds.contains({ 3.1F, 0.0F }), "Aabb2 should reject outside point");
	Expect(bounds.overlaps({ { 2.0F, 3.0F }, { 5.0F, 6.0F } }), "Aabb2 should overlap intersecting bounds");
	Expect(!bounds.overlaps({ { 4.0F, -2.0F }, { 5.0F, 4.0F } }), "Aabb2 should reject separated bounds");
}

void TestRay2PointAtDistance()
{
	const iggy::Ray2 ray { { 1.0F, 2.0F }, { 3.0F, 4.0F } };

	Expect(ray.pointAtDistance(0.0F) == iggy::Vec2 { 1.0F, 2.0F }, "Ray2 point at zero should be origin");
	Expect(ray.pointAtDistance(2.0F) == iggy::Vec2 { 7.0F, 10.0F }, "Ray2 point should advance along direction");
}

} // namespace

int main()
{
	TestVec2Arithmetic();
	TestVec2LengthDotAndNormalize();
	TestRect2ContainmentAndOverlap();
	TestAabb2ContainmentAndOverlap();
	TestRay2PointAtDistance();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
