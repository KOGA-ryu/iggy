#pragma once

#include "EditorWorldLayoutPanelInternal.hpp"
#include "EditorWorldLayoutPlanView.hpp"

#include "imgui.h"

namespace iggy3d_creative_app {

struct CreativeEditorWorldLayoutCanvasTransform {
  ImVec2 origin;
  float pixelsPerCell = 28.0F;
};

inline ImVec2 creativeEditorWorldLayoutCanvasToScreen(
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    double x, double z) noexcept {
  return {transform.origin.x + static_cast<float>(x) * transform.pixelsPerCell,
          transform.origin.y + static_cast<float>(z) * transform.pixelsPerCell};
}

inline CreativeEditorWorldLayoutPoint creativeEditorWorldLayoutCanvasToWorld(
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    ImVec2 screen) noexcept {
  return {(screen.x - transform.origin.x) / transform.pixelsPerCell,
          (screen.y - transform.origin.y) / transform.pixelsPerCell};
}

struct CreativeEditorWorldLayoutCanvasPointerGeometry {
  CreativeEditorWorldLayoutPoint pointerPoint;
  CreativeEditorWorldLayoutPoint hoveredPoint;
  double handleToleranceCells = 0.25;
};

void drawCreativeEditorWorldLayoutTerrainBackground(
    ImDrawList& drawList, ImVec2 minimum, ImVec2 maximum,
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    const CreativeEditorWorldLayoutTopographyState& topography);

void drawCreativeEditorWorldLayoutObjectSymbols(
    ImDrawList& drawList,
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeGridSettings& grid);

void drawCreativeEditorWorldLayoutTopographyHoverFacts(
    const CreativeEditorWorldLayoutTopographyState& topography,
    CreativeEditorWorldLayoutPoint point, cr::CreativeGridSettings grid);

void drawCreativeEditorWorldLayoutPlacementPreviews(
    ImDrawList& drawList,
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint hoveredPoint,
    const cr::CreativeGridSettings& grid);

CreativeEditorWorldLayoutCanvasPointerGeometry
drawCreativeEditorWorldLayoutCanvasScene(
    ImDrawList& drawList, ImVec2 minimum, ImVec2 maximum,
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutTopographyState& topography,
    const CreativeEditorWorldLayoutPlanViewCache& planView,
    std::size_t hoveredPlanPrimitiveIndex,
    const cr::CreativeGridSettings& grid, ImVec2 pointerPosition,
    bool hovered);

}  // namespace iggy3d_creative_app
