#include "runtime/physics/PhysicsCollisionQueries.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

namespace iggy3d {
namespace {

constexpr float kDirectionEpsilonSquared = 0.000001F;
constexpr float kAxisEpsilon = 0.000001F;

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1097
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

PhysicsAabbOverlapQueryResult overlapResult(
    PhysicsCollisionQueryStatus status,
    bool ok) {
  PhysicsAabbOverlapQueryResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsCollisionQueryStatusName(status);
  return result;
}

PhysicsRaycastQueryResult raycastResult(
    PhysicsCollisionQueryStatus status,
    bool ok) {
  PhysicsRaycastQueryResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsCollisionQueryStatusName(status);
  return result;
}

PhysicsSweptAabbQueryResult sweptResult(
    PhysicsCollisionQueryStatus status,
    bool ok) {
  PhysicsSweptAabbQueryResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsCollisionQueryStatusName(status);
  return result;
}

PhysicsGroundCheckQueryResult groundResult(
    PhysicsCollisionQueryStatus status,
    bool ok) {
  PhysicsGroundCheckQueryResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsCollisionQueryStatusName(status);
  return result;
}

bool positiveFinite(float value) {
  return std::isfinite(value) && value > 0.0F;
}

bool nonNegativeFinite(float value) {
  return std::isfinite(value) && value >= 0.0F;
}

bool nonZeroFiniteVector(Vec3 value) {
  return isFinite(value) && lengthSquared(value) > kDirectionEpsilonSquared;
}


bool zeroVector(Vec3 value) {
  return lengthSquared(value) <= kDirectionEpsilonSquared;
}

Aabb3 expandedBounds(const PhysicsAabbCollider& target,
                     Vec3 halfExtentsMeters) {
  return makeAabb3(target.bounds.min - halfExtentsMeters,
                   target.bounds.max + halfExtentsMeters);
}

Aabb3 boundsExpandedByMargin(const PhysicsAabbCollider& target,
                             float marginMeters) {
  const Vec3 margin{marginMeters, marginMeters, marginMeters};
  return expandedBounds(target, margin);
}

PhysicsAabbOverlapQueryResult invalidColliderOverlapResult(
    std::size_t index) {
  PhysicsAabbOverlapQueryResult result =
      overlapResult(PhysicsCollisionQueryStatus::InvalidCollider, false);
  result.invalidColliderIndex = index;
  return result;
}

PhysicsRaycastQueryResult invalidColliderRaycastResult(std::size_t index) {
  PhysicsRaycastQueryResult result =
      raycastResult(PhysicsCollisionQueryStatus::InvalidCollider, false);
  result.invalidColliderIndex = index;
  return result;
}

PhysicsSweptAabbQueryResult invalidColliderSweepResult(std::size_t index) {
  PhysicsSweptAabbQueryResult result =
      sweptResult(PhysicsCollisionQueryStatus::InvalidCollider, false);
  result.invalidColliderIndex = index;
  return result;
}

PhysicsAabbOverlapQueryResult validateOverlapColliders(
    const std::vector<PhysicsAabbCollider>* colliders,
    const PhysicsAabbCollider* queryCollider) {
  // branch-gate: BG-1097
  if (colliders == nullptr) {
    return overlapResult(PhysicsCollisionQueryStatus::MissingColliders,
                         false);
  }
  // branch-gate: BG-1097
  if (queryCollider == nullptr) {
    return overlapResult(PhysicsCollisionQueryStatus::MissingQueryCollider,
                         false);
  }
  // branch-gate: BG-1097
  if (!isValidPhysicsAabbCollider(*queryCollider)) {
    return overlapResult(PhysicsCollisionQueryStatus::InvalidQueryCollider,
                         false);
  }
  for (std::size_t index = 0U; index < colliders->size(); ++index) {
    // branch-gate: BG-1097
    if (!isValidPhysicsAabbCollider((*colliders)[index])) {
      return invalidColliderOverlapResult(index);
    }
  }
  PhysicsAabbOverlapQueryResult result =
      overlapResult(PhysicsCollisionQueryStatus::Queried, true);
  result.colliderCount = colliders->size();
  return result;
}

PhysicsRaycastQueryResult validateRaycastRequest(
    const PhysicsRaycastQueryRequest& request) {
  // branch-gate: BG-1097
  if (request.colliders == nullptr) {
    return raycastResult(PhysicsCollisionQueryStatus::MissingColliders,
                         false);
  }
  // branch-gate: BG-1097
  if (!isFinite(request.originMeters)) {
    return raycastResult(PhysicsCollisionQueryStatus::InvalidRayOrigin,
                         false);
  }
  // branch-gate: BG-1097
  if (!nonZeroFiniteVector(request.direction)) {
    return raycastResult(PhysicsCollisionQueryStatus::InvalidRayDirection,
                         false);
  }
  // branch-gate: BG-1097
  if (!positiveFinite(request.maxDistanceMeters)) {
    return raycastResult(PhysicsCollisionQueryStatus::InvalidMaxDistance,
                         false);
  }
  for (std::size_t index = 0U; index < request.colliders->size(); ++index) {
    // branch-gate: BG-1097
    if (!isValidPhysicsAabbCollider((*request.colliders)[index])) {
      return invalidColliderRaycastResult(index);
    }
  }
  PhysicsRaycastQueryResult result =
      raycastResult(PhysicsCollisionQueryStatus::Queried, true);
  result.colliderCount = request.colliders->size();
  return result;
}

PhysicsSweptAabbQueryResult validateSweepRequest(
    const PhysicsSweptAabbQueryRequest& request) {
  // branch-gate: BG-1097
  if (request.colliders == nullptr) {
    return sweptResult(PhysicsCollisionQueryStatus::MissingColliders, false);
  }
  // branch-gate: BG-1097
  if (request.movingCollider == nullptr) {
    return sweptResult(PhysicsCollisionQueryStatus::MissingQueryCollider,
                       false);
  }
  // branch-gate: BG-1097
  if (!isValidPhysicsAabbCollider(*request.movingCollider)) {
    return sweptResult(PhysicsCollisionQueryStatus::InvalidQueryCollider,
                       false);
  }
  // branch-gate: BG-1097
  if (!isFinite(request.displacementMeters)) {
    return sweptResult(PhysicsCollisionQueryStatus::InvalidSweepDisplacement,
                       false);
  }
  for (std::size_t index = 0U; index < request.colliders->size(); ++index) {
    // branch-gate: BG-1097
    if (!isValidPhysicsAabbCollider((*request.colliders)[index])) {
      return invalidColliderSweepResult(index);
    }
  }
  PhysicsSweptAabbQueryResult result =
      sweptResult(PhysicsCollisionQueryStatus::Queried, true);
  result.colliderCount = request.colliders->size();
  return result;
}

PhysicsGroundCheckQueryResult validateGroundRequest(
    const PhysicsGroundCheckQueryRequest& request) {
  // branch-gate: BG-1097
  if (request.colliders == nullptr) {
    return groundResult(PhysicsCollisionQueryStatus::MissingColliders, false);
  }
  // branch-gate: BG-1097
  if (request.movingCollider == nullptr) {
    return groundResult(PhysicsCollisionQueryStatus::MissingQueryCollider,
                        false);
  }
  // branch-gate: BG-1097
  if (!isValidPhysicsAabbCollider(*request.movingCollider)) {
    return groundResult(PhysicsCollisionQueryStatus::InvalidQueryCollider,
                        false);
  }
  // branch-gate: BG-1097
  if (!nonZeroFiniteVector(request.downDirection)) {
    return groundResult(PhysicsCollisionQueryStatus::InvalidGroundDirection,
                        false);
  }
  // branch-gate: BG-1097
  if (!positiveFinite(request.probeDistanceMeters)) {
    return groundResult(PhysicsCollisionQueryStatus::InvalidProbeDistance,
                        false);
  }
  PhysicsGroundCheckQueryResult result =
      groundResult(PhysicsCollisionQueryStatus::Queried, true);
  result.colliderCount = request.colliders->size();
  return result;
}

void appendOverlapHit(PhysicsAabbOverlapQueryResult* result,
                      std::size_t colliderIndex,
                      const PhysicsAabbCollider& collider) {
  result->colliderIndices.push_back(colliderIndex);
  result->bodyIds.push_back(collider.bodyId);
  result->sensors.push_back(collider.sensor);
}

struct PhysicsRayAabbHit {
  bool hit = false;
  float distanceMeters = 0.0F;
  Vec3 pointMeters;
  Vec3 normalFromColliderToRay;
  bool startInside = false;
};

std::array<float, 3> components(Vec3 value) {
  return {value.x, value.y, value.z};
}

std::array<Vec3, 3> positiveNormals() {
  return {vec3UnitX(), vec3UnitY(), vec3UnitZ()};
}

Vec3 normalForAxis(std::size_t axisIndex, float sign) {
  const std::array<Vec3, 3> normals = positiveNormals();
  return normals[axisIndex] * sign;
}

Vec3 normalFromColliderToRay(Vec3 originMeters,
                             Vec3 normalizedDirection,
                             const Aabb3& bounds,
                             const AabbRayHit& coreHit) {
  if (coreHit.startInside) {
    return {};
  }

  const std::array<float, 3> origin = components(originMeters);
  const std::array<float, 3> direction = components(normalizedDirection);
  const std::array<float, 3> minBounds = components(bounds.min);
  const std::array<float, 3> maxBounds = components(bounds.max);
  Vec3 normal;
  float selectedNearDistance = 0.0F;

  // Core `intersectsRay(...)` owns hit/miss, distance, point, and start-inside
  // truth. Physics keeps only the legacy face-normal tie policy: choose the
  // axis that advanced entry distance, preserving axis order on equal entries.
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (std::fabs(direction[axis]) <= kAxisEpsilon) {
      continue;
    }

    const float inverseDirection = 1.0F / direction[axis];
    float nearDistance = (minBounds[axis] - origin[axis]) * inverseDirection;
    float farDistance = (maxBounds[axis] - origin[axis]) * inverseDirection;
    Vec3 nearNormal = normalForAxis(axis, -1.0F);
    Vec3 farNormal = normalForAxis(axis, 1.0F);
    if (nearDistance > farDistance) {
      std::swap(nearDistance, farDistance);
      std::swap(nearNormal, farNormal);
    }
    if (nearDistance > selectedNearDistance &&
        nearDistance <= coreHit.distanceMeters + kAxisEpsilon) {
      selectedNearDistance = nearDistance;
      normal = nearNormal;
    }
  }

  return normal;
}

PhysicsRayAabbHit intersectPhysicsRayAabb(Vec3 originMeters,
                                          Vec3 normalizedDirection,
                                          float maxDistanceMeters,
                                          const Aabb3& bounds) {
  const AabbRayHit coreHit =
      intersectsRay(bounds,
                    Ray3{originMeters, normalizedDirection},
                    maxDistanceMeters);
  if (!coreHit.hit) {
    return {};
  }

  PhysicsRayAabbHit result;
  result.hit = true;
  result.distanceMeters = coreHit.distanceMeters;
  result.pointMeters = coreHit.pointMeters;
  result.normalFromColliderToRay = normalFromColliderToRay(
      originMeters, normalizedDirection, bounds, coreHit);
  result.startInside = coreHit.startInside;
  return result;
}

PhysicsRaycastHit makeRaycastHit(std::size_t index,
                                 const PhysicsAabbCollider& collider,
                                 const PhysicsRayAabbHit& hit) {
  PhysicsRaycastHit result;
  result.colliderIndex = index;
  result.bodyId = collider.bodyId;
  result.distanceMeters = hit.distanceMeters;
  result.pointMeters = hit.pointMeters;
  result.normalFromColliderToRay = hit.normalFromColliderToRay;
  result.sensor = collider.sensor;
  result.startInside = hit.startInside;
  return result;
}

bool raycastHitLess(const PhysicsRaycastHit& lhs,
                    const PhysicsRaycastHit& rhs) {
  // branch-gate: BG-1097
  if (lhs.distanceMeters != rhs.distanceMeters) {
    return lhs.distanceMeters < rhs.distanceMeters;
  }
  return lhs.colliderIndex < rhs.colliderIndex;
}

PhysicsSweptAabbHit makeSweptHit(const PhysicsAabbCollider& movingCollider,
                                 const PhysicsAabbCollider& target,
                                 std::size_t targetIndex,
                                 float displacementLength,
                                 Vec3 sweepDirection,
                                 const PhysicsRaycastHit& rayHit) {
  PhysicsSweptAabbHit result;
  result.colliderIndex = targetIndex;
  result.bodyId = target.bodyId;
  result.distanceMeters = rayHit.distanceMeters;
  result.fraction = rayHit.distanceMeters / displacementLength;
  result.centerMeters =
      movingCollider.worldCenterMeters + sweepDirection * rayHit.distanceMeters;
  result.pointMeters = rayHit.pointMeters;
  result.normalFromColliderToMovingAabb = rayHit.normalFromColliderToRay;
  result.sensor = target.sensor;
  result.initialOverlap = rayHit.startInside;
  // branch-gate: BG-1097
  if (rayHit.startInside) {
    result.centerMeters = movingCollider.worldCenterMeters;
    result.fraction = 0.0F;
  }
  return result;
}

bool sweptHitLess(const PhysicsSweptAabbHit& lhs,
                  const PhysicsSweptAabbHit& rhs) {
  // branch-gate: BG-1097
  if (lhs.fraction != rhs.fraction) {
    return lhs.fraction < rhs.fraction;
  }
  return lhs.colliderIndex < rhs.colliderIndex;
}

Vec3 groundNormalForHit(const PhysicsSweptAabbHit& hit, Vec3 normalizedDown) {
  // branch-gate: BG-1097
  if (hit.initialOverlap &&
      zeroVector(hit.normalFromColliderToMovingAabb)) {
    return normalizedDown * -1.0F;
  }
  return hit.normalFromColliderToMovingAabb;
}

}  // namespace

std::string_view physicsCollisionQueryStatusName(
    PhysicsCollisionQueryStatus status) {
  static constexpr std::array<std::string_view, 11> kNames{
      "physics_collision_query_queried",
      "physics_collision_query_missing_colliders",
      "physics_collision_query_missing_query_collider",
      "physics_collision_query_invalid_collider",
      "physics_collision_query_invalid_query_collider",
      "physics_collision_query_invalid_ray_origin",
      "physics_collision_query_invalid_ray_direction",
      "physics_collision_query_invalid_max_distance",
      "physics_collision_query_invalid_sweep_displacement",
      "physics_collision_query_invalid_ground_direction",
      "physics_collision_query_invalid_probe_distance",
  };
  return enumName(status, kNames,
                  "physics_collision_query_invalid_collider");
}

PhysicsAabbOverlapQueryResult queryPhysicsAabbOverlaps(
    const PhysicsAabbOverlapQueryRequest& request) {
  PhysicsAabbOverlapQueryResult result =
      validateOverlapColliders(request.colliders, request.queryCollider);
  // branch-gate: BG-1097
  if (!result.ok) {
    return result;
  }

  for (std::size_t index = 0U; index < request.colliders->size(); ++index) {
    const PhysicsAabbCollider& collider = (*request.colliders)[index];
    // branch-gate: BG-1097
    if (collider.sensor && !request.includeSensors) {
      continue;
    }
    ++result.testedColliderCount;
    // branch-gate: BG-1097
    if (physicsAabbOverlaps(*request.queryCollider, collider)) {
      appendOverlapHit(&result, index, collider);
    }
  }
  result.hitCount = result.colliderIndices.size();
  return result;
}

PhysicsRaycastQueryResult raycastPhysicsAabbs(
    const PhysicsRaycastQueryRequest& request) {
  PhysicsRaycastQueryResult result = validateRaycastRequest(request);
  // branch-gate: BG-1097
  if (!result.ok) {
    return result;
  }

  const Vec3 direction = normalized(request.direction);
  for (std::size_t index = 0U; index < request.colliders->size(); ++index) {
    const PhysicsAabbCollider& collider = (*request.colliders)[index];
    // branch-gate: BG-1097
    if (collider.sensor && !request.includeSensors) {
      continue;
    }
    ++result.testedColliderCount;
    const PhysicsRayAabbHit rayHit = intersectPhysicsRayAabb(
        request.originMeters, direction, request.maxDistanceMeters,
        collider.bounds);
    // branch-gate: BG-1097
    if (rayHit.hit) {
      result.hits.push_back(makeRaycastHit(index, collider, rayHit));
    }
  }
  std::sort(result.hits.begin(), result.hits.end(), raycastHitLess);
  result.hitCount = result.hits.size();
  return result;
}

bool segmentHitsAnyPhysicsAabb(std::span<const PhysicsAabbCollider> colliders,
                               Vec3 from,
                               Vec3 to,
                               float marginMeters,
                               bool* startInside) {
  if (startInside != nullptr) {
    *startInside = false;
  }

  const Vec3 displacement = to - from;
  const float maxDistanceMeters = length(displacement);
  // branch-gate: BG-1097
  if (!isFinite(from) || !isFinite(to) || !nonNegativeFinite(marginMeters) ||
      !std::isfinite(maxDistanceMeters) ||
      maxDistanceMeters <= kDirectionEpsilonSquared) {
    return false;
  }

  const Vec3 direction = displacement / maxDistanceMeters;
  for (const PhysicsAabbCollider& collider : colliders) {
    // branch-gate: BG-1097
    if (collider.sensor || !isValidPhysicsAabbCollider(collider)) {
      continue;
    }
    const PhysicsRayAabbHit rayHit = intersectPhysicsRayAabb(
        from, direction, maxDistanceMeters,
        boundsExpandedByMargin(collider, marginMeters));
    // branch-gate: BG-1097
    if (rayHit.hit) {
      if (startInside != nullptr) {
        *startInside = physicsAabbContainsPoint(collider, from);
      }
      return true;
    }
  }
  return false;
}

PhysicsSweptAabbQueryResult sweepPhysicsAabb(
    const PhysicsSweptAabbQueryRequest& request) {
  PhysicsSweptAabbQueryResult result = validateSweepRequest(request);
  // branch-gate: BG-1097
  if (!result.ok) {
    return result;
  }

  // Zero-displacement sweeps intentionally report initial overlaps only.
  // This keeps stationary overlap checks deterministic without inventing
  // movement normals.
  // branch-gate: BG-1097
  if (zeroVector(request.displacementMeters)) {
    PhysicsAabbOverlapQueryRequest overlapRequest;
    overlapRequest.colliders = request.colliders;
    overlapRequest.queryCollider = request.movingCollider;
    overlapRequest.includeSensors = request.includeSensors;
    const PhysicsAabbOverlapQueryResult overlaps =
        queryPhysicsAabbOverlaps(overlapRequest);
    // branch-gate: BG-1097
    if (!overlaps.ok) {
      PhysicsSweptAabbQueryResult failed =
          sweptResult(overlaps.status, false);
      failed.reasonCode = overlaps.reasonCode;
      failed.invalidColliderIndex = overlaps.invalidColliderIndex;
      return failed;
    }
    result.testedColliderCount = overlaps.testedColliderCount;
    for (std::size_t hitIndex = 0U; hitIndex < overlaps.colliderIndices.size();
         ++hitIndex) {
      const std::size_t colliderIndex = overlaps.colliderIndices[hitIndex];
      const PhysicsAabbCollider& collider = (*request.colliders)[colliderIndex];
      PhysicsSweptAabbHit hit;
      hit.colliderIndex = colliderIndex;
      hit.bodyId = collider.bodyId;
      hit.centerMeters = request.movingCollider->worldCenterMeters;
      hit.pointMeters = request.movingCollider->worldCenterMeters;
      hit.sensor = collider.sensor;
      hit.initialOverlap = true;
      result.hits.push_back(hit);
    }
    result.hitCount = result.hits.size();
    result.hit = !result.hits.empty();
    // branch-gate: BG-1097
    if (result.hit) {
      result.nearestHit = result.hits.front();
    }
    return result;
  }

  const float displacementLength = length(request.displacementMeters);
  const Vec3 direction = request.displacementMeters / displacementLength;
  for (std::size_t index = 0U; index < request.colliders->size(); ++index) {
    const PhysicsAabbCollider& target = (*request.colliders)[index];
    // branch-gate: BG-1097
    if (target.sensor && !request.includeSensors) {
      continue;
    }
    ++result.testedColliderCount;

    const Aabb3 expanded = expandedBounds(
        target, request.movingCollider->halfExtentsMeters);
    const PhysicsRayAabbHit rayHit = intersectPhysicsRayAabb(
        request.movingCollider->worldCenterMeters, direction,
        displacementLength, expanded);
    // branch-gate: BG-1097
    if (rayHit.hit) {
      const PhysicsRaycastHit physicsRayHit =
          makeRaycastHit(index, target, rayHit);
      result.hits.push_back(makeSweptHit(*request.movingCollider, target,
                                         index, displacementLength, direction,
                                         physicsRayHit));
    }
  }
  std::sort(result.hits.begin(), result.hits.end(), sweptHitLess);
  result.hitCount = result.hits.size();
  result.hit = !result.hits.empty();
  // branch-gate: BG-1097
  if (result.hit) {
    result.nearestHit = result.hits.front();
  }
  return result;
}

PhysicsGroundCheckQueryResult checkPhysicsGround(
    const PhysicsGroundCheckQueryRequest& request) {
  PhysicsGroundCheckQueryResult result = validateGroundRequest(request);
  // branch-gate: BG-1097
  if (!result.ok) {
    return result;
  }

  const Vec3 down = normalized(request.downDirection);
  PhysicsSweptAabbQueryRequest sweepRequest;
  sweepRequest.colliders = request.colliders;
  sweepRequest.movingCollider = request.movingCollider;
  sweepRequest.displacementMeters = down * request.probeDistanceMeters;
  sweepRequest.includeSensors = request.includeSensors;
  const PhysicsSweptAabbQueryResult sweep = sweepPhysicsAabb(sweepRequest);
  // branch-gate: BG-1097
  if (!sweep.ok) {
    result.ok = false;
    result.status = sweep.status;
    result.reasonCode = sweep.reasonCode;
    result.testedColliderCount = sweep.testedColliderCount;
    return result;
  }

  result.testedColliderCount = sweep.testedColliderCount;
  result.hitCount = sweep.hitCount;
  result.grounded = sweep.hit;
  // branch-gate: BG-1097
  if (result.grounded) {
    result.nearestHit = sweep.nearestHit;
    result.groundDistanceMeters = sweep.nearestHit.distanceMeters;
    result.groundNormal = groundNormalForHit(sweep.nearestHit, down);
  }
  return result;
}

}  // namespace iggy3d
