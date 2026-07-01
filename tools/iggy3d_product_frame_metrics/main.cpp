#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/automation/Automation.hpp"
#include "app/iggy3d/automation/AutomationDispatch.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/Controller.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "app/iggy3d/gameplay/ProjectionRefresh.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/menu/Transitions.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/iggy3d/world/BuiltinDungeon.hpp"
#include "app/input/ActionState.hpp"
#include "content/assets/RoomAsset.hpp"
#include "runtime/session/Session.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

enum class ScenarioKind {
  DefaultGameplay,
  MovementWallRunCorridor,
};

enum class ScenarioFilter {
  All,
  DefaultGameplay,
  MovementWallRunCorridor,
};

enum class DebugOverlayMode {
  Off,
  On,
  Both,
};

struct CliConfig {
  ScenarioFilter scenario = ScenarioFilter::All;
  DebugOverlayMode debugOverlay = DebugOverlayMode::Off;
  std::uint32_t frames = 120U;
  bool noTiming = false;
  bool help = false;
  std::filesystem::path outputPath;
};

struct ScenarioSpec {
  ScenarioKind kind = ScenarioKind::DefaultGameplay;
  std::string_view name = "default_gameplay";
  std::string_view roomId = "none";
  bool debugOverlay = false;
};

struct TimingTotals {
  std::uint64_t setupNs = 0;
  std::uint64_t inputOrAutomationNs = 0;
  std::uint64_t tickOrGameplayUpdateNs = 0;
  std::uint64_t projectionOrFrameBuildNs = 0;
  std::uint64_t receiptOrMetricsBuildNs = 0;
  std::uint64_t totalNs = 0;
};

struct ScenarioCounters {
  std::uint64_t sceneItemMax = 0;
  std::uint64_t debugItemMax = 0;
  std::uint64_t drawItemMax = 0;
  std::uint64_t primitiveCountMax = 0;
  std::uint64_t triangleCountMax = 0;
  std::uint64_t renderBridgeItemMax = 0;
  std::uint64_t debugHudLineMax = 0;
  std::uint64_t movementDebugHudLineMax = 0;
  std::uint64_t physicsDebugHudLineMax = 0;
  std::uint64_t npcDebugHudLineMax = 0;
  std::uint64_t collisionSurfaceMax = 0;
  std::uint64_t physicsMovementSurfaceMax = 0;
  std::uint64_t physicsMovementSweepMax = 0;
  std::uint64_t wallRunCandidateFrameCount = 0;
  std::uint64_t wallRunActiveFrameCount = 0;
  std::uint64_t wallRunningStateFrameCount = 0;
  std::string lastMovementState = "none";
  std::string lastWallRunStatus = "none";
  float lastHorizontalSpeedMetersPerSecond = 0.0F;
  float lastVerticalVelocityMetersPerSecond = 0.0F;
};

struct ScenarioResult {
  ScenarioSpec spec;
  bool ok = false;
  std::string status = "not_run";
  std::string reasonCode = "not_run";
  std::uint32_t frames = 0;
  std::string activeSurface = "none";
  std::string inputOwner = "none";
  bool gameplayInputSuppressed = true;
  std::string roomId = "none";
  std::uint64_t roomFloorCount = 0;
  std::uint64_t roomWallCount = 0;
  std::uint64_t roomSurfaceCount = 0;
  std::uint64_t roomCollisionSurfaceCount = 0;
  TimingTotals timings;
  ScenarioCounters counters;
};

struct ScenarioRuntime {
  iggy3d::FrontendState frontend;
  iggy3d::ProductAppOptions options;
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::FrontendSettings settings = iggy3d::defaultFrontendSettings();
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::WorldSetupDraft worldSetupDraft =
      iggy3d::makeProductDefaultWorldSetupDraft();
  iggy3d::ProductAppWindowState window;
  bool closeRequested = false;
};

void printUsage(std::ostream& out) {
  out << "usage: iggy3d_product_frame_metrics [options]\n"
      << "\n"
      << "options:\n"
      << "  --scenario <all|default_gameplay|movement_wall_run_corridor>\n"
      << "  --frames <positive_uint>\n"
      << "  --debug-overlay <off|on|both>\n"
      << "  --no-timing\n"
      << "  --output <path>\n"
      << "  --help\n";
}

bool needsValue(int argc, const char* const* argv, int index) {
  return index + 1 >= argc ||
         std::string_view(argv[index + 1]).starts_with("--");
}

bool parsePositiveUint(std::string_view text, std::uint32_t& value) {
  std::uint32_t parsed = 0U;
  const char* begin = text.data();
  const char* end = text.data() + text.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc{} || result.ptr != end || parsed == 0U) {
    return false;
  }
  value = parsed;
  return true;
}

bool parseScenario(std::string_view text, ScenarioFilter& scenario) {
  if (text == "all") {
    scenario = ScenarioFilter::All;
    return true;
  }
  if (text == "default_gameplay") {
    scenario = ScenarioFilter::DefaultGameplay;
    return true;
  }
  if (text == "movement_wall_run_corridor") {
    scenario = ScenarioFilter::MovementWallRunCorridor;
    return true;
  }
  return false;
}

bool parseDebugOverlay(std::string_view text, DebugOverlayMode& mode) {
  if (text == "off") {
    mode = DebugOverlayMode::Off;
    return true;
  }
  if (text == "on") {
    mode = DebugOverlayMode::On;
    return true;
  }
  if (text == "both") {
    mode = DebugOverlayMode::Both;
    return true;
  }
  return false;
}

bool parseCli(int argc, const char* const* argv, CliConfig& config) {
  for (int index = 1; index < argc; ++index) {
    const std::string_view arg{argv[index]};
    if (arg == "--help") {
      config.help = true;
      return true;
    }
    if (arg == "--no-timing") {
      config.noTiming = true;
    } else if (arg == "--scenario") {
      if (needsValue(argc, argv, index) ||
          !parseScenario(argv[++index], config.scenario)) {
        return false;
      }
    } else if (arg == "--frames") {
      if (needsValue(argc, argv, index) ||
          !parsePositiveUint(argv[++index], config.frames)) {
        return false;
      }
    } else if (arg == "--debug-overlay") {
      if (needsValue(argc, argv, index) ||
          !parseDebugOverlay(argv[++index], config.debugOverlay)) {
        return false;
      }
    } else if (arg == "--output") {
      if (needsValue(argc, argv, index)) {
        return false;
      }
      config.outputPath = argv[++index];
    } else {
      return false;
    }
  }
  return true;
}

std::string jsonEscape(std::string_view text) {
  std::string escaped;
  escaped.reserve(text.size() + 8U);
  for (const char character : text) {
    switch (character) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped.push_back(character);
        break;
    }
  }
  return escaped;
}

void writeJsonString(std::ostream& out, std::string_view value) {
  out << '"' << jsonEscape(value) << '"';
}

std::uint64_t elapsedNs(Clock::time_point begin,
                        Clock::time_point end,
                        bool noTiming) {
  if (noTiming) {
    return 0U;
  }
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count());
}

iggy3d::ActionState makeDefaultActionState() {
  iggy3d::ActionState actions;
  iggy3d::recordAction(actions, iggy3d::InputAction::PlayerMoveY, true, false,
                       false, 1.0F);
  return actions;
}

iggy3d::ActionState makeWallRunActionState(std::uint32_t frameIndex) {
  iggy3d::ActionState actions;
  if (frameIndex == 0U) {
    iggy3d::recordAction(actions, iggy3d::InputAction::PlayerJump, true, true,
                         false, 1.0F);
  } else {
    iggy3d::recordAction(actions, iggy3d::InputAction::PlayerJump, true, false,
                         false, 1.0F);
    iggy3d::recordAction(actions, iggy3d::InputAction::PlayerMoveX, true, false,
                         false, 1.0F);
  }
  return actions;
}

std::string_view roomIdForScenario(ScenarioKind kind) {
  if (kind == ScenarioKind::MovementWallRunCorridor) {
    return "movement_wall_run_corridor";
  }
  return iggy3d::productDefaultBuiltinDungeon().roomId;
}

const iggy3d::ProductBuiltinDungeonDefinition* dungeonForScenario(
    ScenarioKind kind) {
  if (kind == ScenarioKind::MovementWallRunCorridor) {
    return iggy3d::findProductBuiltinDungeonByRoomId(
        "movement_wall_run_corridor");
  }
  return &iggy3d::productDefaultBuiltinDungeon();
}

bool applyAutomationCommand(ScenarioRuntime& runtime,
                            std::string key,
                            std::string value) {
  return iggy3d::applyProductAutomationAppCommand(
      iggy3d::ProductAutomationCommand{std::move(key), std::move(value)},
      iggy3d::ProductAutomationAppContext{runtime.frontend,
                                          runtime.saves,
                                          runtime.options,
                                          runtime.settings,
                                          runtime.settingsTab,
                                          runtime.activeSession,
                                          runtime.worldSetupDraft,
                                          runtime.window,
                                          runtime.closeRequested});
}

bool initializeScenarioRuntime(const ScenarioSpec& spec,
                               ScenarioRuntime& runtime,
                               std::string& reasonCode) {
  runtime.options = iggy3d::defaultProductAppOptions();
  runtime.options.windowMode = iggy3d::ProductWindowMode::NoWindow;
  runtime.options.renderer = iggy3d::ProductRendererRequest::Vulkan;
  runtime.options.saveRoot =
      std::filesystem::path{"/tmp"} /
      ("iggy3d_product_frame_metrics_" + std::string(spec.name) +
       (spec.debugOverlay ? "_debug_on" : "_debug_off"));
  std::filesystem::remove_all(runtime.options.saveRoot);
  runtime.saves.saveRoot = runtime.options.saveRoot;
  runtime.settings = iggy3d::defaultFrontendSettings();
  runtime.settings.devToolsEnabled = true;
  runtime.settings.debugOverlayEnabled = spec.debugOverlay;
  runtime.worldSetupDraft = iggy3d::makeProductDefaultWorldSetupDraft();
  iggy3d::recordWorldSetupDraftState(runtime.worldSetupDraft, runtime.window);
  iggy3d::initializeProductStarterTransition(runtime.frontend, runtime.window, false);

  const auto fail = [&](std::string_view reason) {
    reasonCode = std::string(reason);
    return false;
  };
  if (!applyAutomationCommand(runtime, "frontend.select", "new_world")) {
    return fail("frontend_select_failed");
  }
  if (!applyAutomationCommand(runtime, "frontend.execute", "true")) {
    return fail("frontend_execute_failed");
  }
  if (!applyAutomationCommand(runtime, "world.dungeon_id",
                              std::string(roomIdForScenario(spec.kind)))) {
    return fail("world_dungeon_select_failed");
  }
  if (!applyAutomationCommand(runtime, "world.create", "true")) {
    return fail("world_create_failed");
  }
  if (spec.kind == ScenarioKind::MovementWallRunCorridor) {
    if (!applyAutomationCommand(runtime, "gameplay.player_position",
                                "0,0,9.65")) {
      return fail("wall_run_reposition_failed");
    }
    if (!applyAutomationCommand(runtime, "gameplay.jump", "true")) {
      return fail("wall_run_jump_failed");
    }
    if (!applyAutomationCommand(runtime, "game.move_x", "1") &&
        !runtime.window.gameplayWallRunActive) {
      return fail("wall_run_move_failed");
    }
  }
  return true;
}

void collectCounters(const iggy3d::ProductAppWindowState& window,
                     const iggy3d::ProductGameplayProjectionFrame& projection,
                     ScenarioCounters& counters) {
  counters.sceneItemMax = std::max(counters.sceneItemMax, window.sceneItemCount);
  counters.debugItemMax = std::max(counters.debugItemMax, window.debugItemCount);
  counters.drawItemMax =
      std::max(counters.drawItemMax, window.viewport.productDrawItemCount);
  counters.primitiveCountMax =
      std::max(counters.primitiveCountMax, window.viewport.productDrawItemCount);
  counters.triangleCountMax =
      std::max(counters.triangleCountMax,
               window.viewport.productVulkanRoomIndexCount / 3U);
  counters.renderBridgeItemMax =
      std::max(counters.renderBridgeItemMax,
               window.viewport.productViewFrameItemCount);
  const std::uint64_t debugHudLines =
      projection.movementHud.lines.size() + window.npcBehaviorDebugHudLineCount +
      window.physicsDebugHud.lineCount + window.positionHud.lineCount;
  counters.debugHudLineMax =
      std::max(counters.debugHudLineMax, debugHudLines);
  counters.movementDebugHudLineMax =
      std::max(counters.movementDebugHudLineMax,
               static_cast<std::uint64_t>(projection.movementHud.lines.size()));
  counters.physicsDebugHudLineMax =
      std::max(counters.physicsDebugHudLineMax,
               static_cast<std::uint64_t>(window.physicsDebugHud.lineCount));
  counters.npcDebugHudLineMax =
      std::max(counters.npcDebugHudLineMax,
               window.npcBehaviorDebugHudLineCount);
  counters.collisionSurfaceMax =
      std::max(counters.collisionSurfaceMax,
               window.activeRoomCollision.querySurfaceCount);
  counters.physicsMovementSurfaceMax =
      std::max(counters.physicsMovementSurfaceMax,
               window.gameplayCollisionSurfaceCount);
  counters.physicsMovementSweepMax =
      std::max(counters.physicsMovementSweepMax,
               window.gameplayMovementCollisionSweepCount);
  if (window.gameplayWallRunCandidateAvailable) {
    ++counters.wallRunCandidateFrameCount;
  }
  if (window.gameplayWallRunActive) {
    ++counters.wallRunActiveFrameCount;
  }
  if (window.gameplayMovementState ==
      iggy3d::ProductGameplayMovementState::WallRunning) {
    ++counters.wallRunningStateFrameCount;
  }
  counters.lastMovementState =
      std::string(iggy3d::productGameplayMovementStateName(
          window.gameplayMovementState));
  counters.lastWallRunStatus = window.gameplayWallRunStatus;
  counters.lastHorizontalSpeedMetersPerSecond =
      window.gameplayMovementHorizontalSpeedMetersPerSecond;
  counters.lastVerticalVelocityMetersPerSecond =
      window.gameplayJumpVelocityMetersPerSecond;
}

ScenarioResult runScenario(const ScenarioSpec& spec,
                           std::uint32_t frames,
                           bool noTiming) {
  ScenarioResult result;
  result.spec = spec;
  result.frames = frames;
  const Clock::time_point totalBegin = Clock::now();
  const Clock::time_point setupBegin = Clock::now();

  if (dungeonForScenario(spec.kind) == nullptr) {
    result.status = "scenario_setup_failed";
    result.reasonCode = "missing_builtin_dungeon";
    return result;
  }

  ScenarioRuntime runtime;
  std::string setupReason = "scenario_setup_failed";
  if (!initializeScenarioRuntime(spec, runtime, setupReason) ||
      !runtime.activeSession.has_value()) {
    result.status = "scenario_setup_failed";
    result.reasonCode = setupReason;
    return result;
  }
  iggy3d::syncProductWindowInputOwnerFromActiveSurface(runtime.frontend,
                                                       runtime.window);
  result.timings.setupNs = elapsedNs(setupBegin, Clock::now(), noTiming);

  for (std::uint32_t frame = 0; frame < frames; ++frame) {
    const Clock::time_point inputBegin = Clock::now();
    iggy3d::ActionState actions =
        spec.kind == ScenarioKind::MovementWallRunCorridor
            ? makeWallRunActionState(frame)
            : makeDefaultActionState();
    result.timings.inputOrAutomationNs +=
        elapsedNs(inputBegin, Clock::now(), noTiming);

    const Clock::time_point updateBegin = Clock::now();
    iggy3d::applyProductGameplayActions(
        *runtime.activeSession, actions, runtime.window, "product_frame_metrics",
        iggy3d::productActiveRoomCollisionSurfaces(
            runtime.window.activeRoomCollision));
    result.timings.tickOrGameplayUpdateNs +=
        elapsedNs(updateBegin, Clock::now(), noTiming);

    const Clock::time_point projectionBegin = Clock::now();
    const iggy3d::ProductGameplayProjectionFrame projection =
        iggy3d::buildProductGameplayProjectionFrame(
            iggy3d::ProductGameplayProjectionFrameRequest{
                runtime.activeSession,
                runtime.window,
                true,
                spec.debugOverlay,
                iggy3d::ProductRendererRequest::Vulkan,
                runtime.frontend});
    iggy3d::applyGameplayProjectionMetrics(
        runtime.window, projection.scenePtr(), projection.debugPtr(),
        projection.drawListPtr(), projection.viewportFramePtr(),
        projection.renderBridgePtr(), projection.viewVisible);
    result.timings.projectionOrFrameBuildNs +=
        elapsedNs(projectionBegin, Clock::now(), noTiming);

    const Clock::time_point metricsBegin = Clock::now();
    collectCounters(runtime.window, projection, result.counters);
    result.timings.receiptOrMetricsBuildNs +=
        elapsedNs(metricsBegin, Clock::now(), noTiming);
  }

  const iggy3d::ProductActiveSurfaceFrame activeSurface =
      iggy3d::resolveProductActiveSurface(
          iggy3d::productActiveSurfaceContextForWindow(runtime.frontend,
                                                       runtime.window));
  result.activeSurface =
      std::string(iggy3d::productFrontendSurfaceName(activeSurface.activeSurface));
  result.inputOwner = std::string(iggy3d::menuOwnerName(activeSurface.inputOwner));
  result.gameplayInputSuppressed = activeSurface.gameplayInputSuppressed;
  result.roomId = runtime.window.activeRoom.roomId;
  result.roomFloorCount = runtime.window.viewport.productDrawFloorTileCount;
  result.roomWallCount = runtime.window.viewport.productDrawWallTileCount;
  result.roomSurfaceCount = runtime.window.activeRoom.spatialSurfaceCount;
  result.roomCollisionSurfaceCount =
      runtime.window.activeRoomCollision.querySurfaceCount;
  result.timings.totalNs = elapsedNs(totalBegin, Clock::now(), noTiming);
  result.ok = true;
  result.status = "product_frame_metrics_ready";
  result.reasonCode = "product_frame_metrics_ready";
  return result;
}

std::vector<ScenarioSpec> makeScenarioSpecs(const CliConfig& config) {
  std::vector<ScenarioSpec> specs;
  const auto addDebugModes = [&](ScenarioKind kind,
                                 std::string_view name,
                                 std::string_view roomId) {
    if (config.debugOverlay == DebugOverlayMode::On ||
        config.debugOverlay == DebugOverlayMode::Both) {
      specs.push_back(ScenarioSpec{kind, name, roomId, true});
    }
    if (config.debugOverlay == DebugOverlayMode::Off ||
        config.debugOverlay == DebugOverlayMode::Both) {
      specs.push_back(ScenarioSpec{kind, name, roomId, false});
    }
  };
  if (config.scenario == ScenarioFilter::All ||
      config.scenario == ScenarioFilter::DefaultGameplay) {
    addDebugModes(ScenarioKind::DefaultGameplay,
                  "default_gameplay",
                  iggy3d::productDefaultBuiltinDungeon().roomId);
  }
  if (config.scenario == ScenarioFilter::All ||
      config.scenario == ScenarioFilter::MovementWallRunCorridor) {
    addDebugModes(ScenarioKind::MovementWallRunCorridor,
                  "movement_wall_run_corridor",
                  "movement_wall_run_corridor");
  }
  return specs;
}

void writeScenarioJson(std::ostream& out,
                       const ScenarioResult& result,
                       std::string_view indent) {
  const ScenarioCounters& counters = result.counters;
  const TimingTotals& timings = result.timings;
  out << indent << "{\n";
  out << indent << "  \"schema\": "
      << "\"iggy3d.product_frame_metrics.scenario.v1\",\n";
  out << indent << "  \"ok\": " << (result.ok ? "true" : "false") << ",\n";
  out << indent << "  \"status\": ";
  writeJsonString(out, result.status);
  out << ",\n";
  out << indent << "  \"reason_code\": ";
  writeJsonString(out, result.reasonCode);
  out << ",\n";
  out << indent << "  \"scenario\": ";
  writeJsonString(out, result.spec.name);
  out << ",\n";
  out << indent << "  \"room_id\": ";
  writeJsonString(out, result.roomId);
  out << ",\n";
  out << indent << "  \"requested_room_id\": ";
  writeJsonString(out, result.spec.roomId);
  out << ",\n";
  out << indent << "  \"frames\": " << result.frames << ",\n";
  out << indent << "  \"debug_overlay\": "
      << (result.spec.debugOverlay ? "true" : "false") << ",\n";
  out << indent << "  \"active_surface\": ";
  writeJsonString(out, result.activeSurface);
  out << ",\n";
  out << indent << "  \"input_owner\": ";
  writeJsonString(out, result.inputOwner);
  out << ",\n";
  out << indent << "  \"gameplay_input_suppressed\": "
      << (result.gameplayInputSuppressed ? "true" : "false") << ",\n";
  out << indent << "  \"timings_ns\": {\n";
  out << indent << "    \"setup_ns\": " << timings.setupNs << ",\n";
  out << indent << "    \"input_or_automation_ns\": "
      << timings.inputOrAutomationNs << ",\n";
  out << indent << "    \"tick_or_gameplay_update_ns\": "
      << timings.tickOrGameplayUpdateNs << ",\n";
  out << indent << "    \"projection_or_frame_build_ns\": "
      << timings.projectionOrFrameBuildNs << ",\n";
  out << indent << "    \"receipt_or_metrics_build_ns\": "
      << timings.receiptOrMetricsBuildNs << ",\n";
  out << indent << "    \"total_ns\": " << timings.totalNs << "\n";
  out << indent << "  },\n";
  out << indent << "  \"counters\": {\n";
  out << indent << "    \"room_floor_count\": " << result.roomFloorCount
      << ",\n";
  out << indent << "    \"room_wall_count\": " << result.roomWallCount
      << ",\n";
  out << indent << "    \"room_surface_count\": " << result.roomSurfaceCount
      << ",\n";
  out << indent << "    \"collision_surface_count\": "
      << result.roomCollisionSurfaceCount << ",\n";
  out << indent << "    \"draw_item_count_max\": " << counters.drawItemMax
      << ",\n";
  out << indent << "    \"primitive_count_max\": "
      << counters.primitiveCountMax << ",\n";
  out << indent << "    \"triangle_count_max\": " << counters.triangleCountMax
      << ",\n";
  out << indent << "    \"render_bridge_item_count_max\": "
      << counters.renderBridgeItemMax << ",\n";
  out << indent << "    \"scene_item_count_max\": " << counters.sceneItemMax
      << ",\n";
  out << indent << "    \"debug_item_count_max\": " << counters.debugItemMax
      << ",\n";
  out << indent << "    \"debug_item_count\": " << counters.debugItemMax
      << ",\n";
  out << indent << "    \"debug_hud_line_count_max\": "
      << counters.debugHudLineMax << ",\n";
  out << indent << "    \"movement_debug_hud_line_count_max\": "
      << counters.movementDebugHudLineMax << ",\n";
  out << indent << "    \"physics_debug_hud_line_count_max\": "
      << counters.physicsDebugHudLineMax << ",\n";
  out << indent << "    \"npc_debug_hud_line_count_max\": "
      << counters.npcDebugHudLineMax << ",\n";
  out << indent << "    \"physics_movement_surface_count_max\": "
      << counters.physicsMovementSurfaceMax << ",\n";
  out << indent << "    \"physics_movement_sweep_count_max\": "
      << counters.physicsMovementSweepMax << "\n";
  out << indent << "  },\n";
  out << indent << "  \"movement\": {\n";
  out << indent << "    \"movement_state_last\": ";
  writeJsonString(out, counters.lastMovementState);
  out << ",\n";
  out << indent << "    \"horizontal_speed_mps_last\": "
      << counters.lastHorizontalSpeedMetersPerSecond << ",\n";
  out << indent << "    \"vertical_velocity_mps_last\": "
      << counters.lastVerticalVelocityMetersPerSecond << ",\n";
  out << indent << "    \"wall_run_candidate_frame_count\": "
      << counters.wallRunCandidateFrameCount << ",\n";
  out << indent << "    \"wall_run_active_frame_count\": "
      << counters.wallRunActiveFrameCount << ",\n";
  out << indent << "    \"wall_running_state_frame_count\": "
      << counters.wallRunningStateFrameCount << ",\n";
  out << indent << "    \"wall_run_status_last\": ";
  writeJsonString(out, counters.lastWallRunStatus);
  out << "\n";
  out << indent << "  }\n";
  out << indent << "}";
}

void writeSuiteJson(std::ostream& out,
                    const std::vector<ScenarioResult>& results,
                    std::uint64_t totalElapsedNs) {
  bool ok = !results.empty();
  for (const ScenarioResult& result : results) {
    ok = ok && result.ok;
  }
  out << "{\n";
  out << "  \"schema\": \"iggy3d.product_frame_metrics.v1\",\n";
  out << "  \"ok\": " << (ok ? "true" : "false") << ",\n";
  out << "  \"status\": ";
  writeJsonString(out, ok ? "product_frame_metrics_ready"
                          : "product_frame_metrics_failed");
  out << ",\n";
  out << "  \"reason_code\": ";
  writeJsonString(out, ok ? "product_frame_metrics_ready"
                          : "product_frame_metrics_failed");
  out << ",\n";
  out << "  \"scenario_count\": " << results.size() << ",\n";
  out << "  \"total_elapsed_ns\": " << totalElapsedNs << ",\n";
  out << "  \"scenarios\": [\n";
  for (std::size_t index = 0; index < results.size(); ++index) {
    writeScenarioJson(out, results[index], "    ");
    out << (index + 1U == results.size() ? "\n" : ",\n");
  }
  out << "  ]\n";
  out << "}\n";
}

}  // namespace

int main(int argc, char** argv) {
  CliConfig config;
  if (!parseCli(argc, argv, config)) {
    printUsage(std::cerr);
    return 2;
  }
  if (config.help) {
    printUsage(std::cout);
    return 0;
  }

  const Clock::time_point begin = Clock::now();
  const std::vector<ScenarioSpec> specs = makeScenarioSpecs(config);
  std::vector<ScenarioResult> results;
  results.reserve(specs.size());
  for (const ScenarioSpec& spec : specs) {
    results.push_back(runScenario(spec, config.frames, config.noTiming));
  }
  const std::uint64_t totalElapsedNs = elapsedNs(begin, Clock::now(),
                                                config.noTiming);

  if (!config.outputPath.empty()) {
    std::ofstream out(config.outputPath);
    if (!out) {
      std::cerr << "failed to open output path: " << config.outputPath << "\n";
      return 1;
    }
    writeSuiteJson(out, results, totalElapsedNs);
  } else {
    writeSuiteJson(std::cout, results, totalElapsedNs);
  }

  for (const ScenarioResult& result : results) {
    if (!result.ok) {
      return 1;
    }
  }
  return 0;
}
