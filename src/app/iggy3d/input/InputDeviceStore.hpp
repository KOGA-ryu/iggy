#pragma once

#include <string>

#include "app/input/InputAction.hpp"

namespace iggy3d {

struct InputDeviceStore {
  bool gamepadAvailable = false;
  std::string gamepadName = "unavailable";
  std::string gamepadMapping = "unavailable";
  InputAction lastInputAction = InputAction::None;
  bool lastInputAccepted = false;
};

}  // namespace iggy3d
