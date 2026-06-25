#include "app/iggy3d/ProductActiveRoomState.hpp"
#include "app/iggy3d/ProductAsciiRoomAuthoring.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

constexpr std::string_view kTrainingRoom =
    "#######\n"
    "#P..N.#\n"
    "#.+.$.#\n"
    "#..E..#\n"
    "#######\n";

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::ProductAsciiRoomAuthoringRequest trainingRequest() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText = std::string(kTrainingRoom);
  request.roomId = "active_room_training";
  request.sourceName = "unit/active_room_training.iggyroom.txt";
  return request;
}

bool buildsLoadedStateFromAsciiAuthoring() {
  const iggy3d::ProductAsciiRoomAuthoringRequest request = trainingRequest();
  const iggy3d::ProductAsciiRoomAuthoringResult authoring =
      iggy3d::buildProductAsciiRoomAuthoring(request);
  const iggy3d::ProductActiveRoomState active =
      iggy3d::buildProductActiveRoomFromAsciiAuthoring(request, authoring);

  return expect(authoring.ok, "authoring ok") &&
         expect(active.loaded, "active room loaded") &&
         expect(active.status == "active_room_loaded", "status") &&
         expect(active.reasonCode == "active_room_loaded", "reason") &&
         expect(active.source == "ascii_room", "source") &&
         expect(active.roomId == "active_room_training", "room id") &&
         expect(active.sourceName == "unit/active_room_training.iggyroom.txt",
                "source name") &&
         expect(active.sourceSubset == "ascii_room_authoring",
                "source subset") &&
         expect(active.hasAuthoredRoom, "authored room present") &&
         expect(active.authoredRoom.id == "active_room_training",
                "authored room id") &&
         expect(active.authoredFloorCount == 15U, "authored floor count") &&
         expect(active.authoredWallCount == 20U, "authored wall count") &&
         expect(active.staticMeshCount == 36U, "static mesh count") &&
         expect(active.anchorCount == 5U, "anchor count") &&
         expect(active.openingCount == 0U, "opening count") &&
         expect(active.spatialSurfaceCount == 56U, "surface count") &&
         expect(active.walkableSurfaceCount == 15U, "walkable count") &&
         expect(active.actorBlockerSurfaceCount == 21U,
                "actor blocker count") &&
         expect(active.projectileBlockerSurfaceCount == 21U,
                "projectile blocker count") &&
         expect(active.room.staticMeshes.size() == 36U, "room meshes owned") &&
         expect(active.room.spatialSurfaces.size() == 56U,
                "room surfaces owned");
}

bool recordsAuthoringFailureWithoutRoomOwnership() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText =
      "...\n"
      "...\n";
  request.roomId = "missing_spawn_room";
  request.sourceName = "unit/missing_spawn.iggyroom.txt";
  const iggy3d::ProductAsciiRoomAuthoringResult authoring =
      iggy3d::buildProductAsciiRoomAuthoring(request);
  const iggy3d::ProductActiveRoomState active =
      iggy3d::buildProductActiveRoomFromAsciiAuthoring(request, authoring);

  return expect(!authoring.ok, "authoring rejected") &&
         expect(!active.loaded, "active room not loaded") &&
         expect(active.status == "ascii_room_missing_player_spawn", "status") &&
         expect(active.reasonCode == "ascii_room_missing_player_spawn",
                "reason") &&
         expect(active.source == "ascii_room", "source") &&
         expect(active.roomId == "missing_spawn_room", "room id") &&
         expect(active.sourceName == "unit/missing_spawn.iggyroom.txt",
                "source name") &&
         expect(!active.hasAuthoredRoom, "no authored room") &&
         expect(active.staticMeshCount == 0U, "no meshes") &&
         expect(active.spatialSurfaceCount == 0U, "no surfaces");
}

}  // namespace

int main() {
  const bool ok = buildsLoadedStateFromAsciiAuthoring() &&
                  recordsAuthoringFailureWithoutRoomOwnership();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
