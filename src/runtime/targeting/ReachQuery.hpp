#pragma once

#include <cstdint>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/command/Command.hpp"

namespace iggy3d {

class WorldState;

enum class ReachQueryStatus : std::uint8_t {
  Reachable,
  OutOfRange,
  InvalidWorld,
  InvalidActor,
  InvalidTarget,
  TargetInactive,
  InvalidPoint,
  InvalidRange,
};

struct ReachQueryRequest {
  const WorldState* world = nullptr;
  EntityId actor;
  EntityId target;
  bool hasTargetPoint = false;
  Vec3 targetPoint;
  float maxRangeMeters = 0.0F;
  bool requireActiveTarget = true;
};

struct ReachQueryResult {
  ReachQueryStatus status = ReachQueryStatus::InvalidWorld;
  EntityId actor;
  EntityId target;
  Vec3 actorPoint;
  Vec3 targetPoint;
  float maxRangeMeters = 0.0F;
  float distanceMeters = 0.0F;
};

ReachQueryResult queryReach(const ReachQueryRequest& request);
CommandRejectionReason rejectionReasonForReach(const ReachQueryResult& result);

}  // namespace iggy3d
