#include "content/PackageLoader.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool firstRoomPackageLoadsRoomAssets() {
  const std::filesystem::path packagePath =
      std::filesystem::current_path() / "fixtures/demos/first_room/package.iggy3d.toml";
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage({packagePath.generic_string()});
  bool ok = expect(package.status == iggy3d::PackageLoadStatus::Ok, "package load") &&
            expect(package.manifest.assets.size() == 3U, "asset ref count") &&
            expect(package.rooms.size() == 1U, "room count") &&
            expect(!package.meshes.primitives.empty(), "mesh primitive count") &&
            expect(!package.materials.materials.empty(), "material count");
  if (!ok) {
    return false;
  }
  const iggy3d::RoomAsset& room = package.rooms.front();
  ok = ok && expect(room.id == "spawn_corridor", "room id") &&
       expect(room.sourceFile == "/Users/kogaryu/edi/artifacts/provingground/provingground.map.toml",
              "source toml") &&
       expect(room.sourceSubset == "spawn_room_corridor_stub", "source subset") &&
       expect(room.staticMeshes.size() > 0U, "static meshes") &&
       expect(room.anchors.size() >= 4U, "anchors") &&
       expect(!room.openings.empty() && room.openings.front().id == "out", "opening");
  bool floor = false;
  bool wall = false;
  bool opening = false;
  bool prop = false;
  for (const iggy3d::RoomStaticMeshAsset& mesh : room.staticMeshes) {
    floor = floor || mesh.role == "floor";
    wall = wall || mesh.role == "wall";
    opening = opening || mesh.role == "opening";
    prop = prop || mesh.role == "prop";
  }
  return ok && expect(floor, "floor role") && expect(wall, "wall role") &&
         expect(opening, "opening role") && expect(prop, "prop role");
}

}  // namespace

int main() {
  return firstRoomPackageLoadsRoomAssets() ? 0 : 1;
}
