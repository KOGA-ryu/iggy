#include "app/PackageRuntimeLookup.hpp"
#if defined(IGGY3D_HAS_SDL3)
#include <SDL3/SDL.h>
#include "app/platform/SdlWindow.hpp"
#endif
#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_APP_VULKAN_BACKEND)
#include "app/platform/SdlVulkanSurface.hpp"
#include "render/vulkan/VulkanBackend.hpp"
#endif
#include "content/PackageLoader.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"
#include "render/RendererApi.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/session/Session.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace {

enum class VisualInputBackend : std::uint8_t {
  Keyboard,
  Gamepad,
  Auto,
  Scripted,
};

struct VisualOptions {
  std::filesystem::path fixturePath = "demos/first_room/package.iggy3d.toml";
  std::filesystem::path shaderRoot;
  std::filesystem::path diagnosticsDir;
  iggy3d::RendererBackendKind backend = iggy3d::RendererBackendKind::Null;
  bool autoBackend = false;
  bool requireRenderer = false;
  bool strictVulkan = false;
  bool printReceipt = false;
  bool window = false;
  bool windowFlagSeen = false;
  bool noWindowFlagSeen = false;
  bool interactive = false;
  bool scriptedPlayableSmoke = false;
  VisualInputBackend inputBackend = VisualInputBackend::Keyboard;
  std::uint32_t holdSeconds = 0U;
  std::uint32_t frames = 1U;
  bool framesExplicit = false;
};

struct ParseResult {
  bool ok = true;
  bool packagePathSet = false;
  bool fixturePathSet = false;
  std::string reason = "visual_demo_ok";
  VisualOptions options;
};

struct WindowReceiptFields {
  bool requested = false;
  bool sdlAvailable = false;
  bool created = false;
  bool opened = false;
  bool closed = false;
  bool drawable = false;
  bool quitRequested = false;
  std::uint32_t windowWidth = 0;
  std::uint32_t windowHeight = 0;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
  std::uint32_t eventPollCount = 0;
};

struct PlayableReceiptFields {
  bool playable = false;
  bool interactiveMode = false;
  VisualInputBackend inputBackend = VisualInputBackend::Keyboard;
  bool targetDiscovered = false;
  bool initialReachFailed = false;
  bool reachPassed = false;
  bool interactionExecuted = false;
  bool attackExecuted = false;
  bool objectiveComplete = false;
  bool resetExecuted = false;
  bool saveLoadReplayStable = true;
  bool retryAvailable = true;
  bool mouseLookAvailable = false;
  bool mouseLookUsed = false;
  bool gamepadAvailable = false;
  bool gamepadLeftStickUsed = false;
  bool gamepadRightStickUsed = false;
  std::string gamepadName = "unavailable";
  std::string gamepadMapping = "unavailable";
  std::string gamepadActionButton = "unavailable";
};

std::string_view inputBackendName(VisualInputBackend backend) {
  switch (backend) {
    case VisualInputBackend::Keyboard:
      return "keyboard";
    case VisualInputBackend::Gamepad:
      return "gamepad";
    case VisualInputBackend::Auto:
      return "auto";
    case VisualInputBackend::Scripted:
      return "scripted";
  }
  return "keyboard";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

ParseResult parseOptions(int argc, const char* const* argv) {
  ParseResult result;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg{argv[i]};
    if (arg == "--package" && hasValue(i, argc)) {
      if (result.fixturePathSet || result.packagePathSet) {
        result.ok = false;
        result.reason = "visual_demo_config_invalid";
        return result;
      }
      result.options.fixturePath = argv[++i];
      result.packagePathSet = true;
    } else if (arg == "--fixture" && hasValue(i, argc)) {
      if (result.packagePathSet || result.fixturePathSet) {
        result.ok = false;
        result.reason = "visual_demo_config_invalid";
        return result;
      }
      result.options.fixturePath = argv[++i];
      result.fixturePathSet = true;
    } else if (arg == "--renderer" && hasValue(i, argc)) {
      const std::string_view renderer{argv[++i]};
      if (renderer == "null") {
        result.options.backend = iggy3d::RendererBackendKind::Null;
        result.options.autoBackend = false;
      } else if (renderer == "vulkan") {
        result.options.backend = iggy3d::RendererBackendKind::Vulkan;
        result.options.autoBackend = false;
      } else if (renderer == "auto") {
        result.options.backend = iggy3d::RendererBackendKind::Null;
        result.options.autoBackend = true;
      } else {
        result.ok = false;
        result.reason = "visual_demo_config_invalid";
      }
    } else if (arg == "--renderer=null") {
      result.options.backend = iggy3d::RendererBackendKind::Null;
      result.options.autoBackend = false;
    } else if (arg == "--renderer=vulkan") {
      result.options.backend = iggy3d::RendererBackendKind::Vulkan;
      result.options.autoBackend = false;
    } else if (arg == "--renderer=auto") {
      result.options.backend = iggy3d::RendererBackendKind::Null;
      result.options.autoBackend = true;
    } else if (arg == "--require-renderer") {
      result.options.requireRenderer = true;
    } else if (arg == "--strict-vulkan") {
      result.options.strictVulkan = true;
      result.options.backend = iggy3d::RendererBackendKind::Vulkan;
      result.options.requireRenderer = true;
    } else if (arg == "--window") {
      if (result.options.noWindowFlagSeen) {
        result.ok = false;
        result.reason = "visual_demo_config_invalid";
        return result;
      }
      result.options.window = true;
      result.options.windowFlagSeen = true;
    } else if (arg == "--no-window") {
      if (result.options.windowFlagSeen) {
        result.ok = false;
        result.reason = "visual_demo_config_invalid";
        return result;
      }
      result.options.window = false;
      result.options.noWindowFlagSeen = true;
    } else if (arg == "--interactive") {
      result.options.interactive = true;
    } else if (arg == "--scripted-playable-smoke") {
      result.options.scriptedPlayableSmoke = true;
      result.options.inputBackend = VisualInputBackend::Scripted;
      result.options.backend = iggy3d::RendererBackendKind::Vulkan;
      result.options.window = true;
      result.options.windowFlagSeen = true;
    } else if (arg == "--hold-seconds" && hasValue(i, argc)) {
      const std::string_view count{argv[++i]};
      std::uint32_t value = 0U;
      for (const char c : count) {
        if (c < '0' || c > '9') {
          result.ok = false;
          result.reason = "visual_demo_config_invalid";
          return result;
        }
        value = value * 10U + static_cast<std::uint32_t>(c - '0');
      }
      result.options.holdSeconds = value;
    } else if (arg == "--input" && hasValue(i, argc)) {
      const std::string_view input{argv[++i]};
      if (input == "keyboard") {
        result.options.inputBackend = VisualInputBackend::Keyboard;
      } else if (input == "gamepad") {
        result.options.inputBackend = VisualInputBackend::Gamepad;
      } else if (input == "auto") {
        result.options.inputBackend = VisualInputBackend::Auto;
      } else {
        result.ok = false;
        result.reason = "visual_demo_config_invalid";
        return result;
      }
    } else if (arg == "--frames" && hasValue(i, argc)) {
      const std::string_view count{argv[++i]};
      std::uint32_t value = 0U;
      for (const char c : count) {
        if (c < '0' || c > '9') {
          result.ok = false;
          result.reason = "visual_demo_config_invalid";
          return result;
        }
        value = value * 10U + static_cast<std::uint32_t>(c - '0');
      }
      result.options.frames = value == 0U ? 1U : value;
      result.options.framesExplicit = true;
    } else if (arg == "--shader-root" && hasValue(i, argc)) {
      result.options.shaderRoot = argv[++i];
    } else if (arg == "--diagnostics-dir" && hasValue(i, argc)) {
      result.options.diagnosticsDir = argv[++i];
    } else if (arg == "--print-render-receipt") {
      result.options.printReceipt = true;
    } else if (arg == "--validation=off" || arg == "--validation=optional" ||
               arg == "--validation=required" || arg == "--sync-validation=off" ||
               arg == "--sync-validation=optional" || arg == "--sync-validation=required") {
      continue;
    } else {
      result.ok = false;
      result.reason = "visual_demo_config_invalid";
      return result;
    }
  }
  return result;
}

bool hasFrameLimit(const VisualOptions& options) {
  return !options.interactive || options.scriptedPlayableSmoke || options.framesExplicit;
}

std::uint32_t receiptFrameCount(const VisualOptions& options, std::uint32_t framesPresented) {
  return hasFrameLimit(options) ? options.frames : framesPresented;
}

iggy3d::RenderReceipt baseReceipt(std::string_view result, std::string_view reasonCode) {
  iggy3d::RenderReceipt receipt;
  iggy3d::appendReceiptField(receipt, "receipt_version", "1");
  iggy3d::appendReceiptField(receipt, "repo", "iggy3d");
  iggy3d::appendReceiptField(receipt, "app", "iggy3d_visual_demo");
  iggy3d::appendReceiptField(receipt, "packet_order", "3");
  iggy3d::appendReceiptField(receipt, "result", result);
  iggy3d::appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
}

std::filesystem::path resolveFixturePath(const std::filesystem::path& fixturePath,
                                         const iggy3d::PackageRuntimeLookup& lookup) {
  if (fixturePath.is_absolute()) {
    return fixturePath.lexically_normal();
  }
  auto firstComponent = fixturePath.begin();
  if (firstComponent != fixturePath.end() && firstComponent->string() == "fixtures") {
    return (lookup.packageRoot / fixturePath).lexically_normal();
  }
  return (lookup.resourceRoot / fixturePath).lexically_normal();
}

int printFailure(std::string_view reasonCode) {
  std::cout << iggy3d::formatRenderReceipt(baseReceipt("fail", reasonCode));
  return 1;
}

void appendReceiptFieldIfMissing(iggy3d::RenderReceipt& receipt,
                                 std::string_view key,
                                 std::string_view value) {
  if (!iggy3d::hasReceiptField(receipt, key)) {
    iggy3d::appendReceiptField(receipt, key, value);
  }
}

void appendReceiptFieldIfMissing(iggy3d::RenderReceipt& receipt,
                                 std::string_view key,
                                 const char* value) {
  appendReceiptFieldIfMissing(
      receipt, key,
      value == nullptr ? std::string_view{"unavailable"} : std::string_view{value});
}

void appendReceiptFieldIfMissing(iggy3d::RenderReceipt& receipt,
                                 std::string_view key,
                                 bool value) {
  if (!iggy3d::hasReceiptField(receipt, key)) {
    iggy3d::appendReceiptField(receipt, key, value);
  }
}

void appendReceiptFieldIfMissing(iggy3d::RenderReceipt& receipt,
                                 std::string_view key,
                                 std::uint64_t value) {
  if (!iggy3d::hasReceiptField(receipt, key)) {
    iggy3d::appendReceiptField(receipt, key, value);
  }
}

void appendVulkanBootFields(iggy3d::RenderReceipt& receipt,
                            iggy3d::RendererBackendKind backend,
                            bool surfaceCreated,
                            bool swapchainReady,
                            iggy3d::RendererLifecycleState lifecycle) {
  if (backend != iggy3d::RendererBackendKind::Vulkan) {
    return;
  }
#if defined(__APPLE__)
  appendReceiptFieldIfMissing(receipt, "platform", "macos");
  appendReceiptFieldIfMissing(receipt, "platform_lane", "moltenvk");
#else
  appendReceiptFieldIfMissing(receipt, "platform", "unknown");
  appendReceiptFieldIfMissing(receipt, "platform_lane", "native_vulkan");
#endif
#if defined(IGGY3D_HAS_VULKAN)
  appendReceiptFieldIfMissing(receipt, "vulkan_loader_found", true);
  appendReceiptFieldIfMissing(receipt, "portability_enumeration_available", true);
#else
  appendReceiptFieldIfMissing(receipt, "vulkan_loader_found", false);
  appendReceiptFieldIfMissing(receipt, "portability_enumeration_available", "unavailable");
#endif
#if defined(IGGY3D_VULKAN_ICD_FOUND_VALUE) && IGGY3D_VULKAN_ICD_FOUND_VALUE
  appendReceiptFieldIfMissing(receipt, "vulkan_icd_found", true);
#else
  appendReceiptFieldIfMissing(receipt, "vulkan_icd_found", false);
#endif
#if defined(__APPLE__) && defined(IGGY3D_HAS_VULKAN)
  appendReceiptFieldIfMissing(receipt, "moltenvk_available", surfaceCreated);
#else
  appendReceiptFieldIfMissing(receipt, "moltenvk_available", "unavailable");
#endif
  appendReceiptFieldIfMissing(receipt, "surface_created", surfaceCreated);
  appendReceiptFieldIfMissing(receipt, "swapchain_ready", swapchainReady);
  appendReceiptFieldIfMissing(receipt, "renderer_lifecycle",
                              iggy3d::rendererLifecycleStateName(lifecycle));
}

int printReceiptAndReturn(iggy3d::RenderReceipt receipt, int exitCode) {
  std::cout << iggy3d::formatRenderReceipt(receipt);
  return exitCode;
}

void appendWindowReceiptFields(iggy3d::RenderReceipt& receipt,
                               const WindowReceiptFields& fields) {
  if (!fields.requested) {
    return;
  }
  iggy3d::appendReceiptField(receipt, "window_mode", "window");
  iggy3d::appendReceiptField(receipt, "window_shell", fields.sdlAvailable ? "sdl3" : "unavailable");
  iggy3d::appendReceiptField(receipt, "sdl3_available", fields.sdlAvailable);
  iggy3d::appendReceiptField(receipt, "window_created", fields.created);
  iggy3d::appendReceiptField(receipt, "window_opened", fields.opened);
  iggy3d::appendReceiptField(receipt, "window_closed", fields.closed);
  iggy3d::appendReceiptField(receipt, "window_width", static_cast<std::uint64_t>(fields.windowWidth));
  iggy3d::appendReceiptField(receipt, "window_height",
                             static_cast<std::uint64_t>(fields.windowHeight));
  iggy3d::appendReceiptField(receipt, "drawable_width",
                             static_cast<std::uint64_t>(fields.drawableWidth));
  iggy3d::appendReceiptField(receipt, "drawable_height",
                             static_cast<std::uint64_t>(fields.drawableHeight));
  iggy3d::appendReceiptField(receipt, "drawable", fields.drawable);
  iggy3d::appendReceiptField(receipt, "event_poll_count",
                             static_cast<std::uint64_t>(fields.eventPollCount));
  iggy3d::appendReceiptField(receipt, "quit_requested", fields.quitRequested);
}

#if !defined(IGGY3D_HAS_SDL3)
int printWindowSdlUnavailable() {
  WindowReceiptFields fields;
  fields.requested = true;
  fields.sdlAvailable = false;

  iggy3d::RenderReceipt receipt = baseReceipt("skip", "sdl3_unavailable");
  iggy3d::appendReceiptField(receipt, "visual_demo", "bounded");
  appendWindowReceiptFields(receipt, fields);
  iggy3d::appendReceiptField(receipt, "backend", "null");
  iggy3d::appendReceiptField(receipt, "frames", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "frames_presented", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "draw_count", static_cast<std::uint64_t>(0));
  std::cout << iggy3d::formatRenderReceipt(receipt);
  return 77;
}
#endif

#if defined(IGGY3D_HAS_SDL3)
int printWindowUnavailable(WindowReceiptFields fields) {
  fields.requested = true;
  fields.sdlAvailable = true;

  iggy3d::RenderReceipt receipt = baseReceipt("fail", "visual_demo_window_unavailable");
  iggy3d::appendReceiptField(receipt, "visual_demo", "bounded");
  appendWindowReceiptFields(receipt, fields);
  iggy3d::appendReceiptField(receipt, "backend", "null");
  iggy3d::appendReceiptField(receipt, "frames", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "frames_presented", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "draw_count", static_cast<std::uint64_t>(0));
  std::cout << iggy3d::formatRenderReceipt(receipt);
  return 1;
}
#endif

iggy3d::EntityId entityIdByName(const iggy3d::Session& session, std::string_view stableName) {
  const iggy3d::EntityState* entity = session.state().world.findByStableName(std::string(stableName));
  return entity == nullptr ? iggy3d::EntityId{} : entity->id;
}

const iggy3d::EntityState* playerEntity(const iggy3d::Session& session) {
  return session.state().world.findByStableName("player");
}

iggy3d::CommandRecord moveCommand(iggy3d::Vec3 point) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Move;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = point;
  return command;
}

iggy3d::CommandRecord interactCommand(iggy3d::EntityId target) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Interact;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = target;
  return command;
}

iggy3d::CommandRecord retryCommand(iggy3d::CommandId sourceCommandId) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Retry;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.retrySourceCommandId = sourceCommandId;
  return command;
}

iggy3d::CommandRecord attackCommand(iggy3d::EntityId target) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Attack;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = target;
  command.payload.attackDamage = 3;
  return command;
}

bool accepted(const iggy3d::SessionCommandResult& result) {
  return result.command.admission == iggy3d::CommandAdmissionStatus::Accepted;
}

bool submitAndDrain(iggy3d::Session& session, const iggy3d::CommandRecord& command) {
  const iggy3d::SessionCommandResult submitted = session.submitCommand(command);
  if (!accepted(submitted)) {
    return false;
  }
  if (submitted.executedImmediately) {
    return true;
  }
  const iggy3d::StatusResult run = session.runUntilIdle(8);
  return run.status == iggy3d::ResultStatus::Ok;
}

bool objectiveComplete(const iggy3d::Session& session) {
  for (const iggy3d::ObjectiveRecord& objective : session.state().objectives.objectives) {
    if (objective.objectiveId == "collect_gold_key" &&
        objective.status == iggy3d::ObjectiveStatus::Complete) {
      return true;
    }
  }
  return false;
}

void attachRoomProjection(const iggy3d::PackageLoadResult& package,
                          iggy3d::SceneProjectionResult& scene) {
  if (package.rooms.empty()) {
    return;
  }
  const iggy3d::RoomAsset& room = package.rooms.front();
  scene.room.loaded = true;
  scene.room.assetId = room.id;
  scene.room.version = room.version;
  scene.room.sourceToml = room.sourceFile;
  scene.room.sourceSubset = room.sourceSubset;
  scene.room.staticMeshCount = room.staticMeshes.size();
  scene.room.materialCount = package.materials.materials.size();
  scene.room.anchorCount = room.anchors.size();
  scene.room.meshes.clear();
  scene.room.meshes.reserve(room.staticMeshes.size());
  iggy3d::Vec3 roomOriginOffset{};
  for (const iggy3d::RoomAnchorAsset& anchor : room.anchors) {
    if (anchor.id == "player_spawn") {
      roomOriginOffset = anchor.positionMeters;
    }
    scene.room.keyAnchorVisible =
        scene.room.keyAnchorVisible || anchor.runtimeStableName == "gold_key";
    scene.room.dummyAnchorVisible =
        scene.room.dummyAnchorVisible || anchor.runtimeStableName == "training_dummy";
  }
  for (const iggy3d::RoomStaticMeshAsset& mesh : room.staticMeshes) {
    iggy3d::SceneRoomMeshItem item;
    item.id = mesh.id;
    item.role = mesh.role;
    item.position = mesh.positionMeters - roomOriginOffset;
    item.size = mesh.sizeMeters;
    scene.room.floorVisible = scene.room.floorVisible || mesh.role == "floor";
    scene.room.wallVisible = scene.room.wallVisible || mesh.role == "wall";
    scene.room.openingVisible = scene.room.openingVisible || mesh.role == "opening";
    scene.room.propVisible = scene.room.propVisible || mesh.role == "prop";
    scene.room.meshes.push_back(std::move(item));
  }
}

iggy3d::Vec3 cross(iggy3d::Vec3 lhs, iggy3d::Vec3 rhs) {
  return {lhs.y * rhs.z - lhs.z * rhs.y,
          lhs.z * rhs.x - lhs.x * rhs.z,
          lhs.x * rhs.y - lhs.y * rhs.x};
}

iggy3d::Vec3 normalizedOr(iggy3d::Vec3 value, iggy3d::Vec3 fallback) {
  const float magnitudeSquared = iggy3d::lengthSquared(value);
  if (magnitudeSquared <= 0.000001F) {
    return fallback;
  }
  return value / std::sqrt(magnitudeSquared);
}

bool runScriptedPlayableStep(iggy3d::Session& session,
                             std::uint32_t frameIndex,
                             PlayableReceiptFields& fields) {
  const iggy3d::EntityId key = entityIdByName(session, "gold_key");
  const iggy3d::EntityId dummy = entityIdByName(session, "training_dummy");
  if (frameIndex == 0U) {
    const iggy3d::SessionCommandResult interact = session.submitCommand(interactCommand(key));
    fields.targetDiscovered = interact.command.payload.target.hasEntity;
    fields.initialReachFailed =
        interact.command.rejection == iggy3d::CommandRejectionReason::OutOfRange;
    return true;
  }
  if (frameIndex == 1U) {
    fields.reachPassed = submitAndDrain(session, moveCommand({2.0F, 0.0F, 0.0F}));
    return fields.reachPassed;
  }
  if (frameIndex == 2U) {
    fields.interactionExecuted = submitAndDrain(session, retryCommand(1));
    fields.objectiveComplete = objectiveComplete(session);
    return fields.interactionExecuted;
  }
  if (frameIndex == 3U) {
    return submitAndDrain(session, moveCommand({2.0F, 0.0F, 1.0F}));
  }
  if (frameIndex == 4U) {
    fields.attackExecuted = submitAndDrain(session, attackCommand(dummy));
    return fields.attackExecuted;
  }
  if (frameIndex == 5U) {
    const iggy3d::SessionResetResult reset = session.resetToBaseline();
    fields.resetExecuted = reset.reset;
    return fields.resetExecuted;
  }
  return true;
}

void appendPlayableReceiptFields(iggy3d::RenderReceipt& receipt,
                                 const PlayableReceiptFields& fields) {
  if (!fields.playable) {
    return;
  }
  iggy3d::appendReceiptField(receipt, "camera_mode", "first_person");
  iggy3d::appendReceiptField(receipt, "interactive_mode", fields.interactiveMode);
  iggy3d::appendReceiptField(receipt, "input_backend", inputBackendName(fields.inputBackend));
  iggy3d::appendReceiptField(receipt, "target_discovered", fields.targetDiscovered);
  iggy3d::appendReceiptField(receipt, "initial_reach_gate",
                             fields.initialReachFailed ? "fail" : "not_attempted");
  iggy3d::appendReceiptField(receipt, "reach_gate",
                             fields.reachPassed ? "pass"
                                                : (fields.initialReachFailed ? "fail"
                                                                             : "not_attempted"));
  iggy3d::appendReceiptField(receipt, "interaction_executed", fields.interactionExecuted);
  iggy3d::appendReceiptField(receipt, "attack_executed", fields.attackExecuted);
  iggy3d::appendReceiptField(receipt, "objective_complete", fields.objectiveComplete);
  iggy3d::appendReceiptField(receipt, "retry_available", fields.retryAvailable);
  iggy3d::appendReceiptField(receipt, "reset_executed", fields.resetExecuted);
  iggy3d::appendReceiptField(receipt, "save_load_replay_stable", fields.saveLoadReplayStable);
  iggy3d::appendReceiptField(receipt, "tactical_view_available", false);
  iggy3d::appendReceiptField(receipt, "mouse_look_available", fields.mouseLookAvailable);
  iggy3d::appendReceiptField(receipt, "mouse_look_used", fields.mouseLookUsed);
  iggy3d::appendReceiptField(receipt, "gamepad_available", fields.gamepadAvailable);
  iggy3d::appendReceiptField(receipt, "gamepad_name", fields.gamepadName);
  iggy3d::appendReceiptField(receipt, "gamepad_mapping", fields.gamepadMapping);
  iggy3d::appendReceiptField(receipt, "gamepad_left_stick_used", fields.gamepadLeftStickUsed);
  iggy3d::appendReceiptField(receipt, "gamepad_right_stick_used", fields.gamepadRightStickUsed);
  iggy3d::appendReceiptField(receipt, "gamepad_action_button", fields.gamepadActionButton);
}

#if defined(IGGY3D_HAS_SDL3)
struct GamepadSession {
  SDL_Gamepad* gamepad = nullptr;
  SDL_Joystick* joystick = nullptr;
  bool subsystemInitialized = false;
  bool crossDown = false;
  bool eastDown = false;
  bool startDown = false;
  bool r2Down = false;
};

bool nameLooksLikeDualSense(std::string_view name) {
  return name.find("DualSense") != std::string_view::npos ||
         name.find("PS5") != std::string_view::npos ||
         name.find("Sony") != std::string_view::npos ||
         name.find("Wireless Controller") != std::string_view::npos;
}

GamepadSession openFirstGamepad(PlayableReceiptFields& fields) {
  GamepadSession session;
  if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
    return session;
  }
  session.subsystemInitialized = true;
  int count = 0;
  SDL_JoystickID* ids = SDL_GetGamepads(&count);
  if (ids != nullptr && count > 0) {
    session.gamepad = SDL_OpenGamepad(ids[0]);
  }
  SDL_free(ids);
  if (session.gamepad != nullptr) {
    fields.gamepadAvailable = true;
    const char* name = SDL_GetGamepadName(session.gamepad);
    fields.gamepadName = name == nullptr ? "sdl_gamepad" : name;
    fields.gamepadMapping =
        nameLooksLikeDualSense(fields.gamepadName) ? "dualsense_default" : "sdl_gamepad";
    fields.gamepadActionButton = "cross";
  } else {
    int joystickCount = 0;
    SDL_JoystickID* joystickIds = SDL_GetJoysticks(&joystickCount);
    if (joystickIds != nullptr && joystickCount > 0) {
      session.joystick = SDL_OpenJoystick(joystickIds[0]);
    }
    SDL_free(joystickIds);
    if (session.joystick != nullptr) {
      fields.gamepadAvailable = true;
      const char* name = SDL_GetJoystickName(session.joystick);
      fields.gamepadName = name == nullptr ? "sdl_joystick" : name;
      fields.gamepadMapping = nameLooksLikeDualSense(fields.gamepadName)
                                  ? "dualsense_joystick_fallback"
                                  : "sdl_joystick_fallback";
      fields.gamepadActionButton = "button0";
    }
  }
  return session;
}

void closeGamepad(GamepadSession& session) {
  if (session.gamepad != nullptr) {
    SDL_CloseGamepad(session.gamepad);
    session.gamepad = nullptr;
  }
  if (session.joystick != nullptr) {
    SDL_CloseJoystick(session.joystick);
    session.joystick = nullptr;
  }
  if (session.subsystemInitialized) {
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    session.subsystemInitialized = false;
  }
}

float axisValue(Sint16 value) {
  constexpr float kDeadZone = 9000.0F;
  const float asFloat = static_cast<float>(value);
  if (std::abs(asFloat) < kDeadZone) {
    return 0.0F;
  }
  return std::clamp(asFloat / 32767.0F, -1.0F, 1.0F);
}

bool pressedEdge(bool current, bool& previous) {
  const bool edge = current && !previous;
  previous = current;
  return edge;
}

void applyMouseLook(float& yaw, float& pitch, PlayableReceiptFields& fields) {
  float mouseX = 0.0F;
  float mouseY = 0.0F;
  const SDL_MouseButtonFlags buttons = SDL_GetRelativeMouseState(&mouseX, &mouseY);
  fields.mouseLookAvailable = true;
  const bool dragging = (buttons & (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK | SDL_BUTTON_MMASK)) != 0U;
  if (!dragging || !std::isfinite(mouseX) || !std::isfinite(mouseY)) {
    return;
  }

  const float dx = std::clamp(mouseX, -80.0F, 80.0F);
  const float dy = std::clamp(mouseY, -80.0F, 80.0F);
  if (dx == 0.0F && dy == 0.0F) {
    return;
  }

  constexpr float kMouseLookScale = 0.004F;
  yaw += dx * kMouseLookScale;
  pitch += dy * kMouseLookScale;
  fields.mouseLookUsed = true;
}
#endif

iggy3d::Mat4 perspectiveMat4(float verticalFovRadians,
                             float aspect,
                             float nearPlane,
                             float farPlane) {
  const float f = 1.0F / std::tan(verticalFovRadians * 0.5F);
  iggy3d::Mat4 result{{{}}};
  result.m[0] = f / aspect;
  result.m[5] = f;
  result.m[10] = farPlane / (nearPlane - farPlane);
  result.m[11] = -(farPlane * nearPlane) / (farPlane - nearPlane);
  result.m[14] = -1.0F;
  return result;
}

iggy3d::Mat4 viewFromCamera(iggy3d::Vec3 eye,
                            iggy3d::Vec3 forward,
                            iggy3d::Vec3 up) {
  const iggy3d::Vec3 f = normalizedOr(forward, {0.0F, 0.0F, -1.0F});
  const iggy3d::Vec3 r = normalizedOr(cross(f, up), {1.0F, 0.0F, 0.0F});
  const iggy3d::Vec3 u = cross(r, f);
  iggy3d::Mat4 result = iggy3d::identityMat4();
  result.m[0] = r.x;
  result.m[1] = r.y;
  result.m[2] = r.z;
  result.m[3] = -iggy3d::dot(r, eye);
  result.m[4] = u.x;
  result.m[5] = u.y;
  result.m[6] = u.z;
  result.m[7] = -iggy3d::dot(u, eye);
  result.m[8] = -f.x;
  result.m[9] = -f.y;
  result.m[10] = -f.z;
  result.m[11] = iggy3d::dot(f, eye);
  return result;
}

iggy3d::FrameInput makeFrame(const iggy3d::SceneProjectionResult& scene,
                             const iggy3d::DebugProjectionResult& debug,
                             std::uint64_t frameIndex,
                             std::uint32_t viewportWidth,
                             std::uint32_t viewportHeight,
                             bool firstPerson,
                             float yaw,
                             float pitch) {
  iggy3d::FrameInput frame;
  frame.viewport = {viewportWidth, viewportHeight,
                    static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight)};
  frame.clock = {scene.sourceTick == iggy3d::kInvalidCommandTick ? 0U : scene.sourceTick,
                 frameIndex, 0.0F, 1.0F / 60.0F};
  frame.camera.mode =
      firstPerson ? iggy3d::RenderCameraMode::FirstPerson : iggy3d::RenderCameraMode::ThirdPerson;
  iggy3d::Vec3 firstPersonEye{0.0F, 1.65F, 0.0F};
  if (firstPerson) {
    for (const iggy3d::SceneItem& item : scene.items) {
      if (item.kind == iggy3d::SceneItemKind::Player || item.stableName == "player") {
        firstPersonEye = item.transform.position + iggy3d::Vec3{0.0F, 1.65F, 0.0F};
        break;
      }
    }
  }
  const float cosPitch = std::cos(pitch);
  frame.camera.worldEye = firstPerson ? firstPersonEye : iggy3d::Vec3{0.0F, 2.0F, 5.0F};
  frame.camera.worldForward = firstPerson
                                  ? iggy3d::Vec3{std::sin(yaw) * cosPitch, std::sin(pitch),
                                                 -std::cos(yaw) * cosPitch}
                                  : iggy3d::Vec3{0.0F, 0.0F, -1.0F};
  frame.camera.worldUp = {0.0F, 1.0F, 0.0F};
  frame.camera.nearPlane = 0.1F;
  frame.camera.farPlane = 200.0F;
  frame.camera.viewFromWorld =
      viewFromCamera(frame.camera.worldEye, frame.camera.worldForward, frame.camera.worldUp);
  frame.camera.clipFromView =
      perspectiveMat4(68.0F * 3.14159265358979323846F / 180.0F,
                      frame.viewport.aspectRatio, frame.camera.nearPlane,
                      frame.camera.farPlane);
  frame.camera.clipFromWorld = frame.camera.clipFromView * frame.camera.viewFromWorld;
  frame.projections.scene = &scene;
  frame.projections.debug = &debug;
  return frame;
}

}  // namespace

int main(int argc, const char* const* argv) {
  const ParseResult parsed = parseOptions(argc, argv);
  if (!parsed.ok) {
    return printFailure(parsed.reason);
  }

  if (parsed.options.window) {
#if !defined(IGGY3D_HAS_SDL3)
    return printWindowSdlUnavailable();
#endif
  }

  iggy3d::PackageLookupConfig lookupConfig;
  lookupConfig.packageMode = iggy3d::PackageMode::BuildTreeVisual;
  lookupConfig.shaderRootOverride = parsed.options.shaderRoot;
  lookupConfig.diagnosticsDirOverride = parsed.options.diagnosticsDir;
  lookupConfig.requireShaderRoot = parsed.options.strictVulkan;
  lookupConfig.requireGraphicsRuntime = parsed.options.requireRenderer;
  const iggy3d::PackageLookupResult lookup = iggy3d::resolvePackageRuntimeLookup(lookupConfig);
  if (lookup.outcome != iggy3d::RenderOutcome::Ok) {
    return printFailure(lookup.reason.code);
  }

  const std::filesystem::path fixturePath =
      resolveFixturePath(parsed.options.fixturePath, lookup.lookup);
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage(iggy3d::PackageLoadRequest{fixturePath.string()});
  if (package.status != iggy3d::PackageLoadStatus::Ok) {
    return printFailure("visual_demo_package_lookup_failed");
  }

  iggy3d::SessionCreateRequest create;
  create.config = package.scenario.config;
  create.seed = package.scenario;
  iggy3d::Result<iggy3d::Session> sessionResult = iggy3d::Session::create(create);
  if (sessionResult.status != iggy3d::ResultStatus::Ok) {
    return printFailure("visual_demo_package_lookup_failed");
  }
  iggy3d::Session session = std::move(sessionResult.value);

  iggy3d::RendererCreateInfo rendererCreate;
  rendererCreate.backend = parsed.options.autoBackend ? iggy3d::RendererBackendKind::Null
                                                      : parsed.options.backend;
  rendererCreate.config.renderer =
      rendererCreate.backend == iggy3d::RendererBackendKind::Vulkan ? iggy3d::RendererMode::Vulkan
                                                                    : iggy3d::RendererMode::Null;
  rendererCreate.config.shaderRoot = lookup.lookup.shaderRoot;
  rendererCreate.config.diagnosticsDir = lookup.lookup.diagnosticsDir;
  rendererCreate.config.strictVulkan = parsed.options.strictVulkan;
  rendererCreate.config.allowSoftwareVulkan =
      rendererCreate.backend == iggy3d::RendererBackendKind::Vulkan;
  rendererCreate.config.rendererRequirement =
      parsed.options.requireRenderer ? iggy3d::RendererRequirement::Required
                                     : iggy3d::RendererRequirement::Optional;

  WindowReceiptFields windowFields;
  windowFields.requested = parsed.options.window;
  std::uint32_t viewportWidth = 1280U;
  std::uint32_t viewportHeight = 720U;
#if defined(IGGY3D_HAS_SDL3)
  std::optional<iggy3d::SdlWindow> window;
#if defined(IGGY3D_APP_VULKAN_BACKEND)
  iggy3d::SdlVulkanSurfaceProvider sdlVulkanProvider;
#endif
  if (parsed.options.window) {
    windowFields.sdlAvailable = true;
    iggy3d::SdlWindowCreateInfo windowCreate;
    windowCreate.title = "iggy3d visual demo";
    windowCreate.width = viewportWidth;
    windowCreate.height = viewportHeight;
    windowCreate.vulkan = rendererCreate.backend == iggy3d::RendererBackendKind::Vulkan;
    window.emplace(windowCreate);
    windowFields.created = window->nativeWindow() != nullptr;
    windowFields.opened = window->isOpen();
    windowFields.drawable = window->isDrawable();
    const iggy3d::SdlWindowEventState& eventState = window->eventState();
    windowFields.windowWidth = eventState.windowWidth;
    windowFields.windowHeight = eventState.windowHeight;
    windowFields.drawableWidth = eventState.drawableWidth;
    windowFields.drawableHeight = eventState.drawableHeight;
    windowFields.quitRequested = eventState.quitRequested;
    if (!windowFields.created) {
      return printWindowUnavailable(windowFields);
    }
  }
#endif

  iggy3d::RendererApi renderer;
  iggy3d::RendererLifecycleState rendererLifecycleForReceipt =
      iggy3d::RendererLifecycleState::NotInitialized;
  bool vulkanSurfaceCreated = false;
  bool vulkanSwapchainReady = false;
  if (rendererCreate.backend == iggy3d::RendererBackendKind::Vulkan) {
    if (!parsed.options.window) {
      iggy3d::RenderReceipt receipt = baseReceipt("fail", "vulkan_surface_provider_missing");
      iggy3d::appendReceiptField(receipt, "visual_demo", "bounded");
      appendWindowReceiptFields(receipt, windowFields);
      appendVulkanBootFields(receipt, rendererCreate.backend, false, false,
                             iggy3d::RendererLifecycleState::NotInitialized);
      return printReceiptAndReturn(std::move(receipt), 1);
    }
#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_APP_VULKAN_BACKEND)
    if (!window.has_value() || !window->nativeWindow()) {
      return printWindowUnavailable(windowFields);
    }
    const iggy3d::SdlVulkanExtensionList extensions =
        sdlVulkanProvider.requiredInstanceExtensions(*window);
    if (extensions.outcome != iggy3d::RenderOutcome::Ok) {
      iggy3d::RenderReceipt receipt;
      appendReceiptFieldIfMissing(receipt, "receipt_version", "1");
      appendReceiptFieldIfMissing(receipt, "repo", "iggy3d");
      appendReceiptFieldIfMissing(receipt, "app", "iggy3d_visual_demo");
      appendReceiptFieldIfMissing(receipt, "visual_demo", "bounded");
      appendWindowReceiptFields(receipt, windowFields);
      appendVulkanBootFields(receipt, rendererCreate.backend, false, false,
                             iggy3d::RendererLifecycleState::NotInitialized);
      appendReceiptFieldIfMissing(receipt, "result", "fail");
      appendReceiptFieldIfMissing(receipt, "reason_code", extensions.reason.code);
      return printReceiptAndReturn(std::move(receipt), 1);
    }
    iggy3d::VulkanBackendCreateInfo backendInfo;
    backendInfo.config = rendererCreate.config;
    backendInfo.drawableWidth = windowFields.drawableWidth == 0U ? viewportWidth
                                                                 : windowFields.drawableWidth;
    backendInfo.drawableHeight = windowFields.drawableHeight == 0U ? viewportHeight
                                                                   : windowFields.drawableHeight;
    backendInfo.surfaceProvider.requiredInstanceExtensions = extensions.names;
    backendInfo.surfaceProvider.createSurface =
        [&sdlVulkanProvider, &window](VkInstance instance, VkSurfaceKHR* surface) {
          if (!window.has_value()) {
            iggy3d::RenderReceipt receipt;
            iggy3d::appendReceiptField(receipt, "surface_provider", "sdl3");
            iggy3d::appendReceiptField(receipt, "surface_created", false);
            iggy3d::appendReceiptField(receipt, "result", "fail");
            iggy3d::appendReceiptField(receipt, "reason_code", "sdl_window_missing");
            return receipt;
          }
          const iggy3d::SdlVulkanSurfaceCreateResult created =
              sdlVulkanProvider.createSurface(*window, instance);
          if (surface != nullptr) {
            *surface = created.surface;
          }
          iggy3d::RenderReceipt receipt;
          iggy3d::appendReceiptField(receipt, "surface_provider", "sdl3");
          iggy3d::appendReceiptField(receipt, "surface_created",
                                     created.outcome == iggy3d::RenderOutcome::Ok &&
                                         created.surface != VK_NULL_HANDLE);
          iggy3d::appendReceiptField(receipt, "result",
                                     created.outcome == iggy3d::RenderOutcome::Ok ? "pass"
                                                                                  : "fail");
          iggy3d::appendReceiptField(receipt, "reason_code", created.reason.code);
          return receipt;
        };
    renderer =
        iggy3d::RendererApi(std::make_unique<iggy3d::VulkanBackend>(std::move(backendInfo)));
    rendererLifecycleForReceipt = renderer.lifecycleState();
    vulkanSurfaceCreated = renderer.lifecycleState() == iggy3d::RendererLifecycleState::Ready;
    vulkanSwapchainReady = renderer.lifecycleState() == iggy3d::RendererLifecycleState::Ready;
    if (renderer.lifecycleState() != iggy3d::RendererLifecycleState::Ready) {
      iggy3d::RenderReceipt receipt = renderer.diagnostics();
      appendReceiptFieldIfMissing(receipt, "receipt_version", "1");
      appendReceiptFieldIfMissing(receipt, "repo", "iggy3d");
      appendReceiptFieldIfMissing(receipt, "app", "iggy3d_visual_demo");
      appendReceiptFieldIfMissing(receipt, "visual_demo", "bounded");
      appendWindowReceiptFields(receipt, windowFields);
      appendVulkanBootFields(receipt, rendererCreate.backend, vulkanSurfaceCreated,
                             vulkanSwapchainReady, renderer.lifecycleState());
      appendReceiptFieldIfMissing(receipt, "frames", static_cast<std::uint64_t>(0));
      appendReceiptFieldIfMissing(receipt, "frames_presented", static_cast<std::uint64_t>(0));
      appendReceiptFieldIfMissing(receipt, "result", "fail");
      appendReceiptFieldIfMissing(receipt, "reason_code", "visual_demo_renderer_unavailable");
      return printReceiptAndReturn(std::move(receipt), 1);
    }
#else
    iggy3d::RenderReceipt receipt = baseReceipt("fail", "visual_demo_renderer_unavailable");
    iggy3d::appendReceiptField(receipt, "visual_demo", "bounded");
    appendWindowReceiptFields(receipt, windowFields);
    appendVulkanBootFields(receipt, rendererCreate.backend, false, false,
                           iggy3d::RendererLifecycleState::NotInitialized);
    return printReceiptAndReturn(std::move(receipt), 1);
#endif
  } else {
    renderer = iggy3d::createRenderer(rendererCreate);
    rendererLifecycleForReceipt = renderer.lifecycleState();
    if (!renderer.hasBackend() && parsed.options.requireRenderer) {
      return printFailure("visual_demo_renderer_unavailable");
    }
  }

  PlayableReceiptFields playableFields;
  playableFields.playable = parsed.options.interactive || parsed.options.scriptedPlayableSmoke;
  playableFields.interactiveMode = parsed.options.interactive;
  playableFields.inputBackend = parsed.options.inputBackend;
#if defined(IGGY3D_HAS_SDL3)
  GamepadSession gamepad;
  if (playableFields.playable &&
      (parsed.options.inputBackend == VisualInputBackend::Gamepad ||
       parsed.options.inputBackend == VisualInputBackend::Auto)) {
    gamepad = openFirstGamepad(playableFields);
    if (parsed.options.inputBackend == VisualInputBackend::Auto) {
      playableFields.inputBackend =
          playableFields.gamepadAvailable ? VisualInputBackend::Gamepad
                                          : VisualInputBackend::Keyboard;
    }
    if (parsed.options.inputBackend == VisualInputBackend::Gamepad &&
        !playableFields.gamepadAvailable) {
      playableFields.inputBackend = VisualInputBackend::Keyboard;
    }
  }
#endif

  iggy3d::RenderSubmitResult submit;
  std::uint32_t framesPresented = 0U;
  float yaw = 0.0F;
  float pitch = 0.0F;
  bool quitRequested = false;
  const auto interactiveStart = std::chrono::steady_clock::now();
  std::uint32_t frameIndex = 0U;
  while (true) {
    if (hasFrameLimit(parsed.options) && frameIndex >= parsed.options.frames) {
      break;
    }
    if (playableFields.playable && parsed.options.holdSeconds > 0U) {
      const auto elapsed = std::chrono::steady_clock::now() - interactiveStart;
      if (elapsed >= std::chrono::seconds(parsed.options.holdSeconds)) {
        break;
      }
    }
#if defined(IGGY3D_HAS_SDL3)
    if (window.has_value()) {
      window->pollEvents();
      ++windowFields.eventPollCount;
      const iggy3d::SdlWindowEventState& eventState = window->eventState();
      windowFields.opened = window->isOpen();
      windowFields.drawable = window->isDrawable();
      windowFields.windowWidth = eventState.windowWidth;
      windowFields.windowHeight = eventState.windowHeight;
      windowFields.drawableWidth = eventState.drawableWidth;
      windowFields.drawableHeight = eventState.drawableHeight;
      windowFields.quitRequested = eventState.quitRequested;
      if (!window->isOpen()) {
        break;
      }
      if (playableFields.playable && windowFields.quitRequested) {
        break;
      }
      if (windowFields.drawableWidth > 0U && windowFields.drawableHeight > 0U) {
        viewportWidth = windowFields.drawableWidth;
        viewportHeight = windowFields.drawableHeight;
      }
    }
#endif
    if (parsed.options.scriptedPlayableSmoke) {
      if (!runScriptedPlayableStep(session, frameIndex, playableFields)) {
        iggy3d::RenderReceipt receipt = baseReceipt("fail", "visual_demo_scripted_action_failed");
        appendWindowReceiptFields(receipt, windowFields);
        appendPlayableReceiptFields(receipt, playableFields);
        return printReceiptAndReturn(std::move(receipt), 1);
      }
    }
#if defined(IGGY3D_HAS_SDL3)
    if (playableFields.playable && !parsed.options.scriptedPlayableSmoke) {
      if (gamepad.gamepad != nullptr) {
        SDL_UpdateGamepads();
      }
      if (gamepad.joystick != nullptr) {
        SDL_UpdateJoysticks();
      }
      iggy3d::Vec3 movement{};
      bool actionRequested = false;
      bool attackRequested = false;
      bool resetRequested = false;
      if (playableFields.inputBackend == VisualInputBackend::Keyboard) {
        int keyCount = 0;
        const bool* keys = SDL_GetKeyboardState(&keyCount);
        if (keys != nullptr) {
          const bool w = SDL_SCANCODE_W < keyCount && keys[SDL_SCANCODE_W];
          const bool s = SDL_SCANCODE_S < keyCount && keys[SDL_SCANCODE_S];
          const bool a = SDL_SCANCODE_A < keyCount && keys[SDL_SCANCODE_A];
          const bool d = SDL_SCANCODE_D < keyCount && keys[SDL_SCANCODE_D];
          const bool left = SDL_SCANCODE_LEFT < keyCount && keys[SDL_SCANCODE_LEFT];
          const bool right = SDL_SCANCODE_RIGHT < keyCount && keys[SDL_SCANCODE_RIGHT];
          const bool up = SDL_SCANCODE_UP < keyCount && keys[SDL_SCANCODE_UP];
          const bool down = SDL_SCANCODE_DOWN < keyCount && keys[SDL_SCANCODE_DOWN];
          if (left) {
            yaw -= 0.035F;
          }
          if (right) {
            yaw += 0.035F;
          }
          if (up) {
            pitch -= 0.020F;
          }
          if (down) {
            pitch += 0.020F;
          }
          const iggy3d::Vec3 forward{std::sin(yaw), 0.0F, -std::cos(yaw)};
          const iggy3d::Vec3 rightVec{std::cos(yaw), 0.0F, std::sin(yaw)};
          movement = movement + forward * (w ? 1.0F : 0.0F);
          movement = movement - forward * (s ? 1.0F : 0.0F);
          movement = movement - rightVec * (a ? 1.0F : 0.0F);
          movement = movement + rightVec * (d ? 1.0F : 0.0F);
          actionRequested = SDL_SCANCODE_E < keyCount && keys[SDL_SCANCODE_E];
          resetRequested = SDL_SCANCODE_R < keyCount && keys[SDL_SCANCODE_R];
          quitRequested = SDL_SCANCODE_ESCAPE < keyCount && keys[SDL_SCANCODE_ESCAPE];
        }
      } else if (playableFields.inputBackend == VisualInputBackend::Gamepad &&
                 (gamepad.gamepad != nullptr || gamepad.joystick != nullptr)) {
        float leftX = 0.0F;
        float leftY = 0.0F;
        float rightX = 0.0F;
        float rightY = 0.0F;
        float r2 = 0.0F;
        bool slowLook = false;
        bool crossDown = false;
        bool eastDown = false;
        bool startDown = false;
        bool r2Down = false;
        if (gamepad.gamepad != nullptr) {
          leftX = axisValue(SDL_GetGamepadAxis(gamepad.gamepad, SDL_GAMEPAD_AXIS_LEFTX));
          leftY = axisValue(SDL_GetGamepadAxis(gamepad.gamepad, SDL_GAMEPAD_AXIS_LEFTY));
          rightX = axisValue(SDL_GetGamepadAxis(gamepad.gamepad, SDL_GAMEPAD_AXIS_RIGHTX));
          rightY = axisValue(SDL_GetGamepadAxis(gamepad.gamepad, SDL_GAMEPAD_AXIS_RIGHTY));
          r2 = axisValue(SDL_GetGamepadAxis(gamepad.gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));
          slowLook = SDL_GetGamepadButton(gamepad.gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
          crossDown = SDL_GetGamepadButton(gamepad.gamepad, SDL_GAMEPAD_BUTTON_SOUTH);
          eastDown = SDL_GetGamepadButton(gamepad.gamepad, SDL_GAMEPAD_BUTTON_EAST);
          startDown = SDL_GetGamepadButton(gamepad.gamepad, SDL_GAMEPAD_BUTTON_START);
          r2Down = r2 > 0.2F;
        } else {
          const int axisCount = SDL_GetNumJoystickAxes(gamepad.joystick);
          const int buttonCount = SDL_GetNumJoystickButtons(gamepad.joystick);
          leftX = axisCount > 0 ? axisValue(SDL_GetJoystickAxis(gamepad.joystick, 0)) : 0.0F;
          leftY = axisCount > 1 ? axisValue(SDL_GetJoystickAxis(gamepad.joystick, 1)) : 0.0F;
          rightX = axisCount > 2 ? axisValue(SDL_GetJoystickAxis(gamepad.joystick, 2)) : 0.0F;
          rightY = axisCount > 3 ? axisValue(SDL_GetJoystickAxis(gamepad.joystick, 3)) : 0.0F;
          r2 = axisCount > 5 ? axisValue(SDL_GetJoystickAxis(gamepad.joystick, 5)) : 0.0F;
          slowLook = buttonCount > 4 && SDL_GetJoystickButton(gamepad.joystick, 4);
          crossDown = buttonCount > 0 && SDL_GetJoystickButton(gamepad.joystick, 0);
          eastDown = buttonCount > 1 && SDL_GetJoystickButton(gamepad.joystick, 1);
          startDown = buttonCount > 9 ? SDL_GetJoystickButton(gamepad.joystick, 9)
                                      : (buttonCount > 7 && SDL_GetJoystickButton(gamepad.joystick, 7));
          r2Down = r2 > 0.2F ||
                   (buttonCount > 7 && SDL_GetJoystickButton(gamepad.joystick, 7));
        }
        playableFields.gamepadLeftStickUsed =
            playableFields.gamepadLeftStickUsed || leftX != 0.0F || leftY != 0.0F;
        playableFields.gamepadRightStickUsed =
            playableFields.gamepadRightStickUsed || rightX != 0.0F || rightY != 0.0F;
        const float lookScale = slowLook ? 0.020F : 0.045F;
        yaw += rightX * lookScale;
        pitch += rightY * lookScale;
        const iggy3d::Vec3 forward{std::sin(yaw), 0.0F, -std::cos(yaw)};
        const iggy3d::Vec3 rightVec{std::cos(yaw), 0.0F, std::sin(yaw)};
        movement = movement + forward * (-leftY);
        movement = movement + rightVec * leftX;
        actionRequested = pressedEdge(crossDown, gamepad.crossDown);
        attackRequested = pressedEdge(r2Down, gamepad.r2Down);
        if (attackRequested) {
          playableFields.gamepadActionButton = "r2";
        }
        resetRequested = pressedEdge(eastDown, gamepad.eastDown);
        quitRequested = pressedEdge(startDown, gamepad.startDown);
      }
      applyMouseLook(yaw, pitch, playableFields);
      if (pitch > 0.8F) {
        pitch = 0.8F;
      }
      if (pitch < -0.8F) {
        pitch = -0.8F;
      }
      const iggy3d::EntityState* player = playerEntity(session);
      if (player != nullptr && iggy3d::lengthSquared(movement) > 0.01F) {
        const float magnitudeSquared = iggy3d::lengthSquared(movement);
        if (magnitudeSquared > 1.0F) {
          movement = movement / std::sqrt(magnitudeSquared);
        }
        constexpr float kMovementStepMeters = 0.08F;
        const iggy3d::Vec3 destination = player->transform.position + movement * kMovementStepMeters;
        (void)submitAndDrain(session, moveCommand(destination));
      }
      if (resetRequested) {
        const iggy3d::SessionResetResult reset = session.resetToBaseline();
        playableFields.resetExecuted = reset.reset;
      }
      if (actionRequested || attackRequested) {
        const iggy3d::EntityId key = entityIdByName(session, "gold_key");
        const iggy3d::EntityId dummy = entityIdByName(session, "training_dummy");
        const iggy3d::EntityState* keyEntity = session.state().world.findByStableName("gold_key");
        if (attackRequested || (keyEntity != nullptr && !keyEntity->active)) {
          playableFields.attackExecuted = submitAndDrain(session, attackCommand(dummy));
        } else {
          const iggy3d::SessionCommandResult interaction = session.submitCommand(interactCommand(key));
          playableFields.targetDiscovered = interaction.command.payload.target.hasEntity;
          playableFields.initialReachFailed =
              interaction.command.rejection == iggy3d::CommandRejectionReason::OutOfRange;
          if (accepted(interaction)) {
            const iggy3d::StatusResult run = session.runUntilIdle(8);
            playableFields.interactionExecuted = run.status == iggy3d::ResultStatus::Ok;
          }
        }
        playableFields.objectiveComplete = objectiveComplete(session);
      }
      if (quitRequested) {
        break;
      }
    }
#endif
    iggy3d::SceneProjectionResult scene = iggy3d::buildSceneProjection(session.state());
    attachRoomProjection(package, scene);
    const iggy3d::DebugProjectionResult debug = iggy3d::buildDebugProjection(session.state());
    submit = renderer.submitFrame(makeFrame(scene, debug, frameIndex + 1U, viewportWidth,
                                            viewportHeight, playableFields.playable, yaw, pitch));
    if (submit.outcome != iggy3d::RenderOutcome::Ok) {
      iggy3d::RenderReceipt receipt = submit.receipt;
      appendReceiptFieldIfMissing(receipt, "receipt_version", "1");
      appendReceiptFieldIfMissing(receipt, "repo", "iggy3d");
      appendReceiptFieldIfMissing(receipt, "app", "iggy3d_visual_demo");
      appendReceiptFieldIfMissing(receipt, "visual_demo", "bounded");
      appendWindowReceiptFields(receipt, windowFields);
      appendPlayableReceiptFields(receipt, playableFields);
      appendVulkanBootFields(receipt, rendererCreate.backend, vulkanSurfaceCreated,
                             vulkanSwapchainReady, renderer.lifecycleState());
      appendReceiptFieldIfMissing(receipt, "frames",
                                  static_cast<std::uint64_t>(
                                      receiptFrameCount(parsed.options, framesPresented)));
      appendReceiptFieldIfMissing(receipt, "frames_presented",
                                  static_cast<std::uint64_t>(framesPresented));
      appendReceiptFieldIfMissing(receipt, "result", "fail");
      appendReceiptFieldIfMissing(receipt, "reason_code", "visual_demo_frame_failed");
      return printReceiptAndReturn(std::move(receipt), 1);
    }
    ++framesPresented;
    ++frameIndex;
    if (playableFields.playable) {
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
  }

#if defined(IGGY3D_HAS_SDL3)
  closeGamepad(gamepad);
  if (window.has_value()) {
    if (renderer.hasBackend()) {
      rendererLifecycleForReceipt = renderer.lifecycleState();
      static_cast<void>(renderer.waitIdle());
      renderer.shutdown();
    }
    const iggy3d::SdlWindowEventState& eventState = window->eventState();
    windowFields.opened = window->isOpen();
    windowFields.drawable = window->isDrawable();
    windowFields.windowWidth = eventState.windowWidth;
    windowFields.windowHeight = eventState.windowHeight;
    windowFields.drawableWidth = eventState.drawableWidth;
    windowFields.drawableHeight = eventState.drawableHeight;
    windowFields.quitRequested = eventState.quitRequested;
    window.reset();
    windowFields.closed = true;
  }
#endif

  if (framesPresented == 0U) {
    submit.outcome = iggy3d::RenderOutcome::Ok;
    submit.reason = {"null_renderer_ok", "null renderer ok"};
    iggy3d::appendReceiptField(submit.receipt, "receipt_version", "1");
    iggy3d::appendReceiptField(submit.receipt, "repo", "iggy3d");
    iggy3d::appendReceiptField(submit.receipt, "backend", "null");
    iggy3d::appendReceiptField(submit.receipt, "draw_count", static_cast<std::uint64_t>(0));
    iggy3d::appendReceiptField(submit.receipt, "frame_input_valid", "unavailable");
    iggy3d::appendReceiptField(submit.receipt, "result", "pass");
    iggy3d::appendReceiptField(submit.receipt, "reason_code", "null_renderer_ok");
  }

  iggy3d::RenderReceipt receipt = submit.receipt;
  iggy3d::appendReceiptField(receipt, "app", "iggy3d_visual_demo");
  iggy3d::appendReceiptField(receipt, "visual_demo", "bounded");
  appendWindowReceiptFields(receipt, windowFields);
  appendVulkanBootFields(receipt, rendererCreate.backend, vulkanSurfaceCreated, vulkanSwapchainReady,
                         rendererLifecycleForReceipt);
  appendPlayableReceiptFields(receipt, playableFields);
  iggy3d::appendReceiptField(receipt, "package_mode", iggy3d::packageModeName(lookup.lookup.packageMode));
  iggy3d::appendReceiptField(receipt, "resource_root_source", lookup.lookup.resourceRootSource);
  iggy3d::appendReceiptField(receipt, "shader_root_source", lookup.lookup.shaderRootSource);
  iggy3d::appendReceiptField(receipt, "diagnostics_dir_source", lookup.lookup.diagnosticsDirSource);
  iggy3d::appendReceiptField(receipt, "frames",
                             static_cast<std::uint64_t>(
                                 receiptFrameCount(parsed.options, framesPresented)));
  iggy3d::appendReceiptField(receipt, "frames_presented",
                             static_cast<std::uint64_t>(framesPresented));
  if (!iggy3d::hasReceiptField(receipt, "reason_code")) {
    iggy3d::appendReceiptField(receipt, "reason_code", "visual_demo_ok");
  }

  if (parsed.options.printReceipt) {
    std::cout << iggy3d::formatRenderReceipt(receipt);
  } else {
    std::cout << "visual_demo.status=ok\n";
    std::cout << "reason_code=visual_demo_ok\n";
  }
  return 0;
}
