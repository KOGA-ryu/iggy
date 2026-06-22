#pragma once

#include <cstdint>

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
};

struct PlayerMotorParams {
  float gravityMetersPerSecondSquared = 18.0F;
  float jumpImpulseMetersPerSecond = 5.8F;
  float groundProbeMeters = 0.20F;
  float landingSnapMeters = 0.30F;
  float footprintToleranceMeters = 0.30F;
  float terminalVelocityMetersPerSecond = -40.0F;
};

struct PlayerMotorState {
  EntityId actor;
  PlayerMotorPhase phase = PlayerMotorPhase::Grounded;
  float verticalVelocityMetersPerSecond = 0.0F;
  bool grounded = true;
  bool jumpAvailable = true;
};

struct PlayerMotorInput {
  bool jumpPressed = false;
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
  bool landed = false;
  bool groundSnapApplied = false;
  bool mutatedWorld = false;
  float verticalVelocityMetersPerSecond = 0.0F;
  float gravityMetersPerSecondSquared = 0.0F;
  float jumpImpulseMetersPerSecond = 0.0F;
  Vec3 startPosition;
  Vec3 finalPosition;
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
