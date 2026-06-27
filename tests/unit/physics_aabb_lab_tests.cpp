#include "runtime/physics/PhysicsAabbStep.hpp"

#include <cmath>
#include <cstdint>
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

float absValue(float value) {
  return std::fabs(value);
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

iggy3d::PhysicsShapeDescriptor shapeDescriptor(
    iggy3d::PhysicsShapeKind kind = iggy3d::PhysicsShapeKind::Box) {
  iggy3d::PhysicsShapeDescriptor descriptor;
  descriptor.kind = kind;
  descriptor.halfExtentsMeters = {0.5F, 0.5F, 0.5F};
  return descriptor;
}

iggy3d::PhysicsMaterialDescriptor materialDescriptor(
    std::string key,
    float dynamicFriction = 0.4F,
    float restitution = 0.0F,
    std::uint32_t flags = 0U) {
  iggy3d::PhysicsMaterialDescriptor descriptor;
  descriptor.key = std::move(key);
  descriptor.staticFriction = dynamicFriction;
  descriptor.dynamicFriction = dynamicFriction;
  descriptor.restitution = restitution;
  descriptor.dampingMultiplier = 1.0F;
  descriptor.weightClass = iggy3d::PhysicsWeightClass::Medium;
  descriptor.flags = flags;
  return descriptor;
}

iggy3d::PhysicsBodyShapeBindingDescriptor bindingDescriptor(
    iggy3d::PhysicsBodyId bodyId,
    iggy3d::PhysicsShapeId shapeId,
    iggy3d::PhysicsMaterialId materialId) {
  iggy3d::PhysicsBodyShapeBindingDescriptor descriptor;
  descriptor.bodyId = bodyId;
  descriptor.shapeId = shapeId;
  descriptor.materialId = materialId;
  return descriptor;
}

struct PhysicsLabWorld {
  iggy3d::PhysicsBodyStore bodies;
  iggy3d::PhysicsShapeStore shapes;
  iggy3d::PhysicsMaterialTable materials;
  iggy3d::PhysicsBodyShapeBindingStore bindings;
};

struct LabBody {
  iggy3d::PhysicsBodyId bodyId;
  iggy3d::PhysicsShapeId shapeId;
  iggy3d::PhysicsMaterialId materialId;
};

iggy3d::PhysicsMaterialId addMaterial(
    PhysicsLabWorld& world,
    std::string key,
    float dynamicFriction = 0.4F,
    float restitution = 0.0F,
    std::uint32_t flags = 0U) {
  const iggy3d::PhysicsMaterialTableResult added =
      world.materials.add(materialDescriptor(std::move(key),
                                             dynamicFriction,
                                             restitution,
                                             flags));
  return added.id;
}

LabBody addBoundBody(
    PhysicsLabWorld& world,
    iggy3d::Vec3 position,
    iggy3d::PhysicsBodyMotionKind motion,
    iggy3d::PhysicsMaterialId materialId,
    iggy3d::Vec3 velocity = {},
    iggy3d::PhysicsShapeKind shapeKind = iggy3d::PhysicsShapeKind::Box) {
  const iggy3d::PhysicsBodyStoreResult body =
      world.bodies.add(bodyDescriptor(position, motion, velocity));
  const iggy3d::PhysicsShapeStoreResult shape =
      world.shapes.add(shapeDescriptor(shapeKind));
  (void)world.bindings.add(bindingDescriptor(body.id, shape.id, materialId));
  return {body.id, shape.id, materialId};
}

iggy3d::PhysicsAabbStepConfig stepConfig(
    iggy3d::Vec3 gravity = {0.0F, -4.0F, 0.0F},
    float stepSeconds = 0.5F) {
  iggy3d::PhysicsAabbStepConfig config;
  config.stepConfig.stepSeconds = stepSeconds;
  config.stepConfig.gravityMetersPerSecondSquared = gravity;
  return config;
}

iggy3d::PhysicsAabbStepRequest request(
    PhysicsLabWorld& world,
    const iggy3d::PhysicsAabbStepConfig& config) {
  iggy3d::PhysicsAabbStepRequest result;
  result.bodies = &world.bodies;
  result.shapes = &world.shapes;
  result.materials = &world.materials;
  result.bindings = &world.bindings;
  result.config = config;
  return result;
}

iggy3d::PhysicsAabbStepResult stepWorld(
    PhysicsLabWorld& world,
    const iggy3d::PhysicsAabbStepConfig& config) {
  return iggy3d::stepPhysicsAabbWorld(request(world, config));
}

bool bodyNearlyMatches(const iggy3d::PhysicsBodyStore& lhs,
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

bool allBodiesFinite(const iggy3d::PhysicsBodyStore& bodies) {
  for (iggy3d::Vec3 position : bodies.positions()) {
    if (!iggy3d::isFinite(position)) {
      return false;
    }
  }
  for (iggy3d::Vec3 velocity : bodies.velocities()) {
    if (!iggy3d::isFinite(velocity)) {
      return false;
    }
  }
  return true;
}

bool freeFallWithoutBindingsUsesVelocityAndPositionPhases() {
  PhysicsLabWorld world;
  const iggy3d::PhysicsBodyStoreResult body =
      world.bodies.add(bodyDescriptor({}, iggy3d::PhysicsBodyMotionKind::Dynamic));

  const iggy3d::PhysicsAabbStepResult result =
      stepWorld(world, stepConfig());
  const iggy3d::PhysicsBodyStoreResult after = world.bodies.read(body.id);

  return expect(result.ok, "free fall step ok") &&
         expect(result.colliderCount == 0U, "free fall no colliders") &&
         expect(result.broadphasePairCount == 0U, "free fall no pairs") &&
         expect(result.contactCount == 0U, "free fall no contacts") &&
         expect(result.deltaApply.status == iggy3d::PhysicsBodyDeltaStatus::NoOp,
                "free fall delta no-op") &&
         expect(result.velocityPhase.velocityIntegratedBodyCount == 1U,
                "free fall velocity phase") &&
         expect(result.positionPhase.positionIntegratedBodyCount == 1U,
                "free fall position phase") &&
         expect(iggy3d::nearlyEqual(after.body.velocityMetersPerSecond,
                                    {0.0F, -2.0F, 0.0F}),
                "free fall velocity") &&
         expect(iggy3d::nearlyEqual(after.body.positionMeters,
                                    {0.0F, -1.0F, 0.0F}),
                "free fall position");
}

bool fallingDynamicBoxHitsStaticFloor() {
  PhysicsLabWorld world;
  const iggy3d::PhysicsMaterialId stone = addMaterial(world, "stone");
  const LabBody dynamic = addBoundBody(
      world, {}, iggy3d::PhysicsBodyMotionKind::Dynamic, stone);
  const LabBody floor = addBoundBody(
      world, {0.0F, -0.75F, 0.0F},
      iggy3d::PhysicsBodyMotionKind::Static, stone);
  iggy3d::PhysicsBodyStore pure = world.bodies;

  const iggy3d::PhysicsStepResult pureStep =
      iggy3d::stepPhysicsBodies(&pure, stepConfig().stepConfig);
  const iggy3d::PhysicsAabbStepResult result =
      stepWorld(world, stepConfig());
  const iggy3d::PhysicsBodyStoreResult dynamicAfter =
      world.bodies.read(dynamic.bodyId);
  const iggy3d::PhysicsBodyStoreResult floorAfter =
      world.bodies.read(floor.bodyId);

  return expect(pureStep.ok, "floor pure step ok") &&
         expect(result.ok, "floor collision step ok") &&
         expect(result.contactCount >= 1U, "floor contact happened") &&
         expect(result.solvePlanCount >= 1U, "floor solve happened") &&
         expect(result.appliedPositionCount >= 1U,
                "floor position delta applied") &&
         expect(dynamicAfter.body.positionMeters.y > pure.positions()[0].y,
                "floor correction moves above pure fall") &&
         expect(dynamicAfter.body.velocityMetersPerSecond.y >
                    pure.velocities()[0].y,
                "floor collision reduces downward velocity") &&
         expect(dynamicAfter.body.velocityMetersPerSecond.y >= -0.001F,
                "floor downward velocity removed") &&
         expect(iggy3d::nearlyEqual(floorAfter.body.positionMeters,
                                    {0.0F, -0.75F, 0.0F}),
                "floor static position unchanged") &&
         expect(iggy3d::nearlyEqual(floorAfter.body.velocityMetersPerSecond,
                                    {}),
                "floor static velocity unchanged");
}

bool movingDynamicBoxHitsStaticWall() {
  PhysicsLabWorld world;
  const iggy3d::PhysicsMaterialId stone = addMaterial(world, "stone");
  const LabBody dynamic = addBoundBody(
      world, {}, iggy3d::PhysicsBodyMotionKind::Dynamic, stone,
      {2.0F, 0.0F, 0.0F});
  const LabBody wall = addBoundBody(
      world, {0.75F, 0.0F, 0.0F},
      iggy3d::PhysicsBodyMotionKind::Static, stone);
  const iggy3d::PhysicsAabbStepConfig config = stepConfig({}, 0.5F);

  const iggy3d::PhysicsAabbStepResult result = stepWorld(world, config);
  const iggy3d::PhysicsBodyStoreResult dynamicAfter =
      world.bodies.read(dynamic.bodyId);
  const iggy3d::PhysicsBodyStoreResult wallAfter =
      world.bodies.read(wall.bodyId);

  return expect(result.ok, "wall collision step ok") &&
         expect(result.contactCount >= 1U, "wall contact happened") &&
         expect(result.appliedPositionCount >= 1U,
                "wall position delta applied") &&
         expect(absValue(dynamicAfter.body.velocityMetersPerSecond.x) < 2.0F,
                "wall reduces x velocity magnitude") &&
         expect(dynamicAfter.body.velocityMetersPerSecond.x <= 0.001F,
                "wall velocity no longer points into wall") &&
         expect(dynamicAfter.body.positionMeters.x < 0.0F,
                "wall correction moves body away") &&
         expect(iggy3d::nearlyEqual(wallAfter.body.positionMeters,
                                    {0.75F, 0.0F, 0.0F}),
                "wall static position unchanged") &&
         expect(iggy3d::nearlyEqual(wallAfter.body.velocityMetersPerSecond,
                                    {}),
                "wall static velocity unchanged");
}

bool frictionReducesTangentialSlideAgainstSurface() {
  PhysicsLabWorld world;
  const iggy3d::PhysicsMaterialId rough =
      addMaterial(world, "rough", 1.0F);
  const LabBody dynamic = addBoundBody(
      world, {}, iggy3d::PhysicsBodyMotionKind::Dynamic, rough,
      {4.0F, -2.0F, 0.0F});
  (void)addBoundBody(world, {0.0F, -0.75F, 0.0F},
                     iggy3d::PhysicsBodyMotionKind::Static, rough);
  iggy3d::PhysicsBodyStore noCollisionBaseline = world.bodies;
  const iggy3d::PhysicsAabbStepConfig config = stepConfig({}, 0.5F);

  const iggy3d::PhysicsStepResult baseline =
      iggy3d::stepPhysicsBodies(&noCollisionBaseline, config.stepConfig);
  const iggy3d::PhysicsAabbStepResult result = stepWorld(world, config);
  const iggy3d::PhysicsBodyStoreResult after =
      world.bodies.read(dynamic.bodyId);

  return expect(baseline.ok, "friction baseline ok") &&
         expect(result.ok, "friction step ok") &&
         expect(result.contactCount >= 1U, "friction contact happened") &&
         expect(result.solvePlanCount >= 1U, "friction solve happened") &&
         expect(result.appliedVelocityCount >= 1U,
                "friction velocity delta applied") &&
         expect(absValue(after.body.velocityMetersPerSecond.x) <
                    absValue(noCollisionBaseline.velocities()[0].x),
                "friction reduces tangential x velocity");
}

bool triggerContactDoesNotApplyCollisionDeltas() {
  PhysicsLabWorld world;
  const iggy3d::PhysicsMaterialId stone = addMaterial(world, "stone");
  const LabBody dynamic = addBoundBody(
      world, {}, iggy3d::PhysicsBodyMotionKind::Dynamic, stone);
  (void)addBoundBody(world,
                     {0.0F, -0.75F, 0.0F},
                     iggy3d::PhysicsBodyMotionKind::Static,
                     stone,
                     {},
                     iggy3d::PhysicsShapeKind::TriggerAabb);
  iggy3d::PhysicsBodyStore pure = world.bodies;

  const iggy3d::PhysicsStepResult pureStep =
      iggy3d::stepPhysicsBodies(&pure, stepConfig().stepConfig);
  const iggy3d::PhysicsAabbStepResult result =
      stepWorld(world, stepConfig());
  const iggy3d::PhysicsBodyStoreResult after =
      world.bodies.read(dynamic.bodyId);

  return expect(pureStep.ok, "trigger pure step ok") &&
         expect(result.ok, "trigger step ok") &&
         expect(result.contactCount == 1U, "trigger contact reported") &&
         expect(result.collisionBatch.sensorContactCount == 1U,
                "trigger sensor count") &&
         expect(result.appliedPositionCount == 0U,
                "trigger no position deltas") &&
         expect(result.appliedVelocityCount == 0U,
                "trigger no velocity deltas") &&
         expect(iggy3d::nearlyEqual(after.body.positionMeters,
                                    pure.positions()[0]),
                "trigger matches pure position") &&
         expect(iggy3d::nearlyEqual(after.body.velocityMetersPerSecond,
                                    pure.velocities()[0]),
                "trigger matches pure velocity");
}

PhysicsLabWorld makeDeterministicWorld() {
  PhysicsLabWorld world;
  const iggy3d::PhysicsMaterialId stone = addMaterial(world, "stone");
  (void)addBoundBody(world, {}, iggy3d::PhysicsBodyMotionKind::Dynamic, stone);
  (void)addBoundBody(world, {0.0F, -0.75F, 0.0F},
                     iggy3d::PhysicsBodyMotionKind::Static, stone);
  (void)addBoundBody(world,
                     {2.0F, 0.0F, 0.0F},
                     iggy3d::PhysicsBodyMotionKind::Dynamic,
                     stone,
                     {-1.0F, 0.0F, 0.0F});
  return world;
}

bool twoIdenticalRunsAreDeterministic() {
  PhysicsLabWorld first = makeDeterministicWorld();
  PhysicsLabWorld second = makeDeterministicWorld();
  const iggy3d::PhysicsAabbStepConfig config = stepConfig();

  bool summariesMatch = true;
  for (std::size_t stepIndex = 0U; stepIndex < 3U; ++stepIndex) {
    const iggy3d::PhysicsAabbStepResult firstStep =
        stepWorld(first, config);
    const iggy3d::PhysicsAabbStepResult secondStep =
        stepWorld(second, config);
    summariesMatch = summariesMatch && firstStep.ok && secondStep.ok &&
                     firstStep.colliderCount == secondStep.colliderCount &&
                     firstStep.broadphasePairCount ==
                         secondStep.broadphasePairCount &&
                     firstStep.contactCount == secondStep.contactCount &&
                     firstStep.solvePlanCount == secondStep.solvePlanCount &&
                     firstStep.accumulatedPlanCount ==
                         secondStep.accumulatedPlanCount &&
                     firstStep.appliedPositionCount ==
                         secondStep.appliedPositionCount &&
                     firstStep.appliedVelocityCount ==
                         secondStep.appliedVelocityCount &&
                     bodyNearlyMatches(first.bodies, second.bodies);
  }

  return expect(summariesMatch, "deterministic summaries and body states");
}

bool simpleStackDoesNotExplodeOrProduceNonfiniteState() {
  PhysicsLabWorld world;
  const iggy3d::PhysicsMaterialId stone = addMaterial(world, "stone");
  (void)addBoundBody(world, {0.0F, -0.75F, 0.0F},
                     iggy3d::PhysicsBodyMotionKind::Static, stone);
  (void)addBoundBody(world, {}, iggy3d::PhysicsBodyMotionKind::Dynamic, stone);
  (void)addBoundBody(world, {0.0F, 0.85F, 0.0F},
                     iggy3d::PhysicsBodyMotionKind::Dynamic, stone);

  std::size_t contactCount = 0U;
  std::size_t solvePlanCount = 0U;
  bool stepsOk = true;
  for (std::size_t stepIndex = 0U; stepIndex < 3U; ++stepIndex) {
    const iggy3d::PhysicsAabbStepResult result =
        stepWorld(world, stepConfig());
    stepsOk = stepsOk && result.ok;
    contactCount += result.contactCount;
    solvePlanCount += result.solvePlanCount;
  }

  return expect(stepsOk, "stack steps ok") &&
         expect(contactCount > 0U, "stack contacts happened") &&
         expect(solvePlanCount > 0U, "stack solves happened") &&
         expect(allBodiesFinite(world.bodies), "stack body states finite");
}

}  // namespace

int main() {
  const bool ok = freeFallWithoutBindingsUsesVelocityAndPositionPhases() &&
                  fallingDynamicBoxHitsStaticFloor() &&
                  movingDynamicBoxHitsStaticWall() &&
                  frictionReducesTangentialSlideAgainstSurface() &&
                  triggerContactDoesNotApplyCollisionDeltas() &&
                  twoIdenticalRunsAreDeterministic() &&
                  simpleStackDoesNotExplodeOrProduceNonfiniteState();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
