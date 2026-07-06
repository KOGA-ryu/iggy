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
  packet.debugAvailable = window.gameplayMovement.debugAvailable;
  packet.attempted = window.gameplayMovement.attempted;
  packet.blocked = window.gameplayMovement.blocked;
  packet.status = window.gameplayMovement.status;
  packet.reasonCode = window.gameplayMovement.reasonCode;
  packet.blockedReason = window.gameplayMovement.blockedReason;
  packet.hitSurfaceId = window.gameplayMovement.hitSurfaceId;
  packet.groundSnapApplied = window.gameplayMovement.groundSnapApplied;
  packet.movementClamped = window.gameplayMovement.clamped;
  packet.movementSlid = window.gameplayMovement.slid;
  packet.collisionSweepCount = window.gameplayMovement.collisionSweepCount;
  packet.policyBand = window.gameplayMovement.policyBand;
  packet.slopeTravelDirection = window.gameplayMovement.slopeTravelDirection;
  packet.slopeAngleDegrees = window.gameplayMovement.slopeAngleDegrees;
  packet.speedMultiplier = window.gameplayMovement.speedMultiplier;
  packet.startX = window.gameplayMovement.startX;
  packet.startY = window.gameplayMovement.startY;
  packet.startZ = window.gameplayMovement.startZ;
  packet.finalX = window.gameplayMovement.finalX;
  packet.finalY = window.gameplayMovement.finalY;
  packet.finalZ = window.gameplayMovement.finalZ;
  packet.horizontalDistanceMeters =
      window.gameplayMovement.horizontalDistanceMeters;
  packet.verticalDeltaMeters = window.gameplayMovement.verticalDeltaMeters;
  packet.groundVelocityX = window.gameplayMovement.groundVelocityX;
  packet.groundVelocityZ = window.gameplayMovement.groundVelocityZ;
  packet.state = window.gameplayMovement.state;
  packet.stateName = std::string{productGameplayMovementStateName(packet.state)};
  packet.stateHudLabel =
      std::string{productGameplayMovementStateHudLabel(packet.state)};
  packet.grounded = window.gameplayMovement.grounded;
  packet.horizontalSpeedMetersPerSecond =
      window.gameplayMovement.horizontalSpeedMetersPerSecond;
  packet.verticalVelocityMetersPerSecond =
      window.gameplayJump.velocityMetersPerSecond;
  packet.gradePercent = window.gameplayMovement.gradePercent;
  packet.profile = window.gameplayMovement.profile;
  packet.maxSpeedMetersPerSecond = window.gameplayMovement.maxSpeedMetersPerSecond;

  packet.wallRunCandidateAvailable = window.gameplayWallRun.candidateAvailable;
  packet.wallRunCandidateStatus = window.gameplayWallRun.candidateStatus;
  packet.wallRunCandidateReasonCode = window.gameplayWallRun.candidateReasonCode;
  packet.wallRunCandidateReasonHudLabel =
      wallRunHudLabel(packet.wallRunCandidateReasonCode);
  packet.wallRunSide = window.gameplayWallRun.side;
  packet.wallRunSurfaceId = window.gameplayWallRun.surfaceId;
  packet.wallRunNormalX = window.gameplayWallRun.normalX;
  packet.wallRunNormalY = window.gameplayWallRun.normalY;
  packet.wallRunNormalZ = window.gameplayWallRun.normalZ;
  packet.wallRunApproachSpeedMetersPerSecond =
      window.gameplayWallRun.approachSpeedMetersPerSecond;
  packet.wallRunActive = window.gameplayWallRun.active;
  packet.wallRunStatus = window.gameplayWallRun.status;
  packet.wallRunReasonCode = window.gameplayWallRun.reasonCode;
  packet.wallRunReasonHudLabel = wallRunHudLabel(packet.wallRunReasonCode);
  packet.wallRunRemainingSeconds = window.gameplayWallRun.remainingSeconds;
  packet.wallRunDurationSeconds = window.gameplayWallRun.durationSeconds;
  packet.wallRunGravityMultiplier = window.gameplayWallRun.gravityMultiplier;
  packet.wallRunSpeedMultiplier = window.gameplayWallRun.speedMultiplier;
  return packet;
}

}  // namespace iggy3d
