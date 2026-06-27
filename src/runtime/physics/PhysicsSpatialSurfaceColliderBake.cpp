#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace iggy3d {
namespace {

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1099
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

PhysicsSpatialSurfaceColliderBakeResult bakeResult(
    PhysicsSpatialSurfaceColliderBakeStatus status,
    bool ok) {
  PhysicsSpatialSurfaceColliderBakeResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsSpatialSurfaceColliderBakeStatusName(status);
  return result;
}

bool positiveFinite(float value) {
  return std::isfinite(value) && value > 0.0F;
}

Vec3 expandHalfExtents(Vec3 halfExtents, float minHalfExtentMeters) {
  return {std::max(halfExtents.x, minHalfExtentMeters),
          std::max(halfExtents.y, minHalfExtentMeters),
          std::max(halfExtents.z, minHalfExtentMeters)};
}

bool actorBlockingSurface(const CollisionSurfaceView& surface) {
  return surface.role == CollisionSurfaceRole::Blocker ||
         surface.blocksActor;
}

bool projectileOnlySurface(const CollisionSurfaceView& surface) {
  return surface.role == CollisionSurfaceRole::ProjectileBlocker ||
         surface.blocksProjectile || surface.hasProjectileMask;
}

bool openingSurface(const CollisionSurfaceView& surface) {
  return surface.role == CollisionSurfaceRole::Opening || surface.opening;
}

bool includeSurface(
    const CollisionSurfaceView& surface,
    const PhysicsSpatialSurfaceColliderBakeConfig& config) {
  // branch-gate: BG-1099
  if (openingSurface(surface)) {
    return config.includeOpenings;
  }
  // branch-gate: BG-1099
  if (surface.role == CollisionSurfaceRole::Walkable) {
    return config.includeWalkable;
  }
  // branch-gate: BG-1099
  if (actorBlockingSurface(surface)) {
    return config.includeActorBlockers;
  }
  // branch-gate: BG-1099
  if (projectileOnlySurface(surface)) {
    return config.includeProjectileBlockers;
  }
  return false;
}

PhysicsBodyId generatedBodyId(PhysicsBodyId firstGeneratedBodyId,
                              std::size_t colliderIndex) {
  return {firstGeneratedBodyId.value +
          static_cast<std::uint32_t>(colliderIndex)};
}

PhysicsAabbCollider makeCollider(PhysicsBodyId bodyId,
                                 const Aabb3& bounds,
                                 bool sensor) {
  PhysicsAabbCollider collider;
  collider.bodyId = bodyId;
  collider.bounds = bounds;
  collider.worldCenterMeters = center(bounds);
  collider.halfExtentsMeters = extents(bounds);
  collider.sensor = sensor;
  return collider;
}

PhysicsSpatialSurfaceColliderBakeResult invalidSurfaceResult(
    PhysicsSpatialSurfaceColliderBakeStatus status,
    std::size_t index,
    const PhysicsSpatialSurfaceColliderBakeResult& partial) {
  PhysicsSpatialSurfaceColliderBakeResult result = bakeResult(status, false);
  result.surfaceCount = partial.surfaceCount;
  result.skippedSurfaceCount = partial.skippedSurfaceCount;
  result.invalidSurfaceIndex = index;
  return result;
}

Aabb3 boxBoundsForSurface(
    const CollisionSurfaceView& surface,
    const PhysicsSpatialSurfaceColliderBakeConfig& config) {
  const Vec3 surfaceCenter = center(surface.bounds);
  const Vec3 halfExtents =
      expandHalfExtents(extents(surface.bounds), config.minHalfExtentMeters);
  return aabbFromCenterExtents(surfaceCenter, halfExtents);
}

Aabb3 planeBoundsForSurface(
    const CollisionSurfaceView& surface,
    const PhysicsSpatialSurfaceColliderBakeConfig& config) {
  Aabb3 bounds = surface.bounds;
  const Vec3 halfExtents = extents(bounds);
  const float expandedHalfX =
      std::max(halfExtents.x, config.minHalfExtentMeters);
  const float expandedHalfZ =
      std::max(halfExtents.z, config.minHalfExtentMeters);
  const Vec3 xzCenter = center(bounds);
  bounds.min.x = xzCenter.x - expandedHalfX;
  bounds.max.x = xzCenter.x + expandedHalfX;
  bounds.min.z = xzCenter.z - expandedHalfZ;
  bounds.max.z = xzCenter.z + expandedHalfZ;

  // Up-facing walkable planes preserve the authored plane as the slab top face.
  // Down-facing planes preserve it as the bottom face for deterministic symmetry.
  // branch-gate: BG-1099
  if (surface.normal.y >= 0.0F) {
    const float topY = surface.bounds.max.y;
    bounds.max.y = topY;
    bounds.min.y = topY - config.planeThicknessMeters;
  } else {
    const float bottomY = surface.bounds.min.y;
    bounds.min.y = bottomY;
    bounds.max.y = bottomY + config.planeThicknessMeters;
  }
  return bounds;
}

bool supportedIncludedShape(const CollisionSurfaceView& surface) {
  // branch-gate: BG-1099
  if (surface.shape == CollisionSurfaceShape::Box) {
    return true;
  }
  return surface.shape == CollisionSurfaceShape::Plane &&
         surface.role == CollisionSurfaceRole::Walkable;
}

bool validGeneratedBodyId(PhysicsBodyId firstGeneratedBodyId,
                          std::size_t colliderIndex) {
  const std::uint64_t id =
      static_cast<std::uint64_t>(firstGeneratedBodyId.value) + colliderIndex;
  return id <= std::numeric_limits<std::uint32_t>::max() && id != 0U;
}

void appendCollider(
    PhysicsSpatialSurfaceColliderBakeResult* result,
    const CollisionSurfaceView& surface,
    std::size_t surfaceIndex,
    const PhysicsAabbCollider& collider) {
  result->colliders.push_back(collider);
  result->sourceSurfaceIndices.push_back(surfaceIndex);
  result->sourceSurfaceIds.push_back(surface.id);
  result->sourceRoles.push_back(surface.role);
  result->sourceShapes.push_back(surface.shape);
  result->runtimeOwnerStableNames.push_back(surface.runtimeOwnerStableName);
  result->includedSurfaceCount = result->colliders.size();
  result->colliderCount = result->colliders.size();
}

}  // namespace

std::string_view physicsSpatialSurfaceColliderBakeStatusName(
    PhysicsSpatialSurfaceColliderBakeStatus status) {
  static constexpr std::array<std::string_view, 6> kNames{
      "physics_spatial_surface_bake_baked",
      "physics_spatial_surface_bake_missing_surface_set",
      "physics_spatial_surface_bake_invalid_config",
      "physics_spatial_surface_bake_invalid_surface_bounds",
      "physics_spatial_surface_bake_invalid_surface_shape",
      "physics_spatial_surface_bake_invalid_generated_collider",
  };
  return enumName(status, kNames,
                  "physics_spatial_surface_bake_invalid_config");
}

bool isValidPhysicsSpatialSurfaceColliderBakeConfig(
    const PhysicsSpatialSurfaceColliderBakeConfig& config) {
  // branch-gate: BG-1099
  if (!positiveFinite(config.planeThicknessMeters)) {
    return false;
  }
  // branch-gate: BG-1099
  if (!positiveFinite(config.minHalfExtentMeters)) {
    return false;
  }
  return isValidPhysicsBodyId(config.firstGeneratedBodyId);
}

PhysicsSpatialSurfaceColliderBakeResult
bakePhysicsAabbCollidersFromSpatialSurfaces(
    const PhysicsSpatialSurfaceColliderBakeRequest& request) {
  // branch-gate: BG-1099
  if (request.surfaces == nullptr) {
    return bakeResult(
        PhysicsSpatialSurfaceColliderBakeStatus::MissingSurfaceSet, false);
  }
  // branch-gate: BG-1099
  if (!isValidPhysicsSpatialSurfaceColliderBakeConfig(request.config)) {
    return bakeResult(PhysicsSpatialSurfaceColliderBakeStatus::InvalidConfig,
                      false);
  }

  PhysicsSpatialSurfaceColliderBakeResult result =
      bakeResult(PhysicsSpatialSurfaceColliderBakeStatus::Baked, true);
  const std::span<const CollisionSurfaceView> surfaces =
      request.surfaces->surfaces();
  result.surfaceCount = surfaces.size();
  result.colliders.reserve(surfaces.size());
  result.sourceSurfaceIndices.reserve(surfaces.size());
  result.sourceSurfaceIds.reserve(surfaces.size());
  result.sourceRoles.reserve(surfaces.size());
  result.sourceShapes.reserve(surfaces.size());
  result.runtimeOwnerStableNames.reserve(surfaces.size());

  for (std::size_t surfaceIndex = 0U; surfaceIndex < surfaces.size();
       ++surfaceIndex) {
    const CollisionSurfaceView& surface = surfaces[surfaceIndex];
    // branch-gate: BG-1099
    if (!includeSurface(surface, request.config)) {
      ++result.skippedSurfaceCount;
      continue;
    }
    // branch-gate: BG-1099
    if (!isValid(surface.bounds) || !isFinite(surface.normal)) {
      return invalidSurfaceResult(
          PhysicsSpatialSurfaceColliderBakeStatus::InvalidSurfaceBounds,
          surfaceIndex, result);
    }
    // branch-gate: BG-1099
    if (!supportedIncludedShape(surface)) {
      return invalidSurfaceResult(
          PhysicsSpatialSurfaceColliderBakeStatus::InvalidSurfaceShape,
          surfaceIndex, result);
    }
    // branch-gate: BG-1099
    if (!validGeneratedBodyId(request.config.firstGeneratedBodyId,
                              result.colliders.size())) {
      return invalidSurfaceResult(
          PhysicsSpatialSurfaceColliderBakeStatus::InvalidGeneratedCollider,
          surfaceIndex, result);
    }

    const Aabb3 colliderBounds =
        surface.shape == CollisionSurfaceShape::Plane
            ? planeBoundsForSurface(surface, request.config)
            : boxBoundsForSurface(surface, request.config);
    const bool sensor = request.config.includeSensors && openingSurface(surface);
    const PhysicsAabbCollider collider = makeCollider(
        generatedBodyId(request.config.firstGeneratedBodyId,
                        result.colliders.size()),
        colliderBounds, sensor);
    // branch-gate: BG-1099
    if (!isValidPhysicsAabbCollider(collider)) {
      return invalidSurfaceResult(
          PhysicsSpatialSurfaceColliderBakeStatus::InvalidGeneratedCollider,
          surfaceIndex, result);
    }

    appendCollider(&result, surface, surfaceIndex, collider);
  }

  return result;
}

}  // namespace iggy3d
