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

bool phaseInvalidInputsRejectWithoutMutation() {
  iggy3d::PhysicsBodyStore velocityStore;
  const iggy3d::PhysicsBodyStoreResult velocityBody =
      velocityStore.add(body(iggy3d::PhysicsBodyMotionKind::Dynamic,
                             {},
                             {1.0F, 0.0F, 0.0F},
                             1.0F));
  iggy3d::PhysicsStepConfig invalidSeconds;
  invalidSeconds.stepSeconds = 0.0F;
  iggy3d::PhysicsStepConfig invalidGravity;
  invalidGravity.gravityMetersPerSecondSquared.x =
      std::numeric_limits<float>::infinity();

  const iggy3d::PhysicsStepResult missingVelocity =
      iggy3d::integratePhysicsBodyVelocities(nullptr, {});
  const iggy3d::PhysicsStepResult velocitySeconds =
      iggy3d::integratePhysicsBodyVelocities(&velocityStore, invalidSeconds);
  const iggy3d::PhysicsStepResult velocityGravity =
      iggy3d::integratePhysicsBodyVelocities(&velocityStore, invalidGravity);
  const iggy3d::PhysicsBodyStoreResult velocityAfter =
      velocityStore.read(velocityBody.id);

  iggy3d::PhysicsBodyStore positionStore;
  const iggy3d::PhysicsBodyStoreResult positionBody =
      positionStore.add(body(iggy3d::PhysicsBodyMotionKind::Dynamic,
                             {},
                             {1.0F, 0.0F, 0.0F},
                             1.0F));
  const iggy3d::PhysicsStepResult missingPosition =
      iggy3d::integratePhysicsBodyPositions(nullptr, {});
  const iggy3d::PhysicsStepResult positionSeconds =
      iggy3d::integratePhysicsBodyPositions(&positionStore, invalidSeconds);
  const iggy3d::PhysicsStepResult positionGravity =
      iggy3d::integratePhysicsBodyPositions(&positionStore, invalidGravity);
  const iggy3d::PhysicsBodyStoreResult positionAfter =
      positionStore.read(positionBody.id);

  return expect(!missingVelocity.ok, "missing velocity store rejected") &&
         expect(missingVelocity.reasonCode == "physics_step_missing_store",
                "missing velocity reason") &&
         expect(!velocitySeconds.ok, "velocity invalid seconds rejected") &&
         expect(velocitySeconds.reasonCode ==
                    "physics_step_invalid_step_seconds",
                "velocity invalid seconds reason") &&
         expect(!velocityGravity.ok, "velocity invalid gravity rejected") &&
         expect(velocityGravity.reasonCode == "physics_step_invalid_gravity",
                "velocity invalid gravity reason") &&
         expect(iggy3d::nearlyEqual(velocityAfter.body.positionMeters, {}),
                "velocity invalid keeps position") &&
         expect(iggy3d::nearlyEqual(velocityAfter.body.velocityMetersPerSecond,
                                    {1.0F, 0.0F, 0.0F}),
                "velocity invalid keeps velocity") &&
         expect(!missingPosition.ok, "missing position store rejected") &&
         expect(missingPosition.reasonCode == "physics_step_missing_store",
                "missing position reason") &&
         expect(!positionSeconds.ok, "position invalid seconds rejected") &&
         expect(positionSeconds.reasonCode ==
                    "physics_step_invalid_step_seconds",
                "position invalid seconds reason") &&
         expect(!positionGravity.ok, "position invalid gravity rejected") &&
         expect(positionGravity.reasonCode == "physics_step_invalid_gravity",
                "position invalid gravity reason") &&
         expect(iggy3d::nearlyEqual(positionAfter.body.positionMeters, {}),
                "position invalid keeps position") &&
         expect(iggy3d::nearlyEqual(positionAfter.body.velocityMetersPerSecond,
                                    {1.0F, 0.0F, 0.0F}),
                "position invalid keeps velocity");
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
         expect(stepped.velocityIntegratedBodyCount == 1U,
                "velocity count") &&
         expect(stepped.positionIntegratedBodyCount == 1U,
                "position count") &&
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
         expect(stepped.velocityIntegratedBodyCount == 0U,
                "kinematic no velocity phase") &&
         expect(stepped.positionIntegratedBodyCount == 1U,
                "kinematic position phase") &&
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
         expect(stepped.gravityAppliedBodyCount == 1U, "mixed gravity count") &&
         expect(stepped.velocityIntegratedBodyCount == 1U,
                "mixed velocity count") &&
         expect(stepped.positionIntegratedBodyCount == 2U,
                "mixed position count");
}

bool velocityPhaseAppliesOnlyGravityVelocity() {
  iggy3d::PhysicsBodyStore store;
  const iggy3d::PhysicsBodyStoreResult dynamicBody =
      store.add(body(iggy3d::PhysicsBodyMotionKind::Dynamic,
                     {1.0F, 2.0F, 3.0F},
                     {1.0F, 0.0F, 0.0F},
                     1.0F));
  const iggy3d::PhysicsBodyStoreResult kinematicBody =
      store.add(body(iggy3d::PhysicsBodyMotionKind::Kinematic,
                     {4.0F, 5.0F, 6.0F},
                     {0.0F, 2.0F, 0.0F},
                     0.0F));
  const iggy3d::PhysicsBodyStoreResult staticBody =
      store.add(body(iggy3d::PhysicsBodyMotionKind::Static,
                     {7.0F, 8.0F, 9.0F},
                     {0.0F, 0.0F, 3.0F},
                     0.0F));
  iggy3d::PhysicsStepConfig config;
  config.stepSeconds = 0.25F;
  config.gravityMetersPerSecondSquared = {0.0F, -8.0F, 0.0F};

  const iggy3d::PhysicsStepResult stepped =
      iggy3d::integratePhysicsBodyVelocities(&store, config);
  const iggy3d::PhysicsBodyStoreResult dynamicAfter =
      store.read(dynamicBody.id);
  const iggy3d::PhysicsBodyStoreResult kinematicAfter =
      store.read(kinematicBody.id);
  const iggy3d::PhysicsBodyStoreResult staticAfter =
      store.read(staticBody.id);

  return expect(stepped.ok, "velocity phase ok") &&
         expect(stepped.bodyCount == 3U, "velocity phase body count") &&
         expect(stepped.dynamicBodyCount == 1U, "velocity phase dynamic") &&
         expect(stepped.kinematicBodyCount == 1U,
                "velocity phase kinematic") &&
         expect(stepped.staticBodyCount == 1U, "velocity phase static") &&
         expect(stepped.integratedBodyCount == 0U,
                "velocity phase no position integration") &&
         expect(stepped.gravityAppliedBodyCount == 1U,
                "velocity phase gravity count") &&
         expect(stepped.velocityIntegratedBodyCount == 1U,
                "velocity phase velocity count") &&
         expect(stepped.positionIntegratedBodyCount == 0U,
                "velocity phase position count") &&
         expect(iggy3d::nearlyEqual(dynamicAfter.body.velocityMetersPerSecond,
                                    {1.0F, -2.0F, 0.0F}),
                "dynamic velocity updated") &&
         expect(iggy3d::nearlyEqual(dynamicAfter.body.positionMeters,
                                    {1.0F, 2.0F, 3.0F}),
                "dynamic position unchanged") &&
         expect(iggy3d::nearlyEqual(
                    kinematicAfter.body.velocityMetersPerSecond,
                    {0.0F, 2.0F, 0.0F}),
                "kinematic velocity unchanged") &&
         expect(iggy3d::nearlyEqual(kinematicAfter.body.positionMeters,
                                    {4.0F, 5.0F, 6.0F}),
                "kinematic position unchanged") &&
         expect(iggy3d::nearlyEqual(staticAfter.body.velocityMetersPerSecond,
                                    {0.0F, 0.0F, 3.0F}),
                "static velocity unchanged") &&
         expect(iggy3d::nearlyEqual(staticAfter.body.positionMeters,
                                    {7.0F, 8.0F, 9.0F}),
                "static position unchanged");
}

bool positionPhaseMovesDynamicAndKinematicOnly() {
  iggy3d::PhysicsBodyStore store;
  const iggy3d::PhysicsBodyStoreResult dynamicBody =
      store.add(body(iggy3d::PhysicsBodyMotionKind::Dynamic,
                     {1.0F, 2.0F, 3.0F},
                     {2.0F, 0.0F, -2.0F},
                     1.0F));
  const iggy3d::PhysicsBodyStoreResult kinematicBody =
      store.add(body(iggy3d::PhysicsBodyMotionKind::Kinematic,
                     {4.0F, 5.0F, 6.0F},
                     {0.0F, 4.0F, 0.0F},
                     0.0F));
  const iggy3d::PhysicsBodyStoreResult staticBody =
      store.add(body(iggy3d::PhysicsBodyMotionKind::Static,
                     {7.0F, 8.0F, 9.0F},
                     {0.0F, 0.0F, 5.0F},
                     0.0F));
  iggy3d::PhysicsStepConfig config;
  config.stepSeconds = 0.5F;
  config.gravityMetersPerSecondSquared = {0.0F, -100.0F, 0.0F};

  const iggy3d::PhysicsStepResult stepped =
      iggy3d::integratePhysicsBodyPositions(&store, config);
  const iggy3d::PhysicsBodyStoreResult dynamicAfter =
      store.read(dynamicBody.id);
  const iggy3d::PhysicsBodyStoreResult kinematicAfter =
      store.read(kinematicBody.id);
  const iggy3d::PhysicsBodyStoreResult staticAfter =
      store.read(staticBody.id);

  return expect(stepped.ok, "position phase ok") &&
         expect(stepped.integratedBodyCount == 2U,
                "position phase integrated count") &&
         expect(stepped.gravityAppliedBodyCount == 0U,
                "position phase no gravity") &&
         expect(stepped.velocityIntegratedBodyCount == 0U,
                "position phase no velocity count") &&
         expect(stepped.positionIntegratedBodyCount == 2U,
                "position phase position count") &&
         expect(iggy3d::nearlyEqual(dynamicAfter.body.positionMeters,
                                    {2.0F, 2.0F, 2.0F}),
                "dynamic position moved") &&
         expect(iggy3d::nearlyEqual(dynamicAfter.body.velocityMetersPerSecond,
                                    {2.0F, 0.0F, -2.0F}),
                "dynamic velocity unchanged") &&
         expect(iggy3d::nearlyEqual(kinematicAfter.body.positionMeters,
                                    {4.0F, 7.0F, 6.0F}),
                "kinematic position moved") &&
         expect(iggy3d::nearlyEqual(
                    kinematicAfter.body.velocityMetersPerSecond,
                    {0.0F, 4.0F, 0.0F}),
                "kinematic velocity unchanged") &&
         expect(iggy3d::nearlyEqual(staticAfter.body.positionMeters,
                                    {7.0F, 8.0F, 9.0F}),
                "static position unchanged") &&
         expect(iggy3d::nearlyEqual(staticAfter.body.velocityMetersPerSecond,
                                    {0.0F, 0.0F, 5.0F}),
                "static velocity unchanged");
}

bool phaseCompositionMatchesAllInOneStep() {
  iggy3d::PhysicsBodyStore allInOne;
  (void)allInOne.add(body(iggy3d::PhysicsBodyMotionKind::Static,
                          {1.0F, 1.0F, 1.0F},
                          {9.0F, 0.0F, 0.0F},
                          0.0F));
  (void)allInOne.add(body(iggy3d::PhysicsBodyMotionKind::Dynamic,
                          {},
                          {1.0F, 0.0F, 0.0F},
                          1.0F));
  (void)allInOne.add(body(iggy3d::PhysicsBodyMotionKind::Kinematic,
                          {2.0F, 0.0F, 0.0F},
                          {0.0F, 2.0F, 0.0F},
                          0.0F));
  iggy3d::PhysicsBodyStore phased = allInOne;
  iggy3d::PhysicsStepConfig config;
  config.stepSeconds = 0.5F;
  config.gravityMetersPerSecondSquared = {0.0F, -10.0F, 0.0F};

  const iggy3d::PhysicsStepResult all =
      iggy3d::stepPhysicsBodies(&allInOne, config);
  const iggy3d::PhysicsStepResult velocities =
      iggy3d::integratePhysicsBodyVelocities(&phased, config);
  const iggy3d::PhysicsStepResult positions =
      iggy3d::integratePhysicsBodyPositions(&phased, config);

  bool stateMatches = allInOne.positions().size() == phased.positions().size();
  for (std::size_t index = 0U; index < allInOne.positions().size(); ++index) {
    stateMatches = stateMatches &&
                   iggy3d::nearlyEqual(allInOne.positions()[index],
                                       phased.positions()[index]) &&
                   iggy3d::nearlyEqual(allInOne.velocities()[index],
                                       phased.velocities()[index]);
  }

  return expect(all.ok, "all-in-one ok") &&
         expect(velocities.ok, "velocity phase ok") &&
         expect(positions.ok, "position phase ok") &&
         expect(stateMatches, "phase composition matches all-in-one") &&
         expect(all.integratedBodyCount == positions.integratedBodyCount,
                "integrated count equivalent") &&
         expect(all.gravityAppliedBodyCount ==
                    velocities.gravityAppliedBodyCount,
                "gravity count equivalent") &&
         expect(all.velocityIntegratedBodyCount ==
                    velocities.velocityIntegratedBodyCount,
                "velocity count equivalent") &&
         expect(all.positionIntegratedBodyCount ==
                    positions.positionIntegratedBodyCount,
                "position count equivalent");
}

}  // namespace

int main() {
  const bool ok = stableNamesAreLowerSnake() &&
                  invalidInputsRejectWithoutMutation() &&
                  phaseInvalidInputsRejectWithoutMutation() &&
                  dynamicBodiesUseSemiImplicitGravityIntegration() &&
                  staticBodiesRemainUnchanged() &&
                  kinematicBodiesMoveWithoutGravity() &&
                  mixedStoreReportsDeterministicCounts() &&
                  velocityPhaseAppliesOnlyGravityVelocity() &&
                  positionPhaseMovesDynamicAndKinematicOnly() &&
                  phaseCompositionMatchesAllInOneStep();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
