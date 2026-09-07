#pragma once

#include <string>

#include "core/math/Vec3.hpp"

namespace iggy3d {

inline constexpr float kProductCreativeCameraEyeHeightMeters = 1.7F;
inline constexpr float kProductCreativeCameraVerticalFovDegrees = 68.0F;

struct ProductCreativeFlyConfig {
  bool enabled = false;
  float speedMetersPerSecond = 8.0F;
  float sprintMultiplier = 3.0F;
  float inputStepSeconds = 1.0F / 60.0F;
};

struct ProductCreativeFlyInput {
  float moveX = 0.0F;
  float moveY = 0.0F;
  float moveZ = 0.0F;
  bool sprinting = false;
  float cameraYawDegrees = 0.0F;
  float cameraPitchDegrees = 0.0F;
};

struct ProductCreativeFlyResult {
  bool applied = false;
  std::string reasonCode = "creative_fly_not_requested";
  Vec3 deltaMeters;
  Vec3 finalPositionMeters;
  float speedMetersPerSecond = 0.0F;
};

struct ProductCreativeCameraFrameRequest {
  Vec3 boundsMinMeters;
  Vec3 boundsMaxMeters;
  float cameraYawDegrees = 0.0F;
  float cameraPitchDegrees = 0.0F;
  float viewportAspectRatio = 1.0F;
  float verticalFovDegrees = kProductCreativeCameraVerticalFovDegrees;
  float eyeHeightMeters = kProductCreativeCameraEyeHeightMeters;
  float paddingFactor = 1.12F;
  float minimumDistanceMeters = 3.0F;
  float nearMarginMeters = 0.25F;
};

struct ProductCreativeCameraFrameResult {
  bool applied = false;
  std::string reasonCode = "creative_camera_frame_not_requested";
  Vec3 anchorPositionMeters;
  float distanceMeters = 0.0F;
};

bool isValidProductCreativeFlyConfig(const ProductCreativeFlyConfig& config);
ProductCreativeFlyResult applyProductCreativeFlyInput(
    const ProductCreativeFlyConfig& config,
    const ProductCreativeFlyInput& input,
    Vec3 startPositionMeters);

// Constant-time and allocation-free. Preserves the current view direction and
// moves only the camera anchor. The exact eight AABB corners must fit the
// requested perspective frustum; invalid or reversed bounds fail closed.
ProductCreativeCameraFrameResult planProductCreativeCameraFrame(
    const ProductCreativeCameraFrameRequest& request);

}  // namespace iggy3d
