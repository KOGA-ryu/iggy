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
                               ProductViewportState& viewport,
                               std::string_view source) {
  const float lookX = actionAxisValue(actions, InputAction::PlayerLookX);
  const float lookY = actionAxisValue(actions, InputAction::PlayerLookY);
  if (lookX == 0.0F && lookY == 0.0F) {
    return;
  }

  constexpr float kYawStepDegrees = 6.0F;
  constexpr float kPitchStepDegrees = 4.0F;
  viewport.cameraControllerActive = true;
  viewport.lookInputUsed = true;
  viewport.cameraInputSource = std::string(source);
  viewport.cameraMode = "first_person";
  viewport.cameraController = "product_camera";
  viewport.cameraYawDegrees =
      wrapYaw(viewport.cameraYawDegrees + lookX * kYawStepDegrees);
  viewport.cameraPitchDegrees =
      clampPitch(viewport.cameraPitchDegrees + lookY * kPitchStepDegrees);
}

}  // namespace iggy3d
