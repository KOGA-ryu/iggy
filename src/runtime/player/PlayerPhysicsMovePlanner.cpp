#include "runtime/player/PlayerPhysicsMovePlanner.hpp"

#include <array>

namespace iggy3d {
namespace {

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
    const PhysicsSpatialSurfaceColliderBakeResult& bake) {
  MovementColliderPacket packet;
  packet.colliders.reserve(bake.colliders.size());
  packet.sourceSurfaceIds.reserve(bake.sourceSurfaceIds.size());
  for (std::size_t index = 0U; index < bake.colliders.size(); ++index) {
    // Walkable floor colliders are retained in the full bake for ground checks,
    // but excluded from horizontal motor sweeps so standing on a floor does not
    // count as a blocking start-overlap.
    // branch-gate: BG-1100
    if (bake.sourceRoles[index] == CollisionSurfaceRole::Walkable) {
      continue;
    }
    packet.colliders.push_back(bake.colliders[index]);
    packet.sourceSurfaceIds.push_back(bake.sourceSurfaceIds[index]);
  }
  return packet;
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
         isValidPhysicsKinematicMotorConfig(config.motor);
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

  PhysicsSpatialSurfaceColliderBakeRequest bakeRequest;
  bakeRequest.surfaces = request.collisionSurfaces;
  bakeRequest.config = request.config.surfaceBake;
  const PhysicsSpatialSurfaceColliderBakeResult bake =
      bakePhysicsAabbCollidersFromSpatialSurfaces(bakeRequest);
  // branch-gate: BG-1100
  if (!bake.ok) {
    PlayerPhysicsMovePlannerResult failed = failedWithUpstream(
        PlayerPhysicsMovePlannerStatus::SurfaceBakeFailed, bake.reasonCode);
    failed.startCenterMeters = request.startCenterMeters;
    failed.finalCenterMeters = request.startCenterMeters;
    failed.bakedSurfaceCount = bake.surfaceCount;
    failed.skippedSurfaceCount = bake.skippedSurfaceCount;
    failed.invalidSurfaceIndex = bake.invalidSurfaceIndex;
    return failed;
  }

  result.bakedSurfaceCount = bake.surfaceCount;
  result.bakedColliderCount = bake.colliderCount;
  result.skippedSurfaceCount = bake.skippedSurfaceCount;
  attachDebugGeometry(&result, bake);

  const MovementColliderPacket movementPacket = makeMovementColliderPacket(bake);
  PhysicsKinematicMotorConfig motorConfig = request.config.motor;
  motorConfig.groundProbeDistanceMeters = 0.0F;
  motorConfig.groundSnapDistanceMeters = 0.0F;
  const PhysicsAabbCollider playerCollider =
      makePlayerCollider(request, request.startCenterMeters);
  PhysicsKinematicMotorRequest motorRequest;
  motorRequest.colliders = &movementPacket.colliders;
  motorRequest.bodyCollider = &playerCollider;
  motorRequest.desiredDisplacementMeters = request.desiredDisplacementMeters;
  motorRequest.includeSensors = request.includeSensors;
  motorRequest.config = motorConfig;
  const PhysicsKinematicMotorResult motor =
      planPhysicsKinematicAabbMove(motorRequest);
  // branch-gate: BG-1100
  if (!motor.ok) {
    PlayerPhysicsMovePlannerResult failed = failedWithUpstream(
        PlayerPhysicsMovePlannerStatus::MotorPlanFailed, motor.reasonCode);
    failed.startCenterMeters = request.startCenterMeters;
    failed.finalCenterMeters = request.startCenterMeters;
    failed.bakedSurfaceCount = bake.surfaceCount;
    failed.bakedColliderCount = bake.colliderCount;
    failed.skippedSurfaceCount = bake.skippedSurfaceCount;
    attachDebugGeometry(&failed, bake);
    return failed;
  }

  copyMotorFacts(&result, motor, movementPacket.sourceSurfaceIds);
  applyGroundFacts(&result, request, bake.colliders);
  return result;
}

}  // namespace iggy3d
