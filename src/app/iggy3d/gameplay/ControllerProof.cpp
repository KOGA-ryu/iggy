#include "app/iggy3d/gameplay/ControllerProof.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ControllerKinematics.hpp"
#include "runtime/collision/CollisionTypes.hpp"
#include "runtime/movement/MovementKinematics.hpp"
#include "runtime/movement/MovementSystem.hpp"
#include "runtime/movement/MovementTraversal.hpp"
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

namespace iggy3d {

void clearProductTraversalProof(ProductAppWindowState& window) {
  window.gameplay.gameplayTraversal.requested = false;
  window.gameplay.gameplayTraversal.consumed = false;
  window.gameplay.gameplayTraversal.accepted = false;
  window.gameplay.gameplayTraversal.fallbackJumpAllowed = false;
  window.gameplay.gameplayTraversal.status = "not_requested";
  window.gameplay.gameplayTraversal.reasonCode = "not_requested";
  window.gameplay.gameplayTraversal.mechanic = "none";
  window.gameplay.gameplayTraversal.slotId = "none";
  window.gameplay.gameplayTraversal.targetId = "none";
  window.gameplay.gameplayTraversal.landingSurfaceId = "none";
  window.gameplay.gameplayTraversal.startX = 0.0F;
  window.gameplay.gameplayTraversal.startY = 0.0F;
  window.gameplay.gameplayTraversal.startZ = 0.0F;
  window.gameplay.gameplayTraversal.finalX = 0.0F;
  window.gameplay.gameplayTraversal.finalY = 0.0F;
  window.gameplay.gameplayTraversal.finalZ = 0.0F;
}

void recordProductTraversalProof(ProductAppWindowState& window,
                                 const TraversalIntentResult& result) {
  window.gameplay.gameplayTraversal.requested = result.requested;
  window.gameplay.gameplayTraversal.consumed = result.consumedInput;
  window.gameplay.gameplayTraversal.accepted = result.accepted;
  window.gameplay.gameplayTraversal.fallbackJumpAllowed = result.fallbackJumpAllowed;
  window.gameplay.gameplayTraversal.status = traversalIntentStatusName(result.status);
  // branch-gate: BG-1156
  window.gameplay.gameplayTraversal.reasonCode =
      result.reasonCode == nullptr ? "unknown" : result.reasonCode;
  // branch-gate: BG-1156
  window.gameplay.gameplayTraversal.mechanic =
      result.traversalAttempted ? traversalMechanicName(result.selectedMechanic)
                                : "none";
  // branch-gate: BG-1156
  window.gameplay.gameplayTraversal.slotId =
      result.traversal.slotId.empty() ? "none" : result.traversal.slotId;
  // branch-gate: BG-1156
  window.gameplay.gameplayTraversal.targetId =
      result.traversal.targetId.empty() ? "none" : result.traversal.targetId;
  // branch-gate: BG-1156
  window.gameplay.gameplayTraversal.landingSurfaceId =
      result.traversal.landingSurfaceId.empty() ? "none"
                                                : result.traversal.landingSurfaceId;
  window.gameplay.gameplayTraversal.startX = result.traversal.start.x;
  window.gameplay.gameplayTraversal.startY = result.traversal.start.y;
  window.gameplay.gameplayTraversal.startZ = result.traversal.start.z;
  window.gameplay.gameplayTraversal.finalX = result.traversal.finalPosition.x;
  window.gameplay.gameplayTraversal.finalY = result.traversal.finalPosition.y;
  window.gameplay.gameplayTraversal.finalZ = result.traversal.finalPosition.z;
}

void recordProductWallJumpTraversalProof(ProductAppWindowState& window,
                                         const CollisionSurfaceView& surface,
                                         Vec3 start,
                                         Vec3 finalPosition) {
  window.gameplay.gameplayTraversal.requested = true;
  window.gameplay.gameplayTraversal.consumed = true;
  window.gameplay.gameplayTraversal.accepted = true;
  window.gameplay.gameplayTraversal.fallbackJumpAllowed = false;
  window.gameplay.gameplayTraversal.status = "traversal_intent_applied";
  window.gameplay.gameplayTraversal.reasonCode = "traversal_intent_applied";
  window.gameplay.gameplayTraversal.mechanic = "wall_jump";
  window.gameplay.gameplayTraversal.slotId = "wall_jump";
  window.gameplay.gameplayTraversal.landingSurfaceId = "wall_jump_surface";
  // branch-gate: BG-1157
  if (!surface.id.empty()) {
    window.gameplay.gameplayTraversal.slotId = surface.id;
    window.gameplay.gameplayTraversal.landingSurfaceId = surface.id;
  }
  window.gameplay.gameplayTraversal.targetId = window.gameplay.gameplayTraversal.slotId;
  // branch-gate: BG-1157
  if (!surface.runtimeOwnerStableName.empty()) {
    window.gameplay.gameplayTraversal.targetId = surface.runtimeOwnerStableName;
  }
  window.gameplay.gameplayTraversal.startX = start.x;
  window.gameplay.gameplayTraversal.startY = start.y;
  window.gameplay.gameplayTraversal.startZ = start.z;
  window.gameplay.gameplayTraversal.finalX = finalPosition.x;
  window.gameplay.gameplayTraversal.finalY = finalPosition.y;
  window.gameplay.gameplayTraversal.finalZ = finalPosition.z;
}

}  // namespace iggy3d
