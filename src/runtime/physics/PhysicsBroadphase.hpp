#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "runtime/physics/PhysicsAabbCollider.hpp"

namespace iggy3d {

enum class PhysicsBroadphaseStatus : std::uint8_t {
  PairsCollected,
  MissingColliders,
  InvalidCollider,
  InvalidGridConfig,
};

struct PhysicsBroadphasePair {
  PhysicsBodyId firstBodyId;
  PhysicsBodyId secondBodyId;
  std::size_t firstColliderIndex = 0U;
  std::size_t secondColliderIndex = 0U;
  bool includesSensor = false;
};

struct PhysicsBroadphaseRequest {
  const std::vector<PhysicsAabbCollider>* colliders = nullptr;
  float cellSizeMeters = 1.0F;
};

struct PhysicsBroadphaseResult {
  bool ok = false;
  PhysicsBroadphaseStatus status = PhysicsBroadphaseStatus::MissingColliders;
  std::string_view reasonCode = "physics_broadphase_missing_colliders";
  std::size_t colliderCount = 0U;
  std::size_t occupiedCellCount = 0U;
  std::size_t cellEntryCount = 0U;
  std::size_t maxBucketSize = 0U;
  std::size_t candidatePairCount = 0U;
  std::size_t testedPairCount = 0U;
  std::size_t duplicatePairRejectedCount = 0U;
  std::size_t overlappingPairCount = 0U;
  std::size_t invalidColliderIndex = 0U;
  std::vector<PhysicsBroadphasePair> pairs;
};

std::string_view physicsBroadphaseStatusName(PhysicsBroadphaseStatus status);

PhysicsBroadphaseResult collectPhysicsBroadphasePairs(
    const PhysicsBroadphaseRequest& request);

}  // namespace iggy3d
