#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBlockout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutDimensions.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

struct EdgeBinding {
  std::size_t wallIndex = kInvalidCreativeWorldLayoutIndex;
  std::int32_t edgeBegin = 0;
  std::int32_t wallBegin = 0;
  std::int32_t edgeLength = 0;
  double edgeBaseLayer = 0.0;
};

bool exteriorWallProvenance(
    const CreativeWorldLayoutRoomCompileResult::WallProvenance& provenance)
    noexcept {
  if (provenance.contributors.empty()) {
    return false;
  }
  for (std::size_t first = 0U; first < provenance.contributors.size(); ++first) {
    for (std::size_t second = first + 1U;
         second < provenance.contributors.size(); ++second) {
      if (provenance.contributors[first].topologyEdgeIndex ==
          provenance.contributors[second].topologyEdgeIndex) {
        return false;
      }
    }
  }
  return true;
}

bool sameFacadeRun(const CreativeWorldLayoutWall& lhs,
                   const CreativeWorldLayoutWall& rhs) noexcept {
  return lhs.buildingIndex == rhs.buildingIndex && lhs.start == rhs.start &&
         lhs.end == rhs.end && lhs.thicknessCells == rhs.thicknessCells &&
         lhs.profile == rhs.profile && lhs.material == rhs.material &&
         lhs.joinStyle == rhs.joinStyle;
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
        result.expanded.walls[wallIndex].profile ==
            CreativeWorldLayoutWallProfile::Exterior &&
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
  const std::size_t invalidLevelIndex =
      firstInvalidCreativeWorldLayoutLevelIndex(layout);
  if (invalidLevelIndex != kInvalidCreativeWorldLayoutIndex) {
    setFailure(result, CreativeWorldLayoutRoomCompileStatus::InvalidLevel,
               invalidLevelIndex,
               "creative_world_layout_level_ownership_invalid");
    return result;
  }

  if (layout.rooms.empty()) {
    if (!layout.topologyVertices.empty() || !layout.topologyEdges.empty() ||
        !layout.roomBoundaries.empty()) {
      setFailure(result, CreativeWorldLayoutRoomCompileStatus::InvalidRoom,
                 kInvalidCreativeWorldLayoutIndex,
                 "creative_world_layout_room_graph_orphan_data");
      return result;
    }
    result.expanded = layout;
    result.wallProvenance.resize(layout.walls.size());
    result.accepted = true;
    result.status = CreativeWorldLayoutRoomCompileStatus::Ready;
    result.reasonCode = "creative_world_layout_rooms_expanded";
    return result;
  }

  const CreativeWorldLayoutRoomGraphMaterializeResult materialized =
      materializeCreativeWorldLayoutRoomGraph(layout);
  if (!materialized.accepted) {
    const CreativeWorldLayoutRoomCompileStatus status =
        materialized.status ==
                CreativeWorldLayoutRoomGraphStatus::OverlappingRooms
            ? CreativeWorldLayoutRoomCompileStatus::OverlappingRooms
        : materialized.status ==
                CreativeWorldLayoutRoomGraphStatus::OpeningHostInvalid
            ? CreativeWorldLayoutRoomCompileStatus::InvalidOpeningHost
            : CreativeWorldLayoutRoomCompileStatus::InvalidRoom;
    setFailure(result, status,
               status == CreativeWorldLayoutRoomCompileStatus::InvalidOpeningHost
                   ? materialized.failedOpeningIndex
                   : kInvalidCreativeWorldLayoutIndex,
               materialized.reasonCode);
    return result;
  }
  const CreativeWorldLayout& canonical = materialized.edited;
  const CreativeWorldLayoutRoomGraph graph =
      buildCreativeWorldLayoutRoomGraph(canonical);
  if (!graph.accepted) {
    setFailure(result,
               graph.status ==
                       CreativeWorldLayoutRoomGraphStatus::OverlappingRooms
                   ? CreativeWorldLayoutRoomCompileStatus::OverlappingRooms
                   : CreativeWorldLayoutRoomCompileStatus::InvalidRoom,
               graph.failedRoomIndex, graph.reasonCode);
    return result;
  }

  result.expanded = canonical;
  result.expanded.rooms.clear();
  result.expanded.topologyVertices.clear();
  result.expanded.topologyEdges.clear();
  result.expanded.roomBoundaries.clear();
  result.wallProvenance.resize(canonical.walls.size());

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
        !geometry.valid ||
        !std::isfinite(room.wallThicknessCells) ||
        room.wallThicknessCells <= 0.0) {
      setFailure(result, CreativeWorldLayoutRoomCompileStatus::InvalidRoom,
                 roomIndex, "creative_world_layout_room_invalid");
      return result;
    }
  }

  std::vector<std::vector<CreativeWorldLayoutRoomCompileResult::WallContributor>>
      contributors(graph.edges.size());
  for (const CreativeWorldLayoutRoomBoundary& boundary : graph.boundaries) {
    CreativeWorldLayoutRoomEdge cardinal = CreativeWorldLayoutRoomEdge::Count;
    const CreativeWorldLayoutTopologyEdge& edge =
        graph.edges[boundary.topologyEdgeIndex];
    const CreativeTerrainCoord2 start =
        graph.vertices[edge.startVertexIndex].position;
    const CreativeTerrainCoord2 end =
        graph.vertices[edge.endVertexIndex].position;
    const CreativeWorldLayoutRect bounds = graph.roomBounds[boundary.roomIndex];
    if (start.z == bounds.minimum.z && end.z == bounds.minimum.z) {
      cardinal = CreativeWorldLayoutRoomEdge::North;
    } else if (start.x == bounds.maximum.x && end.x == bounds.maximum.x) {
      cardinal = CreativeWorldLayoutRoomEdge::East;
    } else if (start.z == bounds.maximum.z && end.z == bounds.maximum.z) {
      cardinal = CreativeWorldLayoutRoomEdge::South;
    } else if (start.x == bounds.minimum.x && end.x == bounds.minimum.x) {
      cardinal = CreativeWorldLayoutRoomEdge::West;
    }
    contributors[boundary.topologyEdgeIndex].push_back(
        {boundary.roomIndex, cardinal, boundary.topologyEdgeIndex});
  }

  struct GraphEdgeLane {
    std::size_t edgeIndex = kInvalidCreativeWorldLayoutIndex;
    std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
    std::size_t levelIndex = kInvalidCreativeWorldLayoutIndex;
    double baseLayer = 0.0;
    std::uint16_t heightCells = 0U;
    double thicknessCells = 0.0;
    CreativeWorldLayoutWallProfile profile =
        CreativeWorldLayoutWallProfile::Automatic;
    CreativeStructuralMaterial material =
        CreativeStructuralMaterial::Blockout;
    CreativeWorldLayoutWallJoinStyle joinStyle =
        CreativeWorldLayoutWallJoinStyle::Square;
    bool horizontal = false;
    bool exterior = false;
    std::int32_t line = 0;
    std::int32_t begin = 0;
    std::int32_t end = 0;
  };
  struct BlockoutWallMaterials {
    bool valid = false;
    CreativeStructuralMaterial exterior =
        CreativeStructuralMaterial::Blockout;
    CreativeStructuralMaterial interior =
        CreativeStructuralMaterial::Blockout;
  };
  std::vector<BlockoutWallMaterials> blockoutWallMaterials(
      canonical.buildings.size());
  if (!graph.sourceWasExplicit) {
    for (std::size_t buildingIndex = 0U;
         buildingIndex < blockoutWallMaterials.size(); ++buildingIndex) {
      BlockoutWallMaterials& materials =
          blockoutWallMaterials[buildingIndex];
      materials.valid = creativeWorldLayoutBuildingBlockoutWallMaterial(
                            canonical, buildingIndex,
                            CreativeWorldLayoutWallProfile::Exterior,
                            materials.exterior) &&
                        creativeWorldLayoutBuildingBlockoutWallMaterial(
                            canonical, buildingIndex,
                            CreativeWorldLayoutWallProfile::Interior,
                            materials.interior);
    }
  }
  std::vector<GraphEdgeLane> lanes;
  lanes.reserve(graph.edges.size());
  for (std::size_t edgeIndex = 0U; edgeIndex < graph.edges.size();
       ++edgeIndex) {
    const CreativeWorldLayoutTopologyEdge& edge = graph.edges[edgeIndex];
    const CreativeWorldLayoutLevel& level = canonical.levels[edge.levelIndex];
    const CreativeTerrainCoord2 start =
        graph.vertices[edge.startVertexIndex].position;
    const CreativeTerrainCoord2 end =
        graph.vertices[edge.endVertexIndex].position;
    const bool horizontal = start.z == end.z;
    const CreativeWorldLayoutWallProfile profile =
        edge.profile == CreativeWorldLayoutWallProfile::Automatic
            ? (contributors[edgeIndex].size() == 1U
                   ? CreativeWorldLayoutWallProfile::Exterior
                   : CreativeWorldLayoutWallProfile::Interior)
            : edge.profile;
    CreativeStructuralMaterial material = edge.material;
    if (level.buildingIndex < blockoutWallMaterials.size() &&
        blockoutWallMaterials[level.buildingIndex].valid) {
      material = profile == CreativeWorldLayoutWallProfile::Exterior
                     ? blockoutWallMaterials[level.buildingIndex].exterior
                     : blockoutWallMaterials[level.buildingIndex].interior;
    }
    lanes.push_back({edgeIndex,
                     level.buildingIndex,
                     edge.levelIndex,
                     level.floorTopLayer,
                     edge.wallHeightCells == 0U ? level.wallHeightCells
                                                : edge.wallHeightCells,
                     edge.wallThicknessCells,
                     profile,
                     material,
                     edge.joinStyle,
                     horizontal,
                     contributors[edgeIndex].size() == 1U,
                     horizontal ? start.z : start.x,
                     horizontal ? start.x : start.z,
                     horizontal ? end.x : end.z});
  }
  std::sort(lanes.begin(), lanes.end(), [](const GraphEdgeLane& lhs,
                                           const GraphEdgeLane& rhs) {
    return std::tuple{lhs.buildingIndex, lhs.levelIndex, lhs.baseLayer,
                      lhs.heightCells, lhs.thicknessCells,
                      lhs.profile, lhs.material, lhs.joinStyle,
                      lhs.horizontal ? 0U : 1U, lhs.exterior, lhs.line,
                      lhs.begin, lhs.end, lhs.edgeIndex} <
           std::tuple{rhs.buildingIndex, rhs.levelIndex, rhs.baseLayer,
                      rhs.heightCells, rhs.thicknessCells,
                      rhs.profile, rhs.material, rhs.joinStyle,
                      rhs.horizontal ? 0U : 1U, rhs.exterior, rhs.line,
                      rhs.begin, rhs.end, rhs.edgeIndex};
  });

  std::vector<EdgeBinding> bindings(graph.edges.size());
  for (std::size_t cursor = 0U; cursor < lanes.size();) {
    const std::size_t laneBegin = cursor;
    std::int32_t mergedBegin = lanes[cursor].begin;
    std::int32_t mergedEnd = lanes[cursor].end;
    ++cursor;
    const auto sameRun = [](const GraphEdgeLane& lhs,
                            const GraphEdgeLane& rhs) {
      return lhs.buildingIndex == rhs.buildingIndex &&
             lhs.levelIndex == rhs.levelIndex &&
             lhs.baseLayer == rhs.baseLayer &&
             lhs.heightCells == rhs.heightCells &&
             lhs.thicknessCells == rhs.thicknessCells &&
             lhs.profile == rhs.profile && lhs.material == rhs.material &&
             lhs.joinStyle == rhs.joinStyle &&
             lhs.horizontal == rhs.horizontal &&
             lhs.exterior == rhs.exterior && lhs.line == rhs.line;
    };
    while (cursor < lanes.size() &&
           sameRun(lanes[laneBegin], lanes[cursor]) &&
           lanes[cursor].begin <= mergedEnd) {
      mergedEnd = std::max(mergedEnd, lanes[cursor].end);
      ++cursor;
    }

    const GraphEdgeLane& lane = lanes[laneBegin];
    CreativeWorldLayoutWall wall;
    wall.buildingIndex = lane.buildingIndex;
    wall.stableKey = canonical.levels[lane.levelIndex].stableKey +
                     ".topology.wall." +
                     (lane.horizontal ? "h." : "v.") +
                     std::to_string(lane.line) + "." +
                     std::to_string(mergedBegin) + "." +
                     std::to_string(mergedEnd);
    wall.name =
        "Room Wall " + std::to_string(result.expanded.walls.size() + 1U);
    wall.start = lane.horizontal
                     ? CreativeTerrainCoord2{mergedBegin, lane.line}
                     : CreativeTerrainCoord2{lane.line, mergedBegin};
    wall.end = lane.horizontal
                   ? CreativeTerrainCoord2{mergedEnd, lane.line}
                   : CreativeTerrainCoord2{lane.line, mergedEnd};
    wall.baseLayer = lane.baseLayer;
    wall.heightCells = lane.heightCells;
    wall.thicknessCells = lane.thicknessCells;
    wall.profile = lane.profile;
    wall.material = lane.material;
    wall.joinStyle = lane.joinStyle;
    const std::size_t wallIndex = result.expanded.walls.size();
    result.expanded.walls.push_back(std::move(wall));
    CreativeWorldLayoutRoomCompileResult::WallProvenance provenance;
    for (std::size_t laneIndex = laneBegin; laneIndex < cursor; ++laneIndex) {
      const GraphEdgeLane& edgeLane = lanes[laneIndex];
      const auto& edgeContributors = contributors[edgeLane.edgeIndex];
      provenance.contributors.insert(provenance.contributors.end(),
                                     edgeContributors.begin(),
                                     edgeContributors.end());
      bindings[edgeLane.edgeIndex] = {
          wallIndex, edgeLane.begin, mergedBegin,
          edgeLane.end - edgeLane.begin, edgeLane.baseLayer};
    }
    result.wallProvenance.push_back(std::move(provenance));
  }

  mergeContiguousExteriorFacades(canonical.walls.size(), result, bindings);

  for (std::size_t openingIndex = 0U;
       openingIndex < result.expanded.openings.size(); ++openingIndex) {
    CreativeWorldLayoutOpening& opening =
        result.expanded.openings[openingIndex];
    if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::Wall) {
      if (opening.wallIndex >= canonical.walls.size()) {
        setFailure(
            result, CreativeWorldLayoutRoomCompileStatus::InvalidOpeningHost,
            openingIndex, "creative_world_layout_opening_wall_host_invalid");
        return result;
      }
      continue;
    }
    if (opening.hostKind != CreativeWorldLayoutOpeningHostKind::RoomEdge ||
        opening.roomIndex >= canonical.rooms.size() ||
        opening.roomTopologyEdgeIndex >= bindings.size() ||
        !std::isfinite(opening.centerOffsetCells)) {
      setFailure(
          result, CreativeWorldLayoutRoomCompileStatus::InvalidOpeningHost,
          openingIndex, "creative_world_layout_opening_room_host_invalid");
      return result;
    }
    const EdgeBinding& binding = bindings[opening.roomTopologyEdgeIndex];
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
    opening.roomTopologyEdgeIndex = kInvalidCreativeWorldLayoutIndex;
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
  const CreativeWorldLayoutRoomGraph graph =
      buildCreativeWorldLayoutRoomGraph(layout);
  if (!graph.accepted) {
    return spans;
  }
  std::vector<std::vector<const CreativeWorldLayoutRoomBoundary*>> owners(
      graph.edges.size());
  for (const CreativeWorldLayoutRoomBoundary& boundary : graph.boundaries) {
    owners[boundary.topologyEdgeIndex].push_back(&boundary);
  }
  const auto cardinal = [&](const CreativeWorldLayoutRoomBoundary& boundary) {
    const CreativeWorldLayoutTopologyEdge& edge =
        graph.edges[boundary.topologyEdgeIndex];
    const CreativeTerrainCoord2 start =
        graph.vertices[edge.startVertexIndex].position;
    const CreativeTerrainCoord2 end =
        graph.vertices[edge.endVertexIndex].position;
    const CreativeWorldLayoutRect bounds = graph.roomBounds[boundary.roomIndex];
    if (start.z == bounds.minimum.z && end.z == bounds.minimum.z) {
      return CreativeWorldLayoutRoomEdge::North;
    }
    if (start.x == bounds.maximum.x && end.x == bounds.maximum.x) {
      return CreativeWorldLayoutRoomEdge::East;
    }
    if (start.z == bounds.maximum.z && end.z == bounds.maximum.z) {
      return CreativeWorldLayoutRoomEdge::South;
    }
    if (start.x == bounds.minimum.x && end.x == bounds.minimum.x) {
      return CreativeWorldLayoutRoomEdge::West;
    }
    return CreativeWorldLayoutRoomEdge::Count;
  };
  for (std::size_t edgeIndex = 0U; edgeIndex < owners.size(); ++edgeIndex) {
    if (owners[edgeIndex].size() != 2U) {
      continue;
    }
    const CreativeWorldLayoutRoomBoundary* first = owners[edgeIndex][0];
    const CreativeWorldLayoutRoomBoundary* second = owners[edgeIndex][1];
    if (second->roomIndex < first->roomIndex) {
      std::swap(first, second);
    }
    const CreativeWorldLayoutTopologyEdge& edge = graph.edges[edgeIndex];
    spans.push_back({first->roomIndex,
                     cardinal(*first),
                     second->roomIndex,
                     cardinal(*second),
                     graph.vertices[edge.startVertexIndex].position,
                     graph.vertices[edge.endVertexIndex].position});
  }
  std::sort(spans.begin(), spans.end(), [](const auto& lhs, const auto& rhs) {
    return std::tie(lhs.firstRoomIndex, lhs.secondRoomIndex, lhs.start.x,
                    lhs.start.z, lhs.end.x, lhs.end.z) <
           std::tie(rhs.firstRoomIndex, rhs.secondRoomIndex, rhs.start.x,
                    rhs.start.z, rhs.end.x, rhs.end.z);
  });
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
  const CreativeWorldLayoutRoomGraph graph =
      buildCreativeWorldLayoutRoomGraph(layout);
  if (!graph.accepted) {
    return false;
  }
  std::vector<std::size_t> edgeUseCount(graph.edges.size(), 0U);
  for (const CreativeWorldLayoutRoomBoundary& boundary : graph.boundaries) {
    ++edgeUseCount[boundary.topologyEdgeIndex];
  }
  const CreativeWorldLayoutRect bounds = graph.roomBounds[roomIndex];
  const bool horizontal = roomEdge == CreativeWorldLayoutRoomEdge::North ||
                          roomEdge == CreativeWorldLayoutRoomEdge::South;
  const double origin = horizontal ? static_cast<double>(bounds.minimum.x)
                                   : static_cast<double>(bounds.minimum.z);
  for (const CreativeWorldLayoutRoomBoundary& boundary :
       creativeWorldLayoutRoomBoundaries(graph, roomIndex)) {
    if (edgeUseCount[boundary.topologyEdgeIndex] != 2U) {
      continue;
    }
    const CreativeWorldLayoutTopologyEdge& edge =
        graph.edges[boundary.topologyEdgeIndex];
    const CreativeTerrainCoord2 start =
        graph.vertices[edge.startVertexIndex].position;
    const CreativeTerrainCoord2 end =
        graph.vertices[edge.endVertexIndex].position;
    const bool sideMatches =
        (roomEdge == CreativeWorldLayoutRoomEdge::North &&
         start.z == bounds.minimum.z && end.z == bounds.minimum.z) ||
        (roomEdge == CreativeWorldLayoutRoomEdge::East &&
         start.x == bounds.maximum.x && end.x == bounds.maximum.x) ||
        (roomEdge == CreativeWorldLayoutRoomEdge::South &&
         start.z == bounds.maximum.z && end.z == bounds.maximum.z) ||
        (roomEdge == CreativeWorldLayoutRoomEdge::West &&
         start.x == bounds.minimum.x && end.x == bounds.minimum.x);
    if (!sideMatches) {
      continue;
    }
    const double sharedBegin =
        (horizontal ? static_cast<double>(start.x)
                    : static_cast<double>(start.z)) -
        origin;
    const double sharedEnd =
        (horizontal ? static_cast<double>(end.x)
                    : static_cast<double>(end.z)) -
        origin;
    if (std::max(intervalBegin, sharedBegin) <
        std::min(intervalEnd, sharedEnd)) {
      return true;
    }
  }
  return false;
}

bool creativeWorldLayoutHasInteriorRoomWindow(
    const CreativeWorldLayout& layout) {
  const CreativeWorldLayoutRoomGraph graph =
      buildCreativeWorldLayoutRoomGraph(layout);
  std::vector<std::size_t> edgeUseCount;
  if (graph.accepted) {
    edgeUseCount.assign(graph.edges.size(), 0U);
    for (const CreativeWorldLayoutRoomBoundary& boundary : graph.boundaries) {
      ++edgeUseCount[boundary.topologyEdgeIndex];
    }
  }
  for (const CreativeWorldLayoutOpening& opening : layout.openings) {
    if (opening.kind == CreativeBuildingOpeningKind::Window &&
        opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge) {
      if (opening.roomTopologyEdgeIndex < edgeUseCount.size() &&
          edgeUseCount[opening.roomTopologyEdgeIndex] == 2U) {
        return true;
      }
      if (opening.roomTopologyEdgeIndex == kInvalidCreativeWorldLayoutIndex &&
          creativeWorldLayoutRoomEdgeIntervalIsShared(
              layout, opening.roomIndex, opening.roomEdge,
              opening.centerOffsetCells, opening.widthCells)) {
        return true;
      }
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
