#include "EditorPreviewFrame.hpp"
#include "EditorPreviewFrameInternal.hpp"
#include "EditorOverlayAssemblersInternal.hpp"
#include "EditorOverlayWireframesInternal.hpp"

#include <algorithm>
#include <vector>

#include "EditorAssetReplacement.hpp"
#include "EditorAssetScatter.hpp"
#include "EditorFrame.hpp"
#include "EditorPlacementFeedback.hpp"
#include "EditorPlacementGridOverlay.hpp"
#include "EditorRoomPlacement.hpp"
#include "EditorState.hpp"
#include "EditorStructuralPlacement.hpp"
#include "app/iggy3d/creative/input/HeldItemRegistry.hpp"
#include "app/iggy3d/creative/tools/AttachmentSnap.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
using namespace iggy3d;
namespace {

void resetCreativeEditorOverlayFrame(CreativeEditorOverlayFrame& output) {
  output.uiRects.clear();
  output.glyphs.clear();
  output.combinedWireLines.clear();
  output.placementGridLineCount = 0;
  output.placementGridDotCount = 0;
  output.placementGridGuideLineCount = 0;
  output.placementGridAnchorGuideLineCount = 0;
  output.placementGridAnchorCandidateLineCount = 0;
  output.placementGridContactGuideLineCount = 0;
  output.placementGridTargetMarkerCount = 0;
  output.placementInvalidTargetEdgeCount = 0;
  output.placementBlockerEdgeCount = 0;
  output.placementGridClipped = false;
  output.documentWireLineCount = 0;
  output.generatedScopeActive = false;
  output.generatedScopeObjectCount = 0;
  output.generatedScopeVisibleObjectCount = 0;
  output.generatedScopeEdgeCount = 0;
  output.architectureScaleGuideActive = false;
  output.architectureScaleGuideLineCount = 0;
  output.worldLayoutRoofHandleEdgeCount = 0;
  output.worldLayoutVerticalConnectorHandleEdgeCount = 0;
  output.architecturalDimensions = {};
  output.pointMarkerEdgeCount = 0;
  output.lineMarkerEdgeCount = 0;
  output.pathPointHandleEdgeCount = 0;
  output.measurementEdgeCount = 0;
  output.movingPlatformPathPreviewEdgeCount = 0;
  output.playerSpawnPreviewActive = false;
  output.playerSpawnPreviewAccepted = false;
  output.playerSpawnPreviewStatus =
      cr::CreativePlayerSpawnStatus::NotRequested;
  output.playerSpawnPreviewReasonCode =
      "creative_player_spawn_not_requested";
  output.playerSpawnPreviewEdgeCount = 0;
  output.playerSpawnPreviewLabelGlyphCount = 0;
  output.structuralSpanEditEdgeCount = 0;
  output.roomPlacementEdgeCount = 0;
  output.ghostEdgeCount = 0;
  output.materialBrushPivotEdgeCount = 0;
  output.materialBrushGuideLineCount = 0;
  output.materialBrushEdgeCount = 0;
  output.connectedFillEdgeCount = 0;
  output.surfaceExtrudeEdgeCount = 0;
  output.terrainSourceImpactActive = false;
  output.terrainSourceImpactControlCount = 0;
  output.terrainSourceImpactMaterialCellCount = 0;
  output.terrainSourceImpactEdgeCount = 0;
  output.terrainSourceImpactClipped = false;
  output.terrainEdgeCount = 0;
  output.volumeEdgeCount = 0;
  output.volumeExteriorEdgeCount = 0;
  output.volumeInteriorEdgeCount = 0;
  output.volumeChangedMemberEdgeCount = 0;
  output.volumeUnchangedMemberEdgeCount = 0;
  output.volumeProtectedMemberEdgeCount = 0;
  output.volumeDependentSourceEdgeCount = 0;
  output.volumeBlockedMemberEdgeCount = 0;
  output.volumeHandleEdgeCount = 0;
  output.patternEdgeCount = 0;
  output.transformPreviewEdgeCount = 0;
  output.assetReplacementEdgeCount = 0;
  output.assetScatterEdgeCount = 0;
  output.attachmentSocketMarkerEdgeCount = 0;
  output.assetCollisionPreviewEdgeCount = 0;
  output.placementFeedbackEdgeCount = 0;
  output.logicLinkEdgeCount = 0;
  output.logicLinkShaftCount = 0;
  output.logicLinkArrowEdgeCount = 0;
  output.logicLinkEndpointEdgeCount = 0;
  output.logicLinkLabelGlyphCount = 0;
  output.invalidLogicLinkCount = 0;
  output.placementVisualization = {};
}

}  // namespace

CreativeEditorWorldOverlayFacts buildCreativeEditorWorldWireframes(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output,
    const CreativeEditorPlacementVisualizationReceipt*
        placementVisualization) {
  cr::CreativeAppState& appState = request.appState;
  CreativeEditorState& editor = request.editor;
  const cr::CreativeDocument& document = appState.facade.document();
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const bool modalOpen = editor.catalog.model.open ||
                         editor.catalog.toolWheel.open ||
                         editor.toolOptions.open ||
                         editor.assetReplacement.active ||
                         editor.transform.active;
  const CreativeEditorOverlaySelectionSnapshot selection{
      request.selection.selectedId,
      request.selection.selected,
      request.selection.selectedObjectIds,
      request.selection.selectionCount,
      request.selection.hasSelection};

  resetCreativeEditorOverlayFrame(output);
  if (placementVisualization != nullptr) {
    output.placementVisualization = *placementVisualization;
  }

  const CreativeEditorOverlayDocumentAssemblyRequest documentRequest{
      document,
      request.wireProjectionRequest,
      editor.worldLayout,
      editor.generatedSourceScopeCache,
      editor.groupFocus,
      editor.interaction.movingPlatformPathEdit,
      request.gizmoFrame,
      selection,
      request.gizmoThickness,
      editor.volume.active};
  const CreativeEditorOverlayDocumentPlan documentPlan =
      prepareCreativeEditorOverlayDocument(documentRequest, output);

  std::vector<RenderCreativeWireframeDebugLine>& combinedWireLines =
      output.combinedWireLines;
  combinedWireLines.reserve(
      documentPlan.wireframe.lines.size() +
      document.logicLinks().size() * 15U + 84U +
      37U +
      kMaxStaticMeshAttachmentSocketCount * 3U +
      editor.interaction.assetScatter.preview.candidateCount * 12U +
      cr::kCreativePlacementGridMaximumLineCount +
      cr::kCreativePlacementGridMaximumDotCount +
      cr::kCreativePlacementAnchorCandidateCapacity * 3U + 4U);

  appendCreativeEditorOverlayDocumentBase(
      documentRequest, documentPlan, output);
  appendCreativeEditorArchitectureScaleGuide(request, output);
  appendCreativeEditorPlacementGridOverlay(request, output);

  const CreativeEditorOverlayWorldAssemblyRequest worldRequest{
      document,
      appState.facade.measurementState(),
      editor.worldLayout,
      editor.interaction.movingPlatformPathEdit,
      editor.interaction.target,
      editor.toolSettings,
      held,
      selection.selected,
      request.inputContext,
      request.gizmoThickness,
      request.captureMode,
      modalOpen};
  appendCreativeEditorOverlayWorldLayoutRoofHandles(worldRequest, output);
  appendCreativeEditorOverlayWorldLayoutVerticalConnectorHandles(
      worldRequest, output);

  const CreativeEditorOverlayPlacementAssemblyRequest placementRequest{
      document,
      held,
      editor.interaction.placementFeedback,
      placementVisualization,
      request.assetCatalog,
      editor.frameIndex,
      request.gizmoThickness};
  appendCreativeEditorOverlayAssetCollision(placementRequest, output);

  appendCreativeEditorOverlayDocumentInteraction(
      documentRequest, documentPlan, output);
  appendCreativeEditorOverlayMeasurement(worldRequest, output);
  appendCreativeEditorOverlayMovingPlatformPath(worldRequest, output);

  const bool structuralEditVisible =
      !request.captureMode &&
      request.inputContext == cr::CreativeInputContext::EditorViewport &&
      !modalOpen;
  if (structuralEditVisible) {
    output.structuralSpanEditEdgeCount =
        appendCreativeEditorStructuralSpanEditWireframe(
            appState, editor.interaction.structuralSpanEdit,
            std::max(0.035F, request.gizmoThickness),
            combinedWireLines);
    output.roomPlacementEdgeCount =
        appendCreativeEditorRoomPlacementWireframe(
            appState, editor,
            std::max(0.035F, request.gizmoThickness),
            combinedWireLines);
  }

  const CreativeEditorOverlayRelationshipsAssemblyRequest
      relationshipsRequest{
          document,
          selection,
          editor.interaction.target,
          editor.logicLinks,
          held,
          editor.toolSettings,
          request.frame,
          request.assetCatalog,
          request.drawableWidth,
          request.drawableHeight,
          request.gizmoThickness,
          request.captureMode,
          modalOpen};
  appendCreativeEditorOverlayAttachmentSockets(
      relationshipsRequest, output);
  appendCreativeEditorOverlayPlacementFeedback(
      placementRequest, output);
  appendCreativeEditorPlacementClearanceWireframes(request, output);
  appendCreativeEditorOverlayLogicLinks(relationshipsRequest, output);

  output.assetReplacementEdgeCount =
      appendCreativeEditorAssetReplacementWireframes(
          editor.assetReplacement,
          std::max(0.06F, request.gizmoThickness * 1.2F),
          combinedWireLines);
  output.assetScatterEdgeCount =
      appendCreativeEditorAssetScatterWireframes(
          editor, std::max(0.05F, request.gizmoThickness),
          combinedWireLines);
  appendCreativeEditorMaterialBrushWireframe(request, output);
  appendCreativeEditorConnectedFillWireframe(request, output);
  appendCreativeEditorSurfaceExtrudeWireframe(request, output);
  const CreativeEditorVolumePreviewFacts volumeFacts =
      appendCreativeEditorVolumeAndToolWireframes(request, output);
  return {volumeFacts, documentPlan.hasSelection};
}

}  // namespace iggy3d_creative_app
