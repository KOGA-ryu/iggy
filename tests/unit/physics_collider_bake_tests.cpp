#include "runtime/physics/PhysicsColliderBake.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::PhysicsBodyDescriptor bodyDescriptor(iggy3d::Vec3 position) {
  iggy3d::PhysicsBodyDescriptor descriptor;
  descriptor.motion = iggy3d::PhysicsBodyMotionKind::Dynamic;
  descriptor.positionMeters = position;
  descriptor.velocityMetersPerSecond = {};
  descriptor.massKilograms = 1.0F;
  return descriptor;
}

iggy3d::PhysicsShapeDescriptor boxShape(
    iggy3d::Vec3 offset = {0.25F, 0.50F, -0.25F},
    iggy3d::Vec3 halfExtents = {0.50F, 1.00F, 0.75F},
    bool sensor = false) {
  iggy3d::PhysicsShapeDescriptor descriptor;
  descriptor.kind = iggy3d::PhysicsShapeKind::Box;
  descriptor.localCenterOffsetMeters = offset;
  descriptor.halfExtentsMeters = halfExtents;
  descriptor.sensor = sensor;
  return descriptor;
}

iggy3d::PhysicsShapeDescriptor triggerShape() {
  iggy3d::PhysicsShapeDescriptor descriptor = boxShape();
  descriptor.kind = iggy3d::PhysicsShapeKind::TriggerAabb;
  return descriptor;
}

iggy3d::PhysicsShapeDescriptor capsuleShape() {
  iggy3d::PhysicsShapeDescriptor descriptor = boxShape();
  descriptor.kind = iggy3d::PhysicsShapeKind::Capsule;
  return descriptor;
}

iggy3d::PhysicsMaterialDescriptor material(std::string key,
                                           std::uint32_t flags = 0U) {
  iggy3d::PhysicsMaterialDescriptor descriptor;
  descriptor.key = std::move(key);
  descriptor.staticFriction = 0.5F;
  descriptor.dynamicFriction = 0.4F;
  descriptor.restitution = 0.1F;
  descriptor.dampingMultiplier = 1.0F;
  descriptor.weightClass = iggy3d::PhysicsWeightClass::Medium;
  descriptor.flags = flags;
  return descriptor;
}

iggy3d::PhysicsBodyShapeBindingDescriptor binding(
    iggy3d::PhysicsBodyId bodyId,
    iggy3d::PhysicsShapeId shapeId,
    iggy3d::PhysicsMaterialId materialId,
    bool enabled = true) {
  iggy3d::PhysicsBodyShapeBindingDescriptor descriptor;
  descriptor.bodyId = bodyId;
  descriptor.shapeId = shapeId;
  descriptor.materialId = materialId;
  descriptor.enabled = enabled;
  return descriptor;
}

iggy3d::PhysicsAabbColliderBakeRequest request(
    const iggy3d::PhysicsBodyStore& bodies,
    const iggy3d::PhysicsShapeStore& shapes,
    const iggy3d::PhysicsMaterialTable& materials,
    const iggy3d::PhysicsBodyShapeBindingStore& bindings) {
  iggy3d::PhysicsAabbColliderBakeRequest result;
  result.bodies = &bodies;
  result.shapes = &shapes;
  result.materials = &materials;
  result.bindings = &bindings;
  return result;
}

struct Fixture {
  iggy3d::PhysicsBodyStore bodies;
  iggy3d::PhysicsShapeStore shapes;
  iggy3d::PhysicsMaterialTable materials;
  iggy3d::PhysicsBodyStoreResult firstBody;
  iggy3d::PhysicsBodyStoreResult secondBody;
  iggy3d::PhysicsShapeStoreResult firstShape;
  iggy3d::PhysicsShapeStoreResult secondShape;
  iggy3d::PhysicsMaterialTableResult stone;
  iggy3d::PhysicsMaterialTableResult trigger;
};

Fixture makeFixture() {
  Fixture fixture;
  fixture.firstBody = fixture.bodies.add(bodyDescriptor({10.0F, 0.0F, 0.0F}));
  fixture.secondBody = fixture.bodies.add(bodyDescriptor({0.0F, 0.0F, 5.0F}));
  fixture.firstShape = fixture.shapes.add(boxShape());
  fixture.secondShape = fixture.shapes.add(
      boxShape({0.0F, 1.0F, 0.0F}, {1.0F, 0.25F, 1.0F}));
  fixture.stone = fixture.materials.add(material("stone"));
  fixture.trigger = fixture.materials.add(
      material("trigger", iggy3d::kPhysicsMaterialFlagTrigger));
  return fixture;
}

bool storeEnabledMatches(const iggy3d::PhysicsBodyShapeBindingStore& store,
                         std::vector<bool> expected) {
  if (store.enabled().size() != expected.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < expected.size(); ++index) {
    if (store.enabled()[index] != expected[index]) {
      return false;
    }
  }
  return true;
}

bool stableNamesAreLowerSnake() {
  return expect(iggy3d::physicsBodyShapeBindingStatusName(
                    iggy3d::PhysicsBodyShapeBindingStatus::Valid) ==
                    "physics_body_shape_binding_valid",
                "binding valid status") &&
         expect(iggy3d::physicsBodyShapeBindingStatusName(
                    iggy3d::PhysicsBodyShapeBindingStatus::InvalidBodyId) ==
                    "physics_body_shape_binding_invalid_body_id",
                "binding invalid body status") &&
         expect(iggy3d::physicsBodyShapeBindingStatusName(
                    iggy3d::PhysicsBodyShapeBindingStatus::BindingAdded) ==
                    "physics_body_shape_binding_added",
                "binding added status") &&
         expect(iggy3d::physicsAabbColliderBakeStatusName(
                    iggy3d::PhysicsAabbColliderBakeStatus::Baked) ==
                    "physics_aabb_bake_baked",
                "baked status") &&
         expect(iggy3d::physicsAabbColliderBakeStatusName(
                    iggy3d::PhysicsAabbColliderBakeStatus::MissingBodyStore) ==
                    "physics_aabb_bake_missing_body_store",
                "missing body store status") &&
         expect(iggy3d::physicsAabbColliderBakeStatusName(
                    iggy3d::PhysicsAabbColliderBakeStatus::InvalidShapeKind) ==
                    "physics_aabb_bake_invalid_shape_kind",
                "invalid shape kind status") &&
         expect(!iggy3d::isValidPhysicsBodyShapeBindingId({0U}),
                "invalid binding id") &&
         expect(iggy3d::isValidPhysicsBodyShapeBindingId({1U}),
                "valid binding id");
}

bool bindingValidationAndMutationPolicy() {
  iggy3d::PhysicsBodyShapeBindingDescriptor valid = binding({1U}, {2U}, {3U});
  iggy3d::PhysicsBodyShapeBindingDescriptor invalidBody = valid;
  invalidBody.bodyId = {};
  iggy3d::PhysicsBodyShapeBindingDescriptor invalidShape = valid;
  invalidShape.shapeId = {};
  iggy3d::PhysicsBodyShapeBindingDescriptor invalidMaterial = valid;
  invalidMaterial.materialId = {};

  const iggy3d::PhysicsBodyShapeBindingValidationResult missing =
      iggy3d::validatePhysicsBodyShapeBindingDescriptor(nullptr);
  const iggy3d::PhysicsBodyShapeBindingValidationResult body =
      iggy3d::validatePhysicsBodyShapeBindingDescriptor(&invalidBody);
  const iggy3d::PhysicsBodyShapeBindingValidationResult shape =
      iggy3d::validatePhysicsBodyShapeBindingDescriptor(&invalidShape);
  const iggy3d::PhysicsBodyShapeBindingValidationResult mat =
      iggy3d::validatePhysicsBodyShapeBindingDescriptor(&invalidMaterial);

  iggy3d::PhysicsBodyShapeBindingStore store;
  const iggy3d::PhysicsBodyShapeBindingStoreResult rejected =
      store.add(invalidMaterial);

  return expect(!missing.ok, "missing descriptor rejected") &&
         expect(missing.reasonCode ==
                    "physics_body_shape_binding_missing_descriptor",
                "missing descriptor reason") &&
         expect(!body.ok, "invalid body rejected") &&
         expect(body.reasonCode ==
                    "physics_body_shape_binding_invalid_body_id",
                "invalid body reason") &&
         expect(!shape.ok, "invalid shape rejected") &&
         expect(shape.reasonCode ==
                    "physics_body_shape_binding_invalid_shape_id",
                "invalid shape reason") &&
         expect(!mat.ok, "invalid material rejected") &&
         expect(mat.reasonCode ==
                    "physics_body_shape_binding_invalid_material_id",
                "invalid material reason") &&
         expect(!rejected.ok, "invalid add rejected") &&
         expect(store.empty(), "invalid add does not mutate");
}

bool bindingStoreAddReadRemoveResetIsDeterministic() {
  iggy3d::PhysicsBodyShapeBindingStore store;
  const iggy3d::PhysicsBodyShapeBindingStoreResult first =
      store.add(binding({1U}, {10U}, {20U}, true));
  const iggy3d::PhysicsBodyShapeBindingStoreResult second =
      store.add(binding({2U}, {11U}, {21U}, false));
  const std::size_t countAfterAdds = store.size();
  const std::size_t idCountAfterAdds = store.ids().size();
  const std::size_t bodyIdCountAfterAdds = store.bodyIds().size();
  const std::size_t shapeIdCountAfterAdds = store.shapeIds().size();
  const std::size_t materialIdCountAfterAdds = store.materialIds().size();
  const bool enabledAfterAdds = storeEnabledMatches(store, {true, false});
  const iggy3d::PhysicsBodyShapeBindingStoreResult read =
      store.read(second.id);
  const iggy3d::PhysicsBodyShapeBindingStoreResult removed =
      store.remove(first.id);
  const iggy3d::PhysicsBodyShapeBindingStoreResult missing =
      store.read(first.id);
  const iggy3d::PhysicsBodyShapeBindingStoreResult remaining =
      store.read(second.id);
  const iggy3d::PhysicsBodyShapeBindingStoreResult reset = store.reset();
  const iggy3d::PhysicsBodyShapeBindingStoreResult afterReset =
      store.add(binding({3U}, {12U}, {22U}, true));

  return expect(first.ok && second.ok, "adds ok") &&
         expect(first.id.value == 1U, "first deterministic id") &&
         expect(second.id.value == 2U, "second deterministic id") &&
         expect(countAfterAdds == 2U, "store count") &&
         expect(idCountAfterAdds == 2U, "ids soa count") &&
         expect(bodyIdCountAfterAdds == 2U, "body ids soa count") &&
         expect(shapeIdCountAfterAdds == 2U, "shape ids soa count") &&
         expect(materialIdCountAfterAdds == 2U, "material ids soa count") &&
         expect(enabledAfterAdds, "enabled soa values") &&
         expect(read.ok, "read ok") &&
         expect(read.binding.id.value == second.id.value, "read id") &&
         expect(read.binding.bodyId.value == 2U, "read body") &&
         expect(read.binding.shapeId.value == 11U, "read shape") &&
         expect(read.binding.materialId.value == 21U, "read material") &&
         expect(!read.binding.enabled, "read disabled") &&
         expect(removed.ok, "remove ok") &&
         expect(removed.reasonCode == "physics_body_shape_binding_removed",
                "remove reason") &&
         expect(!missing.ok, "removed missing") &&
         expect(missing.reasonCode == "physics_body_shape_binding_not_found",
                "missing reason") &&
         expect(remaining.ok, "remaining ok") &&
         expect(remaining.index == 0U, "remaining compacted index") &&
         expect(reset.ok, "reset ok") &&
         expect(reset.reasonCode == "physics_body_shape_binding_store_reset",
                "reset reason") &&
         expect(afterReset.id.value == 1U, "id restarts after reset");
}

bool bakeRejectsMissingStores() {
  Fixture fixture = makeFixture();
  iggy3d::PhysicsBodyShapeBindingStore bindings;
  (void)bindings.add(binding(fixture.firstBody.id,
                             fixture.firstShape.id,
                             fixture.stone.id));
  iggy3d::PhysicsAabbColliderBakeRequest base =
      request(fixture.bodies, fixture.shapes, fixture.materials, bindings);
  iggy3d::PhysicsAabbColliderBakeRequest missingBodies = base;
  missingBodies.bodies = nullptr;
  iggy3d::PhysicsAabbColliderBakeRequest missingShapes = base;
  missingShapes.shapes = nullptr;
  iggy3d::PhysicsAabbColliderBakeRequest missingMaterials = base;
  missingMaterials.materials = nullptr;
  iggy3d::PhysicsAabbColliderBakeRequest missingBindings = base;
  missingBindings.bindings = nullptr;

  const iggy3d::PhysicsAabbColliderBakeResult bodies =
      iggy3d::bakePhysicsAabbColliders(missingBodies);
  const iggy3d::PhysicsAabbColliderBakeResult shapes =
      iggy3d::bakePhysicsAabbColliders(missingShapes);
  const iggy3d::PhysicsAabbColliderBakeResult materials =
      iggy3d::bakePhysicsAabbColliders(missingMaterials);
  const iggy3d::PhysicsAabbColliderBakeResult missing =
      iggy3d::bakePhysicsAabbColliders(missingBindings);

  return expect(!bodies.ok, "missing body store rejected") &&
         expect(bodies.reasonCode == "physics_aabb_bake_missing_body_store",
                "missing body store reason") &&
         expect(!shapes.ok, "missing shape store rejected") &&
         expect(shapes.reasonCode == "physics_aabb_bake_missing_shape_store",
                "missing shape store reason") &&
         expect(!materials.ok, "missing material table rejected") &&
         expect(materials.reasonCode ==
                    "physics_aabb_bake_missing_material_table",
                "missing material table reason") &&
         expect(!missing.ok, "missing binding store rejected") &&
         expect(missing.reasonCode ==
                    "physics_aabb_bake_missing_binding_store",
                "missing binding store reason");
}

bool bakeSkipsDisabledAndPreservesBindingOrder() {
  Fixture fixture = makeFixture();
  iggy3d::PhysicsBodyShapeBindingStore bindings;
  const iggy3d::PhysicsBodyShapeBindingStoreResult first =
      bindings.add(binding(fixture.firstBody.id,
                           fixture.firstShape.id,
                           fixture.stone.id,
                           true));
  const iggy3d::PhysicsBodyShapeBindingStoreResult disabled =
      bindings.add(binding(fixture.secondBody.id,
                           fixture.secondShape.id,
                           fixture.trigger.id,
                           false));
  const iggy3d::PhysicsBodyShapeBindingStoreResult third =
      bindings.add(binding(fixture.secondBody.id,
                           fixture.firstShape.id,
                           fixture.trigger.id,
                           true));
  const std::size_t bodyCountBefore = fixture.bodies.size();
  const std::size_t shapeCountBefore = fixture.shapes.size();
  const std::size_t materialCountBefore = fixture.materials.size();
  const std::size_t bindingCountBefore = bindings.size();

  const iggy3d::PhysicsAabbColliderBakeResult result =
      iggy3d::bakePhysicsAabbColliders(
          request(fixture.bodies, fixture.shapes, fixture.materials, bindings));

  return expect(first.ok && disabled.ok && third.ok, "binding adds ok") &&
         expect(result.ok, "bake ok") &&
         expect(result.reasonCode == "physics_aabb_bake_baked",
                "bake reason") &&
         expect(result.bindingCount == 3U, "binding count") &&
         expect(result.enabledBindingCount == 2U, "enabled count") &&
         expect(result.disabledBindingCount == 1U, "disabled count") &&
         expect(result.colliderCount == 2U, "collider count") &&
         expect(result.colliders.size() == 2U, "collider vector count") &&
         expect(result.sourceBindingIndices[0] == 0U, "first source index") &&
         expect(result.sourceBindingIndices[1] == 2U, "third source index") &&
         expect(result.bodyIds[0].value == fixture.firstBody.id.value,
                "first output body") &&
         expect(result.bodyIds[1].value == fixture.secondBody.id.value,
                "second output body") &&
         expect(result.shapeIds[0].value == fixture.firstShape.id.value,
                "first output shape") &&
         expect(result.materialIds[1].value == fixture.trigger.id.value,
                "second output material") &&
         expect(!result.colliders[0].sensor, "box not sensor") &&
         expect(iggy3d::nearlyEqual(result.colliders[0].worldCenterMeters,
                                    {10.25F, 0.50F, -0.25F}),
                "first world center") &&
         expect(iggy3d::nearlyEqual(result.colliders[0].bounds.min,
                                    {9.75F, -0.50F, -1.00F}),
                "first bounds min") &&
         expect(iggy3d::nearlyEqual(result.colliders[1].worldCenterMeters,
                                    {0.25F, 0.50F, 4.75F}),
                "second world center") &&
         expect(fixture.bodies.size() == bodyCountBefore, "body store size") &&
         expect(fixture.shapes.size() == shapeCountBefore, "shape store size") &&
         expect(fixture.materials.size() == materialCountBefore,
                "material table size") &&
         expect(bindings.size() == bindingCountBefore, "binding store size") &&
         expect(bindings.ids()[0].value == first.id.value,
                "binding store first id unchanged");
}

bool bakeMissingReferencesFailClosedWithoutPartialPacket() {
  Fixture fixture = makeFixture();
  iggy3d::PhysicsBodyShapeBindingStore missingBodyBindings;
  (void)missingBodyBindings.add(binding(fixture.firstBody.id,
                                        fixture.firstShape.id,
                                        fixture.stone.id));
  (void)missingBodyBindings.add(binding({99U},
                                        fixture.firstShape.id,
                                        fixture.stone.id));
  const iggy3d::PhysicsAabbColliderBakeResult missingBody =
      iggy3d::bakePhysicsAabbColliders(request(fixture.bodies,
                                               fixture.shapes,
                                               fixture.materials,
                                               missingBodyBindings));

  iggy3d::PhysicsBodyShapeBindingStore missingShapeBindings;
  (void)missingShapeBindings.add(binding(fixture.firstBody.id,
                                         {99U},
                                         fixture.stone.id));
  const iggy3d::PhysicsAabbColliderBakeResult missingShape =
      iggy3d::bakePhysicsAabbColliders(request(fixture.bodies,
                                               fixture.shapes,
                                               fixture.materials,
                                               missingShapeBindings));

  iggy3d::PhysicsBodyShapeBindingStore missingMaterialBindings;
  (void)missingMaterialBindings.add(binding(fixture.firstBody.id,
                                            fixture.firstShape.id,
                                            {99U}));
  const iggy3d::PhysicsAabbColliderBakeResult missingMaterial =
      iggy3d::bakePhysicsAabbColliders(request(fixture.bodies,
                                               fixture.shapes,
                                               fixture.materials,
                                               missingMaterialBindings));

  return expect(!missingBody.ok, "missing body rejected") &&
         expect(missingBody.reasonCode == "physics_aabb_bake_body_not_found",
                "missing body reason") &&
         expect(missingBody.invalidBindingIndex == 1U,
                "missing body index") &&
         expect(missingBody.invalidBodyId.value == 99U,
                "missing body id") &&
         expect(missingBody.colliders.empty(), "missing body no partial") &&
         expect(!missingShape.ok, "missing shape rejected") &&
         expect(missingShape.reasonCode == "physics_aabb_bake_shape_not_found",
                "missing shape reason") &&
         expect(missingShape.invalidBindingIndex == 0U,
                "missing shape index") &&
         expect(missingShape.invalidShapeId.value == 99U,
                "missing shape id") &&
         expect(missingShape.colliders.empty(), "missing shape no partial") &&
         expect(!missingMaterial.ok, "missing material rejected") &&
         expect(missingMaterial.reasonCode ==
                    "physics_aabb_bake_material_not_found",
                "missing material reason") &&
         expect(missingMaterial.invalidBindingIndex == 0U,
                "missing material index") &&
         expect(missingMaterial.invalidMaterialId.value == 99U,
                "missing material id") &&
         expect(missingMaterial.colliders.empty(),
                "missing material no partial");
}

bool unsupportedShapeKindFailsClosed() {
  Fixture fixture = makeFixture();
  const iggy3d::PhysicsShapeStoreResult capsule =
      fixture.shapes.add(capsuleShape());
  iggy3d::PhysicsBodyShapeBindingStore bindings;
  (void)bindings.add(binding(fixture.firstBody.id,
                             capsule.id,
                             fixture.stone.id));

  const iggy3d::PhysicsAabbColliderBakeResult result =
      iggy3d::bakePhysicsAabbColliders(
          request(fixture.bodies, fixture.shapes, fixture.materials, bindings));

  return expect(capsule.ok, "capsule added") &&
         expect(!result.ok, "unsupported shape rejected") &&
         expect(result.reasonCode == "physics_aabb_bake_invalid_shape_kind",
                "unsupported shape reason") &&
         expect(result.invalidBindingIndex == 0U,
                "unsupported shape index") &&
         expect(result.invalidShapeId.value == capsule.id.value,
                "unsupported shape id") &&
         expect(result.colliders.empty(), "unsupported shape no packet");
}

bool triggerAabbBakesSensorCollider() {
  Fixture fixture = makeFixture();
  const iggy3d::PhysicsShapeStoreResult trigger =
      fixture.shapes.add(triggerShape());
  iggy3d::PhysicsBodyShapeBindingStore bindings;
  (void)bindings.add(binding(fixture.firstBody.id,
                             trigger.id,
                             fixture.trigger.id));

  const iggy3d::PhysicsAabbColliderBakeResult result =
      iggy3d::bakePhysicsAabbColliders(
          request(fixture.bodies, fixture.shapes, fixture.materials, bindings));

  return expect(trigger.ok, "trigger shape added") &&
         expect(result.ok, "trigger bake ok") &&
         expect(result.colliderCount == 1U, "trigger collider count") &&
         expect(result.colliders[0].sensor, "trigger shape bakes sensor") &&
         expect(result.materialIds[0].value == fixture.trigger.id.value,
                "trigger material preserved");
}

}  // namespace

int main() {
  const bool ok = stableNamesAreLowerSnake() &&
                  bindingValidationAndMutationPolicy() &&
                  bindingStoreAddReadRemoveResetIsDeterministic() &&
                  bakeRejectsMissingStores() &&
                  bakeSkipsDisabledAndPreservesBindingOrder() &&
                  bakeMissingReferencesFailClosedWithoutPartialPacket() &&
                  unsupportedShapeKindFailsClosed() &&
                  triggerAabbBakesSensorCollider();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
