#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "runtime/physics/PhysicsAabbContact.hpp"
#include "runtime/physics/PhysicsBodyDeltaAccumulator.hpp"
#include "runtime/physics/PhysicsColliderBake.hpp"

namespace iggy3d {

enum class PhysicsAabbCollisionBatchStatus : std::uint8_t {
  Batched,
  BakeFailed,
  BroadphaseFailed,
  ContactFailed,
  MaterialLookupFailed,
  SolveFailed,
  AccumulateFailed,
  MissingBodyStore,
  InvalidRequest,
};

struct PhysicsAabbCollisionBatchConfig {
  float broadphaseCellSizeMeters = 1.0F;
  PhysicsAabbContactSolveConfig solveConfig;
};

struct PhysicsAabbCollisionBatchRequest {
  const PhysicsBodyStore* bodies = nullptr;
  const PhysicsShapeStore* shapes = nullptr;
  const PhysicsMaterialTable* materials = nullptr;
  const PhysicsBodyShapeBindingStore* bindings = nullptr;
  PhysicsAabbCollisionBatchConfig config;
};

struct PhysicsAabbCollisionBatchResult {
  bool ok = false;
  PhysicsAabbCollisionBatchStatus status =
      PhysicsAabbCollisionBatchStatus::InvalidRequest;
  std::string_view reasonCode = "physics_aabb_collision_batch_invalid_request";
  std::string_view upstreamReasonCode;
  std::size_t bindingCount = 0U;
  std::size_t colliderCount = 0U;
  std::size_t broadphasePairCount = 0U;
  std::size_t contactCount = 0U;
  std::size_t solvePlanCount = 0U;
  std::size_t accumulatedPlanCount = 0U;
  std::size_t sensorContactCount = 0U;
  std::size_t skippedNoOpPlanCount = 0U;
  std::size_t invalidPairIndex = 0U;
  std::size_t invalidColliderIndex = 0U;
  std::size_t invalidBindingIndex = 0U;
  PhysicsBodyId invalidBodyId;
  PhysicsMaterialId invalidMaterialId;
  PhysicsShapeId invalidShapeId;
  std::vector<PhysicsAabbCollider> colliders;
  std::vector<PhysicsMaterialId> colliderMaterialIds;
  std::vector<PhysicsBroadphasePair> broadphasePairs;
  std::vector<PhysicsAabbContact> contacts;
  std::vector<PhysicsMaterialPairTraits> materialPairs;
  std::vector<PhysicsAabbContactSolvePlan> solvePlans;
  PhysicsBodyDeltaAccumulator accumulator;
};

std::string_view physicsAabbCollisionBatchStatusName(
    PhysicsAabbCollisionBatchStatus status);
bool isValidPhysicsAabbCollisionBatchConfig(
    const PhysicsAabbCollisionBatchConfig& config);
PhysicsAabbCollisionBatchResult planPhysicsAabbCollisionBatch(
    const PhysicsAabbCollisionBatchRequest& request);

}  // namespace iggy3d
