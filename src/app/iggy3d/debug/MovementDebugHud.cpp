#include "app/iggy3d/debug/MovementDebugHud.hpp"

#include <charconv>
#include <string_view>
#include <utility>

#include "app/iggy3d/ReceiptBuilder.hpp"

namespace iggy3d {
namespace {

std::string fixed3(float value) {
  char buffer[32]{};
  const auto [ptr, error] =
      std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::fixed, 3);
  if (error != std::errc{}) {
    return "unavailable";
  }
  return std::string(buffer, static_cast<std::size_t>(ptr - buffer));
}

std::string boolDisplay(bool value) {
  return value ? "true" : "false";
}

std::string codeDisplay(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for (const char c : value) {
    out.push_back(c == '_' ? ' ' : c);
  }
  return out;
}

std::string finalPositionDisplay(const ProductAppWindowState& window) {
  return "x" + fixed3(window.gameplayMovementFinalX) +
         " y" + fixed3(window.gameplayMovementFinalY) +
         " z" + fixed3(window.gameplayMovementFinalZ);
}

ProductFeedbackTone statusTone(const ProductAppWindowState& window) {
  if (window.gameplayMovementBlocked) {
    return ProductFeedbackTone::Warn;
  }
  if (window.gameplayMovementStatus == "moved") {
    return ProductFeedbackTone::Pass;
  }
  if (window.gameplayMovementStatus == "tick_failed") {
    return ProductFeedbackTone::Fail;
  }
  return ProductFeedbackTone::Neutral;
}

void addLine(MovementDebugHud& hud,
             std::string label,
             std::string value,
             ProductFeedbackTone tone) {
  hud.lines.push_back(
      MovementDebugHudLine{std::move(label), std::move(value), tone, hud.visible});
}

}  // namespace

MovementDebugHud buildMovementDebugHud(
    const ProductAppWindowState& window,
    bool developerToolsEnabled,
    bool debugOverlayEnabled) {
  MovementDebugHud hud;
  hud.developerToolsEnabled = developerToolsEnabled;
  hud.debugOverlayEnabled = debugOverlayEnabled;
  hud.debugAvailable = window.gameplayMovementDebugAvailable;
  hud.blocked = window.gameplayMovementBlocked;
  hud.blockedReason = window.gameplayMovementBlockedReason;
  hud.hitSurfaceId = window.gameplayMovementHitSurfaceId;
  hud.policyBand = window.gameplayMovementPolicyBand;
  hud.slopeTravelDirection = window.gameplayMovementSlopeTravelDirection;
  hud.groundSnapApplied = window.gameplayMovementGroundSnapApplied;
  hud.movementClamped = window.gameplayMovementClamped;
  hud.collisionSweepCount = window.gameplayMovementCollisionSweepCount;
  hud.speedMultiplier = window.gameplayMovementSpeedMultiplier;
  hud.finalX = window.gameplayMovementFinalX;
  hud.finalY = window.gameplayMovementFinalY;
  hud.finalZ = window.gameplayMovementFinalZ;
  hud.visible = window.gameplayActive && developerToolsEnabled && debugOverlayEnabled;

  if (!hud.visible) {
    return hud;
  }

  hud.status = window.gameplayMovementStatus;
  hud.reasonCode = window.gameplayMovementReasonCode;

  addLine(hud, "STATUS", codeDisplay(hud.status), statusTone(window));
  addLine(hud, "REASON", codeDisplay(hud.reasonCode),
          window.gameplayMovementBlocked ? ProductFeedbackTone::Warn
                                         : ProductFeedbackTone::Neutral);
  addLine(hud, "HIT", codeDisplay(hud.hitSurfaceId),
          window.gameplayMovementBlocked ? ProductFeedbackTone::Warn
                                         : ProductFeedbackTone::Neutral);
  addLine(hud, "POS", finalPositionDisplay(window), ProductFeedbackTone::Neutral);
  addLine(hud, "SLOPE", codeDisplay(hud.policyBand), ProductFeedbackTone::Neutral);
  addLine(hud, "SPEED", fixed3(hud.speedMultiplier), ProductFeedbackTone::Neutral);
  addLine(hud, "SNAP", boolDisplay(hud.groundSnapApplied), ProductFeedbackTone::Neutral);
  return hud;
}

}  // namespace iggy3d
