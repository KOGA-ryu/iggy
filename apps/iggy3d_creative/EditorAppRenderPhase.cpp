#include "EditorAppFramePhases.hpp"

#include <chrono>
#include <cstdint>
#include <thread>

#include "EditorDesktopUi.hpp"
#include "EditorMovingPlatformPreview.hpp"
#include "EditorTransform.hpp"
#include "render/vulkan/VulkanBackend.hpp"

namespace iggy3d_creative_app {

bool runCreativeEditorAppRenderPhase(
    const CreativeEditorAppRenderPhaseRequest& request) {
  const CreativeEditorSelectionFrame selection =
      resolveCreativeEditorSelectionFrame(request.activeAppState.facade);
  std::uint64_t playerSpawnRoomBakeRevision =
      request.sceneCache.refreshCount;
  if (request.preview.selectedPreview ==
      &request.terrainPreviewCache.preview) {
    playerSpawnRoomBakeRevision =
        request.terrainPreviewCache.refreshCount;
  } else if (request.preview.selectedPreview ==
             &request.volumePreviewCache.scene.preview) {
    playerSpawnRoomBakeRevision =
        request.volumePreviewCache.operationRefreshCount;
  }
  static_cast<void>(refreshCreativePlayerSpawnPreviewCache(
      request.playerSpawnPreviewCache,
      request.activeAppState.facade.document(),
      &request.preview.selectedPreview->roomBake, selection.selected,
      playerSpawnRoomBakeRevision));
  static_cast<void>(syncCreativeMovingPlatformPreview(
      request.editor.movingPlatformPreview,
      request.activeAppState.facade.document().id(), selection.selected));
  static_cast<void>(advanceCreativeMovingPlatformPreview(
      request.editor.movingPlatformPreview,
      request.frame.clock.presentationDeltaSeconds));

  const iggy3d::RenderContentViewport contentViewport =
      iggy3d::effectiveContentViewport(request.frame);
  const CreativeEditorGizmoFrame gizmoFrame =
      buildCreativeEditorGizmoFrame(
          selection, request.editor.interaction.movingPlatformPathEdit,
          request.frame.camera, request.drawableWidth,
          request.drawableHeight, request.gizmoAxisLengthMeters,
          contentViewport, &request.editor.transform);
  logCreativeEditorPathHandleCaptureFrame(
      request.editor.captureScript, request.captureMode, gizmoFrame);

  CreativeEditorOverlayFrame overlayFrame;
  buildAndAttachCreativeEditorOverlayFrame(
      {request.activeAppState,
       request.editor,
       selection,
       gizmoFrame,
       request.frame,
       request.wireProjection,
       request.drawableWidth,
       request.drawableHeight,
       request.gizmoThicknessMeters,
       request.captureMode,
       request.frameInput.inputFrame.context,
       request.frameInput.activeControlDevice,
       &request.assetCatalog,
       &request.sceneCache.placementClearance,
       request.preview.renderDocument,
       request.preview.terrainContourSurface,
       request.preview.terrainContourSurfaceKey,
       &request.playerSpawnPreviewCache.geometry,
       request.frameInput.routedInput.controllerCommandLayerActive},
      overlayFrame);

  endCreativeEditorDesktopFrame(request.editor.desktopUi);
  if (submitCreativeEditorFrame(
          {request.backend,
           request.frame,
           request.activeAppState,
           request.editor,
           selection,
           overlayFrame,
           *request.preview.selectedPreview,
           request.maxFrames})) {
    return true;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(16));
  return false;
}

}  // namespace iggy3d_creative_app
