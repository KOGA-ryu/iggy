#pragma once

#include <cstdint>
#include <string_view>

#include "runtime/physics/PhysicsAabbContact.hpp"
#include "runtime/physics/PhysicsBodyStore.hpp"
#include "runtime/physics/PhysicsMaterialTraits.hpp"

namespace iggy3d {

enum class PhysicsAabbContactSolveStatus : std::uint8_t {
  Solved,
  MissingContact,
  MissingFirstBody,
  MissingSecondBody,
  BodyMismatch,
  InvalidContact,
  InvalidBodyState,
  InvalidMaterialPair,
  InvalidConfig,
  NoEffectiveMass,
  SensorContact,
  SeparatingVelocity,
};

struct PhysicsAabbContactSolveConfig {
  float penetrationSlopMeters = 0.001F;
  float positionCorrectionPercent = 0.80F;
  float maxPositionCorrectionMeters = 1.0F;
  bool enableVelocitySolve = true;
  bool enableFriction = true;
};

struct PhysicsAabbContactSolveRequest {
  const PhysicsAabbContact* contact = nullptr;
  const PhysicsBodyView* firstBody = nullptr;
  const PhysicsBodyView* secondBody = nullptr;
  const PhysicsMaterialPairTraits* materialPair = nullptr;
  PhysicsAabbContactSolveConfig config;
};

struct PhysicsAabbContactSolvePlan {
  bool ok = false;
  PhysicsAabbContactSolveStatus status =
      PhysicsAabbContactSolveStatus::MissingContact;
  std::string_view reasonCode = "physics_aabb_contact_solve_missing_contact";
  PhysicsBodyId firstBodyId;
  PhysicsBodyId secondBodyId;
  bool solveContact = false;
  bool positionCorrectionApplied = false;
  bool velocityImpulseApplied = false;
  bool frictionImpulseApplied = false;
  Vec3 firstPositionDeltaMeters;
  Vec3 secondPositionDeltaMeters;
  Vec3 firstVelocityDeltaMetersPerSecond;
  Vec3 secondVelocityDeltaMetersPerSecond;
  float normalImpulseMagnitude = 0.0F;
  float frictionImpulseMagnitude = 0.0F;
  float effectiveInverseMass = 0.0F;
};

std::string_view physicsAabbContactSolveStatusName(
    PhysicsAabbContactSolveStatus status);
bool isValidPhysicsAabbContactSolveConfig(
    const PhysicsAabbContactSolveConfig& config);
PhysicsAabbContactSolvePlan solvePhysicsAabbContact(
    const PhysicsAabbContactSolveRequest& request);

}  // namespace iggy3d
