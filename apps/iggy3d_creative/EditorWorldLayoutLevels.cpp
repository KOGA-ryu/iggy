#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] std::size_t firstLevelForBuilding(
    const cr::CreativeWorldLayout& layout,
    std::size_t buildingIndex) noexcept {
  for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
    if (layout.levels[index].buildingIndex == buildingIndex) {
      return index;
    }
  }
  return cr::kInvalidCreativeWorldLayoutIndex;
}

[[nodiscard]] std::size_t levelCountForBuilding(
    const cr::CreativeWorldLayout& layout,
    std::size_t buildingIndex) noexcept {
  return static_cast<std::size_t>(std::count_if(
      layout.levels.begin(), layout.levels.end(),
      [buildingIndex](const cr::CreativeWorldLayoutLevel& level) {
        return level.buildingIndex == buildingIndex;
      }));
}

[[nodiscard]] std::size_t resolvedLevelIndex(
    const CreativeEditorWorldLayoutState& state,
    std::size_t requested) noexcept {
  if (requested < state.source.levels.size()) {
    return requested;
  }
  return state.activeLevelIndex < state.source.levels.size()
             ? state.activeLevelIndex
             : cr::kInvalidCreativeWorldLayoutIndex;
}

[[nodiscard]] bool nextLevelElevation(
    const cr::CreativeWorldLayout& layout, std::size_t buildingIndex,
    double& output) noexcept {
  bool found = false;
  long double highest = 0.0L;
  for (const cr::CreativeWorldLayoutLevel& level : layout.levels) {
    if (level.buildingIndex != buildingIndex ||
        !std::isfinite(level.floorTopLayer) || level.wallHeightCells == 0U) {
      continue;
    }
    const long double top = static_cast<long double>(level.floorTopLayer) +
                            level.wallHeightCells;
    if (!found || top > highest) {
      found = true;
      highest = top;
    }
  }
  if (!found || highest < -std::numeric_limits<double>::max() ||
      highest > std::numeric_limits<double>::max()) {
    return false;
  }
  output = static_cast<double>(highest);
  return std::isfinite(output);
}

void selectLevelOwner(CreativeEditorWorldLayoutState& state,
                      std::size_t levelIndex) {
  const std::size_t buildingIndex =
      state.source.levels[levelIndex].buildingIndex;
  bool selectionVisible = false;
  if (state.selection.kind == CreativeEditorWorldLayoutSelectionKind::Room &&
      state.selection.index < state.source.rooms.size()) {
    selectionVisible =
        state.source.rooms[state.selection.index].levelIndex == levelIndex;
  } else if (state.selection.kind ==
                 CreativeEditorWorldLayoutSelectionKind::Opening &&
             state.selection.index < state.source.openings.size()) {
    const cr::CreativeWorldLayoutOpening& opening =
        state.source.openings[state.selection.index];
    selectionVisible =
        opening.hostKind != cr::CreativeWorldLayoutOpeningHostKind::RoomEdge ||
        (opening.roomIndex < state.source.rooms.size() &&
         state.source.rooms[opening.roomIndex].levelIndex == levelIndex);
  } else if (state.selection.kind ==
                 CreativeEditorWorldLayoutSelectionKind::VerticalConnector &&
             state.selection.index < state.source.verticalConnectors.size()) {
    const cr::CreativeWorldLayoutVerticalConnector& connector =
        state.source.verticalConnectors[state.selection.index];
    selectionVisible =
        (connector.lowerRoomIndex < state.source.rooms.size() &&
         state.source.rooms[connector.lowerRoomIndex].levelIndex ==
             levelIndex) ||
        (connector.upperRoomIndex < state.source.rooms.size() &&
         state.source.rooms[connector.upperRoomIndex].levelIndex == levelIndex);
  } else if (state.selection.kind ==
                 CreativeEditorWorldLayoutSelectionKind::Building &&
             state.selection.index == buildingIndex) {
    selectionVisible = true;
  } else if (state.selection.kind !=
                 CreativeEditorWorldLayoutSelectionKind::Room &&
             state.selection.kind !=
                 CreativeEditorWorldLayoutSelectionKind::Opening &&
             state.selection.kind !=
                 CreativeEditorWorldLayoutSelectionKind::VerticalConnector) {
    selectionVisible = true;
  }
  if (!selectionVisible) {
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                       buildingIndex};
  }
  state.activeLevelIndex = levelIndex;
}

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt selectLevel(
    CreativeEditorWorldLayoutState& state, std::size_t levelIndex) {
  if (levelIndex >= state.source.levels.size()) {
    state.statusMessage = "building level is unavailable";
    return {false, false,
            "creative_editor_world_layout_level_selection_invalid"};
  }
  const bool changed = state.activeLevelIndex != levelIndex;
  detail::clearWorldLayoutInteraction(state);
  state.anchorActive = false;
  selectLevelOwner(state, levelIndex);
  state.statusMessage = state.source.levels[levelIndex].name + " active";
  return {true, changed,
          "creative_editor_world_layout_level_selected"};
}

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt addLevel(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex) {
  if (buildingIndex >= state.source.buildings.size()) {
    state.statusMessage = "select a building before adding a level";
    return {false, false,
            "creative_editor_world_layout_level_building_invalid"};
  }
  const std::size_t first = firstLevelForBuilding(state.source, buildingIndex);
  if (first == cr::kInvalidCreativeWorldLayoutIndex) {
    const cr::CreativeWorldLayoutBuilding& building =
        state.source.buildings[buildingIndex];
    cr::CreativeWorldLayoutLevel level;
    level.buildingIndex = buildingIndex;
    level.stableKey = detail::mintWorldLayoutStableKey(state, "level");
    level.name = "Level 0";
    level.floorTopLayer = static_cast<double>(building.rootBaseLayer);
    level.wallHeightCells =
        building.rootHeightCells > 0U
            ? building.rootHeightCells
            : cr::kDefaultCreativeWorldLayoutWallHeightCells;
    state.source.levels.push_back(std::move(level));
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                       buildingIndex};
    state.activeLevelIndex = state.source.levels.size() - 1U;
    detail::noteWorldLayoutSourceChange(state, "building level added");
    return {true, true, "creative_editor_world_layout_level_added"};
  }
  const std::size_t sourceIndex =
      state.activeLevelIndex < state.source.levels.size() &&
              state.source.levels[state.activeLevelIndex].buildingIndex ==
                  buildingIndex
          ? state.activeLevelIndex
          : first;
  double floorTopLayer = 0.0;
  if (!nextLevelElevation(state.source, buildingIndex, floorTopLayer)) {
    state.statusMessage = "next level elevation is not representable";
    return {false, false,
            "creative_editor_world_layout_level_elevation_invalid"};
  }

  cr::CreativeWorldLayoutLevel level = state.source.levels[sourceIndex];
  level.stableKey = detail::mintWorldLayoutStableKey(state, "level");
  level.name = "Level " +
               std::to_string(levelCountForBuilding(state.source,
                                                     buildingIndex));
  level.floorTopLayer = floorTopLayer;
  const std::size_t levelIndex = state.source.levels.size();
  state.source.levels.push_back(std::move(level));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                     buildingIndex};
  state.activeLevelIndex = levelIndex;
  detail::noteWorldLayoutSourceChange(state, "building level added");
  return {true, true, "creative_editor_world_layout_level_added"};
}

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt duplicateLevel(
    CreativeEditorWorldLayoutState& state, std::size_t levelIndex) {
  if (levelIndex >= state.source.levels.size()) {
    state.statusMessage = "select a level before duplicating it";
    return {false, false,
            "creative_editor_world_layout_level_duplicate_invalid"};
  }
  const std::size_t buildingIndex =
      state.source.levels[levelIndex].buildingIndex;
  double floorTopLayer = 0.0;
  if (!nextLevelElevation(state.source, buildingIndex, floorTopLayer)) {
    state.statusMessage = "duplicated level elevation is not representable";
    return {false, false,
            "creative_editor_world_layout_level_elevation_invalid"};
  }

  const std::size_t originalRoomCount = state.source.rooms.size();
  const std::size_t originalOpeningCount = state.source.openings.size();
  std::vector<std::size_t> roomMap(
      originalRoomCount, cr::kInvalidCreativeWorldLayoutIndex);
  cr::CreativeWorldLayoutLevel level = state.source.levels[levelIndex];
  level.stableKey = detail::mintWorldLayoutStableKey(state, "level");
  level.name += " Copy";
  level.floorTopLayer = floorTopLayer;
  const std::size_t duplicateLevelIndex = state.source.levels.size();
  state.source.levels.push_back(std::move(level));

  for (std::size_t index = 0U; index < originalRoomCount; ++index) {
    if (state.source.rooms[index].levelIndex != levelIndex) {
      continue;
    }
    cr::CreativeWorldLayoutRoom room = state.source.rooms[index];
    room.levelIndex = duplicateLevelIndex;
    room.stableKey = detail::mintWorldLayoutStableKey(state, "room");
    room.name += " Copy";
    roomMap[index] = state.source.rooms.size();
    state.source.rooms.push_back(std::move(room));
  }
  for (std::size_t index = 0U; index < originalOpeningCount; ++index) {
    const cr::CreativeWorldLayoutOpening& source =
        state.source.openings[index];
    if (source.hostKind != cr::CreativeWorldLayoutOpeningHostKind::RoomEdge ||
        source.roomIndex >= roomMap.size() ||
        roomMap[source.roomIndex] == cr::kInvalidCreativeWorldLayoutIndex) {
      continue;
    }
    cr::CreativeWorldLayoutOpening opening = source;
    opening.roomIndex = roomMap[source.roomIndex];
    opening.stableKey = detail::mintWorldLayoutStableKey(state, "opening");
    opening.name += " Copy";
    state.source.openings.push_back(std::move(opening));
  }

  state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                     buildingIndex};
  state.activeLevelIndex = duplicateLevelIndex;
  detail::noteWorldLayoutSourceChange(state, "building level duplicated");
  return {true, true, "creative_editor_world_layout_level_duplicated"};
}

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt moveLevel(
    CreativeEditorWorldLayoutState& state, std::size_t levelIndex,
    bool later) {
  if (levelIndex >= state.source.levels.size()) {
    return {false, false,
            "creative_editor_world_layout_level_reorder_invalid"};
  }
  const std::size_t buildingIndex =
      state.source.levels[levelIndex].buildingIndex;
  std::size_t otherIndex = cr::kInvalidCreativeWorldLayoutIndex;
  if (later) {
    for (std::size_t index = levelIndex + 1U; index < state.source.levels.size();
         ++index) {
      if (state.source.levels[index].buildingIndex == buildingIndex) {
        otherIndex = index;
        break;
      }
    }
  } else {
    for (std::size_t index = levelIndex; index > 0U; --index) {
      if (state.source.levels[index - 1U].buildingIndex == buildingIndex) {
        otherIndex = index - 1U;
        break;
      }
    }
  }
  if (otherIndex == cr::kInvalidCreativeWorldLayoutIndex) {
    state.statusMessage = later ? "level is already last" :
                                  "level is already first";
    return {true, false,
            "creative_editor_world_layout_level_reorder_no_change"};
  }
  std::swap(state.source.levels[levelIndex],
            state.source.levels[otherIndex]);
  for (cr::CreativeWorldLayoutRoom& room : state.source.rooms) {
    if (room.levelIndex == levelIndex) {
      room.levelIndex = otherIndex;
    } else if (room.levelIndex == otherIndex) {
      room.levelIndex = levelIndex;
    }
  }
  state.activeLevelIndex = otherIndex;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                     buildingIndex};
  detail::noteWorldLayoutSourceChange(state, "building levels reordered");
  return {true, true, "creative_editor_world_layout_level_reordered"};
}

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt deleteLevel(
    CreativeEditorWorldLayoutState& state, std::size_t levelIndex) {
  if (levelIndex >= state.source.levels.size()) {
    state.statusMessage = "select a level before deleting it";
    return {false, false,
            "creative_editor_world_layout_level_delete_invalid"};
  }
  const std::size_t buildingIndex =
      state.source.levels[levelIndex].buildingIndex;
  if (levelCountForBuilding(state.source, buildingIndex) <= 1U) {
    state.statusMessage = "a building must retain one level";
    return {false, false,
            "creative_editor_world_layout_level_delete_last"};
  }

  std::vector<std::size_t> roomMap(
      state.source.rooms.size(), cr::kInvalidCreativeWorldLayoutIndex);
  std::vector<cr::CreativeWorldLayoutRoom> rooms;
  rooms.reserve(state.source.rooms.size());
  for (std::size_t index = 0U; index < state.source.rooms.size(); ++index) {
    cr::CreativeWorldLayoutRoom room = state.source.rooms[index];
    if (room.levelIndex == levelIndex) {
      continue;
    }
    if (room.levelIndex > levelIndex) {
      --room.levelIndex;
    }
    roomMap[index] = rooms.size();
    rooms.push_back(std::move(room));
  }
  std::vector<cr::CreativeWorldLayoutOpening> openings;
  openings.reserve(state.source.openings.size());
  for (cr::CreativeWorldLayoutOpening opening : state.source.openings) {
    if (opening.hostKind ==
        cr::CreativeWorldLayoutOpeningHostKind::RoomEdge) {
      if (opening.roomIndex >= roomMap.size() ||
          roomMap[opening.roomIndex] ==
              cr::kInvalidCreativeWorldLayoutIndex) {
        continue;
      }
      opening.roomIndex = roomMap[opening.roomIndex];
    }
    openings.push_back(std::move(opening));
  }
  std::vector<cr::CreativeWorldLayoutVerticalConnector> verticalConnectors;
  verticalConnectors.reserve(state.source.verticalConnectors.size());
  for (cr::CreativeWorldLayoutVerticalConnector connector :
       state.source.verticalConnectors) {
    if (connector.lowerRoomIndex >= roomMap.size() ||
        connector.upperRoomIndex >= roomMap.size() ||
        roomMap[connector.lowerRoomIndex] ==
            cr::kInvalidCreativeWorldLayoutIndex ||
        roomMap[connector.upperRoomIndex] ==
            cr::kInvalidCreativeWorldLayoutIndex) {
      continue;
    }
    connector.lowerRoomIndex = roomMap[connector.lowerRoomIndex];
    connector.upperRoomIndex = roomMap[connector.upperRoomIndex];
    verticalConnectors.push_back(std::move(connector));
  }
  state.source.rooms = std::move(rooms);
  state.source.openings = std::move(openings);
  state.source.verticalConnectors = std::move(verticalConnectors);
  state.source.levels.erase(
      state.source.levels.begin() + static_cast<std::ptrdiff_t>(levelIndex));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                     buildingIndex};
  state.activeLevelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  repairCreativeEditorWorldLayoutActiveLevel(state, buildingIndex);
  detail::noteWorldLayoutSourceChange(state, "building level deleted");
  return {true, true, "creative_editor_world_layout_level_deleted"};
}

}  // namespace

void repairCreativeEditorWorldLayoutActiveLevel(
    CreativeEditorWorldLayoutState& state,
    std::size_t preferredBuildingIndex) noexcept {
  if (state.activeLevelIndex < state.source.levels.size() &&
      (preferredBuildingIndex == cr::kInvalidCreativeWorldLayoutIndex ||
       state.source.levels[state.activeLevelIndex].buildingIndex ==
           preferredBuildingIndex)) {
    return;
  }
  state.activeLevelIndex =
      preferredBuildingIndex < state.source.buildings.size()
          ? firstLevelForBuilding(state.source, preferredBuildingIndex)
          : cr::kInvalidCreativeWorldLayoutIndex;
  if (preferredBuildingIndex == cr::kInvalidCreativeWorldLayoutIndex &&
      !state.source.levels.empty()) {
    state.activeLevelIndex = 0U;
  }
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutLevelOperation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutLevelOperation operation,
    std::size_t buildingIndex, std::size_t levelIndex) {
  switch (operation) {
    case CreativeEditorWorldLayoutLevelOperation::Select:
      return selectLevel(state, levelIndex);
    case CreativeEditorWorldLayoutLevelOperation::Add:
      return addLevel(state, buildingIndex);
    case CreativeEditorWorldLayoutLevelOperation::Duplicate:
      return duplicateLevel(state, resolvedLevelIndex(state, levelIndex));
    case CreativeEditorWorldLayoutLevelOperation::MoveEarlier:
      return moveLevel(state, resolvedLevelIndex(state, levelIndex), false);
    case CreativeEditorWorldLayoutLevelOperation::MoveLater:
      return moveLevel(state, resolvedLevelIndex(state, levelIndex), true);
    case CreativeEditorWorldLayoutLevelOperation::Delete:
      return deleteLevel(state, resolvedLevelIndex(state, levelIndex));
    case CreativeEditorWorldLayoutLevelOperation::Count:
      break;
  }
  return {false, false,
          "creative_editor_world_layout_level_operation_invalid"};
}

}  // namespace iggy3d_creative_app
