#include "app/iggy3d/ProductCameraController.hpp"

#include <string>

#include "app/input/ActionState.hpp"
#include "app/input/InputAction.hpp"

namespace iggy3d {
namespace {

float clampPitch(float pitch) {
  if (pitch < -80.0F) {
    return -80.0F;
  }
  if (pitch > 80.0F) {
    return 80.0F;
  }
  return pitch;
}

float wrapYaw(float yaw) {
  while (yaw < 0.0F) {
    yaw += 360.0F;
  }
  while (yaw >= 360.0F) {
    yaw -= 360.0F;
  }
  return yaw;
}

}  // namespace

void applyProductCameraActions(const ActionState& actions,
                               ProductAppWindowState& window,
                               std::string_view source) {
  const float lookX = actionAxisValue(actions, InputAction::PlayerLookX);
  const float lookY = actionAxisValue(actions, InputAction::PlayerLookY);
  if (lookX == 0.0F && lookY == 0.0F) {
    return;
  }

  constexpr float kYawStepDegrees = 6.0F;
  constexpr float kPitchStepDegrees = 4.0F;
  window.cameraControllerActive = true;
  window.lookInputUsed = true;
  window.cameraInputSource = std::string(source);
  window.cameraMode = "first_person";
  window.cameraController = "product_camera";
  window.cameraYawDegrees = wrapYaw(window.cameraYawDegrees + lookX * kYawStepDegrees);
  window.cameraPitchDegrees =
      clampPitch(window.cameraPitchDegrees + lookY * kPitchStepDegrees);
}

}  // namespace iggy3d
