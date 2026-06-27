#include "runtime/physics/PhysicsAabbCollisionBatch.hpp"

#include <array>
#include <cmath>
#include <utility>

namespace iggy3d {
namespace {

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1095
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

PhysicsAabbCollisionBatchResult batchResult(
    PhysicsAabbCollisionBatchStatus status,
    bool ok) {
  PhysicsAabbCollisionBatchResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsAabbCollisionBatchStatusName(status);
  return result;
}

bool validBroadphaseCellSize(float cellSizeMeters) {
  return std::isfinite(cellSizeMeters) && cellSizeMeters > 0.0F;
}

PhysicsAabbCollisionBatchResult invalidRequestResult(
    std::string_view upstreamReasonCode) {
  PhysicsAabbCollisionBatchResult result =
      batchResult(PhysicsAabbCollisionBatchStatus::InvalidRequest, false);
  result.upstreamReasonCode = upstreamReasonCode;
  return result;
}

PhysicsAabbCollisionBatchResult missingBodyStoreResult(
    std::string_view upstreamReasonCode) {
  PhysicsAabbCollisionBatchResult result =
      batchResult(PhysicsAabbCollisionBatchStatus::MissingBodyStore, false);
  result.upstreamReasonCode = upstreamReasonCode;
  return result;
}

PhysicsAabbCollisionBatchResult bakeFailedResult(
    const PhysicsAabbColliderBakeResult& bake) {
  PhysicsAabbCollisionBatchResult result =
      batchResult(PhysicsAabbCollisionBatchStatus::BakeFailed, false);
  result.upstreamReasonCode = bake.reasonCode;
  result.bindingCount = bake.bindingCount;
  result.invalidBindingIndex = bake.invalidBindingIndex;
  result.invalidBodyId = bake.invalidBodyId;
  result.invalidShapeId = bake.invalidShapeId;
  result.invalidMaterialId = bake.invalidMaterialId;
  return result;
}

PhysicsAabbCollisionBatchResult broadphaseFailedResult(
    const PhysicsBroadphaseResult& broadphase,
    std::size_t bindingCount,
    std::size_t colliderCount) {
  PhysicsAabbCollisionBatchResult result =
      batchResult(PhysicsAabbCollisionBatchStatus::BroadphaseFailed, false);
  result.upstreamReasonCode = broadphase.reasonCode;
  result.bindingCount = bindingCount;
  result.colliderCount = colliderCount;
  result.invalidColliderIndex = broadphase.invalidColliderIndex;
  return result;
}

PhysicsAabbCollisionBatchResult contactFailedResult(
    const PhysicsAabbContactResult& contact,
    std::size_t pairIndex,
    std::size_t bindingCount,
    std::size_t colliderCount,
    std::size_t pairCount) {
  PhysicsAabbCollisionBatchResult result =
      batchResult(PhysicsAabbCollisionBatchStatus::ContactFailed, false);
  result.upstreamReasonCode = contact.reasonCode;
  result.bindingCount = bindingCount;
  result.colliderCount = colliderCount;
  result.broadphasePairCount = pairCount;
  result.invalidPairIndex = pairIndex;
  result.invalidColliderIndex = contact.invalidColliderIndex;
  return result;
}

PhysicsAabbCollisionBatchResult materialLookupFailedResult(
    const PhysicsMaterialTableResult& material,
    std::size_t pairIndex,
    std::size_t bindingCount,
    std::size_t colliderCount,
    std::size_t pairCount,
    PhysicsMaterialId materialId) {
  PhysicsAabbCollisionBatchResult result =
      batchResult(PhysicsAabbCollisionBatchStatus::MaterialLookupFailed,
                  false);
  result.upstreamReasonCode = material.reasonCode;
  result.bindingCount = bindingCount;
  result.colliderCount = colliderCount;
  result.broadphasePairCount = pairCount;
  result.invalidPairIndex = pairIndex;
  result.invalidMaterialId = materialId;
  return result;
}

PhysicsAabbCollisionBatchResult solveFailedResult(
    const PhysicsAabbContactSolvePlan& plan,
    std::size_t pairIndex,
    std::size_t bindingCount,
    std::size_t colliderCount,
    std::size_t pairCount,
    std::size_t contactCount) {
  PhysicsAabbCollisionBatchResult result =
      batchResult(PhysicsAabbCollisionBatchStatus::SolveFailed, false);
  result.upstreamReasonCode = plan.reasonCode;
  result.bindingCount = bindingCount;
  result.colliderCount = colliderCount;
  result.broadphasePairCount = pairCount;
  result.contactCount = contactCount;
  result.invalidPairIndex = pairIndex;
  result.invalidBodyId = plan.firstBodyId;
  return result;
}

PhysicsAabbCollisionBatchResult accumulateFailedResult(
    const PhysicsBodyDeltaAccumulatorResult& accumulated,
    std::size_t pairIndex,
    std::size_t bindingCount,
    std::size_t colliderCount,
    std::size_t pairCount,
    std::size_t contactCount,
    std::size_t solvePlanCount) {
  PhysicsAabbCollisionBatchResult result =
      batchResult(PhysicsAabbCollisionBatchStatus::AccumulateFailed, false);
  result.upstreamReasonCode = accumulated.reasonCode;
  result.bindingCount = bindingCount;
  result.colliderCount = colliderCount;
  result.broadphasePairCount = pairCount;
  result.contactCount = contactCount;
  result.solvePlanCount = solvePlanCount;
  result.invalidPairIndex = pairIndex;
  result.invalidBodyId = accumulated.invalidBodyId;
  return result;
}

bool noEffectiveMassPlan(const PhysicsAabbContactSolvePlan& plan) {
  return plan.status == PhysicsAabbContactSolveStatus::NoEffectiveMass;
}

bool hardSolveFailure(const PhysicsAabbContactSolvePlan& plan) {
  return !plan.ok && !noEffectiveMassPlan(plan);
}

PhysicsAabbContactResult generateContactForPair(
    const std::vector<PhysicsAabbCollider>& colliders,
    const PhysicsBroadphasePair& pair) {
  PhysicsAabbContactRequest request;
  request.colliders = &colliders;
  request.pair = &pair;
  return generatePhysicsAabbContact(request);
}

PhysicsAabbContactSolvePlan solveContact(
    const PhysicsBodyStore& bodies,
    const PhysicsAabbContact& contact,
    const PhysicsMaterialPairTraits& materialPair,
    const PhysicsAabbContactSolveConfig& solveConfig) {
  const PhysicsBodyStoreResult first = bodies.read(contact.firstBodyId);
  const PhysicsBodyStoreResult second = bodies.read(contact.secondBodyId);
  PhysicsAabbContactSolveRequest request;
  request.contact = &contact;
  request.materialPair = &materialPair;
  request.config = solveConfig;
  // branch-gate: BG-1095
  if (first.ok) {
    request.firstBody = &first.body;
  }
  // branch-gate: BG-1095
  if (second.ok) {
    request.secondBody = &second.body;
  }
  return solvePhysicsAabbContact(request);
}

struct CollisionBatchPacket {
  std::vector<PhysicsAabbContact> contacts;
  std::vector<PhysicsMaterialPairTraits> materialPairs;
  std::vector<PhysicsAabbContactSolvePlan> solvePlans;
};

void appendBatchRow(CollisionBatchPacket* packet,
                    const PhysicsAabbContact& contact,
                    const PhysicsMaterialPairTraits& materialPair,
                    const PhysicsAabbContactSolvePlan& solvePlan) {
  packet->contacts.push_back(contact);
  packet->materialPairs.push_back(materialPair);
  packet->solvePlans.push_back(solvePlan);
}

}  // namespace

std::string_view physicsAabbCollisionBatchStatusName(
    PhysicsAabbCollisionBatchStatus status) {
  static constexpr std::array<std::string_view, 9> kNames{
      "physics_aabb_collision_batch_batched",
      "physics_aabb_collision_batch_bake_failed",
      "physics_aabb_collision_batch_broadphase_failed",
      "physics_aabb_collision_batch_contact_failed",
      "physics_aabb_collision_batch_material_lookup_failed",
      "physics_aabb_collision_batch_solve_failed",
      "physics_aabb_collision_batch_accumulate_failed",
      "physics_aabb_collision_batch_missing_body_store",
      "physics_aabb_collision_batch_invalid_request",
  };
  return enumName(status, kNames,
                  "physics_aabb_collision_batch_invalid_request");
}

bool isValidPhysicsAabbCollisionBatchConfig(
    const PhysicsAabbCollisionBatchConfig& config) {
  return validBroadphaseCellSize(config.broadphaseCellSizeMeters) &&
         isValidPhysicsAabbContactSolveConfig(config.solveConfig);
}

PhysicsAabbCollisionBatchResult planPhysicsAabbCollisionBatch(
    const PhysicsAabbCollisionBatchRequest& request) {
  // branch-gate: BG-1095
  if (request.bodies == nullptr) {
    return missingBodyStoreResult("physics_body_delta_missing_store");
  }
  // branch-gate: BG-1095
  if (!validBroadphaseCellSize(request.config.broadphaseCellSizeMeters)) {
    return invalidRequestResult("physics_broadphase_invalid_grid_config");
  }
  // branch-gate: BG-1095
  if (!isValidPhysicsAabbContactSolveConfig(request.config.solveConfig)) {
    return invalidRequestResult("physics_aabb_contact_solve_invalid_config");
  }

  PhysicsBodyDeltaAccumulatorResult accumulatorResult =
      buildPhysicsBodyDeltaAccumulator(request.bodies);
  // branch-gate: BG-1095
  if (!accumulatorResult.ok) {
    return missingBodyStoreResult(accumulatorResult.reasonCode);
  }

  PhysicsAabbColliderBakeRequest bakeRequest;
  bakeRequest.bodies = request.bodies;
  bakeRequest.shapes = request.shapes;
  bakeRequest.materials = request.materials;
  bakeRequest.bindings = request.bindings;
  const PhysicsAabbColliderBakeResult bake =
      bakePhysicsAabbColliders(bakeRequest);
  // branch-gate: BG-1095
  if (!bake.ok) {
    return bakeFailedResult(bake);
  }

  PhysicsBroadphaseRequest broadphaseRequest;
  broadphaseRequest.colliders = &bake.colliders;
  broadphaseRequest.cellSizeMeters = request.config.broadphaseCellSizeMeters;
  const PhysicsBroadphaseResult broadphase =
      collectPhysicsBroadphasePairs(broadphaseRequest);
  // branch-gate: BG-1095
  if (!broadphase.ok) {
    return broadphaseFailedResult(broadphase, bake.bindingCount,
                                  bake.colliderCount);
  }

  CollisionBatchPacket packet;
  PhysicsBodyDeltaAccumulator accumulator =
      std::move(accumulatorResult.accumulator);
  PhysicsAabbCollisionBatchResult counts =
      batchResult(PhysicsAabbCollisionBatchStatus::Batched, true);
  counts.bindingCount = bake.bindingCount;
  counts.colliderCount = bake.colliderCount;
  counts.broadphasePairCount = broadphase.pairs.size();

  for (std::size_t pairIndex = 0U; pairIndex < broadphase.pairs.size();
       ++pairIndex) {
    const PhysicsBroadphasePair& pair = broadphase.pairs[pairIndex];
    const PhysicsAabbContactResult contact =
        generateContactForPair(bake.colliders, pair);
    // branch-gate: BG-1095
    if (!contact.ok) {
      return contactFailedResult(contact, pairIndex, bake.bindingCount,
                                 bake.colliderCount, broadphase.pairs.size());
    }

    const PhysicsMaterialId firstMaterialId =
        bake.materialIds[pair.firstColliderIndex];
    const PhysicsMaterialId secondMaterialId =
        bake.materialIds[pair.secondColliderIndex];
    const PhysicsMaterialTableResult firstMaterial =
        readPhysicsMaterial(request.materials, firstMaterialId);
    // branch-gate: BG-1095
    if (!firstMaterial.ok) {
      return materialLookupFailedResult(firstMaterial, pairIndex,
                                        bake.bindingCount, bake.colliderCount,
                                        broadphase.pairs.size(),
                                        firstMaterialId);
    }
    const PhysicsMaterialTableResult secondMaterial =
        readPhysicsMaterial(request.materials, secondMaterialId);
    // branch-gate: BG-1095
    if (!secondMaterial.ok) {
      return materialLookupFailedResult(secondMaterial, pairIndex,
                                        bake.bindingCount, bake.colliderCount,
                                        broadphase.pairs.size(),
                                        secondMaterialId);
    }

    const PhysicsMaterialPairTraits materialPair =
        combinePhysicsMaterialPair(firstMaterial.material,
                                   secondMaterial.material);
    const PhysicsAabbContactSolvePlan solvePlan = solveContact(
        *request.bodies, contact.contact, materialPair,
        request.config.solveConfig);
    // branch-gate: BG-1095
    if (hardSolveFailure(solvePlan)) {
      return solveFailedResult(solvePlan, pairIndex, bake.bindingCount,
                               bake.colliderCount, broadphase.pairs.size(),
                               packet.contacts.size() + 1U);
    }

    appendBatchRow(&packet, contact.contact, materialPair, solvePlan);
    // branch-gate: BG-1095
    if (contact.contact.includesSensor ||
        solvePlan.status == PhysicsAabbContactSolveStatus::SensorContact) {
      ++counts.sensorContactCount;
    }

    // branch-gate: BG-1095
    if (noEffectiveMassPlan(solvePlan)) {
      ++counts.skippedNoOpPlanCount;
      continue;
    }

    PhysicsBodyDeltaAccumulatorResult accumulated =
        accumulatePhysicsAabbContactSolvePlan(&accumulator, &solvePlan);
    // branch-gate: BG-1095
    if (!accumulated.ok) {
      return accumulateFailedResult(accumulated, pairIndex, bake.bindingCount,
                                    bake.colliderCount,
                                    broadphase.pairs.size(),
                                    packet.contacts.size(),
                                    packet.solvePlans.size());
    }
    // branch-gate: BG-1095
    if (accumulated.status == PhysicsBodyDeltaStatus::NoOp) {
      ++counts.skippedNoOpPlanCount;
    } else {
      counts.accumulatedPlanCount += accumulated.accumulatedPlanCount;
    }
  }

  counts.contactCount = packet.contacts.size();
  counts.solvePlanCount = packet.solvePlans.size();
  counts.colliders = bake.colliders;
  counts.colliderMaterialIds = bake.materialIds;
  counts.broadphasePairs = broadphase.pairs;
  counts.contacts = std::move(packet.contacts);
  counts.materialPairs = std::move(packet.materialPairs);
  counts.solvePlans = std::move(packet.solvePlans);
  counts.accumulator = std::move(accumulator);
  return counts;
}

}  // namespace iggy3d
