#include "runtime/physics/PhysicsAabbContactSolver.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace iggy3d {
namespace {

constexpr float kNormalEpsilonSquared = 0.000001F;
constexpr float kTangentSpeedEpsilonSquared = 0.000001F;
constexpr float kMassEpsilon = 0.000001F;

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1092
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

PhysicsAabbContactSolvePlan solvePlan(PhysicsAabbContactSolveStatus status,
                                      bool ok) {
  PhysicsAabbContactSolvePlan plan;
  plan.ok = ok;
  plan.status = status;
  plan.reasonCode = physicsAabbContactSolveStatusName(status);
  return plan;
}

bool finiteNonNegative(float value) {
  return std::isfinite(value) && value >= 0.0F;
}

bool finiteUnitRange(float value) {
  return std::isfinite(value) && value >= 0.0F && value <= 1.0F;
}

bool validBodyState(const PhysicsBodyView& body) {
  return isValidPhysicsBodyId(body.id) && isFinite(body.positionMeters) &&
         isFinite(body.velocityMetersPerSecond) &&
         std::isfinite(body.massKilograms) &&
         finiteNonNegative(body.inverseMass);
}

bool validMaterialPair(const PhysicsMaterialPairTraits& materialPair) {
  return std::isfinite(materialPair.staticFriction) &&
         materialPair.staticFriction >= 0.0F &&
         std::isfinite(materialPair.dynamicFriction) &&
         materialPair.dynamicFriction >= 0.0F &&
         std::isfinite(materialPair.restitution) &&
         materialPair.restitution >= 0.0F &&
         materialPair.restitution <= 1.0F &&
         std::isfinite(materialPair.dampingMultiplier) &&
         materialPair.dampingMultiplier >= 0.0F &&
         materialPair.dampingMultiplier <= 1.0F;
}

bool validContact(const PhysicsAabbContact& contact) {
  return isValidPhysicsBodyId(contact.firstBodyId) &&
         isValidPhysicsBodyId(contact.secondBodyId) &&
         isFinite(contact.normalFromFirstToSecond) &&
         lengthSquared(contact.normalFromFirstToSecond) >
             kNormalEpsilonSquared &&
         std::isfinite(contact.penetrationMeters) &&
         contact.penetrationMeters >= 0.0F && isFinite(contact.pointMeters);
}

void applyPositionCorrection(const PhysicsAabbContact& contact,
                             const PhysicsBodyView& first,
                             const PhysicsBodyView& second,
                             const PhysicsAabbContactSolveConfig& config,
                             Vec3 normal,
                             PhysicsAabbContactSolvePlan& plan) {
  const float correctedDepth = std::min(
      std::max(contact.penetrationMeters - config.penetrationSlopMeters, 0.0F) *
          config.positionCorrectionPercent,
      config.maxPositionCorrectionMeters);
  // branch-gate: BG-1092
  if (correctedDepth <= 0.0F) {
    return;
  }
  const Vec3 correction = normal * (correctedDepth / plan.effectiveInverseMass);
  plan.firstPositionDeltaMeters = correction * -first.inverseMass;
  plan.secondPositionDeltaMeters = correction * second.inverseMass;
  plan.positionCorrectionApplied = true;
}

PhysicsAabbContactSolveStatus applyVelocitySolve(
    const PhysicsBodyView& first,
    const PhysicsBodyView& second,
    const PhysicsMaterialPairTraits& materialPair,
    const PhysicsAabbContactSolveConfig& config,
    Vec3 normal,
    PhysicsAabbContactSolvePlan& plan) {
  // branch-gate: BG-1092
  if (!config.enableVelocitySolve) {
    return PhysicsAabbContactSolveStatus::Solved;
  }

  const Vec3 relativeVelocity =
      second.velocityMetersPerSecond - first.velocityMetersPerSecond;
  const float normalVelocity = dot(relativeVelocity, normal);
  // branch-gate: BG-1092
  if (normalVelocity > 0.0F) {
    return PhysicsAabbContactSolveStatus::SeparatingVelocity;
  }

  const float normalImpulse = std::max(
      0.0F,
      -(1.0F + materialPair.restitution) * normalVelocity /
          plan.effectiveInverseMass);
  // branch-gate: BG-1092
  if (normalImpulse <= 0.0F) {
    return PhysicsAabbContactSolveStatus::Solved;
  }

  Vec3 firstVelocityDelta = normal * (-normalImpulse * first.inverseMass);
  Vec3 secondVelocityDelta = normal * (normalImpulse * second.inverseMass);
  plan.normalImpulseMagnitude = normalImpulse;
  plan.velocityImpulseApplied = true;

  const Vec3 tangentVelocity = relativeVelocity - normal * normalVelocity;
  const float tangentSpeedSquared = lengthSquared(tangentVelocity);
  // branch-gate: BG-1092
  if (config.enableFriction &&
      tangentSpeedSquared > kTangentSpeedEpsilonSquared) {
    const Vec3 tangent = normalized(tangentVelocity);
    const float tangentVelocityMagnitude = std::sqrt(tangentSpeedSquared);
    const float rawFrictionImpulse =
        -tangentVelocityMagnitude / plan.effectiveInverseMass;
    const float frictionLimit =
        normalImpulse * materialPair.dynamicFriction;
    const float frictionImpulse = std::clamp(rawFrictionImpulse,
                                             -frictionLimit,
                                             frictionLimit);
    // branch-gate: BG-1092
    if (std::fabs(frictionImpulse) > 0.0F) {
      firstVelocityDelta =
          firstVelocityDelta + tangent * (-frictionImpulse * first.inverseMass);
      secondVelocityDelta = secondVelocityDelta +
                            tangent * (frictionImpulse * second.inverseMass);
      plan.frictionImpulseMagnitude = std::fabs(frictionImpulse);
      plan.frictionImpulseApplied = true;
    }
  }

  plan.firstVelocityDeltaMetersPerSecond = firstVelocityDelta;
  plan.secondVelocityDeltaMetersPerSecond = secondVelocityDelta;
  return PhysicsAabbContactSolveStatus::Solved;
}

bool emittedPlanIsFinite(const PhysicsAabbContactSolvePlan& plan) {
  return isFinite(plan.firstPositionDeltaMeters) &&
         isFinite(plan.secondPositionDeltaMeters) &&
         isFinite(plan.firstVelocityDeltaMetersPerSecond) &&
         isFinite(plan.secondVelocityDeltaMetersPerSecond) &&
         std::isfinite(plan.normalImpulseMagnitude) &&
         std::isfinite(plan.frictionImpulseMagnitude) &&
         std::isfinite(plan.effectiveInverseMass);
}

}  // namespace

std::string_view physicsAabbContactSolveStatusName(
    PhysicsAabbContactSolveStatus status) {
  static constexpr std::array<std::string_view, 12> kNames{
      "physics_aabb_contact_solved",
      "physics_aabb_contact_solve_missing_contact",
      "physics_aabb_contact_solve_missing_first_body",
      "physics_aabb_contact_solve_missing_second_body",
      "physics_aabb_contact_solve_body_mismatch",
      "physics_aabb_contact_solve_invalid_contact",
      "physics_aabb_contact_solve_invalid_body_state",
      "physics_aabb_contact_solve_invalid_material_pair",
      "physics_aabb_contact_solve_invalid_config",
      "physics_aabb_contact_solve_no_effective_mass",
      "physics_aabb_contact_sensor",
      "physics_aabb_contact_solve_separating_velocity",
  };
  return enumName(status, kNames,
                  "physics_aabb_contact_solve_invalid_contact");
}

bool isValidPhysicsAabbContactSolveConfig(
    const PhysicsAabbContactSolveConfig& config) {
  return finiteNonNegative(config.penetrationSlopMeters) &&
         finiteUnitRange(config.positionCorrectionPercent) &&
         finiteNonNegative(config.maxPositionCorrectionMeters);
}

PhysicsAabbContactSolvePlan solvePhysicsAabbContact(
    const PhysicsAabbContactSolveRequest& request) {
  // branch-gate: BG-1092
  if (request.contact == nullptr) {
    return solvePlan(PhysicsAabbContactSolveStatus::MissingContact, false);
  }
  // branch-gate: BG-1092
  if (request.firstBody == nullptr) {
    return solvePlan(PhysicsAabbContactSolveStatus::MissingFirstBody, false);
  }
  // branch-gate: BG-1092
  if (request.secondBody == nullptr) {
    return solvePlan(PhysicsAabbContactSolveStatus::MissingSecondBody, false);
  }
  // branch-gate: BG-1092
  if (request.materialPair == nullptr) {
    return solvePlan(PhysicsAabbContactSolveStatus::InvalidMaterialPair, false);
  }
  // branch-gate: BG-1092
  if (!isValidPhysicsAabbContactSolveConfig(request.config)) {
    return solvePlan(PhysicsAabbContactSolveStatus::InvalidConfig, false);
  }
  // branch-gate: BG-1092
  if (!validContact(*request.contact)) {
    return solvePlan(PhysicsAabbContactSolveStatus::InvalidContact, false);
  }
  // branch-gate: BG-1092
  if (!validBodyState(*request.firstBody) ||
      !validBodyState(*request.secondBody)) {
    return solvePlan(PhysicsAabbContactSolveStatus::InvalidBodyState, false);
  }
  // branch-gate: BG-1092
  if (request.contact->firstBodyId.value != request.firstBody->id.value ||
      request.contact->secondBodyId.value != request.secondBody->id.value) {
    return solvePlan(PhysicsAabbContactSolveStatus::BodyMismatch, false);
  }
  // branch-gate: BG-1092
  if (!validMaterialPair(*request.materialPair)) {
    return solvePlan(PhysicsAabbContactSolveStatus::InvalidMaterialPair, false);
  }

  PhysicsAabbContactSolvePlan plan =
      solvePlan(PhysicsAabbContactSolveStatus::Solved, true);
  plan.firstBodyId = request.contact->firstBodyId;
  plan.secondBodyId = request.contact->secondBodyId;
  plan.solveContact = request.materialPair->solveContact &&
                      !request.contact->includesSensor;
  // branch-gate: BG-1092
  if (!plan.solveContact) {
    plan.status = PhysicsAabbContactSolveStatus::SensorContact;
    plan.reasonCode = physicsAabbContactSolveStatusName(plan.status);
    return plan;
  }

  plan.effectiveInverseMass =
      request.firstBody->inverseMass + request.secondBody->inverseMass;
  // branch-gate: BG-1092
  if (plan.effectiveInverseMass <= kMassEpsilon) {
    plan.status = PhysicsAabbContactSolveStatus::NoEffectiveMass;
    plan.reasonCode = physicsAabbContactSolveStatusName(plan.status);
    plan.ok = false;
    plan.solveContact = false;
    return plan;
  }

  const Vec3 normal = normalized(request.contact->normalFromFirstToSecond);
  applyPositionCorrection(*request.contact, *request.firstBody,
                          *request.secondBody, request.config, normal, plan);
  const PhysicsAabbContactSolveStatus velocityStatus = applyVelocitySolve(
      *request.firstBody, *request.secondBody, *request.materialPair,
      request.config, normal, plan);
  // branch-gate: BG-1092
  if (velocityStatus == PhysicsAabbContactSolveStatus::SeparatingVelocity) {
    plan.status = PhysicsAabbContactSolveStatus::SeparatingVelocity;
    plan.reasonCode = physicsAabbContactSolveStatusName(plan.status);
  }
  // branch-gate: BG-1092
  if (!emittedPlanIsFinite(plan)) {
    return solvePlan(PhysicsAabbContactSolveStatus::InvalidBodyState, false);
  }
  return plan;
}

}  // namespace iggy3d
