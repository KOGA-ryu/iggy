#include "EditorOverlayAssemblersInternal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include "EditorPathEditing.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorWorldLayoutRoofs.hpp"
#include "EditorWorldLayoutVerticalConnectorHandles.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/input/HeldItemRegistry.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
using namespace iggy3d;
namespace {

void appendMeasurementGeometry(
    const cr::CreativeMeasurementGeometry& geometry,
    RenderLineColor lineColor,
    RenderLineColor pointColor,
    CreativeEditorOverlayFrame& output) {
  if (!geometry.visible) {
    return;
  }

  constexpr float kPointMarkerHalfExtent = 0.075F;
  for (std::size_t index = 0U; index < geometry.segmentCount; ++index) {
    const cr::CreativeCoreVec3Conversion start =
        cr::creativeVec3ToCoreChecked(
            {geometry.segments[index].start.x,
             geometry.segments[index].start.y,
             geometry.segments[index].start.z});
    const cr::CreativeCoreVec3Conversion end =
        cr::creativeVec3ToCoreChecked(
            {geometry.segments[index].end.x,
             geometry.segments[index].end.y,
             geometry.segments[index].end.z});
    if (!start.converted || !end.converted) {
      continue;
    }
    output.combinedWireLines.push_back(
        {start.value, end.value, lineColor, 0U, 0U, 0U, 0U, 0.035F});
  }
  for (std::size_t index = 0U; index < geometry.pointCount; ++index) {
    const cr::CreativeCoreVec3Conversion point =
        cr::creativeVec3ToCoreChecked(
            {geometry.points[index].x, geometry.points[index].y,
             geometry.points[index].z});
    if (!point.converted) {
      continue;
    }
    constexpr std::array axes{
        Vec3{kPointMarkerHalfExtent, 0.0F, 0.0F},
        Vec3{0.0F, kPointMarkerHalfExtent, 0.0F},
        Vec3{0.0F, 0.0F, kPointMarkerHalfExtent},
    };
    for (const Vec3 axis : axes) {
      output.combinedWireLines.push_back(
          {point.value - axis, point.value + axis, pointColor,
           0U, 0U, 0U, 0U, 0.045F});
    }
  }
}

}  // namespace

void appendCreativeEditorOverlayWorldLayoutRoofHandles(
    const CreativeEditorOverlayWorldAssemblyRequest& request,
    CreativeEditorOverlayFrame& output) {
  const cr::CreativeHeldItemDefinition& held =
      cr::describeCreativeHeldItem(request.held.kind);
  if (request.captureMode ||
      request.inputContext != cr::CreativeInputContext::EditorViewport ||
      !held.hierarchySelectionTool ||
      request.worldLayout.tool != CreativeEditorWorldLayoutTool::Select) {
    return;
  }
  const std::size_t levelIndex =
      request.worldLayout.roofManipulation.active
          ? request.worldLayout.roofManipulation.target.levelIndex
          : request.worldLayout.selection.kind ==
                    CreativeEditorWorldLayoutSelectionKind::Level
                ? request.worldLayout.selection.index
                : cr::kInvalidCreativeWorldLayoutIndex;
  if (levelIndex == cr::kInvalidCreativeWorldLayoutIndex) {
    return;
  }
  const cr::CreativeGridSettings grid = request.document.gridSettings();
  const CreativeEditorWorldLayoutRoofHandleFrame frame =
      buildCreativeEditorWorldLayoutRoofHandleFrame(
          request.worldLayout, grid, levelIndex);
  if (!frame.accepted) {
    return;
  }

  const bool active = request.worldLayout.roofManipulation.active;
  const RenderLineColor tint =
      active && !request.worldLayout.roofManipulation.previewValid
          ? RenderLineColor{1.0F, 0.18F, 0.16F, 1.0F}
          : active ? RenderLineColor{0.20F, 1.0F, 0.35F, 1.0F}
                   : RenderLineColor{1.0F, 0.82F, 0.18F, 1.0F};
  const float thickness = std::max(0.045F, request.gizmoThickness);
  std::vector<RenderCreativeWireframeDebugLine>& lines =
      output.combinedWireLines;
  const std::size_t before = lines.size();
  for (std::size_t index = 0U;
       index < frame.roof.geometry.edgeCount; ++index) {
    const cr::CreativeStructuralRoofEdgePlan& edge =
        frame.roof.geometry.edges[index];
    const cr::CreativeCoreVec3Conversion start =
        cr::creativeVec3ToCoreChecked(edge.startMeters);
    const cr::CreativeCoreVec3Conversion end =
        cr::creativeVec3ToCoreChecked(edge.endMeters);
    if (start.converted && end.converted) {
      lines.push_back({start.value, end.value, tint, 0U, 0U, 0U, 0U,
                       thickness * 0.72F});
    }
  }

  const float shaftHalfLength = static_cast<float>(std::clamp(
      grid.cellSizeMeters * 0.24, 0.12, 0.40));
  const float crossHalfLength = shaftHalfLength * 0.52F;
  for (std::size_t index = 0U; index < frame.handleCount; ++index) {
    const CreativeEditorWorldLayoutRoofHandle& handle =
        frame.handles[index];
    if (!handle.valid) {
      continue;
    }
    const Vec3 position = handle.worldPosition;
    const Vec3 axis = handle.worldAxis;
    const Vec3 firstCross =
        std::fabs(axis.y) > 0.5F ? Vec3{1.0F, 0.0F, 0.0F}
                                : Vec3{0.0F, 1.0F, 0.0F};
    const Vec3 secondCross =
        std::fabs(axis.y) > 0.5F
            ? Vec3{0.0F, 0.0F, 1.0F}
            : Vec3{-axis.z, 0.0F, axis.x};
    lines.push_back({position - axis * shaftHalfLength,
                     position + axis * shaftHalfLength, tint,
                     0U, 0U, 0U, 0U, thickness});
    lines.push_back({position - firstCross * crossHalfLength,
                     position + firstCross * crossHalfLength, tint,
                     0U, 0U, 0U, 0U, thickness});
    lines.push_back({position - secondCross * crossHalfLength,
                     position + secondCross * crossHalfLength, tint,
                     0U, 0U, 0U, 0U, thickness});
  }
  output.worldLayoutRoofHandleEdgeCount = lines.size() - before;
}

void appendCreativeEditorOverlayWorldLayoutVerticalConnectorHandles(
    const CreativeEditorOverlayWorldAssemblyRequest& request,
    CreativeEditorOverlayFrame& output) {
  const cr::CreativeHeldItemDefinition& held =
      cr::describeCreativeHeldItem(request.held.kind);
  if (request.captureMode ||
      request.inputContext != cr::CreativeInputContext::EditorViewport ||
      !held.hierarchySelectionTool ||
      request.worldLayout.tool != CreativeEditorWorldLayoutTool::Select) {
    return;
  }
  const std::size_t connectorIndex =
      request.worldLayout.verticalConnectorManipulation.active
          ? request.worldLayout.verticalConnectorManipulation.target
                .connectorIndex
          : request.worldLayout.selection.kind ==
                    CreativeEditorWorldLayoutSelectionKind::VerticalConnector
                ? request.worldLayout.selection.index
                : cr::kInvalidCreativeWorldLayoutIndex;
  const cr::CreativeGridSettings grid = request.document.gridSettings();
  const CreativeEditorWorldLayoutVerticalConnectorHandleFrame frame =
      buildCreativeEditorWorldLayoutVerticalConnectorHandleFrame(
          request.worldLayout, grid, connectorIndex);
  if (!frame.accepted ||
      connectorIndex >=
          request.worldLayout.source.verticalConnectors.size()) {
    return;
  }

  const bool active =
      request.worldLayout.verticalConnectorManipulation.active;
  const RenderLineColor tint =
      active &&
              !request.worldLayout.verticalConnectorManipulation.previewValid
          ? RenderLineColor{1.0F, 0.18F, 0.16F, 1.0F}
          : active ? RenderLineColor{0.20F, 1.0F, 0.35F, 1.0F}
                   : RenderLineColor{1.0F, 0.82F, 0.18F, 1.0F};
  const float thickness = std::max(0.045F, request.gizmoThickness);
  std::vector<RenderCreativeWireframeDebugLine>& lines =
      output.combinedWireLines;
  const std::size_t before = lines.size();

  cr::CreativeWorldLayoutRect footprint =
      request.worldLayout.source.verticalConnectors[connectorIndex]
          .footprint;
  if (active &&
      request.worldLayout.verticalConnectorManipulation.previewValid) {
    footprint =
        request.worldLayout.verticalConnectorManipulation.previewFootprint;
  }
  const cr::CreativeCoreVec3Conversion minimum =
      cr::creativeVec3ToCoreChecked(
          {grid.origin.x + footprint.minimum.x * grid.cellSizeMeters,
           frame.connector.authoredBounds.min.y,
           grid.origin.z + footprint.minimum.z * grid.cellSizeMeters});
  const cr::CreativeCoreVec3Conversion maximum =
      cr::creativeVec3ToCoreChecked(
          {grid.origin.x + footprint.maximum.x * grid.cellSizeMeters,
           frame.connector.authoredBounds.max.y,
           grid.origin.z + footprint.maximum.z * grid.cellSizeMeters});
  if (minimum.converted && maximum.converted) {
    appendStandaloneWireframeBoxEdges(lines, minimum.value, maximum.value,
                                      tint, thickness * 0.72F);
  }

  const float shaftHalfLength = static_cast<float>(std::clamp(
      grid.cellSizeMeters * 0.24, 0.12, 0.40));
  const float crossHalfLength = shaftHalfLength * 0.52F;
  for (std::size_t index = 0U; index < frame.handleCount; ++index) {
    const CreativeEditorWorldLayoutVerticalConnectorHandle& handle =
        frame.handles[index];
    if (!handle.valid) {
      continue;
    }
    const Vec3 position = handle.worldPosition;
    const Vec3 axis =
        handle.planeSample ? Vec3{0.0F, 1.0F, 0.0F} : handle.worldAxis;
    const Vec3 firstCross =
        std::fabs(axis.y) > 0.5F ? Vec3{1.0F, 0.0F, 0.0F}
                                : Vec3{0.0F, 1.0F, 0.0F};
    const Vec3 secondCross =
        std::fabs(axis.y) > 0.5F
            ? Vec3{0.0F, 0.0F, 1.0F}
            : Vec3{-axis.z, 0.0F, axis.x};
    lines.push_back({position - axis * shaftHalfLength,
                     position + axis * shaftHalfLength, tint,
                     0U, 0U, 0U, 0U, thickness});
    lines.push_back({position - firstCross * crossHalfLength,
                     position + firstCross * crossHalfLength, tint,
                     0U, 0U, 0U, 0U, thickness});
    lines.push_back({position - secondCross * crossHalfLength,
                     position + secondCross * crossHalfLength, tint,
                     0U, 0U, 0U, 0U, thickness});
  }
  output.worldLayoutVerticalConnectorHandleEdgeCount =
      lines.size() - before;
}

void appendCreativeEditorOverlayMeasurement(
    const CreativeEditorOverlayWorldAssemblyRequest& request,
    CreativeEditorOverlayFrame& output) {
  if (request.captureMode) {
    return;
  }

  constexpr RenderLineColor kSavedLineColor{
      0.20F, 0.70F, 0.82F, 0.90F};
  constexpr RenderLineColor kSavedPointColor{
      0.30F, 0.78F, 0.40F, 0.90F};
  constexpr RenderLineColor kTransientLineColor{
      0.18F, 0.90F, 1.0F, 1.0F};
  constexpr RenderLineColor kTransientPointColor{
      0.32F, 1.0F, 0.42F, 1.0F};
  const std::size_t before = output.combinedWireLines.size();
  for (const cr::CreativeMeasurementAnnotation& annotation :
       request.document.measurementAnnotationStore().annotations) {
    appendMeasurementGeometry(
        cr::buildCreativeMeasurementGeometry(annotation),
        kSavedLineColor, kSavedPointColor, output);
  }
  appendMeasurementGeometry(
      cr::buildCreativeMeasurementGeometry(request.measurementState),
      kTransientLineColor, kTransientPointColor, output);
  output.measurementEdgeCount =
      output.combinedWireLines.size() - before;
}

void appendCreativeEditorOverlayMovingPlatformPath(
    const CreativeEditorOverlayWorldAssemblyRequest& request,
    CreativeEditorOverlayFrame& output) {
  if (request.captureMode || request.modalOpen ||
      request.inputContext != cr::CreativeInputContext::EditorViewport ||
      cr::describeCreativeHeldItem(request.held.kind).interactionMode !=
          cr::CreativeHeldItemInteractionMode::ObjectMove ||
      !request.movingPlatformPathEdit.available ||
      request.selected == nullptr ||
      request.selected->id != request.movingPlatformPathEdit.objectId) {
    return;
  }

  bool visible = false;
  bool allowed = false;
  bool segmentVisible = false;
  cr::CreativeObjectId objectId = cr::kInvalidObjectId;
  cr::CreativeVec3 fromPoint{};
  cr::CreativeVec3 targetPoint{};
  if (request.movingPlatformPathEdit.pointSelected) {
    const CreativeMovingPlatformPathPointTargetPlan preview =
        planCreativeMovingPlatformPathPointTarget(
            request.selected,
            request.movingPlatformPathEdit.selectedPointIndex,
            request.target.grid.valid,
            request.target.grid.placementAnchor,
            request.toolSettings.moveConstraint);
    visible = preview.visible;
    allowed = preview.moveAllowed;
    segmentVisible = preview.segmentVisible;
    objectId = preview.objectId;
    fromPoint = preview.fromPoint;
    targetPoint = preview.targetPoint;
  } else {
    const CreativeMovingPlatformPathTargetPlan preview =
        planCreativeMovingPlatformPathTarget(
            request.selected, request.target.grid.valid,
            request.target.grid.placementAnchor);
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
  appendStandaloneWireframeBoxEdges(
      lines, marker.min, marker.max, color, thickness * 0.8F);
  for (std::size_t index = before; index < lines.size(); ++index) {
    lines[index].objectId = objectId;
  }
  output.movingPlatformPathPreviewEdgeCount = lines.size() - before;
}

}  // namespace iggy3d_creative_app
