#include "runtime/physics/PhysicsAabbStep.hpp"

#include <array>
#include <cmath>

namespace iggy3d {
namespace {

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1096
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

PhysicsAabbStepResult stepResult(PhysicsAabbStepStatus status, bool ok) {
  PhysicsAabbStepResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsAabbStepStatusName(status);
  return result;
}

bool validStepSeconds(float value) {
  return std::isfinite(value) && value > 0.0F;
}

bool validBroadphaseCellSize(float value) {
  return std::isfinite(value) && value > 0.0F;
}

std::string_view invalidConfigReason(const PhysicsAabbStepConfig& config) {
  // branch-gate: BG-1096
  if (!validStepSeconds(config.stepConfig.stepSeconds)) {
    return "physics_step_invalid_step_seconds";
  }
  // branch-gate: BG-1096
  if (!isFinite(config.stepConfig.gravityMetersPerSecondSquared)) {
    return "physics_step_invalid_gravity";
  }
  // branch-gate: BG-1096
  if (!validBroadphaseCellSize(config.collisionConfig.broadphaseCellSizeMeters)) {
    return "physics_broadphase_invalid_grid_config";
  }
  // branch-gate: BG-1096
  if (!isValidPhysicsAabbContactSolveConfig(
          config.collisionConfig.solveConfig)) {
    return "physics_aabb_contact_solve_invalid_config";
  }
  // branch-gate: BG-1096
  if (!isValidPhysicsBodyDeltaApplyConfig(config.deltaApplyConfig)) {
    return "physics_body_delta_invalid_config";
  }
  return {};
}

void copySummaryFromBatch(PhysicsAabbStepResult& result) {
  result.colliderCount = result.collisionBatch.colliderCount;
  result.broadphasePairCount = result.collisionBatch.broadphasePairCount;
  result.contactCount = result.collisionBatch.contactCount;
  result.solvePlanCount = result.collisionBatch.solvePlanCount;
  result.accumulatedPlanCount = result.collisionBatch.accumulatedPlanCount;
}

void copySummaryFromApply(PhysicsAabbStepResult& result) {
  result.appliedPositionCount = result.deltaApply.appliedPositionCount;
  result.appliedVelocityCount = result.deltaApply.appliedVelocityCount;
}

PhysicsAabbStepResult invalidConfigResult(std::string_view reason) {
  PhysicsAabbStepResult result =
      stepResult(PhysicsAabbStepStatus::InvalidConfig, false);
  result.upstreamReasonCode = reason;
  return result;
}

PhysicsAabbStepResult velocityFailedResult(
    const PhysicsStepResult& velocity) {
  PhysicsAabbStepResult result =
      stepResult(PhysicsAabbStepStatus::VelocityPhaseFailed, false);
  result.upstreamReasonCode = velocity.reasonCode;
  result.velocityPhase = velocity;
  result.bodyCount = velocity.bodyCount;
  return result;
}

PhysicsAabbStepResult collisionBatchFailedResult(
    const PhysicsStepResult& velocity,
    const PhysicsAabbCollisionBatchResult& batch) {
  PhysicsAabbStepResult result =
      stepResult(PhysicsAabbStepStatus::CollisionBatchFailed, false);
  result.upstreamReasonCode = batch.reasonCode;
  result.velocityPhase = velocity;
  result.collisionBatch = batch;
  result.bodyCount = velocity.bodyCount;
  copySummaryFromBatch(result);
  return result;
}

PhysicsAabbStepResult deltaApplyFailedResult(
    const PhysicsStepResult& velocity,
    const PhysicsAabbCollisionBatchResult& batch,
    const PhysicsBodyDeltaApplyResult& apply) {
  PhysicsAabbStepResult result =
      stepResult(PhysicsAabbStepStatus::DeltaApplyFailed, false);
  result.upstreamReasonCode = apply.reasonCode;
  result.velocityPhase = velocity;
  result.collisionBatch = batch;
  result.deltaApply = apply;
  result.bodyCount = velocity.bodyCount;
  copySummaryFromBatch(result);
  copySummaryFromApply(result);
  return result;
}

PhysicsAabbStepResult positionFailedResult(
    const PhysicsStepResult& velocity,
    const PhysicsAabbCollisionBatchResult& batch,
    const PhysicsBodyDeltaApplyResult& apply,
    const PhysicsStepResult& position) {
  PhysicsAabbStepResult result =
      stepResult(PhysicsAabbStepStatus::PositionPhaseFailed, false);
  result.upstreamReasonCode = position.reasonCode;
  result.velocityPhase = velocity;
  result.collisionBatch = batch;
  result.deltaApply = apply;
  result.positionPhase = position;
  result.bodyCount = velocity.bodyCount;
  copySummaryFromBatch(result);
  copySummaryFromApply(result);
  return result;
}

PhysicsAabbStepResult steppedResult(
    const PhysicsStepResult& velocity,
    const PhysicsAabbCollisionBatchResult& batch,
    const PhysicsBodyDeltaApplyResult& apply,
    const PhysicsStepResult& position) {
  PhysicsAabbStepResult result =
      stepResult(PhysicsAabbStepStatus::Stepped, true);
  result.velocityPhase = velocity;
  result.collisionBatch = batch;
  result.deltaApply = apply;
  result.positionPhase = position;
  result.bodyCount = position.bodyCount;
  copySummaryFromBatch(result);
  copySummaryFromApply(result);
  return result;
}

}  // namespace

std::string_view physicsAabbStepStatusName(PhysicsAabbStepStatus status) {
  static constexpr std::array<std::string_view, 7> kNames{
      "physics_aabb_step_stepped",
      "physics_aabb_step_velocity_phase_failed",
      "physics_aabb_step_collision_batch_failed",
      "physics_aabb_step_delta_apply_failed",
      "physics_aabb_step_position_phase_failed",
      "physics_aabb_step_invalid_config",
      "physics_aabb_step_missing_body_store",
  };
  return enumName(status, kNames, "physics_aabb_step_invalid_config");
}

bool isValidPhysicsAabbStepConfig(const PhysicsAabbStepConfig& config) {
  return invalidConfigReason(config).empty();
}

PhysicsAabbStepResult stepPhysicsAabbWorld(
    const PhysicsAabbStepRequest& request) {
  // branch-gate: BG-1096
  if (request.bodies == nullptr) {
    return stepResult(PhysicsAabbStepStatus::MissingBodyStore, false);
  }

  const std::string_view configReason = invalidConfigReason(request.config);
  // branch-gate: BG-1096
  if (!configReason.empty()) {
    return invalidConfigResult(configReason);
  }

  const PhysicsStepResult velocity =
      integratePhysicsBodyVelocities(request.bodies,
                                     request.config.stepConfig);
  // branch-gate: BG-1096
  if (!velocity.ok) {
    return velocityFailedResult(velocity);
  }

  PhysicsAabbCollisionBatchRequest batchRequest;
  batchRequest.bodies = request.bodies;
  batchRequest.shapes = request.shapes;
  batchRequest.materials = request.materials;
  batchRequest.bindings = request.bindings;
  batchRequest.config = request.config.collisionConfig;
  const PhysicsAabbCollisionBatchResult batch =
      planPhysicsAabbCollisionBatch(batchRequest);
  // branch-gate: BG-1096
  if (!batch.ok) {
    return collisionBatchFailedResult(velocity, batch);
  }

  const PhysicsBodyDeltaApplyResult apply =
      applyPhysicsBodyDeltas(request.bodies, batch.accumulator,
                             request.config.deltaApplyConfig);
  // branch-gate: BG-1096
  if (!apply.ok) {
    return deltaApplyFailedResult(velocity, batch, apply);
  }

  const PhysicsStepResult position =
      integratePhysicsBodyPositions(request.bodies,
                                    request.config.stepConfig);
  // branch-gate: BG-1096
  if (!position.ok) {
    return positionFailedResult(velocity, batch, apply, position);
  }

  return steppedResult(velocity, batch, apply, position);
}

}  // namespace iggy3d
