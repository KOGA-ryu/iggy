#include "app/iggy3d/gameplay/MovementProof.hpp"

#include <string_view>

#include "app/iggy3d/ProductAppWindowState.hpp"

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
  packet.debugAvailable = window.gameplay.gameplayMovement.debugAvailable;
  packet.attempted = window.gameplay.gameplayMovement.attempted;
  packet.blocked = window.gameplay.gameplayMovement.blocked;
  packet.status = window.gameplay.gameplayMovement.status;
  packet.reasonCode = window.gameplay.gameplayMovement.reasonCode;
  packet.blockedReason = window.gameplay.gameplayMovement.blockedReason;
  packet.hitSurfaceId = window.gameplay.gameplayMovement.hitSurfaceId;
  packet.groundSnapApplied = window.gameplay.gameplayMovement.groundSnapApplied;
  packet.movementClamped = window.gameplay.gameplayMovement.clamped;
  packet.movementSlid = window.gameplay.gameplayMovement.slid;
  packet.collisionSweepCount = window.gameplay.gameplayMovement.collisionSweepCount;
  packet.policyBand = window.gameplay.gameplayMovement.policyBand;
  packet.slopeTravelDirection = window.gameplay.gameplayMovement.slopeTravelDirection;
  packet.slopeAngleDegrees = window.gameplay.gameplayMovement.slopeAngleDegrees;
  packet.speedMultiplier = window.gameplay.gameplayMovement.speedMultiplier;
  packet.startX = window.gameplay.gameplayMovement.startX;
  packet.startY = window.gameplay.gameplayMovement.startY;
  packet.startZ = window.gameplay.gameplayMovement.startZ;
  packet.finalX = window.gameplay.gameplayMovement.finalX;
  packet.finalY = window.gameplay.gameplayMovement.finalY;
  packet.finalZ = window.gameplay.gameplayMovement.finalZ;
  packet.horizontalDistanceMeters =
      window.gameplay.gameplayMovement.horizontalDistanceMeters;
  packet.verticalDeltaMeters = window.gameplay.gameplayMovement.verticalDeltaMeters;
  packet.groundVelocityX = window.gameplay.gameplayMovement.groundVelocityX;
  packet.groundVelocityZ = window.gameplay.gameplayMovement.groundVelocityZ;
  packet.state = window.gameplay.gameplayMovement.state;
  packet.stateName = std::string{productGameplayMovementStateName(packet.state)};
  packet.stateHudLabel =
      std::string{productGameplayMovementStateHudLabel(packet.state)};
  packet.grounded = window.gameplay.gameplayMovement.grounded;
  packet.horizontalSpeedMetersPerSecond =
      window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond;
  packet.verticalVelocityMetersPerSecond =
      window.gameplay.gameplayJump.velocityMetersPerSecond;
  packet.gradePercent = window.gameplay.gameplayMovement.gradePercent;
  packet.profile = window.gameplay.gameplayMovement.profile;
  packet.maxSpeedMetersPerSecond = window.gameplay.gameplayMovement.maxSpeedMetersPerSecond;

  packet.wallRunCandidateAvailable = window.gameplay.gameplayWallRun.candidateAvailable;
  packet.wallRunCandidateStatus = window.gameplay.gameplayWallRun.candidateStatus;
  packet.wallRunCandidateReasonCode = window.gameplay.gameplayWallRun.candidateReasonCode;
  packet.wallRunCandidateReasonHudLabel =
      wallRunHudLabel(packet.wallRunCandidateReasonCode);
  packet.wallRunSide = window.gameplay.gameplayWallRun.side;
  packet.wallRunSurfaceId = window.gameplay.gameplayWallRun.surfaceId;
  packet.wallRunNormalX = window.gameplay.gameplayWallRun.normalX;
  packet.wallRunNormalY = window.gameplay.gameplayWallRun.normalY;
  packet.wallRunNormalZ = window.gameplay.gameplayWallRun.normalZ;
  packet.wallRunApproachSpeedMetersPerSecond =
      window.gameplay.gameplayWallRun.approachSpeedMetersPerSecond;
  packet.wallRunActive = window.gameplay.gameplayWallRun.active;
  packet.wallRunStatus = window.gameplay.gameplayWallRun.status;
  packet.wallRunReasonCode = window.gameplay.gameplayWallRun.reasonCode;
  packet.wallRunReasonHudLabel = wallRunHudLabel(packet.wallRunReasonCode);
  packet.wallRunRemainingSeconds = window.gameplay.gameplayWallRun.remainingSeconds;
  packet.wallRunDurationSeconds = window.gameplay.gameplayWallRun.durationSeconds;
  packet.wallRunGravityMultiplier = window.gameplay.gameplayWallRun.gravityMultiplier;
  packet.wallRunSpeedMultiplier = window.gameplay.gameplayWallRun.speedMultiplier;
  return packet;
}

}  // namespace iggy3d
