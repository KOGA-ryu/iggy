#include "app/iggy3d/gameplay/ProductActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/ascii_room/ProductAsciiRoomAuthoring.hpp"
#include "app/iggy3d/ascii_room/ProductAsciiRoomEditing.hpp"
#include "runtime/collision/CollisionQuery.hpp"
#include "runtime/world/EntityState.hpp"
#include "runtime/session/SessionState.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

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

iggy3d::ProductActiveRoomState trainingActiveRoom() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText = std::string(kTrainingRoom);
  request.roomId = "collision_training_room";
  request.sourceName = "unit/collision_training_room.iggyroom.txt";
  const iggy3d::ProductAsciiRoomAuthoringResult authoring =
      iggy3d::buildProductAsciiRoomAuthoring(request);
  return iggy3d::buildProductActiveRoomFromAsciiAuthoring(request, authoring);
}

iggy3d::ProductActiveRoomState editedActiveRoom() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText =
      "###\n"
      "#P#\n"
      "###\n";
  request.roomId = "collision_editable_room";
  request.sourceName = "unit/collision_editable_room.iggyroom.txt";
  request.emitAssetText = false;
  iggy3d::ProductAsciiRoomEditingResult editing =
      iggy3d::buildProductAsciiRoomEditing(request);

  iggy3d::EditableRoomFloor floor;
  floor.id = "collision_edit_floor_1";
  floor.centerMeters = {2.0F, -0.05F, 0.0F};
  floor.sizeMeters = {1.0F, 0.10F, 1.0F};
  floor.semantics = iggy3d::defaultFloorSemantics("debug_floor");
  (void)editing.controller.submit(iggy3d::ProductRoomAuthoringInputSource::Ai,
                                  iggy3d::addFloorCommand(floor));

  iggy3d::EditableRoomWall wall;
  wall.id = "collision_edit_wall_1";
  wall.startMeters = {2.0F, 0.0F, -0.5F};
  wall.endMeters = {3.0F, 0.0F, -0.5F};
  wall.bottomY = 0.0F;
  wall.heightMeters = 2.50F;
  wall.thicknessMeters = 1.0F;
  wall.semantics = iggy3d::defaultWallSemantics("debug_wall");
  (void)editing.controller.submit(iggy3d::ProductRoomAuthoringInputSource::Mouse,
                                  iggy3d::addWallCommand(wall));

  return iggy3d::buildProductActiveRoomFromRoomAuthoringSnapshot(
      editing.controller.snapshot());
}

iggy3d::SessionState runtimeStateWithDoor(bool doorActive) {
  iggy3d::SessionState state;
  iggy3d::EntityState door;
  door.id = iggy3d::EntityId{2};
  door.stableName = "marker_door_r2_c2";
  door.kind = iggy3d::EntityKind::Door;
  door.active = doorActive;
  state.world.seedEntity(std::move(door));
  return state;
}

bool buildsCollisionFromLoadedAsciiRoom() {
  const iggy3d::ProductActiveRoomState active = trainingActiveRoom();
  const iggy3d::ProductActiveRoomCollisionState collision =
      iggy3d::buildProductActiveRoomCollision(active);
  const iggy3d::SpatialSurfaceSet* surfaces =
      iggy3d::productActiveRoomCollisionSurfaces(collision);
  const iggy3d::CollisionQueryResult wallHit =
      surfaces == nullptr
          ? iggy3d::CollisionQueryResult{}
          : iggy3d::querySegment(*surfaces,
                                 {1.5F, 0.0F, 1.5F},
                                 {1.5F, 0.0F, -2.25F},
                                 iggy3d::CollisionQueryKind::Actor);

  return expect(active.loaded, "active room loaded") &&
         expect(collision.ready, "collision ready") &&
         expect(collision.status == "active_room_collision_ready", "status") &&
         expect(collision.reasonCode == "active_room_collision_ready", "reason") &&
         expect(collision.roomId == "collision_training_room", "room id") &&
         expect(collision.spatialSurfaceCount == 56U, "authored surface count") &&
         expect(collision.querySurfaceCount == 56U, "query surface count") &&
         expect(collision.walkableSurfaceCount == 15U, "walkable count") &&
         expect(collision.actorBlockerSurfaceCount == 21U,
                "actor blocker count") &&
         expect(collision.projectileBlockerSurfaceCount == 21U,
                "projectile blocker count") &&
         expect(collision.runtimeOwnedSurfaceCount == 1U,
                "runtime owned surface count") &&
         expect(collision.runtimeFilteredSurfaceCount == 0U,
                "runtime filtered count") &&
         expect(collision.doorBlockerSurfaceCount == 0U,
                "no-runtime door blocker count") &&
         expect(surfaces != nullptr, "collision pointer available") &&
         expect(wallHit.status == iggy3d::CollisionQueryStatus::Hit,
                "actor query hits wall") &&
         expect(wallHit.role == iggy3d::CollisionSurfaceRole::Blocker,
                "actor query hits blocker");
}

bool runtimeDoorStateFiltersDoorCollision() {
  const iggy3d::ProductActiveRoomState active = trainingActiveRoom();
  const iggy3d::SessionState closedState = runtimeStateWithDoor(true);
  const iggy3d::SessionState openState = runtimeStateWithDoor(false);
  const iggy3d::ProductActiveRoomCollisionState closedCollision =
      iggy3d::buildProductActiveRoomCollision(active, closedState);
  const iggy3d::ProductActiveRoomCollisionState openCollision =
      iggy3d::buildProductActiveRoomCollision(active, openState);
  const iggy3d::SpatialSurfaceSet* closedSurfaces =
      iggy3d::productActiveRoomCollisionSurfaces(closedCollision);
  const iggy3d::SpatialSurfaceSet* openSurfaces =
      iggy3d::productActiveRoomCollisionSurfaces(openCollision);
  const iggy3d::CollisionQueryResult closedDoorHit =
      closedSurfaces == nullptr
          ? iggy3d::CollisionQueryResult{}
          : iggy3d::querySegment(*closedSurfaces,
                                 {-2.0F, 0.0F, -1.0F},
                                 {-1.0F, 0.0F, 0.0F},
                                 iggy3d::CollisionQueryKind::Actor);
  const iggy3d::CollisionQueryResult openDoorStride =
      openSurfaces == nullptr
          ? iggy3d::CollisionQueryResult{}
          : iggy3d::querySegment(*openSurfaces,
                                 {-2.0F, 0.0F, -1.0F},
                                 {-1.0F, 0.0F, 0.0F},
                                 iggy3d::CollisionQueryKind::Actor);

  return expect(closedCollision.ready, "closed collision ready") &&
         expect(openCollision.ready, "open collision ready") &&
         expect(closedCollision.runtimeOwnedSurfaceCount == 1U,
                "closed runtime owned count") &&
         expect(closedCollision.runtimeFilteredSurfaceCount == 0U,
                "closed filtered count") &&
         expect(closedCollision.doorBlockerSurfaceCount == 1U,
                "closed door blocker count") &&
         expect(closedCollision.activeDoorBlockerSurfaceCount == 1U,
                "closed active door blocker count") &&
         expect(closedCollision.querySurfaceCount == 56U,
                "closed query surface count") &&
         expect(closedCollision.actorBlockerSurfaceCount == 21U,
                "closed actor blocker count") &&
         expect(openCollision.runtimeOwnedSurfaceCount == 1U,
                "open runtime owned count") &&
         expect(openCollision.runtimeFilteredSurfaceCount == 1U,
                "open filtered count") &&
         expect(openCollision.doorBlockerSurfaceCount == 1U,
                "open door blocker count") &&
         expect(openCollision.activeDoorBlockerSurfaceCount == 0U,
                "open active door blocker count") &&
         expect(openCollision.querySurfaceCount == 55U,
                "open query surface count") &&
         expect(openCollision.actorBlockerSurfaceCount == 20U,
                "open actor blocker count") &&
         expect(closedDoorHit.status == iggy3d::CollisionQueryStatus::Hit,
                "closed door hit") &&
         expect(closedDoorHit.surfaceId == "marker_door_r2_c2_door_blocker",
                "closed door surface id") &&
         expect(openDoorStride.status == iggy3d::CollisionQueryStatus::NoHit,
                "open door stride clear");
}

bool buildsCollisionFromEditedRoomSnapshot() {
  const iggy3d::ProductActiveRoomState active = editedActiveRoom();
  const iggy3d::ProductActiveRoomCollisionState collision =
      iggy3d::buildProductActiveRoomCollision(active);
  const iggy3d::SpatialSurfaceSet* surfaces =
      iggy3d::productActiveRoomCollisionSurfaces(collision);
  const iggy3d::CollisionQueryResult wallHit =
      surfaces == nullptr
          ? iggy3d::CollisionQueryResult{}
          : iggy3d::querySegment(*surfaces,
                                 {2.5F, 0.0F, 0.25F},
                                 {2.5F, 0.0F, -1.25F},
                                 iggy3d::CollisionQueryKind::Actor);

  return expect(active.loaded, "edited active room loaded") &&
         expect(active.source == "editable_room", "edited active room source") &&
         expect(collision.ready, "edited collision ready") &&
         expect(collision.status == "active_room_collision_ready",
                "edited collision status") &&
         expect(collision.reasonCode == "active_room_collision_ready",
                "edited collision reason") &&
         expect(collision.roomId == "collision_editable_room",
                "edited collision room id") &&
         expect(collision.spatialSurfaceCount == 20U,
                "edited authored surface count") &&
         expect(collision.querySurfaceCount == 20U,
                "edited query surface count") &&
         expect(collision.walkableSurfaceCount == 2U,
                "edited walkable count") &&
         expect(collision.actorBlockerSurfaceCount == 9U,
                "edited actor blocker count") &&
         expect(collision.projectileBlockerSurfaceCount == 9U,
                "edited projectile blocker count") &&
         expect(collision.runtimeOwnedSurfaceCount == 0U,
                "edited runtime owned count") &&
         expect(surfaces != nullptr, "edited surfaces available") &&
         expect(wallHit.status == iggy3d::CollisionQueryStatus::Hit,
                "edited wall hit") &&
         expect(wallHit.role == iggy3d::CollisionSurfaceRole::Blocker,
                "edited wall blocker");
}

bool rejectsUnloadedActiveRoom() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText =
      "...\n"
      "...\n";
  request.roomId = "missing_spawn_collision_room";
  request.sourceName = "unit/missing_spawn_collision_room.iggyroom.txt";
  const iggy3d::ProductAsciiRoomAuthoringResult authoring =
      iggy3d::buildProductAsciiRoomAuthoring(request);
  const iggy3d::ProductActiveRoomState active =
      iggy3d::buildProductActiveRoomFromAsciiAuthoring(request, authoring);
  const iggy3d::ProductActiveRoomCollisionState collision =
      iggy3d::buildProductActiveRoomCollision(active);

  return expect(!active.loaded, "active room not loaded") &&
         expect(!collision.ready, "collision not ready") &&
         expect(collision.status == "active_room_collision_unavailable",
                "unavailable status") &&
         expect(collision.reasonCode == "ascii_room_missing_player_spawn",
                "failure reason") &&
         expect(collision.roomId == "missing_spawn_collision_room",
                "room id preserved") &&
         expect(collision.querySurfaceCount == 0U, "no query surfaces") &&
         expect(iggy3d::productActiveRoomCollisionSurfaces(collision) == nullptr,
                "no surface pointer");
}

bool rejectsLoadedRoomWithoutSurfaces() {
  iggy3d::ProductActiveRoomState active;
  active.loaded = true;
  active.status = "active_room_loaded";
  active.reasonCode = "active_room_loaded";
  active.roomId = "empty_collision_room";
  active.room.id = "empty_collision_room";

  const iggy3d::ProductActiveRoomCollisionState collision =
      iggy3d::buildProductActiveRoomCollision(active);

  return expect(!collision.ready, "empty collision not ready") &&
         expect(collision.status == "active_room_collision_unavailable",
                "empty unavailable status") &&
         expect(collision.reasonCode == "active_room_collision_missing_surfaces",
                "missing surfaces reason") &&
         expect(collision.roomId == "empty_collision_room",
                "empty room id") &&
         expect(collision.querySurfaceCount == 0U, "empty query count") &&
         expect(iggy3d::productActiveRoomCollisionSurfaces(collision) == nullptr,
                "empty pointer unavailable");
}

}  // namespace

int main() {
  const bool ok = buildsCollisionFromLoadedAsciiRoom() &&
                  runtimeDoorStateFiltersDoorCollision() &&
                  buildsCollisionFromEditedRoomSnapshot() &&
                  rejectsUnloadedActiveRoom() &&
                  rejectsLoadedRoomWithoutSurfaces();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
