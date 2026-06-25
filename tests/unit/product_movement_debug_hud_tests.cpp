#include "app/iggy3d/ProductMovementDebugHud.hpp"

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

bool hasLine(const iggy3d::ProductMovementDebugHud& hud,
             const char* label,
             const char* value) {
  for (const iggy3d::ProductMovementDebugHudLine& line : hud.lines) {
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
  const iggy3d::ProductMovementDebugHud inactiveHud =
      iggy3d::buildProductMovementDebugHud(inactive, true, true);
  ok &= expect(!inactiveHud.visible, "inactive gameplay hides movement hud");
  ok &= expect(inactiveHud.lines.empty(), "inactive hud has no visible lines");

  const iggy3d::ProductMovementDebugHud devDisabled =
      iggy3d::buildProductMovementDebugHud(movedWindow(), false, true);
  ok &= expect(!devDisabled.visible, "developer tools disabled hides hud");
  ok &= expect(devDisabled.debugAvailable, "disabled hud still reports debug availability");

  const iggy3d::ProductMovementDebugHud overlayDisabled =
      iggy3d::buildProductMovementDebugHud(movedWindow(), true, false);
  ok &= expect(!overlayDisabled.visible, "debug overlay disabled hides hud");

  const iggy3d::ProductMovementDebugHud moved =
      iggy3d::buildProductMovementDebugHud(movedWindow(), true, true);
  ok &= expect(moved.visible, "moved hud visible");
  ok &= expect(moved.status == "moved", "moved status");
  ok &= expect(moved.reasonCode == "movement_ok", "moved reason");
  ok &= expect(!moved.blocked, "moved not blocked");
  ok &= expect(moved.lines.size() == 7U, "moved line count");
  ok &= expect(hasLine(moved, "STATUS", "moved"), "moved status line");
  ok &= expect(hasLine(moved, "REASON", "movement ok"), "moved reason display");
  ok &= expect(hasLine(moved, "HIT", "none"), "moved hit display");
  ok &= expect(hasLine(moved, "SPEED", "1.000"), "moved speed display");
  ok &= expect(hasLine(moved, "SNAP", "true"), "moved snap display");

  iggy3d::ProductAppWindowState blocked = movedWindow();
  blocked.gameplayMovementBlocked = true;
  blocked.gameplayMovementStatus = "blocked";
  blocked.gameplayMovementReasonCode = "blocked_by_collision";
  blocked.gameplayMovementBlockedReason = "blocked_by_collision";
  blocked.gameplayMovementHitSurfaceId = "wall_r0_c1_actor_blocker";
  blocked.gameplayMovementGroundSnapApplied = false;
  blocked.gameplayMovementClamped = true;
  blocked.gameplayMovementPolicyBand = "none";
  const iggy3d::ProductMovementDebugHud blockedHud =
      iggy3d::buildProductMovementDebugHud(blocked, true, true);
  ok &= expect(blockedHud.visible, "blocked hud visible");
  ok &= expect(blockedHud.blocked, "blocked hud blocked");
  ok &= expect(blockedHud.hitSurfaceId == "wall_r0_c1_actor_blocker", "blocked hit id");
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
