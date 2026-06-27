#include "app/iggy3d/ProductControllerActionRouting.hpp"

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
