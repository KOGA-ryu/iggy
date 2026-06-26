#include "app/iggy3d/ProductGameplayProjectionRefresh.hpp"

#include <cstdint>
#include <string>

#include "app/iggy3d/ProductGameplayFeedback.hpp"
#include "app/iggy3d/ProductGameplayTapeRunner.hpp"
#include "render/vulkan/BufferImageResources.hpp"
#include "runtime/ai/NpcBehaviorDebugSnapshot.hpp"
#include "runtime/ai/NpcBehaviorProfile.hpp"

namespace iggy3d {

namespace {

bool npcBehaviorHudHasUnresolvedProfile(const ProductNpcBehaviorDebugHud& hud) {
  // branch-gate: BG-1025
  for (const ProductNpcBehaviorDebugHudLine& line : hud.lines) {
    // branch-gate: BG-1025
    if (line.text.find("unresolved=") != std::string::npos) {
      return true;
    }
  }
  return false;
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
  // branch-gate: BG-1025
  if (!room.loaded || room.meshes.empty()) {
    return;
  }
  const vulkan::RoomMeshCpuGeometry geometry =
      vulkan::buildRoomMeshCpuGeometry(room);
  viewport.productVulkanRoomMeshCpuReady = geometry.ready;
  viewport.productVulkanRoomMeshBackendPresented = backendPresented;
  viewport.productVulkanRoomMeshSource = "scene_room_projection";
  // branch-gate: BG-1025
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

}  // namespace

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

void applyGameplayProjectionMetrics(ProductAppWindowState& window,
                                    const SceneProjectionResult* scene,
                                    const DebugProjectionResult* debug,
                                    const ProductPrimitiveDrawList* drawList,
                                    const ProductViewportFrame* frame,
                                    const ProductRenderBridgeFrame* bridge,
                                    bool viewVisible) {
  // branch-gate: BG-1025
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
  // branch-gate: BG-1025
  window.debugItemCount =
      debug == nullptr ? 0U : static_cast<std::uint64_t>(debug->items.size());
  window.playerVisible = scene->playerCount > 0;
  window.roomVisible = true;
  window.objectiveVisible =
      scene->pickupCount > 0 || scene->interactableCount > 0 || scene->markerCount > 0 ||
      scene->room.loaded;
  window.rendererMutatedRuntime = false;
  applyProductVulkanRoomMeshProof(window.viewport, scene->room);
  // branch-gate: BG-1025
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
  // branch-gate: BG-1025
  if (frame != nullptr) {
    window.viewport.productViewProjection = frame->projectionMode;
    window.viewport.productViewYawApplied = frame->yawApplied;
    window.viewport.productViewPitchApplied = frame->pitchApplied;
    window.viewport.productViewPlayerAnchorFound = frame->playerAnchorFound;
  }
  // branch-gate: BG-1025
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

void refreshProductGameplayProjectionMetrics(
    const ProductGameplayProjectionRefreshRequest& request) {
  ProductAppWindowState& window = request.window;
  // branch-gate: BG-1025
  if (!window.gameplayActive || !request.activeSession.has_value()) {
    window.sessionOutcome = "None";
    copyProductRoomEditorOverlay(
        window, buildProductRoomEditorOverlay(window.roomEditorCursor, false));
    copyNpcBehaviorDebugHud(window,
                            buildProductNpcBehaviorDebugHud(nullptr,
                                                            window.gameplayActive,
                                                            request.developerToolsEnabled,
                                                            request.debugOverlayEnabled));
    applyGameplayProjectionMetrics(window, nullptr, nullptr, nullptr, nullptr, nullptr,
                                   false);
    return;
  }

  // branch-gate: BG-1025
  const RoomAsset* activeRoom =
      window.activeRoom.loaded ? &window.activeRoom.room : nullptr;
  const SceneProjectionResult scene =
      buildSceneProjection(request.activeSession->state(), activeRoom);
  const DebugProjectionResult debug =
      buildProductDebugProjectionWithNpcBehavior(request.activeSession->state());
  copyNpcBehaviorDebugHud(window,
                          buildProductNpcBehaviorDebugHud(&debug,
                                                          window.gameplayActive,
                                                          request.developerToolsEnabled,
                                                          request.debugOverlayEnabled));
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
  window.runtimeStateHash = request.activeSession->stateHash();
  window.sessionOutcome = std::string(productGameplayTapeSessionOutcomeName(
      request.activeSession->state().outcome));
  applyGameplayProjectionMetrics(window, &scene, &debug, &drawList, &frame, &bridge,
                                 true);
}

}  // namespace iggy3d
