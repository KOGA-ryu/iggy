// i3dp — the playtest process. Loads an explicit save snapshot, starts
// CreativePlaySession immediately, and runs the play loop until the window
// closes, ESC is pressed, or the session stops itself. No editor panels, no
// desktop shell, no starter-scene seeding, no authoring input, no ImGui.
// The loaded document is never mutated: the SAME &document is handed to
// session start and to every tick (the staleness contract holds trivially).

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

#include "EditorBootstrap.hpp"
#include "EditorControls.hpp"
#include "EditorFrame.hpp"
#include "EditorFrustumCull.hpp"
#include "EditorGamepad.hpp"
#include "EditorPersistence.hpp"
#include "EditorState.hpp"
#include "EditorWorldLayout.hpp"
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
               "[--offscreen]\n"
               "  --save-root  directory holding <save-id>.iggy3d.save "
               "(REQUIRED)\n"
               "  --load       save id to play (REQUIRED)\n"
               "  --frames     auto-exit after N frames (headless smoke)\n"
               "  --offscreen  SDL offscreen video driver (true headless: no "
               "display needed)\n");
  return 2;
}

}  // namespace

int main(int argc, char** argv) {
  std::filesystem::path playRoot;
  std::string loadSaveId;
  std::uint64_t maxFrames = 0U;
  bool offscreen = false;
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
  createInfo.width = 1280;
  createInfo.height = 720;
  createInfo.resizable = true;
  createInfo.highDpi = true;
  createInfo.vulkan = true;
  iggy3d::SdlWindow window(createInfo);
  if (!window.isOpen()) {
    SDL_Log("i3dp: failed to open window");
    return 1;
  }
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
    };
    writeProtocolEvent(started);
  }
  std::uint64_t lastHeartbeatBucket = 0U;
  // Runtime events accumulate in session transient state under a cursor
  // (the play loop's own consumption pattern) -- mirror only the new tail.
  std::size_t mirroredRuntimeEventCount = 0U;

  app::CreativeEditorGamepad gamepad;
  std::uint64_t frameIndex = 0U;
  std::string exitReason = "window_closed";

  while (window.isOpen()) {
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
    playTick.input.windowFocused = frameInput.windowFocused;
    playTick.input.actions = app::sampleCreativePlayActions(
        frameInput.inputFrame, frameInput.routedInput,
        editor.controlProfile.bindingSpan());
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
        app::PlaytestEvent mirror;
        mirror.kind = std::string(app::kPlaytestEventKindRuntimeEvent);
        mirror.fields = {{"kind", runtimeEventKindName(runtimeEvents[index].kind)}};
        writeProtocolEvent(mirror);
      }
      mirroredRuntimeEventCount = runtimeEvents.size();
      const std::uint64_t tickIndex = sessionState.clock.tickIndex;
      if (tickIndex / 60U > lastHeartbeatBucket) {
        lastHeartbeatBucket = tickIndex / 60U;
        app::PlaytestEvent heartbeat;
        heartbeat.kind = std::string(app::kPlaytestEventKindHeartbeat);
        heartbeat.fields = {{"tick", std::to_string(tickIndex)}};
        writeProtocolEvent(heartbeat);
      }
    }
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
