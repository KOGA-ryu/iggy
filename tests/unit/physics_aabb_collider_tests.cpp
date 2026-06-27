#include "runtime/physics/PhysicsAabbCollider.hpp"
#include "runtime/physics/PhysicsShapeStore.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::PhysicsAabbColliderDescriptor descriptor(
    iggy3d::PhysicsBodyId bodyId = {1U}) {
  iggy3d::PhysicsAabbColliderDescriptor result;
  result.bodyId = bodyId;
  result.shape.localCenterOffsetMeters = {0.25F, 0.50F, -0.25F};
  result.shape.halfExtentsMeters = {0.50F, 1.00F, 0.75F};
  return result;
}

iggy3d::PhysicsAabbCollider colliderAt(iggy3d::PhysicsBodyId id,
                                       iggy3d::Vec3 center,
                                       iggy3d::Vec3 halfExtents) {
  iggy3d::PhysicsAabbCollider collider;
  collider.bodyId = id;
  collider.worldCenterMeters = center;
  collider.halfExtentsMeters = halfExtents;
  collider.bounds = iggy3d::aabbFromCenterExtents(center, halfExtents);
  return collider;
}

bool stableNamesAreLowerSnake() {
  return expect(iggy3d::physicsAabbColliderStatusName(
                    iggy3d::PhysicsAabbColliderStatus::Valid) ==
                    "physics_aabb_valid",
                "valid status name") &&
         expect(iggy3d::physicsAabbColliderStatusName(
                    iggy3d::PhysicsAabbColliderStatus::InvalidHalfExtents) ==
                    "physics_aabb_invalid_half_extents",
                "invalid extents status name") &&
         expect(iggy3d::physicsAabbColliderStatusName(
                    iggy3d::PhysicsAabbColliderStatus::MissingShapeStore) ==
                    "physics_aabb_missing_shape_store",
                "missing shape store status name") &&
         expect(iggy3d::physicsAabbColliderStatusName(
                    iggy3d::PhysicsAabbColliderStatus::ShapeNotFound) ==
                    "physics_aabb_shape_not_found",
                "shape not found status name") &&
         expect(iggy3d::physicsAabbColliderStatusName(
                    iggy3d::PhysicsAabbColliderStatus::InvalidShapeKind) ==
                    "physics_aabb_invalid_shape_kind",
                "invalid shape kind status name") &&
         expect(iggy3d::physicsAabbColliderStatusName(
                    iggy3d::PhysicsAabbColliderStatus::Built) ==
                    "physics_aabb_built",
                "built status name");
}

bool positiveFiniteHalfExtentsRejectInvalidValues() {
  return expect(iggy3d::isPositiveFinitePhysicsHalfExtents(
                    {0.5F, 1.0F, 0.25F}),
                "positive extents valid") &&
         expect(!iggy3d::isPositiveFinitePhysicsHalfExtents(
                    {0.0F, 1.0F, 0.25F}),
                "zero extent invalid") &&
         expect(!iggy3d::isPositiveFinitePhysicsHalfExtents(
                    {0.5F, -1.0F, 0.25F}),
                "negative extent invalid") &&
         expect(!iggy3d::isPositiveFinitePhysicsHalfExtents(
                    {0.5F, std::numeric_limits<float>::infinity(), 0.25F}),
                "infinite extent invalid");
}

bool descriptorValidationRejectsBadInputs() {
  const float inf = std::numeric_limits<float>::infinity();
  iggy3d::PhysicsAabbColliderDescriptor invalidBody = descriptor({0U});
  iggy3d::PhysicsAabbColliderDescriptor invalidOffset = descriptor();
  invalidOffset.shape.localCenterOffsetMeters.x = inf;
  iggy3d::PhysicsAabbColliderDescriptor invalidExtents = descriptor();
  invalidExtents.shape.halfExtentsMeters.z = 0.0F;

  const iggy3d::PhysicsAabbColliderResult missing =
      iggy3d::buildPhysicsAabbCollider(nullptr, {});
  const iggy3d::PhysicsAabbColliderResult body =
      iggy3d::buildPhysicsAabbCollider(&invalidBody, {});
  const iggy3d::PhysicsAabbColliderResult position =
      iggy3d::buildPhysicsAabbCollider(&invalidOffset, {0.0F, inf, 0.0F});
  const iggy3d::PhysicsAabbColliderResult offset =
      iggy3d::buildPhysicsAabbCollider(&invalidOffset, {});
  const iggy3d::PhysicsAabbColliderResult extents =
      iggy3d::buildPhysicsAabbCollider(&invalidExtents, {});

  return expect(!missing.ok, "missing descriptor rejected") &&
         expect(missing.reasonCode == "physics_aabb_missing_descriptor",
                "missing reason") &&
         expect(!body.ok, "invalid body rejected") &&
         expect(body.reasonCode == "physics_aabb_invalid_body_id",
                "body reason") &&
         expect(!position.ok, "invalid position rejected") &&
         expect(position.reasonCode == "physics_aabb_invalid_body_position",
                "position reason") &&
         expect(!offset.ok, "invalid offset rejected") &&
         expect(offset.reasonCode == "physics_aabb_invalid_center_offset",
                "offset reason") &&
         expect(!extents.ok, "invalid extents rejected") &&
         expect(extents.reasonCode == "physics_aabb_invalid_half_extents",
                "extents reason");
}

bool buildColliderCreatesWorldBoundsFromBodyPositionAndOffset() {
  iggy3d::PhysicsAabbColliderDescriptor input = descriptor({7U});
  input.sensor = true;

  const iggy3d::PhysicsAabbColliderResult result =
      iggy3d::buildPhysicsAabbCollider(&input, {2.0F, 3.0F, 4.0F});

  return expect(result.ok, "build ok") &&
         expect(result.status == iggy3d::PhysicsAabbColliderStatus::Built,
                "build status") &&
         expect(result.reasonCode == "physics_aabb_built", "build reason") &&
         expect(result.collider.bodyId.value == 7U, "body id preserved") &&
         expect(result.collider.sensor, "sensor preserved") &&
         expect(iggy3d::nearlyEqual(result.collider.worldCenterMeters,
                                    {2.25F, 3.50F, 3.75F}),
                "world center") &&
         expect(iggy3d::nearlyEqual(result.collider.halfExtentsMeters,
                                    {0.50F, 1.00F, 0.75F}),
                "half extents") &&
         expect(iggy3d::nearlyEqual(result.collider.bounds.min,
                                    {1.75F, 2.50F, 3.00F}),
                "bounds min") &&
         expect(iggy3d::nearlyEqual(result.collider.bounds.max,
                                    {2.75F, 4.50F, 4.50F}),
                "bounds max");
}

bool shapeStoreHelperBuildsIdenticalBounds() {
  iggy3d::PhysicsAabbColliderDescriptor input = descriptor({7U});
  input.sensor = true;

  iggy3d::PhysicsShapeStore store;
  iggy3d::PhysicsShapeDescriptor shape;
  shape.kind = iggy3d::PhysicsShapeKind::Box;
  shape.localCenterOffsetMeters = input.shape.localCenterOffsetMeters;
  shape.halfExtentsMeters = input.shape.halfExtentsMeters;
  shape.sensor = input.sensor;

  const iggy3d::PhysicsShapeStoreResult added = store.add(shape);
  const iggy3d::PhysicsAabbColliderResult descriptorResult =
      iggy3d::buildPhysicsAabbCollider(&input, {2.0F, 3.0F, 4.0F});
  const iggy3d::PhysicsAabbColliderResult shapeResult =
      iggy3d::buildPhysicsAabbColliderFromShape(&store,
                                                added.id,
                                                input.bodyId,
                                                {2.0F, 3.0F, 4.0F});

  return expect(added.ok, "shape add ok") &&
         expect(descriptorResult.ok, "descriptor build ok") &&
         expect(shapeResult.ok, "shape build ok") &&
         expect(shapeResult.reasonCode == "physics_aabb_built",
                "shape build reason") &&
         expect(shapeResult.collider.bodyId.value ==
                    descriptorResult.collider.bodyId.value,
                "shape body id") &&
         expect(shapeResult.collider.sensor == descriptorResult.collider.sensor,
                "shape sensor") &&
         expect(iggy3d::nearlyEqual(shapeResult.collider.worldCenterMeters,
                                    descriptorResult.collider.worldCenterMeters),
                "shape world center") &&
         expect(iggy3d::nearlyEqual(shapeResult.collider.halfExtentsMeters,
                                    descriptorResult.collider.halfExtentsMeters),
                "shape half extents") &&
         expect(iggy3d::nearlyEqual(shapeResult.collider.bounds.min,
                                    descriptorResult.collider.bounds.min),
                "shape bounds min") &&
         expect(iggy3d::nearlyEqual(shapeResult.collider.bounds.max,
                                    descriptorResult.collider.bounds.max),
                "shape bounds max");
}

bool shapeStoreHelperRejectsBadInputs() {
  iggy3d::PhysicsShapeStore store;
  const iggy3d::PhysicsShapeStoreResult box = store.add({
      iggy3d::PhysicsShapeKind::Box,
      {0.25F, 0.50F, -0.25F},
      {0.50F, 1.00F, 0.75F},
      false,
  });
  const iggy3d::PhysicsShapeStoreResult capsule = store.add({
      iggy3d::PhysicsShapeKind::Capsule,
      {},
      {0.50F, 1.00F, 0.50F},
      false,
  });

  const iggy3d::PhysicsAabbColliderResult missingStore =
      iggy3d::buildPhysicsAabbColliderFromShape(nullptr,
                                                box.id,
                                                {1U},
                                                {});
  const iggy3d::PhysicsAabbColliderResult missingShape =
      iggy3d::buildPhysicsAabbColliderFromShape(&store,
                                                {99U},
                                                {1U},
                                                {});
  const iggy3d::PhysicsAabbColliderResult invalidBody =
      iggy3d::buildPhysicsAabbColliderFromShape(&store,
                                                box.id,
                                                {0U},
                                                {});
  const iggy3d::PhysicsAabbColliderResult invalidKind =
      iggy3d::buildPhysicsAabbColliderFromShape(&store,
                                                capsule.id,
                                                {1U},
                                                {});

  return expect(box.ok, "box shape add ok") &&
         expect(capsule.ok, "capsule shape add ok") &&
         expect(!missingStore.ok, "missing store rejected") &&
         expect(missingStore.reasonCode == "physics_aabb_missing_shape_store",
                "missing store reason") &&
         expect(!missingShape.ok, "missing shape rejected") &&
         expect(missingShape.reasonCode == "physics_aabb_shape_not_found",
                "missing shape reason") &&
         expect(!invalidBody.ok, "invalid body rejected") &&
         expect(invalidBody.reasonCode == "physics_aabb_invalid_body_id",
                "invalid body reason") &&
         expect(!invalidKind.ok, "invalid kind rejected") &&
         expect(invalidKind.reasonCode == "physics_aabb_invalid_shape_kind",
                "invalid kind reason");
}

bool overlapUsesValidAabbBounds() {
  const iggy3d::PhysicsAabbCollider lhs =
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F});
  const iggy3d::PhysicsAabbCollider overlapping =
      colliderAt({2U}, {1.5F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F});
  const iggy3d::PhysicsAabbCollider touching =
      colliderAt({3U}, {2.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F});
  const iggy3d::PhysicsAabbCollider separated =
      colliderAt({4U}, {2.1F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F});
  const iggy3d::PhysicsAabbCollider invalid =
      colliderAt({0U}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F});

  return expect(iggy3d::physicsAabbOverlaps(lhs, overlapping),
                "overlap true") &&
         expect(iggy3d::physicsAabbOverlaps(lhs, touching),
                "touching counts as overlap") &&
         expect(!iggy3d::physicsAabbOverlaps(lhs, separated),
                "separated false") &&
         expect(!iggy3d::physicsAabbOverlaps(lhs, invalid),
                "invalid false");
}

bool pointContainmentUsesValidColliderBounds() {
  const iggy3d::PhysicsAabbCollider collider =
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {1.0F, 2.0F, 3.0F});

  return expect(iggy3d::physicsAabbContainsPoint(collider,
                                                 {0.5F, 1.0F, 2.0F}),
                "point inside") &&
         expect(iggy3d::physicsAabbContainsPoint(collider,
                                                 {1.0F, 2.0F, 3.0F}),
                "point on edge inside") &&
         expect(!iggy3d::physicsAabbContainsPoint(collider,
                                                  {1.1F, 0.0F, 0.0F}),
                "point outside") &&
         expect(!iggy3d::physicsAabbContainsPoint(
                    collider,
                    {0.0F, std::numeric_limits<float>::infinity(), 0.0F}),
                "non-finite point false");
}

}  // namespace

int main() {
  const bool ok = stableNamesAreLowerSnake() &&
                  positiveFiniteHalfExtentsRejectInvalidValues() &&
                  descriptorValidationRejectsBadInputs() &&
                  buildColliderCreatesWorldBoundsFromBodyPositionAndOffset() &&
                  shapeStoreHelperBuildsIdenticalBounds() &&
                  shapeStoreHelperRejectsBadInputs() &&
                  overlapUsesValidAabbBounds() &&
                  pointContainmentUsesValidColliderBounds();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
