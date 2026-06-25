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

bool buildsLoadedStateFromPackageRoom() {
  const iggy3d::ProductAsciiRoomAuthoringRequest request = trainingRequest();
  const iggy3d::ProductAsciiRoomAuthoringResult authoring =
      iggy3d::buildProductAsciiRoomAuthoring(request);
  if (!expect(authoring.ok, "package authoring ok")) {
    return false;
  }

  iggy3d::RoomAsset room = authoring.roomAsset.room;
  room.sourceFile = "assets/rooms/active_room_training.room.iggy3d.toml";
  room.sourceSubset = "ascii_training_room";
  const iggy3d::ProductActiveRoomState active =
      iggy3d::buildProductActiveRoomFromPackageRoom(
          room, "iggy3d.ascii_training_room", "ascii_training_room.runtime_loop");

  return expect(active.loaded, "package active room loaded") &&
         expect(active.status == "active_room_loaded", "package status") &&
         expect(active.reasonCode == "active_room_loaded", "package reason") &&
         expect(active.source == "package_room", "package source") &&
         expect(active.roomId == "active_room_training", "package room id") &&
         expect(active.sourceName ==
                    "assets/rooms/active_room_training.room.iggy3d.toml",
                "package source name") &&
         expect(active.sourceSubset == "ascii_training_room",
                "package source subset") &&
         expect(!active.hasAuthoredRoom, "package authored room absent") &&
         expect(active.authoredFloorCount == 0U, "package authored floor count") &&
         expect(active.authoredWallCount == 0U, "package authored wall count") &&
         expect(active.staticMeshCount == 36U, "package static mesh count") &&
         expect(active.anchorCount == 5U, "package anchor count") &&
         expect(active.spatialSurfaceCount == 56U, "package surface count") &&
         expect(active.walkableSurfaceCount == 15U, "package walkable count") &&
         expect(active.actorBlockerSurfaceCount == 21U,
                "package actor blocker count") &&
         expect(active.projectileBlockerSurfaceCount == 21U,
                "package projectile blocker count");
}

bool buildsLoadedStateFromSavedAuthoredRoom() {
  const iggy3d::ProductAsciiRoomAuthoringRequest request = trainingRequest();
  const iggy3d::ProductAsciiRoomAuthoringResult authoring =
      iggy3d::buildProductAsciiRoomAuthoring(request);
  if (!expect(authoring.ok, "saved authoring ok")) {
    return false;
  }

  const iggy3d::ProductActiveRoomState active =
      iggy3d::buildProductActiveRoomFromSavedAuthoredRoom(
          authoring.authoredRoom.authoredRoom);

  return expect(active.loaded, "saved active room loaded") &&
         expect(active.status == "active_room_loaded", "saved status") &&
         expect(active.reasonCode == "active_room_loaded", "saved reason") &&
         expect(active.source == "saved_authored_room", "saved source") &&
         expect(active.roomId == "active_room_training", "saved room id") &&
         expect(active.sourceName == "unit/active_room_training.iggyroom.txt",
                "saved source name") &&
         expect(active.sourceSubset == "ascii_room_authoring",
                "saved source subset") &&
         expect(active.hasAuthoredRoom, "saved authored room present") &&
         expect(active.authoredFloorCount == 15U,
                "saved authored floor count") &&
         expect(active.authoredWallCount == 20U,
                "saved authored wall count") &&
         expect(active.staticMeshCount == 35U, "saved static mesh count") &&
         expect(active.anchorCount == 0U, "saved anchor count") &&
         expect(active.spatialSurfaceCount == 55U, "saved surface count") &&
         expect(active.walkableSurfaceCount == 15U, "saved walkable count") &&
         expect(active.actorBlockerSurfaceCount == 20U,
                "saved actor blocker count") &&
         expect(active.projectileBlockerSurfaceCount == 20U,
                "saved projectile blocker count") &&
         expect(active.room.staticMeshes.size() == 35U,
                "saved room meshes owned") &&
         expect(active.room.spatialSurfaces.size() == 55U,
                "saved room surfaces owned");
}

}  // namespace

int main() {
  const bool ok = buildsLoadedStateFromAsciiAuthoring() &&
                  recordsAuthoringFailureWithoutRoomOwnership() &&
                  buildsLoadedStateFromPackageRoom() &&
                  buildsLoadedStateFromSavedAuthoredRoom();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
