#pragma once

#include <cstdint>
#include <string_view>

namespace iggy3d {

enum class NeutralInput : std::uint8_t {
  None,

  KeyW,
  KeyA,
  KeyS,
  KeyD,
  KeyUp,
  KeyDown,
  KeyLeft,
  KeyRight,
  KeyEnter,
  KeySpace,
  KeyEscape,
  KeyTab,
  KeyShiftTab,
  KeyF1,
  KeyF2,
  KeyF3,
  KeyM,
  KeyE,
  KeyR,

  MouseLeft,
  MouseDeltaX,
  MouseDeltaY,

  ButtonSouth,
  ButtonEast,
  ButtonNorth,
  ButtonWest,
  DpadUp,
  DpadDown,
  DpadLeft,
  DpadRight,
  Start,
  Back,
  StartBackChord,
  LeftStickX,
  LeftStickY,
  RightStickX,
  RightStickY,
  LeftTrigger,
  RightTrigger,

  AutomationMenuUp,
  AutomationMenuDown,
  AutomationMenuConfirm,
  AutomationMenuBack,
  AutomationPause,
  AutomationDevTools,
  AutomationHardQuit,
  AutomationInteract,
  AutomationAttack,
  AutomationRetryOrReset,
};

struct InputDeviceEvent {
  NeutralInput input = NeutralInput::None;
  bool down = false;
  bool pressed = false;
  bool released = false;
  float value = 0.0F;
};

std::string_view neutralInputName(NeutralInput input);

}  // namespace iggy3d
