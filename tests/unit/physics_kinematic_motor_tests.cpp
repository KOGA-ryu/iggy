#include "runtime/physics/PhysicsKinematicMotor.hpp"

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

bool nearlyEqual(float lhs, float rhs, float epsilon = 0.0001F) {
  const float delta = lhs - rhs;
  return delta >= -epsilon && delta <= epsilon;
}

iggy3d::PhysicsAabbCollider colliderAt(iggy3d::PhysicsBodyId id,
                                       iggy3d::Vec3 center,
                                       iggy3d::Vec3 halfExtents,
                                       bool sensor = false) {
  iggy3d::PhysicsAabbCollider collider;
  collider.bodyId = id;
  collider.worldCenterMeters = center;
  collider.halfExtentsMeters = halfExtents;
  collider.bounds = iggy3d::aabbFromCenterExtents(center, halfExtents);
  collider.sensor = sensor;
  return collider;
}

iggy3d::PhysicsKinematicMotorConfig noGroundConfig() {
  iggy3d::PhysicsKinematicMotorConfig config;
  config.skinMeters = 0.10F;
  config.groundProbeDistanceMeters = 0.0F;
  config.groundSnapDistanceMeters = 0.0F;
  return config;
}

bool colliderUnchanged(const iggy3d::PhysicsAabbCollider& lhs,
                       const iggy3d::PhysicsAabbCollider& rhs) {
  return lhs.bodyId.value == rhs.bodyId.value &&
         iggy3d::nearlyEqual(lhs.worldCenterMeters, rhs.worldCenterMeters) &&
         iggy3d::nearlyEqual(lhs.halfExtentsMeters, rhs.halfExtentsMeters) &&
         iggy3d::nearlyEqual(lhs.bounds.min, rhs.bounds.min) &&
         iggy3d::nearlyEqual(lhs.bounds.max, rhs.bounds.max) &&
         lhs.sensor == rhs.sensor;
}

bool collidersUnchanged(
    const std::vector<iggy3d::PhysicsAabbCollider>& lhs,
    const std::vector<iggy3d::PhysicsAabbCollider>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < lhs.size(); ++index) {
    if (!colliderUnchanged(lhs[index], rhs[index])) {
      return false;
    }
  }
  return true;
}

iggy3d::PhysicsKinematicMotorRequest requestFor(
    const std::vector<iggy3d::PhysicsAabbCollider>* colliders,
    const iggy3d::PhysicsAabbCollider* body,
    iggy3d::Vec3 desired,
    iggy3d::PhysicsKinematicMotorConfig config = noGroundConfig(),
    bool includeSensors = false) {
  iggy3d::PhysicsKinematicMotorRequest request;
  request.colliders = colliders;
  request.bodyCollider = body;
  request.desiredDisplacementMeters = desired;
  request.includeSensors = includeSensors;
  request.config = config;
  return request;
}

bool statusNamesAndConfigValidation() {
  iggy3d::PhysicsKinematicMotorConfig valid;
  iggy3d::PhysicsKinematicMotorConfig invalidIterations = valid;
  invalidIterations.maxIterations = 0U;
  iggy3d::PhysicsKinematicMotorConfig invalidHighIterations = valid;
  invalidHighIterations.maxIterations = 9U;
  iggy3d::PhysicsKinematicMotorConfig invalidSkin = valid;
  invalidSkin.skinMeters = -0.01F;
  iggy3d::PhysicsKinematicMotorConfig invalidProbe = valid;
  invalidProbe.groundProbeDistanceMeters =
      std::numeric_limits<float>::infinity();
  iggy3d::PhysicsKinematicMotorConfig invalidSnap = valid;
  invalidSnap.groundSnapDistanceMeters = -0.01F;
  iggy3d::PhysicsKinematicMotorConfig invalidMin = valid;
  invalidMin.minMoveDistanceMeters = 0.0F;
  iggy3d::PhysicsKinematicMotorConfig invalidMax = valid;
  invalidMax.maxMoveDistanceMeters = 0.0F;
  iggy3d::PhysicsKinematicMotorConfig invalidRange = valid;
  invalidRange.minMoveDistanceMeters = 2.0F;
  invalidRange.maxMoveDistanceMeters = 1.0F;

  return expect(iggy3d::physicsKinematicMotorStatusName(
                    iggy3d::PhysicsKinematicMotorStatus::Planned) ==
                    "physics_kinematic_motor_planned",
                "planned status") &&
         expect(iggy3d::physicsKinematicMotorStatusName(
                    iggy3d::PhysicsKinematicMotorStatus::MissingColliders) ==
                    "physics_kinematic_motor_missing_colliders",
                "missing colliders status") &&
         expect(iggy3d::physicsKinematicMotorStatusName(
                    iggy3d::PhysicsKinematicMotorStatus::MissingBodyCollider) ==
                    "physics_kinematic_motor_missing_body_collider",
                "missing body status") &&
         expect(iggy3d::physicsKinematicMotorStatusName(
                    iggy3d::PhysicsKinematicMotorStatus::InvalidBodyCollider) ==
                    "physics_kinematic_motor_invalid_body_collider",
                "invalid body status") &&
         expect(iggy3d::physicsKinematicMotorStatusName(
                    iggy3d::PhysicsKinematicMotorStatus::
                        InvalidDesiredDisplacement) ==
                    "physics_kinematic_motor_invalid_desired_displacement",
                "invalid displacement status") &&
         expect(iggy3d::physicsKinematicMotorStatusName(
                    iggy3d::PhysicsKinematicMotorStatus::InvalidConfig) ==
                    "physics_kinematic_motor_invalid_config",
                "invalid config status") &&
         expect(iggy3d::physicsKinematicMotorStatusName(
                    iggy3d::PhysicsKinematicMotorStatus::
                        MaxDisplacementExceeded) ==
                    "physics_kinematic_motor_max_displacement_exceeded",
                "max displacement status") &&
         expect(iggy3d::physicsKinematicMotorStatusName(
                    iggy3d::PhysicsKinematicMotorStatus::CollisionQueryFailed) ==
                    "physics_kinematic_motor_collision_query_failed",
                "query failed status") &&
         expect(iggy3d::physicsKinematicMotorStatusName(
                    iggy3d::PhysicsKinematicMotorStatus::GroundQueryFailed) ==
                    "physics_kinematic_motor_ground_query_failed",
                "ground failed status") &&
         expect(iggy3d::isValidPhysicsKinematicMotorConfig(valid),
                "valid config") &&
         expect(!iggy3d::isValidPhysicsKinematicMotorConfig(invalidIterations),
                "invalid low iterations") &&
         expect(!iggy3d::isValidPhysicsKinematicMotorConfig(
                    invalidHighIterations),
                "invalid high iterations") &&
         expect(!iggy3d::isValidPhysicsKinematicMotorConfig(invalidSkin),
                "invalid skin") &&
         expect(!iggy3d::isValidPhysicsKinematicMotorConfig(invalidProbe),
                "invalid probe") &&
         expect(!iggy3d::isValidPhysicsKinematicMotorConfig(invalidSnap),
                "invalid snap") &&
         expect(!iggy3d::isValidPhysicsKinematicMotorConfig(invalidMin),
                "invalid min") &&
         expect(!iggy3d::isValidPhysicsKinematicMotorConfig(invalidMax),
                "invalid max") &&
         expect(!iggy3d::isValidPhysicsKinematicMotorConfig(invalidRange),
                "invalid range");
}

bool invalidRequestsRejectWithoutMovement() {
  std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {4.0F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F}),
  };
  const iggy3d::PhysicsAabbCollider body =
      colliderAt({99U}, {}, {0.5F, 0.5F, 0.5F});
  iggy3d::PhysicsAabbCollider invalidBody = body;
  invalidBody.bodyId = {};
  iggy3d::PhysicsAabbCollider invalidStored = colliders.front();
  invalidStored.bodyId = {};
  std::vector<iggy3d::PhysicsAabbCollider> invalidColliders{invalidStored};

  iggy3d::PhysicsKinematicMotorRequest invalidDisplacement =
      requestFor(&colliders, &body, {});
  invalidDisplacement.desiredDisplacementMeters.x =
      std::numeric_limits<float>::infinity();
  iggy3d::PhysicsKinematicMotorConfig capped = noGroundConfig();
  capped.maxMoveDistanceMeters = 1.0F;

  const iggy3d::PhysicsKinematicMotorResult missingColliders =
      iggy3d::planPhysicsKinematicAabbMove({nullptr, &body, {}, false,
                                            noGroundConfig()});
  const iggy3d::PhysicsKinematicMotorResult missingBody =
      iggy3d::planPhysicsKinematicAabbMove({&colliders, nullptr, {}, false,
                                            noGroundConfig()});
  const iggy3d::PhysicsKinematicMotorResult badBody =
      iggy3d::planPhysicsKinematicAabbMove(
          requestFor(&colliders, &invalidBody, {}));
  const iggy3d::PhysicsKinematicMotorResult badDisplacement =
      iggy3d::planPhysicsKinematicAabbMove(invalidDisplacement);
  const iggy3d::PhysicsKinematicMotorResult maxExceeded =
      iggy3d::planPhysicsKinematicAabbMove(
          requestFor(&colliders, &body, {2.0F, 0.0F, 0.0F}, capped));
  const iggy3d::PhysicsKinematicMotorResult badColliderPacket =
      iggy3d::planPhysicsKinematicAabbMove(
          requestFor(&invalidColliders, &body, {}));

  return expect(!missingColliders.ok, "missing colliders rejected") &&
         expect(missingColliders.reasonCode ==
                    "physics_kinematic_motor_missing_colliders",
                "missing colliders reason") &&
         expect(!missingBody.ok, "missing body rejected") &&
         expect(missingBody.reasonCode ==
                    "physics_kinematic_motor_missing_body_collider",
                "missing body reason") &&
         expect(!badBody.ok, "invalid body rejected") &&
         expect(badBody.reasonCode ==
                    "physics_kinematic_motor_invalid_body_collider",
                "invalid body reason") &&
         expect(!badDisplacement.ok, "invalid displacement rejected") &&
         expect(badDisplacement.reasonCode ==
                    "physics_kinematic_motor_invalid_desired_displacement",
                "invalid displacement reason") &&
         expect(!maxExceeded.ok, "max movement rejected") &&
         expect(maxExceeded.reasonCode ==
                    "physics_kinematic_motor_max_displacement_exceeded",
                "max movement reason") &&
         expect(!badColliderPacket.ok, "bad collider packet rejected") &&
         expect(badColliderPacket.reasonCode ==
                    "physics_kinematic_motor_collision_query_failed",
                "bad collider packet reason") &&
         expect(badColliderPacket.upstreamReasonCode ==
                    "physics_collision_query_invalid_collider",
                "bad collider upstream reason") &&
         expect(iggy3d::nearlyEqual(badBody.appliedDisplacementMeters, {}),
                "invalid request no movement");
}

bool zeroDisplacementAndClearPath() {
  std::vector<iggy3d::PhysicsAabbCollider> colliders;
  const iggy3d::PhysicsAabbCollider body =
      colliderAt({99U}, {}, {0.5F, 0.5F, 0.5F});

  const iggy3d::PhysicsKinematicMotorResult zero =
      iggy3d::planPhysicsKinematicAabbMove(
          requestFor(&colliders, &body, {}));
  const iggy3d::PhysicsKinematicMotorResult clear =
      iggy3d::planPhysicsKinematicAabbMove(
          requestFor(&colliders, &body, {1.0F, 0.0F, 2.0F}));

  return expect(zero.ok, "zero movement ok") &&
         expect(zero.iterationCount == 0U, "zero movement no iterations") &&
         expect(iggy3d::nearlyEqual(zero.finalCenterMeters, {}),
                "zero movement final center") &&
         expect(iggy3d::nearlyEqual(zero.appliedDisplacementMeters, {}),
                "zero movement applied") &&
         expect(clear.ok, "clear movement ok") &&
         expect(clear.iterationCount == 1U, "clear movement one iteration") &&
         expect(!clear.blocked, "clear movement not blocked") &&
         expect(iggy3d::nearlyEqual(clear.finalCenterMeters,
                                    {1.0F, 0.0F, 2.0F}),
                "clear movement final center") &&
         expect(iggy3d::nearlyEqual(clear.appliedDisplacementMeters,
                                    {1.0F, 0.0F, 2.0F}),
                "clear movement applied") &&
         expect(iggy3d::nearlyEqual(clear.remainingDisplacementMeters, {}),
                "clear movement remaining");
}

bool wallCollisionStopsBeforeImpact() {
  std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {2.5F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F}),
  };
  const std::vector<iggy3d::PhysicsAabbCollider> before = colliders;
  const iggy3d::PhysicsAabbCollider body =
      colliderAt({99U}, {}, {0.5F, 0.5F, 0.5F});
  const iggy3d::PhysicsAabbCollider bodyBefore = body;
  const iggy3d::PhysicsKinematicMotorResult result =
      iggy3d::planPhysicsKinematicAabbMove(
          requestFor(&colliders, &body, {3.0F, 0.0F, 0.0F}));

  return expect(result.ok, "wall movement ok") &&
         expect(result.blocked, "wall movement blocked") &&
         expect(result.hitCount == 1U, "wall movement one hit") &&
         expect(result.hits[0].colliderIndex == 0U,
                "wall hit collider index") &&
         expect(iggy3d::nearlyEqual(result.hits[0].normalFromColliderToMotor,
                                    {-1.0F, 0.0F, 0.0F}),
                "wall hit normal") &&
         expect(nearlyEqual(result.finalCenterMeters.x, 1.4F),
                "wall movement skin stop") &&
         expect(iggy3d::nearlyEqual(result.remainingDisplacementMeters, {}),
                "wall movement no remaining") &&
         expect(collidersUnchanged(colliders, before),
                "wall movement colliders unchanged") &&
         expect(colliderUnchanged(body, bodyBefore),
                "wall movement body collider unchanged");
}

bool diagonalMovementSlidesAlongWall() {
  std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {2.5F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F}),
  };
  const iggy3d::PhysicsAabbCollider body =
      colliderAt({99U}, {}, {0.5F, 0.5F, 0.5F});
  const iggy3d::PhysicsKinematicMotorResult result =
      iggy3d::planPhysicsKinematicAabbMove(
          requestFor(&colliders, &body, {3.0F, 0.0F, 2.0F}));

  return expect(result.ok, "slide movement ok") &&
         expect(result.blocked, "slide movement blocked") &&
         expect(result.hitCount == 1U, "slide one wall hit") &&
         expect(result.iterationCount == 2U, "slide uses second iteration") &&
         expect(nearlyEqual(result.finalCenterMeters.x, 1.4168F, 0.001F),
                "slide final x") &&
         expect(nearlyEqual(result.finalCenterMeters.z, 2.0F, 0.001F),
                "slide final z") &&
         expect(iggy3d::nearlyEqual(result.remainingDisplacementMeters, {}),
                "slide no remaining");
}

bool cornerCaseIsBoundedAndDeterministic() {
  std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {2.5F, 0.0F, 0.0F}, {0.5F, 0.5F, 5.0F}),
      colliderAt({2U}, {0.0F, 0.0F, 2.5F}, {5.0F, 0.5F, 0.5F}),
  };
  const iggy3d::PhysicsAabbCollider body =
      colliderAt({99U}, {}, {0.5F, 0.5F, 0.5F});
  const iggy3d::PhysicsKinematicMotorResult first =
      iggy3d::planPhysicsKinematicAabbMove(
          requestFor(&colliders, &body, {3.0F, 0.0F, 3.0F}));
  const iggy3d::PhysicsKinematicMotorResult second =
      iggy3d::planPhysicsKinematicAabbMove(
          requestFor(&colliders, &body, {3.0F, 0.0F, 3.0F}));

  return expect(first.ok, "corner movement ok") &&
         expect(first.blocked, "corner movement blocked") &&
         expect(first.hitCount == 2U, "corner two hits") &&
         expect(first.iterationCount <= noGroundConfig().maxIterations,
                "corner bounded iterations") &&
         expect(first.finalCenterMeters.x < 1.5F,
                "corner final x before wall") &&
         expect(first.finalCenterMeters.z < 1.5F,
                "corner final z before wall") &&
         expect(iggy3d::nearlyEqual(first.finalCenterMeters,
                                    second.finalCenterMeters),
                "corner deterministic final center") &&
         expect(first.hitCount == second.hitCount,
                "corner deterministic hit count");
}

bool groundProbeAndSnapUseQueryLayer() {
  std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {0.0F, 0.0F, 0.0F}, {5.0F, 0.5F, 5.0F}),
  };
  iggy3d::PhysicsKinematicMotorConfig config;
  config.groundProbeDistanceMeters = 0.25F;
  config.groundSnapDistanceMeters = 0.0F;
  const iggy3d::PhysicsAabbCollider groundedBody =
      colliderAt({99U}, {0.0F, 1.1F, 0.0F}, {0.5F, 0.5F, 0.5F});
  const iggy3d::PhysicsAabbCollider highBody =
      colliderAt({100U}, {0.0F, 3.0F, 0.0F}, {0.5F, 0.5F, 0.5F});
  const iggy3d::PhysicsKinematicMotorResult grounded =
      iggy3d::planPhysicsKinematicAabbMove(
          requestFor(&colliders, &groundedBody, {}, config));
  const iggy3d::PhysicsKinematicMotorResult tooHigh =
      iggy3d::planPhysicsKinematicAabbMove(
          requestFor(&colliders, &highBody, {}, config));

  config.groundProbeDistanceMeters = 0.10F;
  config.groundSnapDistanceMeters = 0.75F;
  const iggy3d::PhysicsAabbCollider snapBody =
      colliderAt({101U}, {0.0F, 1.55F, 0.0F}, {0.5F, 0.5F, 0.5F});
  const iggy3d::PhysicsKinematicMotorResult snapped =
      iggy3d::planPhysicsKinematicAabbMove(
          requestFor(&colliders, &snapBody, {}, config));

  return expect(grounded.ok, "grounded motor ok") &&
         expect(grounded.grounded, "grounded detected") &&
         expect(nearlyEqual(grounded.groundDistanceMeters, 0.1F),
                "ground distance") &&
         expect(iggy3d::nearlyEqual(grounded.groundNormal,
                                    {0.0F, 1.0F, 0.0F}),
                "ground normal") &&
         expect(tooHigh.ok, "too high motor ok") &&
         expect(!tooHigh.grounded, "too high not grounded") &&
         expect(snapped.ok, "snap motor ok") &&
         expect(snapped.grounded, "snap grounded") &&
         expect(snapped.snappedToGround, "snap flag") &&
         expect(nearlyEqual(snapped.groundSnapDistanceMetersApplied, 0.55F),
                "snap distance") &&
         expect(nearlyEqual(snapped.finalCenterMeters.y, 1.0F),
                "snap final y");
}

bool sensorPolicyIsHonored() {
  std::vector<iggy3d::PhysicsAabbCollider> colliders{
      colliderAt({1U}, {2.5F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F}, true),
  };
  const iggy3d::PhysicsAabbCollider body =
      colliderAt({99U}, {}, {0.5F, 0.5F, 0.5F});
  const iggy3d::PhysicsKinematicMotorResult skipped =
      iggy3d::planPhysicsKinematicAabbMove(
          requestFor(&colliders, &body, {3.0F, 0.0F, 0.0F}));
  const iggy3d::PhysicsKinematicMotorResult included =
      iggy3d::planPhysicsKinematicAabbMove(
          requestFor(&colliders, &body, {3.0F, 0.0F, 0.0F},
                     noGroundConfig(), true));

  return expect(skipped.ok, "sensor skipped ok") &&
         expect(!skipped.blocked, "sensor skipped not blocked") &&
         expect(iggy3d::nearlyEqual(skipped.finalCenterMeters,
                                    {3.0F, 0.0F, 0.0F}),
                "sensor skipped full movement") &&
         expect(included.ok, "sensor included ok") &&
         expect(included.blocked, "sensor included blocked") &&
         expect(included.hitCount == 1U, "sensor included hit") &&
         expect(included.hits[0].sensor, "sensor hit flag");
}

}  // namespace

int main() {
  const bool ok = statusNamesAndConfigValidation() &&
                  invalidRequestsRejectWithoutMovement() &&
                  zeroDisplacementAndClearPath() &&
                  wallCollisionStopsBeforeImpact() &&
                  diagonalMovementSlidesAlongWall() &&
                  cornerCaseIsBoundedAndDeterministic() &&
                  groundProbeAndSnapUseQueryLayer() &&
                  sensorPolicyIsHonored();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
