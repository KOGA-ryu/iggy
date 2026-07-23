#include "app/iggy3d/creative/world/WorldLayoutOpenings.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/world/WorldLayoutDimensions.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"

#include <cstdlib>
#include <iostream>
#include <iterator>
#include <limits>
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

cr::CreativeWorldLayout baseLayout() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "opening_contract";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building";
  building.name = "Building";
  building.rootFootprint = {{0, 0}, {12, 12}};
  layout.buildings.push_back(building);
  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "ground";
  level.name = "Ground";
  level.wallHeightCells = 3U;
  layout.levels.push_back(level);
  return layout;
}

void appendRoom(cr::CreativeWorldLayout& layout, std::string key,
                cr::CreativeWorldLayoutRect footprint) {
  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = 0U;
  room.levelIndex = 0U;
  room.stableKey = std::move(key);
  room.name = "Room";
  room.footprint = footprint;
  layout.rooms.push_back(std::move(room));
}

cr::CreativeWorldLayout lRoomLayout() {
  cr::CreativeWorldLayout layout = baseLayout();
  appendRoom(layout, "room_l", {{0, 0}, {6, 6}});
  const cr::CreativeTerrainCoord2 points[] = {
      {0, 0}, {6, 0}, {6, 2}, {2, 2}, {2, 6}, {0, 6},
  };
  for (std::size_t index = 0U; index < std::size(points); ++index) {
    layout.topologyVertices.push_back(
        {0U, "vertex_" + std::to_string(index), points[index]});
  }
  const std::size_t edges[][2] = {
      {0U, 1U}, {1U, 2U}, {3U, 2U},
      {3U, 4U}, {5U, 4U}, {0U, 5U},
  };
  const bool reversed[] = {false, false, true, false, true, true};
  for (std::size_t index = 0U; index < std::size(edges); ++index) {
    cr::CreativeWorldLayoutTopologyEdge edge;
    edge.levelIndex = 0U;
    edge.stableKey = "edge_" + std::to_string(index);
    edge.startVertexIndex = edges[index][0];
    edge.endVertexIndex = edges[index][1];
    edge.wallThicknessCells = index == 2U ? 0.4 : 0.25;
    edge.wallHeightCells = index == 2U ? 4U : 0U;
    layout.topologyEdges.push_back(std::move(edge));
    layout.roomBoundaries.push_back({0U, index, index, reversed[index]});
  }
  return layout;
}

cr::CreativeWorldLayoutOpening openingOnEdge(std::size_t edgeIndex) {
  cr::CreativeWorldLayoutOpening opening;
  opening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  opening.roomIndex = 0U;
  opening.roomEdge = cr::CreativeWorldLayoutRoomEdge::Count;
  opening.roomTopologyEdgeIndex = edgeIndex;
  opening.kind = cr::CreativeBuildingOpeningKind::Door;
  opening.stableKey = "opening";
  opening.name = "Opening";
  opening.centerOffsetCells = 2.0;
  opening.widthCells = 1.0;
  opening.cutoutBottomCells = 0.0;
  opening.cutoutHeightCells = 2.1;
  return opening;
}

bool directHostOwnsCanonicalGeometryAndHitTesting() {
  cr::CreativeWorldLayout layout = lRoomLayout();
  const cr::CreativeWorldLayoutOpening opening = openingOnEdge(2U);
  const cr::CreativeWorldLayoutOpeningHostFrame host =
      cr::resolveCreativeWorldLayoutOpeningHost(layout, opening);
  const cr::CreativeWorldLayoutOpeningHostHit hit =
      cr::findNearestCreativeWorldLayoutOpeningHost(layout, {4.0, 2.1}, 0.2,
                                                    0U);
  const cr::CreativeWorldLayoutOpeningHostPoint center =
      cr::creativeWorldLayoutOpeningHostPoint(host, 2.0);
  layout.openings.push_back(opening);
  cr::CreativeGridSettings grid;
  grid.cellSizeMeters = 1.0;
  const cr::CreativeWorldLayoutOpeningDimensions dimensions =
      cr::measureCreativeWorldLayoutOpeningDimensions(grid, layout, 0U);
  return expect(host.accepted && host.topologyEdgeIndex == 2U &&
                    host.roomEdge == cr::CreativeWorldLayoutRoomEdge::Count &&
                    host.start == cr::CreativeTerrainCoord2{2, 2} &&
                    host.end == cr::CreativeTerrainCoord2{6, 2},
                "direct host resolves the concave room edge without a fake side") &&
         expect(host.exterior && host.owningRoomCount == 1U &&
                    host.wallHeightCells == 4.0 &&
                    host.wallThicknessCells == 0.4,
                "host inherits per-edge wall semantics and exterior ownership") &&
         expect(center.x == 4.0 && center.z == 2.0 &&
                    cr::creativeWorldLayoutOpeningHostOffset(host, center) ==
                        2.0,
                "host point and offset use one canonical frame") &&
         expect(dimensions.accepted && dimensions.wallTopMeters == 4.0 &&
                    dimensions.insertThicknessMeters == 0.4,
                "dimension conversion consumes the same per-edge host frame") &&
         expect(hit.hit &&
                    hit.hostKind ==
                        cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
                    hit.roomIndex == 0U && hit.topologyEdgeIndex == 2U &&
                    hit.roomEdge == cr::CreativeWorldLayoutRoomEdge::Count &&
                    hit.centerOffsetCells == 2.0,
                "nearest-host query returns the stable topology edge");
}

bool validationPinsCutoutInsertAndOverlapLaws() {
  cr::CreativeWorldLayout layout = lRoomLayout();
  cr::CreativeWorldLayoutOpening door = openingOnEdge(2U);
  const cr::CreativeWorldLayoutOpeningValidationResult ready =
      cr::validateCreativeWorldLayoutOpening({&layout, &door});

  cr::CreativeWorldLayoutOpening badSill = door;
  badSill.cutoutBottomCells = 0.25;
  const auto sill =
      cr::validateCreativeWorldLayoutOpening({&layout, &badSill});

  cr::CreativeWorldLayoutOpening tooTall = door;
  tooTall.cutoutHeightCells = 4.25;
  const auto height =
      cr::validateCreativeWorldLayoutOpening({&layout, &tooTall});

  cr::CreativeWorldLayoutOpening badFacing = door;
  badFacing.facing = cr::CreativeBuildingOpeningFacing::Count;
  const auto facing =
      cr::validateCreativeWorldLayoutOpening({&layout, &badFacing});

  cr::CreativeWorldLayoutOpening badDoorSettings = door;
  badDoorSettings.door.transitionSeconds =
      std::numeric_limits<double>::quiet_NaN();
  const auto invalidDoor =
      cr::validateCreativeWorldLayoutOpening({&layout, &badDoorSettings});

  cr::CreativeWorldLayoutOpening badWindowSettings = door;
  badWindowSettings.kind = cr::CreativeBuildingOpeningKind::Window;
  badWindowSettings.cutoutBottomCells = 1.0;
  badWindowSettings.insertBottomCells = 1.0;
  badWindowSettings.window.insertKind = cr::CreativeWindowInsertKind::Count;
  const auto invalidWindow =
      cr::validateCreativeWorldLayoutOpening({&layout, &badWindowSettings});

  layout.openings.push_back(door);
  cr::CreativeWorldLayoutOpening overlap = door;
  overlap.stableKey = "overlap";
  overlap.centerOffsetCells = 2.5;
  const auto blocked =
      cr::validateCreativeWorldLayoutOpening({&layout, &overlap});

  cr::CreativeWorldLayoutOpening upperWindow = overlap;
  upperWindow.kind = cr::CreativeBuildingOpeningKind::Window;
  upperWindow.cutoutBottomCells = 2.2;
  upperWindow.cutoutHeightCells = 1.0;
  upperWindow.insertBottomCells = 2.2;
  upperWindow.insertHeightCells = 1.0;
  const auto verticallySeparated =
      cr::validateCreativeWorldLayoutOpening({&layout, &upperWindow});
  const bool validCollection = cr::validCreativeWorldLayoutOpenings(layout);
  layout.openings.front().cutoutHeightCells = 8.0;
  const bool invalidCollection =
      !cr::validCreativeWorldLayoutOpenings(layout);

  return expect(ready.accepted && ready.sillTopCells == 0.0 &&
                    ready.lintelBottomCells == 2.1 &&
                    ready.lintelHeightCells == 1.9,
                "validation publishes derived sill and lintel facts") &&
         expect(!sill.accepted &&
                    sill.status ==
                        cr::CreativeWorldLayoutOpeningValidationStatus::
                            DoorSillInvalid,
                "door sill is rejected by the shared contract") &&
         expect(!height.accepted &&
                    height.status ==
                        cr::CreativeWorldLayoutOpeningValidationStatus::
                            WallHeightExceeded,
                "cutout cannot exceed the per-edge wall height") &&
         expect(!facing.accepted &&
                    facing.status ==
                        cr::CreativeWorldLayoutOpeningValidationStatus::
                            InvalidFacing,
                "invalid authored facing fails closed") &&
         expect(!invalidDoor.accepted &&
                    invalidDoor.status ==
                        cr::CreativeWorldLayoutOpeningValidationStatus::
                            InvalidDoorSettings,
                "invalid authored door settings fail closed") &&
         expect(!invalidWindow.accepted &&
                    invalidWindow.status ==
                        cr::CreativeWorldLayoutOpeningValidationStatus::
                            InvalidWindowSettings,
                "invalid authored window treatment fails closed") &&
         expect(!blocked.accepted && blocked.conflictingOpeningIndex == 0U &&
                    blocked.status ==
                        cr::CreativeWorldLayoutOpeningValidationStatus::Overlap,
                "same-height hosted openings cannot overlap") &&
         expect(verticallySeparated.accepted,
                "vertically separated cutouts may share one host interval") &&
         expect(validCollection && invalidCollection,
                "layout validation checks every authored opening");
}

bool sharedEdgeWindowAndMovementObstructionAreDiagnosed() {
  cr::CreativeWorldLayout legacy = baseLayout();
  appendRoom(legacy, "west", {{0, 0}, {4, 4}});
  appendRoom(legacy, "east", {{4, 0}, {8, 4}});
  const cr::CreativeWorldLayoutRoomGraphMaterializeResult materialized =
      cr::materializeCreativeWorldLayoutRoomGraph(legacy);
  if (!expect(materialized.accepted, "shared-edge fixture materializes")) {
    return false;
  }
  const cr::CreativeWorldLayout& layout = materialized.edited;
  std::size_t sharedEdge = cr::kInvalidCreativeWorldLayoutIndex;
  std::size_t owner = cr::kInvalidCreativeWorldLayoutIndex;
  for (std::size_t edgeIndex = 0U; edgeIndex < layout.topologyEdges.size();
       ++edgeIndex) {
    std::size_t count = 0U;
    for (const cr::CreativeWorldLayoutRoomBoundary& boundary :
         layout.roomBoundaries) {
      if (boundary.topologyEdgeIndex == edgeIndex) {
        ++count;
        owner = boundary.roomIndex;
      }
    }
    if (count == 2U) {
      sharedEdge = edgeIndex;
      break;
    }
  }
  cr::CreativeWorldLayoutOpening window = openingOnEdge(sharedEdge);
  window.roomIndex = owner;
  window.kind = cr::CreativeBuildingOpeningKind::Window;
  window.cutoutBottomCells = 1.0;
  window.cutoutHeightCells = 1.2;
  window.insertBottomCells = 1.0;
  window.insertHeightCells = 1.2;
  window.insertWidthCells = 1.0;
  const auto interior =
      cr::validateCreativeWorldLayoutOpening({&layout, &window});

  cr::CreativeWorldLayoutOpening narrowDoor = openingOnEdge(sharedEdge);
  narrowDoor.roomIndex = owner;
  narrowDoor.widthCells = 0.5;
  const auto width = cr::evaluateCreativeWorldLayoutOpeningClearance(
      {&narrowDoor, 1.0, 0.30, 1.80, 0.35, 0.02});
  narrowDoor.widthCells = 1.0;
  const auto passage = cr::evaluateCreativeWorldLayoutOpeningClearance(
      {&narrowDoor, 1.0, 0.30, 1.80, 0.35, 0.02});

  return expect(sharedEdge != cr::kInvalidCreativeWorldLayoutIndex &&
                    !interior.accepted &&
                    interior.status ==
                        cr::CreativeWorldLayoutOpeningValidationStatus::
                            InteriorWindow,
                "windows reject shared interior topology edges") &&
         expect(width.accepted && !width.traversable &&
                    width.status ==
                        cr::CreativeWorldLayoutOpeningClearanceStatus::
                            WidthObstructed,
                "player envelope diagnoses a narrow movement opening") &&
         expect(passage.accepted && passage.traversable &&
                    passage.status ==
                        cr::CreativeWorldLayoutOpeningClearanceStatus::Ready,
                "player envelope accepts a usable doorway");
}

bool doorSwingClearanceRejectsWallsAndCriticalRoutes() {
  cr::CreativeWorldLayout clearLayout = lRoomLayout();
  cr::CreativeWorldLayoutOpening door = openingOnEdge(2U);
  const auto clear = cr::evaluateCreativeWorldLayoutDoorSwingClearance(
      {&clearLayout, &door});

  cr::CreativeWorldLayout wallLayout = clearLayout;
  cr::CreativeWorldLayoutWall obstacle;
  obstacle.buildingIndex = 0U;
  obstacle.stableKey = "swing_obstacle";
  obstacle.name = "Swing Obstacle";
  obstacle.start = {4, 2};
  obstacle.end = {4, 4};
  obstacle.heightCells = 3U;
  obstacle.thicknessCells = 0.25;
  wallLayout.walls.push_back(obstacle);
  const auto wallBlocked =
      cr::evaluateCreativeWorldLayoutDoorSwingClearance({&wallLayout, &door});
  const auto wallValidation =
      cr::validateCreativeWorldLayoutOpening({&wallLayout, &door});

  cr::CreativeWorldLayout routeLayout = clearLayout;
  door.widthCells = 2.0;
  cr::CreativeWorldLayoutLevel upper = routeLayout.levels[0];
  upper.stableKey = "upper";
  upper.name = "Upper";
  upper.floorTopLayer = 3.0;
  routeLayout.levels.push_back(upper);
  cr::CreativeWorldLayoutRoom upperRoom = routeLayout.rooms[0];
  upperRoom.levelIndex = 1U;
  upperRoom.stableKey = "upper_room";
  routeLayout.rooms.push_back(upperRoom);
  cr::CreativeWorldLayoutVerticalConnector connector;
  connector.buildingIndex = 0U;
  connector.lowerRoomIndex = 0U;
  connector.upperRoomIndex = 1U;
  connector.stableKey = "critical_stair";
  connector.name = "Critical Stair";
  connector.footprint = {{3, 3}, {5, 4}};
  routeLayout.verticalConnectors.push_back(connector);
  const auto routeBlocked =
      cr::evaluateCreativeWorldLayoutDoorSwingClearance({&routeLayout, &door});
  door.door.swingSide = cr::CreativeDoorSwingSide::NegativeNormal;
  const auto reversedClear =
      cr::evaluateCreativeWorldLayoutDoorSwingClearance({&routeLayout, &door});

  return expect(clear.accepted && clear.clear &&
                    clear.status ==
                        cr::CreativeWorldLayoutDoorSwingClearanceStatus::Ready &&
                    cr::measureCreativeBounds(clear.sweepBoundsCells).valid,
                "door recipe publishes a valid clear swept volume") &&
         expect(wallBlocked.accepted && !wallBlocked.clear &&
                    wallBlocked.status ==
                        cr::CreativeWorldLayoutDoorSwingClearanceStatus::
                            WallObstructed &&
                    wallBlocked.conflictingWallIndex == 0U &&
                    !wallValidation.accepted &&
                    wallValidation.status ==
                        cr::CreativeWorldLayoutOpeningValidationStatus::
                            DoorSwingObstructed,
                "door swing colliding with another wall fails before mutation") &&
         expect(routeBlocked.accepted && !routeBlocked.clear &&
                    routeBlocked.status ==
                        cr::CreativeWorldLayoutDoorSwingClearanceStatus::
                            CriticalPathObstructed &&
                    routeBlocked.conflictingVerticalConnectorIndex == 0U,
                "door swing cannot consume a vertical circulation footprint") &&
         expect(reversedClear.accepted && reversedClear.clear,
                "changing the swing side can resolve the critical-route conflict");
}

}  // namespace

int main() {
  const bool ok = directHostOwnsCanonicalGeometryAndHitTesting() &&
                  validationPinsCutoutInsertAndOverlapLaws() &&
                  sharedEdgeWindowAndMovementObstructionAreDiagnosed() &&
                  doorSwingClearanceRejectsWallsAndCriticalRoutes();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
