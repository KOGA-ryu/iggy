// Headless-REAL playtest owner integration: spawns, replaces, and reaps
// actual offscreen i3dp children through the production PlaytestProcessOwner.
//   argv[1] = path to the i3dp binary
//   argv[2] = save root holding map_demo.iggy3d.save (fixtures/worlds)

#include "EditorPlaytestProcess.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

namespace {

namespace app = iggy3d_creative_app;
using Clock = std::chrono::steady_clock;

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

double secondsSince(Clock::time_point start) {
  return std::chrono::duration<double>(Clock::now() - start).count();
}

app::PlaytestLaunchPlan planFor(const std::string& i3dpPath,
                                const std::string& saveRoot,
                                const std::string& frames) {
  app::PlaytestLaunchPlan plan;
  plan.valid = true;
  plan.reasonCode = "test_plan";
  plan.binaryPath = i3dpPath;
  plan.argv = {i3dpPath, "--offscreen", "--save-root", saveRoot,
               "--load", "map_demo", "--frames", frames};
  return plan;
}

bool pidGone(std::uint64_t pid) {
  return !std::filesystem::exists("/proc/" + std::to_string(pid));
}

bool waitForPidGone(std::uint64_t pid, double timeoutSeconds) {
  const Clock::time_point start = Clock::now();
  while (secondsSince(start) < timeoutSeconds) {
    if (pidGone(pid)) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  return pidGone(pid);
}

// ---- 1. spawn a short child, poll to natural exit, clean reap ------------

bool spawnPollAndReapCleanExit(const std::string& i3dpPath,
                               const std::string& saveRoot) {
  app::PlaytestProcessOwner owner;
  std::string reason;
  const Clock::time_point start = Clock::now();
  if (!expect(owner.launch(planFor(i3dpPath, saveRoot, "30"), reason),
              ("short child spawns: " + reason).c_str())) {
    return false;
  }
  const std::uint64_t pid = owner.childPid();
  bool exitObserved = false;
  int exitCode = -1;
  while (secondsSince(start) < 30.0) {
    const app::PlaytestProcessOwner::PollResult poll = owner.poll();
    if (poll.exitObserved) {
      exitObserved = true;
      exitCode = poll.exitCode;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  const double elapsed = secondsSince(start);
  std::cout << "  short child: pid=" << pid << " exit=" << exitCode
            << " observed=" << exitObserved << " in " << elapsed << "s\n";
  return expect(pid != 0U, "short child has a pid") &&
         expect(exitObserved, "poll observes the natural exit") &&
         expect(exitCode == 0, "30-frame child exits 0") &&
         expect(!owner.running(), "owner clear after exit") &&
         expect(owner.childPid() == 0U, "handle cleared after reap") &&
         expect(waitForPidGone(pid, 2.0), "no zombie: pid is gone");
}

// ---- 2. replace a long-running child, then shutdown-reap the second ------

bool replaceRunningChildThenShutdown(const std::string& i3dpPath,
                                     const std::string& saveRoot) {
  app::PlaytestProcessOwner owner;
  std::string reason;
  if (!expect(owner.launch(planFor(i3dpPath, saveRoot, "100000"), reason),
              ("long child spawns: " + reason).c_str())) {
    return false;
  }
  // Give the child a beat to boot, then confirm it is genuinely running.
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  const std::uint64_t firstPid = owner.childPid();
  if (!expect(owner.running(), "long child is running") ||
      !expect(firstPid != 0U, "long child has a pid")) {
    return false;
  }

  // Play-replaces semantics: kill + reap the first, then spawn the second.
  const Clock::time_point replaceStart = Clock::now();
  if (app::decidePlaytestLaunchAction(owner.running()) ==
      app::PlaytestLaunchAction::ReplaceRunning) {
    owner.stopRunning();
  }
  const double stopSeconds = secondsSince(replaceStart);
  if (!expect(!owner.running(), "first child stopped") ||
      !expect(waitForPidGone(firstPid, 2.0),
              "first child is dead and reaped (no /proc entry)") ||
      !expect(owner.launch(planFor(i3dpPath, saveRoot, "100000"), reason),
              ("replacement child spawns: " + reason).c_str())) {
    return false;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  const std::uint64_t secondPid = owner.childPid();
  std::cout << "  replace: first pid=" << firstPid << " stopped in "
            << stopSeconds << "s, second pid=" << secondPid << "\n";
  if (!expect(owner.running(), "replacement child is running") ||
      !expect(secondPid != 0U && secondPid != firstPid,
              "replacement is a distinct process")) {
    return false;
  }

  // Editor-exit semantics: shutdown kills + reaps the running child.
  const Clock::time_point shutdownStart = Clock::now();
  owner.shutdown();
  const double shutdownSeconds = secondsSince(shutdownStart);
  std::cout << "  shutdown: second pid stopped in " << shutdownSeconds
            << "s\n";
  return expect(!owner.running(), "owner empty after shutdown") &&
         expect(waitForPidGone(secondPid, 2.0),
                "second child is dead and reaped");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "usage: playtest_process_owner_tests <i3dp> <save-root>\n";
    return 2;
  }
  const std::string i3dpPath = argv[1];
  const std::string saveRoot = argv[2];
  if (!std::filesystem::exists(i3dpPath)) {
    std::cerr << "FAIL: i3dp binary missing: " << i3dpPath << '\n';
    return 1;
  }
  const bool ok = spawnPollAndReapCleanExit(i3dpPath, saveRoot) &&
                  replaceRunningChildThenShutdown(i3dpPath, saveRoot);
  if (ok) {
    std::cout << "playtest_process_owner_tests passed\n";
  }
  return ok ? 0 : 1;
}
