#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "runtime/physics/PhysicsFrameStats.hpp"

namespace iggy3d {

enum class PhysicsDebugSnapshotStatus : std::uint8_t {
  Ready,
  Disabled,
  MissingStats,
  StatsFailed,
  InvalidConfig,
};

struct PhysicsDebugSnapshotConfig {
  bool enabled = true;
  std::size_t broadphaseMaxBucketWarnThreshold = 16U;
  std::size_t broadphaseCandidatePairWarnThreshold = 1024U;
  float maxPenetrationWarnMeters = 0.10F;
  float normalImpulseWarnMagnitude = 100.0F;
  float frictionImpulseWarnMagnitude = 100.0F;
};

struct PhysicsDebugSnapshotRequest {
  const PhysicsFrameStats* stats = nullptr;
  PhysicsDebugSnapshotConfig config;
};

struct PhysicsDebugSnapshot {
  bool ok = true;
  PhysicsDebugSnapshotStatus status = PhysicsDebugSnapshotStatus::Ready;
  std::string_view reasonCode = "physics_debug_snapshot_ready";
  bool statsOk = true;
  std::string_view upstreamReasonCode;
  std::size_t sourcePacketCount = 0U;
  std::size_t failedPacketCount = 0U;

  std::size_t broadphaseColliderCount = 0U;
  std::size_t broadphaseOccupiedCellCount = 0U;
  std::size_t broadphaseCellEntryCount = 0U;
  std::size_t broadphaseMaxBucketSize = 0U;
  std::size_t broadphaseCandidatePairCount = 0U;
  std::size_t broadphaseTestedPairCount = 0U;
  std::size_t broadphaseDuplicatePairRejectedCount = 0U;
  std::size_t broadphaseOverlappingPairCount = 0U;

  std::size_t bindingCount = 0U;
  std::size_t colliderCount = 0U;
  std::size_t broadphasePairCount = 0U;
  std::size_t contactCount = 0U;
  std::size_t sensorContactCount = 0U;
  std::size_t solvePlanCount = 0U;
  std::size_t accumulatedPlanCount = 0U;
  std::size_t skippedNoOpPlanCount = 0U;
  std::size_t positionCorrectionAppliedCount = 0U;
  std::size_t velocityImpulseAppliedCount = 0U;
  std::size_t frictionImpulseAppliedCount = 0U;
  float maxPenetrationMeters = 0.0F;
  float totalNormalImpulseMagnitude = 0.0F;
  float maxNormalImpulseMagnitude = 0.0F;
  float totalFrictionImpulseMagnitude = 0.0F;
  float maxFrictionImpulseMagnitude = 0.0F;

  std::size_t bodyCount = 0U;
  std::size_t appliedPositionCount = 0U;
  std::size_t appliedVelocityCount = 0U;

  std::size_t kinematicIterationCount = 0U;
  std::size_t kinematicHitCount = 0U;
  std::size_t kinematicSweepTestedColliderCount = 0U;
  std::size_t kinematicGroundTestedColliderCount = 0U;
  std::size_t playerBakedSurfaceCount = 0U;
  std::size_t playerBakedColliderCount = 0U;
  std::size_t playerSkippedSurfaceCount = 0U;
  std::size_t playerHitCount = 0U;
  std::size_t playerIterationCount = 0U;

  bool hasFailedPackets = false;
  bool hasBroadphasePressure = false;
  bool hasContacts = false;
  bool hasSensorContacts = false;
  bool hasSolverActivity = false;
  bool hasAppliedDeltas = false;
  bool hasKinematicHits = false;
  bool hasPlayerHits = false;
  bool hasPenetrationWarning = false;
  bool hasImpulseWarning = false;
  bool hasWarnings = false;
};

std::string_view physicsDebugSnapshotStatusName(
    PhysicsDebugSnapshotStatus status);

bool isValidPhysicsDebugSnapshotConfig(
    const PhysicsDebugSnapshotConfig& config);
PhysicsDebugSnapshot buildPhysicsDebugSnapshot(
    const PhysicsDebugSnapshotRequest& request);

}  // namespace iggy3d
