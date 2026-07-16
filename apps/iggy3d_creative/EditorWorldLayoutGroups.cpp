#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy3d_creative_app {
namespace {

void includePoint(CreativeEditorWorldLayoutBuildingBounds& bounds,
                  cr::CreativeTerrainCoord2 point) noexcept {
  if (!bounds.valid) {
    bounds = {true, point, point};
    return;
  }
  bounds.minimum.x = std::min(bounds.minimum.x, point.x);
  bounds.minimum.z = std::min(bounds.minimum.z, point.z);
  bounds.maximum.x = std::max(bounds.maximum.x, point.x);
  bounds.maximum.z = std::max(bounds.maximum.z, point.z);
}

void includeRect(CreativeEditorWorldLayoutBuildingBounds& bounds,
                 cr::CreativeWorldLayoutRect rect) noexcept {
  includePoint(bounds, rect.minimum);
  includePoint(bounds, rect.maximum);
}

bool offsetPoint(cr::CreativeTerrainCoord2& point, std::int64_t deltaXCells,
                 std::int64_t deltaZCells) noexcept {
  cr::CreativeTerrainCoord2 moved;
  if (!detail::offsetWorldLayoutCoordinate(point.x, deltaXCells, moved.x) ||
      !detail::offsetWorldLayoutCoordinate(point.z, deltaZCells, moved.z)) {
    return false;
  }
  point = moved;
  return true;
}

bool offsetRect(cr::CreativeWorldLayoutRect& rect, std::int64_t deltaXCells,
                std::int64_t deltaZCells) noexcept {
  cr::CreativeWorldLayoutRect moved = rect;
  if (!offsetPoint(moved.minimum, deltaXCells, deltaZCells) ||
      !offsetPoint(moved.maximum, deltaXCells, deltaZCells)) {
    return false;
  }
  rect = moved;
  return true;
}

bool canOffsetPoint(cr::CreativeTerrainCoord2 point,
                    std::int64_t deltaXCells,
                    std::int64_t deltaZCells) noexcept {
  return offsetPoint(point, deltaXCells, deltaZCells);
}

bool canOffsetRect(cr::CreativeWorldLayoutRect rect,
                   std::int64_t deltaXCells,
                   std::int64_t deltaZCells) noexcept {
  return offsetRect(rect, deltaXCells, deltaZCells);
}

bool buildingOwnershipValid(const cr::CreativeWorldLayout& layout) noexcept {
  const auto validOwner = [&](const auto& symbol) {
    return symbol.buildingIndex < layout.buildings.size();
  };
  return std::all_of(layout.rooms.begin(), layout.rooms.end(), validOwner) &&
         std::all_of(layout.boxes.begin(), layout.boxes.end(), validOwner) &&
         std::all_of(layout.walls.begin(), layout.walls.end(), validOwner);
}

bool openingHostsValid(const cr::CreativeWorldLayout& layout) noexcept {
  for (const cr::CreativeWorldLayoutOpening& opening : layout.openings) {
    if (opening.hostKind ==
        cr::CreativeWorldLayoutOpeningHostKind::RoomEdge) {
      if (opening.roomIndex >= layout.rooms.size()) {
        return false;
      }
    } else if (opening.hostKind ==
               cr::CreativeWorldLayoutOpeningHostKind::Wall) {
      if (opening.wallIndex >= layout.walls.size()) {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}

bool offsetBuilding(cr::CreativeWorldLayout& layout,
                    std::size_t buildingIndex, std::int64_t deltaXCells,
                    std::int64_t deltaZCells) noexcept {
  if (buildingIndex >= layout.buildings.size()) {
    return false;
  }
  cr::CreativeWorldLayoutBuilding& building = layout.buildings[buildingIndex];
  if (building.rootMode != cr::CreativeBuildingRootMode::None &&
      !offsetRect(building.rootFootprint, deltaXCells, deltaZCells)) {
    return false;
  }
  for (cr::CreativeWorldLayoutRoom& room : layout.rooms) {
    if (room.buildingIndex == buildingIndex &&
        !offsetRect(room.footprint, deltaXCells, deltaZCells)) {
      return false;
    }
  }
  for (cr::CreativeWorldLayoutBox& box : layout.boxes) {
    if (box.buildingIndex == buildingIndex &&
        !offsetRect(box.footprint, deltaXCells, deltaZCells)) {
      return false;
    }
  }
  for (cr::CreativeWorldLayoutWall& wall : layout.walls) {
    if (wall.buildingIndex == buildingIndex &&
        (!offsetPoint(wall.start, deltaXCells, deltaZCells) ||
         !offsetPoint(wall.end, deltaXCells, deltaZCells))) {
      return false;
    }
  }
  return true;
}

bool canOffsetBuilding(const cr::CreativeWorldLayout& layout,
                       std::size_t buildingIndex,
                       std::int64_t deltaXCells,
                       std::int64_t deltaZCells) noexcept {
  if (buildingIndex >= layout.buildings.size()) {
    return false;
  }
  const cr::CreativeWorldLayoutBuilding& building =
      layout.buildings[buildingIndex];
  if (building.rootMode != cr::CreativeBuildingRootMode::None &&
      !canOffsetRect(building.rootFootprint, deltaXCells, deltaZCells)) {
    return false;
  }
  for (const cr::CreativeWorldLayoutRoom& room : layout.rooms) {
    if (room.buildingIndex == buildingIndex &&
        !canOffsetRect(room.footprint, deltaXCells, deltaZCells)) {
      return false;
    }
  }
  for (const cr::CreativeWorldLayoutBox& box : layout.boxes) {
    if (box.buildingIndex == buildingIndex &&
        !canOffsetRect(box.footprint, deltaXCells, deltaZCells)) {
      return false;
    }
  }
  for (const cr::CreativeWorldLayoutWall& wall : layout.walls) {
    if (wall.buildingIndex == buildingIndex &&
        (!canOffsetPoint(wall.start, deltaXCells, deltaZCells) ||
         !canOffsetPoint(wall.end, deltaXCells, deltaZCells))) {
      return false;
    }
  }
  return true;
}

bool containsPoint(CreativeEditorWorldLayoutBuildingBounds bounds,
                   CreativeEditorWorldLayoutPoint point,
                   double toleranceCells) noexcept {
  return bounds.valid && detail::finiteWorldLayoutPoint(point) &&
         std::isfinite(toleranceCells) && toleranceCells > 0.0 &&
         point.x >= bounds.minimum.x - toleranceCells &&
         point.x <= bounds.maximum.x + toleranceCells &&
         point.z >= bounds.minimum.z - toleranceCells &&
         point.z <= bounds.maximum.z + toleranceCells;
}

std::string copyName(std::string_view name) {
  return std::string(name) + " Copy";
}

}  // namespace

std::size_t creativeEditorWorldLayoutSelectedBuilding(
    const CreativeEditorWorldLayoutState& state) noexcept {
  const CreativeEditorWorldLayoutSelection selection = state.selection;
  if (selection.kind == CreativeEditorWorldLayoutSelectionKind::Building) {
    return selection.index < state.source.buildings.size()
               ? selection.index
               : cr::kInvalidCreativeWorldLayoutIndex;
  }
  if (selection.kind == CreativeEditorWorldLayoutSelectionKind::Room) {
    return selection.index < state.source.rooms.size()
               ? state.source.rooms[selection.index].buildingIndex
               : cr::kInvalidCreativeWorldLayoutIndex;
  }
  if (selection.kind == CreativeEditorWorldLayoutSelectionKind::Box) {
    return selection.index < state.source.boxes.size()
               ? state.source.boxes[selection.index].buildingIndex
               : cr::kInvalidCreativeWorldLayoutIndex;
  }
  if (selection.kind == CreativeEditorWorldLayoutSelectionKind::Wall) {
    return selection.index < state.source.walls.size()
               ? state.source.walls[selection.index].buildingIndex
               : cr::kInvalidCreativeWorldLayoutIndex;
  }
  if (selection.kind != CreativeEditorWorldLayoutSelectionKind::Opening ||
      selection.index >= state.source.openings.size()) {
    return cr::kInvalidCreativeWorldLayoutIndex;
  }
  const cr::CreativeWorldLayoutOpening& opening =
      state.source.openings[selection.index];
  if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall &&
      opening.wallIndex < state.source.walls.size()) {
    return state.source.walls[opening.wallIndex].buildingIndex;
  }
  if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
      opening.roomIndex < state.source.rooms.size()) {
    return state.source.rooms[opening.roomIndex].buildingIndex;
  }
  return cr::kInvalidCreativeWorldLayoutIndex;
}

bool readCreativeEditorWorldLayoutBuildingBounds(
    const CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    CreativeEditorWorldLayoutBuildingBounds& output) noexcept {
  output = {};
  if (buildingIndex >= state.source.buildings.size()) {
    return false;
  }
  const cr::CreativeWorldLayoutBuilding& building =
      state.source.buildings[buildingIndex];
  if (building.rootMode != cr::CreativeBuildingRootMode::None) {
    includeRect(output, building.rootFootprint);
  }
  for (const cr::CreativeWorldLayoutRoom& room : state.source.rooms) {
    if (room.buildingIndex == buildingIndex) {
      includeRect(output, room.footprint);
    }
  }
  for (const cr::CreativeWorldLayoutBox& box : state.source.boxes) {
    if (box.buildingIndex == buildingIndex) {
      includeRect(output, box.footprint);
    }
  }
  for (const cr::CreativeWorldLayoutWall& wall : state.source.walls) {
    if (wall.buildingIndex == buildingIndex) {
      includePoint(output, wall.start);
      includePoint(output, wall.end);
    }
  }
  return output.valid;
}

CreativeEditorWorldLayoutEditReceipt selectCreativeEditorWorldLayoutBuilding(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex) {
  CreativeEditorWorldLayoutBuildingBounds bounds;
  if (buildingIndex >= state.source.buildings.size() ||
      !readCreativeEditorWorldLayoutBuildingBounds(state, buildingIndex,
                                                   bounds)) {
    state.statusMessage = "building group is empty or unavailable";
    return {false, false,
            "creative_editor_world_layout_building_selection_invalid"};
  }
  const bool changed =
      state.selection.kind != CreativeEditorWorldLayoutSelectionKind::Building ||
      state.selection.index != buildingIndex;
  detail::clearWorldLayoutInteraction(state);
  state.anchorActive = false;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                     buildingIndex};
  state.statusMessage = "building selected";
  return {true, changed, "creative_editor_world_layout_building_selected"};
}

CreativeEditorWorldLayoutEditReceipt clearCreativeEditorWorldLayoutSelection(
    CreativeEditorWorldLayoutState& state) {
  const bool changed =
      state.selection.kind != CreativeEditorWorldLayoutSelectionKind::None ||
      state.anchorActive || state.buildingManipulation.active;
  detail::clearWorldLayoutInteraction(state);
  state.anchorActive = false;
  state.selection = {};
  state.statusMessage = "selection cleared";
  return {true, changed, "creative_editor_world_layout_selection_cleared"};
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBuildingManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBuildingManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  if (phase >= CreativeEditorWorldLayoutBuildingManipulationPhase::Count) {
    return {
        false, false,
        "creative_editor_world_layout_building_manipulation_phase_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutBuildingManipulationPhase::Cancel) {
    const bool changed = state.buildingManipulation.active;
    state.buildingManipulation = {};
    state.statusMessage = "building move cancelled";
    return {true, changed,
            "creative_editor_world_layout_building_manipulation_cancelled"};
  }
  if (state.tool != CreativeEditorWorldLayoutTool::Select) {
    return {false, false,
            "creative_editor_world_layout_building_manipulation_tool_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutBuildingManipulationPhase::Begin) {
    const std::size_t buildingIndex =
        creativeEditorWorldLayoutSelectedBuilding(state);
    CreativeEditorWorldLayoutBuildingBounds bounds;
    if (state.selection.kind !=
            CreativeEditorWorldLayoutSelectionKind::Building ||
        !readCreativeEditorWorldLayoutBuildingBounds(state, buildingIndex,
                                                     bounds) ||
        !containsPoint(bounds, point, toleranceCells)) {
      return {false, false,
              "creative_editor_world_layout_building_manipulation_target_missing"};
    }
    detail::clearWorldLayoutInteraction(state);
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                       buildingIndex};
    state.anchorActive = false;
    state.buildingManipulation = {
        true,
        state.revision,
        buildingIndex,
        point,
        bounds,
        0,
        0,
        true,
        "creative_editor_world_layout_building_manipulation_ready",
    };
    state.statusMessage = "drag building";
    return {true, true,
            "creative_editor_world_layout_building_manipulation_started"};
  }
  if (!state.buildingManipulation.active ||
      state.buildingManipulation.buildingIndex >=
          state.source.buildings.size()) {
    return {false, false,
            "creative_editor_world_layout_building_manipulation_not_active"};
  }
  if (state.revision != state.buildingManipulation.sourceRevision) {
    state.buildingManipulation = {};
    state.statusMessage = "building changed while drag was active";
    return {false, false,
            "creative_editor_world_layout_building_manipulation_stale"};
  }
  if (phase == CreativeEditorWorldLayoutBuildingManipulationPhase::Update) {
    std::int64_t deltaXCells = 0;
    std::int64_t deltaZCells = 0;
    const bool deltaValid = detail::snappedWorldLayoutPointerDelta(
                                point.x,
                                state.buildingManipulation.startPoint.x,
                                deltaXCells) &&
                            detail::snappedWorldLayoutPointerDelta(
                                point.z,
                                state.buildingManipulation.startPoint.z,
                                deltaZCells);
    if (deltaValid &&
        deltaXCells == state.buildingManipulation.previewDeltaXCells &&
        deltaZCells == state.buildingManipulation.previewDeltaZCells) {
      return {true, false, state.buildingManipulation.reasonCode};
    }
    const bool previewValid =
        deltaValid &&
        canOffsetBuilding(state.source,
                          state.buildingManipulation.buildingIndex,
                          deltaXCells, deltaZCells);
    state.buildingManipulation.previewDeltaXCells = deltaXCells;
    state.buildingManipulation.previewDeltaZCells = deltaZCells;
    state.buildingManipulation.previewValid = previewValid;
    state.buildingManipulation.reasonCode =
        previewValid
            ? "creative_editor_world_layout_building_manipulation_ready"
            : "creative_editor_world_layout_building_manipulation_out_of_range";
    state.statusMessage =
        previewValid ? "building move preview"
                     : "building move exceeds the layout coordinate range";
    return {true, true, state.buildingManipulation.reasonCode};
  }

  const CreativeEditorWorldLayoutEditReceipt updated =
      applyCreativeEditorWorldLayoutBuildingManipulation(
          state,
          CreativeEditorWorldLayoutBuildingManipulationPhase::Update, point,
          toleranceCells);
  if (!updated.accepted) {
    return updated;
  }
  if (!state.buildingManipulation.previewValid) {
    const std::string reasonCode = state.buildingManipulation.reasonCode;
    state.buildingManipulation = {};
    return {false, false, reasonCode};
  }
  const std::size_t buildingIndex =
      state.buildingManipulation.buildingIndex;
  const std::int64_t deltaXCells =
      state.buildingManipulation.previewDeltaXCells;
  const std::int64_t deltaZCells =
      state.buildingManipulation.previewDeltaZCells;
  state.buildingManipulation = {};
  if (deltaXCells == 0 && deltaZCells == 0) {
    state.statusMessage = "building unchanged";
    return {true, false,
            "creative_editor_world_layout_building_manipulation_no_change"};
  }
  cr::CreativeWorldLayout candidate = state.source;
  if (!offsetBuilding(candidate, buildingIndex, deltaXCells, deltaZCells)) {
    state.statusMessage = "building move exceeds the layout coordinate range";
    return {false, false,
            "creative_editor_world_layout_building_manipulation_out_of_range"};
  }
  state.source = std::move(candidate);
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                     buildingIndex};
  detail::noteWorldLayoutSourceChange(state, "building moved");
  return {true, true,
          "creative_editor_world_layout_building_manipulation_committed"};
}

bool defaultCreativeEditorWorldLayoutBuildingDuplicateOffset(
    const CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    std::int64_t& deltaXCells, std::int64_t& deltaZCells) noexcept {
  CreativeEditorWorldLayoutBuildingBounds bounds;
  if (!readCreativeEditorWorldLayoutBuildingBounds(state, buildingIndex,
                                                   bounds)) {
    return false;
  }
  const std::int64_t width =
      static_cast<std::int64_t>(bounds.maximum.x) - bounds.minimum.x;
  deltaXCells = std::max<std::int64_t>(width, 1) + 2;
  deltaZCells = 0;
  return canOffsetBuilding(state.source, buildingIndex, deltaXCells,
                           deltaZCells);
}

CreativeEditorWorldLayoutEditReceipt duplicateCreativeEditorWorldLayoutBuilding(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex,
    std::int64_t deltaXCells, std::int64_t deltaZCells) {
  CreativeEditorWorldLayoutBuildingBounds bounds;
  if (buildingIndex >= state.source.buildings.size() ||
      !readCreativeEditorWorldLayoutBuildingBounds(state, buildingIndex,
                                                   bounds) ||
      (deltaXCells == 0 && deltaZCells == 0) ||
      !buildingOwnershipValid(state.source) ||
      !openingHostsValid(state.source) ||
      !canOffsetBuilding(state.source, buildingIndex, deltaXCells,
                         deltaZCells)) {
    state.statusMessage = "building cannot be duplicated at that offset";
    return {false, false,
            "creative_editor_world_layout_building_duplicate_invalid"};
  }

  cr::CreativeWorldLayout candidate = state.source;
  std::uint64_t nextOrdinal = state.nextStableOrdinal;
  cr::CreativeWorldLayoutBuilding building =
      state.source.buildings[buildingIndex];
  building.stableKey =
      detail::mintWorldLayoutStableKey(candidate, nextOrdinal, "building");
  building.name = copyName(building.name);
  if (building.rootMode != cr::CreativeBuildingRootMode::None &&
      !offsetRect(building.rootFootprint, deltaXCells, deltaZCells)) {
    return {false, false,
            "creative_editor_world_layout_building_duplicate_out_of_range"};
  }
  const std::size_t duplicateBuildingIndex = candidate.buildings.size();
  candidate.buildings.push_back(std::move(building));

  std::vector<std::size_t> roomMap(
      state.source.rooms.size(), cr::kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < state.source.rooms.size(); ++index) {
    if (state.source.rooms[index].buildingIndex != buildingIndex) {
      continue;
    }
    cr::CreativeWorldLayoutRoom room = state.source.rooms[index];
    room.buildingIndex = duplicateBuildingIndex;
    room.stableKey =
        detail::mintWorldLayoutStableKey(candidate, nextOrdinal, "room");
    room.name = copyName(room.name);
    if (!offsetRect(room.footprint, deltaXCells, deltaZCells)) {
      return {false, false,
              "creative_editor_world_layout_building_duplicate_out_of_range"};
    }
    roomMap[index] = candidate.rooms.size();
    candidate.rooms.push_back(std::move(room));
  }

  for (const cr::CreativeWorldLayoutBox& sourceBox : state.source.boxes) {
    if (sourceBox.buildingIndex != buildingIndex) {
      continue;
    }
    cr::CreativeWorldLayoutBox box = sourceBox;
    box.buildingIndex = duplicateBuildingIndex;
    box.stableKey =
        detail::mintWorldLayoutStableKey(candidate, nextOrdinal, "floor");
    box.name = copyName(box.name);
    if (!offsetRect(box.footprint, deltaXCells, deltaZCells)) {
      return {false, false,
              "creative_editor_world_layout_building_duplicate_out_of_range"};
    }
    candidate.boxes.push_back(std::move(box));
  }

  std::vector<std::size_t> wallMap(
      state.source.walls.size(), cr::kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < state.source.walls.size(); ++index) {
    if (state.source.walls[index].buildingIndex != buildingIndex) {
      continue;
    }
    cr::CreativeWorldLayoutWall wall = state.source.walls[index];
    wall.buildingIndex = duplicateBuildingIndex;
    wall.stableKey =
        detail::mintWorldLayoutStableKey(candidate, nextOrdinal, "wall");
    wall.name = copyName(wall.name);
    if (!offsetPoint(wall.start, deltaXCells, deltaZCells) ||
        !offsetPoint(wall.end, deltaXCells, deltaZCells)) {
      return {false, false,
              "creative_editor_world_layout_building_duplicate_out_of_range"};
    }
    wallMap[index] = candidate.walls.size();
    candidate.walls.push_back(std::move(wall));
  }

  for (const cr::CreativeWorldLayoutOpening& sourceOpening :
       state.source.openings) {
    cr::CreativeWorldLayoutOpening opening = sourceOpening;
    bool owned = false;
    if (opening.hostKind ==
        cr::CreativeWorldLayoutOpeningHostKind::RoomEdge) {
      owned = roomMap[opening.roomIndex] !=
              cr::kInvalidCreativeWorldLayoutIndex;
      if (owned) {
        opening.roomIndex = roomMap[opening.roomIndex];
      }
    } else {
      owned = wallMap[opening.wallIndex] !=
              cr::kInvalidCreativeWorldLayoutIndex;
      if (owned) {
        opening.wallIndex = wallMap[opening.wallIndex];
      }
    }
    if (!owned) {
      continue;
    }
    opening.stableKey = detail::mintWorldLayoutStableKey(
        candidate, nextOrdinal,
        opening.kind == cr::CreativeBuildingOpeningKind::Door ? "door"
                                                              : "window");
    opening.name = copyName(opening.name);
    candidate.openings.push_back(std::move(opening));
  }

  state.source = std::move(candidate);
  state.nextStableOrdinal = nextOrdinal;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                     duplicateBuildingIndex};
  detail::noteWorldLayoutSourceChange(state, "building duplicated");
  return {true, true,
          "creative_editor_world_layout_building_duplicated"};
}

CreativeEditorWorldLayoutEditReceipt deleteCreativeEditorWorldLayoutBuilding(
    CreativeEditorWorldLayoutState& state, std::size_t buildingIndex) {
  if (buildingIndex >= state.source.buildings.size() ||
      !buildingOwnershipValid(state.source) ||
      !openingHostsValid(state.source)) {
    state.statusMessage = "building cannot be deleted";
    return {false, false,
            "creative_editor_world_layout_building_delete_invalid"};
  }

  cr::CreativeWorldLayout candidate = state.source;
  candidate.buildings.erase(
      candidate.buildings.begin() + static_cast<std::ptrdiff_t>(buildingIndex));

  std::vector<std::size_t> roomMap(
      state.source.rooms.size(), cr::kInvalidCreativeWorldLayoutIndex);
  candidate.rooms.clear();
  candidate.rooms.reserve(state.source.rooms.size());
  for (std::size_t index = 0U; index < state.source.rooms.size(); ++index) {
    cr::CreativeWorldLayoutRoom room = state.source.rooms[index];
    if (room.buildingIndex == buildingIndex) {
      continue;
    }
    if (room.buildingIndex > buildingIndex) {
      --room.buildingIndex;
    }
    roomMap[index] = candidate.rooms.size();
    candidate.rooms.push_back(std::move(room));
  }

  candidate.boxes.clear();
  candidate.boxes.reserve(state.source.boxes.size());
  for (cr::CreativeWorldLayoutBox box : state.source.boxes) {
    if (box.buildingIndex == buildingIndex) {
      continue;
    }
    if (box.buildingIndex > buildingIndex) {
      --box.buildingIndex;
    }
    candidate.boxes.push_back(std::move(box));
  }

  std::vector<std::size_t> wallMap(
      state.source.walls.size(), cr::kInvalidCreativeWorldLayoutIndex);
  candidate.walls.clear();
  candidate.walls.reserve(state.source.walls.size());
  for (std::size_t index = 0U; index < state.source.walls.size(); ++index) {
    cr::CreativeWorldLayoutWall wall = state.source.walls[index];
    if (wall.buildingIndex == buildingIndex) {
      continue;
    }
    if (wall.buildingIndex > buildingIndex) {
      --wall.buildingIndex;
    }
    wallMap[index] = candidate.walls.size();
    candidate.walls.push_back(std::move(wall));
  }

  candidate.openings.clear();
  candidate.openings.reserve(state.source.openings.size());
  for (cr::CreativeWorldLayoutOpening opening : state.source.openings) {
    if (opening.hostKind ==
        cr::CreativeWorldLayoutOpeningHostKind::RoomEdge) {
      const std::size_t mappedRoom = roomMap[opening.roomIndex];
      if (mappedRoom == cr::kInvalidCreativeWorldLayoutIndex) {
        continue;
      }
      opening.roomIndex = mappedRoom;
    } else {
      const std::size_t mappedWall = wallMap[opening.wallIndex];
      if (mappedWall == cr::kInvalidCreativeWorldLayoutIndex) {
        continue;
      }
      opening.wallIndex = mappedWall;
    }
    candidate.openings.push_back(std::move(opening));
  }

  state.source = std::move(candidate);
  state.selection = {};
  detail::noteWorldLayoutSourceChange(state, "building deleted");
  return {true, true, "creative_editor_world_layout_building_deleted"};
}

}  // namespace iggy3d_creative_app
