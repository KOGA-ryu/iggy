#include "app/iggy3d/creative/camera/Fly.hpp"

#include <cmath>
#include <iostream>

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    return false;
  }
  return true;
}

bool near(float lhs, float rhs) {
  return std::fabs(lhs - rhs) <= 0.0001F;
}

bool vecNear(iggy3d::Vec3 lhs, iggy3d::Vec3 rhs) {
  return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
}

iggy3d::ProductCreativeFlyConfig testConfig() {
  iggy3d::ProductCreativeFlyConfig config;
  config.enabled = true;
  config.speedMetersPerSecond = 6.0F;
  config.sprintMultiplier = 2.0F;
  config.inputStepSeconds = 0.5F;
  return config;
}

bool disabledAndInvalidDoNotMove() {
  iggy3d::ProductCreativeFlyConfig disabled = testConfig();
  disabled.enabled = false;
  iggy3d::ProductCreativeFlyInput input;
  input.moveY = 1.0F;
  const iggy3d::ProductCreativeFlyResult disabledResult =
      iggy3d::applyProductCreativeFlyInput(disabled, input, {1.0F, 2.0F, 3.0F});

  iggy3d::ProductCreativeFlyConfig invalid = testConfig();
  invalid.speedMetersPerSecond = 0.0F;
  const iggy3d::ProductCreativeFlyResult invalidResult =
      iggy3d::applyProductCreativeFlyInput(invalid, input, {});

  return expect(!disabledResult.applied, "disabled not applied") &&
         expect(disabledResult.reasonCode == "creative_fly_disabled",
                "disabled reason") &&
         expect(vecNear(disabledResult.finalPositionMeters, {1.0F, 2.0F, 3.0F}),
                "disabled unchanged") &&
         expect(!invalidResult.applied, "invalid not applied") &&
         expect(invalidResult.reasonCode == "creative_fly_invalid_config",
                "invalid reason");
}

bool appliesYawRelativeMovement() {
  iggy3d::ProductCreativeFlyInput forward;
  forward.moveY = 1.0F;
  forward.cameraYawDegrees = 0.0F;
  const iggy3d::ProductCreativeFlyResult yaw0 =
      iggy3d::applyProductCreativeFlyInput(testConfig(), forward, {});

  forward.cameraYawDegrees = 90.0F;
  const iggy3d::ProductCreativeFlyResult yaw90 =
      iggy3d::applyProductCreativeFlyInput(testConfig(), forward, {});

  return expect(yaw0.applied, "yaw 0 applied") &&
         expect(vecNear(yaw0.finalPositionMeters, {0.0F, 0.0F, -3.0F}),
                "yaw 0 forward negative z") &&
         expect(yaw90.applied, "yaw 90 applied") &&
         expect(vecNear(yaw90.finalPositionMeters, {3.0F, 0.0F, 0.0F}),
                "yaw 90 forward positive x");
}

bool normalizesVerticalAndSprints() {
  iggy3d::ProductCreativeFlyInput input;
  input.moveX = 1.0F;
  input.moveZ = 1.0F;
  input.sprinting = true;
  const iggy3d::ProductCreativeFlyResult result =
      iggy3d::applyProductCreativeFlyInput(testConfig(), input, {});
  const float expected = 6.0F * 2.0F * 0.5F / std::sqrt(2.0F);

  iggy3d::ProductCreativeFlyInput noInput;
  const iggy3d::ProductCreativeFlyResult idle =
      iggy3d::applyProductCreativeFlyInput(testConfig(), noInput, {});

  return expect(result.applied, "diagonal applied") &&
         expect(near(result.speedMetersPerSecond, 12.0F), "sprint speed") &&
         expect(vecNear(result.finalPositionMeters, {expected, expected, 0.0F}),
                "diagonal normalized") &&
         expect(!idle.applied, "idle not applied") &&
         expect(idle.reasonCode == "creative_fly_no_input", "idle reason");
}

}  // namespace

int main() {
  const bool ok = disabledAndInvalidDoNotMove() &&
                  appliesYawRelativeMovement() && normalizesVerticalAndSprints();
  if (!ok) {
    return 1;
  }
  std::cout << "product_creative_fly_tests=pass\n";
  return 0;
}
