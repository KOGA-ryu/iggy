#include "app/iggy3d/creative/world/WorldLayoutRoomOperations.hpp"

#include "app/iggy3d/creative/world/WorldLayoutBlockout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoomTopology.hpp"
#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <limits>
#include <set>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

using Point = CreativeTerrainCoord2;
using Rect = CreativeWorldLayoutRect;

struct PointOrder {
  [[nodiscard]] bool operator()(Point lhs, Point rhs) const noexcept {
    return std::tie(lhs.x, lhs.z) < std::tie(rhs.x, rhs.z);
  }
};

struct VertexKey {
  std::size_t levelIndex = 0U;
  Point point{};

  [[nodiscard]] friend bool operator<(const VertexKey& lhs,
                                      const VertexKey& rhs) noexcept {
    return std::tie(lhs.levelIndex, lhs.point.x, lhs.point.z) <
           std::tie(rhs.levelIndex, rhs.point.x, rhs.point.z);
  }
};

struct SegmentKey {
  Point start{};
  Point end{};

  [[nodiscard]] friend bool operator<(const SegmentKey& lhs,
                                      const SegmentKey& rhs) noexcept {
    return std::tie(lhs.start.x, lhs.start.z, lhs.end.x, lhs.end.z) <
           std::tie(rhs.start.x, rhs.start.z, rhs.end.x, rhs.end.z);
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
  Point start{};
  Point end{};
};

struct OpeningAnchor {
  bool topologyHosted = false;
  bool horizontal = false;
  std::size_t sourceRoomIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t levelIndex = kInvalidCreativeWorldLayoutIndex;
  std::int32_t line = 0;
  double center = 0.0;
  double minimum = 0.0;
  double maximum = 0.0;
};

struct BoundaryRelocation {
  std::size_t sourceEdgeIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t levelIndex = kInvalidCreativeWorldLayoutIndex;
  Point sourceStart{};
  Point sourceEnd{};
  Point editedStart{};
  Point editedEnd{};
};

[[nodiscard]] bool pointLess(Point lhs, Point rhs) noexcept {
  return PointOrder{}(lhs, rhs);
}

[[nodiscard]] bool validRect(Rect rect) noexcept {
  return rect.minimum.x < rect.maximum.x &&
         rect.minimum.z < rect.maximum.z;
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

[[nodiscard]] bool pointOnSegment(Point point,
                                  Point start,
                                  Point end) noexcept {
  return point == start || point == end ||
         pointOnSegmentInterior(point, start, end);
}

[[nodiscard]] SegmentKey canonicalSegment(Point start, Point end) noexcept {
  if (pointLess(end, start)) {
    std::swap(start, end);
  }
  return {start, end};
}

[[nodiscard]] Point relocatedPoint(const BoundaryRelocation& relocation,
                                   std::size_t levelIndex,
                                   Point point) noexcept {
  if (levelIndex != relocation.levelIndex) {
    return point;
  }
  if (point == relocation.sourceStart) {
    return relocation.editedStart;
  }
  if (point == relocation.sourceEnd) {
    return relocation.editedEnd;
  }
  return point;
}

void appendRectBoundary(std::vector<DirectedSegment>& output, Rect rect) {
  const Point northWest = rect.minimum;
  const Point northEast{rect.maximum.x, rect.minimum.z};
  const Point southEast = rect.maximum;
  const Point southWest{rect.minimum.x, rect.maximum.z};
  output.push_back({northWest, northEast});
  output.push_back({northEast, southEast});
  output.push_back({southEast, southWest});
  output.push_back({southWest, northWest});
}

[[nodiscard]] std::vector<Point> splitPoints(
    Point start, Point end, std::span<const Point> candidates) {
  std::vector<Point> points{start, end};
  for (Point candidate : candidates) {
    if (pointOnSegmentInterior(candidate, start, end)) {
      points.push_back(candidate);
    }
  }
  std::sort(points.begin(), points.end(), PointOrder{});
  points.erase(std::unique(points.begin(), points.end()), points.end());
  if (pointLess(end, start)) {
    std::reverse(points.begin(), points.end());
  }
  return points;
}

// Extracts one simple CCW orthogonal boundary from a non-overlapping rectangle
// union. Multiple loops deliberately reject: one room cannot silently become
// disconnected or acquire a hole.
[[nodiscard]] bool rectanglesToPolygon(std::span<const Rect> rectangles,
                                       std::vector<Point>& polygon) {
  polygon.clear();
  std::vector<DirectedSegment> raw;
  raw.reserve(rectangles.size() * 4U);
  for (Rect rect : rectangles) {
    if (!validRect(rect)) {
      return false;
    }
    appendRectBoundary(raw, rect);
  }
  if (raw.empty()) {
    return false;
  }

  std::vector<Point> candidates;
  candidates.reserve(raw.size() * 2U);
  for (const DirectedSegment& segment : raw) {
    candidates.push_back(segment.start);
    candidates.push_back(segment.end);
  }

  std::map<SegmentKey, int> balances;
  for (const DirectedSegment& segment : raw) {
    const std::vector<Point> points =
        splitPoints(segment.start, segment.end, candidates);
    for (std::size_t index = 1U; index < points.size(); ++index) {
      const SegmentKey key = canonicalSegment(points[index - 1U], points[index]);
      const int direction = points[index - 1U] == key.start ? 1 : -1;
      int& balance = balances[key];
      balance += direction;
      if (balance < -1 || balance > 1) {
        return false;
      }
    }
  }

  std::map<Point, Point, PointOrder> outgoing;
  std::map<Point, std::size_t, PointOrder> incoming;
  for (const auto& [segment, balance] : balances) {
    if (balance == 0) {
      continue;
    }
    const Point start = balance > 0 ? segment.start : segment.end;
    const Point end = balance > 0 ? segment.end : segment.start;
    if (!outgoing.emplace(start, end).second || ++incoming[end] != 1U) {
      return false;
    }
  }
  if (outgoing.size() < 4U || incoming.size() != outgoing.size()) {
    return false;
  }

  const Point first = outgoing.begin()->first;
  Point cursor = first;
  const std::size_t edgeCount = outgoing.size();
  for (std::size_t index = 0U; index < edgeCount; ++index) {
    const auto next = outgoing.find(cursor);
    if (next == outgoing.end()) {
      return false;
    }
    polygon.push_back(cursor);
    cursor = next->second;
    outgoing.erase(next);
  }
  if (cursor != first || !outgoing.empty()) {
    return false;
  }

  bool simplified = true;
  while (simplified && polygon.size() > 4U) {
    simplified = false;
    for (std::size_t index = 0U; index < polygon.size(); ++index) {
      const Point previous =
          polygon[(index + polygon.size() - 1U) % polygon.size()];
      const Point current = polygon[index];
      const Point next = polygon[(index + 1U) % polygon.size()];
      if (pointOnSegmentInterior(current, previous, next)) {
        polygon.erase(polygon.begin() + static_cast<std::ptrdiff_t>(index));
        simplified = true;
        break;
      }
    }
  }

  long double twiceArea = 0.0L;
  for (std::size_t index = 0U; index < polygon.size(); ++index) {
    const Point start = polygon[index];
    const Point end = polygon[(index + 1U) % polygon.size()];
    if (!orthogonal(start, end)) {
      return false;
    }
    twiceArea += static_cast<long double>(start.x) * end.z -
                 static_cast<long double>(end.x) * start.z;
  }
  return twiceArea > 0.0L;
}

[[nodiscard]] Rect polygonBounds(std::span<const Point> polygon) noexcept {
  Rect bounds{polygon.front(), polygon.front()};
  for (Point point : polygon) {
    bounds.minimum.x = std::min(bounds.minimum.x, point.x);
    bounds.minimum.z = std::min(bounds.minimum.z, point.z);
    bounds.maximum.x = std::max(bounds.maximum.x, point.x);
    bounds.maximum.z = std::max(bounds.maximum.z, point.z);
  }
  return bounds;
}

[[nodiscard]] std::vector<std::vector<Point>> roomPolygons(
    const CreativeWorldLayoutRoomGraph& graph) {
  std::vector<std::vector<Point>> polygons(graph.rooms.size());
  for (std::size_t roomIndex = 0U; roomIndex < graph.rooms.size();
       ++roomIndex) {
    std::vector<Point>& polygon = polygons[roomIndex];
    for (const CreativeWorldLayoutRoomBoundary& boundary :
         creativeWorldLayoutRoomBoundaries(graph, roomIndex)) {
      const CreativeWorldLayoutTopologyEdge& edge =
          graph.edges[boundary.topologyEdgeIndex];
      polygon.push_back(
          graph.vertices[boundary.reversed ? edge.endVertexIndex
                                           : edge.startVertexIndex]
              .position);
    }
  }
  return polygons;
}

[[nodiscard]] std::string generatedVertexKey(
    const CreativeWorldLayout& layout,
    std::size_t levelIndex,
    Point point) {
  return layout.levels[levelIndex].stableKey + ".topology.vertex." +
         std::to_string(point.x) + "." + std::to_string(point.z);
}

[[nodiscard]] std::string generatedEdgeKey(
    const CreativeWorldLayout& layout,
    std::size_t levelIndex,
    Point start,
    Point end) {
  const SegmentKey segment = canonicalSegment(start, end);
  return layout.levels[levelIndex].stableKey + ".topology.edge." +
         std::to_string(segment.start.x) + "." +
         std::to_string(segment.start.z) + "." +
         std::to_string(segment.end.x) + "." +
         std::to_string(segment.end.z);
}

[[nodiscard]] std::string uniqueKey(std::string base,
                                    std::set<std::string>& used) {
  if (used.insert(base).second) {
    return base;
  }
  for (std::size_t suffix = 1U;; ++suffix) {
    std::string candidate = base + ".new." + std::to_string(suffix);
    if (used.insert(candidate).second) {
      return candidate;
    }
  }
}

[[nodiscard]] bool oldEdgeContains(
    const CreativeWorldLayoutRoomGraph& graph,
    const CreativeWorldLayoutTopologyEdge& edge,
    std::size_t levelIndex,
    Point start,
    Point end) noexcept {
  if (edge.levelIndex != levelIndex ||
      edge.startVertexIndex >= graph.vertices.size() ||
      edge.endVertexIndex >= graph.vertices.size()) {
    return false;
  }
  const Point oldStart = graph.vertices[edge.startVertexIndex].position;
  const Point oldEnd = graph.vertices[edge.endVertexIndex].position;
  return orthogonal(start, end) && orthogonal(oldStart, oldEnd) &&
         pointOnSegment(start, oldStart, oldEnd) &&
         pointOnSegment(end, oldStart, oldEnd);
}

[[nodiscard]] CreativeWorldLayoutTopologyEdge inheritedWallAttributes(
    const CreativeWorldLayoutRoomGraph& sourceGraph,
    std::size_t levelIndex,
    Point start,
    Point end,
    double fallback) noexcept {
  for (const CreativeWorldLayoutTopologyEdge& edge : sourceGraph.edges) {
    if (oldEdgeContains(sourceGraph, edge, levelIndex, start, end)) {
      return edge;
    }
  }
  CreativeWorldLayoutTopologyEdge attributes;
  attributes.levelIndex = levelIndex;
  attributes.wallThicknessCells = fallback;
  return attributes;
}

[[nodiscard]] bool rebuildTopology(
    CreativeWorldLayout& layout,
    const std::vector<std::vector<Point>>& polygons,
    const CreativeWorldLayoutRoomGraph& sourceGraph,
    CreativeWorldLayoutRoomGraph& rebuilt) {
  if (polygons.size() != layout.rooms.size()) {
    return false;
  }

  std::map<VertexKey, std::pair<std::size_t, std::string>> oldVertices;
  for (std::size_t index = 0U; index < sourceGraph.vertices.size(); ++index) {
    const auto& vertex = sourceGraph.vertices[index];
    oldVertices.emplace(VertexKey{vertex.levelIndex, vertex.position},
                        std::pair{index, vertex.stableKey});
  }
  std::map<EdgeKey, std::pair<std::size_t, std::string>> oldEdges;
  for (std::size_t index = 0U; index < sourceGraph.edges.size(); ++index) {
    const auto& edge = sourceGraph.edges[index];
    const Point start = sourceGraph.vertices[edge.startVertexIndex].position;
    const Point end = sourceGraph.vertices[edge.endVertexIndex].position;
    const SegmentKey segment = canonicalSegment(start, end);
    oldEdges.emplace(EdgeKey{edge.levelIndex, segment.start, segment.end},
                     std::pair{index, edge.stableKey});
  }

  std::vector<VertexKey> splitCandidates;
  for (std::size_t roomIndex = 0U; roomIndex < polygons.size(); ++roomIndex) {
    if (layout.rooms[roomIndex].levelIndex >= layout.levels.size() ||
        polygons[roomIndex].size() < 4U) {
      return false;
    }
    for (Point point : polygons[roomIndex]) {
      splitCandidates.push_back({layout.rooms[roomIndex].levelIndex, point});
    }
  }

  layout.topologyVertices.clear();
  layout.topologyEdges.clear();
  layout.roomBoundaries.clear();
  std::map<VertexKey, std::size_t> vertexIndices;
  std::map<EdgeKey, std::size_t> edgeIndices;
  std::set<std::string> usedVertexKeys;
  std::set<std::string> usedEdgeKeys;

  const auto appendVertex = [&](std::size_t levelIndex,
                                Point point) -> std::size_t {
    const VertexKey key{levelIndex, point};
    const auto existing = vertexIndices.find(key);
    if (existing != vertexIndices.end()) {
      return existing->second;
    }
    std::string stableKey;
    const auto old = oldVertices.find(key);
    if (old != oldVertices.end()) {
      stableKey = old->second.second;
      usedVertexKeys.insert(stableKey);
    } else {
      stableKey = uniqueKey(generatedVertexKey(layout, levelIndex, point),
                            usedVertexKeys);
    }
    const std::size_t index = layout.topologyVertices.size();
    layout.topologyVertices.push_back(
        {levelIndex, std::move(stableKey), point});
    vertexIndices.emplace(key, index);
    return index;
  };

  for (std::size_t roomIndex = 0U; roomIndex < polygons.size(); ++roomIndex) {
    CreativeWorldLayoutRoom& room = layout.rooms[roomIndex];
    const std::vector<Point>& polygon = polygons[roomIndex];
    room.footprint = polygonBounds(polygon);
    std::size_t order = 0U;
    for (std::size_t polygonIndex = 0U; polygonIndex < polygon.size();
         ++polygonIndex) {
      const Point rawStart = polygon[polygonIndex];
      const Point rawEnd = polygon[(polygonIndex + 1U) % polygon.size()];
      std::vector<Point> candidates;
      for (const VertexKey& candidate : splitCandidates) {
        if (candidate.levelIndex == room.levelIndex) {
          candidates.push_back(candidate.point);
        }
      }
      const std::vector<Point> points =
          splitPoints(rawStart, rawEnd, candidates);
      for (std::size_t pointIndex = 1U; pointIndex < points.size();
           ++pointIndex) {
        const Point directedStart = points[pointIndex - 1U];
        const Point directedEnd = points[pointIndex];
        const SegmentKey segment =
            canonicalSegment(directedStart, directedEnd);
        const EdgeKey key{room.levelIndex, segment.start, segment.end};
        std::size_t edgeIndex = kInvalidCreativeWorldLayoutIndex;
        const auto existing = edgeIndices.find(key);
        if (existing != edgeIndices.end()) {
          edgeIndex = existing->second;
          const CreativeWorldLayoutTopologyEdge inherited =
              inheritedWallAttributes(sourceGraph, room.levelIndex,
                                      segment.start, segment.end,
                                      room.wallThicknessCells);
          layout.topologyEdges[edgeIndex].wallThicknessCells = std::max(
              layout.topologyEdges[edgeIndex].wallThicknessCells,
              inherited.wallThicknessCells);
        } else {
          std::string stableKey;
          const auto old = oldEdges.find(key);
          if (old != oldEdges.end()) {
            stableKey = old->second.second;
            usedEdgeKeys.insert(stableKey);
          } else {
            stableKey = uniqueKey(
                generatedEdgeKey(layout, room.levelIndex, segment.start,
                                 segment.end),
                usedEdgeKeys);
          }
          const CreativeWorldLayoutTopologyEdge inherited =
              inheritedWallAttributes(sourceGraph, room.levelIndex,
                                      segment.start, segment.end,
                                      room.wallThicknessCells);
          edgeIndex = layout.topologyEdges.size();
          CreativeWorldLayoutTopologyEdge rebuiltEdge = inherited;
          rebuiltEdge.levelIndex = room.levelIndex;
          rebuiltEdge.stableKey = std::move(stableKey);
          rebuiltEdge.startVertexIndex =
              appendVertex(room.levelIndex, segment.start);
          rebuiltEdge.endVertexIndex =
              appendVertex(room.levelIndex, segment.end);
          layout.topologyEdges.push_back(std::move(rebuiltEdge));
          edgeIndices.emplace(key, edgeIndex);
        }
        layout.roomBoundaries.push_back(
            {roomIndex, edgeIndex, order++, directedStart != segment.start});
      }
    }
    if (order > kCreativeWorldLayoutMaximumRoomBoundaryEdgeCount) {
      return false;
    }
  }

  rebuilt = buildCreativeWorldLayoutRoomGraph(layout);
  if (!rebuilt.accepted) {
    return false;
  }

  std::vector<std::size_t> edgeUseCount(rebuilt.edges.size(), 0U);
  for (const CreativeWorldLayoutRoomBoundary& boundary : rebuilt.boundaries) {
    ++edgeUseCount[boundary.topologyEdgeIndex];
  }
  bool styledNewEdge = false;
  for (std::size_t edgeIndex = 0U; edgeIndex < rebuilt.edges.size();
       ++edgeIndex) {
    CreativeWorldLayoutTopologyEdge& edge = layout.topologyEdges[edgeIndex];
    const Point start =
        layout.topologyVertices[edge.startVertexIndex].position;
    const Point end = layout.topologyVertices[edge.endVertexIndex].position;
    const bool inherited = std::any_of(
        sourceGraph.edges.begin(), sourceGraph.edges.end(),
        [&](const CreativeWorldLayoutTopologyEdge& sourceEdge) {
          return oldEdgeContains(sourceGraph, sourceEdge, edge.levelIndex,
                                 start, end);
        });
    if (inherited || edge.levelIndex >= layout.levels.size() ||
        (edgeUseCount[edgeIndex] != 1U && edgeUseCount[edgeIndex] != 2U)) {
      continue;
    }
    const CreativeWorldLayoutWallProfile profile =
        edgeUseCount[edgeIndex] == 1U
            ? CreativeWorldLayoutWallProfile::Exterior
            : CreativeWorldLayoutWallProfile::Interior;
    CreativeStructuralMaterial material = CreativeStructuralMaterial::Count;
    const std::size_t buildingIndex =
        layout.levels[edge.levelIndex].buildingIndex;
    if (creativeWorldLayoutBuildingBlockoutWallMaterial(
            layout, buildingIndex, profile, material)) {
      edge.profile = profile;
      edge.material = material;
      styledNewEdge = true;
    }
  }
  if (styledNewEdge) {
    rebuilt = buildCreativeWorldLayoutRoomGraph(layout);
  }
  return rebuilt.accepted;
}

[[nodiscard]] bool stableKeyInUse(const CreativeWorldLayout& layout,
                                  const std::string& stableKey) {
  if (stableKey.empty() || stableKey == layout.stableKey) {
    return true;
  }
  const auto contains = [&](const auto& symbols) {
    return std::any_of(symbols.begin(), symbols.end(), [&](const auto& symbol) {
      return symbol.stableKey == stableKey;
    });
  };
  return contains(layout.buildings) || contains(layout.levels) ||
         contains(layout.rooms) || contains(layout.verticalConnectors) ||
         contains(layout.boxes) || contains(layout.walls) ||
         contains(layout.openings) || contains(layout.objects) ||
         contains(layout.terrainProfiles) || contains(layout.terrainPaths) ||
         contains(layout.topologyVertices) || contains(layout.topologyEdges);
}

[[nodiscard]] bool captureOpeningAnchors(
    const CreativeWorldLayout& layout,
    const CreativeWorldLayoutRoomGraph& graph,
    std::vector<OpeningAnchor>& anchors,
    std::size_t& failedOpeningIndex) noexcept {
  anchors.resize(layout.openings.size());
  for (std::size_t index = 0U; index < layout.openings.size(); ++index) {
    const CreativeWorldLayoutOpening& opening = layout.openings[index];
    if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::Wall) {
      continue;
    }
    if (opening.hostKind != CreativeWorldLayoutOpeningHostKind::RoomEdge ||
        opening.roomIndex >= layout.rooms.size() ||
        opening.roomTopologyEdgeIndex >= graph.edges.size() ||
        !std::isfinite(opening.centerOffsetCells) ||
        !std::isfinite(opening.widthCells) || opening.widthCells <= 0.0) {
      failedOpeningIndex = index;
      return false;
    }
    const CreativeWorldLayoutTopologyEdge& edge =
        graph.edges[opening.roomTopologyEdgeIndex];
    const Point start = graph.vertices[edge.startVertexIndex].position;
    const Point end = graph.vertices[edge.endVertexIndex].position;
    const bool horizontal = start.z == end.z;
    const double runMinimum = horizontal ? static_cast<double>(start.x)
                                         : static_cast<double>(start.z);
    const double runMaximum = horizontal ? static_cast<double>(end.x)
                                         : static_cast<double>(end.z);
    const double center = runMinimum + opening.centerOffsetCells;
    const double halfWidth = opening.widthCells * 0.5;
    if (center - halfWidth < runMinimum ||
        center + halfWidth > runMaximum) {
      failedOpeningIndex = index;
      return false;
    }
    anchors[index] = {true,
                      horizontal,
                      opening.roomIndex,
                      edge.levelIndex,
                      horizontal ? start.z : start.x,
                      center,
                      center - halfWidth,
                      center + halfWidth};
  }
  return true;
}

[[nodiscard]] CreativeWorldLayoutRoomEdge cardinalForEdge(
    const CreativeWorldLayoutRoomGraph& graph,
    std::size_t roomIndex,
    std::size_t edgeIndex) noexcept {
  const Rect bounds = graph.roomBounds[roomIndex];
  const CreativeWorldLayoutTopologyEdge& edge = graph.edges[edgeIndex];
  const Point start = graph.vertices[edge.startVertexIndex].position;
  const Point end = graph.vertices[edge.endVertexIndex].position;
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
}

[[nodiscard]] bool remapOpenings(
    const CreativeWorldLayout& source,
    const std::vector<OpeningAnchor>& anchors,
    const std::vector<std::vector<std::size_t>>& candidateRooms,
    const CreativeWorldLayoutRoomGraph& graph,
    CreativeWorldLayout& edited,
    std::size_t& failedOpeningIndex) {
  edited.openings = source.openings;
  constexpr double kEpsilon = 1.0e-9;
  for (std::size_t index = 0U; index < edited.openings.size(); ++index) {
    if (!anchors[index].topologyHosted) {
      continue;
    }
    const OpeningAnchor& anchor = anchors[index];
    if (anchor.sourceRoomIndex >= candidateRooms.size()) {
      failedOpeningIndex = index;
      return false;
    }
    std::size_t matchedRoom = kInvalidCreativeWorldLayoutIndex;
    std::size_t matchedEdge = kInvalidCreativeWorldLayoutIndex;
    for (const std::size_t roomIndex : candidateRooms[anchor.sourceRoomIndex]) {
      if (roomIndex >= graph.rooms.size()) {
        continue;
      }
      for (const CreativeWorldLayoutRoomBoundary& boundary :
           creativeWorldLayoutRoomBoundaries(graph, roomIndex)) {
        const CreativeWorldLayoutTopologyEdge& edge =
            graph.edges[boundary.topologyEdgeIndex];
        if (edge.levelIndex != anchor.levelIndex) {
          continue;
        }
        const Point start = graph.vertices[edge.startVertexIndex].position;
        const Point end = graph.vertices[edge.endVertexIndex].position;
        const bool horizontal = start.z == end.z;
        const std::int32_t line = horizontal ? start.z : start.x;
        const double minimum = horizontal ? static_cast<double>(start.x)
                                          : static_cast<double>(start.z);
        const double maximum = horizontal ? static_cast<double>(end.x)
                                          : static_cast<double>(end.z);
        if (horizontal == anchor.horizontal && line == anchor.line &&
            anchor.minimum >= minimum - kEpsilon &&
            anchor.maximum <= maximum + kEpsilon) {
          if (matchedEdge != kInvalidCreativeWorldLayoutIndex) {
            failedOpeningIndex = index;
            return false;
          }
          matchedRoom = roomIndex;
          matchedEdge = boundary.topologyEdgeIndex;
        }
      }
    }
    if (matchedEdge == kInvalidCreativeWorldLayoutIndex) {
      failedOpeningIndex = index;
      return false;
    }
    CreativeWorldLayoutOpening& opening = edited.openings[index];
    const CreativeWorldLayoutTopologyEdge& edge = graph.edges[matchedEdge];
    const Point start = graph.vertices[edge.startVertexIndex].position;
    const double runMinimum = anchor.horizontal
                                  ? static_cast<double>(start.x)
                                  : static_cast<double>(start.z);
    opening.hostKind = CreativeWorldLayoutOpeningHostKind::RoomEdge;
    opening.wallIndex = kInvalidCreativeWorldLayoutIndex;
    opening.roomIndex = matchedRoom;
    opening.roomTopologyEdgeIndex = matchedEdge;
    opening.roomEdge = cardinalForEdge(graph, matchedRoom, matchedEdge);
    opening.centerOffsetCells = anchor.center - runMinimum;
  }
  return true;
}

[[nodiscard]] bool remapConnectors(
    const CreativeWorldLayout& source,
    const std::vector<std::vector<std::size_t>>& candidateRooms,
    const CreativeWorldLayoutRoomGraph& graph,
    CreativeWorldLayout& edited,
    std::size_t& failedConnectorIndex) {
  edited.verticalConnectors = source.verticalConnectors;
  const auto remapRoom = [&](std::size_t sourceRoomIndex,
                             Rect footprint,
                             std::size_t& output) {
    if (sourceRoomIndex >= candidateRooms.size()) {
      return false;
    }
    output = kInvalidCreativeWorldLayoutIndex;
    for (const std::size_t candidate : candidateRooms[sourceRoomIndex]) {
      if (creativeWorldLayoutRoomContainsRect(graph, candidate, footprint)) {
        if (output != kInvalidCreativeWorldLayoutIndex) {
          return false;
        }
        output = candidate;
      }
    }
    return output != kInvalidCreativeWorldLayoutIndex;
  };

  for (std::size_t index = 0U; index < edited.verticalConnectors.size();
       ++index) {
    CreativeWorldLayoutVerticalConnector& connector =
        edited.verticalConnectors[index];
    if (!remapRoom(source.verticalConnectors[index].lowerRoomIndex,
                   connector.footprint, connector.lowerRoomIndex) ||
        !remapRoom(source.verticalConnectors[index].upperRoomIndex,
                   connector.footprint, connector.upperRoomIndex)) {
      failedConnectorIndex = index;
      return false;
    }
  }

  CreativeGridSettings unitGrid;
  unitGrid.cellSizeMeters = 1.0;
  for (std::size_t index = 0U; index < edited.verticalConnectors.size();
       ++index) {
    if (!planCreativeWorldLayoutVerticalConnector(unitGrid, edited, index)
             .accepted) {
      failedConnectorIndex = index;
      return false;
    }
  }
  return true;
}

void fail(CreativeWorldLayoutRoomOperationResult& result,
          CreativeWorldLayoutRoomOperationStatus status,
          std::string reasonCode) {
  result.accepted = false;
  result.changed = false;
  result.status = status;
  result.edited = {};
  result.sourceToEditedRoomIndices.clear();
  result.sourceToEditedVertexIndices.clear();
  result.sourceToEditedEdgeIndices.clear();
  result.reasonCode = std::move(reasonCode);
}

[[nodiscard]] bool finishOperation(
    const CreativeWorldLayout& canonical,
    const CreativeWorldLayoutRoomGraph& sourceGraph,
    std::vector<std::vector<Point>> polygons,
    const std::vector<std::vector<std::size_t>>& candidateRooms,
    CreativeWorldLayout edited,
    CreativeWorldLayoutRoomOperationResult& result,
    const BoundaryRelocation* relocation = nullptr) {
  std::vector<OpeningAnchor> openingAnchors;
  if (!captureOpeningAnchors(canonical, sourceGraph, openingAnchors,
                             result.failedOpeningIndex)) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::InvalidSourceTopology,
         "creative_world_layout_room_operation_opening_source_invalid");
    return false;
  }
  if (relocation != nullptr) {
    const bool horizontal =
        relocation->editedStart.z == relocation->editedEnd.z;
    const std::int32_t editedLine =
        horizontal ? relocation->editedStart.z : relocation->editedStart.x;
    for (std::size_t index = 0U; index < canonical.openings.size(); ++index) {
      if (canonical.openings[index].roomTopologyEdgeIndex ==
              relocation->sourceEdgeIndex &&
          openingAnchors[index].topologyHosted) {
        openingAnchors[index].line = editedLine;
      }
    }
  }

  edited.openings.clear();
  edited.verticalConnectors.clear();
  CreativeWorldLayoutRoomGraph rebuilt;
  if (!rebuildTopology(edited, polygons, sourceGraph, rebuilt)) {
    fail(result,
         std::any_of(polygons.begin(), polygons.end(), [](const auto& polygon) {
           return polygon.size() >
                  kCreativeWorldLayoutMaximumRoomBoundaryEdgeCount;
         })
             ? CreativeWorldLayoutRoomOperationStatus::BoundaryCapacityExceeded
             : CreativeWorldLayoutRoomOperationStatus::ResultingTopologyInvalid,
         "creative_world_layout_room_operation_topology_rebuild_failed");
    return false;
  }
  if (relocation != nullptr) {
    std::map<VertexKey, std::size_t> rebuiltVertices;
    for (std::size_t index = 0U; index < edited.topologyVertices.size();
         ++index) {
      const auto& vertex = edited.topologyVertices[index];
      rebuiltVertices.emplace(
          VertexKey{vertex.levelIndex, vertex.position}, index);
    }
    std::map<EdgeKey, std::size_t> rebuiltEdges;
    for (std::size_t index = 0U; index < edited.topologyEdges.size(); ++index) {
      const CreativeWorldLayoutTopologyEdge& edge =
          edited.topologyEdges[index];
      const SegmentKey segment = canonicalSegment(
          edited.topologyVertices[edge.startVertexIndex].position,
          edited.topologyVertices[edge.endVertexIndex].position);
      rebuiltEdges.emplace(
          EdgeKey{edge.levelIndex, segment.start, segment.end}, index);
    }

    for (const CreativeWorldLayoutTopologyVertex& sourceVertex :
         sourceGraph.vertices) {
      const Point target = relocatedPoint(
          *relocation, sourceVertex.levelIndex, sourceVertex.position);
      if (target == sourceVertex.position) {
        continue;
      }
      const auto found = rebuiltVertices.find(
          VertexKey{sourceVertex.levelIndex, target});
      if (found == rebuiltVertices.end()) {
        fail(result,
             CreativeWorldLayoutRoomOperationStatus::ResultingTopologyInvalid,
             "creative_world_layout_room_boundary_move_vertex_missing");
        return false;
      }
      edited.topologyVertices[found->second].stableKey = sourceVertex.stableKey;
    }

    for (const CreativeWorldLayoutTopologyEdge& sourceEdge :
         sourceGraph.edges) {
      const Point sourceStart =
          sourceGraph.vertices[sourceEdge.startVertexIndex].position;
      const Point sourceEnd =
          sourceGraph.vertices[sourceEdge.endVertexIndex].position;
      const Point targetStart = relocatedPoint(
          *relocation, sourceEdge.levelIndex, sourceStart);
      const Point targetEnd = relocatedPoint(
          *relocation, sourceEdge.levelIndex, sourceEnd);
      if (targetStart == sourceStart && targetEnd == sourceEnd) {
        continue;
      }
      const SegmentKey target = canonicalSegment(targetStart, targetEnd);
      const auto found = rebuiltEdges.find(
          EdgeKey{sourceEdge.levelIndex, target.start, target.end});
      if (found == rebuiltEdges.end()) {
        if (&sourceEdge ==
            &sourceGraph.edges[relocation->sourceEdgeIndex]) {
          fail(result,
               CreativeWorldLayoutRoomOperationStatus::ResultingTopologyInvalid,
               "creative_world_layout_room_boundary_move_host_missing");
          return false;
        }
        continue;
      }
      CreativeWorldLayoutTopologyEdge& editedEdge =
          edited.topologyEdges[found->second];
      editedEdge.stableKey = sourceEdge.stableKey;
      editedEdge.wallThicknessCells = sourceEdge.wallThicknessCells;
      editedEdge.wallHeightCells = sourceEdge.wallHeightCells;
      editedEdge.profile = sourceEdge.profile;
      editedEdge.material = sourceEdge.material;
      editedEdge.joinStyle = sourceEdge.joinStyle;
    }

    rebuilt = buildCreativeWorldLayoutRoomGraph(edited);
    if (!rebuilt.accepted) {
      fail(result,
           CreativeWorldLayoutRoomOperationStatus::ResultingTopologyInvalid,
           rebuilt.reasonCode);
      return false;
    }
  }
  if (!remapOpenings(canonical, openingAnchors, candidateRooms, rebuilt, edited,
                     result.failedOpeningIndex)) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::OpeningConflict,
         "creative_world_layout_room_operation_opening_conflict");
    return false;
  }
  const CreativeWorldLayoutRoomGraph withOpenings =
      buildCreativeWorldLayoutRoomGraph(edited);
  if (!withOpenings.accepted) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::ResultingTopologyInvalid,
         "creative_world_layout_room_operation_opening_topology_invalid");
    return false;
  }
  if (!remapConnectors(canonical, candidateRooms, withOpenings, edited,
                       result.failedConnectorIndex)) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::ConnectorConflict,
         "creative_world_layout_room_operation_connector_conflict");
    return false;
  }

  for (std::size_t buildingIndex = 0U; buildingIndex < edited.buildings.size();
       ++buildingIndex) {
    const bool hasRoom = std::any_of(
        edited.rooms.begin(), edited.rooms.end(),
        [buildingIndex](const CreativeWorldLayoutRoom& room) {
          return room.buildingIndex == buildingIndex;
        });
    if (hasRoom && !refreshCreativeWorldLayoutBuildingRoomFootprint(
                       edited, buildingIndex)) {
      fail(result,
           CreativeWorldLayoutRoomOperationStatus::ResultingTopologyInvalid,
           "creative_world_layout_room_operation_building_bounds_invalid");
      return false;
    }
  }
  const CreativeWorldLayoutRoomGraph finalGraph =
      buildCreativeWorldLayoutRoomGraph(edited);
  if (!finalGraph.accepted) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::ResultingTopologyInvalid,
         finalGraph.reasonCode);
    return false;
  }

  std::map<VertexKey, std::size_t> finalVertices;
  for (std::size_t index = 0U; index < finalGraph.vertices.size(); ++index) {
    finalVertices.emplace(
        VertexKey{finalGraph.vertices[index].levelIndex,
                  finalGraph.vertices[index].position},
        index);
  }
  result.sourceToEditedVertexIndices.assign(
      sourceGraph.vertices.size(), kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < sourceGraph.vertices.size(); ++index) {
    const auto& vertex = sourceGraph.vertices[index];
    const Point target = relocation != nullptr
                             ? relocatedPoint(*relocation, vertex.levelIndex,
                                              vertex.position)
                             : vertex.position;
    const auto found = finalVertices.find(VertexKey{vertex.levelIndex, target});
    if (found != finalVertices.end()) {
      result.sourceToEditedVertexIndices[index] = found->second;
    }
  }

  std::map<EdgeKey, std::size_t> finalEdges;
  for (std::size_t index = 0U; index < finalGraph.edges.size(); ++index) {
    const auto& edge = finalGraph.edges[index];
    finalEdges.emplace(
        EdgeKey{edge.levelIndex,
                finalGraph.vertices[edge.startVertexIndex].position,
                finalGraph.vertices[edge.endVertexIndex].position},
        index);
  }
  result.sourceToEditedEdgeIndices.assign(
      sourceGraph.edges.size(), kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < sourceGraph.edges.size(); ++index) {
    const auto& edge = sourceGraph.edges[index];
    const Point sourceStart =
        sourceGraph.vertices[edge.startVertexIndex].position;
    const Point sourceEnd = sourceGraph.vertices[edge.endVertexIndex].position;
    const SegmentKey target = canonicalSegment(
        relocation != nullptr
            ? relocatedPoint(*relocation, edge.levelIndex, sourceStart)
            : sourceStart,
        relocation != nullptr
            ? relocatedPoint(*relocation, edge.levelIndex, sourceEnd)
            : sourceEnd);
    const auto found = finalEdges.find(
        EdgeKey{edge.levelIndex, target.start, target.end});
    if (found != finalEdges.end()) {
      result.sourceToEditedEdgeIndices[index] = found->second;
    }
  }
  result.accepted = true;
  result.changed = true;
  result.status = CreativeWorldLayoutRoomOperationStatus::Ready;
  result.edited = std::move(edited);
  result.reasonCode = "creative_world_layout_room_operation_ready";
  return true;
}

[[nodiscard]] bool prepareSource(
    const CreativeWorldLayout& source,
    CreativeWorldLayout& canonical,
    CreativeWorldLayoutRoomGraph& graph,
    CreativeWorldLayoutRoomOperationResult& result) {
  const CreativeWorldLayoutRoomGraphMaterializeResult materialized =
      materializeCreativeWorldLayoutRoomGraph(source);
  if (!materialized.accepted) {
    result.failedOpeningIndex = materialized.failedOpeningIndex;
    fail(result, CreativeWorldLayoutRoomOperationStatus::InvalidSourceTopology,
         materialized.reasonCode);
    return false;
  }
  canonical = materialized.edited;
  graph = buildCreativeWorldLayoutRoomGraph(canonical);
  if (!graph.accepted) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::InvalidSourceTopology,
         graph.reasonCode);
    return false;
  }
  return true;
}

[[nodiscard]] std::vector<std::size_t> identityIndices(std::size_t count) {
  std::vector<std::size_t> indices(count);
  for (std::size_t index = 0U; index < count; ++index) {
    indices[index] = index;
  }
  return indices;
}

void composeIndexRemap(std::vector<std::size_t>& sourceToCurrent,
                       const std::vector<std::size_t>& currentToEdited) {
  for (std::size_t& index : sourceToCurrent) {
    index = index < currentToEdited.size()
                ? currentToEdited[index]
                : kInvalidCreativeWorldLayoutIndex;
  }
}

}  // namespace

CreativeWorldLayoutRoomOperationResult splitCreativeWorldLayoutRoom(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutRoomSplitRequest& request) {
  CreativeWorldLayoutRoomOperationResult result;
  result.requested = true;
  result.primaryRoomIndex = request.roomIndex;
  if (request.roomIndex >= source.rooms.size() ||
      request.axis >= CreativeWorldLayoutRoomSplitAxis::Count ||
      request.newRoomStableKey.empty() || request.newRoomName.empty()) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::InvalidRequest,
         "creative_world_layout_room_split_request_invalid");
    return result;
  }
  if (stableKeyInUse(source, request.newRoomStableKey)) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::DuplicateStableKey,
         "creative_world_layout_room_split_key_duplicate");
    return result;
  }

  CreativeWorldLayout canonical;
  CreativeWorldLayoutRoomGraph sourceGraph;
  if (!prepareSource(source, canonical, sourceGraph, result)) {
    return result;
  }
  std::vector<std::vector<Point>> polygons = roomPolygons(sourceGraph);
  const std::span<const Rect> sourceRects =
      creativeWorldLayoutRoomSurfaceRects(sourceGraph, request.roomIndex);
  std::vector<Rect> minimumSide;
  std::vector<Rect> maximumSide;
  for (Rect rect : sourceRects) {
    if (request.axis == CreativeWorldLayoutRoomSplitAxis::X) {
      if (rect.minimum.x < request.coordinate) {
        Rect clipped = rect;
        clipped.maximum.x = std::min(clipped.maximum.x, request.coordinate);
        if (validRect(clipped)) {
          minimumSide.push_back(clipped);
        }
      }
      if (rect.maximum.x > request.coordinate) {
        Rect clipped = rect;
        clipped.minimum.x = std::max(clipped.minimum.x, request.coordinate);
        if (validRect(clipped)) {
          maximumSide.push_back(clipped);
        }
      }
    } else {
      if (rect.minimum.z < request.coordinate) {
        Rect clipped = rect;
        clipped.maximum.z = std::min(clipped.maximum.z, request.coordinate);
        if (validRect(clipped)) {
          minimumSide.push_back(clipped);
        }
      }
      if (rect.maximum.z > request.coordinate) {
        Rect clipped = rect;
        clipped.minimum.z = std::max(clipped.minimum.z, request.coordinate);
        if (validRect(clipped)) {
          maximumSide.push_back(clipped);
        }
      }
    }
  }
  if (minimumSide.empty() || maximumSide.empty()) {
    fail(result,
         CreativeWorldLayoutRoomOperationStatus::CutDoesNotBisectRoom,
         "creative_world_layout_room_split_does_not_bisect");
    return result;
  }
  std::vector<Point> minimumPolygon;
  std::vector<Point> maximumPolygon;
  if (!rectanglesToPolygon(minimumSide, minimumPolygon) ||
      !rectanglesToPolygon(maximumSide, maximumPolygon)) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::DisconnectedResult,
         "creative_world_layout_room_split_disconnected_result");
    return result;
  }
  if (minimumPolygon.size() >
          kCreativeWorldLayoutMaximumRoomBoundaryEdgeCount ||
      maximumPolygon.size() >
          kCreativeWorldLayoutMaximumRoomBoundaryEdgeCount) {
    fail(result,
         CreativeWorldLayoutRoomOperationStatus::BoundaryCapacityExceeded,
         "creative_world_layout_room_split_boundary_capacity_exceeded");
    return result;
  }

  CreativeWorldLayout edited = canonical;
  polygons[request.roomIndex] = std::move(minimumPolygon);
  CreativeWorldLayoutRoom newRoom = edited.rooms[request.roomIndex];
  newRoom.stableKey = request.newRoomStableKey;
  newRoom.name = request.newRoomName;
  edited.rooms.push_back(std::move(newRoom));
  polygons.push_back(std::move(maximumPolygon));
  result.secondaryRoomIndex = edited.rooms.size() - 1U;
  result.sourceToEditedRoomIndices.resize(source.rooms.size());
  std::vector<std::vector<std::size_t>> candidateRooms(source.rooms.size());
  for (std::size_t roomIndex = 0U; roomIndex < source.rooms.size();
       ++roomIndex) {
    result.sourceToEditedRoomIndices[roomIndex] = roomIndex;
    candidateRooms[roomIndex].push_back(roomIndex);
  }
  candidateRooms[request.roomIndex].push_back(result.secondaryRoomIndex);

  static_cast<void>(finishOperation(
      canonical, sourceGraph, std::move(polygons), candidateRooms,
      std::move(edited), result));
  return result;
}

CreativeWorldLayoutRoomOperationResult mergeCreativeWorldLayoutRooms(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutRoomMergeRequest& request) {
  CreativeWorldLayoutRoomOperationResult result;
  result.requested = true;
  if (request.primaryRoomIndex >= source.rooms.size() ||
      request.secondaryRoomIndex >= source.rooms.size() ||
      request.primaryRoomIndex == request.secondaryRoomIndex) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::InvalidRequest,
         "creative_world_layout_room_merge_request_invalid");
    return result;
  }

  CreativeWorldLayout canonical;
  CreativeWorldLayoutRoomGraph sourceGraph;
  if (!prepareSource(source, canonical, sourceGraph, result)) {
    return result;
  }
  const CreativeWorldLayoutRoom& primary =
      canonical.rooms[request.primaryRoomIndex];
  const CreativeWorldLayoutRoom& secondary =
      canonical.rooms[request.secondaryRoomIndex];
  if (primary.buildingIndex != secondary.buildingIndex ||
      primary.levelIndex != secondary.levelIndex) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::InvalidRequest,
         "creative_world_layout_room_merge_ownership_mismatch");
    return result;
  }

  std::set<std::size_t> primaryEdges;
  for (const CreativeWorldLayoutRoomBoundary& boundary :
       creativeWorldLayoutRoomBoundaries(sourceGraph,
                                         request.primaryRoomIndex)) {
    primaryEdges.insert(boundary.topologyEdgeIndex);
  }
  std::set<std::size_t> sharedEdges;
  for (const CreativeWorldLayoutRoomBoundary& boundary :
       creativeWorldLayoutRoomBoundaries(sourceGraph,
                                         request.secondaryRoomIndex)) {
    if (primaryEdges.contains(boundary.topologyEdgeIndex)) {
      sharedEdges.insert(boundary.topologyEdgeIndex);
    }
  }
  if (sharedEdges.empty()) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::RoomsNotAdjacent,
         "creative_world_layout_room_merge_not_adjacent");
    return result;
  }
  for (std::size_t index = 0U; index < canonical.openings.size(); ++index) {
    const CreativeWorldLayoutOpening& opening = canonical.openings[index];
    if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge &&
        sharedEdges.contains(opening.roomTopologyEdgeIndex)) {
      result.failedOpeningIndex = index;
      fail(result, CreativeWorldLayoutRoomOperationStatus::OpeningConflict,
           "creative_world_layout_room_merge_removes_opening_host");
      return result;
    }
  }

  std::vector<Rect> mergedRects;
  const std::span<const Rect> primaryRects =
      creativeWorldLayoutRoomSurfaceRects(sourceGraph,
                                          request.primaryRoomIndex);
  const std::span<const Rect> secondaryRects =
      creativeWorldLayoutRoomSurfaceRects(sourceGraph,
                                          request.secondaryRoomIndex);
  mergedRects.insert(mergedRects.end(), primaryRects.begin(),
                     primaryRects.end());
  mergedRects.insert(mergedRects.end(), secondaryRects.begin(),
                     secondaryRects.end());
  std::vector<Point> mergedPolygon;
  if (!rectanglesToPolygon(mergedRects, mergedPolygon)) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::DisconnectedResult,
         "creative_world_layout_room_merge_disconnected_result");
    return result;
  }
  if (mergedPolygon.size() >
      kCreativeWorldLayoutMaximumRoomBoundaryEdgeCount) {
    fail(result,
         CreativeWorldLayoutRoomOperationStatus::BoundaryCapacityExceeded,
         "creative_world_layout_room_merge_boundary_capacity_exceeded");
    return result;
  }

  CreativeWorldLayout edited = canonical;
  std::vector<std::vector<Point>> polygons = roomPolygons(sourceGraph);
  polygons[request.primaryRoomIndex] = std::move(mergedPolygon);
  edited.rooms.erase(edited.rooms.begin() +
                     static_cast<std::ptrdiff_t>(request.secondaryRoomIndex));
  polygons.erase(polygons.begin() +
                 static_cast<std::ptrdiff_t>(request.secondaryRoomIndex));
  const std::size_t editedPrimary =
      request.primaryRoomIndex -
      (request.secondaryRoomIndex < request.primaryRoomIndex ? 1U : 0U);
  result.primaryRoomIndex = editedPrimary;
  result.secondaryRoomIndex = kInvalidCreativeWorldLayoutIndex;
  result.sourceToEditedRoomIndices.resize(source.rooms.size());
  std::vector<std::vector<std::size_t>> candidateRooms(source.rooms.size());
  for (std::size_t roomIndex = 0U; roomIndex < source.rooms.size();
       ++roomIndex) {
    std::size_t mapped = roomIndex;
    if (roomIndex == request.primaryRoomIndex ||
        roomIndex == request.secondaryRoomIndex) {
      mapped = editedPrimary;
    } else if (roomIndex > request.secondaryRoomIndex) {
      --mapped;
    }
    result.sourceToEditedRoomIndices[roomIndex] = mapped;
    candidateRooms[roomIndex].push_back(mapped);
  }

  static_cast<void>(finishOperation(
      canonical, sourceGraph, std::move(polygons), candidateRooms,
      std::move(edited), result));
  return result;
}

CreativeWorldLayoutRoomOperationResult moveCreativeWorldLayoutRoomBoundary(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutRoomBoundaryMoveRequest& request) {
  CreativeWorldLayoutRoomOperationResult result;
  result.requested = true;

  CreativeWorldLayout canonical;
  CreativeWorldLayoutRoomGraph sourceGraph;
  if (!prepareSource(source, canonical, sourceGraph, result)) {
    return result;
  }
  if (request.topologyEdgeIndex >= sourceGraph.edges.size()) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::InvalidRequest,
         "creative_world_layout_room_boundary_move_request_invalid");
    return result;
  }

  const CreativeWorldLayoutTopologyEdge& edge =
      sourceGraph.edges[request.topologyEdgeIndex];
  const Point sourceStart =
      sourceGraph.vertices[edge.startVertexIndex].position;
  const Point sourceEnd = sourceGraph.vertices[edge.endVertexIndex].position;
  const bool horizontal = sourceStart.z == sourceEnd.z;
  const std::int32_t sourceCoordinate =
      horizontal ? sourceStart.z : sourceStart.x;
  if (request.coordinate == sourceCoordinate) {
    result.accepted = true;
    result.status = CreativeWorldLayoutRoomOperationStatus::NoChange;
    result.reasonCode =
        "creative_world_layout_room_boundary_move_no_change";
    return result;
  }

  BoundaryRelocation relocation;
  relocation.sourceEdgeIndex = request.topologyEdgeIndex;
  relocation.levelIndex = edge.levelIndex;
  relocation.sourceStart = sourceStart;
  relocation.sourceEnd = sourceEnd;
  relocation.editedStart = sourceStart;
  relocation.editedEnd = sourceEnd;
  if (horizontal) {
    relocation.editedStart.z = request.coordinate;
    relocation.editedEnd.z = request.coordinate;
  } else {
    relocation.editedStart.x = request.coordinate;
    relocation.editedEnd.x = request.coordinate;
  }

  std::vector<std::vector<Point>> polygons = roomPolygons(sourceGraph);
  for (std::size_t roomIndex = 0U; roomIndex < polygons.size(); ++roomIndex) {
    if (canonical.rooms[roomIndex].levelIndex != edge.levelIndex) {
      continue;
    }
    for (Point& point : polygons[roomIndex]) {
      if (point == sourceStart) {
        point = relocation.editedStart;
      } else if (point == sourceEnd) {
        point = relocation.editedEnd;
      }
    }
  }

  std::vector<std::vector<std::size_t>> candidateRooms(source.rooms.size());
  result.sourceToEditedRoomIndices.resize(source.rooms.size());
  for (std::size_t roomIndex = 0U; roomIndex < source.rooms.size();
       ++roomIndex) {
    candidateRooms[roomIndex].push_back(roomIndex);
    result.sourceToEditedRoomIndices[roomIndex] = roomIndex;
  }
  for (const CreativeWorldLayoutRoomBoundary& boundary :
       sourceGraph.boundaries) {
    if (boundary.topologyEdgeIndex == request.topologyEdgeIndex) {
      result.primaryRoomIndex = std::min(result.primaryRoomIndex,
                                         boundary.roomIndex);
    }
  }

  CreativeWorldLayout edited = canonical;
  static_cast<void>(finishOperation(
      canonical, sourceGraph, std::move(polygons), candidateRooms,
      std::move(edited), result, &relocation));
  return result;
}

CreativeWorldLayoutRoomOperationResult moveCreativeWorldLayoutRoomCorner(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutRoomCornerMoveRequest& request) {
  CreativeWorldLayoutRoomOperationResult result;
  result.requested = true;
  result.primaryRoomIndex = request.roomIndex;

  CreativeWorldLayout canonical;
  CreativeWorldLayoutRoomGraph sourceGraph;
  if (!prepareSource(source, canonical, sourceGraph, result)) {
    return result;
  }
  if (request.roomIndex >= sourceGraph.rooms.size() ||
      request.topologyVertexIndex >= sourceGraph.vertices.size()) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::InvalidRequest,
         "creative_world_layout_room_corner_move_request_invalid");
    return result;
  }

  std::size_t horizontalEdgeIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t verticalEdgeIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t incidentEdgeCount = 0U;
  for (const CreativeWorldLayoutRoomBoundary& boundary :
       creativeWorldLayoutRoomBoundaries(sourceGraph, request.roomIndex)) {
    const CreativeWorldLayoutTopologyEdge& edge =
        sourceGraph.edges[boundary.topologyEdgeIndex];
    if (edge.startVertexIndex != request.topologyVertexIndex &&
        edge.endVertexIndex != request.topologyVertexIndex) {
      continue;
    }
    ++incidentEdgeCount;
    const Point start = sourceGraph.vertices[edge.startVertexIndex].position;
    const Point end = sourceGraph.vertices[edge.endVertexIndex].position;
    if (start.z == end.z) {
      horizontalEdgeIndex = boundary.topologyEdgeIndex;
    } else {
      verticalEdgeIndex = boundary.topologyEdgeIndex;
    }
  }
  if (incidentEdgeCount != 2U ||
      horizontalEdgeIndex >= sourceGraph.edges.size() ||
      verticalEdgeIndex >= sourceGraph.edges.size()) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::InvalidRequest,
         "creative_world_layout_room_corner_move_vertex_not_owned");
    return result;
  }

  const Point sourcePosition =
      sourceGraph.vertices[request.topologyVertexIndex].position;
  if (request.position == sourcePosition) {
    result.accepted = true;
    result.status = CreativeWorldLayoutRoomOperationStatus::NoChange;
    result.reasonCode = "creative_world_layout_room_corner_move_no_change";
    return result;
  }

  result.sourceToEditedRoomIndices = identityIndices(sourceGraph.rooms.size());
  result.sourceToEditedVertexIndices =
      identityIndices(sourceGraph.vertices.size());
  result.sourceToEditedEdgeIndices = identityIndices(sourceGraph.edges.size());
  CreativeWorldLayout edited = canonical;

  const auto moveEdge = [&](std::size_t sourceEdgeIndex,
                            std::int32_t coordinate) {
    const std::size_t currentEdgeIndex =
        result.sourceToEditedEdgeIndices[sourceEdgeIndex];
    if (currentEdgeIndex == kInvalidCreativeWorldLayoutIndex) {
      fail(result, CreativeWorldLayoutRoomOperationStatus::ResultingTopologyInvalid,
           "creative_world_layout_room_corner_move_edge_lost");
      return false;
    }
    CreativeWorldLayoutRoomOperationResult step =
        moveCreativeWorldLayoutRoomBoundary(edited,
                                            {currentEdgeIndex, coordinate});
    if (!step.accepted) {
      result.failedOpeningIndex = step.failedOpeningIndex;
      result.failedConnectorIndex = step.failedConnectorIndex;
      fail(result, step.status, std::move(step.reasonCode));
      return false;
    }
    if (!step.changed) {
      return true;
    }
    composeIndexRemap(result.sourceToEditedRoomIndices,
                      step.sourceToEditedRoomIndices);
    composeIndexRemap(result.sourceToEditedVertexIndices,
                      step.sourceToEditedVertexIndices);
    composeIndexRemap(result.sourceToEditedEdgeIndices,
                      step.sourceToEditedEdgeIndices);
    edited = std::move(step.edited);
    return true;
  };

  if (request.position.z != sourcePosition.z &&
      !moveEdge(horizontalEdgeIndex, request.position.z)) {
    return result;
  }
  if (request.position.x != sourcePosition.x &&
      !moveEdge(verticalEdgeIndex, request.position.x)) {
    return result;
  }

  result.accepted = true;
  result.changed = true;
  result.status = CreativeWorldLayoutRoomOperationStatus::Ready;
  result.primaryRoomIndex =
      result.sourceToEditedRoomIndices[request.roomIndex];
  result.edited = std::move(edited);
  result.reasonCode = "creative_world_layout_room_corner_move_ready";
  return result;
}

CreativeWorldLayoutRoomOperationResult setCreativeWorldLayoutRoomEdgeSettings(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutRoomEdgeSettingsRequest& request) {
  CreativeWorldLayoutRoomOperationResult result;
  result.requested = true;

  CreativeWorldLayout canonical;
  CreativeWorldLayoutRoomGraph sourceGraph;
  if (!prepareSource(source, canonical, sourceGraph, result)) {
    return result;
  }
  if (request.topologyEdgeIndex >= sourceGraph.edges.size() ||
      request.fixedEndpoint >= CreativeWorldLayoutRoomEdgeAnchor::Count ||
      request.lengthCells == 0U ||
      !std::isfinite(request.wallThicknessCells) ||
      request.wallThicknessCells <= 0.0 ||
      request.profile >= CreativeWorldLayoutWallProfile::Count ||
      request.material >= CreativeStructuralMaterial::Count ||
      request.joinStyle >= CreativeWorldLayoutWallJoinStyle::Count) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::InvalidRequest,
         "creative_world_layout_room_edge_settings_request_invalid");
    return result;
  }

  const CreativeWorldLayoutTopologyEdge& sourceEdge =
      sourceGraph.edges[request.topologyEdgeIndex];
  const Point sourceStart =
      sourceGraph.vertices[sourceEdge.startVertexIndex].position;
  const Point sourceEnd =
      sourceGraph.vertices[sourceEdge.endVertexIndex].position;
  const std::uint64_t sourceLength = static_cast<std::uint64_t>(
      sourceStart.x == sourceEnd.x
          ? static_cast<std::int64_t>(sourceEnd.z) - sourceStart.z
          : static_cast<std::int64_t>(sourceEnd.x) - sourceStart.x);
  const bool lengthChanged = sourceLength != request.lengthCells;
  const bool thicknessChanged =
      sourceEdge.wallThicknessCells != request.wallThicknessCells;
  const bool attributesChanged =
      sourceEdge.wallHeightCells != request.wallHeightCells ||
      sourceEdge.profile != request.profile ||
      sourceEdge.material != request.material ||
      sourceEdge.joinStyle != request.joinStyle;
  if (!lengthChanged && !thicknessChanged && !attributesChanged) {
    result.accepted = true;
    result.status = CreativeWorldLayoutRoomOperationStatus::NoChange;
    result.reasonCode =
        "creative_world_layout_room_edge_settings_no_change";
    return result;
  }

  result.primaryRoomIndex = kInvalidCreativeWorldLayoutIndex;
  for (const CreativeWorldLayoutRoomBoundary& boundary :
       sourceGraph.boundaries) {
    if (boundary.topologyEdgeIndex == request.topologyEdgeIndex) {
      result.primaryRoomIndex =
          std::min(result.primaryRoomIndex, boundary.roomIndex);
    }
  }
  if (result.primaryRoomIndex >= sourceGraph.rooms.size()) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::InvalidRequest,
         "creative_world_layout_room_edge_settings_owner_missing");
    return result;
  }

  CreativeWorldLayout edited = canonical;
  result.sourceToEditedRoomIndices = identityIndices(sourceGraph.rooms.size());
  result.sourceToEditedVertexIndices =
      identityIndices(sourceGraph.vertices.size());
  result.sourceToEditedEdgeIndices = identityIndices(sourceGraph.edges.size());
  if (lengthChanged) {
    const bool keepStart =
        request.fixedEndpoint == CreativeWorldLayoutRoomEdgeAnchor::Start;
    const Point fixed = keepStart ? sourceStart : sourceEnd;
    Point moved = fixed;
    const std::int64_t direction = keepStart ? 1 : -1;
    const std::int64_t coordinate =
        (sourceStart.x == sourceEnd.x ? static_cast<std::int64_t>(fixed.z)
                                     : static_cast<std::int64_t>(fixed.x)) +
        direction * static_cast<std::int64_t>(request.lengthCells);
    if (coordinate < std::numeric_limits<std::int32_t>::min() ||
        coordinate > std::numeric_limits<std::int32_t>::max()) {
      fail(result, CreativeWorldLayoutRoomOperationStatus::InvalidRequest,
           "creative_world_layout_room_edge_settings_coordinate_overflow");
      return result;
    }
    if (sourceStart.x == sourceEnd.x) {
      moved.z = static_cast<std::int32_t>(coordinate);
    } else {
      moved.x = static_cast<std::int32_t>(coordinate);
    }
    const std::size_t movedVertex =
        keepStart ? sourceEdge.endVertexIndex : sourceEdge.startVertexIndex;
    CreativeWorldLayoutRoomOperationResult resized =
        moveCreativeWorldLayoutRoomCorner(
            canonical, {result.primaryRoomIndex, movedVertex, moved});
    if (!resized.accepted) {
      return resized;
    }
    edited = std::move(resized.edited);
    result.sourceToEditedRoomIndices =
        std::move(resized.sourceToEditedRoomIndices);
    result.sourceToEditedVertexIndices =
        std::move(resized.sourceToEditedVertexIndices);
    result.sourceToEditedEdgeIndices =
        std::move(resized.sourceToEditedEdgeIndices);
    result.primaryRoomIndex = resized.primaryRoomIndex;
  }

  const std::size_t editedEdgeIndex =
      result.sourceToEditedEdgeIndices[request.topologyEdgeIndex];
  if (editedEdgeIndex >= edited.topologyEdges.size()) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::ResultingTopologyInvalid,
         "creative_world_layout_room_edge_settings_edge_lost");
    return result;
  }
  edited.topologyEdges[editedEdgeIndex].wallThicknessCells =
      request.wallThicknessCells;
  edited.topologyEdges[editedEdgeIndex].wallHeightCells =
      request.wallHeightCells;
  edited.topologyEdges[editedEdgeIndex].profile = request.profile;
  edited.topologyEdges[editedEdgeIndex].material = request.material;
  edited.topologyEdges[editedEdgeIndex].joinStyle = request.joinStyle;
  const CreativeWorldLayoutRoomGraph editedGraph =
      buildCreativeWorldLayoutRoomGraph(edited);
  if (!editedGraph.accepted) {
    fail(result, CreativeWorldLayoutRoomOperationStatus::ResultingTopologyInvalid,
         editedGraph.reasonCode);
    return result;
  }

  result.accepted = true;
  result.changed = true;
  result.status = CreativeWorldLayoutRoomOperationStatus::Ready;
  result.edited = std::move(edited);
  result.reasonCode = "creative_world_layout_room_edge_settings_ready";
  return result;
}

}  // namespace iggy3d::creative
