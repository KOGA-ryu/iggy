#include "app/iggy3d/input/ControllerActionRouting.hpp"

#include <array>
#include <cmath>
#include <cstddef>

namespace iggy3d {
namespace {

struct ControllerControlInput {
  ProductControllerControl control = ProductControllerControl::None;
  bool down = false;
  float value = 0.0F;
  bool continuous = false;
};

constexpr bool productControllerActionIsContinuous(InputAction action) {
  return inputActionGroup(action) == InputActionGroup::Player;
}

constexpr std::size_t controlIndex(ProductControllerControl control) {
  return static_cast<std::size_t>(control);
}

std::array<ControllerControlInput, 22> controllerControlInputs(
    const GamepadControllerActionSample& sample) {
  const float leftX = sample.leftStickX;
  const float leftY = sample.leftStickY;
  const float rightX = sample.rightStickX;
  const float rightY = sample.rightStickY;
  return {
      ControllerControlInput{ProductControllerControl::LeftStickUp,
                             leftY > 0.0F,
                             std::fabs(leftY),
                             true},
      ControllerControlInput{ProductControllerControl::LeftStickDown,
                             leftY < 0.0F,
                             std::fabs(leftY),
                             true},
      ControllerControlInput{ProductControllerControl::LeftStickLeft,
                             leftX < 0.0F,
                             std::fabs(leftX),
                             true},
      ControllerControlInput{ProductControllerControl::LeftStickRight,
                             leftX > 0.0F,
                             std::fabs(leftX),
                             true},
      ControllerControlInput{ProductControllerControl::RightStickUp,
                             rightY > 0.0F,
                             std::fabs(rightY),
                             true},
      ControllerControlInput{ProductControllerControl::RightStickDown,
                             rightY < 0.0F,
                             std::fabs(rightY),
                             true},
      ControllerControlInput{ProductControllerControl::RightStickLeft,
                             rightX < 0.0F,
                             std::fabs(rightX),
                             true},
      ControllerControlInput{ProductControllerControl::RightStickRight,
                             rightX > 0.0F,
                             std::fabs(rightX),
                             true},
      ControllerControlInput{ProductControllerControl::DpadUp,
                             sample.dpadUpDown,
                             1.0F,
                             false},
      ControllerControlInput{ProductControllerControl::DpadDown,
                             sample.dpadDownDown,
                             1.0F,
                             false},
      ControllerControlInput{ProductControllerControl::DpadLeft,
                             sample.dpadLeftDown,
                             1.0F,
                             false},
      ControllerControlInput{ProductControllerControl::DpadRight,
                             sample.dpadRightDown,
                             1.0F,
                             false},
      ControllerControlInput{ProductControllerControl::SouthButton,
                             sample.southButtonDown,
                             1.0F,
                             false},
      ControllerControlInput{ProductControllerControl::EastButton,
                             sample.eastButtonDown,
                             1.0F,
                             false},
      ControllerControlInput{ProductControllerControl::WestButton,
                             sample.westButtonDown,
                             1.0F,
                             false},
      ControllerControlInput{ProductControllerControl::NorthButton,
                             sample.northButtonDown,
                             1.0F,
                             false},
      ControllerControlInput{ProductControllerControl::LeftShoulder,
                             sample.leftShoulderDown,
                             1.0F,
                             false},
      ControllerControlInput{ProductControllerControl::RightShoulder,
                             sample.rightShoulderDown,
                             1.0F,
                             false},
      ControllerControlInput{ProductControllerControl::LeftTrigger,
                             sample.leftTriggerDown,
                             1.0F,
                             false},
      ControllerControlInput{ProductControllerControl::RightTrigger,
                             sample.rightTriggerDown,
                             1.0F,
                             false},
      ControllerControlInput{ProductControllerControl::LeftStickPress,
                             sample.leftStickPressDown,
                             1.0F,
                             false},
      ControllerControlInput{ProductControllerControl::RightStickPress,
                             sample.rightStickPressDown,
                             1.0F,
                             false},
  };
}

GamepadControllerActionSample sampleWithLeftStick(float x, float y) {
  GamepadControllerActionSample sample;
  sample.leftStickX = x;
  sample.leftStickY = y;
  return sample;
}

GamepadControllerActionSample sampleWithRightStick(float x, float y) {
  GamepadControllerActionSample sample;
  sample.rightStickX = x;
  sample.rightStickY = y;
  return sample;
}

struct ControllerSampleRow {
  ProductControllerControl control;
  GamepadControllerActionSample sample;
};

}  // namespace

ProductControllerActionRoutingResult recordProductControllerMappedActions(
    ProductControllerActionRoutingRequest request) {
  ProductControllerActionRoutingResult result;
  result.surface = request.surface;
  result.interactionMode = request.interactionMode;

  const std::array inputs = controllerControlInputs(request.sample);
  for (const ControllerControlInput& input : inputs) {
    bool& wasDown = request.state.controlWasDown[controlIndex(input.control)];
    // branch-gate: BG-1060
    if (!input.down) {
      wasDown = false;
      continue;
    }

    const ProductControllerActionMapResult mapped =
        mapProductControllerAction({
            request.surface,
            request.interactionMode,
            input.control,
        });
    result.control = input.control;
    result.status = mapped.status;
    result.reasonCode = mapped.reasonCode;
    // branch-gate: BG-1060
    if (!mapped.mapped) {
      wasDown = true;
      continue;
    }

    const bool continuous =
        input.continuous && productControllerActionIsContinuous(mapped.action);
    const bool pressed = !wasDown;
    wasDown = true;
    // branch-gate: BG-1060
    if (!continuous && !pressed) {
      result.status = "controller_action_held";
      result.reasonCode = result.status;
      continue;
    }

    recordAction(request.actions,
                 mapped.action,
                 true,
                 !continuous,
                 false,
                 mapped.actionValue * input.value);
    result.mapped = true;
    ++result.mappedCount;
    result.action = mapped.action;
    result.status = "controller_action_mapped";
    result.reasonCode = result.status;
  }

  return result;
}

ProductControllerActionRoutingResult productControllerActionRoutingSkipped(
    ProductInputSurface surface,
    ProductInteractionMode interactionMode,
    std::string_view status) {
  ProductControllerActionRoutingResult result;
  result.surface = surface;
  result.interactionMode = interactionMode;
  result.status = status;
  result.reasonCode = status;
  return result;
}

GamepadControllerActionSample productControllerActionSampleForControl(
    ProductControllerControl control) {
  GamepadControllerActionSample dpadUp;
  dpadUp.dpadUpDown = true;
  GamepadControllerActionSample dpadDown;
  dpadDown.dpadDownDown = true;
  GamepadControllerActionSample dpadLeft;
  dpadLeft.dpadLeftDown = true;
  GamepadControllerActionSample dpadRight;
  dpadRight.dpadRightDown = true;
  GamepadControllerActionSample south;
  south.southButtonDown = true;
  GamepadControllerActionSample east;
  east.eastButtonDown = true;
  GamepadControllerActionSample west;
  west.westButtonDown = true;
  GamepadControllerActionSample north;
  north.northButtonDown = true;
  GamepadControllerActionSample leftShoulder;
  leftShoulder.leftShoulderDown = true;
  GamepadControllerActionSample rightShoulder;
  rightShoulder.rightShoulderDown = true;
  GamepadControllerActionSample leftTrigger;
  leftTrigger.leftTriggerDown = true;
  GamepadControllerActionSample rightTrigger;
  rightTrigger.rightTriggerDown = true;
  GamepadControllerActionSample leftStickPress;
  leftStickPress.leftStickPressDown = true;
  GamepadControllerActionSample rightStickPress;
  rightStickPress.rightStickPressDown = true;

  const std::array rows{
      ControllerSampleRow{ProductControllerControl::LeftStickUp,
                          sampleWithLeftStick(0.0F, 1.0F)},
      ControllerSampleRow{ProductControllerControl::LeftStickDown,
                          sampleWithLeftStick(0.0F, -1.0F)},
      ControllerSampleRow{ProductControllerControl::LeftStickLeft,
                          sampleWithLeftStick(-1.0F, 0.0F)},
      ControllerSampleRow{ProductControllerControl::LeftStickRight,
                          sampleWithLeftStick(1.0F, 0.0F)},
      ControllerSampleRow{ProductControllerControl::RightStickUp,
                          sampleWithRightStick(0.0F, 1.0F)},
      ControllerSampleRow{ProductControllerControl::RightStickDown,
                          sampleWithRightStick(0.0F, -1.0F)},
      ControllerSampleRow{ProductControllerControl::RightStickLeft,
                          sampleWithRightStick(-1.0F, 0.0F)},
      ControllerSampleRow{ProductControllerControl::RightStickRight,
                          sampleWithRightStick(1.0F, 0.0F)},
      ControllerSampleRow{ProductControllerControl::DpadUp, dpadUp},
      ControllerSampleRow{ProductControllerControl::DpadDown, dpadDown},
      ControllerSampleRow{ProductControllerControl::DpadLeft, dpadLeft},
      ControllerSampleRow{ProductControllerControl::DpadRight, dpadRight},
      ControllerSampleRow{ProductControllerControl::SouthButton, south},
      ControllerSampleRow{ProductControllerControl::EastButton, east},
      ControllerSampleRow{ProductControllerControl::WestButton, west},
      ControllerSampleRow{ProductControllerControl::NorthButton, north},
      ControllerSampleRow{ProductControllerControl::LeftShoulder, leftShoulder},
      ControllerSampleRow{ProductControllerControl::RightShoulder, rightShoulder},
      ControllerSampleRow{ProductControllerControl::LeftTrigger, leftTrigger},
      ControllerSampleRow{ProductControllerControl::RightTrigger, rightTrigger},
      ControllerSampleRow{ProductControllerControl::LeftStickPress,
                          leftStickPress},
      ControllerSampleRow{ProductControllerControl::RightStickPress,
                          rightStickPress},
  };
  for (const ControllerSampleRow& row : rows) {
    if (row.control == control) {  // branch-gate: BG-1060
      return row.sample;
    }
  }
  return {};
}

GamepadControllerActionSample productControllerModeChordActionSample() {
  GamepadControllerActionSample sample;
  sample.leftTriggerDown = true;
  sample.rightTriggerDown = true;
  sample.leftStickPressDown = true;
  sample.rightStickPressDown = true;
  return sample;
}

void recordProductControllerActionRoutingResult(
    ProductAppWindowState& window,
    const ProductControllerActionRoutingResult& result) {
  window.controllerActionMapped = result.mapped;
  window.controllerActionStatus = result.status;
  window.controllerActionReasonCode = result.reasonCode;
  window.controllerActionControl = productControllerControlName(result.control);
  window.controllerActionMode =
      productInteractionModeName(result.interactionMode);
  window.controllerActionSurface = productInputSurfaceName(result.surface);
  window.controllerActionInputAction = inputActionName(result.action);
}

}  // namespace iggy3d
