#include "app/iggy3d/debug/MovementDebugHud.hpp"

#include <charconv>
#include <string_view>
#include <utility>

#include "app/iggy3d/gameplay/MovementProof.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"

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

std::string finalPositionDisplay(const ProductMovementProofPacket& proof) {
  return "x" + fixed3(proof.finalX) +
         " y" + fixed3(proof.finalY) +
         " z" + fixed3(proof.finalZ);
}

std::string movementStateDisplay(const ProductMovementProofPacket& proof) {
  return proof.stateHudLabel +
         " h" + fixed3(proof.horizontalSpeedMetersPerSecond) +
         " v" + fixed3(proof.verticalVelocityMetersPerSecond);
}

std::string wallRunDisplay(const ProductMovementProofPacket& proof) {
  return (proof.wallRunActive ? std::string{"active "}
                              : std::string{"cand "}) +  // branch-gate: BG-1157
         codeDisplay(proof.wallRunSide) +
         " " + codeDisplay(proof.wallRunSurfaceId) +
         " " + proof.wallRunReasonHudLabel +
         " t" + fixed3(proof.wallRunRemainingSeconds) +
         " h" + fixed3(proof.wallRunApproachSpeedMetersPerSecond);
}

FeedbackTone statusTone(const ProductMovementProofPacket& proof) {
  if (proof.blocked) {
    return FeedbackTone::Warn;
  }
  if (proof.status == "moved") {
    return FeedbackTone::Pass;
  }
  if (proof.status == "tick_failed") {
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
  const ProductMovementProofPacket proof =
      buildProductMovementProofPacket(window);
  return buildMovementDebugHud(proof,
                               window.gameplay.gameplayActive,
                               developerToolsEnabled,
                               debugOverlayEnabled);
}

MovementDebugHud buildMovementDebugHud(
    const ProductMovementProofPacket& proof,
    bool gameplayActive,
    bool developerToolsEnabled,
    bool debugOverlayEnabled) {
  MovementDebugHud hud;
  hud.developerToolsEnabled = developerToolsEnabled;
  hud.debugOverlayEnabled = debugOverlayEnabled;
  hud.debugAvailable = proof.debugAvailable;
  hud.blocked = proof.blocked;
  hud.movementState = proof.stateName;
  hud.blockedReason = proof.blockedReason;
  hud.hitSurfaceId = proof.hitSurfaceId;
  hud.policyBand = proof.policyBand;
  hud.slopeTravelDirection = proof.slopeTravelDirection;
  hud.groundSnapApplied = proof.groundSnapApplied;
  hud.movementClamped = proof.movementClamped;
  hud.collisionSweepCount = proof.collisionSweepCount;
  hud.speedMultiplier = proof.speedMultiplier;
  hud.horizontalSpeedMetersPerSecond = proof.horizontalSpeedMetersPerSecond;
  hud.verticalVelocityMetersPerSecond = proof.verticalVelocityMetersPerSecond;
  hud.wallRunCandidateAvailable = proof.wallRunCandidateAvailable;
  hud.wallRunCandidateStatus = proof.wallRunCandidateStatus;
  hud.wallRunSide = proof.wallRunSide;
  hud.wallRunSurfaceId = proof.wallRunSurfaceId;
  hud.wallRunApproachSpeedMetersPerSecond =
      proof.wallRunApproachSpeedMetersPerSecond;
  hud.wallRunActive = proof.wallRunActive;
  hud.wallRunStatus = proof.wallRunStatus;
  hud.wallRunRemainingSeconds = proof.wallRunRemainingSeconds;
  hud.finalX = proof.finalX;
  hud.finalY = proof.finalY;
  hud.finalZ = proof.finalZ;
  hud.visible = gameplayActive && developerToolsEnabled && debugOverlayEnabled;

  if (!hud.visible) {
    return hud;
  }

  hud.status = proof.status;
  hud.reasonCode = proof.reasonCode;

  // branch-gate: BG-1161
  const FeedbackTone wallRunTone =
      proof.wallRunActive || proof.wallRunCandidateAvailable
          ? FeedbackTone::Pass
          : FeedbackTone::Neutral;
  addLine(hud, "STATUS", codeDisplay(hud.status), statusTone(proof));
  addLine(hud, "STATE", movementStateDisplay(proof), FeedbackTone::Neutral);
  addLine(hud, "WALLRUN", wallRunDisplay(proof), wallRunTone);
  addLine(hud, "REASON", codeDisplay(hud.reasonCode),
          proof.blocked ? FeedbackTone::Warn : FeedbackTone::Neutral);
  addLine(hud, "HIT", codeDisplay(hud.hitSurfaceId),
          proof.blocked ? FeedbackTone::Warn : FeedbackTone::Neutral);
  addLine(hud, "POS", finalPositionDisplay(proof), FeedbackTone::Neutral);
  addLine(hud, "SLOPE", codeDisplay(hud.policyBand), FeedbackTone::Neutral);
  addLine(hud, "SPEED", fixed3(hud.speedMultiplier), FeedbackTone::Neutral);
  addLine(hud, "SNAP", boolDisplay(hud.groundSnapApplied), FeedbackTone::Neutral);
  return hud;
}

}  // namespace iggy3d
