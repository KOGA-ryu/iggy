#include "runtime/physics/PhysicsBroadphase.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::PhysicsAabbCollider colliderAt(iggy3d::PhysicsBodyId id,
                                       iggy3d::Vec3 center,
                                       iggy3d::Vec3 halfExtents,
                                       bool sensor = false) {
  iggy3d::PhysicsAabbCollider collider;
  collider.bodyId = id;
  collider.worldCenterMeters = center;
  collider.halfExtentsMeters = halfExtents;
  collider.bounds = iggy3d::aabbFromCenterExtents(center, halfExtents);
  collider.sensor = sensor;
  return collider;
}

bool stableNamesAreLowerSnake() {
  return expect(iggy3d::physicsBroadphaseStatusName(
                    iggy3d::PhysicsBroadphaseStatus::PairsCollected) ==
                    "physics_broadphase_pairs_collected",
                "pairs status") &&
         expect(iggy3d::physicsBroadphaseStatusName(
                    iggy3d::PhysicsBroadphaseStatus::MissingColliders) ==
                    "physics_broadphase_missing_colliders",
                "missing status") &&
         expect(iggy3d::physicsBroadphaseStatusName(
                    iggy3d::PhysicsBroadphaseStatus::InvalidCollider) ==
                    "physics_broadphase_invalid_collider",
                "invalid status");
}

bool missingColliderListRejects() {
  const iggy3d::PhysicsBroadphaseResult result =
      iggy3d::collectPhysicsBroadphasePairs({nullptr});

  return expect(!result.ok, "missing rejected") &&
         expect(result.reasonCode == "physics_broadphase_missing_colliders",
                "missing reason") &&
         expect(result.colliderCount == 0U, "missing collider count") &&
         expect(result.pairs.empty(), "missing pairs empty");
}

bool emptyAndSingleColliderListsAreValidNoPairResults() {
  const std::vector<iggy3d::PhysicsAabbCollider> empty;
  const std::vector<iggy3d::PhysicsAabbCollider> single{
      colliderAt({1U}, {}, {1.0F, 1.0F, 1.0F})};

  const iggy3d::PhysicsBroadphaseResult emptyResult =
      iggy3d::collectPhysicsBroadphasePairs({&empty});
  const iggy3d::PhysicsBroadphaseResult singleResult =
      iggy3d::collectPhysicsBroadphasePairs({&single});

  return expect(emptyResult.ok, "empty ok") &&
         expect(emptyResult.colliderCount == 0U, "empty count") &&
         expect(emptyResult.testedPairCount == 0U, "empty tested") &&
         expect(emptyResult.overlappingPairCount == 0U, "empty pairs") &&
         expect(singleResult.ok, "single ok") &&
         expect(singleResult.colliderCount == 1U, "single count") &&
         expect(singleResult.testedPairCount == 0U, "single tested") &&
         expect(singleResult.overlappingPairCount == 0U, "single pairs");
}

bool invalidColliderRejectsBeforePairCollection() {
  std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {}, {1.0F, 1.0F, 1.0F}),
      colliderAt({0U}, {}, {1.0F, 1.0F, 1.0F}),
      colliderAt({2U}, {}, {1.0F, 1.0F, 1.0F})};

  const iggy3d::PhysicsBroadphaseResult result =
      iggy3d::collectPhysicsBroadphasePairs({&colliders});

  return expect(!result.ok, "invalid rejected") &&
         expect(result.reasonCode == "physics_broadphase_invalid_collider",
                "invalid reason") &&
         expect(result.colliderCount == 3U, "invalid count") &&
         expect(result.invalidColliderIndex == 1U, "invalid index") &&
         expect(result.testedPairCount == 0U, "invalid no tests") &&
         expect(result.pairs.empty(), "invalid no pairs");
}

bool overlappingPairsAreCollectedDeterministically() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}),
      colliderAt({2U}, {1.5F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}),
      colliderAt({3U}, {5.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}),
      colliderAt({4U}, {1.5F, 0.5F, 0.0F}, {0.5F, 0.5F, 0.5F}, true),
  };

  const iggy3d::PhysicsBroadphaseResult result =
      iggy3d::collectPhysicsBroadphasePairs({&colliders});

  return expect(result.ok, "overlap collection ok") &&
         expect(result.reasonCode == "physics_broadphase_pairs_collected",
                "overlap reason") &&
         expect(result.colliderCount == 4U, "overlap collider count") &&
         expect(result.testedPairCount == 6U, "overlap tested count") &&
         expect(result.overlappingPairCount == 3U, "overlap pair count") &&
         expect(result.pairs.size() == 3U, "overlap vector count") &&
         expect(result.pairs[0].firstBodyId.value == 1U, "pair 0 first") &&
         expect(result.pairs[0].secondBodyId.value == 2U, "pair 0 second") &&
         expect(result.pairs[0].firstColliderIndex == 0U, "pair 0 lhs") &&
         expect(result.pairs[0].secondColliderIndex == 1U, "pair 0 rhs") &&
         expect(!result.pairs[0].includesSensor, "pair 0 sensor false") &&
         expect(result.pairs[1].firstBodyId.value == 1U, "pair 1 first") &&
         expect(result.pairs[1].secondBodyId.value == 4U, "pair 1 second") &&
         expect(result.pairs[1].includesSensor, "pair 1 sensor true") &&
         expect(result.pairs[2].firstBodyId.value == 2U, "pair 2 first") &&
         expect(result.pairs[2].secondBodyId.value == 4U, "pair 2 second") &&
         expect(result.pairs[2].includesSensor, "pair 2 sensor true");
}

bool sameBodyColliderPairsAreSkipped() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}),
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}),
      colliderAt({2U}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F})};

  const iggy3d::PhysicsBroadphaseResult result =
      iggy3d::collectPhysicsBroadphasePairs({&colliders});

  return expect(result.ok, "same body ok") &&
         expect(result.testedPairCount == 3U, "same body tests") &&
         expect(result.overlappingPairCount == 2U, "same body pair count") &&
         expect(result.pairs[0].firstColliderIndex == 0U,
                "same body pair 0 lhs") &&
         expect(result.pairs[0].secondColliderIndex == 2U,
                "same body pair 0 rhs") &&
         expect(result.pairs[1].firstColliderIndex == 1U,
                "same body pair 1 lhs") &&
         expect(result.pairs[1].secondColliderIndex == 2U,
                "same body pair 1 rhs");
}

}  // namespace

int main() {
  const bool ok = stableNamesAreLowerSnake() &&
                  missingColliderListRejects() &&
                  emptyAndSingleColliderListsAreValidNoPairResults() &&
                  invalidColliderRejectsBeforePairCollection() &&
                  overlappingPairsAreCollectedDeterministically() &&
                  sameBodyColliderPairsAreSkipped();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
