#pragma once

#include <cstdint>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/command/Command.hpp"

namespace iggy3d {

enum class MovementMode : std::uint8_t {
  Walk,
  Tactical,
  Reposition,
};

enum class MovementBlockedReason : std::uint8_t {
  None,
  InvalidActor,
  ActorInactive,
  InvalidDestination,
  DestinationNotFinite,
  MovementTooFar,
  BlockedByWorld,
  MissingWorld,
  InternalError,
};

struct MovementRequest {
  EntityId actor;
  Vec3 destination;
  MovementMode mode = MovementMode::Walk;
  float maxDistanceMeters = 0.0F;
  CommandId sourceCommandId = kInvalidCommandId;
};

struct MovementResult {
  EntityId actor;
  Vec3 start;
  Vec3 destination;
  Vec3 finalPosition;
  MovementMode mode = MovementMode::Walk;
  MovementBlockedReason blocked = MovementBlockedReason::None;
  CommandId sourceCommandId = kInvalidCommandId;
  float distanceMeters = 0.0F;
};

inline bool movementSucceeded(const MovementResult& result) {
  return result.blocked == MovementBlockedReason::None;
}

inline bool movementBlocked(const MovementResult& result) {
  return !movementSucceeded(result);
}

inline MovementMode movementModeForClock(bool tacticalModeActive) {
  return tacticalModeActive ? MovementMode::Tactical : MovementMode::Walk;
}

}  // namespace iggy3d
