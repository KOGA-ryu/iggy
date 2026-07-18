// Pure playtest lifecycle decisions: no processes, no SDL calls.

#include "EditorPlaytestProcess.hpp"

#include <iostream>

namespace {

namespace app = iggy3d_creative_app;

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool launchDecision() {
  return expect(app::decidePlaytestLaunchAction(false) ==
                    app::PlaytestLaunchAction::SpawnFresh,
                "no child -> spawn fresh") &&
         expect(app::decidePlaytestLaunchAction(true) ==
                    app::PlaytestLaunchAction::ReplaceRunning,
                "child running -> kill-then-spawn");
}

bool shutdownDecision() {
  return expect(app::decidePlaytestShutdownAction(false) ==
                    app::PlaytestShutdownAction::None,
                "no child -> shutdown does nothing") &&
         expect(app::decidePlaytestShutdownAction(true) ==
                    app::PlaytestShutdownAction::KillAndReap,
                "child running -> shutdown kills and reaps");
}

bool exitMessages() {
  return expect(app::playtestExitStatusMessage(0) == "playtest ended",
                "clean exit message") &&
         expect(app::playtestExitStatusMessage(3) ==
                    "playtest crashed (exit 3)",
                "nonzero exit message") &&
         expect(app::playtestExitStatusMessage(-15) ==
                    "playtest crashed (signal 15)",
                "signal exit message") &&
         expect(app::playtestExitStatusMessage(-255) ==
                    "playtest crashed (signal 255)",
                "abnormal exit reports as the SDL sentinel signal") &&
         expect(app::playtestRunningStatusMessage() == "playtest running",
                "running status text");
}

bool stallDecision() {
  using app::decidePlaytestStalled;
  constexpr std::uint64_t kT = app::kPlaytestStallThresholdMs;
  return expect(!decidePlaytestStalled(true, kT - 1U, kT),
                "alive + fresh heartbeat -> ok") &&
         expect(decidePlaytestStalled(true, kT + 1U, kT),
                "alive + stale -> stalled") &&
         expect(!decidePlaytestStalled(false, kT * 10U, kT),
                "dead is never stalled (exit reporting owns it)") &&
         // Suspended-state heartbeats refresh liveness upstream, so their
         // age stays low -- semantically: suspended-but-heartbeating -> ok.
         expect(!decidePlaytestStalled(true, 0U, kT),
                "suspended-but-heartbeating (age refreshed) -> ok") &&
         expect(app::playtestStalledStatusMessage(7400U) ==
                    "playtest stalled (7s) -- Play to replace",
                "stalled message names the age and the remedy");
}

}  // namespace

int main() {
  const bool ok = launchDecision() && shutdownDecision() && exitMessages() &&
                  stallDecision();
  if (ok) {
    std::cout << "playtest_lifecycle_tests passed\n";
  }
  return ok ? 0 : 1;
}
