#include "runtime/physics/PhysicsCollisionQueries.hpp"

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

bool nearlyEqual(float lhs, float rhs, float epsilon = 0.0001F) {
  const float delta = lhs - rhs;
  return delta >= -epsilon && delta <= epsilon;
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

bool collidersUnchanged(
    const std::vector<iggy3d::PhysicsAabbCollider>& lhs,
    const std::vector<iggy3d::PhysicsAabbCollider>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < lhs.size(); ++index) {
    if (lhs[index].bodyId.value != rhs[index].bodyId.value ||
        !iggy3d::nearlyEqual(lhs[index].worldCenterMeters,
                             rhs[index].worldCenterMeters) ||
        !iggy3d::nearlyEqual(lhs[index].halfExtentsMeters,
                             rhs[index].halfExtentsMeters) ||
        !iggy3d::nearlyEqual(lhs[index].bounds.min, rhs[index].bounds.min) ||
        !iggy3d::nearlyEqual(lhs[index].bounds.max, rhs[index].bounds.max) ||
        lhs[index].sensor != rhs[index].sensor) {
      return false;
    }
  }
  return true;
}

bool statusNamesAreStable() {
  return expect(iggy3d::physicsCollisionQueryStatusName(
                    iggy3d::PhysicsCollisionQueryStatus::Queried) ==
                    "physics_collision_query_queried",
                "queried status") &&
         expect(iggy3d::physicsCollisionQueryStatusName(
                    iggy3d::PhysicsCollisionQueryStatus::MissingColliders) ==
                    "physics_collision_query_missing_colliders",
                "missing colliders status") &&
         expect(iggy3d::physicsCollisionQueryStatusName(
                    iggy3d::PhysicsCollisionQueryStatus::MissingQueryCollider) ==
                    "physics_collision_query_missing_query_collider",
                "missing query collider status") &&
         expect(iggy3d::physicsCollisionQueryStatusName(
                    iggy3d::PhysicsCollisionQueryStatus::InvalidCollider) ==
                    "physics_collision_query_invalid_collider",
                "invalid collider status") &&
         expect(iggy3d::physicsCollisionQueryStatusName(
                    iggy3d::PhysicsCollisionQueryStatus::InvalidQueryCollider) ==
                    "physics_collision_query_invalid_query_collider",
                "invalid query collider status") &&
         expect(iggy3d::physicsCollisionQueryStatusName(
                    iggy3d::PhysicsCollisionQueryStatus::InvalidRayOrigin) ==
                    "physics_collision_query_invalid_ray_origin",
                "invalid ray origin status") &&
         expect(iggy3d::physicsCollisionQueryStatusName(
                    iggy3d::PhysicsCollisionQueryStatus::InvalidRayDirection) ==
                    "physics_collision_query_invalid_ray_direction",
                "invalid ray direction status") &&
         expect(iggy3d::physicsCollisionQueryStatusName(
                    iggy3d::PhysicsCollisionQueryStatus::InvalidMaxDistance) ==
                    "physics_collision_query_invalid_max_distance",
                "invalid max distance status") &&
         expect(iggy3d::physicsCollisionQueryStatusName(
                    iggy3d::PhysicsCollisionQueryStatus::InvalidSweepDisplacement) ==
                    "physics_collision_query_invalid_sweep_displacement",
                "invalid sweep displacement status") &&
         expect(iggy3d::physicsCollisionQueryStatusName(
                    iggy3d::PhysicsCollisionQueryStatus::InvalidGroundDirection) ==
                    "physics_collision_query_invalid_ground_direction",
                "invalid ground direction status") &&
         expect(iggy3d::physicsCollisionQueryStatusName(
                    iggy3d::PhysicsCollisionQueryStatus::InvalidProbeDistance) ==
                    "physics_collision_query_invalid_probe_distance",
                "invalid probe distance status");
}

bool invalidRequestsRejectWithoutOutput() {
  std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {}, {0.5F, 0.5F, 0.5F}),
  };
  const iggy3d::PhysicsAabbCollider query =
      colliderAt({99U}, {}, {0.5F, 0.5F, 0.5F});
  iggy3d::PhysicsAabbCollider invalidCollider = colliders.front();
  invalidCollider.bodyId = {};
  std::vector<iggy3d::PhysicsAabbCollider> invalidColliders{
      invalidCollider,
  };
  iggy3d::PhysicsAabbCollider invalidQuery = query;
  invalidQuery.halfExtentsMeters.x = 0.0F;

  const iggy3d::PhysicsAabbOverlapQueryResult missingOverlap =
      iggy3d::queryPhysicsAabbOverlaps({});
  const iggy3d::PhysicsAabbOverlapQueryResult invalidOverlap =
      iggy3d::queryPhysicsAabbOverlaps({&invalidColliders, &query, false});
  const iggy3d::PhysicsAabbOverlapQueryResult invalidQueryOverlap =
      iggy3d::queryPhysicsAabbOverlaps({&colliders, &invalidQuery, false});

  iggy3d::PhysicsRaycastQueryRequest invalidRay;
  invalidRay.colliders = &colliders;
  invalidRay.direction = {};
  invalidRay.maxDistanceMeters = 10.0F;
  iggy3d::PhysicsRaycastQueryRequest invalidDistance = invalidRay;
  invalidDistance.direction = {1.0F, 0.0F, 0.0F};
  invalidDistance.maxDistanceMeters = 0.0F;
  const iggy3d::PhysicsRaycastQueryResult rayDirection =
      iggy3d::raycastPhysicsAabbs(invalidRay);
  const iggy3d::PhysicsRaycastQueryResult rayDistance =
      iggy3d::raycastPhysicsAabbs(invalidDistance);

  iggy3d::PhysicsSweptAabbQueryRequest invalidSweep;
  invalidSweep.colliders = &colliders;
  invalidSweep.movingCollider = &query;
  invalidSweep.displacementMeters = {
      std::numeric_limits<float>::infinity(), 0.0F, 0.0F};
  const iggy3d::PhysicsSweptAabbQueryResult sweep =
      iggy3d::sweepPhysicsAabb(invalidSweep);

  iggy3d::PhysicsGroundCheckQueryRequest invalidGround;
  invalidGround.colliders = &colliders;
  invalidGround.movingCollider = &query;
  invalidGround.downDirection = {};
  invalidGround.probeDistanceMeters = 1.0F;
  iggy3d::PhysicsGroundCheckQueryRequest invalidProbe = invalidGround;
  invalidProbe.downDirection = {0.0F, -1.0F, 0.0F};
  invalidProbe.probeDistanceMeters = 0.0F;
  const iggy3d::PhysicsGroundCheckQueryResult groundDirection =
      iggy3d::checkPhysicsGround(invalidGround);
  const iggy3d::PhysicsGroundCheckQueryResult groundProbe =
      iggy3d::checkPhysicsGround(invalidProbe);

  return expect(!missingOverlap.ok, "missing overlap colliders rejected") &&
         expect(missingOverlap.reasonCode ==
                    "physics_collision_query_missing_colliders",
                "missing overlap reason") &&
         expect(!invalidOverlap.ok, "invalid overlap collider rejected") &&
         expect(invalidOverlap.reasonCode ==
                    "physics_collision_query_invalid_collider",
                "invalid overlap reason") &&
         expect(invalidOverlap.invalidColliderIndex == 0U,
                "invalid overlap index") &&
         expect(!invalidQueryOverlap.ok, "invalid query overlap rejected") &&
         expect(invalidQueryOverlap.reasonCode ==
                    "physics_collision_query_invalid_query_collider",
                "invalid query overlap reason") &&
         expect(!rayDirection.ok, "invalid ray direction rejected") &&
         expect(rayDirection.reasonCode ==
                    "physics_collision_query_invalid_ray_direction",
                "invalid ray direction reason") &&
         expect(!rayDistance.ok, "invalid ray distance rejected") &&
         expect(rayDistance.reasonCode ==
                    "physics_collision_query_invalid_max_distance",
                "invalid ray distance reason") &&
         expect(!sweep.ok, "invalid sweep rejected") &&
         expect(sweep.reasonCode ==
                    "physics_collision_query_invalid_sweep_displacement",
                "invalid sweep reason") &&
         expect(!groundDirection.ok, "invalid ground direction rejected") &&
         expect(groundDirection.reasonCode ==
                    "physics_collision_query_invalid_ground_direction",
                "invalid ground direction reason") &&
         expect(!groundProbe.ok, "invalid probe rejected") &&
         expect(groundProbe.reasonCode ==
                    "physics_collision_query_invalid_probe_distance",
                "invalid probe reason") &&
         expect(missingOverlap.hitCount == 0U, "invalid overlap no hits") &&
         expect(rayDirection.hitCount == 0U, "invalid ray no hits") &&
         expect(sweep.hitCount == 0U, "invalid sweep no hits") &&
         expect(groundDirection.hitCount == 0U, "invalid ground no hits");
}

bool overlapFindsDeterministicHitsAndSensorPolicy() {
  std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {4.0F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F}),
      colliderAt({2U}, {}, {0.5F, 0.5F, 0.5F}),
      colliderAt({3U}, {0.25F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F}, true),
  };
  const std::vector<iggy3d::PhysicsAabbCollider> before = colliders;
  const iggy3d::PhysicsAabbCollider query =
      colliderAt({99U}, {}, {0.75F, 0.75F, 0.75F});

  const iggy3d::PhysicsAabbOverlapQueryResult withoutSensors =
      iggy3d::queryPhysicsAabbOverlaps({&colliders, &query, false});
  const iggy3d::PhysicsAabbOverlapQueryResult withSensors =
      iggy3d::queryPhysicsAabbOverlaps({&colliders, &query, true});

  return expect(withoutSensors.ok, "overlap query ok") &&
         expect(withoutSensors.hitCount == 1U, "sensor skipped hit count") &&
         expect(withoutSensors.colliderIndices[0] == 1U,
                "sensor skipped hit index") &&
         expect(withoutSensors.bodyIds[0].value == 2U,
                "sensor skipped body id") &&
         expect(withSensors.ok, "overlap include sensors ok") &&
         expect(withSensors.hitCount == 2U, "sensor included hit count") &&
         expect(withSensors.colliderIndices[0] == 1U,
                "sensor included first index") &&
         expect(withSensors.colliderIndices[1] == 2U,
                "sensor included second index") &&
         expect(withSensors.sensors[1], "sensor flag preserved") &&
         expect(collidersUnchanged(colliders, before),
                "overlap query does not mutate colliders");
}

bool raycastHitsSortMissAndStartInsidePolicy() {
  std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {5.0F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F}),
      colliderAt({2U}, {2.0F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F}),
      colliderAt({3U}, {3.0F, 2.0F, 0.0F}, {0.5F, 0.5F, 0.5F}),
  };
  const std::vector<iggy3d::PhysicsAabbCollider> before = colliders;
  iggy3d::PhysicsRaycastQueryRequest request;
  request.colliders = &colliders;
  request.originMeters = {};
  request.direction = {1.0F, 0.0F, 0.0F};
  request.maxDistanceMeters = 10.0F;
  const iggy3d::PhysicsRaycastQueryResult hit =
      iggy3d::raycastPhysicsAabbs(request);

  request.direction = {0.0F, 1.0F, 0.0F};
  const iggy3d::PhysicsRaycastQueryResult miss =
      iggy3d::raycastPhysicsAabbs(request);

  request.direction = {1.0F, 0.0F, 0.0F};
  request.originMeters = {2.0F, 0.0F, 0.0F};
  const iggy3d::PhysicsRaycastQueryResult inside =
      iggy3d::raycastPhysicsAabbs(request);

  return expect(hit.ok, "raycast hit ok") &&
         expect(hit.hitCount == 2U, "raycast sorted hit count") &&
         expect(hit.hits[0].colliderIndex == 1U,
                "raycast nearest index") &&
         expect(iggy3d::nearlyEqual(hit.hits[0].pointMeters,
                                    {1.5F, 0.0F, 0.0F}),
                "raycast nearest point") &&
         expect(iggy3d::nearlyEqual(hit.hits[0].normalFromColliderToRay,
                                    {-1.0F, 0.0F, 0.0F}),
                "raycast nearest normal") &&
         expect(hit.hits[1].colliderIndex == 0U,
                "raycast farther index") &&
         expect(miss.ok, "raycast miss ok") &&
         expect(miss.hitCount == 0U, "raycast miss count") &&
         expect(inside.ok, "raycast start inside ok") &&
         expect(inside.hitCount == 2U, "raycast start inside hit count") &&
         expect(inside.hits[0].colliderIndex == 1U,
                "raycast start inside index") &&
         expect(inside.hits[0].distanceMeters == 0.0F,
                "raycast start inside distance") &&
         expect(inside.hits[0].startInside,
                "raycast start inside flag") &&
         expect(iggy3d::nearlyEqual(inside.hits[0].normalFromColliderToRay,
                                    {}),
                "raycast start inside zero normal") &&
         expect(collidersUnchanged(colliders, before),
                "raycast does not mutate colliders");
}

bool sweptAabbHitsMissesSensorsAndZeroDisplacement() {
  std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {3.0F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F}),
      colliderAt({2U}, {0.0F, 2.0F, 0.0F}, {0.5F, 0.5F, 0.5F}, true),
  };
  const std::vector<iggy3d::PhysicsAabbCollider> before = colliders;
  const iggy3d::PhysicsAabbCollider moving =
      colliderAt({99U}, {}, {0.5F, 0.5F, 0.5F});

  iggy3d::PhysicsSweptAabbQueryRequest request;
  request.colliders = &colliders;
  request.movingCollider = &moving;
  request.displacementMeters = {5.0F, 0.0F, 0.0F};
  const iggy3d::PhysicsSweptAabbQueryResult hit =
      iggy3d::sweepPhysicsAabb(request);

  request.displacementMeters = {0.0F, 5.0F, 0.0F};
  const iggy3d::PhysicsSweptAabbQueryResult sensorSkipped =
      iggy3d::sweepPhysicsAabb(request);
  request.includeSensors = true;
  const iggy3d::PhysicsSweptAabbQueryResult sensorIncluded =
      iggy3d::sweepPhysicsAabb(request);

  request.displacementMeters = {0.0F, 0.0F, 0.0F};
  const iggy3d::PhysicsSweptAabbQueryResult zeroMiss =
      iggy3d::sweepPhysicsAabb(request);
  const iggy3d::PhysicsAabbCollider overlapping =
      colliderAt({100U}, {3.0F, 0.0F, 0.0F}, {3.0F, 0.5F, 0.5F});
  request.movingCollider = &overlapping;
  request.includeSensors = false;
  const iggy3d::PhysicsSweptAabbQueryResult zeroOverlap =
      iggy3d::sweepPhysicsAabb(request);

  return expect(hit.ok, "sweep hit ok") &&
         expect(hit.hit, "sweep hit flag") &&
         expect(hit.hitCount == 1U, "sweep hit count") &&
         expect(hit.nearestHit.colliderIndex == 0U,
                "sweep hit collider index") &&
         expect(nearlyEqual(hit.nearestHit.fraction, 0.4F),
                "sweep hit fraction") &&
         expect(nearlyEqual(hit.nearestHit.distanceMeters, 2.0F),
                "sweep hit distance") &&
         expect(iggy3d::nearlyEqual(hit.nearestHit.centerMeters,
                                    {2.0F, 0.0F, 0.0F}),
                "sweep hit center") &&
         expect(iggy3d::nearlyEqual(hit.nearestHit.normalFromColliderToMovingAabb,
                                    {-1.0F, 0.0F, 0.0F}),
                "sweep hit normal") &&
         expect(sensorSkipped.ok, "sensor skipped sweep ok") &&
         expect(sensorSkipped.hitCount == 0U,
                "sensor skipped sweep no hit") &&
         expect(sensorIncluded.ok, "sensor included sweep ok") &&
         expect(sensorIncluded.hitCount == 1U,
                "sensor included sweep hit") &&
         expect(sensorIncluded.nearestHit.sensor,
                "sensor included flag") &&
         expect(zeroMiss.ok, "zero sweep miss ok") &&
         expect(zeroMiss.hitCount == 0U, "zero sweep miss count") &&
         expect(zeroOverlap.ok, "zero sweep overlap ok") &&
         expect(zeroOverlap.hit, "zero sweep overlap hit") &&
         expect(zeroOverlap.nearestHit.initialOverlap,
                "zero sweep initial overlap") &&
         expect(zeroOverlap.nearestHit.distanceMeters == 0.0F,
                "zero sweep overlap distance") &&
         expect(collidersUnchanged(colliders, before),
                "sweep does not mutate colliders");
}

bool groundCheckUsesSweptAabbAndSensorPolicy() {
  std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {1.0F, 0.5F, 1.0F}),
      colliderAt({2U}, {4.0F, 0.0F, 0.0F}, {1.0F, 0.5F, 1.0F}, true),
  };
  const iggy3d::PhysicsAabbCollider body =
      colliderAt({99U}, {0.0F, 1.1F, 0.0F}, {0.5F, 0.5F, 0.5F});
  const iggy3d::PhysicsAabbCollider highBody =
      colliderAt({100U}, {0.0F, 3.0F, 0.0F}, {0.5F, 0.5F, 0.5F});
  const iggy3d::PhysicsAabbCollider sensorBody =
      colliderAt({101U}, {4.0F, 1.1F, 0.0F}, {0.5F, 0.5F, 0.5F});
  const iggy3d::PhysicsAabbCollider overlappingBody =
      colliderAt({102U}, {0.0F, 0.95F, 0.0F}, {0.5F, 0.5F, 0.5F});

  iggy3d::PhysicsGroundCheckQueryRequest request;
  request.colliders = &colliders;
  request.movingCollider = &body;
  request.probeDistanceMeters = 0.25F;
  const iggy3d::PhysicsGroundCheckQueryResult grounded =
      iggy3d::checkPhysicsGround(request);

  request.movingCollider = &highBody;
  const iggy3d::PhysicsGroundCheckQueryResult tooHigh =
      iggy3d::checkPhysicsGround(request);

  request.movingCollider = &sensorBody;
  const iggy3d::PhysicsGroundCheckQueryResult sensorSkipped =
      iggy3d::checkPhysicsGround(request);
  request.includeSensors = true;
  const iggy3d::PhysicsGroundCheckQueryResult sensorIncluded =
      iggy3d::checkPhysicsGround(request);
  request.includeSensors = false;
  request.movingCollider = &overlappingBody;
  const iggy3d::PhysicsGroundCheckQueryResult initialOverlap =
      iggy3d::checkPhysicsGround(request);

  return expect(grounded.ok, "ground check ok") &&
         expect(grounded.grounded, "ground detected") &&
         expect(grounded.nearestHit.colliderIndex == 0U,
                "ground collider index") &&
         expect(nearlyEqual(grounded.groundDistanceMeters, 0.1F),
                "ground distance") &&
         expect(iggy3d::nearlyEqual(grounded.groundNormal,
                                    {0.0F, 1.0F, 0.0F}),
                "ground normal") &&
         expect(tooHigh.ok, "too high check ok") &&
         expect(!tooHigh.grounded, "too high not grounded") &&
         expect(sensorSkipped.ok, "sensor skipped ground ok") &&
         expect(!sensorSkipped.grounded, "sensor skipped not grounded") &&
         expect(sensorIncluded.ok, "sensor included ground ok") &&
         expect(sensorIncluded.grounded, "sensor included grounded") &&
         expect(sensorIncluded.nearestHit.sensor,
                "sensor included nearest flag") &&
         expect(initialOverlap.ok, "initial overlap ground ok") &&
         expect(initialOverlap.grounded, "initial overlap grounded") &&
         expect(initialOverlap.nearestHit.initialOverlap,
                "initial overlap flag") &&
         expect(initialOverlap.groundDistanceMeters == 0.0F,
                "initial overlap ground distance") &&
         expect(iggy3d::nearlyEqual(initialOverlap.groundNormal,
                                    {0.0F, 1.0F, 0.0F}),
                "initial overlap fallback normal");
}

}  // namespace

int main() {
  const bool ok = statusNamesAreStable() &&
                  invalidRequestsRejectWithoutOutput() &&
                  overlapFindsDeterministicHitsAndSensorPolicy() &&
                  raycastHitsSortMissAndStartInsidePolicy() &&
                  sweptAabbHitsMissesSensorsAndZeroDisplacement() &&
                  groundCheckUsesSweptAabbAndSensorPolicy();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
