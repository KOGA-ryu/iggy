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

std::string movementStateDisplay(const ProductAppWindowState& window) {
  return codeDisplay(productGameplayMovementStateName(window.gameplayMovementState)) +
         " h" + fixed3(window.gameplayMovementHorizontalSpeedMetersPerSecond) +
         " v" + fixed3(window.gameplayJumpVelocityMetersPerSecond);
}

std::string wallRunDisplay(const ProductAppWindowState& window) {
  return codeDisplay(window.gameplayWallRunSide) +
         " " + codeDisplay(window.gameplayWallRunSurfaceId) +
         " " + codeDisplay(window.gameplayWallRunCandidateReasonCode) +
         " h" + fixed3(window.gameplayWallRunApproachSpeedMetersPerSecond);
}

FeedbackTone statusTone(const ProductAppWindowState& window) {
  if (window.gameplayMovementBlocked) {
    return FeedbackTone::Warn;
  }
  if (window.gameplayMovementStatus == "moved") {
    return FeedbackTone::Pass;
  }
  if (window.gameplayMovementStatus == "tick_failed") {
    return FeedbackTone::Fail;
  }
  return FeedbackTone::Neutral;
}

void addLine(MovementDebugHud& hud,
             std::string label,
             std::string value,
             FeedbackTone tone) {
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
  hud.movementState =
      std::string{productGameplayMovementStateName(window.gameplayMovementState)};
  hud.blockedReason = window.gameplayMovementBlockedReason;
  hud.hitSurfaceId = window.gameplayMovementHitSurfaceId;
  hud.policyBand = window.gameplayMovementPolicyBand;
  hud.slopeTravelDirection = window.gameplayMovementSlopeTravelDirection;
  hud.groundSnapApplied = window.gameplayMovementGroundSnapApplied;
  hud.movementClamped = window.gameplayMovementClamped;
  hud.collisionSweepCount = window.gameplayMovementCollisionSweepCount;
  hud.speedMultiplier = window.gameplayMovementSpeedMultiplier;
  hud.horizontalSpeedMetersPerSecond =
      window.gameplayMovementHorizontalSpeedMetersPerSecond;
  hud.verticalVelocityMetersPerSecond = window.gameplayJumpVelocityMetersPerSecond;
  hud.wallRunCandidateAvailable = window.gameplayWallRunCandidateAvailable;
  hud.wallRunCandidateStatus = window.gameplayWallRunCandidateStatus;
  hud.wallRunSide = window.gameplayWallRunSide;
  hud.wallRunSurfaceId = window.gameplayWallRunSurfaceId;
  hud.wallRunApproachSpeedMetersPerSecond =
      window.gameplayWallRunApproachSpeedMetersPerSecond;
  hud.finalX = window.gameplayMovementFinalX;
  hud.finalY = window.gameplayMovementFinalY;
  hud.finalZ = window.gameplayMovementFinalZ;
  hud.visible = window.gameplayActive && developerToolsEnabled && debugOverlayEnabled;

  if (!hud.visible) {
    return hud;
  }

  hud.status = window.gameplayMovementStatus;
  hud.reasonCode = window.gameplayMovementReasonCode;

  // branch-gate: BG-1161
  const FeedbackTone wallRunTone =
      window.gameplayWallRunCandidateAvailable ? FeedbackTone::Pass
                                               : FeedbackTone::Neutral;
  addLine(hud, "STATUS", codeDisplay(hud.status), statusTone(window));
  addLine(hud, "STATE", movementStateDisplay(window), FeedbackTone::Neutral);
  addLine(hud, "WALLRUN", wallRunDisplay(window), wallRunTone);
  addLine(hud, "REASON", codeDisplay(hud.reasonCode),
          window.gameplayMovementBlocked ? FeedbackTone::Warn
                                         : FeedbackTone::Neutral);
  addLine(hud, "HIT", codeDisplay(hud.hitSurfaceId),
          window.gameplayMovementBlocked ? FeedbackTone::Warn
                                         : FeedbackTone::Neutral);
  addLine(hud, "POS", finalPositionDisplay(window), FeedbackTone::Neutral);
  addLine(hud, "SLOPE", codeDisplay(hud.policyBand), FeedbackTone::Neutral);
  addLine(hud, "SPEED", fixed3(hud.speedMultiplier), FeedbackTone::Neutral);
  addLine(hud, "SNAP", boolDisplay(hud.groundSnapApplied), FeedbackTone::Neutral);
  return hud;
}

}  // namespace iggy3d
