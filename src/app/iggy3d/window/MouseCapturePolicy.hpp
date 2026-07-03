#pragma once

#include <string>

#include "app/frontend/MenuInput.hpp"
#include "app/iggy3d/input/InteractionMode.hpp"

namespace iggy3d {

struct ProductMouseCapturePolicyRequest {
  bool gameplayActive = false;
  ProductInteractionMode interactionMode = ProductInteractionMode::Player;
  MenuOwner inputOwner = MenuOwner::None;
  bool frontendBlocksGameplay = true;
  bool windowFocused = true;
  bool windowCaptureSupported = true;
  bool creativeDocumentActive = false;
};

struct ProductMouseCapturePolicy {
  bool requested = false;
  std::string status = "mouse_capture_not_requested";
  std::string reasonCode = "mouse_capture_gameplay_inactive";
  std::string mode = "none";
  std::string inputOwner = "none";
};

ProductMouseCapturePolicy buildProductMouseCapturePolicy(
    const ProductMouseCapturePolicyRequest& request);

}  // namespace iggy3d
