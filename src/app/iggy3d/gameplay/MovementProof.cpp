#include "app/iggy3d/gameplay/MovementProof.hpp"

#include <string_view>

#include "app/iggy3d/ReceiptBuilder.hpp"

namespace iggy3d {
namespace {

std::string wallRunHudLabel(std::string_view key) {
  const ProductWallRunStatusDescriptor* descriptor =
      findProductWallRunStatusDescriptor(key);
  // branch-gate: BG-1157
  if (descriptor != nullptr) {
    return std::string{descriptor->hudLabel};
  }
  return std::string{key};
}

}  // namespace

ProductMovementProofPacket buildProductMovementProofPacket(
    const ProductAppWindowState& window) {
  ProductMovementProofPacket packet;
  packet.debugAvailable = window.gameplayMovementDebugAvailable;
  packet.attempted = window.gameplayMovementAttempted;
  packet.blocked = window.gameplayMovementBlocked;
  packet.status = window.gameplayMovementStatus;
  packet.reasonCode = window.gameplayMovementReasonCode;
  packet.blockedReason = window.gameplayMovementBlockedReason;
  packet.hitSurfaceId = window.gameplayMovementHitSurfaceId;
  packet.groundSnapApplied = window.gameplayMovementGroundSnapApplied;
  packet.movementClamped = window.gameplayMovementClamped;
  packet.movementSlid = window.gameplayMovementSlid;
  packet.collisionSweepCount = window.gameplayMovementCollisionSweepCount;
  packet.policyBand = window.gameplayMovementPolicyBand;
  packet.slopeTravelDirection = window.gameplayMovementSlopeTravelDirection;
  packet.slopeAngleDegrees = window.gameplayMovementSlopeAngleDegrees;
  packet.speedMultiplier = window.gameplayMovementSpeedMultiplier;
  packet.startX = window.gameplayMovementStartX;
  packet.startY = window.gameplayMovementStartY;
  packet.startZ = window.gameplayMovementStartZ;
  packet.finalX = window.gameplayMovementFinalX;
  packet.finalY = window.gameplayMovementFinalY;
  packet.finalZ = window.gameplayMovementFinalZ;
  packet.horizontalDistanceMeters =
      window.gameplayMovementHorizontalDistanceMeters;
  packet.verticalDeltaMeters = window.gameplayMovementVerticalDeltaMeters;
  packet.groundVelocityX = window.gameplayMovementGroundVelocityX;
  packet.groundVelocityZ = window.gameplayMovementGroundVelocityZ;
  packet.state = window.gameplayMovementState;
  packet.stateName = std::string{productGameplayMovementStateName(packet.state)};
  packet.stateHudLabel =
      std::string{productGameplayMovementStateHudLabel(packet.state)};
  packet.grounded = window.gameplayMovementGrounded;
  packet.horizontalSpeedMetersPerSecond =
      window.gameplayMovementHorizontalSpeedMetersPerSecond;
  packet.verticalVelocityMetersPerSecond =
      window.gameplayJumpVelocityMetersPerSecond;
  packet.gradePercent = window.gameplayMovementGradePercent;
  packet.profile = window.gameplayMovementProfile;
  packet.maxSpeedMetersPerSecond = window.gameplayMovementMaxSpeedMetersPerSecond;

  packet.wallRunCandidateAvailable = window.gameplayWallRunCandidateAvailable;
  packet.wallRunCandidateStatus = window.gameplayWallRunCandidateStatus;
  packet.wallRunCandidateReasonCode = window.gameplayWallRunCandidateReasonCode;
  packet.wallRunCandidateReasonHudLabel =
      wallRunHudLabel(packet.wallRunCandidateReasonCode);
  packet.wallRunSide = window.gameplayWallRunSide;
  packet.wallRunSurfaceId = window.gameplayWallRunSurfaceId;
  packet.wallRunNormalX = window.gameplayWallRunNormalX;
  packet.wallRunNormalY = window.gameplayWallRunNormalY;
  packet.wallRunNormalZ = window.gameplayWallRunNormalZ;
  packet.wallRunApproachSpeedMetersPerSecond =
      window.gameplayWallRunApproachSpeedMetersPerSecond;
  packet.wallRunActive = window.gameplayWallRunActive;
  packet.wallRunStatus = window.gameplayWallRunStatus;
  packet.wallRunReasonCode = window.gameplayWallRunReasonCode;
  packet.wallRunReasonHudLabel = wallRunHudLabel(packet.wallRunReasonCode);
  packet.wallRunRemainingSeconds = window.gameplayWallRunRemainingSeconds;
  packet.wallRunDurationSeconds = window.gameplayWallRunDurationSeconds;
  packet.wallRunGravityMultiplier = window.gameplayWallRunGravityMultiplier;
  packet.wallRunSpeedMultiplier = window.gameplayWallRunSpeedMultiplier;
  return packet;
}

}  // namespace iggy3d
