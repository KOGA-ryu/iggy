#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

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

} // namespace

int main()
{
	TestVec2Arithmetic();
	TestVec2LengthDotAndNormalize();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
