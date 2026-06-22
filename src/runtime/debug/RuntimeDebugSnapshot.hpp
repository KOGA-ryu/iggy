#pragma once

#include <cstdint>
#include <string>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/movement/MovementCommand.hpp"
#include "runtime/player/PlayerMotor.hpp"
#include "runtime/session/SessionState.hpp"

namespace iggy3d {

enum class RuntimeDebugSnapshotStatus : std::uint8_t {
  Ok,
  Disabled,
  MissingSession,
  InvalidActor,
  ActorInactive,
  InvalidDelta,
};

struct RuntimeDebugSnapshotRequest {
  bool enabled = false;
  const SessionState* session = nullptr;
  EntityId actor;
  bool hasPreviousPosition = false;
  Vec3 previousPosition;
  bool hasSpawnPosition = false;
  Vec3 spawnPosition;
  float deltaSeconds = 0.0F;
  const PlayerMotorState* motorState = nullptr;
  const PlayerMotorResult* motorResult = nullptr;
  const MovementResult* movementResult = nullptr;
  float yawRadians = 0.0F;
  float pitchRadians = 0.0F;
};

struct RuntimeDebugSnapshot {
  RuntimeDebugSnapshotStatus status = RuntimeDebugSnapshotStatus::Disabled;
  bool enabled = false;
  EntityId actor;
  bool playerPositionAvailable = false;
  bool speedAvailable = false;
  bool hasSpawnDistance = false;
  Vec3 position;
  Vec3 previousPosition;
  Vec3 displacementThisFrame;
  float deltaSeconds = 0.0F;
  float movedThisFrameMeters = 0.0F;
  float horizontalSpeedMetersPerSecond = 0.0F;
  float verticalSpeedMetersPerSecond = 0.0F;
  float distanceFromSpawnMeters = 0.0F;
  bool grounded = false;
  bool jumpAvailable = false;
  PlayerMotorPhase motorPhase = PlayerMotorPhase::Grounded;
  float dashCooldownRemainingSeconds = 0.0F;
  float yawRadians = 0.0F;
  float pitchRadians = 0.0F;
  CommandTick sourceTick = kInvalidCommandTick;
  std::string movementPolicyBand;
  std::string hitSurfaceId;
  const char* reasonCode = "debug_overlay_disabled";
};

RuntimeDebugSnapshot buildRuntimeDebugSnapshot(const RuntimeDebugSnapshotRequest& request);
const char* runtimeDebugSnapshotStatusName(RuntimeDebugSnapshotStatus status);

}  // namespace iggy3d
