#include "runtime/physics/PhysicsBroadphase.hpp"

#include <array>

namespace iggy3d {
namespace {

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1088
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

PhysicsBroadphaseResult broadphaseResult(PhysicsBroadphaseStatus status,
                                         bool ok) {
  PhysicsBroadphaseResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsBroadphaseStatusName(status);
  return result;
}

bool sameBody(const PhysicsAabbCollider& lhs,
              const PhysicsAabbCollider& rhs) {
  return lhs.bodyId.value == rhs.bodyId.value;
}

PhysicsBroadphasePair makePair(const PhysicsAabbCollider& lhs,
                               const PhysicsAabbCollider& rhs,
                               std::size_t lhsIndex,
                               std::size_t rhsIndex) {
  PhysicsBroadphasePair pair;
  pair.firstBodyId = lhs.bodyId;
  pair.secondBodyId = rhs.bodyId;
  pair.firstColliderIndex = lhsIndex;
  pair.secondColliderIndex = rhsIndex;
  pair.includesSensor = lhs.sensor || rhs.sensor;
  return pair;
}

}  // namespace

std::string_view physicsBroadphaseStatusName(PhysicsBroadphaseStatus status) {
  static constexpr std::array<std::string_view, 3> kNames{
      "physics_broadphase_pairs_collected",
      "physics_broadphase_missing_colliders",
      "physics_broadphase_invalid_collider",
  };
  return enumName(status, kNames, "physics_broadphase_invalid_collider");
}

PhysicsBroadphaseResult collectPhysicsBroadphasePairs(
    const PhysicsBroadphaseRequest& request) {
  // branch-gate: BG-1088
  if (request.colliders == nullptr) {
    return broadphaseResult(PhysicsBroadphaseStatus::MissingColliders, false);
  }

  PhysicsBroadphaseResult result =
      broadphaseResult(PhysicsBroadphaseStatus::PairsCollected, true);
  result.colliderCount = request.colliders->size();

  for (std::size_t index = 0U; index < request.colliders->size(); ++index) {
    // branch-gate: BG-1088
    if (!isValidPhysicsAabbCollider((*request.colliders)[index])) {
      result.ok = false;
      result.status = PhysicsBroadphaseStatus::InvalidCollider;
      result.reasonCode = physicsBroadphaseStatusName(result.status);
      result.invalidColliderIndex = index;
      result.pairs.clear();
      result.overlappingPairCount = 0U;
      result.testedPairCount = 0U;
      return result;
    }
  }

  for (std::size_t lhs = 0U; lhs < request.colliders->size(); ++lhs) {
    for (std::size_t rhs = lhs + 1U; rhs < request.colliders->size(); ++rhs) {
      ++result.testedPairCount;
      const PhysicsAabbCollider& first = (*request.colliders)[lhs];
      const PhysicsAabbCollider& second = (*request.colliders)[rhs];
      // branch-gate: BG-1088
      if (sameBody(first, second)) {
        continue;
      }
      // branch-gate: BG-1088
      if (!physicsAabbOverlaps(first, second)) {
        continue;
      }
      result.pairs.push_back(makePair(first, second, lhs, rhs));
    }
  }

  result.overlappingPairCount = result.pairs.size();
  return result;
}

}  // namespace iggy3d
