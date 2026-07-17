#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include <algorithm>
#include <cmath>
#include <span>
#include <string_view>

namespace iggy3d::creative {
namespace {

constexpr std::string_view kSourcePrefix =
    "creative_world_layout_source:";
constexpr std::string_view kRoomEdgePrefix =
    "creative_world_layout_room_edge:";

[[nodiscard]] bool hasTag(std::span<const std::string> tags,
                          std::string_view expected) noexcept {
  return std::any_of(tags.begin(), tags.end(),
                     [expected](const std::string& tag) {
                       return tag == expected;
                     });
}

[[nodiscard]] std::string childKey(std::string_view parent,
                                   std::string_view child) {
  return std::string(parent) + "." + std::string(child);
}

[[nodiscard]] std::string tableKey(CreativeWorldLayoutTable table) {
  switch (table) {
    case CreativeWorldLayoutTable::Building: return "building";
    case CreativeWorldLayoutTable::Level: return "level";
    case CreativeWorldLayoutTable::Room: return "room";
    case CreativeWorldLayoutTable::Box: return "box";
    case CreativeWorldLayoutTable::Wall: return "wall";
    case CreativeWorldLayoutTable::Opening: return "opening";
    case CreativeWorldLayoutTable::Object: return "object";
    case CreativeWorldLayoutTable::None:
    case CreativeWorldLayoutTable::TerrainProfile:
    case CreativeWorldLayoutTable::TerrainPath:
    case CreativeWorldLayoutTable::TerrainPathPoint:
      break;
  }
  return {};
}

[[nodiscard]] std::size_t openingBuildingIndex(
    const CreativeWorldLayout& layout,
    const CreativeWorldLayoutOpening& opening) noexcept {
  if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge &&
      opening.roomIndex < layout.rooms.size()) {
    return layout.rooms[opening.roomIndex].buildingIndex;
  }
  if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::Wall &&
      opening.wallIndex < layout.walls.size()) {
    return layout.walls[opening.wallIndex].buildingIndex;
  }
  return kInvalidCreativeWorldLayoutIndex;
}

[[nodiscard]] std::string symbolKey(const CreativeWorldLayout& layout,
                                    CreativeWorldLayoutTable table,
                                    std::size_t index) {
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::string_view local;
  switch (table) {
    case CreativeWorldLayoutTable::Building:
      return index < layout.buildings.size()
                 ? layout.buildings[index].stableKey
                 : std::string{};
    case CreativeWorldLayoutTable::Level:
      if (index < layout.levels.size()) {
        buildingIndex = layout.levels[index].buildingIndex;
        local = layout.levels[index].stableKey;
      }
      break;
    case CreativeWorldLayoutTable::Room:
      if (index < layout.rooms.size()) {
        buildingIndex = layout.rooms[index].buildingIndex;
        local = layout.rooms[index].stableKey;
      }
      break;
    case CreativeWorldLayoutTable::Box:
      if (index < layout.boxes.size()) {
        buildingIndex = layout.boxes[index].buildingIndex;
        local = layout.boxes[index].stableKey;
      }
      break;
    case CreativeWorldLayoutTable::Wall:
      if (index < layout.walls.size()) {
        buildingIndex = layout.walls[index].buildingIndex;
        local = layout.walls[index].stableKey;
      }
      break;
    case CreativeWorldLayoutTable::Opening:
      if (index < layout.openings.size()) {
        buildingIndex = openingBuildingIndex(layout, layout.openings[index]);
        local = layout.openings[index].stableKey;
      }
      break;
    case CreativeWorldLayoutTable::Object:
      return index < layout.objects.size() ? layout.objects[index].stableKey
                                           : std::string{};
    case CreativeWorldLayoutTable::None:
    case CreativeWorldLayoutTable::TerrainProfile:
    case CreativeWorldLayoutTable::TerrainPath:
    case CreativeWorldLayoutTable::TerrainPathPoint:
      return {};
  }
  if (buildingIndex >= layout.buildings.size() || local.empty()) {
    return {};
  }
  return childKey(layout.buildings[buildingIndex].stableKey, local);
}

[[nodiscard]] CreativeWorldLayoutObjectProvenance resolved(
    CreativeWorldLayoutTable table,
    std::size_t index,
    CreativeWorldLayoutRoomEdge roomEdge = CreativeWorldLayoutRoomEdge::Count,
    std::size_t contributorCount = 1U) noexcept {
  return {true, table, index, roomEdge, contributorCount};
}

[[nodiscard]] bool pointFallsOnEdgeSpan(
    const CreativeWorldLayoutRoom& room,
    CreativeWorldLayoutRoomEdge edge,
    CreativeVec3 sourcePointCells) noexcept {
  constexpr double kEdgeSpanToleranceCells = 1.0e-6;
  const bool horizontal = edge == CreativeWorldLayoutRoomEdge::North ||
                          edge == CreativeWorldLayoutRoomEdge::South;
  const double coordinate = horizontal ? sourcePointCells.x
                                       : sourcePointCells.z;
  const double minimum = horizontal ? room.footprint.minimum.x
                                    : room.footprint.minimum.z;
  const double maximum = horizontal ? room.footprint.maximum.x
                                    : room.footprint.maximum.z;
  return std::isfinite(coordinate) &&
         coordinate >= minimum - kEdgeSpanToleranceCells &&
         coordinate <= maximum + kEdgeSpanToleranceCells;
}

[[nodiscard]] CreativeWorldLayoutObjectProvenance resolveProvenance(
    const CreativeWorldLayout& layout,
    const CreativeObject& object,
    const CreativeVec3* sourcePointCells) {
  if (!hasTag(object.tags, creativeWorldLayoutTag(layout.stableKey))) {
    return {};
  }

  const auto findTable = [&](CreativeWorldLayoutTable table,
                             std::size_t count)
      -> CreativeWorldLayoutObjectProvenance {
    for (std::size_t index = 0U; index < count; ++index) {
      const std::string tag =
          creativeWorldLayoutProvenanceTag(layout, table, index);
      if (!tag.empty() && hasTag(object.tags, tag)) {
        return resolved(table, index);
      }
    }
    return {};
  };

  for (const auto [table, count] : {
           std::pair{CreativeWorldLayoutTable::Opening,
                     layout.openings.size()},
           std::pair{CreativeWorldLayoutTable::Box, layout.boxes.size()},
           std::pair{CreativeWorldLayoutTable::Wall, layout.walls.size()},
       }) {
    const CreativeWorldLayoutObjectProvenance found = findTable(table, count);
    if (found.owned) {
      return found;
    }
  }

  std::size_t contributorCount = 0U;
  std::size_t primaryRoom = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRoomEdge primaryEdge = CreativeWorldLayoutRoomEdge::Count;
  std::size_t pointedRoom = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRoomEdge pointedEdge = CreativeWorldLayoutRoomEdge::Count;
  for (std::size_t roomIndex = 0U; roomIndex < layout.rooms.size();
       ++roomIndex) {
    for (std::size_t edgeIndex = 0U;
         edgeIndex < static_cast<std::size_t>(CreativeWorldLayoutRoomEdge::Count);
         ++edgeIndex) {
      const auto edge = static_cast<CreativeWorldLayoutRoomEdge>(edgeIndex);
      const std::string tag = creativeWorldLayoutRoomEdgeProvenanceTag(
          layout, roomIndex, edge);
      if (!tag.empty() && hasTag(object.tags, tag)) {
        if (primaryRoom == kInvalidCreativeWorldLayoutIndex) {
          primaryRoom = roomIndex;
          primaryEdge = edge;
        }
        if (sourcePointCells != nullptr &&
            pointedRoom == kInvalidCreativeWorldLayoutIndex &&
            pointFallsOnEdgeSpan(layout.rooms[roomIndex], edge,
                                 *sourcePointCells)) {
          pointedRoom = roomIndex;
          pointedEdge = edge;
        }
        ++contributorCount;
      }
    }
  }
  if (primaryRoom != kInvalidCreativeWorldLayoutIndex) {
    return pointedRoom != kInvalidCreativeWorldLayoutIndex
               ? resolved(CreativeWorldLayoutTable::Room, pointedRoom,
                          pointedEdge, contributorCount)
               : resolved(CreativeWorldLayoutTable::Room, primaryRoom,
                          primaryEdge, contributorCount);
  }

  for (const auto [table, count] : {
           std::pair{CreativeWorldLayoutTable::Room, layout.rooms.size()},
           std::pair{CreativeWorldLayoutTable::Object, layout.objects.size()},
           std::pair{CreativeWorldLayoutTable::Building,
                     layout.buildings.size()},
       }) {
    const CreativeWorldLayoutObjectProvenance found = findTable(table, count);
    if (found.owned) {
      return found;
    }
  }
  return {};
}

}  // namespace

std::string creativeWorldLayoutProvenanceTag(
    const CreativeWorldLayout& layout,
    CreativeWorldLayoutTable table,
    std::size_t index) {
  const std::string kind = tableKey(table);
  const std::string key = symbolKey(layout, table, index);
  if (kind.empty() || key.empty()) {
    return {};
  }
  return std::string(kSourcePrefix) + kind + ":" + key;
}

std::string creativeWorldLayoutRoomEdgeProvenanceTag(
    const CreativeWorldLayout& layout,
    std::size_t roomIndex,
    CreativeWorldLayoutRoomEdge edge) {
  if (edge >= CreativeWorldLayoutRoomEdge::Count) {
    return {};
  }
  const std::string key = symbolKey(
      layout, CreativeWorldLayoutTable::Room, roomIndex);
  if (key.empty()) {
    return {};
  }
  return std::string(kRoomEdgePrefix) + key + ":" +
         std::string(creativeWorldLayoutRoomEdgeKey(edge));
}

CreativeWorldLayoutObjectProvenance
resolveCreativeWorldLayoutObjectProvenance(
    const CreativeWorldLayout& layout,
    const CreativeObject& object) {
  return resolveProvenance(layout, object, nullptr);
}

CreativeWorldLayoutObjectProvenance
resolveCreativeWorldLayoutObjectProvenance(
    const CreativeWorldLayout& layout,
    const CreativeObject& object,
    CreativeVec3 sourcePointCells) {
  return resolveProvenance(layout, object, &sourcePointCells);
}

}  // namespace iggy3d::creative
