#include "EditorOverlayAssemblersInternal.hpp"

#include <algorithm>
#include <array>
#include <vector>

#include "EditorAssetScatter.hpp"
#include "EditorPreviewProxies.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "app/iggy3d/creative/input/HeldItemRegistry.hpp"
#include "app/iggy3d/creative/overlay/LogicLinkOverlay.hpp"
#include "app/iggy3d/creative/render/CreativeScreenProjection.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"
#include "app/iggy3d/creative/tools/AttachmentSnap.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
using namespace iggy3d;
namespace {

[[nodiscard]] RenderLineColor logicLinkOverlayColor(
    CreativeLogicLinkOverlayRole role) noexcept {
  const CreativeLogicLinkOverlayColor color =
      creativeLogicLinkOverlayColor(role);
  return {color.r, color.g, color.b, color.a};
}

void appendLogicLinkLabel(
    const CreativeEditorOverlayRelationshipsAssemblyRequest& request,
    const CreativeLogicLinkOverlayPlan& plan,
    RenderLineColor color,
    CreativeEditorOverlayFrame& output) {
  constexpr std::size_t kLabelQuadCapacity = 256U;
  const RenderContentViewport content =
      effectiveContentViewport(request.frame);
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
      creativeLogicLinkOverlayLabel(plan.role), x, y,
      request.drawableWidth, request.drawableHeight, quads);
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
  output.glyphs.insert(
      output.glyphs.end(), quads.begin(),
      quads.begin() + layout.quadCount);
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

}  // namespace

void appendCreativeEditorOverlayLogicLinks(
    const CreativeEditorOverlayRelationshipsAssemblyRequest& request,
    CreativeEditorOverlayFrame& output) {
  if (request.captureMode || request.modalOpen) {
    return;
  }
  const bool connectToolHeld =
      cr::describeCreativeHeldItem(request.held.kind).interactionMode ==
      cr::CreativeHeldItemInteractionMode::LogicLink;
  cr::CreativeObjectId selectedSourceId = cr::kInvalidObjectId;
  if (request.selection.selected != nullptr &&
      cr::creativeObjectCanSourceLogicLink(
          request.selection.selected->kind)) {
    selectedSourceId = request.selection.selected->id;
  }
  if (selectedSourceId == cr::kInvalidObjectId) {
    selectedSourceId = request.logicLinks.sourceObjectId;
  }
  if (!connectToolHeld && selectedSourceId == cr::kInvalidObjectId) {
    return;
  }
  const cr::CreativeLogicDiagnosticReport diagnostics =
      cr::buildCreativeLogicDiagnostics(
          request.document.logicLinks(), request.document.objects());
  std::vector<RenderCreativeWireframeDebugLine>& lines =
      output.combinedWireLines;
  const float thickness = std::max(0.045F, request.gizmoThickness);
  for (const cr::CreativeLogicLink& link :
       request.document.logicLinks()) {
    if (!connectToolHeld && link.sourceObjectId != selectedSourceId) {
      continue;
    }
    const cr::CreativeObject* source =
        request.document.findObject(link.sourceObjectId);
    const cr::CreativeObject* target =
        request.document.findObject(link.targetObjectId);
    const bool sourceVisible =
        source != nullptr &&
        cr::creativeObjectEffectivelyVisible(
            request.document, source->id);
    const bool targetVisible =
        target != nullptr &&
        cr::creativeObjectEffectivelyVisible(
            request.document, target->id);
    if (!sourceVisible) {
      continue;
    }
    if (target != nullptr && !targetVisible) {
      continue;
    }
    const bool valid =
        cr::creativeObjectCanSourceLogicLink(source->kind) &&
        targetVisible &&
        cr::creativeObjectCanTargetLogicLink(target->kind) &&
        cr::creativeLogicLinkActionSupported(target->kind, link.action);
    const VisualBounds sourceBounds = visualBoundsForObject(*source);
    CreativeLogicLinkOverlayRequest overlayRequest;
    overlayRequest.source = {
        sourceBounds.min, sourceBounds.max, true};
    overlayRequest.action = link.action;
    overlayRequest.linkValid = valid;
    const bool targetIsHovered =
        connectToolHeld && request.target.objectHit &&
        target != nullptr && request.target.objectId == target->id;
    if (target != nullptr) {
      const VisualBounds targetBounds = visualBoundsForObject(*target);
      overlayRequest.target = {
          targetBounds.min, targetBounds.max, true};
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
      line.thickness =
          segmentIndex == 0U ? thickness : thickness * 0.82F;
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
    appendLogicLinkLabel(request, plan, color, output);
    if (target != nullptr && !targetIsHovered) {
      const VisualBounds targetBounds = visualBoundsForObject(*target);
      const std::size_t before = lines.size();
      appendStandaloneWireframeBoxEdges(
          lines, targetBounds.min, targetBounds.max, color,
          thickness * 0.88F);
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
      request.document.findObject(selectedSourceId);
  if (selectedSource != nullptr &&
      cr::creativeObjectEffectivelyVisible(
          request.document, selectedSource->id)) {
    bool sourceInvalid = false;
    for (std::size_t index = 0U; index < diagnostics.issueCount; ++index) {
      sourceInvalid =
          sourceInvalid ||
          diagnostics.issues[index].sourceObjectId ==
              selectedSource->id ||
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

  if (!connectToolHeld || !request.target.objectHit) {
    return;
  }
  const cr::CreativeObject* hovered =
      request.document.findObject(request.target.objectId);
  if (hovered == nullptr ||
      !cr::creativeObjectEffectivelyVisible(
          request.document, hovered->id)) {
    return;
  }
  if (hovered->id == request.logicLinks.sourceObjectId) {
    return;
  }
  const bool valid =
      cr::creativeObjectCanSourceLogicLink(hovered->kind) ||
      (request.logicLinks.sourceObjectId != cr::kInvalidObjectId &&
       cr::creativeObjectCanTargetLogicLink(hovered->kind) &&
       cr::creativeLogicLinkActionSupported(
           hovered->kind, request.logicLinks.action));
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

void appendCreativeEditorOverlayAttachmentSockets(
    const CreativeEditorOverlayRelationshipsAssemblyRequest& request,
    CreativeEditorOverlayFrame& output) {
  if (request.captureMode || request.modalOpen ||
      request.assetCatalog == nullptr || !request.target.objectHit ||
      !request.target.grid.valid) {
    return;
  }

  std::string_view sourceAssetId;
  cr::CreativeObjectId ignoredOccupantObjectId =
      cr::kInvalidObjectId;
  cr::CreativeAttachmentSnapSelectionMode selectionMode =
      cr::CreativeAttachmentSnapSelectionMode::BestMatch;
  switch (request.held.kind) {
    case cr::CreativeHeldItemKind::Material:
      sourceAssetId = cr::creativeHotbarAssetId(request.held);
      if (creativeEditorUsesAssetScatter(
              request.held, request.toolSettings)) {
        return;
      }
      selectionMode =
          request.toolSettings.assetAttachmentMode ==
                  cr::CreativeAssetAttachmentMode::AimSocket
              ? cr::CreativeAttachmentSnapSelectionMode::AimedSocket
              : cr::CreativeAttachmentSnapSelectionMode::BestMatch;
      break;
    case cr::CreativeHeldItemKind::ObjectMove:
      if (request.selection.selectionCount != 1U ||
          request.selection.selected == nullptr ||
          request.selection.selected->id == request.target.objectId) {
        return;
      }
      sourceAssetId = request.selection.selected->assetId;
      ignoredOccupantObjectId = request.selection.selected->id;
      selectionMode =
          cr::CreativeAttachmentSnapSelectionMode::AimedSocket;
      break;
    default:
      return;
  }
  if (sourceAssetId.empty()) {
    return;
  }

  cr::CreativeAttachmentSocketMarkerRequest markerRequest;
  markerRequest.document = &request.document;
  markerRequest.assetCatalog = request.assetCatalog;
  markerRequest.sourceAssetId = sourceAssetId;
  markerRequest.targetObjectId = request.target.objectId;
  markerRequest.aimPoint = request.target.grid.hitPoint;
  markerRequest.ignoredOccupantObjectId = ignoredOccupantObjectId;
  markerRequest.selectionMode = selectionMode;
  const cr::CreativeAttachmentSocketMarkerFrame markers =
      cr::buildCreativeAttachmentSocketMarkers(markerRequest);
  if (!markers.accepted) {
    return;
  }

  constexpr float kForwardMeters = 0.24F;
  constexpr float kBackMeters = 0.04F;
  constexpr float kArrowBackMeters = 0.07F;
  constexpr float kArrowHalfWidthMeters = 0.05F;
  constexpr float kUpMeters = 0.17F;
  constexpr float kSideHalfExtentMeters = 0.08F;
  constexpr float kSelectionRadiusMeters = 0.13F;
  const float thickness = std::max(0.035F, request.gizmoThickness);
  for (std::size_t index = 0U; index < markers.markerCount; ++index) {
    const cr::CreativeAttachmentSocketMarker& marker =
        markers.markers[index];
    const cr::CreativeCoreVec3Conversion position =
        cr::creativeVec3ToCoreChecked(marker.worldPosition);
    const cr::CreativeCoreVec3Conversion forward =
        cr::creativeVec3ToCoreChecked(marker.worldForward);
    const cr::CreativeCoreVec3Conversion up =
        cr::creativeVec3ToCoreChecked(marker.worldUp);
    if (!position.converted || !forward.converted || !up.converted) {
      continue;
    }
    const RenderLineColor color =
        attachmentSocketMarkerColor(marker.state);
    const Vec3 side = cross(forward.value, up.value);
    const float markerThickness =
        marker.selected ? thickness * 1.5F : thickness;
    const auto appendLine = [&](Vec3 start, Vec3 end) {
      RenderCreativeWireframeDebugLine line;
      line.start = start;
      line.end = end;
      line.color = color;
      line.objectId = marker.targetObjectId;
      line.thickness = markerThickness;
      output.combinedWireLines.push_back(line);
      ++output.attachmentSocketMarkerEdgeCount;
    };

    const Vec3 forwardTip =
        position.value + forward.value * kForwardMeters;
    appendLine(position.value - forward.value * kBackMeters, forwardTip);
    appendLine(forwardTip,
               forwardTip - forward.value * kArrowBackMeters +
                   side * kArrowHalfWidthMeters);
    appendLine(forwardTip,
               forwardTip - forward.value * kArrowBackMeters -
                   side * kArrowHalfWidthMeters);
    appendLine(position.value, position.value + up.value * kUpMeters);
    appendLine(position.value - side * kSideHalfExtentMeters,
               position.value + side * kSideHalfExtentMeters);

    if (marker.selected) {
      const Vec3 sidePoint = side * kSelectionRadiusMeters;
      const Vec3 upPoint = up.value * kSelectionRadiusMeters;
      appendLine(position.value + sidePoint,
                 position.value + upPoint);
      appendLine(position.value + upPoint,
                 position.value - sidePoint);
      appendLine(position.value - sidePoint,
                 position.value - upPoint);
      appendLine(position.value - upPoint,
                 position.value + sidePoint);
    }
  }
}

}  // namespace iggy3d_creative_app
