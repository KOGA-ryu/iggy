#include "app/iggy3d/gameplay/ProjectionRefresh.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

#include "app/iggy3d/gameplay/TapeRunner.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/view/CreativeFlyAnchorStore.hpp"
#include "render/vulkan/BufferImageResources.hpp"
#include "runtime/ai/NpcBehaviorDebugSnapshot.hpp"
#include "runtime/ai/NpcBehaviorProfile.hpp"
#include "runtime/physics/PhysicsDebugSnapshot.hpp"

namespace iggy3d {

namespace {

bool npcBehaviorHudHasUnresolvedProfile(const NpcBehaviorDebugHud& hud) {
  // branch-gate: BG-1025
  for (const NpcBehaviorDebugHudLine& line : hud.lines) {
    // branch-gate: BG-1025
    if (line.text.find("unresolved=") != std::string::npos) {
      return true;
    }
  }
  return false;
}

struct ProductHudSurfacePolicy {
  bool gameplayHudVisible = false;
  bool roomEditorHudVisible = false;
};

ProductHudSurfacePolicy productHudSurfacePolicy(
    const FrontendState& frontend,
    const ProductAppWindowState& window,
    const creative::CreativeAppState* creativeApp) {
  if (productCreativeDocumentEditorActiveForSource(window, creativeApp)) {
    return {};
  }

  const ProductActiveSurfaceFrame surface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(frontend, window));
  ProductHudSurfacePolicy policy;
  policy.gameplayHudVisible =
      surface.activeSurface == ProductFrontendSurface::Gameplay &&
      surface.inputOwner == MenuOwner::Gameplay &&
      !surface.gameplayInputSuppressed;
  policy.roomEditorHudVisible =
      surface.activeSurface == ProductFrontendSurface::Editor &&
      surface.inputOwner == MenuOwner::Editor;
  return policy;
}

void applyGameplayFeedbackVisibility(GameplayFeedback& feedback,
                                     bool visible) {
  feedback.visible = feedback.visible && visible;
  feedback.targetFeedbackVisible =
      feedback.targetFeedbackVisible && feedback.visible;
  feedback.commandFeedbackVisible =
      feedback.commandFeedbackVisible && feedback.visible;
  feedback.reachFeedbackVisible =
      feedback.reachFeedbackVisible && feedback.visible;
  feedback.combatFeedbackVisible =
      feedback.combatFeedbackVisible && feedback.visible;
  feedback.interactionFeedbackVisible =
      feedback.interactionFeedbackVisible && feedback.visible;
  for (GameplayFeedbackLine& line : feedback.lines) {
    line.visible = line.visible && feedback.visible;
  }
}

void appendPhysicsDebugFromMovement(DebugProjectionResult& debug,
                                    const SessionState& state,
                                    bool includeGeometry) {
  const MovementResult& movement = state.transient.lastMovementResult;
  // branch-gate: BG-1109
  if (!state.transient.lastMovementResultAvailable ||
      !movement.physicsFrameStatsAvailable) {
    return;
  }

  const PhysicsDebugSnapshot snapshot = buildPhysicsDebugSnapshot(
      PhysicsDebugSnapshotRequest{&movement.physicsFrameStats, {}});
  appendPhysicsDebugSnapshot(debug, snapshot);
  // branch-gate: BG-1109
  if (!includeGeometry || !movement.physicsDebugGeometryAvailable) {
    return;
  }

  appendPlayerPhysicsMovePlannerDebugProjection(
      debug,
      PlayerPhysicsMovePlannerDebugProjectionRequest{
          &movement.physicsDebugAabbColliders,
          &movement.physicsDebugHits,
      });
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

// Camera-basis normalize with a deliberately TIGHTER degeneracy cutoff (1e-6) than core
// normalizedOr (1e-20): within ~0.06 deg of vertical the near-zero right-vector cross snaps to the
// fallback instead of normalizing numerical noise (branch-gate: BG-1027). Kept local because that
// threshold is a real gimbal-stability requirement, not lazy duplication; crossProduct now
// delegates to core cross().
Vec3 cameraBasisNormalizedOr(Vec3 value, Vec3 fallback) {
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
  const Vec3 f = cameraBasisNormalizedOr(forward, {0.0F, 0.0F, -1.0F});
  const Vec3 r = cameraBasisNormalizedOr(cross(f, up), {1.0F, 0.0F, 0.0F});
  const Vec3 u = cross(r, f);
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

}  // namespace

const SceneProjectionResult* ProductGameplayProjectionFrame::scenePtr() const {
  // branch-gate: BG-1027
  return hasGameplayProjection ? &scene : nullptr;
}

const DebugProjectionResult* ProductGameplayProjectionFrame::debugPtr() const {
  // branch-gate: BG-1027
  return hasGameplayProjection ? &debug : nullptr;
}

const ProductPrimitiveDrawList* ProductGameplayProjectionFrame::drawListPtr() const {
  // branch-gate: BG-1027
  return hasGameplayProjection ? &drawList : nullptr;
}

const ProductViewportFrame* ProductGameplayProjectionFrame::viewportFramePtr() const {
  // branch-gate: BG-1027
  return hasGameplayProjection ? &viewportFrame : nullptr;
}

const ProductRenderBridgeFrame* ProductGameplayProjectionFrame::renderBridgePtr() const {
  // branch-gate: BG-1027
  return hasGameplayProjection ? &renderBridge : nullptr;
}

DebugProjectionResult buildProductDebugProjectionWithNpcBehavior(
    const SessionState& state,
    bool developerToolsEnabled,
    bool debugOverlayEnabled) {
  DebugProjectionResult debug = buildDebugProjection(state);
  const NpcBehaviorProfileCatalog catalog =
      makeBuiltInNpcBehaviorProfileCatalog();
  const NpcBehaviorDebugSnapshot snapshot = buildNpcBehaviorDebugSnapshot(
      {true, &state.world, &state.ai, &state.combat, &catalog,
       state.clock.tickIndex, 128});
  appendNpcBehaviorDebugSnapshot(debug, snapshot);
  appendPhysicsDebugFromMovement(debug, state,
                                 developerToolsEnabled && debugOverlayEnabled);
  return debug;
}

void copyNpcBehaviorDebugHud(ProductAppWindowState& window,
                             const NpcBehaviorDebugHud& hud) {
  window.npcBehaviorDebugHud.visible = hud.visible;
  window.npcBehaviorDebugHud.debugAvailable = hud.debugAvailable;
  window.npcBehaviorDebugHud.lineCount = static_cast<std::uint64_t>(hud.lineCount);
  window.npcBehaviorDebugHud.status = hud.status;
  window.npcBehaviorDebugHud.reasonCode = hud.reasonCode;
  window.npcBehaviorDebugHud.hasUnresolvedProfile =
      npcBehaviorHudHasUnresolvedProfile(hud);
}

void copyPhysicsDebugHud(ProductAppWindowState& window,
                         const PhysicsDebugHud& hud) {
  window.physicsDebugHud = hud;
}

void copyPositionHud(ProductAppWindowState& window,
                     const PositionHud& hud) {
  window.positionHud = hud;
}

void copyProductRoomEditorOverlay(ProductAppWindowState& window,
                                  const ProductRoomEditorOverlay& overlay) {
  window.roomEditorOverlay.visible = overlay.visible;
  window.roomEditorOverlay.status = overlay.status;
  window.roomEditorOverlay.reasonCode = overlay.reasonCode;
  window.roomEditorOverlay.itemCount = overlay.itemCount;
  window.roomEditorOverlay.worldX = overlay.worldPosition.x;
  window.roomEditorOverlay.worldY = overlay.worldPosition.y;
  window.roomEditorOverlay.worldZ = overlay.worldPosition.z;
}

void copyProductRoomEditorHud(ProductAppWindowState& window,
                              const ProductRoomEditorHud& hud) {
  window.roomEditorHud = hud;
}

void copyInteractionModeHud(ProductAppWindowState& window,
                                   const InteractionModeHud& hud) {
  window.interactionModeHud = hud;
}

void copyTopDownMapOverlay(ProductAppWindowState& window,
                                  const TopDownMapOverlay& overlay) {
  window.topDownMap.visible = overlay.visible;
  window.topDownMap.purpose = overlay.purpose;
  window.topDownMap.size = overlay.size;
  window.topDownMap.status = overlay.status;
  window.topDownMap.reasonCode = overlay.reasonCode;
  window.topDownMap.itemCount = overlay.itemCount;
}

void copyProductRoomEditorPreviewOverlay(
    ProductAppWindowState& window,
    const ProductRoomEditorPreviewOverlay& overlay) {
  window.roomEditorPreview.visible = overlay.visible;
  if (window.roomEditorPreview.active) {  // branch-gate: BG-1050
    window.roomEditorPreview.status = window.roomEditorPlacementPreview.status;
  } else {
    const bool shouldCopyOverlayStatus =
        window.roomEditorPreview.status == "room_editor_preview_not_requested";
    if (shouldCopyOverlayStatus) {  // branch-gate: BG-1052
      window.roomEditorPreview.status = overlay.status;
    }
  }
  if (window.roomEditorPreview.active) {  // branch-gate: BG-1050
    window.roomEditorPreview.reasonCode =
        window.roomEditorPlacementPreview.reasonCode;
  } else {
    const bool shouldCopyOverlayReason =
        window.roomEditorPreview.reasonCode == "room_editor_preview_not_requested";
    if (shouldCopyOverlayReason) {  // branch-gate: BG-1052
      window.roomEditorPreview.reasonCode = overlay.reasonCode;
    }
  }
  window.roomEditorPreview.candidateId = overlay.candidateId;
  window.roomEditorPreview.tool = overlay.toolName;
  window.roomEditorPreview.gridX = overlay.gridX;
  window.roomEditorPreview.gridZ = overlay.gridZ;
  window.roomEditorPreview.optimizedDrawDelta = overlay.optimizedDrawDelta;
  window.roomEditorPreview.optimizedTriangleDelta =
      overlay.optimizedTriangleDelta;
}

const ProductRoomEditorPlacementPreviewResult* activeRoomEditorPlacementPreview(
    const ProductAppWindowState& window) {
  // branch-gate: BG-1050
  if (window.roomEditorPreview.active) {
    return &window.roomEditorPlacementPreview;
  }
  return nullptr;
}

Vec3 playerAnchorFromScene(const SceneProjectionResult& scene, bool& found) {
  for (const SceneItem& item : scene.items) {
    // branch-gate: BG-1205
    if (item.kind == SceneItemKind::Player || item.stableName == "player") {
      found = true;
      return item.transform.position;
    }
  }
  found = false;
  return {};
}

Vec3 mapMakerAnchorFor(ProductAppWindowState& window,
                       const SceneProjectionResult& scene) {
  if (productCreativeFlyAnchorFreshForEpoch(window.viewport.creativeFlyAnchor,
                                            window.creativeWorldEpoch)) {
    return window.viewport.creativeFlyAnchor.positionMeters;
  }
  bool playerFound = false;
  const Vec3 playerPosition = playerAnchorFromScene(scene, playerFound);
  // branch-gate: BG-1205
  if (playerFound) {
    seedCreativeFlyAnchorFromScene(window, playerPosition);
    return playerPosition;
  }
  return {};
}

ProductMapMakerGridSnapshot buildMapMakerGridForFrame(
    ProductAppWindowState& window,
    const SceneProjectionResult& scene,
    bool mapMakerLive,
    bool creativeStageGridLive) {
  const bool active = window.gameplayActive && mapMakerLive;
  // F0: the blank creative stage shows the SAME grid+ground reference without
  // being the LegacyMapMaker surface, so gate the grid geometry on either lane
  // while leaving the map_maker status/fly semantics keyed to mapMakerLive only.
  const bool gridActive =
      window.gameplayActive && (mapMakerLive || creativeStageGridLive);
  // branch-gate: BG-1205
  window.mapMakerStatus = active ? "map_maker_active" : "map_maker_inactive";
  window.mapMakerReasonCode = window.mapMakerStatus;
  // branch-gate: BG-1205
  if (!active) {
    window.viewport.creativeFlyActive = false;
    window.viewport.creativeFlyStatus = "creative_fly_not_requested";
    window.viewport.creativeFlyReasonCode = window.viewport.creativeFlyStatus;
    window.viewport.creativeFlySpeedMetersPerSecond = 0.0F;
  }
  const Vec3 anchor = mapMakerAnchorFor(window, scene);
  ProductMapMakerGridConfig config;
  config.enabled = gridActive;
  config.anchorWorld = anchor;
  // The LegacyMapMaker grid follows the fly camera's working elevation
  // (floor(anchor.y)) so you author at whatever height you climb to. The F0
  // creative blank stage is different: the grid is the GROUND the origin sits
  // on, so it must stay at Y=0. Pinning it to the camera height (6m) parked the
  // only visual reference at eye level, off-screen under a downward-pitched
  // camera — the blank stage looked empty ("creative doesn't launch").
  config.planeY = mapMakerLive ? std::floor(anchor.y) : 0.0F;
  const ProductMapMakerGridSnapshot grid =
      buildProductMapMakerGridSnapshot(config);
  window.mapMakerGridVisible = grid.visible;
  window.mapMakerGridStatus =
      std::string(productMapMakerGridStatusName(grid.status));
  window.mapMakerGridReasonCode = grid.reasonCode;
  window.mapMakerGridPitchMeters = grid.pitchMeters;
  window.mapMakerGridMajorStepMeters = grid.majorStepMeters;
  window.mapMakerGridPlaneY = grid.planeY;
  window.mapMakerGridLayerCount = grid.layerCount;
  window.mapMakerGridDotCount = grid.dotCount;
  window.mapMakerGridMajorDotCount = grid.majorDotCount;
  return grid;
}

// TODO(map-maker): Build these transient meshes from a selected tool/asset
// model instead of hardcoded grid/cube prototype state.
Vec3 mapMakerDotSizeFor(const ProductMapMakerGridDot& dot,
                        float pitchMeters) {
  const float minorSize = std::clamp(pitchMeters * 0.08F, 0.04F, 0.10F);
  const float majorSize = std::clamp(pitchMeters * 0.14F, 0.07F, 0.16F);
  // branch-gate: BG-1207
  const float size = dot.major ? majorSize : minorSize;
  return {size, size, size};
}

void appendMapMakerGridDotsToScene(const ProductMapMakerGridSnapshot& grid,
                                   SceneProjectionResult& scene) {
  // branch-gate: BG-1207
  if (!grid.visible || grid.dots.empty()) {
    return;
  }
  scene.room.meshes.reserve(scene.room.meshes.size() + grid.dots.size());
  std::uint64_t index = 0;
  for (const ProductMapMakerGridDot& dot : grid.dots) {
    SceneRoomMeshItem mesh;
    // branch-gate: BG-1207
    mesh.id = dot.major ? "map_maker.grid_major_dot_" : "map_maker.grid_dot_";
    mesh.id += std::to_string(index);
    mesh.role = "grid";
    // branch-gate: BG-1207
    mesh.materialId = dot.major ? "map_maker_grid_major_dot"
                                : "map_maker_grid_dot";
    mesh.position = dot.worldPosition;
    mesh.size = mapMakerDotSizeFor(dot, grid.pitchMeters);
    scene.room.meshes.push_back(std::move(mesh));
    ++index;
  }
  scene.room.staticMeshCount = scene.room.meshes.size();
  scene.room.loaded = true;
}

void appendMapMakerCubePreviewToScene(const ProductMapMakerCubePreview& cube,
                                      SceneProjectionResult& scene) {
  // branch-gate: BG-1206
  if (!cube.visible) {
    return;
  }
  SceneRoomMeshItem mesh;
  mesh.id = cube.stableName;
  mesh.role = "prop";
  mesh.materialId = "map_maker_unit_cube";
  mesh.position = cube.centerWorld;
  mesh.size = cube.sizeMeters;
  scene.room.meshes.push_back(std::move(mesh));
  scene.room.staticMeshCount = scene.room.meshes.size();
  scene.room.propVisible = true;
  scene.room.loaded = true;
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
    window.viewport.productDrawPropVisible = false;
    window.viewport.productDrawPropTileCount = 0;
    window.viewport.productDrawRoomEditorCursorVisible = false;
    window.viewport.productDrawRoomEditorCursorCount = 0;
    window.viewport.productDrawRoomEditorPreviewVisible = false;
    window.viewport.productDrawRoomEditorPreviewCount = 0;
    window.viewport.productDrawPhysicsDebugVisible = false;
    window.viewport.productDrawPhysicsDebugItemCount = 0;
    window.viewport.productDrawPhysicsAabbDebugCount = 0;
    window.viewport.productDrawPhysicsContactNormalDebugCount = 0;
    window.viewport.productDrawMapMakerGridVisible = false;
    window.viewport.productDrawMapMakerGridDotCount = 0;
    window.viewport.productDrawMapMakerMajorGridDotCount = 0;
    window.viewport.productDrawMapMakerCubePreviewVisible = false;
    window.viewport.productDrawMapMakerCubePreviewCount = 0;
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
    window.viewport.productRenderBridgeRoomEditorPreviewVisible = false;
    window.viewport.productRenderBridgeRoomEditorPreviewCount = 0;
    window.viewport.productRenderBridgePropVisible = false;
    window.viewport.productRenderBridgePropTileCount = 0;
    window.viewport.productRenderBridgePhysicsDebugVisible = false;
    window.viewport.productRenderBridgePhysicsDebugItemCount = 0;
    window.viewport.productRenderBridgePhysicsAabbDebugCount = 0;
    window.viewport.productRenderBridgePhysicsContactNormalDebugCount = 0;
    window.viewport.productRenderBridgeMapMakerGridVisible = false;
    window.viewport.productRenderBridgeMapMakerGridDotCount = 0;
    window.viewport.productRenderBridgeMapMakerMajorGridDotCount = 0;
    window.viewport.productRenderBridgeMapMakerCubePreviewVisible = false;
    window.viewport.productRenderBridgeMapMakerCubePreviewCount = 0;
    window.viewport.productFeedbackBridgeReady = false;
    window.viewport.productFeedbackBridgeLineCount = 0;
    clearProductVulkanRoomMeshProof(window.viewport);
    window.sceneItemCount = 0;
    window.debugItemCount = 0;
    window.playerVisible = false;
    window.roomVisible = false;
    window.objectiveVisible = false;
    window.rendererMutatedRuntime = false;
    window.viewport.creativeFlyActive = false;
    window.viewport.creativeFlyStatus = "creative_fly_not_requested";
    window.viewport.creativeFlyReasonCode = window.viewport.creativeFlyStatus;
    window.viewport.creativeFlySpeedMetersPerSecond = 0.0F;
    window.mapMakerStatus = "map_maker_inactive";
    window.mapMakerReasonCode = window.mapMakerStatus;
    window.mapMakerGridVisible = false;
    window.mapMakerGridStatus = "map_maker_grid_disabled";
    window.mapMakerGridReasonCode = window.mapMakerGridStatus;
    window.mapMakerGridLayerCount = 0;
    window.mapMakerGridDotCount = 0;
    window.mapMakerGridMajorDotCount = 0;
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
    window.viewport.productDrawPropVisible = drawList->propTileCount > 0U;
    window.viewport.productDrawPropTileCount = drawList->propTileCount;
    window.viewport.productDrawRoomEditorCursorVisible =
        drawList->roomEditorCursorVisible;
    window.viewport.productDrawRoomEditorCursorCount =
        drawList->roomEditorCursorCount;
    window.viewport.productDrawRoomEditorPreviewVisible =
        drawList->roomEditorPlacementPreviewVisible;
    window.viewport.productDrawRoomEditorPreviewCount =
        drawList->roomEditorPlacementPreviewCount;
    window.viewport.productDrawPhysicsDebugVisible =
        drawList->physicsDebugVisible;
    window.viewport.productDrawPhysicsDebugItemCount =
        drawList->physicsDebugItemCount;
    window.viewport.productDrawPhysicsAabbDebugCount =
        drawList->physicsAabbDebugCount;
    window.viewport.productDrawPhysicsContactNormalDebugCount =
        drawList->physicsContactNormalDebugCount;
    window.viewport.productDrawMapMakerGridVisible =
        drawList->mapMakerGridVisible;
    window.viewport.productDrawMapMakerGridDotCount =
        drawList->mapMakerGridDotCount;
    window.viewport.productDrawMapMakerMajorGridDotCount =
        drawList->mapMakerMajorGridDotCount;
    window.viewport.productDrawMapMakerCubePreviewVisible =
        drawList->mapMakerCubePreviewVisible;
    window.viewport.productDrawMapMakerCubePreviewCount =
        drawList->mapMakerCubePreviewCount;
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
    window.viewport.productRenderBridgeRoomEditorPreviewVisible =
        bridge->roomEditorPlacementPreviewVisible;
    window.viewport.productRenderBridgeRoomEditorPreviewCount =
        bridge->roomEditorPlacementPreviewCount;
    window.viewport.productRenderBridgePropVisible = bridge->propVisible;
    window.viewport.productRenderBridgePropTileCount = bridge->propTileCount;
    window.viewport.productRenderBridgePhysicsDebugVisible =
        bridge->physicsDebugVisible;
    window.viewport.productRenderBridgePhysicsDebugItemCount =
        bridge->physicsDebugItemCount;
    window.viewport.productRenderBridgePhysicsAabbDebugCount =
        bridge->physicsAabbDebugCount;
    window.viewport.productRenderBridgePhysicsContactNormalDebugCount =
        bridge->physicsContactNormalDebugCount;
    window.viewport.productRenderBridgeMapMakerGridVisible =
        bridge->mapMakerGridVisible;
    window.viewport.productRenderBridgeMapMakerGridDotCount =
        bridge->mapMakerGridDotCount;
    window.viewport.productRenderBridgeMapMakerMajorGridDotCount =
        bridge->mapMakerMajorGridDotCount;
    window.viewport.productRenderBridgeMapMakerCubePreviewVisible =
        bridge->mapMakerCubePreviewVisible;
    window.viewport.productRenderBridgeMapMakerCubePreviewCount =
        bridge->mapMakerCubePreviewCount;
    window.viewport.productFeedbackBridgeReady = bridge->feedbackReady;
    window.viewport.productFeedbackBridgeLineCount = bridge->feedbackLineCount;
  }
}

ProductGameplayProjectionFrame buildProductGameplayProjectionFrame(
    const ProductGameplayProjectionFrameRequest& request) {
  ProductAppWindowState& window = request.window;
  ProductGameplayProjectionFrame frame;
  const ProductHudSurfacePolicy hudSurface =
      productHudSurfacePolicy(request.frontend, window, request.creativeApp);
  frame.feedback = buildGameplayFeedback(window);
  applyGameplayFeedbackVisibility(frame.feedback, hudSurface.gameplayHudVisible);
  frame.interactionModeHud = buildInteractionModeHud(
      InteractionModeHudRequest{
          window.interactionMode,
          hudSurface.gameplayHudVisible || hudSurface.roomEditorHudVisible,
          hudSurface.roomEditorHudVisible,
      });
  copyInteractionModeHud(window, frame.interactionModeHud);
  frame.topDownMapOverlay = buildTopDownMapOverlay(
      TopDownMapOverlayRequest{request.rendererRequest,
                                      window.interactionMode,
                                      hudSurface.gameplayHudVisible ||
                                      hudSurface.roomEditorHudVisible,
                                      hudSurface.roomEditorHudVisible,
                                      0U,
                                      productCreativeWorldActiveForSource(
                                          window, request.creativeApp)});
  copyTopDownMapOverlay(window, frame.topDownMapOverlay);
  frame.movementHud = buildMovementDebugHud(window,
                                                   request.developerToolsEnabled,
                                                   request.debugOverlayEnabled &&
                                                       hudSurface.gameplayHudVisible);
  frame.npcBehaviorHud =
      buildNpcBehaviorDebugHud(nullptr, hudSurface.gameplayHudVisible,
                                      request.developerToolsEnabled,
                                      request.debugOverlayEnabled);
  copyNpcBehaviorDebugHud(window, frame.npcBehaviorHud);
  frame.physicsHud = buildPhysicsDebugHud(nullptr, hudSurface.gameplayHudVisible,
                                                 request.developerToolsEnabled,
                                                 request.debugOverlayEnabled);
  copyPhysicsDebugHud(window, frame.physicsHud);
  frame.positionHud = buildPositionHud(
      PositionHudRequest{nullptr,
                         hudSurface.gameplayHudVisible,
                         false,
                         request.developerToolsEnabled,
                         request.debugOverlayEnabled,
                         window.viewport.cameraYawDegrees,
                         window.viewport.cameraPitchDegrees});
  copyPositionHud(window, frame.positionHud);
  frame.roomEditorOverlay =
      buildProductRoomEditorOverlay(window.roomEditorCursor, false);
  copyProductRoomEditorOverlay(window, frame.roomEditorOverlay);
  frame.roomEditorPreviewOverlay = buildProductRoomEditorPreviewOverlay(nullptr);
  copyProductRoomEditorPreviewOverlay(window, frame.roomEditorPreviewOverlay);
  frame.roomEditorHud = buildProductRoomEditorHud(
      ProductRoomEditorHudRequest{window.roomEditing,
                                  window.roomEditorCursor,
                                  hudSurface.roomEditorHudVisible,
                                  window.roomEditorLastOperation,
                                  window.roomEditorLastOperationAccepted,
                                  window.roomEditorLastPrimitiveId,
                                  activeRoomEditorPlacementPreview(window)});
  copyProductRoomEditorHud(window, frame.roomEditorHud);

  // branch-gate: BG-1027
  if (!window.gameplayActive || !request.activeSession.has_value()) {
    return frame;
  }

  // branch-gate: BG-1027
  const RoomAsset* activeRoom =
      window.activeRoom.loaded ? &window.activeRoom.room : nullptr;
  SceneProjectionConfig sceneConfig;
  sceneConfig.includeNpcVisionDebug = request.debugOverlayEnabled;
  frame.scene = buildSceneProjection(request.activeSession->state(), activeRoom,
                                     sceneConfig);
  frame.debug = buildProductDebugProjectionWithNpcBehavior(
      request.activeSession->state(),
      request.developerToolsEnabled,
      request.debugOverlayEnabled && hudSurface.gameplayHudVisible);
  const bool mapMakerLive =
      productMapMakerLiveForSource(request.frontend,
                                   window,
                                   request.creativeApp);
  // F0: a creative-document world (the blank stage) shows the ground grid too.
  const bool creativeStageGridLive =
      productCreativeDocumentEditorActiveForSource(window,
                                                   request.creativeApp);
  frame.mapMakerGrid = buildMapMakerGridForFrame(window, frame.scene,
                                                 mapMakerLive,
                                                 creativeStageGridLive);
  frame.mapMakerGridOverlay =
      buildProductMapMakerGridOverlay(frame.mapMakerGrid);
  appendMapMakerGridDotsToScene(frame.mapMakerGrid, frame.scene);
  // TV1-H (TD-8): the creative-document Navigate tool drives the SAME fly
  // camera anchor as map_maker. Mirror the map_maker override path here, gated
  // on Navigate-active-in-creative-document rather than on the map_maker
  // surface, so the fly position feeds the projected camera in either lane.
  const bool creativeNavigateOverride =
      productCreativeDocumentEditorActiveForSource(window,
                                                   request.creativeApp) &&
      window.creativeNavigateActive;
  // F0: even before Navigate is engaged, the blank creative stage frames its
  // fly-camera pose (origin-framed on entry) so the origin grid is in view.
  const bool creativeFlyAnchorFresh =
      productCreativeFlyAnchorFreshForEpoch(window.viewport.creativeFlyAnchor,
                                            window.creativeWorldEpoch);
  frame.cameraAnchorOverrideAvailable =
      (mapMakerLive || creativeNavigateOverride || creativeStageGridLive) &&
      creativeFlyAnchorFresh;
  frame.cameraAnchorOverrideMeters =
      window.viewport.creativeFlyAnchor.positionMeters;
  frame.mapMakerCubePreview = buildProductMapMakerCubePreview(
      mapMakerLive,
      frame.cameraAnchorOverrideMeters,
      window.viewport.cameraYawDegrees,
      frame.mapMakerGrid);
  appendMapMakerCubePreviewToScene(frame.mapMakerCubePreview, frame.scene);
  frame.mapMakerHud = buildProductMapMakerHud(mapMakerLive,
                                              frame.mapMakerGrid,
                                              frame.mapMakerCubePreview);
  frame.roomEditorOverlay =
      buildProductRoomEditorOverlay(window.roomEditorCursor,
                                    hudSurface.roomEditorHudVisible);
  copyProductRoomEditorOverlay(window, frame.roomEditorOverlay);
  // branch-gate: BG-1050
  frame.roomEditorPreviewOverlay = buildProductRoomEditorPreviewOverlay(
      activeRoomEditorPlacementPreview(window));
  copyProductRoomEditorPreviewOverlay(window, frame.roomEditorPreviewOverlay);
  frame.roomEditorHud = buildProductRoomEditorHud(
      ProductRoomEditorHudRequest{window.roomEditing,
                                  window.roomEditorCursor,
                                  hudSurface.roomEditorHudVisible,
                                  window.roomEditorLastOperation,
                                  window.roomEditorLastOperationAccepted,
                                  window.roomEditorLastPrimitiveId,
                                  activeRoomEditorPlacementPreview(window)});
  copyProductRoomEditorHud(window, frame.roomEditorHud);
  frame.drawList = buildProductPrimitiveDrawList(&frame.scene, &frame.debug,
                                                 activeRoom,
                                                 &window.activeRoomCollision,
                                                 &frame.roomEditorOverlay,
                                                 &frame.roomEditorPreviewOverlay,
                                                 &frame.mapMakerGridOverlay,
                                                 &frame.mapMakerCubePreview);
  frame.topDownMapOverlay = buildTopDownMapOverlay(
      TopDownMapOverlayRequest{request.rendererRequest,
                                      window.interactionMode,
                                      hudSurface.gameplayHudVisible ||
                                      hudSurface.roomEditorHudVisible,
                                      hudSurface.roomEditorHudVisible,
                                      frame.drawList.itemCount,
                                      productCreativeWorldActiveForSource(
                                          window, request.creativeApp)});
  copyTopDownMapOverlay(window, frame.topDownMapOverlay);
  frame.viewportFrame = buildProductViewportFrame(
      frame.drawList,
      ProductViewportFrameConfig{window.viewport.cameraYawDegrees,
                                 window.viewport.cameraPitchDegrees,
                                 92.0F,
                                 640.0F,
                                 394.0F,
                                 frame.cameraAnchorOverrideAvailable,
                                 frame.cameraAnchorOverrideMeters});
  frame.hasGameplayProjection = true;
  frame.viewVisible = true;
  frame.sceneItemCount = frame.scene.items.size();
  window.runtimeStateHash = request.activeSession->stateHash();
  frame.feedback = buildGameplayFeedback(window);
  applyGameplayFeedbackVisibility(frame.feedback, hudSurface.gameplayHudVisible);
  frame.interactionModeHud = buildInteractionModeHud(
      InteractionModeHudRequest{
          window.interactionMode,
          hudSurface.gameplayHudVisible || hudSurface.roomEditorHudVisible,
          hudSurface.roomEditorHudVisible,
      });
  copyInteractionModeHud(window, frame.interactionModeHud);
  frame.movementHud = buildMovementDebugHud(window,
                                                   request.developerToolsEnabled,
                                                   request.debugOverlayEnabled &&
                                                       hudSurface.gameplayHudVisible);
  frame.npcBehaviorHud = buildNpcBehaviorDebugHud(&frame.debug,
                                                         hudSurface.gameplayHudVisible,
                                                         request.developerToolsEnabled,
                                                         request.debugOverlayEnabled);
  copyNpcBehaviorDebugHud(window, frame.npcBehaviorHud);
  frame.physicsHud = buildPhysicsDebugHud(&frame.debug,
                                                 hudSurface.gameplayHudVisible,
                                                 request.developerToolsEnabled,
                                                 request.debugOverlayEnabled);
  copyPhysicsDebugHud(window, frame.physicsHud);
  frame.positionHud = buildPositionHud(
      PositionHudRequest{&frame.scene,
                         hudSurface.gameplayHudVisible,
                         false,
                         request.developerToolsEnabled,
                         request.debugOverlayEnabled,
                         window.viewport.cameraYawDegrees,
                         window.viewport.cameraPitchDegrees});
  copyPositionHud(window, frame.positionHud);
  frame.renderBridge =
      buildProductRenderBridgeFrame(&frame.drawList, &frame.viewportFrame,
                                    &frame.feedback);
  return frame;
}

FrameInput makeProductVulkanFrame(const SceneProjectionResult& scene,
                                  const DebugProjectionResult& debug,
                                  std::uint64_t frameIndex,
                                  std::uint32_t viewportWidth,
                                  std::uint32_t viewportHeight,
                                  float cameraYawDegrees,
                                  float cameraPitchDegrees,
                                  bool cameraAnchorOverrideAvailable,
                                  Vec3 cameraAnchorOverrideMeters) {
  constexpr float kPi = 3.14159265358979323846F;
  constexpr float kEyeHeightMeters = 1.7F;
  FrameInput frame;
  frame.viewport = {viewportWidth, viewportHeight,
                    static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight)};
  frame.clock = {scene.sourceTick, frameIndex, 0.0F, 1.0F / 60.0F};
  frame.camera.mode = RenderCameraMode::FirstPerson;
  Vec3 eye{0.0F, kEyeHeightMeters, 0.0F};
  // branch-gate: BG-1027
  for (const SceneItem& item : scene.items) {
    // branch-gate: BG-1027
    if (item.kind == SceneItemKind::Player || item.stableName == "player") {
      eye = item.transform.position + Vec3{0.0F, kEyeHeightMeters, 0.0F};
      break;
    }
  }
  // branch-gate: BG-1205
  if (cameraAnchorOverrideAvailable) {
    eye = cameraAnchorOverrideMeters + Vec3{0.0F, kEyeHeightMeters, 0.0F};
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

void refreshProductGameplayProjectionMetrics(
    const ProductGameplayProjectionRefreshRequest& request) {
  ProductAppWindowState& window = request.window;
  const ProductGameplayProjectionFrame frame = buildProductGameplayProjectionFrame(
      ProductGameplayProjectionFrameRequest{request.activeSession, window,
                                            request.developerToolsEnabled,
                                            request.debugOverlayEnabled,
                                            request.rendererRequest,
                                            request.frontend,
                                            request.creativeApp});
  // branch-gate: BG-1025
  if (!frame.hasGameplayProjection) {
    window.sessionOutcome = "None";
    applyGameplayProjectionMetrics(window, nullptr, nullptr, nullptr, nullptr, nullptr,
                                   false);
    return;
  }

  window.sessionOutcome = std::string(productGameplayTapeSessionOutcomeName(
      request.activeSession->state().outcome));
  applyGameplayProjectionMetrics(window, frame.scenePtr(), frame.debugPtr(),
                                 frame.drawListPtr(), frame.viewportFramePtr(),
                                 frame.renderBridgePtr(), frame.viewVisible);
}

}  // namespace iggy3d
