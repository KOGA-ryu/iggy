#include "runtime/collision/CollisionQuery.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace iggy3d {
namespace {

constexpr float kEpsilon = 0.0001F;
constexpr std::array<std::array<std::size_t, 2U>, 4U>
    kHeightPatchCornerPairs{{
        {1U, 2U},
        {2U, 3U},
        {3U, 4U},
        {4U, 1U},
    }};

bool finiteSegment(Vec3 start, Vec3 end) {
  return isFinite(start) && isFinite(end) && distanceSquared(start, end) > kEpsilon * kEpsilon;
}

bool betterHit(float candidateTime, std::string_view candidateId, const CollisionQueryResult& current) {
  if (current.status != CollisionQueryStatus::Hit) {
    return true;
  }
  if (candidateTime < current.timeOfImpact - kEpsilon) {
    return true;
  }
  if (std::fabs(candidateTime - current.timeOfImpact) <= kEpsilon &&
      candidateId < current.surfaceId) {
    return true;
  }
  return false;
}

bool betterHeight(float candidateHeight, std::string_view candidateId, const CollisionQueryResult& current) {
  if (current.status != CollisionQueryStatus::Hit) {
    return true;
  }
  if (candidateHeight > current.heightMeters + kEpsilon) {
    return true;
  }
  if (std::fabs(candidateHeight - current.heightMeters) <= kEpsilon &&
      candidateId < current.surfaceId) {
    return true;
  }
  return false;
}

bool expandedContainsXZ(const Aabb3& bounds, Vec3 point, float tolerance) {
  return point.x >= bounds.min.x - tolerance && point.x <= bounds.max.x + tolerance &&
         point.z >= bounds.min.z - tolerance && point.z <= bounds.max.z + tolerance;
}

bool matchesKind(const CollisionSurfaceView& surface, CollisionQueryKind kind) {
  switch (kind) {
    case CollisionQueryKind::All:
      return true;
    case CollisionQueryKind::Actor:
      return surface.blocksActor || surface.hasActorMask;
    case CollisionQueryKind::Projectile:
      return surface.blocksProjectile || surface.hasProjectileMask;
    case CollisionQueryKind::Walkable:
      return surface.role == CollisionSurfaceRole::Walkable;
    case CollisionQueryKind::Opening:
      return surface.role == CollisionSurfaceRole::Opening;
  }
  return false;
}

CollisionQueryResult emptyResult(std::size_t checked = 0U, std::size_t blocking = 0U) {
  CollisionQueryResult result;
  result.status = CollisionQueryStatus::NoHit;
  result.checkedSurfaceCount = checked;
  result.blockingSurfaceCount = blocking;
  result.reasonCode = "collision_no_hit";
  return result;
}

CollisionQueryResult invalidResult() {
  CollisionQueryResult result;
  result.status = CollisionQueryStatus::InvalidInput;
  result.reasonCode = "collision_invalid_input";
  return result;
}

CollisionQueryResult emptySetResult() {
  CollisionQueryResult result;
  result.status = CollisionQueryStatus::EmptySurfaceSet;
  result.reasonCode = "collision_empty_surface_set";
  return result;
}

void fillHit(CollisionQueryResult& result,
             const CollisionSurfaceView& surface,
             Vec3 point,
             float timeOfImpact,
             float distanceMeters) {
  result.status = CollisionQueryStatus::Hit;
  result.surfaceId = surface.id;
  result.role = surface.role;
  result.shape = surface.shape;
  result.pointMeters = point;
  result.normal = surface.normal;
  result.distanceMeters = distanceMeters;
  result.timeOfImpact = timeOfImpact;
  result.heightMeters = point.y;
  result.reasonCode = "collision_hit";
}

void fillHitWithNormal(CollisionQueryResult& result,
                       const CollisionSurfaceView& surface,
                       Vec3 point,
                       Vec3 normal,
                       float timeOfImpact,
                       float distanceMeters) {
  fillHit(result, surface, point, timeOfImpact, distanceMeters);
  result.normal = normal;
}

struct SurfacePointSample {
  Vec3 point;
  Vec3 normal;
};

[[nodiscard]] bool sampleTriangleAtXZ(Vec3 samplePoint,
                                      Vec3 first,
                                      Vec3 second,
                                      Vec3 third,
                                      SurfacePointSample& output) {
  const float denominator =
      (second.z - third.z) * (first.x - third.x) +
      (third.x - second.x) * (first.z - third.z);
  if (!std::isfinite(denominator) || std::fabs(denominator) <= kEpsilon) {
    return false;
  }
  const float firstWeight =
      ((second.z - third.z) * (samplePoint.x - third.x) +
       (third.x - second.x) * (samplePoint.z - third.z)) /
      denominator;
  const float secondWeight =
      ((third.z - first.z) * (samplePoint.x - third.x) +
       (first.x - third.x) * (samplePoint.z - third.z)) /
      denominator;
  const float thirdWeight = 1.0F - firstWeight - secondWeight;
  if (firstWeight < -kEpsilon || secondWeight < -kEpsilon ||
      thirdWeight < -kEpsilon) {
    return false;
  }

  Vec3 normal;
  if (!tryNormalize(cross(second - first, third - first), normal)) {
    return false;
  }
  if (normal.y < 0.0F) {
    normal = normal * -1.0F;
  }
  const float height = first.y * firstWeight + second.y * secondWeight +
                       third.y * thirdWeight;
  if (!std::isfinite(height) || !isFinite(normal)) {
    return false;
  }
  output.point = {samplePoint.x, height, samplePoint.z};
  output.normal = normal;
  return true;
}

[[nodiscard]] bool sampleHeightPatchAtXZ(
    const CollisionSurfaceView& surface,
    Vec3 worldPoint,
    float tolerance,
    SurfacePointSample& output) {
  if (!expandedContainsXZ(surface.bounds, worldPoint, tolerance)) {
    return false;
  }
  const Vec3 samplePoint{
      std::clamp(worldPoint.x, surface.bounds.min.x, surface.bounds.max.x),
      0.0F,
      std::clamp(worldPoint.z, surface.bounds.min.z, surface.bounds.max.z)};
  for (const auto& pair : kHeightPatchCornerPairs) {
    if (sampleTriangleAtXZ(samplePoint, surface.heightPatchPoints[0],
                           surface.heightPatchPoints[pair[0]],
                           surface.heightPatchPoints[pair[1]], output)) {
      output.point.x = worldPoint.x;
      output.point.z = worldPoint.z;
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool sampleSurfaceAtXZ(const CollisionSurfaceView& surface,
                                     Vec3 worldPoint,
                                     float tolerance,
                                     SurfacePointSample& output) {
  if (surface.shape == CollisionSurfaceShape::HeightPatch) {
    return sampleHeightPatchAtXZ(surface, worldPoint, tolerance, output);
  }
  if (!expandedContainsXZ(surface.bounds, worldPoint, tolerance) ||
      std::fabs(surface.normal.y) <= kEpsilon) {
    return false;
  }
  const float height =
      surface.planePoint.y -
      ((surface.normal.x * (worldPoint.x - surface.planePoint.x)) +
       (surface.normal.z * (worldPoint.z - surface.planePoint.z))) /
          surface.normal.y;
  if (!std::isfinite(height)) {
    return false;
  }
  output.point = {worldPoint.x, height, worldPoint.z};
  output.normal = surface.normal;
  return true;
}

[[nodiscard]] CollisionQueryResult sampleWalkableHeight(
    const SpatialSurfaceSet& surfaces,
    Vec3 worldPoint,
    float footprintToleranceMeters,
    const float* maxHeightMeters) {
  CollisionQueryResult result = emptyResult();
  for (const CollisionSurfaceView& surface : surfaces.surfaces()) {
    ++result.checkedSurfaceCount;
    if (surface.role != CollisionSurfaceRole::Walkable) {
      continue;
    }
    ++result.blockingSurfaceCount;
    SurfacePointSample sample;
    if (!sampleSurfaceAtXZ(surface, worldPoint, footprintToleranceMeters,
                           sample) ||
        (maxHeightMeters != nullptr &&
         sample.point.y > *maxHeightMeters + kEpsilon) ||
        !betterHeight(sample.point.y, surface.id, result)) {
      continue;
    }
    fillHitWithNormal(result, surface, sample.point, sample.normal, 0.0F,
                      0.0F);
  }
  return result;
}

bool segmentAabb(Vec3 start, Vec3 end, const Aabb3& bounds, float& outTime) {
  const Vec3 delta = end - start;
  float tMin = 0.0F;
  float tMax = 1.0F;

  const auto testAxis = [&](float origin, float direction, float minValue, float maxValue) {
    if (std::fabs(direction) <= kEpsilon) {
      return origin >= minValue - kEpsilon && origin <= maxValue + kEpsilon;
    }
    const float inv = 1.0F / direction;
    float t1 = (minValue - origin) * inv;
    float t2 = (maxValue - origin) * inv;
    if (t1 > t2) {
      std::swap(t1, t2);
    }
    tMin = std::max(tMin, t1);
    tMax = std::min(tMax, t2);
    return tMin <= tMax + kEpsilon;
  };

  if (!testAxis(start.x, delta.x, bounds.min.x, bounds.max.x) ||
      !testAxis(start.y, delta.y, bounds.min.y, bounds.max.y) ||
      !testAxis(start.z, delta.z, bounds.min.z, bounds.max.z)) {
    return false;
  }
  outTime = std::clamp(tMin, 0.0F, 1.0F);
  return true;
}

bool segmentPlaneFootprint(Vec3 start,
                           Vec3 end,
                           const CollisionSurfaceView& surface,
                           float& outTime,
                           Vec3& outPoint) {
  const Vec3 delta = end - start;
  const float denom = dot(surface.normal, delta);
  if (std::fabs(denom) <= kEpsilon) {
    return false;
  }
  const float t = dot(surface.normal, surface.planePoint - start) / denom;
  if (t < -kEpsilon || t > 1.0F + kEpsilon) {
    return false;
  }
  const Vec3 point = start + delta * std::clamp(t, 0.0F, 1.0F);
  if (!contains(surface.bounds, closestPoint(surface.bounds, point)) ||
      point.x < surface.bounds.min.x - kEpsilon || point.x > surface.bounds.max.x + kEpsilon ||
      point.y < surface.bounds.min.y - kEpsilon || point.y > surface.bounds.max.y + kEpsilon ||
      point.z < surface.bounds.min.z - kEpsilon || point.z > surface.bounds.max.z + kEpsilon) {
    return false;
  }
  outTime = std::clamp(t, 0.0F, 1.0F);
  outPoint = point;
  return true;
}

bool segmentHeightPatch(Vec3 start,
                        Vec3 end,
                        const CollisionSurfaceView& surface,
                        float& outTime,
                        Vec3& outPoint,
                        Vec3& outNormal) {
  const Vec3 delta = end - start;
  bool found = false;
  float nearestTime = 1.0F;
  for (const auto& pair : kHeightPatchCornerPairs) {
    const Vec3 first = surface.heightPatchPoints[0];
    const Vec3 second = surface.heightPatchPoints[pair[0]];
    const Vec3 third = surface.heightPatchPoints[pair[1]];
    Vec3 normal;
    if (!tryNormalize(cross(second - first, third - first), normal)) {
      continue;
    }
    if (normal.y < 0.0F) {
      normal = normal * -1.0F;
    }
    const float denominator = dot(normal, delta);
    if (std::fabs(denominator) <= kEpsilon) {
      continue;
    }
    const float time = dot(normal, first - start) / denominator;
    if (time < -kEpsilon || time > 1.0F + kEpsilon ||
        (found && time >= nearestTime - kEpsilon)) {
      continue;
    }
    const Vec3 point = start + delta * std::clamp(time, 0.0F, 1.0F);
    SurfacePointSample triangleSample;
    if (!sampleTriangleAtXZ(point, first, second, third, triangleSample)) {
      continue;
    }
    found = true;
    nearestTime = std::clamp(time, 0.0F, 1.0F);
    outPoint = point;
    outNormal = normal;
  }
  outTime = nearestTime;
  return found;
}

CollisionQueryResult overlapResult(const SpatialSurfaceSet& surfaces,
                                   CollisionQueryKind kind,
                                   const auto& overlaps) {
  if (surfaces.empty()) {
    return emptySetResult();
  }
  CollisionQueryResult result = emptyResult();
  for (const CollisionSurfaceView& surface : surfaces.surfaces()) {
    ++result.checkedSurfaceCount;
    if (!matchesKind(surface, kind)) {
      continue;
    }
    ++result.blockingSurfaceCount;
    if (!overlaps(surface)) {
      continue;
    }
    if (result.status != CollisionQueryStatus::Hit || surface.id < result.surfaceId) {
      fillHit(result, surface, center(surface.bounds), 0.0F, 0.0F);
    }
  }
  return result;
}

}  // namespace

std::string_view collisionQueryStatusName(CollisionQueryStatus status) {
  switch (status) {
    case CollisionQueryStatus::Hit:
      return "hit";
    case CollisionQueryStatus::NoHit:
      return "no_hit";
    case CollisionQueryStatus::InvalidInput:
      return "invalid_input";
    case CollisionQueryStatus::EmptySurfaceSet:
      return "empty_surface_set";
  }
  return "invalid_input";
}

std::string_view collisionSurfaceRoleName(CollisionSurfaceRole role) {
  switch (role) {
    case CollisionSurfaceRole::Walkable:
      return "walkable";
    case CollisionSurfaceRole::Blocker:
      return "blocker";
    case CollisionSurfaceRole::ProjectileBlocker:
      return "projectile_blocker";
    case CollisionSurfaceRole::Opening:
      return "opening";
  }
  return "walkable";
}

std::string_view collisionSurfaceShapeName(CollisionSurfaceShape shape) {
  switch (shape) {
    case CollisionSurfaceShape::Box:
      return "box";
    case CollisionSurfaceShape::Plane:
      return "plane";
    case CollisionSurfaceShape::HeightPatch:
      return "height_patch";
    case CollisionSurfaceShape::Opening:
      return "opening";
  }
  return "plane";
}

CollisionQueryResult sampleSurfaceHeight(const SpatialSurfaceSet& surfaces,
                                         Vec3 worldPoint,
                                         float footprintToleranceMeters) {
  if (surfaces.empty()) {
    return emptySetResult();
  }
  if (!isFinite(worldPoint) || !std::isfinite(footprintToleranceMeters) ||
      footprintToleranceMeters < 0.0F) {
    return invalidResult();
  }
  return sampleWalkableHeight(surfaces, worldPoint,
                              footprintToleranceMeters, nullptr);
}

CollisionQueryResult sampleSurfaceHeightAtOrBelow(
    const SpatialSurfaceSet& surfaces,
    Vec3 worldPoint,
    float maxHeightMeters,
    float footprintToleranceMeters) {
  // branch-gate: BG-1191
  if (surfaces.empty()) {
    return emptySetResult();
  }
  // branch-gate: BG-1191
  if (!isFinite(worldPoint) || !std::isfinite(maxHeightMeters) ||
      !std::isfinite(footprintToleranceMeters) ||
      footprintToleranceMeters < 0.0F) {
    return invalidResult();
  }
  return sampleWalkableHeight(surfaces, worldPoint,
                              footprintToleranceMeters, &maxHeightMeters);
}

CollisionQueryResult sampleSurfaceNormal(const SpatialSurfaceSet& surfaces,
                                         Vec3 worldPoint,
                                         float footprintToleranceMeters) {
  CollisionQueryResult result = sampleSurfaceHeight(surfaces, worldPoint, footprintToleranceMeters);
  if (result.status == CollisionQueryStatus::Hit) {
    result.reasonCode = "collision_hit";
  }
  return result;
}

CollisionQueryResult querySegment(const SpatialSurfaceSet& surfaces,
                                  Vec3 start,
                                  Vec3 end,
                                  CollisionQueryKind kind) {
  if (surfaces.empty()) {
    return emptySetResult();
  }
  if (!finiteSegment(start, end)) {
    return invalidResult();
  }
  const float segmentLength = std::sqrt(distanceSquared(start, end));
  CollisionQueryResult result = emptyResult();
  for (const CollisionSurfaceView& surface : surfaces.surfaces()) {
    ++result.checkedSurfaceCount;
    if (!matchesKind(surface, kind)) {
      continue;
    }
    ++result.blockingSurfaceCount;

    float time = 0.0F;
    Vec3 point;
    bool hit = false;
    Vec3 hitNormal = surface.normal;
    if (surface.shape == CollisionSurfaceShape::Box) {
      hit = segmentAabb(start, end, surface.bounds, time);
      point = start + (end - start) * time;
    } else if (surface.shape == CollisionSurfaceShape::HeightPatch) {
      hit = segmentHeightPatch(start, end, surface, time, point, hitNormal);
    } else {
      hit = segmentPlaneFootprint(start, end, surface, time, point);
    }
    if (hit && betterHit(time, surface.id, result)) {
      fillHitWithNormal(result, surface, point, hitNormal, time,
                        segmentLength * time);
    }
  }
  return result;
}

CollisionQueryResult queryPointOverlap(const SpatialSurfaceSet& surfaces,
                                       Vec3 point,
                                       CollisionQueryKind kind) {
  if (!isFinite(point)) {
    return invalidResult();
  }
  return overlapResult(surfaces, kind, [&](const CollisionSurfaceView& surface) {
    return contains(surface.bounds, point);
  });
}

CollisionQueryResult queryAabbOverlap(const SpatialSurfaceSet& surfaces,
                                      const Aabb3& bounds,
                                      CollisionQueryKind kind) {
  if (!isValid(bounds)) {
    return invalidResult();
  }
  return overlapResult(surfaces, kind, [&](const CollisionSurfaceView& surface) {
    return intersects(surface.bounds, bounds);
  });
}

}  // namespace iggy3d
