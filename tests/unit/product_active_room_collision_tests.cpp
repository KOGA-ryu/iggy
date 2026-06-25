#include "app/iggy3d/ProductActiveRoomCollision.hpp"
#include "app/iggy3d/ProductActiveRoomState.hpp"
#include "app/iggy3d/ProductAsciiRoomAuthoring.hpp"
#include "runtime/collision/CollisionQuery.hpp"

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

iggy3d::ProductActiveRoomState trainingActiveRoom() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceText = std::string(kTrainingRoom);
  request.roomId = "collision_training_room";
  request.sourceName = "unit/collision_training_room.iggyroom.txt";
  const iggy3d::ProductAsciiRoomAuthoringResult authoring =
      iggy3d::buildProductAsciiRoomAuthoring(request);
  return iggy3d::buildProductActiveRoomFromAsciiAuthoring(request, authoring);
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
         expect(collision.spatialSurfaceCount == 55U, "authored surface count") &&
         expect(collision.querySurfaceCount == 55U, "query surface count") &&
         expect(collision.walkableSurfaceCount == 15U, "walkable count") &&
         expect(collision.actorBlockerSurfaceCount == 20U,
                "actor blocker count") &&
         expect(collision.projectileBlockerSurfaceCount == 20U,
                "projectile blocker count") &&
         expect(surfaces != nullptr, "collision pointer available") &&
         expect(wallHit.status == iggy3d::CollisionQueryStatus::Hit,
                "actor query hits wall") &&
         expect(wallHit.role == iggy3d::CollisionSurfaceRole::Blocker,
                "actor query hits blocker");
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
                  rejectsUnloadedActiveRoom() &&
                  rejectsLoadedRoomWithoutSurfaces();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
