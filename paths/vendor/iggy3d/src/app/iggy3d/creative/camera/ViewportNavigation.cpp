#include "app/iggy3d/creative/camera/ViewportNavigation.hpp"

#include <algorithm>
#include <cmath>

namespace iggy3d {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kEpsilon = 0.0001F;
constexpr float kCameraBasisFallbackLengthSquared = 0.000001F;

[[nodiscard]] Vec3 cameraForward(float yawDegrees,
                                 float pitchDegrees) noexcept {
  const float yaw = yawDegrees * kPi / 180.0F;
  const float pitch = pitchDegrees * kPi / 180.0F;
  const float cosPitch = std::cos(pitch);
  return {std::sin(yaw) * cosPitch, std::sin(pitch),
          -std::cos(yaw) * cosPitch};
}

[[nodiscard]] bool validFocus(
    const ProductCreativeViewportFocus& focus,
    const ProductCreativeViewportNavigationConfig& config) noexcept {
  return focus.valid && isFinite(focus.worldPointMeters) &&
         std::isfinite(focus.distanceMeters) &&
         focus.distanceMeters >= config.minimumFocusDistanceMeters &&
         focus.distanceMeters <= config.maximumFocusDistanceMeters;
}

[[nodiscard]] ProductCreativeViewportFocus resolvedFocus(
    const ProductCreativeViewportNavigationRequest& request) noexcept {
  if (validFocus(request.focus, request.config)) {
    return request.focus;
  }
  const Vec3 eye = request.pose.anchorPositionMeters +
                   Vec3{0.0F, request.config.eyeHeightMeters, 0.0F};
  return makeProductCreativeViewportFocus(
      eye + cameraForward(request.pose.yawDegrees,
                          request.pose.pitchDegrees) *
                request.config.defaultFocusDistanceMeters,
      request.config.defaultFocusDistanceMeters);
}

[[nodiscard]] bool validRequest(
    const ProductCreativeViewportNavigationRequest& request) noexcept {
  return request.operation <
             ProductCreativeViewportNavigationOperation::Count &&
         isValidProductCreativeViewportNavigationConfig(request.config) &&
         isFinite(request.pose.anchorPositionMeters) &&
         std::isfinite(request.pose.yawDegrees) &&
         std::isfinite(request.pose.pitchDegrees) &&
         std::isfinite(request.horizontalInput) &&
         std::isfinite(request.verticalInput) &&
         std::isfinite(request.viewportHeightPixels) &&
         request.viewportHeightPixels > kEpsilon &&
         std::isfinite(request.orbitDegreesPerPixel) &&
         request.orbitDegreesPerPixel > 0.0F;
}

[[nodiscard]] bool operationHasInput(
    const ProductCreativeViewportNavigationRequest& request) noexcept {
  switch (request.operation) {
    case ProductCreativeViewportNavigationOperation::Orbit:
    case ProductCreativeViewportNavigationOperation::Pan:
      return std::fabs(request.horizontalInput) > kEpsilon ||
             std::fabs(request.verticalInput) > kEpsilon;
    case ProductCreativeViewportNavigationOperation::Dolly:
      return std::fabs(request.verticalInput) > kEpsilon;
    case ProductCreativeViewportNavigationOperation::Count:
      return false;
  }
  return false;
}

[[nodiscard]] Vec3 cameraRight(Vec3 forward) noexcept {
  Vec3 right = cross(forward, {0.0F, 1.0F, 0.0F});
  const float length2 = lengthSquared(right);
  if (!std::isfinite(length2)) {
    return {};
  }
  return length2 > kCameraBasisFallbackLengthSquared
             ? right / std::sqrt(length2)
             : Vec3{1.0F, 0.0F, 0.0F};
}

}  // namespace

std::string_view toString(
    ProductCreativeViewportNavigationStatus status) noexcept {
  switch (status) {
    case ProductCreativeViewportNavigationStatus::Applied: return "Applied";
    case ProductCreativeViewportNavigationStatus::NoInput: return "NoInput";
    case ProductCreativeViewportNavigationStatus::InvalidRequest:
      return "InvalidRequest";
    case ProductCreativeViewportNavigationStatus::ArithmeticOverflow:
      return "ArithmeticOverflow";
    case ProductCreativeViewportNavigationStatus::Count: break;
  }
  return "Unknown";
}

bool isValidProductCreativeViewportNavigationConfig(
    const ProductCreativeViewportNavigationConfig& config) noexcept {
  return std::isfinite(config.eyeHeightMeters) &&
         config.eyeHeightMeters >= 0.0F &&
         std::isfinite(config.verticalFovDegrees) &&
         config.verticalFovDegrees > kEpsilon &&
         config.verticalFovDegrees < 179.0F &&
         std::isfinite(config.defaultFocusDistanceMeters) &&
         config.defaultFocusDistanceMeters >= config.minimumFocusDistanceMeters &&
         config.defaultFocusDistanceMeters <= config.maximumFocusDistanceMeters &&
         std::isfinite(config.minimumFocusDistanceMeters) &&
         config.minimumFocusDistanceMeters > kEpsilon &&
         std::isfinite(config.maximumFocusDistanceMeters) &&
         config.maximumFocusDistanceMeters >= config.minimumFocusDistanceMeters &&
         std::isfinite(config.maximumPitchDegrees) &&
         config.maximumPitchDegrees > 0.0F &&
         config.maximumPitchDegrees < 90.0F &&
         std::isfinite(config.dollyExponentPerStep) &&
         config.dollyExponentPerStep > 0.0F;
}

ProductCreativeViewportFocus makeProductCreativeViewportFocus(
    Vec3 worldPointMeters,
    float distanceMeters) noexcept {
  ProductCreativeViewportFocus focus;
  if (!isFinite(worldPointMeters) || !std::isfinite(distanceMeters) ||
      distanceMeters <= 0.0F) {
    return focus;
  }
  focus.worldPointMeters = worldPointMeters;
  focus.distanceMeters = distanceMeters;
  focus.valid = true;
  return focus;
}

ProductCreativeViewportNavigationResult
applyProductCreativeViewportNavigation(
    const ProductCreativeViewportNavigationRequest& request) noexcept {
  ProductCreativeViewportNavigationResult result;
  result.focus = request.focus;
  result.pose = request.pose;
  if (!validRequest(request)) {
    return result;
  }
  if (!operationHasInput(request)) {
    result.status = ProductCreativeViewportNavigationStatus::NoInput;
    return result;
  }

  result.focus = resolvedFocus(request);
  if (!result.focus.valid) {
    return result;
  }

  switch (request.operation) {
    case ProductCreativeViewportNavigationOperation::Orbit: {
      result.pose.yawDegrees = std::remainder(
          request.pose.yawDegrees +
              request.horizontalInput * request.orbitDegreesPerPixel,
          360.0F);
      result.pose.pitchDegrees = std::clamp(
          request.pose.pitchDegrees -
              request.verticalInput * request.orbitDegreesPerPixel,
          -request.config.maximumPitchDegrees,
          request.config.maximumPitchDegrees);
      const Vec3 eye =
          result.focus.worldPointMeters -
          cameraForward(result.pose.yawDegrees, result.pose.pitchDegrees) *
              result.focus.distanceMeters;
      result.pose.anchorPositionMeters =
          eye - Vec3{0.0F, request.config.eyeHeightMeters, 0.0F};
      break;
    }
    case ProductCreativeViewportNavigationOperation::Pan: {
      const Vec3 forward =
          cameraForward(request.pose.yawDegrees, request.pose.pitchDegrees);
      const Vec3 right = cameraRight(forward);
      const Vec3 up = cross(right, forward);
      const float worldUnitsPerPixel =
          2.0F * result.focus.distanceMeters *
          std::tan(request.config.verticalFovDegrees * kPi / 360.0F) /
          request.viewportHeightPixels;
      const Vec3 translation =
          right * (-request.horizontalInput * worldUnitsPerPixel) +
          up * (request.verticalInput * worldUnitsPerPixel);
      result.focus.worldPointMeters =
          result.focus.worldPointMeters + translation;
      result.pose.anchorPositionMeters =
          result.pose.anchorPositionMeters + translation;
      break;
    }
    case ProductCreativeViewportNavigationOperation::Dolly: {
      result.focus.distanceMeters = std::clamp(
          result.focus.distanceMeters *
              std::exp(-request.verticalInput *
                       request.config.dollyExponentPerStep),
          request.config.minimumFocusDistanceMeters,
          request.config.maximumFocusDistanceMeters);
      const Vec3 eye =
          result.focus.worldPointMeters -
          cameraForward(request.pose.yawDegrees, request.pose.pitchDegrees) *
              result.focus.distanceMeters;
      result.pose.anchorPositionMeters =
          eye - Vec3{0.0F, request.config.eyeHeightMeters, 0.0F};
      break;
    }
    case ProductCreativeViewportNavigationOperation::Count:
      return result;
  }

  if (!isFinite(result.focus.worldPointMeters) ||
      !std::isfinite(result.focus.distanceMeters) ||
      !isFinite(result.pose.anchorPositionMeters) ||
      !std::isfinite(result.pose.yawDegrees) ||
      !std::isfinite(result.pose.pitchDegrees)) {
    result.status =
        ProductCreativeViewportNavigationStatus::ArithmeticOverflow;
    result.focus = request.focus;
    result.pose = request.pose;
    return result;
  }
  result.status = ProductCreativeViewportNavigationStatus::Applied;
  result.applied = true;
  return result;
}

}  // namespace iggy3d
