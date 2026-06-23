#pragma once

#include <string>

namespace iggy3d {

struct ProductViewportState {
  bool gameplayViewVisible = false;
  std::string cameraMode = "first_person";
  std::string cameraController = "product_camera";
  bool cameraControllerActive = false;
  bool lookInputUsed = false;
  std::string cameraInputSource = "none";
  float cameraYawDegrees = 0.0F;
  float cameraPitchDegrees = 0.0F;
  bool cameraHeadingVisible = false;
};

}  // namespace iggy3d
