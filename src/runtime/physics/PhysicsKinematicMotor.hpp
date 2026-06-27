#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "runtime/physics/PhysicsCollisionQueries.hpp"

namespace iggy3d {

enum class PhysicsKinematicMotorStatus : std::uint8_t {
  Planned,
  MissingColliders,
  MissingBodyCollider,
  InvalidBodyCollider,
  InvalidDesiredDisplacement,
  InvalidConfig,
  MaxDisplacementExceeded,
  CollisionQueryFailed,
  GroundQueryFailed,
};

struct PhysicsKinematicMotorConfig {
  std::uint8_t maxIterations = 3U;
  float skinMeters = 0.001F;
  float groundProbeDistanceMeters = 0.10F;
  float groundSnapDistanceMeters = 0.05F;
  float minMoveDistanceMeters = 0.0001F;
  float maxMoveDistanceMeters = 20.0F;
};

struct PhysicsKinematicMotorRequest {
  const std::vector<PhysicsAabbCollider>* colliders = nullptr;
  const PhysicsAabbCollider* bodyCollider = nullptr;
  Vec3 desiredDisplacementMeters;
  bool includeSensors = false;
  PhysicsKinematicMotorConfig config;
};

struct PhysicsKinematicMotorHit {
  std::size_t colliderIndex = 0U;
  PhysicsBodyId bodyId;
  float fraction = 0.0F;
  float distanceMeters = 0.0F;
  Vec3 normalFromColliderToMotor;
  Vec3 centerMeters;
  bool sensor = false;
  bool initialOverlap = false;
};

struct PhysicsKinematicMotorResult {
  bool ok = false;
  PhysicsKinematicMotorStatus status =
      PhysicsKinematicMotorStatus::MissingColliders;
  std::string_view reasonCode = "physics_kinematic_motor_missing_colliders";
  std::string_view upstreamReasonCode;
  Vec3 startCenterMeters;
  Vec3 finalCenterMeters;
  Vec3 appliedDisplacementMeters;
  Vec3 remainingDisplacementMeters;
  bool blocked = false;
  bool grounded = false;
  bool snappedToGround = false;
  float groundDistanceMeters = 0.0F;
  float groundSnapDistanceMetersApplied = 0.0F;
  Vec3 groundNormal;
  std::size_t iterationCount = 0U;
  std::size_t hitCount = 0U;
  std::size_t sweepTestedColliderCount = 0U;
  std::size_t groundTestedColliderCount = 0U;
  std::size_t invalidColliderIndex = 0U;
  std::vector<PhysicsKinematicMotorHit> hits;
};

std::string_view physicsKinematicMotorStatusName(
    PhysicsKinematicMotorStatus status);
bool isValidPhysicsKinematicMotorConfig(
    const PhysicsKinematicMotorConfig& config);
PhysicsKinematicMotorResult planPhysicsKinematicAabbMove(
    const PhysicsKinematicMotorRequest& request);

}  // namespace iggy3d
