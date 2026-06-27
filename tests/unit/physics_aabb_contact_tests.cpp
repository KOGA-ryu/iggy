#include "runtime/physics/PhysicsAabbContact.hpp"

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

iggy3d::PhysicsBroadphasePair pair(std::uint32_t firstBody,
                                   std::uint32_t secondBody,
                                   std::size_t firstIndex = 0U,
                                   std::size_t secondIndex = 1U,
                                   bool includesSensor = false) {
  iggy3d::PhysicsBroadphasePair result;
  result.firstBodyId = {firstBody};
  result.secondBodyId = {secondBody};
  result.firstColliderIndex = firstIndex;
  result.secondColliderIndex = secondIndex;
  result.includesSensor = includesSensor;
  return result;
}

bool stableNamesAreLowerSnake() {
  return expect(iggy3d::physicsAabbContactStatusName(
                    iggy3d::PhysicsAabbContactStatus::ContactGenerated) ==
                    "physics_aabb_contact_generated",
                "generated status") &&
         expect(iggy3d::physicsAabbContactStatusName(
                    iggy3d::PhysicsAabbContactStatus::PairBodyMismatch) ==
                    "physics_aabb_contact_pair_body_mismatch",
                "mismatch status") &&
         expect(iggy3d::physicsAabbContactStatusName(
                    iggy3d::PhysicsAabbContactStatus::NoOverlap) ==
                    "physics_aabb_contact_no_overlap",
                "no overlap status");
}

bool missingInputsReject() {
  std::vector<iggy3d::PhysicsAabbCollider> colliders;
  const iggy3d::PhysicsBroadphasePair candidate = pair(1U, 2U);

  const iggy3d::PhysicsAabbContactResult missingColliders =
      iggy3d::generatePhysicsAabbContact({nullptr, &candidate});
  const iggy3d::PhysicsAabbContactResult missingPair =
      iggy3d::generatePhysicsAabbContact({&colliders, nullptr});

  return expect(!missingColliders.ok, "missing colliders rejected") &&
         expect(missingColliders.reasonCode ==
                    "physics_aabb_contact_missing_colliders",
                "missing colliders reason") &&
         expect(!missingPair.ok, "missing pair rejected") &&
         expect(missingPair.reasonCode == "physics_aabb_contact_missing_pair",
                "missing pair reason");
}

bool invalidPairFactsReject() {
  std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {}, {1.0F, 1.0F, 1.0F}),
      colliderAt({2U}, {}, {1.0F, 1.0F, 1.0F})};
  const iggy3d::PhysicsBroadphasePair outOfRange = pair(1U, 2U, 0U, 2U);
  const iggy3d::PhysicsBroadphasePair mismatch = pair(2U, 1U);
  const iggy3d::PhysicsBroadphasePair same = pair(1U, 1U);

  const iggy3d::PhysicsAabbContactResult range =
      iggy3d::generatePhysicsAabbContact({&colliders, &outOfRange});
  const iggy3d::PhysicsAabbContactResult bodyMismatch =
      iggy3d::generatePhysicsAabbContact({&colliders, &mismatch});
  colliders[1].bodyId = {1U};
  const iggy3d::PhysicsAabbContactResult sameBody =
      iggy3d::generatePhysicsAabbContact({&colliders, &same});

  return expect(!range.ok, "range rejected") &&
         expect(range.reasonCode ==
                    "physics_aabb_contact_pair_index_out_of_range",
                "range reason") &&
         expect(!bodyMismatch.ok, "mismatch rejected") &&
         expect(bodyMismatch.reasonCode ==
                    "physics_aabb_contact_pair_body_mismatch",
                "mismatch reason") &&
         expect(!sameBody.ok, "same body rejected") &&
         expect(sameBody.reasonCode == "physics_aabb_contact_same_body_pair",
                "same body reason");
}

bool invalidColliderRejectsWithIndex() {
  std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {}, {1.0F, 1.0F, 1.0F}),
      colliderAt({2U}, {}, {0.0F, 1.0F, 1.0F})};
  const iggy3d::PhysicsBroadphasePair candidate = pair(1U, 2U);

  const iggy3d::PhysicsAabbContactResult result =
      iggy3d::generatePhysicsAabbContact({&colliders, &candidate});

  return expect(!result.ok, "invalid collider rejected") &&
         expect(result.reasonCode == "physics_aabb_contact_invalid_collider",
                "invalid reason") &&
         expect(result.invalidColliderIndex == 1U, "invalid index");
}

bool separatedPairRejectsAsNoOverlap() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}),
      colliderAt({2U}, {3.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F})};
  const iggy3d::PhysicsBroadphasePair candidate = pair(1U, 2U);

  const iggy3d::PhysicsAabbContactResult result =
      iggy3d::generatePhysicsAabbContact({&colliders, &candidate});

  return expect(!result.ok, "separated rejected") &&
         expect(result.reasonCode == "physics_aabb_contact_no_overlap",
                "separated reason");
}

bool contactUsesShallowestAxisAndPositiveNormal() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}),
      colliderAt({2U}, {1.5F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F})};
  const iggy3d::PhysicsBroadphasePair candidate = pair(1U, 2U);

  const iggy3d::PhysicsAabbContactResult result =
      iggy3d::generatePhysicsAabbContact({&colliders, &candidate});

  return expect(result.ok, "contact ok") &&
         expect(result.reasonCode == "physics_aabb_contact_generated",
                "contact reason") &&
         expect(result.contact.firstBodyId.value == 1U, "contact first id") &&
         expect(result.contact.secondBodyId.value == 2U, "contact second id") &&
         expect(result.contact.firstColliderIndex == 0U, "contact first index") &&
         expect(result.contact.secondColliderIndex == 1U,
                "contact second index") &&
         expect(result.contact.penetrationMeters == 0.5F,
                "contact penetration") &&
         expect(iggy3d::nearlyEqual(result.contact.normalFromFirstToSecond,
                                    {1.0F, 0.0F, 0.0F}),
                "contact normal") &&
         expect(iggy3d::nearlyEqual(result.contact.pointMeters,
                                    {0.75F, 0.0F, 0.0F}),
                "contact point") &&
         expect(!result.contact.includesSensor, "contact sensor false");
}

bool contactNormalPointsTowardSecondBodyForNegativeAxis() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}),
      colliderAt({2U}, {-1.5F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F})};
  const iggy3d::PhysicsBroadphasePair candidate = pair(1U, 2U);

  const iggy3d::PhysicsAabbContactResult result =
      iggy3d::generatePhysicsAabbContact({&colliders, &candidate});

  return expect(result.ok, "negative contact ok") &&
         expect(iggy3d::nearlyEqual(result.contact.normalFromFirstToSecond,
                                    {-1.0F, 0.0F, 0.0F}),
                "negative normal") &&
         expect(iggy3d::nearlyEqual(result.contact.pointMeters,
                                    {-0.75F, 0.0F, 0.0F}),
                "negative point");
}

bool tiesPreferXAxisAndSensorIsPropagated() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}),
      colliderAt({2U}, {0.5F, 0.5F, 0.0F}, {1.0F, 1.0F, 1.0F}, true)};
  const iggy3d::PhysicsBroadphasePair candidate = pair(1U, 2U, 0U, 1U, false);

  const iggy3d::PhysicsAabbContactResult result =
      iggy3d::generatePhysicsAabbContact({&colliders, &candidate});

  return expect(result.ok, "tie contact ok") &&
         expect(result.contact.penetrationMeters == 1.5F,
                "tie penetration") &&
         expect(iggy3d::nearlyEqual(result.contact.normalFromFirstToSecond,
                                    {1.0F, 0.0F, 0.0F}),
                "tie x normal") &&
         expect(result.contact.includesSensor, "sensor propagated");
}

bool touchingAabbsGenerateZeroPenetrationContact() {
  const std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}),
      colliderAt({2U}, {2.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F})};
  const iggy3d::PhysicsBroadphasePair candidate = pair(1U, 2U);

  const iggy3d::PhysicsAabbContactResult result =
      iggy3d::generatePhysicsAabbContact({&colliders, &candidate});

  return expect(result.ok, "touching contact ok") &&
         expect(result.contact.penetrationMeters == 0.0F,
                "touching penetration") &&
         expect(iggy3d::nearlyEqual(result.contact.normalFromFirstToSecond,
                                    {1.0F, 0.0F, 0.0F}),
                "touching normal") &&
         expect(iggy3d::nearlyEqual(result.contact.pointMeters,
                                    {1.0F, 0.0F, 0.0F}),
                "touching point");
}

}  // namespace

int main() {
  const bool ok = stableNamesAreLowerSnake() &&
                  missingInputsReject() &&
                  invalidPairFactsReject() &&
                  invalidColliderRejectsWithIndex() &&
                  separatedPairRejectsAsNoOverlap() &&
                  contactUsesShallowestAxisAndPositiveNormal() &&
                  contactNormalPointsTowardSecondBodyForNegativeAxis() &&
                  tiesPreferXAxisAndSensorIsPropagated() &&
                  touchingAabbsGenerateZeroPenetrationContact();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
