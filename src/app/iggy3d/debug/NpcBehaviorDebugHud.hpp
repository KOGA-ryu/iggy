#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "app/iggy3d/gameplay/ProductGameplayFeedback.hpp"

namespace iggy3d {

struct DebugProjectionResult;

struct NpcBehaviorDebugHudLine {
  std::string text;
  ProductFeedbackTone tone = ProductFeedbackTone::Neutral;
  bool visible = false;
};

struct NpcBehaviorDebugHud {
  bool visible = false;
  bool developerToolsEnabled = false;
  bool debugOverlayEnabled = false;
  bool debugAvailable = false;
  std::size_t lineCount = 0;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::vector<NpcBehaviorDebugHudLine> lines;
};

NpcBehaviorDebugHud buildNpcBehaviorDebugHud(
    const DebugProjectionResult* debug,
    bool gameplayActive,
    bool developerToolsEnabled,
    bool debugOverlayEnabled);

}  // namespace iggy3d
