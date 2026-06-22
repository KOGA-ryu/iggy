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

struct TraversalIntentResult;
struct TraversalResult;
struct TraversalCandidatePreviewResult;

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
  const TraversalIntentResult* traversalIntentResult = nullptr;
  const TraversalResult* traversalResult = nullptr;
  const TraversalCandidatePreviewResult* traversalPreviewResult = nullptr;
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
  float movementHorizontalDistanceMeters = 0.0F;
  float movementVerticalDeltaMeters = 0.0F;
  float movementGradePercent = 0.0F;
  bool grounded = false;
  bool jumpAvailable = false;
  bool groundSampleValid = false;
  bool groundContact = false;
  bool groundWalkable = false;
  bool carefulFooting = false;
  PlayerMotorPhase motorPhase = PlayerMotorPhase::Grounded;
  Vec3 groundNormal = {0.0F, 1.0F, 0.0F};
  float groundDistanceMeters = 0.0F;
  float slopeAngleDegrees = 0.0F;
  float slopeUpDot = 1.0F;
  float speedMultiplier = 1.0F;
  float staminaCostMultiplier = 1.0F;
  float stepPenaltyMultiplier = 1.0F;
  float dashCooldownRemainingSeconds = 0.0F;
  float yawRadians = 0.0F;
  float pitchRadians = 0.0F;
  CommandTick sourceTick = kInvalidCommandTick;
  std::string movementPolicyBand;
  std::string slopeTravelDirection = "stationary";
  std::string groundSurfaceId;
  std::string hitSurfaceId;
  bool traversalPreviewAvailable = false;
  bool traversalPreviewReady = false;
  bool traversalPreviewCandidateAvailable = false;
  std::string traversalPreviewStatus = "traversal_preview_unavailable";
  std::string traversalPreviewHudCode = "NONE";
  std::string traversalPreviewMechanic = "none";
  std::string traversalPreviewSlotId = "none";
  std::string traversalPreviewSlotKind = "none";
  std::string traversalPreviewSlotHeightBand = "none";
  std::string traversalPreviewTargetId = "none";
  std::string traversalPreviewLandingSurfaceId = "none";
  float traversalPreviewSlotLedgeHeightMeters = 0.0F;
  float traversalPreviewSlotUsableWidthMeters = 0.0F;
  float traversalPreviewSlotStartRangeMeters = 0.0F;
  float traversalPreviewSlotFacingDot = 0.0F;
  bool traversalDebugAvailable = false;
  bool traversalIntentRequested = false;
  bool traversalIntentConsumed = false;
  bool traversalIntentAccepted = false;
  bool traversalIntentFallbackJumpAllowed = false;
  bool traversalAttempted = false;
  bool traversalAccepted = false;
  std::string traversalIntentTrigger = "none";
  std::string traversalIntentStatus = "traversal_intent_no_intent";
  std::string traversalIntentSelectedMechanic = "none";
  std::string traversalMechanic = "none";
  std::string traversalReason = "not_attempted";
  std::string traversalSlotId = "none";
  std::string traversalSlotKind = "none";
  std::string traversalSlotHeightBand = "none";
  std::string traversalTargetId = "none";
  std::string traversalLandingSurfaceId = "none";
  float traversalSlotLedgeHeightMeters = 0.0F;
  float traversalSlotUsableWidthMeters = 0.0F;
  float traversalSlotStartRangeMeters = 0.0F;
  float traversalSlotFacingDot = 0.0F;
  const char* reasonCode = "debug_overlay_disabled";
};

RuntimeDebugSnapshot buildRuntimeDebugSnapshot(const RuntimeDebugSnapshotRequest& request);
const char* runtimeDebugSnapshotStatusName(RuntimeDebugSnapshotStatus status);

}  // namespace iggy3d
