#pragma once

#include <limits>

#include "core/math/Aabb2.hpp"
#include "core/math/Ray2.hpp"
#include "core/math/Rect2.hpp"
#include "core/math/Vec2.hpp"

namespace iggy::physics2d {

struct RaycastHit2D {
	bool hit = false;
	float distance = 0.0F;
	Vec2 point;
	Vec2 normal;
};

[[nodiscard]] bool ContainsPoint(Rect2 rect, Vec2 point);
[[nodiscard]] bool Overlaps(Rect2 left, Rect2 right);
[[nodiscard]] RaycastHit2D RaycastAabb(Ray2 ray, Aabb2 bounds, float maxDistance = std::numeric_limits<float>::infinity());

} // namespace iggy::physics2d
