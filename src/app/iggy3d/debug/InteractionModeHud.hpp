#pragma once

#include <string>
#include <string_view>

#include "app/iggy3d/gameplay/GameplayFeedback.hpp"
#include "app/iggy3d/input/InteractionMode.hpp"

namespace iggy3d {

struct InteractionModeHud {
  bool visible = false;
  std::string status = "interaction_mode_hud_hidden";
  std::string reasonCode = "interaction_mode_hud_hidden";
  std::string mode = "player";
  std::string label = "player";
  FeedbackTone tone = FeedbackTone::Neutral;
  bool roomEditingReady = false;
};

struct InteractionModeHudRequest {
  ProductInteractionMode mode = ProductInteractionMode::Player;
  bool gameplayActive = false;
  bool roomEditingReady = false;
};

InteractionModeHud buildInteractionModeHud(
    InteractionModeHudRequest request);

}  // namespace iggy3d
