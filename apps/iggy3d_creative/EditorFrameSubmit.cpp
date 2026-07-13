#include "EditorFrame.hpp"

#include <SDL3/SDL.h>

#include <string>

#include "render/vulkan/VulkanBackend.hpp"

#include "EditorFrustumCull.hpp"
#include "EditorPreviewFrame.hpp"

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

[[nodiscard]] bool submitCreativeEditorFrame(
    const CreativeEditorSubmitFrameRequest& request) {
  iggy3d::FrameInput& frame = request.frame;
  const iggy3d::creative::CreativeAppState& appState = request.appState;
  CreativeEditorState& editor = request.editor;
  const CreativeEditorSelectionFrame& selection = request.selection;
  const CreativeEditorOverlayFrame& overlayFrame = request.overlayFrame;
  const StandaloneRoomBakePreviewScene& roomBakePreview =
      request.roomBakePreview;
  const iggy3d::creative::Id selectedId = selection.selectedId;
  const iggy3d::creative::CreativeObject* selected = selection.selected;
  const bool hasSelection = selection.hasSelection;

  const StandaloneFrustumCullResult frustumCull =
      cullStandaloneSceneRoomMeshesByFrustum(
          *frame.projections.scene, frame.camera.clipFromWorld);
  frame.projections.scene = &frustumCull.scene;

  const iggy3d::RenderSubmitResult submit =
      request.backend.submitFrame(frame);
  if (!editor.loggedSelection) {
    editor.loggedSelection = true;
    SDL_Log("iggy3d_creative: frame %llu submit outcome=%d reason='%s' "
            "meshes=%zu frustumInputMeshes=%zu frustumKeptMeshes=%zu "
            "frustumCulledMeshes=%zu frustumConservativeMeshes=%zu "
            "selectedTarget=%u hasSelection=%d selBoxLines=%zu "
            "pointMarkerLines=%zu lineMarkerLines=%zu pathHandleLines=%zu "
            "gizmoLines=%zu combinedWireLines=%zu uiRects=%zu glyphs=%zu",
            static_cast<unsigned long long>(editor.frameIndex),
            static_cast<int>(submit.outcome),
            std::string(submit.reason.code).c_str(),
            frustumCull.scene.room.meshes.size(),
            frustumCull.receipt.inputRoomMeshCount,
            frustumCull.receipt.keptRoomMeshCount,
            frustumCull.receipt.culledRoomMeshCount,
            frustumCull.receipt.conservativelyKeptMeshCount, selectedId,
            hasSelection ? 1 : 0,
            overlayFrame.documentWireLineCount,
            overlayFrame.pointMarkerEdgeCount,
            overlayFrame.lineMarkerEdgeCount,
            overlayFrame.pathPointHandleEdgeCount,
            overlayFrame.combinedWireLines.size() -
                overlayFrame.documentWireLineCount -
                overlayFrame.pointMarkerEdgeCount -
                overlayFrame.lineMarkerEdgeCount -
                overlayFrame.pathPointHandleEdgeCount,
            overlayFrame.combinedWireLines.size(),
            overlayFrame.uiRects.size(), overlayFrame.glyphs.size());
  }

  if (request.maxFrames != 0U && editor.frameIndex >= request.maxFrames) {
    logStandaloneRoomBakeFinal(roomBakePreview);
    // Name the SELECTED object + kind so the capture is self-documenting; the
    // capture proof expects this target to be the FLOOR.
    const char* selKind =
        hasSelection
            ? creative::toString(selected->kind).data()
            : "<none>";
    SDL_Log("iggy3d_creative: FINAL frame %llu submit outcome=%d reason='%s' "
            "selectedTarget=%u selectedKind='%s' hasSelection=%d selBoxLines=%zu "
            "pointMarkerLines=%zu lineMarkerLines=%zu pathHandleLines=%zu "
            "gizmoLines=%zu combinedWireLines=%zu placeMode=%d brush='%s' "
            "ghostEdges=%zu placed=%llu objectCount=%llu "
            "frustumInputMeshes=%zu frustumKeptMeshes=%zu "
            "frustumCulledMeshes=%zu frustumConservativeMeshes=%zu",
            static_cast<unsigned long long>(editor.frameIndex),
            static_cast<int>(submit.outcome),
            std::string(submit.reason.code).c_str(), selectedId, selKind,
            hasSelection ? 1 : 0, overlayFrame.documentWireLineCount,
            overlayFrame.pointMarkerEdgeCount,
            overlayFrame.lineMarkerEdgeCount,
            overlayFrame.pathPointHandleEdgeCount,
            overlayFrame.combinedWireLines.size() -
                overlayFrame.documentWireLineCount -
                overlayFrame.pointMarkerEdgeCount -
                overlayFrame.lineMarkerEdgeCount -
                overlayFrame.pathPointHandleEdgeCount,
            overlayFrame.combinedWireLines.size(), editor.placeMode ? 1 : 0,
            std::string(creative::toString(editor.placeBrush)).c_str(),
            overlayFrame.ghostEdgeCount,
            static_cast<unsigned long long>(editor.placedCount),
            static_cast<unsigned long long>(
                appState.facade.document().objectCount()),
            frustumCull.receipt.inputRoomMeshCount,
            frustumCull.receipt.keptRoomMeshCount,
            frustumCull.receipt.culledRoomMeshCount,
            frustumCull.receipt.conservativelyKeptMeshCount);
    return true;
  }
  return false;
}

}  // namespace iggy3d_creative_app
