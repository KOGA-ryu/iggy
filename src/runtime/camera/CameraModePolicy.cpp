#include "runtime/camera/CameraModePolicy.hpp"

namespace iggy3d {

namespace {

CameraModePolicyResult result(CameraPolicyStatus status, CameraState camera, bool changed) {
  return {status, camera, changed};
}

bool isKnownCameraMode(CameraMode mode) {
  return isRealtimeCameraMode(mode) || isTacticalCameraMode(mode);
}

}  // namespace

bool isRealtimeCameraMode(CameraMode mode) {
  return mode == CameraMode::FirstPerson || mode == CameraMode::ThirdPerson;
}

bool isTacticalCameraMode(CameraMode mode) {
  return mode == CameraMode::TacticalOverhead;
}

CameraState makeDefaultCameraState() {
  return {};
}

CameraModePolicyResult enterTacticalCamera(CameraState camera) {
  if (!isKnownCameraMode(camera.activeMode)) {
    return result(CameraPolicyStatus::InvalidCameraMode, camera, false);
  }
  if (isRealtimeCameraMode(camera.activeMode)) {
    camera.previousRealtimeMode = camera.activeMode;
  }
  if (camera.activeMode == CameraMode::TacticalOverhead) {
    return result(CameraPolicyStatus::Ok, camera, false);
  }
  camera.activeMode = CameraMode::TacticalOverhead;
  camera.inputClearRequested = true;
  return result(CameraPolicyStatus::Ok, camera, true);
}

CameraModePolicyResult exitTacticalCamera(CameraState camera) {
  if (!isRealtimeCameraMode(camera.previousRealtimeMode)) {
    return result(CameraPolicyStatus::InvalidCameraMode, camera, false);
  }
  if (camera.activeMode == camera.previousRealtimeMode) {
    return result(CameraPolicyStatus::Ok, camera, false);
  }
  camera.activeMode = camera.previousRealtimeMode;
  camera.inputClearRequested = true;
  return result(CameraPolicyStatus::Ok, camera, true);
}

CameraModePolicyResult applyClockModeToCamera(const CameraModePolicyRequest& request) {
  switch (request.clockMode) {
    case ClockMode::Normal:
      return exitTacticalCamera(request.camera);
    case ClockMode::Slow:
      return enterTacticalCamera(request.camera);
    case ClockMode::Paused:
      if (isTacticalCameraMode(request.camera.activeMode)) {
        return result(CameraPolicyStatus::Ok, request.camera, false);
      }
      return enterTacticalCamera(request.camera);
  }
  return result(CameraPolicyStatus::InvalidClockMode, request.camera, false);
}

CameraModePolicyResult clearCameraInputRequest(CameraState camera) {
  camera.inputClearRequested = false;
  return result(CameraPolicyStatus::Ok, camera, false);
}

}  // namespace iggy3d
