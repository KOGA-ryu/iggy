#include "runtime/physics/PhysicsFrameStats.hpp"

#include <algorithm>
#include <array>
#include <cmath>

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
