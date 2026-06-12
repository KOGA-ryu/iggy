#pragma once

#include <string_view>

#include "core/math/Aabb2.hpp"
#include "core/math/Vec2.hpp"
#include "TestHarness.hpp"

namespace iggy::test {

inline bool SameBounds(Aabb2 actual, Aabb2 expected, float tolerance = 0.0001F)
{
	return NearVec(actual.min, expected.min, tolerance) && NearVec(actual.max, expected.max, tolerance);
}

inline void ExpectBounds(Aabb2 actual, Aabb2 expected, std::string_view message)
{
	Expect(SameBounds(actual, expected), message);
}

inline void ExpectBounds(Aabb2 actual, Vec2 expectedMin, Vec2 expectedMax, std::string_view message)
{
	ExpectBounds(actual, { expectedMin, expectedMax }, message);
}

} // namespace iggy::test
