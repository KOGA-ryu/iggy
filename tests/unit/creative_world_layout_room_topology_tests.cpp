#include "app/iggy3d/creative/world/WorldLayoutRoomTopology.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs) {
  return std::abs(lhs - rhs) <= 1.0e-9;
}

cr::CreativeWorldLayout baseLayout() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "room_topology";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building_0";
  building.name = "Building";
  building.rootFootprint = {{0, 0}, {8, 8}};
  layout.buildings.push_back(building);

  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "level_0";
  level.name = "Ground";
  layout.levels.push_back(level);
  return layout;
}

void appendRoom(cr::CreativeWorldLayout& layout, std::string stableKey,
                cr::CreativeWorldLayoutRect footprint) {
  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = 0U;
  room.levelIndex = 0U;
  room.stableKey = std::move(stableKey);
  room.name = "Room " + std::to_string(layout.rooms.size() + 1U);
  room.footprint = footprint;
  layout.rooms.push_back(std::move(room));
}

void appendOpening(cr::CreativeWorldLayout& layout, std::size_t roomIndex,
                   cr::CreativeWorldLayoutRoomEdge edge,
                   cr::CreativeBuildingOpeningKind kind, double center,
                   double width) {
  cr::CreativeWorldLayoutOpening opening;
  opening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  opening.roomIndex = roomIndex;
  opening.roomEdge = edge;
  opening.kind = kind;
  opening.stableKey = "opening_" + std::to_string(layout.openings.size());
  opening.name = "Opening";
  opening.centerOffsetCells = center;
  opening.widthCells = width;
  opening.cutoutHeightCells = 2.0;
  layout.openings.push_back(std::move(opening));
}

bool oneBoundaryMovesEveryOppositeOwner() {
  cr::CreativeWorldLayout layout = baseLayout();
  appendRoom(layout, "room_a", {{0, 0}, {4, 8}});
  appendRoom(layout, "room_b", {{4, 0}, {8, 4}});
  appendRoom(layout, "room_c", {{4, 4}, {8, 8}});
  appendOpening(layout, 0U, cr::CreativeWorldLayoutRoomEdge::East,
                cr::CreativeBuildingOpeningKind::Door, 2.0, 1.0);
  appendOpening(layout, 1U, cr::CreativeWorldLayoutRoomEdge::North,
                cr::CreativeBuildingOpeningKind::Window, 2.0, 1.0);

  const cr::CreativeWorldLayoutRoomFootprintEditResult result =
      cr::editCreativeWorldLayoutRoomFootprint(
          layout, {0U, {{0, 0}, {5, 8}}});
  const auto shared = result.accepted
                          ? cr::inspectCreativeWorldLayoutSharedRoomEdges(
                                result.edited)
                          : std::vector<
                                cr::CreativeWorldLayoutSharedRoomEdgeSpan>{};

  return expect(result.accepted && result.changed &&
                    result.status ==
                        cr::CreativeWorldLayoutRoomFootprintEditStatus::Ready,
                "shared boundary edit is accepted") &&
         expect(result.roomChanges.size() == 3U &&
                    result.roomChanges[0].roomIndex == 0U &&
                    result.roomChanges[1].roomIndex == 1U &&
                    result.roomChanges[2].roomIndex == 2U,
                "one-to-many room changes remain source ordered") &&
         expect(result.edited.rooms[0].footprint.maximum.x == 5 &&
                    result.edited.rooms[1].footprint.minimum.x == 5 &&
                    result.edited.rooms[2].footprint.minimum.x == 5 &&
                    shared.size() == 3U,
                "every opposite owner follows the shared wall") &&
         expect(result.adjustedOpeningCount == 1U &&
                    near(result.edited.openings[0].centerOffsetCells, 2.0) &&
                    near(result.edited.openings[1].centerOffsetCells, 1.0),
                "openings retain their along-wall world coordinate") &&
         expect(result.edited.buildings[0].rootFootprint.minimum ==
                        cr::CreativeTerrainCoord2{0, 0} &&
                    result.edited.buildings[0].rootFootprint.maximum ==
                        cr::CreativeTerrainCoord2{8, 8},
                "building extent remains the union of edited rooms");
}

bool isolatedMoveRemainsRelativeButSharedMoveFailsClosed() {
  cr::CreativeWorldLayout isolated = baseLayout();
  isolated.buildings[0].rootFootprint = {{0, 0}, {4, 4}};
  appendRoom(isolated, "room_a", {{0, 0}, {4, 4}});
  appendOpening(isolated, 0U, cr::CreativeWorldLayoutRoomEdge::North,
                cr::CreativeBuildingOpeningKind::Door, 2.0, 1.0);
  const auto moved = cr::editCreativeWorldLayoutRoomFootprint(
      isolated, {0U, {{3, 2}, {7, 6}}});

  cr::CreativeWorldLayout joined = baseLayout();
  joined.buildings[0].rootFootprint = {{0, 0}, {8, 4}};
  appendRoom(joined, "room_a", {{0, 0}, {4, 4}});
  appendRoom(joined, "room_b", {{4, 0}, {8, 4}});
  const auto rejected = cr::editCreativeWorldLayoutRoomFootprint(
      joined, {0U, {{1, 0}, {5, 4}}});

  return expect(moved.accepted && moved.changed &&
                    moved.edited.rooms[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{3, 2} &&
                    near(moved.edited.openings[0].centerOffsetCells, 2.0) &&
                    moved.adjustedOpeningCount == 0U &&
                    moved.edited.buildings[0].rootFootprint.minimum ==
                        cr::CreativeTerrainCoord2{3, 2} &&
                    moved.edited.buildings[0].rootFootprint.maximum ==
                        cr::CreativeTerrainCoord2{7, 6},
                "isolated room movement carries its openings and extent") &&
         expect(!rejected.accepted && !rejected.changed &&
                    rejected.status ==
                        cr::CreativeWorldLayoutRoomFootprintEditStatus::
                            SharedRoomMoveUnsupported &&
                    rejected.edited.rooms.empty() &&
                    joined.rooms[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{0, 0},
                "shared room movement cannot tear an adjoining plan");
}

bool invalidResultsAreAtomic() {
  cr::CreativeWorldLayout collapse = baseLayout();
  collapse.buildings[0].rootFootprint = {{0, 0}, {5, 4}};
  appendRoom(collapse, "room_a", {{0, 0}, {4, 4}});
  appendRoom(collapse, "room_b", {{4, 0}, {5, 4}});
  const auto collapsed = cr::editCreativeWorldLayoutRoomFootprint(
      collapse, {0U, {{0, 0}, {5, 4}}});

  cr::CreativeWorldLayout clipped = baseLayout();
  clipped.buildings[0].rootFootprint = {{0, 0}, {6, 4}};
  appendRoom(clipped, "room_a", {{0, 0}, {6, 4}});
  appendOpening(clipped, 0U, cr::CreativeWorldLayoutRoomEdge::North,
                cr::CreativeBuildingOpeningKind::Door, 5.25, 0.5);
  const auto opening = cr::editCreativeWorldLayoutRoomFootprint(
      clipped, {0U, {{0, 0}, {5, 4}}});

  cr::CreativeWorldLayout interior = baseLayout();
  appendRoom(interior, "room_a", {{0, 0}, {4, 4}});
  appendRoom(interior, "room_b", {{5, 0}, {8, 4}});
  appendOpening(interior, 0U, cr::CreativeWorldLayoutRoomEdge::East,
                cr::CreativeBuildingOpeningKind::Window, 2.0, 1.0);
  const auto window = cr::editCreativeWorldLayoutRoomFootprint(
      interior, {0U, {{0, 0}, {5, 4}}});

  return expect(!collapsed.accepted &&
                    collapsed.status ==
                        cr::CreativeWorldLayoutRoomFootprintEditStatus::
                            InvalidFootprint &&
                    collapsed.failedRoomIndex == 1U,
                "shared edit rejects a collapsed opposite room") &&
         expect(!opening.accepted &&
                    opening.status ==
                        cr::CreativeWorldLayoutRoomFootprintEditStatus::
                            OpeningDoesNotFit &&
                    opening.failedOpeningIndex == 0U,
                "resize rejects an opening outside the shortened wall") &&
         expect(!window.accepted &&
                    window.status ==
                        cr::CreativeWorldLayoutRoomFootprintEditStatus::
                            InteriorWindow,
                "new adjacency cannot silently internalize a window") &&
         expect(collapse.rooms[0].footprint.maximum.x == 4 &&
                    clipped.openings[0].centerOffsetCells == 5.25 &&
                    interior.rooms[0].footprint.maximum.x == 4,
                "all rejected edits leave source truth untouched");
}

bool noOpRepairsThenStabilizesDerivedBuildingExtent() {
  cr::CreativeWorldLayout layout = baseLayout();
  layout.buildings[0].rootFootprint = {{0, 0}, {1, 1}};
  appendRoom(layout, "room_a", {{-2, -1}, {4, 5}});
  const auto repaired = cr::editCreativeWorldLayoutRoomFootprint(
      layout, {0U, layout.rooms[0].footprint});
  const auto stable = repaired.accepted
                          ? cr::editCreativeWorldLayoutRoomFootprint(
                                repaired.edited,
                                {0U, repaired.edited.rooms[0].footprint})
                          : cr::CreativeWorldLayoutRoomFootprintEditResult{};

  return expect(repaired.accepted && repaired.changed &&
                    repaired.roomChanges.empty() &&
                    repaired.edited.buildings[0].rootFootprint.minimum ==
                        cr::CreativeTerrainCoord2{-2, -1} &&
                    repaired.edited.buildings[0].rootFootprint.maximum ==
                        cr::CreativeTerrainCoord2{4, 5},
                "room extent repairs stale building-derived bounds") &&
         expect(stable.accepted && !stable.changed &&
                    stable.status ==
                        cr::CreativeWorldLayoutRoomFootprintEditStatus::NoChange,
                "derived extent repair is deterministic and idempotent");
}

bool authoredLegacyRootIsNotReplacedByRoomBounds() {
  cr::CreativeWorldLayout layout = baseLayout();
  layout.buildings[0].rootMode = cr::CreativeBuildingRootMode::CreateRoom;
  layout.buildings[0].rootFootprint = {{-4, -3}, {12, 11}};
  appendRoom(layout, "room_a", {{0, 0}, {4, 4}});
  const bool refreshed =
      cr::refreshCreativeWorldLayoutBuildingRoomFootprint(layout, 0U);
  return expect(refreshed &&
                    layout.buildings[0].rootFootprint.minimum ==
                        cr::CreativeTerrainCoord2{-4, -3} &&
                    layout.buildings[0].rootFootprint.maximum ==
                        cr::CreativeTerrainCoord2{12, 11},
                "legacy CreateRoom root remains independently authored");
}

bool invalidOwnedRoomDoesNotProducePartialBuildingBounds() {
  cr::CreativeWorldLayout layout = baseLayout();
  appendRoom(layout, "room_valid", {{0, 0}, {4, 4}});
  appendRoom(layout, "room_invalid", {{8, 8}, {8, 12}});
  const cr::CreativeWorldLayoutRect original =
      layout.buildings[0].rootFootprint;
  const bool refreshed =
      cr::refreshCreativeWorldLayoutBuildingRoomFootprint(layout, 0U);
  return expect(!refreshed &&
                    layout.buildings[0].rootFootprint.minimum ==
                        original.minimum &&
                    layout.buildings[0].rootFootprint.maximum ==
                        original.maximum,
                "derived bounds fail atomically when an owned room is invalid");
}

bool roomResizeCannotStrandVerticalConnector() {
  cr::CreativeWorldLayout layout = baseLayout();
  layout.buildings[0].rootFootprint = {{0, 0}, {6, 4}};
  appendRoom(layout, "room_ground", {{0, 0}, {6, 4}});
  cr::CreativeWorldLayoutLevel upper = layout.levels[0];
  upper.stableKey = "level_1";
  upper.name = "Upper";
  upper.floorTopLayer = 3.0;
  layout.levels.push_back(upper);
  appendRoom(layout, "room_upper", {{0, 0}, {6, 4}});
  layout.rooms[1].levelIndex = 1U;
  cr::CreativeWorldLayoutVerticalConnector connector;
  connector.buildingIndex = 0U;
  connector.lowerRoomIndex = 0U;
  connector.upperRoomIndex = 1U;
  connector.stableKey = "stair_0";
  connector.name = "Stair";
  connector.footprint = {{4, 1}, {6, 3}};
  layout.verticalConnectors.push_back(connector);

  const auto result = cr::editCreativeWorldLayoutRoomFootprint(
      layout, {0U, {{0, 0}, {4, 4}}});
  return expect(!result.accepted && !result.changed &&
                    result.status ==
                        cr::CreativeWorldLayoutRoomFootprintEditStatus::
                            VerticalConnectorDoesNotFit &&
                    result.failedConnectorIndex == 0U &&
                    layout.rooms[0].footprint.maximum.x == 6,
                "room resize cannot strand a stair or ramp footprint");
}

bool verticallySeparatedOpeningsShareOneMergedFacade() {
  cr::CreativeWorldLayout layout = baseLayout();
  layout.buildings[0].rootFootprint = {{0, 0}, {4, 4}};
  appendRoom(layout, "room_ground", {{0, 0}, {4, 4}});
  cr::CreativeWorldLayoutLevel upper = layout.levels[0];
  upper.stableKey = "level_1";
  upper.name = "Upper";
  upper.floorTopLayer = 3.0;
  layout.levels.push_back(upper);
  appendRoom(layout, "room_upper", {{0, 0}, {4, 4}});
  layout.rooms[1].levelIndex = 1U;
  appendOpening(layout, 0U, cr::CreativeWorldLayoutRoomEdge::North,
                cr::CreativeBuildingOpeningKind::Window, 2.0, 1.0);
  appendOpening(layout, 1U, cr::CreativeWorldLayoutRoomEdge::North,
                cr::CreativeBuildingOpeningKind::Window, 2.0, 1.0);
  layout.openings[0].cutoutBottomCells = 1.0;
  layout.openings[0].cutoutHeightCells = 1.0;
  layout.openings[1].cutoutBottomCells = 1.0;
  layout.openings[1].cutoutHeightCells = 1.0;

  const auto result = cr::editCreativeWorldLayoutRoomFootprint(
      layout, {0U, layout.rooms[0].footprint});
  if (!result.accepted) {
    std::cerr << "stacked-window edit rejected: " << result.reasonCode << '\n';
  }
  return expect(result.accepted && !result.changed &&
                    result.status ==
                        cr::CreativeWorldLayoutRoomFootprintEditStatus::NoChange,
                "stacked windows remain valid on one merged exterior facade");
}

}  // namespace

int main() {
  const bool ok = oneBoundaryMovesEveryOppositeOwner() &&
                  isolatedMoveRemainsRelativeButSharedMoveFailsClosed() &&
                  invalidResultsAreAtomic() &&
                  noOpRepairsThenStabilizesDerivedBuildingExtent() &&
                  authoredLegacyRootIsNotReplacedByRoomBounds() &&
                  invalidOwnedRoomDoesNotProducePartialBuildingBounds() &&
                  roomResizeCannotStrandVerticalConnector() &&
                  verticallySeparatedOpeningsShareOneMergedFacade();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
