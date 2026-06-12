#pragma once

#include <cmath>
#include <iostream>
#include <string_view>

#include "core/math/Vec2.hpp"

namespace iggy::test {

inline int Failures = 0;

inline void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

inline bool Near(float actual, float expected, float tolerance = 0.0001F)
{
	return std::fabs(actual - expected) <= tolerance;
}

inline bool NearVec(Vec2 actual, Vec2 expected, float tolerance = 0.0001F)
{
	return Near(actual.x, expected.x, tolerance) && Near(actual.y, expected.y, tolerance);
}

} // namespace iggy::test
