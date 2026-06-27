#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "app/iggy3d/gameplay/ProductGameplayFeedback.hpp"

namespace iggy3d {

struct ProductAppWindowState;

struct ProductMovementDebugHudLine {
  std::string label;
  std::string value;
  ProductFeedbackTone tone = ProductFeedbackTone::Neutral;
  bool visible = false;
};

struct ProductMovementDebugHud {
  bool visible = false;
  bool developerToolsEnabled = false;
  bool debugOverlayEnabled = false;
  bool debugAvailable = false;
  bool blocked = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string blockedReason = "none";
  std::string hitSurfaceId = "none";
  std::string policyBand = "none";
  std::string slopeTravelDirection = "stationary";
  bool groundSnapApplied = false;
  bool movementClamped = false;
  std::uint64_t collisionSweepCount = 0;
  float speedMultiplier = 1.0F;
  float finalX = 0.0F;
  float finalY = 0.0F;
  float finalZ = 0.0F;
  std::vector<ProductMovementDebugHudLine> lines;
};

ProductMovementDebugHud buildProductMovementDebugHud(
    const ProductAppWindowState& window,
    bool developerToolsEnabled,
    bool debugOverlayEnabled);

}  // namespace iggy3d
