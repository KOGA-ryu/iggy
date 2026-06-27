#include "runtime/physics/PhysicsStep.hpp"

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

iggy3d::PhysicsBodyDescriptor body(iggy3d::PhysicsBodyMotionKind motion,
                                   iggy3d::Vec3 position,
                                   iggy3d::Vec3 velocity,
                                   float massKilograms) {
  iggy3d::PhysicsBodyDescriptor descriptor;
  descriptor.motion = motion;
  descriptor.positionMeters = position;
  descriptor.velocityMetersPerSecond = velocity;
  descriptor.massKilograms = massKilograms;
  return descriptor;
}

bool stableNamesAreLowerSnake() {
  return expect(iggy3d::physicsStepStatusName(
                    iggy3d::PhysicsStepStatus::Stepped) ==
                    "physics_step_stepped",
                "stepped status") &&
         expect(iggy3d::physicsStepStatusName(
                    iggy3d::PhysicsStepStatus::InvalidGravity) ==
                    "physics_step_invalid_gravity",
                "invalid gravity status");
}

bool invalidInputsRejectWithoutMutation() {
  iggy3d::PhysicsBodyStore store;
  const iggy3d::PhysicsBodyStoreResult added = store.add(body(
      iggy3d::PhysicsBodyMotionKind::Dynamic, {}, {1.0F, 0.0F, 0.0F}, 1.0F));

  iggy3d::PhysicsStepConfig invalidSeconds;
  invalidSeconds.stepSeconds = 0.0F;
  iggy3d::PhysicsStepConfig invalidGravity;
  invalidGravity.gravityMetersPerSecondSquared.y =
      std::numeric_limits<float>::infinity();

  const iggy3d::PhysicsStepResult missing =
      iggy3d::stepPhysicsBodies(nullptr, {});
  const iggy3d::PhysicsStepResult seconds =
      iggy3d::stepPhysicsBodies(&store, invalidSeconds);
  const iggy3d::PhysicsStepResult gravity =
      iggy3d::stepPhysicsBodies(&store, invalidGravity);
  const iggy3d::PhysicsBodyStoreResult unchanged = store.read(added.id);

  return expect(!missing.ok, "missing store rejected") &&
         expect(missing.reasonCode == "physics_step_missing_store",
                "missing store reason") &&
         expect(!seconds.ok, "invalid seconds rejected") &&
         expect(seconds.reasonCode == "physics_step_invalid_step_seconds",
                "invalid seconds reason") &&
         expect(!gravity.ok, "invalid gravity rejected") &&
         expect(gravity.reasonCode == "physics_step_invalid_gravity",
                "invalid gravity reason") &&
         expect(unchanged.ok, "unchanged read ok") &&
         expect(iggy3d::nearlyEqual(unchanged.body.positionMeters, {}),
                "invalid step does not move position") &&
         expect(iggy3d::nearlyEqual(unchanged.body.velocityMetersPerSecond,
                                    {1.0F, 0.0F, 0.0F}),
                "invalid step does not move velocity");
}

bool dynamicBodiesUseSemiImplicitGravityIntegration() {
  iggy3d::PhysicsBodyStore store;
  const iggy3d::PhysicsBodyStoreResult added = store.add(body(
      iggy3d::PhysicsBodyMotionKind::Dynamic, {}, {1.0F, 0.0F, 0.0F}, 2.0F));
  iggy3d::PhysicsStepConfig config;
  config.stepSeconds = 0.5F;
  config.gravityMetersPerSecondSquared = {0.0F, -10.0F, 0.0F};

  const iggy3d::PhysicsStepResult stepped =
      iggy3d::stepPhysicsBodies(&store, config);
  const iggy3d::PhysicsBodyStoreResult bodyAfter = store.read(added.id);

  return expect(stepped.ok, "dynamic step ok") &&
         expect(stepped.bodyCount == 1U, "dynamic body count") &&
         expect(stepped.dynamicBodyCount == 1U, "dynamic count") &&
         expect(stepped.integratedBodyCount == 1U, "integrated count") &&
         expect(stepped.gravityAppliedBodyCount == 1U, "gravity count") &&
         expect(bodyAfter.ok, "dynamic read ok") &&
         expect(iggy3d::nearlyEqual(bodyAfter.body.velocityMetersPerSecond,
                                    {1.0F, -5.0F, 0.0F}),
                "dynamic velocity") &&
         expect(iggy3d::nearlyEqual(bodyAfter.body.positionMeters,
                                    {0.5F, -2.5F, 0.0F}),
                "dynamic position");
}

bool staticBodiesRemainUnchanged() {
  iggy3d::PhysicsBodyStore store;
  const iggy3d::PhysicsBodyStoreResult added = store.add(body(
      iggy3d::PhysicsBodyMotionKind::Static,
      {3.0F, 2.0F, 1.0F},
      {9.0F, 8.0F, 7.0F},
      0.0F));

  const iggy3d::PhysicsStepResult stepped =
      iggy3d::stepPhysicsBodies(&store, {});
  const iggy3d::PhysicsBodyStoreResult bodyAfter = store.read(added.id);

  return expect(stepped.ok, "static step ok") &&
         expect(stepped.staticBodyCount == 1U, "static count") &&
         expect(stepped.integratedBodyCount == 0U, "static integrated count") &&
         expect(iggy3d::nearlyEqual(bodyAfter.body.positionMeters,
                                    {3.0F, 2.0F, 1.0F}),
                "static position unchanged") &&
         expect(iggy3d::nearlyEqual(bodyAfter.body.velocityMetersPerSecond,
                                    {9.0F, 8.0F, 7.0F}),
                "static velocity unchanged");
}

bool kinematicBodiesMoveWithoutGravity() {
  iggy3d::PhysicsBodyStore store;
  const iggy3d::PhysicsBodyStoreResult added = store.add(body(
      iggy3d::PhysicsBodyMotionKind::Kinematic,
      {1.0F, 1.0F, 1.0F},
      {2.0F, 3.0F, 4.0F},
      0.0F));
  iggy3d::PhysicsStepConfig config;
  config.stepSeconds = 0.25F;
  config.gravityMetersPerSecondSquared = {0.0F, -100.0F, 0.0F};

  const iggy3d::PhysicsStepResult stepped =
      iggy3d::stepPhysicsBodies(&store, config);
  const iggy3d::PhysicsBodyStoreResult bodyAfter = store.read(added.id);

  return expect(stepped.ok, "kinematic step ok") &&
         expect(stepped.kinematicBodyCount == 1U, "kinematic count") &&
         expect(stepped.integratedBodyCount == 1U, "kinematic integrated") &&
         expect(stepped.gravityAppliedBodyCount == 0U, "kinematic no gravity") &&
         expect(iggy3d::nearlyEqual(bodyAfter.body.positionMeters,
                                    {1.5F, 1.75F, 2.0F}),
                "kinematic position") &&
         expect(iggy3d::nearlyEqual(bodyAfter.body.velocityMetersPerSecond,
                                    {2.0F, 3.0F, 4.0F}),
                "kinematic velocity unchanged");
}

bool mixedStoreReportsDeterministicCounts() {
  iggy3d::PhysicsBodyStore store;
  (void)store.add(body(iggy3d::PhysicsBodyMotionKind::Static, {}, {}, 0.0F));
  (void)store.add(body(iggy3d::PhysicsBodyMotionKind::Dynamic,
                       {},
                       {},
                       1.0F));
  (void)store.add(body(iggy3d::PhysicsBodyMotionKind::Kinematic,
                       {},
                       {1.0F, 0.0F, 0.0F},
                       0.0F));

  const iggy3d::PhysicsStepResult stepped =
      iggy3d::stepPhysicsBodies(&store, {});

  return expect(stepped.ok, "mixed step ok") &&
         expect(stepped.bodyCount == 3U, "mixed body count") &&
         expect(stepped.staticBodyCount == 1U, "mixed static count") &&
         expect(stepped.dynamicBodyCount == 1U, "mixed dynamic count") &&
         expect(stepped.kinematicBodyCount == 1U, "mixed kinematic count") &&
         expect(stepped.integratedBodyCount == 2U, "mixed integrated count") &&
         expect(stepped.gravityAppliedBodyCount == 1U, "mixed gravity count");
}

}  // namespace

int main() {
  const bool ok = stableNamesAreLowerSnake() &&
                  invalidInputsRejectWithoutMutation() &&
                  dynamicBodiesUseSemiImplicitGravityIntegration() &&
                  staticBodiesRemainUnchanged() &&
                  kinematicBodiesMoveWithoutGravity() &&
                  mixedStoreReportsDeterministicCounts();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
