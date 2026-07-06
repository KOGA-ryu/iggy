#include "runtime/ai/ReconIntel.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::GuardReconObservation makeGuard(std::uint64_t id, float alert, bool lastKnown) {
  iggy3d::GuardReconObservation g;
  g.guard = iggy3d::EntityId{id};
  g.position = iggy3d::Vec3{static_cast<float>(id), 0.0F, 0.0F};
  g.alertLevel = alert;
  g.hasLastKnownTarget = lastKnown;
  return g;
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

}  // namespace

int main() {
  const bool ok = capturesGarrisonSummary() && emptyGarrisonIsValid() &&
                  hashIsDeterministic() && hashIsOrderSensitive();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
