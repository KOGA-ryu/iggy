#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/iggy3d/ascii_room/Authoring.hpp"
#include "app/iggy3d/ascii_room/Editing.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
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

bool activeRoomRevisionDefaultsAndBumps() {
  iggy3d::ProductAppWindowState window;
  bool ok =
      expect(iggy3d::activeRoomRevision(window) == 0U,
             "active room revision default");

  iggy3d::bumpActiveRoomRevision(window);
  ok = ok && expect(iggy3d::activeRoomRevision(window) == 1U,
                    "active room revision first bump");
  iggy3d::bumpActiveRoomRevision(window);
  iggy3d::bumpActiveRoomRevision(window);
  ok = ok && expect(iggy3d::activeRoomRevision(window) == 3U,
                    "active room revision third bump");
  return ok;
}

bool roomStoreAccessorsAliasWindowOwnedFields() {
  iggy3d::ProductAppWindowState window;
  bool ok = expect(&iggy3d::activeRoom(window) == &window.room.activeRoom,
                   "active room accessor aliases field") &&
            expect(&iggy3d::activeRoomRevision(window) ==
                       &window.room.activeRoomRevision,
                   "active room revision accessor aliases field") &&
            expect(&iggy3d::activeRoomCollision(window) ==
                       &window.room.activeRoomCollision,
                   "active room collision accessor aliases field") &&
            expect(&iggy3d::activeRoomCollisionFreshness(window) ==
                       &window.room.activeRoomCollisionFreshness,
                   "active room collision freshness accessor aliases field");

  iggy3d::activeRoom(window).loaded = true;
  iggy3d::activeRoom(window).roomId = "room_store_alias";
  iggy3d::activeRoomRevision(window) = 42U;
  iggy3d::activeRoomCollision(window).ready = true;
  iggy3d::activeRoomCollision(window).querySurfaceCount = 7U;
  iggy3d::activeRoomCollisionFreshness(window).rebaked = true;
  iggy3d::activeRoomCollisionFreshness(window).observedRoomRevision = 42U;

  ok = ok && expect(window.room.activeRoom.loaded, "active room write-through loaded") &&
       expect(window.room.activeRoom.roomId == "room_store_alias",
              "active room write-through room id") &&
       expect(window.room.activeRoomRevision == 42U,
              "active room revision write-through") &&
       expect(window.room.activeRoomCollision.ready,
              "active room collision write-through ready") &&
       expect(window.room.activeRoomCollision.querySurfaceCount == 7U,
              "active room collision write-through query count") &&
       expect(window.room.activeRoomCollisionFreshness.rebaked,
              "active room collision freshness write-through rebaked") &&
       expect(window.room.activeRoomCollisionFreshness.observedRoomRevision == 42U,
              "active room collision freshness write-through revision");

  const iggy3d::ProductAppWindowState& constWindow = window;
  ok = ok &&
       expect(&iggy3d::activeRoom(constWindow) ==
                  static_cast<const iggy3d::ProductActiveRoomState*>(
                      &window.room.activeRoom),
              "const active room accessor aliases field") &&
       expect(&iggy3d::activeRoomRevision(constWindow) ==
                  static_cast<const std::uint64_t*>(
                      &window.room.activeRoomRevision),
              "const active room revision accessor aliases field") &&
       expect(&iggy3d::activeRoomCollision(constWindow) ==
                  static_cast<const iggy3d::ProductActiveRoomCollisionState*>(
                      &window.room.activeRoomCollision),
              "const active room collision accessor aliases field") &&
       expect(&iggy3d::activeRoomCollisionFreshness(constWindow) ==
                  static_cast<
                      const iggy3d::ProductActiveRoomCollisionFreshnessResult*>(
                      &window.room.activeRoomCollisionFreshness),
              "const active room collision freshness accessor aliases field") &&
       expect(iggy3d::activeRoom(constWindow).loaded,
              "const active room exposes loaded value") &&
       expect(iggy3d::activeRoomRevision(constWindow) == 42U,
              "const active room revision exposes value") &&
       expect(iggy3d::activeRoomCollision(constWindow).querySurfaceCount == 7U,
              "const active room collision exposes query count") &&
       expect(iggy3d::activeRoomCollisionFreshness(constWindow)
                  .observedRoomRevision == 42U,
              "const active room collision freshness exposes revision");

  return ok;
}

iggy3d::ProductAsciiRoomAuthoringRequest trainingRequest() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText = std::string(kTrainingRoom);
  request.roomId = "active_room_training";
  request.sourceName = "unit/active_room_training.iggyroom.txt";
  return request;
}

iggy3d::ProductAsciiRoomAuthoringRequest smallEditingRequest() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText =
      "###\n"
      "#P#\n"
      "###\n";
  request.roomId = "active_editable_room";
  request.sourceName = "unit/active_editable_room.iggyroom.txt";
  request.emitAssetText = false;
  return request;
}

iggy3d::EditableRoomFloor extraFloorPrimitive() {
  iggy3d::EditableRoomFloor floor;
  floor.id = "active_edit_floor_1";
  floor.centerMeters = {2.0F, -0.05F, 0.0F};
  floor.sizeMeters = {1.0F, 0.10F, 1.0F};
  floor.semantics = iggy3d::defaultFloorSemantics("debug_floor");
  return floor;
}

iggy3d::EditableRoomWall extraWallPrimitive() {
  iggy3d::EditableRoomWall wall;
  wall.id = "active_edit_wall_1";
  wall.startMeters = {2.0F, 0.0F, -0.5F};
  wall.endMeters = {3.0F, 0.0F, -0.5F};
  wall.bottomY = 0.0F;
  wall.heightMeters = 2.50F;
  wall.thicknessMeters = 1.0F;
  wall.semantics = iggy3d::defaultWallSemantics("debug_wall");
  return wall;
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
         expect(active.authoredMarkerCount == 5U, "authored marker count") &&
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
         expect(active.authoredMarkerCount == 0U, "no authored markers") &&
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
         expect(active.authoredMarkerCount == 0U,
                "package authored marker count") &&
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
         expect(active.authoredMarkerCount == 5U,
                "saved authored marker count") &&
         expect(active.staticMeshCount == 36U, "saved static mesh count") &&
         expect(active.anchorCount == 5U, "saved anchor count") &&
         expect(active.spatialSurfaceCount == 56U, "saved surface count") &&
         expect(active.walkableSurfaceCount == 15U, "saved walkable count") &&
         expect(active.actorBlockerSurfaceCount == 21U,
                "saved actor blocker count") &&
         expect(active.projectileBlockerSurfaceCount == 21U,
                "saved projectile blocker count") &&
         expect(active.room.staticMeshes.size() == 36U,
                "saved room meshes owned") &&
         expect(active.room.anchors.size() == 5U,
                "saved room anchors owned") &&
         expect(active.room.spatialSurfaces.size() == 56U,
                "saved room surfaces owned");
}

bool buildsLoadedStateFromEditableRoomSnapshot() {
  iggy3d::ProductAsciiRoomEditingResult editing =
      iggy3d::buildProductAsciiRoomEditing(smallEditingRequest());
  const iggy3d::ProductRoomAuthoringCommandResult addFloor =
      editing.controller.submit(iggy3d::ProductRoomAuthoringInputSource::Ai,
                                iggy3d::addFloorCommand(extraFloorPrimitive()));
  const iggy3d::ProductRoomAuthoringCommandResult addWall =
      editing.controller.submit(iggy3d::ProductRoomAuthoringInputSource::Mouse,
                                iggy3d::addWallCommand(extraWallPrimitive()));
  const iggy3d::ProductActiveRoomState active =
      iggy3d::buildProductActiveRoomFromRoomAuthoringSnapshot(
          editing.controller.snapshot());

  return expect(editing.ok, "editing result ok") &&
         expect(addFloor.accepted, "editable add floor accepted") &&
         expect(addWall.accepted, "editable add wall accepted") &&
         expect(active.loaded, "editable active room loaded") &&
         expect(active.status == "active_room_loaded", "editable status") &&
         expect(active.reasonCode == "active_room_loaded",
                "editable reason") &&
         expect(active.source == "editable_room", "editable source") &&
         expect(active.roomId == "active_editable_room", "editable room id") &&
         expect(active.sourceName == "unit/active_editable_room.iggyroom.txt",
                "editable source name") &&
         expect(active.sourceSubset == "ascii_room_authoring",
                "editable source subset") &&
         expect(active.hasAuthoredRoom, "editable authored save present") &&
         expect(active.authoredFloorCount == 2U,
                "editable authored floor count") &&
         expect(active.authoredWallCount == 9U,
                "editable authored wall count") &&
         expect(active.authoredMarkerCount == 0U,
                "editable authored marker count") &&
         expect(active.authoredRoom.id == "active_editable_room",
                "editable authored room id") &&
         expect(active.staticMeshCount == 11U,
                "editable static mesh count") &&
         expect(active.anchorCount == 0U, "editable anchor count") &&
         expect(active.openingCount == 0U, "editable opening count") &&
         expect(active.spatialSurfaceCount == 20U,
                "editable surface count") &&
         expect(active.walkableSurfaceCount == 2U,
                "editable walkable count") &&
         expect(active.actorBlockerSurfaceCount == 9U,
                "editable actor blocker count") &&
         expect(active.projectileBlockerSurfaceCount == 9U,
                "editable projectile blocker count") &&
         expect(active.room.staticMeshes.size() == 11U,
                "editable room meshes owned") &&
         expect(active.room.spatialSurfaces.size() == 20U,
                "editable room surfaces owned");
}

bool rejectsUnreadyEditableRoomSnapshot() {
  const iggy3d::ProductRoomAuthoringSnapshot snapshot;
  const iggy3d::ProductActiveRoomState active =
      iggy3d::buildProductActiveRoomFromRoomAuthoringSnapshot(snapshot);

  return expect(!active.loaded, "unready editable room not loaded") &&
         expect(active.status == "not_requested", "unready status") &&
         expect(active.reasonCode == "not_requested", "unready reason") &&
         expect(active.source == "editable_room", "unready source") &&
         expect(active.roomId == "editable_room", "unready room id") &&
         expect(active.staticMeshCount == 0U, "unready mesh count") &&
         expect(active.spatialSurfaceCount == 0U, "unready surface count");
}

}  // namespace

int main() {
  const bool ok = activeRoomRevisionDefaultsAndBumps() &&
                  roomStoreAccessorsAliasWindowOwnedFields() &&
                  buildsLoadedStateFromAsciiAuthoring() &&
                  recordsAuthoringFailureWithoutRoomOwnership() &&
                  buildsLoadedStateFromPackageRoom() &&
                  buildsLoadedStateFromSavedAuthoredRoom() &&
                  buildsLoadedStateFromEditableRoomSnapshot() &&
                  rejectsUnreadyEditableRoomSnapshot();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
