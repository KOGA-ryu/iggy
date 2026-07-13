#include "EditorPreviewFrame.hpp"
#include "EditorPreviewFrameInternal.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include "EditorConnectedFill.hpp"
#include "EditorFrame.hpp"
#include "EditorTransform.hpp"
#include "EditorGizmo.hpp"
#include "EditorGroup.hpp"
#include "EditorInteraction.hpp"
#include "EditorPattern.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorShapePreview.hpp"
#include "EditorState.hpp"
#include "EditorSurfaceExtrude.hpp"
#include "EditorTerrain.hpp"
#include "EditorVolume.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/DocumentWireframe.hpp"
#include "app/iggy3d/creative/render/CreativeOverlayFrame.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"
#include "projection/debug/DebugProjection.hpp"

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

struct MaterialBrushPreviewPlan {
  bool visible = false;
  bool removing = false;
  bool admitted = false;
  bool hasGuideAnchor = false;
  bool showSymmetryPivot = false;
  cr::CreativeMaterialBrushGuide guide =
      cr::CreativeMaterialBrushGuide::Free;
  cr::CreativeGridCoord3 guideAnchor{};
  cr::CreativeGridCoord3 symmetryPivot{};
  cr::CreativeGridCoord3 center{};
  cr::CreativeMaterialBrushStampPlan stamp{};
  std::array<cr::CreativeGridCoord3,
             cr::kCreativeMaterialBrushSymmetryCapacity>
      plannedDirectCells{};
  std::array<cr::CreativeGridCoord3,
             cr::kCreativeMaterialBrushSymmetryCapacity>
      plannedMirroredCells{};
  std::array<cr::CreativeGridCoord3,
             cr::kCreativeMaterialBrushSymmetryCapacity>
      eligibleDirectCells{};
  std::array<cr::CreativeGridCoord3,
             cr::kCreativeMaterialBrushSymmetryCapacity>
      eligibleMirroredCells{};
  std::uint16_t plannedDirectCellCount = 0U;
  std::uint16_t plannedMirroredCellCount = 0U;
  std::uint16_t eligibleDirectCellCount = 0U;
  std::uint16_t eligibleMirroredCellCount = 0U;

  [[nodiscard]] std::span<const cr::CreativeGridCoord3> renderedDirectCells()
      const noexcept {
    return admitted
               ? std::span<const cr::CreativeGridCoord3>{
                     eligibleDirectCells.data(), eligibleDirectCellCount}
               : std::span<const cr::CreativeGridCoord3>{
                     plannedDirectCells.data(), plannedDirectCellCount};
  }

  [[nodiscard]] std::span<const cr::CreativeGridCoord3>
  renderedMirroredCells() const noexcept {
    return admitted
               ? std::span<const cr::CreativeGridCoord3>{
                     eligibleMirroredCells.data(), eligibleMirroredCellCount}
               : std::span<const cr::CreativeGridCoord3>{
                     plannedMirroredCells.data(), plannedMirroredCellCount};
  }
};

[[nodiscard]] bool materialBrushCellVisited(
    const CreativeMaterialStrokeState& stroke,
    cr::CreativeGridCoord3 cell) noexcept {
  for (std::size_t index = 0U; index < stroke.visitedCount; ++index) {
    const cr::CreativeGridCoord3 visited = stroke.visited[index].cell;
    if (visited.x == cell.x && visited.y == cell.y && visited.z == cell.z) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] MaterialBrushPreviewPlan materialBrushPreviewPlan(
    const CreativeEditorState& editor,
    const cr::CreativeDocument& document) noexcept {
  MaterialBrushPreviewPlan output;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (held.kind != cr::CreativeHeldItemKind::MaterialBrush) {
    return output;
  }
  const CreativeMaterialStrokeState& stroke =
      editor.interaction.materialStroke;
  const CreativeMaterialBrushGestureConfig config =
      stroke.hasBrushAnchor
          ? stroke.brushConfig
          : creativeMaterialBrushGestureConfig(editor.toolSettings);
  cr::CreativeGridCoord3 lockedPivot{};
  const bool hasLockedPivot = creativeMaterialBrushLockedPivot(
      editor.interaction.materialBrushPivot, document.id(), lockedPivot);
  if (config.symmetry != cr::CreativeMaterialBrushSymmetry::Off) {
    if (stroke.hasSymmetryPivot) {
      output.showSymmetryPivot = true;
      output.symmetryPivot = stroke.symmetryPivot;
    } else if (hasLockedPivot) {
      output.showSymmetryPivot = true;
      output.symmetryPivot = lockedPivot;
    }
  }
  if (!editor.interaction.target.grid.valid) {
    return output;
  }
  output.removing = editor.interaction.materialStroke.repeat.active &&
                    editor.interaction.materialStroke.repeat.kind ==
                        CreativeMaterialStrokeKind::Remove;
  if (output.removing && !editor.interaction.target.voxelHit) {
    return output;
  }
  const cr::CreativeGridCoord3 rawCenter =
      output.removing ? editor.interaction.target.voxelCell
                      : editor.interaction.target.grid.adjacentCell;
  const cr::CreativeGridCoord3 anchor =
      stroke.hasBrushAnchor ? stroke.brushAnchor : rawCenter;
  cr::CreativeGridCoord3 center{};
  if (!cr::guideCreativeMaterialBrushCenter(config.guide, anchor, rawCenter,
                                            center)) {
    return output;
  }
  output.hasGuideAnchor = stroke.hasBrushAnchor;
  if (!output.showSymmetryPivot) {
    output.symmetryPivot = anchor;
  }
  output.guide = config.guide;
  output.guideAnchor = anchor;
  output.center = center;
  output.stamp = cr::planCreativeMaterialBrushStamp(
      creativeMaterialBrushStampRequest(config, center));
  output.visible = output.stamp.accepted;
  if (!output.visible) {
    return output;
  }

  const cr::CreativeMaterialBrushSymmetryPlan symmetry =
      cr::planCreativeMaterialBrushSymmetry(
          {config.symmetry, output.symmetryPivot,
           output.stamp.generatedCells()});
  if (!symmetry.accepted) {
    for (cr::CreativeGridCoord3 cell : output.stamp.generatedCells()) {
      output.plannedDirectCells[output.plannedDirectCellCount++] = cell;
    }
    return output;
  }

  const cr::CreativeVoxelField& field = document.voxelField();
  for (std::size_t index = 0U; index < symmetry.cellCount; ++index) {
    const cr::CreativeGridCoord3 cell = symmetry.cells[index];
    const bool mirrored = symmetry.cellIsMirrored(index);
    if (mirrored) {
      output.plannedMirroredCells[output.plannedMirroredCellCount++] = cell;
    } else {
      output.plannedDirectCells[output.plannedDirectCellCount++] = cell;
    }
    const cr::CreativeObjectKind currentMaterial = field.materialAt(cell);
    const bool occupied = currentMaterial != cr::CreativeObjectKind::Unknown;
    const bool allowed =
        output.removing
            ? occupied
            : currentMaterial != held.objectKind &&
                  cr::creativeMaterialBrushPaintAllows(
                      config.mask, currentMaterial,
                      config.replaceSourceKind);
    if (!allowed || materialBrushCellVisited(
                        editor.interaction.materialStroke, cell)) {
      continue;
    }
    if (mirrored) {
      output.eligibleMirroredCells[output.eligibleMirroredCellCount++] = cell;
    } else {
      output.eligibleDirectCells[output.eligibleDirectCellCount++] = cell;
    }
  }
  const std::size_t remainingCapacity =
      editor.interaction.materialStroke.visitedCount <=
              editor.interaction.materialStroke.visited.size()
          ? editor.interaction.materialStroke.visited.size() -
                editor.interaction.materialStroke.visitedCount
          : 0U;
  const bool materialValid =
      output.removing || cr::creativeVolumeBrushSupported(held.objectKind);
  const std::size_t eligibleCellCount =
      output.eligibleDirectCellCount + output.eligibleMirroredCellCount;
  output.admitted = materialValid &&
                    !editor.interaction.materialStroke.capacityReached &&
                    eligibleCellCount > 0U &&
                    eligibleCellCount <= remainingCapacity;
  return output;
}

void resetCreativeEditorOverlayFrame(CreativeEditorOverlayFrame& output) {
  output.uiRects.clear();
  output.glyphs.clear();
  output.combinedWireLines.clear();
  output.documentWireLineCount = 0;
  output.pointMarkerEdgeCount = 0;
  output.lineMarkerEdgeCount = 0;
  output.pathPointHandleEdgeCount = 0;
  output.ghostEdgeCount = 0;
  output.materialBrushPivotEdgeCount = 0;
  output.materialBrushGuideLineCount = 0;
  output.materialBrushEdgeCount = 0;
  output.connectedFillEdgeCount = 0;
  output.surfaceExtrudeEdgeCount = 0;
  output.terrainEdgeCount = 0;
  output.volumeEdgeCount = 0;
  output.patternEdgeCount = 0;
  output.transformPreviewEdgeCount = 0;
  output.placementFeedbackEdgeCount = 0;
}

void appendCreativeEditorPlacementFeedbackWireframe(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  CreativeEditorState& editor = request.editor;
  std::vector<RenderCreativeWireframeDebugLine>& lines =
      output.combinedWireLines;
  const CreativeEditorPlacementFeedback& feedback =
      editor.interaction.placementFeedback;
  if (feedback.status != CreativeEditorPlacementFeedbackStatus::Placed ||
      !creativeEditorPlacementFeedbackVisible(feedback, editor.frameIndex)) {
    return;
  }
  const creative::CreativeObject* placedObject =
      request.appState.facade.findObject(feedback.objectId);
  if (placedObject != nullptr) {
    const VisualBounds placedBounds = visualBoundsForObject(*placedObject);
    const std::size_t before = lines.size();
    appendStandaloneWireframeBoxEdges(
        lines, placedBounds.min, placedBounds.max,
        RenderLineColor{0.25F, 1.0F, 0.35F, 1.0F},
        std::max(0.075F, request.gizmoThickness * 1.4F));
    for (std::size_t index = before; index < lines.size(); ++index) {
      lines[index].objectId = placedObject->id;
    }
    output.placementFeedbackEdgeCount = lines.size() - before;
    return;
  }
  if (!feedback.voxelPlaced) {
    return;
  }
  Vec3 center{};
  Vec3 size{};
  if (!creativePreviewBoundsTransform(feedback.voxelBounds, 1.0F,
                                      center, size)) {
    return;
  }
  const Vec3 minimum =
      cr::creativeVec3ToCoreChecked(feedback.voxelBounds.min).value;
  const Vec3 maximum =
      cr::creativeVec3ToCoreChecked(feedback.voxelBounds.max).value;
  const std::size_t before = lines.size();
  appendStandaloneWireframeBoxEdges(
      lines, minimum, maximum,
      RenderLineColor{0.25F, 1.0F, 0.35F, 1.0F},
      std::max(0.075F, request.gizmoThickness * 1.4F));
  output.placementFeedbackEdgeCount = lines.size() - before;
}

void appendCreativeEditorMaterialBrushWireframe(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  CreativeEditorState& editor = request.editor;
  cr::CreativeAppState& appState = request.appState;
  std::vector<RenderCreativeWireframeDebugLine>& lines =
      output.combinedWireLines;
  const MaterialBrushPreviewPlan preview =
      materialBrushPreviewPlan(editor, appState.facade.document());
  if (editor.transform.active ||
      (!preview.visible && !preview.showSymmetryPivot)) {
    return;
  }
  const RenderLineColor directColor =
      preview.removing
          ? RenderLineColor{1.0F, 0.2F, 0.16F, 1.0F}
          : preview.admitted
                ? RenderLineColor{0.22F, 1.0F, 0.34F, 1.0F}
                : RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F};
  const RenderLineColor mirroredColor =
      preview.admitted ? RenderLineColor{0.18F, 0.9F, 1.0F, 1.0F}
                       : RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F};
  const cr::CreativeGridSettings grid = appState.facade.document().gridSettings();
  const std::size_t pivotBefore = lines.size();
  if (preview.showSymmetryPivot) {
    static_cast<void>(appendCreativeMaterialBrushPivotMarker(
        lines, preview.symmetryPivot, grid,
        std::max(0.045F, request.gizmoThickness * 1.4F)));
  }
  output.materialBrushPivotEdgeCount = lines.size() - pivotBefore;
  const std::size_t guideBefore = lines.size();
  if (preview.visible && preview.hasGuideAnchor) {
    static_cast<void>(appendCreativeMaterialBrushGuideLine(
        lines, preview.guide, preview.guideAnchor, preview.center, grid,
        std::max(0.045F, request.gizmoThickness * 1.4F)));
  }
  output.materialBrushGuideLineCount = lines.size() - guideBefore;
  const std::size_t before = lines.size();
  if (preview.visible) {
    appendCreativeMaterialBrushCellOutlines(
        lines, preview.renderedDirectCells(), grid, directColor,
        request.gizmoThickness);
    appendCreativeMaterialBrushCellOutlines(
        lines, preview.renderedMirroredCells(), grid, mirroredColor,
        request.gizmoThickness);
  }
  output.materialBrushEdgeCount = lines.size() - before;
}

[[nodiscard]] bool creativeEditorWorldPreviewBlocked(
    const CreativeEditorOverlayFrameRequest& request) noexcept {
  const CreativeEditorState& editor = request.editor;
  return request.captureMode || editor.catalog.model.open ||
         editor.catalog.toolWheel.open || editor.toolOptions.open ||
         editor.controls.open || editor.transform.active ||
         editor.transform.controlsOpen;
}

void appendCreativeEditorConnectedFillWireframe(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  CreativeEditorState& editor = request.editor;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (creativeEditorWorldPreviewBlocked(request) ||
      held.kind != cr::CreativeHeldItemKind::ConnectedFill ||
      !editor.interaction.target.voxelHit) {
    return;
  }
  const cr::CreativeConnectedFillPlan& plan =
      resolveCreativeEditorConnectedFillPlan(
          editor.interaction.connectedFill,
          request.appState.facade.document(),
          editor.interaction.target.voxelCell,
          editor.toolSettings.connectedFillLimit);
  const bool valid =
      plan.accepted && cr::creativeVolumeBrushSupported(held.objectKind) &&
      plan.sourceMaterial != held.objectKind;
  const RenderLineColor color =
      valid ? RenderLineColor{0.12F, 0.92F, 1.0F, 1.0F}
            : RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F};
  const std::array<cr::CreativeGridCoord3, 1U> rejectedSeed{
      editor.interaction.target.voxelCell};
  const std::span<const cr::CreativeGridCoord3> cells =
      plan.accepted ? plan.generatedCells()
                    : std::span<const cr::CreativeGridCoord3>{rejectedSeed};
  const std::size_t before = output.combinedWireLines.size();
  appendCreativeMaterialBrushCellOutlines(
      output.combinedWireLines, cells,
      request.appState.facade.document().gridSettings(), color,
      request.gizmoThickness);
  output.connectedFillEdgeCount = output.combinedWireLines.size() - before;
}

void appendCreativeEditorSurfaceExtrudeWireframe(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  CreativeEditorState& editor = request.editor;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (creativeEditorWorldPreviewBlocked(request) ||
      held.kind != cr::CreativeHeldItemKind::SurfaceExtrude ||
      !editor.interaction.target.voxelHit) {
    return;
  }
  cr::CreativeGridCoord3 outward{};
  const bool faceValid = creativeSurfaceFaceOffset(
      editor.interaction.target.grid.faceNormal, outward);
  const cr::CreativeSurfaceExtrudePlan* plan = nullptr;
  if (faceValid) {
    plan = &resolveCreativeEditorSurfaceExtrudePlan(
        editor.interaction.surfaceExtrude,
        request.appState.facade.document(),
        editor.interaction.target.voxelCell, outward,
        cr::CreativeSurfaceExtrudeKind::Extrude,
        editor.toolSettings.surfaceExtrudeDepth,
        editor.toolSettings.surfaceExtrudeLimit);
  }
  const bool valid = plan != nullptr && plan->accepted &&
                     cr::creativeVolumeBrushSupported(held.objectKind);
  const RenderLineColor color =
      valid ? RenderLineColor{0.12F, 0.82F, 1.0F, 1.0F}
            : RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F};
  const std::array<cr::CreativeGridCoord3, 1U> rejectedSeed{
      editor.interaction.target.voxelCell};
  const std::span<const cr::CreativeGridCoord3> cells =
      valid ? plan->generatedCells()
            : std::span<const cr::CreativeGridCoord3>{rejectedSeed};
  const std::size_t before = output.combinedWireLines.size();
  appendCreativeMaterialBrushCellOutlines(
      output.combinedWireLines, cells,
      request.appState.facade.document().gridSettings(), color,
      request.gizmoThickness);
  output.surfaceExtrudeEdgeCount = output.combinedWireLines.size() - before;
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
  appendCreativeEditorTerrainOverlay(
      request.appState.facade.document(), editor, request.gizmoThickness, lines,
      request.captureMode);
  output.terrainEdgeCount = lines.size() - terrainBefore;
  return facts;
}

}  // namespace

CreativeEditorWorldOverlayFacts buildCreativeEditorWorldWireframes(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  cr::CreativeAppState& appState = request.appState;
  CreativeEditorState& editor = request.editor;
  const cr::CreativeSpatialProjectionRequest& wireProjReq =
      request.wireProjectionRequest;
  const float gizmoThickness = request.gizmoThickness;

  const creative::Id selectedId = request.selection.selectedId;
  const creative::CreativeObject* selected = request.selection.selected;
  const bool hasSelection =
      request.selection.hasSelection && !editor.volume.active;
  const auto objectSelected = [&](creative::CreativeObjectId objectId) {
    return std::find(request.selection.selectedObjectIds.begin(),
                     request.selection.selectedObjectIds.end(),
                     objectId) != request.selection.selectedObjectIds.end();
  };
  const std::array<GizmoAxisShaft, 3>& gizmoShafts =
      request.gizmoFrame.shafts;
  const bool selectedIsPathForHandles =
      request.gizmoFrame.selectedIsPathForHandles;

  resetCreativeEditorOverlayFrame(output);

  // ---- BOUNDS BOX (wireframe) --------------------------------------------
  const creative::CreativeDocumentWireframeSegmentBuildResult segs =
      creative::buildCreativeDocumentWireframeSegments(
          appState.facade.document(), wireProjReq);
  ProductCreativeWireframeDebugLineBuildResult lines =
      buildProductCreativeWireframeDebugLines(segs.segmentList);
  // The renderer turns each line into a world-space tube of `thickness` METRES
  // (creativeDebugLineBox: size = |edge| x thickness x thickness), so keep it
  // thin (a few cm) or a 1 m box fills into a solid blob. Selected edges go
  // bright yellow + a touch fatter for emphasis (the kernel colors by style,
  // not by selection).
  for (ProductCreativeWireframeDebugLine& line : lines.lineList.lines) {
    // Recolor the SELECTED object's edges — matched by id, whatever the kind.
    const bool sel = hasSelection && objectSelected(line.objectId);
    line.thickness = sel ? 0.06F : 0.03F;
    if (sel) {
      line.color = {1.0F, 1.0F, 0.0F, 1.0F};
    }
  }
  CreativeWireframeDebugRenderFrame dbg =
      buildCreativeWireframeDebugRenderFrame(&lines.lineList);

  // ---- GIZMO WIREFRAME ----------------------------------------------------
  // Build ONE combined line vector: the document wireframe lines that draw the
  // yellow selection box (dbg.lines, already converted to render lines) PLUS
  // the 3 axis-aligned gizmo shafts. Point frame.creativeWireframeDebug at THIS
  // vector so the renderer draws both. The vector must outlive submitFrame(),
  // so it lives here in the frame-loop body. When nothing is selected we skip
  // the gizmo and the selection box is empty, so this is just dbg.lines.
  std::vector<RenderCreativeWireframeDebugLine>& combinedWireLines =
      output.combinedWireLines;
  combinedWireLines.reserve(dbg.lines.size() + 48);
  std::size_t& documentWireLineCount = output.documentWireLineCount;
  std::size_t& pointMarkerEdgeCount = output.pointMarkerEdgeCount;
  std::size_t& lineMarkerEdgeCount = output.lineMarkerEdgeCount;
  std::size_t& pathPointHandleEdgeCount = output.pathPointHandleEdgeCount;
  for (const RenderCreativeWireframeDebugLine& line : dbg.lines) {
    const creative::CreativeObject* object =
        line.objectId != creative::kInvalidObjectId
            ? appState.facade.findObject(line.objectId)
            : nullptr;
    if (object != nullptr &&
        creative::describeObject(object->kind).shapeKind ==
            creative::CreativeObjectShapeKind::Line) {
      continue;
    }
    combinedWireLines.push_back(line);
  }
  documentWireLineCount = combinedWireLines.size();
  const creative::CreativeObjectId focusedGroupId =
      activeCreativeEditorGroupFocusId(editor.groupFocus);
  if (focusedGroupId != creative::kInvalidObjectId) {
    bool haveFocusedBounds = false;
    VisualBounds focusedBounds{};
    for (const creative::CreativeObject& object :
         appState.facade.document().objects()) {
      if (!object.visible ||
          object.kind == creative::CreativeObjectKind::Group ||
          !creativeEditorObjectInsideActiveGroup(
              appState.facade.document(), editor.groupFocus, object.id)) {
        continue;
      }
      const VisualBounds objectBounds = visualBoundsForObject(object);
      if (!haveFocusedBounds) {
        focusedBounds = objectBounds;
        haveFocusedBounds = true;
        continue;
      }
      focusedBounds.min.x = std::min(focusedBounds.min.x, objectBounds.min.x);
      focusedBounds.min.y = std::min(focusedBounds.min.y, objectBounds.min.y);
      focusedBounds.min.z = std::min(focusedBounds.min.z, objectBounds.min.z);
      focusedBounds.max.x = std::max(focusedBounds.max.x, objectBounds.max.x);
      focusedBounds.max.y = std::max(focusedBounds.max.y, objectBounds.max.y);
      focusedBounds.max.z = std::max(focusedBounds.max.z, objectBounds.max.z);
    }
    if (haveFocusedBounds) {
      const std::size_t before = combinedWireLines.size();
      appendStandaloneWireframeBoxEdges(
          combinedWireLines, focusedBounds.min, focusedBounds.max,
          RenderLineColor{0.18F, 0.90F, 1.0F, 1.0F}, 0.045F);
      for (std::size_t i = before; i < combinedWireLines.size(); ++i) {
        combinedWireLines[i].objectId = focusedGroupId;
      }
    }
  }
  for (const creative::CreativeObject& obj :
       appState.facade.document().objects()) {
    const creative::CreativeObjectDescriptor& descriptor =
        creative::describeObject(obj.kind);
    if (!obj.visible) {
      continue;
    }
    const bool sel = hasSelection && objectSelected(obj.id);
    if (descriptor.shapeKind != creative::CreativeObjectShapeKind::Point &&
        descriptor.shapeKind != creative::CreativeObjectShapeKind::Line) {
      continue;
    }
    const VisualBounds markerBounds = visualBoundsForObject(obj);
    const std::size_t before = combinedWireLines.size();
    appendStandaloneWireframeBoxEdges(
        combinedWireLines, markerBounds.min, markerBounds.max,
        sel ? RenderLineColor{1.0F, 1.0F, 0.0F, 1.0F}
            : descriptor.shapeKind == creative::CreativeObjectShapeKind::Line
                  ? RenderLineColor{0.86F, 0.68F, 0.28F, 1.0F}
                  : RenderLineColor{0.34F, 0.62F, 0.88F, 1.0F},
        sel ? 0.06F : 0.035F);
    for (std::size_t i = before; i < combinedWireLines.size(); ++i) {
      combinedWireLines[i].objectId = obj.id;
    }
    if (descriptor.shapeKind == creative::CreativeObjectShapeKind::Line) {
      lineMarkerEdgeCount += combinedWireLines.size() - before;
    } else {
      pointMarkerEdgeCount += combinedWireLines.size() - before;
    }
  }
  if (selectedIsPathForHandles) {
    for (const creative::CreativePathPoint& point : selected->pathPoints) {
      const VisualBounds handleBounds = pathPointHandleBounds(point.position);
      const std::size_t before = combinedWireLines.size();
      appendStandaloneWireframeBoxEdges(
          combinedWireLines,
          handleBounds.min,
          handleBounds.max,
          RenderLineColor{0.20F, 0.88F, 1.0F, 1.0F},
          0.035F);
      for (std::size_t i = before; i < combinedWireLines.size(); ++i) {
        combinedWireLines[i].objectId =
            static_cast<creative::CreativeObjectId>(selectedId);
      }
      pathPointHandleEdgeCount += combinedWireLines.size() - before;
    }
  }
  if (hasSelection) {
    for (const GizmoAxisShaft& shaft : gizmoShafts) {
      RenderCreativeWireframeDebugLine gizmoLine;
      gizmoLine.start = request.gizmoFrame.center;
      gizmoLine.end = shaft.tip;  // Axis-aligned: only one component differs.
      gizmoLine.color = shaft.color;
      gizmoLine.objectId =
          static_cast<creative::CreativeObjectId>(selectedId);
      gizmoLine.thickness = gizmoThickness;
      combinedWireLines.push_back(gizmoLine);
    }
  }
  appendCreativeEditorPlacementFeedbackWireframe(request, output);
  // ---- MATERIAL BRUSH PREVIEW --------------------------------------------
  appendCreativeEditorMaterialBrushWireframe(request, output);

  // ---- CONNECTED FILL PREVIEW -------------------------------------------
  appendCreativeEditorConnectedFillWireframe(request, output);

  // ---- SURFACE EXTRUDE PREVIEW ------------------------------------------
  appendCreativeEditorSurfaceExtrudeWireframe(request, output);

  // ---- VOLUME PREVIEW ----------------------------------------------------
  const CreativeEditorVolumePreviewFacts volumeFacts =
      appendCreativeEditorVolumeAndToolWireframes(request, output);
  return {volumeFacts, hasSelection};
}

}  // namespace iggy3d_creative_app
