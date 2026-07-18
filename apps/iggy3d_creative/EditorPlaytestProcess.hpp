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
#include <deque>
#include <string>
#include <string_view>
#include <vector>

#include "EditorPlaytestLaunch.hpp"
#include "app/iggy3d/creative/play/PlaytestEventProtocol.hpp"

struct SDL_Process;
struct SDL_IOStream;

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

// ---- hang detection (pure) -----------------------------------------------
// Stalled = child ALIVE and no liveness heartbeat for longer than the
// threshold. Suspended-state heartbeats are still heartbeats (an unfocused
// playtest is the NORMAL state while the editor has focus); a dead child is
// never stalled -- exit reporting owns that. No auto-kill: Play replaces.

inline constexpr std::uint64_t kPlaytestStallThresholdMs = 5000U;

[[nodiscard]] bool decidePlaytestStalled(bool childAlive,
                                         std::uint64_t livenessAgeMs,
                                         std::uint64_t thresholdMs) noexcept;

// "playtest stalled (<age>s) -- Play to replace"
[[nodiscard]] std::string playtestStalledStatusMessage(
    std::uint64_t livenessAgeMs);

// One Play Monitor row, readable: known kinds render as a story line with
// their payload fields ("moved actor=2 tick=140"); unknown kinds fall back
// to the raw wire line. Ids stay raw -- name resolution is a future slice.
[[nodiscard]] std::string formatPlaytestMonitorRow(const PlaytestEvent& event);

// Exit message plus the captured stderr tail's last line (pure; the full
// tail lives in the Play Monitor). Clean exits never carry a tail.
[[nodiscard]] std::string composePlaytestExitStatusMessage(
    int exitCode, const std::deque<std::string>& stderrTail);

// ---- read-only monitor state (transient desktop state; never saved) ------

inline constexpr std::size_t kPlaytestMonitorEventCapacity = 64U;
inline constexpr std::size_t kPlaytestStderrTailCapacity = 6U;

struct PlaytestMonitorState {
  bool childRunning = false;
  bool everRan = false;
  std::string lastExitMessage;
  std::deque<PlaytestEvent> events;  // newest at the back, bounded
  std::size_t totalEventCount = 0U;
  std::size_t unknownKindCount = 0U;
  std::size_t malformedLineCount = 0U;
  std::size_t nonProtocolLineCount = 0U;
  std::uint64_t lastHeartbeatTick = 0U;
  std::uint64_t lastHeartbeatAtMs = 0U;  // SDL_GetTicks() at receipt
  std::string lastHeartbeatState;        // "running" / "suspended"
  // Liveness baseline: spawn time, refreshed by EVERY heartbeat (suspended
  // included). Stall age measures against this.
  std::uint64_t lastLivenessAtMs = 0U;
  bool stalled = false;
  std::uint64_t stallAgeMs = 0U;
  std::deque<std::string> stderrTail;    // bounded
};

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
    bool stalled = false;
    std::uint64_t stallAgeMs = 0U;
    bool stallRecovered = false;  // fresh heartbeat after a stalled stretch
  };
  [[nodiscard]] PollResult poll();

  // Kill + reap + destroy any running child (editor shutdown path; also the
  // destructor's behavior).
  void shutdown();

  // Observability for tests: the child's OS pid, 0 when none.
  [[nodiscard]] std::uint64_t childPid() const;

  // Read-only monitor projection for the Diagnostics Play Monitor tab.
  [[nodiscard]] const PlaytestMonitorState& monitor() const {
    return monitor_;
  }

 private:
  // Non-blocking bounded drain of the child's piped stdout/stderr into the
  // monitor (events parsed, tail collected). THE DRAIN-BEFORE-WAIT LAW:
  // every path that waits on the child drains first, so a full pipe can
  // never deadlock a reap.
  void drainStreams(std::size_t stdoutBudgetBytes,
                    std::size_t stderrBudgetBytes);
  void finishStreams();  // post-exit: drain remainder + flush partial line
  void applyEvent(PlaytestEvent event);
  void applyStderrChunk(std::string_view chunk);

  SDL_Process* process_ = nullptr;
  SDL_IOStream* stdoutStream_ = nullptr;
  SDL_IOStream* stderrStream_ = nullptr;
  PlaytestEventStreamParser parser_;
  std::string stderrPartial_;
  PlaytestMonitorState monitor_;
};

}  // namespace iggy3d_creative_app
