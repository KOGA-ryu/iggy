#include "EditorPreviewFrame.hpp"
#include "EditorPreviewFrameInternal.hpp"
#include "EditorOverlayWireframesInternal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "EditorFrame.hpp"
#include "EditorAssetReplacement.hpp"
#include "EditorAssetScatter.hpp"
#include "EditorGizmo.hpp"
#include "EditorGroup.hpp"
#include "EditorInteraction.hpp"
#include "EditorLogicLinkOverlay.hpp"
#include "EditorPathEditing.hpp"
#include "EditorPlacementGridOverlay.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"
#include "EditorStructuralPlacement.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/DocumentWireframe.hpp"
#include "app/iggy3d/creative/render/CreativeOverlayFrame.hpp"
#include "app/iggy3d/creative/render/CreativeScreenProjection.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"
#include "app/iggy3d/creative/tools/AttachmentSnap.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace creative = iggy3d::creative;
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
  output.placementGridTargetMarkerCount = 0;
  output.placementGridClipped = false;
  output.documentWireLineCount = 0;
  output.pointMarkerEdgeCount = 0;
  output.lineMarkerEdgeCount = 0;
  output.pathPointHandleEdgeCount = 0;
  output.movingPlatformPathPreviewEdgeCount = 0;
  output.structuralSpanEditEdgeCount = 0;
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
  output.assetScatterEdgeCount = 0;
  output.attachmentSocketMarkerEdgeCount = 0;
  output.placementFeedbackEdgeCount = 0;
  output.logicLinkEdgeCount = 0;
  output.logicLinkShaftCount = 0;
  output.logicLinkArrowEdgeCount = 0;
  output.logicLinkEndpointEdgeCount = 0;
  output.logicLinkLabelGlyphCount = 0;
  output.invalidLogicLinkCount = 0;
}

[[nodiscard]] RenderLineColor logicLinkOverlayColor(
    CreativeLogicLinkOverlayRole role) noexcept {
  const CreativeLogicLinkOverlayColor color =
      creativeLogicLinkOverlayColor(role);
  return {color.r, color.g, color.b, color.a};
}

void appendCreativeEditorLogicLinkLabel(
    const CreativeEditorOverlayFrameRequest& request,
    const CreativeLogicLinkOverlayPlan& plan,
    RenderLineColor color,
    CreativeEditorOverlayFrame& output) {
  constexpr std::size_t kLabelQuadCapacity = 256U;
  const RenderContentViewport content = effectiveContentViewport(request.frame);
  if (content.width == 0U || content.height == 0U) {
    return;
  }
  const cr::CreativeScreenPoint projected =
      cr::projectCreativeWorldPointToScreen(
          request.frame.camera.clipFromWorld, plan.labelWorldPosition,
          content.width, content.height);
  if (!projected.valid || !projected.insideViewport) {
    return;
  }
  const std::int32_t x =
      content.x + static_cast<std::int32_t>(projected.x) + 6;
  const std::int32_t y =
      content.y + static_cast<std::int32_t>(projected.y) - 6;
  std::array<DebugHudGlyphQuad, kLabelQuadCapacity> quads{};
  const DebugHudFixedLayoutResult layout = layoutDebugHudTextAtInto(
      creativeLogicLinkOverlayLabel(plan.role), x, y, request.drawableWidth,
      request.drawableHeight, quads);
  if (layout.capacityExceeded) {
    return;
  }
  for (std::size_t index = 0U; index < layout.quadCount; ++index) {
    DebugHudGlyphQuad& quad = quads[index];
    quad.r = color.r;
    quad.g = color.g;
    quad.b = color.b;
    quad.a = color.a;
  }
  output.logicLinkLabelGlyphCount += layout.glyphCount;
  output.glyphs.insert(output.glyphs.end(), quads.begin(),
                       quads.begin() + layout.quadCount);
}

void appendCreativeEditorLogicLinks(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  const CreativeEditorState& editor = request.editor;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const bool modalOpen = editor.catalog.model.open ||
                         editor.catalog.toolWheel.open ||
                         editor.toolOptions.open ||
                         editor.assetReplacement.active ||
                         editor.transform.active;
  if (request.captureMode || modalOpen) {
    return;
  }
  const cr::CreativeDocument& document = request.appState.facade.document();
  const bool connectToolHeld =
      held.kind == cr::CreativeHeldItemKind::LogicLink;
  cr::CreativeObjectId selectedSourceId = cr::kInvalidObjectId;
  if (request.selection.selected != nullptr &&
      cr::creativeObjectCanSourceLogicLink(
          request.selection.selected->kind)) {
    selectedSourceId = request.selection.selected->id;
  }
  if (selectedSourceId == cr::kInvalidObjectId) {
    selectedSourceId = editor.logicLinks.sourceObjectId;
  }
  if (!connectToolHeld && selectedSourceId == cr::kInvalidObjectId) {
    return;
  }
  const cr::CreativeLogicDiagnosticReport diagnostics =
      cr::buildCreativeLogicDiagnostics(document.logicLinks(),
                                        document.objects());
  std::vector<RenderCreativeWireframeDebugLine>& lines =
      output.combinedWireLines;
  const float thickness = std::max(0.045F, request.gizmoThickness);
  for (const cr::CreativeLogicLink& link : document.logicLinks()) {
    if (!connectToolHeld && link.sourceObjectId != selectedSourceId) {
      continue;
    }
    const cr::CreativeObject* source = document.findObject(link.sourceObjectId);
    const cr::CreativeObject* target = document.findObject(link.targetObjectId);
    if (source == nullptr || !source->visible) {
      continue;
    }
    if (target != nullptr && !target->visible) {
      continue;
    }
    const bool valid =
        cr::creativeObjectCanSourceLogicLink(source->kind) &&
        target != nullptr && target->visible &&
        cr::creativeObjectCanTargetLogicLink(target->kind) &&
        cr::creativeLogicLinkActionSupported(target->kind, link.action);
    const VisualBounds sourceBounds = visualBoundsForObject(*source);
    CreativeLogicLinkOverlayRequest overlayRequest;
    overlayRequest.source = {sourceBounds.min, sourceBounds.max, true};
    overlayRequest.action = link.action;
    overlayRequest.linkValid = valid;
    const bool targetIsHovered =
        connectToolHeld && editor.interaction.target.objectHit &&
        target != nullptr && editor.interaction.target.objectId == target->id;
    if (target != nullptr) {
      const VisualBounds targetBounds = visualBoundsForObject(*target);
      overlayRequest.target = {targetBounds.min, targetBounds.max, true};
    } else {
      overlayRequest.target.available = false;
    }
    const CreativeLogicLinkOverlayPlan plan =
        planCreativeLogicLinkOverlay(overlayRequest);
    if (!plan.accepted) {
      continue;
    }
    const RenderLineColor color = logicLinkOverlayColor(plan.role);
    for (std::size_t segmentIndex = 0U;
         segmentIndex < plan.segmentCount; ++segmentIndex) {
      RenderCreativeWireframeDebugLine line;
      line.start = plan.segments[segmentIndex].start;
      line.end = plan.segments[segmentIndex].end;
      line.color = color;
      line.objectId = source->id;
      line.style = static_cast<std::uint32_t>(plan.role);
      line.segmentKind = segmentIndex == 0U ? 0U : 1U;
      line.thickness = segmentIndex == 0U ? thickness : thickness * 0.82F;
      lines.push_back(line);
    }
    output.logicLinkEdgeCount += plan.segmentCount;
    ++output.logicLinkShaftCount;
    output.logicLinkArrowEdgeCount += plan.segmentCount - 1U;
    if (plan.role == CreativeLogicLinkOverlayRole::Invalid) {
      ++output.invalidLogicLinkCount;
    }

    if (link.sourceObjectId != selectedSourceId) {
      continue;
    }
    appendCreativeEditorLogicLinkLabel(request, plan, color, output);
    if (target != nullptr && !targetIsHovered) {
      const VisualBounds targetBounds = visualBoundsForObject(*target);
      const std::size_t before = lines.size();
      appendStandaloneWireframeBoxEdges(
          lines, targetBounds.min, targetBounds.max, color, thickness * 0.88F);
      for (std::size_t index = before; index < lines.size(); ++index) {
        lines[index].objectId = target->id;
        lines[index].style = static_cast<std::uint32_t>(plan.role);
      }
      const std::size_t added = lines.size() - before;
      output.logicLinkEndpointEdgeCount += added;
      output.logicLinkEdgeCount += added;
    }
  }

  const cr::CreativeObject* selectedSource =
      document.findObject(selectedSourceId);
  if (selectedSource != nullptr && selectedSource->visible) {
    bool sourceInvalid = false;
    for (std::size_t index = 0U; index < diagnostics.issueCount; ++index) {
      sourceInvalid =
          sourceInvalid ||
          diagnostics.issues[index].sourceObjectId == selectedSource->id ||
          diagnostics.issues[index].relatedSourceObjectId ==
              selectedSource->id;
    }
    const VisualBounds bounds = visualBoundsForObject(*selectedSource);
    const std::size_t before = lines.size();
    appendStandaloneWireframeBoxEdges(
        lines, bounds.min, bounds.max,
        sourceInvalid ? RenderLineColor{1.0F, 0.20F, 0.20F, 1.0F}
                      : RenderLineColor{0.20F, 0.82F, 1.0F, 1.0F},
        thickness);
    for (std::size_t index = before; index < lines.size(); ++index) {
      lines[index].objectId = selectedSource->id;
    }
    const std::size_t added = lines.size() - before;
    output.logicLinkEndpointEdgeCount += added;
    output.logicLinkEdgeCount += added;
  }

  if (!connectToolHeld || !editor.interaction.target.objectHit) {
    return;
  }
  const cr::CreativeObject* hovered =
      document.findObject(editor.interaction.target.objectId);
  if (hovered == nullptr || !hovered->visible) {
    return;
  }
  if (hovered->id == editor.logicLinks.sourceObjectId) {
    return;
  }
  const bool valid =
      cr::creativeObjectCanSourceLogicLink(hovered->kind) ||
      (editor.logicLinks.sourceObjectId != cr::kInvalidObjectId &&
       cr::creativeObjectCanTargetLogicLink(hovered->kind) &&
       cr::creativeLogicLinkActionSupported(hovered->kind,
                                            editor.logicLinks.action));
  const VisualBounds hoveredBounds = visualBoundsForObject(*hovered);
  const std::size_t before = lines.size();
  appendStandaloneWireframeBoxEdges(
      lines, hoveredBounds.min, hoveredBounds.max,
      valid ? RenderLineColor{0.20F, 1.0F, 0.35F, 1.0F}
            : RenderLineColor{1.0F, 0.20F, 0.20F, 1.0F},
      thickness);
  for (std::size_t index = before; index < lines.size(); ++index) {
    lines[index].objectId = hovered->id;
  }
  const std::size_t added = lines.size() - before;
  output.logicLinkEndpointEdgeCount += added;
  output.logicLinkEdgeCount += added;
}

[[nodiscard]] RenderLineColor attachmentSocketMarkerColor(
    cr::CreativeAttachmentSocketMarkerState state) noexcept {
  switch (state) {
    case cr::CreativeAttachmentSocketMarkerState::Incompatible:
    case cr::CreativeAttachmentSocketMarkerState::OutOfRange:
      return {0.55F, 0.62F, 0.68F, 1.0F};
    case cr::CreativeAttachmentSocketMarkerState::Available:
      return {0.20F, 1.0F, 0.35F, 1.0F};
    case cr::CreativeAttachmentSocketMarkerState::Occupied:
      return {1.0F, 0.20F, 0.20F, 1.0F};
  }
  return {0.55F, 0.62F, 0.68F, 1.0F};
}

void appendCreativeEditorAttachmentSocketMarkers(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  const CreativeEditorState& editor = request.editor;
  const bool modalOpen = editor.catalog.model.open ||
                         editor.catalog.toolWheel.open ||
                         editor.toolOptions.open ||
                         editor.assetReplacement.active ||
                         editor.transform.active;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const std::string_view sourceAssetId = cr::creativeHotbarAssetId(held);
  if (request.captureMode || modalOpen || request.assetCatalog == nullptr ||
      held.kind != cr::CreativeHeldItemKind::Material ||
      sourceAssetId.empty() ||
      creativeEditorUsesAssetScatter(held, editor.toolSettings) ||
      !editor.interaction.target.objectHit ||
      !editor.interaction.target.grid.valid) {
    return;
  }

  cr::CreativeAttachmentSocketMarkerRequest markerRequest;
  markerRequest.document = &request.appState.facade.document();
  markerRequest.assetCatalog = request.assetCatalog;
  markerRequest.sourceAssetId = sourceAssetId;
  markerRequest.targetObjectId = editor.interaction.target.objectId;
  markerRequest.aimPoint = editor.interaction.target.grid.hitPoint;
  const cr::CreativeAttachmentSocketMarkerFrame markers =
      cr::buildCreativeAttachmentSocketMarkers(markerRequest);
  if (!markers.accepted) {
    return;
  }

  constexpr float kMarkerHalfExtentMeters = 0.11F;
  const float thickness = std::max(0.035F, request.gizmoThickness);
  for (std::size_t index = 0U; index < markers.markerCount; ++index) {
    const cr::CreativeAttachmentSocketMarker& marker = markers.markers[index];
    const cr::CreativeCoreVec3Conversion position =
        cr::creativeVec3ToCoreChecked(marker.worldPosition);
    if (!position.converted) {
      continue;
    }
    const RenderLineColor color = attachmentSocketMarkerColor(marker.state);
    const std::array<Vec3, 3U> axes{
        Vec3{kMarkerHalfExtentMeters, 0.0F, 0.0F},
        Vec3{0.0F, kMarkerHalfExtentMeters, 0.0F},
        Vec3{0.0F, 0.0F, kMarkerHalfExtentMeters}};
    for (const Vec3 axis : axes) {
      RenderCreativeWireframeDebugLine line;
      line.start = position.value - axis;
      line.end = position.value + axis;
      line.color = color;
      line.objectId = marker.targetObjectId;
      line.thickness = thickness;
      output.combinedWireLines.push_back(line);
      ++output.attachmentSocketMarkerEdgeCount;
    }
  }
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

void appendCreativeEditorMovingPlatformPathPreview(
    const CreativeEditorOverlayFrameRequest& request,
    const creative::CreativeObject* selected,
    CreativeEditorOverlayFrame& output) {
  const CreativeEditorState& editor = request.editor;
  const bool modalOpen = editor.catalog.model.open ||
                         editor.catalog.toolWheel.open ||
                         editor.toolOptions.open ||
                         editor.assetReplacement.active ||
                         editor.transform.active;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (request.captureMode || modalOpen ||
      request.inputContext != cr::CreativeInputContext::EditorViewport ||
      held.kind != cr::CreativeHeldItemKind::ObjectMove ||
      !editor.interaction.movingPlatformPathEdit.available ||
      selected == nullptr ||
      selected->id != editor.interaction.movingPlatformPathEdit.objectId) {
    return;
  }

  bool visible = false;
  bool allowed = false;
  bool segmentVisible = false;
  cr::CreativeObjectId objectId = cr::kInvalidObjectId;
  cr::CreativeVec3 fromPoint{};
  cr::CreativeVec3 targetPoint{};
  const CreativeMovingPlatformPathEditState& pathEdit =
      editor.interaction.movingPlatformPathEdit;
  if (pathEdit.pointSelected) {
    const CreativeMovingPlatformPathPointTargetPlan preview =
        planCreativeMovingPlatformPathPointTarget(
            selected, pathEdit.selectedPointIndex,
            editor.interaction.target.grid.valid,
            editor.interaction.target.grid.placementAnchor,
            editor.toolSettings.moveConstraint);
    visible = preview.visible;
    allowed = preview.moveAllowed;
    segmentVisible = preview.segmentVisible;
    objectId = preview.objectId;
    fromPoint = preview.fromPoint;
    targetPoint = preview.targetPoint;
  } else {
    const CreativeMovingPlatformPathTargetPlan preview =
        planCreativeMovingPlatformPathTarget(
            selected, editor.interaction.target.grid.valid,
            editor.interaction.target.grid.placementAnchor);
    visible = preview.visible;
    allowed = preview.appendAllowed;
    segmentVisible = preview.segmentVisible;
    objectId = preview.objectId;
    fromPoint = preview.fromPoint;
    targetPoint = preview.targetPoint;
  }
  if (!visible) {
    return;
  }
  const RenderLineColor color =
      allowed ? RenderLineColor{0.20F, 1.0F, 0.35F, 1.0F}
              : RenderLineColor{1.0F, 0.20F, 0.20F, 1.0F};
  const float thickness = std::max(0.05F, request.gizmoThickness);
  std::vector<RenderCreativeWireframeDebugLine>& lines =
      output.combinedWireLines;
  const std::size_t before = lines.size();
  if (segmentVisible) {
    const cr::CreativeCoreVec3Conversion from =
        cr::creativeVec3ToCoreChecked(fromPoint);
    const cr::CreativeCoreVec3Conversion to =
        cr::creativeVec3ToCoreChecked(targetPoint);
    if (from.converted && to.converted) {
      RenderCreativeWireframeDebugLine segment;
      segment.start = from.value;
      segment.end = to.value;
      segment.color = color;
      segment.objectId = objectId;
      segment.thickness = thickness;
      lines.push_back(segment);
    }
  }
  const VisualBounds marker = pathPointHandleBounds(targetPoint);
  appendStandaloneWireframeBoxEdges(lines, marker.min, marker.max, color,
                                    thickness * 0.8F);
  for (std::size_t index = before; index < lines.size(); ++index) {
    lines[index].objectId = objectId;
  }
  output.movingPlatformPathPreviewEdgeCount = lines.size() - before;
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
  combinedWireLines.reserve(
      dbg.lines.size() +
      appState.facade.document().logicLinks().size() * 15U + 72U +
      13U +
      kMaxStaticMeshAttachmentSocketCount * 3U +
      editor.interaction.assetScatter.preview.candidateCount * 12U +
      creative::kCreativePlacementGridMaximumLineCount +
      creative::kCreativePlacementGridMaximumDotCount +
      creative::kCreativePlacementAnchorCandidateCapacity * 3U + 3U);
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
  appendCreativeEditorPlacementGridOverlay(request, output);
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
    const auto appendSegmentSpeedMarker =
        [&](Vec3 from, Vec3 to, double speedMultiplier) {
          if (std::fabs(speedMultiplier - 1.0) <= 1.0e-9) {
            return;
          }
          RenderCreativeWireframeDebugLine speedMarker;
          speedMarker.start = (from + to) * 0.5F;
          speedMarker.end =
              speedMarker.start +
              Vec3{0.0F,
                   0.20F +
                       0.10F * static_cast<float>(
                                   std::fabs(speedMultiplier - 1.0)),
                   0.0F};
          speedMarker.color = {0.92F, 0.32F, 1.0F, 1.0F};
          speedMarker.objectId =
              static_cast<creative::CreativeObjectId>(selectedId);
          speedMarker.thickness = 0.06F;
          combinedWireLines.push_back(speedMarker);
          ++pathPointHandleEdgeCount;
        };
    for (std::size_t index = 1U; index < selected->pathPoints.size(); ++index) {
      const creative::CreativeCoreVec3Conversion from =
          creative::creativeVec3ToCoreChecked(
              selected->pathPoints[index - 1U].position);
      const creative::CreativeCoreVec3Conversion to =
          creative::creativeVec3ToCoreChecked(
              selected->pathPoints[index].position);
      if (!from.converted || !to.converted) {
        continue;
      }
      RenderCreativeWireframeDebugLine routeLine;
      routeLine.start = from.value;
      routeLine.end = to.value;
      routeLine.color = {0.20F, 0.88F, 1.0F, 1.0F};
      routeLine.objectId =
          static_cast<creative::CreativeObjectId>(selectedId);
      routeLine.thickness = 0.045F;
      combinedWireLines.push_back(routeLine);
      ++pathPointHandleEdgeCount;
      if (selected->kind == creative::CreativeObjectKind::MovingPlatform) {
        appendSegmentSpeedMarker(
            from.value, to.value,
            selected->pathPoints[index - 1U].outgoingSpeedMultiplier);
      }
    }
    if (selected->kind == creative::CreativeObjectKind::MovingPlatform &&
        selected->movingPlatform.traversalMode ==
            creative::CreativeMovingPlatformTraversalMode::Loop &&
        selected->pathPoints.size() > 1U) {
      const creative::CreativeCoreVec3Conversion from =
          creative::creativeVec3ToCoreChecked(
              selected->pathPoints.back().position);
      const creative::CreativeCoreVec3Conversion to =
          creative::creativeVec3ToCoreChecked(
              selected->pathPoints.front().position);
      if (from.converted && to.converted &&
          lengthSquared(to.value - from.value) > 1.0e-10F) {
        RenderCreativeWireframeDebugLine closureLine;
        closureLine.start = from.value;
        closureLine.end = to.value;
        closureLine.color = {0.20F, 0.88F, 1.0F, 1.0F};
        closureLine.objectId =
            static_cast<creative::CreativeObjectId>(selectedId);
        closureLine.thickness = 0.045F;
        combinedWireLines.push_back(closureLine);
        ++pathPointHandleEdgeCount;
        appendSegmentSpeedMarker(
            from.value, to.value,
            selected->pathPoints.back().outgoingSpeedMultiplier);
      }
    }
    for (std::size_t index = 0U; index < selected->pathPoints.size(); ++index) {
      const creative::CreativePathPoint& point = selected->pathPoints[index];
      const bool pointSelected =
          editor.interaction.movingPlatformPathEdit.pointSelected &&
          editor.interaction.movingPlatformPathEdit.objectId == selected->id &&
          editor.interaction.movingPlatformPathEdit.selectedPointIndex == index;
      const VisualBounds handleBounds = pathPointHandleBounds(point.position);
      const std::size_t before = combinedWireLines.size();
      appendStandaloneWireframeBoxEdges(
          combinedWireLines,
          handleBounds.min,
          handleBounds.max,
          pointSelected ? RenderLineColor{1.0F, 0.92F, 0.20F, 1.0F}
                        : RenderLineColor{0.20F, 0.88F, 1.0F, 1.0F},
          pointSelected ? 0.06F : 0.035F);
      for (std::size_t i = before; i < combinedWireLines.size(); ++i) {
        combinedWireLines[i].objectId =
            static_cast<creative::CreativeObjectId>(selectedId);
      }
      pathPointHandleEdgeCount += combinedWireLines.size() - before;
      if (point.dwellSeconds > 0.0) {
        const creative::CreativeCoreVec3Conversion markerBase =
            creative::creativeVec3ToCoreChecked(point.position);
        if (markerBase.converted) {
          RenderCreativeWireframeDebugLine dwellMarker;
          dwellMarker.start = markerBase.value;
          dwellMarker.end = markerBase.value + Vec3{0.0F, 0.4F, 0.0F};
          dwellMarker.color = {1.0F, 0.62F, 0.12F, 1.0F};
          dwellMarker.objectId =
              static_cast<creative::CreativeObjectId>(selectedId);
          dwellMarker.thickness = 0.06F;
          combinedWireLines.push_back(dwellMarker);
          ++pathPointHandleEdgeCount;
        }
      }
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
  appendCreativeEditorMovingPlatformPathPreview(request, selected, output);
  const bool structuralEditVisible =
      !request.captureMode &&
      request.inputContext == creative::CreativeInputContext::EditorViewport &&
      !editor.catalog.model.open && !editor.catalog.toolWheel.open &&
      !editor.toolOptions.open && !editor.assetReplacement.active &&
      !editor.transform.active;
  if (structuralEditVisible) {
    output.structuralSpanEditEdgeCount =
        appendCreativeEditorStructuralSpanEditWireframe(
            appState, editor.interaction.structuralSpanEdit,
            std::max(0.035F, gizmoThickness), combinedWireLines);
  }
  appendCreativeEditorAttachmentSocketMarkers(request, output);
  appendCreativeEditorPlacementFeedbackWireframe(request, output);
  appendCreativeEditorLogicLinks(request, output);
  output.assetReplacementEdgeCount =
      appendCreativeEditorAssetReplacementWireframes(
          editor.assetReplacement, std::max(0.06F, gizmoThickness * 1.2F),
          combinedWireLines);
  output.assetScatterEdgeCount =
      appendCreativeEditorAssetScatterWireframes(
          editor, std::max(0.05F, gizmoThickness), combinedWireLines);
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
