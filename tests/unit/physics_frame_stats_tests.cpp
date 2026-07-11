#include "runtime/physics/PhysicsFrameStats.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

#include "runtime/physics/PhysicsBroadphase.hpp"
#include "runtime/physics/PhysicsKinematicMotor.hpp"
#include "runtime/player/PlayerPhysicsMovePlanner.hpp"

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

bool emptyStatsAreReady() {
  const iggy3d::PhysicsFrameStats stats = iggy3d::buildPhysicsFrameStats();

  return expect(iggy3d::physicsFrameStatsStatusName(
                    iggy3d::PhysicsFrameStatsStatus::Ready) ==
                    "physics_frame_stats_ready",
                "ready status name") &&
         expect(iggy3d::physicsFrameStatsStatusName(
                    iggy3d::PhysicsFrameStatsStatus::PacketFailed) ==
                    "physics_frame_stats_packet_failed",
                "packet failed status name") &&
         expect(iggy3d::physicsFrameStatsStatusName(
                    iggy3d::PhysicsFrameStatsStatus::NonfiniteScalar) ==
                    "physics_frame_stats_nonfinite_scalar",
                "nonfinite status name") &&
         expect(stats.ok, "empty stats ok") &&
         expect(stats.status == iggy3d::PhysicsFrameStatsStatus::Ready,
                "empty stats ready") &&
         expect(stats.reasonCode == "physics_frame_stats_ready",
                "empty stats reason") &&
         expect(stats.sourcePacketCount == 0U, "empty source count") &&
         expect(stats.failedPacketCount == 0U, "empty failed count") &&
         expect(stats.contactCount == 0U, "empty contact count");
}

bool broadphaseCountersAccumulate() {
  iggy3d::PhysicsBroadphaseResult packet;
  packet.ok = true;
  packet.reasonCode = "physics_broadphase_pairs_collected";
  packet.colliderCount = 7U;
  packet.occupiedCellCount = 3U;
  packet.cellEntryCount = 11U;
  packet.maxBucketSize = 4U;
  packet.candidatePairCount = 9U;
  packet.testedPairCount = 5U;
  packet.duplicatePairRejectedCount = 2U;
  packet.overlappingPairCount = 3U;

  iggy3d::PhysicsFrameStats stats = iggy3d::buildPhysicsFrameStats();
  iggy3d::accumulatePhysicsBroadphaseStats(stats, packet);

  return expect(stats.sourcePacketCount == 1U, "broadphase source count") &&
         expect(stats.failedPacketCount == 0U, "broadphase failed count") &&
         expect(stats.broadphaseColliderCount == 7U, "broadphase colliders") &&
         expect(stats.broadphaseOccupiedCellCount == 3U,
                "broadphase cells") &&
         expect(stats.broadphaseCellEntryCount == 11U,
                "broadphase entries") &&
         expect(stats.broadphaseMaxBucketSize == 4U,
                "broadphase max bucket") &&
         expect(stats.broadphaseCandidatePairCount == 9U,
                "broadphase candidates") &&
         expect(stats.broadphaseTestedPairCount == 5U,
                "broadphase tested") &&
         expect(stats.broadphaseDuplicatePairRejectedCount == 2U,
                "broadphase duplicates") &&
         expect(stats.broadphaseOverlappingPairCount == 3U,
                "broadphase overlaps");
}

bool motorAndPlayerPlannerCountersAccumulate() {
  iggy3d::PhysicsKinematicMotorResult motor;
  motor.ok = true;
  motor.reasonCode = "physics_kinematic_motor_planned";
  motor.iterationCount = 3U;
  motor.hitCount = 2U;
  motor.sweepTestedColliderCount = 9U;
  motor.groundTestedColliderCount = 4U;

  iggy3d::PlayerPhysicsMovePlannerResult player;
  player.ok = true;
  player.reasonCode = "player_physics_move_planner_planned";
  player.bakedSurfaceCount = 5U;
  player.bakedColliderCount = 4U;
  player.skippedSurfaceCount = 1U;
  player.hitCount = 2U;
  player.iterationCount = 3U;

  iggy3d::PhysicsFrameStats stats = iggy3d::buildPhysicsFrameStats();
  iggy3d::accumulatePhysicsKinematicMotorStats(stats, motor);
  iggy3d::accumulatePlayerPhysicsMovePlannerStats(stats, player);

  return expect(stats.sourcePacketCount == 2U, "motor player source count") &&
         expect(stats.kinematicIterationCount == 3U,
                "motor iterations") &&
         expect(stats.kinematicHitCount == 2U, "motor hits") &&
         expect(stats.kinematicSweepTestedColliderCount == 9U,
                "motor sweep tested") &&
         expect(stats.kinematicGroundTestedColliderCount == 4U,
                "motor ground tested") &&
         expect(stats.playerBakedSurfaceCount == 5U,
                "player baked surfaces") &&
         expect(stats.playerBakedColliderCount == 4U,
                "player baked colliders") &&
         expect(stats.playerSkippedSurfaceCount == 1U,
                "player skipped surfaces") &&
         expect(stats.playerHitCount == 2U, "player hits") &&
         expect(stats.playerIterationCount == 3U, "player iterations");
}

bool broadphaseFailurePropagatesDeterministically() {
  iggy3d::PhysicsBroadphaseResult failedBroadphase;
  failedBroadphase.ok = false;
  failedBroadphase.reasonCode = "physics_broadphase_invalid_grid_config";
  failedBroadphase.colliderCount = 7U;
  iggy3d::PhysicsFrameStats failedStats = iggy3d::buildPhysicsFrameStats();
  iggy3d::accumulatePhysicsBroadphaseStats(failedStats, failedBroadphase);

  return expect(!failedStats.ok, "failed packet marks stats failed") &&
         expect(failedStats.status ==
                    iggy3d::PhysicsFrameStatsStatus::PacketFailed,
                "failed packet status") &&
         expect(failedStats.reasonCode == "physics_frame_stats_packet_failed",
                "failed packet reason") &&
         expect(failedStats.upstreamReasonCode ==
                    "physics_broadphase_invalid_grid_config",
                "failed packet upstream reason") &&
         expect(failedStats.sourcePacketCount == 1U,
                "failed source count") &&
         expect(failedStats.failedPacketCount == 1U,
                "failed packet count") &&
         expect(failedStats.broadphaseColliderCount == 7U,
                "failed safe counter copied");
}

}  // namespace

int main() {
  const bool ok = emptyStatsAreReady() &&
                  broadphaseCountersAccumulate() &&
                  motorAndPlayerPlannerCountersAccumulate() &&
                  broadphaseFailurePropagatesDeterministically();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
