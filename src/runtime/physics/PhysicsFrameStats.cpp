#include "runtime/physics/PhysicsFrameStats.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "runtime/physics/PhysicsAabbCollisionBatch.hpp"
#include "runtime/physics/PhysicsAabbStep.hpp"
#include "runtime/physics/PhysicsBroadphase.hpp"
#include "runtime/physics/PhysicsKinematicMotor.hpp"
#include "runtime/player/PlayerPhysicsMovePlanner.hpp"

namespace iggy3d {
namespace {

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1105
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

void setFailureStatus(PhysicsFrameStats& stats,
                      PhysicsFrameStatsStatus status,
                      std::string_view upstreamReasonCode) {
  stats.ok = false;
  // branch-gate: BG-1105
  if (status == PhysicsFrameStatsStatus::NonfiniteScalar ||
      stats.status == PhysicsFrameStatsStatus::Ready) {
    stats.status = status;
    stats.reasonCode = physicsFrameStatsStatusName(status);
  }
  // branch-gate: BG-1105
  if (stats.upstreamReasonCode.empty()) {
    stats.upstreamReasonCode = upstreamReasonCode;
  }
}

void recordPacketOutcome(PhysicsFrameStats& stats,
                         bool packetOk,
                         std::string_view upstreamReasonCode,
                         bool nonfiniteScalar) {
  ++stats.sourcePacketCount;
  // branch-gate: BG-1105
  if (nonfiniteScalar) {
    ++stats.failedPacketCount;
    setFailureStatus(stats, PhysicsFrameStatsStatus::NonfiniteScalar,
                     upstreamReasonCode);
    return;
  }
  // branch-gate: BG-1105
  if (!packetOk) {
    ++stats.failedPacketCount;
    setFailureStatus(stats, PhysicsFrameStatsStatus::PacketFailed,
                     upstreamReasonCode);
  }
}

bool addFinite(float value, float& total, float& maximum) {
  // branch-gate: BG-1105
  if (!std::isfinite(value)) {
    return false;
  }
  total += value;
  maximum = std::max(maximum, value);
  return true;
}

bool maxFinite(float value, float& maximum) {
  // branch-gate: BG-1105
  if (!std::isfinite(value)) {
    return false;
  }
  maximum = std::max(maximum, value);
  return true;
}

bool accumulateCollisionBatchFields(
    PhysicsFrameStats& stats,
    const PhysicsAabbCollisionBatchResult& result) {
  stats.bindingCount += result.bindingCount;
  stats.colliderCount += result.colliderCount;
  stats.broadphasePairCount += result.broadphasePairCount;
  stats.contactCount += result.contactCount;
  stats.sensorContactCount += result.sensorContactCount;
  stats.solvePlanCount += result.solvePlanCount;
  stats.accumulatedPlanCount += result.accumulatedPlanCount;
  stats.skippedNoOpPlanCount += result.skippedNoOpPlanCount;

  bool finite = true;
  for (const PhysicsAabbContact& contact : result.contacts) {
    finite = maxFinite(contact.penetrationMeters,
                       stats.maxPenetrationMeters) &&
             finite;
  }
  for (const PhysicsAabbContactSolvePlan& plan : result.solvePlans) {
    // branch-gate: BG-1105
    if (plan.positionCorrectionApplied) {
      ++stats.positionCorrectionAppliedCount;
    }
    // branch-gate: BG-1105
    if (plan.velocityImpulseApplied) {
      ++stats.velocityImpulseAppliedCount;
    }
    // branch-gate: BG-1105
    if (plan.frictionImpulseApplied) {
      ++stats.frictionImpulseAppliedCount;
    }
    finite = addFinite(plan.normalImpulseMagnitude,
                       stats.totalNormalImpulseMagnitude,
                       stats.maxNormalImpulseMagnitude) &&
             finite;
    finite = addFinite(plan.frictionImpulseMagnitude,
                       stats.totalFrictionImpulseMagnitude,
                       stats.maxFrictionImpulseMagnitude) &&
             finite;
  }
  return finite;
}

}  // namespace

std::string_view physicsFrameStatsStatusName(PhysicsFrameStatsStatus status) {
  static constexpr std::array<std::string_view, 3> kNames{
      "physics_frame_stats_ready",
      "physics_frame_stats_packet_failed",
      "physics_frame_stats_nonfinite_scalar",
  };
  return enumName(status, kNames, "physics_frame_stats_packet_failed");
}

PhysicsFrameStats buildPhysicsFrameStats() {
  return {};
}

void accumulatePhysicsBroadphaseStats(PhysicsFrameStats& stats,
                                      const PhysicsBroadphaseResult& result) {
  stats.broadphaseColliderCount += result.colliderCount;
  stats.broadphaseOccupiedCellCount += result.occupiedCellCount;
  stats.broadphaseCellEntryCount += result.cellEntryCount;
  stats.broadphaseMaxBucketSize =
      std::max(stats.broadphaseMaxBucketSize, result.maxBucketSize);
  stats.broadphaseCandidatePairCount += result.candidatePairCount;
  stats.broadphaseTestedPairCount += result.testedPairCount;
  stats.broadphaseDuplicatePairRejectedCount +=
      result.duplicatePairRejectedCount;
  stats.broadphaseOverlappingPairCount += result.overlappingPairCount;
  recordPacketOutcome(stats, result.ok, result.reasonCode, false);
}

void accumulatePhysicsCollisionBatchStats(
    PhysicsFrameStats& stats,
    const PhysicsAabbCollisionBatchResult& result) {
  const bool finite = accumulateCollisionBatchFields(stats, result);
  recordPacketOutcome(stats, result.ok, result.reasonCode, !finite);
}

void accumulatePhysicsAabbStepStats(PhysicsFrameStats& stats,
                                    const PhysicsAabbStepResult& result) {
  stats.bodyCount += result.bodyCount;
  stats.appliedPositionCount += result.appliedPositionCount;
  stats.appliedVelocityCount += result.appliedVelocityCount;
  const bool finite =
      accumulateCollisionBatchFields(stats, result.collisionBatch);
  recordPacketOutcome(stats, result.ok, result.reasonCode, !finite);
}

void accumulatePhysicsKinematicMotorStats(
    PhysicsFrameStats& stats,
    const PhysicsKinematicMotorResult& result) {
  stats.kinematicIterationCount += result.iterationCount;
  stats.kinematicHitCount += result.hitCount;
  stats.kinematicSweepTestedColliderCount += result.sweepTestedColliderCount;
  stats.kinematicGroundTestedColliderCount += result.groundTestedColliderCount;
  recordPacketOutcome(stats, result.ok, result.reasonCode, false);
}

void accumulatePlayerPhysicsMovePlannerStats(
    PhysicsFrameStats& stats,
    const PlayerPhysicsMovePlannerResult& result) {
  stats.playerBakedSurfaceCount += result.bakedSurfaceCount;
  stats.playerBakedColliderCount += result.bakedColliderCount;
  stats.playerSkippedSurfaceCount += result.skippedSurfaceCount;
  stats.playerHitCount += result.hitCount;
  stats.playerIterationCount += result.iterationCount;
  recordPacketOutcome(stats, result.ok, result.reasonCode, false);
}

}  // namespace iggy3d
