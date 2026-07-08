#include "app/iggy3d/gameplay/ControllerWallRunEvaluation.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ControllerWallQueries.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"

#include <algorithm>
#include <string>
#include <string_view>

namespace iggy3d {
namespace {

ProductWallRunCandidateEvaluationResult productWallRunCandidateRejected(
    std::string_view reason) {
  ProductWallRunCandidateEvaluationResult result;
  result.status = std::string{reason};
  result.reasonCode = std::string{reason};
  return result;
}

ProductWallRunActiveEvaluationResult productWallRunActiveRejected(
    std::string_view reason) {
  ProductWallRunActiveEvaluationResult result;
  result.status = std::string{reason};
  result.reasonCode = std::string{reason};
  return result;
}

ProductWallRunActiveEvaluationResult productWallRunActiveRecorded(
    const ProductAppWindowState& window,
    std::string_view status,
    std::string_view reason,
    float remainingSeconds) {
  ProductWallRunActiveEvaluationResult result;
  result.active = true;
  result.status = std::string{status};
  result.reasonCode = std::string{reason};
  result.remainingSeconds = remainingSeconds;
  result.durationSeconds =
      std::clamp(window.gameplay.gameplayMovement.tuning.wallRunDurationSeconds, 0.1F, 2.0F);
  result.gravityMultiplier =
      std::clamp(window.gameplay.gameplayMovement.tuning.wallRunGravityMultiplier, 0.0F, 1.0F);
  result.speedMultiplier =
      std::clamp(window.gameplay.gameplayMovement.tuning.wallRunSpeedMultiplier, 0.25F, 2.0F);
  return result;
}

void publishProductWallRunCandidateEvaluation(
    ProductAppWindowState& window,
    const ProductWallRunCandidateEvaluationResult& result) {
  window.gameplay.gameplayWallRun.candidateAvailable = result.available;
  window.gameplay.gameplayWallRun.candidateStatus = result.status;
  window.gameplay.gameplayWallRun.candidateReasonCode = result.reasonCode;
  window.gameplay.gameplayWallRun.side = result.side;
  window.gameplay.gameplayWallRun.surfaceId = result.surfaceId;
  window.gameplay.gameplayWallRun.normalX = result.normal.x;
  window.gameplay.gameplayWallRun.normalY = result.normal.y;
  window.gameplay.gameplayWallRun.normalZ = result.normal.z;
  window.gameplay.gameplayWallRun.approachSpeedMetersPerSecond =
      result.approachSpeedMetersPerSecond;
}

void publishProductWallRunActiveEvaluation(
    ProductAppWindowState& window,
    const ProductWallRunActiveEvaluationResult& result) {
  window.gameplay.gameplayWallRun.active = result.active;
  window.gameplay.gameplayWallRun.status = result.status;
  window.gameplay.gameplayWallRun.reasonCode = result.reasonCode;
  window.gameplay.gameplayWallRun.remainingSeconds = result.remainingSeconds;
  window.gameplay.gameplayWallRun.durationSeconds = result.durationSeconds;
  window.gameplay.gameplayWallRun.gravityMultiplier = result.gravityMultiplier;
  window.gameplay.gameplayWallRun.speedMultiplier = result.speedMultiplier;
}

ProductWallRunActiveEvaluationResult evaluateProductWallRunActiveWithoutJumpExit(
    const ProductWallRunEvaluationRequest& request,
    const ProductWallRunCandidateEvaluationResult& candidate) {
  const ProductAppWindowState& window = request.window;
  const bool hasMoveInput = request.moveX != 0.0F || request.moveY != 0.0F;
  // branch-gate: BG-1157
  if (window.gameplay.gameplayWallRun.active && !window.gameplay.gameplayJump.active) {
    return productWallRunActiveRejected("wall_run_landed");
  }
  // branch-gate: BG-1157
  if (window.gameplay.gameplayWallRun.active && !hasMoveInput) {
    return productWallRunActiveRejected("wall_run_input_stopped");
  }
  Vec3 tangent;
  // branch-gate: BG-1157
  if (window.gameplay.gameplayWallRun.active &&
      !wallRunTangentDirectionFromNormal(
          window, candidate.normal, request.moveX, request.moveY, tangent)) {
    return productWallRunActiveRejected("wall_run_input_away");
  }
  // branch-gate: BG-1157
  if (window.gameplay.gameplayWallRun.active && !candidate.available) {
    return productWallRunActiveRejected(candidate.reasonCode);
  }
  // branch-gate: BG-1157
  if (!window.gameplay.gameplayWallRun.active && (!candidate.available || !hasMoveInput)) {
    return productWallRunActiveRejected("wall_run_inactive");
  }
  // branch-gate: BG-1157
  if (!window.gameplay.gameplayWallRun.active &&
      !wallRunTangentDirectionFromNormal(
          window, candidate.normal, request.moveX, request.moveY, tangent)) {
    return productWallRunActiveRejected("wall_run_input_away");
  }

  const float dt = std::max(0.0F, window.gameplay.gameplayMovement.tuning.inputStepSeconds);
  // branch-gate: BG-1157
  if (!window.gameplay.gameplayWallRun.active) {
    const float remaining =
        std::clamp(window.gameplay.gameplayMovement.tuning.wallRunDurationSeconds,
                   0.1F,
                   2.0F);
    ProductWallRunActiveEvaluationResult result =
        productWallRunActiveRecorded(
            window, "wall_run_active", "wall_run_started", remaining);
    return result;
  }

  const float remaining =
      std::max(0.0F, window.gameplay.gameplayWallRun.remainingSeconds - dt);
  // branch-gate: BG-1157
  if (remaining <= 0.0F) {
    return productWallRunActiveRejected("wall_run_expired");
  }
  ProductWallRunActiveEvaluationResult result =
      productWallRunActiveRecorded(
          window, "wall_run_active", "wall_run_active", remaining);
  return result;
}

ProductWallRunCandidateEvaluationResult evaluateProductWallRunCandidate(
    const ProductWallRunEvaluationRequest& request) {
  const ProductAppWindowState& window = request.window;
  ProductWallRunCandidateEvaluationResult result;
  result.approachSpeedMetersPerSecond =
      window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond;

  // branch-gate: BG-1153
  if (!window.gameplay.gameplayJump.active) {
    return productWallRunCandidateRejected("wall_run_grounded");
  }
  const float minSpeed =
      std::max(0.0F, window.gameplay.gameplayMovement.tuning.wallRunMinSpeedMetersPerSecond);
  // branch-gate: BG-1161
  if (window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond < minSpeed) {
    result = productWallRunCandidateRejected("wall_run_low_speed");
    result.approachSpeedMetersPerSecond =
        window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond;
    return result;
  }
  // branch-gate: BG-1157
  if (request.collisionSurfaces == nullptr) {
    return productWallRunCandidateRejected("wall_run_no_surfaces");
  }
  // branch-gate: BG-1153
  if (!request.playerPosition.has_value()) {
    return productWallRunCandidateRejected("wall_run_missing_player");
  }

  Vec3 awayNormal;
  const CollisionSurfaceView* surface =
      findWallRunSurface(*request.collisionSurfaces,
                         *request.playerPosition,
                         awayNormal,
                         window.gameplay.gameplayMovement.tuning);
  // branch-gate: BG-1157
  if (surface == nullptr) {
    result = productWallRunCandidateRejected("wall_run_no_wall_contact");
    result.approachSpeedMetersPerSecond =
        window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond;
    return result;
  }
  // branch-gate: BG-1161
  if (!productMovementDebugAlongWall(window, awayNormal)) {
    result = productWallRunCandidateRejected("wall_run_not_along_wall");
    // branch-gate: BG-1157
    result.surfaceId = surface->id.empty() ? "wall_run_surface" : surface->id;
    result.normal = {awayNormal.x, surface->normal.y, awayNormal.z};
    result.approachSpeedMetersPerSecond =
        window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond;
    return result;
  }

  result.available = true;
  result.status = "wall_run_candidate";
  result.reasonCode = "wall_run_candidate";
  result.side = wallRunSideName(awayNormal, window.viewport.cameraYawDegrees);
  // branch-gate: BG-1157
  result.surfaceId = surface->id.empty() ? "wall_run_surface" : surface->id;
  result.normal = {awayNormal.x, surface->normal.y, awayNormal.z};
  result.approachSpeedMetersPerSecond =
      window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond;
  return result;
}

}  // namespace

void publishProductWallRunEvaluation(
    ProductAppWindowState& window,
    const ProductWallRunEvaluationResult& result) {
  publishProductWallRunCandidateEvaluation(window, result.candidate);
  publishProductWallRunActiveEvaluation(window, result.active);
}

void clearProductWallRunActiveProof(ProductAppWindowState& window,
                                    std::string_view reason) {
  publishProductWallRunActiveEvaluation(
      window, productWallRunActiveRejected(reason));
}

ProductWallRunEvaluationResult evaluateProductWallRun(
    const ProductWallRunEvaluationRequest& request) {
  ProductWallRunEvaluationResult result;
  result.candidate = evaluateProductWallRunCandidate(request);
  result.active = evaluateProductWallRunActiveWithoutJumpExit(
      request, result.candidate);
  // branch-gate: BG-1157
  if (request.jumpPressed) {
    result.active = productWallRunActiveRejected("wall_run_exit_jump");
  }
  return result;
}

}  // namespace iggy3d
