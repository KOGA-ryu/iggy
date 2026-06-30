#include "app/iggy3d/debug/MovementDebugHud.hpp"

#include <iostream>

#include "app/iggy3d/ReceiptBuilder.hpp"

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    return false;
  }
  return true;
}

bool hasLine(const iggy3d::MovementDebugHud& hud,
             const char* label,
             const char* value) {
  for (const iggy3d::MovementDebugHudLine& line : hud.lines) {
    if (line.visible && line.label == label && line.value == value) {
      return true;
    }
  }
  return false;
}

iggy3d::ProductAppWindowState movedWindow() {
  iggy3d::ProductAppWindowState window;
  window.gameplayActive = true;
  window.gameplayMovementAttempted = true;
  window.gameplayMovementStatus = "moved";
  window.gameplayMovementDebugAvailable = true;
  window.gameplayMovementReasonCode = "movement_ok";
  window.gameplayMovementBlockedReason = "movement_ok";
  window.gameplayMovementHitSurfaceId = "none";
  window.gameplayMovementGroundSnapApplied = true;
  window.gameplayMovementPolicyBand = "flat";
  window.gameplayMovementSpeedMultiplier = 1.0F;
  window.gameplayMovementState =
      iggy3d::ProductGameplayMovementState::MovingGrounded;
  window.gameplayMovementGrounded = true;
  window.gameplayMovementHorizontalSpeedMetersPerSecond = 3.3F;
  window.gameplayJumpVelocityMetersPerSecond = 0.0F;
  window.gameplayWallRunCandidateStatus = "wall_run_grounded";
  window.gameplayWallRunCandidateReasonCode = "wall_run_grounded";
  window.gameplayWallRunApproachSpeedMetersPerSecond = 3.3F;
  window.gameplayMovementFinalX = -1.0F;
  window.gameplayMovementFinalY = 0.0F;
  window.gameplayMovementFinalZ = -1.0F;
  return window;
}

}  // namespace

int main() {
  bool ok = true;

  iggy3d::ProductAppWindowState inactive = movedWindow();
  inactive.gameplayActive = false;
  const iggy3d::MovementDebugHud inactiveHud =
      iggy3d::buildMovementDebugHud(inactive, true, true);
  ok &= expect(!inactiveHud.visible, "inactive gameplay hides movement hud");
  ok &= expect(inactiveHud.status == "not_requested", "inactive hud status");
  ok &= expect(inactiveHud.reasonCode == "not_requested", "inactive hud reason");
  ok &= expect(inactiveHud.lines.empty(), "inactive hud has no visible lines");

  const iggy3d::MovementDebugHud devDisabled =
      iggy3d::buildMovementDebugHud(movedWindow(), false, true);
  ok &= expect(!devDisabled.visible, "developer tools disabled hides hud");
  ok &= expect(devDisabled.debugAvailable, "disabled hud still reports debug availability");
  ok &= expect(devDisabled.status == "not_requested", "developer tools disabled status");
  ok &= expect(devDisabled.reasonCode == "not_requested", "developer tools disabled reason");

  const iggy3d::MovementDebugHud overlayDisabled =
      iggy3d::buildMovementDebugHud(movedWindow(), true, false);
  ok &= expect(!overlayDisabled.visible, "debug overlay disabled hides hud");
  ok &= expect(overlayDisabled.debugAvailable, "overlay disabled still reports debug availability");
  ok &= expect(overlayDisabled.status == "not_requested", "overlay disabled status");
  ok &= expect(overlayDisabled.reasonCode == "not_requested", "overlay disabled reason");

  const iggy3d::MovementDebugHud moved =
      iggy3d::buildMovementDebugHud(movedWindow(), true, true);
  ok &= expect(moved.visible, "moved hud visible");
  ok &= expect(moved.status == "moved", "moved status");
  ok &= expect(moved.reasonCode == "movement_ok", "moved reason");
  ok &= expect(!moved.blocked, "moved not blocked");
  ok &= expect(moved.movementState == "moving_grounded",
               "moved movement state");
  ok &= expect(!moved.wallRunCandidateAvailable,
               "moved wall-run candidate unavailable");
  ok &= expect(moved.lines.size() == 9U, "moved line count");
  ok &= expect(hasLine(moved, "STATUS", "moved"), "moved status line");
  ok &= expect(hasLine(moved, "STATE", "moving grounded h3.300 v0.000"),
               "moved state display");
  ok &= expect(hasLine(moved,
                       "WALLRUN",
                       "none none wall run grounded h3.300"),
               "moved wall-run display");
  ok &= expect(hasLine(moved, "REASON", "movement ok"), "moved reason display");
  ok &= expect(hasLine(moved, "HIT", "none"), "moved hit display");
  ok &= expect(hasLine(moved, "SPEED", "1.000"), "moved speed display");
  ok &= expect(hasLine(moved, "SNAP", "true"), "moved snap display");

  iggy3d::ProductAppWindowState blocked = movedWindow();
  blocked.gameplayMovementBlocked = true;
  blocked.gameplayMovementState =
      iggy3d::ProductGameplayMovementState::BlockedOrSliding;
  blocked.gameplayMovementStatus = "blocked";
  blocked.gameplayMovementReasonCode = "blocked_by_collision";
  blocked.gameplayMovementBlockedReason = "blocked_by_collision";
  blocked.gameplayMovementHitSurfaceId = "wall_r0_c1_actor_blocker";
  blocked.gameplayMovementGroundSnapApplied = false;
  blocked.gameplayMovementClamped = true;
  blocked.gameplayMovementPolicyBand = "none";
  blocked.gameplayWallRunCandidateAvailable = true;
  blocked.gameplayWallRunCandidateStatus = "wall_run_candidate";
  blocked.gameplayWallRunCandidateReasonCode = "wall_run_candidate";
  blocked.gameplayWallRunSide = "left";
  blocked.gameplayWallRunSurfaceId = "wall_r0_c1_actor_blocker";
  const iggy3d::MovementDebugHud blockedHud =
      iggy3d::buildMovementDebugHud(blocked, true, true);
  ok &= expect(blockedHud.visible, "blocked hud visible");
  ok &= expect(blockedHud.blocked, "blocked hud blocked");
  ok &= expect(blockedHud.movementState == "blocked_or_sliding",
               "blocked hud state");
  ok &= expect(blockedHud.wallRunCandidateAvailable,
               "blocked hud wall-run candidate");
  ok &= expect(blockedHud.hitSurfaceId == "wall_r0_c1_actor_blocker", "blocked hit id");
  ok &= expect(hasLine(blockedHud, "STATE", "blocked or sliding h3.300 v0.000"),
               "blocked state display");
  ok &= expect(hasLine(blockedHud,
                       "WALLRUN",
                       "left wall r0 c1 actor blocker wall run candidate h3.300"),
               "blocked wall-run display");
  ok &= expect(hasLine(blockedHud, "STATUS", "blocked"), "blocked status line");
  ok &= expect(hasLine(blockedHud, "REASON", "blocked by collision"),
               "blocked reason display");
  ok &= expect(hasLine(blockedHud, "HIT", "wall r0 c1 actor blocker"),
               "blocked hit display");

  if (!ok) {
    return 1;
  }
  std::cout << "product_movement_debug_hud_tests=pass\n";
  return 0;
}
