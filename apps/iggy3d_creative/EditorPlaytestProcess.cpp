#include "EditorPlaytestProcess.hpp"

#include <algorithm>
#include <array>
#include <vector>

#include <SDL3/SDL.h>

namespace iggy3d_creative_app {
namespace {

// Graceful-stop budget before escalating to a forced kill: 50 polls x 10ms.
constexpr int kGracefulStopPolls = 50;
constexpr Uint32 kGracefulStopPollDelayMs = 10U;
// Per-frame drain budgets (bounded main-thread work).
constexpr std::size_t kFrameStdoutBudgetBytes = 64U * 1024U;
constexpr std::size_t kFrameStderrBudgetBytes = 16U * 1024U;
// Reap-path drain budget per pass (loops until quiescent).
constexpr std::size_t kReapDrainPassBytes = 256U * 1024U;
constexpr int kReapDrainMaxPasses = 64;

// Bounded read of one stream. Returns bytes read this call; sets `done`
// when the stream hit EOF/error (no more data will ever arrive).
std::size_t readBounded(SDL_IOStream* stream, std::size_t budgetBytes,
                        std::string& out, bool& done) {
  out.clear();
  if (stream == nullptr) {
    done = true;
    return 0U;
  }
  done = false;
  std::array<char, 4096U> chunk{};
  std::size_t total = 0U;
  while (total < budgetBytes) {
    const std::size_t want = std::min(chunk.size(), budgetBytes - total);
    const std::size_t got = SDL_ReadIO(stream, chunk.data(), want);
    if (got == 0U) {
      const SDL_IOStatus status = SDL_GetIOStatus(stream);
      if (status != SDL_IO_STATUS_NOT_READY) {
        done = true;  // EOF or error: the pipe is finished
      }
      break;
    }
    out.append(chunk.data(), got);
    total += got;
  }
  return total;
}

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

bool decidePlaytestStalled(bool childAlive, std::uint64_t livenessAgeMs,
                           std::uint64_t thresholdMs) noexcept {
  return childAlive && livenessAgeMs > thresholdMs;
}

std::string playtestStalledStatusMessage(std::uint64_t livenessAgeMs) {
  return "playtest stalled (" + std::to_string(livenessAgeMs / 1000U) +
         "s) -- Play to replace";
}

std::string composePlaytestExitStatusMessage(
    int exitCode, const std::deque<std::string>& stderrTail) {
  std::string message = playtestExitStatusMessage(exitCode);
  if (exitCode != 0 && !stderrTail.empty()) {
    message += ": ";
    message += stderrTail.back();
  }
  return message;
}

std::string formatPlaytestMonitorRow(const PlaytestEvent& event,
                                     const PlaytestEntityNameMap* names) {
  const auto resolveId = [names](std::string_view raw) -> std::string {
    if (names != nullptr && !raw.empty()) {
      char* end = nullptr;
      const std::uint64_t id = SDL_strtoull(std::string(raw).c_str(), &end, 10);
      const auto found = names->find(id);
      if (found != names->end()) {
        return found->second;
      }
    }
    return {};
  };
  const auto appendFieldsExcept = [&event](std::string& row,
                                           std::initializer_list<std::string_view>
                                               skipKeys) {
    for (const auto& [key, value] : event.fields) {
      bool skip = false;
      for (const std::string_view skipKey : skipKeys) {
        if (key == skipKey) {
          skip = true;
          break;
        }
      }
      if (skip) {
        continue;
      }
      row.push_back(' ');
      row += key;
      row.push_back('=');
      row += value;
    }
  };
  if (event.kind == kPlaytestEventKindRuntimeEvent) {
    std::string row{event.field("kind", "runtime_event")};
    const std::string actorLabel = resolveId(event.field("actor"));
    const std::string targetLabel = resolveId(event.field("target"));
    if (!actorLabel.empty()) {
      row += " " + actorLabel;
    }
    if (!targetLabel.empty()) {
      row += " -> " + targetLabel;
    }
    // Resolved ids drop their raw k=v; unresolved ones stay raw.
    appendFieldsExcept(
        row, {"kind", actorLabel.empty() ? std::string_view{} : "actor",
              targetLabel.empty() ? std::string_view{} : "target"});
    return row;
  }
  if (event.kind == kPlaytestEventKindHeartbeat) {
    std::string row = "heartbeat tick=" + std::string(event.field("tick", "?"));
    row += " (";
    row += event.field("state", "running");
    row += ")";
    return row;
  }
  if (event.kind == kPlaytestEventKindSessionStarted) {
    std::string row = "session started";
    appendFieldsExcept(row, {});
    return row;
  }
  if (event.kind == kPlaytestEventKindSessionEnded) {
    std::string row = "session ended: ";
    row += event.field("reason", "unknown");
    row += " (";
    row += event.field("frames", "?");
    row += " frames)";
    return row;
  }
  return formatPlaytestEventLine(event);  // unknown kind: raw wire line
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

void PlaytestProcessOwner::applyEvent(PlaytestEvent event) {
  SDL_Log("i3dc.playtest: %s", formatPlaytestEventLine(event).c_str());
  ++monitor_.totalEventCount;
  if (!isKnownPlaytestEventKind(event.kind)) {
    ++monitor_.unknownKindCount;
  } else if (event.kind == kPlaytestEventKindHeartbeat) {
    const std::string tick{event.field("tick", "0")};
    monitor_.lastHeartbeatTick = SDL_strtoull(tick.c_str(), nullptr, 10);
    monitor_.lastHeartbeatAtMs = SDL_GetTicks();
    monitor_.lastHeartbeatState = std::string(event.field("state", "running"));
    // EVERY heartbeat is liveness -- suspended ones included.
    monitor_.lastLivenessAtMs = monitor_.lastHeartbeatAtMs;
  }
  monitor_.events.push_back(std::move(event));
  while (monitor_.events.size() > kPlaytestMonitorEventCapacity) {
    monitor_.events.pop_front();
  }
}

void PlaytestProcessOwner::applyStderrChunk(std::string_view chunk) {
  stderrPartial_.append(chunk);
  std::size_t start = 0U;
  while (true) {
    const std::size_t newline = stderrPartial_.find('\n', start);
    if (newline == std::string::npos) {
      break;
    }
    std::string line = stderrPartial_.substr(start, newline - start);
    if (!line.empty()) {
      monitor_.stderrTail.push_back(std::move(line));
      while (monitor_.stderrTail.size() > kPlaytestStderrTailCapacity) {
        monitor_.stderrTail.pop_front();
      }
    }
    start = newline + 1U;
  }
  stderrPartial_.erase(0, start);
}

void PlaytestProcessOwner::drainStreams(std::size_t stdoutBudgetBytes,
                                        std::size_t stderrBudgetBytes) {
  std::string chunk;
  bool done = false;
  if (readBounded(stdoutStream_, stdoutBudgetBytes, chunk, done) > 0U) {
    std::vector<PlaytestEvent> events;
    const PlaytestEventStreamParser::FeedStats stats =
        parser_.feed(chunk, events);
    monitor_.malformedLineCount += stats.malformedCount;
    monitor_.nonProtocolLineCount += stats.nonProtocolCount;
    for (PlaytestEvent& event : events) {
      applyEvent(std::move(event));
    }
  }
  if (done) {
    stdoutStream_ = nullptr;
  }
  done = false;
  if (readBounded(stderrStream_, stderrBudgetBytes, chunk, done) > 0U) {
    applyStderrChunk(chunk);
  }
  if (done) {
    stderrStream_ = nullptr;
  }
}

void PlaytestProcessOwner::finishStreams() {
  // Post-exit: the pipes hold at most their buffered remainder; drain to
  // EOF (bounded passes -- the writer is gone, so this terminates).
  for (int pass = 0; pass < kReapDrainMaxPasses; ++pass) {
    if (stdoutStream_ == nullptr && stderrStream_ == nullptr) {
      break;
    }
    drainStreams(kReapDrainPassBytes, kReapDrainPassBytes);
  }
  std::vector<PlaytestEvent> events;
  const PlaytestEventStreamParser::FeedStats stats = parser_.finish(events);
  monitor_.malformedLineCount += stats.malformedCount;
  monitor_.nonProtocolLineCount += stats.nonProtocolCount;
  for (PlaytestEvent& event : events) {
    applyEvent(std::move(event));
  }
  if (!stderrPartial_.empty()) {
    applyStderrChunk("\n");
  }
}

void PlaytestProcessOwner::stopRunning() {
  if (process_ == nullptr) {
    return;
  }
  // Graceful first, bounded, then forced. DRAIN BEFORE EVERY WAIT: a child
  // blocked writing into a full pipe cannot exit, and a blocking
  // SDL_WaitProcess on it would deadlock -- each poll iteration drains
  // first so the pipe can never wedge the reap.
  static_cast<void>(SDL_KillProcess(process_, false));
  bool exited = false;
  for (int poll = 0; poll < kGracefulStopPolls; ++poll) {
    drainStreams(kReapDrainPassBytes, kReapDrainPassBytes);
    if (SDL_WaitProcess(process_, false, nullptr)) {
      exited = true;
      break;
    }
    SDL_Delay(kGracefulStopPollDelayMs);
  }
  if (!exited) {
    static_cast<void>(SDL_KillProcess(process_, true));
  }
  drainStreams(kReapDrainPassBytes, kReapDrainPassBytes);
  static_cast<void>(SDL_WaitProcess(process_, true, nullptr));
  finishStreams();
  SDL_DestroyProcess(process_);
  process_ = nullptr;
  stdoutStream_ = nullptr;
  stderrStream_ = nullptr;
  monitor_.childRunning = false;
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
    // Exited but not yet observed by poll(): drain, then reap quietly
    // (drain-before-wait holds here too).
    finishStreams();
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

  // Piped stdio: stdout is the IGGY3DP1 event channel, stderr feeds the
  // crash tail. stdin stays at SDL's default (null device).
  const SDL_PropertiesID createProperties = SDL_CreateProperties();
  SDL_SetPointerProperty(createProperties,
                         SDL_PROP_PROCESS_CREATE_ARGS_POINTER, argv.data());
  SDL_SetNumberProperty(createProperties,
                        SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER,
                        SDL_PROCESS_STDIO_APP);
  SDL_SetNumberProperty(createProperties,
                        SDL_PROP_PROCESS_CREATE_STDERR_NUMBER,
                        SDL_PROCESS_STDIO_APP);
  process_ = SDL_CreateProcessWithProperties(createProperties);
  SDL_DestroyProperties(createProperties);
  if (process_ == nullptr) {
    reasonCode = std::string("playtest_spawn_failed: ") + SDL_GetError();
    return false;
  }
  stdoutStream_ = SDL_GetProcessOutput(process_);
  stderrStream_ = static_cast<SDL_IOStream*>(SDL_GetPointerProperty(
      SDL_GetProcessProperties(process_), SDL_PROP_PROCESS_STDERR_POINTER,
      nullptr));
  parser_ = {};
  stderrPartial_.clear();
  monitor_ = {};
  monitor_.childRunning = true;
  monitor_.everRan = true;
  monitor_.lastLivenessAtMs = SDL_GetTicks();  // stall baseline until the
                                               // first heartbeat lands
  reasonCode = "playtest_spawned";
  return true;
}

PlaytestProcessOwner::PollResult PlaytestProcessOwner::poll() {
  PollResult result;
  if (process_ == nullptr) {
    return result;
  }
  // Per-frame bounded, non-blocking drain BEFORE the wait poll (the law).
  drainStreams(kFrameStdoutBudgetBytes, kFrameStderrBudgetBytes);
  int exitCode = 0;
  if (SDL_WaitProcess(process_, false, &exitCode)) {
    finishStreams();
    result.exitObserved = true;
    result.exitCode = exitCode;
    SDL_DestroyProcess(process_);
    process_ = nullptr;
    stdoutStream_ = nullptr;
    stderrStream_ = nullptr;
    monitor_.childRunning = false;
    monitor_.stalled = false;
    monitor_.stallAgeMs = 0U;
    monitor_.lastExitMessage =
        composePlaytestExitStatusMessage(exitCode, monitor_.stderrTail);
    return result;
  }
  result.running = true;
  monitor_.childRunning = true;
  const std::uint64_t ageMs = SDL_GetTicks() - monitor_.lastLivenessAtMs;
  const bool wasStalled = monitor_.stalled;
  monitor_.stalled =
      decidePlaytestStalled(true, ageMs, kPlaytestStallThresholdMs);
  monitor_.stallAgeMs = monitor_.stalled ? ageMs : 0U;
  result.stalled = monitor_.stalled;
  result.stallAgeMs = monitor_.stallAgeMs;
  result.stallRecovered = wasStalled && !monitor_.stalled;
  return result;
}

void PlaytestProcessOwner::setSnapshotEntityNames(
    PlaytestEntityNameMap names) {
  monitor_.entityNames = std::move(names);
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
