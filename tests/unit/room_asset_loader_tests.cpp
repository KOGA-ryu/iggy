#include "content/assets/RoomAsset.hpp"
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

std::string baseRoomText(std::string_view spatialRows) {
  std::string text = R"([room]
id = "spawn_corridor"
version = 1
units = "ft"
source = "edi.provingground"
source_file = "/Users/kogaryu/edi/artifacts/provingground/provingground.map.toml"
source_subset = "spawn_room_corridor_stub"

[conversion]
feet_to_meters = 0.3048

[[static_meshes]]
id = "spawn_floor"
mesh = "floor_rect"
role = "floor"
position_ft = [10.0, 0.0, 9.0]
size_ft = [20.0, 0.10, 18.0]
material = "floor_stone"

[[static_meshes]]
id = "north_wall"
mesh = "wall_segment"
role = "wall"
position_ft = [10.0, 4.5, 0.5]
size_ft = [20.0, 9.0, 1.0]
material = "stone_wall"

[[static_meshes]]
id = "spawn_crate"
mesh = "crate_box"
role = "prop"
position_ft = [4.0, 1.0, 16.0]
size_ft = [2.0, 2.0, 2.0]
material = "prop_wood"

[[static_meshes]]
id = "spawn_out_frame_top"
mesh = "door_frame"
role = "opening"
position_ft = [19.5, 8.0, 9.0]
size_ft = [1.0, 2.0, 10.0]
material = "opening_trim"

[[openings]]
id = "out"
edge = "E"
kind = "door"
offset_ft = 9.0
width_ft = 10.0

)";
  text += spatialRows;
  text += R"(
[[anchors]]
id = "player_spawn"
kind = "spawn"
position_ft = [10.0, 0.0, 9.0]
)";
  return text;
}

std::string walkableSurface(std::string_view id = "surface_a",
                            std::string_view sourceMesh = "spawn_floor",
                            std::string_view normal = "[0.0, 1.0, 0.0]",
                            std::string_view points =
                                "[[0.0, 0.05, 0.0], [20.0, 0.05, 0.0], "
                                "[20.0, 0.05, 18.0], [0.0, 0.05, 18.0]]",
                            std::string_view traversalTags = "[\"walkable\"]") {
  std::string text = "\n[[spatial_surfaces]]\n";
  text += "id = \"";
  text += id;
  text += "\"\nsource_static_mesh = \"";
  text += sourceMesh;
  text += "\"\nshape = \"plane\"\nrole = \"walkable\"\npoints_ft = ";
  text += points;
  text += "\nnormal = ";
  text += normal;
  text += "\ntraversal_tags = ";
  text += traversalTags;
  text +=
      "\ncollision_mask = [\"actor\"]\nblocks_actor = false\nblocks_projectile = false\nopening_id = \"\"\n";
  return text;
}

std::string openingSurface(std::string_view openingId = "out",
                           bool blocksActor = false,
                           bool blocksProjectile = false) {
  std::string text = R"(
[[spatial_surfaces]]
id = "opening_surface"
source_static_mesh = "spawn_out_frame_top"
shape = "opening"
role = "opening"
points_ft = [[19.5, 0.0, 4.0], [19.5, 0.0, 14.0], [19.5, 8.0, 14.0], [19.5, 8.0, 4.0]]
normal = [1.0, 0.0, 0.0]
traversal_tags = ["opening"]
collision_mask = []
)";
  text += std::string("blocks_actor = ") + (blocksActor ? "true" : "false") + "\n";
  text += std::string("blocks_projectile = ") + (blocksProjectile ? "true" : "false") + "\n";
  text += "opening_id = \"";
  text += openingId;
  text += "\"\n";
  return text;
}

std::string projectileSurface() {
  return R"(
[[spatial_surfaces]]
id = "projectile_surface"
source_static_mesh = "spawn_crate"
shape = "box"
role = "projectile_blocker"
points_ft = [[3.0, 0.0, 15.0], [5.0, 0.0, 15.0], [5.0, 2.0, 17.0], [3.0, 2.0, 17.0]]
normal = [0.0, 0.0, -1.0]
traversal_tags = ["projectile_blocker"]
collision_mask = ["projectile"]
blocks_actor = false
blocks_projectile = true
opening_id = ""
)";
}

bool parseFailsWith(std::string_view spatialRows, std::string_view reason) {
  const iggy3d::RoomAssetParseResult result =
      iggy3d::parseRoomAssetText(baseRoomText(spatialRows));
  return expect(!result.ok, "parse should fail") && expect(result.reason == reason, reason);
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
       expect(!room.openings.empty() && room.openings.front().id == "out", "opening") &&
       expect(room.spatialSurfaces.size() >= 4U, "spatial surfaces");
  bool floor = false;
  bool wall = false;
  bool opening = false;
  bool prop = false;
  std::size_t walkableSurfaces = 0;
  std::size_t blockerSurfaces = 0;
  std::size_t projectileBlockers = 0;
  std::size_t openingSurfaces = 0;
  for (const iggy3d::RoomStaticMeshAsset& mesh : room.staticMeshes) {
    floor = floor || mesh.role == "floor";
    wall = wall || mesh.role == "wall";
    opening = opening || mesh.role == "opening";
    prop = prop || mesh.role == "prop";
  }
  for (const iggy3d::RoomSpatialSurface& surface : room.spatialSurfaces) {
    walkableSurfaces += surface.role == iggy3d::RoomSpatialSurfaceRole::Walkable ? 1U : 0U;
    blockerSurfaces += surface.role == iggy3d::RoomSpatialSurfaceRole::Blocker ? 1U : 0U;
    projectileBlockers +=
        surface.role == iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker ? 1U : 0U;
    openingSurfaces += surface.role == iggy3d::RoomSpatialSurfaceRole::Opening ? 1U : 0U;
  }
  return ok && expect(floor, "floor role") && expect(wall, "wall role") &&
         expect(opening, "opening role") && expect(prop, "prop role") &&
         expect(walkableSurfaces >= 1U, "walkable surfaces") &&
         expect(blockerSurfaces >= 1U, "blocker surfaces") &&
         expect(projectileBlockers >= 1U, "projectile blocker surfaces") &&
         expect(openingSurfaces >= 1U, "opening surfaces");
}

bool movementPlaygroundPackageLoadsMovementSemantics() {
  const std::filesystem::path packagePath =
      std::filesystem::current_path() /
      "fixtures/demos/movement_playground/package.iggy3d.toml";
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage({packagePath.generic_string()});
  bool ok = expect(package.status == iggy3d::PackageLoadStatus::Ok,
                   "movement playground package load") &&
            expect(package.manifest.assets.size() == 3U, "movement asset ref count") &&
            expect(package.rooms.size() == 1U, "movement room count") &&
            expect(!package.meshes.primitives.empty(), "movement mesh primitive count") &&
            expect(!package.materials.materials.empty(), "movement material count");
  if (!ok) {
    return false;
  }

  const iggy3d::RoomAsset& room = package.rooms.front();
  ok = ok && expect(room.id == "movement_playground", "movement room id") &&
       expect(room.sourceSubset == "movement_playground_v1", "movement source subset") &&
       expect(room.staticMeshes.size() >= 70U, "movement static mesh count") &&
       expect(room.anchors.size() >= 5U, "movement anchors") &&
       expect(room.spatialSurfaces.size() >= 10U, "movement spatial surfaces");

  bool floor = false;
  bool wall = false;
  bool grid = false;
  bool ledge = false;
  bool rail = false;
  bool hazard = false;
  bool dash = false;
  bool spell = false;
  bool largeFloor = false;
  std::size_t gridMeshes = 0;
  std::size_t walkableSurfaces = 0;
  std::size_t blockerSurfaces = 0;
  std::size_t projectileBlockers = 0;
  for (const iggy3d::RoomStaticMeshAsset& mesh : room.staticMeshes) {
    floor = floor || mesh.role == "floor";
    if (mesh.id == "main_floor") {
      largeFloor = mesh.sizeMeters.x > 29.0F && mesh.sizeMeters.z > 29.0F;
    }
    wall = wall || mesh.role == "wall";
    grid = grid || mesh.role == "grid";
    ledge = ledge || mesh.role == "ledge";
    rail = rail || mesh.role == "rail";
    hazard = hazard || mesh.role == "hazard";
    dash = dash || mesh.role == "dash";
    spell = spell || mesh.role == "spell";
    gridMeshes += mesh.role == "grid" ? 1U : 0U;
  }
  for (const iggy3d::RoomSpatialSurface& surface : room.spatialSurfaces) {
    walkableSurfaces += surface.role == iggy3d::RoomSpatialSurfaceRole::Walkable ? 1U : 0U;
    blockerSurfaces += surface.role == iggy3d::RoomSpatialSurfaceRole::Blocker ? 1U : 0U;
    projectileBlockers +=
        surface.role == iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker ? 1U : 0U;
  }
  return ok && expect(floor, "movement floor role") &&
         expect(largeFloor, "movement large floor") && expect(wall, "movement wall role") &&
         expect(grid && gridMeshes >= 65U, "movement grid role") &&
         expect(ledge, "movement ledge role") && expect(rail, "movement rail role") &&
         expect(hazard, "movement hazard role") && expect(dash, "movement dash role") &&
         expect(spell, "movement spell role") &&
         expect(walkableSurfaces >= 6U, "movement walkable surfaces") &&
         expect(blockerSurfaces >= 5U, "movement blocker surfaces") &&
         expect(projectileBlockers >= 2U, "movement projectile blockers");
}

bool invalidSpatialSurfaceCasesReject() {
  bool ok = true;
  ok = ok && parseFailsWith(walkableSurface("dupe") + walkableSurface("dupe"),
                            "room_duplicate_spatial_surface_id");
  ok = ok && parseFailsWith(walkableSurface("bad_normal", "spawn_floor",
                                            "[0.0, 0.0, 0.0]"),
                            "room_invalid_spatial_surface_normal");
  ok = ok && parseFailsWith(walkableSurface("bad_point", "spawn_floor",
                                            "[0.0, 1.0, 0.0]",
                                            "[[0.0, nan, 0.0], [1.0, 0.0, 0.0], "
                                            "[1.0, 0.0, 1.0]]"),
                            "room_invalid_spatial_surface_point");
  ok = ok && parseFailsWith(walkableSurface("bad_tag", "spawn_floor",
                                            "[0.0, 1.0, 0.0]",
                                            "[[0.0, 0.05, 0.0], [20.0, 0.05, 0.0], "
                                            "[20.0, 0.05, 18.0], [0.0, 0.05, 18.0]]",
                                            "[\"walkable\", \"magic\"]"),
                            "room_unknown_traversal_tag");
  ok = ok && parseFailsWith(openingSurface("out", true, false),
                            "room_invalid_opening_surface");
  ok = ok && parseFailsWith(walkableSurface("missing_mesh", "missing_mesh"),
                            "room_unknown_spatial_surface_mesh");
  ok = ok && parseFailsWith(openingSurface("missing_opening"),
                            "room_unknown_spatial_surface_opening");
  return ok;
}

bool spatialSurfaceRolesRemainDistinct() {
  const iggy3d::RoomAssetParseResult result =
      iggy3d::parseRoomAssetText(baseRoomText(openingSurface() + projectileSurface()));
  bool sawOpening = false;
  bool sawProjectile = false;
  bool openingBlocks = true;
  bool projectileBlocksActor = true;
  for (const iggy3d::RoomSpatialSurface& surface : result.room.spatialSurfaces) {
    if (surface.role == iggy3d::RoomSpatialSurfaceRole::Opening) {
      sawOpening = true;
      openingBlocks = surface.blocksActor || surface.blocksProjectile;
    }
    if (surface.role == iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker) {
      sawProjectile = true;
      projectileBlocksActor = surface.blocksActor;
    }
  }
  return expect(result.ok, "distinct role parse") &&
         expect(sawOpening && !openingBlocks, "opening is non blocker") &&
         expect(sawProjectile && !projectileBlocksActor, "projectile blocker distinct");
}

}  // namespace

int main() {
  return firstRoomPackageLoadsRoomAssets() && movementPlaygroundPackageLoadsMovementSemantics() &&
                 invalidSpatialSurfaceCasesReject() && spatialSurfaceRolesRemainDistinct()
             ? 0
             : 1;
}
