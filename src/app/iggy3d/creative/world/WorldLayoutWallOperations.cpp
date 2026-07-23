#include "app/iggy3d/creative/world/WorldLayoutWallOperations.hpp"

#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoomTopology.hpp"

#include <algorithm>
#include <cmath>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

using Point = CreativeTerrainCoord2;

constexpr double kGeometryEpsilon = 1.0e-9;

[[nodiscard]] bool horizontal(Point start, Point end) noexcept {
  return start.z == end.z && start.x < end.x;
}

[[nodiscard]] std::uint32_t edgeLength(Point start, Point end) noexcept {
  const std::int64_t length = horizontal(start, end)
                                  ? static_cast<std::int64_t>(end.x) - start.x
                                  : static_cast<std::int64_t>(end.z) - start.z;
  return length > 0 ? static_cast<std::uint32_t>(length) : 0U;
}

[[nodiscard]] double edgeOrigin(Point start, Point end) noexcept {
  return horizontal(start, end) ? static_cast<double>(start.x)
                                : static_cast<double>(start.z);
}

[[nodiscard]] Point directedBoundaryStart(
    const CreativeWorldLayoutRoomGraph& graph,
    const CreativeWorldLayoutRoomBoundary& boundary) noexcept {
  const CreativeWorldLayoutTopologyEdge& edge =
      graph.edges[boundary.topologyEdgeIndex];
  return graph.vertices[boundary.reversed ? edge.endVertexIndex
                                          : edge.startVertexIndex]
      .position;
}

[[nodiscard]] bool sameWallAttributes(
    const CreativeWorldLayoutTopologyEdge& lhs,
    const CreativeWorldLayoutTopologyEdge& rhs) noexcept {
  return lhs.levelIndex == rhs.levelIndex &&
         lhs.wallThicknessCells == rhs.wallThicknessCells &&
         lhs.wallHeightCells == rhs.wallHeightCells &&
         lhs.profile == rhs.profile && lhs.material == rhs.material &&
         lhs.joinStyle == rhs.joinStyle;
}

void reject(CreativeWorldLayoutWallOperationResult& result,
            CreativeWorldLayoutWallOperationStatus status,
            std::string reasonCode) {
  result.accepted = false;
  result.changed = false;
  result.status = status;
  result.edited = {};
  result.sourceToEditedVertexIndices.clear();
  result.sourceToEditedEdgeIndices.clear();
  result.reasonCode = std::move(reasonCode);
}

[[nodiscard]] bool explicitAcceptedGraph(
    const CreativeWorldLayout& source,
    CreativeWorldLayoutRoomGraph& graph) {
  graph = buildCreativeWorldLayoutRoomGraph(source);
  return graph.accepted && graph.sourceWasExplicit;
}

[[nodiscard]] std::vector<std::size_t> edgeOwners(
    const CreativeWorldLayoutRoomGraph& graph,
    std::size_t edgeIndex) {
  std::vector<std::size_t> owners;
  for (const CreativeWorldLayoutRoomBoundary& boundary : graph.boundaries) {
    if (boundary.topologyEdgeIndex == edgeIndex) {
      owners.push_back(boundary.roomIndex);
    }
  }
  std::sort(owners.begin(), owners.end());
  return owners;
}

[[nodiscard]] bool verticalIntervalsOverlap(
    const CreativeWorldLayoutOpening& lhs,
    const CreativeWorldLayoutOpening& rhs) noexcept {
  const double lhsTop = lhs.cutoutBottomCells + lhs.cutoutHeightCells;
  const double rhsTop = rhs.cutoutBottomCells + rhs.cutoutHeightCells;
  return lhsTop > rhs.cutoutBottomCells + kGeometryEpsilon &&
         rhsTop > lhs.cutoutBottomCells + kGeometryEpsilon;
}

[[nodiscard]] bool hostedOpeningsOverlap(
    const CreativeWorldLayout& layout,
    std::size_t edgeIndex,
    std::size_t& failedOpeningIndex) noexcept {
  for (std::size_t index = 0U; index < layout.openings.size(); ++index) {
    const CreativeWorldLayoutOpening& opening = layout.openings[index];
    if (opening.hostKind != CreativeWorldLayoutOpeningHostKind::RoomEdge ||
        opening.roomTopologyEdgeIndex != edgeIndex) {
      continue;
    }
    for (std::size_t prior = 0U; prior < index; ++prior) {
      const CreativeWorldLayoutOpening& other = layout.openings[prior];
      if (other.hostKind != CreativeWorldLayoutOpeningHostKind::RoomEdge ||
          other.roomTopologyEdgeIndex != edgeIndex) {
        continue;
      }
      const double separation =
          std::fabs(opening.centerOffsetCells - other.centerOffsetCells);
      if (separation + kGeometryEpsilon <
              (opening.widthCells + other.widthCells) * 0.5 &&
          verticalIntervalsOverlap(opening, other)) {
        failedOpeningIndex = index;
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] std::size_t remapAfterErase(std::size_t index,
                                           std::size_t erased) noexcept {
  if (index == erased) {
    return kInvalidCreativeWorldLayoutIndex;
  }
  return index > erased ? index - 1U : index;
}

}  // namespace

CreativeWorldLayoutWallOperationResult splitCreativeWorldLayoutWall(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutWallSplitRequest& request) {
  CreativeWorldLayoutWallOperationResult result;
  result.requested = true;
  result.topologyEdgeIndex = request.topologyEdgeIndex;

  CreativeWorldLayoutRoomGraph graph;
  if (!explicitAcceptedGraph(source, graph)) {
    reject(result,
           CreativeWorldLayoutWallOperationStatus::InvalidSourceTopology,
           "creative_world_layout_wall_split_source_topology_invalid");
    return result;
  }
  if (request.topologyEdgeIndex >= graph.edges.size() ||
      !validCreativeWorldLayoutStableKey(request.newVertexStableKey) ||
      !validCreativeWorldLayoutStableKey(request.newEdgeStableKey)) {
    reject(result, CreativeWorldLayoutWallOperationStatus::InvalidRequest,
           "creative_world_layout_wall_split_request_invalid");
    return result;
  }
  if (creativeWorldLayoutStableKeyExists(source,
                                         request.newVertexStableKey) ||
      creativeWorldLayoutStableKeyExists(source, request.newEdgeStableKey) ||
      request.newVertexStableKey == request.newEdgeStableKey) {
    reject(result, CreativeWorldLayoutWallOperationStatus::DuplicateStableKey,
           "creative_world_layout_wall_split_stable_key_duplicate");
    return result;
  }

  const CreativeWorldLayoutTopologyEdge& sourceEdge =
      graph.edges[request.topologyEdgeIndex];
  const Point sourceStart =
      graph.vertices[sourceEdge.startVertexIndex].position;
  const Point sourceEnd = graph.vertices[sourceEdge.endVertexIndex].position;
  const std::uint32_t sourceLength = edgeLength(sourceStart, sourceEnd);
  if (request.offsetCells == 0U || request.offsetCells >= sourceLength) {
    reject(result,
           CreativeWorldLayoutWallOperationStatus::SplitOutsideInterior,
           "creative_world_layout_wall_split_outside_interior");
    return result;
  }

  for (const CreativeWorldLayoutRoomBoundary& boundary : graph.boundaries) {
    if (boundary.topologyEdgeIndex == request.topologyEdgeIndex &&
        graph.rooms[boundary.roomIndex].boundaryCount >=
            kCreativeWorldLayoutMaximumRoomBoundaryEdgeCount) {
      reject(result,
             CreativeWorldLayoutWallOperationStatus::BoundaryCapacityExceeded,
             "creative_world_layout_wall_split_boundary_capacity_exceeded");
      return result;
    }
  }

  for (std::size_t index = 0U; index < source.openings.size(); ++index) {
    const CreativeWorldLayoutOpening& opening = source.openings[index];
    if (opening.hostKind != CreativeWorldLayoutOpeningHostKind::RoomEdge ||
        opening.roomTopologyEdgeIndex != request.topologyEdgeIndex) {
      continue;
    }
    if (!std::isfinite(opening.centerOffsetCells) ||
        !std::isfinite(opening.widthCells) || opening.widthCells <= 0.0) {
      result.failedOpeningIndex = index;
      reject(result, CreativeWorldLayoutWallOperationStatus::OpeningConflict,
             "creative_world_layout_wall_split_opening_invalid");
      result.failedOpeningIndex = index;
      return result;
    }
    const double minimum =
        opening.centerOffsetCells - opening.widthCells * 0.5;
    const double maximum =
        opening.centerOffsetCells + opening.widthCells * 0.5;
    const double cut = static_cast<double>(request.offsetCells);
    if (maximum > cut - kCreativeWorldLayoutRoomOpeningEndClearanceCells +
                      kGeometryEpsilon &&
        minimum < cut + kCreativeWorldLayoutRoomOpeningEndClearanceCells -
                      kGeometryEpsilon) {
      result.failedOpeningIndex = index;
      reject(result, CreativeWorldLayoutWallOperationStatus::OpeningConflict,
             "creative_world_layout_wall_split_crosses_opening");
      result.failedOpeningIndex = index;
      return result;
    }
  }

  result.edited = source;
  const Point splitPoint = horizontal(sourceStart, sourceEnd)
                               ? Point{sourceStart.x +
                                           static_cast<std::int32_t>(
                                               request.offsetCells),
                                       sourceStart.z}
                               : Point{sourceStart.x,
                                       sourceStart.z +
                                           static_cast<std::int32_t>(
                                               request.offsetCells)};
  const std::size_t newVertexIndex = result.edited.topologyVertices.size();
  result.edited.topologyVertices.push_back(
      {sourceEdge.levelIndex, request.newVertexStableKey, splitPoint});

  CreativeWorldLayoutTopologyEdge& retained =
      result.edited.topologyEdges[request.topologyEdgeIndex];
  const std::size_t oldEndVertexIndex = retained.endVertexIndex;
  retained.endVertexIndex = newVertexIndex;
  CreativeWorldLayoutTopologyEdge appended = retained;
  appended.stableKey = request.newEdgeStableKey;
  appended.startVertexIndex = newVertexIndex;
  appended.endVertexIndex = oldEndVertexIndex;
  const std::size_t newEdgeIndex = result.edited.topologyEdges.size();
  result.edited.topologyEdges.push_back(std::move(appended));

  result.edited.roomBoundaries.clear();
  result.edited.roomBoundaries.reserve(graph.boundaries.size() + 2U);
  for (std::size_t roomIndex = 0U; roomIndex < graph.rooms.size(); ++roomIndex) {
    std::size_t order = 0U;
    for (const CreativeWorldLayoutRoomBoundary& boundary :
         creativeWorldLayoutRoomBoundaries(graph, roomIndex)) {
      if (boundary.topologyEdgeIndex != request.topologyEdgeIndex) {
        result.edited.roomBoundaries.push_back(
            {roomIndex, boundary.topologyEdgeIndex, order++,
             boundary.reversed});
        continue;
      }
      if (boundary.reversed) {
        result.edited.roomBoundaries.push_back(
            {roomIndex, newEdgeIndex, order++, true});
        result.edited.roomBoundaries.push_back(
            {roomIndex, request.topologyEdgeIndex, order++, true});
      } else {
        result.edited.roomBoundaries.push_back(
            {roomIndex, request.topologyEdgeIndex, order++, false});
        result.edited.roomBoundaries.push_back(
            {roomIndex, newEdgeIndex, order++, false});
      }
    }
  }

  for (CreativeWorldLayoutOpening& opening : result.edited.openings) {
    if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge &&
        opening.roomTopologyEdgeIndex == request.topologyEdgeIndex &&
        opening.centerOffsetCells > static_cast<double>(request.offsetCells)) {
      opening.roomTopologyEdgeIndex = newEdgeIndex;
      opening.centerOffsetCells -= static_cast<double>(request.offsetCells);
    }
  }

  const CreativeWorldLayoutRoomGraph editedGraph =
      buildCreativeWorldLayoutRoomGraph(result.edited);
  if (!editedGraph.accepted) {
    reject(result,
           CreativeWorldLayoutWallOperationStatus::ResultingTopologyInvalid,
           "creative_world_layout_wall_split_result_invalid");
    return result;
  }

  result.sourceToEditedVertexIndices.resize(source.topologyVertices.size());
  for (std::size_t index = 0U; index < source.topologyVertices.size(); ++index) {
    result.sourceToEditedVertexIndices[index] = index;
  }
  result.sourceToEditedEdgeIndices.resize(source.topologyEdges.size());
  for (std::size_t index = 0U; index < source.topologyEdges.size(); ++index) {
    result.sourceToEditedEdgeIndices[index] = index;
  }
  result.accepted = true;
  result.changed = true;
  result.status = CreativeWorldLayoutWallOperationStatus::Ready;
  result.newTopologyEdgeIndex = newEdgeIndex;
  result.reasonCode = "creative_world_layout_wall_split_ready";
  return result;
}

CreativeWorldLayoutWallOperationResult mergeCreativeWorldLayoutWalls(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutWallMergeRequest& request) {
  CreativeWorldLayoutWallOperationResult result;
  result.requested = true;
  result.topologyEdgeIndex = request.primaryTopologyEdgeIndex;

  CreativeWorldLayoutRoomGraph graph;
  if (!explicitAcceptedGraph(source, graph)) {
    reject(result,
           CreativeWorldLayoutWallOperationStatus::InvalidSourceTopology,
           "creative_world_layout_wall_merge_source_topology_invalid");
    return result;
  }
  if (request.primaryTopologyEdgeIndex >= graph.edges.size() ||
      request.secondaryTopologyEdgeIndex >= graph.edges.size() ||
      request.primaryTopologyEdgeIndex == request.secondaryTopologyEdgeIndex) {
    reject(result, CreativeWorldLayoutWallOperationStatus::InvalidRequest,
           "creative_world_layout_wall_merge_request_invalid");
    return result;
  }

  const CreativeWorldLayoutTopologyEdge& primary =
      graph.edges[request.primaryTopologyEdgeIndex];
  const CreativeWorldLayoutTopologyEdge& secondary =
      graph.edges[request.secondaryTopologyEdgeIndex];
  if (!sameWallAttributes(primary, secondary)) {
    reject(result, CreativeWorldLayoutWallOperationStatus::AttributesDiffer,
           "creative_world_layout_wall_merge_attributes_differ");
    return result;
  }

  const Point primaryStart =
      graph.vertices[primary.startVertexIndex].position;
  const Point primaryEnd = graph.vertices[primary.endVertexIndex].position;
  const Point secondaryStart =
      graph.vertices[secondary.startVertexIndex].position;
  const Point secondaryEnd =
      graph.vertices[secondary.endVertexIndex].position;
  std::size_t sharedVertexIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t outerStartVertexIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t outerEndVertexIndex = kInvalidCreativeWorldLayoutIndex;
  if (primary.endVertexIndex == secondary.startVertexIndex &&
      ((horizontal(primaryStart, primaryEnd) &&
        horizontal(secondaryStart, secondaryEnd) &&
        primaryStart.z == secondaryStart.z) ||
       (!horizontal(primaryStart, primaryEnd) &&
        !horizontal(secondaryStart, secondaryEnd) &&
        primaryStart.x == secondaryStart.x))) {
    sharedVertexIndex = primary.endVertexIndex;
    outerStartVertexIndex = primary.startVertexIndex;
    outerEndVertexIndex = secondary.endVertexIndex;
  } else if (secondary.endVertexIndex == primary.startVertexIndex &&
             ((horizontal(primaryStart, primaryEnd) &&
               horizontal(secondaryStart, secondaryEnd) &&
               primaryStart.z == secondaryStart.z) ||
              (!horizontal(primaryStart, primaryEnd) &&
               !horizontal(secondaryStart, secondaryEnd) &&
               primaryStart.x == secondaryStart.x))) {
    sharedVertexIndex = primary.startVertexIndex;
    outerStartVertexIndex = secondary.startVertexIndex;
    outerEndVertexIndex = primary.endVertexIndex;
  } else {
    reject(result,
           CreativeWorldLayoutWallOperationStatus::EdgesNotMergeable,
           "creative_world_layout_wall_merge_edges_not_adjacent_collinear");
    return result;
  }

  if (edgeOwners(graph, request.primaryTopologyEdgeIndex) !=
      edgeOwners(graph, request.secondaryTopologyEdgeIndex)) {
    reject(result,
           CreativeWorldLayoutWallOperationStatus::EdgesNotMergeable,
           "creative_world_layout_wall_merge_ownership_differs");
    return result;
  }
  const std::size_t incidentCount = static_cast<std::size_t>(std::count_if(
      graph.edges.begin(), graph.edges.end(),
      [&](const CreativeWorldLayoutTopologyEdge& edge) {
        return edge.startVertexIndex == sharedVertexIndex ||
               edge.endVertexIndex == sharedVertexIndex;
      }));
  if (incidentCount != 2U) {
    reject(result, CreativeWorldLayoutWallOperationStatus::JunctionInUse,
           "creative_world_layout_wall_merge_junction_in_use");
    return result;
  }

  struct RoomMerge {
    std::size_t roomIndex = kInvalidCreativeWorldLayoutIndex;
    std::size_t firstLocalIndex = kInvalidCreativeWorldLayoutIndex;
    bool reversed = false;
  };
  std::vector<RoomMerge> roomMerges;
  for (std::size_t roomIndex : edgeOwners(
           graph, request.primaryTopologyEdgeIndex)) {
    const std::span<const CreativeWorldLayoutRoomBoundary> boundaries =
        creativeWorldLayoutRoomBoundaries(graph, roomIndex);
    bool found = false;
    for (std::size_t index = 0U; index < boundaries.size(); ++index) {
      const std::size_t next = (index + 1U) % boundaries.size();
      const std::size_t firstEdge = boundaries[index].topologyEdgeIndex;
      const std::size_t secondEdge = boundaries[next].topologyEdgeIndex;
      if (!((firstEdge == request.primaryTopologyEdgeIndex &&
             secondEdge == request.secondaryTopologyEdgeIndex) ||
            (firstEdge == request.secondaryTopologyEdgeIndex &&
             secondEdge == request.primaryTopologyEdgeIndex))) {
        continue;
      }
      const Point directedStart = directedBoundaryStart(graph,
                                                        boundaries[index]);
      const Point outerStart =
          graph.vertices[outerStartVertexIndex].position;
      const Point outerEnd = graph.vertices[outerEndVertexIndex].position;
      if (directedStart != outerStart && directedStart != outerEnd) {
        continue;
      }
      roomMerges.push_back({roomIndex, index, directedStart == outerEnd});
      found = true;
      break;
    }
    if (!found) {
      reject(result,
             CreativeWorldLayoutWallOperationStatus::EdgesNotMergeable,
             "creative_world_layout_wall_merge_boundaries_not_adjacent");
      return result;
    }
  }

  result.edited = source;
  CreativeWorldLayoutTopologyEdge& retained =
      result.edited.topologyEdges[request.primaryTopologyEdgeIndex];
  retained.startVertexIndex = outerStartVertexIndex;
  retained.endVertexIndex = outerEndVertexIndex;

  const double mergedOrigin = edgeOrigin(
      graph.vertices[outerStartVertexIndex].position,
      graph.vertices[outerEndVertexIndex].position);
  for (CreativeWorldLayoutOpening& opening : result.edited.openings) {
    if (opening.hostKind != CreativeWorldLayoutOpeningHostKind::RoomEdge ||
        (opening.roomTopologyEdgeIndex != request.primaryTopologyEdgeIndex &&
         opening.roomTopologyEdgeIndex != request.secondaryTopologyEdgeIndex)) {
      continue;
    }
    const CreativeWorldLayoutTopologyEdge& oldEdge =
        graph.edges[opening.roomTopologyEdgeIndex];
    const Point oldStart = graph.vertices[oldEdge.startVertexIndex].position;
    const Point oldEnd = graph.vertices[oldEdge.endVertexIndex].position;
    const double worldCenter =
        edgeOrigin(oldStart, oldEnd) + opening.centerOffsetCells;
    opening.roomTopologyEdgeIndex = request.primaryTopologyEdgeIndex;
    opening.centerOffsetCells = worldCenter - mergedOrigin;
  }

  const std::size_t removedEdgeIndex = request.secondaryTopologyEdgeIndex;
  const std::size_t survivingEdgeIndex =
      remapAfterErase(request.primaryTopologyEdgeIndex, removedEdgeIndex);
  result.edited.roomBoundaries.clear();
  result.edited.roomBoundaries.reserve(graph.boundaries.size() -
                                       roomMerges.size());
  for (std::size_t roomIndex = 0U; roomIndex < graph.rooms.size(); ++roomIndex) {
    const std::span<const CreativeWorldLayoutRoomBoundary> boundaries =
        creativeWorldLayoutRoomBoundaries(graph, roomIndex);
    const auto merge = std::find_if(
        roomMerges.begin(), roomMerges.end(),
        [roomIndex](const RoomMerge& item) {
          return item.roomIndex == roomIndex;
        });
    std::size_t order = 0U;
    if (merge == roomMerges.end()) {
      for (const CreativeWorldLayoutRoomBoundary& boundary : boundaries) {
        result.edited.roomBoundaries.push_back(
            {roomIndex,
             remapAfterErase(boundary.topologyEdgeIndex, removedEdgeIndex),
             order++, boundary.reversed});
      }
      continue;
    }
    result.edited.roomBoundaries.push_back(
        {roomIndex, survivingEdgeIndex, order++, merge->reversed});
    for (std::size_t offset = 2U; offset < boundaries.size(); ++offset) {
      const CreativeWorldLayoutRoomBoundary& boundary =
          boundaries[(merge->firstLocalIndex + offset) % boundaries.size()];
      result.edited.roomBoundaries.push_back(
          {roomIndex,
           remapAfterErase(boundary.topologyEdgeIndex, removedEdgeIndex),
           order++, boundary.reversed});
    }
  }

  result.edited.topologyEdges.erase(
      result.edited.topologyEdges.begin() +
      static_cast<std::ptrdiff_t>(removedEdgeIndex));
  for (CreativeWorldLayoutOpening& opening : result.edited.openings) {
    if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge &&
        opening.roomTopologyEdgeIndex != kInvalidCreativeWorldLayoutIndex) {
      opening.roomTopologyEdgeIndex =
          opening.roomTopologyEdgeIndex == request.primaryTopologyEdgeIndex ||
                  opening.roomTopologyEdgeIndex == removedEdgeIndex
              ? survivingEdgeIndex
              : remapAfterErase(opening.roomTopologyEdgeIndex,
                                removedEdgeIndex);
    }
  }

  result.edited.topologyVertices.erase(
      result.edited.topologyVertices.begin() +
      static_cast<std::ptrdiff_t>(sharedVertexIndex));
  for (CreativeWorldLayoutTopologyEdge& edge : result.edited.topologyEdges) {
    edge.startVertexIndex =
        remapAfterErase(edge.startVertexIndex, sharedVertexIndex);
    edge.endVertexIndex = remapAfterErase(edge.endVertexIndex,
                                         sharedVertexIndex);
  }

  if (hostedOpeningsOverlap(result.edited, survivingEdgeIndex,
                            result.failedOpeningIndex)) {
    const std::size_t failedOpening = result.failedOpeningIndex;
    reject(result, CreativeWorldLayoutWallOperationStatus::OpeningConflict,
           "creative_world_layout_wall_merge_openings_overlap");
    result.failedOpeningIndex = failedOpening;
    return result;
  }
  const CreativeWorldLayoutRoomGraph editedGraph =
      buildCreativeWorldLayoutRoomGraph(result.edited);
  if (!editedGraph.accepted) {
    reject(result,
           CreativeWorldLayoutWallOperationStatus::ResultingTopologyInvalid,
           "creative_world_layout_wall_merge_result_invalid");
    return result;
  }

  result.sourceToEditedVertexIndices.resize(source.topologyVertices.size());
  for (std::size_t index = 0U; index < source.topologyVertices.size(); ++index) {
    result.sourceToEditedVertexIndices[index] =
        remapAfterErase(index, sharedVertexIndex);
  }
  result.sourceToEditedEdgeIndices.resize(source.topologyEdges.size());
  for (std::size_t index = 0U; index < source.topologyEdges.size(); ++index) {
    result.sourceToEditedEdgeIndices[index] =
        index == request.primaryTopologyEdgeIndex ||
                index == request.secondaryTopologyEdgeIndex
            ? survivingEdgeIndex
            : remapAfterErase(index, removedEdgeIndex);
  }
  result.accepted = true;
  result.changed = true;
  result.status = CreativeWorldLayoutWallOperationStatus::Ready;
  result.topologyEdgeIndex = survivingEdgeIndex;
  result.reasonCode = "creative_world_layout_wall_merge_ready";
  return result;
}

}  // namespace iggy3d::creative
