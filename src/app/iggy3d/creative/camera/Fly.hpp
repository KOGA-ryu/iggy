#pragma once

#include <string>

#include "core/math/Vec3.hpp"

namespace iggy3d {

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

bool isValidProductCreativeFlyConfig(const ProductCreativeFlyConfig& config);
ProductCreativeFlyResult applyProductCreativeFlyInput(
    const ProductCreativeFlyConfig& config,
    const ProductCreativeFlyInput& input,
    Vec3 startPositionMeters);

}  // namespace iggy3d
