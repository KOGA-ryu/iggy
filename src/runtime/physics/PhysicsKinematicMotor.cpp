#include "runtime/physics/PhysicsKinematicMotor.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace iggy3d {
namespace {

constexpr float kDirectionEpsilonSquared = 0.000001F;

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1098
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

PhysicsKinematicMotorResult motorResult(PhysicsKinematicMotorStatus status,
                                        bool ok) {
  PhysicsKinematicMotorResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsKinematicMotorStatusName(status);
  return result;
}

bool nonNegativeFinite(float value) {
  return std::isfinite(value) && value >= 0.0F;
}

bool positiveFinite(float value) {
  return std::isfinite(value) && value > 0.0F;
}

float vectorLength(Vec3 value) {
  return std::sqrt(lengthSquared(value));
}

Vec3 normalized(Vec3 value) {
  return value / vectorLength(value);
}

bool belowMinMove(Vec3 value, float minMoveDistanceMeters) {
  return lengthSquared(value) <=
         minMoveDistanceMeters * minMoveDistanceMeters;
}

PhysicsAabbCollider movedCollider(const PhysicsAabbCollider& collider,
                                  Vec3 centerMeters) {
  PhysicsAabbCollider moved = collider;
  moved.worldCenterMeters = centerMeters;
  moved.bounds = aabbFromCenterExtents(centerMeters, collider.halfExtentsMeters);
  return moved;
}

Vec3 slideAlongCollisionPlane(Vec3 remainingDisplacementMeters, Vec3 normal) {
  // branch-gate: BG-1098
  if (lengthSquared(normal) <= kDirectionEpsilonSquared) {
    return vec3Zero();
  }
  const float intoSurface =
      std::min(dot(remainingDisplacementMeters, normal), 0.0F);
  return remainingDisplacementMeters - normal * intoSurface;
}

PhysicsKinematicMotorHit makeMotorHit(const PhysicsSweptAabbHit& hit) {
  PhysicsKinematicMotorHit result;
  result.colliderIndex = hit.colliderIndex;
  result.bodyId = hit.bodyId;
  result.fraction = hit.fraction;
  result.distanceMeters = hit.distanceMeters;
  result.normalFromColliderToMotor = hit.normalFromColliderToMovingAabb;
  result.centerMeters = hit.centerMeters;
  result.sensor = hit.sensor;
  result.initialOverlap = hit.initialOverlap;
  return result;
}

PhysicsKinematicMotorResult failedFromCollisionQuery(
    std::string_view upstreamReasonCode,
    std::size_t invalidColliderIndex) {
  PhysicsKinematicMotorResult result =
      motorResult(PhysicsKinematicMotorStatus::CollisionQueryFailed, false);
  result.upstreamReasonCode = upstreamReasonCode;
  result.invalidColliderIndex = invalidColliderIndex;
  return result;
}

PhysicsKinematicMotorResult validateRequest(
    const PhysicsKinematicMotorRequest& request) {
  // branch-gate: BG-1098
  if (!isValidPhysicsKinematicMotorConfig(request.config)) {
    return motorResult(PhysicsKinematicMotorStatus::InvalidConfig, false);
  }
  // branch-gate: BG-1098
  if (request.colliders == nullptr) {
    return motorResult(PhysicsKinematicMotorStatus::MissingColliders, false);
  }
  // branch-gate: BG-1098
  if (request.bodyCollider == nullptr) {
    return motorResult(PhysicsKinematicMotorStatus::MissingBodyCollider,
                       false);
  }
  // branch-gate: BG-1098
  if (!isValidPhysicsAabbCollider(*request.bodyCollider)) {
    return motorResult(PhysicsKinematicMotorStatus::InvalidBodyCollider,
                       false);
  }
  // branch-gate: BG-1098
  if (!isFinite(request.desiredDisplacementMeters)) {
    return motorResult(
        PhysicsKinematicMotorStatus::InvalidDesiredDisplacement, false);
  }
  // branch-gate: BG-1098
  if (vectorLength(request.desiredDisplacementMeters) >
      request.config.maxMoveDistanceMeters) {
    return motorResult(PhysicsKinematicMotorStatus::MaxDisplacementExceeded,
                       false);
  }

  const PhysicsAabbOverlapQueryResult validation =
      queryPhysicsAabbOverlaps({request.colliders, request.bodyCollider, true});
  // branch-gate: BG-1098
  if (!validation.ok) {
    return failedFromCollisionQuery(validation.reasonCode,
                                    validation.invalidColliderIndex);
  }

  return motorResult(PhysicsKinematicMotorStatus::Planned, true);
}

PhysicsKinematicMotorResult groundQueryFailed(
    const PhysicsGroundCheckQueryResult& ground) {
  PhysicsKinematicMotorResult result =
      motorResult(PhysicsKinematicMotorStatus::GroundQueryFailed, false);
  result.upstreamReasonCode = ground.reasonCode;
  return result;
}

PhysicsGroundCheckQueryResult runGroundCheck(
    const PhysicsKinematicMotorRequest& request,
    const PhysicsAabbCollider& finalCollider,
    float probeDistanceMeters) {
  PhysicsGroundCheckQueryRequest groundRequest;
  groundRequest.colliders = request.colliders;
  groundRequest.movingCollider = &finalCollider;
  groundRequest.probeDistanceMeters = probeDistanceMeters;
  groundRequest.includeSensors = request.includeSensors;
  return checkPhysicsGround(groundRequest);
}

void copyGroundHit(PhysicsKinematicMotorResult* result,
                   const PhysicsGroundCheckQueryResult& ground) {
  result->grounded = ground.grounded;
  result->groundDistanceMeters = ground.groundDistanceMeters;
  result->groundNormal = ground.groundNormal;
}

PhysicsKinematicMotorResult finishWithGroundQueries(
    const PhysicsKinematicMotorRequest& request,
    PhysicsKinematicMotorResult result,
    Vec3 currentCenterMeters) {
  PhysicsAabbCollider finalCollider =
      movedCollider(*request.bodyCollider, currentCenterMeters);

  // branch-gate: BG-1098
  if (request.config.groundProbeDistanceMeters > 0.0F) {
    const PhysicsGroundCheckQueryResult ground = runGroundCheck(
        request, finalCollider, request.config.groundProbeDistanceMeters);
    // branch-gate: BG-1098
    if (!ground.ok) {
      return groundQueryFailed(ground);
    }
    result.groundTestedColliderCount += ground.testedColliderCount;
    copyGroundHit(&result, ground);
  }

  // branch-gate: BG-1098
  if (!result.grounded && request.config.groundSnapDistanceMeters > 0.0F) {
    const PhysicsGroundCheckQueryResult snap = runGroundCheck(
        request, finalCollider, request.config.groundSnapDistanceMeters);
    // branch-gate: BG-1098
    if (!snap.ok) {
      return groundQueryFailed(snap);
    }
    result.groundTestedColliderCount += snap.testedColliderCount;
    // branch-gate: BG-1098
    if (snap.grounded) {
      const Vec3 down = normalized({0.0F, -1.0F, 0.0F});
      currentCenterMeters = currentCenterMeters + down * snap.groundDistanceMeters;
      finalCollider = movedCollider(*request.bodyCollider, currentCenterMeters);
      result.snappedToGround = true;
      result.grounded = true;
      result.groundSnapDistanceMetersApplied = snap.groundDistanceMeters;
      result.groundDistanceMeters = 0.0F;
      result.groundNormal = snap.groundNormal;
    }
  }

  result.finalCenterMeters = currentCenterMeters;
  result.appliedDisplacementMeters =
      result.finalCenterMeters - result.startCenterMeters;
  return result;
}

}  // namespace

std::string_view physicsKinematicMotorStatusName(
    PhysicsKinematicMotorStatus status) {
  static constexpr std::array<std::string_view, 9> kNames{
      "physics_kinematic_motor_planned",
      "physics_kinematic_motor_missing_colliders",
      "physics_kinematic_motor_missing_body_collider",
      "physics_kinematic_motor_invalid_body_collider",
      "physics_kinematic_motor_invalid_desired_displacement",
      "physics_kinematic_motor_invalid_config",
      "physics_kinematic_motor_max_displacement_exceeded",
      "physics_kinematic_motor_collision_query_failed",
      "physics_kinematic_motor_ground_query_failed",
  };
  return enumName(status, kNames,
                  "physics_kinematic_motor_invalid_config");
}

bool isValidPhysicsKinematicMotorConfig(
    const PhysicsKinematicMotorConfig& config) {
  // branch-gate: BG-1098
  if (config.maxIterations < 1U || config.maxIterations > 8U) {
    return false;
  }
  // branch-gate: BG-1098
  if (!nonNegativeFinite(config.skinMeters)) {
    return false;
  }
  // branch-gate: BG-1098
  if (!nonNegativeFinite(config.groundProbeDistanceMeters)) {
    return false;
  }
  // branch-gate: BG-1098
  if (!nonNegativeFinite(config.groundSnapDistanceMeters)) {
    return false;
  }
  // branch-gate: BG-1098
  if (!positiveFinite(config.minMoveDistanceMeters)) {
    return false;
  }
  // branch-gate: BG-1098
  if (!positiveFinite(config.maxMoveDistanceMeters)) {
    return false;
  }
  return config.maxMoveDistanceMeters >= config.minMoveDistanceMeters;
}

PhysicsKinematicMotorResult planPhysicsKinematicAabbMove(
    const PhysicsKinematicMotorRequest& request) {
  PhysicsKinematicMotorResult result = validateRequest(request);
  // branch-gate: BG-1098
  if (!result.ok) {
    return result;
  }

  result.startCenterMeters = request.bodyCollider->worldCenterMeters;
  result.finalCenterMeters = result.startCenterMeters;
  Vec3 currentCenterMeters = result.startCenterMeters;
  Vec3 remaining = request.desiredDisplacementMeters;

  // branch-gate: BG-1098
  if (belowMinMove(remaining, request.config.minMoveDistanceMeters)) {
    result.remainingDisplacementMeters = remaining;
    return finishWithGroundQueries(request, result, currentCenterMeters);
  }

  for (std::uint8_t iteration = 0U; iteration < request.config.maxIterations;
       ++iteration) {
    // branch-gate: BG-1098
    if (belowMinMove(remaining, request.config.minMoveDistanceMeters)) {
      break;
    }

    ++result.iterationCount;
    const float remainingLength = vectorLength(remaining);
    const Vec3 direction = remaining / remainingLength;
    const PhysicsAabbCollider moving =
        movedCollider(*request.bodyCollider, currentCenterMeters);

    PhysicsSweptAabbQueryRequest sweepRequest;
    sweepRequest.colliders = request.colliders;
    sweepRequest.movingCollider = &moving;
    sweepRequest.displacementMeters = remaining;
    sweepRequest.includeSensors = request.includeSensors;
    const PhysicsSweptAabbQueryResult sweep = sweepPhysicsAabb(sweepRequest);
    // branch-gate: BG-1098
    if (!sweep.ok) {
      return failedFromCollisionQuery(sweep.reasonCode,
                                      sweep.invalidColliderIndex);
    }
    result.sweepTestedColliderCount += sweep.testedColliderCount;

    // branch-gate: BG-1098
    if (!sweep.hit) {
      currentCenterMeters = currentCenterMeters + remaining;
      remaining = vec3Zero();
      break;
    }

    const PhysicsSweptAabbHit& hit = sweep.nearestHit;
    result.blocked = true;
    result.hits.push_back(makeMotorHit(hit));
    result.hitCount = result.hits.size();

    const float safeDistance =
        std::max(hit.distanceMeters - request.config.skinMeters, 0.0F);
    const Vec3 safeMovement = direction * safeDistance;
    currentCenterMeters = currentCenterMeters + safeMovement;
    const Vec3 unconsumed = remaining - safeMovement;
    remaining = slideAlongCollisionPlane(unconsumed,
                                         hit.normalFromColliderToMovingAabb);

    // branch-gate: BG-1098
    if (hit.initialOverlap ||
        lengthSquared(hit.normalFromColliderToMovingAabb) <=
            kDirectionEpsilonSquared) {
      remaining = vec3Zero();
      break;
    }
  }

  result.remainingDisplacementMeters = remaining;
  return finishWithGroundQueries(request, result, currentCenterMeters);
}

}  // namespace iggy3d
