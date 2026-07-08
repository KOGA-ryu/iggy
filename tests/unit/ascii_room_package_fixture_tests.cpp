#include "content/PackageLoader.hpp"

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view kSourceRoomFixturePath =
    "fixtures/rooms/ascii/training_room.room.iggy3d.toml";
constexpr std::string_view kAsciiTrainingRoomPackageRoomFixturePath =
    "fixtures/demos/ascii_training_room/assets/rooms/training_room.room.iggy3d.toml";
constexpr std::string_view kAsciiTrainingRoomPackagePath =
    "fixtures/demos/ascii_training_room/package.iggy3d.toml";
constexpr std::string_view kNpcVisionLabPackageRoomFixturePath =
    "fixtures/demos/npc_vision_lab/assets/rooms/training_room.room.iggy3d.toml";
constexpr std::string_view kNpcVisionLabPackagePath =
    "fixtures/demos/npc_vision_lab/package.iggy3d.toml";

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

std::string readTextFile(const std::filesystem::path& path) {
  std::ifstream input(path);
  std::ostringstream out;
  out << input.rdbuf();
  return out.str();
}

std::size_t countSurfacesWithRole(const iggy3d::RoomAsset& room,
                                  iggy3d::RoomSpatialSurfaceRole role) {
  std::size_t count = 0;
  for (const iggy3d::RoomSpatialSurface& surface : room.spatialSurfaces) {
    if (surface.role == role) {
      ++count;
    }
  }
  return count;
}

const iggy3d::RoomAnchorAsset* findAnchor(const iggy3d::RoomAsset& room,
                                          std::string_view id) {
  for (const iggy3d::RoomAnchorAsset& anchor : room.anchors) {
    if (anchor.id == id) {
      return &anchor;
    }
  }
  return nullptr;
}

bool anchorHasKind(const iggy3d::RoomAsset& room,
                   std::string_view id,
                   std::string_view kind) {
  const iggy3d::RoomAnchorAsset* anchor = findAnchor(room, id);
  return anchor != nullptr && anchor->kind == kind;
}

bool surfaceHasTraversalTag(const iggy3d::RoomSpatialSurface& surface,
                            std::string_view tag) {
  for (const std::string& traversalTag : surface.traversalTags) {
    if (traversalTag == tag) {
      return true;
    }
  }
  return false;
}

std::size_t countBlockerSurfacesWithTraversalTag(
    const iggy3d::RoomAsset& room,
    std::string_view tag) {
  std::size_t count = 0;
  for (const iggy3d::RoomSpatialSurface& surface : room.spatialSurfaces) {
    if (surface.role == iggy3d::RoomSpatialSurfaceRole::Blocker &&
        surfaceHasTraversalTag(surface, tag)) {
      ++count;
    }
  }
  return count;
}

bool hasMeshPrimitive(const iggy3d::MeshAssetLibrary& library, std::string_view id) {
  for (const iggy3d::MeshAssetPrimitive& primitive : library.primitives) {
    if (primitive.id == id) {
      return true;
    }
  }
  return false;
}

bool hasMaterial(const iggy3d::MaterialAssetLibrary& library, std::string_view id) {
  for (const iggy3d::MaterialAssetRecord& material : library.materials) {
    if (material.id == id) {
      return true;
    }
  }
  return false;
}

bool trainingRoomCoreFactsMatch(const iggy3d::RoomAsset& room) {
  return expect(room.id == "training_room_ascii", "room id") &&
         expect(room.source == "iggy3d.ascii_room", "room source") &&
         expect(room.sourceFile == "fixtures/rooms/ascii/training_room.iggyroom.txt",
                "room source file") &&
         expect(room.sourceSubset == "ascii_room_authoring", "room source subset") &&
         expect(room.staticMeshes.size() == 36U, "room static mesh count") &&
         expect(room.anchors.size() == 5U, "room anchor count") &&
         expect(room.spatialSurfaces.size() == 56U, "room spatial surface count") &&
         expect(countSurfacesWithRole(room,
                                      iggy3d::RoomSpatialSurfaceRole::Walkable) == 15U,
                "walkable surface count") &&
         expect(countSurfacesWithRole(room,
                                      iggy3d::RoomSpatialSurfaceRole::Blocker) == 21U,
                "blocker surface count") &&
         expect(countSurfacesWithRole(
                    room,
                    iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker) == 20U,
                "projectile blocker surface count") &&
         expect(countBlockerSurfacesWithTraversalTag(room,
                                                     "clamber_candidate") == 20U,
                "blocker clamber_candidate surface count") &&
         expect(anchorHasKind(room, "marker_player_spawn_r1_c1", "spawn"),
                "player spawn anchor") &&
         expect(anchorHasKind(room, "marker_npc_spawn_r1_c4", "npc"), "npc anchor") &&
         expect(anchorHasKind(room, "marker_treasure_r2_c4", "treasure"),
                "treasure anchor") &&
         expect(anchorHasKind(room, "marker_door_r2_c2", "door"), "door anchor") &&
         expect(anchorHasKind(room, "marker_exit_r3_c3", "exit"), "exit anchor");
}

bool asciiTrainingRoomPackageLoadsFixtureRoom() {
  const std::string sourceRoomText = readTextFile(std::filesystem::path{kSourceRoomFixturePath});
  const std::string packageRoomText =
      readTextFile(std::filesystem::path{kAsciiTrainingRoomPackageRoomFixturePath});
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage({std::string(kAsciiTrainingRoomPackagePath)});

  bool ok = expect(!sourceRoomText.empty(), "source room fixture not empty") &&
            expect(packageRoomText == sourceRoomText, "package room text parity") &&
            expect(package.status == iggy3d::PackageLoadStatus::Ok, "package load ok") &&
            expect(package.manifest.packageId == "iggy3d.ascii_training_room",
                   "package id") &&
            expect(package.scenario.scenarioId == "ascii_training_room.runtime_loop",
                   "scenario id") &&
            expect(package.manifest.assets.size() == 3U, "asset ref count") &&
            expect(package.rooms.size() == 1U, "one room") &&
            expect(!package.meshes.primitives.empty(), "mesh library non-empty") &&
            expect(!package.materials.materials.empty(), "material library non-empty") &&
            expect(hasMeshPrimitive(package.meshes, "floor_rect"), "floor mesh primitive") &&
            expect(hasMeshPrimitive(package.meshes, "wall_segment"), "wall mesh primitive") &&
            expect(hasMaterial(package.materials, "debug_floor"), "debug floor material") &&
            expect(hasMaterial(package.materials, "debug_wall"), "debug wall material");
  if (!ok || package.rooms.empty()) {
    return false;
  }

  const iggy3d::RoomAsset& room = package.rooms.front();
  return trainingRoomCoreFactsMatch(room);
}

bool npcVisionLabPackageLoadsFixtureRoom() {
  const std::string sourceRoomText = readTextFile(std::filesystem::path{kSourceRoomFixturePath});
  const std::string packageRoomText =
      readTextFile(std::filesystem::path{kNpcVisionLabPackageRoomFixturePath});
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage({std::string(kNpcVisionLabPackagePath)});

  bool ok = expect(!sourceRoomText.empty(), "npc source room fixture not empty") &&
            expect(packageRoomText == sourceRoomText, "npc package room text parity") &&
            expect(package.status == iggy3d::PackageLoadStatus::Ok,
                   "npc package load ok") &&
            expect(package.manifest.packageId == "iggy3d.npc_vision_lab",
                   "npc package id") &&
            expect(package.scenario.scenarioId == "npc_vision_lab.runtime_loop",
                   "npc scenario id") &&
            expect(package.manifest.assets.size() == 3U, "npc asset ref count") &&
            expect(package.rooms.size() == 1U, "npc one room") &&
            expect(!package.meshes.primitives.empty(), "npc mesh library non-empty") &&
            expect(!package.materials.materials.empty(), "npc material library non-empty") &&
            expect(hasMeshPrimitive(package.meshes, "floor_rect"),
                   "npc floor mesh primitive") &&
            expect(hasMeshPrimitive(package.meshes, "wall_segment"),
                   "npc wall mesh primitive") &&
            expect(hasMaterial(package.materials, "debug_floor"),
                   "npc debug floor material") &&
            expect(hasMaterial(package.materials, "debug_wall"),
                   "npc debug wall material");
  if (!ok || package.rooms.empty()) {
    return false;
  }

  const iggy3d::RoomAsset& room = package.rooms.front();
  return trainingRoomCoreFactsMatch(room);
}

}  // namespace

int main() {
  return asciiTrainingRoomPackageLoadsFixtureRoom() &&
                 npcVisionLabPackageLoadsFixtureRoom()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
