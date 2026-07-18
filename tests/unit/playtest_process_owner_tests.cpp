// Headless-REAL playtest owner integration: spawns, replaces, and reaps
// actual offscreen i3dp children through the production PlaytestProcessOwner.
//   argv[1] = path to the i3dp binary
//   argv[2] = save root holding map_demo.iggy3d.save (fixtures/worlds)

#include "EditorPlaytestNames.hpp"
#include "EditorPlaytestProcess.hpp"

#include "app/iggy3d/creative/play/PlayPreparation.hpp"
#include "app/iggy3d/creative/world/WorldService.hpp"
#include "content/assets/StaticMeshAsset.hpp"

#include <csignal>

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
  if (!expect(owner.launch(planFor(i3dpPath, saveRoot, "240"), reason),
              ("short child spawns: " + reason).c_str())) {
    return false;
  }
  // Snapshot-time name capture, exactly as the Play case does: from the
  // same fixture document the child is playing.
  const iggy3d::CreativeWorldOpenResult fixture =
      iggy3d::openCreativeWorld({saveRoot, "map_demo"});
  const iggy3d::StaticMeshAssetCatalog catalog =
      iggy3d::discoverStaticMeshAssetCatalog("assets/creative");
  iggy3d::creative::CreativePlayPreparationRequest preparationRequest;
  preparationRequest.document = &fixture.document;
  preparationRequest.staticMeshAssetCatalog = &catalog;
  const iggy3d::creative::CreativePlayPreparationResult preparation =
      iggy3d::creative::prepareCreativePlay(preparationRequest);
  if (!expect(fixture.accepted && preparation.accepted &&
                  preparation.payload.has_value(),
              "fixture prepares for name capture")) {
    return false;
  }
  owner.setSnapshotEntityNames(
      app::buildPlaytestNameMap(*preparation.payload, fixture.document));
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
  const app::PlaytestMonitorState& monitor = owner.monitor();
  std::cout << "  short child: pid=" << pid << " exit=" << exitCode
            << " observed=" << exitObserved << " in " << elapsed
            << "s events=" << monitor.totalEventCount << "\n";
  // Protocol assertions: session_started first, heartbeats present,
  // session_ended last with the frame-limit reason, no wire noise.
  bool sawStarted = false;
  bool sawHeartbeat = false;
  bool startedFirst = false;
  bool sawEnrichedRuntimeEvent = false;
  bool startedCarriesFocusField = false;
  for (const app::PlaytestEvent& event : monitor.events) {
    if (event.kind == app::kPlaytestEventKindSessionStarted) {
      // Presence only: headless cannot own focus truth (the value is the
      // OS's answer on a real desktop -- Ace's verification).
      startedCarriesFocusField = !event.field("focused").empty() &&
                                 !event.field("window").empty() &&
                                 !event.field("fullscreen").empty();
    }
    if (event.kind == app::kPlaytestEventKindRuntimeEvent &&
        !event.field("actor").empty() && !event.field("sequence").empty()) {
      sawEnrichedRuntimeEvent = true;  // guard patrol 'moved' carries ids
    }
    if (event.kind == app::kPlaytestEventKindSessionStarted) {
      sawStarted = true;
      startedFirst = &event == &monitor.events.front() ||
                     monitor.totalEventCount >
                         monitor.events.size();  // ring may have evicted it
    } else if (event.kind == app::kPlaytestEventKindHeartbeat) {
      sawHeartbeat = true;
    }
  }
  // The ring may evict mid-run events; the monitor keeps sticky heartbeat
  // facts on receipt, which is the durable proof. Wall-clock cadence: any
  // received heartbeat counts, and the offscreen (focused) child runs.
  sawHeartbeat = sawHeartbeat || monitor.lastHeartbeatAtMs != 0U;
  const bool heartbeatStateOk = monitor.lastHeartbeatState == "running";
  const bool ringEvicted = monitor.totalEventCount > monitor.events.size();
  return expect(pid != 0U, "short child has a pid") &&
         expect(exitObserved, "poll observes the natural exit") &&
         expect(exitCode == 0, "240-frame child exits 0") &&
         expect(sawStarted || ringEvicted,
                "session_started received (or evicted by volume)") &&
         expect(startedFirst || ringEvicted, "session_started arrived first") &&
         expect(sawHeartbeat, "heartbeat received") &&
         expect(heartbeatStateOk, "wall-clock heartbeat carries state=running") &&
         expect(sawEnrichedRuntimeEvent,
                "an enriched runtime_event arrived with raw ids intact") &&
         expect(startedCarriesFocusField || monitor.totalEventCount >
                                                monitor.events.size(),
                "session_started carries focused/window/fullscreen receipts") &&
         expect([&monitor]() {
                  for (const app::PlaytestEvent& event : monitor.events) {
                    if (event.kind != app::kPlaytestEventKindRuntimeEvent) {
                      continue;
                    }
                    const std::string row = app::formatPlaytestMonitorRow(
                        event, &monitor.entityNames);
                    if (row.find("Guard") != std::string::npos &&
                        row.find("actor=") == std::string::npos) {
                      std::cout << "  resolved row: " << row << "\n";
                      return true;
                    }
                  }
                  return false;
                }(),
                "a patrol event resolves to the fixture's guard name") &&
         expect(!monitor.events.empty() &&
                    monitor.events.back().kind ==
                        app::kPlaytestEventKindSessionEnded &&
                    monitor.events.back().field("reason") ==
                        "frame_limit_reached",
                "session_ended last with frame_limit_reached") &&
         expect(monitor.malformedLineCount == 0U &&
                    monitor.nonProtocolLineCount == 0U,
                "clean wire: no malformed or foreign stdout lines") &&
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

// ---- 3. the pipe-full kill-replace deadlock case -------------------------
// A child flooding stdout blocks in write() once the pipe fills; a blocking
// wait without draining would deadlock the reap. The owner's drain-before-
// wait law must make kill-and-replace complete promptly anyway.

bool pipeFullKillReplaceDoesNotDeadlock(const std::string& i3dpPath,
                                        const std::string& saveRoot) {
  app::PlaytestProcessOwner owner;
  std::string reason;
  app::PlaytestLaunchPlan flood;
  flood.valid = true;
  flood.reasonCode = "test_flood_plan";
  flood.binaryPath = "/bin/sh";
  flood.argv = {"/bin/sh", "-c",
                "i=0; while [ $i -lt 200000 ]; do echo IGGY3DP1 heartbeat "
                "tick=$i; i=$((i+1)); done; sleep 600"};
  if (!expect(owner.launch(flood, reason),
              ("flood child spawns: " + reason).c_str())) {
    return false;
  }
  const std::uint64_t floodPid = owner.childPid();
  // Do NOT drain: let the writer fill the pipe and block.
  std::this_thread::sleep_for(std::chrono::milliseconds(400));
  const Clock::time_point stopStart = Clock::now();
  owner.stopRunning();  // kill-and-replace path; must drain before waits
  const double stopSeconds = secondsSince(stopStart);
  const app::PlaytestMonitorState& monitor = owner.monitor();
  std::cout << "  flood child: pid=" << floodPid << " stopped in "
            << stopSeconds << "s drained events=" << monitor.totalEventCount
            << "\n";
  if (!expect(stopSeconds < 3.0,
              "pipe-full kill-replace completes promptly (no deadlock)") ||
      !expect(waitForPidGone(floodPid, 2.0), "flood child reaped") ||
      !expect(monitor.totalEventCount > 1000U,
              "the drained backlog was actually processed")) {
    return false;
  }
  // Replace with a real i3dp child, then shutdown-reap it.
  if (!expect(owner.launch(planFor(i3dpPath, saveRoot, "100000"), reason),
              ("replacement after flood spawns: " + reason).c_str())) {
    return false;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  const std::uint64_t replacementPid = owner.childPid();
  const bool replacementRuns = owner.running();
  owner.shutdown();
  return expect(replacementRuns && replacementPid != 0U,
                "replacement child ran after the flood") &&
         expect(waitForPidGone(replacementPid, 2.0),
                "replacement reaped on shutdown");
}

// ---- 4. hang detection: silent-but-alive children ------------------------

app::PlaytestLaunchPlan scriptPlan(const std::string& script) {
  app::PlaytestLaunchPlan plan;
  plan.valid = true;
  plan.reasonCode = "test_script_plan";
  plan.binaryPath = "/bin/sh";
  plan.argv = {"/bin/sh", "-c", script};
  return plan;
}

bool silentChildStallsRecoversAndReplaces(const std::string& i3dpPath,
                                          const std::string& saveRoot) {
  static_cast<void>(i3dpPath);
  static_cast<void>(saveRoot);
  app::PlaytestProcessOwner owner;
  std::string reason;
  // Child 1: heartbeats ~1s, then SILENT-BUT-ALIVE for 8s, then resumes
  // with a state=suspended heartbeat (still liveness), then keeps beating.
  const std::string script =
      "echo IGGY3DP1 session_started doc=0 rev=0 room=hang_test; "
      "echo IGGY3DP1 heartbeat tick=1 state=running; sleep 0.5; "
      "echo IGGY3DP1 heartbeat tick=2 state=running; "
      "sleep 8; "
      "echo IGGY3DP1 heartbeat tick=3 state=suspended; "
      "while true; do echo IGGY3DP1 heartbeat tick=4 state=running; "
      "sleep 1; done";
  if (!expect(owner.launch(scriptPlan(script), reason),
              ("hang-test child spawns: " + reason).c_str())) {
    return false;
  }
  bool everStalledEarly = false;
  bool stalledObserved = false;
  bool recoveredObserved = false;
  double stalledAtSeconds = 0.0;
  const Clock::time_point start = Clock::now();
  while (secondsSince(start) < 20.0) {
    const app::PlaytestProcessOwner::PollResult poll = owner.poll();
    if (!poll.running) {
      break;
    }
    if (secondsSince(start) < 4.0 && poll.stalled) {
      everStalledEarly = true;  // must NOT trip while heartbeating
    }
    if (poll.stalled && !stalledObserved) {
      stalledObserved = true;
      stalledAtSeconds = secondsSince(start);
    }
    if (poll.stallRecovered) {
      recoveredObserved = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::cout << "  hang child: stalled at " << stalledAtSeconds
            << "s recovered=" << recoveredObserved << "\n";
  const bool phaseOneOk =
      expect(!everStalledEarly, "no false positive while heartbeating") &&
      expect(stalledObserved, "silent-but-alive child flagged stalled") &&
      expect(stalledAtSeconds > 5.0 && stalledAtSeconds < 12.0,
             "stall flagged at the threshold, not before") &&
      expect(recoveredObserved,
             "a fresh (suspended-state) heartbeat clears the stall") &&
      expect(owner.running(), "child stayed alive throughout");
  owner.shutdown();
  if (!phaseOneOk) {
    return false;
  }

  // Child 2: starts then never heartbeats -- stalls from the spawn
  // baseline; the normal kill-replace path must work WHILE stalled.
  if (!expect(owner.launch(
                  scriptPlan("echo IGGY3DP1 session_started doc=0 rev=0 "
                             "room=hang_test2; sleep 600"),
                  reason),
              ("mute child spawns: " + reason).c_str())) {
    return false;
  }
  const std::uint64_t mutePid = owner.childPid();
  bool muteStalled = false;
  const Clock::time_point muteStart = Clock::now();
  while (secondsSince(muteStart) < 10.0) {
    const app::PlaytestProcessOwner::PollResult poll = owner.poll();
    if (poll.stalled) {
      muteStalled = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  const double muteStalledAt = secondsSince(muteStart);
  owner.stopRunning();  // Play-replaces path, exercised while stalled
  const bool replaced =
      owner.launch(scriptPlan("sleep 600"), reason) && owner.running();
  std::cout << "  mute child: pid=" << mutePid << " stalled at "
            << muteStalledAt << "s, replaced=" << replaced << "\n";
  const bool ok =
      expect(muteStalled, "never-heartbeating child stalls from spawn") &&
      expect(waitForPidGone(mutePid, 2.0), "stalled child reaped on replace") &&
      expect(replaced, "replacement launched while predecessor was stalled");
  owner.shutdown();
  return ok;
}

// ---- 5. the command channel: pause/resume, acks, EPIPE, stall guard ------

bool waitForAck(app::PlaytestProcessOwner& owner, std::uint64_t seq,
                double timeoutSeconds) {
  const Clock::time_point start = Clock::now();
  while (secondsSince(start) < timeoutSeconds) {
    static_cast<void>(owner.poll());
    if (owner.monitor().lastAckSeq == seq) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  return false;
}

bool commandChannelPauseResumeAcks(const std::string& i3dpPath,
                                   const std::string& saveRoot) {
  app::PlaytestProcessOwner owner;
  std::string reason;
  if (!expect(owner.launch(planFor(i3dpPath, saveRoot, "100000"), reason),
              ("channel child spawns: " + reason).c_str())) {
    return false;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(800));
  static_cast<void>(owner.poll());

  // pause -> ack applied, heartbeats flip to suspended, ticks freeze
  if (!expect(owner.sendPlaytestCommand("pause", {}, reason),
              ("pause sends: " + reason).c_str()) ||
      !expect(waitForAck(owner, 1U, 5.0), "pause ack arrives (seq 1)") ||
      !expect(owner.monitor().lastAckVerb == "pause" &&
                  owner.monitor().lastAckStatus == "applied",
              "pause acked applied")) {
    return false;
  }
  // observe two heartbeats while paused: state suspended, tick frozen
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  static_cast<void>(owner.poll());
  const std::uint64_t pausedTickA = owner.monitor().lastHeartbeatTick;
  const std::string pausedStateA = owner.monitor().lastHeartbeatState;
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  static_cast<void>(owner.poll());
  const std::uint64_t pausedTickB = owner.monitor().lastHeartbeatTick;
  std::cout << "  pause: state=" << pausedStateA << " tick " << pausedTickA
            << " -> " << pausedTickB << "\n";
  if (!expect(pausedStateA == "suspended",
              "paused heartbeats say state=suspended") ||
      !expect(pausedTickA == pausedTickB, "session ticks freeze while paused")) {
    return false;
  }

  // resume -> ack applied, ticks advance again
  if (!expect(owner.sendPlaytestCommand("resume", {}, reason),
              ("resume sends: " + reason).c_str()) ||
      !expect(waitForAck(owner, 2U, 5.0), "resume ack arrives (seq 2)") ||
      !expect(owner.monitor().lastAckStatus == "applied",
              "resume acked applied")) {
    return false;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(2500));
  static_cast<void>(owner.poll());
  const std::uint64_t resumedTick = owner.monitor().lastHeartbeatTick;
  std::cout << "  resume: state=" << owner.monitor().lastHeartbeatState
            << " tick " << resumedTick << "\n";
  if (!expect(owner.monitor().lastHeartbeatState == "running",
              "resumed heartbeats say state=running") ||
      !expect(resumedTick > pausedTickB, "session ticks advance after resume")) {
    return false;
  }

  // unknown verb -> acked unknown (wire-law), seq correlated
  if (!expect(owner.sendPlaytestCommand("teleport",
                                        {{"pos", "1,0,1"}}, reason),
              "unknown verb sends") ||
      !expect(waitForAck(owner, 3U, 5.0), "unknown ack arrives (seq 3)") ||
      !expect(owner.monitor().lastAckVerb == "teleport" &&
                  owner.monitor().lastAckStatus == "unknown",
              "unshipped verb acked status=unknown")) {
    return false;
  }

  // EPIPE leg: kill the child OUT FROM UNDER the owner, then write.
  const std::uint64_t pid = owner.childPid();
  static_cast<void>(kill(static_cast<pid_t>(pid), SIGKILL));
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  std::string epipeReason;
  const bool epipeSend =
      owner.sendPlaytestCommand("pause", {}, epipeReason);
  std::cout << "  epipe: send=" << epipeSend << " reason=" << epipeReason
            << " (still alive)\n";
  const bool epipeOk =
      expect(!epipeSend || !epipeReason.empty(),
             "write to a dead child fails gracefully or is flushed away") &&
      expect(true, "editor survived the dead-child write (no signal death)");
  // The exit is then observed normally.
  bool exitSeen = false;
  const Clock::time_point start = Clock::now();
  while (secondsSince(start) < 5.0) {
    if (owner.poll().exitObserved) {
      exitSeen = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  owner.shutdown();
  return epipeOk && expect(exitSeen, "killed child's exit observed after");
}

bool stalledChildSendRejected() {
  app::PlaytestProcessOwner owner;
  std::string reason;
  if (!expect(owner.launch(
                  scriptPlan("echo IGGY3DP1 session_started doc=0 rev=0 "
                             "room=stall_send; sleep 600"),
                  reason),
              "mute child spawns for stall-guard")) {
    return false;
  }
  const Clock::time_point start = Clock::now();
  bool stalled = false;
  while (secondsSince(start) < 10.0) {
    if (owner.poll().stalled) {
      stalled = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::string sendReason;
  const bool sent = owner.sendPlaytestCommand("pause", {}, sendReason);
  owner.shutdown();
  return expect(stalled, "child stalls") &&
         expect(!sent && sendReason == "playtest_command_child_stalled",
                "send to a stalled child is rejected without a write") &&
         expect(owner.monitor().lastCommandSeqSent == 0U,
                "nothing was queued to the stalled child");
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
                  replaceRunningChildThenShutdown(i3dpPath, saveRoot) &&
                  pipeFullKillReplaceDoesNotDeadlock(i3dpPath, saveRoot) &&
                  silentChildStallsRecoversAndReplaces(i3dpPath, saveRoot) &&
                  commandChannelPauseResumeAcks(i3dpPath, saveRoot) &&
                  stalledChildSendRejected();
  if (ok) {
    std::cout << "playtest_process_owner_tests passed\n";
  }
  return ok ? 0 : 1;
}
