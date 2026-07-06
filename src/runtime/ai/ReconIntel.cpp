#include "runtime/ai/ReconIntel.hpp"

namespace iggy3d {

ReconIntel captureReconIntel(std::span<const GuardReconObservation> observations) {
  ReconIntel intel;
  intel.guards.assign(observations.begin(), observations.end());
  intel.guardCount = static_cast<std::uint32_t>(intel.guards.size());
  for (const GuardReconObservation& guard : intel.guards) {
    if (guard.alertLevel >= kReconAlertedLevel) {
      ++intel.alertedGuardCount;
    }
    if (guard.hasLastKnownTarget) {
      intel.anyGuardHasLastKnownTarget = true;
    }
  }
  return intel;
}

StableHashValue hashReconIntel(const ReconIntel& intel) {
  StableHasher hasher;
  hasher.addU64(intel.guardCount);
  hasher.addU64(intel.alertedGuardCount);
  hasher.addBool(intel.anyGuardHasLastKnownTarget);
  // Order-sensitive fold: swapping two guards changes the digest (no set-semantics).
  for (const GuardReconObservation& g : intel.guards) {
    hasher.addU64(g.guard.value);
    addVec3Quantized(hasher, g.position);
    addVec3Quantized(hasher, g.facing);
    hasher.addFloatQuantized(g.alertLevel);
    hasher.addString(g.behavior);
    hasher.addBool(g.patrols);
    hasher.addU64(g.patrolWaypointCount);
    hasher.addU64(g.patrolTargetIndex);
    hasher.addU64(static_cast<std::uint64_t>(g.patrolMode));
    addVec3Quantized(hasher, g.patrolTarget);
    hasher.addBool(g.hasLastKnownTarget);
    addVec3Quantized(hasher, g.lastKnownTargetPosition);
    hasher.addBool(g.hasWatchedNode);
    hasher.addU64(static_cast<std::uint64_t>(g.watchedNodeKind));
    addVec3Quantized(hasher, g.watchedNodePosition);
  }
  return hasher.value();
}

}  // namespace iggy3d
