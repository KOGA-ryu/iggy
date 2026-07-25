#include "EditorWorldLayoutElevationPanel.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>

#include "imgui.h"
#include "EditorWorldLayoutBuildings.hpp"
#include "EditorWorldLayoutElevationDrawInternal.hpp"
#include "EditorWorldLayoutElevationPlanner.hpp"
#include "EditorWorldLayoutLifecycle.hpp"
#include "EditorWorldLayoutOpenings.hpp"
#include "EditorWorldLayoutPlan.hpp"
#include "EditorWorldLayoutSources.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOpenings.hpp"

namespace iggy3d_creative_app {
namespace {

using ElevationCanvasTransform =
    CreativeEditorWorldLayoutElevationCanvasTransform;

ImVec2 toElevationScreen(
    const ElevationCanvasTransform& transform,
    CreativeEditorWorldLayoutElevationPoint point) {
  const CreativeEditorWorldLayoutElevationScreenPoint screen =
      planCreativeEditorWorldLayoutElevationScreenPoint(transform, point);
  return {screen.x, screen.y};
}

CreativeEditorWorldLayoutElevationPoint toElevationWorld(
    const ElevationCanvasTransform& transform, ImVec2 screen) {
  return planCreativeEditorWorldLayoutElevationWorldPoint(
      transform, {screen.x, screen.y});
}

std::size_t elevationBuildingIndex(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeWorldLayout& source) noexcept {
  std::size_t buildingIndex = creativeEditorWorldLayoutSelectedBuilding(state);
  if (buildingIndex < source.buildings.size()) {
    return buildingIndex;
  }
  if (state.activeLevelIndex < source.levels.size()) {
    buildingIndex = source.levels[state.activeLevelIndex].buildingIndex;
    if (buildingIndex < source.buildings.size()) {
      return buildingIndex;
    }
  }
  for (const cr::CreativeWorldLayoutRoom& room : source.rooms) {
    if (room.buildingIndex < source.buildings.size()) {
      return room.buildingIndex;
    }
  }
  return cr::kInvalidCreativeWorldLayoutIndex;
}

bool elevationGridMatches(const CreativeEditorWorldLayoutElevationCache& cache,
                          const cr::CreativeGridSettings& grid) noexcept {
  return cache.gridOrigin.x == grid.origin.x &&
         cache.gridOrigin.y == grid.origin.y &&
         cache.gridOrigin.z == grid.origin.z &&
         cache.gridCellSizeMeters == grid.cellSizeMeters;
}

const CreativeEditorWorldLayoutElevationProjection& elevationProjection(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeGridSettings& grid, std::size_t buildingIndex,
    const CreativeEditorWorldLayoutInspection& inspection) {
  const bool previewSource =
      inspection.sourceKind ==
      CreativeEditorWorldLayoutInspectionSourceKind::Preview;
  if (!state.elevationCache.valid || inspection.volatileSource ||
      state.elevationCache.sourceRevision != state.revision ||
      state.elevationCache.inspectionContentRevision !=
          inspection.contentRevision ||
      state.elevationCache.inspectionPreviewSource != previewSource ||
      state.elevationCache.buildingIndex != buildingIndex ||
      state.elevationCache.axis != state.elevationAxis ||
      !elevationGridMatches(state.elevationCache, grid)) {
    state.elevationCache.valid = true;
    state.elevationCache.sourceRevision = state.revision;
    state.elevationCache.inspectionContentRevision =
        inspection.contentRevision;
    state.elevationCache.inspectionPreviewSource = previewSource;
    state.elevationCache.buildingIndex = buildingIndex;
    state.elevationCache.axis = state.elevationAxis;
    state.elevationCache.gridOrigin = grid.origin;
    state.elevationCache.gridCellSizeMeters = grid.cellSizeMeters;
    state.elevationCache.projection = planCreativeEditorWorldLayoutElevation(
        {inspection.source, grid, buildingIndex, state.elevationAxis});
  }
  return state.elevationCache.projection;
}

bool buildElevationDatumPreview(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeGridSettings& grid,
    std::size_t buildingIndex,
    CreativeEditorWorldLayoutElevationProjection& output) {
  const CreativeEditorWorldLayoutElevationManipulationState& manipulation =
      state.elevationManipulation;
  if (!manipulation.active || !manipulation.preview.accepted ||
      manipulation.handle.kind !=
          CreativeEditorWorldLayoutElevationHandleKind::LevelFloor ||
      manipulation.handle.sourceKind !=
          CreativeEditorWorldLayoutElevationSourceKind::Room ||
      !manipulation.preview.levelDatumPlan.changed) {
    return false;
  }
  cr::CreativeWorldLayout candidate = state.source;
  if (!cr::applyCreativeWorldLayoutLevelDatumEditPlan(
          candidate, manipulation.preview.levelDatumPlan) ||
      !cr::validCreativeWorldLayoutOpenings(candidate)) {
    return false;
  }
  output = planCreativeEditorWorldLayoutElevation(
      {&candidate, grid, buildingIndex, state.elevationAxis});
  return output.accepted;
}

void drawElevationCanvas(
    CreativeEditorState& editor, const cr::CreativeGridSettings& grid,
    const cr::CreativeMeasurementAnnotationStore& measurementAnnotations,
    const cr::CreativeMeasurementState& measurement,
    CreativeDesktopCommandFrame& commands, bool interactionEnabled,
    const CreativeEditorUiInputFrame& input) {
  CreativeEditorWorldLayoutState& state = editor.worldLayout;
  const CreativeEditorWorldLayoutInspection inspection =
      inspectCreativeEditorWorldLayout(state);
  const std::size_t buildingIndex =
      elevationBuildingIndex(state, *inspection.source);
  const CreativeEditorWorldLayoutElevationProjection& sourceProjection =
      elevationProjection(state, grid, buildingIndex, inspection);
  CreativeEditorWorldLayoutElevationProjection datumPreview;
  const bool datumPreviewReady =
      buildElevationDatumPreview(state, grid, buildingIndex, datumPreview);
  const CreativeEditorWorldLayoutElevationProjection& projection =
      datumPreviewReady ? datumPreview : sourceProjection;
  drawCreativeEditorWorldLayoutElevationSectionControls(state,
                                                        interactionEnabled);

  const ImVec2 available = ImGui::GetContentRegionAvail();
  const ImVec2 canvasSize{std::max(available.x, 160.0F),
                          std::max(available.y, 160.0F)};
  const ImVec2 minimum = ImGui::GetCursorScreenPos();
  const ImVec2 maximum{minimum.x + canvasSize.x, minimum.y + canvasSize.y};
  ImGui::InvisibleButton(
      "##world_layout_elevation_canvas", canvasSize,
      ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle |
          ImGuiButtonFlags_MouseButtonRight);
  const bool hovered = ImGui::IsItemHovered();
  const ImVec2 pointerPosition{input.pointer.x, input.pointer.y};

  const double centerHorizontal =
      sourceProjection.bounds.valid
          ? (sourceProjection.bounds.minimumHorizontal +
             sourceProjection.bounds.maximumHorizontal) *
                0.5
          : 0.0;
  const double centerVertical =
      sourceProjection.bounds.valid
          ? (sourceProjection.bounds.minimumVertical +
             sourceProjection.bounds.maximumVertical) *
                0.5
          : 0.0;
  const auto makeTransform = [&]() {
    return ElevationCanvasTransform{
        {minimum.x + canvasSize.x * 0.5F + state.elevationPanHorizontal -
             static_cast<float>(centerHorizontal) *
                 state.elevationPixelsPerCell,
         minimum.y + canvasSize.y * 0.5F + state.elevationPanY +
             static_cast<float>(centerVertical) *
                 state.elevationPixelsPerCell},
        state.elevationPixelsPerCell};
  };
  ElevationCanvasTransform transform = makeTransform();
  if (hovered && input.pointer.wheelY != 0.0F) {
    const CreativeEditorWorldLayoutElevationPoint before =
        toElevationWorld(transform, pointerPosition);
    state.elevationPixelsPerCell = std::clamp(
        state.elevationPixelsPerCell *
            (input.pointer.wheelY > 0.0F ? 1.15F : 0.87F),
        12.0F, 80.0F);
    transform = makeTransform();
    const ImVec2 anchored = toElevationScreen(transform, before);
    state.elevationPanHorizontal += pointerPosition.x - anchored.x;
    state.elevationPanY += pointerPosition.y - anchored.y;
    transform = makeTransform();
  }
  if (hovered && input.pointer.middleDragging) {
    state.elevationPanHorizontal += input.pointer.deltaX;
    state.elevationPanY += input.pointer.deltaY;
    transform = makeTransform();
  }

  drawCreativeEditorWorldLayoutElevationProjection(
      *ImGui::GetWindowDrawList(), minimum, maximum, transform, projection,
      state, grid, measurementAnnotations, measurement);

  const CreativeEditorWorldLayoutElevationPoint pointer =
      toElevationWorld(transform, pointerPosition);
  const double handleTolerance = std::clamp(
      8.0 / static_cast<double>(transform.pixelsPerCell), 0.10, 0.45);
  const CreativeEditorWorldLayoutElevationInteractionPlan plan =
      planCreativeEditorWorldLayoutElevationInteraction(
          {&state,
           &projection,
           interactionEnabled,
           {hovered,
            input.pointer.primaryPressed,
            input.pointer.primaryReleased,
            input.pointer.primaryDown,
            input.pointer.secondaryPressed,
            input.pointer.focusLost,
            input.cancelPressed,
            pointer,
            handleTolerance}});
  if (plan.cursor ==
      CreativeEditorWorldLayoutElevationCursor::ResizeHorizontal) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
  } else if (plan.cursor ==
             CreativeEditorWorldLayoutElevationCursor::ResizeVertical) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
  }
  if (plan.selectionChanged) {
    state.selection = plan.selection;
    if (plan.activeLevelIndex < state.source.levels.size()) {
      state.activeLevelIndex = plan.activeLevelIndex;
    }
  }
  if (plan.replaceElevationManipulation) {
    state.elevationManipulation = plan.elevationManipulation;
  }
  const bool allEnqueued =
      enqueueCreativeEditorWorldLayoutElevationCommandPlan(plan.commands,
                                                           commands);
  if (!allEnqueued) {
    assert(commands.overflowed);
  }
}

}  // namespace

void drawCreativeEditorWorldLayoutElevationCanvas(
    CreativeEditorState& editor, const cr::CreativeGridSettings& grid,
    const cr::CreativeMeasurementAnnotationStore& measurementAnnotations,
    const cr::CreativeMeasurementState& measurement,
    CreativeDesktopCommandFrame& commands, bool interactionEnabled,
    const CreativeEditorUiInputFrame& input) {
  drawElevationCanvas(editor, grid, measurementAnnotations, measurement,
                      commands, interactionEnabled, input);
}

}  // namespace iggy3d_creative_app
