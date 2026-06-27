#include "runtime/physics/PhysicsAabbCollider.hpp"

#include <array>

#include "runtime/physics/PhysicsShapeStore.hpp"

namespace iggy3d {
namespace {

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1087
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

PhysicsAabbColliderResult colliderResult(PhysicsAabbColliderStatus status,
                                         bool ok) {
  PhysicsAabbColliderResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsAabbColliderStatusName(status);
  return result;
}

bool isAabbCompatibleShapeKind(PhysicsShapeKind kind) {
  return kind == PhysicsShapeKind::Box || kind == PhysicsShapeKind::TriggerAabb;
}

PhysicsAabbColliderResult buildColliderFromShapeFacts(
    PhysicsBodyId bodyId,
    PhysicsAabbShape shape,
    bool sensor,
    Vec3 bodyPositionMeters) {
  // branch-gate: BG-1087
  if (!isValidPhysicsBodyId(bodyId)) {
    return colliderResult(PhysicsAabbColliderStatus::InvalidBodyId, false);
  }
  // branch-gate: BG-1087
  if (!isFinite(bodyPositionMeters)) {
    return colliderResult(PhysicsAabbColliderStatus::InvalidBodyPosition,
                          false);
  }
  // branch-gate: BG-1087
  if (!isFinite(shape.localCenterOffsetMeters)) {
    return colliderResult(PhysicsAabbColliderStatus::InvalidCenterOffset,
                          false);
  }
  // branch-gate: BG-1087
  if (!isPositiveFinitePhysicsHalfExtents(shape.halfExtentsMeters)) {
    return colliderResult(PhysicsAabbColliderStatus::InvalidHalfExtents,
                          false);
  }

  PhysicsAabbColliderResult result =
      colliderResult(PhysicsAabbColliderStatus::Built, true);
  result.collider.bodyId = bodyId;
  result.collider.sensor = sensor;
  result.collider.worldCenterMeters =
      bodyPositionMeters + shape.localCenterOffsetMeters;
  result.collider.halfExtentsMeters = shape.halfExtentsMeters;
  result.collider.bounds = aabbFromCenterExtents(
      result.collider.worldCenterMeters, result.collider.halfExtentsMeters);
  return result;
}

}  // namespace

std::string_view physicsAabbColliderStatusName(
    PhysicsAabbColliderStatus status) {
  static constexpr std::array<std::string_view, 10> kNames{
      "physics_aabb_valid",
      "physics_aabb_missing_descriptor",
      "physics_aabb_invalid_body_id",
      "physics_aabb_invalid_body_position",
      "physics_aabb_invalid_center_offset",
      "physics_aabb_invalid_half_extents",
      "physics_aabb_missing_shape_store",
      "physics_aabb_shape_not_found",
      "physics_aabb_invalid_shape_kind",
      "physics_aabb_built",
  };
  return enumName(status, kNames, "physics_aabb_invalid_body_id");
}

bool isPositiveFinitePhysicsHalfExtents(Vec3 halfExtentsMeters) {
  return isFinite(halfExtentsMeters) && halfExtentsMeters.x > 0.0F &&
         halfExtentsMeters.y > 0.0F && halfExtentsMeters.z > 0.0F;
}

bool isValidPhysicsAabbCollider(const PhysicsAabbCollider& collider) {
  return isValidPhysicsBodyId(collider.bodyId) &&
         isFinite(collider.worldCenterMeters) &&
         isPositiveFinitePhysicsHalfExtents(collider.halfExtentsMeters) &&
         isValid(collider.bounds);
}

PhysicsAabbColliderResult buildPhysicsAabbCollider(
    const PhysicsAabbColliderDescriptor* descriptor,
    Vec3 bodyPositionMeters) {
  // branch-gate: BG-1087
  if (descriptor == nullptr) {
    return colliderResult(PhysicsAabbColliderStatus::MissingDescriptor, false);
  }
  return buildColliderFromShapeFacts(descriptor->bodyId, descriptor->shape,
                                     descriptor->sensor, bodyPositionMeters);
}

PhysicsAabbColliderResult buildPhysicsAabbColliderFromShape(
    const PhysicsShapeStore* shapeStore,
    PhysicsShapeId shapeId,
    PhysicsBodyId bodyId,
    Vec3 bodyPositionMeters) {
  // branch-gate: BG-1087
  if (shapeStore == nullptr) {
    return colliderResult(PhysicsAabbColliderStatus::MissingShapeStore, false);
  }

  const PhysicsShapeStoreResult shape = shapeStore->read(shapeId);
  // branch-gate: BG-1087
  if (!shape.ok) {
    return colliderResult(PhysicsAabbColliderStatus::ShapeNotFound, false);
  }
  // branch-gate: BG-1087
  if (!isAabbCompatibleShapeKind(shape.shape.kind)) {
    return colliderResult(PhysicsAabbColliderStatus::InvalidShapeKind, false);
  }

  PhysicsAabbShape aabbShape;
  aabbShape.localCenterOffsetMeters = shape.shape.localCenterOffsetMeters;
  aabbShape.halfExtentsMeters = shape.shape.halfExtentsMeters;
  const bool sensor =
      shape.shape.sensor || shape.shape.kind == PhysicsShapeKind::TriggerAabb;
  return buildColliderFromShapeFacts(bodyId, aabbShape, sensor,
                                     bodyPositionMeters);
}

bool physicsAabbOverlaps(const PhysicsAabbCollider& lhs,
                         const PhysicsAabbCollider& rhs) {
  // branch-gate: BG-1087
  if (!isValidPhysicsAabbCollider(lhs) || !isValidPhysicsAabbCollider(rhs)) {
    return false;
  }
  return intersects(lhs.bounds, rhs.bounds);
}

bool physicsAabbContainsPoint(const PhysicsAabbCollider& collider,
                              Vec3 pointMeters) {
  // branch-gate: BG-1087
  if (!isValidPhysicsAabbCollider(collider) || !isFinite(pointMeters)) {
    return false;
  }
  return contains(collider.bounds, pointMeters);
}

}  // namespace iggy3d
