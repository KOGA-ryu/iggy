#include "app/iggy3d/ascii_room/AsciiRoomToRoomAsset.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/movement/MovementTraversalSlots.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view kFixturePath =
    "fixtures/rooms/ascii/training_room.iggyroom.txt";

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float lhs, float rhs) {
  return std::abs(lhs - rhs) < 0.0001F;
}

std::string readTextFile(const std::filesystem::path& path) {
  std::ifstream input(path);
  std::ostringstream out;
  out << input.rdbuf();
  return out.str();
}

iggy3d::AsciiRoomAuthoredRoomResult compileFixture() {
  const std::string text = readTextFile(std::filesystem::path{kFixturePath});
  const iggy3d::AsciiRoomSource source =
      iggy3d::parseAsciiRoomSource(text, std::string(kFixturePath));
  const iggy3d::AsciiRoomGridBuildResult grid =
      iggy3d::buildAsciiRoomGrid(source);
  iggy3d::AsciiRoomCompileConfig config;
  config.roomId = "training_room_ascii";
  config.sourceName = std::string(kFixturePath);
  return iggy3d::compileAsciiRoomToAuthoredRoom(grid.grid, config);
}

iggy3d::AsciiRoomToRoomAssetResult buildFixtureRoomAsset() {
  iggy3d::AsciiRoomToRoomAssetConfig config;
  config.roomId = "training_room_ascii";
  config.sourceName = std::string(kFixturePath);
  return iggy3d::buildRoomAssetFromAsciiRoom(compileFixture(), config);
}

iggy3d::AsciiRoomToRoomAssetResult buildInlineRoomAsset(std::string_view text) {
  const iggy3d::AsciiRoomSource source =
      iggy3d::parseAsciiRoomSource(std::string(text), "inline_terrain_room");
  const iggy3d::AsciiRoomGridBuildResult grid =
      iggy3d::buildAsciiRoomGrid(source);
  iggy3d::AsciiRoomCompileConfig compileConfig;
  compileConfig.roomId = "inline_terrain_room";
  compileConfig.sourceName = "inline_terrain_room";
  const iggy3d::AsciiRoomAuthoredRoomResult authored =
      iggy3d::compileAsciiRoomToAuthoredRoom(grid.grid, compileConfig);
  iggy3d::AsciiRoomToRoomAssetConfig assetConfig;
  assetConfig.roomId = "inline_terrain_room";
  assetConfig.sourceName = "inline_terrain_room";
  return iggy3d::buildRoomAssetFromAsciiRoom(authored, assetConfig);
}

iggy3d::SaveAuthoredRoomSemanticsRecord defaultWallSemantics() {
  iggy3d::SaveAuthoredRoomSemanticsRecord semantics;
  semantics.materialId = "debug_wall";
  semantics.traversalTags = {"clamber_candidate"};
  semantics.blocksActor = true;
  semantics.blocksProjectile = true;
  return semantics;
}

iggy3d::AsciiRoomToRoomAssetResult buildAuthoredZWallRoomAsset() {
  iggy3d::AsciiRoomAuthoredRoomResult authored;
  authored.ok = true;
  authored.status = "ascii_room_ok";
  authored.reasonCode = "ascii_room_ok";
  authored.authoredRoom.present = true;
  authored.authoredRoom.id = "authored_z_wall_room";
  authored.authoredRoom.sourceFile = "authored_z_wall_room";

  iggy3d::SaveAuthoredRoomWallRecord wall;
  wall.id = "wall_z_run";
  wall.startMeters = {2.0F, 0.0F, 0.0F};
  wall.endMeters = {2.0F, 0.0F, 3.0F};
  wall.bottomY = 0.0F;
  wall.heightMeters = 2.5F;
  wall.thicknessMeters = 0.5F;
  wall.semantics = defaultWallSemantics();
  authored.authoredRoom.walls.push_back(wall);

  iggy3d::AsciiRoomToRoomAssetConfig assetConfig;
  assetConfig.roomId = "authored_z_wall_room";
  assetConfig.sourceName = "authored_z_wall_room";
  return iggy3d::buildRoomAssetFromAsciiRoom(authored, assetConfig);
}

const iggy3d::RoomStaticMeshAsset* findMesh(const iggy3d::RoomAsset& room,
                                            std::string_view id) {
  for (const auto& mesh : room.staticMeshes) {
    if (mesh.id == id) {
      return &mesh;
    }
  }
  return nullptr;
}

const iggy3d::RoomSpatialSurface* findSurface(const iggy3d::RoomAsset& room,
                                              std::string_view id) {
  for (const auto& surface : room.spatialSurfaces) {
    if (surface.id == id) {
      return &surface;
    }
  }
  return nullptr;
}

const iggy3d::RoomAnchorAsset* findAnchor(const iggy3d::RoomAsset& room,
                                          std::string_view id) {
  for (const auto& anchor : room.anchors) {
    if (anchor.id == id) {
      return &anchor;
    }
  }
  return nullptr;
}

std::size_t countSurfacesWithRole(const iggy3d::RoomAsset& room,
                                  iggy3d::RoomSpatialSurfaceRole role) {
  std::size_t count = 0;
  for (const auto& surface : room.spatialSurfaces) {
    if (surface.role == role) {
      ++count;
    }
  }
  return count;
}

bool hasTag(const std::vector<std::string>& tags, std::string_view expected) {
  for (const std::string& tag : tags) {
    if (tag == expected) {
      return true;
    }
  }
  return false;
}

bool roomHeaderAndCountsMatchFixture() {
  const auto result = buildFixtureRoomAsset();
  const auto& room = result.room;
  return expect(result.ok, "bridge ok") &&
         expect(result.status == "ascii_room_ok", "bridge status") &&
         expect(result.reasonCode == "ascii_room_ok", "bridge reason") &&
         expect(room.id == "training_room_ascii", "room id") &&
         expect(room.version == 1U, "room version") &&
         expect(room.units == "m", "room units") &&
         expect(room.source == "iggy3d.ascii_room", "room source") &&
         expect(room.sourceFile == kFixturePath, "room source file") &&
         expect(room.sourceSubset == "ascii_room_authoring", "room source subset") &&
         expect(room.staticMeshes.size() == 36U, "static mesh count") &&
         expect(result.staticMeshCount == 36U, "result mesh count") &&
         expect(room.anchors.size() == 5U, "anchor count") &&
         expect(result.anchorCount == 5U, "result anchor count") &&
         expect(room.spatialSurfaces.size() == 56U, "spatial surface count") &&
         expect(result.spatialSurfaceCount == 56U, "result surface count") &&
         expect(result.walkableSurfaceCount == 15U, "walkable surface count") &&
         expect(result.actorBlockerSurfaceCount == 21U, "actor blocker count") &&
         expect(result.projectileBlockerSurfaceCount == 21U,
                "projectile blocker count") &&
         expect(countSurfacesWithRole(room, iggy3d::RoomSpatialSurfaceRole::Walkable) ==
                    15U,
                "walkable role count") &&
         expect(countSurfacesWithRole(room, iggy3d::RoomSpatialSurfaceRole::Blocker) ==
                    21U,
                "blocker role count") &&
         expect(countSurfacesWithRole(
                    room, iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker) == 20U,
                "projectile role count");
}

bool anchorsUseExpectedMarkerKinds() {
  const auto result = buildFixtureRoomAsset();
  const auto& room = result.room;
  const auto* player = findAnchor(room, "marker_player_spawn_r1_c1");
  const auto* npc = findAnchor(room, "marker_npc_spawn_r1_c4");
  const auto* door = findAnchor(room, "marker_door_r2_c2");
  const auto* treasure = findAnchor(room, "marker_treasure_r2_c4");
  const auto* exit = findAnchor(room, "marker_exit_r3_c3");
  return expect(player != nullptr, "player anchor") &&
         expect(player->kind == "spawn", "player kind") &&
         expect(player->runtimeStableName == player->id, "player stable name") &&
         expect(near(player->positionMeters.x, -2.0F), "player x") &&
         expect(near(player->positionMeters.y, 0.05F), "player y") &&
         expect(near(player->positionMeters.z, -1.0F), "player z") &&
         expect(npc != nullptr, "npc anchor") && expect(npc->kind == "npc", "npc kind") &&
         expect(door != nullptr, "door anchor") &&
         expect(door->kind == "door", "door kind") &&
         expect(treasure != nullptr, "treasure anchor") &&
         expect(treasure->kind == "treasure", "treasure kind") &&
         expect(exit != nullptr, "exit anchor") &&
         expect(exit->kind == "exit", "exit kind");
}

bool npcAndMonsterSpawnsUseDistinctAnchorKinds() {
  const auto result = buildInlineRoomAsset("######\n#PNME#\n######\n");
  const auto* npc = findAnchor(result.room, "marker_npc_spawn_r1_c2");
  const auto* monster = findAnchor(result.room, "marker_monster_spawn_r1_c3");
  return expect(result.ok, "npc monster room asset ok") &&
         expect(npc != nullptr, "npc spawn anchor exists") &&
         expect(npc != nullptr && npc->kind == "npc",
                "npc_spawn anchor remains npc") &&
         expect(monster != nullptr, "monster spawn anchor exists") &&
         expect(monster != nullptr && monster->kind == "monster",
                "monster_spawn anchor becomes monster");
}

bool doorMeshAndBlockerUseRuntimeOwner() {
  const auto result = buildFixtureRoomAsset();
  const auto* mesh = findMesh(result.room, "marker_door_r2_c2_panel");
  const auto* surface =
      findSurface(result.room, "marker_door_r2_c2_door_blocker");
  return expect(mesh != nullptr, "door mesh exists") &&
         expect(mesh->meshId == "door_panel", "door mesh id") &&
         expect(mesh->role == "door", "door role") &&
         expect(surface != nullptr, "door blocker exists") &&
         expect(surface->sourceStaticMeshId == "marker_door_r2_c2_panel",
                "door blocker source mesh") &&
         expect(surface->shape == iggy3d::RoomSpatialSurfaceShape::Box,
                "door blocker shape") &&
         expect(surface->role == iggy3d::RoomSpatialSurfaceRole::Blocker,
                "door blocker role") &&
         expect(surface->runtimeOwnerStableName == "marker_door_r2_c2",
                "door blocker owner") &&
         expect(surface->blocksActor, "door blocks actor") &&
         expect(surface->blocksProjectile, "door blocks projectile") &&
         expect(hasTag(surface->traversalTags, "blocker"),
                "door blocker tag");
}

bool representativeFloorMeshAndSurfaceMatch() {
  const auto result = buildFixtureRoomAsset();
  const auto* mesh = findMesh(result.room, "floor_r1_c1");
  const auto* surface = findSurface(result.room, "floor_r1_c1_walkable");
  return expect(mesh != nullptr, "floor mesh exists") &&
         expect(mesh->meshId == "floor_rect", "floor mesh id") &&
         expect(mesh->materialId == "debug_floor", "floor material") &&
         expect(mesh->role == "floor", "floor role") &&
         expect(near(mesh->positionMeters.x, -2.0F), "floor mesh x") &&
         expect(near(mesh->positionMeters.y, -0.05F), "floor mesh y") &&
         expect(near(mesh->positionMeters.z, -1.0F), "floor mesh z") &&
         expect(near(mesh->sizeMeters.x, 1.0F), "floor mesh size x") &&
         expect(near(mesh->sizeMeters.y, 0.10F), "floor mesh size y") &&
         expect(near(mesh->sizeMeters.z, 1.0F), "floor mesh size z") &&
         expect(surface != nullptr, "walkable surface exists") &&
         expect(surface->sourceStaticMeshId == "floor_r1_c1", "floor surface source") &&
         expect(surface->shape == iggy3d::RoomSpatialSurfaceShape::Plane,
                "floor surface shape") &&
         expect(surface->role == iggy3d::RoomSpatialSurfaceRole::Walkable,
                "floor surface role") &&
         expect(surface->pointsMeters.size() == 4U, "floor surface point count") &&
         expect(near(surface->pointsMeters[0].x, -2.5F), "floor point 0 x") &&
         expect(near(surface->pointsMeters[0].y, 0.0F), "floor point 0 y") &&
         expect(near(surface->pointsMeters[0].z, -1.5F), "floor point 0 z") &&
         expect(near(surface->pointsMeters[2].x, -1.5F), "floor point 2 x") &&
         expect(near(surface->pointsMeters[2].z, -0.5F), "floor point 2 z") &&
         expect(near(surface->normal.x, 0.0F) && near(surface->normal.y, 1.0F) &&
                    near(surface->normal.z, 0.0F),
                "floor normal") &&
         expect(hasTag(surface->traversalTags, "walkable"), "floor traversal") &&
         expect(surface->collisionMask.size() == 1U &&
                    surface->collisionMask[0] == "actor",
                "floor collision mask") &&
         expect(!surface->blocksActor, "floor actor pass") &&
         expect(!surface->blocksProjectile, "floor projectile pass");
}

bool representativeWallMeshAndSurfacesMatch() {
  const auto result = buildFixtureRoomAsset();
  const auto* mesh = findMesh(result.room, "wall_r0_c0");
  const auto* actor = findSurface(result.room, "wall_r0_c0_actor_blocker");
  const auto* projectile = findSurface(result.room, "wall_r0_c0_projectile_blocker");
  return expect(mesh != nullptr, "wall mesh exists") &&
         expect(mesh->meshId == "wall_segment", "wall mesh id") &&
         expect(mesh->materialId == "debug_wall", "wall material") &&
         expect(mesh->role == "wall", "wall role") &&
         expect(mesh->hasWallSegment, "wall segment present") &&
         expect(near(mesh->wallStartMeters.x, -3.5F), "wall segment start x") &&
         expect(near(mesh->wallStartMeters.z, -2.0F), "wall segment start z") &&
         expect(near(mesh->wallEndMeters.x, -2.5F), "wall segment end x") &&
         expect(near(mesh->wallEndMeters.z, -2.0F), "wall segment end z") &&
         expect(near(mesh->wallBottomY, 0.0F), "wall segment bottom") &&
         expect(near(mesh->wallHeightMeters, 2.5F), "wall segment height") &&
         expect(near(mesh->wallThicknessMeters, 1.0F), "wall segment thickness") &&
         expect(near(mesh->positionMeters.x, -3.0F), "wall mesh x") &&
         expect(near(mesh->positionMeters.y, 1.25F), "wall mesh y") &&
         expect(near(mesh->positionMeters.z, -2.0F), "wall mesh z") &&
         expect(near(mesh->sizeMeters.x, 1.0F), "wall mesh size x") &&
         expect(near(mesh->sizeMeters.y, 2.5F), "wall mesh size y") &&
         expect(near(mesh->sizeMeters.z, 1.0F), "wall mesh size z") &&
         expect(actor != nullptr, "actor blocker exists") &&
         expect(actor->sourceStaticMeshId == "wall_r0_c0", "actor source") &&
         expect(actor->shape == iggy3d::RoomSpatialSurfaceShape::Box,
                "actor shape") &&
         expect(actor->role == iggy3d::RoomSpatialSurfaceRole::Blocker,
                "actor role") &&
         expect(actor->pointsMeters.size() == 8U, "actor point count") &&
         expect(near(actor->pointsMeters[0].x, -3.5F), "actor min x") &&
         expect(near(actor->pointsMeters[0].y, 0.0F), "actor min y") &&
         expect(near(actor->pointsMeters[0].z, -2.5F), "actor min z") &&
         expect(near(actor->pointsMeters[6].x, -2.5F), "actor max x") &&
         expect(near(actor->pointsMeters[6].y, 2.5F), "actor max y") &&
         expect(near(actor->pointsMeters[6].z, -1.5F), "actor max z") &&
         expect(near(actor->normal.x, 0.0F) && near(actor->normal.y, 0.0F) &&
                    near(actor->normal.z, 1.0F),
                "actor normal") &&
         expect(hasTag(actor->traversalTags, "blocker"), "actor blocker tag") &&
         expect(hasTag(actor->traversalTags, "clamber_candidate"),
                "actor preserves wall tag") &&
         expect(actor->collisionMask.size() == 1U && actor->collisionMask[0] == "actor",
                "actor collision mask") &&
         expect(actor->blocksActor, "actor blocks actor") &&
         expect(!actor->blocksProjectile, "actor projectile pass") &&
         expect(projectile != nullptr, "projectile blocker exists") &&
         expect(projectile->role == iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker,
                "projectile role") &&
         expect(projectile->pointsMeters.size() == 8U, "projectile point count") &&
         expect(hasTag(projectile->traversalTags, "projectile_blocker"),
                "projectile tag") &&
         expect(projectile->collisionMask.size() == 1U &&
                    projectile->collisionMask[0] == "projectile",
                "projectile collision mask") &&
         expect(!projectile->blocksActor, "projectile actor pass") &&
         expect(projectile->blocksProjectile, "projectile blocks projectile");
}

bool authoredZRunningWallPreservesSegmentAndSurfaceExtents() {
  const auto result = buildAuthoredZWallRoomAsset();
  const auto* mesh = findMesh(result.room, "wall_z_run");
  const auto* actor = findSurface(result.room, "wall_z_run_actor_blocker");
  const auto* projectile = findSurface(result.room, "wall_z_run_projectile_blocker");
  return expect(result.ok, "z wall room asset ok") &&
         expect(mesh != nullptr, "z wall mesh exists") &&
         expect(mesh->hasWallSegment, "z wall segment present") &&
         expect(near(mesh->wallStartMeters.x, 2.0F), "z wall start x") &&
         expect(near(mesh->wallStartMeters.z, 0.0F), "z wall start z") &&
         expect(near(mesh->wallEndMeters.x, 2.0F), "z wall end x") &&
         expect(near(mesh->wallEndMeters.z, 3.0F), "z wall end z") &&
         expect(near(mesh->wallBottomY, 0.0F), "z wall bottom") &&
         expect(near(mesh->wallHeightMeters, 2.5F), "z wall height") &&
         expect(near(mesh->wallThicknessMeters, 0.5F), "z wall thickness") &&
         expect(actor != nullptr, "z actor blocker exists") &&
         expect(actor->pointsMeters.size() == 8U, "z actor point count") &&
         expect(near(actor->pointsMeters[0].x, 1.75F), "z actor min x") &&
         expect(near(actor->pointsMeters[0].z, 0.0F), "z actor min z") &&
         expect(near(actor->pointsMeters[6].x, 2.25F), "z actor max x") &&
         expect(near(actor->pointsMeters[6].z, 3.0F), "z actor max z") &&
         expect(projectile != nullptr, "z projectile blocker exists") &&
         expect(projectile->pointsMeters.size() == 8U, "z projectile point count") &&
         expect(near(projectile->pointsMeters[0].x, 1.75F),
                "z projectile min x") &&
         expect(near(projectile->pointsMeters[6].z, 3.0F),
                "z projectile max z");
}

bool terrainSurfacesPreserveHeightAndSlope() {
  const auto result = buildInlineRoomAsset("######\n#P1>!#\n######\n");
  const auto* elevatedMesh = findMesh(result.room, "floor_r1_c2");
  const auto* elevatedSurface = findSurface(result.room, "floor_r1_c2_walkable");
  const auto* rampSurface = findSurface(result.room, "floor_r1_c3_walkable");
  const auto* blockedSurface = findSurface(result.room, "floor_r1_c4_walkable");
  return expect(result.ok, "terrain room asset ok") &&
         expect(elevatedMesh != nullptr, "elevated mesh exists") &&
         expect(near(elevatedMesh->positionMeters.y, 0.45F),
                "elevated mesh y") &&
         expect(elevatedSurface != nullptr, "elevated surface exists") &&
         expect(elevatedSurface->pointsMeters.size() == 4U,
                "elevated point count") &&
         expect(near(elevatedSurface->pointsMeters[0].y, 0.5F),
                "elevated top point") &&
         expect(near(elevatedSurface->normal.x, 0.0F) &&
                    near(elevatedSurface->normal.y, 1.0F) &&
                    near(elevatedSurface->normal.z, 0.0F),
                "elevated normal") &&
         expect(hasTag(elevatedSurface->traversalTags, "elevated_floor"),
                "elevated traversal tag") &&
         expect(rampSurface != nullptr, "ramp surface exists") &&
         expect(near(rampSurface->pointsMeters[0].y, 0.0F),
                "ramp low west point") &&
         expect(near(rampSurface->pointsMeters[1].y, 0.5F),
                "ramp high east point") &&
         expect(near(rampSurface->normal.x, -0.4472136F),
                "ramp normal x") &&
         expect(near(rampSurface->normal.y, 0.8944272F),
                "ramp normal y") &&
         expect(near(rampSurface->normal.z, 0.0F), "ramp normal z") &&
         expect(hasTag(rampSurface->traversalTags, "ramp"), "ramp tag") &&
         expect(hasTag(rampSurface->traversalTags, "terrain_ramp_east"),
                "ramp terrain tag") &&
         expect(blockedSurface != nullptr, "blocked slope surface exists") &&
         expect(near(blockedSurface->pointsMeters[0].y, 0.0F),
                "blocked low west point") &&
         expect(near(blockedSurface->pointsMeters[1].y, 1.0F),
                "blocked high east point") &&
         expect(near(blockedSurface->normal.x, -0.7071068F),
                "blocked normal x") &&
         expect(near(blockedSurface->normal.y, 0.7071068F),
                "blocked normal y") &&
         expect(hasTag(blockedSurface->traversalTags, "blocked_slope"),
                "blocked slope tag");
}

bool crateObjectBuildsPropMeshAndBlockerSurfaces() {
  const auto result = buildInlineRoomAsset("#####\n#PCE#\n#####\n");
  const auto* prop = findMesh(result.room, "object_crate_r1_c2");
  const auto* actor =
      findSurface(result.room, "object_crate_r1_c2_actor_blocker");
  const auto* projectile =
      findSurface(result.room, "object_crate_r1_c2_projectile_blocker");
  return expect(result.ok, "crate room asset ok") &&
         expect(result.room.staticMeshes.size() == 16U,
                "crate static mesh count") &&
         expect(result.room.spatialSurfaces.size() == 29U,
                "crate spatial surface count") &&
         expect(result.walkableSurfaceCount == 3U,
                "crate walkable surface count") &&
         expect(result.actorBlockerSurfaceCount == 13U,
                "crate actor blocker count") &&
         expect(result.projectileBlockerSurfaceCount == 13U,
                "crate projectile blocker count") &&
         expect(prop != nullptr, "crate prop mesh exists") &&
         expect(prop != nullptr && prop->meshId == "wood_crate_proxy",
                "crate mesh id") &&
         expect(prop != nullptr && prop->materialId == "wood_crate_proxy",
                "crate material id") &&
         expect(prop != nullptr && prop->role == "prop", "crate prop role") &&
         expect(prop != nullptr && near(prop->positionMeters.x, 0.0F),
                "crate prop x") &&
         expect(prop != nullptr && near(prop->positionMeters.y, 0.4F),
                "crate prop y") &&
         expect(prop != nullptr && near(prop->positionMeters.z, 0.0F),
                "crate prop z") &&
         expect(prop != nullptr && near(prop->sizeMeters.x, 0.8F),
                "crate prop size x") &&
         expect(prop != nullptr && near(prop->sizeMeters.y, 0.8F),
                "crate prop size y") &&
         expect(actor != nullptr, "crate actor surface exists") &&
         expect(actor != nullptr && actor->sourceStaticMeshId == "object_crate_r1_c2",
                "crate actor source mesh") &&
         expect(actor != nullptr && actor->role == iggy3d::RoomSpatialSurfaceRole::Blocker,
                "crate actor role") &&
         expect(actor != nullptr && actor->pointsMeters.size() == 8U,
                "crate actor point count") &&
         expect(actor != nullptr && near(actor->pointsMeters[0].x, -0.4F),
                "crate actor min x") &&
         expect(actor != nullptr && near(actor->pointsMeters[0].y, 0.0F),
                "crate actor min y") &&
         expect(actor != nullptr && near(actor->pointsMeters[6].y, 0.8F),
                "crate actor max y") &&
         expect(actor != nullptr && actor->runtimeOwnerStableName ==
                                     "object_crate_r1_c2",
                "crate actor owner") &&
         expect(actor != nullptr && actor->blocksActor,
                "crate actor blocks actor") &&
         expect(actor != nullptr && !actor->blocksProjectile,
                "crate actor projectile pass") &&
         expect(actor != nullptr && hasTag(actor->traversalTags, "crate"),
                "crate actor traversal tag") &&
         expect(projectile != nullptr, "crate projectile surface exists") &&
         expect(projectile != nullptr &&
                    projectile->role ==
                        iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker,
                "crate projectile role") &&
         expect(projectile != nullptr && projectile->runtimeOwnerStableName ==
                                          "object_crate_r1_c2",
                "crate projectile owner") &&
         expect(projectile != nullptr && !projectile->blocksActor,
                "crate projectile actor pass") &&
         expect(projectile != nullptr && projectile->blocksProjectile,
                "crate projectile blocks projectile") &&
         expect(projectile != nullptr &&
                    hasTag(projectile->traversalTags, "projectile_blocker"),
                "crate projectile tag");
}

bool ledgeObjectBuildsClamberMeshAndSurfaces() {
  const auto result = buildInlineRoomAsset("#####\n#PLE#\n#####\n");
  const auto* ledge = findMesh(result.room, "object_clamber_ledge_r1_c2");
  const auto* top =
      findSurface(result.room, "object_clamber_ledge_r1_c2_walkable_top");
  const auto* actor =
      findSurface(result.room, "object_clamber_ledge_r1_c2_actor_blocker");
  const auto* projectile =
      findSurface(result.room, "object_clamber_ledge_r1_c2_projectile_blocker");
  const iggy3d::MovementTraversalSlotRegistry slots =
      iggy3d::buildMovementTraversalSlotRegistry(result.room,
                                                 iggy3d::Vec3{});

  return expect(result.ok, "ledge room asset ok") &&
         expect(result.room.staticMeshes.size() == 16U,
                "ledge static mesh count") &&
         expect(result.room.spatialSurfaces.size() == 30U,
                "ledge spatial surface count") &&
         expect(result.walkableSurfaceCount == 4U,
                "ledge walkable surface count") &&
         expect(result.actorBlockerSurfaceCount == 13U,
                "ledge actor blocker count") &&
         expect(result.projectileBlockerSurfaceCount == 13U,
                "ledge projectile blocker count") &&
         expect(ledge != nullptr, "ledge mesh exists") &&
         expect(ledge != nullptr &&
                    ledge->meshId == "movement_clamber_ledge_proxy",
                "ledge mesh id") &&
         expect(ledge != nullptr && ledge->role == "ledge", "ledge role") &&
         expect(ledge != nullptr && near(ledge->positionMeters.y, 0.85F),
                "ledge center y") &&
         expect(ledge != nullptr && near(ledge->sizeMeters.x, 2.0F),
                "ledge size x") &&
         expect(ledge != nullptr && near(ledge->sizeMeters.y, 1.7F),
                "ledge eye height") &&
         expect(ledge != nullptr && near(ledge->sizeMeters.z, 1.0F),
                "ledge size z") &&
         expect(top != nullptr, "ledge top surface exists") &&
         expect(top != nullptr &&
                    top->role == iggy3d::RoomSpatialSurfaceRole::Walkable,
                "ledge top walkable role") &&
         expect(top != nullptr && near(top->pointsMeters[0].y, 1.7F),
                "ledge top y") &&
         expect(top != nullptr && hasTag(top->traversalTags, "clamber"),
                "ledge top clamber tag") &&
         expect(actor != nullptr, "ledge actor surface exists") &&
         expect(actor != nullptr &&
                    actor->sourceStaticMeshId == "object_clamber_ledge_r1_c2",
                "ledge actor source") &&
         expect(actor != nullptr && near(actor->pointsMeters[6].y, 1.7F),
                "ledge actor max y") &&
         expect(actor != nullptr && hasTag(actor->traversalTags, "clamber"),
                "ledge actor clamber tag") &&
         expect(projectile != nullptr, "ledge projectile surface exists") &&
         expect(slots.slots.size() == 1U, "ledge traversal slot count") &&
         expect(!slots.slots.empty() &&
                    slots.slots[0].slotId ==
                        "object_clamber_ledge_r1_c2:object_clamber_ledge_r1_c2_walkable_top",
                "ledge traversal slot id");
}

bool wallJumpGlyphBuildsTaggedActorBlockerSurface() {
  const auto result = buildInlineRoomAsset("#####\n#P.E#\n##J##\n");
  const auto* actor = findSurface(result.room, "wall_r2_c2_actor_blocker");
  return expect(result.ok, "wall jump room asset ok") &&
         expect(actor != nullptr, "wall jump actor surface exists") &&
         expect(actor != nullptr &&
                    actor->role == iggy3d::RoomSpatialSurfaceRole::Blocker,
                "wall jump actor role") &&
         expect(actor != nullptr && actor->blocksActor,
                "wall jump actor blocks actor") &&
         expect(actor != nullptr && hasTag(actor->traversalTags, "blocker"),
                "wall jump actor blocker tag") &&
         expect(actor != nullptr && hasTag(actor->traversalTags, "wall_jump"),
                "wall jump actor traversal tag");
}

bool resetZoneGlyphBuildsResetAnchorAndWalkableFloor() {
  const auto result = buildInlineRoomAsset("######\n#P.RE#\n######\n");
  const auto* anchor = findAnchor(result.room, "marker_reset_zone_r1_c3");
  const auto* floor = findSurface(result.room, "floor_r1_c3_walkable");
  const auto* mesh = findMesh(result.room, "marker_reset_zone_r1_c3_marker");
  return expect(result.ok, "reset zone room asset ok") &&
         expect(anchor != nullptr, "reset zone anchor exists") &&
         expect(anchor != nullptr && anchor->kind == "reset_zone",
                "reset zone anchor kind") &&
         expect(anchor != nullptr &&
                    anchor->runtimeStableName == "marker_reset_zone_r1_c3",
                "reset zone stable name") &&
         expect(floor != nullptr, "reset zone keeps walkable floor") &&
         expect(floor != nullptr &&
                    floor->role == iggy3d::RoomSpatialSurfaceRole::Walkable,
                "reset zone walkable role") &&
         expect(mesh != nullptr, "reset zone marker mesh exists") &&
         expect(mesh != nullptr && mesh->role == "prop",
                "reset zone marker prop role") &&
         expect(mesh != nullptr && mesh->meshId == "reset_zone_marker",
                "reset zone marker mesh id") &&
         expect(mesh != nullptr && mesh->materialId == "reset_zone_marker",
                "reset zone marker material id") &&
         expect(mesh != nullptr && near(mesh->positionMeters.y, 0.08F),
                "reset zone marker y") &&
         expect(mesh != nullptr && near(mesh->sizeMeters.x, 0.72F),
                "reset zone marker size x") &&
         expect(mesh != nullptr && near(mesh->sizeMeters.y, 0.06F),
                "reset zone marker size y");
}

bool affordanceGlyphsBuildAnchorsAndReasoningNodes() {
  const auto result = buildInlineRoomAsset("######\n#PcpE#\n######\n");
  const auto* cover = findAnchor(result.room, "marker_cover_r1_c2");
  const auto* patrol = findAnchor(result.room, "marker_patrol_post_r1_c3");
  const auto* coverFloor = findSurface(result.room, "floor_r1_c2_walkable");
  const auto* patrolFloor = findSurface(result.room, "floor_r1_c3_walkable");
  const std::vector<iggy3d::Vec3> noWaypoints;
  const iggy3d::ReasoningGraph graph =
      iggy3d::buildReasoningGraph(result.room, noWaypoints);

  std::size_t coverNodes = 0;
  std::size_t patrolNodes = 0;
  for (const iggy3d::ReasoningNode& node : graph.nodes) {
    if (node.kind == iggy3d::ReasoningNodeKind::coverCluster &&
        node.sourceLabel == "cover") {
      ++coverNodes;
    }
    if (node.kind == iggy3d::ReasoningNodeKind::patrolPost &&
        node.sourceLabel == "patrol_post") {
      ++patrolNodes;
    }
  }

  return expect(result.ok, "affordance glyph room asset ok") &&
         expect(cover != nullptr, "cover anchor exists") &&
         expect(cover != nullptr && cover->kind == "cover",
                "cover glyph anchor kind") &&
         expect(patrol != nullptr, "patrol_post anchor exists") &&
         expect(patrol != nullptr && patrol->kind == "patrol_post",
                "patrol glyph anchor kind") &&
         expect(coverFloor != nullptr &&
                    coverFloor->role == iggy3d::RoomSpatialSurfaceRole::Walkable,
                "cover glyph keeps walkable floor") &&
         expect(patrolFloor != nullptr &&
                    patrolFloor->role == iggy3d::RoomSpatialSurfaceRole::Walkable,
                "patrol glyph keeps walkable floor") &&
         expect(coverNodes == 1U, "cover glyph becomes coverCluster node") &&
         expect(patrolNodes == 1U, "patrol glyph becomes patrolPost node");
}

bool invalidAuthoredResultRejectsWithoutPartialRoom() {
  iggy3d::AsciiRoomAuthoredRoomResult invalid;
  invalid.ok = false;
  invalid.status = "ascii_room_empty";
  invalid.reasonCode = "ascii_room_empty";
  const auto forwarded = iggy3d::buildRoomAssetFromAsciiRoom(invalid);

  iggy3d::AsciiRoomAuthoredRoomResult missing;
  missing.ok = true;
  missing.status = "ascii_room_ok";
  missing.reasonCode = "ascii_room_ok";
  missing.authoredRoom.present = false;
  const auto noRoom = iggy3d::buildRoomAssetFromAsciiRoom(missing);

  return expect(!forwarded.ok, "invalid rejected") &&
         expect(forwarded.status == "ascii_room_empty", "invalid status forwarded") &&
         expect(forwarded.reasonCode == "ascii_room_empty", "invalid reason forwarded") &&
         expect(forwarded.room.staticMeshes.empty(), "invalid no meshes") &&
         expect(forwarded.room.anchors.empty(), "invalid no anchors") &&
         expect(forwarded.room.spatialSurfaces.empty(), "invalid no surfaces") &&
         expect(!noRoom.ok, "missing room rejected") &&
         expect(noRoom.status == "ascii_room_asset_missing_authored_room",
                "missing room status") &&
         expect(noRoom.reasonCode == "ascii_room_asset_missing_authored_room",
                "missing room reason") &&
         expect(noRoom.room.staticMeshes.empty(), "missing room no meshes") &&
         expect(noRoom.room.anchors.empty(), "missing room no anchors") &&
         expect(noRoom.room.spatialSurfaces.empty(), "missing room no surfaces");
}

}  // namespace

int main() {
  bool ok = true;
  ok = roomHeaderAndCountsMatchFixture() && ok;
  ok = anchorsUseExpectedMarkerKinds() && ok;
  ok = npcAndMonsterSpawnsUseDistinctAnchorKinds() && ok;
  ok = doorMeshAndBlockerUseRuntimeOwner() && ok;
  ok = representativeFloorMeshAndSurfaceMatch() && ok;
  ok = representativeWallMeshAndSurfacesMatch() && ok;
  ok = authoredZRunningWallPreservesSegmentAndSurfaceExtents() && ok;
  ok = terrainSurfacesPreserveHeightAndSlope() && ok;
  ok = crateObjectBuildsPropMeshAndBlockerSurfaces() && ok;
  ok = ledgeObjectBuildsClamberMeshAndSurfaces() && ok;
  ok = wallJumpGlyphBuildsTaggedActorBlockerSurface() && ok;
  ok = resetZoneGlyphBuildsResetAnchorAndWalkableFloor() && ok;
  ok = affordanceGlyphsBuildAnchorsAndReasoningNodes() && ok;
  ok = invalidAuthoredResultRejectsWithoutPartialRoom() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
