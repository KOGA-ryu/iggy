#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "runtime/physics/PhysicsAabbCollider.hpp"

namespace iggy3d {

enum class PhysicsCollisionQueryStatus : std::uint8_t {
  Queried,
  MissingColliders,
  MissingQueryCollider,
  InvalidCollider,
  InvalidQueryCollider,
  InvalidRayOrigin,
  InvalidRayDirection,
  InvalidMaxDistance,
  InvalidSweepDisplacement,
  InvalidGroundDirection,
  InvalidProbeDistance,
};

struct PhysicsAabbOverlapQueryRequest {
  const std::vector<PhysicsAabbCollider>* colliders = nullptr;
  const PhysicsAabbCollider* queryCollider = nullptr;
  bool includeSensors = false;
};

struct PhysicsAabbOverlapQueryResult {
  bool ok = false;
  PhysicsCollisionQueryStatus status =
      PhysicsCollisionQueryStatus::MissingColliders;
  std::string_view reasonCode = "physics_collision_query_missing_colliders";
  std::size_t colliderCount = 0U;
  std::size_t testedColliderCount = 0U;
  std::size_t hitCount = 0U;
  std::size_t invalidColliderIndex = 0U;
  std::vector<std::size_t> colliderIndices;
  std::vector<PhysicsBodyId> bodyIds;
  std::vector<bool> sensors;
};

struct PhysicsRaycastHit {
  std::size_t colliderIndex = 0U;
  PhysicsBodyId bodyId;
  float distanceMeters = 0.0F;
  Vec3 pointMeters;
  Vec3 normalFromColliderToRay;
  bool sensor = false;
  bool startInside = false;
};

struct PhysicsRaycastQueryRequest {
  const std::vector<PhysicsAabbCollider>* colliders = nullptr;
  Vec3 originMeters;
  Vec3 direction;
  float maxDistanceMeters = 0.0F;
  bool includeSensors = false;
};

struct PhysicsRaycastQueryResult {
  bool ok = false;
  PhysicsCollisionQueryStatus status =
      PhysicsCollisionQueryStatus::MissingColliders;
  std::string_view reasonCode = "physics_collision_query_missing_colliders";
  std::size_t colliderCount = 0U;
  std::size_t testedColliderCount = 0U;
  std::size_t hitCount = 0U;
  std::size_t invalidColliderIndex = 0U;
  std::vector<PhysicsRaycastHit> hits;
};

struct PhysicsSweptAabbHit {
  std::size_t colliderIndex = 0U;
  PhysicsBodyId bodyId;
  float fraction = 0.0F;
  float distanceMeters = 0.0F;
  Vec3 centerMeters;
  Vec3 pointMeters;
  Vec3 normalFromColliderToMovingAabb;
  bool sensor = false;
  bool initialOverlap = false;
};

struct PhysicsSweptAabbQueryRequest {
  const std::vector<PhysicsAabbCollider>* colliders = nullptr;
  const PhysicsAabbCollider* movingCollider = nullptr;
  Vec3 displacementMeters;
  bool includeSensors = false;
};

struct PhysicsSweptAabbQueryResult {
  bool ok = false;
  PhysicsCollisionQueryStatus status =
      PhysicsCollisionQueryStatus::MissingColliders;
  std::string_view reasonCode = "physics_collision_query_missing_colliders";
  std::size_t colliderCount = 0U;
  std::size_t testedColliderCount = 0U;
  std::size_t hitCount = 0U;
  std::size_t invalidColliderIndex = 0U;
  bool hit = false;
  PhysicsSweptAabbHit nearestHit;
  std::vector<PhysicsSweptAabbHit> hits;
};

struct PhysicsGroundCheckQueryRequest {
  const std::vector<PhysicsAabbCollider>* colliders = nullptr;
  const PhysicsAabbCollider* movingCollider = nullptr;
  Vec3 downDirection{0.0F, -1.0F, 0.0F};
  float probeDistanceMeters = 0.0F;
  bool includeSensors = false;
};

struct PhysicsGroundCheckQueryResult {
  bool ok = false;
  PhysicsCollisionQueryStatus status =
      PhysicsCollisionQueryStatus::MissingColliders;
  std::string_view reasonCode = "physics_collision_query_missing_colliders";
  std::size_t colliderCount = 0U;
  std::size_t testedColliderCount = 0U;
  std::size_t hitCount = 0U;
  bool grounded = false;
  float groundDistanceMeters = 0.0F;
  Vec3 groundNormal;
  PhysicsSweptAabbHit nearestHit;
};

std::string_view physicsCollisionQueryStatusName(
    PhysicsCollisionQueryStatus status);
PhysicsAabbOverlapQueryResult queryPhysicsAabbOverlaps(
    const PhysicsAabbOverlapQueryRequest& request);
PhysicsRaycastQueryResult raycastPhysicsAabbs(
    const PhysicsRaycastQueryRequest& request);
PhysicsSweptAabbQueryResult sweepPhysicsAabb(
    const PhysicsSweptAabbQueryRequest& request);
PhysicsGroundCheckQueryResult checkPhysicsGround(
    const PhysicsGroundCheckQueryRequest& request);

}  // namespace iggy3d
