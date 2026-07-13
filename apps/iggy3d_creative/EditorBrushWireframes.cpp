#include "EditorOverlayWireframesInternal.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include "EditorConnectedFill.hpp"
#include "EditorInteraction.hpp"
#include "EditorShapePreview.hpp"
#include "EditorState.hpp"
#include "EditorSurfaceExtrude.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace creative = iggy3d::creative;
using namespace iggy3d;
namespace {

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

}  // namespace

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

namespace {

[[nodiscard]] bool creativeEditorWorldPreviewBlocked(
    const CreativeEditorOverlayFrameRequest& request) noexcept {
  const CreativeEditorState& editor = request.editor;
  return request.captureMode || editor.catalog.model.open ||
         editor.catalog.toolWheel.open || editor.toolOptions.open ||
         editor.controls.open || editor.transform.active ||
         editor.transform.controlsOpen;
}

}  // namespace

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

}  // namespace iggy3d_creative_app
