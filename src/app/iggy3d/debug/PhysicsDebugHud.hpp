#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "app/iggy3d/gameplay/ProductGameplayFeedback.hpp"

namespace iggy3d {

struct DebugProjectionResult;

struct PhysicsDebugHudLine {
  std::string text;
  ProductFeedbackTone tone = ProductFeedbackTone::Neutral;
  bool visible = false;
};

struct PhysicsDebugHud {
  bool visible = false;
  bool developerToolsEnabled = false;
  bool debugOverlayEnabled = false;
  bool debugAvailable = false;
  std::size_t lineCount = 0U;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  bool hasWarnings = false;
  std::vector<PhysicsDebugHudLine> lines;
};

PhysicsDebugHud buildPhysicsDebugHud(
    const DebugProjectionResult* debug,
    bool gameplayActive,
    bool developerToolsEnabled,
    bool debugOverlayEnabled);

}  // namespace iggy3d
