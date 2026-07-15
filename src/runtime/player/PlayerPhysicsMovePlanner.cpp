#include "runtime/player/PlayerPhysicsMovePlanner.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace iggy3d {
namespace {

constexpr float kStepNormalVerticalLimit = 0.5F;

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1100
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

PlayerPhysicsMovePlannerResult plannerResult(
    PlayerPhysicsMovePlannerStatus status,
    bool ok) {
  PlayerPhysicsMovePlannerResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = playerPhysicsMovePlannerStatusName(status);
  return result;
}

PlayerPhysicsMovePlannerResult failedWithUpstream(
    PlayerPhysicsMovePlannerStatus status,
    std::string_view upstreamReasonCode) {
  PlayerPhysicsMovePlannerResult result = plannerResult(status, false);
  result.upstreamReasonCode = upstreamReasonCode;
  return result;
}

void refreshDisplacementFacts(PlayerPhysicsMovePlannerResult* result,
                              Vec3 desiredDisplacementMeters) {
  result->appliedDisplacementMeters =
      result->finalCenterMeters - result->startCenterMeters;
  result->remainingDisplacementMeters =
      desiredDisplacementMeters - result->appliedDisplacementMeters;
}

void attachDebugGeometry(PlayerPhysicsMovePlannerResult* result,
                         const PhysicsSpatialSurfaceColliderBakeResult& bake) {
  result->debugGeometryAvailable = true;
  result->debugAabbColliders = bake.colliders;
  result->debugAabbSourceSurfaceIds = bake.sourceSurfaceIds;
}

PlayerPhysicsMovePlannerResult validateRequest(
    const PlayerPhysicsMovePlannerRequest& request) {
  // branch-gate: BG-1100
  if (request.collisionSurfaces == nullptr) {
    return plannerResult(
        PlayerPhysicsMovePlannerStatus::MissingCollisionSurfaces, false);
  }
  // branch-gate: BG-1100
  if (!isFinite(request.startCenterMeters)) {
    return plannerResult(PlayerPhysicsMovePlannerStatus::InvalidStartCenter,
                         false);
  }
  // branch-gate: BG-1100
  if (!isPositiveFinitePhysicsHalfExtents(request.bodyHalfExtentsMeters)) {
    return plannerResult(PlayerPhysicsMovePlannerStatus::InvalidBodyHalfExtents,
                         false);
  }
  // branch-gate: BG-1100
  if (!isValidPhysicsBodyId(request.physicsBodyId)) {
    return plannerResult(PlayerPhysicsMovePlannerStatus::InvalidBodyId, false);
  }
  // branch-gate: BG-1100
  if (!isFinite(request.desiredDisplacementMeters)) {
    return plannerResult(
        PlayerPhysicsMovePlannerStatus::InvalidDesiredDisplacement, false);
  }
  // branch-gate: BG-1100
  if (!isValidPhysicsSpatialSurfaceColliderBakeConfig(
          request.config.surfaceBake)) {
    return failedWithUpstream(
        PlayerPhysicsMovePlannerStatus::InvalidSurfaceBakeConfig,
        "physics_spatial_surface_bake_invalid_config");
  }
  // branch-gate: BG-1100
  if (!isValidPhysicsKinematicMotorConfig(request.config.motor)) {
    return failedWithUpstream(
        PlayerPhysicsMovePlannerStatus::InvalidMotorConfig,
        "physics_kinematic_motor_invalid_config");
  }
  return plannerResult(PlayerPhysicsMovePlannerStatus::Planned, true);
}

PhysicsAabbCollider makePlayerCollider(
    const PlayerPhysicsMovePlannerRequest& request,
    Vec3 centerMeters) {
  PhysicsAabbCollider collider;
  collider.bodyId = request.physicsBodyId;
  collider.worldCenterMeters = centerMeters;
  collider.halfExtentsMeters = request.bodyHalfExtentsMeters;
  collider.bounds = aabbFromCenterExtents(collider.worldCenterMeters,
                                          collider.halfExtentsMeters);
  return collider;
}

std::string sourceSurfaceIdForHit(
    const std::vector<std::string>& sourceSurfaceIds,
    const PhysicsKinematicMotorHit& hit) {
  // branch-gate: BG-1100
  if (hit.colliderIndex >= sourceSurfaceIds.size()) {
    return {};
  }
  return sourceSurfaceIds[hit.colliderIndex];
}

struct MovementColliderPacket {
  std::vector<PhysicsAabbCollider> colliders;
  std::vector<std::string> sourceSurfaceIds;
};

MovementColliderPacket makeMovementColliderPacket(
    const PlayerPhysicsMovePlannerRequest& request,
    const PhysicsSpatialSurfaceColliderBakeResult& bake) {
  MovementColliderPacket packet;
  packet.colliders.reserve(bake.colliders.size());
  packet.sourceSurfaceIds.reserve(bake.sourceSurfaceIds.size());
  const float feetHeight =
      request.startCenterMeters.y - request.bodyHalfExtentsMeters.y;
  const float supportTolerance =
      request.config.motor.minMoveDistanceMeters;
  for (std::size_t index = 0U; index < bake.colliders.size(); ++index) {
    // Walkable floor colliders are retained in the full bake for ground checks,
    // but excluded from motor sweeps. Compound blocker boxes at or below the
    // current feet are support too; keeping them would fabricate an initial
    // overlap whenever the player moves across the next stair tread.
    // branch-gate: BG-1100
    if (bake.sourceRoles[index] == CollisionSurfaceRole::Walkable ||
        bake.colliders[index].bounds.max.y <=
            feetHeight + supportTolerance) {
      continue;
    }
    packet.colliders.push_back(bake.colliders[index]);
    packet.sourceSurfaceIds.push_back(bake.sourceSurfaceIds[index]);
  }
  return packet;
}

std::vector<PhysicsAabbCollider> makeWalkableColliderPacket(
    const PhysicsSpatialSurfaceColliderBakeResult& bake) {
  std::vector<PhysicsAabbCollider> colliders;
  colliders.reserve(bake.colliders.size());
  for (std::size_t index = 0U; index < bake.colliders.size(); ++index) {
    // branch-gate: BG-1100
    if (bake.sourceRoles[index] == CollisionSurfaceRole::Walkable) {
      colliders.push_back(bake.colliders[index]);
    }
  }
  return colliders;
}

PhysicsKinematicMotorResult runMotor(
    const PlayerPhysicsMovePlannerRequest& request,
    const MovementColliderPacket& movementPacket,
    const PhysicsKinematicMotorConfig& motorConfig,
    Vec3 startCenterMeters,
    Vec3 desiredDisplacementMeters) {
  const PhysicsAabbCollider playerCollider =
      makePlayerCollider(request, startCenterMeters);
  PhysicsKinematicMotorRequest motorRequest;
  motorRequest.colliders = &movementPacket.colliders;
  motorRequest.bodyCollider = &playerCollider;
  motorRequest.desiredDisplacementMeters = desiredDisplacementMeters;
  motorRequest.includeSensors = request.includeSensors;
  motorRequest.config = motorConfig;
  return planPhysicsKinematicAabbMove(motorRequest);
}

float horizontalDistance(Vec3 value) {
  value.y = 0.0F;
  return std::sqrt(lengthSquared(value));
}

bool hasWalkableSupport(
    const PlayerPhysicsMovePlannerRequest& request,
    const std::vector<PhysicsAabbCollider>& walkableColliders) {
  if (walkableColliders.empty()) {
    return false;
  }
  const PhysicsAabbCollider body =
      makePlayerCollider(request, request.startCenterMeters);
  PhysicsGroundCheckQueryRequest groundRequest;
  groundRequest.colliders = &walkableColliders;
  groundRequest.movingCollider = &body;
  groundRequest.probeDistanceMeters = std::max(
      {request.config.motor.groundProbeDistanceMeters,
       request.config.motor.groundSnapDistanceMeters,
       request.config.motor.skinMeters +
           request.config.motor.minMoveDistanceMeters});
  groundRequest.includeSensors = request.includeSensors;
  const PhysicsGroundCheckQueryResult ground = checkPhysicsGround(groundRequest);
  return ground.ok && ground.grounded &&
         ground.groundNormal.y > kStepNormalVerticalLimit;
}

bool finalStepPlacementIsClear(
    const PlayerPhysicsMovePlannerRequest& request,
    const MovementColliderPacket& movementPacket,
    Vec3 centerMeters) {
  PhysicsAabbCollider body = makePlayerCollider(request, centerMeters);
  const float requestedClearance = std::max(
      request.config.motor.skinMeters,
      request.config.motor.minMoveDistanceMeters);
  const float clearance =
      std::min({requestedClearance,
                request.bodyHalfExtentsMeters.x * 0.5F,
                request.bodyHalfExtentsMeters.y * 0.5F,
                request.bodyHalfExtentsMeters.z * 0.5F});
  body.halfExtentsMeters = body.halfExtentsMeters -
                           Vec3{clearance, clearance, clearance};
  body.bounds = aabbFromCenterExtents(body.worldCenterMeters,
                                     body.halfExtentsMeters);
  PhysicsAabbOverlapQueryRequest overlapRequest;
  overlapRequest.colliders = &movementPacket.colliders;
  overlapRequest.queryCollider = &body;
  overlapRequest.includeSensors = request.includeSensors;
  const PhysicsAabbOverlapQueryResult overlap =
      queryPhysicsAabbOverlaps(overlapRequest);
  return overlap.ok && overlap.hitCount == 0U;
}

struct StepCandidate {
  bool attempted = false;
  bool accepted = false;
  float heightMeters = 0.0F;
  std::size_t additionalIterationCount = 0U;
  Vec3 finalCenterMeters;
  std::string obstacleSourceSurfaceId;
  PhysicsKinematicMotorResult forwardMotor;
};

StepCandidate planStepCandidate(
    const PlayerPhysicsMovePlannerRequest& request,
    const MovementColliderPacket& movementPacket,
    const std::vector<PhysicsAabbCollider>& walkableColliders,
    const PhysicsKinematicMotorConfig& motorConfig,
    const PhysicsKinematicMotorResult& normalMotor) {
  StepCandidate candidate;
  const float minMove = request.config.motor.minMoveDistanceMeters;
  if (request.config.maxStepHeightMeters <= 0.0F ||
      std::fabs(request.desiredDisplacementMeters.y) > minMove ||
      horizontalDistance(request.desiredDisplacementMeters) <= minMove ||
      !normalMotor.blocked || normalMotor.hits.empty() ||
      std::fabs(normalMotor.hits.front().normalFromColliderToMotor.y) >
          kStepNormalVerticalLimit) {
    return candidate;
  }

  candidate.attempted = true;
  candidate.obstacleSourceSurfaceId = sourceSurfaceIdForHit(
      movementPacket.sourceSurfaceIds, normalMotor.hits.front());
  if (!hasWalkableSupport(request, walkableColliders)) {
    return candidate;
  }

  const float clearance = std::max(request.config.motor.skinMeters, minMove);
  const float liftDistance = request.config.maxStepHeightMeters + clearance;
  PhysicsKinematicMotorConfig stepMotorConfig = motorConfig;
  stepMotorConfig.maxMoveDistanceMeters =
      std::max(stepMotorConfig.maxMoveDistanceMeters, liftDistance);
  const PhysicsKinematicMotorResult up =
      runMotor(request, movementPacket, stepMotorConfig,
               request.startCenterMeters, {0.0F, liftDistance, 0.0F});
  candidate.additionalIterationCount += up.iterationCount;
  if (!up.ok || up.blocked ||
      std::fabs(up.appliedDisplacementMeters.y - liftDistance) > minMove ||
      horizontalDistance(up.appliedDisplacementMeters) > minMove) {
    return candidate;
  }

  candidate.forwardMotor =
      runMotor(request, movementPacket, stepMotorConfig,
               up.finalCenterMeters,
               {request.desiredDisplacementMeters.x, 0.0F,
                request.desiredDisplacementMeters.z});
  candidate.additionalIterationCount += candidate.forwardMotor.iterationCount;
  if (!candidate.forwardMotor.ok ||
      horizontalDistance(candidate.forwardMotor.appliedDisplacementMeters) <=
          horizontalDistance(normalMotor.appliedDisplacementMeters) + minMove) {
    return candidate;
  }

  const PhysicsAabbCollider liftedBody =
      makePlayerCollider(request, candidate.forwardMotor.finalCenterMeters);
  PhysicsGroundCheckQueryRequest landingRequest;
  landingRequest.colliders = &walkableColliders;
  landingRequest.movingCollider = &liftedBody;
  landingRequest.probeDistanceMeters =
      liftDistance +
      std::max(request.config.motor.groundProbeDistanceMeters,
               request.config.motor.groundSnapDistanceMeters);
  landingRequest.includeSensors = request.includeSensors;
  const PhysicsGroundCheckQueryResult landing =
      checkPhysicsGround(landingRequest);
  if (!landing.ok || !landing.grounded ||
      landing.groundNormal.y <= kStepNormalVerticalLimit) {
    return candidate;
  }

  candidate.finalCenterMeters =
      candidate.forwardMotor.finalCenterMeters -
      Vec3{0.0F, landing.groundDistanceMeters, 0.0F};
  candidate.heightMeters =
      candidate.finalCenterMeters.y - request.startCenterMeters.y;
  if (candidate.heightMeters <= minMove ||
      candidate.heightMeters > request.config.maxStepHeightMeters + minMove ||
      !finalStepPlacementIsClear(request, movementPacket,
                                 candidate.finalCenterMeters)) {
    candidate.heightMeters = 0.0F;
    return candidate;
  }

  candidate.accepted = true;
  return candidate;
}

void copyMotorFacts(PlayerPhysicsMovePlannerResult* result,
                    const PhysicsKinematicMotorResult& motor,
                    const std::vector<std::string>& sourceSurfaceIds) {
  result->finalCenterMeters = motor.finalCenterMeters;
  result->appliedDisplacementMeters = motor.appliedDisplacementMeters;
  result->remainingDisplacementMeters = motor.remainingDisplacementMeters;
  result->blocked = motor.blocked;
  result->hitCount = motor.hitCount;
  result->iterationCount = motor.iterationCount;
  result->hits = motor.hits;
  result->hitSourceSurfaceIds.reserve(motor.hits.size());
  for (const PhysicsKinematicMotorHit& hit : motor.hits) {
    result->hitSourceSurfaceIds.push_back(
        sourceSurfaceIdForHit(sourceSurfaceIds, hit));
  }
  // branch-gate: BG-1100
  if (!result->hits.empty()) {
    result->firstHitBodyId = result->hits.front().bodyId;
    result->firstHitSourceSurfaceId = result->hitSourceSurfaceIds.front();
  }
}

void applyGroundFacts(PlayerPhysicsMovePlannerResult* result,
                      const PlayerPhysicsMovePlannerRequest& request,
                      const std::vector<PhysicsAabbCollider>& groundColliders) {
  PhysicsAabbCollider body =
      makePlayerCollider(request, result->finalCenterMeters);

  // branch-gate: BG-1100
  if (request.config.motor.groundProbeDistanceMeters > 0.0F) {
    PhysicsGroundCheckQueryRequest groundRequest;
    groundRequest.colliders = &groundColliders;
    groundRequest.movingCollider = &body;
    groundRequest.probeDistanceMeters =
        request.config.motor.groundProbeDistanceMeters;
    groundRequest.includeSensors = request.includeSensors;
    const PhysicsGroundCheckQueryResult ground =
        checkPhysicsGround(groundRequest);
    // branch-gate: BG-1100
    if (ground.ok && ground.grounded) {
      result->grounded = true;
      return;
    }
  }

  // branch-gate: BG-1100
  if (request.config.motor.groundSnapDistanceMeters > 0.0F) {
    PhysicsGroundCheckQueryRequest snapRequest;
    snapRequest.colliders = &groundColliders;
    snapRequest.movingCollider = &body;
    snapRequest.probeDistanceMeters =
        request.config.motor.groundSnapDistanceMeters;
    snapRequest.includeSensors = request.includeSensors;
    const PhysicsGroundCheckQueryResult snap = checkPhysicsGround(snapRequest);
    // branch-gate: BG-1100
    if (snap.ok && snap.grounded) {
      result->finalCenterMeters = result->finalCenterMeters +
                                  Vec3{0.0F, -snap.groundDistanceMeters, 0.0F};
      refreshDisplacementFacts(result, request.desiredDisplacementMeters);
      result->grounded = true;
      result->snappedToGround = true;
    }
  }
}

}  // namespace

std::string_view playerPhysicsMovePlannerStatusName(
    PlayerPhysicsMovePlannerStatus status) {
  static constexpr std::array<std::string_view, 10> kNames{
      "player_physics_move_planner_planned",
      "player_physics_move_planner_missing_collision_surfaces",
      "player_physics_move_planner_invalid_start_center",
      "player_physics_move_planner_invalid_body_half_extents",
      "player_physics_move_planner_invalid_body_id",
      "player_physics_move_planner_invalid_desired_displacement",
      "player_physics_move_planner_invalid_surface_bake_config",
      "player_physics_move_planner_invalid_motor_config",
      "player_physics_move_planner_surface_bake_failed",
      "player_physics_move_planner_motor_plan_failed",
  };
  return enumName(status, kNames,
                  "player_physics_move_planner_invalid_motor_config");
}

bool isValidPlayerPhysicsMovePlannerConfig(
    const PlayerPhysicsMovePlannerConfig& config) {
  return isValidPhysicsSpatialSurfaceColliderBakeConfig(config.surfaceBake) &&
         isValidPhysicsKinematicMotorConfig(config.motor) &&
         std::isfinite(config.maxStepHeightMeters) &&
         config.maxStepHeightMeters >= 0.0F;
}

PlayerPhysicsMovePlannerResult planPlayerPhysicsMove(
    const PlayerPhysicsMovePlannerRequest& request) {
  PlayerPhysicsMovePlannerResult result = validateRequest(request);
  // branch-gate: BG-1100
  if (!result.ok) {
    return result;
  }
  result.startCenterMeters = request.startCenterMeters;
  result.finalCenterMeters = request.startCenterMeters;

  PhysicsSpatialSurfaceColliderBakeResult ownedBake;
  const PhysicsSpatialSurfaceColliderBakeResult* bake =
      request.precomputedSurfaceBake;
  if (bake == nullptr) {
    PhysicsSpatialSurfaceColliderBakeRequest bakeRequest;
    bakeRequest.surfaces = request.collisionSurfaces;
    bakeRequest.config = request.config.surfaceBake;
    ownedBake = bakePhysicsAabbCollidersFromSpatialSurfaces(bakeRequest);
    bake = &ownedBake;
  }
  // branch-gate: BG-1100
  if (!bake->ok) {
    PlayerPhysicsMovePlannerResult failed = failedWithUpstream(
        PlayerPhysicsMovePlannerStatus::SurfaceBakeFailed, bake->reasonCode);
    failed.startCenterMeters = request.startCenterMeters;
    failed.finalCenterMeters = request.startCenterMeters;
    failed.bakedSurfaceCount = bake->surfaceCount;
    failed.skippedSurfaceCount = bake->skippedSurfaceCount;
    failed.invalidSurfaceIndex = bake->invalidSurfaceIndex;
    return failed;
  }

  result.bakedSurfaceCount = bake->surfaceCount;
  result.bakedColliderCount = bake->colliderCount;
  result.skippedSurfaceCount = bake->skippedSurfaceCount;
  attachDebugGeometry(&result, *bake);

  const MovementColliderPacket movementPacket =
      makeMovementColliderPacket(request, *bake);
  const std::vector<PhysicsAabbCollider> walkableColliders =
      makeWalkableColliderPacket(*bake);
  PhysicsKinematicMotorConfig motorConfig = request.config.motor;
  motorConfig.groundProbeDistanceMeters = 0.0F;
  motorConfig.groundSnapDistanceMeters = 0.0F;
  const PhysicsKinematicMotorResult motor =
      runMotor(request, movementPacket, motorConfig,
               request.startCenterMeters,
               request.desiredDisplacementMeters);
  // branch-gate: BG-1100
  if (!motor.ok) {
    PlayerPhysicsMovePlannerResult failed = failedWithUpstream(
        PlayerPhysicsMovePlannerStatus::MotorPlanFailed, motor.reasonCode);
    failed.startCenterMeters = request.startCenterMeters;
    failed.finalCenterMeters = request.startCenterMeters;
    failed.bakedSurfaceCount = bake->surfaceCount;
    failed.bakedColliderCount = bake->colliderCount;
    failed.skippedSurfaceCount = bake->skippedSurfaceCount;
    attachDebugGeometry(&failed, *bake);
    return failed;
  }

  const StepCandidate step =
      planStepCandidate(request, movementPacket, walkableColliders,
                        motorConfig, motor);
  if (step.accepted) {
    copyMotorFacts(&result, step.forwardMotor,
                   movementPacket.sourceSurfaceIds);
    result.finalCenterMeters = step.finalCenterMeters;
    refreshDisplacementFacts(&result, request.desiredDisplacementMeters);
    result.grounded = true;
    result.iterationCount =
        motor.iterationCount + step.additionalIterationCount;
  } else {
    copyMotorFacts(&result, motor, movementPacket.sourceSurfaceIds);
    result.iterationCount += step.additionalIterationCount;
  }
  result.stepAttempted = step.attempted;
  result.stepAccepted = step.accepted;
  result.stepHeightMetersApplied = step.heightMeters;
  result.stepObstacleSourceSurfaceId = step.obstacleSourceSurfaceId;
  applyGroundFacts(&result, request, bake->colliders);
  return result;
}

}  // namespace iggy3d
