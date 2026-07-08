#include "app/iggy3d/gameplay/ControllerKinematics.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string_view>

namespace iggy3d {
namespace {

constexpr float kPi = 3.14159265358979323846F;

}  // namespace

Vec3 productManualFirstPersonDirection(float moveX,
                                       float moveY,
                                       float yawDegrees) {
  const float yawRadians = yawDegrees * kPi / 180.0F;
  const float cosYaw = std::cos(yawRadians);
  const float sinYaw = std::sin(yawRadians);
  const Vec3 forward{sinYaw, 0.0F, -cosYaw};
  const Vec3 right{cosYaw, 0.0F, sinYaw};
  const Vec3 raw = right * moveX + forward * moveY;
  const float magnitude = std::sqrt(raw.x * raw.x + raw.z * raw.z);
  // branch-gate: BG-1155
  if (magnitude <= 0.0001F) {
    return forward;
  }
  return raw * (1.0F / magnitude);
}

float productManualFirstPersonMaxSpeedMetersPerSecond(
    const ProductGameplayMovementTuning& tuning,
    bool sprinting) {
  const std::array<float, 2U> speeds{tuning.walkSpeedMetersPerSecond,
                                     tuning.sprintSpeedMetersPerSecond};
  return speeds[static_cast<std::size_t>(sprinting)];
}

std::string_view productManualFirstPersonMovementProfile(
    const ProductGameplayMovementTuning& tuning,
    bool sprinting) {
  const std::array<std::string_view, 2U> profiles{tuning.walkProfile,
                                                  tuning.sprintProfile};
  return profiles[static_cast<std::size_t>(sprinting)];
}

Vec3 productManualFirstPersonMoveDelta(
    float moveX,
    float moveY,
    float yawDegrees,
    bool sprinting,
    const ProductGameplayMovementTuning& tuning,
    float responseMultiplier) {
  const float magnitude = std::sqrt(moveX * moveX + moveY * moveY);
  const float scale = 1.0F / std::max(1.0F, magnitude);
  const float stepMeters =
      productManualFirstPersonMaxSpeedMetersPerSecond(tuning, sprinting) *
      tuning.inputStepSeconds *
      std::clamp(responseMultiplier, 0.0F, 4.0F);
  const float yawRadians = yawDegrees * kPi / 180.0F;
  const float cosYaw = std::cos(yawRadians);
  const float sinYaw = std::sin(yawRadians);
  const Vec3 forward{sinYaw, 0.0F, -cosYaw};
  const Vec3 right{cosYaw, 0.0F, sinYaw};
  return (right * moveX + forward * moveY) * (scale * stepMeters);
}

Vec3 productManualFirstPersonDesiredVelocity(
    float moveX,
    float moveY,
    float yawDegrees,
    bool sprinting,
    const ProductGameplayMovementTuning& tuning) {
  const float magnitude = std::sqrt(moveX * moveX + moveY * moveY);
  // branch-gate: BG-1161
  if (magnitude <= 0.0F || !std::isfinite(magnitude)) {
    return {};
  }
  const float scale = 1.0F / std::max(1.0F, magnitude);
  const float speed =
      productManualFirstPersonMaxSpeedMetersPerSecond(tuning, sprinting);
  const float yawRadians = yawDegrees * kPi / 180.0F;
  const float cosYaw = std::cos(yawRadians);
  const float sinYaw = std::sin(yawRadians);
  const Vec3 forward{sinYaw, 0.0F, -cosYaw};
  const Vec3 right{cosYaw, 0.0F, sinYaw};
  return (right * moveX + forward * moveY) * (scale * speed);
}

Vec3 moveProductHorizontalVelocityToward(Vec3 current,
                                         Vec3 target,
                                         float maxDelta) {
  Vec3 delta{target.x - current.x, 0.0F, target.z - current.z};
  const float distance = std::sqrt(delta.x * delta.x + delta.z * delta.z);
  // branch-gate: BG-1161
  if (distance <= 0.0001F || !std::isfinite(distance)) {
    return target;
  }
  // branch-gate: BG-1161
  if (maxDelta >= distance) {
    return target;
  }
  const float scale = std::max(0.0F, maxDelta) / distance;
  return {current.x + delta.x * scale, 0.0F, current.z + delta.z * scale};
}

Vec3 clampProductHorizontalVelocity(Vec3 velocity, float maxSpeed) {
  const float speed =
      std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
  // branch-gate: BG-1161
  if (speed <= maxSpeed || speed <= 0.0001F || !std::isfinite(speed)) {
    return velocity;
  }
  const float scale = maxSpeed / speed;
  return {velocity.x * scale, 0.0F, velocity.z * scale};
}

}  // namespace iggy3d
