#include "app/iggy3d/AsciiRoomToRoomAsset.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

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
         expect(room.staticMeshes.size() == 35U, "static mesh count") &&
         expect(result.staticMeshCount == 35U, "result mesh count") &&
         expect(room.anchors.size() == 5U, "anchor count") &&
         expect(result.anchorCount == 5U, "result anchor count") &&
         expect(room.spatialSurfaces.size() == 55U, "spatial surface count") &&
         expect(result.spatialSurfaceCount == 55U, "result surface count") &&
         expect(result.walkableSurfaceCount == 15U, "walkable surface count") &&
         expect(result.actorBlockerSurfaceCount == 20U, "actor blocker count") &&
         expect(result.projectileBlockerSurfaceCount == 20U,
                "projectile blocker count") &&
         expect(countSurfacesWithRole(room, iggy3d::RoomSpatialSurfaceRole::Walkable) ==
                    15U,
                "walkable role count") &&
         expect(countSurfacesWithRole(room, iggy3d::RoomSpatialSurfaceRole::Blocker) ==
                    20U,
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
         expect(treasure->kind == "pickup", "treasure kind") &&
         expect(exit != nullptr, "exit anchor") &&
         expect(exit->kind == "exit", "exit kind");
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
  ok = representativeFloorMeshAndSurfaceMatch() && ok;
  ok = representativeWallMeshAndSurfacesMatch() && ok;
  ok = invalidAuthoredResultRejectsWithoutPartialRoom() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
