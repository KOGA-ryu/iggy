#include "EditorOverlayWireframesInternal.hpp"

#include <array>
#include <cstdint>
#include <vector>

#include "EditorPattern.hpp"
#include "EditorShapePreview.hpp"
#include "EditorState.hpp"
#include "EditorTerrain.hpp"
#include "EditorTransform.hpp"
#include "EditorVolume.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace creative = iggy3d::creative;
using namespace iggy3d;
namespace {

[[nodiscard]] RenderLineColor volumeOperationColor(
    cr::CreativeVolumeOperationKind operation) noexcept {
  constexpr std::array<RenderLineColor,
                       static_cast<std::size_t>(
                           cr::CreativeVolumeOperationKind::Count)>
      colors{
          RenderLineColor{0.18F, 0.95F, 0.34F, 1.0F},
          RenderLineColor{0.10F, 0.90F, 0.95F, 1.0F},
          RenderLineColor{1.0F, 0.72F, 0.12F, 1.0F},
          RenderLineColor{1.0F, 0.20F, 0.18F, 1.0F},
          RenderLineColor{0.30F, 0.55F, 1.0F, 1.0F},
      };
  const std::size_t index = static_cast<std::size_t>(operation);
  return index < colors.size() ? colors[index] : colors.front();
}

[[nodiscard]] bool terrainStampHasPositionableFootprint(
    const creative::CreativeTerrainStampPlan& plan) noexcept {
  switch (plan.status) {
    case creative::CreativeTerrainStampPlanStatus::CapacityExceeded:
    case creative::CreativeTerrainStampPlanStatus::HeightOutOfRange:
    case creative::CreativeTerrainStampPlanStatus::NoChange:
    case creative::CreativeTerrainStampPlanStatus::Ready:
      return plan.transformedWidthCells > 0U &&
             plan.transformedDepthCells > 0U;
    case creative::CreativeTerrainStampPlanStatus::NotRequested:
    case creative::CreativeTerrainStampPlanStatus::InvalidStamp:
    case creative::CreativeTerrainStampPlanStatus::InvalidDestination:
    case creative::CreativeTerrainStampPlanStatus::InvalidRequest:
    case creative::CreativeTerrainStampPlanStatus::CoordinateOverflow:
      return false;
  }
  return false;
}

}  // namespace

CreativeEditorVolumePreviewFacts appendCreativeEditorVolumeAndToolWireframes(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  CreativeEditorVolumePreviewFacts facts;
  CreativeEditorState& editor = request.editor;
  std::vector<RenderCreativeWireframeDebugLine>& lines =
      output.combinedWireLines;
  if (editor.volume.active && !editor.transform.active) {
    const creative::CreativeHotbarEntry& held =
        creative::selectedCreativeHotbarEntry(editor.interaction.hotbar);
    facts.terrainRegion =
        held.kind == creative::CreativeHeldItemKind::TerrainRegion;
    facts.terrainStamp =
        facts.terrainRegion && editor.terrain.region.stamp.active;
    if (facts.terrainStamp) {
      const CreativeTerrainStampPreviewCache& preview =
          editor.terrain.region.stamp.preview;
      if (preview.valid &&
          terrainStampHasPositionableFootprint(preview.plan)) {
        facts.selection.phase =
            creative::CreativeVolumeSelectionPhase::Complete;
        facts.selection.firstCell = {preview.plan.targetMinimum.x, 0,
                                     preview.plan.targetMinimum.z};
        facts.selection.secondCell = {preview.plan.targetMaximum.x, 0,
                                      preview.plan.targetMaximum.z};
        const creative::CreativeGridSettings grid =
            request.appState.facade.document().gridSettings();
        facts.selection.origin = grid.origin;
        facts.selection.cellSize = grid.cellSizeMeters;
      }
    } else {
      facts.selection = creativeEditorVolumePreviewSelection(editor.volume);
    }
    const bool regionHidden =
        facts.terrainRegion &&
        (request.captureMode || editor.catalog.model.open ||
         editor.catalog.toolWheel.open || editor.toolOptions.open ||
         editor.controls.open || editor.transform.controlsOpen);
    if (creative::creativeVolumeSelectionValid(facts.selection) &&
        !regionHidden) {
      facts.selectionVisible = true;
      facts.usesShapePlan =
          !facts.terrainRegion &&
          (editor.volume.operation ==
               creative::CreativeVolumeOperationKind::Fill ||
           editor.volume.operation ==
               creative::CreativeVolumeOperationKind::Hollow);
      if (facts.usesShapePlan) {
        creative::CreativeShapeBrushPlanRequest planRequest;
        planRequest.kind = editor.toolSettings.shapeBrushKind;
        planRequest.axis = editor.toolSettings.shapeBrushAxis;
        planRequest.firstCell = facts.selection.firstCell;
        planRequest.secondCell = facts.selection.secondCell;
        planRequest.hollow =
            editor.volume.operation ==
            creative::CreativeVolumeOperationKind::Hollow;
        planRequest.maxCandidateCellCount = kCreativeEditorVolumeCellLimit;
        planRequest.maxGeneratedCellCount = kCreativeEditorVolumeCellLimit;
        facts.shapePlan = creative::planCreativeShapeBrush(planRequest);
      }
      const std::size_t before = lines.size();
      RenderLineColor color = volumeOperationColor(editor.volume.operation);
      if (facts.terrainRegion) {
        const bool accepted =
            facts.terrainStamp
                ? editor.terrain.region.stamp.preview.valid &&
                      editor.terrain.region.stamp.preview.plan.accepted &&
                      editor.terrain.region.stamp.preview.renderAccepted
                : editor.terrain.region.preview.valid &&
                      editor.terrain.region.preview.plan.accepted &&
                      editor.terrain.region.preview.renderAccepted;
        const bool replacing =
            facts.terrainStamp
                ? editor.terrain.region.stamp.preview.mode ==
                      creative::CreativeTerrainStampMode::Replace
                : editor.terrain.region.preview.operation ==
                      creative::CreativeTerrainRegionOperation::Erase;
        color = !accepted
                    ? RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F}
                    : replacing
                          ? RenderLineColor{1.0F, 0.46F, 0.12F, 1.0F}
                          : RenderLineColor{0.20F, 1.0F, 0.35F, 1.0F};
      } else if (facts.usesShapePlan && !facts.shapePlan.accepted) {
        color = RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F};
      }
      appendCreativeShapeBrushOutline(
          lines, facts.selection,
          facts.usesShapePlan ? editor.toolSettings.shapeBrushKind
                              : creative::CreativeShapeBrushKind::Box,
          editor.toolSettings.shapeBrushAxis, color, request.gizmoThickness);
      output.volumeEdgeCount = lines.size() - before;
    }
  }
  if (!editor.transform.active) {
    output.patternEdgeCount = appendCreativeEditorArrayPreview(
        request.appState, editor, request.gizmoThickness, lines);
  }
  output.transformPreviewEdgeCount =
      appendCreativeEditorSelectionTransformPreview(
          editor.transform, request.gizmoThickness, lines);
  const std::size_t terrainBefore = lines.size();
  const CreativeEditorTerrainSourceImpactOverlayFacts sourceImpact =
      appendCreativeEditorWorldLayoutTerrainImpactOverlay(
          request.appState.facade.document(), editor, request.gizmoThickness,
          lines, request.captureMode);
  output.terrainSourceImpactActive = sourceImpact.active;
  output.terrainSourceImpactControlCount = sourceImpact.controlCount;
  output.terrainSourceImpactMaterialCellCount = sourceImpact.materialCellCount;
  output.terrainSourceImpactEdgeCount = sourceImpact.edgeCount;
  output.terrainSourceImpactClipped = sourceImpact.influenceCellsClipped;
  const cr::CreativeDocument& terrainDocument =
      request.terrainDocument == nullptr
          ? request.appState.facade.document()
          : *request.terrainDocument;
  static_cast<void>(refreshCreativeEditorTerrainContours(
      editor.terrain.contours, terrainDocument, request.terrainSurface,
      request.terrainSurfaceKey));
  output.terrainContourEdgeCount = appendCreativeEditorTerrainContours(
      terrainDocument, editor, request.gizmoThickness, lines,
      request.captureMode);
  appendCreativeEditorTerrainOverlay(
      request.appState.facade.document(), editor, request.gizmoThickness, lines,
      request.captureMode);
  output.terrainEdgeCount = lines.size() - terrainBefore;
  return facts;
}

}  // namespace iggy3d_creative_app
