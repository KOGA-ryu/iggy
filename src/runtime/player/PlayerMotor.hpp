#pragma once

#include <cstdint>
#include <string>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/world/WorldState.hpp"

namespace iggy3d {

enum class PlayerMotorStatus : std::uint8_t {
  Ok,
  MissingWorld,
  MissingCollisionSurfaces,
  InvalidActor,
  ActorInactive,
  InvalidParameters,
  NoGround,
};

enum class PlayerMotorPhase : std::uint8_t {
  Grounded,
  Airborne,
  WireWalk,
};

struct PlayerMotorParams {
  float gravityMetersPerSecondSquared = 18.0F;
  float jumpImpulseMetersPerSecond = 5.8F;
  float groundProbeMeters = 0.20F;
  float landingSnapMeters = 0.30F;
  float footprintToleranceMeters = 0.30F;
  float maxWalkableSlopeDegrees = 40.0F;
  float terminalVelocityMetersPerSecond = -40.0F;
  float airMaxSpeedMetersPerSecond = 3.20F;
  float airAccelerationMetersPerSecondSquared = 9.50F;
  float airDragPerSecond = 0.25F;
  float airLaunchSpeedMetersPerSecond = 2.00F;
  float airCollisionProbeHeightMeters = 0.90F;
  float airCollisionSkinMeters = 0.03F;
  float dashSpeedMetersPerSecond = 9.50F;
  float dashDurationSeconds = 0.18F;
  float dashCooldownSeconds = 0.45F;
  float wireWalkSpeedMetersPerSecond = 2.25F;
};

struct PlayerMotorState {
  EntityId actor;
  PlayerMotorPhase phase = PlayerMotorPhase::Grounded;
  Vec3 horizontalVelocityMetersPerSecond;
  Vec3 dashDirection;
  Vec3 wireWalkRailStartMeters;
  Vec3 wireWalkRailEndMeters;
  Vec3 wireWalkAxis = {1.0F, 0.0F, 0.0F};
  float verticalVelocityMetersPerSecond = 0.0F;
  float dashRemainingSeconds = 0.0F;
  float dashCooldownRemainingSeconds = 0.0F;
  float wireWalkCoordinateMeters = 0.0F;
  bool grounded = true;
  bool jumpAvailable = true;
};

struct PlayerMotorInput {
  Vec3 moveIntent;
  bool jumpPressed = false;
  bool dashPressed = false;
  bool crouched = false;
  float seconds = 0.0F;
};

struct PlayerMotorResult {
  PlayerMotorStatus status = PlayerMotorStatus::Ok;
  EntityId actor;
  PlayerMotorPhase phase = PlayerMotorPhase::Grounded;
  bool grounded = true;
  bool wasGrounded = true;
  bool jumpRequested = false;
  bool jumpAccepted = false;
  bool dashRequested = false;
  bool dashAccepted = false;
  bool dashActive = false;
  bool dashRejectedNoIntent = false;
  bool dashRejectedCooldown = false;
  bool landed = false;
  bool groundSnapApplied = false;
  bool airMoveIntent = false;
  bool airControlActive = false;
  bool airMovementClamped = false;
  bool airMovementSlid = false;
  bool dashMovementClamped = false;
  bool dashMovementSlid = false;
  bool wireWalkActive = false;
  bool wireWalkMoved = false;
  bool wireWalkEndpointReached = false;
  bool groundSampleValid = false;
  bool groundContact = false;
  bool groundWalkable = false;
  bool carefulFooting = false;
  bool mutatedWorld = false;
  Vec3 groundNormal = {0.0F, 1.0F, 0.0F};
  Vec3 horizontalVelocityMetersPerSecond;
  float verticalVelocityMetersPerSecond = 0.0F;
  float horizontalSpeedMetersPerSecond = 0.0F;
  float groundDistanceMeters = 0.0F;
  float slopeAngleDegrees = 0.0F;
  float slopeUpDot = 1.0F;
  float speedMultiplier = 1.0F;
  float staminaCostMultiplier = 1.0F;
  float stepPenaltyMultiplier = 1.0F;
  float dashRemainingSeconds = 0.0F;
  float dashCooldownRemainingSeconds = 0.0F;
  float gravityMetersPerSecondSquared = 0.0F;
  float jumpImpulseMetersPerSecond = 0.0F;
  float airMaxSpeedMetersPerSecond = 0.0F;
  float airAccelerationMetersPerSecondSquared = 0.0F;
  float dashSpeedMetersPerSecond = 0.0F;
  float dashDurationSeconds = 0.0F;
  float dashCooldownSeconds = 0.0F;
  float wireWalkSpeedMetersPerSecond = 0.0F;
  float wireWalkCoordinateMeters = 0.0F;
  float wireWalkRailLengthMeters = 0.0F;
  Vec3 startPosition;
  Vec3 finalPosition;
  std::string movementPolicyBand = "not_sampled";
  std::string groundSurfaceId;
  std::string hitSurfaceId;
  const char* reasonCode = "player_motor_ok";
};

struct PlayerMotorContext {
  WorldState* world = nullptr;
  const SpatialSurfaceSet* collisionSurfaces = nullptr;
};

bool playerMotorSucceeded(const PlayerMotorResult& result);
const char* playerMotorStatusName(PlayerMotorStatus status);
const char* playerMotorPhaseName(PlayerMotorPhase phase);

PlayerMotorResult updatePlayerMotor(PlayerMotorContext& context,
                                    PlayerMotorState& state,
                                    const PlayerMotorInput& input,
                                    const PlayerMotorParams& params = {});

}  // namespace iggy3d
