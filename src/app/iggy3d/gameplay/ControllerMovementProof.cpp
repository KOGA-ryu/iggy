#include "app/iggy3d/gameplay/ControllerMovementProof.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ControllerKinematics.hpp"
#include "runtime/movement/MovementKinematics.hpp"
#include "runtime/movement/MovementSystem.hpp"
#include "runtime/session/Session.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace iggy3d {
namespace {

constexpr float kMovementStateSpeedEpsilonMetersPerSecond = 0.001F;
constexpr float kMovementStateDistanceEpsilonMeters = 0.0001F;

}  // namespace

void clearProductMovementDebug(ProductAppWindowState& window) {
  window.gameplay.gameplayMovement.debugAvailable = false;
  window.gameplay.gameplayMovement.reasonCode = "not_requested";
  window.gameplay.gameplayMovement.blockedReason = "none";
  window.gameplay.gameplayMovement.hitSurfaceId = "none";
  window.gameplay.gameplayMovement.groundSnapApplied = false;
  window.gameplay.gameplayMovement.clamped = false;
  window.gameplay.gameplayMovement.slid = false;
  window.gameplay.gameplayMovement.collisionSweepCount = 0;
  window.gameplay.gameplayMovement.policyBand = "none";
  window.gameplay.gameplayMovement.slopeTravelDirection = "stationary";
  window.gameplay.gameplayMovement.slopeAngleDegrees = 0.0F;
  window.gameplay.gameplayMovement.speedMultiplier = 1.0F;
  window.gameplay.gameplayMovement.startX = 0.0F;
  window.gameplay.gameplayMovement.startY = 0.0F;
  window.gameplay.gameplayMovement.startZ = 0.0F;
  window.gameplay.gameplayMovement.finalX = 0.0F;
  window.gameplay.gameplayMovement.finalY = 0.0F;
  window.gameplay.gameplayMovement.finalZ = 0.0F;
  window.gameplay.gameplayMovement.horizontalDistanceMeters = 0.0F;
  window.gameplay.gameplayMovement.verticalDeltaMeters = 0.0F;
  window.gameplay.gameplayMovement.gradePercent = 0.0F;
}

float productHorizontalMovementSpeedMetersPerSecond(
    const ProductAppWindowState& window) {
  const float speedSquared =
      window.gameplay.gameplayMovement.groundVelocityX *
          window.gameplay.gameplayMovement.groundVelocityX +
      window.gameplay.gameplayMovement.groundVelocityZ *
          window.gameplay.gameplayMovement.groundVelocityZ;
  const float retainedSpeed = std::sqrt(std::max(0.0F, speedSquared));
  const float dt =
      std::max(0.0F, window.gameplay.gameplayMovement.tuning.inputStepSeconds);
  // branch-gate: BG-1161
  const float debugSpeed =
      dt > 0.0F ? window.gameplay.gameplayMovement.horizontalDistanceMeters / dt
                : 0.0F;
  return std::max(retainedSpeed, debugSpeed);
}

void updateProductMovementStateProof(ProductAppWindowState& window) {
  window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond =
      productHorizontalMovementSpeedMetersPerSecond(window);
  window.gameplay.gameplayMovement.grounded = !window.gameplay.gameplayJump.active;

  const bool blockedOrSliding =
      window.gameplay.gameplayMovement.blocked ||
      window.gameplay.gameplayMovement.clamped ||
      window.gameplay.gameplayMovement.slid ||
      (window.gameplay.gameplayMovement.blockedReason != "none" &&
       window.gameplay.gameplayMovement.blockedReason != "movement_ok");
  // branch-gate: BG-1161
  if (blockedOrSliding) {
    window.gameplay.gameplayMovement.state =
        ProductGameplayMovementState::BlockedOrSliding;
    return;
  }

  // branch-gate: BG-1153
  if (window.gameplay.gameplayJump.active) {
    // branch-gate: BG-1157
    if (window.gameplay.gameplayWallRun.active) {
      window.gameplay.gameplayMovement.state =
          ProductGameplayMovementState::WallRunning;
      return;
    }
    // branch-gate: BG-1161
    if (window.gameplay.gameplayMovement.reasonCode == "airborne_manual_move" &&
        window.gameplay.gameplayMovement.horizontalDistanceMeters >
            kMovementStateDistanceEpsilonMeters) {
      window.gameplay.gameplayMovement.state =
          ProductGameplayMovementState::AirborneControl;
      return;
    }
    // branch-gate: BG-1153
    if (window.gameplay.gameplayJump.status == "accepted") {
      window.gameplay.gameplayMovement.state = ProductGameplayMovementState::Jumping;
      return;
    }
    // branch-gate: BG-1153
    window.gameplay.gameplayMovement.state =
        window.gameplay.gameplayJump.velocityMetersPerSecond > 0.0F
            ? ProductGameplayMovementState::Rising
            : ProductGameplayMovementState::Falling;
    return;
  }

  const bool horizontalVelocityActive =
      window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond >
      kMovementStateSpeedEpsilonMetersPerSecond;
  const bool movementDebugMoved =
      window.gameplay.gameplayMovement.status == "moved" &&
      window.gameplay.gameplayMovement.horizontalDistanceMeters >
          kMovementStateDistanceEpsilonMeters;
  // branch-gate: BG-1161
  window.gameplay.gameplayMovement.state =
      horizontalVelocityActive || movementDebugMoved
          ? ProductGameplayMovementState::MovingGrounded
          : ProductGameplayMovementState::IdleGrounded;
}

void recordProductMovementProfile(ProductAppWindowState& window, bool sprinting) {
  window.gameplay.gameplayMovement.profile =
      std::string{productManualFirstPersonMovementProfile(
          window.gameplay.gameplayMovement.tuning, sprinting)};
  window.gameplay.gameplayMovement.maxSpeedMetersPerSecond =
      productManualFirstPersonMaxSpeedMetersPerSecond(
          window.gameplay.gameplayMovement.tuning, sprinting);
}

void recordProductAirborneMovementDebug(ProductAppWindowState& window,
                                        Vec3 start,
                                        Vec3 finalPosition) {
  const MovementTravelFacts facts =
      computeMovementTravelFacts(start, finalPosition);
  window.gameplay.gameplayMovement.debugAvailable = true;
  window.gameplay.gameplayMovement.reasonCode = "airborne_manual_move";
  window.gameplay.gameplayMovement.blockedReason = "movement_ok";
  window.gameplay.gameplayMovement.hitSurfaceId = "none";
  window.gameplay.gameplayMovement.groundSnapApplied = false;
  window.gameplay.gameplayMovement.clamped = false;
  window.gameplay.gameplayMovement.slid = false;
  window.gameplay.gameplayMovement.collisionSweepCount = 0;
  window.gameplay.gameplayMovement.policyBand = "airborne";
  window.gameplay.gameplayMovement.slopeTravelDirection =
      movementTravelDirectionName(facts.direction);
  window.gameplay.gameplayMovement.slopeAngleDegrees = 0.0F;
  window.gameplay.gameplayMovement.speedMultiplier = 1.0F;
  window.gameplay.gameplayMovement.startX = start.x;
  window.gameplay.gameplayMovement.startY = start.y;
  window.gameplay.gameplayMovement.startZ = start.z;
  window.gameplay.gameplayMovement.finalX = finalPosition.x;
  window.gameplay.gameplayMovement.finalY = finalPosition.y;
  window.gameplay.gameplayMovement.finalZ = finalPosition.z;
  window.gameplay.gameplayMovement.horizontalDistanceMeters =
      facts.horizontalDistanceMeters;
  window.gameplay.gameplayMovement.verticalDeltaMeters = facts.verticalDeltaMeters;
  window.gameplay.gameplayMovement.gradePercent = facts.gradePercent;
}

void recordProductLedgeFallMovementDebug(ProductAppWindowState& window,
                                         Vec3 start,
                                         Vec3 finalPosition) {
  const MovementTravelFacts facts =
      computeMovementTravelFacts(start, finalPosition);
  window.gameplay.gameplayMovement.debugAvailable = true;
  window.gameplay.gameplayMovement.reasonCode = "grounded_ledge_fall";
  window.gameplay.gameplayMovement.blockedReason = "movement_ok";
  window.gameplay.gameplayMovement.hitSurfaceId = "none";
  window.gameplay.gameplayMovement.groundSnapApplied = false;
  window.gameplay.gameplayMovement.clamped = false;
  window.gameplay.gameplayMovement.slid = false;
  window.gameplay.gameplayMovement.collisionSweepCount = 0;
  window.gameplay.gameplayMovement.policyBand = "falling";
  window.gameplay.gameplayMovement.slopeTravelDirection =
      movementTravelDirectionName(facts.direction);
  window.gameplay.gameplayMovement.slopeAngleDegrees = 0.0F;
  window.gameplay.gameplayMovement.speedMultiplier = 1.0F;
  window.gameplay.gameplayMovement.startX = start.x;
  window.gameplay.gameplayMovement.startY = start.y;
  window.gameplay.gameplayMovement.startZ = start.z;
  window.gameplay.gameplayMovement.finalX = finalPosition.x;
  window.gameplay.gameplayMovement.finalY = finalPosition.y;
  window.gameplay.gameplayMovement.finalZ = finalPosition.z;
  window.gameplay.gameplayMovement.horizontalDistanceMeters =
      facts.horizontalDistanceMeters;
  window.gameplay.gameplayMovement.verticalDeltaMeters = facts.verticalDeltaMeters;
  window.gameplay.gameplayMovement.gradePercent = facts.gradePercent;
}

bool productMovementDebugChangedPosition(const ProductAppWindowState& window) {
  if (!window.gameplay.gameplayMovement.debugAvailable) {
    return false;
  }
  const Vec3 start{window.gameplay.gameplayMovement.startX,
                   window.gameplay.gameplayMovement.startY,
                   window.gameplay.gameplayMovement.startZ};
  const Vec3 final{window.gameplay.gameplayMovement.finalX,
                   window.gameplay.gameplayMovement.finalY,
                   window.gameplay.gameplayMovement.finalZ};
  return !nearlyEqual(start, final);
}

void recordProductMovementDebug(const Session& session,
                                ProductAppWindowState& window) {
  const SessionTransientState& transient = session.state().transient;
  if (!transient.lastMovementResultAvailable) {
    clearProductMovementDebug(window);
    return;
  }

  const MovementResult& movement = transient.lastMovementResult;
  window.gameplay.gameplayMovement.debugAvailable = true;
  window.gameplay.gameplayMovement.reasonCode =
      movement.reasonCode.empty() ? movementBlockedReasonName(movement.blocked)
                                  : movement.reasonCode;
  window.gameplay.gameplayMovement.blockedReason =
      movementBlockedReasonName(movement.blocked);
  window.gameplay.gameplayMovement.hitSurfaceId =
      movement.hitSurfaceId.empty() ? "none" : movement.hitSurfaceId;
  window.gameplay.gameplayMovement.groundSnapApplied = movement.groundSnapApplied;
  window.gameplay.gameplayMovement.clamped = movement.movementClamped;
  window.gameplay.gameplayMovement.slid = movement.movementSlid;
  window.gameplay.gameplayMovement.collisionSweepCount = movement.collisionSweepCount;
  window.gameplay.gameplayMovement.policyBand =
      movement.movementPolicyBand.empty() ? "none" : movement.movementPolicyBand;
  window.gameplay.gameplayMovement.slopeTravelDirection =
      movement.slopeTravelDirection;
  window.gameplay.gameplayMovement.slopeAngleDegrees = movement.slopeAngleDegrees;
  window.gameplay.gameplayMovement.speedMultiplier = movement.speedMultiplier;
  window.gameplay.gameplayMovement.startX = movement.start.x;
  window.gameplay.gameplayMovement.startY = movement.start.y;
  window.gameplay.gameplayMovement.startZ = movement.start.z;
  window.gameplay.gameplayMovement.finalX = movement.finalPosition.x;
  window.gameplay.gameplayMovement.finalY = movement.finalPosition.y;
  window.gameplay.gameplayMovement.finalZ = movement.finalPosition.z;
  window.gameplay.gameplayMovement.horizontalDistanceMeters =
      movement.horizontalDistanceMeters;
  window.gameplay.gameplayMovement.verticalDeltaMeters = movement.verticalDeltaMeters;
  window.gameplay.gameplayMovement.gradePercent = movement.gradePercent;
}

}  // namespace iggy3d
