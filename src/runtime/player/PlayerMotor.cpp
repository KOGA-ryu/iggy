#include "runtime/player/PlayerMotor.hpp"

#include "runtime/collision/CollisionQuery.hpp"
#include "runtime/movement/MovementPolicy.hpp"
#include "runtime/player/PlayerPhysicsMovePlanner.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace iggy3d {
namespace {

inline constexpr float kPlayerMotorEpsilon = 0.0001F;

float vectorLength(Vec3 value) {
  return std::sqrt(lengthSquared(value));
}

bool validParams(const PlayerMotorParams& params) {
  return std::isfinite(params.gravityMetersPerSecondSquared) &&
         params.gravityMetersPerSecondSquared > 0.0F &&
         std::isfinite(params.jumpImpulseMetersPerSecond) &&
         params.jumpImpulseMetersPerSecond > 0.0F && std::isfinite(params.groundProbeMeters) &&
         params.groundProbeMeters >= 0.0F && std::isfinite(params.landingSnapMeters) &&
         params.landingSnapMeters >= 0.0F &&
         std::isfinite(params.stepHeightMeters) && params.stepHeightMeters >= 0.0F &&
         std::isfinite(params.footprintToleranceMeters) &&
         params.footprintToleranceMeters >= 0.0F &&
         std::isfinite(params.maxWalkableSlopeDegrees) &&
         params.maxWalkableSlopeDegrees >= 0.0F &&
         params.maxWalkableSlopeDegrees <= 90.0F &&
         std::isfinite(params.terminalVelocityMetersPerSecond) &&
         params.terminalVelocityMetersPerSecond < 0.0F &&
         std::isfinite(params.airMaxSpeedMetersPerSecond) &&
         params.airMaxSpeedMetersPerSecond >= 0.0F &&
         std::isfinite(params.airAccelerationMetersPerSecondSquared) &&
         params.airAccelerationMetersPerSecondSquared >= 0.0F &&
         std::isfinite(params.airDragPerSecond) && params.airDragPerSecond >= 0.0F &&
         std::isfinite(params.airLaunchSpeedMetersPerSecond) &&
         params.airLaunchSpeedMetersPerSecond >= 0.0F &&
         std::isfinite(params.airCollisionProbeHeightMeters) &&
         params.airCollisionProbeHeightMeters > 0.0F &&
         std::isfinite(params.airCollisionSkinMeters) &&
         params.airCollisionSkinMeters >= 0.0F &&
         std::isfinite(params.dashSpeedMetersPerSecond) &&
         params.dashSpeedMetersPerSecond >= 0.0F &&
         std::isfinite(params.dashDurationSeconds) && params.dashDurationSeconds >= 0.0F &&
         std::isfinite(params.dashCooldownSeconds) && params.dashCooldownSeconds >= 0.0F &&
         std::isfinite(params.wireWalkSpeedMetersPerSecond) &&
         params.wireWalkSpeedMetersPerSecond >= 0.0F &&
         (!params.usePhysicsMovePlanner ||
          (isFinite(params.physicsBodyHalfExtentsMeters) &&
           params.physicsBodyHalfExtentsMeters.x > 0.0F &&
           params.physicsBodyHalfExtentsMeters.y > 0.0F &&
           params.physicsBodyHalfExtentsMeters.z > 0.0F));
}

PlayerMotorResult baseResult(const PlayerMotorState& state,
                             Vec3 start,
                             PlayerMotorStatus status,
                             const PlayerMotorParams& params) {
  PlayerMotorResult result;
  result.status = status;
  result.actor = state.actor;
  result.phase = state.phase;
  result.grounded = state.grounded;
  result.wasGrounded = state.grounded;
  result.horizontalVelocityMetersPerSecond = state.horizontalVelocityMetersPerSecond;
  result.verticalVelocityMetersPerSecond = state.verticalVelocityMetersPerSecond;
  result.horizontalSpeedMetersPerSecond = vectorLength(state.horizontalVelocityMetersPerSecond);
  result.dashRemainingSeconds = state.dashRemainingSeconds;
  result.dashCooldownRemainingSeconds = state.dashCooldownRemainingSeconds;
  result.gravityMetersPerSecondSquared = params.gravityMetersPerSecondSquared;
  result.jumpImpulseMetersPerSecond = params.jumpImpulseMetersPerSecond;
  result.airMaxSpeedMetersPerSecond = params.airMaxSpeedMetersPerSecond;
  result.airAccelerationMetersPerSecondSquared = params.airAccelerationMetersPerSecondSquared;
  result.dashSpeedMetersPerSecond = params.dashSpeedMetersPerSecond;
  result.dashDurationSeconds = params.dashDurationSeconds;
  result.dashCooldownSeconds = params.dashCooldownSeconds;
  result.wireWalkSpeedMetersPerSecond = params.wireWalkSpeedMetersPerSecond;
  result.wireWalkCoordinateMeters = state.wireWalkCoordinateMeters;
  result.wireWalkRailLengthMeters =
      vectorLength(state.wireWalkRailEndMeters - state.wireWalkRailStartMeters);
  result.startPosition = start;
  result.finalPosition = start;
  result.reasonCode = playerMotorStatusName(status);
  return result;
}

bool closeToGround(const CollisionQueryResult& ground, Vec3 position, const PlayerMotorParams& params) {
  return ground.status == CollisionQueryStatus::Hit &&
         std::fabs(position.y - ground.heightMeters) <= params.groundProbeMeters;
}

bool walkableGround(const CollisionQueryResult& ground,
                    const PlayerMotorParams& params) {
  if (ground.status != CollisionQueryStatus::Hit) {
    return false;
  }
  MovementParams slopeParams;
  slopeParams.maxWalkableSlopeDegrees = params.maxWalkableSlopeDegrees;
  const SlopeSample slope = sampleSlope(ground.normal, slopeParams);
  return slope.valid && slope.walkable;
}

void applyGroundSample(PlayerMotorResult& result,
                       const CollisionQueryResult& ground,
                       Vec3 position,
                       const PlayerMotorParams& params) {
  if (ground.status != CollisionQueryStatus::Hit) {
    result.groundSampleValid = false;
    result.groundContact = false;
    result.groundWalkable = false;
    result.groundDistanceMeters = 0.0F;
    result.movementPolicyBand = "not_sampled";
    return;
  }

  MovementParams slopeParams;
  slopeParams.maxWalkableSlopeDegrees = params.maxWalkableSlopeDegrees;
  const SlopeSample slope = sampleSlope(ground.normal, slopeParams);

  result.groundSampleValid = true;
  result.groundContact = result.grounded && closeToGround(ground, position, params);
  result.groundDistanceMeters = position.y - ground.heightMeters;
  result.groundSurfaceId = ground.surfaceId;
  if (slope.valid) {
    result.groundNormal = slope.normal;
    result.slopeAngleDegrees = slope.angleDegrees;
    result.slopeUpDot = slope.upDot;
    result.groundWalkable = slope.walkable;
    result.speedMultiplier = slope.speedMultiplier;
    result.staminaCostMultiplier = slope.staminaCostMultiplier;
    result.stepPenaltyMultiplier = slope.stepPenaltyMultiplier;
    result.carefulFooting = slope.carefulFooting;
    result.movementPolicyBand = std::string{slope.bandId};
  } else {
    result.groundNormal = ground.normal;
    result.slopeAngleDegrees = 0.0F;
    result.slopeUpDot = 0.0F;
    result.groundWalkable = false;
    result.speedMultiplier = 0.0F;
    result.staminaCostMultiplier = 0.0F;
    result.stepPenaltyMultiplier = 0.0F;
    result.carefulFooting = false;
    result.movementPolicyBand = "invalid";
  }
}

Vec3 horizontal(Vec3 value) {
  return {value.x, 0.0F, value.z};
}

bool normalizedHorizontal(Vec3 value, Vec3& out) {
  value = horizontal(value);
  if (!isFinite(value)) {
    return false;
  }
  const float len = vectorLength(value);
  if (!std::isfinite(len) || len <= kPlayerMotorEpsilon) {
    return false;
  }
  out = value / len;
  return isFinite(out);
}

Vec3 approach(Vec3 current, Vec3 target, float maxDelta) {
  if (maxDelta <= 0.0F) {
    return current;
  }
  const Vec3 delta = target - current;
  const float len = vectorLength(delta);
  if (!std::isfinite(len) || len <= kPlayerMotorEpsilon) {
    return target;
  }
  if (len <= maxDelta) {
    return target;
  }
  return current + delta / len * maxDelta;
}

Vec3 dampVelocity(Vec3 velocity, float dragPerSecond, float seconds) {
  if (dragPerSecond <= 0.0F || seconds <= 0.0F) {
    return velocity;
  }
  const float scale = std::clamp(1.0F - dragPerSecond * seconds, 0.0F, 1.0F);
  return velocity * scale;
}

Vec3 withoutNormal(Vec3 value, Vec3 normal) {
  Vec3 adjusted = value - normal * dot(value, normal);
  adjusted.y = 0.0F;
  return adjusted;
}

struct HorizontalCollisionResult {
  bool clamped = false;
  bool slid = false;
  bool stepAttempted = false;
  bool stepAccepted = false;
  float stepHeightMetersApplied = 0.0F;
  std::string hitSurfaceId;
};

HorizontalCollisionResult applyHorizontalCollision(const SpatialSurfaceSet& surfaces,
                                                   const PlayerMotorParams& params,
                                                   Vec3 start,
                                                   Vec3 horizontalDisplacement,
                                                   Vec3& finalPosition,
                                                   Vec3& horizontalVelocity) {
  HorizontalCollisionResult result;
  const float horizontalDistance = vectorLength(horizontalDisplacement);
  if (!std::isfinite(horizontalDistance) || horizontalDistance <= kPlayerMotorEpsilon) {
    return result;
  }

  Vec3 candidate = finalPosition;
  const Vec3 sweepStart = start + vec3UnitY() * params.airCollisionProbeHeightMeters;
  const Vec3 sweepEnd = candidate + vec3UnitY() * params.airCollisionProbeHeightMeters;
  const CollisionQueryResult hit =
      querySegment(surfaces, sweepStart, sweepEnd, CollisionQueryKind::Actor);
  if (hit.status != CollisionQueryStatus::Hit) {
    return result;
  }

  result.clamped = true;
  result.hitSurfaceId = hit.surfaceId;
  const float skinTime =
      std::clamp(params.airCollisionSkinMeters / horizontalDistance, 0.0F, 1.0F);
  const float safeTime = std::clamp(hit.timeOfImpact - skinTime, 0.0F, 1.0F);
  candidate = {
      start.x + horizontalDisplacement.x * safeTime,
      finalPosition.y,
      start.z + horizontalDisplacement.z * safeTime,
  };

  Vec3 remaining = horizontalDisplacement * (1.0F - safeTime);
  Vec3 slide = withoutNormal(remaining, hit.normal);
  if (isFinite(slide) && vectorLength(slide) > params.airCollisionSkinMeters) {
    result.slid = true;
    const Vec3 slideCandidate = candidate + slide;
    const CollisionQueryResult slideHit =
        querySegment(surfaces,
                     candidate + vec3UnitY() * params.airCollisionProbeHeightMeters,
                     slideCandidate + vec3UnitY() * params.airCollisionProbeHeightMeters,
                     CollisionQueryKind::Actor);
    if (slideHit.status == CollisionQueryStatus::Hit) {
      const float slideLength = vectorLength(slide);
      const float slideSkinTime =
          slideLength > kPlayerMotorEpsilon
              ? std::clamp(params.airCollisionSkinMeters / slideLength, 0.0F, 1.0F)
              : 0.0F;
      const float slideSafeTime =
          std::clamp(slideHit.timeOfImpact - slideSkinTime, 0.0F, 1.0F);
      candidate = candidate + slide * slideSafeTime;
      if (result.hitSurfaceId.empty()) {
        result.hitSurfaceId = slideHit.surfaceId;
      }
      horizontalVelocity = withoutNormal(horizontalVelocity, slideHit.normal);
    } else {
      candidate = slideCandidate;
    }
  }

  horizontalVelocity = withoutNormal(horizontalVelocity, hit.normal);
  finalPosition.x = candidate.x;
  finalPosition.z = candidate.z;
  return result;
}

HorizontalCollisionResult applyPhysicsHorizontalCollision(const SpatialSurfaceSet& surfaces,
                                                          const PlayerMotorParams& params,
                                                          Vec3 start,
                                                          Vec3 horizontalDisplacement,
                                                          Vec3& finalPosition,
                                                          Vec3& horizontalVelocity,
                                                          bool allowStepUp) {
  HorizontalCollisionResult result;
  const float horizontalDistance = vectorLength(horizontalDisplacement);
  // branch-gate: BG-1101
  if (!std::isfinite(horizontalDistance) || horizontalDistance <= kPlayerMotorEpsilon) {
    return result;
  }

  PlayerPhysicsMovePlannerConfig config;
  config.motor.skinMeters = params.airCollisionSkinMeters;
  config.motor.groundProbeDistanceMeters = 0.0F;
  config.motor.groundSnapDistanceMeters = 0.0F;
  config.motor.maxMoveDistanceMeters =
      std::max(config.motor.maxMoveDistanceMeters, horizontalDistance + 1.0F);
  config.maxStepHeightMeters = allowStepUp ? params.stepHeightMeters : 0.0F;

  PlayerPhysicsMovePlannerRequest request;
  request.collisionSurfaces = &surfaces;
  request.startCenterMeters =
      start + vec3UnitY() * params.physicsBodyHalfExtentsMeters.y;
  request.bodyHalfExtentsMeters = params.physicsBodyHalfExtentsMeters;
  request.desiredDisplacementMeters = horizontal(horizontalDisplacement);
  request.config = config;
  const PlayerPhysicsMovePlannerResult planned = planPlayerPhysicsMove(request);
  // branch-gate: BG-1101
  if (!planned.ok) {
    finalPosition.x = start.x;
    finalPosition.z = start.z;
    horizontalVelocity = {};
    result.clamped = true;
    return result;
  }

  finalPosition.x = planned.finalCenterMeters.x;
  if (planned.stepAccepted) {
    finalPosition.y = planned.finalCenterMeters.y -
                      params.physicsBodyHalfExtentsMeters.y;
  }
  finalPosition.z = planned.finalCenterMeters.z;
  result.clamped = planned.blocked || planned.hitCount > 0U;
  result.stepAttempted = planned.stepAttempted;
  result.stepAccepted = planned.stepAccepted;
  result.stepHeightMetersApplied = planned.stepHeightMetersApplied;
  result.hitSurfaceId = planned.firstHitSourceSurfaceId.empty()
                            ? planned.stepObstacleSourceSurfaceId
                            : planned.firstHitSourceSurfaceId;
  for (const PhysicsKinematicMotorHit& hit : planned.hits) {
    horizontalVelocity = withoutNormal(horizontalVelocity, hit.normalFromColliderToMotor);
  }
  // branch-gate: BG-1101
  if (!planned.hits.empty()) {
    const Vec3 tangent =
        withoutNormal(horizontal(planned.appliedDisplacementMeters),
                      planned.hits.front().normalFromColliderToMotor);
    result.slid = vectorLength(tangent) > params.airCollisionSkinMeters;
  }
  return result;
}

HorizontalCollisionResult applyConfiguredHorizontalCollision(
    const SpatialSurfaceSet& surfaces,
    const PlayerMotorParams& params,
    Vec3 start,
    Vec3 horizontalDisplacement,
    Vec3& finalPosition,
    Vec3& horizontalVelocity,
    bool allowStepUp) {
  // branch-gate: BG-1101
  if (params.usePhysicsMovePlanner) {
    return applyPhysicsHorizontalCollision(
        surfaces, params, start, horizontalDisplacement, finalPosition,
        horizontalVelocity, allowStepUp);
  }
  return applyHorizontalCollision(
      surfaces, params, start, horizontalDisplacement, finalPosition, horizontalVelocity);
}

}  // namespace

bool playerMotorSucceeded(const PlayerMotorResult& result) {
  return result.status == PlayerMotorStatus::Ok;
}

const char* playerMotorStatusName(PlayerMotorStatus status) {
  switch (status) {
    case PlayerMotorStatus::Ok:
      return "player_motor_ok";
    case PlayerMotorStatus::MissingWorld:
      return "missing_world";
    case PlayerMotorStatus::MissingCollisionSurfaces:
      return "missing_collision_surfaces";
    case PlayerMotorStatus::InvalidActor:
      return "invalid_actor";
    case PlayerMotorStatus::ActorInactive:
      return "actor_inactive";
    case PlayerMotorStatus::InvalidParameters:
      return "invalid_parameters";
    case PlayerMotorStatus::NoGround:
      return "no_ground";
    case PlayerMotorStatus::UnwalkableGround:
      return "unwalkable_ground";
  }
  return "invalid_parameters";
}

const char* playerMotorPhaseName(PlayerMotorPhase phase) {
  switch (phase) {
    case PlayerMotorPhase::Grounded:
      return "grounded";
    case PlayerMotorPhase::Airborne:
      return "airborne";
    case PlayerMotorPhase::WireWalk:
      return "wire_walk";
  }
  return "grounded";
}

PlayerMotorResult updatePlayerMotor(PlayerMotorContext& context,
                                    PlayerMotorState& state,
                                    const PlayerMotorInput& input,
                                    const PlayerMotorParams& params) {
  Vec3 start;
  if (context.world == nullptr) {
    return baseResult(state, start, PlayerMotorStatus::MissingWorld, params);
  }
  if (context.collisionSurfaces == nullptr) {
    return baseResult(state, start, PlayerMotorStatus::MissingCollisionSurfaces, params);
  }
  if (!validParams(params) || !std::isfinite(input.seconds) || input.seconds < 0.0F ||
      !isFinite(input.moveIntent)) {
    return baseResult(state, start, PlayerMotorStatus::InvalidParameters, params);
  }
  if (!isValid(state.actor)) {
    return baseResult(state, start, PlayerMotorStatus::InvalidActor, params);
  }

  const EntityState* actor = context.world->findById(state.actor);
  if (actor == nullptr) {
    return baseResult(state, start, PlayerMotorStatus::InvalidActor, params);
  }
  start = actor->transform.position;
  if (!actor->active) {
    return baseResult(state, start, PlayerMotorStatus::ActorInactive, params);
  }
  if (!isFinite(start)) {
    return baseResult(state, start, PlayerMotorStatus::InvalidParameters, params);
  }

  const CollisionQueryResult ground =
      sampleSurfaceHeight(*context.collisionSurfaces, start, params.footprintToleranceMeters);
  const bool groundIsWalkable = walkableGround(ground, params);
  const bool nearGround = groundIsWalkable && closeToGround(ground, start, params);
  if (ground.status != CollisionQueryStatus::Hit && state.phase == PlayerMotorPhase::Grounded) {
    return baseResult(state, start, PlayerMotorStatus::NoGround, params);
  }
  if (!groundIsWalkable && state.phase == PlayerMotorPhase::Grounded) {
    return baseResult(state, start, PlayerMotorStatus::UnwalkableGround,
                      params);
  }

  PlayerMotorResult result = baseResult(state, start, PlayerMotorStatus::Ok, params);
  result.jumpRequested = input.jumpPressed;
  result.dashRequested = input.dashPressed;
  if (input.seconds > 0.0F && state.dashCooldownRemainingSeconds > 0.0F) {
    state.dashCooldownRemainingSeconds =
        std::max(0.0F, state.dashCooldownRemainingSeconds - input.seconds);
  }

  if (nearGround && state.phase == PlayerMotorPhase::Grounded &&
      state.verticalVelocityMetersPerSecond <= 0.0F) {
    state.grounded = true;
    state.jumpAvailable = true;
    if (state.dashRemainingSeconds <= 0.0F) {
      state.horizontalVelocityMetersPerSecond = {};
    }
    state.verticalVelocityMetersPerSecond = 0.0F;
    if (std::fabs(start.y - ground.heightMeters) <= params.landingSnapMeters &&
        !nearlyEqual(start, {start.x, ground.heightMeters, start.z})) {
      Transform3 snappedTransform = actor->transform;
      snappedTransform.position = {start.x, ground.heightMeters, start.z};
      const WorldMutationResult mutation =
          context.world->updateTransform(state.actor, snappedTransform);
      if (mutation.status == WorldStatus::Ok) {
        start = snappedTransform.position;
        result.groundSnapApplied = true;
        result.mutatedWorld = true;
      }
    }
  } else if (!nearGround && state.phase == PlayerMotorPhase::Grounded) {
    state.phase = PlayerMotorPhase::Airborne;
    state.grounded = false;
    state.jumpAvailable = false;
  }

  if (input.jumpPressed && state.phase == PlayerMotorPhase::WireWalk) {
    Vec3 launchDirection;
    if (normalizedHorizontal(input.moveIntent, launchDirection)) {
      const float launchSpeed =
          std::min(params.airLaunchSpeedMetersPerSecond, params.airMaxSpeedMetersPerSecond);
      state.horizontalVelocityMetersPerSecond = launchDirection * launchSpeed;
    }
    state.phase = PlayerMotorPhase::Airborne;
    state.grounded = false;
    state.jumpAvailable = false;
    state.verticalVelocityMetersPerSecond = params.jumpImpulseMetersPerSecond;
    result.jumpAccepted = true;
  } else if (input.jumpPressed && state.grounded && state.jumpAvailable) {
    Vec3 launchDirection;
    if (normalizedHorizontal(input.moveIntent, launchDirection)) {
      const float launchSpeed =
          std::min(params.airLaunchSpeedMetersPerSecond, params.airMaxSpeedMetersPerSecond);
      state.horizontalVelocityMetersPerSecond = launchDirection * launchSpeed;
    }
    state.phase = PlayerMotorPhase::Airborne;
    state.grounded = false;
    state.jumpAvailable = false;
    state.verticalVelocityMetersPerSecond = params.jumpImpulseMetersPerSecond;
    result.jumpAccepted = true;
  }

  if (input.dashPressed) {
    Vec3 dashDirection;
    if (!normalizedHorizontal(input.moveIntent, dashDirection)) {
      result.dashRejectedNoIntent = true;
    } else if (state.dashRemainingSeconds > 0.0F ||
               state.dashCooldownRemainingSeconds > 0.0F) {
      result.dashRejectedCooldown = true;
    } else {
      state.dashDirection = dashDirection;
      state.dashRemainingSeconds = params.dashDurationSeconds;
      state.dashCooldownRemainingSeconds = params.dashCooldownSeconds;
      state.horizontalVelocityMetersPerSecond =
          state.dashDirection * params.dashSpeedMetersPerSecond;
      result.dashAccepted = true;
    }
  }

  Vec3 finalPosition = start;
  bool transformUpdateNeeded = false;
  if (state.phase == PlayerMotorPhase::WireWalk && input.seconds > 0.0F) {
    Vec3 railAxis;
    Vec3 railDelta = state.wireWalkRailEndMeters - state.wireWalkRailStartMeters;
    railDelta.y = 0.0F;
    const float railLength = vectorLength(railDelta);
    if (railLength <= kPlayerMotorEpsilon ||
        !normalizedHorizontal(state.wireWalkAxis, railAxis)) {
      state.phase = PlayerMotorPhase::Airborne;
      state.grounded = false;
      state.jumpAvailable = false;
    } else {
      Vec3 moveDirection;
      const bool hasMoveIntent = normalizedHorizontal(input.moveIntent, moveDirection);
      const float inputAlongRail = hasMoveIntent ? dot(moveDirection, railAxis) : 0.0F;
      const float previousCoordinate = state.wireWalkCoordinateMeters;
      state.wireWalkCoordinateMeters =
          std::clamp(state.wireWalkCoordinateMeters +
                         inputAlongRail * params.wireWalkSpeedMetersPerSecond * input.seconds,
                     0.0F,
                     railLength);
      finalPosition = state.wireWalkRailStartMeters + railAxis * state.wireWalkCoordinateMeters;
      finalPosition.y = state.wireWalkRailStartMeters.y;
      state.horizontalVelocityMetersPerSecond =
          railAxis * (inputAlongRail * params.wireWalkSpeedMetersPerSecond);
      state.verticalVelocityMetersPerSecond = 0.0F;
      state.grounded = false;
      state.jumpAvailable = true;
      result.wireWalkActive = true;
      result.wireWalkMoved =
          std::fabs(state.wireWalkCoordinateMeters - previousCoordinate) > kPlayerMotorEpsilon;
      result.wireWalkEndpointReached =
          state.wireWalkCoordinateMeters <= kPlayerMotorEpsilon ||
          state.wireWalkCoordinateMeters >= railLength - kPlayerMotorEpsilon;
      transformUpdateNeeded = true;
    }
  } else if (state.dashRemainingSeconds > 0.0F && input.seconds > 0.0F &&
      params.dashSpeedMetersPerSecond > 0.0F) {
    result.dashActive = true;
    const float dashSeconds = std::min(input.seconds, state.dashRemainingSeconds);
    state.horizontalVelocityMetersPerSecond =
        state.dashDirection * params.dashSpeedMetersPerSecond;
    const Vec3 horizontalDisplacement =
        state.horizontalVelocityMetersPerSecond * dashSeconds;
    finalPosition.x = start.x + horizontalDisplacement.x;
    finalPosition.z = start.z + horizontalDisplacement.z;
    HorizontalCollisionResult dashCollision =
        applyConfiguredHorizontalCollision(*context.collisionSurfaces,
                                           params,
                                           start,
                                           horizontalDisplacement,
                                           finalPosition,
                                           state.horizontalVelocityMetersPerSecond,
                                           state.phase == PlayerMotorPhase::Grounded &&
                                               state.grounded);
    result.dashMovementClamped = dashCollision.clamped;
    result.dashMovementSlid = dashCollision.slid;
    result.stepAttempted = dashCollision.stepAttempted;
    result.stepAccepted = dashCollision.stepAccepted;
    result.stepHeightMetersApplied = dashCollision.stepHeightMetersApplied;
    if (!dashCollision.hitSurfaceId.empty()) {
      result.hitSurfaceId = std::move(dashCollision.hitSurfaceId);
    }
    state.dashRemainingSeconds = std::max(0.0F, state.dashRemainingSeconds - dashSeconds);
    transformUpdateNeeded = true;
  } else if (state.phase == PlayerMotorPhase::Airborne && input.seconds > 0.0F) {
    Vec3 airIntentDirection;
    const bool hasAirIntent = normalizedHorizontal(input.moveIntent, airIntentDirection);
    result.airMoveIntent = hasAirIntent;
    const Vec3 desiredHorizontalVelocity =
        hasAirIntent ? airIntentDirection * params.airMaxSpeedMetersPerSecond : vec3Zero();
    state.horizontalVelocityMetersPerSecond =
        approach(state.horizontalVelocityMetersPerSecond,
                 desiredHorizontalVelocity,
                 params.airAccelerationMetersPerSecondSquared * input.seconds);
    if (!hasAirIntent) {
      state.horizontalVelocityMetersPerSecond =
          dampVelocity(state.horizontalVelocityMetersPerSecond, params.airDragPerSecond, input.seconds);
    }
    state.horizontalVelocityMetersPerSecond = horizontal(state.horizontalVelocityMetersPerSecond);
    result.airControlActive = hasAirIntent ||
                              vectorLength(state.horizontalVelocityMetersPerSecond) >
                                  kPlayerMotorEpsilon;

    const Vec3 horizontalDisplacement = state.horizontalVelocityMetersPerSecond * input.seconds;
    finalPosition.x = start.x + horizontalDisplacement.x;
    finalPosition.z = start.z + horizontalDisplacement.z;
    HorizontalCollisionResult airCollision =
        applyConfiguredHorizontalCollision(*context.collisionSurfaces,
                                           params,
                                           start,
                                           horizontalDisplacement,
                                           finalPosition,
                                           state.horizontalVelocityMetersPerSecond,
                                           false);
    result.airMovementClamped = airCollision.clamped;
    result.airMovementSlid = airCollision.slid;
    if (!airCollision.hitSurfaceId.empty()) {
      result.hitSurfaceId = std::move(airCollision.hitSurfaceId);
    }
    transformUpdateNeeded = true;
  }

  if (state.phase == PlayerMotorPhase::Airborne && input.seconds > 0.0F) {
    const float previousVelocity = state.verticalVelocityMetersPerSecond;
    const float nextVelocity =
        std::max(params.terminalVelocityMetersPerSecond,
                 previousVelocity - params.gravityMetersPerSecondSquared * input.seconds);
    const float displacement = ((previousVelocity + nextVelocity) * 0.5F) * input.seconds;
    finalPosition.y = start.y + displacement;
    state.verticalVelocityMetersPerSecond = nextVelocity;

    const CollisionQueryResult landingGround = sampleSurfaceHeight(
        *context.collisionSurfaces, finalPosition, params.footprintToleranceMeters);
    if (walkableGround(landingGround, params) &&
        finalPosition.y <= landingGround.heightMeters + params.landingSnapMeters &&
        state.verticalVelocityMetersPerSecond <= 0.0F) {
      finalPosition.y = landingGround.heightMeters;
      state.phase = PlayerMotorPhase::Grounded;
      state.grounded = true;
      state.jumpAvailable = true;
      state.horizontalVelocityMetersPerSecond = {};
      state.verticalVelocityMetersPerSecond = 0.0F;
      result.landed = true;
      result.groundSnapApplied = true;
    }
    transformUpdateNeeded = true;
  } else if (state.phase == PlayerMotorPhase::Grounded && transformUpdateNeeded) {
    const CollisionQueryResult finalGround =
        sampleSurfaceHeight(*context.collisionSurfaces, finalPosition, params.footprintToleranceMeters);
    if (walkableGround(finalGround, params) &&
        std::fabs(finalPosition.y - finalGround.heightMeters) <= params.landingSnapMeters) {
      if (!nearlyEqual(finalPosition, {finalPosition.x, finalGround.heightMeters, finalPosition.z})) {
        result.groundSnapApplied = true;
      }
      finalPosition.y = finalGround.heightMeters;
      state.grounded = true;
      state.jumpAvailable = true;
    } else {
      state.phase = PlayerMotorPhase::Airborne;
      state.grounded = false;
      state.jumpAvailable = false;
    }
  }

  if (state.dashRemainingSeconds <= 0.0F && state.phase == PlayerMotorPhase::Grounded) {
    state.horizontalVelocityMetersPerSecond = {};
    state.dashDirection = {};
  }

  if (transformUpdateNeeded) {
    Transform3 nextTransform = actor->transform;
    nextTransform.position = finalPosition;
    const WorldMutationResult mutation = context.world->updateTransform(state.actor, nextTransform);
    if (mutation.status != WorldStatus::Ok) {
      return baseResult(state, start, PlayerMotorStatus::InvalidParameters, params);
    }
    result.mutatedWorld = true;
  }

  result.actor = state.actor;
  result.phase = state.phase;
  result.grounded = state.grounded;
  result.dashActive = result.dashActive || state.dashRemainingSeconds > 0.0F;
  result.horizontalVelocityMetersPerSecond = state.horizontalVelocityMetersPerSecond;
  result.verticalVelocityMetersPerSecond = state.verticalVelocityMetersPerSecond;
  result.horizontalSpeedMetersPerSecond = vectorLength(state.horizontalVelocityMetersPerSecond);
  result.dashRemainingSeconds = state.dashRemainingSeconds;
  result.dashCooldownRemainingSeconds = state.dashCooldownRemainingSeconds;
  result.wireWalkCoordinateMeters = state.wireWalkCoordinateMeters;
  result.wireWalkRailLengthMeters =
      vectorLength(state.wireWalkRailEndMeters - state.wireWalkRailStartMeters);
  result.startPosition = start;
  result.finalPosition = finalPosition;
  const CollisionQueryResult resultGround =
      sampleSurfaceHeight(*context.collisionSurfaces, finalPosition, params.footprintToleranceMeters);
  applyGroundSample(result, resultGround, finalPosition, params);
  result.reasonCode = playerMotorStatusName(PlayerMotorStatus::Ok);
  return result;
}

}  // namespace iggy3d
