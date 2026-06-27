#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "runtime/physics/PhysicsBroadphase.hpp"

namespace iggy3d {

enum class PhysicsAabbContactStatus : std::uint8_t {
  ContactGenerated,
  MissingColliders,
  MissingPair,
  PairIndexOutOfRange,
  InvalidCollider,
  PairBodyMismatch,
  SameBodyPair,
  NoOverlap,
};

struct PhysicsAabbContact {
  PhysicsBodyId firstBodyId;
  PhysicsBodyId secondBodyId;
  std::size_t firstColliderIndex = 0U;
  std::size_t secondColliderIndex = 0U;
  Vec3 normalFromFirstToSecond{1.0F, 0.0F, 0.0F};
  float penetrationMeters = 0.0F;
  Vec3 pointMeters;
  bool includesSensor = false;
};

struct PhysicsAabbContactRequest {
  const std::vector<PhysicsAabbCollider>* colliders = nullptr;
  const PhysicsBroadphasePair* pair = nullptr;
};

struct PhysicsAabbContactResult {
  bool ok = false;
  PhysicsAabbContactStatus status = PhysicsAabbContactStatus::MissingPair;
  std::string_view reasonCode = "physics_aabb_contact_missing_pair";
  std::size_t invalidColliderIndex = 0U;
  PhysicsAabbContact contact;
};

std::string_view physicsAabbContactStatusName(
    PhysicsAabbContactStatus status);

PhysicsAabbContactResult generatePhysicsAabbContact(
    const PhysicsAabbContactRequest& request);

}  // namespace iggy3d
