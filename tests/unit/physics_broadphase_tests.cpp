#include "runtime/physics/PhysicsBroadphase.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
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
                "invalid status") &&
         expect(iggy3d::physicsBroadphaseStatusName(
                    iggy3d::PhysicsBroadphaseStatus::InvalidGridConfig) ==
                    "physics_broadphase_invalid_grid_config",
                "invalid grid status");
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

bool invalidGridConfigRejectsBeforePairCollection() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {}, {1.0F, 1.0F, 1.0F}),
      colliderAt({2U}, {}, {1.0F, 1.0F, 1.0F})};

  iggy3d::PhysicsBroadphaseRequest zeroCellRequest;
  zeroCellRequest.colliders = &colliders;
  zeroCellRequest.cellSizeMeters = 0.0F;

  iggy3d::PhysicsBroadphaseRequest nonFiniteCellRequest;
  nonFiniteCellRequest.colliders = &colliders;
  nonFiniteCellRequest.cellSizeMeters =
      std::numeric_limits<float>::infinity();

  const iggy3d::PhysicsBroadphaseResult zeroResult =
      iggy3d::collectPhysicsBroadphasePairs(zeroCellRequest);
  const iggy3d::PhysicsBroadphaseResult nonFiniteResult =
      iggy3d::collectPhysicsBroadphasePairs(nonFiniteCellRequest);

  return expect(!zeroResult.ok, "zero cell rejected") &&
         expect(zeroResult.reasonCode ==
                    "physics_broadphase_invalid_grid_config",
                "zero cell reason") &&
         expect(zeroResult.pairs.empty(), "zero cell no pairs") &&
         expect(zeroResult.cellEntryCount == 0U, "zero cell no entries") &&
         expect(zeroResult.candidatePairCount == 0U,
                "zero cell no candidates") &&
         expect(zeroResult.testedPairCount == 0U, "zero cell no tests") &&
         expect(!nonFiniteResult.ok, "non-finite cell rejected") &&
         expect(nonFiniteResult.reasonCode ==
                    "physics_broadphase_invalid_grid_config",
                "non-finite cell reason") &&
         expect(nonFiniteResult.pairs.empty(), "non-finite cell no pairs") &&
         expect(nonFiniteResult.cellEntryCount == 0U,
                "non-finite cell no entries") &&
         expect(nonFiniteResult.candidatePairCount == 0U,
                "non-finite cell no candidates") &&
         expect(nonFiniteResult.testedPairCount == 0U,
                "non-finite cell no tests");
}

bool separatedCollidersInDifferentCellsAreNotCandidateTested() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {0.25F, 0.25F, 0.25F}),
      colliderAt({2U}, {10.0F, 0.0F, 0.0F}, {0.25F, 0.25F, 0.25F})};

  iggy3d::PhysicsBroadphaseRequest request;
  request.colliders = &colliders;
  request.cellSizeMeters = 1.0F;

  const iggy3d::PhysicsBroadphaseResult result =
      iggy3d::collectPhysicsBroadphasePairs(request);

  return expect(result.ok, "separated ok") &&
         expect(result.colliderCount == 2U, "separated collider count") &&
         expect(result.cellEntryCount == 16U, "separated cell entries") &&
         expect(result.occupiedCellCount == 16U, "separated occupied cells") &&
         expect(result.maxBucketSize == 1U, "separated max bucket") &&
         expect(result.candidatePairCount == 0U,
                "separated no raw candidates") &&
         expect(result.duplicatePairRejectedCount == 0U,
                "separated no duplicate candidates") &&
         expect(result.testedPairCount == 0U, "separated no tests") &&
         expect(result.overlappingPairCount == 0U, "separated no overlaps") &&
         expect(result.pairs.empty(), "separated no pairs");
}

bool spanningOverlapDeduplicatesToOnePair() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {1.25F, 1.25F, 1.25F}),
      colliderAt({2U}, {0.5F, 0.0F, 0.0F}, {1.25F, 1.25F, 1.25F})};

  iggy3d::PhysicsBroadphaseRequest request;
  request.colliders = &colliders;
  request.cellSizeMeters = 1.0F;

  const iggy3d::PhysicsBroadphaseResult result =
      iggy3d::collectPhysicsBroadphasePairs(request);

  return expect(result.ok, "spanning ok") &&
         expect(result.candidatePairCount > 1U,
                "spanning raw duplicate candidates") &&
         expect(result.duplicatePairRejectedCount > 0U,
                "spanning duplicate rejected") &&
         expect(result.testedPairCount == 1U, "spanning one test") &&
         expect(result.overlappingPairCount == 1U, "spanning one overlap") &&
         expect(result.pairs.size() == 1U, "spanning one pair") &&
         expect(result.pairs[0].firstColliderIndex == 0U,
                "spanning pair lhs") &&
         expect(result.pairs[0].secondColliderIndex == 1U,
                "spanning pair rhs");
}

bool overlappingPairsAreCollectedDeterministically() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}),
      colliderAt({2U}, {1.5F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}),
      colliderAt({3U}, {5.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}),
      colliderAt({4U}, {1.5F, 0.5F, 0.0F}, {0.5F, 0.5F, 0.5F}, true),
  };

  iggy3d::PhysicsBroadphaseRequest request;
  request.colliders = &colliders;
  request.cellSizeMeters = 10.0F;

  const iggy3d::PhysicsBroadphaseResult result =
      iggy3d::collectPhysicsBroadphasePairs(request);

  return expect(result.ok, "overlap collection ok") &&
         expect(result.reasonCode == "physics_broadphase_pairs_collected",
                "overlap reason") &&
         expect(result.colliderCount == 4U, "overlap collider count") &&
         expect(result.candidatePairCount > 6U,
                "overlap raw candidate count") &&
         expect(result.duplicatePairRejectedCount > 0U,
                "overlap duplicate candidate count") &&
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

bool candidateOrderingIsDeterministicByColliderIndex() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({10U}, {0.2F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F}),
      colliderAt({20U}, {0.0F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F}),
      colliderAt({30U}, {0.1F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F})};

  iggy3d::PhysicsBroadphaseRequest request;
  request.colliders = &colliders;
  request.cellSizeMeters = 10.0F;

  const iggy3d::PhysicsBroadphaseResult result =
      iggy3d::collectPhysicsBroadphasePairs(request);

  return expect(result.ok, "deterministic order ok") &&
         expect(result.testedPairCount == 3U, "deterministic tests") &&
         expect(result.pairs.size() == 3U, "deterministic pair count") &&
         expect(result.pairs[0].firstColliderIndex == 0U,
                "deterministic pair 0 lhs") &&
         expect(result.pairs[0].secondColliderIndex == 1U,
                "deterministic pair 0 rhs") &&
         expect(result.pairs[1].firstColliderIndex == 0U,
                "deterministic pair 1 lhs") &&
         expect(result.pairs[1].secondColliderIndex == 2U,
                "deterministic pair 1 rhs") &&
         expect(result.pairs[2].firstColliderIndex == 1U,
                "deterministic pair 2 lhs") &&
         expect(result.pairs[2].secondColliderIndex == 2U,
                "deterministic pair 2 rhs");
}

bool sameBodyColliderPairsAreSkipped() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}),
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}),
      colliderAt({2U}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F})};

  const iggy3d::PhysicsBroadphaseResult result =
      iggy3d::collectPhysicsBroadphasePairs({&colliders});

  return expect(result.ok, "same body ok") &&
         expect(result.candidatePairCount > 3U,
                "same body raw candidates include duplicates") &&
         expect(result.duplicatePairRejectedCount > 0U,
                "same body duplicate rejected") &&
         expect(result.testedPairCount == 2U, "same body tests") &&
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
                  invalidGridConfigRejectsBeforePairCollection() &&
                  separatedCollidersInDifferentCellsAreNotCandidateTested() &&
                  spanningOverlapDeduplicatesToOnePair() &&
                  overlappingPairsAreCollectedDeterministically() &&
                  candidateOrderingIsDeterministicByColliderIndex() &&
                  sameBodyColliderPairsAreSkipped();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
