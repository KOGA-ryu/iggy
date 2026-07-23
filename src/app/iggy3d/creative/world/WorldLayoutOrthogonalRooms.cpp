#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"

#include "app/iggy3d/creative/world/WorldLayoutBlockout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

using Point = CreativeTerrainCoord2;
using Rect = CreativeWorldLayoutRect;

struct VertexKey {
  std::size_t levelIndex = 0U;
  Point point{};

  [[nodiscard]] friend bool operator<(const VertexKey& lhs,
                                      const VertexKey& rhs) noexcept {
    return std::tie(lhs.levelIndex, lhs.point.x, lhs.point.z) <
           std::tie(rhs.levelIndex, rhs.point.x, rhs.point.z);
  }
};

struct EdgeKey {
  std::size_t levelIndex = 0U;
  Point start{};
  Point end{};

  [[nodiscard]] friend bool operator<(const EdgeKey& lhs,
                                      const EdgeKey& rhs) noexcept {
    return std::tie(lhs.levelIndex, lhs.start.x, lhs.start.z, lhs.end.x,
                    lhs.end.z) <
           std::tie(rhs.levelIndex, rhs.start.x, rhs.start.z, rhs.end.x,
                    rhs.end.z);
  }
};

struct DirectedSegment {
  std::size_t roomIndex = 0U;
  std::size_t levelIndex = 0U;
  Point start{};
  Point end{};
};

[[nodiscard]] bool pointLess(Point lhs, Point rhs) noexcept {
  return std::tie(lhs.x, lhs.z) < std::tie(rhs.x, rhs.z);
}

[[nodiscard]] bool validRect(Rect rect) noexcept {
  return rect.minimum.x < rect.maximum.x &&
         rect.minimum.z < rect.maximum.z;
}

[[nodiscard]] bool sameRect(Rect lhs, Rect rhs) noexcept {
  return lhs.minimum == rhs.minimum && lhs.maximum == rhs.maximum;
}

[[nodiscard]] double rectWidth(Rect rect) noexcept {
  return static_cast<double>(static_cast<std::int64_t>(rect.maximum.x) -
                             rect.minimum.x);
}

[[nodiscard]] double rectDepth(Rect rect) noexcept {
  return static_cast<double>(static_cast<std::int64_t>(rect.maximum.z) -
                             rect.minimum.z);
}

[[nodiscard]] bool orthogonal(Point start, Point end) noexcept {
  return (start.x == end.x) != (start.z == end.z);
}

[[nodiscard]] bool pointOnSegmentInterior(Point point,
                                          Point start,
                                          Point end) noexcept {
  if (start.x == end.x && point.x == start.x) {
    return point.z > std::min(start.z, end.z) &&
           point.z < std::max(start.z, end.z);
  }
  if (start.z == end.z && point.z == start.z) {
    return point.x > std::min(start.x, end.x) &&
           point.x < std::max(start.x, end.x);
  }
  return false;
}

[[nodiscard]] bool rectsOverlap(Rect lhs, Rect rhs) noexcept {
  return std::max(lhs.minimum.x, rhs.minimum.x) <
             std::min(lhs.maximum.x, rhs.maximum.x) &&
         std::max(lhs.minimum.z, rhs.minimum.z) <
             std::min(lhs.maximum.z, rhs.maximum.z);
}

[[nodiscard]] std::uint64_t rectArea(Rect rect) noexcept {
  if (!validRect(rect)) {
    return 0U;
  }
  const std::uint64_t width = static_cast<std::uint64_t>(
      static_cast<std::int64_t>(rect.maximum.x) - rect.minimum.x);
  const std::uint64_t depth = static_cast<std::uint64_t>(
      static_cast<std::int64_t>(rect.maximum.z) - rect.minimum.z);
  return width * depth;
}

[[nodiscard]] std::string vertexStableKey(const CreativeWorldLayout& layout,
                                          std::size_t levelIndex,
                                          Point point) {
  return layout.levels[levelIndex].stableKey + ".topology.vertex." +
         std::to_string(point.x) + "." + std::to_string(point.z);
}

[[nodiscard]] std::string edgeStableKey(const CreativeWorldLayout& layout,
                                        std::size_t levelIndex,
                                        Point start,
                                        Point end) {
  return layout.levels[levelIndex].stableKey + ".topology.edge." +
         std::to_string(start.x) + "." + std::to_string(start.z) + "." +
         std::to_string(end.x) + "." + std::to_string(end.z);
}

void reject(CreativeWorldLayoutRoomGraph& graph,
            CreativeWorldLayoutRoomGraphStatus status,
            std::string reasonCode) {
  graph.accepted = false;
  graph.status = status;
  graph.reasonCode = std::move(reasonCode);
}

[[nodiscard]] Point boundaryStart(
    const CreativeWorldLayoutRoomGraph& graph,
    const CreativeWorldLayoutRoomBoundary& boundary) noexcept {
  const CreativeWorldLayoutTopologyEdge& edge =
      graph.edges[boundary.topologyEdgeIndex];
  return graph.vertices[boundary.reversed ? edge.endVertexIndex
                                          : edge.startVertexIndex]
      .position;
}

[[nodiscard]] Point boundaryEnd(
    const CreativeWorldLayoutRoomGraph& graph,
    const CreativeWorldLayoutRoomBoundary& boundary) noexcept {
  const CreativeWorldLayoutTopologyEdge& edge =
      graph.edges[boundary.topologyEdgeIndex];
  return graph.vertices[boundary.reversed ? edge.startVertexIndex
                                          : edge.endVertexIndex]
      .position;
}

[[nodiscard]] bool segmentsIntersect(Point a,
                                     Point b,
                                     Point c,
                                     Point d) noexcept {
  const bool firstVertical = a.x == b.x;
  const bool secondVertical = c.x == d.x;
  if (firstVertical == secondVertical) {
    if (firstVertical && a.x != c.x) {
      return false;
    }
    if (!firstVertical && a.z != c.z) {
      return false;
    }
    const std::int32_t a0 = firstVertical ? std::min(a.z, b.z)
                                          : std::min(a.x, b.x);
    const std::int32_t a1 = firstVertical ? std::max(a.z, b.z)
                                          : std::max(a.x, b.x);
    const std::int32_t c0 = firstVertical ? std::min(c.z, d.z)
                                          : std::min(c.x, d.x);
    const std::int32_t c1 = firstVertical ? std::max(c.z, d.z)
                                          : std::max(c.x, d.x);
    return std::max(a0, c0) <= std::min(a1, c1);
  }
  const Point verticalStart = firstVertical ? a : c;
  const Point verticalEnd = firstVertical ? b : d;
  const Point horizontalStart = firstVertical ? c : a;
  const Point horizontalEnd = firstVertical ? d : b;
  return verticalStart.x >= std::min(horizontalStart.x, horizontalEnd.x) &&
         verticalStart.x <= std::max(horizontalStart.x, horizontalEnd.x) &&
         horizontalStart.z >= std::min(verticalStart.z, verticalEnd.z) &&
         horizontalStart.z <= std::max(verticalStart.z, verticalEnd.z);
}

[[nodiscard]] bool decomposeRoom(
    const std::vector<Point>& polygon,
    std::vector<Rect>& output) {
  const std::size_t initialSize = output.size();
  std::vector<std::int32_t> xCoordinates;
  xCoordinates.reserve(polygon.size());
  for (Point point : polygon) {
    xCoordinates.push_back(point.x);
  }
  std::sort(xCoordinates.begin(), xCoordinates.end());
  xCoordinates.erase(
      std::unique(xCoordinates.begin(), xCoordinates.end()),
      xCoordinates.end());
  if (xCoordinates.size() < 2U) {
    return false;
  }

  std::map<std::pair<std::int32_t, std::int32_t>, std::size_t> active;
  for (std::size_t xIndex = 1U; xIndex < xCoordinates.size(); ++xIndex) {
    const std::int32_t minimumX = xCoordinates[xIndex - 1U];
    const std::int32_t maximumX = xCoordinates[xIndex];
    if (minimumX >= maximumX) {
      return false;
    }
    const long double sampleX =
        (static_cast<long double>(minimumX) + maximumX) * 0.5L;
    std::vector<std::int32_t> intersections;
    for (std::size_t index = 0U; index < polygon.size(); ++index) {
      const Point start = polygon[index];
      const Point end = polygon[(index + 1U) % polygon.size()];
      if (start.z == end.z &&
          sampleX > std::min(start.x, end.x) &&
          sampleX < std::max(start.x, end.x)) {
        intersections.push_back(start.z);
      }
    }
    std::sort(intersections.begin(), intersections.end());
    if (intersections.size() % 2U != 0U) {
      return false;
    }

    std::map<std::pair<std::int32_t, std::int32_t>, std::size_t> nextActive;
    for (std::size_t index = 0U; index < intersections.size(); index += 2U) {
      const std::int32_t minimumZ = intersections[index];
      const std::int32_t maximumZ = intersections[index + 1U];
      if (minimumZ >= maximumZ) {
        return false;
      }
      const auto interval = std::make_pair(minimumZ, maximumZ);
      const auto existing = active.find(interval);
      if (existing != active.end() &&
          output[existing->second].maximum.x == minimumX) {
        output[existing->second].maximum.x = maximumX;
        nextActive.emplace(interval, existing->second);
      } else {
        output.push_back({{minimumX, minimumZ}, {maximumX, maximumZ}});
        nextActive.emplace(interval, output.size() - 1U);
      }
    }
    active = std::move(nextActive);
  }
  return output.size() > initialSize;
}

[[nodiscard]] std::array<DirectedSegment, 4U> rectangleSegments(
    std::size_t roomIndex,
    std::size_t levelIndex,
    Rect rect) noexcept {
  const Point northWest = rect.minimum;
  const Point northEast{rect.maximum.x, rect.minimum.z};
  const Point southEast = rect.maximum;
  const Point southWest{rect.minimum.x, rect.maximum.z};
  return {{{roomIndex, levelIndex, northWest, northEast},
           {roomIndex, levelIndex, northEast, southEast},
           {roomIndex, levelIndex, southEast, southWest},
           {roomIndex, levelIndex, southWest, northWest}}};
}

[[nodiscard]] std::size_t appendVertex(
    const CreativeWorldLayout& layout,
    std::map<VertexKey, std::size_t>& indices,
    CreativeWorldLayoutRoomGraph& graph,
    std::size_t levelIndex,
    Point point) {
  const VertexKey key{levelIndex, point};
  const auto existing = indices.find(key);
  if (existing != indices.end()) {
    return existing->second;
  }
  const std::size_t index = graph.vertices.size();
  graph.vertices.push_back(
      {levelIndex, vertexStableKey(layout, levelIndex, point), point});
  indices.emplace(key, index);
  return index;
}

[[nodiscard]] std::size_t appendEdge(
    const CreativeWorldLayout& layout,
    std::map<VertexKey, std::size_t>& vertexIndices,
    std::map<EdgeKey, std::size_t>& edgeIndices,
    CreativeWorldLayoutRoomGraph& graph,
    std::size_t levelIndex,
    Point start,
    Point end,
    double thickness) {
  Point canonicalStart = start;
  Point canonicalEnd = end;
  if (pointLess(canonicalEnd, canonicalStart)) {
    std::swap(canonicalStart, canonicalEnd);
  }
  const EdgeKey key{levelIndex, canonicalStart, canonicalEnd};
  const auto existing = edgeIndices.find(key);
  if (existing != edgeIndices.end()) {
    graph.edges[existing->second].wallThicknessCells =
        std::max(graph.edges[existing->second].wallThicknessCells, thickness);
    return existing->second;
  }
  const std::size_t startVertex = appendVertex(
      layout, vertexIndices, graph, levelIndex, canonicalStart);
  const std::size_t endVertex =
      appendVertex(layout, vertexIndices, graph, levelIndex, canonicalEnd);
  const std::size_t index = graph.edges.size();
  graph.edges.push_back({levelIndex,
                         edgeStableKey(layout, levelIndex, canonicalStart,
                                       canonicalEnd),
                         startVertex, endVertex, thickness});
  edgeIndices.emplace(key, index);
  return index;
}

[[nodiscard]] bool buildLegacyGraph(const CreativeWorldLayout& layout,
                                    CreativeWorldLayoutRoomGraph& graph) {
  std::vector<DirectedSegment> rawSegments;
  rawSegments.reserve(layout.rooms.size() * 4U);
  for (std::size_t roomIndex = 0U; roomIndex < layout.rooms.size();
       ++roomIndex) {
    const CreativeWorldLayoutRoom& room = layout.rooms[roomIndex];
    if (room.buildingIndex >= layout.buildings.size() ||
        room.levelIndex >= layout.levels.size() ||
        layout.levels[room.levelIndex].buildingIndex != room.buildingIndex ||
        !validRect(room.footprint) ||
        !std::isfinite(room.wallThicknessCells) ||
        room.wallThicknessCells <= 0.0 ||
        rectWidth(room.footprint) <= room.wallThicknessCells * 2.0 ||
        rectDepth(room.footprint) <= room.wallThicknessCells * 2.0) {
      graph.failedRoomIndex = roomIndex;
      reject(graph, CreativeWorldLayoutRoomGraphStatus::InvalidRoom,
             "creative_world_layout_room_graph_room_invalid");
      return false;
    }
    const auto segments =
        rectangleSegments(roomIndex, room.levelIndex, room.footprint);
    rawSegments.insert(rawSegments.end(), segments.begin(), segments.end());
  }

  std::map<VertexKey, std::size_t> vertexIndices;
  std::map<EdgeKey, std::size_t> edgeIndices;
  graph.rooms.resize(layout.rooms.size());
  graph.roomBounds.resize(layout.rooms.size());
  for (std::size_t roomIndex = 0U; roomIndex < layout.rooms.size();
       ++roomIndex) {
    graph.rooms[roomIndex].firstBoundary = graph.boundaries.size();
    graph.rooms[roomIndex].firstSurfaceRect = graph.surfaceRects.size();
    graph.roomBounds[roomIndex] = layout.rooms[roomIndex].footprint;
    graph.surfaceRects.push_back(layout.rooms[roomIndex].footprint);
    graph.rooms[roomIndex].surfaceRectCount = 1U;

    std::size_t order = 0U;
    for (const DirectedSegment& raw : rawSegments) {
      if (raw.roomIndex != roomIndex) {
        continue;
      }
      std::vector<Point> splitPoints{raw.start, raw.end};
      for (const DirectedSegment& candidate : rawSegments) {
        if (candidate.levelIndex != raw.levelIndex) {
          continue;
        }
        if (pointOnSegmentInterior(candidate.start, raw.start, raw.end)) {
          splitPoints.push_back(candidate.start);
        }
        if (pointOnSegmentInterior(candidate.end, raw.start, raw.end)) {
          splitPoints.push_back(candidate.end);
        }
      }
      std::sort(splitPoints.begin(), splitPoints.end(), pointLess);
      splitPoints.erase(std::unique(splitPoints.begin(), splitPoints.end()),
                        splitPoints.end());
      if (pointLess(raw.end, raw.start)) {
        std::reverse(splitPoints.begin(), splitPoints.end());
      }
      for (std::size_t pointIndex = 1U; pointIndex < splitPoints.size();
           ++pointIndex) {
        const Point start = splitPoints[pointIndex - 1U];
        const Point end = splitPoints[pointIndex];
        const std::size_t edgeIndex = appendEdge(
            layout, vertexIndices, edgeIndices, graph, raw.levelIndex, start,
            end, layout.rooms[roomIndex].wallThicknessCells);
        const CreativeWorldLayoutTopologyEdge& edge = graph.edges[edgeIndex];
        const bool reversed =
            graph.vertices[edge.startVertexIndex].position != start;
        graph.boundaries.push_back({roomIndex, edgeIndex, order++, reversed});
      }
    }
    graph.rooms[roomIndex].boundaryCount =
        graph.boundaries.size() - graph.rooms[roomIndex].firstBoundary;
    if (graph.rooms[roomIndex].boundaryCount >
        kCreativeWorldLayoutMaximumRoomBoundaryEdgeCount) {
      graph.failedRoomIndex = roomIndex;
      reject(graph,
             CreativeWorldLayoutRoomGraphStatus::BoundaryCapacityExceeded,
             "creative_world_layout_room_graph_boundary_capacity_exceeded");
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool validateExplicitGraph(const CreativeWorldLayout& layout,
                                         CreativeWorldLayoutRoomGraph& graph) {
  graph.vertices = layout.topologyVertices;
  graph.edges = layout.topologyEdges;
  graph.boundaries = layout.roomBoundaries;
  graph.rooms.resize(layout.rooms.size());
  graph.roomBounds.resize(layout.rooms.size());

  std::set<std::string> vertexKeys;
  for (std::size_t index = 0U; index < graph.vertices.size(); ++index) {
    const CreativeWorldLayoutTopologyVertex& vertex = graph.vertices[index];
    if (vertex.levelIndex >= layout.levels.size() || vertex.stableKey.empty() ||
        !vertexKeys.insert(vertex.stableKey).second) {
      graph.failedVertexIndex = index;
      reject(graph, CreativeWorldLayoutRoomGraphStatus::InvalidVertex,
             "creative_world_layout_room_graph_vertex_invalid");
      return false;
    }
  }

  std::set<std::string> edgeKeys;
  for (std::size_t index = 0U; index < graph.edges.size(); ++index) {
    const CreativeWorldLayoutTopologyEdge& edge = graph.edges[index];
    if (edge.levelIndex >= layout.levels.size() || edge.stableKey.empty() ||
        !edgeKeys.insert(edge.stableKey).second ||
        edge.startVertexIndex >= graph.vertices.size() ||
        edge.endVertexIndex >= graph.vertices.size() ||
        graph.vertices[edge.startVertexIndex].levelIndex != edge.levelIndex ||
        graph.vertices[edge.endVertexIndex].levelIndex != edge.levelIndex ||
        !std::isfinite(edge.wallThicknessCells) ||
        edge.wallThicknessCells <= 0.0 ||
        edge.profile >= CreativeWorldLayoutWallProfile::Count ||
        edge.material >= CreativeStructuralMaterial::Count ||
        edge.joinStyle >= CreativeWorldLayoutWallJoinStyle::Count) {
      graph.failedEdgeIndex = index;
      reject(graph, CreativeWorldLayoutRoomGraphStatus::InvalidEdge,
             "creative_world_layout_room_graph_edge_invalid");
      return false;
    }
    const Point start = graph.vertices[edge.startVertexIndex].position;
    const Point end = graph.vertices[edge.endVertexIndex].position;
    if (!orthogonal(start, end)) {
      graph.failedEdgeIndex = index;
      reject(graph, CreativeWorldLayoutRoomGraphStatus::NonOrthogonal,
             "creative_world_layout_room_graph_edge_non_orthogonal");
      return false;
    }
    if (!pointLess(start, end)) {
      graph.failedEdgeIndex = index;
      reject(graph, CreativeWorldLayoutRoomGraphStatus::InvalidEdge,
             "creative_world_layout_room_graph_edge_not_canonical");
      return false;
    }
    for (std::size_t vertexIndex = 0U; vertexIndex < graph.vertices.size();
         ++vertexIndex) {
      if (vertexIndex != edge.startVertexIndex &&
          vertexIndex != edge.endVertexIndex &&
          graph.vertices[vertexIndex].levelIndex == edge.levelIndex &&
          pointOnSegmentInterior(graph.vertices[vertexIndex].position, start,
                                 end)) {
        graph.failedEdgeIndex = index;
        graph.failedVertexIndex = vertexIndex;
        reject(graph, CreativeWorldLayoutRoomGraphStatus::InvalidEdge,
               "creative_world_layout_room_graph_edge_requires_split");
        return false;
      }
    }
  }

  std::sort(graph.boundaries.begin(), graph.boundaries.end(),
            [](const CreativeWorldLayoutRoomBoundary& lhs,
               const CreativeWorldLayoutRoomBoundary& rhs) {
              return std::tie(lhs.roomIndex, lhs.order) <
                     std::tie(rhs.roomIndex, rhs.order);
            });
  std::vector<std::size_t> edgeUseCounts(graph.edges.size(), 0U);
  std::vector<bool> edgeFirstDirection(graph.edges.size(), false);
  std::vector<std::size_t> edgeFirstRoom(
      graph.edges.size(), kInvalidCreativeWorldLayoutIndex);
  std::size_t boundaryCursor = 0U;
  for (std::size_t roomIndex = 0U; roomIndex < layout.rooms.size();
       ++roomIndex) {
    const CreativeWorldLayoutRoom& room = layout.rooms[roomIndex];
    if (room.buildingIndex >= layout.buildings.size() ||
        room.levelIndex >= layout.levels.size() ||
        layout.levels[room.levelIndex].buildingIndex != room.buildingIndex ||
        !std::isfinite(room.wallThicknessCells) ||
        room.wallThicknessCells <= 0.0) {
      graph.failedRoomIndex = roomIndex;
      reject(graph, CreativeWorldLayoutRoomGraphStatus::InvalidRoom,
             "creative_world_layout_room_graph_room_invalid");
      return false;
    }

    CreativeWorldLayoutRoomGraphRange& range = graph.rooms[roomIndex];
    range.firstBoundary = boundaryCursor;
    while (boundaryCursor < graph.boundaries.size() &&
           graph.boundaries[boundaryCursor].roomIndex == roomIndex) {
      CreativeWorldLayoutRoomBoundary& boundary =
          graph.boundaries[boundaryCursor];
      if (boundary.order != range.boundaryCount ||
          boundary.topologyEdgeIndex >= graph.edges.size() ||
          graph.edges[boundary.topologyEdgeIndex].levelIndex != room.levelIndex) {
        graph.failedRoomIndex = roomIndex;
        graph.failedBoundaryIndex = boundaryCursor;
        reject(graph, CreativeWorldLayoutRoomGraphStatus::InvalidBoundary,
               "creative_world_layout_room_graph_boundary_invalid");
        return false;
      }
      const std::size_t edgeIndex = boundary.topologyEdgeIndex;
      if (edgeUseCounts[edgeIndex] == 0U) {
        edgeFirstDirection[edgeIndex] = boundary.reversed;
        edgeFirstRoom[edgeIndex] = roomIndex;
      } else if (edgeUseCounts[edgeIndex] >= 2U ||
                 edgeFirstRoom[edgeIndex] == roomIndex ||
                 edgeFirstDirection[edgeIndex] == boundary.reversed) {
        graph.failedRoomIndex = roomIndex;
        graph.failedBoundaryIndex = boundaryCursor;
        reject(graph, CreativeWorldLayoutRoomGraphStatus::InvalidBoundary,
               "creative_world_layout_room_graph_edge_ownership_invalid");
        return false;
      }
      ++edgeUseCounts[edgeIndex];
      ++range.boundaryCount;
      ++boundaryCursor;
    }
    if (range.boundaryCount < 4U ||
        range.boundaryCount >
            kCreativeWorldLayoutMaximumRoomBoundaryEdgeCount) {
      graph.failedRoomIndex = roomIndex;
      reject(graph,
             range.boundaryCount >
                     kCreativeWorldLayoutMaximumRoomBoundaryEdgeCount
                 ? CreativeWorldLayoutRoomGraphStatus::BoundaryCapacityExceeded
                 : CreativeWorldLayoutRoomGraphStatus::InvalidBoundary,
             "creative_world_layout_room_graph_boundary_count_invalid");
      return false;
    }

    std::vector<Point> polygon;
    polygon.reserve(range.boundaryCount);
    for (std::size_t local = 0U; local < range.boundaryCount; ++local) {
      const CreativeWorldLayoutRoomBoundary& boundary =
          graph.boundaries[range.firstBoundary + local];
      const Point start = boundaryStart(graph, boundary);
      const Point end = boundaryEnd(graph, boundary);
      const CreativeWorldLayoutRoomBoundary& next = graph.boundaries[
          range.firstBoundary + (local + 1U) % range.boundaryCount];
      if (end != boundaryStart(graph, next)) {
        graph.failedRoomIndex = roomIndex;
        graph.failedBoundaryIndex = range.firstBoundary + local;
        reject(graph, CreativeWorldLayoutRoomGraphStatus::OpenBoundary,
               "creative_world_layout_room_graph_boundary_open");
        return false;
      }
      polygon.push_back(start);
    }

    long double twiceArea = 0.0L;
    Rect bounds{polygon.front(), polygon.front()};
    for (std::size_t local = 0U; local < polygon.size(); ++local) {
      const Point start = polygon[local];
      const Point end = polygon[(local + 1U) % polygon.size()];
      twiceArea += static_cast<long double>(start.x) * end.z -
                   static_cast<long double>(end.x) * start.z;
      bounds.minimum.x = std::min(bounds.minimum.x, start.x);
      bounds.minimum.z = std::min(bounds.minimum.z, start.z);
      bounds.maximum.x = std::max(bounds.maximum.x, start.x);
      bounds.maximum.z = std::max(bounds.maximum.z, start.z);
      for (std::size_t other = local + 1U; other < polygon.size(); ++other) {
        const bool adjacent = other == local + 1U ||
                              (local == 0U && other + 1U == polygon.size());
        if (adjacent) {
          continue;
        }
        const Point otherStart = polygon[other];
        const Point otherEnd = polygon[(other + 1U) % polygon.size()];
        if (segmentsIntersect(start, end, otherStart, otherEnd)) {
          graph.failedRoomIndex = roomIndex;
          graph.failedBoundaryIndex = range.firstBoundary + local;
          reject(graph, CreativeWorldLayoutRoomGraphStatus::SelfIntersection,
                 "creative_world_layout_room_graph_boundary_self_intersection");
          return false;
        }
      }
    }
    if (twiceArea <= 0.0L || !validRect(bounds) ||
        !sameRect(bounds, room.footprint)) {
      graph.failedRoomIndex = roomIndex;
      reject(graph, CreativeWorldLayoutRoomGraphStatus::InvalidRoom,
             "creative_world_layout_room_graph_bounds_invalid");
      return false;
    }

    graph.roomBounds[roomIndex] = bounds;
    range.firstSurfaceRect = graph.surfaceRects.size();
    if (!decomposeRoom(polygon, graph.surfaceRects)) {
      graph.failedRoomIndex = roomIndex;
      reject(graph, CreativeWorldLayoutRoomGraphStatus::DisconnectedRoom,
             "creative_world_layout_room_graph_decomposition_failed");
      return false;
    }
    range.surfaceRectCount =
        graph.surfaceRects.size() - range.firstSurfaceRect;
    double maximumThickness = room.wallThicknessCells;
    for (std::size_t local = 0U; local < range.boundaryCount; ++local) {
      maximumThickness = std::max(
          maximumThickness,
          graph.edges[graph.boundaries[range.firstBoundary + local]
                          .topologyEdgeIndex]
              .wallThicknessCells);
    }
    const std::span<const Rect> roomPieces =
        std::span<const Rect>{graph.surfaceRects}.subspan(
            range.firstSurfaceRect, range.surfaceRectCount);
    if (std::any_of(roomPieces.begin(), roomPieces.end(), [&](Rect piece) {
          return rectWidth(piece) <= maximumThickness * 2.0 ||
                 rectDepth(piece) <= maximumThickness * 2.0;
        })) {
      graph.failedRoomIndex = roomIndex;
      reject(graph, CreativeWorldLayoutRoomGraphStatus::InvalidRoom,
             "creative_world_layout_room_graph_wall_thickness_invalid");
      return false;
    }
  }
  if (boundaryCursor != graph.boundaries.size() ||
      std::any_of(edgeUseCounts.begin(), edgeUseCounts.end(),
                  [](std::size_t count) { return count == 0U; })) {
    reject(graph, CreativeWorldLayoutRoomGraphStatus::InvalidBoundary,
           "creative_world_layout_room_graph_orphan_boundary_data");
    return false;
  }
  return true;
}

[[nodiscard]] bool validateRoomOverlap(const CreativeWorldLayout& layout,
                                       CreativeWorldLayoutRoomGraph& graph) {
  for (std::size_t roomIndex = 0U; roomIndex < layout.rooms.size();
       ++roomIndex) {
    const std::span<const Rect> roomRects =
        creativeWorldLayoutRoomSurfaceRects(graph, roomIndex);
    for (std::size_t prior = 0U; prior < roomIndex; ++prior) {
      if (layout.rooms[prior].levelIndex != layout.rooms[roomIndex].levelIndex) {
        continue;
      }
      const std::span<const Rect> priorRects =
          creativeWorldLayoutRoomSurfaceRects(graph, prior);
      for (Rect roomRect : roomRects) {
        if (std::any_of(priorRects.begin(), priorRects.end(),
                        [&](Rect priorRect) {
                          return rectsOverlap(roomRect, priorRect);
                        })) {
          graph.failedRoomIndex = roomIndex;
          reject(graph, CreativeWorldLayoutRoomGraphStatus::OverlappingRooms,
                 "creative_world_layout_room_graph_rooms_overlap");
          return false;
        }
      }
    }
  }
  return true;
}

[[nodiscard]] bool edgeBelongsToRoom(
    const CreativeWorldLayoutRoomGraph& graph,
    std::size_t roomIndex,
    std::size_t edgeIndex) noexcept {
  const std::span<const CreativeWorldLayoutRoomBoundary> boundaries =
      creativeWorldLayoutRoomBoundaries(graph, roomIndex);
  return std::any_of(
      boundaries.begin(), boundaries.end(),
      [&](const CreativeWorldLayoutRoomBoundary& boundary) {
        return boundary.topologyEdgeIndex == edgeIndex;
      });
}

[[nodiscard]] bool validateOpeningHosts(
    const CreativeWorldLayout& layout,
    CreativeWorldLayoutRoomGraph& graph) {
  for (std::size_t index = 0U; index < layout.openings.size(); ++index) {
    const CreativeWorldLayoutOpening& opening = layout.openings[index];
    if (opening.hostKind != CreativeWorldLayoutOpeningHostKind::RoomEdge ||
        opening.roomTopologyEdgeIndex == kInvalidCreativeWorldLayoutIndex) {
      continue;
    }
    if (opening.roomIndex >= layout.rooms.size() ||
        opening.roomTopologyEdgeIndex >= graph.edges.size() ||
        !edgeBelongsToRoom(graph, opening.roomIndex,
                           opening.roomTopologyEdgeIndex)) {
      graph.failedOpeningIndex = index;
      reject(graph, CreativeWorldLayoutRoomGraphStatus::OpeningHostInvalid,
             "creative_world_layout_room_graph_opening_host_invalid");
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool sideMatches(CreativeWorldLayoutRoomEdge side,
                               Rect bounds,
                               Point start,
                               Point end) noexcept {
  switch (side) {
    case CreativeWorldLayoutRoomEdge::North:
      return start.z == bounds.minimum.z && end.z == bounds.minimum.z;
    case CreativeWorldLayoutRoomEdge::East:
      return start.x == bounds.maximum.x && end.x == bounds.maximum.x;
    case CreativeWorldLayoutRoomEdge::South:
      return start.z == bounds.maximum.z && end.z == bounds.maximum.z;
    case CreativeWorldLayoutRoomEdge::West:
      return start.x == bounds.minimum.x && end.x == bounds.minimum.x;
    case CreativeWorldLayoutRoomEdge::Count:
      return false;
  }
  return false;
}

[[nodiscard]] bool materializeOpeningHost(
    const CreativeWorldLayoutRoomGraph& graph,
    CreativeWorldLayoutOpening& opening) noexcept {
  if (opening.roomIndex >= graph.rooms.size() ||
      opening.roomEdge >= CreativeWorldLayoutRoomEdge::Count ||
      !std::isfinite(opening.centerOffsetCells) ||
      !std::isfinite(opening.widthCells) || opening.widthCells <= 0.0) {
    return false;
  }
  if (opening.roomTopologyEdgeIndex != kInvalidCreativeWorldLayoutIndex) {
    return opening.roomTopologyEdgeIndex < graph.edges.size() &&
           edgeBelongsToRoom(graph, opening.roomIndex,
                             opening.roomTopologyEdgeIndex);
  }

  const Rect bounds = graph.roomBounds[opening.roomIndex];
  const bool horizontal =
      opening.roomEdge == CreativeWorldLayoutRoomEdge::North ||
      opening.roomEdge == CreativeWorldLayoutRoomEdge::South;
  const double roomOrigin = horizontal ? static_cast<double>(bounds.minimum.x)
                                       : static_cast<double>(bounds.minimum.z);
  const double center = roomOrigin + opening.centerOffsetCells;
  const double minimum = center - opening.widthCells * 0.5;
  const double maximum = center + opening.widthCells * 0.5;
  std::size_t match = kInvalidCreativeWorldLayoutIndex;
  double matchOrigin = 0.0;
  for (const CreativeWorldLayoutRoomBoundary& boundary :
       creativeWorldLayoutRoomBoundaries(graph, opening.roomIndex)) {
    const CreativeWorldLayoutTopologyEdge& edge =
        graph.edges[boundary.topologyEdgeIndex];
    const Point start = graph.vertices[edge.startVertexIndex].position;
    const Point end = graph.vertices[edge.endVertexIndex].position;
    if (!sideMatches(opening.roomEdge, bounds, start, end)) {
      continue;
    }
    const double edgeMinimum =
        horizontal ? static_cast<double>(start.x) : static_cast<double>(start.z);
    const double edgeMaximum =
        horizontal ? static_cast<double>(end.x) : static_cast<double>(end.z);
    if (minimum >= edgeMinimum && maximum <= edgeMaximum) {
      if (match != kInvalidCreativeWorldLayoutIndex) {
        return false;
      }
      match = boundary.topologyEdgeIndex;
      matchOrigin = edgeMinimum;
    }
  }
  if (match == kInvalidCreativeWorldLayoutIndex) {
    return false;
  }
  opening.roomTopologyEdgeIndex = match;
  opening.centerOffsetCells = center - matchOrigin;
  return true;
}

}  // namespace

CreativeWorldLayoutRoomGraph buildCreativeWorldLayoutRoomGraph(
    const CreativeWorldLayout& layout) {
  CreativeWorldLayoutRoomGraph graph;
  graph.requested = true;
  if (layout.schemaVersion != kCreativeWorldLayoutSchemaVersion ||
      layout.rooms.empty()) {
    reject(graph, CreativeWorldLayoutRoomGraphStatus::InvalidLayout,
           "creative_world_layout_room_graph_layout_invalid");
    return graph;
  }

  const bool anyExplicit = !layout.topologyVertices.empty() ||
                           !layout.topologyEdges.empty() ||
                           !layout.roomBoundaries.empty();
  const bool allExplicit = !layout.topologyVertices.empty() &&
                           !layout.topologyEdges.empty() &&
                           !layout.roomBoundaries.empty();
  if (anyExplicit != allExplicit) {
    reject(graph, CreativeWorldLayoutRoomGraphStatus::InvalidLayout,
           "creative_world_layout_room_graph_tables_incomplete");
    return graph;
  }
  graph.sourceWasExplicit = allExplicit;
  if (!(allExplicit ? validateExplicitGraph(layout, graph)
                    : buildLegacyGraph(layout, graph)) ||
      !validateRoomOverlap(layout, graph) ||
      !validateOpeningHosts(layout, graph)) {
    return graph;
  }

  graph.accepted = true;
  graph.status = CreativeWorldLayoutRoomGraphStatus::Ready;
  graph.reasonCode = "creative_world_layout_room_graph_ready";
  return graph;
}

std::span<const CreativeWorldLayoutRoomBoundary>
creativeWorldLayoutRoomBoundaries(
    const CreativeWorldLayoutRoomGraph& graph,
    std::size_t roomIndex) noexcept {
  if (roomIndex >= graph.rooms.size()) {
    return {};
  }
  const CreativeWorldLayoutRoomGraphRange& range = graph.rooms[roomIndex];
  if (range.firstBoundary > graph.boundaries.size() ||
      range.boundaryCount > graph.boundaries.size() - range.firstBoundary) {
    return {};
  }
  return std::span<const CreativeWorldLayoutRoomBoundary>{graph.boundaries}
      .subspan(range.firstBoundary, range.boundaryCount);
}

std::span<const CreativeWorldLayoutRect> creativeWorldLayoutRoomSurfaceRects(
    const CreativeWorldLayoutRoomGraph& graph,
    std::size_t roomIndex) noexcept {
  if (roomIndex >= graph.rooms.size()) {
    return {};
  }
  const CreativeWorldLayoutRoomGraphRange& range = graph.rooms[roomIndex];
  if (range.firstSurfaceRect > graph.surfaceRects.size() ||
      range.surfaceRectCount >
          graph.surfaceRects.size() - range.firstSurfaceRect) {
    return {};
  }
  return std::span<const CreativeWorldLayoutRect>{graph.surfaceRects}
      .subspan(range.firstSurfaceRect, range.surfaceRectCount);
}

bool creativeWorldLayoutRoomContainsPoint(
    const CreativeWorldLayoutRoomGraph& graph,
    std::size_t roomIndex,
    CreativeTerrainCoord2 point) noexcept {
  const std::span<const Rect> rectangles =
      creativeWorldLayoutRoomSurfaceRects(graph, roomIndex);
  return std::any_of(rectangles.begin(), rectangles.end(), [&](Rect rect) {
    return point.x >= rect.minimum.x && point.x <= rect.maximum.x &&
           point.z >= rect.minimum.z && point.z <= rect.maximum.z;
  });
}

bool creativeWorldLayoutRoomContainsRect(
    const CreativeWorldLayoutRoomGraph& graph,
    std::size_t roomIndex,
    CreativeWorldLayoutRect rect) noexcept {
  if (!validRect(rect)) {
    return false;
  }
  const std::span<const Rect> rectangles =
      creativeWorldLayoutRoomSurfaceRects(graph, roomIndex);
  std::uint64_t coveredArea = 0U;
  for (Rect piece : rectangles) {
    const Rect intersection{
        {std::max(rect.minimum.x, piece.minimum.x),
         std::max(rect.minimum.z, piece.minimum.z)},
        {std::min(rect.maximum.x, piece.maximum.x),
         std::min(rect.maximum.z, piece.maximum.z)}};
    coveredArea += rectArea(intersection);
  }
  return coveredArea == rectArea(rect);
}

CreativeWorldLayoutRoomGraphMaterializeResult
materializeCreativeWorldLayoutRoomGraph(const CreativeWorldLayout& source) {
  CreativeWorldLayoutRoomGraphMaterializeResult result;
  result.requested = true;
  const CreativeWorldLayoutRoomGraph graph =
      buildCreativeWorldLayoutRoomGraph(source);
  result.status = graph.status;
  if (!graph.accepted) {
    result.reasonCode = graph.reasonCode;
    result.failedOpeningIndex = graph.failedOpeningIndex;
    return result;
  }

  result.edited = source;
  result.edited.topologyVertices = graph.vertices;
  result.edited.topologyEdges = graph.edges;
  result.edited.roomBoundaries = graph.boundaries;
  if (!graph.sourceWasExplicit) {
    std::vector<std::size_t> edgeUseCount(graph.edges.size(), 0U);
    for (const CreativeWorldLayoutRoomBoundary& boundary : graph.boundaries) {
      ++edgeUseCount[boundary.topologyEdgeIndex];
    }
    for (std::size_t edgeIndex = 0U;
         edgeIndex < result.edited.topologyEdges.size(); ++edgeIndex) {
      CreativeWorldLayoutTopologyEdge& edge =
          result.edited.topologyEdges[edgeIndex];
      if (edge.levelIndex >= result.edited.levels.size() ||
          (edgeUseCount[edgeIndex] != 1U && edgeUseCount[edgeIndex] != 2U)) {
        continue;
      }
      const CreativeWorldLayoutWallProfile profile =
          edgeUseCount[edgeIndex] == 1U
              ? CreativeWorldLayoutWallProfile::Exterior
              : CreativeWorldLayoutWallProfile::Interior;
      CreativeStructuralMaterial material = CreativeStructuralMaterial::Count;
      const std::size_t buildingIndex =
          result.edited.levels[edge.levelIndex].buildingIndex;
      if (creativeWorldLayoutBuildingBlockoutWallMaterial(
              result.edited, buildingIndex, profile, material)) {
        edge.profile = profile;
        edge.material = material;
      }
    }
  }
  for (std::size_t roomIndex = 0U; roomIndex < result.edited.rooms.size();
       ++roomIndex) {
    result.edited.rooms[roomIndex].footprint = graph.roomBounds[roomIndex];
  }
  for (std::size_t index = 0U; index < result.edited.openings.size(); ++index) {
    CreativeWorldLayoutOpening& opening = result.edited.openings[index];
    if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge &&
        !materializeOpeningHost(graph, opening)) {
      result.status = CreativeWorldLayoutRoomGraphStatus::OpeningHostInvalid;
      result.failedOpeningIndex = index;
      result.edited = {};
      result.reasonCode =
          "creative_world_layout_room_graph_opening_materialization_failed";
      return result;
    }
  }

  result.accepted = true;
  result.changed = !graph.sourceWasExplicit;
  result.status = CreativeWorldLayoutRoomGraphStatus::Ready;
  result.reasonCode = result.changed
                          ? "creative_world_layout_room_graph_materialized"
                          : "creative_world_layout_room_graph_already_explicit";
  return result;
}

}  // namespace iggy3d::creative
