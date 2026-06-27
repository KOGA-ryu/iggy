#include "runtime/physics/PhysicsBodyStore.hpp"

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

iggy3d::PhysicsBodyDescriptor dynamicDescriptor() {
  iggy3d::PhysicsBodyDescriptor descriptor;
  descriptor.motion = iggy3d::PhysicsBodyMotionKind::Dynamic;
  descriptor.positionMeters = {1.0F, 2.0F, 3.0F};
  descriptor.velocityMetersPerSecond = {0.5F, 0.0F, -1.0F};
  descriptor.massKilograms = 2.0F;
  return descriptor;
}

bool stableNamesAreLowerSnake() {
  return expect(iggy3d::physicsBodyMotionKindName(
                    iggy3d::PhysicsBodyMotionKind::Static) == "static",
                "static name") &&
         expect(iggy3d::physicsBodyMotionKindName(
                    iggy3d::PhysicsBodyMotionKind::Dynamic) == "dynamic",
                "dynamic name") &&
         expect(iggy3d::physicsBodyMotionKindName(
                    iggy3d::PhysicsBodyMotionKind::Kinematic) == "kinematic",
                "kinematic name") &&
         expect(iggy3d::physicsStatusName(
                    iggy3d::PhysicsStatus::InvalidVelocity) ==
                    "physics_body_invalid_velocity",
                "status name");
}

bool validationRejectsInvalidDescriptorFacts() {
  const float inf = std::numeric_limits<float>::infinity();
  iggy3d::PhysicsBodyDescriptor invalidPosition = dynamicDescriptor();
  invalidPosition.positionMeters.x = inf;
  iggy3d::PhysicsBodyDescriptor invalidVelocity = dynamicDescriptor();
  invalidVelocity.velocityMetersPerSecond.z = inf;
  iggy3d::PhysicsBodyDescriptor invalidMass = dynamicDescriptor();
  invalidMass.massKilograms = 0.0F;

  const iggy3d::PhysicsValidationResult missing =
      iggy3d::validatePhysicsBodyDescriptor(nullptr);
  const iggy3d::PhysicsValidationResult position =
      iggy3d::validatePhysicsBodyDescriptor(&invalidPosition);
  const iggy3d::PhysicsValidationResult velocity =
      iggy3d::validatePhysicsBodyDescriptor(&invalidVelocity);
  const iggy3d::PhysicsValidationResult mass =
      iggy3d::validatePhysicsBodyDescriptor(&invalidMass);

  return expect(!missing.ok, "missing rejected") &&
         expect(missing.reasonCode == "physics_body_missing_descriptor",
                "missing reason") &&
         expect(!position.ok, "position rejected") &&
         expect(position.reasonCode == "physics_body_invalid_position",
                "position reason") &&
         expect(!velocity.ok, "velocity rejected") &&
         expect(velocity.reasonCode == "physics_body_invalid_velocity",
                "velocity reason") &&
         expect(!mass.ok, "mass rejected") &&
         expect(mass.reasonCode == "physics_body_invalid_mass",
                "mass reason");
}

bool inverseMassMatchesMotionPolicy() {
  return expect(iggy3d::computePhysicsInverseMass(
                    iggy3d::PhysicsBodyMotionKind::Dynamic, 2.0F) == 0.5F,
                "dynamic inverse") &&
         expect(iggy3d::computePhysicsInverseMass(
                    iggy3d::PhysicsBodyMotionKind::Static, 2.0F) == 0.0F,
                "static inverse") &&
         expect(iggy3d::computePhysicsInverseMass(
                    iggy3d::PhysicsBodyMotionKind::Kinematic, 2.0F) == 0.0F,
                "kinematic inverse") &&
         expect(iggy3d::computePhysicsInverseMass(
                    iggy3d::PhysicsBodyMotionKind::Dynamic, 0.0F) == 0.0F,
                "invalid dynamic inverse");
}

bool addAndReadBodyPreservesSoAFacts() {
  iggy3d::PhysicsBodyStore store;
  const iggy3d::PhysicsBodyStoreResult added = store.add(dynamicDescriptor());
  const iggy3d::PhysicsBodyStoreResult read = store.read(added.id);

  return expect(added.ok, "add ok") &&
         expect(added.status == iggy3d::PhysicsStatus::BodyAdded,
                "add status") &&
         expect(added.reasonCode == "physics_body_added", "add reason") &&
         expect(added.id.value == 1U, "first id") &&
         expect(added.index == 0U, "first index") &&
         expect(store.size() == 1U, "store count") &&
         expect(store.ids().size() == 1U, "ids soa count") &&
         expect(store.positions().size() == 1U, "positions soa count") &&
         expect(store.velocities().size() == 1U, "velocities soa count") &&
         expect(store.masses().size() == 1U, "masses soa count") &&
         expect(store.inverseMasses().size() == 1U,
                "inverse masses soa count") &&
         expect(read.ok, "read ok") &&
         expect(read.body.id.value == 1U, "read id") &&
         expect(read.body.motion == iggy3d::PhysicsBodyMotionKind::Dynamic,
                "read motion") &&
         expect(iggy3d::nearlyEqual(read.body.positionMeters,
                                    {1.0F, 2.0F, 3.0F}),
                "read position") &&
         expect(iggy3d::nearlyEqual(read.body.velocityMetersPerSecond,
                                    {0.5F, 0.0F, -1.0F}),
                "read velocity") &&
         expect(read.body.massKilograms == 2.0F, "read mass") &&
         expect(read.body.inverseMass == 0.5F, "read inverse mass");
}

bool staticBodiesUseZeroInverseMass() {
  iggy3d::PhysicsBodyStore store;
  iggy3d::PhysicsBodyDescriptor descriptor;
  descriptor.motion = iggy3d::PhysicsBodyMotionKind::Static;
  descriptor.positionMeters = {4.0F, 0.0F, 1.0F};
  descriptor.velocityMetersPerSecond = {};
  descriptor.massKilograms = 100.0F;

  const iggy3d::PhysicsBodyStoreResult added = store.add(descriptor);

  return expect(added.ok, "static add ok") &&
         expect(added.body.inverseMass == 0.0F, "static inverse zero");
}

bool invalidBodyDoesNotMutateStore() {
  iggy3d::PhysicsBodyStore store;
  iggy3d::PhysicsBodyDescriptor descriptor = dynamicDescriptor();
  descriptor.massKilograms = -1.0F;

  const iggy3d::PhysicsBodyStoreResult added = store.add(descriptor);

  return expect(!added.ok, "invalid add rejected") &&
         expect(added.status == iggy3d::PhysicsStatus::InvalidMass,
                "invalid add status") &&
         expect(added.reasonCode == "physics_body_invalid_mass",
                "invalid add reason") &&
         expect(store.empty(), "store still empty");
}

bool removePreservesRemainingOrderAndMissingIsStable() {
  iggy3d::PhysicsBodyStore store;
  const iggy3d::PhysicsBodyStoreResult first = store.add(dynamicDescriptor());
  iggy3d::PhysicsBodyDescriptor secondDescriptor = dynamicDescriptor();
  secondDescriptor.positionMeters = {8.0F, 0.0F, 0.0F};
  const iggy3d::PhysicsBodyStoreResult second = store.add(secondDescriptor);

  const iggy3d::PhysicsBodyStoreResult removed = store.remove(first.id);
  const iggy3d::PhysicsBodyStoreResult missing = store.read(first.id);
  const iggy3d::PhysicsBodyStoreResult remaining = store.read(second.id);

  return expect(removed.ok, "remove ok") &&
         expect(removed.status == iggy3d::PhysicsStatus::BodyRemoved,
                "remove status") &&
         expect(removed.body.id.value == first.id.value, "removed body id") &&
         expect(store.size() == 1U, "remaining size") &&
         expect(store.ids()[0].value == second.id.value,
                "remaining order preserved") &&
         expect(!missing.ok, "missing read rejected") &&
         expect(missing.reasonCode == "physics_body_not_found",
                "missing reason") &&
         expect(remaining.ok, "remaining read ok") &&
         expect(remaining.index == 0U, "remaining compacted index");
}

bool resetClearsStoreAndRestartsIds() {
  iggy3d::PhysicsBodyStore store;
  (void)store.add(dynamicDescriptor());
  const iggy3d::PhysicsBodyStoreResult reset = store.reset();
  const iggy3d::PhysicsBodyStoreResult added = store.add(dynamicDescriptor());

  return expect(reset.ok, "reset ok") &&
         expect(reset.reasonCode == "physics_body_store_reset",
                "reset reason") &&
         expect(store.size() == 1U, "one body after reset add") &&
         expect(added.id.value == 1U, "id restarts after reset");
}

}  // namespace

int main() {
  const bool ok = stableNamesAreLowerSnake() &&
                  validationRejectsInvalidDescriptorFacts() &&
                  inverseMassMatchesMotionPolicy() &&
                  addAndReadBodyPreservesSoAFacts() &&
                  staticBodiesUseZeroInverseMass() &&
                  invalidBodyDoesNotMutateStore() &&
                  removePreservesRemainingOrderAndMissingIsStable() &&
                  resetClearsStoreAndRestartsIds();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
