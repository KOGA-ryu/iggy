#include "runtime/physics/PhysicsDebugSnapshot.hpp"

#include <array>
#include <cmath>

namespace iggy3d {
namespace {

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1106
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

PhysicsDebugSnapshot makeStatusSnapshot(PhysicsDebugSnapshotStatus status,
                                        bool ok,
                                        bool statsOk = true) {
  PhysicsDebugSnapshot snapshot;
  snapshot.ok = ok;
  snapshot.status = status;
  snapshot.reasonCode = physicsDebugSnapshotStatusName(status);
  snapshot.statsOk = statsOk;
  return snapshot;
}

bool validWarnScalar(float value) {
  return std::isfinite(value) && value >= 0.0F;
}

bool reachedThreshold(std::size_t value, std::size_t threshold) {
  // branch-gate: BG-1106
  if (threshold == 0U) {
    return false;
  }
  return value >= threshold;
}

bool reachedThreshold(float value, float threshold) {
  // branch-gate: BG-1106
  if (threshold <= 0.0F) {
    return false;
  }
  return value >= threshold;
}

void copyStats(PhysicsDebugSnapshot& snapshot,
               const PhysicsFrameStats& stats) {
  snapshot.statsOk = stats.ok;
  snapshot.upstreamReasonCode = stats.upstreamReasonCode;
  snapshot.sourcePacketCount = stats.sourcePacketCount;
  snapshot.failedPacketCount = stats.failedPacketCount;

  snapshot.broadphaseColliderCount = stats.broadphaseColliderCount;
  snapshot.broadphaseOccupiedCellCount = stats.broadphaseOccupiedCellCount;
  snapshot.broadphaseCellEntryCount = stats.broadphaseCellEntryCount;
  snapshot.broadphaseMaxBucketSize = stats.broadphaseMaxBucketSize;
  snapshot.broadphaseCandidatePairCount =
      stats.broadphaseCandidatePairCount;
  snapshot.broadphaseTestedPairCount = stats.broadphaseTestedPairCount;
  snapshot.broadphaseDuplicatePairRejectedCount =
      stats.broadphaseDuplicatePairRejectedCount;
  snapshot.broadphaseOverlappingPairCount =
      stats.broadphaseOverlappingPairCount;

  snapshot.bindingCount = stats.bindingCount;
  snapshot.colliderCount = stats.colliderCount;
  snapshot.broadphasePairCount = stats.broadphasePairCount;
  snapshot.contactCount = stats.contactCount;
  snapshot.sensorContactCount = stats.sensorContactCount;
  snapshot.solvePlanCount = stats.solvePlanCount;
  snapshot.accumulatedPlanCount = stats.accumulatedPlanCount;
  snapshot.skippedNoOpPlanCount = stats.skippedNoOpPlanCount;
  snapshot.positionCorrectionAppliedCount =
      stats.positionCorrectionAppliedCount;
  snapshot.velocityImpulseAppliedCount = stats.velocityImpulseAppliedCount;
  snapshot.frictionImpulseAppliedCount = stats.frictionImpulseAppliedCount;
  snapshot.maxPenetrationMeters = stats.maxPenetrationMeters;
  snapshot.totalNormalImpulseMagnitude =
      stats.totalNormalImpulseMagnitude;
  snapshot.maxNormalImpulseMagnitude = stats.maxNormalImpulseMagnitude;
  snapshot.totalFrictionImpulseMagnitude =
      stats.totalFrictionImpulseMagnitude;
  snapshot.maxFrictionImpulseMagnitude =
      stats.maxFrictionImpulseMagnitude;

  snapshot.bodyCount = stats.bodyCount;
  snapshot.appliedPositionCount = stats.appliedPositionCount;
  snapshot.appliedVelocityCount = stats.appliedVelocityCount;

  snapshot.kinematicIterationCount = stats.kinematicIterationCount;
  snapshot.kinematicHitCount = stats.kinematicHitCount;
  snapshot.kinematicSweepTestedColliderCount =
      stats.kinematicSweepTestedColliderCount;
  snapshot.kinematicGroundTestedColliderCount =
      stats.kinematicGroundTestedColliderCount;
  snapshot.playerBakedSurfaceCount = stats.playerBakedSurfaceCount;
  snapshot.playerBakedColliderCount = stats.playerBakedColliderCount;
  snapshot.playerSkippedSurfaceCount = stats.playerSkippedSurfaceCount;
  snapshot.playerHitCount = stats.playerHitCount;
  snapshot.playerIterationCount = stats.playerIterationCount;
}

void deriveFlags(PhysicsDebugSnapshot& snapshot,
                 const PhysicsDebugSnapshotConfig& config) {
  snapshot.hasFailedPackets = snapshot.failedPacketCount > 0U;
  snapshot.hasBroadphasePressure =
      reachedThreshold(snapshot.broadphaseMaxBucketSize,
                       config.broadphaseMaxBucketWarnThreshold) ||
      reachedThreshold(snapshot.broadphaseCandidatePairCount,
                       config.broadphaseCandidatePairWarnThreshold);
  snapshot.hasContacts = snapshot.contactCount > 0U;
  snapshot.hasSensorContacts = snapshot.sensorContactCount > 0U;
  snapshot.hasSolverActivity =
      snapshot.solvePlanCount > 0U ||
      snapshot.positionCorrectionAppliedCount > 0U ||
      snapshot.velocityImpulseAppliedCount > 0U ||
      snapshot.frictionImpulseAppliedCount > 0U;
  snapshot.hasAppliedDeltas = snapshot.appliedPositionCount > 0U ||
                              snapshot.appliedVelocityCount > 0U;
  snapshot.hasKinematicHits = snapshot.kinematicHitCount > 0U;
  snapshot.hasPlayerHits = snapshot.playerHitCount > 0U;
  snapshot.hasPenetrationWarning =
      reachedThreshold(snapshot.maxPenetrationMeters,
                       config.maxPenetrationWarnMeters);
  snapshot.hasImpulseWarning =
      reachedThreshold(snapshot.maxNormalImpulseMagnitude,
                       config.normalImpulseWarnMagnitude) ||
      reachedThreshold(snapshot.maxFrictionImpulseMagnitude,
                       config.frictionImpulseWarnMagnitude);
  snapshot.hasWarnings = snapshot.hasFailedPackets ||
                         snapshot.hasBroadphasePressure ||
                         snapshot.hasPenetrationWarning ||
                         snapshot.hasImpulseWarning ||
                         snapshot.status ==
                             PhysicsDebugSnapshotStatus::StatsFailed;
}

}  // namespace

std::string_view physicsDebugSnapshotStatusName(
    PhysicsDebugSnapshotStatus status) {
  static constexpr std::array<std::string_view, 5> kNames{
      "physics_debug_snapshot_ready",
      "physics_debug_snapshot_disabled",
      "physics_debug_snapshot_missing_stats",
      "physics_debug_snapshot_stats_failed",
      "physics_debug_snapshot_invalid_config",
  };
  return enumName(status, kNames, "physics_debug_snapshot_invalid_config");
}

bool isValidPhysicsDebugSnapshotConfig(
    const PhysicsDebugSnapshotConfig& config) {
  return validWarnScalar(config.maxPenetrationWarnMeters) &&
         validWarnScalar(config.normalImpulseWarnMagnitude) &&
         validWarnScalar(config.frictionImpulseWarnMagnitude);
}

PhysicsDebugSnapshot buildPhysicsDebugSnapshot(
    const PhysicsDebugSnapshotRequest& request) {
  // branch-gate: BG-1106
  if (!request.config.enabled) {
    return makeStatusSnapshot(PhysicsDebugSnapshotStatus::Disabled, true);
  }
  // branch-gate: BG-1106
  if (!isValidPhysicsDebugSnapshotConfig(request.config)) {
    return makeStatusSnapshot(PhysicsDebugSnapshotStatus::InvalidConfig,
                              false, false);
  }
  // branch-gate: BG-1106
  if (request.stats == nullptr) {
    return makeStatusSnapshot(PhysicsDebugSnapshotStatus::MissingStats,
                              false, false);
  }

  PhysicsDebugSnapshotStatus status = PhysicsDebugSnapshotStatus::Ready;
  // branch-gate: BG-1106
  if (!request.stats->ok) {
    status = PhysicsDebugSnapshotStatus::StatsFailed;
  }
  PhysicsDebugSnapshot snapshot =
      makeStatusSnapshot(status, request.stats->ok, request.stats->ok);
  copyStats(snapshot, *request.stats);
  deriveFlags(snapshot, request.config);
  return snapshot;
}

}  // namespace iggy3d
