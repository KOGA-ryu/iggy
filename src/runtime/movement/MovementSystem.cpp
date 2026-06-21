#include "runtime/movement/MovementSystem.hpp"

#include <cmath>
#include <limits>

namespace iggy3d {

namespace {

inline constexpr float kFallbackMovementLimitMeters = 3.0F;

float movementLimitMeters(const MovementRequest& request, const RuntimeConfig* config) {
  if (request.maxDistanceMeters > 0.0F) {
    return request.maxDistanceMeters;
  }
  if (config != nullptr && std::isfinite(config->movementDistanceMeters) &&
      config->movementDistanceMeters > 0.0F) {
    return config->movementDistanceMeters;
  }
  return kFallbackMovementLimitMeters;
}

MovementResult blockedResult(const MovementRequest& request,
                             Vec3 start,
                             MovementBlockedReason reason,
                             float distanceMeters = 0.0F) {
  MovementResult result;
  result.actor = request.actor;
  result.start = start;
  result.destination = request.destination;
  result.finalPosition = start;
  result.mode = request.mode;
  result.blocked = reason;
  result.sourceCommandId = request.sourceCommandId;
  result.distanceMeters = distanceMeters;
  return result;
}

}  // namespace

float movementDistanceMeters(const Vec3& start, const Vec3& destination) {
  return std::sqrt(distanceSquared(start, destination));
}

float movementLimitMeters(const MovementRequest& request, const RuntimeConfig& config) {
  return movementLimitMeters(request, &config);
}

MovementBlockedReason validateMovementRequest(
    const MovementSystemContext& context,
    const MovementRequest& request) {
  if (context.world == nullptr) {
    return MovementBlockedReason::MissingWorld;
  }
  if (!isValid(request.actor)) {
    return MovementBlockedReason::InvalidActor;
  }
  const EntityState* actor = context.world->findById(request.actor);
  if (actor == nullptr) {
    return MovementBlockedReason::InvalidActor;
  }
  if (!actor->active) {
    return MovementBlockedReason::ActorInactive;
  }
  if (!isFinite(request.destination)) {
    return MovementBlockedReason::DestinationNotFinite;
  }
  const float distance = movementDistanceMeters(actor->transform.position, request.destination);
  const float limit = movementLimitMeters(request, context.config);
  if (!std::isfinite(distance) || !std::isfinite(limit) || limit <= 0.0F) {
    return MovementBlockedReason::InternalError;
  }
  if (distance > limit) {
    return MovementBlockedReason::MovementTooFar;
  }
  return MovementBlockedReason::None;
}

MovementResult executeMovement(MovementSystemContext& context, const MovementRequest& request) {
  Vec3 start;
  if (context.world == nullptr) {
    return blockedResult(request, start, MovementBlockedReason::MissingWorld);
  }
  if (!isValid(request.actor)) {
    return blockedResult(request, start, MovementBlockedReason::InvalidActor);
  }

  const EntityState* actor = context.world->findById(request.actor);
  if (actor == nullptr) {
    return blockedResult(request, start, MovementBlockedReason::InvalidActor);
  }
  start = actor->transform.position;
  if (!actor->active) {
    return blockedResult(request, start, MovementBlockedReason::ActorInactive);
  }
  if (!isFinite(request.destination)) {
    return blockedResult(request, start, MovementBlockedReason::DestinationNotFinite);
  }

  const float distance = movementDistanceMeters(start, request.destination);
  const float limit = movementLimitMeters(request, context.config);
  if (!std::isfinite(distance) || !std::isfinite(limit) || limit <= 0.0F) {
    return blockedResult(request, start, MovementBlockedReason::InternalError, distance);
  }
  if (distance > limit) {
    return blockedResult(request, start, MovementBlockedReason::MovementTooFar, distance);
  }

  Transform3 nextTransform = actor->transform;
  nextTransform.position = request.destination;
  const WorldMutationResult mutation = context.world->updateTransform(request.actor, nextTransform);
  if (mutation.status != WorldStatus::Ok) {
    return blockedResult(request, start, MovementBlockedReason::BlockedByWorld, distance);
  }

  MovementResult result;
  result.actor = request.actor;
  result.start = start;
  result.destination = request.destination;
  result.finalPosition = request.destination;
  result.mode = request.mode;
  result.blocked = MovementBlockedReason::None;
  result.sourceCommandId = request.sourceCommandId;
  result.distanceMeters = distance;
  return result;
}

MovementRequest movementRequestFromAcceptedCommand(
    const CommandRecord& command,
    MovementMode mode,
    const RuntimeConfig& config) {
  MovementRequest request;
  request.actor = command.actor;
  request.mode = mode;
  request.maxDistanceMeters = config.movementDistanceMeters;
  request.sourceCommandId = command.commandId;

  if (command.kind != CommandKind::Move ||
      command.admission != CommandAdmissionStatus::Accepted ||
      !command.payload.target.hasPoint ||
      !isFinite(command.payload.target.point)) {
    const float quietNan = std::numeric_limits<float>::quiet_NaN();
    request.destination = {quietNan, quietNan, quietNan};
    return request;
  }

  request.destination = command.payload.target.point;
  return request;
}

}  // namespace iggy3d
