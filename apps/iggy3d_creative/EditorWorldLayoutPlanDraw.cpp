#include "EditorWorldLayoutPlanDraw.hpp"

#include "EditorWorldLayoutCanvasInternal.hpp"
#include "EditorWorldLayoutPlanView.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace iggy3d_creative_app {
namespace {

namespace cr = iggy3d::creative;

using Primitive = cr::CreativeWorldLayoutPlanPrimitive;
using PrimitiveKind = cr::CreativeWorldLayoutPlanPrimitiveKind;
using Style = CreativeEditorDraftingStyle;

constexpr float kPointRadiusPixels = 4.0F;
constexpr float kPi = 3.14159265358979323846F;

ImU32 color(CreativeEditorDraftingColor tint, float alphaMultiplier = 1.0F) {
  tint.a = static_cast<std::uint8_t>(std::clamp(
      static_cast<float>(tint.a) * alphaMultiplier, 0.0F, 255.0F));
  return IM_COL32(tint.r, tint.g, tint.b, tint.a);
}

ImVec2 toScreen(const CreativeEditorWorldLayoutCanvasTransform& transform,
                const cr::CreativeWorldLayoutPlanPoint& point,
                double offsetX, double offsetZ) noexcept {
  return creativeEditorWorldLayoutCanvasToScreen(
      transform, point.x + offsetX, point.z + offsetZ);
}

bool styleApplies(const Style& style,
                  cr::CreativeWorldLayoutPlanLayer layer) noexcept {
  switch (layer) {
    case cr::CreativeWorldLayoutPlanLayer::Context:
      return style.contextLayer;
    case cr::CreativeWorldLayoutPlanLayer::Active:
      return style.normalLayer;
    case cr::CreativeWorldLayoutPlanLayer::Overhead:
      return style.overheadLayer;
    case cr::CreativeWorldLayoutPlanLayer::Count:
      return false;
  }
  return false;
}

Style resolvedStyle(const Primitive& primitive) noexcept {
  Style style = creativeEditorDraftingStyle(
      creativeEditorWorldLayoutPlanDraftingRole(primitive));
  if (primitive.layer != cr::CreativeWorldLayoutPlanLayer::Context) {
    return style;
  }
  const Style& ghost = creativeEditorDraftingStyle(
      CreativeEditorDraftingRole::LowerLevelGhostOverlay);
  style.tint = ghost.tint;
  style.drawOrder = ghost.drawOrder;
  return style;
}

void drawStyledLine(ImDrawList& drawList, ImVec2 start, ImVec2 end,
                    ImU32 tint, float thickness, float dashPixels,
                    float gapPixels, float& patternRemaining,
                    bool& drawing) {
  const float dx = end.x - start.x;
  const float dy = end.y - start.y;
  const float length = std::hypot(dx, dy);
  if (length <= 1.0e-4F || thickness <= 0.0F) {
    return;
  }
  if (dashPixels <= 0.0F || gapPixels <= 0.0F) {
    drawList.AddLine(start, end, tint, thickness);
    return;
  }
  const float ux = dx / length;
  const float uy = dy / length;
  float cursor = 0.0F;
  while (cursor < length - 1.0e-4F) {
    const float step = std::min(patternRemaining, length - cursor);
    if (drawing && step > 0.0F) {
      drawList.AddLine({start.x + ux * cursor, start.y + uy * cursor},
                       {start.x + ux * (cursor + step),
                        start.y + uy * (cursor + step)},
                       tint, thickness);
    }
    cursor += step;
    patternRemaining -= step;
    if (patternRemaining <= 1.0e-4F) {
      drawing = !drawing;
      patternRemaining = drawing ? dashPixels : gapPixels;
    }
  }
}

void drawStyledPolyline(ImDrawList& drawList, const ImVec2* points,
                        std::size_t pointCount, bool closed,
                        const Style& style, float pixelsPerCell) {
  if (pointCount < 2U) {
    return;
  }
  const float thickness = creativeEditorDraftingStrokeThicknessPixels(
      style, pixelsPerCell);
  if (thickness <= 0.0F) {
    return;
  }
  const ImU32 tint = color(style.tint);
  if (style.dashCells <= 0.0F || style.gapCells <= 0.0F) {
    drawList.AddPolyline(points, static_cast<int>(pointCount), tint,
                         closed ? ImDrawFlags_Closed : ImDrawFlags_None,
                         thickness);
    return;
  }
  const float dashPixels = std::max(1.0F, style.dashCells * pixelsPerCell);
  const float gapPixels = std::max(1.0F, style.gapCells * pixelsPerCell);
  float remaining = dashPixels;
  bool drawing = true;
  const std::size_t segmentCount = closed ? pointCount : pointCount - 1U;
  for (std::size_t index = 0U; index < segmentCount; ++index) {
    drawStyledLine(drawList, points[index], points[(index + 1U) % pointCount],
                   tint, thickness, dashPixels, gapPixels, remaining, drawing);
  }
}

void drawPolygonHatch(ImDrawList& drawList, const ImVec2* points,
                      std::size_t pointCount, ImU32 tint) {
  if (pointCount < 3U) {
    return;
  }
  constexpr float kInvSqrtTwo = 0.70710678118F;
  constexpr ImVec2 normal{kInvSqrtTwo, kInvSqrtTwo};
  constexpr ImVec2 direction{kInvSqrtTwo, -kInvSqrtTwo};
  float minimum = points[0].x * normal.x + points[0].y * normal.y;
  float maximum = minimum;
  for (std::size_t index = 1U; index < pointCount; ++index) {
    const float coordinate =
        points[index].x * normal.x + points[index].y * normal.y;
    minimum = std::min(minimum, coordinate);
    maximum = std::max(maximum, coordinate);
  }
  constexpr float kSpacingPixels = 8.0F;
  for (float coordinate = minimum + kSpacingPixels; coordinate < maximum;
       coordinate += kSpacingPixels) {
    std::array<ImVec2, 8U> intersections{};
    std::size_t count = 0U;
    for (std::size_t index = 0U; index < pointCount; ++index) {
      const ImVec2 a = points[index];
      const ImVec2 b = points[(index + 1U) % pointCount];
      const float aCoordinate = a.x * normal.x + a.y * normal.y;
      const float bCoordinate = b.x * normal.x + b.y * normal.y;
      const float denominator = bCoordinate - aCoordinate;
      if (std::abs(denominator) <= 1.0e-4F) {
        continue;
      }
      const float t = (coordinate - aCoordinate) / denominator;
      if (t >= 0.0F && t < 1.0F && count < intersections.size()) {
        intersections[count++] =
            {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
      }
    }
    if (count < 2U) {
      continue;
    }
    std::sort(intersections.begin(), intersections.begin() + count,
              [](ImVec2 lhs, ImVec2 rhs) {
                return lhs.x * direction.x + lhs.y * direction.y <
                       rhs.x * direction.x + rhs.y * direction.y;
              });
    drawList.AddLine(intersections.front(), intersections[count - 1U], tint,
                     1.0F);
  }
}

void drawCircleHatch(ImDrawList& drawList, ImVec2 center, float radius,
                     ImU32 tint) {
  constexpr float kInvSqrtTwo = 0.70710678118F;
  constexpr ImVec2 normal{kInvSqrtTwo, kInvSqrtTwo};
  constexpr ImVec2 direction{kInvSqrtTwo, -kInvSqrtTwo};
  constexpr float kSpacingPixels = 8.0F;
  for (float offset = -radius + kSpacingPixels; offset < radius;
       offset += kSpacingPixels) {
    const float halfChord = std::sqrt(std::max(0.0F, radius * radius -
                                                        offset * offset));
    const ImVec2 midpoint{center.x + normal.x * offset,
                          center.y + normal.y * offset};
    drawList.AddLine({midpoint.x - direction.x * halfChord,
                      midpoint.y - direction.y * halfChord},
                     {midpoint.x + direction.x * halfChord,
                      midpoint.y + direction.y * halfChord},
                     tint, 1.0F);
  }
}

void drawPrimitive(ImDrawList& drawList,
                   const CreativeEditorWorldLayoutCanvasTransform& transform,
                   const Primitive& primitive, const Style& style,
                   double offsetX, double offsetZ) {
  std::array<ImVec2, 65U> points{};
  const float fillAlpha = std::clamp(style.fillAlpha, 0.0F, 1.0F);
  const ImU32 fillTint = color(style.tint, fillAlpha);
  const ImU32 hatchTint = color(style.tint, std::max(fillAlpha, 0.35F));
  switch (primitive.kind) {
    case PrimitiveKind::Segment: {
      points[0] = toScreen(transform, primitive.points[0], offsetX, offsetZ);
      points[1] = toScreen(transform, primitive.points[1], offsetX, offsetZ);
      if (primitive.widthCells > 0.0 &&
          style.fillPattern == CreativeEditorDraftingFillPattern::Solid &&
          fillAlpha > 0.0F) {
        drawList.AddLine(points[0], points[1], fillTint,
                         std::max(1.0F,
                                  static_cast<float>(primitive.widthCells) *
                                      transform.pixelsPerCell));
      }
      drawStyledPolyline(drawList, points.data(), 2U, false, style,
                         transform.pixelsPerCell);
      break;
    }
    case PrimitiveKind::Polygon: {
      for (std::size_t index = 0U; index < primitive.pointCount; ++index) {
        points[index] =
            toScreen(transform, primitive.points[index], offsetX, offsetZ);
      }
      if (style.fillPattern == CreativeEditorDraftingFillPattern::Solid &&
          fillAlpha > 0.0F) {
        drawList.AddConvexPolyFilled(points.data(), primitive.pointCount,
                                     fillTint);
      } else if (style.fillPattern ==
                     CreativeEditorDraftingFillPattern::Hatched &&
                 fillAlpha > 0.0F) {
        drawPolygonHatch(drawList, points.data(), primitive.pointCount,
                         hatchTint);
      }
      drawStyledPolyline(drawList, points.data(), primitive.pointCount, true,
                         style, transform.pixelsPerCell);
      break;
    }
    case PrimitiveKind::Circle: {
      const ImVec2 center =
          toScreen(transform, primitive.points[0], offsetX, offsetZ);
      const float radius = static_cast<float>(primitive.radiusCells) *
                           transform.pixelsPerCell;
      if (style.fillPattern == CreativeEditorDraftingFillPattern::Solid &&
          fillAlpha > 0.0F) {
        drawList.AddCircleFilled(center, radius, fillTint, 48);
      } else if (style.fillPattern ==
                     CreativeEditorDraftingFillPattern::Hatched &&
                 fillAlpha > 0.0F) {
        drawCircleHatch(drawList, center, radius, hatchTint);
      }
      constexpr std::size_t kCircleSegments = 64U;
      for (std::size_t index = 0U; index < kCircleSegments; ++index) {
        const float radians =
            2.0F * kPi * static_cast<float>(index) / kCircleSegments;
        points[index] = {center.x + std::cos(radians) * radius,
                         center.y + std::sin(radians) * radius};
      }
      drawStyledPolyline(drawList, points.data(), kCircleSegments, true, style,
                         transform.pixelsPerCell);
      break;
    }
    case PrimitiveKind::Arc: {
      const ImVec2 center =
          toScreen(transform, primitive.points[0], offsetX, offsetZ);
      const float radius = static_cast<float>(primitive.radiusCells) *
                           transform.pixelsPerCell;
      const std::size_t segmentCount = static_cast<std::size_t>(std::clamp(
          std::ceil(std::abs(primitive.sweepRadians) * radius / 7.0), 6.0,
          64.0));
      for (std::size_t index = 0U; index <= segmentCount; ++index) {
        const double t = static_cast<double>(index) /
                         static_cast<double>(segmentCount);
        const double radians =
            primitive.startRadians + primitive.sweepRadians * t;
        points[index] =
            {center.x + static_cast<float>(std::cos(radians)) * radius,
             center.y + static_cast<float>(std::sin(radians)) * radius};
      }
      drawStyledPolyline(drawList, points.data(), segmentCount + 1U, false,
                         style, transform.pixelsPerCell);
      break;
    }
    case PrimitiveKind::Point: {
      const ImVec2 center =
          toScreen(transform, primitive.points[0], offsetX, offsetZ);
      const float thickness = creativeEditorDraftingStrokeThicknessPixels(
          style, transform.pixelsPerCell);
      const float radius = std::max(kPointRadiusPixels, thickness * 1.7F);
      drawList.AddCircleFilled(center, radius, color(style.tint), 20);
      drawList.AddCircle(center, radius + 3.0F, color(style.tint), 20,
                         std::max(1.0F, thickness));
      break;
    }
    case PrimitiveKind::Count:
      break;
  }
}

}  // namespace

CreativeEditorWorldLayoutPlanDrawReceipt drawCreativeEditorWorldLayoutPlan(
    ImDrawList& drawList,
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeWorldLayout& layout,
    const CreativeEditorWorldLayoutPlanViewCache& planView) {
  CreativeEditorWorldLayoutPlanDrawReceipt receipt;
  const cr::CreativeWorldLayoutPlanProjection& projection =
      planView.projection;
  if (!projection.accepted) {
    return receipt;
  }

  for (const std::size_t index : planView.paintOrder) {
    if (index >= projection.primitives.size()) {
      continue;
    }
    const Primitive& primitive = projection.primitives[index];
    if (creativeEditorWorldLayoutPlanPrimitiveSuppressed(state, layout,
                                                          primitive)) {
      ++receipt.suppressedPrimitiveCount;
      continue;
    }
    const Style style = resolvedStyle(primitive);
    if (!styleApplies(style, primitive.layer)) {
      continue;
    }
    const auto [offsetX, offsetZ] =
        creativeEditorWorldLayoutPlanPrimitiveOffset(state, layout, primitive);
    drawPrimitive(drawList, transform, primitive, style, offsetX, offsetZ);
    ++receipt.basePrimitiveCount;
  }

  const Style& selectedStyle = creativeEditorDraftingStyle(
      CreativeEditorDraftingRole::SelectedOverlay);
  for (const std::size_t index : planView.paintOrder) {
    if (index >= projection.primitives.size()) {
      continue;
    }
    const Primitive& primitive = projection.primitives[index];
    if (creativeEditorWorldLayoutPlanPrimitiveSuppressed(state, layout,
                                                          primitive) ||
        !creativeEditorWorldLayoutPlanPrimitiveSelected(state, primitive)) {
      continue;
    }
    const auto [offsetX, offsetZ] =
        creativeEditorWorldLayoutPlanPrimitiveOffset(state, layout, primitive);
    drawPrimitive(drawList, transform, primitive, selectedStyle, offsetX,
                  offsetZ);
    ++receipt.selectedPrimitiveCount;
  }
  return receipt;
}

}  // namespace iggy3d_creative_app
