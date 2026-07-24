// i3dp — the playtest process. Loads an explicit save snapshot, starts
// CreativePlaySession immediately, and runs the play loop until the window
// closes, ESC is pressed, or the session stops itself. No editor panels, no
// desktop shell, no starter-scene seeding, no authoring input, no ImGui.
// The loaded document is never mutated: the SAME &document is handed to
// session start and to every tick (the staleness contract holds trivially).

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <thread>

#include <SDL3/SDL.h>

#include <fcntl.h>
#include <unistd.h>

#include "EditorBootstrap.hpp"
#include "EditorControls.hpp"
#include "EditorFrame.hpp"
#include "EditorFrustumCull.hpp"
#include "EditorGamepad.hpp"
#include "EditorPersistence.hpp"
#include "EditorState.hpp"
#include "EditorWorldLayoutLifecycle.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/play/PlaySession.hpp"
#include "app/iggy3d/creative/play/PlaytestEventProtocol.hpp"
#include "app/iggy3d/creative/render/CreativeSceneFrame.hpp"
#include "app/platform/SdlWindow.hpp"
#include "render/FrameInput.hpp"
#include "render/vulkan/VulkanBackend.hpp"
#include "runtime/save/SaveFileStore.hpp"

namespace {

namespace creative = iggy3d::creative;
namespace app = iggy3d_creative_app;

// stdout is the IGGY3DP1 protocol channel; ALL human chatter goes to stderr
// (SDL_Log already does). One flush per event keeps lines whole on the pipe.
void writeProtocolEvent(const app::PlaytestEvent& event) {
  const std::string line = app::formatPlaytestEventLine(event);
  std::fwrite(line.data(), 1U, line.size(), stdout);
  std::fputc('\n', stdout);
  std::fflush(stdout);
}

// IGGY3DC1 stdin drain: the mirror of the editor's read discipline --
// non-blocking, once per frame, partial lines carried across frames, no
// threads. The parent's SDL pipe leaves OUR read end blocking, so we flip
// stdin to O_NONBLOCK at boot (POSIX; this app targets Linux/macOS).
void makeStdinNonBlocking() {
  const int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  if (flags >= 0) {
    static_cast<void>(fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK));
  }
}

// Bounded per-frame drain; returns complete commands via the shared stream
// parser (unknown verbs and malformed lines are the CALLER's wire-law duty).
void drainStdinCommands(app::PlaytestEventStreamParser& parser,
                        std::vector<app::PlaytestEvent>& out) {
  std::array<char, 4096U> chunk{};
  std::size_t total = 0U;
  while (total < 64U * 1024U) {
    const ssize_t got = read(STDIN_FILENO, chunk.data(), chunk.size());
    if (got <= 0) {
      break;  // EAGAIN (nothing buffered) or EOF/error: stop this frame
    }
    static_cast<void>(
        parser.feed(std::string_view(chunk.data(),
                                     static_cast<std::size_t>(got)),
                    out));
    total += static_cast<std::size_t>(got);
  }
}

void writeCommandAck(const app::PlaytestEvent& command,
                     std::string_view status, std::string_view reason = {}) {
  app::PlaytestEvent ack;
  ack.kind = std::string(app::kPlaytestEventKindCommandAck);
  ack.fields = {{"seq", std::string(command.field("seq", "0"))},
                {"verb", command.kind},
                {"status", std::string(status)}};
  if (!reason.empty()) {
    ack.fields.emplace_back("reason", std::string(reason));
  }
  writeProtocolEvent(ack);
}

// Mirror of RuntimeEventKind as wire strings (kind STRING only on the wire).
[[nodiscard]] const char* runtimeEventKindName(iggy3d::RuntimeEventKind kind) {
  switch (kind) {
    case iggy3d::RuntimeEventKind::CommandAccepted: return "command_accepted";
    case iggy3d::RuntimeEventKind::CommandRejected: return "command_rejected";
    case iggy3d::RuntimeEventKind::Moved: return "moved";
    case iggy3d::RuntimeEventKind::Interacted: return "interacted";
    case iggy3d::RuntimeEventKind::CombatAttacked: return "combat_attacked";
    case iggy3d::RuntimeEventKind::CombatantDefeated:
      return "combatant_defeated";
    case iggy3d::RuntimeEventKind::AbilityCast: return "ability_cast";
    case iggy3d::RuntimeEventKind::AbilityImpacted: return "ability_impacted";
    case iggy3d::RuntimeEventKind::ItemAcquired: return "item_acquired";
    case iggy3d::RuntimeEventKind::ObjectiveCompleted:
      return "objective_completed";
    case iggy3d::RuntimeEventKind::ClockChanged: return "clock_changed";
    case iggy3d::RuntimeEventKind::CameraChanged: return "camera_changed";
    case iggy3d::RuntimeEventKind::SaveCreated: return "save_created";
    case iggy3d::RuntimeEventKind::LoadCompleted: return "load_completed";
    case iggy3d::RuntimeEventKind::ResetCompleted: return "reset_completed";
    case iggy3d::RuntimeEventKind::ReplayCompleted: return "replay_completed";
    case iggy3d::RuntimeEventKind::RuntimeFailed: return "runtime_failed";
  }
  return "unknown_runtime_event";
}

int usage() {
  std::fprintf(stderr,
               "usage: i3dp --save-root <dir> --load <save-id> [--frames N] "
               "[--offscreen] [--fullscreen] [--resolution WxH]\n"
               "  --save-root  directory holding <save-id>.iggy3d.save "
               "(REQUIRED)\n"
               "  --load       save id to play (REQUIRED)\n"
               "  --frames     auto-exit after N frames (headless smoke)\n"
               "  --offscreen  SDL offscreen video driver (true headless: no "
               "display needed)\n"
               "  --fullscreen borderless desktop fullscreen (wins over "
               "--resolution)\n"
               "  --resolution WxH windowed size, 640x360..16384x16384\n");
  return 2;
}

}  // namespace

int main(int argc, char** argv) {
  std::filesystem::path playRoot;
  std::string loadSaveId;
  std::uint64_t maxFrames = 0U;
  bool offscreen = false;
  bool fullscreenRequested = false;
  app::PlaytestResolution resolution;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--save-root" && i + 1 < argc) {
      playRoot = argv[++i];
    } else if (arg == "--load" && i + 1 < argc) {
      loadSaveId = argv[++i];
    } else if (arg == "--frames" && i + 1 < argc) {
      maxFrames = std::strtoull(argv[++i], nullptr, 10);
    } else if (arg == "--offscreen") {
      offscreen = true;
    } else if (arg == "--fullscreen") {
      fullscreenRequested = true;
    } else if (arg == "--resolution" && i + 1 < argc) {
      resolution = app::parsePlaytestResolution(argv[++i]);
      if (!resolution.valid) {
        std::fprintf(stderr, "i3dp: invalid --resolution (WxH, %ux%u..%ux%u)\n",
                     app::kPlaytestMinWindowWidth,
                     app::kPlaytestMinWindowHeight,
                     app::kPlaytestMaxWindowDimension,
                     app::kPlaytestMaxWindowDimension);
        return usage();
      }
    } else {
      std::fprintf(stderr, "i3dp: unknown argument '%s'\n", arg.c_str());
      return usage();
    }
  }
  if (playRoot.empty() || loadSaveId.empty()) {
    return usage();
  }
  // Refuse a missing/invalid save BEFORE any window or renderer work: a bad
  // launch must not flash a window (or, headless, touch the GPU) at all.
  const std::filesystem::path savePath =
      iggy3d::saveFilePathForId(playRoot, loadSaveId);
  std::error_code saveError;
  if (!std::filesystem::is_regular_file(savePath, saveError) || saveError) {
    std::fprintf(stderr, "i3dp: save not found: '%s' (save-root='%s' id='%s')\n",
                 savePath.generic_string().c_str(),
                 playRoot.generic_string().c_str(), loadSaveId.c_str());
    return 1;
  }
  if (offscreen) {
    // First-class headless boot: the same SDL offscreen video driver the
    // i3dc capture path runs under, selected here in-process so the smoke
    // does not depend on the caller's environment.
    SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "offscreen",
                            SDL_HINT_OVERRIDE);
  }

  iggy3d::SdlWindowCreateInfo createInfo;
  createInfo.title = "iggy3d playtest";
  // Decided precedence: fullscreen wins; a resolution then only sets the
  // windowed size the fullscreen would restore to.
  createInfo.width = resolution.valid ? static_cast<int>(resolution.width)
                                      : 1280;
  createInfo.height = resolution.valid ? static_cast<int>(resolution.height)
                                       : 720;
  createInfo.resizable = true;
  createInfo.highDpi = true;
  createInfo.vulkan = true;
  iggy3d::SdlWindow window(createInfo);
  if (!window.isOpen()) {
    SDL_Log("i3dp: failed to open window");
    return 1;
  }
  // FOCUS HANDOFF: one polite raise request at boot, immediately after the
  // window is shown -- the editor deliberately holds nothing that fights
  // it. If the OS denies (Wayland focus-stealing prevention), the child
  // starts suspended and the heartbeat state already tells that story.
  makeStdinNonBlocking();
  bool bootFullscreen = false;
  if (fullscreenRequested) {
    // Borderless desktop only; the offscreen driver ignores the request and
    // this reports the ACTUAL resulting state.
    bootFullscreen = window.applyBorderlessFullscreen();
  }
  const bool bootFocused = window.requestRaiseAndFocus();
  window.setRelativeMouseMode(true);

  std::unique_ptr<iggy3d::VulkanBackend> backend =
      app::createCreativeRenderer(window, /*enableExternalUi=*/false);
  if (backend == nullptr ||
      backend->lifecycleState() != iggy3d::RendererLifecycleState::Ready) {
    SDL_Log("i3dp: renderer not ready (lifecycle=%d)",
            backend == nullptr ? -1
                               : static_cast<int>(backend->lifecycleState()));
    return 1;
  }

  // Bootstrap WITHOUT the starter Floor/Crate scene; the snapshot is the
  // only content this process ever holds.
  app::CreativeEditorBootstrapData bootstrapData;
  app::initializeCreativeEditorBootstrapData(bootstrapData,
                                             /*captureMode=*/false,
                                             /*seedStarterScene=*/false);
  app::CreativeEditorState& editor = bootstrapData.editor;
  creative::CreativeAppState& appState = bootstrapData.appState;

  creative::CreativeWorldLayout loadedLayout;
  if (!app::loadStandaloneScene(appState, playRoot, loadSaveId,
                                &loadedLayout)) {
    std::fprintf(stderr, "i3dp: failed to load save-root='%s' id='%s'\n",
                 playRoot.generic_string().c_str(), loadSaveId.c_str());
    return 1;
  }
  app::installCreativeEditorWorldLayout(editor.worldLayout,
                                        std::move(loadedLayout));

  // Player controls come from the standard editor profile location so the
  // playtest honors the same bindings the author tuned in i3dc.
  const std::filesystem::path controlsPath =
      bootstrapData.saveRoot / "creative_controls_v1.cfg";
  static_cast<void>(app::loadCreativeEditorControlProfile(
      editor.controlProfile, controlsPath));

  app::CreativePlaySession playSession;
  {
    app::CreativePlayStartRequest startRequest;
    startRequest.document = &appState.facade.document();
    startRequest.staticMeshAssetCatalog = &bootstrapData.staticMeshAssetCatalog;
    const app::CreativePlayStartReceipt started =
        app::startCreativePlaySession(playSession, std::move(startRequest));
    if (!started.accepted) {
      std::fprintf(stderr, "i3dp: play start refused: %s\n",
                   started.reasonCode.c_str());
      return 1;
    }
  }
  SDL_Log("i3dp: playing save-root='%s' id='%s'",
          playRoot.generic_string().c_str(), loadSaveId.c_str());
  {
    app::PlaytestEvent started;
    started.kind = std::string(app::kPlaytestEventKindSessionStarted);
    started.fields = {
        {"doc", std::to_string(appState.facade.document().id())},
        {"rev", std::to_string(appState.facade.document().revision())},
        {"room", "creative_editor_play"},
        {"focused", bootFocused ? "1" : "0"},
        {"window",
         std::to_string(window.drawableExtent().width) + "x" +
             std::to_string(window.drawableExtent().height)},
        {"fullscreen", bootFullscreen ? "1" : "0"},
    };
    writeProtocolEvent(started);
  }
  // WALL-CLOCK liveness heartbeat (~1s of real time, regardless of sim
  // suspension): heartbeat means "process responsive", not "sim advancing".
  // The editor-stays-open workflow keeps this window unfocused (suspended)
  // most of the time -- a suspended child must still prove it is alive.
  constexpr std::uint64_t kHeartbeatIntervalMs = 1000U;
  std::uint64_t lastHeartbeatAtMs = SDL_GetTicks();
  const auto emitWallClockHeartbeat = [&](bool simRunning) {
    const std::uint64_t nowMs = SDL_GetTicks();
    if (nowMs - lastHeartbeatAtMs < kHeartbeatIntervalMs) {
      return;
    }
    lastHeartbeatAtMs = nowMs;
    app::PlaytestEvent heartbeat;
    heartbeat.kind = std::string(app::kPlaytestEventKindHeartbeat);
    const std::uint64_t tickIndex =
        playSession.sandbox.has_value()
            ? playSession.sandbox->session.state().clock.tickIndex
            : 0U;
    heartbeat.fields = {{"tick", std::to_string(tickIndex)},
                       {"state", simRunning ? "running" : "suspended"}};
    writeProtocolEvent(heartbeat);
  };
  // Runtime events accumulate in session transient state under a cursor
  // (the play loop's own consumption pattern) -- mirror only the new tail.
  std::size_t mirroredRuntimeEventCount = 0U;

  app::CreativeEditorGamepad gamepad;
  std::uint64_t frameIndex = 0U;
  std::string exitReason = "window_closed";
  // USER-PAUSE (command channel): rides the existing suspend machinery by
  // masking the focus input to the tick -- the sim suspends exactly as it
  // does when unfocused, heartbeats say state=suspended, and focus gain
  // does NOT auto-resume: only the `resume` verb clears it. A paused child
  // still drains stdin, heartbeats, and honors close/ESC below.
  bool userPaused = false;
  app::PlaytestEventStreamParser commandParser{app::kPlaytestCommandMagic};
  std::vector<app::PlaytestEvent> pendingCommands;

  while (window.isOpen()) {
    pendingCommands.clear();
    drainStdinCommands(commandParser, pendingCommands);
    for (const app::PlaytestEvent& command : pendingCommands) {
      if (command.kind == app::kPlaytestCommandVerbPause) {
        userPaused = true;
        writeCommandAck(command, "applied");
      } else if (command.kind == app::kPlaytestCommandVerbResume) {
        userPaused = false;
        writeCommandAck(command, "applied");
      } else {
        // Wire-law: unknown verbs are counted (parser) and acked unknown.
        writeCommandAck(command, "unknown");
      }
    }
    app::CreativeEditorFrameInputResult frameInput =
        app::beginCreativeEditorFrameInput(window, *backend, gamepad, editor,
                                           /*captureMode=*/false,
                                           /*applyEditorNavigation=*/false);
    if (!frameInput.keepRunning) {
      break;
    }
    if (frameInput.inputFrame.keysDown[static_cast<std::size_t>(
            creative::CreativeInputKey::Escape)]) {
      exitReason = "escape_pressed";
      break;
    }
    const iggy3d::SdlDrawableExtent extent = frameInput.extent;
    if (frameInput.skipFrame) {
      app::CreativePlayTickRequest playTick;
      playTick.sourceDocument = &appState.facade.document();
      playTick.input.windowFocused = false;
      playTick.monotonicTimeNanoseconds = frameInput.monotonicTimeNanoseconds;
      static_cast<void>(app::tickCreativePlaySession(playSession, playTick));
      emitWallClockHeartbeat(/*simRunning=*/false);
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
      continue;
    }

    app::CreativePlayTickRequest playTick;
    playTick.sourceDocument = &appState.facade.document();
    playTick.input.moveRight = frameInput.navigationMoveRight;
    playTick.input.moveForward = frameInput.navigationMoveForward;
    playTick.input.yawDeltaDegrees = frameInput.navigationYawDeltaDegrees;
    playTick.input.pitchDeltaDegrees = frameInput.navigationPitchDeltaDegrees;
    playTick.input.sprinting = frameInput.navigationSprinting;
    playTick.input.windowFocused = frameInput.windowFocused && !userPaused;
    playTick.input.actions =
        app::sampleCreativePlayActions(frameInput.routedInput);
    playTick.monotonicTimeNanoseconds = frameInput.monotonicTimeNanoseconds;
    const app::CreativePlayTickReceipt tickReceipt =
        app::tickCreativePlaySession(playSession, playTick);
    if (tickReceipt.ticksAdvanced > 0U && playSession.sandbox.has_value()) {
      // Mirror the runtime events the play loop already consumed this tick
      // (kind string only) and heartbeat every 60 session ticks.
      const iggy3d::SessionState& sessionState =
          playSession.sandbox->session.state();
      const auto& runtimeEvents = sessionState.transient.events;
      if (mirroredRuntimeEventCount > runtimeEvents.size()) {
        mirroredRuntimeEventCount = 0U;  // transient log was reset
      }
      for (std::size_t index = mirroredRuntimeEventCount;
           index < runtimeEvents.size(); ++index) {
        const iggy3d::RuntimeEvent& source = runtimeEvents[index];
        app::PlaytestEvent mirror;
        mirror.kind = std::string(app::kPlaytestEventKindRuntimeEvent);
        // APPEND-ONLY payload: the fields the event struct already carries,
        // named after the struct fields, raw ids, sentinels omitted. No new
        // session probing.
        mirror.fields = {{"kind", runtimeEventKindName(source.kind)}};
        if (source.tick != iggy3d::kInvalidCommandTick) {
          mirror.fields.emplace_back("tick", std::to_string(source.tick));
        }
        if (source.commandId != iggy3d::kInvalidCommandId) {
          mirror.fields.emplace_back("commandId",
                                     std::to_string(source.commandId));
        }
        if (source.sequence != iggy3d::kInvalidCommandSequence) {
          mirror.fields.emplace_back("sequence",
                                     std::to_string(source.sequence));
        }
        if (source.playerSlot != iggy3d::kInvalidPlayerSlotId) {
          mirror.fields.emplace_back("playerSlot",
                                     std::to_string(source.playerSlot));
        }
        if (source.actor.value != 0U) {
          mirror.fields.emplace_back("actor",
                                     std::to_string(source.actor.value));
        }
        if (source.target.value != 0U) {
          mirror.fields.emplace_back("target",
                                     std::to_string(source.target.value));
        }
        if (source.rejection != iggy3d::CommandRejectionReason::None) {
          mirror.fields.emplace_back(
              "rejection", std::to_string(static_cast<int>(source.rejection)));
        }
        if (!source.stableId.empty()) {
          mirror.fields.emplace_back("stableId", source.stableId);
        }
        writeProtocolEvent(mirror);
      }
      mirroredRuntimeEventCount = runtimeEvents.size();
    }
    emitWallClockHeartbeat(
        /*simRunning=*/tickReceipt.status !=
        app::CreativePlayTickStatus::Suspended);
    if (!app::creativePlaySessionActive(playSession)) {
      exitReason = tickReceipt.reasonCode.empty() ? "session_stopped"
                                                  : tickReceipt.reasonCode;
      break;
    }

    app::CreativePlayScene playScene = app::buildCreativePlaySessionScene(
        playSession, creative::kInvalidObjectId);
    if (!playScene.available) {
      exitReason = "play_scene_unavailable";
      break;
    }
    iggy3d::DebugProjectionResult debug{};
    iggy3d::FrameInput frame = iggy3d::makeCreativeVulkanFrame(
        playScene.scene, debug, frameIndex++, extent.width, extent.height,
        playSession.cameraYawDegrees, playSession.cameraPitchDegrees,
        /*cameraAnchorOverrideAvailable=*/true, playScene.cameraAnchorMeters,
        editor.desktopUi.contentViewport);
    const app::CreativePlayHudFrame playHud =
        app::buildCreativePlayHud(playSession, frame);
    app::attachCreativePlayHud(playHud, frame);
    frame.creativeWireframeDebug.available = true;
    frame.creativeWireframeDebug.visible =
        !playScene.logicOverlay.lines.empty();
    frame.creativeWireframeDebug.lines = playScene.logicOverlay.lines.data();
    frame.creativeWireframeDebug.lineCount =
        playScene.logicOverlay.lines.size();
    const app::StandaloneFrustumCullResult frustumCull =
        app::cullStandaloneSceneRoomMeshesByFrustum(
            playScene.scene, frame.camera.clipFromWorld);
    frame.projections.scene = &frustumCull.scene;
    static_cast<void>(backend->submitFrame(frame));
    if (maxFrames != 0U && frameIndex >= maxFrames) {
      exitReason = "frame_limit_reached";
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  static_cast<void>(app::stopCreativePlaySession(playSession));
  {
    app::PlaytestEvent ended;
    ended.kind = std::string(app::kPlaytestEventKindSessionEnded);
    ended.fields = {{"reason", exitReason},
                    {"frames", std::to_string(frameIndex)}};
    writeProtocolEvent(ended);
  }
  std::fprintf(stderr, "i3dp: exit reason='%s' frames=%llu\n",
               exitReason.c_str(),
               static_cast<unsigned long long>(frameIndex));
  return 0;
}
