#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>

#include "app/iggy3d/creative/camera/Fly.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

enum class ProductCreativeViewportNavigationOperation : std::uint8_t {
  Orbit,
  Pan,
  Dolly,
  Count,
};

enum class ProductCreativeViewportNavigationStatus : std::uint8_t {
  Applied,
  NoInput,
  InvalidRequest,
  ArithmeticOverflow,
  Count,
};

struct ProductCreativeViewportNavigationConfig {
  float eyeHeightMeters = kProductCreativeCameraEyeHeightMeters;
  float verticalFovDegrees = kProductCreativeCameraVerticalFovDegrees;
  float defaultFocusDistanceMeters = 10.0F;
  float minimumFocusDistanceMeters = 0.25F;
  float maximumFocusDistanceMeters = 500.0F;
  float maximumPitchDegrees = 89.0F;
  float dollyExponentPerStep = 0.12F;
};

struct ProductCreativeViewportFocus {
  Vec3 worldPointMeters;
  float distanceMeters = 10.0F;
  bool valid = false;
};

struct ProductCreativeViewportPose {
  Vec3 anchorPositionMeters;
  float yawDegrees = 0.0F;
  float pitchDegrees = 0.0F;
};

struct ProductCreativeViewportNavigationRequest {
  ProductCreativeViewportNavigationConfig config;
  ProductCreativeViewportFocus focus;
  ProductCreativeViewportPose pose;
  ProductCreativeViewportNavigationOperation operation =
      ProductCreativeViewportNavigationOperation::Orbit;
  float horizontalInput = 0.0F;
  float verticalInput = 0.0F;
  float viewportHeightPixels = 1.0F;
  float orbitDegreesPerPixel = 0.12F;
};

struct ProductCreativeViewportNavigationResult {
  ProductCreativeViewportNavigationStatus status =
      ProductCreativeViewportNavigationStatus::InvalidRequest;
  ProductCreativeViewportFocus focus;
  ProductCreativeViewportPose pose;
  bool applied = false;
};

static_assert(std::is_trivially_copyable_v<ProductCreativeViewportFocus>);
static_assert(std::is_standard_layout_v<ProductCreativeViewportFocus>);
static_assert(std::is_trivially_copyable_v<ProductCreativeViewportPose>);
static_assert(std::is_standard_layout_v<ProductCreativeViewportPose>);

[[nodiscard]] std::string_view toString(
    ProductCreativeViewportNavigationStatus status) noexcept;
[[nodiscard]] bool isValidProductCreativeViewportNavigationConfig(
    const ProductCreativeViewportNavigationConfig& config) noexcept;

// Establishes an exact orbit focus after Frame Selection / Frame All. Invalid
// centers or distances fail closed and return an invalid focus.
[[nodiscard]] ProductCreativeViewportFocus
makeProductCreativeViewportFocus(Vec3 worldPointMeters,
                                 float distanceMeters) noexcept;

// Constant-time, allocation-free navigation shared by mouse, touchpad, and
// future controller camera modes. Orbit input is measured in pixels, pan uses
// perspective-correct world units per pixel, and dolly input is wheel steps.
[[nodiscard]] ProductCreativeViewportNavigationResult
applyProductCreativeViewportNavigation(
    const ProductCreativeViewportNavigationRequest& request) noexcept;

}  // namespace iggy3d
