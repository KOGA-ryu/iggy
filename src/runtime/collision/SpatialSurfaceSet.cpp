#include "runtime/collision/SpatialSurfaceSet.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace iggy3d {
namespace {

CollisionSurfaceShape toCollisionShape(RoomSpatialSurfaceShape shape) {
  switch (shape) {
    case RoomSpatialSurfaceShape::Box:
      return CollisionSurfaceShape::Box;
    case RoomSpatialSurfaceShape::Plane:
      return CollisionSurfaceShape::Plane;
    case RoomSpatialSurfaceShape::HeightPatch:
      return CollisionSurfaceShape::HeightPatch;
    case RoomSpatialSurfaceShape::Opening:
      return CollisionSurfaceShape::Opening;
  }
  return CollisionSurfaceShape::Plane;
}

CollisionSurfaceRole toCollisionRole(RoomSpatialSurfaceRole role) {
  switch (role) {
    case RoomSpatialSurfaceRole::Walkable:
      return CollisionSurfaceRole::Walkable;
    case RoomSpatialSurfaceRole::Blocker:
      return CollisionSurfaceRole::Blocker;
    case RoomSpatialSurfaceRole::ProjectileBlocker:
      return CollisionSurfaceRole::ProjectileBlocker;
    case RoomSpatialSurfaceRole::Opening:
      return CollisionSurfaceRole::Opening;
  }
  return CollisionSurfaceRole::Walkable;
}

bool contains(const std::vector<std::string>& values, std::string_view expected) {
  for (const std::string& value : values) {
    if (value == expected) {
      return true;
    }
  }
  return false;
}

bool buildBounds(std::span<const Vec3> points, Vec3 worldOffsetMeters, Aabb3& out) {
  if (points.empty()) {
    return false;
  }
  Vec3 minPoint{std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()};
  Vec3 maxPoint{-std::numeric_limits<float>::max(),
                -std::numeric_limits<float>::max(),
                -std::numeric_limits<float>::max()};
  for (const Vec3 rawPoint : points) {
    const Vec3 point = rawPoint + worldOffsetMeters;
    if (!isFinite(point)) {
      return false;
    }
    minPoint.x = std::min(minPoint.x, point.x);
    minPoint.y = std::min(minPoint.y, point.y);
    minPoint.z = std::min(minPoint.z, point.z);
    maxPoint.x = std::max(maxPoint.x, point.x);
    maxPoint.y = std::max(maxPoint.y, point.y);
    maxPoint.z = std::max(maxPoint.z, point.z);
  }
  out = makeAabb3(minPoint, maxPoint);
  return isValid(out);
}

bool normalized(Vec3 value, Vec3& out) {
  if (!isFinite(value)) {
    return false;
  }
  const float lenSq = lengthSquared(value);
  if (lenSq <= 0.000001F) {
    return false;
  }
  out = value / std::sqrt(lenSq);
  return isFinite(out);
}

}  // namespace

std::span<const CollisionSurfaceView> SpatialSurfaceSet::surfaces() const {
  return surfaces_;
}

bool SpatialSurfaceSet::empty() const {
  return surfaces_.empty();
}

std::size_t SpatialSurfaceSet::size() const {
  return surfaces_.size();
}

SpatialSurfaceSet buildSpatialSurfaceSet(const RoomAsset& room) {
  return buildSpatialSurfaceSet(room, {});
}

SpatialSurfaceSet buildSpatialSurfaceSet(const RoomAsset& room, Vec3 worldOffsetMeters) {
  SpatialSurfaceSet set;
  set.surfaces_.reserve(room.spatialSurfaces.size());
  for (const RoomSpatialSurface& surface : room.spatialSurfaces) {
    Aabb3 bounds;
    Vec3 normal;
    const bool heightPatch =
        surface.shape == RoomSpatialSurfaceShape::HeightPatch;
    if (surface.id.empty() || surface.pointsMeters.empty() ||
        (heightPatch && surface.pointsMeters.size() != 5U) ||
        !buildBounds(surface.pointsMeters, worldOffsetMeters, bounds) ||
        !normalized(surface.normal, normal)) {
      continue;
    }
    CollisionSurfaceView view;
    view.id = surface.id;
    view.shape = toCollisionShape(surface.shape);
    view.role = toCollisionRole(surface.role);
    view.bounds = bounds;
    view.normal = normal;
    view.planePoint = surface.pointsMeters.front() + worldOffsetMeters;
    if (heightPatch) {
      for (std::size_t index = 0U; index < view.heightPatchPoints.size();
           ++index) {
        view.heightPatchPoints[index] =
            surface.pointsMeters[index] + worldOffsetMeters;
      }
    }
    view.blocksActor = surface.blocksActor;
    view.blocksProjectile = surface.blocksProjectile;
    view.hasActorMask = contains(surface.collisionMask, "actor");
    view.hasProjectileMask = contains(surface.collisionMask, "projectile");
    view.opening = surface.role == RoomSpatialSurfaceRole::Opening;
    view.runtimeOwnerStableName = surface.runtimeOwnerStableName;
    view.traversalTags = surface.traversalTags;
    set.surfaces_.push_back(view);
  }
  return set;
}

}  // namespace iggy3d
