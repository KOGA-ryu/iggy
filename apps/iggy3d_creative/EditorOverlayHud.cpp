#include "EditorPreviewFrame.hpp"
#include "EditorPreviewFrameInternal.hpp"

#include <cstdint>
#include <cstdio>
#include <string>

#include "EditorActionHints.hpp"
#include "EditorAssetReplacement.hpp"
#include "EditorFrame.hpp"
#include "EditorCatalog.hpp"
#include "EditorControls.hpp"
#include "EditorToolOptions.hpp"
#include "EditorTransform.hpp"
#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/render/CreativeScreenProjection.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace creative = iggy3d::creative;
using namespace iggy3d;
namespace {

void appendCreativeEditorVolumePreviewLabel(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output,
    const CreativeEditorVolumePreviewFacts& facts) {
  if (output.volumeEdgeCount == 0U || !facts.selectionVisible) {
    return;
  }
  CreativeEditorState& editor = request.editor;
  const creative::CreativeGridBounds3 gridBounds =
      creative::creativeVolumeGridBounds(facts.selection);
  const creative::CreativeBounds bounds =
      creative::creativeVolumeWorldBounds(facts.selection);
  const creative::CreativeBoundsMetrics metrics =
      creative::measureCreativeBounds(bounds);
  const Vec3 labelPosition = creative::creativeVec3ToCoreChecked(
                                 {metrics.center.x, bounds.max.y,
                                  metrics.center.z})
                                 .value;
  const creative::CreativeScreenPoint screenPoint =
      creative::projectCreativeWorldPointToScreen(
          request.frame.camera.clipFromWorld, labelPosition,
          request.drawableWidth, request.drawableHeight);
  if (!screenPoint.valid) {
    return;
  }
  char label[128];
  if (facts.terrainStamp) {
    const CreativeTerrainStampPreviewCache& preview =
        editor.terrain.region.stamp.preview;
    std::snprintf(
        label, sizeof(label), "STAMP %s %s Y%+d | %d x %d | %u rods | %s",
        std::string(creative::toString(preview.mode)).c_str(),
        std::string(creative::toString(preview.elevationMode)).c_str(),
        preview.plan.appliedHeightOffsetCells,
        gridBounds.max.x - gridBounds.min.x,
        gridBounds.max.z - gridBounds.min.z,
        static_cast<unsigned>(preview.plan.finalControlCount),
        std::string(creative::toString(preview.plan.status)).c_str());
  } else if (facts.terrainRegion) {
    const CreativeTerrainRegionPreviewCache& preview =
        editor.terrain.region.preview;
    std::snprintf(
        label, sizeof(label), "TERRAIN %s %d x %d | %u rods | %s",
        std::string(creative::toString(
                        editor.toolSettings.terrainRegionOperation))
            .c_str(),
        gridBounds.max.x - gridBounds.min.x,
        gridBounds.max.z - gridBounds.min.z,
        static_cast<unsigned>(preview.plan.affectedControlCount),
        std::string(creative::toString(preview.plan.status)).c_str());
  } else {
    const std::uint64_t plannedCellCount =
      facts.usesShapePlan && facts.shapePlan.accepted
          ? facts.shapePlan.generatedCellCount
          : facts.usesShapePlan
                ? 0U
                : creative::creativeVolumeCellCount(facts.selection);
    const std::string shapeLabel =
      facts.usesShapePlan
          ? std::string(creative::toString(editor.toolSettings.shapeBrushKind))
          : std::string{"BOX"};
    const std::string axisLabel =
      facts.usesShapePlan &&
              editor.toolSettings.shapeBrushKind ==
                  creative::CreativeShapeBrushKind::Cylinder
          ? " " + std::string(
                      creative::toString(editor.toolSettings.shapeBrushAxis))
          : std::string{};
    if (editor.volume.lastReceipt.requested) {
      std::snprintf(
          label, sizeof(label), "%s %s%s %d x %d x %d | %llu cells | %s",
          std::string(creative::toString(editor.volume.operation)).c_str(),
          shapeLabel.c_str(), axisLabel.c_str(),
          gridBounds.max.x - gridBounds.min.x,
          gridBounds.max.y - gridBounds.min.y,
          gridBounds.max.z - gridBounds.min.z,
          static_cast<unsigned long long>(plannedCellCount),
          std::string(creative::toString(editor.volume.lastReceipt.status))
              .c_str());
    } else {
      std::snprintf(
          label, sizeof(label), "%s %s%s %d x %d x %d | %llu cells",
          std::string(creative::toString(editor.volume.operation)).c_str(),
          shapeLabel.c_str(), axisLabel.c_str(),
          gridBounds.max.x - gridBounds.min.x,
          gridBounds.max.y - gridBounds.min.y,
          gridBounds.max.z - gridBounds.min.z,
          static_cast<unsigned long long>(plannedCellCount));
    }
  }
  const DebugHudLayoutResult layout = layoutDebugHudTextAt(
      label, static_cast<std::int32_t>(screenPoint.x),
      static_cast<std::int32_t>(screenPoint.y), request.drawableWidth,
      request.drawableHeight);
  output.glyphs.insert(output.glyphs.end(), layout.quads.begin(),
                       layout.quads.end());
}

void appendCreativeEditorSelectionDimensionLabel(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output,
    bool hasSelection) {
  if (!hasSelection) {
    return;
  }
  const Vec3 center{
      (request.selection.boxMin.x + request.selection.boxMax.x) * 0.5F,
      (request.selection.boxMin.y + request.selection.boxMax.y) * 0.5F,
      (request.selection.boxMin.z + request.selection.boxMax.z) * 0.5F};
  const creative::CreativeScreenPoint screenPoint =
      creative::projectCreativeWorldPointToScreen(
          request.frame.camera.clipFromWorld, center, request.drawableWidth,
          request.drawableHeight);
  if (!screenPoint.valid) {
    return;
  }
  const float width = request.selection.boxMax.x - request.selection.boxMin.x;
  const float height = request.selection.boxMax.y - request.selection.boxMin.y;
  const float depth = request.selection.boxMax.z - request.selection.boxMin.z;
  char label[64];
  std::snprintf(label, sizeof(label), "%.1f x %.1f x %.1f m",
                static_cast<double>(width), static_cast<double>(height),
                static_cast<double>(depth));
  const DebugHudLayoutResult layout = layoutDebugHudTextAt(
      label, static_cast<std::int32_t>(screenPoint.x),
      static_cast<std::int32_t>(screenPoint.y), request.drawableWidth,
      request.drawableHeight);
  output.glyphs.insert(output.glyphs.end(), layout.quads.begin(),
                       layout.quads.end());
}

}  // namespace

void appendCreativeEditorHudOverlays(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output,
    const CreativeEditorVolumePreviewFacts& volumeFacts,
    bool hasSelection) {
  CreativeEditorState& editor = request.editor;
  appendCreativeEditorInteractionOverlay(
      editor, request.drawableWidth, request.drawableHeight,
      request.gizmoThickness, output.uiRects, output.glyphs,
      output.combinedWireLines);
  appendCreativeEditorTransformOverlay(
      editor.transform, request.drawableWidth, request.drawableHeight,
      output.uiRects, output.glyphs);
  appendCreativeEditorAssetReplacementOverlay(
      editor.assetReplacement, request.drawableWidth, request.drawableHeight,
      output.uiRects, output.glyphs);
  appendCreativeEditorCatalogOverlay(
      request.appState, editor, request.drawableWidth, request.drawableHeight,
      output.uiRects, output.glyphs);
  appendCreativeEditorToolOptionsOverlay(
      editor, request.drawableWidth, request.drawableHeight, output.uiRects,
      output.glyphs);
  appendCreativeEditorControlsOverlay(
      editor, request.drawableWidth, request.drawableHeight, output.uiRects,
      output.glyphs);
  appendCreativeEditorVolumePreviewLabel(request, output, volumeFacts);
  appendCreativeEditorSelectionDimensionLabel(request, output, hasSelection);
  appendCreativeEditorActionHintsOverlay(
      editor, request.inputContext, request.activeControlDevice,
      request.captureMode, request.drawableWidth, request.drawableHeight,
      output.uiRects, output.glyphs);
}

}  // namespace iggy3d_creative_app
