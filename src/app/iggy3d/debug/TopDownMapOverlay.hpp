#pragma once

#include <cstdint>
#include <string>

#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/input/InteractionMode.hpp"

namespace iggy3d {

struct TopDownMapOverlay {
  bool visible = false;
  std::string purpose = "hidden";
  std::string size = "hidden";
  std::string status = "top_down_map_hidden";
  std::string reasonCode = "top_down_map_hidden";
  std::uint64_t itemCount = 0;
};

struct TopDownMapOverlayRequest {
  ProductRendererRequest rendererRequest = ProductRendererRequest::Vulkan;
  ProductInteractionMode interactionMode = ProductInteractionMode::Player;
  bool gameplayActive = false;
  bool roomEditingReady = false;
  std::uint64_t itemCount = 0;
};

TopDownMapOverlay buildTopDownMapOverlay(
    TopDownMapOverlayRequest request);

}  // namespace iggy3d
