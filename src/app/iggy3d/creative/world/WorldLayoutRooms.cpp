#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/world/WorldLayoutDimensions.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"

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
  std::size_t levelIndex = kInvalidCreativeWorldLayoutIndex;
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
  double edgeBaseLayer = 0.0;
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

bool sameFloorTop(double lhs, double rhs) noexcept {
  constexpr double kFloorTopEpsilonLayers = 1.0e-9;
  return std::abs(lhs - rhs) <= kFloorTopEpsilonLayers;
}

bool roomsOverlap(const CreativeWorldLayoutRoom& lhs,
                  const CreativeWorldLayoutRoom& rhs) noexcept {
  if (lhs.buildingIndex != rhs.buildingIndex ||
      lhs.levelIndex != rhs.levelIndex) {
    return false;
  }
  return std::max(lhs.footprint.minimum.x, rhs.footprint.minimum.x) <
             std::min(lhs.footprint.maximum.x, rhs.footprint.maximum.x) &&
         std::max(lhs.footprint.minimum.z, rhs.footprint.minimum.z) <
             std::min(lhs.footprint.maximum.z, rhs.footprint.maximum.z);
}

bool sameMergeLane(const EdgeRecord& lhs, const EdgeRecord& rhs) noexcept {
  return lhs.buildingIndex == rhs.buildingIndex &&
         lhs.levelIndex == rhs.levelIndex &&
         lhs.orientation == rhs.orientation && lhs.line == rhs.line &&
         lhs.baseLayer == rhs.baseLayer && lhs.heightCells == rhs.heightCells &&
         lhs.thicknessCells == rhs.thicknessCells;
}

bool oppositeEdges(CreativeWorldLayoutRoomEdge lhs,
                   CreativeWorldLayoutRoomEdge rhs) noexcept {
  return (lhs == CreativeWorldLayoutRoomEdge::North &&
          rhs == CreativeWorldLayoutRoomEdge::South) ||
         (lhs == CreativeWorldLayoutRoomEdge::East &&
          rhs == CreativeWorldLayoutRoomEdge::West) ||
         (lhs == CreativeWorldLayoutRoomEdge::South &&
          rhs == CreativeWorldLayoutRoomEdge::North) ||
         (lhs == CreativeWorldLayoutRoomEdge::West &&
          rhs == CreativeWorldLayoutRoomEdge::East);
}

bool exteriorWallProvenance(
    const CreativeWorldLayoutRoomCompileResult::WallProvenance& provenance)
    noexcept {
  if (provenance.contributors.empty()) {
    return false;
  }
  for (std::size_t first = 0U; first < provenance.contributors.size();
       ++first) {
    for (std::size_t second = first + 1U;
         second < provenance.contributors.size(); ++second) {
      if (oppositeEdges(provenance.contributors[first].roomEdge,
                        provenance.contributors[second].roomEdge)) {
        return false;
      }
    }
  }
  return true;
}

bool sameFacadeRun(const CreativeWorldLayoutWall& lhs,
                   const CreativeWorldLayoutWall& rhs) noexcept {
  return lhs.buildingIndex == rhs.buildingIndex && lhs.start == rhs.start &&
         lhs.end == rhs.end && lhs.thicknessCells == rhs.thicknessCells;
}

bool mergeContiguousFacadeHeight(CreativeWorldLayoutWall& destination,
                                 const CreativeWorldLayoutWall& source)
    noexcept {
  constexpr double kFacadeLayerEpsilon = 1.0e-9;
  const double destinationTop =
      destination.baseLayer + static_cast<double>(destination.heightCells);
  const double sourceTop =
      source.baseLayer + static_cast<double>(source.heightCells);
  if (source.baseLayer > destinationTop + kFacadeLayerEpsilon ||
      destination.baseLayer > sourceTop + kFacadeLayerEpsilon) {
    return false;
  }

  const double mergedBase = std::min(destination.baseLayer, source.baseLayer);
  const double mergedTop = std::max(destinationTop, sourceTop);
  const double mergedHeight = mergedTop - mergedBase;
  const double roundedHeight = std::round(mergedHeight);
  if (!std::isfinite(mergedHeight) ||
      std::abs(mergedHeight - roundedHeight) > kFacadeLayerEpsilon ||
      roundedHeight <= 0.0 ||
      roundedHeight >
          static_cast<double>(std::numeric_limits<std::uint16_t>::max())) {
    return false;
  }

  destination.baseLayer = mergedBase;
  destination.heightCells = static_cast<std::uint16_t>(roundedHeight);
  return true;
}

void mergeContiguousExteriorFacades(
    std::size_t explicitWallCount,
    CreativeWorldLayoutRoomCompileResult& result,
    std::vector<EdgeBinding>& bindings) {
  std::vector<CreativeWorldLayoutWall> mergedWalls;
  std::vector<CreativeWorldLayoutRoomCompileResult::WallProvenance>
      mergedProvenance;
  std::vector<bool> mergedExterior;
  std::vector<std::size_t> wallRemap(
      result.expanded.walls.size(), kInvalidCreativeWorldLayoutIndex);
  mergedWalls.reserve(result.expanded.walls.size());
  mergedProvenance.reserve(result.wallProvenance.size());
  mergedExterior.reserve(result.wallProvenance.size());

  for (std::size_t wallIndex = 0U;
       wallIndex < result.expanded.walls.size(); ++wallIndex) {
    const bool exterior =
        wallIndex >= explicitWallCount &&
        wallIndex < result.wallProvenance.size() &&
        exteriorWallProvenance(result.wallProvenance[wallIndex]);
    std::size_t destinationIndex = kInvalidCreativeWorldLayoutIndex;
    if (exterior) {
      for (std::size_t candidateIndex = explicitWallCount;
           candidateIndex < mergedWalls.size(); ++candidateIndex) {
        if (mergedExterior[candidateIndex] &&
            sameFacadeRun(mergedWalls[candidateIndex],
                          result.expanded.walls[wallIndex]) &&
            mergeContiguousFacadeHeight(
                mergedWalls[candidateIndex],
                result.expanded.walls[wallIndex])) {
          destinationIndex = candidateIndex;
          break;
        }
      }
    }

    if (destinationIndex == kInvalidCreativeWorldLayoutIndex) {
      destinationIndex = mergedWalls.size();
      mergedWalls.push_back(result.expanded.walls[wallIndex]);
      mergedProvenance.push_back(result.wallProvenance[wallIndex]);
      mergedExterior.push_back(exterior);
    } else {
      auto& contributors = mergedProvenance[destinationIndex].contributors;
      const auto& sourceContributors =
          result.wallProvenance[wallIndex].contributors;
      contributors.insert(contributors.end(), sourceContributors.begin(),
                          sourceContributors.end());
    }
    wallRemap[wallIndex] = destinationIndex;
  }

  for (EdgeBinding& binding : bindings) {
    if (binding.wallIndex < wallRemap.size()) {
      binding.wallIndex = wallRemap[binding.wallIndex];
    }
  }
  result.expanded.walls = std::move(mergedWalls);
  result.wallProvenance = std::move(mergedProvenance);
}

bool sharedMergeLane(const EdgeRecord& lhs,
                     const EdgeRecord& rhs) noexcept {
  return lhs.buildingIndex == rhs.buildingIndex &&
         lhs.levelIndex == rhs.levelIndex &&
         lhs.orientation == rhs.orientation && lhs.line == rhs.line &&
         sameFloorTop(lhs.baseLayer, rhs.baseLayer) &&
         lhs.heightCells == rhs.heightCells &&
         lhs.thicknessCells == rhs.thicknessCells &&
         oppositeEdges(lhs.roomEdge, rhs.roomEdge);
}

auto edgeSortKey(const EdgeRecord& edge) noexcept {
  return std::tuple{edge.buildingIndex, edge.levelIndex, edge.baseLayer,
                    edge.heightCells, edge.thicknessCells, edge.orientation,
                    edge.line, edge.begin, edge.end, edge.roomIndex,
                    edge.roomEdge};
}

std::array<EdgeRecord, kRoomEdgeCount> roomEdges(
    const CreativeWorldLayoutRoom& room, std::size_t roomIndex,
    const CreativeWorldLayoutResolvedRoomGeometry& geometry) {
  const CreativeWorldLayoutRect rect = room.footprint;
  const auto make = [&](CreativeWorldLayoutRoomEdge edge,
                        EdgeOrientation orientation, std::int32_t line,
                        std::int32_t begin, std::int32_t end) {
    return EdgeRecord{room.buildingIndex,
                      room.levelIndex,
                      roomIndex,
                      edge,
                      orientation,
                      line,
                      begin,
                      end,
                      geometry.floorTopLayer,
                      geometry.wallHeightCells,
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
  result.wallProvenance.resize(layout.walls.size());

  const std::size_t invalidLevelIndex =
      firstInvalidCreativeWorldLayoutLevelIndex(layout);
  if (invalidLevelIndex != kInvalidCreativeWorldLayoutIndex) {
    setFailure(result, CreativeWorldLayoutRoomCompileStatus::InvalidLevel,
               invalidLevelIndex,
               "creative_world_layout_level_ownership_invalid");
    return result;
  }

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
    if (room.levelIndex >= layout.levels.size() ||
        layout.levels[room.levelIndex].buildingIndex != room.buildingIndex) {
      setFailure(result, CreativeWorldLayoutRoomCompileStatus::InvalidRoom,
                 roomIndex,
                 "creative_world_layout_room_level_ownership_invalid");
      return result;
    }
    const CreativeWorldLayoutResolvedRoomGeometry geometry =
        resolveCreativeWorldLayoutRoomGeometry(layout, roomIndex);
    if (room.buildingIndex >= layout.buildings.size() || room.name.empty() ||
        !geometry.valid || !validRect(room.footprint) ||
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
    for (std::size_t prior = 0U; prior < roomIndex; ++prior) {
      if (roomsOverlap(layout.rooms[prior], room)) {
        setFailure(result,
                   CreativeWorldLayoutRoomCompileStatus::OverlappingRooms,
                   roomIndex, "creative_world_layout_rooms_overlap");
        return result;
      }
    }

    const auto generated = roomEdges(room, roomIndex, geometry);
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
    CreativeWorldLayoutRoomCompileResult::WallProvenance provenance;
    provenance.contributors.reserve(cursor - laneBegin);

    for (std::size_t edgeIndex = laneBegin; edgeIndex < cursor; ++edgeIndex) {
      const EdgeRecord& edge = edges[edgeIndex];
      provenance.contributors.push_back({edge.roomIndex, edge.roomEdge});
      bindings[bindingIndex(edge.roomIndex, edge.roomEdge)] = {
          wallIndex, edge.begin, mergedBegin, edge.end - edge.begin,
          edge.baseLayer};
    }
    result.wallProvenance.push_back(std::move(provenance));
  }

  mergeContiguousExteriorFacades(layout.walls.size(), result, bindings);

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
    const double verticalOffset =
        binding.edgeBaseLayer -
        result.expanded.walls[binding.wallIndex].baseLayer;
    const bool alignWindowInsertToCutout =
        opening.kind == CreativeBuildingOpeningKind::Window &&
        opening.insertBottomCells == 0.0;
    opening.cutoutBottomCells += verticalOffset;
    if (alignWindowInsertToCutout) {
      opening.insertBottomCells = opening.cutoutBottomCells;
    } else {
      opening.insertBottomCells += verticalOffset;
    }
    opening.hostKind = CreativeWorldLayoutOpeningHostKind::Wall;
    opening.roomIndex = kInvalidCreativeWorldLayoutIndex;
  }

  result.accepted = true;
  result.status = CreativeWorldLayoutRoomCompileStatus::Ready;
  result.failedIndex = kInvalidCreativeWorldLayoutIndex;
  result.reasonCode = "creative_world_layout_rooms_expanded";
  return result;
}

std::vector<CreativeWorldLayoutSharedRoomEdgeSpan>
inspectCreativeWorldLayoutSharedRoomEdges(
    const CreativeWorldLayout& layout) {
  std::vector<CreativeWorldLayoutSharedRoomEdgeSpan> spans;
  for (std::size_t firstRoomIndex = 0U;
       firstRoomIndex < layout.rooms.size(); ++firstRoomIndex) {
    const auto firstEdges = roomEdges(
        layout.rooms[firstRoomIndex], firstRoomIndex,
        resolveCreativeWorldLayoutRoomGeometry(layout, firstRoomIndex));
    for (std::size_t secondRoomIndex = firstRoomIndex + 1U;
         secondRoomIndex < layout.rooms.size(); ++secondRoomIndex) {
      const auto secondEdges = roomEdges(
          layout.rooms[secondRoomIndex], secondRoomIndex,
          resolveCreativeWorldLayoutRoomGeometry(layout, secondRoomIndex));
      for (const EdgeRecord& first : firstEdges) {
        for (const EdgeRecord& second : secondEdges) {
          if (!sharedMergeLane(first, second)) {
            continue;
          }
          const std::int32_t begin = std::max(first.begin, second.begin);
          const std::int32_t end = std::min(first.end, second.end);
          if (begin >= end) {
            continue;
          }
          spans.push_back({
              firstRoomIndex,
              first.roomEdge,
              secondRoomIndex,
              second.roomEdge,
              first.orientation == EdgeOrientation::Horizontal
                  ? CreativeTerrainCoord2{begin, first.line}
                  : CreativeTerrainCoord2{first.line, begin},
              first.orientation == EdgeOrientation::Horizontal
                  ? CreativeTerrainCoord2{end, first.line}
                  : CreativeTerrainCoord2{first.line, end},
          });
        }
      }
    }
  }
  return spans;
}

bool creativeWorldLayoutRoomEdgeIntervalIsShared(
    const CreativeWorldLayout& layout, std::size_t roomIndex,
    CreativeWorldLayoutRoomEdge roomEdge, double centerOffsetCells,
    double widthCells) {
  if (roomIndex >= layout.rooms.size() ||
      roomEdge >= CreativeWorldLayoutRoomEdge::Count ||
      !std::isfinite(centerOffsetCells) || !std::isfinite(widthCells) ||
      widthCells <= 0.0) {
    return false;
  }
  const double intervalBegin = centerOffsetCells - widthCells * 0.5;
  const double intervalEnd = centerOffsetCells + widthCells * 0.5;
  const CreativeWorldLayoutRoom& room = layout.rooms[roomIndex];
  const auto candidateEdges = roomEdges(
      room, roomIndex,
      resolveCreativeWorldLayoutRoomGeometry(layout, roomIndex));
  const EdgeRecord& candidate =
      candidateEdges[static_cast<std::size_t>(roomEdge)];
  const double offsetOrigin =
      candidate.orientation == EdgeOrientation::Horizontal
          ? static_cast<double>(room.footprint.minimum.x)
          : static_cast<double>(room.footprint.minimum.z);
  for (std::size_t otherRoomIndex = 0U;
       otherRoomIndex < layout.rooms.size(); ++otherRoomIndex) {
    if (otherRoomIndex == roomIndex) {
      continue;
    }
    const auto otherEdges = roomEdges(
        layout.rooms[otherRoomIndex], otherRoomIndex,
        resolveCreativeWorldLayoutRoomGeometry(layout, otherRoomIndex));
    const auto found = std::find_if(
        otherEdges.begin(), otherEdges.end(), [&](const EdgeRecord& edge) {
          return sharedMergeLane(candidate, edge) &&
                 std::max(candidate.begin, edge.begin) <
                     std::min(candidate.end, edge.end);
        });
    if (found == otherEdges.end()) {
      continue;
    }
    const double sharedBegin =
        static_cast<double>(std::max(candidate.begin, found->begin)) -
        offsetOrigin;
    const double sharedEnd =
        static_cast<double>(std::min(candidate.end, found->end)) -
        offsetOrigin;
    if (std::max(intervalBegin, sharedBegin) <
        std::min(intervalEnd, sharedEnd)) {
      return true;
    }
  }
  return false;
}

bool creativeWorldLayoutHasInteriorRoomWindow(
    const CreativeWorldLayout& layout) {
  for (const CreativeWorldLayoutOpening& opening : layout.openings) {
    if (opening.kind == CreativeBuildingOpeningKind::Window &&
        opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge &&
        creativeWorldLayoutRoomEdgeIntervalIsShared(
            layout, opening.roomIndex, opening.roomEdge,
            opening.centerOffsetCells, opening.widthCells)) {
      return true;
    }
  }
  return false;
}

CreativeRectangularRoomGeometryPlan planCreativeWorldLayoutRoomGeometry(
    const CreativeGridSettings& grid, const CreativeWorldLayout& layout,
    std::size_t roomIndex) noexcept {
  CreativeRectangularRoomGeometryRequest request;
  if (roomIndex >= layout.rooms.size()) {
    return planCreativeRectangularRoomGeometry(request);
  }
  const CreativeWorldLayoutRoom& room = layout.rooms[roomIndex];
  const CreativeWorldLayoutLevelDimensions dimensions =
      measureCreativeWorldLayoutLevelDimensions(grid, layout,
                                                room.levelIndex);
  if (!dimensions.accepted || dimensions.buildingIndex != room.buildingIndex) {
    return planCreativeRectangularRoomGeometry(request);
  }
  const auto coordinate = [](double origin, double cellSize,
                             double value) noexcept {
    return origin + cellSize * value;
  };
  request.firstFloorCorner = {
      coordinate(grid.origin.x, grid.cellSizeMeters, room.footprint.minimum.x),
      dimensions.floorTopMeters,
      coordinate(grid.origin.z, grid.cellSizeMeters, room.footprint.minimum.z),
  };
  request.oppositeFloorCorner = {
      coordinate(grid.origin.x, grid.cellSizeMeters, room.footprint.maximum.x),
      request.firstFloorCorner.y,
      coordinate(grid.origin.z, grid.cellSizeMeters, room.footprint.maximum.z),
  };
  request.wallHeightMeters = dimensions.wallHeightMeters;
  request.wallThicknessMeters = room.wallThicknessCells * grid.cellSizeMeters;
  request.floorThicknessMeters = dimensions.floorThicknessMeters;
  return planCreativeRectangularRoomGeometry(request);
}

}  // namespace iggy3d::creative
