#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <utility>

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] std::string* sourceName(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index) noexcept {
  switch (table) {
    case cr::CreativeWorldLayoutTable::Building:
      return index < state.source.buildings.size()
                 ? &state.source.buildings[index].name
                 : nullptr;
    case cr::CreativeWorldLayoutTable::Level:
      return index < state.source.levels.size()
                 ? &state.source.levels[index].name
                 : nullptr;
    case cr::CreativeWorldLayoutTable::Room:
      return index < state.source.rooms.size()
                 ? &state.source.rooms[index].name
                 : nullptr;
    case cr::CreativeWorldLayoutTable::VerticalConnector:
      return index < state.source.verticalConnectors.size()
                 ? &state.source.verticalConnectors[index].name
                 : nullptr;
    case cr::CreativeWorldLayoutTable::Box:
      return index < state.source.boxes.size()
                 ? &state.source.boxes[index].name
                 : nullptr;
    case cr::CreativeWorldLayoutTable::Wall:
      return index < state.source.walls.size()
                 ? &state.source.walls[index].name
                 : nullptr;
    case cr::CreativeWorldLayoutTable::Opening:
      return index < state.source.openings.size()
                 ? &state.source.openings[index].name
                 : nullptr;
    case cr::CreativeWorldLayoutTable::Object:
      return index < state.source.objects.size()
                 ? &state.source.objects[index].name
                 : nullptr;
    case cr::CreativeWorldLayoutTable::None:
    case cr::CreativeWorldLayoutTable::TerrainProfile:
    case cr::CreativeWorldLayoutTable::TerrainPath:
    case cr::CreativeWorldLayoutTable::TerrainPathPoint:
      return nullptr;
  }
  return nullptr;
}

[[nodiscard]] std::string_view sourceStableKey(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index) noexcept {
  switch (table) {
    case cr::CreativeWorldLayoutTable::Building:
      return index < state.source.buildings.size()
                 ? state.source.buildings[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::Level:
      return index < state.source.levels.size()
                 ? state.source.levels[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::Room:
      return index < state.source.rooms.size()
                 ? state.source.rooms[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::VerticalConnector:
      return index < state.source.verticalConnectors.size()
                 ? state.source.verticalConnectors[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::Box:
      return index < state.source.boxes.size()
                 ? state.source.boxes[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::Wall:
      return index < state.source.walls.size()
                 ? state.source.walls[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::Opening:
      return index < state.source.openings.size()
                 ? state.source.openings[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::Object:
      return index < state.source.objects.size()
                 ? state.source.objects[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::TerrainProfile:
      return index < state.source.terrainProfiles.size()
                 ? state.source.terrainProfiles[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::TerrainPath:
      return index < state.source.terrainPaths.size()
                 ? state.source.terrainPaths[index].stableKey
                 : std::string_view{};
    case cr::CreativeWorldLayoutTable::None:
    case cr::CreativeWorldLayoutTable::TerrainPathPoint:
      return {};
  }
  return {};
}

struct SourceTarget {
  bool valid = false;
  CreativeEditorWorldLayoutSelection selection;
  std::size_t activeLevelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  bool hasCenter = false;
  CreativeEditorWorldLayoutPoint center;
};

CreativeEditorWorldLayoutPoint rectCenter(cr::CreativeWorldLayoutRect value) {
  return {(static_cast<double>(value.minimum.x) + value.maximum.x) * 0.5,
          (static_cast<double>(value.minimum.z) + value.maximum.z) * 0.5};
}

bool resolveBuildingCenter(const CreativeEditorWorldLayoutState& state,
                           std::size_t buildingIndex,
                           CreativeEditorWorldLayoutPoint& center) noexcept {
  cr::CreativeWorldLayoutBuildingBounds bounds;
  if (!cr::measureCreativeWorldLayoutBuildingBounds(state.source,
                                                     buildingIndex, bounds)) {
    return false;
  }
  center = {(static_cast<double>(bounds.minimum.x) + bounds.maximum.x) * 0.5,
            (static_cast<double>(bounds.minimum.z) + bounds.maximum.z) * 0.5};
  return true;
}

SourceTarget resolveSourceTarget(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index) {
  SourceTarget target;
  const cr::CreativeWorldLayout& source = state.source;
  switch (table) {
    case cr::CreativeWorldLayoutTable::Building:
      if (index >= source.buildings.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                          index};
      target.buildingIndex = index;
      target.hasCenter = resolveBuildingCenter(state, index, target.center);
      break;
    case cr::CreativeWorldLayoutTable::Level:
      if (index >= source.levels.size()) return target;
      target.activeLevelIndex = index;
      target.buildingIndex = source.levels[index].buildingIndex;
      if (target.buildingIndex >= source.buildings.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Level,
                          index};
      target.hasCenter =
          resolveBuildingCenter(state, target.buildingIndex, target.center);
      break;
    case cr::CreativeWorldLayoutTable::Room:
      if (index >= source.rooms.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Room, index};
      target.activeLevelIndex = source.rooms[index].levelIndex;
      target.buildingIndex = source.rooms[index].buildingIndex;
      target.center = rectCenter(source.rooms[index].footprint);
      target.hasCenter = true;
      break;
    case cr::CreativeWorldLayoutTable::VerticalConnector:
      if (index >= source.verticalConnectors.size()) return target;
      target.selection = {
          CreativeEditorWorldLayoutSelectionKind::VerticalConnector, index};
      target.buildingIndex = source.verticalConnectors[index].buildingIndex;
      target.center = rectCenter(source.verticalConnectors[index].footprint);
      target.hasCenter = true;
      if (source.verticalConnectors[index].lowerRoomIndex < source.rooms.size()) {
        target.activeLevelIndex =
            source.rooms[source.verticalConnectors[index].lowerRoomIndex]
                .levelIndex;
      }
      break;
    case cr::CreativeWorldLayoutTable::Box:
      if (index >= source.boxes.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Box, index};
      target.buildingIndex = source.boxes[index].buildingIndex;
      target.center = rectCenter(source.boxes[index].footprint);
      target.hasCenter = true;
      break;
    case cr::CreativeWorldLayoutTable::Wall:
      if (index >= source.walls.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Wall, index};
      target.buildingIndex = source.walls[index].buildingIndex;
      target.center = {
          (static_cast<double>(source.walls[index].start.x) +
           source.walls[index].end.x) *
              0.5,
          (static_cast<double>(source.walls[index].start.z) +
           source.walls[index].end.z) *
              0.5};
      target.hasCenter = true;
      break;
    case cr::CreativeWorldLayoutTable::Opening: {
      if (index >= source.openings.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Opening,
                          index};
      const cr::CreativeWorldLayoutOpening& opening = source.openings[index];
      const CreativeEditorWorldLayoutOpeningHost host =
          resolveCreativeEditorWorldLayoutOpeningHost(state, index);
      if (host.valid && host.lengthCells > 0.0) {
        const double t = std::clamp(opening.centerOffsetCells /
                                        host.lengthCells,
                                    0.0, 1.0);
        target.center = {host.start.x + (host.end.x - host.start.x) * t,
                         host.start.z + (host.end.z - host.start.z) * t};
        target.hasCenter = true;
      }
      if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall &&
          opening.wallIndex < source.walls.size()) {
        target.buildingIndex =
            source.walls[opening.wallIndex].buildingIndex;
      } else if (opening.hostKind ==
                     cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
                 opening.roomIndex < source.rooms.size()) {
        target.buildingIndex =
            source.rooms[opening.roomIndex].buildingIndex;
        target.activeLevelIndex =
            source.rooms[opening.roomIndex].levelIndex;
      }
      break;
    }
    case cr::CreativeWorldLayoutTable::Object: {
      if (index >= source.objects.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Object,
                          index};
      const cr::CreativeWorldLayoutObject& object = source.objects[index];
      target.center =
          object.mode == cr::CreativeObjectLibraryPlacementMode::Bounds
              ? CreativeEditorWorldLayoutPoint{
                    (object.boundsCells.min.x + object.boundsCells.max.x) * 0.5,
                    (object.boundsCells.min.z + object.boundsCells.max.z) * 0.5}
              : CreativeEditorWorldLayoutPoint{object.pointCells.x,
                                               object.pointCells.z};
      target.hasCenter = std::isfinite(target.center.x) &&
                         std::isfinite(target.center.z);
      break;
    }
    case cr::CreativeWorldLayoutTable::TerrainProfile:
      if (index >= source.terrainProfiles.size()) return target;
      target.selection = {
          CreativeEditorWorldLayoutSelectionKind::TerrainProfile, index};
      target.center = {
          static_cast<double>(source.terrainProfiles[index].center.x),
          static_cast<double>(source.terrainProfiles[index].center.z)};
      target.hasCenter = true;
      break;
    case cr::CreativeWorldLayoutTable::TerrainPath: {
      if (index >= source.terrainPaths.size()) return target;
      const cr::CreativeWorldLayoutTerrainPath& path =
          source.terrainPaths[index];
      if (path.pointCount == 0U ||
          path.firstPointIndex > source.terrainPathPoints.size() ||
          path.pointCount >
              source.terrainPathPoints.size() - path.firstPointIndex) {
        return target;
      }
      target.selection = {
          CreativeEditorWorldLayoutSelectionKind::TerrainPath, index};
      double sumX = 0.0;
      double sumZ = 0.0;
      for (std::size_t pointIndex = 0U; pointIndex < path.pointCount;
           ++pointIndex) {
        const cr::CreativeTerrainPathPoint& point =
            source.terrainPathPoints[path.firstPointIndex + pointIndex];
        sumX += point.coord.x;
        sumZ += point.coord.z;
      }
      const double pointCount = static_cast<double>(path.pointCount);
      target.center = {sumX / pointCount, sumZ / pointCount};
      target.hasCenter = true;
      break;
    }
    case cr::CreativeWorldLayoutTable::TerrainPathPoint: {
      if (index >= source.terrainPathPoints.size()) return target;
      const auto owner = std::find_if(
          source.terrainPaths.begin(), source.terrainPaths.end(),
          [index](const cr::CreativeWorldLayoutTerrainPath& path) {
            return index >= path.firstPointIndex &&
                   index - path.firstPointIndex < path.pointCount;
          });
      if (owner == source.terrainPaths.end()) return target;
      target.selection = {
          CreativeEditorWorldLayoutSelectionKind::TerrainPath,
          static_cast<std::size_t>(owner - source.terrainPaths.begin())};
      target.center = {
          static_cast<double>(source.terrainPathPoints[index].coord.x),
          static_cast<double>(source.terrainPathPoints[index].coord.z)};
      target.hasCenter = true;
      break;
    }
    case cr::CreativeWorldLayoutTable::None:
      return target;
  }
  target.valid = true;
  return target;
}

}  // namespace

bool creativeEditorWorldLayoutSourceCanRename(
    cr::CreativeWorldLayoutTable table) noexcept {
  switch (table) {
    case cr::CreativeWorldLayoutTable::Building:
    case cr::CreativeWorldLayoutTable::Level:
    case cr::CreativeWorldLayoutTable::Room:
    case cr::CreativeWorldLayoutTable::VerticalConnector:
    case cr::CreativeWorldLayoutTable::Box:
    case cr::CreativeWorldLayoutTable::Wall:
    case cr::CreativeWorldLayoutTable::Opening:
    case cr::CreativeWorldLayoutTable::Object:
      return true;
    case cr::CreativeWorldLayoutTable::None:
    case cr::CreativeWorldLayoutTable::TerrainProfile:
    case cr::CreativeWorldLayoutTable::TerrainPath:
    case cr::CreativeWorldLayoutTable::TerrainPathPoint:
      return false;
  }
  return false;
}

bool creativeEditorWorldLayoutSourceCanDuplicate(
    cr::CreativeWorldLayoutTable table) noexcept {
  return table == cr::CreativeWorldLayoutTable::Building ||
         table == cr::CreativeWorldLayoutTable::Level;
}

bool creativeEditorWorldLayoutSourceCanDelete(
    cr::CreativeWorldLayoutTable table) noexcept {
  return table != cr::CreativeWorldLayoutTable::None &&
         table != cr::CreativeWorldLayoutTable::TerrainPathPoint;
}

bool creativeEditorWorldLayoutSourceStableKeyMatches(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index,
    std::string_view stableKey) noexcept {
  return !stableKey.empty() && sourceStableKey(state, table, index) == stableKey;
}

CreativeEditorWorldLayoutEditReceipt renameCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index,
    std::string name) {
  std::string* current = sourceName(state, table, index);
  if (current == nullptr || !detail::hasVisibleWorldLayoutName(name)) {
    state.statusMessage = "source name is invalid";
    return {false, false,
            "creative_editor_world_layout_source_rename_invalid"};
  }
  if (*current == name) {
    state.statusMessage = "source name unchanged";
    return {true, false,
            "creative_editor_world_layout_source_rename_no_change"};
  }
  *current = std::move(name);
  detail::noteWorldLayoutSourceChange(state, "layout source renamed");
  return {true, true, "creative_editor_world_layout_source_renamed"};
}

CreativeEditorWorldLayoutEditReceipt duplicateCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index) {
  if (table == cr::CreativeWorldLayoutTable::Building) {
    std::int64_t deltaX = 0;
    std::int64_t deltaZ = 0;
    if (!defaultCreativeEditorWorldLayoutBuildingDuplicateOffset(
            state, index, deltaX, deltaZ)) {
      state.statusMessage = "building has no valid duplicate offset";
      return {false, false,
              "creative_editor_world_layout_source_duplicate_invalid"};
    }
    return duplicateCreativeEditorWorldLayoutBuilding(state, index, deltaX,
                                                       deltaZ);
  }
  if (table == cr::CreativeWorldLayoutTable::Level) {
    return applyCreativeEditorWorldLayoutLevelOperation(
        state, CreativeEditorWorldLayoutLevelOperation::Duplicate,
        cr::kInvalidCreativeWorldLayoutIndex, index);
  }
  state.statusMessage = "source type cannot be duplicated";
  return {false, false,
          "creative_editor_world_layout_source_duplicate_unsupported"};
}

CreativeEditorWorldLayoutEditReceipt deleteCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index) {
  if (table == cr::CreativeWorldLayoutTable::Building) {
    return deleteCreativeEditorWorldLayoutBuilding(state, index);
  }
  if (table == cr::CreativeWorldLayoutTable::Level) {
    return applyCreativeEditorWorldLayoutLevelOperation(
        state, CreativeEditorWorldLayoutLevelOperation::Delete,
        cr::kInvalidCreativeWorldLayoutIndex, index);
  }
  if (!creativeEditorWorldLayoutSourceCanDelete(table)) {
    state.statusMessage = "source type cannot be deleted";
    return {false, false,
            "creative_editor_world_layout_source_delete_unsupported"};
  }
  const CreativeEditorWorldLayoutEditReceipt focused =
      focusCreativeEditorWorldLayoutSource(state, table, index);
  if (!focused.accepted) {
    return focused;
  }
  return deleteCreativeEditorWorldLayoutSelection(state);
}

CreativeEditorWorldLayoutEditReceipt
focusCreativeEditorWorldLayoutSource(
    CreativeEditorWorldLayoutState& state, cr::CreativeWorldLayoutTable table,
    std::size_t index) {
  const SourceTarget target = resolveSourceTarget(state, table, index);
  if (!target.valid) {
    state.statusMessage = "layout source has no selectable symbol";
    return {false, false,
            "creative_editor_world_layout_source_target_invalid"};
  }

  const bool changed = state.tool != CreativeEditorWorldLayoutTool::Select ||
                       state.selection.kind != target.selection.kind ||
                       state.selection.index != target.selection.index ||
                       (target.activeLevelIndex !=
                            cr::kInvalidCreativeWorldLayoutIndex &&
                        state.activeLevelIndex != target.activeLevelIndex);
  detail::clearWorldLayoutInteraction(state);
  state.anchorActive = false;
  state.tool = CreativeEditorWorldLayoutTool::Select;
  state.selection = target.selection;
  if (target.activeLevelIndex < state.source.levels.size()) {
    state.activeLevelIndex = target.activeLevelIndex;
  } else if (target.buildingIndex < state.source.buildings.size()) {
    repairCreativeEditorWorldLayoutActiveLevel(state, target.buildingIndex);
  }
  if (target.hasCenter) {
    state.canvasPanX =
        -static_cast<float>(target.center.x) * state.canvasPixelsPerCell;
    state.canvasPanZ =
        -static_cast<float>(target.center.z) * state.canvasPixelsPerCell;
    const double horizontal =
        state.elevationAxis == CreativeEditorWorldLayoutElevationAxis::X
            ? target.center.x
            : target.center.z;
    state.elevationPanHorizontal =
        -static_cast<float>(horizontal) * state.elevationPixelsPerCell;
  }
  state.statusMessage = "layout source selected";
  return {true, changed,
          "creative_editor_world_layout_source_target_focused"};
}

}  // namespace iggy3d_creative_app
