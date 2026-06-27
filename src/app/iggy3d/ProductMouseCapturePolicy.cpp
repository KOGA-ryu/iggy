#include "app/iggy3d/ProductMouseCapturePolicy.hpp"

namespace iggy3d {

ProductMouseCapturePolicy buildProductMouseCapturePolicy(
    const ProductMouseCapturePolicyRequest& request) {
  ProductMouseCapturePolicy policy;
  // branch-gate: BG-1074
  if (!request.gameplayActive) {
    policy.reasonCode = "mouse_capture_gameplay_inactive";
    return policy;
  }
  // branch-gate: BG-1074
  if (request.frontendBlocksGameplay) {
    policy.reasonCode = "mouse_capture_frontend_blocked";
    return policy;
  }
  // branch-gate: BG-1074
  if (request.inputOwner != MenuOwner::Gameplay) {
    policy.reasonCode = "mouse_capture_input_owner_blocked";
    return policy;
  }
  // branch-gate: BG-1074
  if (request.interactionMode != ProductInteractionMode::Player) {
    policy.reasonCode = "mouse_capture_mode_blocked";
    return policy;
  }
  // branch-gate: BG-1074
  if (!request.windowFocused) {
    policy.reasonCode = "mouse_capture_window_unfocused";
    return policy;
  }

  policy.requested = true;
  policy.status = "mouse_capture_requested";
  policy.reasonCode = "mouse_capture_gameplay_mouselook";
  return policy;
}

}  // namespace iggy3d
