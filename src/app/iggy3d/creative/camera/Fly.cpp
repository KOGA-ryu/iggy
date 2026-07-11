#include "app/iggy3d/creative/camera/Fly.hpp"

#include <cmath>

namespace iggy3d {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kEpsilon = 0.0001F;

float safeAxis(float value) {
  // branch-gate: BG-1205
  if (!std::isfinite(value)) {
    return 0.0F;
  }
  // branch-gate: BG-1205
  if (value < -1.0F) {
    return -1.0F;
  }
  // branch-gate: BG-1205
  if (value > 1.0F) {
    return 1.0F;
  }
  return value;
}

}  // namespace

bool isValidProductCreativeFlyConfig(const ProductCreativeFlyConfig& config) {
  return std::isfinite(config.speedMetersPerSecond) &&
         config.speedMetersPerSecond > kEpsilon &&
         std::isfinite(config.sprintMultiplier) &&
         config.sprintMultiplier >= 1.0F &&
         std::isfinite(config.inputStepSeconds) &&
         config.inputStepSeconds > kEpsilon;
}

ProductCreativeFlyResult applyProductCreativeFlyInput(
    const ProductCreativeFlyConfig& config,
    const ProductCreativeFlyInput& input,
    Vec3 startPositionMeters) {
  ProductCreativeFlyResult result;
  result.finalPositionMeters = startPositionMeters;

  // branch-gate: BG-1205
  if (!config.enabled) {
    result.reasonCode = "creative_fly_disabled";
    return result;
  }
  // branch-gate: BG-1205
  if (!isValidProductCreativeFlyConfig(config) ||
      !isFinite(startPositionMeters) ||
      !std::isfinite(input.cameraYawDegrees)) {
    result.reasonCode = "creative_fly_invalid_config";
    return result;
  }

  const float moveX = safeAxis(input.moveX);
  const float moveY = safeAxis(input.moveY);
  const float moveZ = safeAxis(input.moveZ);
  const float magnitude =
      std::sqrt(moveX * moveX + moveY * moveY + moveZ * moveZ);
  // branch-gate: BG-1205
  if (magnitude <= kEpsilon) {
    result.reasonCode = "creative_fly_no_input";
    result.speedMetersPerSecond = config.speedMetersPerSecond;
    return result;
  }

  const float yaw = input.cameraYawDegrees * kPi / 180.0F;
  const float cosYaw = std::cos(yaw);
  const float sinYaw = std::sin(yaw);
  const Vec3 forward{sinYaw, 0.0F, -cosYaw};
  const Vec3 right{cosYaw, 0.0F, sinYaw};
  const Vec3 up{0.0F, 1.0F, 0.0F};
  const float invMagnitude = 1.0F / magnitude;
  const float analogMagnitude = std::min(magnitude, 1.0F);
  // branch-gate: BG-1205
  const float speed =
      config.speedMetersPerSecond *
      (input.sprinting ? config.sprintMultiplier : 1.0F) * analogMagnitude;
  const Vec3 direction =
      (right * moveX + forward * moveY + up * moveZ) * invMagnitude;

  result.deltaMeters = direction * speed * config.inputStepSeconds;
  result.finalPositionMeters = startPositionMeters + result.deltaMeters;
  result.speedMetersPerSecond = speed;
  result.applied = true;
  result.reasonCode = "creative_fly_applied";
  return result;
}

}  // namespace iggy3d
