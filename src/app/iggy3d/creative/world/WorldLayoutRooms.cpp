#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

enum class EdgeOrientation : std::uint8_t { Horizontal, Vertical };

struct EdgeRecord {
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t roomIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRoomEdge roomEdge = CreativeWorldLayoutRoomEdge::North;
  EdgeOrientation orientation = EdgeOrientation::Horizontal;
  std::int32_t line = 0;
  std::int32_t begin = 0;
  std::int32_t end = 0;
  double baseLayer = 0.0;
  std::uint16_t heightCells = 0U;
  double thicknessCells = 0.0;
};

struct EdgeBinding {
  std::size_t wallIndex = kInvalidCreativeWorldLayoutIndex;
  std::int32_t edgeBegin = 0;
  std::int32_t wallBegin = 0;
  std::int32_t edgeLength = 0;
};

constexpr std::size_t kRoomEdgeCount =
    static_cast<std::size_t>(CreativeWorldLayoutRoomEdge::Count);

std::size_t bindingIndex(std::size_t roomIndex,
                         CreativeWorldLayoutRoomEdge edge) noexcept {
  return roomIndex * kRoomEdgeCount + static_cast<std::size_t>(edge);
}

bool validRect(CreativeWorldLayoutRect rect) noexcept {
  return rect.minimum.x < rect.maximum.x && rect.minimum.z < rect.maximum.z;
}

bool roomsOverlap(const CreativeWorldLayoutRoom& lhs,
                  const CreativeWorldLayoutRoom& rhs) noexcept {
  if (lhs.buildingIndex != rhs.buildingIndex ||
      lhs.baseLayer != rhs.baseLayer) {
    return false;
  }
  return std::max(lhs.footprint.minimum.x, rhs.footprint.minimum.x) <
             std::min(lhs.footprint.maximum.x, rhs.footprint.maximum.x) &&
         std::max(lhs.footprint.minimum.z, rhs.footprint.minimum.z) <
             std::min(lhs.footprint.maximum.z, rhs.footprint.maximum.z);
}

bool sameMergeLane(const EdgeRecord& lhs, const EdgeRecord& rhs) noexcept {
  return lhs.buildingIndex == rhs.buildingIndex &&
         lhs.orientation == rhs.orientation && lhs.line == rhs.line &&
         lhs.baseLayer == rhs.baseLayer && lhs.heightCells == rhs.heightCells &&
         lhs.thicknessCells == rhs.thicknessCells;
}

auto edgeSortKey(const EdgeRecord& edge) noexcept {
  return std::tuple{edge.buildingIndex,  edge.baseLayer,   edge.heightCells,
                    edge.thicknessCells, edge.orientation, edge.line,
                    edge.begin,          edge.end,         edge.roomIndex,
                    edge.roomEdge};
}

std::array<EdgeRecord, kRoomEdgeCount> roomEdges(
    const CreativeWorldLayoutRoom& room, std::size_t roomIndex,
    double wallBaseLayer) {
  const CreativeWorldLayoutRect rect = room.footprint;
  const auto make = [&](CreativeWorldLayoutRoomEdge edge,
                        EdgeOrientation orientation, std::int32_t line,
                        std::int32_t begin, std::int32_t end) {
    return EdgeRecord{room.buildingIndex,
                      roomIndex,
                      edge,
                      orientation,
                      line,
                      begin,
                      end,
                      wallBaseLayer,
                      room.wallHeightCells,
                      room.wallThicknessCells};
  };
  return {
      make(CreativeWorldLayoutRoomEdge::North, EdgeOrientation::Horizontal,
           rect.minimum.z, rect.minimum.x, rect.maximum.x),
      make(CreativeWorldLayoutRoomEdge::East, EdgeOrientation::Vertical,
           rect.maximum.x, rect.minimum.z, rect.maximum.z),
      make(CreativeWorldLayoutRoomEdge::South, EdgeOrientation::Horizontal,
           rect.maximum.z, rect.minimum.x, rect.maximum.x),
      make(CreativeWorldLayoutRoomEdge::West, EdgeOrientation::Vertical,
           rect.minimum.x, rect.minimum.z, rect.maximum.z),
  };
}

void setFailure(CreativeWorldLayoutRoomCompileResult& result,
                CreativeWorldLayoutRoomCompileStatus status,
                std::size_t failedIndex, std::string reasonCode) {
  result.status = status;
  result.failedIndex = failedIndex;
  result.reasonCode = std::move(reasonCode);
}

}  // namespace

CreativeWorldLayoutRoomCompileResult expandCreativeWorldLayoutRooms(
    const CreativeWorldLayout& layout) {
  CreativeWorldLayoutRoomCompileResult result;
  result.expanded = layout;
  result.expanded.rooms.clear();

  if (layout.rooms.size() >
      (std::numeric_limits<std::size_t>::max() / kRoomEdgeCount)) {
    setFailure(result, CreativeWorldLayoutRoomCompileStatus::CapacityExceeded,
               kInvalidCreativeWorldLayoutIndex,
               "creative_world_layout_room_edge_capacity_exceeded");
    return result;
  }

  std::vector<EdgeRecord> edges;
  edges.reserve(layout.rooms.size() * kRoomEdgeCount);
  for (std::size_t roomIndex = 0U; roomIndex < layout.rooms.size();
       ++roomIndex) {
    const CreativeWorldLayoutRoom& room = layout.rooms[roomIndex];
    if (room.buildingIndex >= layout.buildings.size() || room.name.empty() ||
        !validRect(room.footprint) || room.wallHeightCells == 0U ||
        room.floorThicknessCells == 0U ||
        !std::isfinite(room.wallThicknessCells) ||
        room.wallThicknessCells <= 0.0 ||
        (static_cast<double>(room.footprint.maximum.x) -
         static_cast<double>(room.footprint.minimum.x)) <=
            room.wallThicknessCells * 2.0 ||
        (static_cast<double>(room.footprint.maximum.z) -
         static_cast<double>(room.footprint.minimum.z)) <=
            room.wallThicknessCells * 2.0) {
      setFailure(result, CreativeWorldLayoutRoomCompileStatus::InvalidRoom,
                 roomIndex, "creative_world_layout_room_invalid");
      return result;
    }
    const double wallBaseLayer =
        static_cast<double>(room.baseLayer) +
        static_cast<double>(room.floorThicknessCells) * 0.5;
    for (std::size_t prior = 0U; prior < roomIndex; ++prior) {
      if (roomsOverlap(layout.rooms[prior], room)) {
        setFailure(result,
                   CreativeWorldLayoutRoomCompileStatus::OverlappingRooms,
                   roomIndex, "creative_world_layout_rooms_overlap");
        return result;
      }
    }

    CreativeWorldLayoutBox floor;
    floor.buildingIndex = room.buildingIndex;
    floor.kind = CreativeObjectKind::Floor;
    floor.stableKey = room.stableKey + ".floor";
    floor.name = room.name + " Floor";
    floor.footprint = room.footprint;
    floor.baseLayer = room.baseLayer;
    floor.heightCells = room.floorThicknessCells;
    result.expanded.boxes.push_back(std::move(floor));

    const auto generated = roomEdges(room, roomIndex, wallBaseLayer);
    edges.insert(edges.end(), generated.begin(), generated.end());
  }

  std::sort(edges.begin(), edges.end(),
            [](const EdgeRecord& lhs, const EdgeRecord& rhs) {
              return edgeSortKey(lhs) < edgeSortKey(rhs);
            });

  std::vector<EdgeBinding> bindings(layout.rooms.size() * kRoomEdgeCount);
  for (std::size_t cursor = 0U; cursor < edges.size();) {
    const std::size_t laneBegin = cursor;
    std::int32_t mergedBegin = edges[cursor].begin;
    std::int32_t mergedEnd = edges[cursor].end;
    ++cursor;
    while (cursor < edges.size() &&
           sameMergeLane(edges[laneBegin], edges[cursor]) &&
           edges[cursor].begin <= mergedEnd) {
      mergedEnd = std::max(mergedEnd, edges[cursor].end);
      ++cursor;
    }

    const EdgeRecord& lane = edges[laneBegin];
    CreativeWorldLayoutWall wall;
    wall.buildingIndex = lane.buildingIndex;
    wall.stableKey =
        "room_shell_wall_" + std::to_string(result.expanded.walls.size());
    wall.name =
        "Room Shell Wall " + std::to_string(result.expanded.walls.size() + 1U);
    wall.start = lane.orientation == EdgeOrientation::Horizontal
                     ? CreativeTerrainCoord2{mergedBegin, lane.line}
                     : CreativeTerrainCoord2{lane.line, mergedBegin};
    wall.end = lane.orientation == EdgeOrientation::Horizontal
                   ? CreativeTerrainCoord2{mergedEnd, lane.line}
                   : CreativeTerrainCoord2{lane.line, mergedEnd};
    wall.baseLayer = lane.baseLayer;
    wall.heightCells = lane.heightCells;
    wall.thicknessCells = lane.thicknessCells;
    const std::size_t wallIndex = result.expanded.walls.size();
    result.expanded.walls.push_back(std::move(wall));

    for (std::size_t edgeIndex = laneBegin; edgeIndex < cursor; ++edgeIndex) {
      const EdgeRecord& edge = edges[edgeIndex];
      bindings[bindingIndex(edge.roomIndex, edge.roomEdge)] = {
          wallIndex, edge.begin, mergedBegin, edge.end - edge.begin};
    }
  }

  for (std::size_t openingIndex = 0U;
       openingIndex < result.expanded.openings.size(); ++openingIndex) {
    CreativeWorldLayoutOpening& opening =
        result.expanded.openings[openingIndex];
    if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::Wall) {
      if (opening.wallIndex >= layout.walls.size()) {
        setFailure(
            result, CreativeWorldLayoutRoomCompileStatus::InvalidOpeningHost,
            openingIndex, "creative_world_layout_opening_wall_host_invalid");
        return result;
      }
      continue;
    }
    if (opening.hostKind != CreativeWorldLayoutOpeningHostKind::RoomEdge ||
        opening.roomIndex >= layout.rooms.size() ||
        opening.roomEdge >= CreativeWorldLayoutRoomEdge::Count ||
        !std::isfinite(opening.centerOffsetCells)) {
      setFailure(
          result, CreativeWorldLayoutRoomCompileStatus::InvalidOpeningHost,
          openingIndex, "creative_world_layout_opening_room_host_invalid");
      return result;
    }
    const EdgeBinding& binding =
        bindings[bindingIndex(opening.roomIndex, opening.roomEdge)];
    const double halfWidth = opening.widthCells * 0.5;
    if (binding.wallIndex == kInvalidCreativeWorldLayoutIndex ||
        !std::isfinite(opening.widthCells) || opening.widthCells <= 0.0 ||
        opening.centerOffsetCells - halfWidth < 0.0 ||
        opening.centerOffsetCells + halfWidth > binding.edgeLength) {
      setFailure(
          result, CreativeWorldLayoutRoomCompileStatus::InvalidOpeningHost,
          openingIndex, "creative_world_layout_opening_room_offset_invalid");
      return result;
    }
    opening.wallIndex = binding.wallIndex;
    opening.centerOffsetCells +=
        static_cast<double>(binding.edgeBegin - binding.wallBegin);
    opening.hostKind = CreativeWorldLayoutOpeningHostKind::Wall;
    opening.roomIndex = kInvalidCreativeWorldLayoutIndex;
  }

  result.accepted = true;
  result.status = CreativeWorldLayoutRoomCompileStatus::Ready;
  result.failedIndex = kInvalidCreativeWorldLayoutIndex;
  result.reasonCode = "creative_world_layout_rooms_expanded";
  return result;
}

}  // namespace iggy3d::creative
