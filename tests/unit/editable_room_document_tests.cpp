#include "content/authoring/EditableRoomDocument.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/movement/MovementTraversalSlots.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::EditableRoomFloor floorPrimitive() {
  iggy3d::EditableRoomFloor floor;
  floor.id = "floor_1";
  floor.centerMeters = {0.0F, -0.05F, 0.0F};
  floor.sizeMeters = {6.0F, 0.10F, 6.0F};
  floor.semantics = iggy3d::defaultFloorSemantics("stone_floor");
  return floor;
}

iggy3d::EditableRoomWall wallPrimitive() {
  iggy3d::EditableRoomWall wall;
  wall.id = "wall_1";
  wall.startMeters = {-1.0F, 0.0F, -1.0F};
  wall.endMeters = {1.0F, 0.0F, -1.0F};
  wall.bottomY = 0.0F;
  wall.heightMeters = 1.0F;
  wall.thicknessMeters = 0.20F;
  wall.semantics = iggy3d::defaultWallSemantics("stone_wall");
  return wall;
}

bool sessionAddsDeletesAndRestoresPrimitives() {
  iggy3d::EditableRoomSession session;
  const iggy3d::RoomEditResult addFloor =
      session.submit(iggy3d::addFloorCommand(floorPrimitive()));
  const iggy3d::RoomEditResult addWall =
      session.submit(iggy3d::addWallCommand(wallPrimitive()));
  const iggy3d::RoomEditResult duplicate =
      session.submit(iggy3d::addWallCommand(wallPrimitive()));
  const iggy3d::RoomEditResult deleteWall =
      session.submit(iggy3d::deleteWallCommand("wall_1"));
  const iggy3d::RoomEditResult undoDelete = session.undo();
  const bool undoRestoredWall =
      iggy3d::findEditableWall(session.document(), "wall_1") != nullptr;
  const iggy3d::RoomEditResult redoDelete = session.redo();
  const bool redoRemovedWall =
      iggy3d::findEditableWall(session.document(), "wall_1") == nullptr;

  return expect(addFloor.status == iggy3d::RoomEditStatus::Applied, "add floor") &&
         expect(addWall.status == iggy3d::RoomEditStatus::Applied, "add wall") &&
         expect(duplicate.status == iggy3d::RoomEditStatus::DuplicateId,
                "duplicate wall rejected") &&
         expect(deleteWall.status == iggy3d::RoomEditStatus::Applied, "delete wall") &&
         expect(deleteWall.affectedRuntimeIds.size() == 3U, "wall delete affected ids") &&
         expect(deleteWall.affectedRuntimeIds[0] == "wall_1", "wall affected mesh") &&
         expect(deleteWall.affectedRuntimeIds[1] == "wall_1_actor_blocker",
                "wall affected actor blocker") &&
         expect(deleteWall.affectedRuntimeIds[2] == "wall_1_projectile_blocker",
                "wall affected projectile blocker") &&
         expect(undoDelete.status == iggy3d::RoomEditStatus::UndoApplied, "undo delete") &&
         expect(undoRestoredWall, "undo restored wall") &&
         expect(redoDelete.status == iggy3d::RoomEditStatus::RedoApplied, "redo delete") &&
         expect(redoRemovedWall, "redo removed wall");
}

bool semanticsBakeIntoRuntimeSurfaces() {
  iggy3d::EditableRoomSession session;
  (void)session.submit(iggy3d::addFloorCommand(floorPrimitive()));
  (void)session.submit(iggy3d::addWallCommand(wallPrimitive()));

  iggy3d::EditableRoomSemantics clamberWall = iggy3d::defaultWallSemantics("stone_wall");
  clamberWall.traversalTags = {"clamber"};
  const iggy3d::RoomEditResult semanticEdit =
      session.submit(iggy3d::setWallSemanticsCommand("wall_1", clamberWall));
  const iggy3d::RoomBakeResult bake = iggy3d::bakeEditableRoomDocument(session.document());

  if (!expect(semanticEdit.status == iggy3d::RoomEditStatus::Applied, "semantic edit") ||
      !expect(bake.ok, "bake ok")) {
    return false;
  }

  bool floorMesh = false;
  bool wallMesh = false;
  bool floorWalkable = false;
  bool wallActorBlocker = false;
  bool wallProjectileBlocker = false;
  bool wallTopWalkable = false;
  for (const iggy3d::RoomStaticMeshAsset& mesh : bake.room.staticMeshes) {
    floorMesh = floorMesh || (mesh.id == "floor_1" && mesh.role == "floor" &&
                              mesh.materialId == "stone_floor");
    wallMesh = wallMesh || (mesh.id == "wall_1" && mesh.role == "wall" &&
                            mesh.materialId == "stone_wall");
  }
  for (const iggy3d::RoomSpatialSurface& surface : bake.room.spatialSurfaces) {
    if (surface.id == "floor_1_walkable") {
      floorWalkable = surface.role == iggy3d::RoomSpatialSurfaceRole::Walkable &&
                      surface.traversalTags.size() == 1U &&
                      surface.traversalTags.front() == "walkable";
    }
    if (surface.id == "wall_1_actor_blocker") {
      wallActorBlocker = surface.role == iggy3d::RoomSpatialSurfaceRole::Blocker &&
                         surface.blocksActor &&
                         surface.traversalTags.size() == 2U &&
                         surface.traversalTags[0] == "blocker" &&
                         surface.traversalTags[1] == "clamber";
    }
    if (surface.id == "wall_1_projectile_blocker") {
      wallProjectileBlocker =
          surface.role == iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker &&
          surface.blocksProjectile &&
          surface.traversalTags.size() == 1U &&
          surface.traversalTags.front() == "projectile_blocker";
    }
    if (surface.id == "wall_1_top_walkable") {
      wallTopWalkable = surface.role == iggy3d::RoomSpatialSurfaceRole::Walkable &&
                        surface.traversalTags.size() == 2U &&
                        surface.traversalTags[0] == "walkable" &&
                        surface.traversalTags[1] == "clamber";
    }
  }

  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(bake.room);
  const iggy3d::MovementTraversalSlotRegistry slots =
      iggy3d::buildMovementTraversalSlotRegistry(bake.room, {});

  return expect(floorMesh, "floor mesh") && expect(wallMesh, "wall mesh") &&
         expect(floorWalkable, "floor walkable surface") &&
         expect(wallActorBlocker, "wall actor blocker") &&
         expect(wallProjectileBlocker, "wall projectile blocker") &&
         expect(wallTopWalkable, "wall top walkable") &&
         expect(surfaces.size() == bake.room.spatialSurfaces.size(), "collision surfaces") &&
         expect(slots.slots.size() == 1U, "clamber slot baked") &&
         expect(slots.slots.front().slotId == "wall_1:wall_1_top_walkable",
                "clamber slot id") &&
         expect(slots.slots.front().authoredAffordance, "clamber slot authored");
}

bool validationRejectsAmbiguousWallGeometryAndBadTags() {
  iggy3d::EditableRoomSession session;
  iggy3d::EditableRoomWall diagonal = wallPrimitive();
  diagonal.id = "diagonal_wall";
  diagonal.endMeters = {1.0F, 0.0F, 1.0F};
  const iggy3d::RoomEditResult badWall =
      session.submit(iggy3d::addWallCommand(diagonal));

  iggy3d::EditableRoomFloor floor = floorPrimitive();
  floor.id = "bad_floor";
  floor.semantics.traversalTags = {"mystery_tag"};
  const iggy3d::RoomEditResult badFloor =
      session.submit(iggy3d::addFloorCommand(floor));

  return expect(badWall.status == iggy3d::RoomEditStatus::InvalidPrimitive,
                "diagonal wall rejected") &&
         expect(badFloor.status == iggy3d::RoomEditStatus::InvalidPrimitive,
                "bad floor tag rejected") &&
         expect(session.document().floors.empty(), "no bad floor mutation") &&
         expect(session.document().walls.empty(), "no bad wall mutation");
}

bool nextEditableCountersIgnoreNonMatchingIds() {
  iggy3d::EditableRoomDocument document;
  iggy3d::EditableRoomFloor floor = floorPrimitive();
  floor.id = "edit_floor_7";
  document.floors.push_back(floor);
  floor.id = "floor_misc_99";
  document.floors.push_back(floor);
  floor.id = "edit_floor_bad";
  document.floors.push_back(floor);

  iggy3d::EditableRoomWall wall = wallPrimitive();
  wall.id = "edit_wall_12";
  document.walls.push_back(wall);
  wall.id = "wall_misc_99";
  document.walls.push_back(wall);
  wall.id = "edit_wall_bad";
  document.walls.push_back(wall);

  return expect(iggy3d::nextEditableFloorIndex(document) == 8U,
                "floor counter from numeric suffix") &&
         expect(iggy3d::nextEditableWallIndex(document) == 13U,
                "wall counter from numeric suffix");
}

bool lockedHiddenAndProjectileSemanticsAreStable() {
  iggy3d::EditableRoomSession session;
  iggy3d::EditableRoomFloor floor = floorPrimitive();
  floor.id = "hidden_floor";
  floor.hidden = true;
  floor.locked = true;
  const iggy3d::RoomEditResult addFloor =
      session.submit(iggy3d::addFloorCommand(floor));
  const iggy3d::RoomEditResult lockedDelete =
      session.submit(iggy3d::deleteFloorCommand("hidden_floor"));

  iggy3d::EditableRoomWall wall = wallPrimitive();
  wall.id = "projectile_wall";
  wall.semantics.materialId = "debug_wall";
  wall.semantics.blocksActor = false;
  wall.semantics.blocksProjectile = true;
  const iggy3d::RoomEditResult addWall =
      session.submit(iggy3d::addWallCommand(wall));
  const iggy3d::RoomBakeResult bake = iggy3d::bakeEditableRoomDocument(session.document());

  bool projectileOnly = false;
  for (const iggy3d::RoomSpatialSurface& surface : bake.room.spatialSurfaces) {
    if (surface.id == "projectile_wall_projectile_blocker") {
      projectileOnly = surface.blocksProjectile && !surface.blocksActor &&
                       surface.role == iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker;
    }
  }

  return expect(addFloor.status == iggy3d::RoomEditStatus::Applied,
                "locked floor added") &&
         expect(lockedDelete.status == iggy3d::RoomEditStatus::LockedPrimitive,
                "locked floor delete rejected") &&
         expect(addWall.status == iggy3d::RoomEditStatus::Applied,
                "projectile wall added") &&
         expect(bake.ok, "hidden/projectile bake ok") &&
         expect(projectileOnly, "projectile-only blocker baked");
}

}  // namespace

int main() {
  const bool ok = sessionAddsDeletesAndRestoresPrimitives() &&
                  semanticsBakeIntoRuntimeSurfaces() &&
                  validationRejectsAmbiguousWallGeometryAndBadTags() &&
                  nextEditableCountersIgnoreNonMatchingIds() &&
                  lockedHiddenAndProjectileSemanticsAreStable();
  return ok ? 0 : 1;
}
