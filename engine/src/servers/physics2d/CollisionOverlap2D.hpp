#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Aabb2.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::physics2d {

struct CollisionOverlap2DHit {
	std::size_t objectIndex = 0;
	CollisionObject2D object;
	Aabb2 objectBounds;
};

struct CollisionOverlap2DResult {
	std::vector<CollisionOverlap2DHit> hits;

	[[nodiscard]] bool hasHits() const;
};

class CollisionOverlap2D {
public:
	[[nodiscard]] CollisionOverlap2DResult queryAabb(const CollisionWorld2D &world, Aabb2 queryBounds) const;
};

} // namespace iggy::physics2d
