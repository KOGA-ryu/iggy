#include "app/iggy3d/gameplay/ControllerGroundQueries.hpp"

#include "runtime/collision/SpatialSurfaceSet.hpp"

#include <cmath>

namespace iggy3d {
namespace {

constexpr float kGameplayGroundFootprintToleranceMeters = 0.35F;

bool surfaceContainsXZ(const CollisionSurfaceView& surface,
                       Vec3 position,
                       float toleranceMeters) {
  return position.x >= surface.bounds.min.x - toleranceMeters &&
         position.x <= surface.bounds.max.x + toleranceMeters &&
         position.z >= surface.bounds.min.z - toleranceMeters &&
         position.z <= surface.bounds.max.z + toleranceMeters;
}

bool walkableSurfaceHeightAt(const CollisionSurfaceView& surface,
                             Vec3 position,
                             float& heightMeters) {
  // branch-gate: BG-1169
  if (surface.role != CollisionSurfaceRole::Walkable ||
      std::fabs(surface.normal.y) <= 0.0001F ||
      !surfaceContainsXZ(surface, position, kGameplayGroundFootprintToleranceMeters)) {
    return false;
  }

  const float height =
      surface.planePoint.y -
      ((surface.normal.x * (position.x - surface.planePoint.x)) +
       (surface.normal.z * (position.z - surface.planePoint.z))) /
          surface.normal.y;
  // branch-gate: BG-1170
  if (!std::isfinite(height)) {
    return false;
  }
  heightMeters = height;
  return true;
}

}  // namespace

bool findHighestWalkableGroundAtOrBelow(const SpatialSurfaceSet* surfaces,
                                        Vec3 position,
                                        float maxY,
                                        float& groundY) {
  // branch-gate: BG-1171
  if (surfaces == nullptr) {
    return false;
  }

  bool found = false;
  float bestY = 0.0F;
  for (const CollisionSurfaceView& surface : surfaces->surfaces()) {
    float height = 0.0F;
    // branch-gate: BG-1169
    if (!walkableSurfaceHeightAt(surface, position, height) ||
        height > maxY + kProductGameplayGroundContactToleranceMeters) {
      continue;
    }
    // branch-gate: BG-1172
    if (!found || height > bestY) {
      found = true;
      bestY = height;
    }
  }
  // branch-gate: BG-1171
  if (!found) {
    return false;
  }
  groundY = bestY;
  return true;
}

bool playerHasNearbyGround(const SpatialSurfaceSet* surfaces, Vec3 position) {
  float groundY = 0.0F;
  return findHighestWalkableGroundAtOrBelow(
             surfaces,
             position,
             position.y + kProductGameplayGroundContactToleranceMeters,
             groundY) &&
         std::fabs(position.y - groundY) <=
             kProductGameplayGroundContactToleranceMeters;
}

bool findLowestWalkableFloorY(const SpatialSurfaceSet* surfaces, float& floorY) {
  // branch-gate: BG-1183
  if (surfaces == nullptr) {
    return false;
  }

  bool found = false;
  float lowest = 0.0F;
  for (const CollisionSurfaceView& surface : surfaces->surfaces()) {
    // branch-gate: BG-1176
    if (surface.role != CollisionSurfaceRole::Walkable ||
        std::fabs(surface.normal.y) <= 0.0001F) {
      continue;
    }
    const float candidate = surface.planePoint.y;
    // branch-gate: BG-1177
    if (!std::isfinite(candidate)) {
      continue;
    }
    // branch-gate: BG-1178
    if (!found || candidate < lowest) {
      found = true;
      lowest = candidate;
    }
  }
  // branch-gate: BG-1184
  if (!found) {
    return false;
  }
  floorY = lowest;
  return true;
}

}  // namespace iggy3d
