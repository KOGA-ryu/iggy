#include "EditorShapePreview.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>

#include "EditorPreviewProxies.hpp"
#include "app/iggy3d/creative/Geometry.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
using namespace iggy3d;
namespace {

void appendPreviewLine(
    std::vector<RenderCreativeWireframeDebugLine>& lines,
    Vec3 start,
    Vec3 end,
    RenderLineColor color,
    float thickness) {
  if (!isFinite(start) || !isFinite(end) ||
      (start.x == end.x && start.y == end.y && start.z == end.z)) {
    return;
  }
  RenderCreativeWireframeDebugLine line;
  line.start = start;
  line.end = end;
  line.color = color;
  line.thickness = thickness;
  lines.push_back(line);
}

[[nodiscard]] Vec3 ellipsePoint(Vec3 center,
                                Vec3 firstAxis,
                                Vec3 secondAxis,
                                float angle) noexcept {
  const float cosine = std::cos(angle);
  const float sine = std::sin(angle);
  return {center.x + firstAxis.x * cosine + secondAxis.x * sine,
          center.y + firstAxis.y * cosine + secondAxis.y * sine,
          center.z + firstAxis.z * cosine + secondAxis.z * sine};
}

void appendEllipseLoop(
    std::vector<RenderCreativeWireframeDebugLine>& lines,
    Vec3 center,
    Vec3 firstAxis,
    Vec3 secondAxis,
    RenderLineColor color,
    float thickness) {
  constexpr std::size_t kSegmentCount = 48U;
  constexpr float kTurn = 2.0F * std::numbers::pi_v<float>;
  Vec3 previous = ellipsePoint(center, firstAxis, secondAxis, 0.0F);
  for (std::size_t segment = 1U; segment <= kSegmentCount; ++segment) {
    const float angle = kTurn * static_cast<float>(segment) /
                        static_cast<float>(kSegmentCount);
    const Vec3 current = ellipsePoint(center, firstAxis, secondAxis, angle);
    appendPreviewLine(lines, previous, current, color, thickness);
    previous = current;
  }
}

[[nodiscard]] Vec3 renderVec3(cr::CreativeVec3 value) noexcept {
  return cr::creativeVec3ToCoreChecked(value).value;
}

[[nodiscard]] Vec3 cellCenter(const cr::CreativeVolumeSelection& selection,
                              cr::CreativeGridCoord3 cell) noexcept {
  const cr::CreativeBounds bounds = cr::creativeVolumeCellBounds(
      cell, selection.cellSize, selection.origin);
  return cr::creativeVec3ToCoreChecked(
             cr::measureCreativeBounds(bounds).center)
      .value;
}

}  // namespace

void appendCreativeMaterialBrushCellOutlines(
    std::vector<RenderCreativeWireframeDebugLine>& lines,
    std::span<const cr::CreativeGridCoord3> cells,
    const cr::CreativeGridSettings& grid,
    RenderLineColor color,
    float thickness) {
  constexpr std::size_t kBoxEdgeCount = 12U;
  lines.reserve(lines.size() + cells.size() * kBoxEdgeCount);
  for (cr::CreativeGridCoord3 cell : cells) {
    const cr::CreativeBounds bounds = cr::creativeVolumeCellBounds(
        cell, grid.cellSizeMeters, grid.origin);
    const cr::CreativeCoreVec3Conversion minimum =
        cr::creativeVec3ToCoreChecked(bounds.min);
    const cr::CreativeCoreVec3Conversion maximum =
        cr::creativeVec3ToCoreChecked(bounds.max);
    if (!minimum.converted || !maximum.converted) {
      continue;
    }
    appendStandaloneWireframeBoxEdges(lines, minimum.value, maximum.value,
                                      color, thickness);
  }
}

void appendCreativeMaterialBrushStampOutline(
    std::vector<RenderCreativeWireframeDebugLine>& lines,
    const cr::CreativeMaterialBrushStampPlan& plan,
    const cr::CreativeGridSettings& grid,
    RenderLineColor color,
    float thickness) {
  if (!plan.accepted) {
    return;
  }
  appendCreativeMaterialBrushCellOutlines(
      lines, plan.generatedCells(), grid, color, thickness);
}

bool appendCreativeMaterialBrushGuideLine(
    std::vector<RenderCreativeWireframeDebugLine>& lines,
    cr::CreativeMaterialBrushGuide guide,
    cr::CreativeGridCoord3 anchor,
    cr::CreativeGridCoord3 target,
    const cr::CreativeGridSettings& grid,
    float thickness) {
  cr::CreativeAxis3 axis = cr::CreativeAxis3::X;
  if (!cr::creativeMaterialBrushLineAxis(guide, axis)) {
    return false;
  }
  const cr::CreativeBounds anchorBounds = cr::creativeVolumeCellBounds(
      anchor, grid.cellSizeMeters, grid.origin);
  const cr::CreativeBounds targetBounds = cr::creativeVolumeCellBounds(
      target, grid.cellSizeMeters, grid.origin);
  const cr::CreativeCoreVec3Conversion start = cr::creativeVec3ToCoreChecked(
      cr::measureCreativeBounds(anchorBounds).center);
  const cr::CreativeCoreVec3Conversion end = cr::creativeVec3ToCoreChecked(
      cr::measureCreativeBounds(targetBounds).center);
  if (!start.converted || !end.converted) {
    return false;
  }

  RenderLineColor color;
  switch (axis) {
    case cr::CreativeAxis3::X:
      color = {1.0F, 0.22F, 0.18F, 1.0F};
      break;
    case cr::CreativeAxis3::Y:
      color = {0.24F, 1.0F, 0.34F, 1.0F};
      break;
    case cr::CreativeAxis3::Z:
      color = {0.22F, 0.55F, 1.0F, 1.0F};
      break;
    case cr::CreativeAxis3::Count:
      return false;
  }
  const std::size_t before = lines.size();
  appendPreviewLine(lines, start.value, end.value, color, thickness);
  return lines.size() == before + 1U;
}

void appendCreativeShapeBrushOutline(
    std::vector<RenderCreativeWireframeDebugLine>& lines,
    const cr::CreativeVolumeSelection& selection,
    cr::CreativeShapeBrushKind kind,
    cr::CreativeShapeBrushAxis axis,
    RenderLineColor color,
    float thickness) {
  const cr::CreativeBounds bounds = cr::creativeVolumeWorldBounds(selection);
  const Vec3 minimum = renderVec3(bounds.min);
  const Vec3 maximum = renderVec3(bounds.max);
  const Vec3 center{(minimum.x + maximum.x) * 0.5F,
                    (minimum.y + maximum.y) * 0.5F,
                    (minimum.z + maximum.z) * 0.5F};
  const Vec3 radii{(maximum.x - minimum.x) * 0.5F,
                   (maximum.y - minimum.y) * 0.5F,
                   (maximum.z - minimum.z) * 0.5F};

  switch (kind) {
    case cr::CreativeShapeBrushKind::Box:
      appendStandaloneWireframeBoxEdges(lines, minimum, maximum, color,
                                        thickness);
      return;
    case cr::CreativeShapeBrushKind::Line: {
      const Vec3 first = cellCenter(selection, selection.firstCell);
      const Vec3 second = cellCenter(selection, selection.secondCell);
      appendPreviewLine(lines, first, second, color,
                        std::max(thickness, 0.045F));
      const cr::CreativeBounds firstBounds = cr::creativeVolumeCellBounds(
          selection.firstCell, selection.cellSize, selection.origin);
      appendStandaloneWireframeBoxEdges(lines, renderVec3(firstBounds.min),
                                        renderVec3(firstBounds.max), color,
                                        thickness);
      if (selection.firstCell.x != selection.secondCell.x ||
          selection.firstCell.y != selection.secondCell.y ||
          selection.firstCell.z != selection.secondCell.z) {
        const cr::CreativeBounds secondBounds = cr::creativeVolumeCellBounds(
            selection.secondCell, selection.cellSize, selection.origin);
        appendStandaloneWireframeBoxEdges(lines, renderVec3(secondBounds.min),
                                          renderVec3(secondBounds.max), color,
                                          thickness);
      }
      return;
    }
    case cr::CreativeShapeBrushKind::Ellipsoid:
      appendEllipseLoop(lines, center, {radii.x, 0.0F, 0.0F},
                        {0.0F, radii.y, 0.0F}, color, thickness);
      appendEllipseLoop(lines, center, {radii.x, 0.0F, 0.0F},
                        {0.0F, 0.0F, radii.z}, color, thickness);
      appendEllipseLoop(lines, center, {0.0F, radii.y, 0.0F},
                        {0.0F, 0.0F, radii.z}, color, thickness);
      return;
    case cr::CreativeShapeBrushKind::Cylinder: {
      Vec3 firstCenter = center;
      Vec3 secondCenter = center;
      Vec3 firstRadius{};
      Vec3 secondRadius{};
      switch (axis) {
        case cr::CreativeShapeBrushAxis::X:
          firstCenter.x = minimum.x;
          secondCenter.x = maximum.x;
          firstRadius = {0.0F, radii.y, 0.0F};
          secondRadius = {0.0F, 0.0F, radii.z};
          break;
        case cr::CreativeShapeBrushAxis::Y:
          firstCenter.y = minimum.y;
          secondCenter.y = maximum.y;
          firstRadius = {radii.x, 0.0F, 0.0F};
          secondRadius = {0.0F, 0.0F, radii.z};
          break;
        case cr::CreativeShapeBrushAxis::Z:
          firstCenter.z = minimum.z;
          secondCenter.z = maximum.z;
          firstRadius = {radii.x, 0.0F, 0.0F};
          secondRadius = {0.0F, radii.y, 0.0F};
          break;
        case cr::CreativeShapeBrushAxis::Count:
          appendStandaloneWireframeBoxEdges(lines, minimum, maximum,
                                            {1.0F, 0.15F, 0.12F, 1.0F},
                                            thickness);
          return;
      }
      appendEllipseLoop(lines, firstCenter, firstRadius, secondRadius, color,
                        thickness);
      appendEllipseLoop(lines, secondCenter, firstRadius, secondRadius, color,
                        thickness);
      for (const float sign : {-1.0F, 1.0F}) {
        appendPreviewLine(
            lines,
            {firstCenter.x + firstRadius.x * sign,
             firstCenter.y + firstRadius.y * sign,
             firstCenter.z + firstRadius.z * sign},
            {secondCenter.x + firstRadius.x * sign,
             secondCenter.y + firstRadius.y * sign,
             secondCenter.z + firstRadius.z * sign},
            color, thickness);
        appendPreviewLine(
            lines,
            {firstCenter.x + secondRadius.x * sign,
             firstCenter.y + secondRadius.y * sign,
             firstCenter.z + secondRadius.z * sign},
            {secondCenter.x + secondRadius.x * sign,
             secondCenter.y + secondRadius.y * sign,
             secondCenter.z + secondRadius.z * sign},
            color, thickness);
      }
      return;
    }
    case cr::CreativeShapeBrushKind::Count:
      appendStandaloneWireframeBoxEdges(lines, minimum, maximum,
                                        {1.0F, 0.15F, 0.12F, 1.0F},
                                        thickness);
      return;
  }
  appendStandaloneWireframeBoxEdges(lines, minimum, maximum,
                                    {1.0F, 0.15F, 0.12F, 1.0F}, thickness);
}

}  // namespace iggy3d_creative_app
