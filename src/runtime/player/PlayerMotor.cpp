#include "runtime/player/PlayerMotor.hpp"

#include "runtime/collision/CollisionQuery.hpp"

#include <algorithm>
#include <cmath>

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
         std::isfinite(params.footprintToleranceMeters) &&
         params.footprintToleranceMeters >= 0.0F &&
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
         params.airCollisionSkinMeters >= 0.0F;
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
  result.gravityMetersPerSecondSquared = params.gravityMetersPerSecondSquared;
  result.jumpImpulseMetersPerSecond = params.jumpImpulseMetersPerSecond;
  result.airMaxSpeedMetersPerSecond = params.airMaxSpeedMetersPerSecond;
  result.airAccelerationMetersPerSecondSquared = params.airAccelerationMetersPerSecondSquared;
  result.startPosition = start;
  result.finalPosition = start;
  result.reasonCode = playerMotorStatusName(status);
  return result;
}

bool closeToGround(const CollisionQueryResult& ground, Vec3 position, const PlayerMotorParams& params) {
  return ground.status == CollisionQueryStatus::Hit &&
         std::fabs(position.y - ground.heightMeters) <= params.groundProbeMeters;
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

void applyAirCollision(const SpatialSurfaceSet& surfaces,
                       const PlayerMotorParams& params,
                       Vec3 start,
                       Vec3 horizontalDisplacement,
                       Vec3& finalPosition,
                       PlayerMotorState& state,
                       PlayerMotorResult& result) {
  const float horizontalDistance = vectorLength(horizontalDisplacement);
  if (!std::isfinite(horizontalDistance) || horizontalDistance <= kPlayerMotorEpsilon) {
    return;
  }

  Vec3 candidate = finalPosition;
  const Vec3 sweepStart = start + vec3UnitY() * params.airCollisionProbeHeightMeters;
  const Vec3 sweepEnd = candidate + vec3UnitY() * params.airCollisionProbeHeightMeters;
  const CollisionQueryResult hit =
      querySegment(surfaces, sweepStart, sweepEnd, CollisionQueryKind::Actor);
  if (hit.status != CollisionQueryStatus::Hit) {
    return;
  }

  result.airMovementClamped = true;
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
    result.airMovementSlid = true;
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
      state.horizontalVelocityMetersPerSecond =
          withoutNormal(state.horizontalVelocityMetersPerSecond, slideHit.normal);
    } else {
      candidate = slideCandidate;
    }
  }

  state.horizontalVelocityMetersPerSecond =
      withoutNormal(state.horizontalVelocityMetersPerSecond, hit.normal);
  finalPosition.x = candidate.x;
  finalPosition.z = candidate.z;
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
  }
  return "invalid_parameters";
}

const char* playerMotorPhaseName(PlayerMotorPhase phase) {
  switch (phase) {
    case PlayerMotorPhase::Grounded:
      return "grounded";
    case PlayerMotorPhase::Airborne:
      return "airborne";
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
  const bool nearGround = closeToGround(ground, start, params);
  if (ground.status != CollisionQueryStatus::Hit && state.phase == PlayerMotorPhase::Grounded) {
    return baseResult(state, start, PlayerMotorStatus::NoGround, params);
  }

  PlayerMotorResult result = baseResult(state, start, PlayerMotorStatus::Ok, params);
  result.jumpRequested = input.jumpPressed;

  if (nearGround && state.phase == PlayerMotorPhase::Grounded &&
      state.verticalVelocityMetersPerSecond <= 0.0F) {
    state.grounded = true;
    state.jumpAvailable = true;
    state.horizontalVelocityMetersPerSecond = {};
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

  if (input.jumpPressed && state.grounded && state.jumpAvailable) {
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

  Vec3 finalPosition = start;
  if (state.phase == PlayerMotorPhase::Airborne && input.seconds > 0.0F) {
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

    const float previousVelocity = state.verticalVelocityMetersPerSecond;
    const float nextVelocity =
        std::max(params.terminalVelocityMetersPerSecond,
                 previousVelocity - params.gravityMetersPerSecondSquared * input.seconds);
    const float displacement = ((previousVelocity + nextVelocity) * 0.5F) * input.seconds;
    finalPosition.y = start.y + displacement;
    state.verticalVelocityMetersPerSecond = nextVelocity;
    const Vec3 horizontalDisplacement = state.horizontalVelocityMetersPerSecond * input.seconds;
    finalPosition.x = start.x + horizontalDisplacement.x;
    finalPosition.z = start.z + horizontalDisplacement.z;
    applyAirCollision(*context.collisionSurfaces,
                      params,
                      start,
                      horizontalDisplacement,
                      finalPosition,
                      state,
                      result);

    const CollisionQueryResult landingGround = sampleSurfaceHeight(
        *context.collisionSurfaces, finalPosition, params.footprintToleranceMeters);
    if (landingGround.status == CollisionQueryStatus::Hit &&
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
  result.horizontalVelocityMetersPerSecond = state.horizontalVelocityMetersPerSecond;
  result.verticalVelocityMetersPerSecond = state.verticalVelocityMetersPerSecond;
  result.horizontalSpeedMetersPerSecond = vectorLength(state.horizontalVelocityMetersPerSecond);
  result.startPosition = start;
  result.finalPosition = finalPosition;
  result.reasonCode = playerMotorStatusName(PlayerMotorStatus::Ok);
  return result;
}

}  // namespace iggy3d
