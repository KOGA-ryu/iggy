#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "app/iggy3d/gameplay/GameplayFeedback.hpp"

namespace iggy3d {

struct ProductAppWindowState;
struct ProductMovementProofPacket;

struct MovementDebugHudLine {
  std::string label;
  std::string value;
  FeedbackTone tone = FeedbackTone::Neutral;
  bool visible = false;
};

struct MovementDebugHud {
  bool visible = false;
  bool developerToolsEnabled = false;
  bool debugOverlayEnabled = false;
  bool debugAvailable = false;
  bool blocked = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string movementState = "idle_grounded";
  std::string blockedReason = "none";
  std::string hitSurfaceId = "none";
  std::string policyBand = "none";
  std::string slopeTravelDirection = "stationary";
  bool groundSnapApplied = false;
  bool movementClamped = false;
  std::uint64_t collisionSweepCount = 0;
  float speedMultiplier = 1.0F;
  float horizontalSpeedMetersPerSecond = 0.0F;
  float verticalVelocityMetersPerSecond = 0.0F;
  bool wallRunCandidateAvailable = false;
  std::string wallRunCandidateStatus = "wall_run_not_checked";
  std::string wallRunSide = "none";
  std::string wallRunSurfaceId = "none";
  float wallRunApproachSpeedMetersPerSecond = 0.0F;
  bool wallRunActive = false;
  std::string wallRunStatus = "wall_run_inactive";
  float wallRunRemainingSeconds = 0.0F;
  float finalX = 0.0F;
  float finalY = 0.0F;
  float finalZ = 0.0F;
  std::vector<MovementDebugHudLine> lines;
};

MovementDebugHud buildMovementDebugHud(
    const ProductAppWindowState& window,
    bool developerToolsEnabled,
    bool debugOverlayEnabled);

MovementDebugHud buildMovementDebugHud(
    const ProductMovementProofPacket& proof,
    bool gameplayActive,
    bool developerToolsEnabled,
    bool debugOverlayEnabled);

}  // namespace iggy3d
