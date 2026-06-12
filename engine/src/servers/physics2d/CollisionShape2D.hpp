#pragma once

#include "core/math/Aabb2.hpp"

namespace iggy::physics2d {

enum class CollisionShape2DType {
	Unknown,
	Aabb,
};

struct CollisionShape2D {
	CollisionShape2DType type = CollisionShape2DType::Unknown;
	Aabb2 bounds;
};

[[nodiscard]] CollisionShape2D makeAabbShape(Aabb2 bounds);
[[nodiscard]] Aabb2 boundsOf(const CollisionShape2D &shape);
[[nodiscard]] bool isValid(const CollisionShape2D &shape);

} // namespace iggy::physics2d
