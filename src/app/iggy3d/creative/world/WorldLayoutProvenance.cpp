#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"

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
    case CreativeWorldLayoutTable::VerticalConnector:
      return "vertical_connector";
    case CreativeWorldLayoutTable::Box: return "box";
    case CreativeWorldLayoutTable::Wall: return "wall";
    case CreativeWorldLayoutTable::Opening: return "opening";
    case CreativeWorldLayoutTable::Object: return "object";
    case CreativeWorldLayoutTable::TopologyEdge: return "topology_edge";
    case CreativeWorldLayoutTable::RoofAperture: return "roof_aperture";
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
    case CreativeWorldLayoutTable::VerticalConnector:
      if (index < layout.verticalConnectors.size()) {
        buildingIndex = layout.verticalConnectors[index].buildingIndex;
        local = layout.verticalConnectors[index].stableKey;
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
    case CreativeWorldLayoutTable::TopologyEdge:
      if (index < layout.topologyEdges.size()) {
        const CreativeWorldLayoutTopologyEdge& edge =
            layout.topologyEdges[index];
        if (edge.levelIndex < layout.levels.size()) {
          buildingIndex = layout.levels[edge.levelIndex].buildingIndex;
          local = edge.stableKey;
        }
      }
      break;
    case CreativeWorldLayoutTable::RoofAperture:
      if (index < layout.roofApertures.size()) {
        const CreativeWorldLayoutRoofAperture& aperture =
            layout.roofApertures[index];
        if (aperture.levelIndex < layout.levels.size()) {
          buildingIndex =
              layout.levels[aperture.levelIndex].buildingIndex;
          local = aperture.stableKey;
        }
      }
      break;
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

[[nodiscard]] bool pointFallsOnTopologyEdge(
    const CreativeWorldLayout& layout,
    const CreativeWorldLayoutTopologyEdge& edge,
    CreativeVec3 sourcePointCells) noexcept {
  constexpr double kSpanToleranceCells = 1.0e-6;
  if (!std::isfinite(sourcePointCells.x) ||
      !std::isfinite(sourcePointCells.y) ||
      !std::isfinite(sourcePointCells.z) ||
      edge.levelIndex >= layout.levels.size() ||
      edge.startVertexIndex >= layout.topologyVertices.size() ||
      edge.endVertexIndex >= layout.topologyVertices.size() ||
      !std::isfinite(edge.wallThicknessCells) ||
      edge.wallThicknessCells <= 0.0) {
    return false;
  }
  const CreativeTerrainCoord2 start =
      layout.topologyVertices[edge.startVertexIndex].position;
  const CreativeTerrainCoord2 end =
      layout.topologyVertices[edge.endVertexIndex].position;
  const bool horizontal = start.z == end.z && start.x != end.x;
  const bool vertical = start.x == end.x && start.z != end.z;
  if (!horizontal && !vertical) {
    return false;
  }
  const double along = horizontal ? sourcePointCells.x : sourcePointCells.z;
  const double perpendicular =
      horizontal ? sourcePointCells.z : sourcePointCells.x;
  const double alongStart = horizontal ? start.x : start.z;
  const double alongEnd = horizontal ? end.x : end.z;
  const double lineCoordinate = horizontal ? start.z : start.x;
  const double minimum = std::min(alongStart, alongEnd);
  const double maximum = std::max(alongStart, alongEnd);
  const double halfThickness = edge.wallThicknessCells * 0.5;
  const CreativeWorldLayoutLevel& level = layout.levels[edge.levelIndex];
  const double inheritedHeight =
      creativeWorldLayoutLevelFacadeHeightCells(layout, edge.levelIndex);
  const double height =
      edge.wallHeightCells != 0U
          ? static_cast<double>(edge.wallHeightCells)
          : (std::isfinite(inheritedHeight)
                 ? inheritedHeight
                 : static_cast<double>(level.wallHeightCells));
  const double top = level.floorTopLayer + height;
  return along >= minimum - kSpanToleranceCells &&
         along <= maximum + kSpanToleranceCells &&
         std::abs(perpendicular - lineCoordinate) <=
             halfThickness + kSpanToleranceCells &&
         sourcePointCells.y >= level.floorTopLayer - kSpanToleranceCells &&
         sourcePointCells.y <= top + kSpanToleranceCells;
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

  std::size_t topologyContributorCount = 0U;
  std::size_t primaryTopologyEdge = kInvalidCreativeWorldLayoutIndex;
  std::size_t pointedTopologyEdge = kInvalidCreativeWorldLayoutIndex;
  for (std::size_t edgeIndex = 0U;
       edgeIndex < layout.topologyEdges.size(); ++edgeIndex) {
    const std::string tag = creativeWorldLayoutProvenanceTag(
        layout, CreativeWorldLayoutTable::TopologyEdge, edgeIndex);
    if (tag.empty() || !hasTag(object.tags, tag)) {
      continue;
    }
    if (primaryTopologyEdge == kInvalidCreativeWorldLayoutIndex) {
      primaryTopologyEdge = edgeIndex;
    }
    if (sourcePointCells != nullptr &&
        pointedTopologyEdge == kInvalidCreativeWorldLayoutIndex &&
        pointFallsOnTopologyEdge(layout, layout.topologyEdges[edgeIndex],
                                 *sourcePointCells)) {
      pointedTopologyEdge = edgeIndex;
    }
    ++topologyContributorCount;
  }

  for (const auto [table, count] : {
           std::pair{CreativeWorldLayoutTable::RoofAperture,
                     layout.roofApertures.size()},
           std::pair{CreativeWorldLayoutTable::Opening, layout.openings.size()},
           std::pair{CreativeWorldLayoutTable::VerticalConnector,
                     layout.verticalConnectors.size()},
           std::pair{CreativeWorldLayoutTable::Box, layout.boxes.size()},
           std::pair{CreativeWorldLayoutTable::Wall, layout.walls.size()},
       }) {
    const CreativeWorldLayoutObjectProvenance found = findTable(table, count);
    if (found.owned) {
      return found;
    }
  }
  if (primaryTopologyEdge != kInvalidCreativeWorldLayoutIndex) {
    return resolved(
        CreativeWorldLayoutTable::TopologyEdge,
        pointedTopologyEdge != kInvalidCreativeWorldLayoutIndex
            ? pointedTopologyEdge
            : primaryTopologyEdge,
        CreativeWorldLayoutRoomEdge::Count, topologyContributorCount);
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
           std::pair{CreativeWorldLayoutTable::Level, layout.levels.size()},
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

CreativeWorldLayoutSourceAncestry buildCreativeWorldLayoutSourceAncestry(
    const CreativeWorldLayout& layout,
    CreativeWorldLayoutObjectProvenance provenance) noexcept {
  CreativeWorldLayoutSourceAncestry ancestry;
  if (!provenance.owned) {
    return ancestry;
  }

  const auto append = [&](CreativeWorldLayoutTable table, std::size_t index) {
    if (table == CreativeWorldLayoutTable::None ||
        index == kInvalidCreativeWorldLayoutIndex ||
        ancestry.count >= ancestry.entries.size() ||
        findCreativeWorldLayoutSource(ancestry, table, index) < ancestry.count) {
      return;
    }
    ancestry.entries[ancestry.count++] = {table, index};
  };
  const auto appendBuilding = [&](std::size_t buildingIndex) {
    if (buildingIndex < layout.buildings.size()) {
      append(CreativeWorldLayoutTable::Building, buildingIndex);
    }
  };
  const auto appendLevel = [&](std::size_t levelIndex) {
    if (levelIndex >= layout.levels.size()) {
      return;
    }
    appendBuilding(layout.levels[levelIndex].buildingIndex);
    append(CreativeWorldLayoutTable::Level, levelIndex);
  };
  const auto appendRoom = [&](std::size_t roomIndex) {
    if (roomIndex >= layout.rooms.size()) {
      return;
    }
    const CreativeWorldLayoutRoom& room = layout.rooms[roomIndex];
    appendBuilding(room.buildingIndex);
    if (room.levelIndex < layout.levels.size() &&
        layout.levels[room.levelIndex].buildingIndex == room.buildingIndex) {
      append(CreativeWorldLayoutTable::Level, room.levelIndex);
    }
    append(CreativeWorldLayoutTable::Room, roomIndex);
  };

  switch (provenance.table) {
    case CreativeWorldLayoutTable::Building:
      appendBuilding(provenance.index);
      break;
    case CreativeWorldLayoutTable::Level:
      appendLevel(provenance.index);
      break;
    case CreativeWorldLayoutTable::Room:
      appendRoom(provenance.index);
      break;
    case CreativeWorldLayoutTable::TopologyEdge:
      if (provenance.index < layout.topologyEdges.size()) {
        appendLevel(layout.topologyEdges[provenance.index].levelIndex);
      }
      append(provenance.table, provenance.index);
      break;
    case CreativeWorldLayoutTable::VerticalConnector:
      if (provenance.index < layout.verticalConnectors.size()) {
        appendBuilding(
            layout.verticalConnectors[provenance.index].buildingIndex);
      }
      append(provenance.table, provenance.index);
      break;
    case CreativeWorldLayoutTable::Box:
      if (provenance.index < layout.boxes.size()) {
        appendBuilding(layout.boxes[provenance.index].buildingIndex);
      }
      append(provenance.table, provenance.index);
      break;
    case CreativeWorldLayoutTable::Wall:
      if (provenance.index < layout.walls.size()) {
        appendBuilding(layout.walls[provenance.index].buildingIndex);
      }
      append(provenance.table, provenance.index);
      break;
    case CreativeWorldLayoutTable::Opening:
      if (provenance.index < layout.openings.size()) {
        const CreativeWorldLayoutOpening& opening =
            layout.openings[provenance.index];
        if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge) {
          appendRoom(opening.roomIndex);
        } else if (opening.wallIndex < layout.walls.size()) {
          appendBuilding(layout.walls[opening.wallIndex].buildingIndex);
        }
      }
      append(provenance.table, provenance.index);
      break;
    case CreativeWorldLayoutTable::RoofAperture:
      if (provenance.index < layout.roofApertures.size()) {
        appendLevel(layout.roofApertures[provenance.index].levelIndex);
      }
      append(provenance.table, provenance.index);
      break;
    case CreativeWorldLayoutTable::Object:
    case CreativeWorldLayoutTable::TerrainProfile:
    case CreativeWorldLayoutTable::TerrainPath:
      append(provenance.table, provenance.index);
      break;
    case CreativeWorldLayoutTable::None:
    case CreativeWorldLayoutTable::TerrainPathPoint:
      break;
  }
  const std::size_t direct = findCreativeWorldLayoutSource(
      ancestry, provenance.table, provenance.index);
  ancestry.directEntryIndex = direct < ancestry.count ? direct : 0U;
  return ancestry;
}

std::size_t findCreativeWorldLayoutSource(
    const CreativeWorldLayoutSourceAncestry& ancestry,
    CreativeWorldLayoutTable table,
    std::size_t index) noexcept {
  for (std::size_t sourceIndex = 0U; sourceIndex < ancestry.count;
       ++sourceIndex) {
    const CreativeWorldLayoutSourceRef& source = ancestry.entries[sourceIndex];
    if (source.table == table && source.index == index) {
      return sourceIndex;
    }
  }
  return ancestry.count;
}

bool creativeWorldLayoutObjectBelongsToSource(
    const CreativeWorldLayout& layout,
    const CreativeObject& object,
    CreativeWorldLayoutTable table,
    std::size_t index) {
  if (table == CreativeWorldLayoutTable::None ||
      index == kInvalidCreativeWorldLayoutIndex) {
    return false;
  }
  const CreativeWorldLayoutObjectProvenance provenance =
      resolveCreativeWorldLayoutObjectProvenance(layout, object);
  if (!provenance.owned) {
    return false;
  }
  const CreativeWorldLayoutSourceAncestry ancestry =
      buildCreativeWorldLayoutSourceAncestry(layout, provenance);
  if (findCreativeWorldLayoutSource(ancestry, table, index) < ancestry.count) {
    return true;
  }
  if (table == CreativeWorldLayoutTable::Room) {
    if (index >= layout.rooms.size()) {
      return false;
    }
    for (std::size_t edgeIndex = 0U;
         edgeIndex < static_cast<std::size_t>(CreativeWorldLayoutRoomEdge::Count);
         ++edgeIndex) {
      if (hasTag(object.tags, creativeWorldLayoutRoomEdgeProvenanceTag(
                                  layout, index,
                                  static_cast<CreativeWorldLayoutRoomEdge>(
                                      edgeIndex)))) {
        return true;
      }
    }
    return false;
  }
  return false;
}

}  // namespace iggy3d::creative
