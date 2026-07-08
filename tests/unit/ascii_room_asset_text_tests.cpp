#include "app/iggy3d/ascii_room/AsciiRoomAssetText.hpp"
#include "app/iggy3d/ascii_room/AsciiRoomToRoomAsset.hpp"
#include "content/assets/RoomAsset.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
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

iggy3d::RoomAsset buildFixtureRoomAsset() {
  const std::string text = readTextFile(std::filesystem::path{kFixturePath});
  const iggy3d::AsciiRoomSource source =
      iggy3d::parseAsciiRoomSource(text, std::string(kFixturePath));
  const iggy3d::AsciiRoomGridBuildResult grid =
      iggy3d::buildAsciiRoomGrid(source);
  iggy3d::AsciiRoomCompileConfig compileConfig;
  compileConfig.roomId = "training_room_ascii";
  compileConfig.sourceName = std::string(kFixturePath);
  const iggy3d::AsciiRoomAuthoredRoomResult authored =
      iggy3d::compileAsciiRoomToAuthoredRoom(grid.grid, compileConfig);
  iggy3d::AsciiRoomToRoomAssetConfig roomConfig;
  roomConfig.roomId = "training_room_ascii";
  roomConfig.sourceName = std::string(kFixturePath);
  return iggy3d::buildRoomAssetFromAsciiRoom(authored, roomConfig).room;
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

const iggy3d::RoomAnchorAsset* findAnchor(const iggy3d::RoomAsset& room,
                                          std::string_view id) {
  for (const auto& anchor : room.anchors) {
    if (anchor.id == id) {
      return &anchor;
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

bool contains(std::string_view haystack, std::string_view needle) {
  return haystack.find(needle) != std::string_view::npos;
}

bool vecNear(const iggy3d::Vec3& lhs, const iggy3d::Vec3& rhs) {
  return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
}

std::size_t countOccurrences(std::string_view haystack, std::string_view needle) {
  std::size_t count = 0;
  std::size_t offset = 0;
  while ((offset = haystack.find(needle, offset)) != std::string_view::npos) {
    ++count;
    offset += needle.size();
  }
  return count;
}

iggy3d::RoomSpatialSurface exportSurface(
    std::string id,
    iggy3d::RoomSpatialSurfaceRole role,
    iggy3d::RoomSpatialSurfaceShape shape,
    std::vector<std::string> traversalTags) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = std::move(id);
  surface.sourceStaticMeshId = "mesh";
  surface.shape = shape;
  surface.role = role;
  surface.pointsMeters = {{0.0F, 0.0F, 0.0F},
                          {1.0F, 0.0F, 0.0F},
                          {1.0F, 0.0F, 1.0F},
                          {0.0F, 0.0F, 1.0F}};
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = std::move(traversalTags);
  surface.collisionMask = {"actor"};
  surface.runtimeOwnerStableName = surface.id;
  return surface;
}

iggy3d::RoomAsset buildTraversalTagExportRoom() {
  iggy3d::RoomAsset room;
  room.id = "tag_export_room";
  room.units = "m";
  room.source = "test";
  room.sourceFile = "tag_export_room";
  room.sourceSubset = "unit";
  iggy3d::RoomStaticMeshAsset mesh;
  mesh.id = "mesh";
  mesh.meshId = "mesh";
  mesh.materialId = "material";
  mesh.role = "floor";
  mesh.positionMeters = {0.0F, 0.0F, 0.0F};
  mesh.sizeMeters = {1.0F, 1.0F, 1.0F};
  room.staticMeshes.push_back(mesh);
  room.anchors.push_back({"spawn", "spawn", "spawn", {0.0F, 0.0F, 0.0F}});
  room.spatialSurfaces.push_back(exportSurface(
      "walk_surface",
      iggy3d::RoomSpatialSurfaceRole::Walkable,
      iggy3d::RoomSpatialSurfaceShape::Plane,
      {"clamber", "walkable", "magic", "vault"}));
  room.spatialSurfaces.push_back(exportSurface(
      "block_surface",
      iggy3d::RoomSpatialSurfaceRole::Blocker,
      iggy3d::RoomSpatialSurfaceShape::Box,
      {"debug_only", "blocker"}));
  room.spatialSurfaces.push_back(exportSurface(
      "projectile_surface",
      iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker,
      iggy3d::RoomSpatialSurfaceShape::Plane,
      {"wire_walk", "projectile_blocker"}));
  room.spatialSurfaces.push_back(exportSurface(
      "opening_surface",
      iggy3d::RoomSpatialSurfaceRole::Opening,
      iggy3d::RoomSpatialSurfaceShape::Opening,
      {"no_player", "opening"}));
  return room;
}

bool fixtureExportsAndParsesBack() {
  const iggy3d::RoomAsset original = buildFixtureRoomAsset();
  const iggy3d::AsciiRoomAssetTextResult written =
      iggy3d::writeAsciiRoomAssetText(original);
  const iggy3d::RoomAssetParseResult parsed =
      iggy3d::parseRoomAssetText(written.text);

  const iggy3d::RoomStaticMeshAsset* originalFloor = findMesh(original, "floor_r1_c1");
  const iggy3d::RoomStaticMeshAsset* parsedFloor = findMesh(parsed.room, "floor_r1_c1");
  const iggy3d::RoomStaticMeshAsset* originalWall = findMesh(original, "wall_r0_c0");
  const iggy3d::RoomStaticMeshAsset* parsedWall = findMesh(parsed.room, "wall_r0_c0");
  const iggy3d::RoomSpatialSurface* originalDoor =
      findSurface(original, "marker_door_r2_c2_door_blocker");
  const iggy3d::RoomSpatialSurface* parsedDoor =
      findSurface(parsed.room, "marker_door_r2_c2_door_blocker");
  const iggy3d::RoomSpatialSurface* originalWallSurface =
      findSurface(original, "wall_r0_c0_actor_blocker");
  const iggy3d::RoomSpatialSurface* parsedWallSurface =
      findSurface(parsed.room, "wall_r0_c0_actor_blocker");
  const iggy3d::RoomAnchorAsset* originalAnchor =
      findAnchor(original, "marker_player_spawn_r1_c1");
  const iggy3d::RoomAnchorAsset* parsedAnchor =
      findAnchor(parsed.room, "marker_player_spawn_r1_c1");

  return expect(written.ok, "text write ok") &&
         expect(written.status == "ascii_room_asset_text_written", "text write status") &&
         expect(written.reasonCode == "ascii_room_asset_text_written", "text write reason") &&
         expect(contains(written.text, "[room]"), "contains room table") &&
         expect(contains(written.text, "[conversion]"), "contains conversion table") &&
         expect(contains(written.text, "[[static_meshes]]"), "contains meshes") &&
         expect(contains(written.text, "[[spatial_surfaces]]"), "contains surfaces") &&
         expect(contains(written.text, "[[anchors]]"), "contains anchors") &&
         expect(!contains(written.text, "[[openings]]"), "no openings emitted") &&
         expect(!written.text.empty() && written.text.back() == '\n', "text ends newline") &&
         expect(written.staticMeshCount == 36U, "written mesh count") &&
         expect(written.anchorCount == 5U, "written anchor count") &&
         expect(written.spatialSurfaceCount == 56U, "written surface count") &&
         expect(parsed.ok, parsed.reason) &&
         expect(parsed.room.id == original.id, "parsed room id") &&
         expect(parsed.room.source == original.source, "parsed source") &&
         expect(parsed.room.sourceFile == original.sourceFile, "parsed source file") &&
         expect(parsed.room.sourceSubset == original.sourceSubset, "parsed subset") &&
         expect(parsed.room.staticMeshes.size() == 36U, "parsed mesh count") &&
         expect(parsed.room.anchors.size() == 5U, "parsed anchor count") &&
         expect(parsed.room.spatialSurfaces.size() == 56U, "parsed surface count") &&
         expect(originalFloor != nullptr && parsedFloor != nullptr, "floor exists") &&
         expect(vecNear(parsedFloor->positionMeters, originalFloor->positionMeters),
                "floor position roundtrip") &&
         expect(vecNear(parsedFloor->sizeMeters, originalFloor->sizeMeters),
                "floor size roundtrip") &&
         expect(originalWall != nullptr && parsedWall != nullptr, "wall exists") &&
         expect(vecNear(parsedWall->positionMeters, originalWall->positionMeters),
                "wall position roundtrip") &&
         expect(vecNear(parsedWall->sizeMeters, originalWall->sizeMeters),
                "wall size roundtrip") &&
         expect(originalAnchor != nullptr && parsedAnchor != nullptr, "anchor exists") &&
         expect(vecNear(parsedAnchor->positionMeters, originalAnchor->positionMeters),
                "anchor position roundtrip") &&
         expect(originalDoor != nullptr && parsedDoor != nullptr, "door surface exists") &&
         expect(parsedDoor->runtimeOwnerStableName ==
                    originalDoor->runtimeOwnerStableName,
                "door owner roundtrip") &&
         expect(originalWallSurface != nullptr && parsedWallSurface != nullptr,
                "wall surface exists") &&
         expect(originalWallSurface->pointsMeters.size() == 8U,
                "original wall surface is box") &&
         expect(parsedWallSurface->pointsMeters.size() == 8U,
                "parsed wall surface keeps box points") &&
         expect(vecNear(parsedWallSurface->pointsMeters[4],
                        originalWallSurface->pointsMeters[4]),
                "wall surface top point roundtrip") &&
         expect(countSurfacesWithRole(parsed.room, iggy3d::RoomSpatialSurfaceRole::Walkable) ==
                    15U,
                "walkable parsed count") &&
         expect(countSurfacesWithRole(parsed.room, iggy3d::RoomSpatialSurfaceRole::Blocker) ==
                    21U,
                "blocker parsed count") &&
         expect(countSurfacesWithRole(
                    parsed.room, iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker) == 20U,
                "projectile parsed count");
}

bool traversalTagExportPreservesRoleFirstOrderAndDedup() {
  const iggy3d::AsciiRoomAssetTextResult written =
      iggy3d::writeAsciiRoomAssetText(buildTraversalTagExportRoom());

  return expect(written.ok, "tag export write ok") &&
         expect(countOccurrences(written.text, "traversal_tags = ") == 4U,
                "tag export surface count") &&
         expect(contains(written.text,
                         "traversal_tags = [\"walkable\", \"clamber\", \"vault\"]"),
                "walkable role tag first with input order") &&
         expect(contains(written.text,
                         "traversal_tags = [\"blocker\", \"debug_only\"]"),
                "blocker role tag first deduped") &&
         expect(contains(
                    written.text,
                    "traversal_tags = [\"projectile_blocker\", \"wire_walk\"]"),
                "projectile role tag first deduped") &&
         expect(contains(written.text,
                         "traversal_tags = [\"opening\", \"no_player\"]"),
                "opening role tag first deduped") &&
         expect(!contains(written.text, "magic"), "unknown traversal tag omitted");
}

bool invalidInputRejects() {
  iggy3d::RoomAsset valid = buildFixtureRoomAsset();

  iggy3d::RoomAsset missingRequired = valid;
  missingRequired.id.clear();
  const auto missingRequiredResult = iggy3d::writeAsciiRoomAssetText(missingRequired);

  iggy3d::AsciiRoomAssetTextConfig invalidConversion;
  invalidConversion.feetToMeters = 0.0F;
  const auto invalidConversionResult =
      iggy3d::writeAsciiRoomAssetText(valid, invalidConversion);

  iggy3d::RoomAsset noMeshes = valid;
  noMeshes.staticMeshes.clear();
  const auto noMeshesResult = iggy3d::writeAsciiRoomAssetText(noMeshes);

  iggy3d::RoomAsset noAnchors = valid;
  noAnchors.anchors.clear();
  const auto noAnchorsResult = iggy3d::writeAsciiRoomAssetText(noAnchors);

  iggy3d::RoomAsset unsafe = valid;
  unsafe.id = "bad\"id";
  const auto unsafeResult = iggy3d::writeAsciiRoomAssetText(unsafe);

  return expect(!missingRequiredResult.ok, "missing required rejected") &&
         expect(missingRequiredResult.status == "ascii_room_asset_text_missing_required",
                "missing required status") &&
         expect(!invalidConversionResult.ok, "invalid conversion rejected") &&
         expect(invalidConversionResult.status ==
                    "ascii_room_asset_text_invalid_conversion",
                "invalid conversion status") &&
         expect(!noMeshesResult.ok, "empty meshes rejected") &&
         expect(noMeshesResult.status == "ascii_room_asset_text_missing_static_meshes",
                "empty meshes status") &&
         expect(!noAnchorsResult.ok, "empty anchors rejected") &&
         expect(noAnchorsResult.status == "ascii_room_asset_text_missing_anchors",
                "empty anchors status") &&
         expect(!unsafeResult.ok, "unsafe string rejected") &&
         expect(unsafeResult.status == "ascii_room_asset_text_unsafe_string",
                "unsafe string status");
}

}  // namespace

int main() {
  bool ok = true;
  ok = fixtureExportsAndParsesBack() && ok;
  ok = traversalTagExportPreservesRoleFirstOrderAndDedup() && ok;
  ok = invalidInputRejects() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
