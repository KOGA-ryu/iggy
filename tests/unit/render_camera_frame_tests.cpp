#include "render/FrameInput.hpp"

#include "runtime/camera/CameraState.hpp"

#include <iostream>
#include <limits>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::FrameInput validFrame(const iggy3d::SceneProjectionResult& scene) {
  iggy3d::FrameInput frame;
  frame.viewport = {640U, 480U, 640.0F / 480.0F};
  frame.clock = {1U, 1U, 0.0F, 0.0F};
  frame.camera.mode = iggy3d::RenderCameraMode::ThirdPerson;
  frame.camera.worldEye = {0.0F, 2.0F, 5.0F};
  frame.camera.worldForward = {0.0F, 0.0F, -1.0F};
  frame.camera.worldUp = {0.0F, 1.0F, 0.0F};
  frame.camera.nearPlane = 0.1F;
  frame.camera.farPlane = 200.0F;
  frame.projections.scene = &scene;
  return frame;
}

bool cameraModesAreStable() {
  iggy3d::SceneProjectionResult scene;
  iggy3d::FrameInput firstPerson = validFrame(scene);
  firstPerson.camera.mode = iggy3d::RenderCameraMode::FirstPerson;
  iggy3d::FrameInput tactical = validFrame(scene);
  tactical.camera.mode = iggy3d::RenderCameraMode::TacticalOverhead;

  return expect(iggy3d::validateFrameInput(validFrame(scene)) == iggy3d::FrameInputStatus::Valid,
                "third person valid") &&
         expect(iggy3d::validateFrameInput(firstPerson) == iggy3d::FrameInputStatus::Valid,
                "first person valid") &&
         expect(iggy3d::validateFrameInput(tactical) == iggy3d::FrameInputStatus::Valid,
                "tactical valid") &&
         expect(iggy3d::renderCameraModeName(iggy3d::RenderCameraMode::FirstPerson) ==
                    "first_person",
                "first person name") &&
         expect(iggy3d::renderCameraModeName(iggy3d::RenderCameraMode::TacticalOverhead) ==
                    "tactical_overhead",
                "tactical name");
}

bool cameraFailuresAreDiagnosed() {
  iggy3d::SceneProjectionResult scene;
  const float infinity = std::numeric_limits<float>::infinity();

  iggy3d::FrameInput invalidMode = validFrame(scene);
  invalidMode.camera.mode = static_cast<iggy3d::RenderCameraMode>(99U);
  iggy3d::FrameInput badEye = validFrame(scene);
  badEye.camera.worldEye.x = infinity;
  iggy3d::FrameInput badForward = validFrame(scene);
  badForward.camera.worldForward.z = infinity;
  iggy3d::FrameInput badMatrix = validFrame(scene);
  badMatrix.camera.clipFromWorld.m[0] = infinity;
  iggy3d::FrameInput badNear = validFrame(scene);
  badNear.camera.nearPlane = 0.0F;
  iggy3d::FrameInput badFar = validFrame(scene);
  badFar.camera.farPlane = badFar.camera.nearPlane;

  return expect(iggy3d::validateFrameInput(invalidMode) ==
                    iggy3d::FrameInputStatus::InvalidCameraMode,
                "invalid mode") &&
         expect(iggy3d::validateFrameInput(badEye) ==
                    iggy3d::FrameInputStatus::InvalidCameraBasis,
                "bad eye") &&
         expect(iggy3d::validateFrameInput(badForward) ==
                    iggy3d::FrameInputStatus::InvalidCameraBasis,
                "bad forward") &&
         expect(iggy3d::validateFrameInput(badMatrix) ==
                    iggy3d::FrameInputStatus::InvalidCameraMatrix,
                "bad matrix") &&
         expect(iggy3d::validateFrameInput(badNear) ==
                    iggy3d::FrameInputStatus::InvalidClipPlanes,
                "bad near") &&
         expect(iggy3d::validateFrameInput(badFar) ==
                    iggy3d::FrameInputStatus::InvalidClipPlanes,
                "bad far") &&
         expect(iggy3d::frameInputReasonCode(iggy3d::FrameInputStatus::InvalidCameraBasis) ==
                    "frame_camera_basis_invalid",
                "basis reason");
}

bool runtimeCameraTruthIsNotMutated() {
  iggy3d::CameraState camera;
  const iggy3d::CameraMode before = camera.activeMode;
  iggy3d::SceneProjectionResult scene;
  iggy3d::FrameInput frame = validFrame(scene);
  frame.camera.mode = iggy3d::RenderCameraMode::ThirdPerson;
  const iggy3d::FrameInputStatus status = iggy3d::validateFrameInput(frame);
  return expect(status == iggy3d::FrameInputStatus::Valid, "render camera valid") &&
         expect(camera.activeMode == before, "runtime camera unchanged");
}

}  // namespace

int main() {
  bool ok = true;
  ok = cameraModesAreStable() && ok;
  ok = cameraFailuresAreDiagnosed() && ok;
  ok = runtimeCameraTruthIsNotMutated() && ok;
  return ok ? 0 : 1;
}
