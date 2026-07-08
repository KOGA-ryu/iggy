#include "app/iggy3d/gameplay/ControllerJumpDashState.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"

#include <algorithm>
#include <string>

namespace iggy3d {

void recordProductJumpPosition(ProductAppWindowState& window,
                               float groundY,
                               float startY,
                               float finalY) {
  window.gameplay.gameplayJump.groundY = groundY;
  window.gameplay.gameplayJump.startY = startY;
  window.gameplay.gameplayJump.finalY = finalY;
  window.gameplay.gameplayJump.heightMeters = std::max(0.0F, finalY - groundY);
}

void clearProductJumpTiming(ProductAppWindowState& window) {
  window.gameplay.gameplayJump.coyoteSecondsRemaining = 0.0F;
  window.gameplay.gameplayJump.bufferSecondsRemaining = 0.0F;
  window.gameplay.gameplayJump.held = false;
  window.gameplay.gameplayJump.cutApplied = false;
}

void rejectProductJump(ProductAppWindowState& window,
                       std::string_view status,
                       std::string_view reason) {
  window.gameplay.gameplayJump.requested = true;
  window.gameplay.gameplayJump.accepted = false;
  window.gameplay.gameplayJump.status = std::string{status};
  window.gameplay.gameplayJump.reasonCode = std::string{reason};
}

bool productJumpBufferLive(const ProductAppWindowState& window) {
  return window.gameplay.gameplayJump.bufferSecondsRemaining > 0.0F;
}

void bufferProductJump(ProductAppWindowState& window) {
  window.gameplay.gameplayJump.bufferSecondsRemaining =
      window.gameplay.gameplayMovement.tuning.jumpBufferSeconds;
}

void applyProductJumpReleaseCut(ProductAppWindowState& window) {
  auto& tuning = window.gameplay.gameplayMovement.tuning;
  window.gameplay.gameplayJump.held = false;
  // branch-gate: BG-1153
  if (!window.gameplay.gameplayJump.active || window.gameplay.gameplayJump.cutApplied ||
      window.gameplay.gameplayJump.velocityMetersPerSecond <= 0.0F) {
    return;
  }
  const float multiplier = std::clamp(tuning.jumpCutMultiplier, 0.1F, 1.0F);
  window.gameplay.gameplayJump.velocityMetersPerSecond *= multiplier;
  window.gameplay.gameplayJump.cutApplied = true;
}

void advanceProductDashCooldown(ProductAppWindowState& window) {
  const auto& tuning = window.gameplay.gameplayMovement.tuning;
  // branch-gate: BG-1155
  if (window.gameplay.gameplayDash.cooldownRemainingSeconds <= 0.0F) {
    window.gameplay.gameplayDash.cooldownRemainingSeconds = 0.0F;
    return;
  }
  window.gameplay.gameplayDash.cooldownRemainingSeconds =
      std::max(0.0F,
               window.gameplay.gameplayDash.cooldownRemainingSeconds -
                   tuning.inputStepSeconds);
}

void rejectProductDash(ProductAppWindowState& window,
                       std::string_view status,
                       std::string_view reason) {
  window.gameplay.gameplayDash.requested = true;
  window.gameplay.gameplayDash.accepted = false;
  window.gameplay.gameplayDash.status = std::string{status};
  window.gameplay.gameplayDash.reasonCode = std::string{reason};
}

}  // namespace iggy3d
