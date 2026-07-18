#include "EditorPlaytestProcess.hpp"

#include <vector>

#include <SDL3/SDL.h>

namespace iggy3d_creative_app {
namespace {

// Graceful-stop budget before escalating to a forced kill: 50 polls x 10ms.
constexpr int kGracefulStopPolls = 50;
constexpr Uint32 kGracefulStopPollDelayMs = 10U;

}  // namespace

PlaytestLaunchAction decidePlaytestLaunchAction(bool childRunning) noexcept {
  return childRunning ? PlaytestLaunchAction::ReplaceRunning
                      : PlaytestLaunchAction::SpawnFresh;
}

PlaytestShutdownAction decidePlaytestShutdownAction(
    bool childRunning) noexcept {
  return childRunning ? PlaytestShutdownAction::KillAndReap
                      : PlaytestShutdownAction::None;
}

std::string playtestExitStatusMessage(int exitCode) {
  if (exitCode == 0) {
    return "playtest ended";
  }
  if (exitCode < 0) {
    return "playtest crashed (signal " + std::to_string(-exitCode) + ")";
  }
  return "playtest crashed (exit " + std::to_string(exitCode) + ")";
}

std::string_view playtestRunningStatusMessage() noexcept {
  return "playtest running";
}

PlaytestProcessControl::~PlaytestProcessControl() = default;

PlaytestProcessOwner::~PlaytestProcessOwner() {
  shutdown();
}

bool PlaytestProcessOwner::running() {
  if (process_ == nullptr) {
    return false;
  }
  // Repeatable non-consuming poll (SDL_WaitProcess may be called multiple
  // times); an exited-but-not-yet-polled child still counts as not running.
  return !SDL_WaitProcess(process_, false, nullptr);
}

void PlaytestProcessOwner::stopRunning() {
  if (process_ == nullptr) {
    return;
  }
  // Graceful first (half-written state is the child's to avoid), bounded,
  // then forced. SDL_KillProcess only signals; the blocking wait is the
  // reap, and SDL_DestroyProcess frees the handle without stopping anything
  // -- hence this exact order.
  static_cast<void>(SDL_KillProcess(process_, false));
  bool exited = false;
  for (int poll = 0; poll < kGracefulStopPolls; ++poll) {
    if (SDL_WaitProcess(process_, false, nullptr)) {
      exited = true;
      break;
    }
    SDL_Delay(kGracefulStopPollDelayMs);
  }
  if (!exited) {
    static_cast<void>(SDL_KillProcess(process_, true));
  }
  static_cast<void>(SDL_WaitProcess(process_, true, nullptr));
  SDL_DestroyProcess(process_);
  process_ = nullptr;
}

bool PlaytestProcessOwner::launch(const PlaytestLaunchPlan& plan,
                                  std::string& reasonCode) {
  if (!plan.valid) {
    reasonCode = plan.reasonCode;
    return false;
  }
  if (running()) {
    reasonCode = "playtest_child_still_running";
    return false;
  }
  if (process_ != nullptr) {
    // Exited but not yet observed by poll(): reap quietly before reuse.
    static_cast<void>(SDL_WaitProcess(process_, true, nullptr));
    SDL_DestroyProcess(process_);
    process_ = nullptr;
  }
  std::vector<const char*> argv;
  argv.reserve(plan.argv.size() + 1U);
  for (const std::string& argument : plan.argv) {
    argv.push_back(argument.c_str());
  }
  argv.push_back(nullptr);
  process_ = SDL_CreateProcess(argv.data(), /*pipe_stdio=*/false);
  if (process_ == nullptr) {
    reasonCode = std::string("playtest_spawn_failed: ") + SDL_GetError();
    return false;
  }
  reasonCode = "playtest_spawned";
  return true;
}

PlaytestProcessOwner::PollResult PlaytestProcessOwner::poll() {
  PollResult result;
  if (process_ == nullptr) {
    return result;
  }
  int exitCode = 0;
  if (SDL_WaitProcess(process_, false, &exitCode)) {
    result.exitObserved = true;
    result.exitCode = exitCode;
    SDL_DestroyProcess(process_);
    process_ = nullptr;
    return result;
  }
  result.running = true;
  return result;
}

void PlaytestProcessOwner::shutdown() {
  stopRunning();
}

std::uint64_t PlaytestProcessOwner::childPid() const {
  if (process_ == nullptr) {
    return 0U;
  }
  const SDL_PropertiesID properties = SDL_GetProcessProperties(process_);
  if (properties == 0U) {
    return 0U;
  }
  const Sint64 pid =
      SDL_GetNumberProperty(properties, SDL_PROP_PROCESS_PID_NUMBER, 0);
  return pid <= 0 ? 0U : static_cast<std::uint64_t>(pid);
}

}  // namespace iggy3d_creative_app
