#include "runtime/player/PlayerMotor.hpp"

#include "runtime/collision/CollisionQuery.hpp"

#include <algorithm>
#include <cmath>

namespace iggy3d {
namespace {

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
         params.terminalVelocityMetersPerSecond < 0.0F;
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
  result.verticalVelocityMetersPerSecond = state.verticalVelocityMetersPerSecond;
  result.gravityMetersPerSecondSquared = params.gravityMetersPerSecondSquared;
  result.jumpImpulseMetersPerSecond = params.jumpImpulseMetersPerSecond;
  result.startPosition = start;
  result.finalPosition = start;
  result.reasonCode = playerMotorStatusName(status);
  return result;
}

bool closeToGround(const CollisionQueryResult& ground, Vec3 position, const PlayerMotorParams& params) {
  return ground.status == CollisionQueryStatus::Hit &&
         std::fabs(position.y - ground.heightMeters) <= params.groundProbeMeters;
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
  if (!validParams(params) || !std::isfinite(input.seconds) || input.seconds < 0.0F) {
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
    state.phase = PlayerMotorPhase::Airborne;
    state.grounded = false;
    state.jumpAvailable = false;
    state.verticalVelocityMetersPerSecond = params.jumpImpulseMetersPerSecond;
    result.jumpAccepted = true;
  }

  Vec3 finalPosition = start;
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
    if (landingGround.status == CollisionQueryStatus::Hit &&
        finalPosition.y <= landingGround.heightMeters + params.landingSnapMeters &&
        state.verticalVelocityMetersPerSecond <= 0.0F) {
      finalPosition.y = landingGround.heightMeters;
      state.phase = PlayerMotorPhase::Grounded;
      state.grounded = true;
      state.jumpAvailable = true;
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
  result.verticalVelocityMetersPerSecond = state.verticalVelocityMetersPerSecond;
  result.startPosition = start;
  result.finalPosition = finalPosition;
  result.reasonCode = playerMotorStatusName(PlayerMotorStatus::Ok);
  return result;
}

}  // namespace iggy3d
