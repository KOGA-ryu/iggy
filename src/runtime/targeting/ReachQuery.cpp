#include "runtime/targeting/ReachQuery.hpp"

#include <cmath>

#include "runtime/world/WorldState.hpp"

namespace iggy3d {

namespace {

float distanceMeters(Vec3 lhs, Vec3 rhs) {
  return std::sqrt(distanceSquared(lhs, rhs));
}

}  // namespace

ReachQueryResult queryReach(const ReachQueryRequest& request) {
  ReachQueryResult result;
  result.actor = request.actor;
  result.target = request.target;
  result.targetPoint = request.targetPoint;
  result.maxRangeMeters = request.maxRangeMeters;

  if (request.world == nullptr) {
    result.status = ReachQueryStatus::InvalidWorld;
    return result;
  }

  const EntityState* actor = request.world->findById(request.actor);
  if (actor == nullptr || !actor->active) {
    result.status = ReachQueryStatus::InvalidActor;
    return result;
  }
  result.actorPoint = actor->transform.position;
  if (!isFinite(result.actorPoint)) {
    result.status = ReachQueryStatus::InvalidActor;
    return result;
  }

  if (request.hasTargetPoint) {
    result.targetPoint = request.targetPoint;
  } else {
    const EntityState* target = request.world->findById(request.target);
    if (target == nullptr) {
      result.status = ReachQueryStatus::InvalidTarget;
      return result;
    }
    if (request.requireActiveTarget && !target->active) {
      result.status = ReachQueryStatus::TargetInactive;
      return result;
    }
    result.targetPoint = target->transform.position;
  }

  if (!isFinite(result.targetPoint)) {
    result.status = ReachQueryStatus::InvalidPoint;
    return result;
  }
  if (!std::isfinite(request.maxRangeMeters) || request.maxRangeMeters <= 0.0F) {
    result.status = ReachQueryStatus::InvalidRange;
    return result;
  }

  result.distanceMeters = distanceMeters(result.actorPoint, result.targetPoint);
  result.status = result.distanceMeters <= request.maxRangeMeters ? ReachQueryStatus::Reachable
                                                                  : ReachQueryStatus::OutOfRange;
  return result;
}

CommandRejectionReason rejectionReasonForReach(const ReachQueryResult& result) {
  switch (result.status) {
    case ReachQueryStatus::Reachable:
      return CommandRejectionReason::None;
    case ReachQueryStatus::OutOfRange:
      return CommandRejectionReason::OutOfRange;
    case ReachQueryStatus::InvalidWorld:
      return CommandRejectionReason::InternalError;
    case ReachQueryStatus::InvalidActor:
      return CommandRejectionReason::InvalidActor;
    case ReachQueryStatus::InvalidTarget:
      return CommandRejectionReason::InvalidTarget;
    case ReachQueryStatus::TargetInactive:
      return CommandRejectionReason::TargetInactive;
    case ReachQueryStatus::InvalidPoint:
      return CommandRejectionReason::InvalidTargetPoint;
    case ReachQueryStatus::InvalidRange:
      return CommandRejectionReason::InternalError;
  }
  return CommandRejectionReason::InternalError;
}

}  // namespace iggy3d
