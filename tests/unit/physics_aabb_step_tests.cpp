#include "runtime/physics/PhysicsAabbStep.hpp"

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
                                 : 0.0F;
  return descriptor;
}

iggy3d::PhysicsShapeDescriptor boxShape(
    iggy3d::PhysicsShapeKind kind = iggy3d::PhysicsShapeKind::Box) {
  iggy3d::PhysicsShapeDescriptor descriptor;
  descriptor.kind = kind;
  descriptor.halfExtentsMeters = {0.5F, 0.5F, 0.5F};
  return descriptor;
}

iggy3d::PhysicsMaterialDescriptor material(std::string key) {
  iggy3d::PhysicsMaterialDescriptor descriptor;
  descriptor.key = std::move(key);
  descriptor.staticFriction = 0.5F;
  descriptor.dynamicFriction = 0.4F;
  descriptor.restitution = 0.0F;
  descriptor.dampingMultiplier = 1.0F;
  descriptor.weightClass = iggy3d::PhysicsWeightClass::Medium;
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

struct PhysicsWorld {
  iggy3d::PhysicsBodyStore bodies;
  iggy3d::PhysicsShapeStore shapes;
  iggy3d::PhysicsMaterialTable materials;
  iggy3d::PhysicsBodyShapeBindingStore bindings;
  iggy3d::PhysicsMaterialTableResult stone;
};

struct StoreSnapshot {
  std::vector<std::uint32_t> shapeIds;
  std::vector<std::uint32_t> materialIds;
  std::vector<std::uint32_t> bindingIds;
  std::vector<bool> bindingEnabled;
};

StoreSnapshot snapshotStores(const PhysicsWorld& world) {
  StoreSnapshot snapshot;
  for (iggy3d::PhysicsShapeId id : world.shapes.ids()) {
    snapshot.shapeIds.push_back(id.value);
  }
  for (iggy3d::PhysicsMaterialId id : world.materials.ids()) {
    snapshot.materialIds.push_back(id.value);
  }
  for (iggy3d::PhysicsBodyShapeBindingId id : world.bindings.ids()) {
    snapshot.bindingIds.push_back(id.value);
  }
  snapshot.bindingEnabled.assign(world.bindings.enabled().begin(),
                                 world.bindings.enabled().end());
  return snapshot;
}

bool storesMatch(const PhysicsWorld& world, const StoreSnapshot& snapshot) {
  if (world.shapes.ids().size() != snapshot.shapeIds.size() ||
      world.materials.ids().size() != snapshot.materialIds.size() ||
      world.bindings.ids().size() != snapshot.bindingIds.size() ||
      world.bindings.enabled().size() != snapshot.bindingEnabled.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < snapshot.shapeIds.size(); ++index) {
    if (world.shapes.ids()[index].value != snapshot.shapeIds[index]) {
      return false;
    }
  }
  for (std::size_t index = 0U; index < snapshot.materialIds.size(); ++index) {
    if (world.materials.ids()[index].value != snapshot.materialIds[index]) {
      return false;
    }
  }
  for (std::size_t index = 0U; index < snapshot.bindingIds.size(); ++index) {
    if (world.bindings.ids()[index].value != snapshot.bindingIds[index] ||
        world.bindings.enabled()[index] != snapshot.bindingEnabled[index]) {
      return false;
    }
  }
  return true;
}

PhysicsWorld makeWorld() {
  PhysicsWorld world;
  world.stone = world.materials.add(material("stone"));
  return world;
}

iggy3d::PhysicsAabbStepConfig stepConfig() {
  iggy3d::PhysicsAabbStepConfig config;
  config.stepConfig.stepSeconds = 0.5F;
  config.stepConfig.gravityMetersPerSecondSquared = {0.0F, -4.0F, 0.0F};
  return config;
}

iggy3d::PhysicsBodyStoreResult addBoundBody(
    PhysicsWorld& world,
    iggy3d::Vec3 position,
    iggy3d::PhysicsBodyMotionKind motion =
        iggy3d::PhysicsBodyMotionKind::Dynamic,
    iggy3d::Vec3 velocity = {},
    iggy3d::PhysicsShapeKind shapeKind = iggy3d::PhysicsShapeKind::Box,
    bool bindingEnabled = true) {
  const iggy3d::PhysicsBodyStoreResult body =
      world.bodies.add(bodyDescriptor(position, motion, velocity));
  const iggy3d::PhysicsShapeStoreResult shape =
      world.shapes.add(boxShape(shapeKind));
  (void)world.bindings.add(
      binding(body.id, shape.id, world.stone.id, bindingEnabled));
  return body;
}

iggy3d::PhysicsAabbStepRequest request(PhysicsWorld& world,
                                       iggy3d::PhysicsAabbStepConfig config) {
  iggy3d::PhysicsAabbStepRequest result;
  result.bodies = &world.bodies;
  result.shapes = &world.shapes;
  result.materials = &world.materials;
  result.bindings = &world.bindings;
  result.config = config;
  return result;
}

bool bodyStateMatches(const iggy3d::PhysicsBodyStore& lhs,
                      const iggy3d::PhysicsBodyStore& rhs) {
  if (lhs.ids().size() != rhs.ids().size()) {
    return false;
  }
  for (std::size_t index = 0U; index < lhs.ids().size(); ++index) {
    if (lhs.ids()[index].value != rhs.ids()[index].value ||
        !iggy3d::nearlyEqual(lhs.positions()[index], rhs.positions()[index]) ||
        !iggy3d::nearlyEqual(lhs.velocities()[index],
                             rhs.velocities()[index])) {
      return false;
    }
  }
  return true;
}

bool stableNamesAreLowerSnake() {
  return expect(iggy3d::physicsAabbStepStatusName(
                    iggy3d::PhysicsAabbStepStatus::Stepped) ==
                    "physics_aabb_step_stepped",
                "stepped status") &&
         expect(iggy3d::physicsAabbStepStatusName(
                    iggy3d::PhysicsAabbStepStatus::VelocityPhaseFailed) ==
                    "physics_aabb_step_velocity_phase_failed",
                "velocity failed status") &&
         expect(iggy3d::physicsAabbStepStatusName(
                    iggy3d::PhysicsAabbStepStatus::CollisionBatchFailed) ==
                    "physics_aabb_step_collision_batch_failed",
                "collision failed status") &&
         expect(iggy3d::physicsAabbStepStatusName(
                    iggy3d::PhysicsAabbStepStatus::DeltaApplyFailed) ==
                    "physics_aabb_step_delta_apply_failed",
                "delta failed status") &&
         expect(iggy3d::physicsAabbStepStatusName(
                    iggy3d::PhysicsAabbStepStatus::PositionPhaseFailed) ==
                    "physics_aabb_step_position_phase_failed",
                "position failed status") &&
         expect(iggy3d::physicsAabbStepStatusName(
                    iggy3d::PhysicsAabbStepStatus::InvalidConfig) ==
                    "physics_aabb_step_invalid_config",
                "invalid config status") &&
         expect(iggy3d::physicsAabbStepStatusName(
                    iggy3d::PhysicsAabbStepStatus::MissingBodyStore) ==
                    "physics_aabb_step_missing_body_store",
                "missing body store status");
}

bool missingStoreAndInvalidConfigsRejectWithoutMutation() {
  PhysicsWorld world = makeWorld();
  const iggy3d::PhysicsBodyStoreResult dynamic =
      addBoundBody(world, {}, iggy3d::PhysicsBodyMotionKind::Dynamic);
  iggy3d::PhysicsAabbStepConfig invalidStep = stepConfig();
  invalidStep.stepConfig.stepSeconds = 0.0F;
  iggy3d::PhysicsAabbStepConfig invalidCollision = stepConfig();
  invalidCollision.collisionConfig.broadphaseCellSizeMeters = 0.0F;
  iggy3d::PhysicsAabbStepConfig invalidApply = stepConfig();
  invalidApply.deltaApplyConfig.maxPositionDeltaMeters = -1.0F;

  iggy3d::PhysicsAabbStepRequest missingRequest = request(world, stepConfig());
  missingRequest.bodies = nullptr;
  const iggy3d::PhysicsAabbStepResult missing =
      iggy3d::stepPhysicsAabbWorld(missingRequest);
  const iggy3d::PhysicsAabbStepResult step =
      iggy3d::stepPhysicsAabbWorld(request(world, invalidStep));
  const iggy3d::PhysicsAabbStepResult collision =
      iggy3d::stepPhysicsAabbWorld(request(world, invalidCollision));
  const iggy3d::PhysicsAabbStepResult apply =
      iggy3d::stepPhysicsAabbWorld(request(world, invalidApply));
  const iggy3d::PhysicsBodyStoreResult after = world.bodies.read(dynamic.id);

  return expect(!missing.ok, "missing store rejected") &&
         expect(missing.reasonCode == "physics_aabb_step_missing_body_store",
                "missing store reason") &&
         expect(!step.ok, "invalid step rejected") &&
         expect(step.reasonCode == "physics_aabb_step_invalid_config",
                "invalid step status") &&
         expect(step.upstreamReasonCode == "physics_step_invalid_step_seconds",
                "invalid step upstream") &&
         expect(!collision.ok, "invalid collision rejected") &&
         expect(collision.upstreamReasonCode ==
                    "physics_broadphase_invalid_grid_config",
                "invalid collision upstream") &&
         expect(!apply.ok, "invalid apply rejected") &&
         expect(apply.upstreamReasonCode == "physics_body_delta_invalid_config",
                "invalid apply upstream") &&
         expect(iggy3d::nearlyEqual(after.body.positionMeters, {}),
                "invalid configs keep position") &&
         expect(iggy3d::nearlyEqual(after.body.velocityMetersPerSecond, {}),
                "invalid configs keep velocity");
}

bool noCollisionBodyFallsThroughVelocityAndPositionPhases() {
  PhysicsWorld world = makeWorld();
  const iggy3d::PhysicsBodyStoreResult dynamic =
      addBoundBody(world, {}, iggy3d::PhysicsBodyMotionKind::Dynamic);

  const iggy3d::PhysicsAabbStepResult result =
      iggy3d::stepPhysicsAabbWorld(request(world, stepConfig()));
  const iggy3d::PhysicsBodyStoreResult after = world.bodies.read(dynamic.id);

  return expect(result.ok, "no-collision step ok") &&
         expect(result.reasonCode == "physics_aabb_step_stepped",
                "no-collision reason") &&
         expect(result.bodyCount == 1U, "no-collision body count") &&
         expect(result.colliderCount == 1U, "one collider baked") &&
         expect(result.broadphasePairCount == 0U, "no pairs") &&
         expect(result.contactCount == 0U, "no contacts") &&
         expect(result.deltaApply.status == iggy3d::PhysicsBodyDeltaStatus::NoOp,
                "delta apply no-op") &&
         expect(result.positionPhase.positionIntegratedBodyCount == 1U,
                "position phase ran") &&
         expect(iggy3d::nearlyEqual(after.body.velocityMetersPerSecond,
                                    {0.0F, -2.0F, 0.0F}),
                "gravity velocity applied") &&
         expect(iggy3d::nearlyEqual(after.body.positionMeters,
                                    {0.0F, -1.0F, 0.0F}),
                "position integrated after velocity");
}

bool emptyBindingsStillIntegrateWithoutCollisions() {
  PhysicsWorld world = makeWorld();
  const iggy3d::PhysicsBodyStoreResult dynamic =
      world.bodies.add(bodyDescriptor({}, iggy3d::PhysicsBodyMotionKind::Dynamic));

  const iggy3d::PhysicsAabbStepResult result =
      iggy3d::stepPhysicsAabbWorld(request(world, stepConfig()));
  const iggy3d::PhysicsBodyStoreResult after = world.bodies.read(dynamic.id);

  return expect(result.ok, "empty binding step ok") &&
         expect(result.colliderCount == 0U, "empty binding no colliders") &&
         expect(result.contactCount == 0U, "empty binding no contacts") &&
         expect(iggy3d::nearlyEqual(after.body.velocityMetersPerSecond,
                                    {0.0F, -2.0F, 0.0F}),
                "empty binding velocity phase") &&
         expect(iggy3d::nearlyEqual(after.body.positionMeters,
                                    {0.0F, -1.0F, 0.0F}),
                "empty binding position phase");
}

bool dynamicBodyAgainstStaticFloorAppliesCollisionBeforePosition() {
  PhysicsWorld world = makeWorld();
  const iggy3d::PhysicsBodyStoreResult dynamic =
      addBoundBody(world, {0.0F, 0.0F, 0.0F},
                   iggy3d::PhysicsBodyMotionKind::Dynamic);
  (void)addBoundBody(world, {0.0F, -0.75F, 0.0F},
                     iggy3d::PhysicsBodyMotionKind::Static);
  const StoreSnapshot beforeStores = snapshotStores(world);

  iggy3d::PhysicsBodyStore pure = world.bodies;
  const iggy3d::PhysicsStepResult pureResult =
      iggy3d::stepPhysicsBodies(&pure, stepConfig().stepConfig);
  const iggy3d::PhysicsAabbStepResult result =
      iggy3d::stepPhysicsAabbWorld(request(world, stepConfig()));
  const iggy3d::PhysicsBodyStoreResult after = world.bodies.read(dynamic.id);

  return expect(pureResult.ok, "pure step ok") &&
         expect(result.ok, "collision step ok") &&
         expect(result.colliderCount == 2U, "collision colliders") &&
         expect(result.broadphasePairCount == 1U, "collision pair count") &&
         expect(result.contactCount == 1U, "collision contact count") &&
         expect(result.solvePlanCount == 1U, "collision solve count") &&
         expect(result.accumulatedPlanCount == 1U,
                "collision accumulated count") &&
         expect(result.appliedPositionCount == 1U,
                "collision position applied") &&
         expect(result.appliedVelocityCount == 1U,
                "collision velocity applied") &&
         expect(result.positionPhase.positionIntegratedBodyCount == 1U,
                "position phase ran after apply") &&
         expect(after.body.positionMeters.y > pure.positions()[0].y,
                "collision position differs upward from pure step") &&
         expect(after.body.velocityMetersPerSecond.y >
                    pure.velocities()[0].y,
                "collision velocity differs upward from pure step") &&
         expect(after.body.positionMeters.y > 0.0F,
                "collision moved dynamic out of floor") &&
         expect(iggy3d::nearlyEqual(after.body.velocityMetersPerSecond, {}),
                "collision removed downward velocity") &&
         expect(storesMatch(world, beforeStores),
                "shape material binding stores unchanged");
}

bool collisionBatchFailureStopsAfterVelocityPhase() {
  PhysicsWorld world = makeWorld();
  const iggy3d::PhysicsBodyStoreResult dynamic =
      addBoundBody(world,
                   {},
                   iggy3d::PhysicsBodyMotionKind::Dynamic,
                   {},
                   iggy3d::PhysicsShapeKind::Capsule);

  const iggy3d::PhysicsAabbStepResult result =
      iggy3d::stepPhysicsAabbWorld(request(world, stepConfig()));
  const iggy3d::PhysicsBodyStoreResult after = world.bodies.read(dynamic.id);

  return expect(!result.ok, "unsupported shape fails") &&
         expect(result.reasonCode ==
                    "physics_aabb_step_collision_batch_failed",
                "collision batch failure reason") &&
         expect(result.collisionBatch.reasonCode ==
                    "physics_aabb_collision_batch_bake_failed",
                "batch failure reason") &&
         expect(result.collisionBatch.upstreamReasonCode ==
                    "physics_aabb_bake_invalid_shape_kind",
                "batch upstream bake reason") &&
         expect(result.velocityPhase.ok, "velocity phase happened") &&
         expect(!result.positionPhase.ok, "position phase did not run") &&
         expect(iggy3d::nearlyEqual(after.body.velocityMetersPerSecond,
                                    {0.0F, -2.0F, 0.0F}),
                "velocity mutation remains honest") &&
         expect(iggy3d::nearlyEqual(after.body.positionMeters, {}),
                "position not integrated after collision failure");
}

bool deltaApplyFailureStopsBeforePositionPhase() {
  PhysicsWorld world = makeWorld();
  const iggy3d::PhysicsBodyStoreResult dynamic =
      addBoundBody(world, {}, iggy3d::PhysicsBodyMotionKind::Dynamic);
  (void)addBoundBody(world, {0.0F, -0.75F, 0.0F},
                     iggy3d::PhysicsBodyMotionKind::Static);
  iggy3d::PhysicsAabbStepConfig config = stepConfig();
  config.deltaApplyConfig.maxPositionDeltaMeters = 0.01F;

  const iggy3d::PhysicsAabbStepResult result =
      iggy3d::stepPhysicsAabbWorld(request(world, config));
  const iggy3d::PhysicsBodyStoreResult after = world.bodies.read(dynamic.id);

  return expect(!result.ok, "delta apply cap fails") &&
         expect(result.reasonCode == "physics_aabb_step_delta_apply_failed",
                "delta apply failure reason") &&
         expect(result.upstreamReasonCode == "physics_body_delta_invalid_delta",
                "delta apply upstream reason") &&
         expect(result.collisionBatch.ok, "batch completed before apply") &&
         expect(result.contactCount == 1U, "apply failure has contact proof") &&
         expect(result.appliedPositionCount == 0U,
                "no position deltas applied") &&
         expect(result.appliedVelocityCount == 0U,
                "no velocity deltas applied") &&
         expect(!result.positionPhase.ok, "position phase skipped") &&
         expect(iggy3d::nearlyEqual(after.body.velocityMetersPerSecond,
                                    {0.0F, -2.0F, 0.0F}),
                "velocity phase remains") &&
         expect(iggy3d::nearlyEqual(after.body.positionMeters, {}),
                "position phase did not run");
}

bool pipelineMatchesManualStageOrdering() {
  PhysicsWorld pipeline = makeWorld();
  (void)addBoundBody(pipeline, {}, iggy3d::PhysicsBodyMotionKind::Dynamic);
  (void)addBoundBody(pipeline, {0.0F, -0.75F, 0.0F},
                     iggy3d::PhysicsBodyMotionKind::Static);
  PhysicsWorld manual = pipeline;
  const iggy3d::PhysicsAabbStepConfig config = stepConfig();

  const iggy3d::PhysicsAabbStepResult pipelineResult =
      iggy3d::stepPhysicsAabbWorld(request(pipeline, config));

  const iggy3d::PhysicsStepResult velocity =
      iggy3d::integratePhysicsBodyVelocities(&manual.bodies,
                                             config.stepConfig);
  iggy3d::PhysicsAabbCollisionBatchRequest batchRequest;
  batchRequest.bodies = &manual.bodies;
  batchRequest.shapes = &manual.shapes;
  batchRequest.materials = &manual.materials;
  batchRequest.bindings = &manual.bindings;
  batchRequest.config = config.collisionConfig;
  const iggy3d::PhysicsAabbCollisionBatchResult batch =
      iggy3d::planPhysicsAabbCollisionBatch(batchRequest);
  const iggy3d::PhysicsBodyDeltaApplyResult apply =
      iggy3d::applyPhysicsBodyDeltas(&manual.bodies, batch.accumulator,
                                     config.deltaApplyConfig);
  const iggy3d::PhysicsStepResult position =
      iggy3d::integratePhysicsBodyPositions(&manual.bodies,
                                            config.stepConfig);

  return expect(pipelineResult.ok, "pipeline step ok") &&
         expect(velocity.ok, "manual velocity ok") &&
         expect(batch.ok, "manual batch ok") &&
         expect(apply.ok, "manual apply ok") &&
         expect(position.ok, "manual position ok") &&
         expect(bodyStateMatches(pipeline.bodies, manual.bodies),
                "pipeline matches manual staged calls") &&
         expect(pipelineResult.appliedPositionCount ==
                    apply.appliedPositionCount,
                "pipeline apply position summary") &&
         expect(pipelineResult.appliedVelocityCount ==
                    apply.appliedVelocityCount,
                "pipeline apply velocity summary");
}

}  // namespace

int main() {
  const bool ok = stableNamesAreLowerSnake() &&
                  missingStoreAndInvalidConfigsRejectWithoutMutation() &&
                  noCollisionBodyFallsThroughVelocityAndPositionPhases() &&
                  emptyBindingsStillIntegrateWithoutCollisions() &&
                  dynamicBodyAgainstStaticFloorAppliesCollisionBeforePosition() &&
                  collisionBatchFailureStopsAfterVelocityPhase() &&
                  deltaApplyFailureStopsBeforePositionPhase() &&
                  pipelineMatchesManualStageOrdering();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
