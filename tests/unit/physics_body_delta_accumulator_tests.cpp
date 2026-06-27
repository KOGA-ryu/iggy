#include "runtime/physics/PhysicsBodyDeltaAccumulator.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
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
    iggy3d::Vec3 velocity = {},
    iggy3d::PhysicsBodyMotionKind motion =
        iggy3d::PhysicsBodyMotionKind::Dynamic,
    float massKilograms = 2.0F) {
  iggy3d::PhysicsBodyDescriptor descriptor;
  descriptor.motion = motion;
  descriptor.positionMeters = position;
  descriptor.velocityMetersPerSecond = velocity;
  descriptor.massKilograms = massKilograms;
  return descriptor;
}

iggy3d::PhysicsAabbContactSolvePlan solvePlan(
    iggy3d::PhysicsBodyId firstId,
    iggy3d::PhysicsBodyId secondId,
    iggy3d::Vec3 firstPositionDelta,
    iggy3d::Vec3 secondPositionDelta,
    iggy3d::Vec3 firstVelocityDelta,
    iggy3d::Vec3 secondVelocityDelta) {
  iggy3d::PhysicsAabbContactSolvePlan plan;
  plan.ok = true;
  plan.status = iggy3d::PhysicsAabbContactSolveStatus::Solved;
  plan.reasonCode = "physics_aabb_contact_solved";
  plan.firstBodyId = firstId;
  plan.secondBodyId = secondId;
  plan.solveContact = true;
  plan.positionCorrectionApplied =
      !iggy3d::nearlyEqual(firstPositionDelta, {}) ||
      !iggy3d::nearlyEqual(secondPositionDelta, {});
  plan.velocityImpulseApplied = !iggy3d::nearlyEqual(firstVelocityDelta, {}) ||
                                !iggy3d::nearlyEqual(secondVelocityDelta, {});
  plan.frictionImpulseApplied = false;
  plan.firstPositionDeltaMeters = firstPositionDelta;
  plan.secondPositionDeltaMeters = secondPositionDelta;
  plan.firstVelocityDeltaMetersPerSecond = firstVelocityDelta;
  plan.secondVelocityDeltaMetersPerSecond = secondVelocityDelta;
  plan.normalImpulseMagnitude = plan.velocityImpulseApplied ? 0.5F : 0.0F;
  plan.frictionImpulseMagnitude = 0.0F;
  plan.effectiveInverseMass = 1.0F;
  return plan;
}

iggy3d::PhysicsAabbContactSolvePlan noOpSolvePlan(
    iggy3d::PhysicsBodyId firstId,
    iggy3d::PhysicsBodyId secondId,
    bool solveContact) {
  iggy3d::PhysicsAabbContactSolvePlan plan;
  plan.ok = true;
  plan.status = solveContact ? iggy3d::PhysicsAabbContactSolveStatus::Solved
                             : iggy3d::PhysicsAabbContactSolveStatus::SensorContact;
  plan.reasonCode = solveContact ? "physics_aabb_contact_solved"
                                 : "physics_aabb_contact_sensor";
  plan.firstBodyId = firstId;
  plan.secondBodyId = secondId;
  plan.solveContact = solveContact;
  plan.effectiveInverseMass = 1.0F;
  return plan;
}

bool accumulatorUnchanged(const iggy3d::PhysicsBodyDeltaAccumulator& lhs,
                          const iggy3d::PhysicsBodyDeltaAccumulator& rhs) {
  if (lhs.bodyIds.size() != rhs.bodyIds.size() ||
      lhs.positionDeltasMeters.size() != rhs.positionDeltasMeters.size() ||
      lhs.velocityDeltasMetersPerSecond.size() !=
          rhs.velocityDeltasMetersPerSecond.size() ||
      lhs.contributingPlanCounts.size() != rhs.contributingPlanCounts.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < lhs.bodyIds.size(); ++index) {
    if (lhs.bodyIds[index].value != rhs.bodyIds[index].value ||
        !iggy3d::nearlyEqual(lhs.positionDeltasMeters[index],
                             rhs.positionDeltasMeters[index]) ||
        !iggy3d::nearlyEqual(lhs.velocityDeltasMetersPerSecond[index],
                             rhs.velocityDeltasMetersPerSecond[index]) ||
        lhs.contributingPlanCounts[index] !=
            rhs.contributingPlanCounts[index]) {
      return false;
    }
  }
  return true;
}

bool planUnchanged(const iggy3d::PhysicsAabbContactSolvePlan& lhs,
                   const iggy3d::PhysicsAabbContactSolvePlan& rhs) {
  return lhs.ok == rhs.ok && lhs.status == rhs.status &&
         lhs.reasonCode == rhs.reasonCode &&
         lhs.firstBodyId.value == rhs.firstBodyId.value &&
         lhs.secondBodyId.value == rhs.secondBodyId.value &&
         lhs.solveContact == rhs.solveContact &&
         lhs.positionCorrectionApplied == rhs.positionCorrectionApplied &&
         lhs.velocityImpulseApplied == rhs.velocityImpulseApplied &&
         lhs.frictionImpulseApplied == rhs.frictionImpulseApplied &&
         iggy3d::nearlyEqual(lhs.firstPositionDeltaMeters,
                             rhs.firstPositionDeltaMeters) &&
         iggy3d::nearlyEqual(lhs.secondPositionDeltaMeters,
                             rhs.secondPositionDeltaMeters) &&
         iggy3d::nearlyEqual(lhs.firstVelocityDeltaMetersPerSecond,
                             rhs.firstVelocityDeltaMetersPerSecond) &&
         iggy3d::nearlyEqual(lhs.secondVelocityDeltaMetersPerSecond,
                             rhs.secondVelocityDeltaMetersPerSecond) &&
         lhs.normalImpulseMagnitude == rhs.normalImpulseMagnitude &&
         lhs.frictionImpulseMagnitude == rhs.frictionImpulseMagnitude &&
         lhs.effectiveInverseMass == rhs.effectiveInverseMass;
}

bool stableNamesAreLowerSnake() {
  return expect(iggy3d::physicsBodyDeltaStatusName(
                    iggy3d::PhysicsBodyDeltaStatus::AccumulatorBuilt) ==
                    "physics_body_delta_accumulator_built",
                "accumulator built status") &&
         expect(iggy3d::physicsBodyDeltaStatusName(
                    iggy3d::PhysicsBodyDeltaStatus::DeltaAccumulated) ==
                    "physics_body_delta_accumulated",
                "delta accumulated status") &&
         expect(iggy3d::physicsBodyDeltaStatusName(
                    iggy3d::PhysicsBodyDeltaStatus::DeltasApplied) ==
                    "physics_body_delta_applied",
                "deltas applied status") &&
         expect(iggy3d::physicsBodyDeltaStatusName(
                    iggy3d::PhysicsBodyDeltaStatus::NoOp) ==
                    "physics_body_delta_no_op",
                "no-op status") &&
         expect(iggy3d::physicsBodyDeltaStatusName(
                    iggy3d::PhysicsBodyDeltaStatus::MissingStore) ==
                    "physics_body_delta_missing_store",
                "missing store status") &&
         expect(iggy3d::physicsBodyDeltaStatusName(
                    iggy3d::PhysicsBodyDeltaStatus::MissingAccumulator) ==
                    "physics_body_delta_missing_accumulator",
                "missing accumulator status") &&
         expect(iggy3d::physicsBodyDeltaStatusName(
                    iggy3d::PhysicsBodyDeltaStatus::MissingPlan) ==
                    "physics_body_delta_missing_plan",
                "missing plan status") &&
         expect(iggy3d::physicsBodyDeltaStatusName(
                    iggy3d::PhysicsBodyDeltaStatus::InvalidStoreState) ==
                    "physics_body_delta_invalid_store_state",
                "invalid store status") &&
         expect(iggy3d::physicsBodyDeltaStatusName(
                    iggy3d::PhysicsBodyDeltaStatus::InvalidPlan) ==
                    "physics_body_delta_invalid_plan",
                "invalid plan status") &&
         expect(iggy3d::physicsBodyDeltaStatusName(
                    iggy3d::PhysicsBodyDeltaStatus::BodyNotFound) ==
                    "physics_body_delta_body_not_found",
                "body not found status") &&
         expect(iggy3d::physicsBodyDeltaStatusName(
                    iggy3d::PhysicsBodyDeltaStatus::InvalidDelta) ==
                    "physics_body_delta_invalid_delta",
                "invalid delta status") &&
         expect(iggy3d::physicsBodyDeltaStatusName(
                    iggy3d::PhysicsBodyDeltaStatus::InvalidConfig) ==
                    "physics_body_delta_invalid_config",
                "invalid config status");
}

bool buildAccumulatorPreservesStoreOrder() {
  iggy3d::PhysicsBodyStore emptyStore;
  const iggy3d::PhysicsBodyDeltaAccumulatorResult empty =
      iggy3d::buildPhysicsBodyDeltaAccumulator(&emptyStore);

  iggy3d::PhysicsBodyStore store;
  const iggy3d::PhysicsBodyStoreResult first =
      store.add(bodyDescriptor({1.0F, 0.0F, 0.0F}));
  const iggy3d::PhysicsBodyStoreResult second =
      store.add(bodyDescriptor({2.0F, 0.0F, 0.0F}));

  const iggy3d::PhysicsBodyDeltaAccumulatorResult built =
      iggy3d::buildPhysicsBodyDeltaAccumulator(&store);
  const iggy3d::PhysicsBodyDeltaAccumulatorResult missing =
      iggy3d::buildPhysicsBodyDeltaAccumulator(nullptr);

  return expect(empty.ok, "empty build ok") &&
         expect(empty.bodyCount == 0U, "empty body count") &&
         expect(built.ok, "non-empty build ok") &&
         expect(built.reasonCode == "physics_body_delta_accumulator_built",
                "build reason") &&
         expect(built.bodyCount == 2U, "build body count") &&
         expect(built.accumulator.bodyIds[0].value == first.id.value,
                "first id order") &&
         expect(built.accumulator.bodyIds[1].value == second.id.value,
                "second id order") &&
         expect(iggy3d::nearlyEqual(built.accumulator.positionDeltasMeters[0],
                                    {}),
                "first zero position delta") &&
         expect(iggy3d::nearlyEqual(
                    built.accumulator.velocityDeltasMetersPerSecond[1], {}),
                "second zero velocity delta") &&
         expect(built.accumulator.contributingPlanCounts[0] == 0U,
                "first zero contributing count") &&
         expect(!missing.ok, "missing store rejected") &&
         expect(missing.reasonCode == "physics_body_delta_missing_store",
                "missing store reason");
}

bool accumulatePlanUpdatesRowsAndKeepsPlanConst() {
  iggy3d::PhysicsBodyStore store;
  const iggy3d::PhysicsBodyStoreResult first =
      store.add(bodyDescriptor({0.0F, 0.0F, 0.0F}));
  const iggy3d::PhysicsBodyStoreResult second =
      store.add(bodyDescriptor({1.0F, 0.0F, 0.0F}));
  iggy3d::PhysicsBodyDeltaAccumulator accumulator =
      iggy3d::buildPhysicsBodyDeltaAccumulator(&store).accumulator;
  const iggy3d::PhysicsAabbContactSolvePlan plan =
      solvePlan(first.id, second.id, {-0.25F, 0.0F, 0.0F},
                {0.25F, 0.0F, 0.0F}, {-1.0F, 0.0F, 0.0F},
                {1.0F, 0.0F, 0.0F});
  const iggy3d::PhysicsAabbContactSolvePlan before = plan;

  const iggy3d::PhysicsBodyDeltaAccumulatorResult accumulated =
      iggy3d::accumulatePhysicsAabbContactSolvePlan(&accumulator, &plan);

  return expect(accumulated.ok, "accumulate ok") &&
         expect(accumulated.reasonCode == "physics_body_delta_accumulated",
                "accumulate reason") &&
         expect(accumulated.accumulatedPlanCount == 1U,
                "one plan accumulated") &&
         expect(iggy3d::nearlyEqual(accumulator.positionDeltasMeters[0],
                                    {-0.25F, 0.0F, 0.0F}),
                "first position accumulated") &&
         expect(iggy3d::nearlyEqual(accumulator.positionDeltasMeters[1],
                                    {0.25F, 0.0F, 0.0F}),
                "second position accumulated") &&
         expect(iggy3d::nearlyEqual(
                    accumulator.velocityDeltasMetersPerSecond[0],
                    {-1.0F, 0.0F, 0.0F}),
                "first velocity accumulated") &&
         expect(iggy3d::nearlyEqual(
                    accumulator.velocityDeltasMetersPerSecond[1],
                    {1.0F, 0.0F, 0.0F}),
                "second velocity accumulated") &&
         expect(accumulator.contributingPlanCounts[0] == 1U,
                "first count") &&
         expect(accumulator.contributingPlanCounts[1] == 1U,
                "second count") &&
         expect(planUnchanged(plan, before), "solve plan unchanged");
}

bool accumulatingMultiplePlansSumsDeterministically() {
  iggy3d::PhysicsBodyStore store;
  const iggy3d::PhysicsBodyStoreResult first =
      store.add(bodyDescriptor({0.0F, 0.0F, 0.0F}));
  const iggy3d::PhysicsBodyStoreResult second =
      store.add(bodyDescriptor({1.0F, 0.0F, 0.0F}));
  const iggy3d::PhysicsBodyStoreResult third =
      store.add(bodyDescriptor({2.0F, 0.0F, 0.0F}));
  iggy3d::PhysicsBodyDeltaAccumulator accumulator =
      iggy3d::buildPhysicsBodyDeltaAccumulator(&store).accumulator;
  const iggy3d::PhysicsAabbContactSolvePlan firstPlan =
      solvePlan(first.id, second.id, {-0.1F, 0.0F, 0.0F},
                {0.1F, 0.0F, 0.0F}, {}, {});
  const iggy3d::PhysicsAabbContactSolvePlan secondPlan =
      solvePlan(third.id, first.id, {0.0F, 0.0F, 0.4F},
                {0.0F, 0.0F, -0.2F}, {0.0F, 2.0F, 0.0F},
                {0.0F, -1.0F, 0.0F});

  const iggy3d::PhysicsBodyDeltaAccumulatorResult a =
      iggy3d::accumulatePhysicsAabbContactSolvePlan(&accumulator, &firstPlan);
  const iggy3d::PhysicsBodyDeltaAccumulatorResult b =
      iggy3d::accumulatePhysicsAabbContactSolvePlan(&accumulator, &secondPlan);

  return expect(a.ok && b.ok, "both accumulations ok") &&
         expect(iggy3d::nearlyEqual(accumulator.positionDeltasMeters[0],
                                    {-0.1F, 0.0F, -0.2F}),
                "first body summed position") &&
         expect(iggy3d::nearlyEqual(accumulator.positionDeltasMeters[1],
                                    {0.1F, 0.0F, 0.0F}),
                "second body position") &&
         expect(iggy3d::nearlyEqual(accumulator.positionDeltasMeters[2],
                                    {0.0F, 0.0F, 0.4F}),
                "third body position") &&
         expect(iggy3d::nearlyEqual(
                    accumulator.velocityDeltasMetersPerSecond[0],
                    {0.0F, -1.0F, 0.0F}),
                "first body summed velocity") &&
         expect(iggy3d::nearlyEqual(
                    accumulator.velocityDeltasMetersPerSecond[2],
                    {0.0F, 2.0F, 0.0F}),
                "third body velocity") &&
         expect(accumulator.contributingPlanCounts[0] == 2U,
                "first two contributing plans") &&
         expect(accumulator.contributingPlanCounts[1] == 1U,
                "second one contributing plan") &&
         expect(accumulator.contributingPlanCounts[2] == 1U,
                "third one contributing plan");
}

bool noOpAndInvalidPlansDoNotMutateAccumulator() {
  iggy3d::PhysicsBodyStore store;
  const iggy3d::PhysicsBodyStoreResult first =
      store.add(bodyDescriptor({0.0F, 0.0F, 0.0F}));
  const iggy3d::PhysicsBodyStoreResult second =
      store.add(bodyDescriptor({1.0F, 0.0F, 0.0F}));
  iggy3d::PhysicsBodyDeltaAccumulator accumulator =
      iggy3d::buildPhysicsBodyDeltaAccumulator(&store).accumulator;
  const iggy3d::PhysicsBodyDeltaAccumulator initial = accumulator;
  const iggy3d::PhysicsAabbContactSolvePlan sensor =
      noOpSolvePlan(first.id, second.id, false);
  const iggy3d::PhysicsAabbContactSolvePlan separatingNoDelta =
      noOpSolvePlan(first.id, second.id, true);
  iggy3d::PhysicsAabbContactSolvePlan missingBody =
      solvePlan(first.id, {99U}, {-1.0F, 0.0F, 0.0F},
                {1.0F, 0.0F, 0.0F}, {}, {});
  iggy3d::PhysicsAabbContactSolvePlan nonfinite =
      solvePlan(first.id, second.id, {}, {}, {}, {});
  nonfinite.velocityImpulseApplied = true;
  nonfinite.firstVelocityDeltaMetersPerSecond.x =
      std::numeric_limits<float>::infinity();

  const iggy3d::PhysicsBodyDeltaAccumulatorResult sensorResult =
      iggy3d::accumulatePhysicsAabbContactSolvePlan(&accumulator, &sensor);
  const iggy3d::PhysicsBodyDeltaAccumulator afterSensor = accumulator;
  const iggy3d::PhysicsBodyDeltaAccumulatorResult noDeltaResult =
      iggy3d::accumulatePhysicsAabbContactSolvePlan(&accumulator,
                                                    &separatingNoDelta);
  const iggy3d::PhysicsBodyDeltaAccumulator afterNoDelta = accumulator;
  const iggy3d::PhysicsBodyDeltaAccumulatorResult unknown =
      iggy3d::accumulatePhysicsAabbContactSolvePlan(&accumulator,
                                                    &missingBody);
  const iggy3d::PhysicsBodyDeltaAccumulator afterUnknown = accumulator;
  const iggy3d::PhysicsBodyDeltaAccumulatorResult invalid =
      iggy3d::accumulatePhysicsAabbContactSolvePlan(&accumulator, &nonfinite);

  return expect(sensorResult.ok, "sensor no-op ok") &&
         expect(sensorResult.reasonCode == "physics_body_delta_no_op",
                "sensor no-op reason") &&
         expect(accumulatorUnchanged(initial, afterSensor),
                "sensor no-op unchanged") &&
         expect(noDeltaResult.ok, "no-delta no-op ok") &&
         expect(noDeltaResult.reasonCode == "physics_body_delta_no_op",
                "no-delta no-op reason") &&
         expect(accumulatorUnchanged(initial, afterNoDelta),
                "no-delta unchanged") &&
         expect(!unknown.ok, "unknown body rejected") &&
         expect(unknown.reasonCode == "physics_body_delta_body_not_found",
                "unknown body reason") &&
         expect(unknown.invalidBodyId.value == 99U, "unknown body id") &&
         expect(accumulatorUnchanged(initial, afterUnknown),
                "unknown body unchanged") &&
         expect(!invalid.ok, "nonfinite rejected") &&
         expect(invalid.reasonCode == "physics_body_delta_invalid_delta",
                "nonfinite reason") &&
         expect(accumulatorUnchanged(initial, accumulator),
                "nonfinite unchanged");
}

bool applyMutatesPositionsAndVelocitiesOnly() {
  iggy3d::PhysicsBodyStore store;
  const iggy3d::PhysicsBodyStoreResult first =
      store.add(bodyDescriptor({0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}));
  const iggy3d::PhysicsBodyStoreResult second =
      store.add(bodyDescriptor({10.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}));
  const std::vector<iggy3d::PhysicsBodyId> idsBefore = store.ids();
  const std::vector<iggy3d::PhysicsBodyMotionKind> motionsBefore =
      store.motions();
  const std::vector<float> massesBefore = store.masses();
  const std::vector<float> inverseMassesBefore = store.inverseMasses();

  iggy3d::PhysicsBodyDeltaAccumulator accumulator;
  accumulator.bodyIds = {second.id, first.id};
  accumulator.positionDeltasMeters = {{0.0F, 0.0F, 3.0F},
                                      {1.0F, 2.0F, 0.0F}};
  accumulator.velocityDeltasMetersPerSecond = {{0.0F, -0.5F, 0.0F},
                                               {-0.25F, 0.0F, 0.0F}};
  accumulator.contributingPlanCounts = {1U, 1U};

  const iggy3d::PhysicsBodyDeltaApplyResult applied =
      iggy3d::applyPhysicsBodyDeltas(&store, accumulator,
                                     iggy3d::PhysicsBodyDeltaApplyConfig{});
  const iggy3d::PhysicsBodyStoreResult firstAfter = store.read(first.id);
  const iggy3d::PhysicsBodyStoreResult secondAfter = store.read(second.id);

  return expect(applied.ok, "apply ok") &&
         expect(applied.reasonCode == "physics_body_delta_applied",
                "apply reason") &&
         expect(applied.appliedPositionCount == 2U,
                "two position rows applied") &&
         expect(applied.appliedVelocityCount == 2U,
                "two velocity rows applied") &&
         expect(iggy3d::nearlyEqual(firstAfter.body.positionMeters,
                                    {1.0F, 2.0F, 0.0F}),
                "first position applied") &&
         expect(iggy3d::nearlyEqual(secondAfter.body.positionMeters,
                                    {10.0F, 0.0F, 3.0F}),
                "second position applied") &&
         expect(iggy3d::nearlyEqual(firstAfter.body.velocityMetersPerSecond,
                                    {0.75F, 0.0F, 0.0F}),
                "first velocity applied") &&
         expect(iggy3d::nearlyEqual(secondAfter.body.velocityMetersPerSecond,
                                    {0.0F, 0.5F, 0.0F}),
                "second velocity applied") &&
         expect(store.ids()[0].value == idsBefore[0].value &&
                    store.ids()[1].value == idsBefore[1].value,
                "ids unchanged") &&
         expect(store.motions()[0] == motionsBefore[0] &&
                    store.motions()[1] == motionsBefore[1],
                "motions unchanged") &&
         expect(store.masses()[0] == massesBefore[0] &&
                    store.masses()[1] == massesBefore[1],
                "masses unchanged") &&
         expect(store.inverseMasses()[0] == inverseMassesBefore[0] &&
                    store.inverseMasses()[1] == inverseMassesBefore[1],
                "inverse masses unchanged");
}

bool applyConfigCanDisablePositionOrVelocityDeltas() {
  iggy3d::PhysicsBodyStore velocityOnlyStore;
  const iggy3d::PhysicsBodyStoreResult velocityBody =
      velocityOnlyStore.add(bodyDescriptor({0.0F, 0.0F, 0.0F},
                                           {1.0F, 0.0F, 0.0F}));
  iggy3d::PhysicsBodyDeltaAccumulator accumulator;
  accumulator.bodyIds = {velocityBody.id};
  accumulator.positionDeltasMeters = {{9.0F, 0.0F, 0.0F}};
  accumulator.velocityDeltasMetersPerSecond = {{0.0F, 2.0F, 0.0F}};
  accumulator.contributingPlanCounts = {1U};
  iggy3d::PhysicsBodyDeltaApplyConfig velocityOnlyConfig;
  velocityOnlyConfig.applyPositionDeltas = false;
  const iggy3d::PhysicsBodyDeltaApplyResult velocityOnly =
      iggy3d::applyPhysicsBodyDeltas(&velocityOnlyStore, accumulator,
                                     velocityOnlyConfig);
  const iggy3d::PhysicsBodyStoreResult velocityAfter =
      velocityOnlyStore.read(velocityBody.id);

  iggy3d::PhysicsBodyStore positionOnlyStore;
  const iggy3d::PhysicsBodyStoreResult positionBody =
      positionOnlyStore.add(bodyDescriptor({0.0F, 0.0F, 0.0F},
                                           {1.0F, 0.0F, 0.0F}));
  iggy3d::PhysicsBodyDeltaApplyConfig positionOnlyConfig;
  positionOnlyConfig.applyVelocityDeltas = false;
  const iggy3d::PhysicsBodyDeltaApplyResult positionOnly =
      iggy3d::applyPhysicsBodyDeltas(&positionOnlyStore, accumulator,
                                     positionOnlyConfig);
  const iggy3d::PhysicsBodyStoreResult positionAfter =
      positionOnlyStore.read(positionBody.id);

  return expect(velocityOnly.ok, "velocity-only apply ok") &&
         expect(velocityOnly.appliedPositionCount == 0U,
                "position disabled count") &&
         expect(velocityOnly.appliedVelocityCount == 1U,
                "velocity applied count") &&
         expect(iggy3d::nearlyEqual(velocityAfter.body.positionMeters, {}),
                "position disabled unchanged") &&
         expect(iggy3d::nearlyEqual(velocityAfter.body.velocityMetersPerSecond,
                                    {1.0F, 2.0F, 0.0F}),
                "velocity applied") &&
         expect(positionOnly.ok, "position-only apply ok") &&
         expect(positionOnly.appliedPositionCount == 1U,
                "position applied count") &&
         expect(positionOnly.appliedVelocityCount == 0U,
                "velocity disabled count") &&
         expect(iggy3d::nearlyEqual(positionAfter.body.positionMeters,
                                    {9.0F, 0.0F, 0.0F}),
                "position applied") &&
         expect(iggy3d::nearlyEqual(positionAfter.body.velocityMetersPerSecond,
                                    {1.0F, 0.0F, 0.0F}),
                "velocity disabled unchanged");
}

bool allZeroAndDisabledApplyNoOp() {
  iggy3d::PhysicsBodyStore store;
  const iggy3d::PhysicsBodyStoreResult body =
      store.add(bodyDescriptor({0.0F, 0.0F, 0.0F}));
  iggy3d::PhysicsBodyDeltaAccumulator zero =
      iggy3d::buildPhysicsBodyDeltaAccumulator(&store).accumulator;
  iggy3d::PhysicsBodyDeltaAccumulator disabledOnly;
  disabledOnly.bodyIds = {body.id};
  disabledOnly.positionDeltasMeters = {{3.0F, 0.0F, 0.0F}};
  disabledOnly.velocityDeltasMetersPerSecond = {{}};
  disabledOnly.contributingPlanCounts = {1U};
  iggy3d::PhysicsBodyDeltaApplyConfig config;
  config.applyPositionDeltas = false;

  const iggy3d::PhysicsBodyDeltaApplyResult zeroResult =
      iggy3d::applyPhysicsBodyDeltas(&store, zero, {});
  const iggy3d::PhysicsBodyDeltaApplyResult disabledResult =
      iggy3d::applyPhysicsBodyDeltas(&store, disabledOnly, config);
  const iggy3d::PhysicsBodyStoreResult after = store.read(body.id);

  return expect(zeroResult.ok, "zero apply ok") &&
         expect(zeroResult.reasonCode == "physics_body_delta_no_op",
                "zero no-op reason") &&
         expect(disabledResult.ok, "disabled-only apply ok") &&
         expect(disabledResult.reasonCode == "physics_body_delta_no_op",
                "disabled-only no-op reason") &&
         expect(iggy3d::nearlyEqual(after.body.positionMeters, {}),
                "store unchanged") &&
         expect(iggy3d::nearlyEqual(after.body.velocityMetersPerSecond, {}),
                "velocity unchanged");
}

bool invalidApplyInputsRejectWithoutMutation() {
  iggy3d::PhysicsBodyStore store;
  const iggy3d::PhysicsBodyStoreResult body =
      store.add(bodyDescriptor({0.0F, 0.0F, 0.0F}));
  iggy3d::PhysicsBodyDeltaAccumulator accumulator;
  accumulator.bodyIds = {body.id};
  accumulator.positionDeltasMeters = {{2.0F, 0.0F, 0.0F}};
  accumulator.velocityDeltasMetersPerSecond = {{}};
  accumulator.contributingPlanCounts = {1U};
  iggy3d::PhysicsBodyDeltaApplyConfig capped;
  capped.maxPositionDeltaMeters = 1.0F;
  iggy3d::PhysicsBodyDeltaApplyConfig invalidConfig;
  invalidConfig.maxVelocityDeltaMetersPerSecond =
      std::numeric_limits<float>::infinity();
  iggy3d::PhysicsBodyDeltaAccumulator missingBody = accumulator;
  missingBody.bodyIds[0] = {99U};

  const iggy3d::PhysicsBodyDeltaApplyResult cap =
      iggy3d::applyPhysicsBodyDeltas(&store, accumulator, capped);
  const iggy3d::PhysicsBodyStoreResult afterCap = store.read(body.id);
  const iggy3d::PhysicsBodyDeltaApplyResult config =
      iggy3d::applyPhysicsBodyDeltas(&store, accumulator, invalidConfig);
  const iggy3d::PhysicsBodyDeltaApplyResult missing =
      iggy3d::applyPhysicsBodyDeltas(&store, missingBody, {});
  const iggy3d::PhysicsBodyDeltaApplyResult missingStore =
      iggy3d::applyPhysicsBodyDeltas(nullptr, accumulator, {});

  return expect(!cap.ok, "over-cap rejected") &&
         expect(cap.reasonCode == "physics_body_delta_invalid_delta",
                "over-cap reason") &&
         expect(iggy3d::nearlyEqual(afterCap.body.positionMeters, {}),
                "over-cap no mutation") &&
         expect(!config.ok, "invalid config rejected") &&
         expect(config.reasonCode == "physics_body_delta_invalid_config",
                "invalid config reason") &&
         expect(!missing.ok, "missing accumulator body rejected") &&
         expect(missing.reasonCode == "physics_body_delta_body_not_found",
                "missing accumulator body reason") &&
         expect(!missingStore.ok, "missing store rejected") &&
         expect(missingStore.reasonCode == "physics_body_delta_missing_store",
                "missing store reason");
}

}  // namespace

int main() {
  const bool ok = stableNamesAreLowerSnake() &&
                  buildAccumulatorPreservesStoreOrder() &&
                  accumulatePlanUpdatesRowsAndKeepsPlanConst() &&
                  accumulatingMultiplePlansSumsDeterministically() &&
                  noOpAndInvalidPlansDoNotMutateAccumulator() &&
                  applyMutatesPositionsAndVelocitiesOnly() &&
                  applyConfigCanDisablePositionOrVelocityDeltas() &&
                  allZeroAndDisabledApplyNoOp() &&
                  invalidApplyInputsRejectWithoutMutation();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
