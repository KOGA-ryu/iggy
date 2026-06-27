#include "runtime/physics/PhysicsDebugSnapshot.hpp"

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

bool near(float lhs, float rhs, float epsilon = 0.0001F) {
  const float delta = lhs - rhs;
  return delta >= -epsilon && delta <= epsilon;
}

iggy3d::PhysicsFrameStats representativeStats() {
  iggy3d::PhysicsFrameStats stats;
  stats.sourcePacketCount = 3U;
  stats.failedPacketCount = 0U;
  stats.broadphaseColliderCount = 9U;
  stats.broadphaseOccupiedCellCount = 4U;
  stats.broadphaseCellEntryCount = 17U;
  stats.broadphaseMaxBucketSize = 5U;
  stats.broadphaseCandidatePairCount = 19U;
  stats.broadphaseTestedPairCount = 7U;
  stats.broadphaseDuplicatePairRejectedCount = 2U;
  stats.broadphaseOverlappingPairCount = 6U;
  stats.bindingCount = 8U;
  stats.colliderCount = 7U;
  stats.broadphasePairCount = 6U;
  stats.contactCount = 5U;
  stats.sensorContactCount = 1U;
  stats.solvePlanCount = 4U;
  stats.accumulatedPlanCount = 3U;
  stats.skippedNoOpPlanCount = 1U;
  stats.positionCorrectionAppliedCount = 2U;
  stats.velocityImpulseAppliedCount = 3U;
  stats.frictionImpulseAppliedCount = 1U;
  stats.maxPenetrationMeters = 0.05F;
  stats.totalNormalImpulseMagnitude = 12.0F;
  stats.maxNormalImpulseMagnitude = 7.0F;
  stats.totalFrictionImpulseMagnitude = 4.0F;
  stats.maxFrictionImpulseMagnitude = 2.0F;
  stats.bodyCount = 11U;
  stats.appliedPositionCount = 3U;
  stats.appliedVelocityCount = 2U;
  stats.kinematicIterationCount = 4U;
  stats.kinematicHitCount = 1U;
  stats.kinematicSweepTestedColliderCount = 13U;
  stats.kinematicGroundTestedColliderCount = 5U;
  stats.playerBakedSurfaceCount = 10U;
  stats.playerBakedColliderCount = 8U;
  stats.playerSkippedSurfaceCount = 2U;
  stats.playerHitCount = 1U;
  stats.playerIterationCount = 4U;
  return stats;
}

iggy3d::PhysicsDebugSnapshot buildSnapshot(
    const iggy3d::PhysicsFrameStats& stats,
    iggy3d::PhysicsDebugSnapshotConfig config = {}) {
  return iggy3d::buildPhysicsDebugSnapshot({&stats, config});
}

bool statusNamesAreStable() {
  return expect(iggy3d::physicsDebugSnapshotStatusName(
                    iggy3d::PhysicsDebugSnapshotStatus::Ready) ==
                    "physics_debug_snapshot_ready",
                "ready status name") &&
         expect(iggy3d::physicsDebugSnapshotStatusName(
                    iggy3d::PhysicsDebugSnapshotStatus::Disabled) ==
                    "physics_debug_snapshot_disabled",
                "disabled status name") &&
         expect(iggy3d::physicsDebugSnapshotStatusName(
                    iggy3d::PhysicsDebugSnapshotStatus::MissingStats) ==
                    "physics_debug_snapshot_missing_stats",
                "missing stats status name") &&
         expect(iggy3d::physicsDebugSnapshotStatusName(
                    iggy3d::PhysicsDebugSnapshotStatus::StatsFailed) ==
                    "physics_debug_snapshot_stats_failed",
                "stats failed status name") &&
         expect(iggy3d::physicsDebugSnapshotStatusName(
                    iggy3d::PhysicsDebugSnapshotStatus::InvalidConfig) ==
                    "physics_debug_snapshot_invalid_config",
                "invalid config status name");
}

bool disabledReturnsQuietSnapshot() {
  iggy3d::PhysicsFrameStats stats = representativeStats();
  iggy3d::PhysicsDebugSnapshotConfig config;
  config.enabled = false;

  const iggy3d::PhysicsDebugSnapshot snapshot =
      iggy3d::buildPhysicsDebugSnapshot({&stats, config});

  return expect(snapshot.ok, "disabled ok") &&
         expect(snapshot.status ==
                    iggy3d::PhysicsDebugSnapshotStatus::Disabled,
                "disabled status") &&
         expect(snapshot.reasonCode == "physics_debug_snapshot_disabled",
                "disabled reason") &&
         expect(snapshot.sourcePacketCount == 0U,
                "disabled source count zero") &&
         expect(snapshot.contactCount == 0U,
                "disabled contact count zero") &&
         expect(!snapshot.hasWarnings, "disabled no warnings") &&
         expect(!snapshot.hasContacts, "disabled no contacts");
}

bool missingStatsRejects() {
  const iggy3d::PhysicsDebugSnapshot snapshot =
      iggy3d::buildPhysicsDebugSnapshot({});

  return expect(!snapshot.ok, "missing stats not ok") &&
         expect(snapshot.status ==
                    iggy3d::PhysicsDebugSnapshotStatus::MissingStats,
                "missing stats status") &&
         expect(snapshot.reasonCode ==
                    "physics_debug_snapshot_missing_stats",
                "missing stats reason") &&
         expect(!snapshot.statsOk, "missing stats statsOk false");
}

bool invalidConfigRejects() {
  iggy3d::PhysicsDebugSnapshotConfig negative;
  negative.maxPenetrationWarnMeters = -0.01F;
  iggy3d::PhysicsDebugSnapshotConfig nonfinite;
  nonfinite.normalImpulseWarnMagnitude =
      std::numeric_limits<float>::infinity();
  iggy3d::PhysicsFrameStats stats = representativeStats();

  const iggy3d::PhysicsDebugSnapshot negativeSnapshot =
      buildSnapshot(stats, negative);
  const iggy3d::PhysicsDebugSnapshot nonfiniteSnapshot =
      buildSnapshot(stats, nonfinite);

  return expect(!iggy3d::isValidPhysicsDebugSnapshotConfig(negative),
                "negative config invalid") &&
         expect(!iggy3d::isValidPhysicsDebugSnapshotConfig(nonfinite),
                "nonfinite config invalid") &&
         expect(!negativeSnapshot.ok, "negative snapshot not ok") &&
         expect(negativeSnapshot.status ==
                    iggy3d::PhysicsDebugSnapshotStatus::InvalidConfig,
                "negative invalid status") &&
         expect(nonfiniteSnapshot.reasonCode ==
                    "physics_debug_snapshot_invalid_config",
                "nonfinite invalid reason") &&
         expect(nonfiniteSnapshot.sourcePacketCount == 0U,
                "invalid config does not copy counters");
}

bool readyStatsCopyRepresentativeCounters() {
  const iggy3d::PhysicsFrameStats stats = representativeStats();
  const iggy3d::PhysicsDebugSnapshot snapshot = buildSnapshot(stats);

  return expect(snapshot.ok, "ready ok") &&
         expect(snapshot.status == iggy3d::PhysicsDebugSnapshotStatus::Ready,
                "ready status") &&
         expect(snapshot.statsOk, "ready stats ok") &&
         expect(snapshot.sourcePacketCount == 3U, "copy source count") &&
         expect(snapshot.broadphaseColliderCount == 9U,
                "copy broadphase collider count") &&
         expect(snapshot.broadphaseOccupiedCellCount == 4U,
                "copy occupied cells") &&
         expect(snapshot.broadphaseCellEntryCount == 17U,
                "copy cell entries") &&
         expect(snapshot.broadphaseMaxBucketSize == 5U,
                "copy max bucket") &&
         expect(snapshot.broadphaseCandidatePairCount == 19U,
                "copy candidate pairs") &&
         expect(snapshot.broadphaseTestedPairCount == 7U,
                "copy tested pairs") &&
         expect(snapshot.broadphaseDuplicatePairRejectedCount == 2U,
                "copy duplicate rejected") &&
         expect(snapshot.broadphaseOverlappingPairCount == 6U,
                "copy overlaps") &&
         expect(snapshot.bindingCount == 8U, "copy bindings") &&
         expect(snapshot.colliderCount == 7U, "copy colliders") &&
         expect(snapshot.broadphasePairCount == 6U, "copy pair count") &&
         expect(snapshot.contactCount == 5U, "copy contact count") &&
         expect(snapshot.sensorContactCount == 1U,
                "copy sensor count") &&
         expect(snapshot.solvePlanCount == 4U, "copy solve plans") &&
         expect(snapshot.accumulatedPlanCount == 3U,
                "copy accumulated plans") &&
         expect(snapshot.skippedNoOpPlanCount == 1U,
                "copy skipped plans") &&
         expect(snapshot.positionCorrectionAppliedCount == 2U,
                "copy position corrections") &&
         expect(snapshot.velocityImpulseAppliedCount == 3U,
                "copy velocity impulses") &&
         expect(snapshot.frictionImpulseAppliedCount == 1U,
                "copy friction impulses") &&
         expect(near(snapshot.maxPenetrationMeters, 0.05F),
                "copy max penetration") &&
         expect(near(snapshot.totalNormalImpulseMagnitude, 12.0F),
                "copy total normal") &&
         expect(near(snapshot.maxNormalImpulseMagnitude, 7.0F),
                "copy max normal") &&
         expect(near(snapshot.totalFrictionImpulseMagnitude, 4.0F),
                "copy total friction") &&
         expect(near(snapshot.maxFrictionImpulseMagnitude, 2.0F),
                "copy max friction") &&
         expect(snapshot.bodyCount == 11U, "copy body count") &&
         expect(snapshot.appliedPositionCount == 3U,
                "copy applied position") &&
         expect(snapshot.appliedVelocityCount == 2U,
                "copy applied velocity") &&
         expect(snapshot.kinematicIterationCount == 4U,
                "copy kinematic iterations") &&
         expect(snapshot.kinematicHitCount == 1U,
                "copy kinematic hits") &&
         expect(snapshot.kinematicSweepTestedColliderCount == 13U,
                "copy sweep tested") &&
         expect(snapshot.kinematicGroundTestedColliderCount == 5U,
                "copy ground tested") &&
         expect(snapshot.playerBakedSurfaceCount == 10U,
                "copy player surfaces") &&
         expect(snapshot.playerBakedColliderCount == 8U,
                "copy player colliders") &&
         expect(snapshot.playerSkippedSurfaceCount == 2U,
                "copy player skipped") &&
         expect(snapshot.playerHitCount == 1U, "copy player hits") &&
         expect(snapshot.playerIterationCount == 4U,
                "copy player iterations") &&
         expect(snapshot.hasContacts, "derive has contacts") &&
         expect(snapshot.hasSensorContacts, "derive has sensors") &&
         expect(snapshot.hasSolverActivity, "derive solver activity") &&
         expect(snapshot.hasAppliedDeltas, "derive applied deltas") &&
         expect(snapshot.hasKinematicHits, "derive kinematic hits") &&
         expect(snapshot.hasPlayerHits, "derive player hits") &&
         expect(!snapshot.hasWarnings, "representative below warnings");
}

bool failedStatsStillCopyForInspection() {
  iggy3d::PhysicsFrameStats stats = representativeStats();
  stats.ok = false;
  stats.status = iggy3d::PhysicsFrameStatsStatus::PacketFailed;
  stats.reasonCode = "physics_frame_stats_packet_failed";
  stats.upstreamReasonCode = "physics_broadphase_invalid_grid_config";
  stats.failedPacketCount = 1U;

  const iggy3d::PhysicsDebugSnapshot snapshot = buildSnapshot(stats);

  return expect(!snapshot.ok, "failed stats snapshot not ok") &&
         expect(!snapshot.statsOk, "failed statsOk false") &&
         expect(snapshot.status ==
                    iggy3d::PhysicsDebugSnapshotStatus::StatsFailed,
                "failed stats status") &&
         expect(snapshot.reasonCode == "physics_debug_snapshot_stats_failed",
                "failed stats reason") &&
         expect(snapshot.upstreamReasonCode ==
                    "physics_broadphase_invalid_grid_config",
                "failed stats upstream") &&
         expect(snapshot.failedPacketCount == 1U,
                "failed packet count copied") &&
         expect(snapshot.contactCount == 5U,
                "failed stats still copy contacts") &&
         expect(snapshot.hasFailedPackets, "failed packets flag") &&
         expect(snapshot.hasWarnings, "failed stats warnings");
}

bool warningFlagsUseThresholds() {
  iggy3d::PhysicsFrameStats stats = representativeStats();
  iggy3d::PhysicsDebugSnapshotConfig config;
  config.broadphaseMaxBucketWarnThreshold = 5U;
  config.broadphaseCandidatePairWarnThreshold = 100U;
  config.maxPenetrationWarnMeters = 0.05F;
  config.normalImpulseWarnMagnitude = 7.0F;
  config.frictionImpulseWarnMagnitude = 100.0F;

  const iggy3d::PhysicsDebugSnapshot snapshot = buildSnapshot(stats, config);

  return expect(snapshot.hasBroadphasePressure,
                "bucket threshold pressure") &&
         expect(snapshot.hasPenetrationWarning,
                "penetration warning") &&
         expect(snapshot.hasImpulseWarning, "normal impulse warning") &&
         expect(snapshot.hasWarnings, "aggregate warnings");
}

bool frictionImpulseCanTriggerWarning() {
  iggy3d::PhysicsFrameStats stats = representativeStats();
  iggy3d::PhysicsDebugSnapshotConfig config;
  config.normalImpulseWarnMagnitude = 100.0F;
  config.frictionImpulseWarnMagnitude = 2.0F;

  const iggy3d::PhysicsDebugSnapshot snapshot = buildSnapshot(stats, config);

  return expect(snapshot.hasImpulseWarning,
                "friction impulse warning") &&
         expect(snapshot.hasWarnings, "friction aggregate warning");
}

bool quietStatsHaveNoWarnings() {
  const iggy3d::PhysicsFrameStats stats;
  const iggy3d::PhysicsDebugSnapshot snapshot = buildSnapshot(stats);

  return expect(snapshot.ok, "quiet ok") &&
         expect(!snapshot.hasFailedPackets, "quiet no failed packets") &&
         expect(!snapshot.hasBroadphasePressure,
                "quiet no broadphase pressure") &&
         expect(!snapshot.hasContacts, "quiet no contacts") &&
         expect(!snapshot.hasSensorContacts, "quiet no sensors") &&
         expect(!snapshot.hasSolverActivity, "quiet no solver") &&
         expect(!snapshot.hasAppliedDeltas, "quiet no deltas") &&
         expect(!snapshot.hasKinematicHits, "quiet no kinematic hits") &&
         expect(!snapshot.hasPlayerHits, "quiet no player hits") &&
         expect(!snapshot.hasPenetrationWarning,
                "quiet no penetration warning") &&
         expect(!snapshot.hasImpulseWarning, "quiet no impulse warning") &&
         expect(!snapshot.hasWarnings, "quiet no warnings");
}

bool sourceStatsAreNotMutated() {
  iggy3d::PhysicsFrameStats stats = representativeStats();
  const iggy3d::PhysicsFrameStats before = stats;

  (void)buildSnapshot(stats);

  return expect(stats.ok == before.ok, "source ok unchanged") &&
         expect(stats.sourcePacketCount == before.sourcePacketCount,
                "source count unchanged") &&
         expect(stats.broadphaseMaxBucketSize ==
                    before.broadphaseMaxBucketSize,
                "source bucket unchanged") &&
         expect(stats.contactCount == before.contactCount,
                "source contacts unchanged") &&
         expect(near(stats.maxPenetrationMeters,
                     before.maxPenetrationMeters),
                "source penetration unchanged") &&
         expect(near(stats.maxNormalImpulseMagnitude,
                     before.maxNormalImpulseMagnitude),
                "source normal unchanged") &&
         expect(stats.playerIterationCount == before.playerIterationCount,
                "source player iterations unchanged");
}

}  // namespace

int main() {
  const bool ok = statusNamesAreStable() &&
                  disabledReturnsQuietSnapshot() &&
                  missingStatsRejects() &&
                  invalidConfigRejects() &&
                  readyStatsCopyRepresentativeCounters() &&
                  failedStatsStillCopyForInspection() &&
                  warningFlagsUseThresholds() &&
                  frictionImpulseCanTriggerWarning() &&
                  quietStatsHaveNoWarnings() &&
                  sourceStatsAreNotMutated();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
