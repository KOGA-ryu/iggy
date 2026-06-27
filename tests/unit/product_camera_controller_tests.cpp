#include "app/iggy3d/view/CameraController.hpp"

#include <cmath>
#include <iostream>
#include <string_view>

#include "app/input/ActionState.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool nearlyEqual(float lhs, float rhs, float epsilon = 0.0001F) {
  return std::fabs(lhs - rhs) <= epsilon;
}

iggy3d::ActionState lookActions(float lookX, float lookY) {
  iggy3d::ActionState actions;
  iggy3d::recordAction(actions, iggy3d::InputAction::PlayerLookX, true, false,
                       false, lookX);
  iggy3d::recordAction(actions, iggy3d::InputAction::PlayerLookY, true, false,
                       false, lookY);
  return actions;
}

bool defaultLookIsNotInverted() {
  iggy3d::ProductViewportState viewport;
  iggy3d::FrontendSettings settings;

  iggy3d::applyProductCameraActions(lookActions(0.0F, -1.0F),
                                    viewport,
                                    settings,
                                    "unit/default_mouse_up");

  return expect(viewport.cameraControllerActive,
                "default look activates camera controller") &&
         expect(viewport.lookInputUsed, "default look records input") &&
         expect(viewport.cameraInputSource == "unit/default_mouse_up",
                "default look source") &&
         expect(nearlyEqual(viewport.cameraPitchDegrees, 4.0F),
                "negative look y looks up by default");
}

bool mouseDownLooksDownByDefault() {
  iggy3d::ProductViewportState viewport;
  iggy3d::FrontendSettings settings;

  iggy3d::applyProductCameraActions(lookActions(0.0F, 1.0F),
                                    viewport,
                                    settings,
                                    "unit/default_mouse_down");

  return expect(nearlyEqual(viewport.cameraPitchDegrees, -4.0F),
                "positive look y looks down by default");
}

bool invertLookFlipsPitchDirection() {
  iggy3d::ProductViewportState viewport;
  iggy3d::FrontendSettings settings;
  settings.invertLook = true;

  iggy3d::applyProductCameraActions(lookActions(0.0F, -1.0F),
                                    viewport,
                                    settings,
                                    "unit/inverted_mouse_up");

  return expect(nearlyEqual(viewport.cameraPitchDegrees, -4.0F),
                "invert look flips mouse up to look down");
}

bool yawStillTurnsRightOnPositiveLookX() {
  iggy3d::ProductViewportState viewport;
  iggy3d::FrontendSettings settings;

  iggy3d::applyProductCameraActions(lookActions(1.0F, 0.0F),
                                    viewport,
                                    settings,
                                    "unit/look_right");

  return expect(nearlyEqual(viewport.cameraYawDegrees, 6.0F),
                "positive look x still turns right") &&
         expect(nearlyEqual(viewport.cameraPitchDegrees, 0.0F),
                "pure yaw leaves pitch unchanged");
}

bool pitchClampsAtLimits() {
  iggy3d::ProductViewportState viewport;
  viewport.cameraPitchDegrees = 79.0F;
  iggy3d::FrontendSettings settings;

  iggy3d::applyProductCameraActions(lookActions(0.0F, -1.0F),
                                    viewport,
                                    settings,
                                    "unit/pitch_clamp");

  return expect(nearlyEqual(viewport.cameraPitchDegrees, 80.0F),
                "pitch clamps at upper limit");
}

}  // namespace

int main() {
  const bool ok = defaultLookIsNotInverted() && mouseDownLooksDownByDefault() &&
                  invertLookFlipsPitchDirection() &&
                  yawStillTurnsRightOnPositiveLookX() && pitchClampsAtLimits();
  return ok ? 0 : 1;
}
