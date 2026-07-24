#include "runtime/physics/PhysicsFrameStats.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

#include "runtime/physics/PhysicsKinematicMotor.hpp"
#include "runtime/player/PlayerPhysicsMovePlanner.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
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

}  // namespace

int main() {
  const bool ok = emptyStatsAreReady() &&
                  motorAndPlayerPlannerCountersAccumulate();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
