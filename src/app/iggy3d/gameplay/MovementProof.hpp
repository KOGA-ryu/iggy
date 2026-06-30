#pragma once

#include <cstdint>
#include <string>

#include "app/iggy3d/gameplay/MovementTuning.hpp"

namespace iggy3d {

struct ProductAppWindowState;

struct ProductMovementProofPacket {
  bool debugAvailable = false;
  bool attempted = false;
  bool blocked = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string blockedReason = "none";
  std::string hitSurfaceId = "none";
  bool groundSnapApplied = false;
  bool movementClamped = false;
  bool movementSlid = false;
  std::uint64_t collisionSweepCount = 0;
  std::string policyBand = "none";
  std::string slopeTravelDirection = "stationary";
  float slopeAngleDegrees = 0.0F;
  float speedMultiplier = 1.0F;
  float startX = 0.0F;
  float startY = 0.0F;
  float startZ = 0.0F;
  float finalX = 0.0F;
  float finalY = 0.0F;
  float finalZ = 0.0F;
  float horizontalDistanceMeters = 0.0F;
  float verticalDeltaMeters = 0.0F;
  float groundVelocityX = 0.0F;
  float groundVelocityZ = 0.0F;
  ProductGameplayMovementState state =
      ProductGameplayMovementState::IdleGrounded;
  std::string stateName = "idle_grounded";
  std::string stateHudLabel = "idle grounded";
  bool grounded = true;
  float horizontalSpeedMetersPerSecond = 0.0F;
  float verticalVelocityMetersPerSecond = 0.0F;
  float gradePercent = 0.0F;
  std::string profile = "manual_first_person";
  float maxSpeedMetersPerSecond =
      kProductGameplayMovementTuning.walkSpeedMetersPerSecond;

  bool wallRunCandidateAvailable = false;
  std::string wallRunCandidateStatus = "wall_run_not_checked";
  std::string wallRunCandidateReasonCode = "wall_run_not_checked";
  std::string wallRunCandidateReasonHudLabel = "wall run not checked";
  std::string wallRunSide = "none";
  std::string wallRunSurfaceId = "none";
  float wallRunNormalX = 0.0F;
  float wallRunNormalY = 0.0F;
  float wallRunNormalZ = 0.0F;
  float wallRunApproachSpeedMetersPerSecond = 0.0F;
  bool wallRunActive = false;
  std::string wallRunStatus = "wall_run_inactive";
  std::string wallRunReasonCode = "wall_run_inactive";
  std::string wallRunReasonHudLabel = "wall run inactive";
  float wallRunRemainingSeconds = 0.0F;
  float wallRunDurationSeconds = 0.0F;
  float wallRunGravityMultiplier = 1.0F;
  float wallRunSpeedMultiplier = 1.0F;
};

ProductMovementProofPacket buildProductMovementProofPacket(
    const ProductAppWindowState& window);

}  // namespace iggy3d
