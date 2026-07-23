#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoomOperations.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool sameRect(cr::CreativeWorldLayoutRect lhs,
              cr::CreativeWorldLayoutRect rhs) noexcept {
  return lhs.minimum == rhs.minimum && lhs.maximum == rhs.maximum;
}

cr::CreativeWorldLayout oneRoomLayout() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "room_operations";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building";
  building.name = "Building";
  building.rootFootprint = {{0, 0}, {8, 6}};
  layout.buildings.push_back(building);
  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "ground";
  level.name = "Ground";
  layout.levels.push_back(level);
  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = 0U;
  room.levelIndex = 0U;
  room.stableKey = "great_room";
  room.name = "Great Room";
  room.footprint = {{0, 0}, {8, 6}};
  layout.rooms.push_back(room);
  return layout;
}

void appendOpening(cr::CreativeWorldLayout& layout,
                   cr::CreativeWorldLayoutRoomEdge edge,
                   double center,
                   double width,
                   std::string key) {
  cr::CreativeWorldLayoutOpening opening;
  opening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  opening.roomIndex = 0U;
  opening.roomEdge = edge;
  opening.kind = cr::CreativeBuildingOpeningKind::Door;
  opening.stableKey = std::move(key);
  opening.name = "Door";
  opening.centerOffsetCells = center;
  opening.widthCells = width;
  layout.openings.push_back(std::move(opening));
}

void replaceRoomWithExplicitPolygon(cr::CreativeWorldLayout& layout,
                                    const std::vector<cr::CreativeTerrainCoord2>& points) {
  layout.topologyVertices.clear();
  layout.topologyEdges.clear();
  layout.roomBoundaries.clear();
  cr::CreativeWorldLayoutRect bounds{points.front(), points.front()};
  for (std::size_t index = 0U; index < points.size(); ++index) {
    bounds.minimum.x = std::min(bounds.minimum.x, points[index].x);
    bounds.minimum.z = std::min(bounds.minimum.z, points[index].z);
    bounds.maximum.x = std::max(bounds.maximum.x, points[index].x);
    bounds.maximum.z = std::max(bounds.maximum.z, points[index].z);
    layout.topologyVertices.push_back(
        {0U, "vertex_" + std::to_string(index), points[index]});
  }
  layout.rooms[0].footprint = bounds;
  for (std::size_t index = 0U; index < points.size(); ++index) {
    const std::size_t next = (index + 1U) % points.size();
    const bool reversed = points[next].x < points[index].x ||
                          points[next].z < points[index].z;
    const std::size_t start = reversed ? next : index;
    const std::size_t end = reversed ? index : next;
    layout.topologyEdges.push_back(
        {0U, "edge_" + std::to_string(index), start, end, 0.25});
    layout.roomBoundaries.push_back({0U, index, index, reversed});
  }
}

bool splitCreatesTwoRoomsAndPreservesHostedGeometry() {
  cr::CreativeWorldLayout layout = oneRoomLayout();
  appendOpening(layout, cr::CreativeWorldLayoutRoomEdge::East, 3.0, 1.0,
                "east_door");
  const cr::CreativeWorldLayoutRoomGraph sourceGraph =
      cr::buildCreativeWorldLayoutRoomGraph(layout);
  const cr::CreativeWorldLayoutRoomOperationResult split =
      cr::splitCreativeWorldLayoutRoom(
          layout, {0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4,
                   "east_room", "East Room"});
  const cr::CreativeWorldLayoutRoomGraph graph =
      split.accepted
          ? cr::buildCreativeWorldLayoutRoomGraph(split.edited)
          : cr::CreativeWorldLayoutRoomGraph{};
  std::vector<std::size_t> edgeUses(graph.edges.size(), 0U);
  for (const cr::CreativeWorldLayoutRoomBoundary& boundary : graph.boundaries) {
    ++edgeUses[boundary.topologyEdgeIndex];
  }
  const std::size_t sharedEdgeCount = static_cast<std::size_t>(
      std::count(edgeUses.begin(), edgeUses.end(), 2U));
  const std::size_t preservedEdgeCount = static_cast<std::size_t>(
      std::count_if(split.sourceToEditedEdgeIndices.begin(),
                    split.sourceToEditedEdgeIndices.end(),
                    [](std::size_t index) {
                      return index != cr::kInvalidCreativeWorldLayoutIndex;
                    }));

  return expect(sourceGraph.accepted && split.accepted && split.changed,
                "rectangle split materializes and edits atomically") &&
         expect(split.edited.rooms.size() == 2U &&
                    split.primaryRoomIndex == 0U &&
                    split.secondaryRoomIndex == 1U &&
                    split.edited.rooms[0].stableKey == "great_room" &&
                    split.edited.rooms[1].stableKey == "east_room",
                "minimum side retains identity and maximum side is appended") &&
         expect(sameRect(split.edited.rooms[0].footprint,
                         {{0, 0}, {4, 6}}) &&
                    sameRect(split.edited.rooms[1].footprint,
                             {{4, 0}, {8, 6}}) &&
                    sharedEdgeCount == 1U,
                "split produces one relational shared partition") &&
         expect(split.edited.openings.size() == 1U &&
                    split.edited.openings[0].roomIndex == 1U &&
                    split.edited.openings[0].roomTopologyEdgeIndex <
                        split.edited.topologyEdges.size() &&
                    split.edited.openings[0].roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::East,
                "exterior door follows its geometric host into the new room") &&
         expect(split.sourceToEditedVertexIndices.size() ==
                        sourceGraph.vertices.size() &&
                    std::none_of(
                        split.sourceToEditedVertexIndices.begin(),
                        split.sourceToEditedVertexIndices.end(),
                        [](std::size_t index) {
                          return index == cr::kInvalidCreativeWorldLayoutIndex;
                        }) &&
                    preservedEdgeCount == 2U,
                "surviving exact vertices and unsplit exterior edges retain identity") &&
         expect(cr::expandCreativeWorldLayoutRooms(split.edited).accepted,
                "split topology reaches canonical wall expansion");
}

bool mergeRestoresOneRoomAndRemovesOnlyThePartition() {
  cr::CreativeWorldLayout layout = oneRoomLayout();
  appendOpening(layout, cr::CreativeWorldLayoutRoomEdge::East, 3.0, 1.0,
                "east_door");
  const cr::CreativeWorldLayoutRoomOperationResult split =
      cr::splitCreativeWorldLayoutRoom(
          layout, {0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4,
                   "east_room", "East Room"});
  const cr::CreativeWorldLayoutRoomGraph splitGraph =
      cr::buildCreativeWorldLayoutRoomGraph(split.edited);
  std::size_t sharedEdge = cr::kInvalidCreativeWorldLayoutIndex;
  std::vector<std::size_t> edgeUses(splitGraph.edges.size(), 0U);
  for (const auto& boundary : splitGraph.boundaries) {
    ++edgeUses[boundary.topologyEdgeIndex];
  }
  for (std::size_t index = 0U; index < edgeUses.size(); ++index) {
    if (edgeUses[index] == 2U) {
      sharedEdge = index;
    }
  }
  const cr::CreativeWorldLayoutRoomOperationResult merged =
      cr::mergeCreativeWorldLayoutRooms(split.edited, {0U, 1U});
  const cr::CreativeWorldLayoutRoomGraph mergedGraph =
      merged.accepted
          ? cr::buildCreativeWorldLayoutRoomGraph(merged.edited)
          : cr::CreativeWorldLayoutRoomGraph{};

  return expect(split.accepted && merged.accepted && merged.changed,
                "adjacent split rooms merge atomically") &&
         expect(merged.edited.rooms.size() == 1U &&
                    merged.edited.rooms[0].stableKey == "great_room" &&
                    sameRect(merged.edited.rooms[0].footprint,
                             {{0, 0}, {8, 6}}) &&
                    merged.sourceToEditedRoomIndices ==
                        std::vector<std::size_t>{0U, 0U},
                "primary metadata survives and both source rooms remap to it") &&
         expect(sharedEdge < merged.sourceToEditedEdgeIndices.size() &&
                    merged.sourceToEditedEdgeIndices[sharedEdge] ==
                        cr::kInvalidCreativeWorldLayoutIndex,
                "the removed partition is the only intentionally dead host") &&
         expect(merged.edited.openings[0].roomIndex == 0U &&
                    merged.edited.openings[0].roomTopologyEdgeIndex <
                        mergedGraph.edges.size(),
                "surviving exterior door rehosts on the merged room") &&
         expect(mergedGraph.accepted &&
                    cr::creativeWorldLayoutRoomSurfaceRects(mergedGraph, 0U)
                            .size() == 1U &&
                    mergedGraph.rooms[0].boundaryCount == 4U,
                "merged room resolves to one exact rectangle without stale split handles");
}

bool dependencyConflictsRejectWithoutPartialOutput() {
  cr::CreativeWorldLayout openingConflict = oneRoomLayout();
  appendOpening(openingConflict, cr::CreativeWorldLayoutRoomEdge::North, 4.0,
                2.0, "crossing_door");
  const cr::CreativeWorldLayoutRoomOperationResult crossing =
      cr::splitCreativeWorldLayoutRoom(
          openingConflict,
          {0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4, "east", "East"});

  cr::CreativeWorldLayout connectorConflict = oneRoomLayout();
  cr::CreativeWorldLayoutLevel upper = connectorConflict.levels[0];
  upper.stableKey = "upper";
  upper.name = "Upper";
  upper.floorTopLayer = 3.0;
  connectorConflict.levels.push_back(upper);
  cr::CreativeWorldLayoutRoom upperRoom = connectorConflict.rooms[0];
  upperRoom.levelIndex = 1U;
  upperRoom.stableKey = "upper_room";
  upperRoom.name = "Upper Room";
  connectorConflict.rooms.push_back(upperRoom);
  connectorConflict.verticalConnectors.push_back(
      {0U, 0U, 1U, cr::CreativeWorldLayoutVerticalConnectorKind::Stair,
       cr::CreativeWorldLayoutVerticalDirection::PositiveX, "stair", "Stair",
       {{3, 2}, {7, 4}}});
  const cr::CreativeWorldLayoutRoomOperationResult crossingConnector =
      cr::splitCreativeWorldLayoutRoom(
          connectorConflict,
          {0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4, "east", "East"});

  return expect(!crossing.accepted && !crossing.changed &&
                    crossing.status ==
                        cr::CreativeWorldLayoutRoomOperationStatus::
                            OpeningConflict &&
                    crossing.failedOpeningIndex == 0U &&
                    crossing.edited.rooms.empty(),
                "opening spanning a new edge junction rejects atomically") &&
         expect(!crossingConnector.accepted && !crossingConnector.changed &&
                    crossingConnector.status ==
                        cr::CreativeWorldLayoutRoomOperationStatus::
                            ConnectorConflict &&
                    crossingConnector.failedConnectorIndex == 0U &&
                    crossingConnector.edited.rooms.empty(),
                "connector crossing a split rejects atomically");
}

bool dependenciesRemapWhenTheyFitOneSide() {
  cr::CreativeWorldLayout layout = oneRoomLayout();
  cr::CreativeWorldLayoutLevel upper = layout.levels[0];
  upper.stableKey = "upper";
  upper.name = "Upper";
  upper.floorTopLayer = 3.0;
  layout.levels.push_back(upper);
  cr::CreativeWorldLayoutRoom upperRoom = layout.rooms[0];
  upperRoom.levelIndex = 1U;
  upperRoom.stableKey = "upper_room";
  upperRoom.name = "Upper Room";
  layout.rooms.push_back(upperRoom);
  layout.verticalConnectors.push_back(
      {0U, 0U, 1U, cr::CreativeWorldLayoutVerticalConnectorKind::Stair,
       cr::CreativeWorldLayoutVerticalDirection::PositiveZ, "stair", "Stair",
       {{5, 1}, {7, 5}}});

  const cr::CreativeWorldLayoutRoomOperationResult split =
      cr::splitCreativeWorldLayoutRoom(
          layout,
          {0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4, "east", "East"});
  return expect(split.accepted && split.edited.rooms.size() == 3U,
                "split accepts a connector wholly on one side") &&
         expect(split.edited.verticalConnectors.size() == 1U &&
                    split.edited.verticalConnectors[0].lowerRoomIndex == 2U &&
                    split.edited.verticalConnectors[0].upperRoomIndex == 1U,
                "connector lower owner follows the appended split room");
}

bool disconnectedCutsAndUnrelatedMergesReject() {
  cr::CreativeWorldLayout uRoom = oneRoomLayout();
  replaceRoomWithExplicitPolygon(
      uRoom, {{0, 0}, {6, 0}, {6, 6}, {4, 6},
              {4, 2}, {2, 2}, {2, 6}, {0, 6}});
  const cr::CreativeWorldLayoutRoomOperationResult disconnected =
      cr::splitCreativeWorldLayoutRoom(
          uRoom,
          {0U, cr::CreativeWorldLayoutRoomSplitAxis::Z, 3, "north", "North"});

  cr::CreativeWorldLayout separate = oneRoomLayout();
  cr::CreativeWorldLayoutRoom second = separate.rooms[0];
  second.stableKey = "separate";
  second.name = "Separate";
  second.footprint = {{10, 0}, {14, 4}};
  separate.rooms.push_back(second);
  const cr::CreativeWorldLayoutRoomOperationResult nonAdjacent =
      cr::mergeCreativeWorldLayoutRooms(separate, {0U, 1U});

  return expect(!disconnected.accepted &&
                    disconnected.status ==
                        cr::CreativeWorldLayoutRoomOperationStatus::
                            DisconnectedResult,
                "a cut that creates two islands on one side fails closed") &&
         expect(!nonAdjacent.accepted &&
                    nonAdjacent.status ==
                        cr::CreativeWorldLayoutRoomOperationStatus::
                            RoomsNotAdjacent,
                "rooms without a shared topology edge cannot merge");
}

bool mergeRefusesToEraseAnOpeningBearingPartition() {
  cr::CreativeWorldLayout layout = oneRoomLayout();
  const cr::CreativeWorldLayoutRoomOperationResult split =
      cr::splitCreativeWorldLayoutRoom(
          layout,
          {0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4, "east", "East"});
  cr::CreativeWorldLayout withDoor = split.edited;
  const cr::CreativeWorldLayoutRoomGraph graph =
      cr::buildCreativeWorldLayoutRoomGraph(withDoor);
  std::vector<std::size_t> uses(graph.edges.size(), 0U);
  for (const auto& boundary : graph.boundaries) {
    ++uses[boundary.topologyEdgeIndex];
  }
  const auto shared = std::find(uses.begin(), uses.end(), 2U);
  if (shared == uses.end()) {
    return expect(false, "split exposes a shared partition for merge test");
  }
  cr::CreativeWorldLayoutOpening door;
  door.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  door.roomIndex = 0U;
  door.roomEdge = cr::CreativeWorldLayoutRoomEdge::East;
  door.roomTopologyEdgeIndex =
      static_cast<std::size_t>(std::distance(uses.begin(), shared));
  door.stableKey = "partition_door";
  door.name = "Partition Door";
  door.centerOffsetCells = 3.0;
  withDoor.openings.push_back(door);

  const cr::CreativeWorldLayoutRoomOperationResult merged =
      cr::mergeCreativeWorldLayoutRooms(withDoor, {0U, 1U});
  return expect(!merged.accepted && !merged.changed &&
                    merged.status ==
                        cr::CreativeWorldLayoutRoomOperationStatus::
                            OpeningConflict &&
                    merged.failedOpeningIndex == 0U,
                "merge reports the partition opening instead of deleting it");
}

std::size_t sharedEdgeIndex(
    const cr::CreativeWorldLayoutRoomGraph& graph) {
  std::vector<std::size_t> uses(graph.edges.size(), 0U);
  for (const cr::CreativeWorldLayoutRoomBoundary& boundary :
       graph.boundaries) {
    ++uses[boundary.topologyEdgeIndex];
  }
  const auto shared = std::find(uses.begin(), uses.end(), 2U);
  return shared == uses.end()
             ? cr::kInvalidCreativeWorldLayoutIndex
             : static_cast<std::size_t>(std::distance(uses.begin(), shared));
}

std::size_t horizontalEdgeIndex(
    const cr::CreativeWorldLayoutRoomGraph& graph, std::size_t roomIndex,
    std::int32_t z) {
  for (const cr::CreativeWorldLayoutRoomBoundary& boundary :
       cr::creativeWorldLayoutRoomBoundaries(graph, roomIndex)) {
    const cr::CreativeWorldLayoutTopologyEdge& edge =
        graph.edges[boundary.topologyEdgeIndex];
    const cr::CreativeTerrainCoord2 start =
        graph.vertices[edge.startVertexIndex].position;
    const cr::CreativeTerrainCoord2 end =
        graph.vertices[edge.endVertexIndex].position;
    if (start.z == z && end.z == z) {
      return boundary.topologyEdgeIndex;
    }
  }
  return cr::kInvalidCreativeWorldLayoutIndex;
}

std::size_t vertexIndexAt(const cr::CreativeWorldLayoutRoomGraph& graph,
                          cr::CreativeTerrainCoord2 point) {
  const auto found = std::find_if(
      graph.vertices.begin(), graph.vertices.end(),
      [&](const cr::CreativeWorldLayoutTopologyVertex& vertex) {
        return vertex.position == point;
      });
  return found == graph.vertices.end()
             ? cr::kInvalidCreativeWorldLayoutIndex
             : static_cast<std::size_t>(
                   std::distance(graph.vertices.begin(), found));
}

bool sharedBoundaryMovePreservesRelationalOwnership() {
  const cr::CreativeWorldLayoutRoomOperationResult split =
      cr::splitCreativeWorldLayoutRoom(
          oneRoomLayout(),
          {0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4, "east", "East"});
  cr::CreativeWorldLayout layout = split.edited;
  const cr::CreativeWorldLayoutRoomGraph sourceGraph =
      cr::buildCreativeWorldLayoutRoomGraph(layout);
  const std::size_t sharedEdge = sharedEdgeIndex(sourceGraph);
  const std::size_t eastNorthEdge = horizontalEdgeIndex(sourceGraph, 1U, 0);
  if (sharedEdge >= sourceGraph.edges.size() ||
      eastNorthEdge >= sourceGraph.edges.size()) {
    return expect(false, "boundary move fixture exposes required edges");
  }
  const std::string sharedStableKey = sourceGraph.edges[sharedEdge].stableKey;
  const std::string sharedStartVertexKey =
      sourceGraph.vertices[sourceGraph.edges[sharedEdge].startVertexIndex]
          .stableKey;
  const std::string sharedEndVertexKey =
      sourceGraph.vertices[sourceGraph.edges[sharedEdge].endVertexIndex]
          .stableKey;
  const std::string adjacentEdgeStableKey =
      sourceGraph.edges[eastNorthEdge].stableKey;
  layout.topologyEdges[sharedEdge].wallHeightCells = 9U;
  layout.topologyEdges[sharedEdge].profile =
      cr::CreativeWorldLayoutWallProfile::Interior;
  layout.topologyEdges[sharedEdge].material =
      cr::CreativeStructuralMaterial::Timber;
  layout.topologyEdges[eastNorthEdge].wallHeightCells = 7U;
  layout.topologyEdges[eastNorthEdge].profile =
      cr::CreativeWorldLayoutWallProfile::Exterior;
  layout.topologyEdges[eastNorthEdge].material =
      cr::CreativeStructuralMaterial::Plaster;

  cr::CreativeWorldLayoutOpening partitionDoor;
  partitionDoor.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  partitionDoor.roomIndex = 0U;
  partitionDoor.roomEdge = cr::CreativeWorldLayoutRoomEdge::East;
  partitionDoor.roomTopologyEdgeIndex = sharedEdge;
  partitionDoor.kind = cr::CreativeBuildingOpeningKind::Door;
  partitionDoor.stableKey = "partition_door";
  partitionDoor.name = "Partition Door";
  partitionDoor.centerOffsetCells = 3.0;
  partitionDoor.widthCells = 1.0;
  layout.openings.push_back(partitionDoor);

  cr::CreativeWorldLayoutOpening exteriorDoor = partitionDoor;
  exteriorDoor.roomIndex = 1U;
  exteriorDoor.roomEdge = cr::CreativeWorldLayoutRoomEdge::North;
  exteriorDoor.roomTopologyEdgeIndex = eastNorthEdge;
  exteriorDoor.stableKey = "north_door";
  exteriorDoor.name = "North Door";
  exteriorDoor.centerOffsetCells = 2.0;
  layout.openings.push_back(exteriorDoor);

  const cr::CreativeWorldLayoutRoomOperationResult moved =
      cr::moveCreativeWorldLayoutRoomBoundary(layout, {sharedEdge, 5});
  const cr::CreativeWorldLayoutRoomGraph graph =
      moved.accepted
          ? cr::buildCreativeWorldLayoutRoomGraph(moved.edited)
          : cr::CreativeWorldLayoutRoomGraph{};
  const std::size_t movedSharedEdge = sharedEdgeIndex(graph);
  const cr::CreativeWorldLayoutTopologyEdge* movedEdge =
      movedSharedEdge < graph.edges.size() ? &graph.edges[movedSharedEdge]
                                           : nullptr;
  const bool edgeAtFive =
      movedEdge != nullptr &&
      graph.vertices[movedEdge->startVertexIndex].position.x == 5 &&
      graph.vertices[movedEdge->endVertexIndex].position.x == 5;
  const std::size_t movedAdjacentEdge =
      moved.sourceToEditedEdgeIndices[eastNorthEdge];

  return expect(moved.accepted && moved.changed && graph.accepted &&
                    sameRect(moved.edited.rooms[0].footprint,
                             {{0, 0}, {5, 6}}) &&
                    sameRect(moved.edited.rooms[1].footprint,
                             {{5, 0}, {8, 6}}),
                "shared edge move reshapes both owning rooms") &&
         expect(edgeAtFive && movedEdge->stableKey == sharedStableKey &&
                    movedEdge->wallHeightCells == 9U &&
                    movedEdge->profile ==
                        cr::CreativeWorldLayoutWallProfile::Interior &&
                    movedEdge->material ==
                        cr::CreativeStructuralMaterial::Timber &&
                    moved.sourceToEditedEdgeIndices[sharedEdge] ==
                        movedSharedEdge,
                "moved physical wall preserves identity, semantics, and remap") &&
         expect(graph.vertices[movedEdge->startVertexIndex].stableKey ==
                        sharedStartVertexKey &&
                    graph.vertices[movedEdge->endVertexIndex].stableKey ==
                        sharedEndVertexKey &&
                    movedAdjacentEdge < graph.edges.size() &&
                    graph.edges[movedAdjacentEdge].stableKey ==
                        adjacentEdgeStableKey &&
                    graph.edges[movedAdjacentEdge].wallHeightCells == 7U &&
                    graph.edges[movedAdjacentEdge].profile ==
                        cr::CreativeWorldLayoutWallProfile::Exterior &&
                    graph.edges[movedAdjacentEdge].material ==
                        cr::CreativeStructuralMaterial::Plaster,
                "moved endpoints and resized incident walls preserve identity and semantics") &&
         expect(moved.edited.openings.size() == 2U &&
                    moved.edited.openings[0].roomTopologyEdgeIndex ==
                        movedSharedEdge &&
                    moved.edited.openings[0].centerOffsetCells == 3.0 &&
                    moved.edited.openings[1].centerOffsetCells == 1.0,
                "hosted and adjacent-wall openings retain world placement") &&
         expect(sameRect(moved.edited.buildings[0].rootFootprint,
                         {{0, 0}, {8, 6}}),
                "shared partition move refreshes but does not shrink the envelope");
}

bool invalidBoundaryMovesRejectAtomically() {
  cr::CreativeWorldLayout layout = oneRoomLayout();
  cr::CreativeWorldLayoutLevel upper = layout.levels[0];
  upper.stableKey = "upper";
  upper.floorTopLayer = 3.0;
  layout.levels.push_back(upper);
  cr::CreativeWorldLayoutRoom upperRoom = layout.rooms[0];
  upperRoom.levelIndex = 1U;
  upperRoom.stableKey = "upper_room";
  layout.rooms.push_back(upperRoom);
  const cr::CreativeWorldLayoutRoomOperationResult split =
      cr::splitCreativeWorldLayoutRoom(
          layout,
          {0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4, "east", "East"});
  cr::CreativeWorldLayout withConnector = split.edited;
  const cr::CreativeWorldLayoutRoomGraph graph =
      cr::buildCreativeWorldLayoutRoomGraph(withConnector);
  const std::size_t sharedEdge = sharedEdgeIndex(graph);
  withConnector.verticalConnectors.push_back(
      {0U, 0U, 1U, cr::CreativeWorldLayoutVerticalConnectorKind::Stair,
       cr::CreativeWorldLayoutVerticalDirection::PositiveZ, "stair", "Stair",
       {{3, 1}, {4, 5}}});

  const cr::CreativeWorldLayoutRoomOperationResult noChange =
      cr::moveCreativeWorldLayoutRoomBoundary(withConnector, {sharedEdge, 4});
  const cr::CreativeWorldLayoutRoomOperationResult collapsed =
      cr::moveCreativeWorldLayoutRoomBoundary(withConnector, {sharedEdge, 0});
  const cr::CreativeWorldLayoutRoomOperationResult connectorConflict =
      cr::moveCreativeWorldLayoutRoomBoundary(withConnector, {sharedEdge, 2});

  return expect(noChange.accepted && !noChange.changed &&
                    noChange.status ==
                        cr::CreativeWorldLayoutRoomOperationStatus::NoChange,
                "stationary boundary move is an accepted no-op") &&
         expect(!collapsed.accepted && !collapsed.changed &&
                    collapsed.edited.rooms.empty(),
                "boundary collapse rejects without partial output") &&
         expect(!connectorConflict.accepted && !connectorConflict.changed &&
                    connectorConflict.status ==
                        cr::CreativeWorldLayoutRoomOperationStatus::
                            ConnectorConflict &&
                    connectorConflict.failedConnectorIndex == 0U &&
                    connectorConflict.edited.rooms.empty(),
                "boundary move cannot strand a vertical connector");
}

bool cornerMoveComposesBothAxesWithoutLosingIdentity() {
  const cr::CreativeWorldLayoutRoomGraphMaterializeResult materialized =
      cr::materializeCreativeWorldLayoutRoomGraph(oneRoomLayout());
  cr::CreativeWorldLayout layout = materialized.edited;
  const cr::CreativeWorldLayoutRoomGraph sourceGraph =
      cr::buildCreativeWorldLayoutRoomGraph(layout);
  const std::size_t corner = vertexIndexAt(sourceGraph, {0, 0});
  const std::size_t northEdge = horizontalEdgeIndex(sourceGraph, 0U, 0);
  if (corner >= sourceGraph.vertices.size() ||
      northEdge >= sourceGraph.edges.size()) {
    return expect(false, "corner fixture exposes canonical handles");
  }
  std::size_t westEdge = cr::kInvalidCreativeWorldLayoutIndex;
  for (const cr::CreativeWorldLayoutRoomBoundary& boundary :
       cr::creativeWorldLayoutRoomBoundaries(sourceGraph, 0U)) {
    const cr::CreativeWorldLayoutTopologyEdge& edge =
        sourceGraph.edges[boundary.topologyEdgeIndex];
    const cr::CreativeTerrainCoord2 start =
        sourceGraph.vertices[edge.startVertexIndex].position;
    if (start.x == 0 &&
        sourceGraph.vertices[edge.endVertexIndex].position.x == 0) {
      westEdge = boundary.topologyEdgeIndex;
    }
  }
  if (westEdge >= sourceGraph.edges.size()) {
    return expect(false, "corner fixture exposes its vertical wall");
  }
  const std::string cornerKey = sourceGraph.vertices[corner].stableKey;
  const std::string northKey = sourceGraph.edges[northEdge].stableKey;
  const std::string westKey = sourceGraph.edges[westEdge].stableKey;

  cr::CreativeWorldLayoutOpening door;
  door.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  door.roomIndex = 0U;
  door.roomEdge = cr::CreativeWorldLayoutRoomEdge::North;
  door.roomTopologyEdgeIndex = northEdge;
  door.stableKey = "north_door";
  door.name = "North Door";
  door.centerOffsetCells = 4.0;
  door.widthCells = 1.0;
  layout.openings.push_back(door);

  const cr::CreativeWorldLayoutRoomOperationResult moved =
      cr::moveCreativeWorldLayoutRoomCorner(layout, {0U, corner, {1, 1}});
  const cr::CreativeWorldLayoutRoomGraph graph =
      moved.accepted
          ? cr::buildCreativeWorldLayoutRoomGraph(moved.edited)
          : cr::CreativeWorldLayoutRoomGraph{};
  const std::size_t movedCorner =
      corner < moved.sourceToEditedVertexIndices.size()
          ? moved.sourceToEditedVertexIndices[corner]
          : cr::kInvalidCreativeWorldLayoutIndex;
  const std::size_t movedNorth =
      northEdge < moved.sourceToEditedEdgeIndices.size()
          ? moved.sourceToEditedEdgeIndices[northEdge]
          : cr::kInvalidCreativeWorldLayoutIndex;
  const std::size_t movedWest =
      westEdge < moved.sourceToEditedEdgeIndices.size()
          ? moved.sourceToEditedEdgeIndices[westEdge]
          : cr::kInvalidCreativeWorldLayoutIndex;
  const cr::CreativeWorldLayoutRoomOperationResult collapsed =
      cr::moveCreativeWorldLayoutRoomCorner(layout, {0U, corner, {8, 6}});

  return expect(moved.accepted && moved.changed && graph.accepted &&
                    sameRect(moved.edited.rooms[0].footprint,
                             {{1, 1}, {8, 6}}),
                "corner move composes horizontal and vertical boundary edits") &&
         expect(movedCorner < graph.vertices.size() &&
                    graph.vertices[movedCorner].position ==
                        cr::CreativeTerrainCoord2{1, 1} &&
                    graph.vertices[movedCorner].stableKey == cornerKey &&
                    movedNorth < graph.edges.size() &&
                    graph.edges[movedNorth].stableKey == northKey &&
                    movedWest < graph.edges.size() &&
                    graph.edges[movedWest].stableKey == westKey,
                "corner and incident wall identities survive both axis moves") &&
         expect(moved.edited.openings.size() == 1U &&
                    moved.edited.openings[0].roomTopologyEdgeIndex ==
                        movedNorth &&
                    moved.edited.openings[0].centerOffsetCells == 3.0,
                "corner move preserves the hosted door world position") &&
         expect(!collapsed.accepted && !collapsed.changed &&
                    collapsed.edited.rooms.empty(),
                "collapsing corner move rejects atomically");
}

bool edgeSettingsOwnExactLengthThicknessAndAnchor() {
  cr::CreativeWorldLayout layout = oneRoomLayout();
  appendOpening(layout, cr::CreativeWorldLayoutRoomEdge::North, 4.0, 1.0,
                "north_door");
  const cr::CreativeWorldLayoutRoomGraph sourceGraph =
      cr::buildCreativeWorldLayoutRoomGraph(layout);
  const std::size_t northEdge = horizontalEdgeIndex(sourceGraph, 0U, 0);
  if (northEdge >= sourceGraph.edges.size()) {
    return expect(false, "edge-settings fixture exposes its north wall");
  }
  const std::string stableKey = sourceGraph.edges[northEdge].stableKey;

  const cr::CreativeWorldLayoutRoomOperationResult extended =
      cr::setCreativeWorldLayoutRoomEdgeSettings(
          layout,
          {northEdge, cr::CreativeWorldLayoutRoomEdgeAnchor::Start, 10U,
           0.5, 8U, cr::CreativeWorldLayoutWallProfile::Interior,
           cr::CreativeStructuralMaterial::Stone,
           cr::CreativeWorldLayoutWallJoinStyle::Square});
  const cr::CreativeWorldLayoutRoomGraph extendedGraph =
      extended.accepted
          ? cr::buildCreativeWorldLayoutRoomGraph(extended.edited)
          : cr::CreativeWorldLayoutRoomGraph{};
  const std::size_t extendedNorth =
      northEdge < extended.sourceToEditedEdgeIndices.size()
          ? extended.sourceToEditedEdgeIndices[northEdge]
          : cr::kInvalidCreativeWorldLayoutIndex;
  const bool exactExtendedEdge =
      extendedNorth < extendedGraph.edges.size() &&
      extendedGraph.edges[extendedNorth].stableKey == stableKey &&
      extendedGraph.edges[extendedNorth].wallThicknessCells == 0.5 &&
      extendedGraph.edges[extendedNorth].wallHeightCells == 8U &&
      extendedGraph.edges[extendedNorth].profile ==
          cr::CreativeWorldLayoutWallProfile::Interior &&
      extendedGraph.edges[extendedNorth].material ==
          cr::CreativeStructuralMaterial::Stone &&
      extendedGraph.edges[extendedNorth].joinStyle ==
          cr::CreativeWorldLayoutWallJoinStyle::Square &&
      extendedGraph.vertices[extendedGraph.edges[extendedNorth]
                                 .startVertexIndex]
              .position == cr::CreativeTerrainCoord2{0, 0} &&
      extendedGraph.vertices[extendedGraph.edges[extendedNorth].endVertexIndex]
              .position == cr::CreativeTerrainCoord2{10, 0};

  const cr::CreativeWorldLayoutRoomOperationResult anchoredEnd =
      cr::setCreativeWorldLayoutRoomEdgeSettings(
          layout, {northEdge, cr::CreativeWorldLayoutRoomEdgeAnchor::End, 6U,
                   0.25});
  const cr::CreativeWorldLayoutRoomGraph anchoredGraph =
      anchoredEnd.accepted
          ? cr::buildCreativeWorldLayoutRoomGraph(anchoredEnd.edited)
          : cr::CreativeWorldLayoutRoomGraph{};
  const std::size_t anchoredNorth =
      northEdge < anchoredEnd.sourceToEditedEdgeIndices.size()
          ? anchoredEnd.sourceToEditedEdgeIndices[northEdge]
          : cr::kInvalidCreativeWorldLayoutIndex;
  const bool exactAnchoredEdge =
      anchoredNorth < anchoredGraph.edges.size() &&
      anchoredGraph.vertices[anchoredGraph.edges[anchoredNorth]
                                 .startVertexIndex]
              .position == cr::CreativeTerrainCoord2{2, 0} &&
      anchoredGraph.vertices[anchoredGraph.edges[anchoredNorth].endVertexIndex]
              .position == cr::CreativeTerrainCoord2{8, 0};

  const cr::CreativeWorldLayoutRoomOperationResult overflow =
      cr::setCreativeWorldLayoutRoomEdgeSettings(
          layout,
          {northEdge, cr::CreativeWorldLayoutRoomEdgeAnchor::Start,
           std::numeric_limits<std::uint32_t>::max(), 0.25});

  return expect(extended.accepted && extended.changed &&
                    extendedGraph.accepted && exactExtendedEdge &&
                    sameRect(extended.edited.rooms[0].footprint,
                             {{0, 0}, {10, 6}}),
                "edge settings apply exact geometry and wall semantics atomically") &&
         expect(extended.edited.openings.size() == 1U &&
                    extended.edited.openings[0].roomTopologyEdgeIndex ==
                        extendedNorth &&
                    extended.edited.openings[0].centerOffsetCells == 4.0,
                "edge settings retain hosted opening placement") &&
         expect(anchoredEnd.accepted && exactAnchoredEdge &&
                    anchoredEnd.edited.openings[0].centerOffsetCells == 2.0,
                "fixed endpoint determines which corner moves") &&
         expect(!overflow.accepted && !overflow.changed &&
                    overflow.edited.rooms.empty(),
                "unrepresentable edge length rejects without partial output");
}

}  // namespace

int main() {
  const bool ok = splitCreatesTwoRoomsAndPreservesHostedGeometry() &&
                  mergeRestoresOneRoomAndRemovesOnlyThePartition() &&
                  dependencyConflictsRejectWithoutPartialOutput() &&
                  dependenciesRemapWhenTheyFitOneSide() &&
                  disconnectedCutsAndUnrelatedMergesReject() &&
                  mergeRefusesToEraseAnOpeningBearingPartition() &&
                  sharedBoundaryMovePreservesRelationalOwnership() &&
                  invalidBoundaryMovesRejectAtomically() &&
                  cornerMoveComposesBothAxesWithoutLosingIdentity() &&
                  edgeSettingsOwnExactLengthThicknessAndAnchor();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "creative_world_layout_room_operations_tests: PASS\n";
  return EXIT_SUCCESS;
}
