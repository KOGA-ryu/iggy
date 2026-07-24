#pragma once

#include "EditorWorldLayoutCanvasPlanner.hpp"
#include "EditorWorldLayoutPanelInternal.hpp"
#include "EditorWorldLayoutPlanView.hpp"

#include "imgui.h"

namespace iggy3d_creative_app {

inline ImVec2 creativeEditorWorldLayoutCanvasToScreen(
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    double x, double z) noexcept {
  const CreativeEditorWorldLayoutCanvasScreenPoint point =
      planCreativeEditorWorldLayoutCanvasScreenPoint(transform, x, z);
  return {point.x, point.y};
}

inline CreativeEditorWorldLayoutPoint creativeEditorWorldLayoutCanvasToWorld(
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    ImVec2 screen) noexcept {
  return planCreativeEditorWorldLayoutCanvasWorldPoint(
      transform, {screen.x, screen.y});
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

void drawCreativeEditorWorldLayoutTerrainAnnotations(
    ImDrawList& drawList,
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    const CreativeEditorWorldLayoutTopographyState& topography,
    const cr::CreativeGridSettings& grid);

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
    const cr::CreativeGridSettings& grid,
    const cr::CreativeMeasurementAnnotationStore& measurementAnnotations,
    const cr::CreativeMeasurementState& measurement,
    ImVec2 pointerPosition, bool hovered);

}  // namespace iggy3d_creative_app
