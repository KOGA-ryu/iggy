#include "EditorWorldLayoutElevationPanel.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>

#include "imgui.h"
#include "EditorDraftingStyle.hpp"
#include "EditorMeasurement.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutRoofs.hpp"
#include "EditorWorldLayoutVerticalConnectorHandles.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOpenings.hpp"

namespace iggy3d_creative_app {
namespace {

ImU32 color(ImVec4 value) { return ImGui::ColorConvertFloat4ToU32(value); }

ImU32 draftingColor(CreativeEditorDraftingColor value) {
  return IM_COL32(value.r, value.g, value.b, value.a);
}

bool selected(const CreativeEditorWorldLayoutState& state,
              CreativeEditorWorldLayoutSelectionKind kind,
              std::size_t index) {
  return state.selection.kind == kind && state.selection.index == index;
}

struct ElevationCanvasTransform {
  ImVec2 origin;
  float pixelsPerCell = 28.0F;
};

ImVec2 toElevationScreen(
    const ElevationCanvasTransform& transform,
    CreativeEditorWorldLayoutElevationPoint point) {
  return {transform.origin.x +
              static_cast<float>(point.horizontal) * transform.pixelsPerCell,
          transform.origin.y -
              static_cast<float>(point.vertical) * transform.pixelsPerCell};
}

CreativeEditorWorldLayoutElevationPoint toElevationWorld(
    const ElevationCanvasTransform& transform, ImVec2 screen) {
  return {(screen.x - transform.origin.x) / transform.pixelsPerCell,
          (transform.origin.y - screen.y) / transform.pixelsPerCell};
}

CreativeEditorWorldLayoutElevationPoint measurementElevationPoint(
    CreativeEditorWorldLayoutElevationAxis axis,
    cr::CreativeMeasurementPoint point) noexcept {
  return {axis == CreativeEditorWorldLayoutElevationAxis::X ? point.x : point.z,
          point.y};
}

void drawMeasurementGeometry(
    ImDrawList& drawList, const ElevationCanvasTransform& transform,
    CreativeEditorWorldLayoutElevationAxis axis,
    const cr::CreativeMeasurementGeometry& geometry,
    std::string_view label, bool transient) {
  if (!geometry.visible) {
    return;
  }
  const CreativeEditorDraftingStyle& style = creativeEditorDraftingStyle(
      CreativeEditorDraftingRole::MeasurementOverlay);
  CreativeEditorDraftingColor tintValue = style.tint;
  if (!transient) {
    tintValue.a = static_cast<std::uint8_t>(
        static_cast<float>(tintValue.a) * 0.72F);
  }
  const ImU32 tint = draftingColor(tintValue);
  const float thickness = creativeEditorDraftingStrokeThicknessPixels(
      style, transform.pixelsPerCell);
  for (std::size_t index = 0U; index < geometry.segmentCount; ++index) {
    const cr::CreativeMeasurementSegment& segment = geometry.segments[index];
    drawList.AddLine(
        toElevationScreen(transform,
                          measurementElevationPoint(axis, segment.start)),
        toElevationScreen(transform,
                          measurementElevationPoint(axis, segment.end)),
        tint, thickness);
  }
  for (std::size_t index = 0U; index < geometry.pointCount; ++index) {
    drawList.AddCircleFilled(
        toElevationScreen(
            transform, measurementElevationPoint(axis, geometry.points[index])),
        index + 1U == geometry.pointCount ? 4.5F : 3.5F, tint);
  }
  if (!label.empty() && geometry.pointCount > 0U) {
    const ImVec2 anchor = toElevationScreen(
        transform,
        measurementElevationPoint(axis,
                                  geometry.points[geometry.pointCount - 1U]));
    drawList.AddText({anchor.x + 8.0F, anchor.y + 8.0F}, tint, label.data(),
                     label.data() + label.size());
  }
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

constexpr std::array<CreativeEditorWorldLayoutLevelEditScope, 4U>
    kLevelEditScopes = {
        CreativeEditorWorldLayoutLevelEditScope::Selected,
        CreativeEditorWorldLayoutLevelEditScope::SelectedAndAbove,
        CreativeEditorWorldLayoutLevelEditScope::SelectedAndBelow,
        CreativeEditorWorldLayoutLevelEditScope::All,
};

void drawElevationSectionControls(
    CreativeEditorWorldLayoutState& state, bool interactionEnabled) {
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted("Level edit");
  ImGui::SameLine();
  ImGui::BeginDisabled(!interactionEnabled ||
                       state.elevationManipulation.active);
  ImGui::PushID("section_level_edit_scope");
  for (std::size_t index = 0U; index < kLevelEditScopes.size(); ++index) {
    if (index > 0U) {
      ImGui::SameLine();
    }
    const CreativeEditorWorldLayoutLevelEditScope scope =
        kLevelEditScopes[index];
    const std::string_view label =
        creativeEditorWorldLayoutLevelEditScopeLabel(scope);
    if (ImGui::RadioButton(label.data(),
                           state.elevationLevelEditScope == scope)) {
      state.elevationLevelEditScope = scope;
    }
  }
  ImGui::PopID();
  ImGui::EndDisabled();
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

void drawElevationSectionAnnotations(
    ImDrawList& drawList, ImVec2 minimum, ImVec2 maximum,
    const ElevationCanvasTransform& transform,
    const CreativeEditorWorldLayoutElevationProjection& projection,
    const CreativeEditorWorldLayoutState& state) {
  for (const CreativeEditorWorldLayoutSectionLevel& level :
       projection.sectionLevels) {
    const float datumY =
        toElevationScreen(transform, {0.0, level.floorDatumCells}).y;
    if (datumY < minimum.y || datumY > maximum.y) {
      continue;
    }
    const bool active = state.activeLevelIndex == level.levelIndex;
    const ImU32 datumColor =
        active ? color({0.34F, 0.92F, 0.46F, 0.95F})
               : color({0.55F, 0.66F, 0.74F, 0.72F});
    drawList.AddLine({minimum.x + 4.0F, datumY},
                     {maximum.x - 4.0F, datumY}, datumColor,
                     active ? 1.8F : 1.0F);

    char label[256];
    const std::string_view extent =
        creativeEditorWorldLayoutSectionPartitionExtentLabel(
            level.partitionExtent);
    if (!level.occupied) {
      std::snprintf(label, sizeof(label), "%s  %+.2f m  |  %.*s",
                    level.name.c_str(), level.floorDatumMeters,
                    static_cast<int>(extent.size()), extent.data());
    } else if (level.floorToFloorMeters > 0.0) {
      std::snprintf(label, sizeof(label),
                    "%s  %+.2f m  |  clear %.2f m  |  F2F %.2f m  |  %.*s",
                    level.name.c_str(), level.floorDatumMeters,
                    level.clearHeightMeters, level.floorToFloorMeters,
                    static_cast<int>(extent.size()), extent.data());
    } else {
      std::snprintf(label, sizeof(label),
                    "%s  %+.2f m  |  clear %.2f m  |  %.*s",
                    level.name.c_str(), level.floorDatumMeters,
                    level.clearHeightMeters,
                    static_cast<int>(extent.size()), extent.data());
    }
    const ImVec2 textSize = ImGui::CalcTextSize(label);
    const ImVec2 textMinimum{minimum.x + 8.0F, datumY - textSize.y - 3.0F};
    const ImVec2 textMaximum{textMinimum.x + textSize.x + 8.0F,
                             textMinimum.y + textSize.y + 4.0F};
    drawList.AddRectFilled(textMinimum, textMaximum,
                           color({0.07F, 0.08F, 0.09F, 0.88F}), 2.0F);
    drawList.AddText({textMinimum.x + 4.0F, textMinimum.y + 2.0F},
                     datumColor, label);

    if (!active || !level.occupied) {
      continue;
    }
    const float partitionY =
        toElevationScreen(transform, {0.0, level.partitionTopCells}).y;
    if (partitionY >= minimum.y && partitionY <= maximum.y) {
      drawList.AddLine({minimum.x + 8.0F, partitionY},
                       {minimum.x + 116.0F, partitionY},
                       color({0.96F, 0.72F, 0.28F, 0.86F}), 1.2F);
      drawList.AddText({minimum.x + 122.0F, partitionY - 7.0F},
                       color({0.96F, 0.72F, 0.28F, 0.92F}),
                       "partition top");
    }
  }

  if (state.elevationManipulation.active &&
      !state.elevationManipulation.preview.accepted) {
    drawList.AddText(
        {minimum.x + 10.0F, maximum.y - 24.0F},
        color({0.96F, 0.28F, 0.24F, 1.0F}),
        "Invalid level position: release is blocked");
  }
}

void drawElevationGrid(
    ImDrawList& drawList, ImVec2 minimum, ImVec2 maximum,
    const ElevationCanvasTransform& transform) {
  const CreativeEditorWorldLayoutElevationPoint lowerLeft =
      toElevationWorld(transform, {minimum.x, maximum.y});
  const CreativeEditorWorldLayoutElevationPoint upperRight =
      toElevationWorld(transform, {maximum.x, minimum.y});
  const int firstHorizontal =
      static_cast<int>(std::floor(lowerLeft.horizontal));
  const int lastHorizontal =
      static_cast<int>(std::ceil(upperRight.horizontal));
  const int firstVertical =
      static_cast<int>(std::floor(lowerLeft.vertical));
  const int lastVertical = static_cast<int>(std::ceil(upperRight.vertical));
  for (int horizontal = firstHorizontal; horizontal <= lastHorizontal;
       ++horizontal) {
    const float screen =
        toElevationScreen(transform,
                          {static_cast<double>(horizontal), 0.0})
            .x;
    const bool major = horizontal % 5 == 0;
    drawList.AddLine({screen, minimum.y}, {screen, maximum.y},
                     major ? color({0.30F, 0.34F, 0.38F, 1.0F})
                           : color({0.20F, 0.23F, 0.26F, 1.0F}),
                     major ? 1.4F : 1.0F);
  }
  for (int vertical = firstVertical; vertical <= lastVertical; ++vertical) {
    const float screen =
        toElevationScreen(transform, {0.0, static_cast<double>(vertical)})
            .y;
    const bool major = vertical % 5 == 0;
    drawList.AddLine({minimum.x, screen}, {maximum.x, screen},
                     major ? color({0.30F, 0.34F, 0.38F, 1.0F})
                           : color({0.20F, 0.23F, 0.26F, 1.0F}),
                     major ? 1.4F : 1.0F);
  }
  const float zero = toElevationScreen(transform, {0.0, 0.0}).y;
  drawList.AddLine({minimum.x, zero}, {maximum.x, zero},
                   color({0.86F, 0.42F, 0.36F, 1.0F}), 1.8F);
}

ImU32 elevationItemColor(
    CreativeEditorWorldLayoutElevationItemKind kind) {
  switch (kind) {
    case CreativeEditorWorldLayoutElevationItemKind::FloorSlab:
      return color({0.38F, 0.45F, 0.52F, 0.90F});
    case CreativeEditorWorldLayoutElevationItemKind::WallEnvelope:
      return color({0.46F, 0.49F, 0.52F, 0.54F});
    case CreativeEditorWorldLayoutElevationItemKind::CeilingSlab:
      return color({0.53F, 0.58F, 0.63F, 0.88F});
    case CreativeEditorWorldLayoutElevationItemKind::RoofBase:
      return color({0.44F, 0.28F, 0.22F, 0.92F});
    case CreativeEditorWorldLayoutElevationItemKind::RoofSkylight:
      return color({0.20F, 0.65F, 0.82F, 0.92F});
    case CreativeEditorWorldLayoutElevationItemKind::RoofClearance:
      return color({0.78F, 0.54F, 0.22F, 0.82F});
    case CreativeEditorWorldLayoutElevationItemKind::Door:
      return color({0.72F, 0.45F, 0.20F, 0.96F});
    case CreativeEditorWorldLayoutElevationItemKind::Window:
      return color({0.20F, 0.65F, 0.82F, 0.88F});
    case CreativeEditorWorldLayoutElevationItemKind::Stair:
      return color({0.84F, 0.66F, 0.20F, 0.86F});
    case CreativeEditorWorldLayoutElevationItemKind::Ramp:
      return color({0.35F, 0.72F, 0.40F, 0.86F});
    case CreativeEditorWorldLayoutElevationItemKind::Volume:
      return color({0.62F, 0.42F, 0.74F, 0.82F});
    case CreativeEditorWorldLayoutElevationItemKind::Count:
      break;
  }
  return color({0.75F, 0.20F, 0.20F, 1.0F});
}

bool elevationItemSelected(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationItem& item) noexcept {
  switch (item.sourceKind) {
    case CreativeEditorWorldLayoutElevationSourceKind::Room:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Room,
                      item.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::Box:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Box,
                      item.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::Wall:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Wall,
                      item.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::Opening:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Opening,
                      item.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::RoofAperture:
      return selected(
          state, CreativeEditorWorldLayoutSelectionKind::RoofAperture,
          item.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::VerticalConnector:
      return selected(
          state, CreativeEditorWorldLayoutSelectionKind::VerticalConnector,
          item.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::None:
    case CreativeEditorWorldLayoutElevationSourceKind::Count:
      break;
  }
  return false;
}

CreativeEditorWorldLayoutElevationSourceKind elevationSelectionSourceKind(
    CreativeEditorWorldLayoutSelectionKind kind) noexcept {
  switch (kind) {
    case CreativeEditorWorldLayoutSelectionKind::Room:
      return CreativeEditorWorldLayoutElevationSourceKind::Room;
    case CreativeEditorWorldLayoutSelectionKind::Box:
      return CreativeEditorWorldLayoutElevationSourceKind::Box;
    case CreativeEditorWorldLayoutSelectionKind::Wall:
      return CreativeEditorWorldLayoutElevationSourceKind::Wall;
    case CreativeEditorWorldLayoutSelectionKind::Opening:
      return CreativeEditorWorldLayoutElevationSourceKind::Opening;
    case CreativeEditorWorldLayoutSelectionKind::RoofAperture:
      return CreativeEditorWorldLayoutElevationSourceKind::RoofAperture;
    case CreativeEditorWorldLayoutSelectionKind::VerticalConnector:
      return CreativeEditorWorldLayoutElevationSourceKind::VerticalConnector;
    case CreativeEditorWorldLayoutSelectionKind::None:
    case CreativeEditorWorldLayoutSelectionKind::Building:
    case CreativeEditorWorldLayoutSelectionKind::Level:
    case CreativeEditorWorldLayoutSelectionKind::TerrainProfile:
    case CreativeEditorWorldLayoutSelectionKind::TerrainPath:
    case CreativeEditorWorldLayoutSelectionKind::Object:
    case CreativeEditorWorldLayoutSelectionKind::TopologyEdge:
      break;
  }
  return CreativeEditorWorldLayoutElevationSourceKind::None;
}

bool elevationHandleVisible(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationHandle& handle) noexcept {
  if (state.elevationManipulation.active &&
      state.elevationManipulation.handle.kind == handle.kind &&
      state.elevationManipulation.handle.sourceKind == handle.sourceKind &&
      state.elevationManipulation.handle.sourceIndex == handle.sourceIndex) {
    return true;
  }
  switch (handle.sourceKind) {
    case CreativeEditorWorldLayoutElevationSourceKind::Room:
      if (selected(state, CreativeEditorWorldLayoutSelectionKind::Room,
                   handle.sourceIndex)) {
        return true;
      }
      return (state.selection.kind ==
                  CreativeEditorWorldLayoutSelectionKind::None ||
              state.selection.kind ==
                  CreativeEditorWorldLayoutSelectionKind::Building ||
              state.selection.kind ==
                  CreativeEditorWorldLayoutSelectionKind::Level) &&
             handle.levelIndex == state.activeLevelIndex;
    case CreativeEditorWorldLayoutElevationSourceKind::Box:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Box,
                      handle.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::Wall:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Wall,
                      handle.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::Opening:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Opening,
                      handle.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::RoofAperture:
      return false;
    case CreativeEditorWorldLayoutElevationSourceKind::VerticalConnector:
      return (state.verticalConnectorManipulation.active &&
              state.verticalConnectorManipulation.target.connectorIndex ==
                  handle.sourceIndex) ||
             selected(
                 state,
                 CreativeEditorWorldLayoutSelectionKind::VerticalConnector,
                 handle.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::None:
    case CreativeEditorWorldLayoutElevationSourceKind::Count:
      break;
  }
  return false;
}

CreativeEditorWorldLayoutElevationHandle findVisibleElevationHandle(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationProjection& projection,
    CreativeEditorWorldLayoutElevationPoint point,
    double toleranceCells) noexcept {
  if (!projection.accepted || !std::isfinite(point.horizontal) ||
      !std::isfinite(point.vertical) || !std::isfinite(toleranceCells) ||
      toleranceCells <= 0.0) {
    return {};
  }
  const double toleranceSquared = toleranceCells * toleranceCells;
  for (auto iterator = projection.handles.rbegin();
       iterator != projection.handles.rend(); ++iterator) {
    if (!elevationHandleVisible(state, *iterator)) {
      continue;
    }
    const double horizontal =
        point.horizontal - iterator->position.horizontal;
    const double vertical = point.vertical - iterator->position.vertical;
    if (horizontal * horizontal + vertical * vertical <= toleranceSquared) {
      return *iterator;
    }
  }
  return {};
}

void selectElevationItem(
    CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationItem& item) {
  switch (item.sourceKind) {
    case CreativeEditorWorldLayoutElevationSourceKind::Room:
      if (item.sourceIndex < state.source.rooms.size()) {
        state.selection = {CreativeEditorWorldLayoutSelectionKind::Room,
                           item.sourceIndex};
        state.activeLevelIndex = state.source.rooms[item.sourceIndex].levelIndex;
      }
      break;
    case CreativeEditorWorldLayoutElevationSourceKind::Box:
      if (item.sourceIndex < state.source.boxes.size()) {
        state.selection = {CreativeEditorWorldLayoutSelectionKind::Box,
                           item.sourceIndex};
      }
      break;
    case CreativeEditorWorldLayoutElevationSourceKind::Wall:
      if (item.sourceIndex < state.source.walls.size()) {
        state.selection = {CreativeEditorWorldLayoutSelectionKind::Wall,
                           item.sourceIndex};
      }
      break;
    case CreativeEditorWorldLayoutElevationSourceKind::Opening:
      if (item.sourceIndex < state.source.openings.size()) {
        state.selection = {CreativeEditorWorldLayoutSelectionKind::Opening,
                           item.sourceIndex};
        if (item.levelIndex < state.source.levels.size()) {
          state.activeLevelIndex = item.levelIndex;
        }
      }
      break;
    case CreativeEditorWorldLayoutElevationSourceKind::RoofAperture:
      if (item.sourceIndex < state.source.roofApertures.size()) {
        state.selection = {
            CreativeEditorWorldLayoutSelectionKind::RoofAperture,
            item.sourceIndex};
        if (item.levelIndex < state.source.levels.size()) {
          state.activeLevelIndex = item.levelIndex;
        }
      }
      break;
    case CreativeEditorWorldLayoutElevationSourceKind::VerticalConnector:
      if (item.sourceIndex < state.source.verticalConnectors.size()) {
        state.selection = {
            CreativeEditorWorldLayoutSelectionKind::VerticalConnector,
            item.sourceIndex};
        if (item.levelIndex < state.source.levels.size()) {
          state.activeLevelIndex = item.levelIndex;
        }
      }
      break;
    case CreativeEditorWorldLayoutElevationSourceKind::None:
    case CreativeEditorWorldLayoutElevationSourceKind::Count:
      break;
  }
}

void selectElevationHandle(
    CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationHandle& handle) {
  if (handle.kind == CreativeEditorWorldLayoutElevationHandleKind::RoofRidge &&
      handle.levelIndex < state.source.levels.size()) {
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Level,
                       handle.levelIndex};
    state.activeLevelIndex = handle.levelIndex;
  } else if (handle.sourceKind ==
          CreativeEditorWorldLayoutElevationSourceKind::Room &&
      handle.sourceIndex < state.source.rooms.size()) {
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Room,
                       handle.sourceIndex};
    state.activeLevelIndex = state.source.rooms[handle.sourceIndex].levelIndex;
  } else if (handle.sourceKind ==
                 CreativeEditorWorldLayoutElevationSourceKind::Box &&
             handle.sourceIndex < state.source.boxes.size()) {
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Box,
                       handle.sourceIndex};
  } else if (handle.sourceKind ==
                 CreativeEditorWorldLayoutElevationSourceKind::Wall &&
             handle.sourceIndex < state.source.walls.size()) {
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Wall,
                       handle.sourceIndex};
  } else if (handle.sourceKind ==
                 CreativeEditorWorldLayoutElevationSourceKind::Opening &&
             handle.sourceIndex < state.source.openings.size()) {
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Opening,
                       handle.sourceIndex};
    if (handle.levelIndex < state.source.levels.size()) {
      state.activeLevelIndex = handle.levelIndex;
    }
  } else if (handle.sourceKind ==
                 CreativeEditorWorldLayoutElevationSourceKind::
                     VerticalConnector &&
             handle.sourceIndex < state.source.verticalConnectors.size()) {
    state.selection = {
        CreativeEditorWorldLayoutSelectionKind::VerticalConnector,
        handle.sourceIndex};
    if (handle.levelIndex < state.source.levels.size()) {
      state.activeLevelIndex = handle.levelIndex;
    }
  }
}

[[nodiscard]] cr::CreativeWorldLayoutTable elevationSourceTable(
    CreativeEditorWorldLayoutElevationSourceKind kind) noexcept {
  switch (kind) {
    case CreativeEditorWorldLayoutElevationSourceKind::Room:
      return cr::CreativeWorldLayoutTable::Room;
    case CreativeEditorWorldLayoutElevationSourceKind::Box:
      return cr::CreativeWorldLayoutTable::Box;
    case CreativeEditorWorldLayoutElevationSourceKind::Wall:
      return cr::CreativeWorldLayoutTable::Wall;
    case CreativeEditorWorldLayoutElevationSourceKind::Opening:
      return cr::CreativeWorldLayoutTable::Opening;
    case CreativeEditorWorldLayoutElevationSourceKind::RoofAperture:
      return cr::CreativeWorldLayoutTable::RoofAperture;
    case CreativeEditorWorldLayoutElevationSourceKind::VerticalConnector:
      return cr::CreativeWorldLayoutTable::VerticalConnector;
    case CreativeEditorWorldLayoutElevationSourceKind::None:
    case CreativeEditorWorldLayoutElevationSourceKind::Count:
      return cr::CreativeWorldLayoutTable::None;
  }
  return cr::CreativeWorldLayoutTable::None;
}

void queueElevationSourceSelection(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutElevationSourceKind sourceKind,
    std::size_t sourceIndex,
    std::size_t levelIndex,
    CreativeDesktopCommandFrame& commands) {
  const cr::CreativeWorldLayoutTable table = elevationSourceTable(sourceKind);
  if (table == cr::CreativeWorldLayoutTable::None) {
    return;
  }
  commands.push(
      CreativeDesktopCommandId::WorldLayoutSelectSourceScope,
      CreativeDesktopWorldLayoutSourcePayload{
          table, sourceIndex,
          std::string(creativeEditorWorldLayoutSourceStableKey(
              state, table, sourceIndex)),
          levelIndex});
}

void queueElevationRoofSelection(
    const CreativeEditorWorldLayoutState& state,
    std::size_t levelIndex,
    CreativeDesktopCommandFrame& commands) {
  if (levelIndex >= state.source.levels.size()) {
    return;
  }
  commands.push(
      CreativeDesktopCommandId::WorldLayoutSelectSourceScope,
      CreativeDesktopWorldLayoutSourcePayload{
          cr::CreativeWorldLayoutTable::Level, levelIndex,
          std::string(creativeEditorWorldLayoutSourceStableKey(
              state, cr::CreativeWorldLayoutTable::Level, levelIndex)),
          levelIndex});
}

void queueElevationRoofManipulation(
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutRoofManipulationPhase phase,
    std::size_t levelIndex,
    double verticalCells) {
  commands.push(
      CreativeDesktopCommandId::WorldLayoutManipulateRoof,
      CreativeDesktopWorldLayoutRoofManipulationPayload{
          phase,
          {levelIndex,
           CreativeEditorWorldLayoutRoofHandleKind::RidgeHeight},
          verticalCells});
}

CreativeEditorWorldLayoutVerticalConnectorTarget
elevationConnectorTarget(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationHandle& handle) {
  CreativeEditorWorldLayoutVerticalConnectorTarget target;
  if (handle.sourceKind !=
          CreativeEditorWorldLayoutElevationSourceKind::VerticalConnector ||
      handle.sourceIndex >= state.source.verticalConnectors.size()) {
    return target;
  }
  target.connectorIndex = handle.sourceIndex;
  const cr::CreativeWorldLayoutVerticalDirection direction =
      state.source.verticalConnectors[handle.sourceIndex].direction;
  const bool low =
      handle.kind ==
      CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunLow;
  const bool high =
      handle.kind ==
      CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunHigh;
  if (!low && !high) {
    return {};
  }
  switch (direction) {
    case cr::CreativeWorldLayoutVerticalDirection::PositiveX:
      target.handle = low ? CreativeEditorWorldLayoutRectHandle::West
                          : CreativeEditorWorldLayoutRectHandle::East;
      break;
    case cr::CreativeWorldLayoutVerticalDirection::NegativeX:
      target.handle = low ? CreativeEditorWorldLayoutRectHandle::East
                          : CreativeEditorWorldLayoutRectHandle::West;
      break;
    case cr::CreativeWorldLayoutVerticalDirection::PositiveZ:
      target.handle = low ? CreativeEditorWorldLayoutRectHandle::North
                          : CreativeEditorWorldLayoutRectHandle::South;
      break;
    case cr::CreativeWorldLayoutVerticalDirection::NegativeZ:
      target.handle = low ? CreativeEditorWorldLayoutRectHandle::South
                          : CreativeEditorWorldLayoutRectHandle::North;
      break;
    case cr::CreativeWorldLayoutVerticalDirection::Count:
      return {};
  }
  return target;
}

CreativeEditorWorldLayoutPoint elevationConnectorPoint(
    cr::CreativeWorldLayoutRect footprint,
    CreativeEditorWorldLayoutRectHandle handle,
    double horizontal) {
  CreativeEditorWorldLayoutPoint point{
      (static_cast<double>(footprint.minimum.x) + footprint.maximum.x) * 0.5,
      (static_cast<double>(footprint.minimum.z) + footprint.maximum.z) * 0.5};
  if (handle == CreativeEditorWorldLayoutRectHandle::East ||
      handle == CreativeEditorWorldLayoutRectHandle::West) {
    point.x = horizontal;
  } else if (handle == CreativeEditorWorldLayoutRectHandle::North ||
             handle == CreativeEditorWorldLayoutRectHandle::South) {
    point.z = horizontal;
  }
  return point;
}

void queueElevationConnectorManipulation(
    const CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands,
    CreativeEditorWorldLayoutVerticalConnectorManipulationPhase phase,
    CreativeEditorWorldLayoutVerticalConnectorTarget target,
    double horizontal) {
  if (target.connectorIndex >= state.source.verticalConnectors.size()) {
    return;
  }
  const cr::CreativeWorldLayoutRect footprint =
      state.source.verticalConnectors[target.connectorIndex].footprint;
  commands.push(
      CreativeDesktopCommandId::WorldLayoutManipulateVerticalConnector,
      CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload{
          phase,
          elevationConnectorPoint(footprint, target.handle, horizontal),
          0.25,
          target});
}

bool queueElevationEdit(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationEditResult& edit,
    CreativeDesktopCommandFrame& commands) {
  if (!edit.accepted) {
    return false;
  }
  if (edit.handle.sourceKind ==
      CreativeEditorWorldLayoutElevationSourceKind::Room) {
    if (edit.handle.kind ==
        CreativeEditorWorldLayoutElevationHandleKind::LevelFloor) {
      if (edit.handle.levelIndex >= state.source.levels.size()) {
        return false;
      }
      commands.push(
          CreativeDesktopCommandId::WorldLayoutSetLevelDatum,
          CreativeDesktopWorldLayoutLevelDatumPayload{
              edit.handle.levelIndex,
              state.source.levels[edit.handle.levelIndex].stableKey,
              state.elevationLevelEditScope,
              edit.floorTopLayer});
      return true;
    }
    if (edit.handle.kind !=
            CreativeEditorWorldLayoutElevationHandleKind::WallTop ||
        edit.handle.levelIndex >= state.source.levels.size()) {
      return false;
    }
    CreativeEditorWorldLayoutLevelSettings settings;
    if (!readCreativeEditorWorldLayoutLevelSettings(
            state, edit.handle.levelIndex, settings)) {
      return false;
    }
    settings.wallHeightCells = edit.wallHeightCells;
    commands.push(
        CreativeDesktopCommandId::WorldLayoutSetLevelSettings,
        CreativeDesktopWorldLayoutLevelSettingsPayload{
            edit.handle.levelIndex,
            state.source.levels[edit.handle.levelIndex].stableKey,
            settings});
    return true;
  }
  if (edit.handle.sourceKind ==
      CreativeEditorWorldLayoutElevationSourceKind::Box) {
    CreativeEditorWorldLayoutBoxSettings settings;
    if (!readCreativeEditorWorldLayoutBoxSettings(
            state, edit.handle.sourceIndex, settings)) {
      return false;
    }
    settings.anchorLayer = edit.floorTopLayer;
    commands.push(
        CreativeDesktopCommandId::WorldLayoutSetBoxSettings,
        CreativeDesktopWorldLayoutBoxSettingsPayload{edit.handle.sourceIndex,
                                                     settings});
    return true;
  }
  if (edit.handle.sourceKind ==
      CreativeEditorWorldLayoutElevationSourceKind::Wall) {
    CreativeEditorWorldLayoutWallSettings settings;
    if (!readCreativeEditorWorldLayoutWallSettings(
            state, edit.handle.sourceIndex, settings)) {
      return false;
    }
    settings.heightCells = edit.wallHeightCells;
    commands.push(
        CreativeDesktopCommandId::WorldLayoutSetWallSettings,
        CreativeDesktopWorldLayoutWallSettingsPayload{edit.handle.sourceIndex,
                                                      settings});
    return true;
  }
  if (edit.handle.sourceKind ==
      CreativeEditorWorldLayoutElevationSourceKind::Opening) {
    CreativeEditorWorldLayoutOpeningSettings settings;
    if (!readCreativeEditorWorldLayoutOpeningSettings(
            state, edit.handle.sourceIndex, settings)) {
      return false;
    }
    settings.sillHeightCells = edit.openingSillCells;
    settings.heightCells = edit.openingHeightCells;
    commands.push(
        CreativeDesktopCommandId::WorldLayoutSetOpeningSettings,
        CreativeDesktopWorldLayoutOpeningSettingsPayload{
            edit.handle.sourceIndex, settings});
    return true;
  }
  return false;
}

CreativeEditorWorldLayoutElevationPoint previewElevationHandlePosition(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationProjection& projection,
    const CreativeEditorWorldLayoutElevationHandle& handle) {
  CreativeEditorWorldLayoutElevationPoint position = handle.position;
  if ((handle.kind ==
           CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunLow ||
       handle.kind ==
           CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunHigh) &&
      handle.sourceIndex < state.source.verticalConnectors.size() &&
      state.verticalConnectorManipulation.active &&
      state.verticalConnectorManipulation.target.connectorIndex ==
          handle.sourceIndex &&
      state.verticalConnectorManipulation.previewValid) {
    const CreativeEditorWorldLayoutVerticalConnectorTarget target =
        elevationConnectorTarget(state, handle);
    const cr::CreativeWorldLayoutRect footprint =
        state.verticalConnectorManipulation.previewFootprint;
    if (target.handle == CreativeEditorWorldLayoutRectHandle::East) {
      position.horizontal = footprint.maximum.x;
    } else if (target.handle == CreativeEditorWorldLayoutRectHandle::West) {
      position.horizontal = footprint.minimum.x;
    } else if (target.handle == CreativeEditorWorldLayoutRectHandle::North) {
      position.horizontal = footprint.minimum.z;
    } else if (target.handle == CreativeEditorWorldLayoutRectHandle::South) {
      position.horizontal = footprint.maximum.z;
    }
    return position;
  }
  const CreativeEditorWorldLayoutElevationManipulationState& manipulation =
      state.elevationManipulation;
  if (!manipulation.active || !manipulation.preview.accepted ||
      manipulation.handle.kind != handle.kind ||
      manipulation.handle.sourceKind != handle.sourceKind ||
      manipulation.handle.sourceIndex != handle.sourceIndex) {
    return position;
  }
  const CreativeEditorWorldLayoutElevationEditResult& edit =
      manipulation.preview;
  switch (handle.kind) {
    case CreativeEditorWorldLayoutElevationHandleKind::LevelFloor:
      position.vertical = edit.floorTopLayer;
      break;
    case CreativeEditorWorldLayoutElevationHandleKind::WallTop:
      if (handle.sourceKind ==
              CreativeEditorWorldLayoutElevationSourceKind::Wall &&
          handle.sourceIndex < state.source.walls.size()) {
        position.vertical =
            state.source.walls[handle.sourceIndex].baseLayer +
            static_cast<double>(edit.wallHeightCells);
      } else if (handle.levelIndex < state.source.levels.size()) {
        position.vertical =
            state.source.levels[handle.levelIndex].floorTopLayer +
            static_cast<double>(edit.wallHeightCells);
      }
      break;
    case CreativeEditorWorldLayoutElevationHandleKind::RoofRidge:
      for (const CreativeEditorWorldLayoutElevationLine& line :
           projection.lines) {
        if (line.kind ==
                CreativeEditorWorldLayoutElevationLineKind::RoofSlope &&
            line.levelIndex == handle.levelIndex) {
          constexpr double kDegreesToRadians =
              0.01745329251994329576923690768489;
          const double run =
              std::abs(line.end.horizontal - line.start.horizontal);
          position.vertical =
              line.start.vertical +
              std::tan(edit.roofPitchDegrees * kDegreesToRadians) * run;
          break;
        }
      }
      break;
    case CreativeEditorWorldLayoutElevationHandleKind::OpeningBottom:
      if (handle.sourceIndex < state.source.openings.size()) {
        position.vertical +=
            edit.openingSillCells -
            state.source.openings[handle.sourceIndex].cutoutBottomCells;
      }
      break;
    case CreativeEditorWorldLayoutElevationHandleKind::OpeningTop:
      if (handle.sourceIndex < state.source.openings.size()) {
        const cr::CreativeWorldLayoutOpening& opening =
            state.source.openings[handle.sourceIndex];
        position.vertical += edit.openingSillCells + edit.openingHeightCells -
                             opening.cutoutBottomCells -
                             opening.cutoutHeightCells;
      }
      break;
    case CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunLow:
    case CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunHigh:
      break;
    case CreativeEditorWorldLayoutElevationHandleKind::None:
    case CreativeEditorWorldLayoutElevationHandleKind::Count:
      break;
  }
  return position;
}


void drawElevationCanvas(CreativeEditorState& editor,
                         const cr::CreativeGridSettings& grid,
                         const cr::CreativeMeasurementAnnotationStore&
                             measurementAnnotations,
                         const cr::CreativeMeasurementState& measurement,
                         CreativeDesktopCommandFrame& commands,
                         bool interactionEnabled,
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
  drawElevationSectionControls(state, interactionEnabled);
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

  ImDrawList* drawList = ImGui::GetWindowDrawList();
  drawList->PushClipRect(minimum, maximum, true);
  drawList->AddRectFilled(minimum, maximum,
                          color({0.105F, 0.12F, 0.135F, 1.0F}));
  drawElevationGrid(*drawList, minimum, maximum, transform);
  if (projection.accepted) {
    for (const CreativeEditorWorldLayoutElevationItem& item :
         projection.items) {
      ImVec2 itemMinimum = toElevationScreen(
          transform, {item.minimumHorizontal, item.maximumVertical});
      ImVec2 itemMaximum = toElevationScreen(
          transform, {item.maximumHorizontal, item.minimumVertical});
      if (itemMaximum.x - itemMinimum.x < 3.0F) {
        const float center = (itemMinimum.x + itemMaximum.x) * 0.5F;
        itemMinimum.x = center - 1.5F;
        itemMaximum.x = center + 1.5F;
      }
      if (itemMaximum.y - itemMinimum.y < 3.0F) {
        const float center = (itemMinimum.y + itemMaximum.y) * 0.5F;
        itemMinimum.y = center - 1.5F;
        itemMaximum.y = center + 1.5F;
      }
      drawList->AddRectFilled(itemMinimum, itemMaximum,
                              elevationItemColor(item.kind));
      drawList->AddRect(
          itemMinimum, itemMaximum,
          elevationItemSelected(state, item)
              ? color({0.30F, 0.95F, 0.42F, 1.0F})
              : color({0.70F, 0.74F, 0.78F, 0.92F}),
          0.0F, 0, elevationItemSelected(state, item) ? 2.4F : 1.0F);
    }
    for (const CreativeEditorWorldLayoutElevationLine& line :
         projection.lines) {
      const ImU32 lineColor =
          line.kind ==
                  CreativeEditorWorldLayoutElevationLineKind::ConnectorRise
              ? color({0.96F, 0.74F, 0.22F, 1.0F})
              : color({0.87F, 0.55F, 0.43F, 1.0F});
      drawList->AddLine(toElevationScreen(transform, line.start),
                        toElevationScreen(transform, line.end), lineColor,
                        2.4F);
    }
    for (const CreativeEditorWorldLayoutElevationHandle& handle :
         projection.handles) {
      if (!elevationHandleVisible(state, handle)) {
        continue;
      }
      const CreativeEditorWorldLayoutElevationPoint position =
          previewElevationHandlePosition(state, projection, handle);
      const bool roofActive =
          handle.kind ==
              CreativeEditorWorldLayoutElevationHandleKind::RoofRidge &&
          state.roofManipulation.active &&
          state.roofManipulation.target.levelIndex == handle.levelIndex;
      const bool elevationActive =
          state.elevationManipulation.active &&
          state.elevationManipulation.handle.kind == handle.kind &&
          state.elevationManipulation.handle.sourceKind == handle.sourceKind &&
          state.elevationManipulation.handle.sourceIndex == handle.sourceIndex;
      const bool connectorActive =
          handle.sourceKind ==
              CreativeEditorWorldLayoutElevationSourceKind::
                  VerticalConnector &&
          state.verticalConnectorManipulation.active &&
          state.verticalConnectorManipulation.target.connectorIndex ==
              handle.sourceIndex;
      const bool active = roofActive || elevationActive || connectorActive;
      const bool valid =
          !active ||
          (roofActive
               ? state.roofManipulation.previewValid
               : connectorActive
                     ? state.verticalConnectorManipulation.previewValid
                     : state.elevationManipulation.preview.accepted);
      drawList->AddCircleFilled(
          toElevationScreen(transform, position), active ? 6.0F : 4.0F,
          valid ? color({0.30F, 0.95F, 0.42F, 1.0F})
                : color({0.95F, 0.24F, 0.20F, 1.0F}));
      drawList->AddCircle(toElevationScreen(transform, position),
                          active ? 6.0F : 4.0F,
                          color({0.06F, 0.07F, 0.08F, 1.0F}), 0, 1.2F);
    }
    drawElevationSectionAnnotations(*drawList, minimum, maximum, transform,
                                    projection, state);
  }
  for (const cr::CreativeMeasurementAnnotation& annotation :
       measurementAnnotations.annotations) {
    drawMeasurementGeometry(
        *drawList, transform, state.elevationAxis,
        projectCreativeEditorMeasurementGeometryToGrid(
            cr::buildCreativeMeasurementGeometry(annotation), grid),
        annotation.name, false);
  }
  const cr::CreativeMeasurementGeometry transientMeasurement =
      projectCreativeEditorMeasurementGeometryToGrid(
          cr::buildCreativeMeasurementGeometry(measurement), grid);
  drawMeasurementGeometry(
      *drawList, transform, state.elevationAxis, transientMeasurement,
      transientMeasurement.visible
          ? formatCreativeEditorMeasurementReadout(measurement)
          : std::string{},
      true);
  drawList->PopClipRect();

  if (!interactionEnabled || !projection.accepted) {
    if (state.roofManipulation.active) {
      queueElevationRoofManipulation(
          commands, CreativeEditorWorldLayoutRoofManipulationPhase::Cancel,
          state.roofManipulation.target.levelIndex, 0.0);
    }
    if (state.verticalConnectorManipulation.active) {
      queueElevationConnectorManipulation(
          state, commands,
          CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Cancel,
          state.verticalConnectorManipulation.target, 0.0);
    }
    state.elevationManipulation = {};
    return;
  }
  const CreativeEditorWorldLayoutElevationPoint pointer =
      toElevationWorld(transform, pointerPosition);
  const double handleTolerance = std::clamp(
      8.0 / static_cast<double>(transform.pixelsPerCell), 0.10, 0.45);
  const CreativeEditorWorldLayoutElevationHandle hoveredHandle =
      hovered ? findVisibleElevationHandle(state, projection, pointer,
                                            handleTolerance)
              : CreativeEditorWorldLayoutElevationHandle{};
  const bool connectorHandle =
      hoveredHandle.kind ==
          CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunLow ||
      hoveredHandle.kind ==
          CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunHigh;
  if (state.verticalConnectorManipulation.active || connectorHandle) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
  } else if (state.elevationManipulation.active ||
             state.roofManipulation.active ||
      hoveredHandle.kind !=
          CreativeEditorWorldLayoutElevationHandleKind::None) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
  }

  if (hovered && input.pointer.primaryPressed) {
    if (hoveredHandle.kind !=
        CreativeEditorWorldLayoutElevationHandleKind::None) {
      selectElevationHandle(state, hoveredHandle);
      if (hoveredHandle.kind ==
          CreativeEditorWorldLayoutElevationHandleKind::RoofRidge) {
        queueElevationRoofSelection(state, hoveredHandle.levelIndex, commands);
        queueElevationRoofManipulation(
            commands, CreativeEditorWorldLayoutRoofManipulationPhase::Begin,
            hoveredHandle.levelIndex, pointer.vertical);
      } else if (connectorHandle) {
        const CreativeEditorWorldLayoutVerticalConnectorTarget target =
            elevationConnectorTarget(state, hoveredHandle);
        queueElevationSourceSelection(
            state, hoveredHandle.sourceKind, hoveredHandle.sourceIndex,
            hoveredHandle.levelIndex, commands);
        queueElevationConnectorManipulation(
            state, commands,
            CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Begin,
            target, pointer.horizontal);
      } else {
        state.elevationManipulation = {
            true,
            state.revision,
            hoveredHandle,
            planCreativeEditorWorldLayoutElevationEdit(
                state.source, projection, hoveredHandle, pointer.vertical,
                state.elevationLevelEditScope)};
        queueElevationSourceSelection(
            state, hoveredHandle.sourceKind, hoveredHandle.sourceIndex,
            hoveredHandle.levelIndex, commands);
      }
    } else if (const CreativeEditorWorldLayoutElevationItem* item =
                   cycleCreativeEditorWorldLayoutElevationItem(
                       findCreativeEditorWorldLayoutElevationItemStack(
                           projection, pointer, handleTolerance),
                       elevationSelectionSourceKind(state.selection.kind),
                       state.selection.index);
               item != nullptr) {
      selectElevationItem(state, *item);
      queueElevationSourceSelection(state, item->sourceKind, item->sourceIndex,
                                    item->levelIndex, commands);
    } else {
      commands.push(CreativeDesktopCommandId::WorldLayoutClearSelection);
    }
  }

  if (state.roofManipulation.active) {
    const bool cancelRoof =
        input.pointer.focusLost ||
        state.roofManipulation.sourceRevision != state.revision ||
        (hovered && input.pointer.secondaryPressed) ||
        input.cancelPressed;
    if (cancelRoof) {
      queueElevationRoofManipulation(
          commands, CreativeEditorWorldLayoutRoofManipulationPhase::Cancel,
          state.roofManipulation.target.levelIndex, pointer.vertical);
    } else if (input.pointer.primaryReleased) {
      queueElevationRoofManipulation(
          commands, CreativeEditorWorldLayoutRoofManipulationPhase::Commit,
          state.roofManipulation.target.levelIndex, pointer.vertical);
    } else if (input.pointer.primaryDown) {
      queueElevationRoofManipulation(
          commands, CreativeEditorWorldLayoutRoofManipulationPhase::Update,
          state.roofManipulation.target.levelIndex, pointer.vertical);
    }
    return;
  }

  if (state.verticalConnectorManipulation.active) {
    const bool cancelConnector =
        input.pointer.focusLost ||
        state.verticalConnectorManipulation.sourceRevision != state.revision ||
        (hovered && input.pointer.secondaryPressed) ||
        input.cancelPressed;
    const CreativeEditorWorldLayoutVerticalConnectorTarget target =
        state.verticalConnectorManipulation.target;
    if (cancelConnector) {
      queueElevationConnectorManipulation(
          state, commands,
          CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Cancel,
          target, pointer.horizontal);
    } else if (input.pointer.primaryReleased) {
      queueElevationConnectorManipulation(
          state, commands,
          CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Commit,
          target, pointer.horizontal);
    } else if (input.pointer.primaryDown) {
      queueElevationConnectorManipulation(
          state, commands,
          CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Update,
          target, pointer.horizontal);
    }
    return;
  }

  if (!state.elevationManipulation.active) {
    return;
  }
  const bool cancel =
      input.pointer.focusLost || state.elevationManipulation.sourceRevision !=
                             state.revision ||
      (hovered && input.pointer.secondaryPressed) ||
      input.cancelPressed;
  if (cancel) {
    state.elevationManipulation = {};
    return;
  }
  state.elevationManipulation.preview =
      planCreativeEditorWorldLayoutElevationEdit(
          state.source, projection, state.elevationManipulation.handle,
          pointer.vertical, state.elevationLevelEditScope);
  if (input.pointer.primaryReleased) {
    static_cast<void>(queueElevationEdit(
        state, state.elevationManipulation.preview, commands));
    state.elevationManipulation = {};
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
