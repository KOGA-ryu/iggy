#include "runtime/ai/ReconIntel.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool sameVec3(iggy3d::Vec3 a, iggy3d::Vec3 b) {
  return a.x == b.x && a.y == b.y && a.z == b.z;
}

iggy3d::GuardReconObservation makeGuard(std::uint64_t id, float alert, bool lastKnown) {
  iggy3d::GuardReconObservation g;
  g.guard = iggy3d::EntityId{id};
  g.position = iggy3d::Vec3{static_cast<float>(id), 0.0F, 0.0F};
  g.alertLevel = alert;
  g.hasLastKnownTarget = lastKnown;
  return g;
}

// Every field populated -- the serialization round-trip target.
iggy3d::GuardReconObservation makeFullGuard(std::uint64_t id) {
  iggy3d::GuardReconObservation g;
  g.guard = iggy3d::EntityId{id};
  g.position = iggy3d::Vec3{1.25F, 2.0F, -3.5F};
  g.facing = iggy3d::Vec3{0.0F, 0.0F, -1.0F};
  g.alertLevel = 0.6F;
  g.behavior = iggy3d::aiBehaviorKindName(iggy3d::AiBehaviorKind::Searching);
  g.patrols = true;
  g.patrolWaypointCount = 3;
  g.patrolTargetIndex = 2;
  g.patrolMode = iggy3d::PatrolMode::Loop;
  g.patrolTarget = iggy3d::Vec3{5.0F, 0.0F, 5.0F};
  g.hasLastKnownTarget = true;
  g.lastKnownTargetPosition = iggy3d::Vec3{3.0F, 0.0F, 2.0F};
  g.hasWatchedNode = true;
  g.watchedNodeKind = iggy3d::ReasoningNodeKind::patrolPost;
  g.watchedNodePosition = iggy3d::Vec3{6.0F, 0.0F, 6.0F};
  return g;
}

bool sameGuard(const iggy3d::GuardReconObservation& a,
               const iggy3d::GuardReconObservation& b) {
  return a.guard == b.guard && sameVec3(a.position, b.position) &&
         sameVec3(a.facing, b.facing) && a.alertLevel == b.alertLevel &&
         a.behavior == b.behavior && a.patrols == b.patrols &&
         a.patrolWaypointCount == b.patrolWaypointCount &&
         a.patrolTargetIndex == b.patrolTargetIndex && a.patrolMode == b.patrolMode &&
         sameVec3(a.patrolTarget, b.patrolTarget) &&
         a.hasLastKnownTarget == b.hasLastKnownTarget &&
         sameVec3(a.lastKnownTargetPosition, b.lastKnownTargetPosition) &&
         a.hasWatchedNode == b.hasWatchedNode &&
         a.watchedNodeKind == b.watchedNodeKind &&
         sameVec3(a.watchedNodePosition, b.watchedNodePosition);
}

bool capturesGarrisonSummary() {
  const std::vector<iggy3d::GuardReconObservation> obs = {
      makeGuard(1, 0.9F, true),   // alerted, knows about the intruder
      makeGuard(2, 0.1F, false),  // calm patrol
      makeGuard(3, 0.5F, false),  // exactly at threshold -> counts as alerted
  };
  const iggy3d::ReconIntel intel = iggy3d::captureReconIntel(obs);
  return expect(intel.guardCount == 3U, "guard count") &&
         expect(intel.guards.size() == 3U, "guards stored") &&
         expect(intel.alertedGuardCount == 2U, "alerted count (>= threshold)") &&
         expect(intel.anyGuardHasLastKnownTarget, "any last-known target") &&
         expect(intel.guards[0].guard.value == 1U, "scouted order preserved");
}

bool emptyGarrisonIsValid() {
  const std::vector<iggy3d::GuardReconObservation> none;
  const iggy3d::ReconIntel intel = iggy3d::captureReconIntel(none);
  return expect(intel.guardCount == 0U, "empty guard count") &&
         expect(intel.alertedGuardCount == 0U, "empty alerted count") &&
         expect(!intel.anyGuardHasLastKnownTarget, "empty last-known");
}

bool hashIsDeterministic() {
  const std::vector<iggy3d::GuardReconObservation> obs = {
      makeGuard(1, 0.9F, true), makeGuard(2, 0.1F, false)};
  const iggy3d::ReconIntel a = iggy3d::captureReconIntel(obs);
  const iggy3d::ReconIntel b = iggy3d::captureReconIntel(obs);
  return expect(iggy3d::hashReconIntel(a) == iggy3d::hashReconIntel(b),
                "identical packets hash identically");
}

bool hashIsOrderSensitive() {
  const std::vector<iggy3d::GuardReconObservation> ab = {
      makeGuard(1, 0.9F, true), makeGuard(2, 0.1F, false)};
  const std::vector<iggy3d::GuardReconObservation> ba = {
      makeGuard(2, 0.1F, false), makeGuard(1, 0.9F, true)};
  const iggy3d::StableHashValue h1 =
      iggy3d::hashReconIntel(iggy3d::captureReconIntel(ab));
  const iggy3d::StableHashValue h2 =
      iggy3d::hashReconIntel(iggy3d::captureReconIntel(ba));
  return expect(h1 != h2,
                "reordering guards changes the hash (no set-semantics)");
}

bool serializeRoundTrips() {
  const std::vector<iggy3d::GuardReconObservation> obs = {makeFullGuard(11),
                                                          makeFullGuard(22)};
  const iggy3d::ReconIntel original = iggy3d::captureReconIntel(obs);
  const std::string text = iggy3d::serializeReconIntel(original);
  const iggy3d::ReconIntelDecodeResult decoded =
      iggy3d::deserializeReconIntel(text);

  bool ok =
      expect(decoded.ok, "decode ok") &&
      expect(decoded.intel.guards.size() == 2U, "two guards restored") &&
      expect(decoded.intel.guardCount == original.guardCount, "guard count restored") &&
      expect(decoded.intel.alertedGuardCount == original.alertedGuardCount,
             "alerted count restored") &&
      expect(decoded.intel.anyGuardHasLastKnownTarget ==
                 original.anyGuardHasLastKnownTarget,
             "last-known restored") &&
      expect(iggy3d::hashReconIntel(decoded.intel) == iggy3d::hashReconIntel(original),
             "round-trip preserves the hash");
  for (std::size_t i = 0; ok && i < original.guards.size(); ++i) {
    ok = expect(sameGuard(original.guards[i], decoded.intel.guards[i]),
                "every guard field survives the round-trip");
  }
  return ok;
}

bool emptyIntelRoundTrips() {
  const std::vector<iggy3d::GuardReconObservation> none;
  const std::string text =
      iggy3d::serializeReconIntel(iggy3d::captureReconIntel(none));
  const iggy3d::ReconIntelDecodeResult r = iggy3d::deserializeReconIntel(text);
  return expect(r.ok, "empty packet ok") &&
         expect(r.intel.guards.empty(), "no guards restored");
}

bool decodeRejectsBadHeader() {
  const iggy3d::ReconIntelDecodeResult r =
      iggy3d::deserializeReconIntel("not a recon packet\n");
  return expect(!r.ok, "bad header rejected") &&
         expect(r.reasonCode == "recon_intel_bad_header", "reason code");
}

}  // namespace

int main() {
  const bool ok = capturesGarrisonSummary() && emptyGarrisonIsValid() &&
                  hashIsDeterministic() && hashIsOrderSensitive() &&
                  serializeRoundTrips() && emptyIntelRoundTrips() &&
                  decodeRejectsBadHeader();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
