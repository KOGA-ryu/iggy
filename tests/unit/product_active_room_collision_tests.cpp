#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/ascii_room/Authoring.hpp"
#include "app/iggy3d/ascii_room/Editing.hpp"
#include "runtime/collision/CollisionQuery.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/world/EntityState.hpp"
#include "runtime/session/SessionState.hpp"

#include <cstdlib>
#include <iostream>
#include <initializer_list>
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

bool collisionProvenanceDefaultsToZero() {
  const iggy3d::ProductActiveRoomCollisionState collision;
  return expect(collision.bakedFromRoomRevision == 0U,
                "default baked room revision") &&
         expect(collision.bakedFromSessionHash == 0U,
                "default baked session hash");
}

iggy3d::RoomSpatialSurface walkableSurface(std::string id) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = std::move(id);
  surface.sourceStaticMeshId = surface.id + "_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {{0.0F, 0.0F, 0.0F},
                          {1.0F, 0.0F, 0.0F},
                          {1.0F, 0.0F, 1.0F},
                          {0.0F, 0.0F, 1.0F}};
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {"walkable"};
  return surface;
}

iggy3d::RoomSpatialSurface blockerSurface(std::string id,
                                          std::string runtimeOwnerStableName = {}) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = std::move(id);
  surface.sourceStaticMeshId = surface.id + "_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {{0.0F, 0.0F, 0.0F}, {1.0F, 2.0F, 1.0F}};
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.traversalTags = {"blocker"};
  surface.blocksActor = true;
  surface.runtimeOwnerStableName = std::move(runtimeOwnerStableName);
  return surface;
}

iggy3d::ProductActiveRoomState packageActiveRoom(
    std::string id,
    std::initializer_list<iggy3d::RoomSpatialSurface> surfaces) {
  iggy3d::RoomAsset room;
  room.id = std::move(id);
  room.units = "m";
  room.source = "unit";
  room.sourceFile = "unit/freshness_room";
  room.sourceSubset = "collision_freshness";
  room.spatialSurfaces.assign(surfaces.begin(), surfaces.end());
  return iggy3d::buildProductActiveRoomFromPackageRoom(room, "unit", "freshness");
}

iggy3d::Session doorSession(bool doorActive) {
  iggy3d::SessionState state;
  state.lifecycle = iggy3d::SessionLifecycle::Playing;
  state.currentStateHash = 11U;
  iggy3d::EntityState door;
  door.id = iggy3d::EntityId{2};
  door.stableName = "freshness_door";
  door.kind = iggy3d::EntityKind::Door;
  door.active = doorActive;
  (void)state.world.seedEntity(std::move(door));
  return iggy3d::Session(std::move(state));
}

bool idempotentAfterRebake(iggy3d::ProductAppWindowState& window,
                           const iggy3d::Session* session,
                           std::string_view message) {
  const iggy3d::ProductActiveRoomCollisionState before =
      window.activeRoomCollision;
  const iggy3d::ProductActiveRoomCollisionFreshnessResult second =
      iggy3d::ensureActiveRoomCollisionFresh(window, session);
  return expect(!second.rebaked, std::string(message) + " idempotent skip") &&
         expect(second.reasonCode == "skipped_fresh",
                std::string(message) + " idempotent reason") &&
         expect(window.activeRoomCollision.ready == before.ready,
                std::string(message) + " ready unchanged") &&
         expect(window.activeRoomCollision.status == before.status,
                std::string(message) + " status unchanged") &&
         expect(window.activeRoomCollision.reasonCode == before.reasonCode,
                std::string(message) + " collision reason unchanged") &&
         expect(window.activeRoomCollision.roomId == before.roomId,
                std::string(message) + " room id unchanged") &&
         expect(window.activeRoomCollision.querySurfaceCount ==
                    before.querySurfaceCount,
                std::string(message) + " query count unchanged") &&
         expect(window.activeRoomCollision.runtimeFilteredSurfaceCount ==
                    before.runtimeFilteredSurfaceCount,
                std::string(message) + " filtered count unchanged") &&
         expect(window.activeRoomCollision.activeDoorBlockerSurfaceCount ==
                    before.activeDoorBlockerSurfaceCount,
                std::string(message) + " active door count unchanged") &&
         expect(window.activeRoomCollision.bakedFromRoomRevision ==
                    before.bakedFromRoomRevision,
                std::string(message) + " room stamp unchanged") &&
         expect(window.activeRoomCollision.bakedFromSessionHash ==
                    before.bakedFromSessionHash,
                std::string(message) + " session stamp unchanged");
}

bool freshnessStoreRebakesWhenRoomReplaced() {
  iggy3d::ProductAppWindowState window;
  window.activeRoomRevision = 1U;
  window.activeRoom =
      packageActiveRoom("floor_and_crate",
                        {walkableSurface("floor"),
                         blockerSurface("crate_actor"),
                         blockerSurface("crate_projectile")});
  window.activeRoomCollision =
      iggy3d::buildProductActiveRoomCollision(window.activeRoom);
  window.activeRoomCollision.bakedFromRoomRevision = window.activeRoomRevision;
  window.activeRoomCollision.bakedFromSessionHash = 0U;

  const bool baseline =
      expect(window.activeRoomCollision.querySurfaceCount == 3U,
             "room stale baseline query count") &&
      expect(window.activeRoomCollision.bakedFromRoomRevision ==
                 window.activeRoomRevision,
             "room stale baseline provenance");
  window.activeRoom = packageActiveRoom("floor_only", {walkableSurface("floor")});
  iggy3d::bumpActiveRoomRevision(window);

  const bool mismatch =
      expect(window.activeRoom.spatialSurfaceCount == 1U,
             "room stale new authored count") &&
      expect(window.activeRoomCollision.querySurfaceCount == 3U,
             "room stale old query count remains before ensure") &&
      expect(window.activeRoomCollision.bakedFromRoomRevision !=
                 window.activeRoomRevision,
             "room stale provenance mismatch before ensure");

  const iggy3d::ProductActiveRoomCollisionFreshnessResult result =
      iggy3d::ensureActiveRoomCollisionFresh(window, nullptr);
  const iggy3d::SpatialSurfaceSet* surfaces =
      iggy3d::productActiveRoomCollisionSurfaces(window.activeRoomCollision);

  return baseline && mismatch &&
         expect(result.rebaked, "room stale rebaked") &&
         expect(result.reasonCode == "rebaked_room", "room stale reason") &&
         expect(window.activeRoomCollision.querySurfaceCount == 1U,
                "room stale query count refreshed") &&
         expect(window.activeRoomCollision.roomId == "floor_only",
                "room stale room id refreshed") &&
         expect(window.activeRoomCollision.bakedFromRoomRevision ==
                    window.activeRoomRevision,
                "room stale room revision stamped") &&
         expect(window.activeRoomCollision.bakedFromSessionHash == 0U,
                "room stale null session hash stamped") &&
         expect(surfaces != nullptr && surfaces->size() == 1U,
                "room stale reader surface count") &&
         idempotentAfterRebake(window, nullptr, "room stale");
}

bool freshnessStoreRebakesWhenSessionHashChanges() {
  iggy3d::ProductAppWindowState window;
  window.activeRoomRevision = 1U;
  window.activeRoom =
      packageActiveRoom("door_room",
                        {walkableSurface("floor"),
                         blockerSurface("door_blocker", "freshness_door")});
  iggy3d::Session session = doorSession(true);
  window.activeRoomCollision =
      iggy3d::buildProductActiveRoomCollision(window.activeRoom, session.state());
  window.activeRoomCollision.bakedFromRoomRevision = window.activeRoomRevision;
  window.activeRoomCollision.bakedFromSessionHash =
      session.state().currentStateHash;

  const std::uint64_t baselineHash = session.state().currentStateHash;
  const bool baseline =
      expect(window.activeRoomCollision.runtimeOwnedSurfaceCount == 1U,
             "session stale baseline runtime owned") &&
      expect(window.activeRoomCollision.activeDoorBlockerSurfaceCount == 1U,
             "session stale baseline active door") &&
      expect(window.activeRoomCollision.runtimeFilteredSurfaceCount == 0U,
             "session stale baseline no filter") &&
      expect(window.activeRoomCollision.querySurfaceCount == 2U,
             "session stale baseline query count");

  const iggy3d::WorldMutationResult mutation =
      session.mutableStateForOwnedSystems().world.setActive({2}, false);
  const iggy3d::StatusResult ticked = session.tick();
  const bool changed =
      expect(mutation.status == iggy3d::WorldStatus::Ok,
             "session stale door mutation") &&
      expect(ticked.status == iggy3d::ResultStatus::Ok,
             "session stale non-interact tick") &&
      expect(session.state().currentStateHash != baselineHash,
             "session stale hash changed");
  const iggy3d::ProductActiveRoomCollisionFreshnessResult result =
      iggy3d::ensureActiveRoomCollisionFresh(window, &session);

  return baseline && changed &&
         expect(result.rebaked, "session stale rebaked") &&
         expect(result.reasonCode == "rebaked_session", "session stale reason") &&
         expect(result.observedSessionHash == session.state().currentStateHash,
                "session stale observed hash") &&
         expect(window.activeRoomCollision.runtimeOwnedSurfaceCount == 1U,
                "session stale two-arg overload used") &&
         expect(window.activeRoomCollision.runtimeFilteredSurfaceCount == 1U,
                "session stale runtime filtered") &&
         expect(window.activeRoomCollision.activeDoorBlockerSurfaceCount == 0U,
                "session stale active door cleared") &&
         expect(window.activeRoomCollision.querySurfaceCount == 1U,
                "session stale query count drops") &&
         expect(window.activeRoomCollision.bakedFromRoomRevision ==
                    window.activeRoomRevision,
                "session stale room stamp") &&
         expect(window.activeRoomCollision.bakedFromSessionHash ==
                    session.state().currentStateHash,
                "session stale session stamp") &&
         idempotentAfterRebake(window, &session, "session stale");
}

bool freshnessStoreRebakesUnloadedRoom() {
  iggy3d::ProductAppWindowState window;
  window.activeRoomRevision = 1U;
  window.activeRoom = packageActiveRoom("loaded_room", {walkableSurface("floor")});
  window.activeRoomCollision =
      iggy3d::buildProductActiveRoomCollision(window.activeRoom);
  window.activeRoomCollision.bakedFromRoomRevision = window.activeRoomRevision;
  window.activeRoomCollision.bakedFromSessionHash = 0U;

  const bool baseline =
      expect(window.activeRoom.loaded, "unloaded stale baseline loaded") &&
      expect(window.activeRoomCollision.ready, "unloaded stale baseline ready");
  window.activeRoom = {};
  iggy3d::bumpActiveRoomRevision(window);
  const bool stale =
      expect(!window.activeRoom.loaded, "unloaded stale room cleared") &&
      expect(window.activeRoomCollision.ready,
             "unloaded stale collision still ready before ensure");

  const iggy3d::ProductActiveRoomCollisionFreshnessResult result =
      iggy3d::ensureActiveRoomCollisionFresh(window, nullptr);

  return baseline && stale &&
         expect(result.rebaked, "unloaded stale rebaked") &&
         expect(result.reasonCode == "rebaked_unloaded",
                "unloaded stale reason") &&
         expect(!window.activeRoomCollision.ready,
                "unloaded stale collision unavailable") &&
         expect(window.activeRoomCollision.status ==
                    "active_room_collision_unavailable",
                "unloaded stale status") &&
         expect(window.activeRoomCollision.reasonCode == "not_loaded",
                "unloaded stale collision reason from active room") &&
         expect(window.activeRoomCollision.querySurfaceCount == 0U,
                "unloaded stale query count") &&
         expect(iggy3d::productActiveRoomCollisionSurfaces(
                    window.activeRoomCollision) == nullptr,
                "unloaded stale no surface pointer") &&
         expect(window.activeRoomCollision.bakedFromRoomRevision ==
                    window.activeRoomRevision,
                "unloaded stale room stamp") &&
         expect(window.activeRoomCollision.bakedFromSessionHash == 0U,
                "unloaded stale session stamp") &&
         idempotentAfterRebake(window, nullptr, "unloaded stale");
}

bool freshnessStoreSkipsFreshCollision() {
  iggy3d::ProductAppWindowState window;
  window.activeRoomRevision = 5U;
  window.activeRoom = packageActiveRoom("fresh_room", {walkableSurface("floor")});
  window.activeRoomCollision =
      iggy3d::buildProductActiveRoomCollision(window.activeRoom);
  window.activeRoomCollision.bakedFromRoomRevision = window.activeRoomRevision;
  window.activeRoomCollision.bakedFromSessionHash = 0U;

  const iggy3d::ProductActiveRoomCollisionFreshnessResult result =
      iggy3d::ensureActiveRoomCollisionFresh(window, nullptr);
  return expect(!result.rebaked, "fresh skip no rebake") &&
         expect(result.reasonCode == "skipped_fresh", "fresh skip reason") &&
         expect(window.activeRoomCollision.querySurfaceCount == 1U,
                "fresh skip query count unchanged") &&
         expect(window.activeRoomCollision.bakedFromRoomRevision == 5U,
                "fresh skip room stamp unchanged") &&
         expect(window.activeRoomCollision.bakedFromSessionHash == 0U,
                "fresh skip session stamp unchanged");
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
  const bool ok = collisionProvenanceDefaultsToZero() &&
                  freshnessStoreSkipsFreshCollision() &&
                  freshnessStoreRebakesWhenRoomReplaced() &&
                  freshnessStoreRebakesWhenSessionHashChanges() &&
                  freshnessStoreRebakesUnloadedRoom() &&
                  buildsCollisionFromLoadedAsciiRoom() &&
                  runtimeDoorStateFiltersDoorCollision() &&
                  buildsCollisionFromEditedRoomSnapshot() &&
                  rejectsUnloadedActiveRoom() &&
                  rejectsLoadedRoomWithoutSurfaces();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
