#pragma once

#include <cstdint>
#include <string_view>

#include "core/math/Aabb3.hpp"
#include "runtime/physics/PhysicsTypes.hpp"

namespace iggy3d {

class PhysicsShapeStore;

enum class PhysicsAabbColliderStatus : std::uint8_t {
  Valid,
  MissingDescriptor,
  InvalidBodyId,
  InvalidBodyPosition,
  InvalidCenterOffset,
  InvalidHalfExtents,
  MissingShapeStore,
  ShapeNotFound,
  InvalidShapeKind,
  Built,
};

struct PhysicsAabbShape {
  Vec3 localCenterOffsetMeters;
  Vec3 halfExtentsMeters{0.5F, 0.5F, 0.5F};
};

struct PhysicsAabbColliderDescriptor {
  PhysicsBodyId bodyId;
  PhysicsAabbShape shape;
  bool sensor = false;
};

struct PhysicsAabbCollider {
  PhysicsBodyId bodyId;
  Vec3 worldCenterMeters;
  Vec3 halfExtentsMeters;
  Aabb3 bounds;
  bool sensor = false;
};

struct PhysicsAabbColliderResult {
  bool ok = false;
  PhysicsAabbColliderStatus status =
      PhysicsAabbColliderStatus::MissingDescriptor;
  std::string_view reasonCode = "physics_aabb_missing_descriptor";
  PhysicsAabbCollider collider;
};

std::string_view physicsAabbColliderStatusName(
    PhysicsAabbColliderStatus status);

bool isPositiveFinitePhysicsHalfExtents(Vec3 halfExtentsMeters);
bool isValidPhysicsAabbCollider(const PhysicsAabbCollider& collider);
PhysicsAabbColliderResult buildPhysicsAabbCollider(
    const PhysicsAabbColliderDescriptor* descriptor,
    Vec3 bodyPositionMeters);
PhysicsAabbColliderResult buildPhysicsAabbColliderFromShape(
    const PhysicsShapeStore* shapeStore,
    PhysicsShapeId shapeId,
    PhysicsBodyId bodyId,
    Vec3 bodyPositionMeters);
bool physicsAabbOverlaps(const PhysicsAabbCollider& lhs,
                         const PhysicsAabbCollider& rhs);
bool physicsAabbContainsPoint(const PhysicsAabbCollider& collider,
                              Vec3 pointMeters);

}  // namespace iggy3d
