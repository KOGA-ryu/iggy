#include "EditorPlacementFeedback.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "EditorInteraction.hpp"
#include "EditorPlacement.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/spatial/SurfacePose.hpp"
#include "app/iggy3d/creative/tools/Volume.hpp"
#include "core/math/OrientedBox.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

constexpr std::array<std::array<std::size_t, 2U>, 12U> kBoxEdgeIndices{{
    {0U, 1U}, {2U, 3U}, {4U, 5U}, {6U, 7U},
    {0U, 2U}, {1U, 3U}, {4U, 6U}, {5U, 7U},
    {0U, 4U}, {1U, 5U}, {2U, 6U}, {3U, 7U},
}};

constexpr std::uint32_t kPlacementInvalidTargetSegmentKind = 8U;
constexpr std::uint32_t kPlacementBlockerSegmentKind = 9U;

std::size_t appendBoxCornerEdges(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    const std::array<iggy3d::Vec3, 8U>& corners,
    iggy3d::RenderLineColor color,
    float thickness,
    cr::CreativeObjectId objectId,
    std::uint32_t segmentKind) {
  for (const iggy3d::Vec3 corner : corners) {
    if (!iggy3d::isFinite(corner)) {
      return 0U;
    }
  }
  const std::size_t begin = lines.size();
  for (const auto& edge : kBoxEdgeIndices) {
    iggy3d::RenderCreativeWireframeDebugLine line;
    line.start = corners[edge[0]];
    line.end = corners[edge[1]];
    line.color = color;
    line.objectId = objectId;
    line.segmentKind = segmentKind;
    line.thickness = thickness;
    lines.push_back(line);
  }
  return lines.size() - begin;
}

std::size_t appendCreativeBoxCornerEdges(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    const std::array<cr::CreativeVec3, 8U>& corners,
    iggy3d::RenderLineColor color,
    float thickness,
    cr::CreativeObjectId objectId,
    std::uint32_t segmentKind) {
  std::array<iggy3d::Vec3, 8U> converted{};
  for (std::size_t index = 0U; index < corners.size(); ++index) {
    const cr::CreativeCoreVec3Conversion corner =
        cr::creativeVec3ToCoreChecked(corners[index]);
    if (!corner.converted) {
      return 0U;
    }
    converted[index] = corner.value;
  }
  return appendBoxCornerEdges(lines, converted, color, thickness, objectId,
                              segmentKind);
}

std::size_t appendPlacementTargetBoundary(
    const CreativeBrushPlacementPlan& plan,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines) {
  const cr::CreativeTransformedBounds transformed =
      cr::resolveCreativeTransformedBounds(plan.authoredBounds,
                                           plan.transform);
  if (!transformed.valid) {
    return 0U;
  }
  return appendCreativeBoxCornerEdges(
      lines, transformed.corners, {1.0F, 0.12F, 0.10F, 1.0F}, thickness,
      cr::kInvalidObjectId, kPlacementInvalidTargetSegmentKind);
}

std::size_t appendAabbBlocker(
    cr::CreativeBounds bounds,
    cr::CreativeObjectId objectId,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines) {
  const cr::CreativeCoreVec3Conversion minimum =
      cr::creativeVec3ToCoreChecked(bounds.min);
  const cr::CreativeCoreVec3Conversion maximum =
      cr::creativeVec3ToCoreChecked(bounds.max);
  if (!minimum.converted || !maximum.converted) {
    return 0U;
  }
  const std::size_t begin = lines.size();
  appendStandaloneWireframeBoxEdges(
      lines, minimum.value, maximum.value,
      {1.0F, 0.28F, 0.12F, 1.0F}, thickness);
  for (std::size_t index = begin; index < lines.size(); ++index) {
    lines[index].objectId = objectId;
    lines[index].segmentKind = kPlacementBlockerSegmentKind;
  }
  return lines.size() - begin;
}

std::size_t appendAuthoredPlacementBlocker(
    const cr::CreativeDocument& document,
    cr::CreativeObjectId objectId,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines) {
  const cr::CreativeObject* object = document.findObject(objectId);
  if (object == nullptr) {
    return 0U;
  }
  if (const std::optional<iggy3d::OrientedBox> box =
          orientedVisualBoxForObject(*object)) {
    return appendBoxCornerEdges(
        lines, iggy3d::orientedBoxCorners(*box),
        {1.0F, 0.28F, 0.12F, 1.0F}, thickness, objectId,
        kPlacementBlockerSegmentKind);
  }
  const VisualBounds visual = visualBoundsForObject(*object);
  return appendAabbBlocker(
      {{visual.min.x, visual.min.y, visual.min.z},
       {visual.max.x, visual.max.y, visual.max.z}},
      objectId, thickness, lines);
}

std::size_t appendTerrainPlacementBlocker(
    const cr::CreativeDocument& document,
    cr::CreativeTerrainCoord2 cell,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines) {
  const cr::CreativeGridSettings grid = document.gridSettings();
  if (!cr::isFiniteCreativeVec3(grid.origin) ||
      !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0) {
    return 0U;
  }
  const cr::CreativeVec3 center{
      grid.origin.x +
          (static_cast<double>(cell.x) + 0.5) * grid.cellSizeMeters,
      grid.origin.y,
      grid.origin.z +
          (static_cast<double>(cell.z) + 0.5) * grid.cellSizeMeters};
  const cr::CreativeTerrainSurfacePose pose =
      cr::sampleCreativeTerrainSurfacePose(
          {&document.terrainField(), center, grid.origin,
           grid.cellSizeMeters});
  if (!pose.accepted || !pose.present) {
    return 0U;
  }
  const double halfHeight =
      std::clamp(grid.cellSizeMeters * 0.04, 0.025, 0.10);
  return appendAabbBlocker(
      {{center.x - grid.cellSizeMeters * 0.5,
        pose.position.y - halfHeight,
        center.z - grid.cellSizeMeters * 0.5},
       {center.x + grid.cellSizeMeters * 0.5,
        pose.position.y + halfHeight,
        center.z + grid.cellSizeMeters * 0.5}},
      cr::kInvalidObjectId, thickness, lines);
}

std::size_t appendPlacementBlocker(
    const cr::CreativeDocument& document,
    const cr::CreativePlacementClearanceResult& clearance,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines) {
  switch (clearance.status) {
    case cr::CreativePlacementClearanceStatus::OutsideWorldBounds:
      return appendAabbBlocker(document.worldBounds(),
                               cr::kInvalidObjectId, thickness, lines);
    case cr::CreativePlacementClearanceStatus::AuthoredObjectBlocked:
      return appendAuthoredPlacementBlocker(
          document, clearance.blockingObjectId, thickness, lines);
    case cr::CreativePlacementClearanceStatus::VoxelBlocked:
      return appendAabbBlocker(
          cr::creativeVolumeCellBounds(
              clearance.blockingVoxelCell,
              document.gridSettings().cellSizeMeters,
              document.gridSettings().origin),
          cr::kInvalidObjectId, thickness, lines);
    case cr::CreativePlacementClearanceStatus::TerrainBlocked:
      return appendTerrainPlacementBlocker(
          document, clearance.blockingTerrainCell, thickness, lines);
    case cr::CreativePlacementClearanceStatus::NotEvaluated:
    case cr::CreativePlacementClearanceStatus::InvalidRequest:
    case cr::CreativePlacementClearanceStatus::TraversalLimitExceeded:
    case cr::CreativePlacementClearanceStatus::Ready:
      return 0U;
  }
  return 0U;
}

}  // namespace

std::string creativeEditorPlacementFeedbackLabel(
    const CreativeEditorPlacementFeedback& feedback,
    const cr::CreativeDocument& document) {
  if (feedback.status != CreativeEditorPlacementFeedbackStatus::Rejected) {
    return {};
  }
  switch (feedback.clearance.status) {
    case cr::CreativePlacementClearanceStatus::OutsideWorldBounds:
      return "Outside build bounds";
    case cr::CreativePlacementClearanceStatus::AuthoredObjectBlocked: {
      const cr::CreativeObject* blocker =
          document.findObject(feedback.clearance.blockingObjectId);
      if (blocker == nullptr) {
        return "Blocked: object";
      }
      std::string label = "Blocked: ";
      label.append(cr::toString(blocker->kind));
      return label;
    }
    case cr::CreativePlacementClearanceStatus::VoxelBlocked: {
      const cr::CreativeObjectKind material = document.voxelField().materialAt(
          feedback.clearance.blockingVoxelCell);
      if (material == cr::CreativeObjectKind::Unknown) {
        return "Blocked: voxel";
      }
      std::string label = "Blocked: ";
      label.append(cr::toString(material));
      return label;
    }
    case cr::CreativePlacementClearanceStatus::TerrainBlocked:
      return "Blocked: terrain";
    case cr::CreativePlacementClearanceStatus::TraversalLimitExceeded:
      return "Placement too large";
    case cr::CreativePlacementClearanceStatus::InvalidRequest:
      return "Invalid placement";
    case cr::CreativePlacementClearanceStatus::NotEvaluated:
    case cr::CreativePlacementClearanceStatus::Ready:
      return "Placement rejected";
  }
  return "Placement rejected";
}

void appendCreativeEditorPlacementClearanceWireframes(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  const cr::CreativeDocument& document = request.appState.facade.document();
  const float thickness =
      std::max(0.07F, request.gizmoThickness * 1.35F);
  if (output.placementPreview.hasTargetPlan) {
    const CreativeBrushPlacementPlan& plan =
        output.placementPreview.targetPlan;
    if (!plan.clearance.evaluated || plan.clearance.allowed) {
      return;
    }
    output.placementInvalidTargetEdgeCount = appendPlacementTargetBoundary(
        plan, thickness, output.combinedWireLines);
    output.placementBlockerEdgeCount = appendPlacementBlocker(
        document, plan.clearance, thickness, output.combinedWireLines);
    return;
  }

  const CreativeEditorPlacementFeedback& feedback =
      request.editor.interaction.placementFeedback;
  if (feedback.status != CreativeEditorPlacementFeedbackStatus::Rejected ||
      !creativeEditorPlacementFeedbackVisible(
          feedback, request.editor.frameIndex) ||
      !feedback.clearance.evaluated || feedback.clearance.allowed) {
    return;
  }
  output.placementBlockerEdgeCount = appendPlacementBlocker(
      document, feedback.clearance, thickness, output.combinedWireLines);
}

}  // namespace iggy3d_creative_app
