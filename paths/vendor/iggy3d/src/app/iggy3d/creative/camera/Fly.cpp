#include "app/iggy3d/creative/camera/Fly.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace iggy3d {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kEpsilon = 0.0001F;
constexpr float kCameraBasisFallbackLengthSquared = 0.000001F;

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

bool validCameraFrameRequest(
    const ProductCreativeCameraFrameRequest& request) {
  return isFinite(request.boundsMinMeters) &&
         isFinite(request.boundsMaxMeters) &&
         request.boundsMinMeters.x <= request.boundsMaxMeters.x &&
         request.boundsMinMeters.y <= request.boundsMaxMeters.y &&
         request.boundsMinMeters.z <= request.boundsMaxMeters.z &&
         std::isfinite(request.cameraYawDegrees) &&
         std::isfinite(request.cameraPitchDegrees) &&
         std::isfinite(request.viewportAspectRatio) &&
         request.viewportAspectRatio > kEpsilon &&
         std::isfinite(request.verticalFovDegrees) &&
         request.verticalFovDegrees > kEpsilon &&
         request.verticalFovDegrees < 179.0F &&
         std::isfinite(request.eyeHeightMeters) &&
         std::isfinite(request.paddingFactor) &&
         request.paddingFactor >= 1.0F &&
         std::isfinite(request.minimumDistanceMeters) &&
         request.minimumDistanceMeters >= 0.0F &&
         std::isfinite(request.nearMarginMeters) &&
         request.nearMarginMeters > 0.0F;
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

ProductCreativeCameraFrameResult planProductCreativeCameraFrame(
    const ProductCreativeCameraFrameRequest& request) {
  ProductCreativeCameraFrameResult result;
  // branch-gate: BG-1205
  if (!validCameraFrameRequest(request)) {
    result.reasonCode = "creative_camera_frame_invalid_request";
    return result;
  }

  const float yaw = request.cameraYawDegrees * kPi / 180.0F;
  const float pitch = request.cameraPitchDegrees * kPi / 180.0F;
  const float cosPitch = std::cos(pitch);
  const Vec3 forward{std::sin(yaw) * cosPitch, std::sin(pitch),
                     -std::cos(yaw) * cosPitch};
  Vec3 right = cross(forward, {0.0F, 1.0F, 0.0F});
  const float rightLengthSquared = lengthSquared(right);
  // branch-gate: BG-1205
  if (!std::isfinite(rightLengthSquared)) {
    result.reasonCode = "creative_camera_frame_invalid_basis";
    return result;
  }
  // Match CreativeSceneFrame's view-basis fallback at near-vertical pitch.
  right = rightLengthSquared > kCameraBasisFallbackLengthSquared
              ? right / std::sqrt(rightLengthSquared)
              : Vec3{1.0F, 0.0F, 0.0F};
  const Vec3 up = cross(right, forward);

  const float verticalTangent =
      std::tan(request.verticalFovDegrees * kPi / 360.0F);
  const float horizontalTangent =
      verticalTangent * request.viewportAspectRatio;
  // branch-gate: BG-1205
  if (!std::isfinite(verticalTangent) || verticalTangent <= kEpsilon ||
      !std::isfinite(horizontalTangent) || horizontalTangent <= kEpsilon) {
    result.reasonCode = "creative_camera_frame_invalid_lens";
    return result;
  }

  const Vec3 center =
      (request.boundsMinMeters + request.boundsMaxMeters) * 0.5F;
  float distance = request.minimumDistanceMeters;
  for (std::uint32_t cornerIndex = 0U; cornerIndex < 8U; ++cornerIndex) {
    const Vec3 corner{
        (cornerIndex & 1U) != 0U ? request.boundsMaxMeters.x
                                 : request.boundsMinMeters.x,
        (cornerIndex & 2U) != 0U ? request.boundsMaxMeters.y
                                 : request.boundsMinMeters.y,
        (cornerIndex & 4U) != 0U ? request.boundsMaxMeters.z
                                 : request.boundsMinMeters.z};
    const Vec3 offset = corner - center;
    const float forwardOffset = dot(offset, forward);
    const float horizontalOffset = std::fabs(dot(offset, right));
    const float verticalOffset = std::fabs(dot(offset, up));
    distance = std::max(
        distance,
        horizontalOffset * request.paddingFactor / horizontalTangent -
            forwardOffset);
    distance =
        std::max(distance,
                 verticalOffset * request.paddingFactor / verticalTangent -
                     forwardOffset);
    distance =
        std::max(distance, request.nearMarginMeters - forwardOffset);
  }

  const Vec3 eye = center - forward * distance;
  const Vec3 anchor =
      eye - Vec3{0.0F, request.eyeHeightMeters, 0.0F};
  // branch-gate: BG-1205
  if (!std::isfinite(distance) || !isFinite(anchor)) {
    result.reasonCode = "creative_camera_frame_arithmetic_overflow";
    return result;
  }
  result.applied = true;
  result.reasonCode = "creative_camera_frame_applied";
  result.anchorPositionMeters = anchor;
  result.distanceMeters = distance;
  return result;
}

}  // namespace iggy3d
