#include "EditorWorldLayoutElevationPanel.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

#include "imgui.h"
#include "EditorDraftingStyle.hpp"
#include "EditorMeasurement.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutRoofs.hpp"

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

bool queueElevationEdit(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationEditResult& edit,
    CreativeDesktopCommandFrame& commands) {
  if (!edit.accepted) {
    return false;
  }
  if (edit.handle.sourceKind ==
      CreativeEditorWorldLayoutElevationSourceKind::Room) {
    CreativeEditorWorldLayoutRoomSettings settings;
    if (!readCreativeEditorWorldLayoutRoomSettings(
            state, edit.handle.sourceIndex, settings)) {
      return false;
    }
    switch (edit.handle.kind) {
      case CreativeEditorWorldLayoutElevationHandleKind::LevelFloor:
        settings.floorTopLayer = edit.floorTopLayer;
        break;
      case CreativeEditorWorldLayoutElevationHandleKind::WallTop:
        settings.wallHeightCells = edit.wallHeightCells;
        break;
      case CreativeEditorWorldLayoutElevationHandleKind::RoofRidge:
        return false;
      case CreativeEditorWorldLayoutElevationHandleKind::None:
      case CreativeEditorWorldLayoutElevationHandleKind::OpeningBottom:
      case CreativeEditorWorldLayoutElevationHandleKind::OpeningTop:
      case CreativeEditorWorldLayoutElevationHandleKind::Count:
        return false;
    }
    commands.push(
        CreativeDesktopCommandId::WorldLayoutSetRoomSettings,
        CreativeDesktopWorldLayoutRoomSettingsPayload{edit.handle.sourceIndex,
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
                         bool interactionEnabled) {
  CreativeEditorWorldLayoutState& state = editor.worldLayout;
  const CreativeEditorWorldLayoutInspection inspection =
      inspectCreativeEditorWorldLayout(state);
  const std::size_t buildingIndex =
      elevationBuildingIndex(state, *inspection.source);
  const CreativeEditorWorldLayoutElevationProjection& projection =
      elevationProjection(state, grid, buildingIndex, inspection);
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
  ImGuiIO& io = ImGui::GetIO();

  const double centerHorizontal =
      projection.bounds.valid
          ? (projection.bounds.minimumHorizontal +
             projection.bounds.maximumHorizontal) *
                0.5
          : 0.0;
  const double centerVertical =
      projection.bounds.valid
          ? (projection.bounds.minimumVertical +
             projection.bounds.maximumVertical) *
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
  if (hovered && io.MouseWheel != 0.0F) {
    const CreativeEditorWorldLayoutElevationPoint before =
        toElevationWorld(transform, io.MousePos);
    state.elevationPixelsPerCell = std::clamp(
        state.elevationPixelsPerCell * (io.MouseWheel > 0.0F ? 1.15F : 0.87F),
        12.0F, 80.0F);
    transform = makeTransform();
    const ImVec2 anchored = toElevationScreen(transform, before);
    state.elevationPanHorizontal += io.MousePos.x - anchored.x;
    state.elevationPanY += io.MousePos.y - anchored.y;
    transform = makeTransform();
  }
  if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
    state.elevationPanHorizontal += io.MouseDelta.x;
    state.elevationPanY += io.MouseDelta.y;
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
      const bool active = roofActive || elevationActive;
      const bool valid =
          !active || (roofActive ? state.roofManipulation.previewValid
                                 : state.elevationManipulation.preview.accepted);
      drawList->AddCircleFilled(
          toElevationScreen(transform, position), active ? 6.0F : 4.0F,
          valid ? color({0.30F, 0.95F, 0.42F, 1.0F})
                : color({0.95F, 0.24F, 0.20F, 1.0F}));
      drawList->AddCircle(toElevationScreen(transform, position),
                          active ? 6.0F : 4.0F,
                          color({0.06F, 0.07F, 0.08F, 1.0F}), 0, 1.2F);
    }
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
    state.elevationManipulation = {};
    return;
  }
  const CreativeEditorWorldLayoutElevationPoint pointer =
      toElevationWorld(transform, io.MousePos);
  const double handleTolerance = std::clamp(
      8.0 / static_cast<double>(transform.pixelsPerCell), 0.10, 0.45);
  const CreativeEditorWorldLayoutElevationHandle hoveredHandle =
      hovered ? findVisibleElevationHandle(state, projection, pointer,
                                            handleTolerance)
              : CreativeEditorWorldLayoutElevationHandle{};
  if (state.elevationManipulation.active ||
      state.roofManipulation.active ||
      hoveredHandle.kind !=
          CreativeEditorWorldLayoutElevationHandleKind::None) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
  }

  if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    if (hoveredHandle.kind !=
        CreativeEditorWorldLayoutElevationHandleKind::None) {
      selectElevationHandle(state, hoveredHandle);
      if (hoveredHandle.kind ==
          CreativeEditorWorldLayoutElevationHandleKind::RoofRidge) {
        queueElevationRoofSelection(state, hoveredHandle.levelIndex, commands);
        queueElevationRoofManipulation(
            commands, CreativeEditorWorldLayoutRoofManipulationPhase::Begin,
            hoveredHandle.levelIndex, pointer.vertical);
      } else {
        state.elevationManipulation = {
            true,
            state.revision,
            hoveredHandle,
            planCreativeEditorWorldLayoutElevationEdit(
                state.source, projection, hoveredHandle, pointer.vertical)};
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
        io.AppFocusLost ||
        state.roofManipulation.sourceRevision != state.revision ||
        (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
        ImGui::IsKeyPressed(ImGuiKey_Escape);
    if (cancelRoof) {
      queueElevationRoofManipulation(
          commands, CreativeEditorWorldLayoutRoofManipulationPhase::Cancel,
          state.roofManipulation.target.levelIndex, pointer.vertical);
    } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
      queueElevationRoofManipulation(
          commands, CreativeEditorWorldLayoutRoofManipulationPhase::Commit,
          state.roofManipulation.target.levelIndex, pointer.vertical);
    } else if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
      queueElevationRoofManipulation(
          commands, CreativeEditorWorldLayoutRoofManipulationPhase::Update,
          state.roofManipulation.target.levelIndex, pointer.vertical);
    }
    return;
  }

  if (!state.elevationManipulation.active) {
    return;
  }
  const bool cancel =
      io.AppFocusLost || state.elevationManipulation.sourceRevision !=
                             state.revision ||
      (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ||
      ImGui::IsKeyPressed(ImGuiKey_Escape);
  if (cancel) {
    state.elevationManipulation = {};
    return;
  }
  state.elevationManipulation.preview =
      planCreativeEditorWorldLayoutElevationEdit(
          state.source, projection, state.elevationManipulation.handle,
          pointer.vertical);
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
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
    CreativeDesktopCommandFrame& commands, bool interactionEnabled) {
  drawElevationCanvas(editor, grid, measurementAnnotations, measurement,
                      commands, interactionEnabled);
}

}  // namespace iggy3d_creative_app
