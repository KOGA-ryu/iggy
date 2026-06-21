#pragma once

#include <cstdint>

#include "runtime/camera/CameraState.hpp"
#include "runtime/clock/ClockState.hpp"

namespace iggy3d {

enum class CameraPolicyStatus : std::uint8_t {
  Ok,
  InvalidCameraMode,
  InvalidClockMode,
};

struct CameraModePolicyRequest {
  CameraState camera;
  ClockMode clockMode = ClockMode::Normal;
};

struct CameraModePolicyResult {
  CameraPolicyStatus status = CameraPolicyStatus::Ok;
  CameraState camera;
  bool modeChanged = false;
};

bool isRealtimeCameraMode(CameraMode mode);
bool isTacticalCameraMode(CameraMode mode);
CameraState makeDefaultCameraState();

CameraModePolicyResult enterTacticalCamera(CameraState camera);
CameraModePolicyResult exitTacticalCamera(CameraState camera);
CameraModePolicyResult applyClockModeToCamera(const CameraModePolicyRequest& request);
CameraModePolicyResult clearCameraInputRequest(CameraState camera);

}  // namespace iggy3d
