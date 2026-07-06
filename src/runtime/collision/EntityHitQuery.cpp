#include "runtime/collision/EntityHitQuery.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace iggy3d {
namespace {

constexpr float kEntityHitEpsilon = 0.0001F;

struct SegmentAabbHit {
  bool hit = false;
  float time = 0.0F;
  Vec3 normal;
};

bool finiteSegment(Vec3 start, Vec3 end) {
  return isFinite(start) && isFinite(end) &&
         distanceSquared(start, end) > kEntityHitEpsilon * kEntityHitEpsilon;
}

Aabb3 expanded(Aabb3 bounds, float radiusMeters) {
  const Vec3 radius{radiusMeters, radiusMeters, radiusMeters};
  return {bounds.min - radius, bounds.max + radius};
}

Aabb3 worldBoundsForEntity(const EntityState& entity) {
  const Vec3 worldMin =
      transformPointScaleTranslate(entity.transform, entity.localBounds.min);
  const Vec3 worldMax =
      transformPointScaleTranslate(entity.transform, entity.localBounds.max);
  return {{std::min(worldMin.x, worldMax.x),
           std::min(worldMin.y, worldMax.y),
           std::min(worldMin.z, worldMax.z)},
          {std::max(worldMin.x, worldMax.x),
           std::max(worldMin.y, worldMax.y),
           std::max(worldMin.z, worldMax.z)}};
}

bool candidateEntity(const EntityState& entity, const EntityHitQueryRequest& request) {
  if (!entity.active || !isValid(entity.id) || entity.id == request.ignoredEntity ||
      !isValid(entity.localBounds)) {
    return false;
  }
  if (request.requireAttackTarget &&
      !isTargetActionSupported(entity.targeting, TargetAction::Attack)) {
    return false;
  }
  return true;
}

bool betterHit(const EntityHitQueryResult& current,
               float candidateTime,
               std::string_view candidateStableName) {
  if (current.status != EntityHitStatus::Hit) {
    return true;
  }
  if (candidateTime < current.timeOfImpact - kEntityHitEpsilon) {
    return true;
  }
  return std::fabs(candidateTime - current.timeOfImpact) <= kEntityHitEpsilon &&
         candidateStableName < current.stableName;
}

bool testAxis(float origin,
              float direction,
              float minValue,
              float maxValue,
              Vec3 nearNormal,
              float& tMin,
              float& tMax,
              Vec3& normal) {
  if (std::fabs(direction) <= kEntityHitEpsilon) {
    return origin >= minValue - kEntityHitEpsilon &&
           origin <= maxValue + kEntityHitEpsilon;
  }

  const float inv = 1.0F / direction;
  float tNear = (minValue - origin) * inv;
  float tFar = (maxValue - origin) * inv;
  Vec3 entryNormal = nearNormal;
  if (tNear > tFar) {
    std::swap(tNear, tFar);
    entryNormal = entryNormal * -1.0F;
  }
  if (tNear > tMin) {
    tMin = tNear;
    normal = entryNormal;
  }
  tMax = std::min(tMax, tFar);
  return tMin <= tMax + kEntityHitEpsilon;
}

SegmentAabbHit segmentAabb(Vec3 start, Vec3 end, const Aabb3& bounds) {
  SegmentAabbHit result;
  const Vec3 delta = end - start;
  float tMin = 0.0F;
  float tMax = 1.0F;
  Vec3 normal;

  if (!testAxis(start.x, delta.x, bounds.min.x, bounds.max.x, {-1.0F, 0.0F, 0.0F},
                tMin, tMax, normal) ||
      !testAxis(start.y, delta.y, bounds.min.y, bounds.max.y, {0.0F, -1.0F, 0.0F},
                tMin, tMax, normal) ||
      !testAxis(start.z, delta.z, bounds.min.z, bounds.max.z, {0.0F, 0.0F, -1.0F},
                tMin, tMax, normal)) {
    return result;
  }

  result.hit = true;
  result.time = std::clamp(tMin, 0.0F, 1.0F);
  result.normal = normal;
  return result;
}

EntityHitQueryResult invalidResult() {
  EntityHitQueryResult result;
  result.status = EntityHitStatus::InvalidInput;
  result.reasonCode = "entity_hit_invalid_input";
  return result;
}

EntityHitQueryResult missingWorldResult() {
  EntityHitQueryResult result;
  result.status = EntityHitStatus::MissingWorld;
  result.reasonCode = "entity_hit_missing_world";
  return result;
}

}  // namespace

std::string_view entityHitStatusName(EntityHitStatus status) {
  switch (status) {
    case EntityHitStatus::Hit:
      return "hit";
    case EntityHitStatus::NoHit:
      return "no_hit";
    case EntityHitStatus::InvalidInput:
      return "invalid_input";
    case EntityHitStatus::MissingWorld:
      return "missing_world";
  }
  return "invalid_input";
}

EntityHitQueryResult queryFirstEntityHit(const EntityHitQueryRequest& request) {
  if (request.world == nullptr) {
    return missingWorldResult();
  }
  if (!finiteSegment(request.startMeters, request.endMeters) ||
      !std::isfinite(request.radiusMeters) || request.radiusMeters < 0.0F) {
    return invalidResult();
  }

  EntityHitQueryResult result;
  result.status = EntityHitStatus::NoHit;
  result.reasonCode = "entity_hit_no_hit";
  const float segmentLength = std::sqrt(distanceSquared(request.startMeters, request.endMeters));
  for (const EntityState& entity : request.world->entities()) {
    ++result.checkedEntityCount;
    if (!candidateEntity(entity, request)) {
      continue;
    }
    ++result.candidateEntityCount;
    const Aabb3 bounds = expanded(worldBoundsForEntity(entity), request.radiusMeters);
    if (!isValid(bounds)) {
      continue;
    }
    const SegmentAabbHit hit = segmentAabb(request.startMeters, request.endMeters, bounds);
    if (!hit.hit || !betterHit(result, hit.time, entity.stableName)) {
      continue;
    }
    result.status = EntityHitStatus::Hit;
    result.entity = entity.id;
    result.stableName = entity.stableName;
    result.worldBounds = bounds;
    result.timeOfImpact = hit.time;
    result.distanceMeters = segmentLength * hit.time;
    result.pointMeters = request.startMeters + (request.endMeters - request.startMeters) * hit.time;
    result.normal = hit.normal;
    result.reasonCode = "entity_hit";
  }
  return result;
}

}  // namespace iggy3d
