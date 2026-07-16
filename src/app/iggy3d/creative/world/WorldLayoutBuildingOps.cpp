#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

void setFailure(CreativeWorldLayoutBuildingEditResult& result,
                CreativeWorldLayoutBuildingEditStatus status,
                std::string reasonCode) {
  result.accepted = false;
  result.changed = false;
  result.status = status;
  result.edited = {};
  result.reasonCode = std::move(reasonCode);
}

void setReady(CreativeWorldLayoutBuildingEditResult& result,
              bool changed,
              CreativeWorldLayout edited,
              std::string reasonCode) {
  result.accepted = true;
  result.changed = changed;
  result.status = changed ? CreativeWorldLayoutBuildingEditStatus::Ready
                          : CreativeWorldLayoutBuildingEditStatus::NoChange;
  result.edited = std::move(edited);
  result.reasonCode = std::move(reasonCode);
}

void includePoint(CreativeWorldLayoutBuildingBounds& bounds,
                  CreativeTerrainCoord2 point) noexcept {
  if (!bounds.valid) {
    bounds = {true, point, point};
    return;
  }
  bounds.minimum.x = std::min(bounds.minimum.x, point.x);
  bounds.minimum.z = std::min(bounds.minimum.z, point.z);
  bounds.maximum.x = std::max(bounds.maximum.x, point.x);
  bounds.maximum.z = std::max(bounds.maximum.z, point.z);
}

void includeRect(CreativeWorldLayoutBuildingBounds& bounds,
                 CreativeWorldLayoutRect rect) noexcept {
  includePoint(bounds, rect.minimum);
  includePoint(bounds, rect.maximum);
}

bool offsetCoordinate(std::int32_t value,
                      std::int64_t delta,
                      std::int32_t& output) noexcept {
  const std::int64_t minimumDelta =
      static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min()) -
      value;
  const std::int64_t maximumDelta =
      static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max()) -
      value;
  if (delta < minimumDelta || delta > maximumDelta) {
    return false;
  }
  output = static_cast<std::int32_t>(static_cast<std::int64_t>(value) + delta);
  return true;
}

bool offsetPoint(CreativeTerrainCoord2& point,
                 std::int64_t deltaXCells,
                 std::int64_t deltaZCells) noexcept {
  CreativeTerrainCoord2 moved;
  if (!offsetCoordinate(point.x, deltaXCells, moved.x) ||
      !offsetCoordinate(point.z, deltaZCells, moved.z)) {
    return false;
  }
  point = moved;
  return true;
}

bool offsetRect(CreativeWorldLayoutRect& rect,
                std::int64_t deltaXCells,
                std::int64_t deltaZCells) noexcept {
  CreativeWorldLayoutRect moved = rect;
  if (!offsetPoint(moved.minimum, deltaXCells, deltaZCells) ||
      !offsetPoint(moved.maximum, deltaXCells, deltaZCells)) {
    return false;
  }
  rect = moved;
  return true;
}

bool offsetBuilding(CreativeWorldLayout& layout,
                    std::size_t buildingIndex,
                    std::int64_t deltaXCells,
                    std::int64_t deltaZCells) noexcept {
  CreativeWorldLayoutBuilding& building = layout.buildings[buildingIndex];
  if (building.rootMode != CreativeBuildingRootMode::None &&
      !offsetRect(building.rootFootprint, deltaXCells, deltaZCells)) {
    return false;
  }
  for (CreativeWorldLayoutRoom& room : layout.rooms) {
    if (room.buildingIndex == buildingIndex &&
        !offsetRect(room.footprint, deltaXCells, deltaZCells)) {
      return false;
    }
  }
  for (CreativeWorldLayoutBox& box : layout.boxes) {
    if (box.buildingIndex == buildingIndex &&
        !offsetRect(box.footprint, deltaXCells, deltaZCells)) {
      return false;
    }
  }
  for (CreativeWorldLayoutWall& wall : layout.walls) {
    if (wall.buildingIndex == buildingIndex &&
        (!offsetPoint(wall.start, deltaXCells, deltaZCells) ||
         !offsetPoint(wall.end, deltaXCells, deltaZCells))) {
      return false;
    }
  }
  return true;
}

}  // namespace

std::string_view toString(
    CreativeWorldLayoutBuildingEditStatus status) noexcept {
  switch (status) {
    case CreativeWorldLayoutBuildingEditStatus::NotRequested:
      return "NotRequested";
    case CreativeWorldLayoutBuildingEditStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeWorldLayoutBuildingEditStatus::InvalidOwnership:
      return "InvalidOwnership";
    case CreativeWorldLayoutBuildingEditStatus::EmptyBuilding:
      return "EmptyBuilding";
    case CreativeWorldLayoutBuildingEditStatus::NoChange:
      return "NoChange";
    case CreativeWorldLayoutBuildingEditStatus::CoordinateOverflow:
      return "CoordinateOverflow";
    case CreativeWorldLayoutBuildingEditStatus::Ready:
      return "Ready";
  }
  return "Invalid";
}

bool validCreativeWorldLayoutBuildingOwnership(
    const CreativeWorldLayout& layout) noexcept {
  const auto validOwner = [&](const auto& symbol) {
    return symbol.buildingIndex < layout.buildings.size();
  };
  if (!std::all_of(layout.rooms.begin(), layout.rooms.end(), validOwner) ||
      !std::all_of(layout.boxes.begin(), layout.boxes.end(), validOwner) ||
      !std::all_of(layout.walls.begin(), layout.walls.end(), validOwner)) {
    return false;
  }
  return std::all_of(
      layout.openings.begin(), layout.openings.end(),
      [&](const CreativeWorldLayoutOpening& opening) {
        if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::Wall) {
          return opening.wallIndex < layout.walls.size();
        }
        return opening.hostKind ==
                   CreativeWorldLayoutOpeningHostKind::RoomEdge &&
               opening.roomIndex < layout.rooms.size() &&
               opening.roomEdge < CreativeWorldLayoutRoomEdge::Count;
      });
}

bool measureCreativeWorldLayoutBuildingBounds(
    const CreativeWorldLayout& layout,
    std::size_t buildingIndex,
    CreativeWorldLayoutBuildingBounds& output) noexcept {
  output = {};
  if (buildingIndex >= layout.buildings.size()) {
    return false;
  }
  const CreativeWorldLayoutBuilding& building = layout.buildings[buildingIndex];
  if (building.rootMode != CreativeBuildingRootMode::None) {
    includeRect(output, building.rootFootprint);
  }
  for (const CreativeWorldLayoutRoom& room : layout.rooms) {
    if (room.buildingIndex == buildingIndex) {
      includeRect(output, room.footprint);
    }
  }
  for (const CreativeWorldLayoutBox& box : layout.boxes) {
    if (box.buildingIndex == buildingIndex) {
      includeRect(output, box.footprint);
    }
  }
  for (const CreativeWorldLayoutWall& wall : layout.walls) {
    if (wall.buildingIndex == buildingIndex) {
      includePoint(output, wall.start);
      includePoint(output, wall.end);
    }
  }
  return output.valid;
}

bool canMoveCreativeWorldLayoutBuilding(
    const CreativeWorldLayout& layout,
    const CreativeWorldLayoutBuildingMoveRequest& request) noexcept {
  if (request.buildingIndex >= layout.buildings.size()) {
    return false;
  }
  const CreativeWorldLayoutBuilding& building =
      layout.buildings[request.buildingIndex];
  if (building.rootMode != CreativeBuildingRootMode::None) {
    CreativeWorldLayoutRect footprint = building.rootFootprint;
    if (!offsetRect(footprint, request.deltaXCells, request.deltaZCells)) {
      return false;
    }
  }
  for (const CreativeWorldLayoutRoom& room : layout.rooms) {
    if (room.buildingIndex == request.buildingIndex) {
      CreativeWorldLayoutRect footprint = room.footprint;
      if (!offsetRect(footprint, request.deltaXCells, request.deltaZCells)) {
        return false;
      }
    }
  }
  for (const CreativeWorldLayoutBox& box : layout.boxes) {
    if (box.buildingIndex == request.buildingIndex) {
      CreativeWorldLayoutRect footprint = box.footprint;
      if (!offsetRect(footprint, request.deltaXCells, request.deltaZCells)) {
        return false;
      }
    }
  }
  for (const CreativeWorldLayoutWall& wall : layout.walls) {
    if (wall.buildingIndex == request.buildingIndex) {
      CreativeTerrainCoord2 start = wall.start;
      CreativeTerrainCoord2 end = wall.end;
      if (!offsetPoint(start, request.deltaXCells, request.deltaZCells) ||
          !offsetPoint(end, request.deltaXCells, request.deltaZCells)) {
        return false;
      }
    }
  }
  return true;
}

bool defaultCreativeWorldLayoutBuildingDuplicateOffset(
    const CreativeWorldLayout& layout,
    std::size_t buildingIndex,
    std::int64_t& deltaXCells,
    std::int64_t& deltaZCells) noexcept {
  CreativeWorldLayoutBuildingBounds bounds;
  if (!measureCreativeWorldLayoutBuildingBounds(layout, buildingIndex,
                                                bounds)) {
    return false;
  }
  const std::int64_t width =
      static_cast<std::int64_t>(bounds.maximum.x) - bounds.minimum.x;
  deltaXCells = std::max<std::int64_t>(width, 1) + 2;
  deltaZCells = 0;
  return canMoveCreativeWorldLayoutBuilding(
      layout, {buildingIndex, deltaXCells, deltaZCells});
}

CreativeWorldLayoutBuildingEditResult moveCreativeWorldLayoutBuilding(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutBuildingMoveRequest& request) {
  CreativeWorldLayoutBuildingEditResult result;
  result.requested = true;
  result.sourceBuildingIndex = request.buildingIndex;
  result.resultBuildingIndex = request.buildingIndex;
  if (request.buildingIndex >= source.buildings.size()) {
    setFailure(result, CreativeWorldLayoutBuildingEditStatus::InvalidRequest,
               "creative_world_layout_building_move_request_invalid");
    return result;
  }
  if (!validCreativeWorldLayoutBuildingOwnership(source)) {
    setFailure(result, CreativeWorldLayoutBuildingEditStatus::InvalidOwnership,
               "creative_world_layout_building_move_ownership_invalid");
    return result;
  }
  CreativeWorldLayoutBuildingBounds bounds;
  if (!measureCreativeWorldLayoutBuildingBounds(source, request.buildingIndex,
                                                bounds)) {
    setFailure(result, CreativeWorldLayoutBuildingEditStatus::EmptyBuilding,
               "creative_world_layout_building_move_empty");
    return result;
  }
  if (request.deltaXCells == 0 && request.deltaZCells == 0) {
    setReady(result, false, source,
             "creative_world_layout_building_move_no_change");
    return result;
  }
  if (!canMoveCreativeWorldLayoutBuilding(source, request)) {
    setFailure(result,
               CreativeWorldLayoutBuildingEditStatus::CoordinateOverflow,
               "creative_world_layout_building_move_coordinate_overflow");
    return result;
  }
  CreativeWorldLayout edited = source;
  if (!offsetBuilding(edited, request.buildingIndex, request.deltaXCells,
                      request.deltaZCells)) {
    setFailure(result,
               CreativeWorldLayoutBuildingEditStatus::CoordinateOverflow,
               "creative_world_layout_building_move_coordinate_overflow");
    return result;
  }
  setReady(result, true, std::move(edited),
           "creative_world_layout_building_move_ready");
  return result;
}

CreativeWorldLayoutBuildingEditResult duplicateCreativeWorldLayoutBuilding(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutBuildingDuplicateRequest& request) {
  CreativeWorldLayoutBuildingEditResult result;
  result.requested = true;
  result.sourceBuildingIndex = request.buildingIndex;
  result.nextStableOrdinal = request.nextStableOrdinal;
  CreativeWorldLayoutBuildingBounds bounds;
  if (request.buildingIndex >= source.buildings.size() ||
      (request.deltaXCells == 0 && request.deltaZCells == 0)) {
    setFailure(result, CreativeWorldLayoutBuildingEditStatus::InvalidRequest,
               "creative_world_layout_building_duplicate_request_invalid");
    return result;
  }
  if (!validCreativeWorldLayoutBuildingOwnership(source)) {
    setFailure(result, CreativeWorldLayoutBuildingEditStatus::InvalidOwnership,
               "creative_world_layout_building_duplicate_ownership_invalid");
    return result;
  }
  if (!measureCreativeWorldLayoutBuildingBounds(source, request.buildingIndex,
                                                bounds)) {
    setFailure(result, CreativeWorldLayoutBuildingEditStatus::EmptyBuilding,
               "creative_world_layout_building_duplicate_empty");
    return result;
  }
  if (!canMoveCreativeWorldLayoutBuilding(
          source,
          {request.buildingIndex, request.deltaXCells, request.deltaZCells})) {
    setFailure(result,
               CreativeWorldLayoutBuildingEditStatus::CoordinateOverflow,
               "creative_world_layout_building_duplicate_coordinate_overflow");
    return result;
  }

  CreativeWorldLayoutBuildingTemplateResult captured =
      captureCreativeWorldLayoutBuildingTemplate(
          source, {request.buildingIndex, "duplicate_source",
                   source.buildings[request.buildingIndex].name});
  if (!captured.accepted) {
    setFailure(result, CreativeWorldLayoutBuildingEditStatus::InvalidRequest,
               "creative_world_layout_building_duplicate_source_invalid");
    return result;
  }
  const std::int64_t anchorX =
      static_cast<std::int64_t>(bounds.minimum.x) + request.deltaXCells;
  const std::int64_t anchorZ =
      static_cast<std::int64_t>(bounds.minimum.z) + request.deltaZCells;
  result = stampCreativeWorldLayoutBuildingTemplate(
      source, captured.value,
      {{static_cast<std::int32_t>(anchorX), static_cast<std::int32_t>(anchorZ)},
       request.nextStableOrdinal,
       true});
  result.sourceBuildingIndex = request.buildingIndex;
  if (result.accepted) {
    result.reasonCode = "creative_world_layout_building_duplicate_ready";
  }
  return result;
}

CreativeWorldLayoutBuildingEditResult deleteCreativeWorldLayoutBuilding(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutBuildingDeleteRequest& request) {
  CreativeWorldLayoutBuildingEditResult result;
  result.requested = true;
  result.sourceBuildingIndex = request.buildingIndex;
  if (request.buildingIndex >= source.buildings.size()) {
    setFailure(result, CreativeWorldLayoutBuildingEditStatus::InvalidRequest,
               "creative_world_layout_building_delete_request_invalid");
    return result;
  }
  if (!validCreativeWorldLayoutBuildingOwnership(source)) {
    setFailure(result, CreativeWorldLayoutBuildingEditStatus::InvalidOwnership,
               "creative_world_layout_building_delete_ownership_invalid");
    return result;
  }

  CreativeWorldLayout edited = source;
  edited.buildings.erase(edited.buildings.begin() +
                         static_cast<std::ptrdiff_t>(request.buildingIndex));

  std::vector<std::size_t> roomMap(source.rooms.size(),
                                   kInvalidCreativeWorldLayoutIndex);
  edited.rooms.clear();
  edited.rooms.reserve(source.rooms.size());
  for (std::size_t index = 0U; index < source.rooms.size(); ++index) {
    CreativeWorldLayoutRoom room = source.rooms[index];
    if (room.buildingIndex == request.buildingIndex) {
      continue;
    }
    if (room.buildingIndex > request.buildingIndex) {
      --room.buildingIndex;
    }
    roomMap[index] = edited.rooms.size();
    edited.rooms.push_back(std::move(room));
  }

  edited.boxes.clear();
  edited.boxes.reserve(source.boxes.size());
  for (CreativeWorldLayoutBox box : source.boxes) {
    if (box.buildingIndex == request.buildingIndex) {
      continue;
    }
    if (box.buildingIndex > request.buildingIndex) {
      --box.buildingIndex;
    }
    edited.boxes.push_back(std::move(box));
  }

  std::vector<std::size_t> wallMap(source.walls.size(),
                                   kInvalidCreativeWorldLayoutIndex);
  edited.walls.clear();
  edited.walls.reserve(source.walls.size());
  for (std::size_t index = 0U; index < source.walls.size(); ++index) {
    CreativeWorldLayoutWall wall = source.walls[index];
    if (wall.buildingIndex == request.buildingIndex) {
      continue;
    }
    if (wall.buildingIndex > request.buildingIndex) {
      --wall.buildingIndex;
    }
    wallMap[index] = edited.walls.size();
    edited.walls.push_back(std::move(wall));
  }

  edited.openings.clear();
  edited.openings.reserve(source.openings.size());
  for (CreativeWorldLayoutOpening opening : source.openings) {
    if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge) {
      const std::size_t mappedRoom = roomMap[opening.roomIndex];
      if (mappedRoom == kInvalidCreativeWorldLayoutIndex) {
        continue;
      }
      opening.roomIndex = mappedRoom;
    } else {
      const std::size_t mappedWall = wallMap[opening.wallIndex];
      if (mappedWall == kInvalidCreativeWorldLayoutIndex) {
        continue;
      }
      opening.wallIndex = mappedWall;
    }
    edited.openings.push_back(std::move(opening));
  }

  setReady(result, true, std::move(edited),
           "creative_world_layout_building_delete_ready");
  return result;
}

}  // namespace iggy3d::creative
