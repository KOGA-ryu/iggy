#include "runtime/physics/PhysicsFrameStats.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

#include "runtime/physics/PhysicsAabbCollisionBatch.hpp"
#include "runtime/physics/PhysicsAabbStep.hpp"
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

iggy3d::PhysicsAabbContact contact(float penetrationMeters) {
  iggy3d::PhysicsAabbContact result;
  result.firstBodyId = {1U};
  result.secondBodyId = {2U};
  result.penetrationMeters = penetrationMeters;
  return result;
}

iggy3d::PhysicsAabbContactSolvePlan solvePlan(
    float normalImpulse,
    float frictionImpulse,
    bool positionCorrection = true,
    bool velocityImpulse = true,
    bool frictionImpulseApplied = true) {
  iggy3d::PhysicsAabbContactSolvePlan plan;
  plan.ok = true;
  plan.reasonCode = "physics_aabb_contact_solve_solved";
  plan.firstBodyId = {1U};
  plan.secondBodyId = {2U};
  plan.solveContact = true;
  plan.positionCorrectionApplied = positionCorrection;
  plan.velocityImpulseApplied = velocityImpulse;
  plan.frictionImpulseApplied = frictionImpulseApplied;
  plan.normalImpulseMagnitude = normalImpulse;
  plan.frictionImpulseMagnitude = frictionImpulse;
  return plan;
}

iggy3d::PhysicsAabbCollisionBatchResult collisionBatchPacket() {
  iggy3d::PhysicsAabbCollisionBatchResult result;
  result.ok = true;
  result.status = iggy3d::PhysicsAabbCollisionBatchStatus::Batched;
  result.reasonCode = "physics_aabb_collision_batch_batched";
  result.bindingCount = 3U;
  result.colliderCount = 4U;
  result.broadphasePairCount = 2U;
  result.contactCount = 2U;
  result.sensorContactCount = 1U;
  result.solvePlanCount = 2U;
  result.accumulatedPlanCount = 1U;
  result.skippedNoOpPlanCount = 1U;
  result.contacts.push_back(contact(0.25F));
  result.contacts.push_back(contact(0.50F));
  result.solvePlans.push_back(solvePlan(2.0F, 0.25F));
  result.solvePlans.push_back(solvePlan(3.0F, 0.50F, false, true, false));
  return result;
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

bool collisionBatchDerivesContactAndImpulseStats() {
  const iggy3d::PhysicsAabbCollisionBatchResult before =
      collisionBatchPacket();
  iggy3d::PhysicsAabbCollisionBatchResult packet = before;
  iggy3d::PhysicsFrameStats stats = iggy3d::buildPhysicsFrameStats();

  iggy3d::accumulatePhysicsCollisionBatchStats(stats, packet);

  return expect(stats.sourcePacketCount == 1U, "batch source count") &&
         expect(stats.bindingCount == 3U, "batch bindings") &&
         expect(stats.colliderCount == 4U, "batch colliders") &&
         expect(stats.broadphasePairCount == 2U, "batch pairs") &&
         expect(stats.contactCount == 2U, "batch contacts") &&
         expect(stats.sensorContactCount == 1U, "batch sensors") &&
         expect(stats.solvePlanCount == 2U, "batch plans") &&
         expect(stats.accumulatedPlanCount == 1U, "batch accumulated") &&
         expect(stats.skippedNoOpPlanCount == 1U, "batch skipped") &&
         expect(stats.positionCorrectionAppliedCount == 1U,
                "position correction count") &&
         expect(stats.velocityImpulseAppliedCount == 2U,
                "velocity impulse count") &&
         expect(stats.frictionImpulseAppliedCount == 1U,
                "friction impulse count") &&
         expect(near(stats.maxPenetrationMeters, 0.50F),
                "max penetration") &&
         expect(near(stats.totalNormalImpulseMagnitude, 5.0F),
                "total normal impulse") &&
         expect(near(stats.maxNormalImpulseMagnitude, 3.0F),
                "max normal impulse") &&
         expect(near(stats.totalFrictionImpulseMagnitude, 0.75F),
                "total friction impulse") &&
         expect(near(stats.maxFrictionImpulseMagnitude, 0.50F),
                "max friction impulse") &&
         expect(packet.contacts.size() == before.contacts.size(),
                "batch contacts not mutated") &&
         expect(near(packet.contacts[1].penetrationMeters,
                     before.contacts[1].penetrationMeters),
                "batch penetration not mutated") &&
         expect(near(packet.solvePlans[0].normalImpulseMagnitude,
                     before.solvePlans[0].normalImpulseMagnitude),
                "batch plan not mutated");
}

bool aabbStepCopiesApplyAndBatchStats() {
  iggy3d::PhysicsAabbStepResult step;
  step.ok = true;
  step.status = iggy3d::PhysicsAabbStepStatus::Stepped;
  step.reasonCode = "physics_aabb_step_stepped";
  step.bodyCount = 8U;
  step.appliedPositionCount = 3U;
  step.appliedVelocityCount = 2U;
  step.collisionBatch = collisionBatchPacket();

  iggy3d::PhysicsFrameStats stats = iggy3d::buildPhysicsFrameStats();
  iggy3d::accumulatePhysicsAabbStepStats(stats, step);

  return expect(stats.sourcePacketCount == 1U, "step source count") &&
         expect(stats.bodyCount == 8U, "step body count") &&
         expect(stats.appliedPositionCount == 3U, "step position applies") &&
         expect(stats.appliedVelocityCount == 2U, "step velocity applies") &&
         expect(stats.bindingCount == 3U, "step batch bindings") &&
         expect(stats.contactCount == 2U, "step batch contacts") &&
         expect(near(stats.maxPenetrationMeters, 0.50F),
                "step max penetration");
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

bool multiplePacketsAddCountsAndKeepMaxima() {
  iggy3d::PhysicsAabbCollisionBatchResult first = collisionBatchPacket();
  iggy3d::PhysicsAabbCollisionBatchResult second = collisionBatchPacket();
  second.contacts[1].penetrationMeters = 0.75F;
  second.solvePlans[0].normalImpulseMagnitude = 4.0F;

  iggy3d::PhysicsFrameStats stats = iggy3d::buildPhysicsFrameStats();
  iggy3d::accumulatePhysicsCollisionBatchStats(stats, first);
  iggy3d::accumulatePhysicsCollisionBatchStats(stats, second);

  return expect(stats.sourcePacketCount == 2U, "multi source count") &&
         expect(stats.bindingCount == 6U, "multi bindings") &&
         expect(stats.contactCount == 4U, "multi contacts") &&
         expect(stats.positionCorrectionAppliedCount == 2U,
                "multi correction count") &&
         expect(near(stats.maxPenetrationMeters, 0.75F),
                "multi max penetration") &&
         expect(near(stats.totalNormalImpulseMagnitude, 12.0F),
                "multi total normal") &&
         expect(near(stats.maxNormalImpulseMagnitude, 4.0F),
                "multi max normal");
}

bool failedAndNonfinitePacketsFailStatsDeterministically() {
  iggy3d::PhysicsBroadphaseResult failedBroadphase;
  failedBroadphase.ok = false;
  failedBroadphase.reasonCode = "physics_broadphase_invalid_grid_config";
  failedBroadphase.colliderCount = 7U;
  iggy3d::PhysicsFrameStats failedStats = iggy3d::buildPhysicsFrameStats();
  iggy3d::accumulatePhysicsBroadphaseStats(failedStats, failedBroadphase);

  iggy3d::PhysicsAabbCollisionBatchResult nonfiniteBatch;
  nonfiniteBatch.ok = true;
  nonfiniteBatch.reasonCode = "physics_aabb_collision_batch_batched";
  nonfiniteBatch.contactCount = 1U;
  nonfiniteBatch.contacts.push_back(
      contact(std::numeric_limits<float>::infinity()));
  nonfiniteBatch.solvePlans.push_back(solvePlan(
      std::numeric_limits<float>::quiet_NaN(), 1.0F));
  iggy3d::PhysicsFrameStats nonfiniteStats = iggy3d::buildPhysicsFrameStats();
  iggy3d::accumulatePhysicsCollisionBatchStats(nonfiniteStats,
                                               nonfiniteBatch);

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
                "failed safe counter copied") &&
         expect(!nonfiniteStats.ok, "nonfinite marks stats failed") &&
         expect(nonfiniteStats.status ==
                    iggy3d::PhysicsFrameStatsStatus::NonfiniteScalar,
                "nonfinite status") &&
         expect(nonfiniteStats.reasonCode ==
                    "physics_frame_stats_nonfinite_scalar",
                "nonfinite reason") &&
         expect(nonfiniteStats.failedPacketCount == 1U,
                "nonfinite failed count") &&
         expect(nonfiniteStats.contactCount == 1U,
                "nonfinite safe count copied") &&
         expect(near(nonfiniteStats.maxPenetrationMeters, 0.0F),
                "nonfinite max not poisoned") &&
         expect(near(nonfiniteStats.totalNormalImpulseMagnitude, 0.0F),
                "nonfinite total not poisoned");
}

}  // namespace

int main() {
  const bool ok = emptyStatsAreReady() &&
                  broadphaseCountersAccumulate() &&
                  collisionBatchDerivesContactAndImpulseStats() &&
                  aabbStepCopiesApplyAndBatchStats() &&
                  motorAndPlayerPlannerCountersAccumulate() &&
                  multiplePacketsAddCountsAndKeepMaxima() &&
                  failedAndNonfinitePacketsFailStatsDeterministically();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
