#include "runtime/physics/PhysicsAabbCollisionBatch.hpp"

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

iggy3d::PhysicsBodyDescriptor bodyDescriptor(
    iggy3d::Vec3 position,
    iggy3d::PhysicsBodyMotionKind motion =
        iggy3d::PhysicsBodyMotionKind::Dynamic,
    iggy3d::Vec3 velocity = {}) {
  iggy3d::PhysicsBodyDescriptor descriptor;
  descriptor.motion = motion;
  descriptor.positionMeters = position;
  descriptor.velocityMetersPerSecond = velocity;
  descriptor.massKilograms = motion == iggy3d::PhysicsBodyMotionKind::Dynamic
                                 ? 1.0F
                                 : 100.0F;
  return descriptor;
}

iggy3d::PhysicsShapeDescriptor boxShape(
    iggy3d::Vec3 halfExtents = {0.5F, 0.5F, 0.5F},
    bool sensor = false) {
  iggy3d::PhysicsShapeDescriptor descriptor;
  descriptor.kind = iggy3d::PhysicsShapeKind::Box;
  descriptor.localCenterOffsetMeters = {};
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
  descriptor.restitution = 0.0F;
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

iggy3d::PhysicsAabbCollisionBatchRequest request(
    const iggy3d::PhysicsBodyStore& bodies,
    const iggy3d::PhysicsShapeStore& shapes,
    const iggy3d::PhysicsMaterialTable& materials,
    const iggy3d::PhysicsBodyShapeBindingStore& bindings) {
  iggy3d::PhysicsAabbCollisionBatchRequest result;
  result.bodies = &bodies;
  result.shapes = &shapes;
  result.materials = &materials;
  result.bindings = &bindings;
  return result;
}

bool accumulatorAllZero(const iggy3d::PhysicsBodyDeltaAccumulator& accumulator) {
  for (iggy3d::Vec3 value : accumulator.positionDeltasMeters) {
    if (!iggy3d::nearlyEqual(value, {})) {
      return false;
    }
  }
  for (iggy3d::Vec3 value : accumulator.velocityDeltasMetersPerSecond) {
    if (!iggy3d::nearlyEqual(value, {})) {
      return false;
    }
  }
  return true;
}

struct BasicWorld {
  iggy3d::PhysicsBodyStore bodies;
  iggy3d::PhysicsShapeStore shapes;
  iggy3d::PhysicsMaterialTable materials;
  iggy3d::PhysicsBodyShapeBindingStore bindings;
  iggy3d::PhysicsShapeStoreResult box;
  iggy3d::PhysicsMaterialTableResult stone;
  iggy3d::PhysicsMaterialTableResult trigger;
};

BasicWorld makeBasicWorld() {
  BasicWorld world;
  world.box = world.shapes.add(boxShape());
  world.stone = world.materials.add(material("stone"));
  world.trigger = world.materials.add(
      material("trigger", iggy3d::kPhysicsMaterialFlagTrigger));
  return world;
}

bool storesUnchanged(const iggy3d::PhysicsBodyStore& bodies,
                     const std::vector<iggy3d::PhysicsBodyId>& bodyIds,
                     const std::vector<iggy3d::Vec3>& positions,
                     const std::vector<iggy3d::Vec3>& velocities) {
  if (bodies.ids().size() != bodyIds.size() ||
      bodies.positions().size() != positions.size() ||
      bodies.velocities().size() != velocities.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < bodyIds.size(); ++index) {
    if (bodies.ids()[index].value != bodyIds[index].value ||
        !iggy3d::nearlyEqual(bodies.positions()[index], positions[index]) ||
        !iggy3d::nearlyEqual(bodies.velocities()[index],
                             velocities[index])) {
      return false;
    }
  }
  return true;
}

bool stableNamesAreLowerSnake() {
  return expect(iggy3d::physicsAabbCollisionBatchStatusName(
                    iggy3d::PhysicsAabbCollisionBatchStatus::Batched) ==
                    "physics_aabb_collision_batch_batched",
                "batched status") &&
         expect(iggy3d::physicsAabbCollisionBatchStatusName(
                    iggy3d::PhysicsAabbCollisionBatchStatus::BakeFailed) ==
                    "physics_aabb_collision_batch_bake_failed",
                "bake failed status") &&
         expect(iggy3d::physicsAabbCollisionBatchStatusName(
                    iggy3d::PhysicsAabbCollisionBatchStatus::BroadphaseFailed) ==
                    "physics_aabb_collision_batch_broadphase_failed",
                "broadphase failed status") &&
         expect(iggy3d::physicsAabbCollisionBatchStatusName(
                    iggy3d::PhysicsAabbCollisionBatchStatus::ContactFailed) ==
                    "physics_aabb_collision_batch_contact_failed",
                "contact failed status") &&
         expect(iggy3d::physicsAabbCollisionBatchStatusName(
                    iggy3d::PhysicsAabbCollisionBatchStatus::MaterialLookupFailed) ==
                    "physics_aabb_collision_batch_material_lookup_failed",
                "material lookup failed status") &&
         expect(iggy3d::physicsAabbCollisionBatchStatusName(
                    iggy3d::PhysicsAabbCollisionBatchStatus::SolveFailed) ==
                    "physics_aabb_collision_batch_solve_failed",
                "solve failed status") &&
         expect(iggy3d::physicsAabbCollisionBatchStatusName(
                    iggy3d::PhysicsAabbCollisionBatchStatus::AccumulateFailed) ==
                    "physics_aabb_collision_batch_accumulate_failed",
                "accumulate failed status") &&
         expect(iggy3d::physicsAabbCollisionBatchStatusName(
                    iggy3d::PhysicsAabbCollisionBatchStatus::MissingBodyStore) ==
                    "physics_aabb_collision_batch_missing_body_store",
                "missing body store status") &&
         expect(iggy3d::physicsAabbCollisionBatchStatusName(
                    iggy3d::PhysicsAabbCollisionBatchStatus::InvalidRequest) ==
                    "physics_aabb_collision_batch_invalid_request",
                "invalid request status");
}

bool invalidRequestFailsBeforeStages() {
  BasicWorld world = makeBasicWorld();
  iggy3d::PhysicsAabbCollisionBatchRequest missingBodies =
      request(world.bodies, world.shapes, world.materials, world.bindings);
  missingBodies.bodies = nullptr;
  iggy3d::PhysicsAabbCollisionBatchRequest invalidBroadphase =
      request(world.bodies, world.shapes, world.materials, world.bindings);
  invalidBroadphase.config.broadphaseCellSizeMeters = 0.0F;
  iggy3d::PhysicsAabbCollisionBatchRequest invalidSolve =
      request(world.bodies, world.shapes, world.materials, world.bindings);
  invalidSolve.config.solveConfig.positionCorrectionPercent = 2.0F;

  const iggy3d::PhysicsAabbCollisionBatchResult missing =
      iggy3d::planPhysicsAabbCollisionBatch(missingBodies);
  const iggy3d::PhysicsAabbCollisionBatchResult broadphase =
      iggy3d::planPhysicsAabbCollisionBatch(invalidBroadphase);
  const iggy3d::PhysicsAabbCollisionBatchResult solve =
      iggy3d::planPhysicsAabbCollisionBatch(invalidSolve);

  return expect(!missing.ok, "missing body store rejected") &&
         expect(missing.reasonCode ==
                    "physics_aabb_collision_batch_missing_body_store",
                "missing body store reason") &&
         expect(!broadphase.ok, "invalid broadphase rejected") &&
         expect(broadphase.reasonCode ==
                    "physics_aabb_collision_batch_invalid_request",
                "invalid broadphase reason") &&
         expect(broadphase.upstreamReasonCode ==
                    "physics_broadphase_invalid_grid_config",
                "invalid broadphase upstream") &&
         expect(!solve.ok, "invalid solve config rejected") &&
         expect(solve.upstreamReasonCode ==
                    "physics_aabb_contact_solve_invalid_config",
                "invalid solve upstream");
}

bool emptyBindingsProduceEmptyBatchWithBodyRows() {
  BasicWorld world = makeBasicWorld();
  const iggy3d::PhysicsBodyStoreResult body =
      world.bodies.add(bodyDescriptor({0.0F, 0.0F, 0.0F}));

  const iggy3d::PhysicsAabbCollisionBatchResult result =
      iggy3d::planPhysicsAabbCollisionBatch(
          request(world.bodies, world.shapes, world.materials,
                  world.bindings));

  return expect(body.ok, "body add ok") &&
         expect(result.ok, "empty batch ok") &&
         expect(result.bindingCount == 0U, "empty binding count") &&
         expect(result.colliderCount == 0U, "empty collider count") &&
         expect(result.broadphasePairCount == 0U, "empty pair count") &&
         expect(result.contactCount == 0U, "empty contact count") &&
         expect(result.solvePlanCount == 0U, "empty solve count") &&
         expect(result.accumulator.bodyIds.size() == 1U,
                "accumulator body row") &&
         expect(result.accumulator.bodyIds[0].value == body.id.value,
                "accumulator body id") &&
         expect(accumulatorAllZero(result.accumulator),
                "empty accumulator deltas");
}

bool twoDynamicOverlappingBoxesProduceSolveAndDeltas() {
  BasicWorld world = makeBasicWorld();
  const iggy3d::PhysicsBodyStoreResult first =
      world.bodies.add(bodyDescriptor({0.0F, 0.0F, 0.0F}));
  const iggy3d::PhysicsBodyStoreResult second =
      world.bodies.add(bodyDescriptor({0.75F, 0.0F, 0.0F}));
  (void)world.bindings.add(binding(first.id, world.box.id, world.stone.id));
  (void)world.bindings.add(binding(second.id, world.box.id, world.stone.id));
  const std::vector<iggy3d::PhysicsBodyId> bodyIds = world.bodies.ids();
  const std::vector<iggy3d::Vec3> positions = world.bodies.positions();
  const std::vector<iggy3d::Vec3> velocities = world.bodies.velocities();

  const iggy3d::PhysicsAabbCollisionBatchResult result =
      iggy3d::planPhysicsAabbCollisionBatch(
          request(world.bodies, world.shapes, world.materials,
                  world.bindings));

  return expect(result.ok, "dynamic batch ok") &&
         expect(result.colliderCount == 2U, "dynamic collider count") &&
         expect(result.broadphasePairCount == 1U, "dynamic pair count") &&
         expect(result.contactCount == 1U, "dynamic contact count") &&
         expect(result.materialPairs.size() == 1U, "material pair count") &&
         expect(result.solvePlanCount == 1U, "solve plan count") &&
         expect(result.accumulatedPlanCount == 1U,
                "one solve accumulated") &&
         expect(result.contacts[0].firstBodyId.value == first.id.value,
                "contact first id") &&
         expect(result.contacts[0].secondBodyId.value == second.id.value,
                "contact second id") &&
         expect(result.solvePlans[0].positionCorrectionApplied,
                "position correction planned") &&
         expect(result.accumulator.contributingPlanCounts[0] == 1U,
                "first contribution") &&
         expect(result.accumulator.contributingPlanCounts[1] == 1U,
                "second contribution") &&
         expect(result.accumulator.positionDeltasMeters[0].x < 0.0F,
                "first moves left") &&
         expect(result.accumulator.positionDeltasMeters[1].x > 0.0F,
                "second moves right") &&
         expect(storesUnchanged(world.bodies, bodyIds, positions, velocities),
                "body store unchanged");
}

bool dynamicAgainstStaticAccumulatesOnlyDynamicDelta() {
  BasicWorld world = makeBasicWorld();
  const iggy3d::PhysicsBodyStoreResult dynamicBody =
      world.bodies.add(bodyDescriptor({0.0F, 0.0F, 0.0F}));
  const iggy3d::PhysicsBodyStoreResult staticBody = world.bodies.add(
      bodyDescriptor({0.75F, 0.0F, 0.0F},
                     iggy3d::PhysicsBodyMotionKind::Static));
  (void)world.bindings.add(binding(dynamicBody.id, world.box.id,
                                   world.stone.id));
  (void)world.bindings.add(binding(staticBody.id, world.box.id,
                                   world.stone.id));

  const iggy3d::PhysicsAabbCollisionBatchResult result =
      iggy3d::planPhysicsAabbCollisionBatch(
          request(world.bodies, world.shapes, world.materials,
                  world.bindings));

  return expect(result.ok, "dynamic-static batch ok") &&
         expect(result.accumulatedPlanCount == 1U,
                "dynamic-static accumulated") &&
         expect(result.accumulator.positionDeltasMeters[0].x < 0.0F,
                "dynamic body corrected") &&
         expect(iggy3d::nearlyEqual(result.accumulator.positionDeltasMeters[1],
                                    {}),
                "static body position unchanged") &&
         expect(iggy3d::nearlyEqual(
                    result.accumulator.velocityDeltasMetersPerSecond[1], {}),
                "static body velocity unchanged");
}

bool disabledAndSeparatedBindingsProduceNoPairs() {
  BasicWorld disabledWorld = makeBasicWorld();
  const iggy3d::PhysicsBodyStoreResult disabledFirst =
      disabledWorld.bodies.add(bodyDescriptor({0.0F, 0.0F, 0.0F}));
  const iggy3d::PhysicsBodyStoreResult disabledSecond =
      disabledWorld.bodies.add(bodyDescriptor({0.75F, 0.0F, 0.0F}));
  (void)disabledWorld.bindings.add(binding(disabledFirst.id,
                                           disabledWorld.box.id,
                                           disabledWorld.stone.id));
  (void)disabledWorld.bindings.add(binding(disabledSecond.id,
                                           disabledWorld.box.id,
                                           disabledWorld.stone.id,
                                           false));
  const iggy3d::PhysicsAabbCollisionBatchResult disabled =
      iggy3d::planPhysicsAabbCollisionBatch(request(
          disabledWorld.bodies, disabledWorld.shapes, disabledWorld.materials,
          disabledWorld.bindings));

  BasicWorld separatedWorld = makeBasicWorld();
  const iggy3d::PhysicsBodyStoreResult separatedFirst =
      separatedWorld.bodies.add(bodyDescriptor({0.0F, 0.0F, 0.0F}));
  const iggy3d::PhysicsBodyStoreResult separatedSecond =
      separatedWorld.bodies.add(bodyDescriptor({4.0F, 0.0F, 0.0F}));
  (void)separatedWorld.bindings.add(binding(separatedFirst.id,
                                            separatedWorld.box.id,
                                            separatedWorld.stone.id));
  (void)separatedWorld.bindings.add(binding(separatedSecond.id,
                                            separatedWorld.box.id,
                                            separatedWorld.stone.id));
  const iggy3d::PhysicsAabbCollisionBatchResult separated =
      iggy3d::planPhysicsAabbCollisionBatch(request(
          separatedWorld.bodies, separatedWorld.shapes,
          separatedWorld.materials, separatedWorld.bindings));

  return expect(disabled.ok, "disabled batch ok") &&
         expect(disabled.colliderCount == 1U, "disabled collider skipped") &&
         expect(disabled.broadphasePairCount == 0U, "disabled no pair") &&
         expect(disabled.contactCount == 0U, "disabled no contact") &&
         expect(accumulatorAllZero(disabled.accumulator),
                "disabled zero accumulator") &&
         expect(separated.ok, "separated batch ok") &&
         expect(separated.colliderCount == 2U, "separated collider count") &&
         expect(separated.broadphasePairCount == 0U, "separated no pair") &&
         expect(separated.contactCount == 0U, "separated no contact") &&
         expect(accumulatorAllZero(separated.accumulator),
                "separated zero accumulator");
}

bool triggerShapeAndMaterialAreNoOpRows() {
  BasicWorld triggerShapeWorld = makeBasicWorld();
  const iggy3d::PhysicsShapeStoreResult triggerShapeAdded =
      triggerShapeWorld.shapes.add(triggerShape());
  const iggy3d::PhysicsBodyStoreResult shapeFirst =
      triggerShapeWorld.bodies.add(bodyDescriptor({0.0F, 0.0F, 0.0F}));
  const iggy3d::PhysicsBodyStoreResult shapeSecond =
      triggerShapeWorld.bodies.add(bodyDescriptor({0.75F, 0.0F, 0.0F}));
  (void)triggerShapeWorld.bindings.add(binding(shapeFirst.id,
                                               triggerShapeWorld.box.id,
                                               triggerShapeWorld.stone.id));
  (void)triggerShapeWorld.bindings.add(binding(shapeSecond.id,
                                               triggerShapeAdded.id,
                                               triggerShapeWorld.stone.id));
  const iggy3d::PhysicsAabbCollisionBatchResult shapeResult =
      iggy3d::planPhysicsAabbCollisionBatch(request(
          triggerShapeWorld.bodies, triggerShapeWorld.shapes,
          triggerShapeWorld.materials, triggerShapeWorld.bindings));

  BasicWorld triggerMaterialWorld = makeBasicWorld();
  const iggy3d::PhysicsBodyStoreResult materialFirst =
      triggerMaterialWorld.bodies.add(bodyDescriptor({0.0F, 0.0F, 0.0F}));
  const iggy3d::PhysicsBodyStoreResult materialSecond =
      triggerMaterialWorld.bodies.add(bodyDescriptor({0.75F, 0.0F, 0.0F}));
  (void)triggerMaterialWorld.bindings.add(binding(materialFirst.id,
                                                  triggerMaterialWorld.box.id,
                                                  triggerMaterialWorld.stone.id));
  (void)triggerMaterialWorld.bindings.add(binding(materialSecond.id,
                                                  triggerMaterialWorld.box.id,
                                                  triggerMaterialWorld.trigger.id));
  const iggy3d::PhysicsAabbCollisionBatchResult materialResult =
      iggy3d::planPhysicsAabbCollisionBatch(request(
          triggerMaterialWorld.bodies, triggerMaterialWorld.shapes,
          triggerMaterialWorld.materials, triggerMaterialWorld.bindings));

  return expect(shapeResult.ok, "trigger shape batch ok") &&
         expect(shapeResult.contactCount == 1U, "trigger shape contact") &&
         expect(shapeResult.sensorContactCount == 1U,
                "trigger shape sensor count") &&
         expect(shapeResult.skippedNoOpPlanCount == 1U,
                "trigger shape skipped") &&
         expect(shapeResult.accumulatedPlanCount == 0U,
                "trigger shape no accumulated") &&
         expect(accumulatorAllZero(shapeResult.accumulator),
                "trigger shape zero accumulator") &&
         expect(materialResult.ok, "trigger material batch ok") &&
         expect(materialResult.sensorContactCount == 1U,
                "trigger material sensor count") &&
         expect(materialResult.skippedNoOpPlanCount == 1U,
                "trigger material skipped") &&
         expect(materialResult.materialPairs[0].trigger,
                "trigger material pair") &&
         expect(accumulatorAllZero(materialResult.accumulator),
                "trigger material zero accumulator");
}

bool unsupportedShapeKindMapsToBakeFailed() {
  BasicWorld world = makeBasicWorld();
  const iggy3d::PhysicsShapeStoreResult capsule =
      world.shapes.add(capsuleShape());
  const iggy3d::PhysicsBodyStoreResult body =
      world.bodies.add(bodyDescriptor({0.0F, 0.0F, 0.0F}));
  (void)world.bindings.add(binding(body.id, capsule.id, world.stone.id));

  const iggy3d::PhysicsAabbCollisionBatchResult result =
      iggy3d::planPhysicsAabbCollisionBatch(
          request(world.bodies, world.shapes, world.materials,
                  world.bindings));

  return expect(!result.ok, "unsupported shape rejected") &&
         expect(result.reasonCode ==
                    "physics_aabb_collision_batch_bake_failed",
                "unsupported maps to bake failed") &&
         expect(result.upstreamReasonCode ==
                    "physics_aabb_bake_invalid_shape_kind",
                "unsupported upstream reason") &&
         expect(result.invalidBindingIndex == 0U,
                "unsupported binding index") &&
         expect(result.invalidShapeId.value == capsule.id.value,
                "unsupported shape id") &&
         expect(result.colliders.empty(), "unsupported no packet");
}

bool staticStaticOverlapIsNoEffectiveMassNoOp() {
  BasicWorld world = makeBasicWorld();
  const iggy3d::PhysicsBodyStoreResult first = world.bodies.add(
      bodyDescriptor({0.0F, 0.0F, 0.0F},
                     iggy3d::PhysicsBodyMotionKind::Static));
  const iggy3d::PhysicsBodyStoreResult second = world.bodies.add(
      bodyDescriptor({0.75F, 0.0F, 0.0F},
                     iggy3d::PhysicsBodyMotionKind::Static));
  (void)world.bindings.add(binding(first.id, world.box.id, world.stone.id));
  (void)world.bindings.add(binding(second.id, world.box.id, world.stone.id));

  const iggy3d::PhysicsAabbCollisionBatchResult result =
      iggy3d::planPhysicsAabbCollisionBatch(
          request(world.bodies, world.shapes, world.materials,
                  world.bindings));

  return expect(result.ok, "static-static batch ok") &&
         expect(result.solvePlanCount == 1U, "static-static solve row") &&
         expect(result.solvePlans[0].status ==
                    iggy3d::PhysicsAabbContactSolveStatus::NoEffectiveMass,
                "static-static no effective mass") &&
         expect(result.skippedNoOpPlanCount == 1U,
                "static-static skipped no-op") &&
         expect(result.accumulatedPlanCount == 0U,
                "static-static no accumulated") &&
         expect(accumulatorAllZero(result.accumulator),
                "static-static zero accumulator");
}

bool outputOrderingFollowsBroadphasePairs() {
  BasicWorld world = makeBasicWorld();
  const iggy3d::PhysicsMaterialTableResult ice =
      world.materials.add(material("ice"));
  const iggy3d::PhysicsBodyStoreResult first =
      world.bodies.add(bodyDescriptor({0.0F, 0.0F, 0.0F}));
  const iggy3d::PhysicsBodyStoreResult second =
      world.bodies.add(bodyDescriptor({0.75F, 0.0F, 0.0F}));
  const iggy3d::PhysicsBodyStoreResult third =
      world.bodies.add(bodyDescriptor({1.50F, 0.0F, 0.0F}));
  (void)world.bindings.add(binding(first.id, world.box.id, world.stone.id));
  (void)world.bindings.add(binding(second.id, world.box.id, world.trigger.id));
  (void)world.bindings.add(binding(third.id, world.box.id, ice.id));
  iggy3d::PhysicsAabbCollisionBatchRequest batch =
      request(world.bodies, world.shapes, world.materials, world.bindings);
  batch.config.broadphaseCellSizeMeters = 10.0F;

  const iggy3d::PhysicsAabbCollisionBatchResult result =
      iggy3d::planPhysicsAabbCollisionBatch(batch);

  return expect(result.ok, "ordering batch ok") &&
         expect(result.broadphasePairCount == 2U, "two pair count") &&
         expect(result.contacts.size() == 2U, "two contacts") &&
         expect(result.materialPairs.size() == 2U, "two material pairs") &&
         expect(result.solvePlans.size() == 2U, "two solve plans") &&
         expect(result.broadphasePairs[0].firstColliderIndex == 0U &&
                    result.broadphasePairs[0].secondColliderIndex == 1U,
                "first pair order") &&
         expect(result.broadphasePairs[1].firstColliderIndex == 1U &&
                    result.broadphasePairs[1].secondColliderIndex == 2U,
                "second pair order") &&
         expect(result.contacts[0].firstColliderIndex == 0U &&
                    result.contacts[0].secondColliderIndex == 1U,
                "first contact lines up") &&
         expect(result.contacts[1].firstColliderIndex == 1U &&
                    result.contacts[1].secondColliderIndex == 2U,
                "second contact lines up") &&
         expect(result.colliderMaterialIds[0].value == world.stone.id.value,
                "first collider material") &&
         expect(result.colliderMaterialIds[1].value == world.trigger.id.value,
                "second collider material") &&
         expect(result.colliderMaterialIds[2].value == ice.id.value,
                "third collider material") &&
         expect(result.materialPairs[0].firstMaterialId.value ==
                    world.stone.id.value,
                "first pair material min id") &&
         expect(result.materialPairs[1].secondMaterialId.value == ice.id.value,
                "second pair material max id");
}

}  // namespace

int main() {
  const bool ok = stableNamesAreLowerSnake() &&
                  invalidRequestFailsBeforeStages() &&
                  emptyBindingsProduceEmptyBatchWithBodyRows() &&
                  twoDynamicOverlappingBoxesProduceSolveAndDeltas() &&
                  dynamicAgainstStaticAccumulatesOnlyDynamicDelta() &&
                  disabledAndSeparatedBindingsProduceNoPairs() &&
                  triggerShapeAndMaterialAreNoOpRows() &&
                  unsupportedShapeKindMapsToBakeFailed() &&
                  staticStaticOverlapIsNoEffectiveMassNoOp() &&
                  outputOrderingFollowsBroadphasePairs();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
