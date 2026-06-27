#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "runtime/physics/PhysicsAabbContactSolver.hpp"
#include "runtime/physics/PhysicsBodyStore.hpp"

namespace iggy3d {

enum class PhysicsBodyDeltaStatus : std::uint8_t {
  AccumulatorBuilt,
  DeltaAccumulated,
  DeltasApplied,
  NoOp,
  MissingStore,
  MissingAccumulator,
  MissingPlan,
  InvalidStoreState,
  InvalidPlan,
  BodyNotFound,
  InvalidDelta,
  InvalidConfig,
};

struct PhysicsBodyDeltaApplyConfig {
  bool applyPositionDeltas = true;
  bool applyVelocityDeltas = true;
  float maxPositionDeltaMeters = 10.0F;
  float maxVelocityDeltaMetersPerSecond = 100.0F;
};

struct PhysicsBodyDeltaAccumulator {
  std::vector<PhysicsBodyId> bodyIds;
  std::vector<Vec3> positionDeltasMeters;
  std::vector<Vec3> velocityDeltasMetersPerSecond;
  std::vector<std::size_t> contributingPlanCounts;
};

struct PhysicsBodyDeltaAccumulatorResult {
  bool ok = false;
  PhysicsBodyDeltaStatus status = PhysicsBodyDeltaStatus::MissingAccumulator;
  std::string_view reasonCode = "physics_body_delta_missing_accumulator";
  std::size_t bodyCount = 0U;
  std::size_t accumulatedPlanCount = 0U;
  PhysicsBodyId invalidBodyId;
  std::size_t invalidBodyIndex = 0U;
  PhysicsBodyDeltaAccumulator accumulator;
};

struct PhysicsBodyDeltaApplyResult {
  bool ok = false;
  PhysicsBodyDeltaStatus status = PhysicsBodyDeltaStatus::MissingStore;
  std::string_view reasonCode = "physics_body_delta_missing_store";
  std::size_t bodyCount = 0U;
  std::size_t appliedPositionCount = 0U;
  std::size_t appliedVelocityCount = 0U;
  PhysicsBodyId invalidBodyId;
  std::size_t invalidBodyIndex = 0U;
};

std::string_view physicsBodyDeltaStatusName(PhysicsBodyDeltaStatus status);
bool isValidPhysicsBodyDeltaApplyConfig(
    const PhysicsBodyDeltaApplyConfig& config);
PhysicsBodyDeltaAccumulatorResult buildPhysicsBodyDeltaAccumulator(
    const PhysicsBodyStore* store);
PhysicsBodyDeltaAccumulatorResult accumulatePhysicsAabbContactSolvePlan(
    PhysicsBodyDeltaAccumulator* accumulator,
    const PhysicsAabbContactSolvePlan* plan);
PhysicsBodyDeltaApplyResult applyPhysicsBodyDeltas(
    PhysicsBodyStore* store,
    const PhysicsBodyDeltaAccumulator& accumulator,
    const PhysicsBodyDeltaApplyConfig& config);

}  // namespace iggy3d
