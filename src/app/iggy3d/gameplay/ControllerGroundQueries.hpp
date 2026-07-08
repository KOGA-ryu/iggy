#pragma once

#include "core/math/Vec3.hpp"

namespace iggy3d {

class SpatialSurfaceSet;

inline constexpr float kProductGameplayGroundContactToleranceMeters = 0.12F;

bool findHighestWalkableGroundAtOrBelow(
    const SpatialSurfaceSet* surfaces,
    Vec3 position,
    float maxY,
    float& groundY);

bool playerHasNearbyGround(const SpatialSurfaceSet* surfaces, Vec3 position);

bool findLowestWalkableFloorY(const SpatialSurfaceSet* surfaces, float& floorY);

}  // namespace iggy3d
