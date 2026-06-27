#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/movement/MovementParams.hpp"
#include "runtime/physics/PhysicsAabbCollider.hpp"
#include "runtime/physics/PhysicsFrameStats.hpp"
#include "runtime/physics/PhysicsKinematicMotor.hpp"

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
  MissingCollisionSurfaces,
  InvalidMovementParams,
  NoWalkableGround,
  SlopeRejected,
  BlockedByCollision,
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
  float horizontalDistanceMeters = 0.0F;
  float verticalDeltaMeters = 0.0F;
  float gradePercent = 0.0F;
  bool kinematic = false;
  bool movementClamped = false;
  bool movementSlid = false;
  bool groundSnapApplied = false;
  bool carefulFooting = false;
  float slopeAngleDegrees = 0.0F;
  float slopeUpDot = 0.0F;
  float speedMultiplier = 1.0F;
  float staminaCostMultiplier = 1.0F;
  float stepPenaltyMultiplier = 1.0F;
  std::uint32_t collisionSweepCount = 0;
  bool physicsFrameStatsAvailable = false;
  PhysicsFrameStats physicsFrameStats;
  bool physicsDebugGeometryAvailable = false;
  std::vector<PhysicsAabbCollider> physicsDebugAabbColliders;
  std::vector<std::string> physicsDebugAabbSourceSurfaceIds;
  std::vector<PhysicsKinematicMotorHit> physicsDebugHits;
  std::vector<std::string> physicsDebugHitSourceSurfaceIds;
  std::string slopeTravelDirection = "stationary";
  std::string movementPolicyBand;
  std::string hitSurfaceId;
  std::string reasonCode = "movement_ok";
};

struct KinematicMovementRequest {
  EntityId actor;
  Vec3 intent;
  MovementMode mode = MovementMode::Walk;
  MovementParams params;
  float seconds = 0.0F;
  CommandId sourceCommandId = kInvalidCommandId;
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
