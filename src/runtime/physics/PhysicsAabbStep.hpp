#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "runtime/physics/PhysicsAabbCollisionBatch.hpp"
#include "runtime/physics/PhysicsStep.hpp"

namespace iggy3d {

enum class PhysicsAabbStepStatus : std::uint8_t {
  Stepped,
  VelocityPhaseFailed,
  CollisionBatchFailed,
  DeltaApplyFailed,
  PositionPhaseFailed,
  InvalidConfig,
  MissingBodyStore,
};

struct PhysicsAabbStepConfig {
  PhysicsStepConfig stepConfig;
  PhysicsAabbCollisionBatchConfig collisionConfig;
  PhysicsBodyDeltaApplyConfig deltaApplyConfig;
};

struct PhysicsAabbStepRequest {
  PhysicsBodyStore* bodies = nullptr;
  const PhysicsShapeStore* shapes = nullptr;
  const PhysicsMaterialTable* materials = nullptr;
  const PhysicsBodyShapeBindingStore* bindings = nullptr;
  PhysicsAabbStepConfig config;
};

struct PhysicsAabbStepResult {
  bool ok = false;
  PhysicsAabbStepStatus status = PhysicsAabbStepStatus::MissingBodyStore;
  std::string_view reasonCode = "physics_aabb_step_missing_body_store";
  std::string_view upstreamReasonCode;
  PhysicsStepResult velocityPhase;
  PhysicsAabbCollisionBatchResult collisionBatch;
  PhysicsBodyDeltaApplyResult deltaApply;
  PhysicsStepResult positionPhase;
  std::size_t bodyCount = 0U;
  std::size_t colliderCount = 0U;
  std::size_t broadphasePairCount = 0U;
  std::size_t contactCount = 0U;
  std::size_t solvePlanCount = 0U;
  std::size_t accumulatedPlanCount = 0U;
  std::size_t appliedPositionCount = 0U;
  std::size_t appliedVelocityCount = 0U;
};

std::string_view physicsAabbStepStatusName(PhysicsAabbStepStatus status);
bool isValidPhysicsAabbStepConfig(const PhysicsAabbStepConfig& config);
PhysicsAabbStepResult stepPhysicsAabbWorld(
    const PhysicsAabbStepRequest& request);

}  // namespace iggy3d
