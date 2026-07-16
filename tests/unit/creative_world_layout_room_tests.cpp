#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <cstdlib>
#include <iostream>

namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeWorldLayout adjacentRooms() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "room_topology";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building_1";
  building.name = "House";
  layout.buildings.push_back(building);

  cr::CreativeWorldLayoutRoom left;
  left.buildingIndex = 0U;
  left.stableKey = "room_left";
  left.name = "Left Room";
  left.footprint = {{0, 0}, {4, 4}};
  layout.rooms.push_back(left);

  cr::CreativeWorldLayoutRoom right = left;
  right.stableKey = "room_right";
  right.name = "Right Room";
  right.footprint = {{4, 0}, {8, 4}};
  layout.rooms.push_back(right);

  cr::CreativeWorldLayoutOpening door;
  door.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  door.roomIndex = 1U;
  door.roomEdge = cr::CreativeWorldLayoutRoomEdge::MinimumX;
  door.kind = cr::CreativeBuildingOpeningKind::Door;
  door.stableKey = "shared_door";
  door.name = "Shared Door";
  door.centerOffsetCells = 2.0;
  layout.openings.push_back(door);
  return layout;
}

bool adjacentRoomsShareOneCanonicalWall() {
  const cr::CreativeWorldLayoutRoomCompileResult first =
      cr::expandCreativeWorldLayoutRooms(adjacentRooms());
  const cr::CreativeWorldLayoutRoomCompileResult second =
      cr::expandCreativeWorldLayoutRooms(adjacentRooms());
  if (!expect(first.accepted && second.accepted,
              "adjacent room expansion is accepted")) {
    return false;
  }
  const cr::CreativeWorldLayoutOpening& opening = first.expanded.openings[0];
  const cr::CreativeWorldLayoutWall& host =
      first.expanded.walls[opening.wallIndex];
  return expect(first.expanded.boxes.size() == 2U,
                "each room derives one floor") &&
         expect(first.expanded.walls.size() == 5U,
                "shared boundary is emitted exactly once") &&
         expect(
             opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall &&
                 host.start == cr::CreativeTerrainCoord2{4, 0} &&
                 host.end == cr::CreativeTerrainCoord2{4, 4} &&
                 opening.centerOffsetCells == 2.0,
             "room-edge opening resolves onto the canonical shared wall") &&
         expect(first.expanded.walls[0].stableKey ==
                        second.expanded.walls[0].stableKey &&
                    first.expanded.walls.back().start ==
                        second.expanded.walls.back().start,
                "room expansion ordering is deterministic");
}

bool invalidTopologyFailsClosed() {
  cr::CreativeWorldLayout overlap = adjacentRooms();
  overlap.rooms[1].footprint = {{3, 1}, {7, 5}};
  const auto overlapResult = cr::expandCreativeWorldLayoutRooms(overlap);

  cr::CreativeWorldLayout badOpening = adjacentRooms();
  badOpening.openings[0].centerOffsetCells = 5.0;
  const auto openingResult = cr::expandCreativeWorldLayoutRooms(badOpening);

  cr::CreativeWorldLayout straddledOpening = adjacentRooms();
  straddledOpening.openings[0].centerOffsetCells = 3.75;
  const auto straddledResult =
      cr::expandCreativeWorldLayoutRooms(straddledOpening);

  return expect(
             !overlapResult.accepted &&
                 overlapResult.status ==
                     cr::CreativeWorldLayoutRoomCompileStatus::OverlappingRooms,
             "interior-overlapping rooms are rejected") &&
         expect(!openingResult.accepted &&
                    openingResult.status ==
                        cr::CreativeWorldLayoutRoomCompileStatus::
                            InvalidOpeningHost,
                "opening outside its semantic room edge is rejected") &&
         expect(!straddledResult.accepted &&
                    straddledResult.status ==
                        cr::CreativeWorldLayoutRoomCompileStatus::
                            InvalidOpeningHost,
                "opening width cannot straddle beyond its semantic edge");
}

bool roomTopologyCompilesThroughExistingBuildingRecipe() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Room Layout");
  static_cast<void>(document.assignId(9201U));
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, adjacentRooms());
  const cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, compiled.plan);
  return expect(compiled.receipt.accepted &&
                    compiled.receipt.objectRecipeCount == 1U,
                "semantic rooms compile through one building recipe") &&
         expect(preview.accepted && preview.document.objectCount() ==
                                        compiled.receipt.objectCount,
                "exact preview materializes the compiled room shell");
}

}  // namespace

int main() {
  const bool ok = adjacentRoomsShareOneCanonicalWall() &&
                  invalidTopologyFailsClosed() &&
                  roomTopologyCompilesThroughExistingBuildingRecipe();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
