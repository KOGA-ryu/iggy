#pragma once

// Playtest child lifecycle. The editor owns exactly one playtest process:
// Play always replaces (kill -> reap -> snapshot -> spawn), editor exit
// reaps the child, exits are polled non-blocking each frame and surfaced in
// the status bar. Decision logic is pure free functions (unit-tested, no
// processes); the process plumbing lives in PlaytestProcessOwner over
// SDL_CreateProcess / SDL_WaitProcess(block=false) / SDL_KillProcess /
// SDL_DestroyProcess. Verified SDL 3.4 semantics: WaitProcess is a
// repeatable poll (exit code: >=0 normal, negative = signal, -255
// abnormal); KillProcess signals but never reaps; DestroyProcess only
// frees the tracking object -- so the reap order is kill(graceful) ->
// bounded poll -> kill(force) -> Wait(block=true) -> Destroy.

#include <cstdint>
#include <string>
#include <string_view>

#include "EditorPlaytestLaunch.hpp"

struct SDL_Process;

namespace iggy3d_creative_app {

// ---- pure lifecycle decisions -------------------------------------------

enum class PlaytestLaunchAction : std::uint8_t {
  SpawnFresh,      // no child running
  ReplaceRunning,  // kill + reap the running child first
};
[[nodiscard]] PlaytestLaunchAction decidePlaytestLaunchAction(
    bool childRunning) noexcept;

enum class PlaytestShutdownAction : std::uint8_t {
  None,
  KillAndReap,
};
[[nodiscard]] PlaytestShutdownAction decidePlaytestShutdownAction(
    bool childRunning) noexcept;

// Exit-code -> status-bar text. 0 -> "playtest ended"; negative (signal
// termination per SDL) -> "playtest crashed (signal N)"; anything else ->
// "playtest crashed (exit N)".
[[nodiscard]] std::string playtestExitStatusMessage(int exitCode);

[[nodiscard]] std::string_view playtestRunningStatusMessage() noexcept;

// ---- the narrow seam the dispatcher sees --------------------------------
// Keeps dispatchCreativeDesktopCommands pure-decision and headless-testable:
// it decides (validate; running? stop first) and asks this interface to act.

class PlaytestProcessControl {
 public:
  virtual ~PlaytestProcessControl();
  [[nodiscard]] virtual bool running() = 0;
  // Kill (graceful, then forced) and reap the running child. No-op without
  // one. Returns after the child is reaped -- the snapshot may be safely
  // rewritten once this returns.
  virtual void stopRunning() = 0;
  // Spawn a fresh child from the plan, keeping the handle. Refuses (with
  // reason) if a child is still running or the plan/spawn is invalid.
  [[nodiscard]] virtual bool launch(const PlaytestLaunchPlan& plan,
                                    std::string& reasonCode) = 0;
};

// ---- the app-shell owner -------------------------------------------------

class PlaytestProcessOwner final : public PlaytestProcessControl {
 public:
  PlaytestProcessOwner() = default;
  ~PlaytestProcessOwner() override;
  PlaytestProcessOwner(const PlaytestProcessOwner&) = delete;
  PlaytestProcessOwner& operator=(const PlaytestProcessOwner&) = delete;

  [[nodiscard]] bool running() override;
  void stopRunning() override;
  [[nodiscard]] bool launch(const PlaytestLaunchPlan& plan,
                            std::string& reasonCode) override;

  // Non-blocking per-frame poll. When the child exited since the last poll,
  // exitObserved is true exactly once and the handle is reaped + cleared.
  struct PollResult {
    bool running = false;
    bool exitObserved = false;
    int exitCode = 0;
  };
  [[nodiscard]] PollResult poll();

  // Kill + reap + destroy any running child (editor shutdown path; also the
  // destructor's behavior).
  void shutdown();

  // Observability for tests: the child's OS pid, 0 when none.
  [[nodiscard]] std::uint64_t childPid() const;

 private:
  SDL_Process* process_ = nullptr;
};

}  // namespace iggy3d_creative_app
