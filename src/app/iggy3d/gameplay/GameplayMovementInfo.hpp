#pragma once

#include <cstdint>
#include <string>

#include "app/iggy3d/gameplay/MovementTuning.hpp"  // ProductGameplayMovementState/Tuning/TuningField, kProductGameplayMovementTuning

namespace iggy3d {

// Owned gameplay-movement report/mirror state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Named *Info (not *State) because ProductGameplayMovementState
// is the existing movement enum. Domain: gameplay. Behavior-identical (defaults preserved).
struct ProductGameplayMovementInfo {
  bool attempted = false;
  bool blocked = false;
  std::string status = "not_requested";
  bool debugAvailable = false;
  std::string reasonCode = "not_requested";
  std::string blockedReason = "none";
  std::string hitSurfaceId = "none";
  bool groundSnapApplied = false;
  bool clamped = false;
  bool slid = false;
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
  ProductGameplayMovementState state = ProductGameplayMovementState::IdleGrounded;
  bool grounded = true;
  float horizontalSpeedMetersPerSecond = 0.0F;
  float gradePercent = 0.0F;
  std::string profile = std::string{kProductGameplayMovementTuning.walkProfile};
  float maxSpeedMetersPerSecond =
      kProductGameplayMovementTuning.walkSpeedMetersPerSecond;
  ProductGameplayMovementTuning tuning = productGameplayMovementTuning();
  ProductGameplayMovementTuningField tuningSelectedField =
      ProductGameplayMovementTuningField::WalkSpeed;
  bool tuningVisible = false;
  std::string tuningStatus = "movement_tuning_ready";
  std::string tuningReasonCode = "movement_tuning_ready";
};

}  // namespace iggy3d
