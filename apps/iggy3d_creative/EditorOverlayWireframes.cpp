#include "EditorPreviewFrame.hpp"
#include "EditorPreviewFrameInternal.hpp"
#include "EditorOverlayWireframesInternal.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include "EditorFrame.hpp"
#include "EditorAssetReplacement.hpp"
#include "EditorGizmo.hpp"
#include "EditorGroup.hpp"
#include "EditorInteraction.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"
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
  output.assetReplacementEdgeCount = 0;
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
  output.assetReplacementEdgeCount =
      appendCreativeEditorAssetReplacementWireframes(
          editor.assetReplacement, std::max(0.06F, gizmoThickness * 1.2F),
          combinedWireLines);
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
