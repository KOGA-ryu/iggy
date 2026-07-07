#pragma once

#include <string>

#include "app/input/InputAction.hpp"
#include "app/iggy3d/input/ControllerActionState.hpp"
#include "app/iggy3d/input/ControllerModeToggleState.hpp"
#include "app/iggy3d/window/MouseCaptureState.hpp"

namespace iggy3d {

struct InputDeviceStore {
  bool gamepadAvailable = false;
  std::string gamepadName = "unavailable";
  std::string gamepadMapping = "unavailable";
  InputAction lastInputAction = InputAction::None;
  bool lastInputAccepted = false;
  ProductMouseCaptureState mouseCapture;
  ProductControllerModeToggleState controllerModeToggle;
  ProductControllerActionState controllerAction;
};

}  // namespace iggy3d
