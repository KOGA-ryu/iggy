#include "app/iggy3d/AppShell.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "app/PackageRuntimeLookup.hpp"
#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/ProductAppOperations.hpp"
#include "app/iggy3d/ProductCameraController.hpp"
#include "app/iggy3d/ProductAppOptions.hpp"
#include "app/iggy3d/ProductActiveRoomCollision.hpp"
#include "app/iggy3d/ProductAsciiRoomActivation.hpp"
#include "app/iggy3d/ProductAsciiRoomPreview.hpp"
#include "app/iggy3d/product/Automation.hpp"
#include "app/iggy3d/ProductBuiltinDungeon.hpp"
#include "app/iggy3d/ProductDungeonDraft.hpp"
#include "app/iggy3d/ProductGameplayController.hpp"
#include "app/iggy3d/ProductGameplayFeedback.hpp"
#include "app/iggy3d/ProductGameplayTape.hpp"
#include "app/iggy3d/ProductGameplayTapeRunner.hpp"
#include "app/iggy3d/ProductMenuTransitions.hpp"
#include "app/iggy3d/ProductMovementDebugHud.hpp"
#include "app/iggy3d/ProductNpcBehaviorDebugHud.hpp"
#include "app/iggy3d/ProductPrimitiveDrawList.hpp"
#include "app/iggy3d/ProductRenderBridge.hpp"
#include "app/iggy3d/ProductRoomEditorActionController.hpp"
#include "app/iggy3d/ProductRoomEditorCursor.hpp"
#include "app/iggy3d/ProductRoomEditorOverlay.hpp"
#include "app/iggy3d/ProductRoomEditingState.hpp"
#include "app/iggy3d/ProductScriptedGameplayDriver.hpp"
#include "app/iggy3d/ProductSaveFlow.hpp"
#include "app/iggy3d/ProductViewportFraming.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/SaveBridge.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputRouter.hpp"
#include "app/input/GamepadInput.hpp"
#include "app/input/KeyboardInput.hpp"
#include "app/input/MouseInput.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"
#include "render/RenderDiagnostics.hpp"
#include "render/RendererApi.hpp"
#include "render/vulkan/BufferImageResources.hpp"
#include "runtime/ai/NpcBehaviorDebugSnapshot.hpp"
#include "runtime/ai/NpcBehaviorProfile.hpp"
#include "runtime/session/Session.hpp"

#if defined(IGGY3D_HAS_SDL3)
#include <chrono>
#include <thread>

#include <SDL3/SDL.h>

#include "app/iggy3d/OpeningMenuView.hpp"
#include "app/platform/SdlWindow.hpp"
#endif

#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_APP_VULKAN_BACKEND)
#include "app/platform/SdlVulkanSurface.hpp"
#include "render/vulkan/VulkanBackend.hpp"
#endif

namespace iggy3d {

namespace {

void recordProductRoomEditingStart(ProductAppWindowState& window,
                                   const ProductRoomEditingStartResult& result,
                                   std::string_view operation);

FrontendInputBackend settingsInputBackendFromOptions(ProductInputBackend backend) {
  switch (backend) {
    case ProductInputBackend::Keyboard:
      return FrontendInputBackend::Keyboard;
    case ProductInputBackend::Gamepad:
      return FrontendInputBackend::Gamepad;
    case ProductInputBackend::Auto:
      return FrontendInputBackend::Auto;
  }
  return FrontendInputBackend::Auto;
}

FrontendRendererChoice settingsRendererFromOptions(ProductRendererRequest renderer) {
  switch (renderer) {
    case ProductRendererRequest::Null:
      return FrontendRendererChoice::Null;
    case ProductRendererRequest::Vulkan:
      return FrontendRendererChoice::Vulkan;
  }
  return FrontendRendererChoice::Null;
}

FrontendWindowMode settingsWindowModeFromOptions(ProductWindowMode mode) {
  switch (mode) {
    case ProductWindowMode::NoWindow:
      return FrontendWindowMode::NoWindow;
    case ProductWindowMode::Window:
      return FrontendWindowMode::Window;
  }
  return FrontendWindowMode::NoWindow;
}

FrontendSettings productFrontendSettingsFromOptions(const ProductAppOptions& options) {
  FrontendSettings settings = defaultFrontendSettings();
  settings.inputBackend = settingsInputBackendFromOptions(options.inputBackend);
  settings.renderer = settingsRendererFromOptions(options.renderer);
  settings.windowMode = settingsWindowModeFromOptions(options.windowMode);
  settings.cameraMode = FrontendCameraMode::FirstPerson;
  settings.devToolsEnabled = true;
  settings.debugOverlayEnabled = true;
  return settings;
}

DebugProjectionResult buildProductDebugProjectionWithNpcBehavior(
    const SessionState& state) {
  DebugProjectionResult debug = buildDebugProjection(state);
  const NpcBehaviorProfileCatalog catalog =
      makeBuiltInNpcBehaviorProfileCatalog();
  const NpcBehaviorDebugSnapshot snapshot = buildNpcBehaviorDebugSnapshot(
      {true, &state.world, &state.ai, &state.combat, &catalog,
       state.clock.tickIndex, 128});
  appendNpcBehaviorDebugSnapshot(debug, snapshot);
  return debug;
}

bool npcBehaviorHudHasUnresolvedProfile(const ProductNpcBehaviorDebugHud& hud) {
  for (const ProductNpcBehaviorDebugHudLine& line : hud.lines) {
    if (line.text.find("unresolved=") != std::string::npos) {
      return true;
    }
  }
  return false;
}

void copyNpcBehaviorDebugHud(ProductAppWindowState& window,
                             const ProductNpcBehaviorDebugHud& hud) {
  window.npcBehaviorDebugHudVisible = hud.visible;
  window.npcBehaviorDebugHudDebugAvailable = hud.debugAvailable;
  window.npcBehaviorDebugHudLineCount = static_cast<std::uint64_t>(hud.lineCount);
  window.npcBehaviorDebugHudStatus = hud.status;
  window.npcBehaviorDebugHudReasonCode = hud.reasonCode;
  window.npcBehaviorDebugHudHasUnresolvedProfile =
      npcBehaviorHudHasUnresolvedProfile(hud);
}

void clearProductVulkanRoomMeshProof(ProductViewportState& viewport) {
  viewport.productVulkanRoomMeshCpuReady = false;
  viewport.productVulkanRoomMeshBackendPresented = false;
  viewport.productVulkanRoomMeshSource = "none";
  viewport.productVulkanRoomAssetId = "none";
  viewport.productVulkanRoomFloorVisible = false;
  viewport.productVulkanRoomWallVisible = false;
  viewport.productVulkanRoomGridVisible = false;
  viewport.productVulkanRoomSourceMeshCount = 0;
  viewport.productVulkanRoomVertexCount = 0;
  viewport.productVulkanRoomIndexCount = 0;
  viewport.productVulkanRoomDrawCount = 0;
  viewport.productVulkanRoomFloorDrawCount = 0;
  viewport.productVulkanRoomWallDrawCount = 0;
  viewport.productVulkanRoomGridLineDrawCount = 0;
  viewport.productVulkanRoomGridTruncated = false;
  viewport.productVulkanRoomGeometrySignature = 0;
}

void applyProductVulkanRoomMeshProof(ProductViewportState& viewport,
                                     const SceneRoomProjection& room) {
  const bool backendPresented = viewport.productVulkanRoomMeshBackendPresented;
  clearProductVulkanRoomMeshProof(viewport);
  if (!room.loaded || room.meshes.empty()) {
    return;
  }
  const vulkan::RoomMeshCpuGeometry geometry =
      vulkan::buildRoomMeshCpuGeometry(room);
  viewport.productVulkanRoomMeshCpuReady = geometry.ready;
  viewport.productVulkanRoomMeshBackendPresented = backendPresented;
  viewport.productVulkanRoomMeshSource = "scene_room_projection";
  viewport.productVulkanRoomAssetId =
      geometry.sourceRoomAssetId.empty() ? "none" : geometry.sourceRoomAssetId;
  viewport.productVulkanRoomFloorVisible = room.floorVisible;
  viewport.productVulkanRoomWallVisible = room.wallVisible;
  viewport.productVulkanRoomGridVisible = geometry.roomGridVisible;
  viewport.productVulkanRoomSourceMeshCount =
      static_cast<std::uint64_t>(geometry.sourceRoomStaticMeshCount);
  viewport.productVulkanRoomVertexCount =
      static_cast<std::uint64_t>(geometry.vertices.size());
  viewport.productVulkanRoomIndexCount =
      static_cast<std::uint64_t>(geometry.indices.size());
  viewport.productVulkanRoomDrawCount =
      static_cast<std::uint64_t>(geometry.indexedDraws.size());
  viewport.productVulkanRoomFloorDrawCount =
      static_cast<std::uint64_t>(geometry.roomFloorDrawCount);
  viewport.productVulkanRoomWallDrawCount =
      static_cast<std::uint64_t>(geometry.roomWallDrawCount);
  viewport.productVulkanRoomGridLineDrawCount =
      static_cast<std::uint64_t>(geometry.roomGridLineDrawCount);
  viewport.productVulkanRoomGridTruncated = geometry.roomGridTruncated;
  viewport.productVulkanRoomGeometrySignature =
      geometry.sourceRoomGeometrySignature;
}

std::string receiptFieldValueOr(const RenderReceipt& receipt,
                                std::string_view key,
                                std::string_view fallback) {
  for (const RenderReceiptField& field : receipt.fields) {
    if (field.key == key) {
      return field.value;
    }
  }
  return std::string(fallback);
}

void recordProductVulkanRendererUnavailable(ProductAppWindowState& window,
                                            std::string_view reasonCode) {
  window.productVulkanRendererCreated = false;
  window.productVulkanRendererReady = false;
  window.productVulkanSurfaceCreated = false;
  window.productVulkanSwapchainReady = false;
  window.productVulkanStatus = "renderer_unavailable";
  window.productVulkanReasonCode = std::string(reasonCode);
}

void recordProductVulkanRendererReady(ProductAppWindowState& window,
                                      const RendererApi& renderer) {
  const RenderReceipt diagnostics = renderer.diagnostics();
  window.productVulkanRendererCreated = renderer.hasBackend();
  window.productVulkanRendererReady =
      renderer.lifecycleState() == RendererLifecycleState::Ready;
  window.productVulkanSurfaceCreated =
      window.productVulkanRendererReady ||
      hasReceiptField(diagnostics, "surface_ready", "true");
  window.productVulkanSwapchainReady =
      window.productVulkanRendererReady ||
      receiptFieldValueOr(diagnostics, "swapchain_state", "none") == "ready";
  window.productVulkanStatus = window.productVulkanRendererReady
                                   ? "renderer_ready"
                                   : "renderer_unavailable";
  window.productVulkanReasonCode =
      receiptFieldValueOr(diagnostics, "reason_code", "renderer_unavailable");
}

void recordProductVulkanSubmit(ProductAppWindowState& window,
                               const RenderSubmitResult& submit) {
  window.productVulkanRenderingPath =
      receiptFieldValueOr(submit.receipt, "rendering_path", "none");
  window.productVulkanRecordMode =
      receiptFieldValueOr(submit.receipt, "record_mode", "none");
  window.productVulkanReasonCode = std::string(submit.reason.code);
  if (submit.outcome == RenderOutcome::Ok) {
    window.productVulkanSurfaceCreated = true;
    window.productVulkanSwapchainReady = true;
    window.productVulkanFrameSubmitted = true;
    ++window.productVulkanFrameSubmittedCount;
    window.productVulkanStatus = "frame_submitted";
    if (window.productVulkanRenderingPath == "package_room_meshes" &&
        window.productVulkanRecordMode == "room_mesh_draws") {
      window.viewport.productVulkanRoomMeshBackendPresented = true;
    }
  } else {
    window.productVulkanStatus = "frame_not_submitted";
  }
}

Vec3 crossProduct(Vec3 lhs, Vec3 rhs) {
  return {lhs.y * rhs.z - lhs.z * rhs.y,
          lhs.z * rhs.x - lhs.x * rhs.z,
          lhs.x * rhs.y - lhs.y * rhs.x};
}

Vec3 normalizedOr(Vec3 value, Vec3 fallback) {
  const float len2 = lengthSquared(value);
  if (!std::isfinite(len2) || len2 <= 0.000001F) {
    return fallback;
  }
  return value / std::sqrt(len2);
}

Mat4 productPerspectiveMat4(float verticalFovRadians,
                            float aspect,
                            float nearPlane,
                            float farPlane) {
  const float f = 1.0F / std::tan(verticalFovRadians * 0.5F);
  Mat4 result{{{}}};
  result.m[0] = f / aspect;
  result.m[5] = -f;
  result.m[10] = farPlane / (nearPlane - farPlane);
  result.m[11] = -(farPlane * nearPlane) / (farPlane - nearPlane);
  result.m[14] = -1.0F;
  return result;
}

Mat4 productViewFromCamera(Vec3 eye, Vec3 forward, Vec3 up) {
  const Vec3 f = normalizedOr(forward, {0.0F, 0.0F, -1.0F});
  const Vec3 r = normalizedOr(crossProduct(f, up), {1.0F, 0.0F, 0.0F});
  const Vec3 u = crossProduct(r, f);
  Mat4 result = identityMat4();
  result.m[0] = r.x;
  result.m[1] = r.y;
  result.m[2] = r.z;
  result.m[3] = -dot(r, eye);
  result.m[4] = u.x;
  result.m[5] = u.y;
  result.m[6] = u.z;
  result.m[7] = -dot(u, eye);
  result.m[8] = -f.x;
  result.m[9] = -f.y;
  result.m[10] = -f.z;
  result.m[11] = dot(f, eye);
  return result;
}

FrameInput makeProductVulkanFrame(const SceneProjectionResult& scene,
                                  const DebugProjectionResult& debug,
                                  std::uint64_t frameIndex,
                                  std::uint32_t viewportWidth,
                                  std::uint32_t viewportHeight,
                                  float cameraYawDegrees,
                                  float cameraPitchDegrees) {
  constexpr float kPi = 3.14159265358979323846F;
  constexpr float kEyeHeightMeters = 1.7F;
  FrameInput frame;
  frame.viewport = {viewportWidth, viewportHeight,
                    static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight)};
  frame.clock = {scene.sourceTick, frameIndex, 0.0F, 1.0F / 60.0F};
  frame.camera.mode = RenderCameraMode::FirstPerson;
  Vec3 eye{0.0F, kEyeHeightMeters, 0.0F};
  for (const SceneItem& item : scene.items) {
    if (item.kind == SceneItemKind::Player || item.stableName == "player") {
      eye = item.transform.position + Vec3{0.0F, kEyeHeightMeters, 0.0F};
      break;
    }
  }
  const float yaw = cameraYawDegrees * kPi / 180.0F;
  const float pitch = cameraPitchDegrees * kPi / 180.0F;
  const float cosPitch = std::cos(pitch);
  frame.camera.worldEye = eye;
  frame.camera.worldForward = {std::sin(yaw) * cosPitch, std::sin(pitch),
                               -std::cos(yaw) * cosPitch};
  frame.camera.worldUp = {0.0F, 1.0F, 0.0F};
  frame.camera.nearPlane = 0.1F;
  frame.camera.farPlane = 200.0F;
  frame.camera.viewFromWorld =
      productViewFromCamera(frame.camera.worldEye, frame.camera.worldForward,
                            frame.camera.worldUp);
  frame.camera.clipFromView =
      productPerspectiveMat4(68.0F * kPi / 180.0F, frame.viewport.aspectRatio,
                             frame.camera.nearPlane, frame.camera.farPlane);
  frame.camera.clipFromWorld = frame.camera.clipFromView * frame.camera.viewFromWorld;
  frame.projections.scene = &scene;
  frame.projections.debug = &debug;
  return frame;
}

RendererConfig makeProductVulkanRendererConfig() {
  PackageLookupConfig lookupConfig;
  lookupConfig.packageMode = PackageMode::BuildTreeProduct;
  lookupConfig.requireShaderRoot = true;
  lookupConfig.requireGraphicsRuntime = true;
  const PackageLookupResult lookup = resolvePackageRuntimeLookup(lookupConfig);

  RendererConfig config;
  config.renderer = RendererMode::Vulkan;
  config.rendererRequirement = RendererRequirement::Optional;
  config.allowSoftwareVulkan = true;
  if (lookup.outcome == RenderOutcome::Ok) {
    config.shaderRoot = lookup.lookup.shaderRoot;
    config.diagnosticsDir = lookup.lookup.diagnosticsDir;
  }
  return config;
}

void applyGameplayProjectionMetrics(ProductAppWindowState& window,
                                    const SceneProjectionResult* scene,
                                    const DebugProjectionResult* debug,
                                    const ProductPrimitiveDrawList* drawList,
                                    const ProductViewportFrame* frame,
                                    const ProductRenderBridgeFrame* bridge,
                                    bool viewVisible) {
  if (!window.gameplayActive || scene == nullptr) {
    window.viewport.gameplayViewVisible = false;
    window.viewport.productDrawGridVisible = false;
    window.viewport.productDrawPlayerVisible = false;
    window.viewport.productDrawRoomVisible = false;
    window.viewport.productDrawObjectiveVisible = false;
    window.viewport.productDrawTargetIndicatorVisible = false;
    window.viewport.productDrawDoorVisible = false;
    window.viewport.productDrawOpenDoorVisible = false;
    window.viewport.productDrawClosedDoorVisible = false;
    window.viewport.productDrawItemCount = 0;
    window.viewport.productDrawDebugMarkerCount = 0;
    window.viewport.productDrawDoorCount = 0;
    window.viewport.productDrawOpenDoorCount = 0;
    window.viewport.productDrawClosedDoorCount = 0;
    window.viewport.productDrawRoomGeometryCount = 0;
    window.viewport.productDrawFloorTileCount = 0;
    window.viewport.productDrawElevatedFloorTileCount = 0;
    window.viewport.productDrawRampTileCount = 0;
    window.viewport.productDrawBlockedSlopeTileCount = 0;
    window.viewport.productDrawWallTileCount = 0;
    window.viewport.productDrawRoomEditorCursorVisible = false;
    window.viewport.productDrawRoomEditorCursorCount = 0;
    window.viewport.productViewProjection = "primitive_first_person";
    window.viewport.productViewYawApplied = false;
    window.viewport.productViewPitchApplied = false;
    window.viewport.productViewPlayerAnchorFound = false;
    window.viewport.productRenderBridgeReady = false;
    window.viewport.productViewFrameReady = false;
    window.viewport.productViewFrameItemCount = 0;
    window.viewport.productViewFrameOnScreenItemCount = 0;
    window.viewport.productViewFrameTargetItemCount = 0;
    window.viewport.productRenderBridgeRoomEditorCursorVisible = false;
    window.viewport.productRenderBridgeRoomEditorCursorCount = 0;
    window.viewport.productFeedbackBridgeReady = false;
    window.viewport.productFeedbackBridgeLineCount = 0;
    clearProductVulkanRoomMeshProof(window.viewport);
    window.sceneItemCount = 0;
    window.debugItemCount = 0;
    window.playerVisible = false;
    window.roomVisible = false;
    window.objectiveVisible = false;
    window.rendererMutatedRuntime = false;
    return;
  }

  window.viewport.gameplayViewVisible = viewVisible;
  window.sceneItemCount = static_cast<std::uint64_t>(scene->items.size());
  window.debugItemCount =
      debug == nullptr ? 0U : static_cast<std::uint64_t>(debug->items.size());
  window.playerVisible = scene->playerCount > 0;
  window.roomVisible = true;
  window.objectiveVisible =
      scene->pickupCount > 0 || scene->interactableCount > 0 || scene->markerCount > 0 ||
      scene->room.loaded;
  window.rendererMutatedRuntime = false;
  applyProductVulkanRoomMeshProof(window.viewport, scene->room);
  if (drawList != nullptr) {
    window.viewport.productDrawGridVisible = drawList->gridVisible;
    window.viewport.productDrawPlayerVisible = drawList->playerVisible;
    window.viewport.productDrawRoomVisible = drawList->roomVisible;
    window.viewport.productDrawObjectiveVisible = drawList->objectiveVisible;
    window.viewport.productDrawTargetIndicatorVisible =
        drawList->playerFocusIndicatorVisible;
    window.viewport.productDrawItemCount = drawList->itemCount;
    window.viewport.productDrawDebugMarkerCount = drawList->debugMarkerCount;
    window.viewport.productDrawDoorVisible = drawList->doorVisible;
    window.viewport.productDrawOpenDoorVisible = drawList->openDoorVisible;
    window.viewport.productDrawClosedDoorVisible = drawList->closedDoorVisible;
    window.viewport.productDrawDoorCount = drawList->doorMarkerCount;
    window.viewport.productDrawOpenDoorCount = drawList->openDoorMarkerCount;
    window.viewport.productDrawClosedDoorCount = drawList->closedDoorMarkerCount;
    window.viewport.productDrawRoomGeometryCount = drawList->roomGeometryCount;
    window.viewport.productDrawFloorTileCount = drawList->floorTileCount;
    window.viewport.productDrawElevatedFloorTileCount =
        drawList->elevatedFloorTileCount;
    window.viewport.productDrawRampTileCount = drawList->rampTileCount;
    window.viewport.productDrawBlockedSlopeTileCount =
        drawList->blockedSlopeTileCount;
    window.viewport.productDrawWallTileCount = drawList->wallTileCount;
    window.viewport.productDrawRoomEditorCursorVisible =
        drawList->roomEditorCursorVisible;
    window.viewport.productDrawRoomEditorCursorCount =
        drawList->roomEditorCursorCount;
  }
  if (frame != nullptr) {
    window.viewport.productViewProjection = frame->projectionMode;
    window.viewport.productViewYawApplied = frame->yawApplied;
    window.viewport.productViewPitchApplied = frame->pitchApplied;
    window.viewport.productViewPlayerAnchorFound = frame->playerAnchorFound;
  }
  if (bridge != nullptr) {
    window.viewport.productRenderBridgeReady = bridge->ready;
    window.viewport.productViewFrameReady = bridge->viewFrameReady;
    window.viewport.productViewFrameItemCount = bridge->frameItemCount;
    window.viewport.productViewFrameOnScreenItemCount = bridge->onScreenItemCount;
    window.viewport.productViewFrameTargetItemCount = bridge->targetItemCount;
    window.viewport.productRenderBridgeRoomEditorCursorVisible =
        bridge->roomEditorCursorVisible;
    window.viewport.productRenderBridgeRoomEditorCursorCount =
        bridge->roomEditorCursorCount;
    window.viewport.productFeedbackBridgeReady = bridge->feedbackReady;
    window.viewport.productFeedbackBridgeLineCount = bridge->feedbackLineCount;
  }
}

std::string failedTapeStepReceiptValue(std::uint64_t stepIndex) {
  return stepIndex == 0U ? "none" : std::to_string(stepIndex);
}

void recordProductGameplayTapeParse(const ProductGameplayTapeParseResult& parsed,
                                    ProductAppWindowState& window) {
  window.gameplayTapeLoaded = parsed.ok;
  window.gameplayTapeStatus = parsed.status;
  window.gameplayTapeReasonCode = parsed.reasonCode;
  window.gameplayTapeLineCount = parsed.lineCount;
  window.gameplayTapeStepCount =
      static_cast<std::uint64_t>(parsed.tape.steps.size());
  window.gameplayTapeFailedStep = failedTapeStepReceiptValue(parsed.failedLine);
  window.gameplayTapeFailedSourceLine = parsed.failedLine;
  window.gameplayTapeFailedAction = "none";
  window.gameplayTapeFailedTarget = parsed.failedToken;
  window.gameplayTapeFailedRejection = "none";
}

void recordProductGameplayTapeRun(const ProductGameplayTapeRunResult& run,
                                  ProductAppWindowState& window) {
  window.gameplayTapeStatus = run.status;
  window.gameplayTapeReasonCode = run.reasonCode;
  window.gameplayTapeStepCount = run.stepCount;
  window.gameplayTapeExecutedStepCount = run.executedStepCount;
  window.gameplayTapeExpectedRejectedStepCount = run.expectedRejectedStepCount;
  window.gameplayTapeExpectedBlockedStepCount = run.expectedBlockedStepCount;
  window.gameplayTapeFailedStep = failedTapeStepReceiptValue(run.failedStepIndex);
  window.gameplayTapeFailedSourceLine = run.failedSourceLine;
  window.gameplayTapeFailedAction = run.failedAction;
  window.gameplayTapeFailedTarget = run.failedTarget;
  window.gameplayTapeFailedRejection = run.failedRejection;
  window.gameplayTapeFailedMovementBlock = run.failedMovementBlock;
  window.gameplayTapeLastAction = run.lastAction;
  window.gameplayTapeLastTarget = run.lastTarget;
  window.gameplayTapeLastMovementBlock = run.lastMovementBlock;
  window.gameplayTapeKeyCollected = run.keyCollected;
  window.gameplayTapeSecretDoorOpened = run.secretDoorOpened;
  window.gameplayTapeTreasureCollected = run.treasureCollected;
  window.gameplayTapeNpcTargetable = run.npcTargetable;
  window.gameplayTapeNpcDefeated = run.npcDefeated;
  window.gameplayTapeExitObjectiveComplete = run.exitObjectiveComplete;
  window.gameplayTapeLoopComplete = run.loopComplete;
  window.gameplayTapeAiCommandLogged = run.aiCommandLogged;
  window.gameplayTapeAiAttackLogged = run.aiAttackLogged;
  window.gameplayTapeAiWaitLogged = run.aiWaitLogged;
  window.gameplayTapeAiPlayerDamaged = run.aiPlayerDamaged;
  window.gameplayTapeAiPlayerHpBefore = run.aiPlayerHpBefore;
  window.gameplayTapeAiPlayerHpAfter = run.aiPlayerHpAfter;
  window.gameplayTapeAiActorId = run.aiActorId;
  window.gameplayTapeAiTargetId = run.aiTargetId;
  window.gameplayTapeAiBehavior = run.aiBehavior;
  window.gameplayTapeAiIntent = run.aiIntent;
  window.sessionOutcome = run.sessionOutcome;
  window.runtimeStateHash = run.runtimeStateHash;
  if (!run.ok) {
    window.status = "gameplay_tape_failed";
  }
}

void runProductGameplayTapeFromOptions(const ProductAppOptions& options,
                                       std::optional<Session>& activeSession,
                                       ProductAppWindowState& window) {
  if (options.gameplayTapePath.empty()) {
    return;
  }

  window.gameplayTapeRequested = true;
  window.gameplayTapePath = options.gameplayTapePath.generic_string();
  const ProductGameplayTapeParseResult parsed =
      loadProductGameplayTapeFile(options.gameplayTapePath);
  recordProductGameplayTapeParse(parsed, window);
  if (!parsed.ok) {
    window.status = "gameplay_tape_parse_failed";
    return;
  }

  const ProductGameplayTapeRunResult run = runProductGameplayTape(
      ProductGameplayTapeRunRequest{activeSession.has_value() ? &*activeSession : nullptr,
                                    &parsed.tape,
                                    productActiveRoomCollisionSurfaces(
                                        window.activeRoomCollision),
                                    &window.activeRoom,
                                    &window.activeRoomCollision});
  recordProductGameplayTapeRun(run, window);
}

void copyProductRoomEditorOverlay(ProductAppWindowState& window,
                                  const ProductRoomEditorOverlay& overlay) {
  window.roomEditorOverlayVisible = overlay.visible;
  window.roomEditorOverlayStatus = overlay.status;
  window.roomEditorOverlayReasonCode = overlay.reasonCode;
  window.roomEditorOverlayItemCount = overlay.itemCount;
  window.roomEditorOverlayWorldX = overlay.worldPosition.x;
  window.roomEditorOverlayWorldY = overlay.worldPosition.y;
  window.roomEditorOverlayWorldZ = overlay.worldPosition.z;
}

void refreshGameplayProjectionMetrics(const std::optional<Session>& activeSession,
                                      ProductAppWindowState& window,
                                      bool developerToolsEnabled,
                                      bool debugOverlayEnabled) {
  if (!window.gameplayActive || !activeSession.has_value()) {
    window.sessionOutcome = "None";
    copyProductRoomEditorOverlay(
        window, buildProductRoomEditorOverlay(window.roomEditorCursor, false));
    copyNpcBehaviorDebugHud(window,
                            buildProductNpcBehaviorDebugHud(nullptr,
                                                            window.gameplayActive,
                                                            developerToolsEnabled,
                                                            debugOverlayEnabled));
    applyGameplayProjectionMetrics(window, nullptr, nullptr, nullptr, nullptr, nullptr,
                                   false);
    return;
  }

  const RoomAsset* activeRoom =
      window.activeRoom.loaded ? &window.activeRoom.room : nullptr;
  const SceneProjectionResult scene =
      buildSceneProjection(activeSession->state(), activeRoom);
  const DebugProjectionResult debug =
      buildProductDebugProjectionWithNpcBehavior(activeSession->state());
  copyNpcBehaviorDebugHud(window,
                          buildProductNpcBehaviorDebugHud(&debug,
                                                          window.gameplayActive,
                                                          developerToolsEnabled,
                                                          debugOverlayEnabled));
  const ProductRoomEditorOverlay roomEditorOverlay =
      buildProductRoomEditorOverlay(window.roomEditorCursor,
                                    window.roomEditing.ready);
  copyProductRoomEditorOverlay(window, roomEditorOverlay);
  const ProductPrimitiveDrawList drawList =
      buildProductPrimitiveDrawList(&scene, &debug, activeRoom,
                                    &window.activeRoomCollision,
                                    &roomEditorOverlay);
  const ProductViewportFrame frame = buildProductViewportFrame(
      drawList, ProductViewportFrameConfig{window.viewport.cameraYawDegrees,
                                           window.viewport.cameraPitchDegrees});
  const ProductGameplayFeedback feedback = buildProductGameplayFeedback(window);
  const ProductRenderBridgeFrame bridge =
      buildProductRenderBridgeFrame(&drawList, &frame, &feedback);
  window.runtimeStateHash = activeSession->stateHash();
  window.sessionOutcome =
      std::string(productGameplayTapeSessionOutcomeName(activeSession->state().outcome));
  applyGameplayProjectionMetrics(window, &scene, &debug, &drawList, &frame, &bridge,
                                 true);
}

FrontendAction nextStarterSelection(FrontendAction current, InputAction action) {
  const std::array<FrontendAction, 6> actions = {
      FrontendAction::Continue,
      FrontendAction::NewWorld,
      FrontendAction::LoadSave,
      FrontendAction::Settings,
      FrontendAction::DevTools,
      FrontendAction::Exit,
  };

  std::size_t index = 0;
  for (std::size_t i = 0; i < actions.size(); ++i) {
    if (actions[i] == current) {
      index = i;
      break;
    }
  }

  if (action == InputAction::MenuUp) {
    index = index == 0 ? actions.size() - 1 : index - 1;
  } else if (action == InputAction::MenuDown) {
    index = (index + 1) % actions.size();
  }
  return actions[index];
}

FrontendDevToolsCategory nextDevToolsSelection(FrontendDevToolsCategory current,
                                               InputAction action) {
  const auto& categories = devToolsCategoryOrder();
  std::size_t index = 0;
  for (std::size_t i = 0; i < categories.size(); ++i) {
    if (categories[i] == current) {
      index = i;
      break;
    }
  }
  if (action == InputAction::MenuUp) {
    index = index == 0 ? categories.size() - 1 : index - 1;
  } else if (action == InputAction::MenuDown) {
    index = (index + 1) % categories.size();
  }
  return categories[index];
}

FrontendSettingsTab nextSettingsSelection(FrontendSettingsTab current, InputAction action) {
  const auto& tabs = settingsTabOrder();
  std::size_t index = 0;
  for (std::size_t i = 0; i < tabs.size(); ++i) {
    if (tabs[i] == current) {
      index = i;
      break;
    }
  }
  if (action == InputAction::MenuUp) {
    index = index == 0 ? tabs.size() - 1 : index - 1;
  } else if (action == InputAction::MenuDown) {
    index = (index + 1) % tabs.size();
  }
  return tabs[index];
}

FrontendAction nextPauseSelection(FrontendAction current, InputAction action) {
  const auto& actions = pauseActionOrder();
  std::size_t index = 0;
  for (std::size_t i = 0; i < actions.size(); ++i) {
    if (actions[i] == current) {
      index = i;
      break;
    }
  }
  if (action == InputAction::MenuUp) {
    index = index == 0 ? actions.size() - 1 : index - 1;
  } else if (action == InputAction::MenuDown) {
    index = (index + 1) % actions.size();
  }
  return actions[index];
}

MenuOwner productInputOwnerFor(const FrontendState& frontend,
                               const ProductAppWindowState& window) {
  if (frontend.screen == FrontendScreen::Settings ||
      frontend.childScreen == FrontendScreen::Settings) {
    return MenuOwner::Settings;
  }
  if (frontend.screen == FrontendScreen::DevOverlay ||
      frontend.childScreen == FrontendScreen::StarterDevTools) {
    return MenuOwner::DevTools;
  }
  if (frontend.screen == FrontendScreen::Starter) {
    return MenuOwner::Starter;
  }
  if (frontend.screen == FrontendScreen::Pause) {
    return MenuOwner::Pause;
  }
  if (frontend.screen == FrontendScreen::Gameplay && window.gameplayActive) {
    if (window.roomEditing.ready) {
      return MenuOwner::Editor;
    }
    return MenuOwner::Gameplay;
  }
  return MenuOwner::None;
}

void recordWorldSetupDraftState(const WorldSetupDraft& draft,
                                ProductAppWindowState& window) {
  window.worldSetupTitle = draft.worldName;
  window.worldSetupAsciiRoomEnabled = draft.asciiRoomEnabled;
  window.worldSetupAsciiRoomTextPresent = !draft.asciiRoomText.empty();
  window.worldSetupAsciiRoomId =
      draft.asciiRoomId.empty() ? "none" : draft.asciiRoomId;
  window.worldSetupAsciiRoomSourceName =
      draft.asciiRoomSourceName.empty() ? "none" : draft.asciiRoomSourceName;
  window.asciiRoomDraftText = draft.asciiRoomText;
  window.asciiRoomDraftRoomId =
      draft.asciiRoomId.empty() ? "ascii_preview" : draft.asciiRoomId;
  window.asciiRoomDraftSourceName =
      draft.asciiRoomSourceName.empty() ? "world_setup_ascii_room" :
                                          draft.asciiRoomSourceName;
  if (draft.asciiRoomEnabled && !draft.asciiRoomText.empty()) {
    buildProductAsciiRoomPreviewResult(window);
    return;
  }
  window.asciiRoomPreviewStatus = "not_requested";
  window.asciiRoomPreviewReasonCode = "not_requested";
  window.asciiRoomPreviewFailedStage = "not_started";
  window.asciiRoomPreviewRoomId = "none";
  window.asciiRoomPreviewSourceName = "none";
  window.asciiRoomPreviewReady = false;
  window.asciiRoomPreviewWidth = 0;
  window.asciiRoomPreviewHeight = 0;
  window.asciiRoomPreviewFloorCount = 0;
  window.asciiRoomPreviewWallCount = 0;
  window.asciiRoomPreviewMarkerCount = 0;
  window.asciiRoomPreviewElevatedFloorCount = 0;
  window.asciiRoomPreviewRampCount = 0;
  window.asciiRoomPreviewBlockedSlopeCount = 0;
  window.asciiRoomPreviewStaticMeshCount = 0;
  window.asciiRoomPreviewAnchorCount = 0;
  window.asciiRoomPreviewSpatialSurfaceCount = 0;
  window.asciiRoomPreviewAssetTextWritten = false;
  window.asciiRoomPreviewAssetTextBytes = 0;
}

ProductDungeonDraftCursor dungeonDraftCursorFromWindow(
    const ProductAppWindowState& window) {
  return ProductDungeonDraftCursor{
      static_cast<std::size_t>(window.worldSetupDungeonDraftCursorRow),
      static_cast<std::size_t>(window.worldSetupDungeonDraftCursorColumn),
  };
}

void recordDungeonDraftOperation(ProductAppWindowState& window,
                                 const ProductDungeonDraftOperationResult& result) {
  window.worldSetupDungeonDraftStatus = std::string(result.status);
  window.worldSetupDungeonDraftReasonCode = std::string(result.reasonCode);
  window.worldSetupDungeonDraftCursorRow =
      static_cast<std::uint64_t>(result.cursor.row);
  window.worldSetupDungeonDraftCursorColumn =
      static_cast<std::uint64_t>(result.cursor.column);
  window.worldSetupDungeonDraftLastGlyph =
      result.glyph == '\0' ? std::string{"none"} : std::string(1U, result.glyph);
  if (result.modified) {
    window.worldSetupDungeonDraftModified = true;
  }
}

void resetDungeonDraftWindowCursor(const WorldSetupDraft& draft,
                                   ProductAppWindowState& window) {
  const ProductDungeonDraftCursor cursor =
      clampProductDungeonDraftCursor(draft, ProductDungeonDraftCursor{});
  window.worldSetupDungeonDraftCursorRow = static_cast<std::uint64_t>(cursor.row);
  window.worldSetupDungeonDraftCursorColumn =
      static_cast<std::uint64_t>(cursor.column);
  window.worldSetupDungeonDraftLastGlyph = "none";
}

bool applyDungeonDraftPaintGlyph(WorldSetupDraft& worldSetupDraft,
                                 ProductAppWindowState& window,
                                 char glyph) {
  if (!window.worldSetupDungeonDraftEditMode) {
    window.worldSetupDungeonDraftStatus = "dungeon_draft_edit_mode_off";
    window.worldSetupDungeonDraftReasonCode = "dungeon_draft_edit_mode_off";
    return false;
  }
  ProductDungeonDraftOperationResult painted = paintProductDungeonDraftCell(
      worldSetupDraft, dungeonDraftCursorFromWindow(window), glyph);
  recordDungeonDraftOperation(window, painted);
  recordWorldSetupDraftState(worldSetupDraft, window);
  return painted.ok;
}

void applyOpeningMenuAction(FrontendState& frontend,
                            const ProductSaveBridgeResult& saves,
                            const ProductAppOptions& options,
                            FrontendSettingsTab& settingsTab,
                            std::optional<Session>& activeSession,
                            WorldSetupDraft& worldSetupDraft,
                            ProductAppWindowState& window,
                            InputAction action,
                            bool& closeRequested) {
  if (action == InputAction::SystemPause) {
    if (frontend.screen == FrontendScreen::Gameplay && window.gameplayActive) {
      openProductPauseTransition(frontend, window, FrontendAction::Resume);
      frontend.status = "pause_opened_from_gameplay";
      return;
    }
    if (frontend.screen == FrontendScreen::Pause) {
      closeProductOverlayToGameplayTransition(frontend, window);
      return;
    }
    if (frontend.screen == FrontendScreen::DevOverlay) {
      closeProductOverlayToGameplayTransition(frontend, window);
      return;
    }
    if (frontend.screen == FrontendScreen::Settings &&
        frontend.childScreen == FrontendScreen::Pause) {
      openProductPauseTransition(frontend, window, FrontendAction::Settings);
      return;
    }
    frontend.status = "opening_menu_pause_back_requested";
    closeRequested = true;
    return;
  }

  if (frontend.screen == FrontendScreen::Pause) {
    if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
      frontend.selectedAction = nextPauseSelection(frontend.selectedAction, action);
      frontend.status = "pause_menu_selection_changed";
      return;
    }
    if (action == InputAction::MenuBack) {
      closeProductOverlayToGameplayTransition(frontend, window);
      return;
    }
    if (action != InputAction::MenuConfirm) {
      return;
    }
    if (frontend.selectedAction == FrontendAction::Resume) {
      closeProductOverlayToGameplayTransition(frontend, window);
      return;
    }
    if (frontend.selectedAction == FrontendAction::EditRoom) {
      const ProductRoomEditingStartResult started =
          startProductRoomAuthoringFromActiveRoom({window.activeRoom});
      recordProductRoomEditingStart(window, started, "pause_edit_room");
      frontend.status = started.ok ? "pause_edit_room_requested"
                                   : "pause_edit_room_failed";
      if (started.ok) {
        closeProductOverlayToGameplayTransition(frontend, window);
      }
      return;
    }
    if (frontend.selectedAction == FrontendAction::Settings) {
      openProductPauseSettingsTransition(frontend, window, settingsTab);
      return;
    }
    if (frontend.selectedAction == FrontendAction::DevTools) {
      openProductPauseDevToolsTransition(frontend, window,
                                         FrontendDevToolsCategory::Session);
      return;
    }
    if (frontend.selectedAction == FrontendAction::Save) {
      executeProductPauseSaveFlow(ProductPauseSaveFlowKind::Save, options,
                                  frontend, activeSession, window);
      return;
    }
    if (frontend.selectedAction == FrontendAction::SaveAndExit) {
      executeProductPauseSaveFlow(ProductPauseSaveFlowKind::SaveAndExit, options,
                                  frontend, activeSession, window);
      return;
    }
    if (frontend.selectedAction == FrontendAction::ReturnToTitle) {
      returnProductToTitleTransition(frontend, window);
      activeSession.reset();
      return;
    }
    if (frontend.selectedAction == FrontendAction::ExitGame) {
      frontend.status = "pause_exit_game_requested";
      closeRequested = true;
      return;
    }
    frontend.status = "pause_action_selected";
    return;
  }

  if (frontend.screen == FrontendScreen::DevOverlay) {
    if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
      frontend.devToolsCategory = nextDevToolsSelection(frontend.devToolsCategory, action);
      frontend.status = "dev_overlay_selection_changed";
      return;
    }
    if (action == InputAction::MenuBack) {
      closeProductOverlayToGameplayTransition(frontend, window);
      return;
    }
    if (action == InputAction::MenuConfirm) {
      frontend.status = "dev_overlay_category_selected";
      return;
    }
  }

  if (frontend.childScreen == FrontendScreen::StarterDevTools) {
    if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
      frontend.devToolsCategory = nextDevToolsSelection(frontend.devToolsCategory, action);
      frontend.status = "dev_tools_selection_changed";
      return;
    }
    if (action == InputAction::MenuBack) {
      frontend.childScreen = FrontendScreen::Gameplay;
      frontend.devToolsOpen = false;
      frontend.status = "dev_tools_closed";
      return;
    }
    if (action == InputAction::MenuConfirm) {
      frontend.status = "dev_tools_category_selected";
      return;
    }
  }

  if (frontend.childScreen == FrontendScreen::Settings) {
    if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
      settingsTab = nextSettingsSelection(settingsTab, action);
      frontend.status = "settings_selection_changed";
      return;
    }
    if (action == InputAction::MenuBack) {
      if (frontend.screen == FrontendScreen::Settings &&
          frontend.childScreen == FrontendScreen::Pause) {
        openProductPauseTransition(frontend, window, FrontendAction::Settings);
      } else {
        frontend.childScreen = FrontendScreen::Gameplay;
      }
      frontend.status = "settings_closed";
      return;
    }
    if (action == InputAction::MenuConfirm) {
      frontend.status = "settings_tab_selected";
      return;
    }
  }

  if (frontend.childScreen == FrontendScreen::DeleteConfirm &&
      window.saveDeleteConfirmationOpen) {
    if (action == InputAction::MenuBack) {
      cancelProductSaveDeleteConfirmation(window, frontend);
      return;
    }
    if (action == InputAction::MenuConfirm) {
      executeProductSaveSoftDelete(options, window, frontend);
      return;
    }
  }

  if (frontend.childScreen == FrontendScreen::NewWorld) {
    recordWorldSetupDraftState(worldSetupDraft, window);
    if (action == InputAction::MenuNextTab) {
      window.worldSetupDungeonDraftEditMode =
          !window.worldSetupDungeonDraftEditMode;
      worldSetupDraft.selectedField =
          window.worldSetupDungeonDraftEditMode ? WorldSetupField::AsciiRoom
                                                : WorldSetupField::Create;
      window.worldSetupDungeonDraftStatus =
          window.worldSetupDungeonDraftEditMode ? "dungeon_draft_edit_mode_on"
                                                : "dungeon_draft_edit_mode_off";
      window.worldSetupDungeonDraftReasonCode = window.worldSetupDungeonDraftStatus;
      resetDungeonDraftWindowCursor(worldSetupDraft, window);
      frontend.status = window.worldSetupDungeonDraftStatus;
      return;
    }
    if (window.worldSetupDungeonDraftEditMode &&
        (action == InputAction::MenuUp || action == InputAction::MenuDown ||
         action == InputAction::MenuLeft || action == InputAction::MenuRight)) {
      ProductDungeonDraftDirection direction = ProductDungeonDraftDirection::Up;
      if (action == InputAction::MenuDown) {
        direction = ProductDungeonDraftDirection::Down;
      } else if (action == InputAction::MenuLeft) {
        direction = ProductDungeonDraftDirection::Left;
      } else if (action == InputAction::MenuRight) {
        direction = ProductDungeonDraftDirection::Right;
      }
      const ProductDungeonDraftOperationResult moved =
          moveProductDungeonDraftCursor(worldSetupDraft,
                                        dungeonDraftCursorFromWindow(window),
                                        direction);
      recordDungeonDraftOperation(window, moved);
      frontend.status = moved.status;
      return;
    }
    if (!window.worldSetupDungeonDraftEditMode &&
        (action == InputAction::MenuUp || action == InputAction::MenuDown ||
         action == InputAction::MenuLeft || action == InputAction::MenuRight)) {
      const bool previous =
          action == InputAction::MenuUp || action == InputAction::MenuLeft;
      const bool changed = previous
                               ? selectPreviousProductBuiltinDungeon(worldSetupDraft)
                               : selectNextProductBuiltinDungeon(worldSetupDraft);
      window.worldSetupDungeonDraftModified = false;
      window.worldSetupDungeonDraftEditMode = false;
      resetDungeonDraftWindowCursor(worldSetupDraft, window);
      recordWorldSetupDraftState(worldSetupDraft, window);
      frontend.status =
          changed ? "new_world_dungeon_selection_changed" : "new_world_input_ignored";
      window.worldSetupStatus =
          changed ? "world_setup_dungeon_selected" : "world_setup_dungeon_unavailable";
      return;
    }
    if (action == InputAction::MenuBack) {
      frontend.childScreen = FrontendScreen::Gameplay;
      frontend.selectedAction = FrontendAction::NewWorld;
      frontend.status = "new_world_closed";
      window.worldSetupStatus = "world_setup_back";
      return;
    }
    if (action == InputAction::MenuConfirm) {
      launchProductNewWorld(options, worldSetupDraft, frontend, activeSession, window);
      return;
    }
    frontend.status = "new_world_input_ignored";
    return;
  }

  if (frontend.childScreen == FrontendScreen::LoadSave) {
    if (action == InputAction::MenuBack) {
      frontend.childScreen = FrontendScreen::Gameplay;
      frontend.status = "load_save_closed";
      return;
    }
    if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
      moveSelectedProductSaveSlot(saves.slots, action, window);
      frontend.status = "load_save_selection_changed";
      return;
    }
    if (action == InputAction::MenuConfirm) {
      if (frontend.selectedAction == FrontendAction::Delete) {
        openProductSaveDeleteConfirmation(saves.slots, window, frontend);
      } else {
        launchProductLoadSaveSelection(options,
                                       productWorldTemplateFromOptions(options),
                                       saves,
                                       frontend,
                                       activeSession,
                                       window);
      }
      return;
    }
  }

  if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
    frontend.selectedAction = nextStarterSelection(frontend.selectedAction, action);
    frontend.status = "opening_menu_selection_changed";
    return;
  }
  if (action == InputAction::MenuBack) {
    frontend.status = "opening_menu_back_requested";
    closeRequested = true;
    return;
  }
  if (action != InputAction::MenuConfirm) {
    return;
  }

  if (frontend.selectedAction == FrontendAction::Continue) {
    if (saves.slots.compatibleCount == 0) {
      frontend.status = "opening_menu_action_disabled";
      return;
    }
    launchProductContinueSave(options, productWorldTemplateFromOptions(options), saves,
                              frontend, activeSession, window);
    return;
  }
  if (frontend.selectedAction == FrontendAction::Exit) {
    frontend.status = "opening_menu_exit_requested";
    closeRequested = true;
    return;
  }
  if (frontend.selectedAction == FrontendAction::NewWorld) {
    frontend.childScreen = FrontendScreen::NewWorld;
    frontend.selectedAction = FrontendAction::CreateAndEnter;
    frontend.status = "opening_menu_new_world_selected";
    recordWorldSetupDraftState(worldSetupDraft, window);
    window.worldSetupStatus = "world_setup_open";
    return;
  }
  if (frontend.selectedAction == FrontendAction::LoadSave) {
    frontend.childScreen = FrontendScreen::LoadSave;
    initializeSelectedProductSaveSlot(saves.slots, window);
    frontend.status = "opening_menu_load_save_selected";
    return;
  }
  if (frontend.selectedAction == FrontendAction::Settings) {
    frontend.childScreen = FrontendScreen::Settings;
    settingsTab = FrontendSettingsTab::Input;
    frontend.status = "opening_menu_settings_selected";
    return;
  }
  if (frontend.selectedAction == FrontendAction::DevTools) {
    frontend.childScreen = FrontendScreen::StarterDevTools;
    frontend.devToolsOpen = true;
    frontend.devToolsCategory = FrontendDevToolsCategory::Session;
    frontend.status = "opening_menu_dev_tools_selected";
    return;
  }
  frontend.status = "opening_menu_action_selected";
}

void routeOpeningMenuInput(FrontendState& frontend,
                           const ProductSaveBridgeResult& saves,
                           const ProductAppOptions& options,
                           FrontendSettingsTab& settingsTab,
                           std::optional<Session>& activeSession,
                           WorldSetupDraft& worldSetupDraft,
                           ActionState& actionState,
                           InputAction inputAction,
                           ProductAppWindowState& window,
                           bool& closeRequested) {
  if (inputAction == InputAction::None) {
    return;
  }

  if (frontend.screen == FrontendScreen::Gameplay &&
      inputAction == InputAction::MenuBack) {
    inputAction = InputAction::SystemPause;
  }

  recordAction(actionState, inputAction, true, true, false, 1.0F);

  InputRoutingContext routingContext;
  routingContext.owners.starter = frontend.screen == FrontendScreen::Starter;
  routingContext.owners.pause = frontend.screen == FrontendScreen::Pause;
  routingContext.owners.settings = frontend.screen == FrontendScreen::Settings ||
                                   frontend.childScreen == FrontendScreen::Settings;
  routingContext.owners.devTools = frontend.screen == FrontendScreen::DevOverlay ||
                                   frontend.childScreen == FrontendScreen::StarterDevTools;
  routingContext.owners.gameplay = frontend.screen == FrontendScreen::Gameplay &&
                                   window.gameplayActive;
  const InputRoutingResult routed = routeInputAction(routingContext, inputAction);
  window.inputOwner = routed.owner;
  window.lastInputAction = routed.action;
  window.lastInputAccepted = routed.accepted;
  window.gameplayInputSuppressed = routed.gameplaySuppressed;
  if (routed.accepted) {
    applyOpeningMenuAction(frontend, saves, options, settingsTab, activeSession,
                           worldSetupDraft, window, routed.action, closeRequested);
  }
}

struct ProductAutomationCommand {
  std::string key;
  std::string value;
};

bool parseAutomationBool(std::string_view value, bool& out) {
  if (value == "true" || value == "1" || value == "yes") {
    out = true;
    return true;
  }
  if (value == "false" || value == "0" || value == "no") {
    out = false;
    return true;
  }
  return false;
}

bool parseAutomationFloat(std::string_view value, float& out) {
  if (value.empty()) {
    return false;
  }
  const auto [ptr, error] =
      std::from_chars(value.data(), value.data() + value.size(), out);
  return error == std::errc{} && ptr == value.data() + value.size() &&
         std::isfinite(out);
}

bool parseAutomationSize(std::string_view value, std::size_t& out) {
  if (value.empty()) {
    return false;
  }
  const auto [ptr, error] =
      std::from_chars(value.data(), value.data() + value.size(), out);
  return error == std::errc{} && ptr == value.data() + value.size();
}

std::vector<std::string_view> splitAutomationCsv(std::string_view value) {
  std::vector<std::string_view> fields;
  std::size_t start = 0;
  while (start <= value.size()) {
    const std::size_t comma = value.find(',', start);
    if (comma == std::string_view::npos) {
      fields.push_back(value.substr(start));
      break;
    }
    fields.push_back(value.substr(start, comma - start));
    start = comma + 1U;
  }
  return fields;
}

bool parseAutomationCsvFloat(std::string_view value, float& out) {
  return parseAutomationFloat(value, out);
}

bool parseAutomationFloorCommand(std::string_view value,
                                 RoomEditCommand& command) {
  const std::vector<std::string_view> fields = splitAutomationCsv(value);
  if (fields.size() != 7U && fields.size() != 8U) {
    return false;
  }

  EditableRoomFloor floor;
  floor.id = std::string(fields[0]);
  if (floor.id.empty() ||
      !parseAutomationCsvFloat(fields[1], floor.centerMeters.x) ||
      !parseAutomationCsvFloat(fields[2], floor.centerMeters.y) ||
      !parseAutomationCsvFloat(fields[3], floor.centerMeters.z) ||
      !parseAutomationCsvFloat(fields[4], floor.sizeMeters.x) ||
      !parseAutomationCsvFloat(fields[5], floor.sizeMeters.y) ||
      !parseAutomationCsvFloat(fields[6], floor.sizeMeters.z)) {
    return false;
  }
  floor.semantics = defaultFloorSemantics(
      fields.size() == 8U && !fields[7].empty()
          ? std::string(fields[7])
          : std::string{"debug_floor"});
  command = addFloorCommand(std::move(floor));
  return true;
}

bool parseAutomationWallCommand(std::string_view value,
                                RoomEditCommand& command) {
  const std::vector<std::string_view> fields = splitAutomationCsv(value);
  if (fields.size() != 10U && fields.size() != 11U) {
    return false;
  }

  EditableRoomWall wall;
  wall.id = std::string(fields[0]);
  if (wall.id.empty() ||
      !parseAutomationCsvFloat(fields[1], wall.startMeters.x) ||
      !parseAutomationCsvFloat(fields[2], wall.startMeters.y) ||
      !parseAutomationCsvFloat(fields[3], wall.startMeters.z) ||
      !parseAutomationCsvFloat(fields[4], wall.endMeters.x) ||
      !parseAutomationCsvFloat(fields[5], wall.endMeters.y) ||
      !parseAutomationCsvFloat(fields[6], wall.endMeters.z) ||
      !parseAutomationCsvFloat(fields[7], wall.bottomY) ||
      !parseAutomationCsvFloat(fields[8], wall.heightMeters) ||
      !parseAutomationCsvFloat(fields[9], wall.thicknessMeters)) {
    return false;
  }
  wall.semantics = defaultWallSemantics(
      fields.size() == 11U && !fields[10].empty()
          ? std::string(fields[10])
          : std::string{"debug_wall"});
  command = addWallCommand(std::move(wall));
  return true;
}

template <typename Value>
struct AutomationParserRow {
  std::string_view name;
  Value value;
};

struct RoomEditorInputActionRow {
  std::string_view name;
  InputAction action;
  float value;
};

template <typename Value, std::size_t Count>
bool parseAutomationTableValue(
    std::string_view value,
    const std::array<AutomationParserRow<Value>, Count>& rows,
    Value& out) {
  const auto row = std::find_if(
      rows.begin(), rows.end(),
      [value](const AutomationParserRow<Value>& candidate) {
        return candidate.name == value;
      });
  if (row == rows.end()) {
    return false;
  }
  out = row->value;
  return true;
}

bool parseAutomationInputAction(std::string_view value, InputAction& out) {
  static constexpr std::array rows{
      AutomationParserRow<InputAction>{"up", InputAction::MenuUp},
      AutomationParserRow<InputAction>{"down", InputAction::MenuDown},
      AutomationParserRow<InputAction>{"left", InputAction::MenuLeft},
      AutomationParserRow<InputAction>{"right", InputAction::MenuRight},
      AutomationParserRow<InputAction>{"confirm", InputAction::MenuConfirm},
      AutomationParserRow<InputAction>{"back", InputAction::MenuBack},
      AutomationParserRow<InputAction>{"next_tab", InputAction::MenuNextTab},
      AutomationParserRow<InputAction>{"previous_tab",
                                       InputAction::MenuPreviousTab},
      AutomationParserRow<InputAction>{"none", InputAction::None},
  };
  return parseAutomationTableValue(value, rows, out);
}

bool parseProductRoomEditorDirection(std::string_view value,
                                     ProductRoomEditorDirection& out) {
  static constexpr std::array rows{
      AutomationParserRow<ProductRoomEditorDirection>{
          "up", ProductRoomEditorDirection::Up},
      AutomationParserRow<ProductRoomEditorDirection>{
          "down", ProductRoomEditorDirection::Down},
      AutomationParserRow<ProductRoomEditorDirection>{
          "left", ProductRoomEditorDirection::Left},
      AutomationParserRow<ProductRoomEditorDirection>{
          "right", ProductRoomEditorDirection::Right},
  };
  return parseAutomationTableValue(value, rows, out);
}

bool parseProductRoomEditorTool(std::string_view value,
                                ProductRoomEditorTool& out) {
  static constexpr std::array rows{
      AutomationParserRow<ProductRoomEditorTool>{
          "floor", ProductRoomEditorTool::Floor},
      AutomationParserRow<ProductRoomEditorTool>{
          "wall", ProductRoomEditorTool::Wall},
  };
  return parseAutomationTableValue(value, rows, out);
}

bool parseProductRoomEditorInputAction(std::string_view value,
                                       InputAction& out,
                                       float& actionValue) {
  static constexpr std::array rows{
      RoomEditorInputActionRow{"editor.nudge_x_pos",
                               InputAction::EditorNudgeX, 1.0F},
      RoomEditorInputActionRow{"editor.nudge_x_neg",
                               InputAction::EditorNudgeX, -1.0F},
      RoomEditorInputActionRow{"editor.nudge_z_pos",
                               InputAction::EditorNudgeZ, 1.0F},
      RoomEditorInputActionRow{"editor.nudge_z_neg",
                               InputAction::EditorNudgeZ, -1.0F},
      RoomEditorInputActionRow{"editor.next_tool",
                               InputAction::EditorNextTool, 1.0F},
      RoomEditorInputActionRow{"editor.previous_tool",
                               InputAction::EditorPreviousTool, 1.0F},
      RoomEditorInputActionRow{"editor.place", InputAction::EditorPlace, 1.0F},
      RoomEditorInputActionRow{"editor.apply", InputAction::EditorApply, 1.0F},
  };
  actionValue = 1.0F;
  const auto row = std::find_if(
      rows.begin(), rows.end(),
      [value](const RoomEditorInputActionRow& candidate) {
        return candidate.name == value;
      });
  if (row == rows.end()) {
    return false;
  }
  out = row->action;
  actionValue = row->value;
  return true;
}

bool parseAutomationFrontendAction(std::string_view value, FrontendAction& out) {
  static constexpr std::array rows{
      AutomationParserRow<FrontendAction>{"continue", FrontendAction::Continue},
      AutomationParserRow<FrontendAction>{"new_world", FrontendAction::NewWorld},
      AutomationParserRow<FrontendAction>{"load_save", FrontendAction::LoadSave},
      AutomationParserRow<FrontendAction>{"settings", FrontendAction::Settings},
      AutomationParserRow<FrontendAction>{"dev_tools", FrontendAction::DevTools},
      AutomationParserRow<FrontendAction>{"exit", FrontendAction::Exit},
      AutomationParserRow<FrontendAction>{"resume", FrontendAction::Resume},
      AutomationParserRow<FrontendAction>{"edit_room", FrontendAction::EditRoom},
      AutomationParserRow<FrontendAction>{"save", FrontendAction::Save},
      AutomationParserRow<FrontendAction>{"save_and_exit",
                                          FrontendAction::SaveAndExit},
      AutomationParserRow<FrontendAction>{"return_to_title",
                                          FrontendAction::ReturnToTitle},
      AutomationParserRow<FrontendAction>{"exit_game", FrontendAction::ExitGame},
  };
  return parseAutomationTableValue(value, rows, out);
}

bool parseAutomationSettingsTab(std::string_view value,
                                FrontendSettingsTab& out) {
  static constexpr std::array rows{
      AutomationParserRow<FrontendSettingsTab>{"input",
                                               FrontendSettingsTab::Input},
      AutomationParserRow<FrontendSettingsTab>{"controls",
                                               FrontendSettingsTab::Controls},
      AutomationParserRow<FrontendSettingsTab>{"camera",
                                               FrontendSettingsTab::Camera},
      AutomationParserRow<FrontendSettingsTab>{"gameplay",
                                               FrontendSettingsTab::Gameplay},
      AutomationParserRow<FrontendSettingsTab>{
          "video_display", FrontendSettingsTab::VideoDisplay},
      AutomationParserRow<FrontendSettingsTab>{"audio",
                                               FrontendSettingsTab::Audio},
      AutomationParserRow<FrontendSettingsTab>{
          "accessibility", FrontendSettingsTab::Accessibility},
      AutomationParserRow<FrontendSettingsTab>{"developer",
                                               FrontendSettingsTab::Developer},
  };
  return parseAutomationTableValue(value, rows, out);
}

bool parseAutomationDevToolsCategory(std::string_view value,
                                     FrontendDevToolsCategory& out) {
  static constexpr std::array rows{
      AutomationParserRow<FrontendDevToolsCategory>{
          "session", FrontendDevToolsCategory::Session},
      AutomationParserRow<FrontendDevToolsCategory>{
          "input", FrontendDevToolsCategory::Input},
      AutomationParserRow<FrontendDevToolsCategory>{
          "player", FrontendDevToolsCategory::Player},
      AutomationParserRow<FrontendDevToolsCategory>{
          "movement", FrontendDevToolsCategory::Movement},
      AutomationParserRow<FrontendDevToolsCategory>{
          "world_editor", FrontendDevToolsCategory::WorldEditor},
      AutomationParserRow<FrontendDevToolsCategory>{
          "collision", FrontendDevToolsCategory::Collision},
      AutomationParserRow<FrontendDevToolsCategory>{
          "spells", FrontendDevToolsCategory::Spells},
      AutomationParserRow<FrontendDevToolsCategory>{
          "camera", FrontendDevToolsCategory::Camera},
      AutomationParserRow<FrontendDevToolsCategory>{
          "renderer", FrontendDevToolsCategory::Renderer},
      AutomationParserRow<FrontendDevToolsCategory>{
          "performance", FrontendDevToolsCategory::Performance},
  };
  return parseAutomationTableValue(value, rows, out);
}

bool parseAutomationOwner(std::string_view value, MenuOwner& out) {
  static constexpr std::array rows{
      AutomationParserRow<MenuOwner>{"starter", MenuOwner::Starter},
      AutomationParserRow<MenuOwner>{"pause", MenuOwner::Pause},
      AutomationParserRow<MenuOwner>{"settings", MenuOwner::Settings},
      AutomationParserRow<MenuOwner>{"dev_tools", MenuOwner::DevTools},
  };
  return parseAutomationTableValue(value, rows, out);
}

bool hasAutomationKey(const std::vector<ProductAutomationCommand>& commands,
                      const std::string& key) {
  for (const ProductAutomationCommand& command : commands) {
    if (command.key == key) {
      return true;
    }
  }
  return false;
}

bool readProductAutomationCommands(const std::filesystem::path& path,
                                   ProductAppWindowState& window,
                                   std::vector<ProductAutomationCommand>& commands) {
  window.automationControlRequested = !path.empty();
  window.automationControlPath = path.empty() ? "" : path.generic_string();
  if (path.empty()) {
    return false;
  }
  window.automationControlScope = "frontend_menu";
  std::ifstream input(path);
  if (!input) {
    window.automationControlStatus = "read_failed";
    window.automationControlLastResult = "failed";
    return false;
  }

  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    ++window.automationControlLineCount;
    const std::size_t equals = line.find('=');
    if (equals == std::string::npos || equals == 0U) {
      window.automationControlStatus = "parse_error";
      window.automationControlLastKey = "none";
      window.automationControlLastResult = "failed";
      return false;
    }
    ProductAutomationCommand command{line.substr(0, equals), line.substr(equals + 1U)};
    if (hasAutomationKey(commands, command.key)) {
      window.automationControlStatus = "duplicate_key";
      window.automationControlLastKey = command.key;
      window.automationControlLastResult = "failed";
      return false;
    }
    commands.push_back(std::move(command));
  }
  window.automationControlLoaded = true;
  window.automationControlStatus = "loaded";
  return true;
}

void markAutomationApplied(ProductAppWindowState& window,
                           const ProductAutomationCommand& command,
                           std::string_view action,
                           MenuOwner owner,
                           std::string_view result) {
  window.automationControlLastKey = command.key;
  window.automationControlLastAction = std::string(action);
  window.automationControlLastOwner = owner;
  window.automationControlLastResult = std::string(result);
  if (result == "applied") {
    ++window.automationControlAppliedCount;
    window.automationControlStatus = "applied";
  }
}

bool automationFailurePreservesLoaded(std::string_view status) {
  return status == "applied" || status == "loaded" ||
         status == "command_failed";
}

bool routeAutomationInput(FrontendState& frontend,
                          const ProductSaveBridgeResult& saves,
                          const ProductAppOptions& options,
                          FrontendSettingsTab& settingsTab,
                          std::optional<Session>& activeSession,
                          WorldSetupDraft& worldSetupDraft,
                          ProductAppWindowState& window,
                          InputAction action,
                          bool& closeRequested) {
  ActionState actionState;
  routeOpeningMenuInput(frontend, saves, options, settingsTab, activeSession,
                        worldSetupDraft, actionState, action, window, closeRequested);
  window.automationControlLastOwner = productInputOwnerFor(frontend, window);
  return window.lastInputAccepted || action == InputAction::None;
}

bool applyAutomationGameplayAxis(InputAction action,
                                 float value,
                                 FrontendState& frontend,
                                 std::optional<Session>& activeSession,
                                 ProductAppWindowState& window) {
  if (frontend.screen != FrontendScreen::Gameplay || !window.gameplayActive ||
      !activeSession.has_value()) {
    window.automationControlStatus = "owner_unavailable";
    return false;
  }

  ActionState actions;
  recordAction(actions, action, true, false, false, value);

  InputRoutingContext routingContext;
  routingContext.owners.gameplay = true;
  const InputRoutingResult routed = routeInputAction(routingContext, action);
  window.inputOwner = routed.owner;
  window.lastInputAction = routed.action;
  window.lastInputAccepted = routed.accepted;
  window.gameplayInputSuppressed = routed.gameplaySuppressed;
  if (!routed.accepted) {
    return false;
  }

  applyProductGameplayActions(
      *activeSession, actions, window, "automation",
      productActiveRoomCollisionSurfaces(window.activeRoomCollision));
  return window.gameplayCommandSubmitted && window.gameplayCommandAccepted &&
         window.gameplayTickAdvanced;
}

bool applyAutomationGameplayButton(InputAction action,
                                   FrontendState& frontend,
                                   std::optional<Session>& activeSession,
                                   ProductAppWindowState& window) {
  if (frontend.screen != FrontendScreen::Gameplay || !window.gameplayActive ||
      !activeSession.has_value()) {
    window.automationControlStatus = "owner_unavailable";
    return false;
  }

  ActionState actions;
  recordAction(actions, action, true, true, false, 1.0F);

  InputRoutingContext routingContext;
  routingContext.owners.gameplay = true;
  const InputRoutingResult routed = routeInputAction(routingContext, action);
  window.inputOwner = routed.owner;
  window.lastInputAction = routed.action;
  window.lastInputAccepted = routed.accepted;
  window.gameplayInputSuppressed = routed.gameplaySuppressed;
  if (!routed.accepted) {
    return false;
  }

  applyProductGameplayActions(
      *activeSession, actions, window, "automation",
      productActiveRoomCollisionSurfaces(window.activeRoomCollision));
  return window.gameplayCommandSubmitted && window.gameplayCommandAccepted &&
         window.gameplayTickAdvanced;
}

void copyRoomEditingStateToWindow(ProductAppWindowState& window,
                                  const ProductRoomEditingState& state) {
  window.roomEditing = state;
  if (state.ready) {
    window.activeRoom = state.activeRoom;
    window.activeRoomCollision = state.activeRoomCollision;
  }
}

void recordProductRoomEditingStart(ProductAppWindowState& window,
                                   const ProductRoomEditingStartResult& result,
                                   std::string_view operation = "room_edit.start") {
  window.roomEditingLastOperation = std::string(operation);
  window.roomEditingLastOperationStatus = result.status;
  window.roomEditingLastOperationReasonCode = result.reasonCode;
  window.roomEditingLastInputSource =
      productRoomAuthoringInputSourceName(ProductRoomAuthoringInputSource::Script);
  window.roomEditingLastOperationAccepted = result.ok;
  window.roomEditingLastPrimitiveId = "none";
  copyRoomEditingStateToWindow(window, result.state);
  if (result.ok) {
    window.roomEditorCursorReady = true;
    window.roomEditorCursor = ProductRoomEditorCursorState{};
    window.roomEditorStatus = "room_editor_cursor_ready";
    window.roomEditorReasonCode = "room_editor_cursor_ready";
    window.roomEditorLastOperation = "none";
    window.roomEditorLastOperationAccepted = false;
    window.roomEditorLastPrimitiveId = "none";
  }
}

void recordProductRoomEditingOperation(
    ProductAppWindowState& window,
    std::string_view operation,
    const ProductRoomEditingOperationResult& result) {
  window.roomEditingLastOperation = std::string(operation);
  window.roomEditingLastOperationStatus = result.status;
  window.roomEditingLastOperationReasonCode = result.reasonCode;
  window.roomEditingLastInputSource =
      productRoomAuthoringInputSourceName(result.inputSource);
  window.roomEditingLastOperationAccepted = result.accepted;
  window.roomEditingLastPrimitiveId =
      result.edit.primitiveId.empty() ? std::string{"none"}
                                      : result.edit.primitiveId;
  copyRoomEditingStateToWindow(window, result.state);
}

void recordProductRoomEditorCursorResult(
    ProductAppWindowState& window,
    std::string_view operation,
    const ProductRoomEditorCursorResult& result) {
  window.roomEditorCursorReady = window.roomEditing.ready;
  window.roomEditorCursor = result.state;
  window.roomEditorStatus = result.status;
  window.roomEditorReasonCode = result.reasonCode;
  window.roomEditorLastOperation = std::string(operation);
  window.roomEditorLastOperationAccepted = result.ok;
  window.roomEditorLastPrimitiveId = "none";
}

bool rejectProductRoomEditorNotReady(ProductAppWindowState& window,
                                     std::string_view operation) {
  window.roomEditorCursorReady = false;
  window.roomEditorStatus = "room_editor_not_ready";
  window.roomEditorReasonCode = "room_editor_not_ready";
  window.roomEditorLastOperation = std::string(operation);
  window.roomEditorLastOperationAccepted = false;
  window.roomEditorLastPrimitiveId = "none";
  window.automationControlStatus = "command_failed";
  return false;
}

void recordProductRoomEditorActionResult(
    ProductAppWindowState& window,
    const ProductRoomEditorActionResult& result,
    std::string_view operationOverride = {}) {
  window.roomEditing = result.editing;
  copyRoomEditingStateToWindow(window, result.editing);
  window.roomEditorCursorReady = result.editing.ready;
  window.roomEditorCursor = result.cursor;
  window.roomEditorStatus = result.status;
  window.roomEditorReasonCode = result.reasonCode;
  window.roomEditorLastOperation =
      operationOverride.empty() ? result.operation : std::string(operationOverride);
  window.roomEditorLastOperationAccepted = result.operationAccepted;
  window.roomEditorLastPrimitiveId = result.primitiveId;

  if (result.status == "room_editor_command_applied") {
    window.roomEditingLastOperation = window.roomEditorLastOperation;
    window.roomEditingLastOperationStatus = "product_room_editing_edit_applied";
    window.roomEditingLastOperationReasonCode = "product_room_editing_edit_applied";
    window.roomEditingLastInputSource =
        productRoomAuthoringInputSourceName(ProductRoomAuthoringInputSource::Hotkey);
    window.roomEditingLastOperationAccepted = result.operationAccepted;
    window.roomEditingLastPrimitiveId = result.primitiveId;
  }
}

bool applyProductAutomationCommand(const ProductAutomationCommand& command,
                                   FrontendState& frontend,
                                   const ProductSaveBridgeResult& saves,
                                   const ProductAppOptions& options,
                                   FrontendSettingsTab& settingsTab,
                                   std::optional<Session>& activeSession,
                                   WorldSetupDraft& worldSetupDraft,
                                   ProductAppWindowState& window,
                                   bool& closeRequested) {
  const std::string_view key{command.key};
  const std::string_view value{command.value};
  static const ProductAutomationCommandRegistry automationRegistry =
      makeProductAutomationCommandRegistry();
  const std::string_view canonicalKey =
      productAutomationCanonicalKey(automationRegistry, key);
  const ProductAutomationCommandDispatchResult automationDispatch =
      resolveProductAutomationCommandDispatch({&automationRegistry, key});
  const ProductAutomationCommandDispatchSpec& automationSpec =
      automationDispatch.spec;
  bool boolValue = false;
  InputAction inputAction = InputAction::None;

  if (key == "automation.owner") {
    MenuOwner expectedOwner = MenuOwner::None;
    if (!parseAutomationOwner(value, expectedOwner)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    const MenuOwner currentOwner = productInputOwnerFor(frontend, window);
    if (currentOwner != expectedOwner) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, menuOwnerName(expectedOwner), currentOwner,
                            "failed");
      return false;
    }
    markAutomationApplied(window, command, menuOwnerName(expectedOwner), currentOwner,
                          "applied");
    return true;
  }

  if (key == "menu.input") {
    if (!parseAutomationInputAction(value, inputAction)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    const bool routed = routeAutomationInput(frontend, saves, options, settingsTab,
                                            activeSession, worldSetupDraft, window,
                                            inputAction, closeRequested);
    markAutomationApplied(window, command, inputActionName(inputAction),
                          window.automationControlLastOwner,
                          routed ? "applied" : "ignored");
    return routed;
  }

  if (automationSpec.commandId == ProductAutomationCommandId::MenuShortcut) {
    const ProductMenuShortcutAutomationResult shortcut =
        resolveProductMenuShortcutAutomation(automationSpec, value);
    if (!shortcut.valid) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!shortcut.routeRequested) {
      markAutomationApplied(window, command, "none", productInputOwnerFor(frontend, window),
                            "ignored");
      return true;
    }
    const bool routed = routeAutomationInput(frontend, saves, options, settingsTab,
                                            activeSession, worldSetupDraft, window,
                                            shortcut.inputAction, closeRequested);
    markAutomationApplied(window, command, inputActionName(shortcut.inputAction),
                          window.automationControlLastOwner,
                          routed ? "applied" : "ignored");
    return routed;
  }

  if (key == "frontend.select" || key == "pause.select") {
    FrontendAction action = FrontendAction::None;
    if (!parseAutomationFrontendAction(value, action)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    frontend.selectedAction = action;
    markAutomationApplied(window, command, frontendActionName(action),
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "world.title" || key == "world_setup.title") {
    if (frontend.childScreen != FrontendScreen::NewWorld) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "world.title",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    worldSetupDraft.worldName = std::string(value);
    recordWorldSetupDraftState(worldSetupDraft, window);
    window.worldSetupStatus = "world_setup_title_updated";
    markAutomationApplied(window, command, "world.title",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "world.dungeon_id" || key == "world.map_id" ||
      key == "world_setup.dungeon_id" || key == "world_setup.map_id") {
    if (frontend.childScreen != FrontendScreen::NewWorld) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "world.dungeon_id",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    const std::size_t index = productBuiltinDungeonIndexForRoomId(value);
    if (!applyProductBuiltinDungeonToDraft(index, worldSetupDraft)) {
      window.automationControlStatus = "invalid_value";
      markAutomationApplied(window, command, "world.dungeon_id",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    recordWorldSetupDraftState(worldSetupDraft, window);
    window.worldSetupDungeonDraftModified = false;
    window.worldSetupDungeonDraftEditMode = false;
    resetDungeonDraftWindowCursor(worldSetupDraft, window);
    window.worldSetupStatus = "world_setup_dungeon_selected";
    markAutomationApplied(window, command, "world.dungeon_id",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "world.draft_edit_mode" ||
      key == "world_setup.draft_edit_mode") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (frontend.childScreen != FrontendScreen::NewWorld) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "world.draft_edit_mode",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    window.worldSetupDungeonDraftEditMode = boolValue;
    worldSetupDraft.selectedField =
        boolValue ? WorldSetupField::AsciiRoom : WorldSetupField::Create;
    window.worldSetupDungeonDraftStatus =
        boolValue ? "dungeon_draft_edit_mode_on" : "dungeon_draft_edit_mode_off";
    window.worldSetupDungeonDraftReasonCode = window.worldSetupDungeonDraftStatus;
    resetDungeonDraftWindowCursor(worldSetupDraft, window);
    markAutomationApplied(window, command, "world.draft_edit_mode",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "world.draft_move" || key == "world_setup.draft_move" ||
      key == "frontend.draft_move") {
    if (frontend.childScreen != FrontendScreen::NewWorld) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "world.draft_move",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    if (!window.worldSetupDungeonDraftEditMode) {
      window.worldSetupDungeonDraftStatus = "dungeon_draft_edit_mode_off";
      window.worldSetupDungeonDraftReasonCode = "dungeon_draft_edit_mode_off";
      window.automationControlStatus = "command_failed";
      markAutomationApplied(window, command, "world.draft_move",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    ProductDungeonDraftDirection direction = ProductDungeonDraftDirection::Up;
    if (value == "up") {
      direction = ProductDungeonDraftDirection::Up;
    } else if (value == "down") {
      direction = ProductDungeonDraftDirection::Down;
    } else if (value == "left") {
      direction = ProductDungeonDraftDirection::Left;
    } else if (value == "right") {
      direction = ProductDungeonDraftDirection::Right;
    } else {
      window.automationControlStatus = "invalid_value";
      markAutomationApplied(window, command, "world.draft_move",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    const ProductDungeonDraftOperationResult moved =
        moveProductDungeonDraftCursor(worldSetupDraft,
                                      dungeonDraftCursorFromWindow(window),
                                      direction);
    recordDungeonDraftOperation(window, moved);
    markAutomationApplied(window, command, "world.draft_move",
                          productInputOwnerFor(frontend, window),
                          moved.ok ? "applied" : "failed");
    if (!moved.ok) {
      window.automationControlStatus = "command_failed";
    }
    return moved.ok;
  }

  if (key == "world.draft_paint" || key == "world_setup.draft_paint" ||
      key == "frontend.draft_paint") {
    if (frontend.childScreen != FrontendScreen::NewWorld) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "world.draft_paint",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    if (value.size() != 1U) {
      window.automationControlStatus = "invalid_value";
      markAutomationApplied(window, command, "world.draft_paint",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    const bool painted =
        applyDungeonDraftPaintGlyph(worldSetupDraft, window, value.front());
    markAutomationApplied(window, command, "world.draft_paint",
                          productInputOwnerFor(frontend, window),
                          painted ? "applied" : "failed");
    if (!painted) {
      window.automationControlStatus = "command_failed";
    }
    return painted;
  }

  if (key == "world.draft_cell" || key == "world_setup.draft_cell") {
    if (frontend.childScreen != FrontendScreen::NewWorld) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "world.draft_cell",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    const std::vector<std::string_view> fields = splitAutomationCsv(value);
    std::size_t row = 0;
    std::size_t column = 0;
    if (fields.size() != 3U || fields[2].size() != 1U ||
        !parseAutomationSize(fields[0], row) ||
        !parseAutomationSize(fields[1], column)) {
      window.automationControlStatus = "invalid_value";
      markAutomationApplied(window, command, "world.draft_cell",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    ProductDungeonDraftOperationResult painted =
        setProductDungeonDraftCell(worldSetupDraft, row, column, fields[2].front());
    recordDungeonDraftOperation(window, painted);
    recordWorldSetupDraftState(worldSetupDraft, window);
    markAutomationApplied(window, command, "world.draft_cell",
                          productInputOwnerFor(frontend, window),
                          painted.ok ? "applied" : "failed");
    if (!painted.ok) {
      window.automationControlStatus = "command_failed";
    }
    return painted.ok;
  }

  if (key == "world.ascii_room_text" ||
      key == "world_setup.ascii_room_text") {
    if (frontend.childScreen != FrontendScreen::NewWorld) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "world.ascii_room_text",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    worldSetupDraft.asciiRoomEnabled = true;
    worldSetupDraft.asciiRoomText =
        decodeProductAsciiRoomAutomationText(value);
    recordWorldSetupDraftState(worldSetupDraft, window);
    window.worldSetupStatus = "world_setup_ascii_room_text_updated";
    markAutomationApplied(window, command, "world.ascii_room_text",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "world.ascii_room_id" ||
      key == "world_setup.ascii_room_id") {
    if (frontend.childScreen != FrontendScreen::NewWorld || value.empty()) {
      window.automationControlStatus =
          value.empty() ? "invalid_value" : "owner_unavailable";
      markAutomationApplied(window, command, "world.ascii_room_id",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    worldSetupDraft.asciiRoomEnabled = true;
    worldSetupDraft.asciiRoomId = std::string(value);
    recordWorldSetupDraftState(worldSetupDraft, window);
    window.worldSetupStatus = "world_setup_ascii_room_id_updated";
    markAutomationApplied(window, command, "world.ascii_room_id",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "world.ascii_room_source_name" ||
      key == "world_setup.ascii_room_source_name") {
    if (frontend.childScreen != FrontendScreen::NewWorld || value.empty()) {
      window.automationControlStatus =
          value.empty() ? "invalid_value" : "owner_unavailable";
      markAutomationApplied(window, command, "world.ascii_room_source_name",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    worldSetupDraft.asciiRoomEnabled = true;
    worldSetupDraft.asciiRoomSourceName = std::string(value);
    recordWorldSetupDraftState(worldSetupDraft, window);
    window.worldSetupStatus = "world_setup_ascii_room_source_name_updated";
    markAutomationApplied(window, command, "world.ascii_room_source_name",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "world.create" || key == "world_setup.create") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "world.create",
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }
    if (frontend.childScreen != FrontendScreen::NewWorld) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "world.create",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    const bool routed = routeAutomationInput(frontend, saves, options, settingsTab,
                                            activeSession, worldSetupDraft, window,
                                            InputAction::MenuConfirm, closeRequested);
    markAutomationApplied(window, command, inputActionName(InputAction::MenuConfirm),
                          window.automationControlLastOwner,
                          routed ? "applied" : "failed");
    return routed;
  }

  if (key == "ascii_room.text" || key == "frontend.ascii_room_text") {
    window.asciiRoomDraftText = decodeProductAsciiRoomAutomationText(value);
    window.asciiRoomPreviewStatus = "ascii_room_text_updated";
    window.asciiRoomPreviewReasonCode = "ascii_room_text_updated";
    window.asciiRoomPreviewFailedStage = "not_started";
    window.asciiRoomPreviewReady = false;
    markAutomationApplied(window, command, "ascii_room.text",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "ascii_room.room_id" || key == "frontend.ascii_room_id") {
    if (value.empty()) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    window.asciiRoomDraftRoomId = std::string(value);
    markAutomationApplied(window, command, "ascii_room.room_id",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "ascii_room.source_name" ||
      key == "frontend.ascii_room_source_name") {
    if (value.empty()) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    window.asciiRoomDraftSourceName = std::string(value);
    markAutomationApplied(window, command, "ascii_room.source_name",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "ascii_room.build" || key == "frontend.ascii_room_build") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "ascii_room.build",
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }

    const bool previewBuilt = buildProductAsciiRoomPreview(window);
    markAutomationApplied(window, command, "ascii_room.build",
                          productInputOwnerFor(frontend, window),
                          previewBuilt ? "applied" : "failed");
    return previewBuilt;
  }

  if (key == "ascii_room.activate" || key == "frontend.ascii_room_activate") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "ascii_room.activate",
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }

    const ProductAsciiRoomActivationResult activated =
        activateProductAsciiRoomPreview(activeSession, window);
    if (activated.ok) {
      enterProductGameplayTransition(frontend, window,
                                     FrontendAction::CreateAndEnter);
    }
    markAutomationApplied(window, command, "ascii_room.activate",
                          productInputOwnerFor(frontend, window),
                          activated.ok ? "applied" : "failed");
    return activated.ok;
  }

  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditStart) {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, automationSpec.canonicalKey,
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }

    const ProductAsciiRoomAuthoringRequest request =
        productAsciiRoomAuthoringRequestFromDraft(window);
    const ProductRoomEditingStartResult started =
        startProductRoomEditAutomationFromAsciiDraft({request});
    recordProductRoomEditingStart(window, started);
    markAutomationApplied(window, command, automationSpec.canonicalKey,
                          productInputOwnerFor(frontend, window),
                          started.ok ? "applied" : "failed");
    return started.ok;
  }

  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditStartActive) {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, automationSpec.canonicalKey,
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }

    const ProductRoomEditingStartResult started =
        startProductRoomEditAutomationFromActiveRoom({window.activeRoom});
    recordProductRoomEditingStart(window, started, automationSpec.canonicalKey);
    markAutomationApplied(window, command, automationSpec.canonicalKey,
                          productInputOwnerFor(frontend, window),
                          started.ok ? "applied" : "failed");
    return started.ok;
  }

  if (key == "editor.input") {
    const std::vector<std::string_view> editorInputs = splitAutomationCsv(value);
    if (editorInputs.empty()) {
      window.automationControlStatus = "invalid_value";
      return false;
    }

    InputAction lastAction = InputAction::None;
    MenuOwner lastOwner = productInputOwnerFor(frontend, window);
    ProductRoomEditorActionResult lastResult;
    bool anyApplied = false;
    for (const std::string_view token : editorInputs) {
      InputAction editorAction = InputAction::None;
      float actionValue = 1.0F;
      if (!parseProductRoomEditorInputAction(token, editorAction, actionValue)) {
        window.automationControlStatus = "invalid_value";
        return false;
      }
      lastAction = editorAction;
      if (frontend.screen != FrontendScreen::Gameplay || !window.gameplayActive ||
          !activeSession.has_value() || !window.roomEditing.ready) {
        rejectProductRoomEditorNotReady(window, inputActionName(editorAction));
        markAutomationApplied(window, command, inputActionName(editorAction),
                              productInputOwnerFor(frontend, window), "failed");
        return false;
      }

      InputRoutingContext routingContext;
      routingContext.owners.editor = window.roomEditing.ready;
      routingContext.owners.gameplay = true;
      const InputRoutingResult routed = routeInputAction(routingContext, editorAction);
      lastOwner = routed.owner;
      window.inputOwner = routed.owner;
      window.lastInputAction = routed.action;
      window.lastInputAccepted = routed.accepted;
      window.gameplayInputSuppressed = routed.gameplaySuppressed;
      if (!routed.accepted || routed.owner != MenuOwner::Editor) {
        window.automationControlStatus = "owner_unavailable";
        markAutomationApplied(window, command, inputActionName(editorAction),
                              routed.owner, "failed");
        return false;
      }

      ActionState actions;
      recordAction(actions, editorAction, true, true, false, actionValue);
      lastResult = applyProductEditorInputAutomation(
          window.roomEditing, window.roomEditorCursor, actions,
          ProductRoomAuthoringInputSource::Hotkey);
      recordProductRoomEditorActionResult(window, lastResult);
      if (!lastResult.ok) {
        window.automationControlStatus = "command_failed";
        markAutomationApplied(window, command, inputActionName(editorAction),
                              routed.owner, "failed");
        return false;
      }
      anyApplied = true;
    }

    if (!anyApplied) {
      window.automationControlStatus = "command_failed";
      markAutomationApplied(window, command, "editor.input", lastOwner, "failed");
      return false;
    }
    markAutomationApplied(window, command, inputActionName(lastAction),
                          lastOwner, "applied");
    return true;
  }

  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditorMove) {
    ProductRoomEditorDirection direction = ProductRoomEditorDirection::Up;
    if (!parseProductRoomEditorDirection(value, direction)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!window.roomEditing.ready) {
      rejectProductRoomEditorNotReady(window, automationSpec.canonicalKey);
      markAutomationApplied(window, command, automationSpec.canonicalKey,
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    const ProductRoomEditorCursorResult moved =
        applyProductRoomEditorMoveAutomation(window.roomEditorCursor, direction);
    recordProductRoomEditorCursorResult(window, automationSpec.canonicalKey, moved);
    if (!moved.ok) {
      window.automationControlStatus = "command_failed";
    }
    markAutomationApplied(window, command, automationSpec.canonicalKey,
                          productInputOwnerFor(frontend, window),
                          moved.ok ? "applied" : "failed");
    return moved.ok;
  }

  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditorTool) {
    ProductRoomEditorTool tool = ProductRoomEditorTool::Floor;
    if (!parseProductRoomEditorTool(value, tool)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!window.roomEditing.ready) {
      rejectProductRoomEditorNotReady(window, automationSpec.canonicalKey);
      markAutomationApplied(window, command, automationSpec.canonicalKey,
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    const ProductRoomEditorCursorResult changed =
        applyProductRoomEditorToolAutomation(window.roomEditorCursor, tool);
    recordProductRoomEditorCursorResult(window, automationSpec.canonicalKey, changed);
    if (!changed.ok) {
      window.automationControlStatus = "command_failed";
    }
    markAutomationApplied(window, command, automationSpec.canonicalKey,
                          productInputOwnerFor(frontend, window),
                          changed.ok ? "applied" : "failed");
    return changed.ok;
  }

  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditorCycleTool) {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, automationSpec.canonicalKey,
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }
    if (!window.roomEditing.ready) {
      rejectProductRoomEditorNotReady(window, automationSpec.canonicalKey);
      markAutomationApplied(window, command, automationSpec.canonicalKey,
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    const ProductRoomEditorCursorResult changed =
        applyProductRoomEditorCycleToolAutomation(window.roomEditorCursor);
    recordProductRoomEditorCursorResult(window, automationSpec.canonicalKey, changed);
    if (!changed.ok) {
      window.automationControlStatus = "command_failed";
    }
    markAutomationApplied(window, command, automationSpec.canonicalKey,
                          productInputOwnerFor(frontend, window),
                          changed.ok ? "applied" : "failed");
    return changed.ok;
  }

  if (automationSpec.commandId ==
      ProductAutomationCommandId::RoomEditorWallDirection) {
    ProductRoomEditorDirection direction = ProductRoomEditorDirection::Up;
    if (!parseProductRoomEditorDirection(value, direction)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!window.roomEditing.ready) {
      rejectProductRoomEditorNotReady(window, automationSpec.canonicalKey);
      markAutomationApplied(window, command, automationSpec.canonicalKey,
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    const ProductRoomEditorCursorResult changed =
        applyProductRoomEditorWallDirectionAutomation(window.roomEditorCursor,
                                                      direction);
    recordProductRoomEditorCursorResult(
        window, automationSpec.canonicalKey, changed);
    if (!changed.ok) {
      window.automationControlStatus = "command_failed";
    }
    markAutomationApplied(window, command, automationSpec.canonicalKey,
                          productInputOwnerFor(frontend, window),
                          changed.ok ? "applied" : "failed");
    return changed.ok;
  }

  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditorPlace) {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, automationSpec.canonicalKey,
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }
    if (!window.roomEditing.ready) {
      rejectProductRoomEditorNotReady(window, automationSpec.canonicalKey);
      markAutomationApplied(window, command, automationSpec.canonicalKey,
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }

    const ProductRoomEditorActionResult result =
        applyProductRoomEditorPlaceAutomation(
            window.roomEditing, window.roomEditorCursor,
            ProductRoomAuthoringInputSource::Hotkey);
    recordProductRoomEditorActionResult(window, result, automationSpec.canonicalKey);
    if (!result.ok) {
      window.automationControlStatus = "command_failed";
    }
    markAutomationApplied(window, command, automationSpec.canonicalKey,
                          productInputOwnerFor(frontend, window),
                          result.ok ? "applied" : "failed");
    return result.ok;
  }

  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditAddFloor) {
    RoomEditCommand edit;
    if (!parseAutomationFloorCommand(value, edit)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    ProductRoomEditingOperationResult result = applyProductRoomEditAutomation(
        {window.roomEditing, ProductRoomAuthoringInputSource::Script, edit});
    recordProductRoomEditingOperation(window, automationSpec.canonicalKey, result);
    markAutomationApplied(window, command, automationSpec.canonicalKey,
                          productInputOwnerFor(frontend, window),
                          result.accepted ? "applied" : "failed");
    return result.accepted;
  }

  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditAddWall) {
    RoomEditCommand edit;
    if (!parseAutomationWallCommand(value, edit)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    ProductRoomEditingOperationResult result = applyProductRoomEditAutomation(
        {window.roomEditing, ProductRoomAuthoringInputSource::Script, edit});
    recordProductRoomEditingOperation(window, automationSpec.canonicalKey, result);
    markAutomationApplied(window, command, automationSpec.canonicalKey,
                          productInputOwnerFor(frontend, window),
                          result.accepted ? "applied" : "failed");
    return result.accepted;
  }

  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditDeleteFloor) {
    if (value.empty()) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    const RoomEditCommand edit = deleteFloorCommand(std::string(value));
    ProductRoomEditingOperationResult result = applyProductRoomEditAutomation(
        {window.roomEditing, ProductRoomAuthoringInputSource::Script, edit});
    recordProductRoomEditingOperation(window, automationSpec.canonicalKey, result);
    markAutomationApplied(window, command, automationSpec.canonicalKey,
                          productInputOwnerFor(frontend, window),
                          result.accepted ? "applied" : "failed");
    return result.accepted;
  }

  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditDeleteWall) {
    if (value.empty()) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    const RoomEditCommand edit = deleteWallCommand(std::string(value));
    ProductRoomEditingOperationResult result = applyProductRoomEditAutomation(
        {window.roomEditing, ProductRoomAuthoringInputSource::Script, edit});
    recordProductRoomEditingOperation(window, automationSpec.canonicalKey, result);
    markAutomationApplied(window, command, automationSpec.canonicalKey,
                          productInputOwnerFor(frontend, window),
                          result.accepted ? "applied" : "failed");
    return result.accepted;
  }

  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditUndo) {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, automationSpec.canonicalKey,
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }
    ProductRoomEditingOperationResult result =
        undoProductRoomEditAutomation(
            {window.roomEditing, ProductRoomAuthoringInputSource::Script});
    recordProductRoomEditingOperation(window, automationSpec.canonicalKey, result);
    markAutomationApplied(window, command, automationSpec.canonicalKey,
                          productInputOwnerFor(frontend, window),
                          result.accepted ? "applied" : "failed");
    return result.accepted;
  }

  if (automationSpec.commandId == ProductAutomationCommandId::RoomEditRedo) {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, automationSpec.canonicalKey,
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }
    ProductRoomEditingOperationResult result =
        redoProductRoomEditAutomation(
            {window.roomEditing, ProductRoomAuthoringInputSource::Script});
    recordProductRoomEditingOperation(window, automationSpec.canonicalKey, result);
    markAutomationApplied(window, command, automationSpec.canonicalKey,
                          productInputOwnerFor(frontend, window),
                          result.accepted ? "applied" : "failed");
    return result.accepted;
  }

  static constexpr std::array gameplayAxisRows{
      std::pair{std::string_view{"game.move_x"}, InputAction::PlayerMoveX},
      std::pair{std::string_view{"game.move_y"}, InputAction::PlayerMoveY},
  };
  const auto gameplayAxis = std::find_if(
      gameplayAxisRows.begin(), gameplayAxisRows.end(),
      [canonicalKey](const auto& row) {
        return row.first == canonicalKey;
      });
  if (gameplayAxis != gameplayAxisRows.end()) {
    float axisValue = 0.0F;
    if (!parseAutomationFloat(value, axisValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    const InputAction action = gameplayAxis->second;
    const bool moved = applyAutomationGameplayAxis(action, axisValue, frontend,
                                                   activeSession, window);
    markAutomationApplied(window,
                          command,
                          inputActionName(action),
                          productInputOwnerFor(frontend, window),
                          moved ? "applied" : "failed");
    return moved;
  }

  static constexpr std::array gameplayButtonRows{
      std::pair{std::string_view{"game.attack"}, InputAction::PlayerAttack},
      std::pair{std::string_view{"game.interact"}, InputAction::PlayerInteract},
  };
  const auto gameplayButton = std::find_if(
      gameplayButtonRows.begin(), gameplayButtonRows.end(),
      [canonicalKey](const auto& row) {
        return row.first == canonicalKey;
      });
  if (gameplayButton != gameplayButtonRows.end()) {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    const InputAction action = gameplayButton->second;
    if (!boolValue) {
      markAutomationApplied(window, command, inputActionName(action),
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }
    const bool executed =
        applyAutomationGameplayButton(action, frontend, activeSession, window);
    markAutomationApplied(window,
                          command,
                          inputActionName(action),
                          productInputOwnerFor(frontend, window),
                          executed ? "applied" : "failed");
    return executed;
  }

  if (key == "save.select" || key == "frontend.save_select") {
    if (frontend.childScreen != FrontendScreen::LoadSave) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "save.select",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    const bool selected = selectProductSaveSlotById(saves.slots, value, window);
    markAutomationApplied(window,
                          command,
                          window.selectedProductSaveId,
                          productInputOwnerFor(frontend, window),
                          selected ? "applied" : "ignored");
    return selected;
  }

  if (key == "save.delete" || key == "frontend.save_delete") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "save.delete",
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }
    if (frontend.childScreen != FrontendScreen::LoadSave) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "save.delete",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    openProductSaveDeleteConfirmation(saves.slots, window, frontend);
    markAutomationApplied(window, command, "save.delete",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "save.show_deleted" || key == "frontend.show_deleted_saves") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "save.show_deleted",
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }
    if (frontend.childScreen != FrontendScreen::LoadSave) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "save.show_deleted",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    openDeletedProductSaveBrowser(options, window, frontend);
    markAutomationApplied(window, command, "save.show_deleted",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "save.deleted_select" || key == "frontend.deleted_save_select") {
    if (frontend.childScreen != FrontendScreen::LoadSave ||
        !window.deletedSaveBrowserOpen) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "save.deleted_select",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    const ProductSaveBridgeResult deletedSaves =
        scanDeletedProductSavesForOptions(options);
    recordDeletedProductSaveSlots(deletedSaves, window);
    const bool selected =
        selectDeletedProductSaveSlotById(deletedSaves.slots, value, window);
    markAutomationApplied(window,
                          command,
                          window.deletedSelectedSaveId,
                          productInputOwnerFor(frontend, window),
                          selected ? "applied" : "ignored");
    return selected;
  }

  if (key == "save.recover" || key == "frontend.save_recover") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "save.recover",
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }
    if (frontend.childScreen != FrontendScreen::LoadSave ||
        !window.deletedSaveBrowserOpen) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "save.recover",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    executeProductSaveRecover(options, window, frontend);
    markAutomationApplied(window, command, "save.recover",
                          productInputOwnerFor(frontend, window),
                          window.saveRecoverExecuted ? "applied" : "failed");
    return window.saveRecoverExecuted;
  }

  if (key == "settings.tab") {
    if (!parseAutomationSettingsTab(value, settingsTab)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    window.selectedSettingsTab = settingsTab;
    markAutomationApplied(window, command, frontendSettingsTabName(settingsTab),
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "dev_tools.category") {
    FrontendDevToolsCategory category = FrontendDevToolsCategory::None;
    if (!parseAutomationDevToolsCategory(value, category)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    frontend.devToolsCategory = category;
    markAutomationApplied(window, command, frontendDevToolsCategoryName(category),
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "frontend.execute" || key == "pause.execute" ||
      key == "dev_tools.execute") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "none", productInputOwnerFor(frontend, window),
                            "ignored");
      return true;
    }
    const bool routed = routeAutomationInput(frontend, saves, options, settingsTab,
                                            activeSession, worldSetupDraft, window,
                                            InputAction::MenuConfirm, closeRequested);
    markAutomationApplied(window, command, inputActionName(InputAction::MenuConfirm),
                          window.automationControlLastOwner,
                          routed ? "applied" : "ignored");
    return routed;
  }

  if (key == "settings.apply" || key == "settings.restore_defaults") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (boolValue) {
      frontend.status = key == "settings.apply" ? "settings_applied"
                                                : "settings_defaults_restored";
    }
    markAutomationApplied(window, command, key == "settings.apply" ? "settings.apply"
                                                                   : "settings.restore_defaults",
                          productInputOwnerFor(frontend, window),
                          boolValue ? "applied" : "ignored");
    return true;
  }

  if (key == "settings.back" || key == "system.back") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "none", productInputOwnerFor(frontend, window),
                            "ignored");
      return true;
    }
    const bool routed = routeAutomationInput(frontend, saves, options, settingsTab,
                                            activeSession, worldSetupDraft, window,
                                            InputAction::MenuBack, closeRequested);
    markAutomationApplied(window, command, inputActionName(InputAction::MenuBack),
                          window.automationControlLastOwner,
                          routed ? "applied" : "ignored");
    return routed;
  }

  if (key == "frontend.return_to_title") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (boolValue) {
      returnProductToTitleTransition(frontend, window);
      activeSession.reset();
    }
    markAutomationApplied(window, command, "frontend.return_to_title",
                          productInputOwnerFor(frontend, window),
                          boolValue ? "applied" : "ignored");
    return true;
  }

  if (key == "system.pause") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "none", productInputOwnerFor(frontend, window),
                            "ignored");
      return true;
    }
    const bool routed = routeAutomationInput(frontend, saves, options, settingsTab,
                                            activeSession, worldSetupDraft, window,
                                            InputAction::SystemPause, closeRequested);
    markAutomationApplied(window, command, inputActionName(InputAction::SystemPause),
                          window.automationControlLastOwner,
                          routed ? "applied" : "ignored");
    return routed;
  }

  if (key == "system.quit") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (boolValue) {
      closeRequested = true;
    }
    markAutomationApplied(window, command, "system.quit",
                          productInputOwnerFor(frontend, window),
                          boolValue ? "applied" : "ignored");
    return true;
  }

  window.automationControlStatus = "unknown_key";
  return false;
}

void applyProductAutomationControl(const ProductAppOptions& options,
                                   FrontendState& frontend,
                                   const ProductSaveBridgeResult& saves,
                                   FrontendSettingsTab& settingsTab,
                                   std::optional<Session>& activeSession,
                                   WorldSetupDraft& worldSetupDraft,
                                   ProductAppWindowState& window,
                                   bool& closeRequested) {
  std::vector<ProductAutomationCommand> commands;
  if (!readProductAutomationCommands(options.automationControlPath, window, commands)) {
    return;
  }
  for (const ProductAutomationCommand& command : commands) {
    if (!applyProductAutomationCommand(command, frontend, saves, options, settingsTab,
                                       activeSession, worldSetupDraft, window,
                                       closeRequested)) {
      if (window.automationControlLastKey == "none") {
        window.automationControlLastKey = command.key;
      }
      if (window.automationControlLastAction == "none") {
        window.automationControlLastAction = command.value.empty() ? "none" : command.value;
      }
      window.automationControlLastOwner = productInputOwnerFor(frontend, window);
      window.automationControlLastResult = "failed";
      if (automationFailurePreservesLoaded(window.automationControlStatus)) {
        window.automationControlStatus = "command_failed";
      } else {
        window.automationControlLoaded = false;
      }
      return;
    }
  }
  if (window.automationControlStatus == "loaded" && commands.empty()) {
    window.automationControlLastResult = "none";
  }
  window.selectedSettingsTab = settingsTab;
  window.inputOwner = productInputOwnerFor(frontend, window);
  window.gameplayInputSuppressed =
      frontendBlocksGameplayInput(frontend) ||
      menuOwnerBlocksGameplay(window.inputOwner);
}

ProductAppWindowState runOpeningMenuWindow(const ProductAppOptions& options,
                                           const ProductWorldTemplate& world,
                                           FrontendState& frontend,
                                           std::optional<Session>& activeSession,
                                           WorldSetupDraft& worldSetupDraft,
                                           ProductAppWindowState window,
                                           const FrontendSettings& settings,
                                           const ProductSaveBridgeResult& saves) {
  window.requested = options.windowMode == ProductWindowMode::Window;
  const bool useVulkanRenderer = options.renderer == ProductRendererRequest::Vulkan;
  window.productVulkanRendererRequested = useVulkanRenderer;
  window.inputOwner = productInputOwnerFor(frontend, window);
  window.gameplayInputSuppressed =
      frontendBlocksGameplayInput(frontend) ||
      menuOwnerBlocksGameplay(window.inputOwner);
  if (!window.requested) {
    return window;
  }

#if defined(IGGY3D_HAS_SDL3)
  window.sdlAvailable = true;

  SdlWindowCreateInfo createInfo;
  createInfo.title = "iggy3d - Opening Menu";
  createInfo.width = 1280;
  createInfo.height = 720;
  createInfo.resizable = true;
  createInfo.highDpi = true;
  createInfo.vulkan = useVulkanRenderer;

  SdlWindow sdlWindow(createInfo);
  window.created = sdlWindow.nativeWindow() != nullptr;
  window.drawable = sdlWindow.isDrawable();
  window.openingMenuVisible = window.created && frontend.screen == FrontendScreen::Starter;
  if (!window.created) {
    window.status = "window_create_failed";
    return window;
  }

  SDL_Renderer* renderer = nullptr;
  RendererApi vulkanRenderer;
  if (useVulkanRenderer) {
#if defined(IGGY3D_APP_VULKAN_BACKEND)
    SdlVulkanSurfaceProvider sdlVulkanProvider;
    const SdlVulkanExtensionList extensions =
        sdlVulkanProvider.requiredInstanceExtensions(sdlWindow);
    if (extensions.outcome != RenderOutcome::Ok) {
      recordProductVulkanRendererUnavailable(window, extensions.reason.code);
      window.status = "product_vulkan_renderer_unavailable";
      return window;
    }
    VulkanBackendCreateInfo backendInfo;
    backendInfo.config = makeProductVulkanRendererConfig();
    const SdlDrawableExtent drawableExtent = sdlWindow.drawableExtent();
    backendInfo.drawableWidth =
        drawableExtent.width == 0U ? createInfo.width : drawableExtent.width;
    backendInfo.drawableHeight =
        drawableExtent.height == 0U ? createInfo.height : drawableExtent.height;
    backendInfo.surfaceProvider.requiredInstanceExtensions = extensions.names;
    backendInfo.surfaceProvider.createSurface =
        [&sdlVulkanProvider, &sdlWindow](VkInstance instance, VkSurfaceKHR* surface) {
          const SdlVulkanSurfaceCreateResult created =
              sdlVulkanProvider.createSurface(sdlWindow, instance);
          if (surface != nullptr) {
            *surface = created.surface;
          }
          RenderReceipt receipt;
          appendReceiptField(receipt, "surface_provider", "sdl3");
          appendReceiptField(receipt, "surface_created",
                             created.outcome == RenderOutcome::Ok &&
                                 created.surface != VK_NULL_HANDLE);
          appendReceiptField(receipt, "result",
                             created.outcome == RenderOutcome::Ok ? "pass" : "fail");
          appendReceiptField(receipt, "reason_code", created.reason.code);
          return receipt;
        };
    vulkanRenderer =
        RendererApi(std::make_unique<VulkanBackend>(std::move(backendInfo)));
    recordProductVulkanRendererReady(window, vulkanRenderer);
    if (!window.productVulkanRendererReady) {
      window.status = "product_vulkan_renderer_unavailable";
      return window;
    }
#else
    recordProductVulkanRendererUnavailable(window, "product_vulkan_backend_unavailable");
    window.status = "product_vulkan_renderer_unavailable";
    return window;
#endif
  } else {
    renderer = SDL_CreateRenderer(sdlWindow.nativeWindow(), nullptr);
    if (renderer == nullptr) {
      window.status = "renderer_create_failed";
      return window;
    }
  }

  sdlWindow.setTitle(window.gameplayActive ? "iggy3d - Gameplay" : "iggy3d - Opening Menu");
  const auto start = std::chrono::steady_clock::now();
  KeyboardInputState keyboard;
  MouseInputState mouse;
  GamepadMenuState gamepad;
  initializeGamepadMenuState(gamepad);
  window.gamepadAvailable = gamepad.gamepadAvailable;
  window.gamepadName = gamepad.gamepadName;
  window.gamepadMapping = gamepad.gamepadAvailable ? "sdl_gamepad" : "unavailable";
  bool closeRequested = false;
  FrontendSettingsTab settingsTab = FrontendSettingsTab::Input;
  while (sdlWindow.isOpen()) {
    ActionState actionState;
    sdlWindow.pollEvents();
    ++window.eventPollCount;
    window.drawable = sdlWindow.isDrawable();
    sdlWindow.setTitle(window.gameplayActive ? "iggy3d - Gameplay" : "iggy3d - Opening Menu");

    routeOpeningMenuInput(frontend, saves, options, settingsTab, activeSession,
                          worldSetupDraft, actionState, pollKeyboardMenuAction(keyboard),
                          window, closeRequested);
    if (frontend.childScreen == FrontendScreen::NewWorld) {
      const char paintGlyph = pollKeyboardAsciiRoomPaintGlyph(keyboard);
      if (paintGlyph != '\0') {
        applyDungeonDraftPaintGlyph(worldSetupDraft, window, paintGlyph);
      }
    }

    const InputAction gamepadAction = pollGamepadMenuAction(gamepad);
    if (gamepadAction != InputAction::None) {
      window.gamepadMenuSelectUsed = true;
      routeOpeningMenuInput(frontend, saves, options, settingsTab, activeSession,
                            worldSetupDraft, actionState, gamepadAction, window,
                            closeRequested);
    }

    const MouseClick click = pollMouseClick(mouse);
    if (click.clicked) {
      const OpeningMenuHitTestResult hit = openingMenuActionAt(frontend, click.x, click.y);
      if (hit.hit) {
        window.mouseMenuSelectUsed = true;
        if (hit.area == OpeningMenuHitArea::StarterAction) {
          frontend.selectedAction = hit.action;
          routeOpeningMenuInput(frontend, saves, options, settingsTab, activeSession,
                                worldSetupDraft, actionState, mouseClickAction(click),
                                window, closeRequested);
        } else if (hit.area == OpeningMenuHitArea::DevToolsCategory) {
          frontend.devToolsCategory = hit.devToolsCategory;
          frontend.status = "dev_tools_category_selected";
        } else if (hit.area == OpeningMenuHitArea::SettingsTab) {
          settingsTab = hit.settingsTab;
          frontend.status = "settings_tab_selected";
        }
      }
    }

    if (window.gameplayActive && activeSession.has_value() &&
        !frontendBlocksGameplayInput(frontend)) {
      ActionState gameplayActions;
      if (window.roomEditing.ready) {
        pollKeyboardRoomEditorActions(keyboard, gameplayActions);
        pollGamepadRoomEditorActions(gamepad, gameplayActions);
      } else {
        pollKeyboardGameplayActions(keyboard, gameplayActions);
        pollGamepadGameplayActions(gamepad, gameplayActions);
        pollMouseGameplayActions(mouse, gameplayActions);
      }

      ActionState acceptedGameplayActions;
      ActionState acceptedEditorActions;
      InputRoutingContext routingContext;
      routingContext.owners.editor = window.roomEditing.ready;
      routingContext.owners.gameplay = true;
      for (const ActionStateEntry& entry : gameplayActions.entries) {
        const InputRoutingResult routed = routeInputAction(routingContext, entry.action);
        window.inputOwner = routed.owner;
        window.lastInputAction = routed.action;
        window.lastInputAccepted = routed.accepted;
        window.gameplayInputSuppressed = routed.gameplaySuppressed;
        if (routed.accepted && routed.owner == MenuOwner::Editor &&
            inputActionGroup(entry.action) == InputActionGroup::Editor) {
          recordAction(acceptedEditorActions, entry.action, entry.down, entry.pressed,
                       entry.released, entry.value);
        } else if (routed.accepted && routed.owner == MenuOwner::Gameplay) {
          recordAction(acceptedGameplayActions, entry.action, entry.down, entry.pressed,
                       entry.released, entry.value);
        }
      }
      if (!acceptedEditorActions.entries.empty()) {
        const ProductRoomEditorActionResult result =
            applyProductRoomEditorActions(window.roomEditing,
                                          window.roomEditorCursor,
                                          acceptedEditorActions,
                                          ProductRoomAuthoringInputSource::Hotkey);
        recordProductRoomEditorActionResult(window, result);
      } else {
        applyProductCameraActions(acceptedGameplayActions, window.viewport, settings,
                                  "action_map");
        const SpatialSurfaceSet* collisionSurfaces =
            productActiveRoomCollisionSurfaces(window.activeRoomCollision);
        applyProductGameplayActions(*activeSession, acceptedGameplayActions, window,
                                    "action_map", collisionSurfaces);
      }
    }

    SceneProjectionResult scene;
    DebugProjectionResult debug;
    ProductPrimitiveDrawList drawList;
    ProductViewportFrame frame;
    std::size_t sceneItemCount = 0;
    const SceneProjectionResult* scenePtr = nullptr;
    const DebugProjectionResult* debugPtr = nullptr;
    const ProductPrimitiveDrawList* drawListPtr = nullptr;
    const ProductViewportFrame* framePtr = nullptr;
    const ProductRenderBridgeFrame* bridgePtr = nullptr;
    ProductGameplayFeedback feedback = buildProductGameplayFeedback(window);
    ProductMovementDebugHud movementHud = buildProductMovementDebugHud(
        window, settings.devToolsEnabled, settings.debugOverlayEnabled);
    ProductNpcBehaviorDebugHud npcBehaviorHud = buildProductNpcBehaviorDebugHud(
        nullptr, window.gameplayActive, settings.devToolsEnabled,
        settings.debugOverlayEnabled);
    copyNpcBehaviorDebugHud(window, npcBehaviorHud);
    ProductRoomEditorOverlay roomEditorOverlay =
        buildProductRoomEditorOverlay(window.roomEditorCursor, false);
    copyProductRoomEditorOverlay(window, roomEditorOverlay);
    ProductRenderBridgeFrame bridge;
    if (window.gameplayActive && activeSession.has_value()) {
      const RoomAsset* activeRoom =
          window.activeRoom.loaded ? &window.activeRoom.room : nullptr;
      scene = buildSceneProjection(activeSession->state(), activeRoom);
      debug = buildProductDebugProjectionWithNpcBehavior(activeSession->state());
      roomEditorOverlay = buildProductRoomEditorOverlay(window.roomEditorCursor,
                                                        window.roomEditing.ready);
      copyProductRoomEditorOverlay(window, roomEditorOverlay);
      drawList = buildProductPrimitiveDrawList(&scene, &debug, activeRoom,
                                               &window.activeRoomCollision,
                                               &roomEditorOverlay);
      frame = buildProductViewportFrame(
          drawList, ProductViewportFrameConfig{window.viewport.cameraYawDegrees,
                                               window.viewport.cameraPitchDegrees});
      scenePtr = &scene;
      debugPtr = &debug;
      drawListPtr = &drawList;
      framePtr = &frame;
      sceneItemCount = scene.items.size();
      window.runtimeStateHash = activeSession->stateHash();
      feedback = buildProductGameplayFeedback(window);
      movementHud = buildProductMovementDebugHud(window, settings.devToolsEnabled,
                                                 settings.debugOverlayEnabled);
      npcBehaviorHud = buildProductNpcBehaviorDebugHud(&debug,
                                                       window.gameplayActive,
                                                       settings.devToolsEnabled,
                                                       settings.debugOverlayEnabled);
      copyNpcBehaviorDebugHud(window, npcBehaviorHud);
      bridge = buildProductRenderBridgeFrame(&drawList, &frame, &feedback);
      bridgePtr = &bridge;
    }

    if (window.drawable) {
      applyGameplayProjectionMetrics(window, scenePtr, debugPtr, drawListPtr, framePtr,
                                     bridgePtr,
                                     window.gameplayActive && scenePtr != nullptr);
      if (useVulkanRenderer) {
        if (scenePtr != nullptr && debugPtr != nullptr && scenePtr->room.loaded) {
          const SdlDrawableExtent drawableExtent = sdlWindow.drawableExtent();
          if (drawableExtent.width > 0U && drawableExtent.height > 0U) {
            const FrameInput renderFrame = makeProductVulkanFrame(
                *scenePtr, *debugPtr, window.framesPresented + 1U, drawableExtent.width,
                drawableExtent.height, window.viewport.cameraYawDegrees,
                window.viewport.cameraPitchDegrees);
            const RenderSubmitResult submit = vulkanRenderer.submitFrame(renderFrame);
            recordProductVulkanSubmit(window, submit);
          } else {
            window.productVulkanStatus = "frame_not_submitted";
            window.productVulkanReasonCode = "frame_not_drawable";
          }
        } else {
          window.productVulkanStatus = "waiting_for_gameplay_room";
          window.productVulkanReasonCode = "product_vulkan_waiting_for_gameplay_room";
        }
      } else {
        const OpeningMenuViewState view =
            drawOpeningMenuView(*renderer, options, world, frontend, settingsTab,
                                worldSetupDraft,
                                window.worldSetupDungeonDraftEditMode,
                                window.worldSetupDungeonDraftModified,
                                window.worldSetupDungeonDraftCursorRow,
                                window.worldSetupDungeonDraftCursorColumn,
                                window.gameplayActive, window.runtimeStateHash, framePtr,
                                &feedback, &movementHud, &npcBehaviorHud,
                                sceneItemCount, debugPtr,
                                window.viewport.cameraYawDegrees,
                                window.viewport.cameraPitchDegrees, saves);
        window.viewport.cameraHeadingVisible =
            window.viewport.cameraHeadingVisible || view.cameraHeadingDrawn;
        window.menuTextDrawn = window.menuTextDrawn || view.textDrawn;
        window.selectedRowDrawn = window.selectedRowDrawn || view.selectedRowDrawn;
        window.menuRowCount = view.rowCount;
      }
    } else {
      applyGameplayProjectionMetrics(window, scenePtr, debugPtr, drawListPtr, framePtr,
                                     bridgePtr, false);
    }
    ++window.framesPresented;

    if (options.frames > 0 && window.framesPresented >= options.frames) {
      break;
    }
    if (closeRequested) {
      break;
    }
    if (options.holdSeconds > 0) {
      const auto elapsed = std::chrono::steady_clock::now() - start;
      if (elapsed >= std::chrono::seconds(options.holdSeconds)) {
        break;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  shutdownGamepadMenuState(gamepad);
  if (useVulkanRenderer) {
    (void)vulkanRenderer.waitIdle();
    vulkanRenderer.shutdown();
  } else {
    SDL_DestroyRenderer(renderer);
  }
  window.selectedSettingsTab = settingsTab;
  if (useVulkanRenderer) {
    window.status = window.productVulkanFrameSubmitted
                        ? "product_vulkan_frame_presented"
                        : window.productVulkanStatus;
  } else {
    window.status = window.menuTextDrawn ? "opening_menu_text_ready"
                                         : "opening_menu_window_ready";
  }
  return window;
#else
  (void)world;
  (void)frontend;
  (void)worldSetupDraft;
  (void)saves;
  window.sdlAvailable = false;
  window.created = false;
  window.drawable = false;
  window.openingMenuVisible = false;
  window.status = "sdl3_unavailable";
  return window;
#endif
}

}  // namespace

int runProductApp(int argc, char** argv) {
  const ProductAppOptionsParseResult parsed = parseProductAppOptions(argc, argv);
  if (parsed.status == ProductAppOptionStatus::Help) {
    std::cout << productAppHelpText();
    return 0;
  }
  if (parsed.status != ProductAppOptionStatus::Ok) {
    RenderReceipt receipt;
    appendReceiptField(receipt, "app", "iggy3d");
    appendReceiptField(receipt, "result", "fail");
    appendReceiptField(receipt, "reason_code", productAppOptionStatusReason(parsed.status));
    appendReceiptField(receipt, "option", parsed.option);
    std::cout << formatRenderReceipt(receipt);
    return 2;
  }

  const ProductAppOptions& options = parsed.options;
  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  ProductSaveBridgeResult saves =
      scanProductSaves(options.saveRoot, world.packageId, world.scenarioId);
  const FrontendSettings settings = productFrontendSettingsFromOptions(options);
  std::optional<Session> activeSession;
  WorldSetupDraft worldSetupDraft = makeProductDefaultWorldSetupDraft();
  ProductAppWindowState window;
  recordWorldSetupDraftState(worldSetupDraft, window);

  FrontendState frontend;
  initializeProductStarterTransition(frontend, window, saves.slots.compatibleCount > 0);

  if (options.autoNewWorld) {
    launchProductNewWorld(options, worldSetupDraft, frontend, activeSession, window);
  }
  if (options.scriptedGameplaySmoke) {
    runScriptedProductGameplaySmoke(activeSession, window);
    ActionState scriptedLook;
    recordAction(scriptedLook, InputAction::PlayerLookX, true, false, false, 1.0F);
    recordAction(scriptedLook, InputAction::PlayerLookY, true, false, false, 0.5F);
    applyProductCameraActions(scriptedLook, window.viewport, settings, "scripted");
  }

  FrontendSettingsTab automationSettingsTab = FrontendSettingsTab::None;
  bool automationCloseRequested = false;
  applyProductAutomationControl(options, frontend, saves, automationSettingsTab,
                                activeSession, worldSetupDraft, window,
                                automationCloseRequested);
  if (!automationCloseRequested) {
    runProductGameplayTapeFromOptions(options, activeSession, window);
  }
  saves = scanProductSaves(options.saveRoot, world.packageId, world.scenarioId);
  if (automationCloseRequested) {
    window.status = "automation_close_requested";
  }

  window =
      runOpeningMenuWindow(options, world, frontend, activeSession, worldSetupDraft,
                           window, settings, saves);
  refreshGameplayProjectionMetrics(activeSession, window, settings.devToolsEnabled,
                                   settings.debugOverlayEnabled);

  if (options.printRenderReceipt) {
    std::cout << formatRenderReceipt(
        buildProductAppReceipt(options, world, frontend, settings, window, saves));
  }

  return window.requested && !window.created ? 77 : 0;
}

}  // namespace iggy3d
