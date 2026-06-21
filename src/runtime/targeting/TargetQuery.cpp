#include "runtime/targeting/TargetQuery.hpp"

#include <cmath>

#include "runtime/world/WorldState.hpp"

namespace iggy3d {

namespace {

bool commandKindToTargetAction(CommandKind kind, TargetAction& action) {
  switch (kind) {
    case CommandKind::Interact:
      action = TargetAction::Interact;
      return true;
    case CommandKind::Inspect:
      action = TargetAction::Inspect;
      return true;
    case CommandKind::Move:
      action = TargetAction::Move;
      return true;
    default:
      return false;
  }
}

float distanceMeters(Vec3 lhs, Vec3 rhs) {
  return std::sqrt(distanceSquared(lhs, rhs));
}

}  // namespace

bool targetSupportsCommandKind(const EntityState& entity, CommandKind kind) {
  TargetAction action = TargetAction::Inspect;
  if (!commandKindToTargetAction(kind, action)) {
    return false;
  }
  return isTargetActionSupported(entity.targeting, action);
}

TargetQueryResult queryTarget(const TargetQueryRequest& request) {
  TargetQueryResult result;
  if (request.world == nullptr) {
    result.status = TargetQueryStatus::InvalidWorld;
    return result;
  }

  Vec3 origin = request.origin;
  if (!request.hasOrigin) {
    const EntityState* actor = request.world->findById(request.actor);
    if (actor == nullptr || !actor->active) {
      result.status = TargetQueryStatus::InvalidActor;
      return result;
    }
    origin = actor->transform.position;
  }
  if (!isFinite(origin)) {
    result.status = TargetQueryStatus::InvalidOrigin;
    return result;
  }

  const bool hasDistanceCap = request.maxDistanceMeters > 0.0F;
  const float maxDistanceSquared = request.maxDistanceMeters * request.maxDistanceMeters;
  bool found = false;
  float bestDistanceSquared = 0.0F;
  EntityId bestId;
  Vec3 bestPoint;
  bool bestActive = false;
  bool bestSupports = false;

  for (const EntityState& entity : request.world->entities()) {
    if (!isValid(entity.id)) {
      continue;
    }
    if (!request.allowSelf && entity.id == request.actor) {
      continue;
    }
    if (request.requireActive && !entity.active) {
      continue;
    }
    if (!targetSupportsCommandKind(entity, request.commandKind)) {
      continue;
    }
    const Vec3 targetPoint = entity.transform.position;
    if (!isFinite(targetPoint)) {
      continue;
    }
    const float candidateDistanceSquared = distanceSquared(origin, targetPoint);
    if (!std::isfinite(candidateDistanceSquared)) {
      continue;
    }
    if (hasDistanceCap && candidateDistanceSquared > maxDistanceSquared) {
      continue;
    }
    if (!found || candidateDistanceSquared < bestDistanceSquared ||
        (candidateDistanceSquared == bestDistanceSquared && entity.id.value < bestId.value)) {
      found = true;
      bestDistanceSquared = candidateDistanceSquared;
      bestId = entity.id;
      bestPoint = targetPoint;
      bestActive = entity.active;
      bestSupports = true;
    }
  }

  result.origin = origin;
  if (!found) {
    result.status = TargetQueryStatus::NotFound;
    return result;
  }

  result.status = TargetQueryStatus::Found;
  result.target = bestId;
  result.targetPoint = bestPoint;
  result.distanceMeters = distanceMeters(origin, bestPoint);
  result.targetActive = bestActive;
  result.targetSupportsCommand = bestSupports;
  return result;
}

}  // namespace iggy3d
