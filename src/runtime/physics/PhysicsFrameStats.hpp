#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace iggy3d {

struct PhysicsAabbCollisionBatchResult;
struct PhysicsAabbStepResult;
struct PhysicsBroadphaseResult;
struct PhysicsKinematicMotorResult;
struct PlayerPhysicsMovePlannerResult;

enum class PhysicsFrameStatsStatus : std::uint8_t {
  Ready,
  PacketFailed,
  NonfiniteScalar,
};

struct PhysicsFrameStats {
  bool ok = true;
  PhysicsFrameStatsStatus status = PhysicsFrameStatsStatus::Ready;
  std::string_view reasonCode = "physics_frame_stats_ready";
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
};

std::string_view physicsFrameStatsStatusName(PhysicsFrameStatsStatus status);

PhysicsFrameStats buildPhysicsFrameStats();
void accumulatePhysicsBroadphaseStats(PhysicsFrameStats& stats,
                                      const PhysicsBroadphaseResult& result);
void accumulatePhysicsCollisionBatchStats(
    PhysicsFrameStats& stats,
    const PhysicsAabbCollisionBatchResult& result);
void accumulatePhysicsAabbStepStats(PhysicsFrameStats& stats,
                                    const PhysicsAabbStepResult& result);
void accumulatePhysicsKinematicMotorStats(
    PhysicsFrameStats& stats,
    const PhysicsKinematicMotorResult& result);
void accumulatePlayerPhysicsMovePlannerStats(
    PhysicsFrameStats& stats,
    const PlayerPhysicsMovePlannerResult& result);

}  // namespace iggy3d
