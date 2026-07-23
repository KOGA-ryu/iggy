#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeWorldLayout baseLayout() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "orthogonal_rooms";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building";
  building.name = "Building";
  building.rootFootprint = {{0, 0}, {12, 12}};
  layout.buildings.push_back(building);
  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "ground";
  level.name = "Ground";
  layout.levels.push_back(level);
  return layout;
}

void appendRoom(cr::CreativeWorldLayout& layout,
                std::string key,
                cr::CreativeWorldLayoutRect footprint) {
  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = 0U;
  room.levelIndex = 0U;
  room.stableKey = std::move(key);
  room.name = "Room";
  room.footprint = footprint;
  layout.rooms.push_back(std::move(room));
}

void appendLRoomGraph(cr::CreativeWorldLayout& layout) {
  appendRoom(layout, "room_l", {{0, 0}, {6, 6}});
  const cr::CreativeTerrainCoord2 points[] = {
      {0, 0}, {6, 0}, {6, 2}, {2, 2}, {2, 6}, {0, 6},
  };
  for (std::size_t index = 0U; index < std::size(points); ++index) {
    layout.topologyVertices.push_back(
        {0U, "vertex_" + std::to_string(index), points[index]});
  }
  const struct {
    std::size_t start;
    std::size_t end;
  } edges[] = {
      {0U, 1U}, {1U, 2U}, {3U, 2U},
      {3U, 4U}, {5U, 4U}, {0U, 5U},
  };
  for (std::size_t index = 0U; index < std::size(edges); ++index) {
    layout.topologyEdges.push_back(
        {0U, "edge_" + std::to_string(index), edges[index].start,
         edges[index].end, 0.25});
  }
  const bool reversed[] = {false, false, true, false, true, true};
  for (std::size_t index = 0U; index < std::size(reversed); ++index) {
    layout.roomBoundaries.push_back({0U, index, index, reversed[index]});
  }
}

bool legacyRectanglesNormalizeToSharedGraph() {
  cr::CreativeWorldLayout layout = baseLayout();
  appendRoom(layout, "room_a", {{0, 0}, {4, 6}});
  appendRoom(layout, "room_b", {{4, 0}, {8, 3}});

  cr::CreativeWorldLayoutOpening opening;
  opening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  opening.roomIndex = 0U;
  opening.roomEdge = cr::CreativeWorldLayoutRoomEdge::East;
  opening.stableKey = "door";
  opening.name = "Door";
  opening.centerOffsetCells = 1.5;
  opening.widthCells = 1.0;
  layout.openings.push_back(opening);

  const cr::CreativeWorldLayoutRoomGraph graph =
      cr::buildCreativeWorldLayoutRoomGraph(layout);
  std::vector<std::size_t> uses(graph.edges.size(), 0U);
  for (const cr::CreativeWorldLayoutRoomBoundary& boundary : graph.boundaries) {
    if (boundary.topologyEdgeIndex < uses.size()) {
      ++uses[boundary.topologyEdgeIndex];
    }
  }
  const std::size_t shared = static_cast<std::size_t>(
      std::count(uses.begin(), uses.end(), 2U));

  const cr::CreativeWorldLayoutRoomGraphMaterializeResult materialized =
      cr::materializeCreativeWorldLayoutRoomGraph(layout);
  return expect(graph.accepted && !graph.sourceWasExplicit,
                "legacy rectangles normalize through the graph kernel") &&
         expect(graph.rooms.size() == 2U &&
                    graph.rooms[0].boundaryCount == 5U &&
                    graph.rooms[1].boundaryCount == 4U && shared == 1U,
                "a T junction splits one shared edge without duplicate walls") &&
         expect(materialized.accepted && materialized.changed &&
                    !materialized.edited.topologyVertices.empty() &&
                    materialized.edited.openings[0].roomTopologyEdgeIndex !=
                        cr::kInvalidCreativeWorldLayoutIndex &&
                    materialized.edited.openings[0].centerOffsetCells == 1.5,
                "materialization gives a legacy opening a direct stable host");
}

bool arbitraryOrthogonalRoomHasExactSurfaceDecomposition() {
  cr::CreativeWorldLayout layout = baseLayout();
  appendLRoomGraph(layout);
  const cr::CreativeWorldLayoutRoomGraph graph =
      cr::buildCreativeWorldLayoutRoomGraph(layout);
  const std::span<const cr::CreativeWorldLayoutRect> pieces =
      cr::creativeWorldLayoutRoomSurfaceRects(graph, 0U);

  return expect(graph.accepted && graph.sourceWasExplicit &&
                    graph.rooms[0].boundaryCount == 6U && pieces.size() == 2U,
                "an explicit L room is accepted and decomposed exactly") &&
         expect(cr::creativeWorldLayoutRoomContainsPoint(graph, 0U, {1, 5}) &&
                    cr::creativeWorldLayoutRoomContainsPoint(graph, 0U,
                                                             {5, 1}) &&
                    !cr::creativeWorldLayoutRoomContainsPoint(graph, 0U,
                                                              {5, 5}),
                "point containment honors the concave room notch") &&
         expect(cr::creativeWorldLayoutRoomContainsRect(
                    graph, 0U, {{0, 0}, {2, 6}}) &&
                    !cr::creativeWorldLayoutRoomContainsRect(
                        graph, 0U, {{1, 1}, {5, 5}}),
                "connector-sized rectangles must be covered by the room union");
}

bool malformedAndOverlappingTopologyFailsClosed() {
  cr::CreativeWorldLayout malformed = baseLayout();
  appendLRoomGraph(malformed);
  malformed.roomBoundaries.back().order = 9U;
  const cr::CreativeWorldLayoutRoomGraph badBoundary =
      cr::buildCreativeWorldLayoutRoomGraph(malformed);

  cr::CreativeWorldLayout overlap = baseLayout();
  appendRoom(overlap, "room_a", {{0, 0}, {5, 5}});
  appendRoom(overlap, "room_b", {{4, 4}, {8, 8}});
  const cr::CreativeWorldLayoutRoomGraph badOverlap =
      cr::buildCreativeWorldLayoutRoomGraph(overlap);

  cr::CreativeWorldLayout crossingOpening = baseLayout();
  appendRoom(crossingOpening, "room_a", {{0, 0}, {4, 6}});
  appendRoom(crossingOpening, "room_b", {{4, 0}, {8, 3}});
  cr::CreativeWorldLayoutOpening opening;
  opening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  opening.roomIndex = 0U;
  opening.roomEdge = cr::CreativeWorldLayoutRoomEdge::East;
  opening.stableKey = "crossing";
  opening.name = "Crossing";
  opening.centerOffsetCells = 3.0;
  opening.widthCells = 1.0;
  crossingOpening.openings.push_back(opening);
  const auto badOpening =
      cr::materializeCreativeWorldLayoutRoomGraph(crossingOpening);

  return expect(!badBoundary.accepted &&
                    badBoundary.status ==
                        cr::CreativeWorldLayoutRoomGraphStatus::InvalidBoundary,
                "non-contiguous boundary order fails closed") &&
         expect(!badOverlap.accepted &&
                    badOverlap.status ==
                        cr::CreativeWorldLayoutRoomGraphStatus::OverlappingRooms,
                "overlapping room interiors fail closed") &&
         expect(!badOpening.accepted &&
                    badOpening.status ==
                        cr::CreativeWorldLayoutRoomGraphStatus::OpeningHostInvalid,
                "an opening cannot straddle a required topology split");
}

bool fullCoordinateRangeDoesNotOverflowExtentValidation() {
  cr::CreativeWorldLayout layout = baseLayout();
  const std::int32_t minimum = std::numeric_limits<std::int32_t>::min();
  const std::int32_t maximum = std::numeric_limits<std::int32_t>::max();
  layout.buildings[0].rootFootprint = {{minimum, minimum},
                                       {maximum, maximum}};
  appendRoom(layout, "full_range", {{minimum, minimum}, {maximum, maximum}});

  const cr::CreativeWorldLayoutRoomGraph graph =
      cr::buildCreativeWorldLayoutRoomGraph(layout);
  return expect(graph.accepted && graph.roomBounds.size() == 1U &&
                    graph.roomBounds[0].minimum ==
                        layout.rooms[0].footprint.minimum &&
                    graph.roomBounds[0].maximum ==
                        layout.rooms[0].footprint.maximum,
                "room extent validation widens before subtracting int32 coordinates");
}

}  // namespace

int main() {
  const bool ok = legacyRectanglesNormalizeToSharedGraph() &&
                  arbitraryOrthogonalRoomHasExactSurfaceDecomposition() &&
                  malformedAndOverlappingTopologyFailsClosed() &&
                  fullCoordinateRangeDoesNotOverflowExtentValidation();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
