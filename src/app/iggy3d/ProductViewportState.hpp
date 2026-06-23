#pragma once

#include <string>
#include <cstdint>

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
  bool productDrawGridVisible = false;
  bool productDrawPlayerVisible = false;
  bool productDrawRoomVisible = false;
  bool productDrawObjectiveVisible = false;
  bool productDrawTargetIndicatorVisible = false;
  std::uint64_t productDrawItemCount = 0;
  std::uint64_t productDrawDebugMarkerCount = 0;
};

}  // namespace iggy3d
