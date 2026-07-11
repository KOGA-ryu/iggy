#include "runtime/camera/CameraModePolicy.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool defaultAndModePredicates() {
  const iggy3d::CameraState camera = iggy3d::makeDefaultCameraState();
  return expect(iggy3d::isRealtimeCameraMode(iggy3d::CameraMode::FirstPerson), "first realtime") &&
         expect(iggy3d::isRealtimeCameraMode(iggy3d::CameraMode::ThirdPerson), "third realtime") &&
         expect(!iggy3d::isRealtimeCameraMode(iggy3d::CameraMode::TacticalOverhead),
                "tactical not realtime") &&
         expect(iggy3d::isTacticalCameraMode(iggy3d::CameraMode::TacticalOverhead),
                "tactical mode") &&
         expect(camera.activeMode == iggy3d::CameraMode::ThirdPerson, "default active") &&
         expect(camera.previousRealtimeMode == iggy3d::CameraMode::ThirdPerson, "default previous") &&
         expect(camera.orbitDistance == 8.0F, "default orbit") &&
         expect(!camera.inputClearRequested, "default clear flag");
}

bool tacticalFlow() {
  iggy3d::CameraState camera = iggy3d::makeDefaultCameraState();
  iggy3d::CameraModePolicyResult tactical = iggy3d::enterTacticalCamera(camera);
  bool ok = expect(tactical.status == iggy3d::CameraPolicyStatus::Ok, "enter tactical ok") &&
            expect(tactical.modeChanged, "enter changed") &&
            expect(tactical.camera.activeMode == iggy3d::CameraMode::TacticalOverhead,
                   "tactical active") &&
            expect(tactical.camera.previousRealtimeMode == iggy3d::CameraMode::ThirdPerson,
                   "previous preserved") &&
            expect(tactical.camera.inputClearRequested, "enter clear flag");

  iggy3d::CameraModePolicyResult paused =
      iggy3d::applyClockModeToCamera({tactical.camera, iggy3d::ClockMode::Paused});
  ok = ok && expect(paused.status == iggy3d::CameraPolicyStatus::Ok, "paused policy ok") &&
       expect(!paused.modeChanged, "paused unchanged") &&
       expect(paused.camera.activeMode == iggy3d::CameraMode::TacticalOverhead, "paused tactical");

  iggy3d::CameraModePolicyResult resumed =
      iggy3d::applyClockModeToCamera({paused.camera, iggy3d::ClockMode::Slow});
  ok = ok && expect(resumed.status == iggy3d::CameraPolicyStatus::Ok, "resume policy ok") &&
       expect(resumed.camera.activeMode == iggy3d::CameraMode::TacticalOverhead, "resume tactical");

  iggy3d::CameraModePolicyResult realtime =
      iggy3d::applyClockModeToCamera({resumed.camera, iggy3d::ClockMode::Normal});
  ok = ok && expect(realtime.status == iggy3d::CameraPolicyStatus::Ok, "exit policy ok") &&
       expect(realtime.modeChanged, "exit changed") &&
       expect(realtime.camera.activeMode == iggy3d::CameraMode::ThirdPerson, "exit third") &&
       expect(realtime.camera.inputClearRequested, "exit clear flag");

  iggy3d::CameraModePolicyResult cleared = iggy3d::clearCameraInputRequest(realtime.camera);
  ok = ok && expect(cleared.status == iggy3d::CameraPolicyStatus::Ok, "clear ok") &&
       expect(!cleared.camera.inputClearRequested, "clear flag cleared") &&
       expect(cleared.camera.activeMode == realtime.camera.activeMode, "clear preserves mode");
  return ok;
}

bool invalidPreviousRealtimeRejected() {
  iggy3d::CameraState camera = iggy3d::makeDefaultCameraState();
  camera.activeMode = iggy3d::CameraMode::TacticalOverhead;
  camera.previousRealtimeMode = iggy3d::CameraMode::TacticalOverhead;
  const iggy3d::CameraModePolicyResult result = iggy3d::exitTacticalCamera(camera);
  return expect(result.status == iggy3d::CameraPolicyStatus::InvalidCameraMode,
                "invalid previous rejected") &&
         expect(result.camera.activeMode == iggy3d::CameraMode::TacticalOverhead,
                "invalid unchanged");
}

}  // namespace

int main() {
  bool ok = true;
  ok = defaultAndModePredicates() && ok;
  ok = tacticalFlow() && ok;
  ok = invalidPreviousRealtimeRejected() && ok;
  return ok ? 0 : 1;
}
