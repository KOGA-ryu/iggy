#include "runtime/physics/PhysicsAabbContactSolver.hpp"

#include <cmath>
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

bool nearlyEqual(float lhs, float rhs, float epsilon = 0.0001F) {
  return std::fabs(lhs - rhs) <= epsilon;
}

iggy3d::PhysicsBodyView body(iggy3d::PhysicsBodyId id,
                             iggy3d::Vec3 velocity,
                             float inverseMass,
                             iggy3d::PhysicsBodyMotionKind motion =
                                 iggy3d::PhysicsBodyMotionKind::Dynamic) {
  iggy3d::PhysicsBodyView result;
  result.id = id;
  result.motion = motion;
  result.positionMeters = {};
  result.velocityMetersPerSecond = velocity;
  result.massKilograms = inverseMass > 0.0F ? 1.0F / inverseMass : 0.0F;
  result.inverseMass = inverseMass;
  return result;
}

iggy3d::PhysicsAabbContact contact(float penetration = 0.5F,
                                   iggy3d::Vec3 normal = {1.0F, 0.0F, 0.0F},
                                   bool sensor = false) {
  iggy3d::PhysicsAabbContact result;
  result.firstBodyId = {1U};
  result.secondBodyId = {2U};
  result.normalFromFirstToSecond = normal;
  result.penetrationMeters = penetration;
  result.pointMeters = {};
  result.includesSensor = sensor;
  return result;
}

iggy3d::PhysicsMaterialPairTraits materialPair(float restitution = 0.0F,
                                               float dynamicFriction = 0.0F,
                                               bool solveContact = true) {
  iggy3d::PhysicsMaterialPairTraits result;
  result.firstMaterialId = {1U};
  result.secondMaterialId = {2U};
  result.staticFriction = dynamicFriction;
  result.dynamicFriction = dynamicFriction;
  result.restitution = restitution;
  result.dampingMultiplier = 1.0F;
  result.solveContact = solveContact;
  result.trigger = !solveContact;
  result.flags = solveContact ? 0U : iggy3d::kPhysicsMaterialFlagTrigger;
  return result;
}

iggy3d::PhysicsAabbContactSolveConfig easyConfig() {
  iggy3d::PhysicsAabbContactSolveConfig config;
  config.penetrationSlopMeters = 0.0F;
  config.positionCorrectionPercent = 1.0F;
  config.maxPositionCorrectionMeters = 1.0F;
  return config;
}

bool planIsFinite(const iggy3d::PhysicsAabbContactSolvePlan& plan) {
  return iggy3d::isFinite(plan.firstPositionDeltaMeters) &&
         iggy3d::isFinite(plan.secondPositionDeltaMeters) &&
         iggy3d::isFinite(plan.firstVelocityDeltaMetersPerSecond) &&
         iggy3d::isFinite(plan.secondVelocityDeltaMetersPerSecond) &&
         std::isfinite(plan.normalImpulseMagnitude) &&
         std::isfinite(plan.frictionImpulseMagnitude) &&
         std::isfinite(plan.effectiveInverseMass);
}

iggy3d::PhysicsAabbContactSolvePlan solve(
    const iggy3d::PhysicsAabbContact* c,
    const iggy3d::PhysicsBodyView* first,
    const iggy3d::PhysicsBodyView* second,
    const iggy3d::PhysicsMaterialPairTraits* pair,
    iggy3d::PhysicsAabbContactSolveConfig config =
        iggy3d::PhysicsAabbContactSolveConfig{}) {
  iggy3d::PhysicsAabbContactSolveRequest request;
  request.contact = c;
  request.firstBody = first;
  request.secondBody = second;
  request.materialPair = pair;
  request.config = config;
  return iggy3d::solvePhysicsAabbContact(request);
}

bool stableNamesAreLowerSnake() {
  return expect(iggy3d::physicsAabbContactSolveStatusName(
                    iggy3d::PhysicsAabbContactSolveStatus::Solved) ==
                    "physics_aabb_contact_solved",
                "solved status") &&
         expect(iggy3d::physicsAabbContactSolveStatusName(
                    iggy3d::PhysicsAabbContactSolveStatus::MissingContact) ==
                    "physics_aabb_contact_solve_missing_contact",
                "missing contact status") &&
         expect(iggy3d::physicsAabbContactSolveStatusName(
                    iggy3d::PhysicsAabbContactSolveStatus::MissingFirstBody) ==
                    "physics_aabb_contact_solve_missing_first_body",
                "missing first body status") &&
         expect(iggy3d::physicsAabbContactSolveStatusName(
                    iggy3d::PhysicsAabbContactSolveStatus::MissingSecondBody) ==
                    "physics_aabb_contact_solve_missing_second_body",
                "missing second body status") &&
         expect(iggy3d::physicsAabbContactSolveStatusName(
                    iggy3d::PhysicsAabbContactSolveStatus::BodyMismatch) ==
                    "physics_aabb_contact_solve_body_mismatch",
                "body mismatch status") &&
         expect(iggy3d::physicsAabbContactSolveStatusName(
                    iggy3d::PhysicsAabbContactSolveStatus::InvalidContact) ==
                    "physics_aabb_contact_solve_invalid_contact",
                "invalid contact status") &&
         expect(iggy3d::physicsAabbContactSolveStatusName(
                    iggy3d::PhysicsAabbContactSolveStatus::InvalidBodyState) ==
                    "physics_aabb_contact_solve_invalid_body_state",
                "invalid body status") &&
         expect(iggy3d::physicsAabbContactSolveStatusName(
                    iggy3d::PhysicsAabbContactSolveStatus::InvalidMaterialPair) ==
                    "physics_aabb_contact_solve_invalid_material_pair",
                "invalid material status") &&
         expect(iggy3d::physicsAabbContactSolveStatusName(
                    iggy3d::PhysicsAabbContactSolveStatus::InvalidConfig) ==
                    "physics_aabb_contact_solve_invalid_config",
                "invalid config status") &&
         expect(iggy3d::physicsAabbContactSolveStatusName(
                    iggy3d::PhysicsAabbContactSolveStatus::NoEffectiveMass) ==
                    "physics_aabb_contact_solve_no_effective_mass",
                "no mass status") &&
         expect(iggy3d::physicsAabbContactSolveStatusName(
                    iggy3d::PhysicsAabbContactSolveStatus::SensorContact) ==
                    "physics_aabb_contact_sensor",
                "sensor status") &&
         expect(iggy3d::physicsAabbContactSolveStatusName(
                    iggy3d::PhysicsAabbContactSolveStatus::SeparatingVelocity) ==
                    "physics_aabb_contact_solve_separating_velocity",
                "separating status");
}

bool nullAndInvalidInputsReject() {
  iggy3d::PhysicsAabbContact c = contact();
  iggy3d::PhysicsBodyView first = body({1U}, {}, 1.0F);
  iggy3d::PhysicsBodyView second = body({2U}, {}, 1.0F);
  iggy3d::PhysicsMaterialPairTraits pair = materialPair();
  iggy3d::PhysicsAabbContactSolveConfig invalidConfig = easyConfig();
  invalidConfig.positionCorrectionPercent = 1.1F;

  const iggy3d::PhysicsAabbContactSolvePlan missingContact =
      solve(nullptr, &first, &second, &pair);
  const iggy3d::PhysicsAabbContactSolvePlan missingFirst =
      solve(&c, nullptr, &second, &pair);
  const iggy3d::PhysicsAabbContactSolvePlan missingSecond =
      solve(&c, &first, nullptr, &pair);
  const iggy3d::PhysicsAabbContactSolvePlan missingMaterial =
      solve(&c, &first, &second, nullptr);
  const iggy3d::PhysicsAabbContactSolvePlan config =
      solve(&c, &first, &second, &pair, invalidConfig);

  return expect(!missingContact.ok, "missing contact rejected") &&
         expect(missingContact.reasonCode ==
                    "physics_aabb_contact_solve_missing_contact",
                "missing contact reason") &&
         expect(!missingFirst.ok, "missing first rejected") &&
         expect(missingFirst.reasonCode ==
                    "physics_aabb_contact_solve_missing_first_body",
                "missing first reason") &&
         expect(!missingSecond.ok, "missing second rejected") &&
         expect(missingSecond.reasonCode ==
                    "physics_aabb_contact_solve_missing_second_body",
                "missing second reason") &&
         expect(!missingMaterial.ok, "missing material rejected") &&
         expect(missingMaterial.reasonCode ==
                    "physics_aabb_contact_solve_invalid_material_pair",
                "missing material reason") &&
         expect(!config.ok, "invalid config rejected") &&
         expect(config.reasonCode ==
                    "physics_aabb_contact_solve_invalid_config",
                "invalid config reason");
}

bool invalidContactBodyAndMaterialReject() {
  iggy3d::PhysicsAabbContact invalidNormal = contact();
  invalidNormal.normalFromFirstToSecond = {};
  iggy3d::PhysicsAabbContact invalidPenetration = contact();
  invalidPenetration.penetrationMeters = -0.1F;
  iggy3d::PhysicsAabbContact mismatch = contact();
  mismatch.secondBodyId = {3U};
  iggy3d::PhysicsBodyView invalidBody = body({1U}, {}, 1.0F);
  invalidBody.velocityMetersPerSecond.x =
      std::numeric_limits<float>::infinity();
  iggy3d::PhysicsAabbContact validContact = contact();
  iggy3d::PhysicsBodyView first = body({1U}, {}, 1.0F);
  iggy3d::PhysicsBodyView second = body({2U}, {}, 1.0F);
  iggy3d::PhysicsMaterialPairTraits invalidMaterial = materialPair();
  invalidMaterial.restitution = 1.1F;
  iggy3d::PhysicsMaterialPairTraits pair = materialPair();

  const iggy3d::PhysicsAabbContactSolvePlan normal =
      solve(&invalidNormal, &first, &second, &pair);
  const iggy3d::PhysicsAabbContactSolvePlan penetration =
      solve(&invalidPenetration, &first, &second, &pair);
  const iggy3d::PhysicsAabbContactSolvePlan bodyMismatch =
      solve(&mismatch, &first, &second, &pair);
  const iggy3d::PhysicsAabbContactSolvePlan bodyState =
      solve(&validContact, &invalidBody, &second, &pair);
  const iggy3d::PhysicsAabbContactSolvePlan material =
      solve(&validContact, &first, &second, &invalidMaterial);

  return expect(!normal.ok, "invalid normal rejected") &&
         expect(normal.reasonCode ==
                    "physics_aabb_contact_solve_invalid_contact",
                "invalid normal reason") &&
         expect(!penetration.ok, "invalid penetration rejected") &&
         expect(penetration.reasonCode ==
                    "physics_aabb_contact_solve_invalid_contact",
                "invalid penetration reason") &&
         expect(!bodyMismatch.ok, "body mismatch rejected") &&
         expect(bodyMismatch.reasonCode ==
                    "physics_aabb_contact_solve_body_mismatch",
                "body mismatch reason") &&
         expect(!bodyState.ok, "invalid body rejected") &&
         expect(bodyState.reasonCode ==
                    "physics_aabb_contact_solve_invalid_body_state",
                "invalid body reason") &&
         expect(!material.ok, "invalid material rejected") &&
         expect(material.reasonCode ==
                    "physics_aabb_contact_solve_invalid_material_pair",
                "invalid material reason");
}

bool zeroInverseMassPairHasNoEffectiveMass() {
  iggy3d::PhysicsAabbContact c = contact();
  iggy3d::PhysicsBodyView first =
      body({1U}, {}, 0.0F, iggy3d::PhysicsBodyMotionKind::Static);
  iggy3d::PhysicsBodyView second =
      body({2U}, {}, 0.0F, iggy3d::PhysicsBodyMotionKind::Static);
  iggy3d::PhysicsMaterialPairTraits pair = materialPair();

  const iggy3d::PhysicsAabbContactSolvePlan plan =
      solve(&c, &first, &second, &pair);

  return expect(!plan.ok, "no mass rejected") &&
         expect(plan.reasonCode ==
                    "physics_aabb_contact_solve_no_effective_mass",
                "no mass reason") &&
         expect(!plan.solveContact, "no mass not solving") &&
         expect(!plan.positionCorrectionApplied, "no position correction") &&
         expect(!plan.velocityImpulseApplied, "no velocity impulse") &&
         expect(iggy3d::nearlyEqual(plan.firstPositionDeltaMeters, {}),
                "first position zero") &&
         expect(iggy3d::nearlyEqual(plan.secondPositionDeltaMeters, {}),
                "second position zero");
}

bool dynamicAgainstStaticGetsPositionCorrectionOnlyOnDynamicBody() {
  iggy3d::PhysicsAabbContact c = contact(0.5F);
  iggy3d::PhysicsBodyView first = body({1U}, {}, 1.0F);
  iggy3d::PhysicsBodyView second =
      body({2U}, {}, 0.0F, iggy3d::PhysicsBodyMotionKind::Static);
  iggy3d::PhysicsMaterialPairTraits pair = materialPair();

  const iggy3d::PhysicsAabbContactSolvePlan plan =
      solve(&c, &first, &second, &pair);

  return expect(plan.ok, "dynamic static ok") &&
         expect(plan.reasonCode == "physics_aabb_contact_solved",
                "dynamic static reason") &&
         expect(plan.positionCorrectionApplied, "position correction applied") &&
         expect(iggy3d::nearlyEqual(plan.firstPositionDeltaMeters,
                                    {-0.3992F, 0.0F, 0.0F}),
                "dynamic correction") &&
         expect(iggy3d::nearlyEqual(plan.secondPositionDeltaMeters, {}),
                "static correction zero") &&
         expect(!plan.velocityImpulseApplied, "no velocity impulse at rest") &&
         expect(planIsFinite(plan), "dynamic static finite");
}

bool dynamicDynamicCorrectionUsesInverseMassWeights() {
  iggy3d::PhysicsAabbContact c = contact(0.4F);
  iggy3d::PhysicsBodyView first = body({1U}, {}, 1.0F);
  iggy3d::PhysicsBodyView second = body({2U}, {}, 3.0F);
  iggy3d::PhysicsMaterialPairTraits pair = materialPair();

  const iggy3d::PhysicsAabbContactSolvePlan plan =
      solve(&c, &first, &second, &pair, easyConfig());

  return expect(plan.ok, "dynamic dynamic ok") &&
         expect(plan.positionCorrectionApplied, "weighted correction applied") &&
         expect(nearlyEqual(plan.effectiveInverseMass, 4.0F),
                "effective mass") &&
         expect(iggy3d::nearlyEqual(plan.firstPositionDeltaMeters,
                                    {-0.1F, 0.0F, 0.0F}),
                "first weighted correction") &&
         expect(iggy3d::nearlyEqual(plan.secondPositionDeltaMeters,
                                    {0.3F, 0.0F, 0.0F}),
                "second weighted correction") &&
         expect(planIsFinite(plan), "dynamic dynamic finite");
}

bool restitutionCreatesNormalVelocityDeltasForApproach() {
  iggy3d::PhysicsAabbContact c = contact(0.0F);
  iggy3d::PhysicsBodyView first = body({1U}, {1.0F, 0.0F, 0.0F}, 1.0F);
  iggy3d::PhysicsBodyView second =
      body({2U}, {}, 0.0F, iggy3d::PhysicsBodyMotionKind::Static);
  iggy3d::PhysicsMaterialPairTraits pair = materialPair(0.5F);

  const iggy3d::PhysicsAabbContactSolvePlan plan =
      solve(&c, &first, &second, &pair, easyConfig());

  return expect(plan.ok, "restitution ok") &&
         expect(plan.velocityImpulseApplied, "velocity impulse applied") &&
         expect(nearlyEqual(plan.normalImpulseMagnitude, 1.5F),
                "normal impulse") &&
         expect(iggy3d::nearlyEqual(plan.firstVelocityDeltaMetersPerSecond,
                                    {-1.5F, 0.0F, 0.0F}),
                "first velocity delta") &&
         expect(iggy3d::nearlyEqual(plan.secondVelocityDeltaMetersPerSecond,
                                    {}),
                "second velocity zero") &&
         expect(planIsFinite(plan), "restitution finite");
}

bool separatingVelocitySkipsNormalImpulseButKeepsCorrection() {
  iggy3d::PhysicsAabbContact c = contact(0.25F);
  iggy3d::PhysicsBodyView first = body({1U}, {-1.0F, 0.0F, 0.0F}, 1.0F);
  iggy3d::PhysicsBodyView second =
      body({2U}, {}, 0.0F, iggy3d::PhysicsBodyMotionKind::Static);
  iggy3d::PhysicsMaterialPairTraits pair = materialPair();

  const iggy3d::PhysicsAabbContactSolvePlan plan =
      solve(&c, &first, &second, &pair, easyConfig());

  return expect(plan.ok, "separating ok") &&
         expect(plan.reasonCode ==
                    "physics_aabb_contact_solve_separating_velocity",
                "separating reason") &&
         expect(plan.positionCorrectionApplied, "separating correction") &&
         expect(!plan.velocityImpulseApplied, "separating no normal impulse") &&
         expect(nearlyEqual(plan.normalImpulseMagnitude, 0.0F),
                "separating normal impulse zero") &&
         expect(planIsFinite(plan), "separating finite");
}

bool frictionClampsTangentImpulseByDynamicFriction() {
  iggy3d::PhysicsAabbContact c = contact(0.0F);
  iggy3d::PhysicsBodyView first = body({1U}, {1.0F, 1.0F, 0.0F}, 1.0F);
  iggy3d::PhysicsBodyView second =
      body({2U}, {}, 0.0F, iggy3d::PhysicsBodyMotionKind::Static);
  iggy3d::PhysicsMaterialPairTraits pair = materialPair(0.0F, 0.25F);

  const iggy3d::PhysicsAabbContactSolvePlan plan =
      solve(&c, &first, &second, &pair, easyConfig());

  return expect(plan.ok, "friction ok") &&
         expect(plan.velocityImpulseApplied, "normal impulse applied") &&
         expect(plan.frictionImpulseApplied, "friction impulse applied") &&
         expect(nearlyEqual(plan.normalImpulseMagnitude, 1.0F),
                "friction normal impulse") &&
         expect(nearlyEqual(plan.frictionImpulseMagnitude, 0.25F),
                "friction clamp") &&
         expect(iggy3d::nearlyEqual(plan.firstVelocityDeltaMetersPerSecond,
                                    {-1.0F, -0.25F, 0.0F}),
                "friction first velocity") &&
         expect(iggy3d::nearlyEqual(plan.secondVelocityDeltaMetersPerSecond,
                                    {}),
                "friction second velocity") &&
         expect(planIsFinite(plan), "friction finite");
}

bool sensorContactAndTriggerPairAreNoSolveNoOps() {
  iggy3d::PhysicsAabbContact sensor = contact(0.5F, {1.0F, 0.0F, 0.0F}, true);
  iggy3d::PhysicsAabbContact c = contact();
  iggy3d::PhysicsBodyView first = body({1U}, {1.0F, 0.0F, 0.0F}, 1.0F);
  iggy3d::PhysicsBodyView second =
      body({2U}, {}, 0.0F, iggy3d::PhysicsBodyMotionKind::Static);
  iggy3d::PhysicsMaterialPairTraits pair = materialPair();
  iggy3d::PhysicsMaterialPairTraits trigger = materialPair(0.0F, 0.0F, false);

  const iggy3d::PhysicsAabbContactSolvePlan sensorPlan =
      solve(&sensor, &first, &second, &pair);
  const iggy3d::PhysicsAabbContactSolvePlan triggerPlan =
      solve(&c, &first, &second, &trigger);

  return expect(sensorPlan.ok, "sensor ok") &&
         expect(sensorPlan.reasonCode == "physics_aabb_contact_sensor",
                "sensor reason") &&
         expect(!sensorPlan.solveContact, "sensor no solve") &&
         expect(iggy3d::nearlyEqual(sensorPlan.firstPositionDeltaMeters, {}),
                "sensor position zero") &&
         expect(iggy3d::nearlyEqual(sensorPlan.firstVelocityDeltaMetersPerSecond,
                                    {}),
                "sensor velocity zero") &&
         expect(triggerPlan.ok, "trigger ok") &&
         expect(triggerPlan.reasonCode == "physics_aabb_contact_sensor",
                "trigger reason") &&
         expect(!triggerPlan.solveContact, "trigger no solve") &&
         expect(iggy3d::nearlyEqual(triggerPlan.firstPositionDeltaMeters, {}),
                "trigger position zero") &&
         expect(iggy3d::nearlyEqual(triggerPlan.firstVelocityDeltaMetersPerSecond,
                                    {}),
                "trigger velocity zero");
}

bool solverDoesNotMutateBodyInputs() {
  iggy3d::PhysicsAabbContact c = contact(0.25F);
  iggy3d::PhysicsBodyView first = body({1U}, {1.0F, 0.5F, 0.0F}, 1.0F);
  iggy3d::PhysicsBodyView second = body({2U}, {}, 2.0F);
  const iggy3d::PhysicsBodyView firstBefore = first;
  const iggy3d::PhysicsBodyView secondBefore = second;
  iggy3d::PhysicsMaterialPairTraits pair = materialPair(0.25F, 0.2F);

  const iggy3d::PhysicsAabbContactSolvePlan plan =
      solve(&c, &first, &second, &pair, easyConfig());

  return expect(plan.ok, "mutation proof plan ok") &&
         expect(first.id.value == firstBefore.id.value, "first id unchanged") &&
         expect(iggy3d::nearlyEqual(first.positionMeters,
                                    firstBefore.positionMeters),
                "first position unchanged") &&
         expect(iggy3d::nearlyEqual(first.velocityMetersPerSecond,
                                    firstBefore.velocityMetersPerSecond),
                "first velocity unchanged") &&
         expect(nearlyEqual(first.inverseMass, firstBefore.inverseMass),
                "first inverse unchanged") &&
         expect(second.id.value == secondBefore.id.value,
                "second id unchanged") &&
         expect(iggy3d::nearlyEqual(second.positionMeters,
                                    secondBefore.positionMeters),
                "second position unchanged") &&
         expect(iggy3d::nearlyEqual(second.velocityMetersPerSecond,
                                    secondBefore.velocityMetersPerSecond),
                "second velocity unchanged") &&
         expect(nearlyEqual(second.inverseMass, secondBefore.inverseMass),
                "second inverse unchanged");
}

}  // namespace

int main() {
  const bool ok = stableNamesAreLowerSnake() &&
                  nullAndInvalidInputsReject() &&
                  invalidContactBodyAndMaterialReject() &&
                  zeroInverseMassPairHasNoEffectiveMass() &&
                  dynamicAgainstStaticGetsPositionCorrectionOnlyOnDynamicBody() &&
                  dynamicDynamicCorrectionUsesInverseMassWeights() &&
                  restitutionCreatesNormalVelocityDeltasForApproach() &&
                  separatingVelocitySkipsNormalImpulseButKeepsCorrection() &&
                  frictionClampsTangentImpulseByDynamicFriction() &&
                  sensorContactAndTriggerPairAreNoSolveNoOps() &&
                  solverDoesNotMutateBodyInputs();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
