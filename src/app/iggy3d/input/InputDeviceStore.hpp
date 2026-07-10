#pragma once

#include <string>

#include "app/input/InputActions.hpp"
#include "app/iggy3d/debug/DebugHudState.hpp"
#include "app/iggy3d/input/InteractionMode.hpp"

namespace iggy3d {



// Owned mouse-capture policy state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). A new mouse-capture field now lands here, in its domain
// (window/MouseCapturePolicy), not in the 600+-member struct. Behavior-identical.
struct ProductMouseCaptureState {
  bool requested = false;
  bool active = false;
  std::string status = "mouse_capture_not_requested";
  std::string reasonCode = "mouse_capture_gameplay_inactive";
  std::string mode = "none";
  std::string inputOwner = "none";
};

// Owned controller mode-toggle state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: input/InteractionModeState. Behavior-identical.
struct ProductControllerModeToggleState {
  bool requested = false;
  bool accepted = false;
  std::string status = "interaction_mode_toggle_not_requested";
  std::string reasonCode = "interaction_mode_toggle_not_requested";
  std::string surface = "none";
};

// Owned controller-action routing state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: input/ControllerActionRouting. Behavior-identical.
struct ProductControllerActionState {
  bool mapped = false;
  std::string status = "controller_action_not_requested";
  std::string reasonCode = "controller_action_not_requested";
  std::string control = "none";
  std::string mode = "player";
  std::string surface = "none";
  std::string inputAction = "none";
};
struct InputDeviceStore {
  bool gamepadAvailable = false;
  std::string gamepadName = "unavailable";
  std::string gamepadMapping = "unavailable";
  InputAction lastInputAction = InputAction::None;
  bool lastInputAccepted = false;
  ProductMouseCaptureState mouseCapture;
  ProductControllerModeToggleState controllerModeToggle;
  ProductControllerActionState controllerAction;
  ProductInteractionMode interactionMode = ProductInteractionMode::Player;
  InteractionModeHud interactionModeHud;
};

}  // namespace iggy3d
