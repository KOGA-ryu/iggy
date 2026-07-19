#include "EditorWorldLayoutCanvasInternal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>

namespace iggy3d_creative_app {
namespace {

using CanvasTransform = CreativeEditorWorldLayoutCanvasTransform;

ImVec2 toScreen(const CanvasTransform& transform, double x, double z) {
  return creativeEditorWorldLayoutCanvasToScreen(transform, x, z);
}

ImU32 color(ImVec4 value) { return ImGui::ColorConvertFloat4ToU32(value); }

void drawOpeningPlacementPlan(
    ImDrawList& drawList, const CanvasTransform& transform,
    CreativeEditorWorldLayoutPoint hovered,
    const CreativeEditorWorldLayoutOpeningPlacementPlan& plan,
    cr::CreativeBuildingOpeningKind kind, std::string_view label) {
  const ImU32 previewColor =
      !plan.accepted
          ? color({0.92F, 0.29F, 0.24F, 1.0F})
          : kind == cr::CreativeBuildingOpeningKind::Door
                ? color({0.20F, 0.82F, 0.38F, 1.0F})
                : color({0.27F, 0.72F, 0.91F, 1.0F});
  if (!plan.accepted) {
    const ImVec2 center = toScreen(transform, hovered.x, hovered.z);
    drawList.AddLine({center.x - 7.0F, center.y - 7.0F},
                     {center.x + 7.0F, center.y + 7.0F}, previewColor, 3.0F);
    drawList.AddLine({center.x - 7.0F, center.y + 7.0F},
                     {center.x + 7.0F, center.y - 7.0F}, previewColor, 3.0F);
    drawList.AddText({center.x + 11.0F, center.y + 9.0F}, previewColor,
                     plan.message.data());
    return;
  }

  const ImVec2 start =
      toScreen(transform, plan.startPoint.x, plan.startPoint.z);
  const ImVec2 center =
      toScreen(transform, plan.centerPoint.x, plan.centerPoint.z);
  const ImVec2 end = toScreen(transform, plan.endPoint.x, plan.endPoint.z);
  if (plan.pointerDistanceCells > 0.01) {
    const ImVec2 pointer = toScreen(transform, hovered.x, hovered.z);
    drawList.AddLine(pointer, center, previewColor, 1.0F);
  }
  drawList.AddLine(start, end, previewColor, 8.0F);
  drawList.AddRectFilled({start.x - 3.5F, start.y - 3.5F},
                         {start.x + 3.5F, start.y + 3.5F}, previewColor);
  drawList.AddRectFilled({end.x - 3.5F, end.y - 3.5F},
                         {end.x + 3.5F, end.y + 3.5F}, previewColor);
  if (kind == cr::CreativeBuildingOpeningKind::Door) {
    drawList.AddCircleFilled(center, 6.0F, previewColor);
  } else {
    drawList.AddRectFilled({center.x - 6.0F, center.y - 6.0F},
                           {center.x + 6.0F, center.y + 6.0F}, previewColor);
  }
  char placementLabel[96]{};
  const std::size_t visibleLabelSize =
      std::min(label.size(), std::size_t{48U});
  std::snprintf(placementLabel, sizeof(placementLabel),
                "%.*s | %.2f wide x %.2f high",
                static_cast<int>(visibleLabelSize), label.data(),
                plan.opening.widthCells, plan.opening.cutoutHeightCells);
  drawList.AddText({center.x + 9.0F, center.y + 9.0F}, previewColor,
                   placementLabel);
}

void drawOpeningPlacementPreview(
    ImDrawList& drawList, const CanvasTransform& transform,
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint hovered) {
  cr::CreativeBuildingOpeningKind kind;
  if (state.tool == CreativeEditorWorldLayoutTool::Door) {
    kind = cr::CreativeBuildingOpeningKind::Door;
  } else if (state.tool == CreativeEditorWorldLayoutTool::Window) {
    kind = cr::CreativeBuildingOpeningKind::Window;
  } else {
    return;
  }
  const CreativeEditorWorldLayoutOpeningPlacementPlan plan =
      planCreativeEditorWorldLayoutOpeningPlacement(state, hovered, kind);
  drawOpeningPlacementPlan(
      drawList, transform, hovered, plan, kind,
      kind == cr::CreativeBuildingOpeningKind::Door ? "Door" : "Window");
}

void drawAnchorPreview(ImDrawList& drawList, const CanvasTransform& transform,
                       const CreativeEditorWorldLayoutState& state,
                       CreativeEditorWorldLayoutPoint hovered) {
  if (!state.anchorActive) {
    return;
  }
  const double snappedX = std::round(hovered.x);
  const double snappedZ = std::round(hovered.z);
  const ImU32 previewColor = color({0.96F, 0.82F, 0.22F, 0.95F});
  const ImVec2 start = toScreen(transform, state.anchor.x, state.anchor.z);
  if (state.tool == CreativeEditorWorldLayoutTool::BuildingShell ||
      state.tool == CreativeEditorWorldLayoutTool::Room ||
      state.tool == CreativeEditorWorldLayoutTool::Floor ||
      creativeEditorWorldLayoutToolIsVerticalConnector(state.tool) ||
      state.tool == CreativeEditorWorldLayoutTool::Bridge) {
    const ImVec2 end = toScreen(transform, snappedX, snappedZ);
    if (state.tool == CreativeEditorWorldLayoutTool::BuildingShell ||
        state.tool == CreativeEditorWorldLayoutTool::Room ||
        creativeEditorWorldLayoutToolIsVerticalConnector(state.tool) ||
        state.tool == CreativeEditorWorldLayoutTool::Bridge) {
      drawList.AddRectFilled(
          {std::min(start.x, end.x), std::min(start.y, end.y)},
          {std::max(start.x, end.x), std::max(start.y, end.y)},
          state.tool == CreativeEditorWorldLayoutTool::Ramp
              ? color({0.72F, 0.38F, 0.12F, 0.28F})
              : creativeEditorWorldLayoutToolIsVerticalConnector(state.tool)
                    ? color({0.18F, 0.55F, 0.72F, 0.28F})
                    : color({0.22F, 0.58F, 0.38F, 0.22F}));
    }
    drawList.AddRect({std::min(start.x, end.x), std::min(start.y, end.y)},
                     {std::max(start.x, end.x), std::max(start.y, end.y)},
                     previewColor, 0.0F, 0, 2.0F);
    const int width = static_cast<int>(std::fabs(snappedX - state.anchor.x));
    const int depth = static_cast<int>(std::fabs(snappedZ - state.anchor.z));
    const std::string dimensions =
        std::to_string(width) + " x " + std::to_string(depth);
    drawList.AddText({std::min(start.x, end.x) + 6.0F,
                      std::min(start.y, end.y) + 6.0F},
                     previewColor, dimensions.c_str());
  } else if (state.tool == CreativeEditorWorldLayoutTool::Wall) {
    const double deltaX = std::fabs(snappedX - state.anchor.x);
    const double deltaZ = std::fabs(snappedZ - state.anchor.z);
    const ImVec2 end = deltaX >= deltaZ
                           ? toScreen(transform, snappedX, state.anchor.z)
                           : toScreen(transform, state.anchor.x, snappedZ);
    drawList.AddLine(start, end, previewColor, 4.0F);
  } else if (state.tool == CreativeEditorWorldLayoutTool::Road ||
             state.tool == CreativeEditorWorldLayoutTool::Ditch) {
    const ImVec2 end = toScreen(transform, snappedX, snappedZ);
    const float width = state.tool == CreativeEditorWorldLayoutTool::Ditch
                            ? transform.pixelsPerCell * 3.0F
                            : transform.pixelsPerCell * 3.0F;
    drawList.AddLine(start, end,
                     state.tool == CreativeEditorWorldLayoutTool::Ditch
                         ? color({0.25F, 0.47F, 0.68F, 0.32F})
                         : color({0.72F, 0.56F, 0.28F, 0.32F}),
                     width);
    drawList.AddLine(start, end, previewColor, 2.0F);
  }
}

void drawCatalogPlacementPreview(
    ImDrawList& drawList, const CanvasTransform& transform,
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint hovered,
    cr::CreativeGridSettings grid) {
  if (state.tool != CreativeEditorWorldLayoutTool::CatalogAsset ||
      !state.catalogPlacement.active) {
    return;
  }
  const CreativeEditorWorldLayoutCatalogPlacementPlan plan =
      planCreativeEditorWorldLayoutCatalogPlacement(state, hovered, grid);
  if (plan.hostedOpening) {
    drawOpeningPlacementPlan(
        drawList, transform, hovered, plan.openingPlacement,
        state.catalogPlacement.categoryId == "window"
            ? cr::CreativeBuildingOpeningKind::Window
            : cr::CreativeBuildingOpeningKind::Door,
        state.catalogPlacement.label);
    return;
  }
  const ImU32 outline = plan.accepted
                            ? color({0.20F, 0.82F, 0.38F, 1.0F})
                            : color({0.92F, 0.29F, 0.24F, 1.0F});
  if (!plan.accepted || !plan.footprint.valid) {
    const ImVec2 center = toScreen(transform, hovered.x, hovered.z);
    drawList.AddLine({center.x - 8.0F, center.y - 8.0F},
                     {center.x + 8.0F, center.y + 8.0F}, outline, 3.0F);
    drawList.AddLine({center.x - 8.0F, center.y + 8.0F},
                     {center.x + 8.0F, center.y - 8.0F}, outline, 3.0F);
    drawList.AddText({center.x + 12.0F, center.y + 10.0F}, outline,
                     plan.message.c_str());
    return;
  }
  std::array<ImVec2, 4U> points;
  for (std::size_t index = 0U; index < points.size(); ++index) {
    points[index] = toScreen(transform, plan.footprint.corners[index].x,
                             plan.footprint.corners[index].z);
  }
  drawList.AddConvexPolyFilled(points.data(), static_cast<int>(points.size()),
                               color({0.20F, 0.82F, 0.38F, 0.22F}));
  drawList.AddPolyline(points.data(), static_cast<int>(points.size()), outline,
                       ImDrawFlags_Closed, 2.5F);
  const ImVec2 pivot = toScreen(transform, plan.object.pointCells.x,
                                plan.object.pointCells.z);
  if (plan.snapMode == CreativeEditorWorldLayoutCatalogSnapMode::Wall) {
    const CreativeEditorWorldLayoutPoint centerline{
        plan.snapSurfacePoint.x -
            plan.snapNormal.x * plan.snapWallThicknessCells * 0.5,
        plan.snapSurfacePoint.z -
            plan.snapNormal.z * plan.snapWallThicknessCells * 0.5};
    const CreativeEditorWorldLayoutPoint tangent{-plan.snapNormal.z,
                                                  plan.snapNormal.x};
    const CreativeEditorWorldLayoutPoint faceStart{
        plan.snapSurfacePoint.x - tangent.x * 0.45,
        plan.snapSurfacePoint.z - tangent.z * 0.45};
    const CreativeEditorWorldLayoutPoint faceEnd{
        plan.snapSurfacePoint.x + tangent.x * 0.45,
        plan.snapSurfacePoint.z + tangent.z * 0.45};
    const CreativeEditorWorldLayoutPoint normalTip{
        plan.snapSurfacePoint.x + plan.snapNormal.x * 0.65,
        plan.snapSurfacePoint.z + plan.snapNormal.z * 0.65};
    const ImVec2 centerlineScreen =
        toScreen(transform, centerline.x, centerline.z);
    const ImVec2 surfaceScreen = toScreen(
        transform, plan.snapSurfacePoint.x, plan.snapSurfacePoint.z);
    const ImVec2 normalTipScreen =
        toScreen(transform, normalTip.x, normalTip.z);
    const ImU32 faceColor = color({0.98F, 0.78F, 0.20F, 1.0F});
    if (plan.snapDistanceCells > 0.01) {
      const ImVec2 pointer = toScreen(transform, hovered.x, hovered.z);
      drawList.AddLine(pointer, centerlineScreen, outline, 1.0F);
    }
    drawList.AddLine(centerlineScreen, surfaceScreen, faceColor, 2.0F);
    drawList.AddLine(toScreen(transform, faceStart.x, faceStart.z),
                     toScreen(transform, faceEnd.x, faceEnd.z), faceColor,
                     3.0F);
    drawList.AddCircleFilled(surfaceScreen, 3.5F, faceColor);
    drawList.AddLine(surfaceScreen, normalTipScreen, outline, 2.0F);
    const float arrowDx = normalTipScreen.x - surfaceScreen.x;
    const float arrowDy = normalTipScreen.y - surfaceScreen.y;
    const float arrowLength = std::hypot(arrowDx, arrowDy);
    if (arrowLength > 0.0F) {
      const float unitX = arrowDx / arrowLength;
      const float unitY = arrowDy / arrowLength;
      const ImVec2 arrowBase{normalTipScreen.x - unitX * 7.0F,
                             normalTipScreen.y - unitY * 7.0F};
      const ImVec2 arrowSide{-unitY * 3.5F, unitX * 3.5F};
      drawList.AddTriangleFilled(
          normalTipScreen,
          {arrowBase.x + arrowSide.x, arrowBase.y + arrowSide.y},
          {arrowBase.x - arrowSide.x, arrowBase.y - arrowSide.y}, outline);
    }
  }
  drawList.AddCircleFilled(pivot, 3.5F, outline);
  const std::string previewLabel =
      state.catalogPlacement.label + " | " + plan.message;
  drawList.AddText({points.front().x + 6.0F, points.front().y + 6.0F}, outline,
                   previewLabel.c_str());
}



}  // namespace

void drawCreativeEditorWorldLayoutPlacementPreviews(
    ImDrawList& drawList,
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint hoveredPoint,
    const cr::CreativeGridSettings& grid) {
  drawOpeningPlacementPreview(drawList, transform, state, hoveredPoint);
  drawCatalogPlacementPreview(drawList, transform, state, hoveredPoint, grid);
  drawAnchorPreview(drawList, transform, state, hoveredPoint);
}

}  // namespace iggy3d_creative_app
