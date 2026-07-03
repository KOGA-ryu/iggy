#include "app/iggy3d/window/MouseCapturePolicy.hpp"

namespace iggy3d {

ProductMouseCapturePolicy buildProductMouseCapturePolicy(
    const ProductMouseCapturePolicyRequest& request) {
  ProductMouseCapturePolicy policy;
  policy.inputOwner = std::string(menuOwnerName(request.inputOwner));
  // branch-gate: BG-1074
  if (!request.windowCaptureSupported) {
    policy.reasonCode = "mouse_capture_no_window";
    policy.mode = "no_window";
    return policy;
  }
  // branch-gate: BG-1074
  if (!request.gameplayActive) {
    policy.reasonCode = "mouse_capture_gameplay_inactive";
    return policy;
  }
  // branch-gate: BG-1074
  if (request.frontendBlocksGameplay) {
    policy.reasonCode = "mouse_capture_frontend_blocked";
    policy.mode = "gameplay_released";
    return policy;
  }
  // branch-gate: BG-1074
  if (request.inputOwner != MenuOwner::Gameplay) {
    policy.reasonCode = "mouse_capture_input_owner_blocked";
    policy.mode = "gameplay_released";
    return policy;
  }
  // CreativeDocument exposes absolute-position editor UI rows over the gameplay
  // backdrop. Releasing relative capture keeps those rows click-addressable.
  if (request.creativeDocumentActive) {
    policy.reasonCode = "mouse_capture_creative_editor_pointer";
    policy.mode = "gameplay_released";
    return policy;
  }
  // branch-gate: BG-1074
  if (request.interactionMode != ProductInteractionMode::Player &&
      request.interactionMode != ProductInteractionMode::Creative) {
    policy.reasonCode = "mouse_capture_mode_blocked";
    policy.mode = "gameplay_released";
    return policy;
  }
  // branch-gate: BG-1074
  if (!request.windowFocused) {
    policy.reasonCode = "mouse_capture_window_unfocused";
    policy.mode = "gameplay_released";
    return policy;
  }

  policy.requested = true;
  policy.status = "mouse_capture_requested";
  policy.reasonCode = "mouse_capture_gameplay_mouselook";
  policy.mode = "relative";
  return policy;
}

}  // namespace iggy3d
