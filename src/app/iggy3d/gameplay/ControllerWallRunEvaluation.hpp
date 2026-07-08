#pragma once

#include "core/math/Vec3.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace iggy3d {

class SpatialSurfaceSet;
struct ProductAppWindowState;

struct ProductWallRunCandidateEvaluationResult {
  bool available = false;
  std::string status = "wall_run_not_checked";
  std::string reasonCode = "wall_run_not_checked";
  std::string side = "none";
  std::string surfaceId = "none";
  Vec3 normal{0.0F, 0.0F, 0.0F};
  float approachSpeedMetersPerSecond = 0.0F;
};

struct ProductWallRunActiveEvaluationResult {
  bool active = false;
  std::string status = "wall_run_inactive";
  std::string reasonCode = "wall_run_inactive";
  float remainingSeconds = 0.0F;
  float durationSeconds = 0.0F;
  float gravityMultiplier = 1.0F;
  float speedMultiplier = 1.0F;
};

struct ProductWallRunEvaluationRequest {
  const ProductAppWindowState& window;
  const SpatialSurfaceSet* collisionSurfaces = nullptr;
  std::optional<Vec3> playerPosition;
  float moveX = 0.0F;
  float moveY = 0.0F;
  bool jumpPressed = false;
};

struct ProductWallRunEvaluationResult {
  ProductWallRunCandidateEvaluationResult candidate;
  ProductWallRunActiveEvaluationResult active;
};

void publishProductWallRunEvaluation(
    ProductAppWindowState& window,
    const ProductWallRunEvaluationResult& result);

void clearProductWallRunActiveProof(ProductAppWindowState& window,
                                    std::string_view reason);

ProductWallRunEvaluationResult evaluateProductWallRun(
    const ProductWallRunEvaluationRequest& request);

}  // namespace iggy3d
