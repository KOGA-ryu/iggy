#include "EditorOverlayWireframesInternal.hpp"

#include <array>
#include <algorithm>
#include <cstdint>
#include <span>
#include <vector>

#include "EditorPattern.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorShapePreview.hpp"
#include "EditorState.hpp"
#include "EditorTerrain.hpp"
#include "EditorTransformOverlay.hpp"
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
    case creative::CreativeTerrainStampPlanStatus::OutputRejected:
      return false;
  }
  return false;
}

[[nodiscard]] RenderLineColor volumeAxisColor(
    cr::CreativeAxis3 axis) noexcept {
  switch (axis) {
    case cr::CreativeAxis3::X: return {0.95F, 0.24F, 0.18F, 1.0F};
    case cr::CreativeAxis3::Y: return {0.20F, 0.92F, 0.34F, 1.0F};
    case cr::CreativeAxis3::Z: return {0.22F, 0.48F, 1.0F, 1.0F};
    case cr::CreativeAxis3::Count: break;
  }
  return {1.0F, 1.0F, 1.0F, 1.0F};
}

void volumeFaceTangents(cr::CreativeAxis3 axis,
                        Vec3& first,
                        Vec3& second) noexcept {
  switch (axis) {
    case cr::CreativeAxis3::X:
      first = {0.0F, 1.0F, 0.0F};
      second = {0.0F, 0.0F, 1.0F};
      return;
    case cr::CreativeAxis3::Y:
      first = {1.0F, 0.0F, 0.0F};
      second = {0.0F, 0.0F, 1.0F};
      return;
    case cr::CreativeAxis3::Z:
      first = {1.0F, 0.0F, 0.0F};
      second = {0.0F, 1.0F, 0.0F};
      return;
    case cr::CreativeAxis3::Count: break;
  }
  first = {};
  second = {};
}

void appendCreativeEditorVolumeHandles(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  CreativeEditorState& editor = request.editor;
  const bool hidden = request.captureMode ||
                      request.inputContext !=
                          cr::CreativeInputContext::EditorViewport ||
                      editor.catalog.model.open || editor.catalog.toolWheel.open ||
                      editor.toolOptions.open || editor.controls.open ||
                      editor.transform.controlsOpen;
  if (hidden) {
    return;
  }
  const CreativeEditorVolumeHandleFrame frame =
      buildCreativeEditorVolumeHandleFrame(
          editor.volume, request.frame.camera,
          effectiveContentViewport(request.frame));
  if (!frame.valid) {
    return;
  }

  const float thickness = std::max(0.035F, request.gizmoThickness);
  const float markerRadius = static_cast<float>(std::clamp(
      editor.volume.selection.cellSize * 0.18, 0.08, 0.24));
  const auto appendLine = [&](Vec3 start, Vec3 end, RenderLineColor color) {
    RenderCreativeWireframeDebugLine line;
    line.start = start;
    line.end = end;
    line.color = color;
    line.thickness = thickness;
    output.combinedWireLines.push_back(line);
    ++output.volumeHandleEdgeCount;
  };
  for (const CreativeEditorVolumeHandle& handle : frame.handles) {
    if (!handle.valid) {
      continue;
    }
    const bool active = editor.volume.handleGesture.active &&
                        editor.volume.handleGesture.handle.kind == handle.kind &&
                        editor.volume.handleGesture.handle.axis == handle.axis &&
                        editor.volume.handleGesture.handle.face == handle.face;
    const RenderLineColor color =
        active ? RenderLineColor{1.0F, 1.0F, 1.0F, 1.0F}
               : handle.kind == CreativeEditorVolumeHandleKind::MoveAxis
                     ? volumeAxisColor(handle.axis)
                     : RenderLineColor{1.0F, 0.82F, 0.12F, 1.0F};
    if (handle.kind == CreativeEditorVolumeHandleKind::MoveAxis) {
      appendLine(frame.center, handle.worldPosition, color);
      continue;
    }
    Vec3 first;
    Vec3 second;
    volumeFaceTangents(handle.axis, first, second);
    const Vec3 a = handle.worldPosition + first * markerRadius;
    const Vec3 b = handle.worldPosition + second * markerRadius;
    const Vec3 c = handle.worldPosition - first * markerRadius;
    const Vec3 d = handle.worldPosition - second * markerRadius;
    appendLine(a, b, color);
    appendLine(b, c, color);
    appendLine(c, d, color);
    appendLine(d, a, color);
  }
}

[[nodiscard]] bool volumeSelectionForBounds(
    const cr::CreativeVolumeSelection& source,
    cr::CreativeGridBounds3 bounds,
    cr::CreativeVolumeSelection& output) noexcept {
  output = source;
  if (cr::setCreativeVolumeSelectionGridBounds(output, bounds)) {
    return true;
  }
  const cr::CreativeGridBounds3 current =
      cr::creativeVolumeGridBounds(output);
  return cr::creativeVolumeSelectionValid(output) &&
         current.min.x == bounds.min.x && current.min.y == bounds.min.y &&
         current.min.z == bounds.min.z && current.max.x == bounds.max.x &&
         current.max.y == bounds.max.y && current.max.z == bounds.max.z;
}

void appendCreativeVolumeObjectOutlines(
    std::vector<RenderCreativeWireframeDebugLine>& lines,
    const cr::CreativeDocument& document,
    std::span<const cr::CreativeObjectId> objectIds,
    RenderLineColor color,
    float thickness) {
  constexpr std::size_t kBoxEdgeCount = 12U;
  lines.reserve(lines.size() + objectIds.size() * kBoxEdgeCount);
  for (const cr::CreativeObjectId objectId : objectIds) {
    const cr::CreativeObject* object = document.findObject(objectId);
    if (object == nullptr) {
      continue;
    }
    const cr::CreativeTransformedBounds bounds =
        cr::resolveCreativeObjectBounds(*object);
    const cr::CreativeCoreVec3Conversion minimum =
        cr::creativeVec3ToCoreChecked(bounds.worldBounds.min);
    const cr::CreativeCoreVec3Conversion maximum =
        cr::creativeVec3ToCoreChecked(bounds.worldBounds.max);
    if (!bounds.valid || !minimum.converted || !maximum.converted) {
      continue;
    }
    appendStandaloneWireframeBoxEdges(lines, minimum.value, maximum.value,
                                      color, thickness);
  }
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
      if (!facts.terrainRegion) {
        facts.operationPreview = refreshCreativeEditorVolumeOperationPreview(
            editor.volume, request.appState.facade.document(), facts.selection,
            editor.placeBrush, editor.toolSettings);
        facts.hasOperationPreview = true;
        appendCreativeEditorVolumeHandles(request, output);
      }
      creative::CreativeVolumeSelection exteriorSelection = facts.selection;
      const bool hollowBoundsVisible =
          editor.volume.operation ==
              creative::CreativeVolumeOperationKind::Hollow &&
          facts.hasOperationPreview &&
          facts.operationPreview.hollowBoundsValid &&
          volumeSelectionForBounds(
              facts.selection, facts.operationPreview.hollowExteriorBounds,
              exteriorSelection);
      if (facts.usesShapePlan) {
        creative::CreativeShapeBrushPlanRequest planRequest;
        planRequest.kind = editor.toolSettings.shapeBrushKind;
        planRequest.axis = editor.toolSettings.shapeBrushAxis;
        planRequest.firstCell = exteriorSelection.firstCell;
        planRequest.secondCell = exteriorSelection.secondCell;
        planRequest.hollow =
            editor.volume.operation ==
            creative::CreativeVolumeOperationKind::Hollow;
        planRequest.shellThicknessCells =
            creative::creativeVolumeHollowThicknessCells(
                editor.toolSettings.volumeHollowThickness);
        planRequest.shellOpening = editor.toolSettings.volumeHollowOpening;
        planRequest.shellCornerRule =
            editor.toolSettings.volumeHollowCornerRule;
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
                      editor.terrain.region.stamp.preview.operationPreview.receipt
                          .accepted &&
                      editor.terrain.region.stamp.preview.renderAccepted
                : editor.terrain.region.preview.valid &&
                      editor.terrain.region.preview.operationPreview.receipt
                          .accepted &&
                      editor.terrain.region.preview.renderAccepted;
        const bool replacing =
            facts.terrainStamp
                ? editor.terrain.region.stamp.preview.recipe.mode ==
                      creative::CreativeTerrainStampMode::Replace
                : editor.terrain.region.preview.recipe.mode ==
                      creative::CreativeTerrainRegionMode::Erase;
        color = !accepted
                    ? RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F}
                    : replacing
                          ? RenderLineColor{1.0F, 0.46F, 0.12F, 1.0F}
                          : RenderLineColor{0.20F, 1.0F, 0.35F, 1.0F};
      } else if ((facts.usesShapePlan && !facts.shapePlan.accepted) ||
                 (facts.hasOperationPreview &&
                  !facts.operationPreview.accepted)) {
        color = RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F};
      }
      const std::size_t exteriorBefore = lines.size();
      appendCreativeShapeBrushOutline(
          lines, exteriorSelection,
          facts.usesShapePlan ? editor.toolSettings.shapeBrushKind
                              : creative::CreativeShapeBrushKind::Box,
          editor.toolSettings.shapeBrushAxis, color, request.gizmoThickness);
      output.volumeExteriorEdgeCount = lines.size() - exteriorBefore;
      if (hollowBoundsVisible) {
        creative::CreativeVolumeSelection interiorSelection;
        if (volumeSelectionForBounds(
                facts.selection, facts.operationPreview.hollowInteriorBounds,
                interiorSelection)) {
          const std::size_t interiorBefore = lines.size();
          appendCreativeShapeBrushOutline(
              lines, interiorSelection, editor.toolSettings.shapeBrushKind,
              editor.toolSettings.shapeBrushAxis,
              RenderLineColor{0.72F, 0.92F, 1.0F, 0.82F},
              std::max(0.02F, request.gizmoThickness * 0.72F));
          output.volumeInteriorEdgeCount = lines.size() - interiorBefore;
        }
      }
      if (editor.volume.operation ==
              creative::CreativeVolumeOperationKind::Replace &&
          facts.hasOperationPreview && facts.operationPreview.accepted) {
        creative::CreativeGridSettings grid;
        grid.origin = facts.selection.origin;
        grid.cellSizeMeters = facts.selection.cellSize;
        const std::size_t changedBefore = lines.size();
        appendCreativeMaterialBrushCellOutlines(
            lines, facts.operationPreview.changedVoxelCells, grid,
            RenderLineColor{0.20F, 1.0F, 0.35F, 1.0F},
            request.gizmoThickness);
        appendCreativeVolumeObjectOutlines(
            lines, request.appState.facade.document(),
            facts.operationPreview.changedObjectIds,
            RenderLineColor{0.20F, 1.0F, 0.35F, 1.0F},
            request.gizmoThickness);
        output.volumeChangedMemberEdgeCount = lines.size() - changedBefore;

        const std::size_t unchangedBefore = lines.size();
        appendCreativeMaterialBrushCellOutlines(
            lines, facts.operationPreview.unchangedVoxelCells, grid,
            RenderLineColor{0.56F, 0.62F, 0.70F, 0.72F},
            std::max(0.02F, request.gizmoThickness * 0.72F));
        appendCreativeVolumeObjectOutlines(
            lines, request.appState.facade.document(),
            facts.operationPreview.unchangedObjectIds,
            RenderLineColor{0.56F, 0.62F, 0.70F, 0.72F},
            std::max(0.02F, request.gizmoThickness * 0.72F));
        output.volumeUnchangedMemberEdgeCount =
            lines.size() - unchangedBefore;
      }
      if (editor.volume.operation ==
              creative::CreativeVolumeOperationKind::Erase &&
          facts.hasOperationPreview) {
        creative::CreativeGridSettings grid;
        grid.origin = facts.selection.origin;
        grid.cellSizeMeters = facts.selection.cellSize;
        const std::size_t removedBefore = lines.size();
        appendCreativeMaterialBrushCellOutlines(
            lines, facts.operationPreview.removedVoxelCells, grid,
            RenderLineColor{1.0F, 0.25F, 0.16F, 1.0F},
            request.gizmoThickness);
        appendCreativeVolumeObjectOutlines(
            lines, request.appState.facade.document(),
            facts.operationPreview.removedObjectIds,
            RenderLineColor{1.0F, 0.25F, 0.16F, 1.0F},
            request.gizmoThickness);
        output.volumeChangedMemberEdgeCount = lines.size() - removedBefore;

        const std::size_t protectedBefore = lines.size();
        appendCreativeVolumeObjectOutlines(
            lines, request.appState.facade.document(),
            facts.operationPreview.protectedObjectIds,
            RenderLineColor{1.0F, 0.68F, 0.12F, 0.90F},
            std::max(0.02F, request.gizmoThickness * 0.82F));
        output.volumeProtectedMemberEdgeCount =
            lines.size() - protectedBefore;

        const std::size_t dependentBefore = lines.size();
        appendCreativeVolumeObjectOutlines(
            lines, request.appState.facade.document(),
            facts.operationPreview.dependentSourceObjectIds,
            RenderLineColor{1.0F, 0.92F, 0.34F, 0.82F},
            std::max(0.02F, request.gizmoThickness * 0.72F));
        output.volumeDependentSourceEdgeCount =
            lines.size() - dependentBefore;

        const std::size_t blockedBefore = lines.size();
        appendCreativeVolumeObjectOutlines(
            lines, request.appState.facade.document(),
            facts.operationPreview.blockedObjectIds,
            RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F},
            request.gizmoThickness);
        output.volumeBlockedMemberEdgeCount = lines.size() - blockedBefore;
      }
      if (editor.volume.operation ==
              creative::CreativeVolumeOperationKind::Clone &&
          facts.hasOperationPreview) {
        creative::CreativeGridSettings grid;
        grid.origin = facts.selection.origin;
        grid.cellSizeMeters = facts.selection.cellSize;

        const std::size_t changedBefore = lines.size();
        appendCreativeMaterialBrushCellOutlines(
            lines, facts.operationPreview.changedVoxelCells, grid,
            RenderLineColor{0.20F, 1.0F, 0.35F, 1.0F},
            request.gizmoThickness);
        if (editor.volume.preview.stagedDocumentValid) {
          appendCreativeVolumeObjectOutlines(
              lines, editor.volume.preview.stagedDocument,
              facts.operationPreview.createdObjectIds,
              RenderLineColor{0.20F, 1.0F, 0.35F, 1.0F},
              request.gizmoThickness);
        }
        output.volumeChangedMemberEdgeCount = lines.size() - changedBefore;

        const std::size_t unchangedBefore = lines.size();
        appendCreativeMaterialBrushCellOutlines(
            lines, facts.operationPreview.unchangedVoxelCells, grid,
            RenderLineColor{0.56F, 0.62F, 0.70F, 0.72F},
            std::max(0.02F, request.gizmoThickness * 0.72F));
        output.volumeUnchangedMemberEdgeCount =
            lines.size() - unchangedBefore;

        const std::size_t protectedBefore = lines.size();
        appendCreativeVolumeObjectOutlines(
            lines, request.appState.facade.document(),
            facts.operationPreview.protectedObjectIds,
            RenderLineColor{1.0F, 0.68F, 0.12F, 0.90F},
            std::max(0.02F, request.gizmoThickness * 0.82F));
        output.volumeProtectedMemberEdgeCount =
            lines.size() - protectedBefore;

        const std::size_t dependentBefore = lines.size();
        appendCreativeVolumeObjectOutlines(
            lines, request.appState.facade.document(),
            facts.operationPreview.dependentSourceObjectIds,
            RenderLineColor{1.0F, 0.92F, 0.34F, 0.82F},
            std::max(0.02F, request.gizmoThickness * 0.72F));
        output.volumeDependentSourceEdgeCount =
            lines.size() - dependentBefore;

        const std::size_t blockedBefore = lines.size();
        appendCreativeMaterialBrushCellOutlines(
            lines, facts.operationPreview.blockedVoxelCells, grid,
            RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F},
            request.gizmoThickness);
        appendCreativeVolumeObjectOutlines(
            lines, request.appState.facade.document(),
            facts.operationPreview.blockedObjectIds,
            RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F},
            request.gizmoThickness);
        output.volumeBlockedMemberEdgeCount = lines.size() - blockedBefore;
      }
      output.volumeEdgeCount = lines.size() - before;
    }
  }
  if (!editor.transform.active) {
    output.patternEdgeCount = appendCreativeEditorArrayPreview(
        request.appState, editor, request.gizmoThickness, lines,
        request.placementClearanceCache);
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
