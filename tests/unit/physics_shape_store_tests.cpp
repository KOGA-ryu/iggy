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

iggy3d::PhysicsShapeDescriptor boxShape() {
  iggy3d::PhysicsShapeDescriptor descriptor;
  descriptor.kind = iggy3d::PhysicsShapeKind::Box;
  descriptor.localCenterOffsetMeters = {0.25F, 0.50F, -0.25F};
  descriptor.halfExtentsMeters = {0.50F, 1.00F, 0.75F};
  descriptor.sensor = true;
  return descriptor;
}

bool stableNamesAreLowerSnake() {
  return expect(iggy3d::physicsShapeKindName(
                    iggy3d::PhysicsShapeKind::Box) == "box",
                "box kind name") &&
         expect(iggy3d::physicsShapeKindName(
                    iggy3d::PhysicsShapeKind::Capsule) == "capsule",
                "capsule kind name") &&
         expect(iggy3d::physicsShapeKindName(
                    iggy3d::PhysicsShapeKind::FloorSpan) == "floor_span",
                "floor span kind name") &&
         expect(iggy3d::physicsShapeKindName(
                    iggy3d::PhysicsShapeKind::WallSlab) == "wall_slab",
                "wall slab kind name") &&
         expect(iggy3d::physicsShapeKindName(
                    iggy3d::PhysicsShapeKind::TriggerAabb) ==
                    "trigger_aabb",
                "trigger kind name") &&
         expect(iggy3d::physicsShapeStatusName(
                    iggy3d::PhysicsShapeStatus::InvalidHalfExtents) ==
                    "physics_shape_invalid_half_extents",
                "invalid extents status") &&
         expect(iggy3d::physicsShapeStatusName(
                    iggy3d::PhysicsShapeStatus::ShapeAdded) ==
                    "physics_shape_added",
                "added status") &&
         expect(!iggy3d::isValidPhysicsShapeId({0U}), "invalid shape id") &&
         expect(iggy3d::isValidPhysicsShapeId({1U}), "valid shape id");
}

bool validationRejectsInvalidDescriptorFacts() {
  const float inf = std::numeric_limits<float>::infinity();
  iggy3d::PhysicsShapeDescriptor invalidKind = boxShape();
  invalidKind.kind = static_cast<iggy3d::PhysicsShapeKind>(99U);
  iggy3d::PhysicsShapeDescriptor invalidOffset = boxShape();
  invalidOffset.localCenterOffsetMeters.x = inf;
  iggy3d::PhysicsShapeDescriptor invalidExtents = boxShape();
  invalidExtents.halfExtentsMeters.y = 0.0F;

  const iggy3d::PhysicsShapeValidationResult missing =
      iggy3d::validatePhysicsShapeDescriptor(nullptr);
  const iggy3d::PhysicsShapeValidationResult kind =
      iggy3d::validatePhysicsShapeDescriptor(&invalidKind);
  const iggy3d::PhysicsShapeValidationResult offset =
      iggy3d::validatePhysicsShapeDescriptor(&invalidOffset);
  const iggy3d::PhysicsShapeValidationResult extents =
      iggy3d::validatePhysicsShapeDescriptor(&invalidExtents);

  return expect(!missing.ok, "missing rejected") &&
         expect(missing.reasonCode == "physics_shape_missing_descriptor",
                "missing reason") &&
         expect(!kind.ok, "invalid kind rejected") &&
         expect(kind.reasonCode == "physics_shape_invalid_kind",
                "kind reason") &&
         expect(!offset.ok, "invalid offset rejected") &&
         expect(offset.reasonCode == "physics_shape_invalid_center_offset",
                "offset reason") &&
         expect(!extents.ok, "invalid extents rejected") &&
         expect(extents.reasonCode == "physics_shape_invalid_half_extents",
                "extents reason");
}

bool addAndReadShapePreservesSoAFacts() {
  iggy3d::PhysicsShapeStore store;
  const iggy3d::PhysicsShapeStoreResult added = store.add(boxShape());
  const iggy3d::PhysicsShapeStoreResult read = store.read(added.id);

  return expect(added.ok, "add ok") &&
         expect(added.status == iggy3d::PhysicsShapeStatus::ShapeAdded,
                "add status") &&
         expect(added.reasonCode == "physics_shape_added", "add reason") &&
         expect(added.id.value == 1U, "first id") &&
         expect(added.index == 0U, "first index") &&
         expect(store.size() == 1U, "store count") &&
         expect(store.ids().size() == 1U, "ids soa count") &&
         expect(store.kinds().size() == 1U, "kinds soa count") &&
         expect(store.localCenterOffsets().size() == 1U,
                "offsets soa count") &&
         expect(store.halfExtents().size() == 1U, "extents soa count") &&
         expect(store.sensors().size() == 1U, "sensors soa count") &&
         expect(read.ok, "read ok") &&
         expect(read.shape.id.value == 1U, "read id") &&
         expect(read.shape.kind == iggy3d::PhysicsShapeKind::Box,
                "read kind") &&
         expect(iggy3d::nearlyEqual(read.shape.localCenterOffsetMeters,
                                    {0.25F, 0.50F, -0.25F}),
                "read offset") &&
         expect(iggy3d::nearlyEqual(read.shape.halfExtentsMeters,
                                    {0.50F, 1.00F, 0.75F}),
                "read extents") &&
         expect(read.shape.sensor, "read sensor");
}

bool invalidShapeDoesNotMutateStore() {
  iggy3d::PhysicsShapeStore store;
  iggy3d::PhysicsShapeDescriptor descriptor = boxShape();
  descriptor.halfExtentsMeters.z = -1.0F;

  const iggy3d::PhysicsShapeStoreResult added = store.add(descriptor);

  return expect(!added.ok, "invalid add rejected") &&
         expect(added.status == iggy3d::PhysicsShapeStatus::InvalidHalfExtents,
                "invalid add status") &&
         expect(added.reasonCode == "physics_shape_invalid_half_extents",
                "invalid add reason") &&
         expect(store.empty(), "store still empty");
}

bool removePreservesRemainingOrderAndMissingIsStable() {
  iggy3d::PhysicsShapeStore store;
  const iggy3d::PhysicsShapeStoreResult first = store.add(boxShape());
  iggy3d::PhysicsShapeDescriptor secondDescriptor = boxShape();
  secondDescriptor.localCenterOffsetMeters = {8.0F, 0.0F, 0.0F};
  const iggy3d::PhysicsShapeStoreResult second = store.add(secondDescriptor);

  const iggy3d::PhysicsShapeStoreResult removed = store.remove(first.id);
  const iggy3d::PhysicsShapeStoreResult missing = store.read(first.id);
  const iggy3d::PhysicsShapeStoreResult remaining = store.read(second.id);

  return expect(removed.ok, "remove ok") &&
         expect(removed.status == iggy3d::PhysicsShapeStatus::ShapeRemoved,
                "remove status") &&
         expect(removed.shape.id.value == first.id.value, "removed shape id") &&
         expect(store.size() == 1U, "remaining size") &&
         expect(store.ids()[0].value == second.id.value,
                "remaining order preserved") &&
         expect(!missing.ok, "missing read rejected") &&
         expect(missing.reasonCode == "physics_shape_not_found",
                "missing reason") &&
         expect(remaining.ok, "remaining read ok") &&
         expect(remaining.index == 0U, "remaining compacted index");
}

bool resetClearsStoreAndRestartsIds() {
  iggy3d::PhysicsShapeStore store;
  (void)store.add(boxShape());
  const iggy3d::PhysicsShapeStoreResult reset = store.reset();
  const iggy3d::PhysicsShapeStoreResult added = store.add(boxShape());

  return expect(reset.ok, "reset ok") &&
         expect(reset.reasonCode == "physics_shape_store_reset",
                "reset reason") &&
         expect(store.size() == 1U, "one shape after reset add") &&
         expect(added.id.value == 1U, "id restarts after reset");
}

bool invalidShapeIdReadsFailWithStableReason() {
  iggy3d::PhysicsShapeStore store;
  (void)store.add(boxShape());

  const iggy3d::PhysicsShapeStoreResult invalid = store.read({0U});
  const iggy3d::PhysicsShapeStoreResult missing = store.read({99U});

  return expect(!invalid.ok, "invalid id rejected") &&
         expect(invalid.reasonCode == "physics_shape_not_found",
                "invalid id reason") &&
         expect(!missing.ok, "missing id rejected") &&
         expect(missing.reasonCode == "physics_shape_not_found",
                "missing id reason");
}

}  // namespace

int main() {
  const bool ok = stableNamesAreLowerSnake() &&
                  validationRejectsInvalidDescriptorFacts() &&
                  addAndReadShapePreservesSoAFacts() &&
                  invalidShapeDoesNotMutateStore() &&
                  removePreservesRemainingOrderAndMissingIsStable() &&
                  resetClearsStoreAndRestartsIds() &&
                  invalidShapeIdReadsFailWithStableReason();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
