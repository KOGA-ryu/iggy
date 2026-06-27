#pragma once

#include <string>
#include <string_view>

#include "app/iggy3d/ProductGameplayFeedback.hpp"
#include "app/iggy3d/ProductInteractionMode.hpp"

namespace iggy3d {

struct ProductInteractionModeHud {
  bool visible = false;
  std::string status = "interaction_mode_hud_hidden";
  std::string reasonCode = "interaction_mode_hud_hidden";
  std::string mode = "player";
  std::string label = "player";
  ProductFeedbackTone tone = ProductFeedbackTone::Neutral;
  bool roomEditingReady = false;
};

struct ProductInteractionModeHudRequest {
  ProductInteractionMode mode = ProductInteractionMode::Player;
  bool gameplayActive = false;
  bool roomEditingReady = false;
};

ProductInteractionModeHud buildProductInteractionModeHud(
    ProductInteractionModeHudRequest request);

}  // namespace iggy3d
