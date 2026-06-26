#include "content/authoring/EditableRoomDocument.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/movement/MovementTraversalSlots.hpp"

#include <cmath>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool nearly(float actual, float expected) {
  return std::fabs(actual - expected) <= 0.001F;
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

const iggy3d::RoomStaticMeshAsset* findMesh(const iggy3d::RoomAsset& room,
                                            std::string_view id) {
  for (const iggy3d::RoomStaticMeshAsset& mesh : room.staticMeshes) {
    if (mesh.id == id) {
      return &mesh;
    }
  }
  return nullptr;
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

bool wallBakePreservesSegmentMetadataAndSurfaceCounts() {
  iggy3d::EditableRoomDocument document;
  iggy3d::EditableRoomWall xWall = wallPrimitive();
  xWall.id = "wall_x";
  xWall.startMeters = {-1.0F, 0.0F, -2.0F};
  xWall.endMeters = {2.0F, 0.0F, -2.0F};
  xWall.bottomY = 0.25F;
  xWall.heightMeters = 2.5F;
  xWall.thicknessMeters = 0.35F;
  document.walls.push_back(xWall);

  iggy3d::EditableRoomWall zWall = wallPrimitive();
  zWall.id = "wall_z";
  zWall.startMeters = {4.0F, 0.0F, -1.0F};
  zWall.endMeters = {4.0F, 0.0F, 3.0F};
  zWall.bottomY = 0.0F;
  zWall.heightMeters = 3.0F;
  zWall.thicknessMeters = 0.5F;
  document.walls.push_back(zWall);

  const iggy3d::RoomBakeResult bake = iggy3d::bakeEditableRoomDocument(document);
  const iggy3d::RoomStaticMeshAsset* xMesh = findMesh(bake.room, "wall_x");
  const iggy3d::RoomStaticMeshAsset* zMesh = findMesh(bake.room, "wall_z");

  return expect(bake.ok, "segment metadata bake ok") &&
         expect(bake.room.staticMeshes.size() == 2U, "segment mesh count") &&
         expect(bake.room.spatialSurfaces.size() == 4U,
                "segment surface count unchanged") &&
         expect(xMesh != nullptr, "x wall mesh exists") &&
         expect(xMesh->hasWallSegment, "x wall segment present") &&
         expect(nearly(xMesh->wallStartMeters.x, -1.0F), "x wall start x") &&
         expect(nearly(xMesh->wallStartMeters.z, -2.0F), "x wall start z") &&
         expect(nearly(xMesh->wallEndMeters.x, 2.0F), "x wall end x") &&
         expect(nearly(xMesh->wallEndMeters.z, -2.0F), "x wall end z") &&
         expect(nearly(xMesh->wallBottomY, 0.25F), "x wall bottom") &&
         expect(nearly(xMesh->wallHeightMeters, 2.5F), "x wall height") &&
         expect(nearly(xMesh->wallThicknessMeters, 0.35F), "x wall thickness") &&
         expect(nearly(xMesh->positionMeters.x, 0.5F), "x wall fallback center x") &&
         expect(nearly(xMesh->sizeMeters.x, 3.0F), "x wall fallback size x") &&
         expect(zMesh != nullptr, "z wall mesh exists") &&
         expect(zMesh->hasWallSegment, "z wall segment present") &&
         expect(nearly(zMesh->wallStartMeters.x, 4.0F), "z wall start x") &&
         expect(nearly(zMesh->wallStartMeters.z, -1.0F), "z wall start z") &&
         expect(nearly(zMesh->wallEndMeters.x, 4.0F), "z wall end x") &&
         expect(nearly(zMesh->wallEndMeters.z, 3.0F), "z wall end z") &&
         expect(nearly(zMesh->wallBottomY, 0.0F), "z wall bottom") &&
         expect(nearly(zMesh->wallHeightMeters, 3.0F), "z wall height") &&
         expect(nearly(zMesh->wallThicknessMeters, 0.5F), "z wall thickness") &&
         expect(nearly(zMesh->positionMeters.z, 1.0F), "z wall fallback center z") &&
         expect(nearly(zMesh->sizeMeters.z, 4.0F), "z wall fallback size z");
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

bool floorMoveResizeUndoRedoAndValidation() {
  iggy3d::EditableRoomSession session;
  (void)session.submit(iggy3d::addFloorCommand(floorPrimitive()));

  const iggy3d::RoomEditResult move =
      session.submit(iggy3d::moveFloorCommand("floor_1", {1.0F, 0.0F, -0.5F}));
  const iggy3d::EditableRoomFloor* movedFloor =
      iggy3d::findEditableFloor(session.document(), "floor_1");
  const bool movedFloorOk = movedFloor != nullptr &&
                            nearly(movedFloor->centerMeters.x, 1.0F) &&
                            nearly(movedFloor->centerMeters.y, -0.05F) &&
                            nearly(movedFloor->centerMeters.z, -0.5F);
  const iggy3d::RoomEditResult resize =
      session.submit(iggy3d::resizeFloorCommand("floor_1", {4.0F, 0.0F, 3.0F}));
  const iggy3d::EditableRoomFloor* resizedFloor =
      iggy3d::findEditableFloor(session.document(), "floor_1");
  const bool resizedFloorOk = resizedFloor != nullptr &&
                              nearly(resizedFloor->sizeMeters.x, 4.0F) &&
                              nearly(resizedFloor->sizeMeters.y, 0.10F) &&
                              nearly(resizedFloor->sizeMeters.z, 3.0F);
  const iggy3d::RoomEditResult undoResize = session.undo();
  const iggy3d::EditableRoomFloor* undoFloor =
      iggy3d::findEditableFloor(session.document(), "floor_1");
  const bool undoFloorOk = undoFloor != nullptr && nearly(undoFloor->sizeMeters.x, 6.0F);
  const iggy3d::RoomEditResult redoResize = session.redo();
  const iggy3d::EditableRoomFloor* redoFloor =
      iggy3d::findEditableFloor(session.document(), "floor_1");
  const bool redoFloorOk = redoFloor != nullptr && nearly(redoFloor->sizeMeters.x, 4.0F);
  const iggy3d::RoomEditResult badResize =
      session.submit(iggy3d::resizeFloorCommand("floor_1", {0.0F, 0.0F, 3.0F}));
  const iggy3d::RoomEditResult badMove =
      session.submit(iggy3d::moveFloorCommand("floor_1", {0.0F, 0.5F, 0.0F}));

  return expect(move.status == iggy3d::RoomEditStatus::Applied, "floor move applied") &&
         expect(movedFloorOk, "floor moved on xz only") &&
         expect(resize.status == iggy3d::RoomEditStatus::Applied, "floor resize applied") &&
         expect(resizedFloorOk, "floor resized xz only") &&
         expect(undoResize.status == iggy3d::RoomEditStatus::UndoApplied,
                "floor resize undo") &&
         expect(undoFloorOk, "floor undo restored size") &&
         expect(redoResize.status == iggy3d::RoomEditStatus::RedoApplied,
                "floor resize redo") &&
         expect(redoFloorOk, "floor redo restored size") &&
         expect(badResize.status == iggy3d::RoomEditStatus::InvalidGeometry,
                "bad floor resize rejected") &&
         expect(badMove.status == iggy3d::RoomEditStatus::InvalidGeometry,
                "floor y move rejected");
}

bool wallTransformCommandsAreUndoableAndValidated() {
  iggy3d::EditableRoomSession session;
  iggy3d::EditableRoomWall wall = wallPrimitive();
  wall.semantics.traversalTags = {"clamber"};
  (void)session.submit(iggy3d::addWallCommand(wall));

  const iggy3d::RoomEditResult move =
      session.submit(iggy3d::moveWallCommand("wall_1", {1.0F, 0.0F, 0.5F}));
  const iggy3d::EditableRoomWall* movedWall =
      iggy3d::findEditableWall(session.document(), "wall_1");
  const bool movedWallOk = movedWall != nullptr &&
                           nearly(movedWall->startMeters.x, 0.0F) &&
                           nearly(movedWall->endMeters.x, 2.0F) &&
                           nearly(movedWall->startMeters.z, -0.5F);
  const iggy3d::RoomEditResult stretch =
      session.submit(iggy3d::stretchWallEndCommand("wall_1", {3.0F, 0.0F, -0.5F}));
  const iggy3d::EditableRoomWall* stretchedWall =
      iggy3d::findEditableWall(session.document(), "wall_1");
  const bool stretchedWallOk = stretchedWall != nullptr &&
                               nearly(stretchedWall->endMeters.x, 3.0F);
  const iggy3d::RoomEditResult rotate =
      session.submit(iggy3d::rotateWall90Command("wall_1", true));
  const iggy3d::EditableRoomWall* rotatedWall =
      iggy3d::findEditableWall(session.document(), "wall_1");
  const bool rotatedWallOk = rotatedWall != nullptr &&
                             nearly(rotatedWall->startMeters.x,
                                    rotatedWall->endMeters.x) &&
                             !nearly(rotatedWall->startMeters.z,
                                     rotatedWall->endMeters.z);
  const iggy3d::RoomEditResult height =
      session.submit(iggy3d::setWallHeightCommand("wall_1", 2.5F));
  const iggy3d::RoomEditResult thickness =
      session.submit(iggy3d::setWallThicknessCommand("wall_1", 0.35F));
  const iggy3d::EditableRoomWall* sizedWall =
      iggy3d::findEditableWall(session.document(), "wall_1");
  const bool sizedWallOk = sizedWall != nullptr && nearly(sizedWall->heightMeters, 2.5F) &&
                           nearly(sizedWall->thicknessMeters, 0.35F) &&
                           sizedWall->semantics.traversalTags.size() == 1U;
  const iggy3d::RoomEditResult badStretch =
      session.submit(iggy3d::stretchWallStartCommand("wall_1", {0.0F, 0.0F, 0.25F}));
  const iggy3d::RoomEditResult badHeight =
      session.submit(iggy3d::setWallHeightCommand("wall_1", 0.0F));
  const iggy3d::RoomEditResult badMove =
      session.submit(iggy3d::moveWallCommand("wall_1", {0.0F, 0.25F, 0.0F}));
  const iggy3d::RoomBakeResult bake = iggy3d::bakeEditableRoomDocument(session.document());
  const iggy3d::MovementTraversalSlotRegistry slots =
      iggy3d::buildMovementTraversalSlotRegistry(bake.room, {});

  return expect(move.status == iggy3d::RoomEditStatus::Applied, "wall move applied") &&
         expect(movedWallOk, "wall translated") &&
         expect(stretch.status == iggy3d::RoomEditStatus::Applied,
                "wall stretch applied") &&
         expect(stretchedWallOk, "wall end stretched along axis") &&
         expect(rotate.status == iggy3d::RoomEditStatus::Applied, "wall rotate applied") &&
         expect(rotatedWallOk, "wall rotated to other axis") &&
         expect(height.status == iggy3d::RoomEditStatus::Applied, "wall height applied") &&
         expect(thickness.status == iggy3d::RoomEditStatus::Applied,
                "wall thickness applied") &&
         expect(sizedWallOk, "wall dimensions changed without semantics loss") &&
         expect(badStretch.status == iggy3d::RoomEditStatus::InvalidGeometry,
                "diagonal stretch rejected") &&
         expect(badHeight.status == iggy3d::RoomEditStatus::InvalidGeometry,
                "bad height rejected") &&
         expect(badMove.status == iggy3d::RoomEditStatus::InvalidGeometry,
                "wall y move rejected") &&
         expect(bake.ok, "bake after transforms ok") &&
         expect(!slots.slots.empty(), "clamber slot preserved after transforms");
}

bool lockedTransformsRejectedAndHiddenTransformsAllowed() {
  iggy3d::EditableRoomSession session;
  iggy3d::EditableRoomFloor floor = floorPrimitive();
  floor.id = "locked_floor";
  floor.locked = true;
  (void)session.submit(iggy3d::addFloorCommand(floor));
  const iggy3d::RoomEditResult lockedMove =
      session.submit(iggy3d::moveFloorCommand("locked_floor", {1.0F, 0.0F, 0.0F}));

  iggy3d::EditableRoomWall wall = wallPrimitive();
  wall.id = "hidden_wall";
  wall.hidden = true;
  (void)session.submit(iggy3d::addWallCommand(wall));
  const iggy3d::RoomEditResult hiddenMove =
      session.submit(iggy3d::moveWallCommand("hidden_wall", {1.0F, 0.0F, 0.0F}));
  const iggy3d::EditableRoomWall* movedHidden =
      iggy3d::findEditableWall(session.document(), "hidden_wall");

  return expect(lockedMove.status == iggy3d::RoomEditStatus::LockedPrimitive,
                "locked transform rejected") &&
         expect(hiddenMove.status == iggy3d::RoomEditStatus::Applied,
                "hidden primitive remains editable") &&
         expect(movedHidden != nullptr && movedHidden->hidden &&
                    nearly(movedHidden->startMeters.x, 0.0F),
                "hidden move preserved hidden flag");
}

}  // namespace

int main() {
  const bool ok = sessionAddsDeletesAndRestoresPrimitives() &&
                  semanticsBakeIntoRuntimeSurfaces() &&
                  wallBakePreservesSegmentMetadataAndSurfaceCounts() &&
                  validationRejectsAmbiguousWallGeometryAndBadTags() &&
                  nextEditableCountersIgnoreNonMatchingIds() &&
                  lockedHiddenAndProjectileSemanticsAreStable() &&
                  floorMoveResizeUndoRedoAndValidation() &&
                  wallTransformCommandsAreUndoableAndValidated() &&
                  lockedTransformsRejectedAndHiddenTransformsAllowed();
  return ok ? 0 : 1;
}
