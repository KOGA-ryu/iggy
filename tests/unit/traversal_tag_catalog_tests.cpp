#include "app/iggy3d/ascii_room/AsciiRoomAssetText.hpp"
#include "content/assets/RoomAsset.hpp"
#include "content/assets/TraversalTag.hpp"
#include "content/authoring/EditableRoomDocument.hpp"

#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool hasTag(const std::vector<std::string>& tags, std::string_view expected) {
  return std::find(tags.begin(), tags.end(), expected) != tags.end();
}

std::string quoted(std::string_view value) {
  return "\"" + std::string(value) + "\"";
}

iggy3d::RoomStaticMeshAsset staticMesh(std::string id, std::string role) {
  iggy3d::RoomStaticMeshAsset mesh;
  mesh.id = std::move(id);
  mesh.meshId = "unit_mesh";
  mesh.materialId = "unit_material";
  mesh.role = std::move(role);
  mesh.positionMeters = {0.0F, 0.0F, 0.0F};
  mesh.sizeMeters = {1.0F, 1.0F, 1.0F};
  return mesh;
}

iggy3d::RoomAsset baseRoomAsset() {
  iggy3d::RoomAsset room;
  room.id = "traversal_tag_catalog_room";
  room.version = 1;
  room.units = "m";
  room.source = "unit";
  room.sourceFile = "traversal_tag_catalog_tests";
  room.sourceSubset = "unit";
  room.staticMeshes.push_back(staticMesh("floor_mesh", "floor"));
  room.staticMeshes.push_back(staticMesh("wall_mesh", "wall"));
  room.staticMeshes.push_back(staticMesh("prop_mesh", "prop"));
  room.staticMeshes.push_back(staticMesh("opening_mesh", "opening"));
  iggy3d::RoomOpeningAsset opening;
  opening.id = "opening_1";
  opening.edge = "N";
  opening.kind = "door";
  opening.offsetMeters = 0.0F;
  opening.widthMeters = 1.0F;
  room.openings.push_back(opening);
  iggy3d::RoomAnchorAsset anchor;
  anchor.id = "spawn_anchor";
  anchor.kind = "spawn";
  anchor.runtimeStableName = "spawn_anchor";
  anchor.positionMeters = {0.0F, 0.0F, 0.0F};
  room.anchors.push_back(anchor);
  return room;
}

iggy3d::RoomSpatialSurface walkableSurface(std::vector<std::string> tags) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "walkable_surface";
  surface.sourceStaticMeshId = "floor_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {
      {-1.0F, 0.0F, -1.0F},
      {1.0F, 0.0F, -1.0F},
      {1.0F, 0.0F, 1.0F},
      {-1.0F, 0.0F, 1.0F},
  };
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = std::move(tags);
  surface.collisionMask = {"actor"};
  surface.blocksActor = false;
  surface.blocksProjectile = false;
  return surface;
}

iggy3d::RoomSpatialSurface blockerSurface(std::vector<std::string> tags) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "blocker_surface";
  surface.sourceStaticMeshId = "wall_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {
      {-1.0F, 0.0F, -0.1F},
      {1.0F, 0.0F, -0.1F},
      {1.0F, 2.0F, 0.1F},
      {-1.0F, 2.0F, 0.1F},
  };
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.traversalTags = std::move(tags);
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  surface.blocksProjectile = false;
  return surface;
}

iggy3d::RoomSpatialSurface projectileBlockerSurface(std::vector<std::string> tags) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "projectile_surface";
  surface.sourceStaticMeshId = "prop_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker;
  surface.pointsMeters = {
      {-0.5F, 0.0F, -0.5F},
      {0.5F, 0.0F, -0.5F},
      {0.5F, 1.0F, 0.5F},
      {-0.5F, 1.0F, 0.5F},
  };
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.traversalTags = std::move(tags);
  surface.collisionMask = {"projectile"};
  surface.blocksActor = false;
  surface.blocksProjectile = true;
  return surface;
}

iggy3d::RoomSpatialSurface openingSurface(std::vector<std::string> tags) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "opening_surface";
  surface.sourceStaticMeshId = "opening_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Opening;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Opening;
  surface.pointsMeters = {
      {-0.5F, 0.0F, 0.0F},
      {0.5F, 0.0F, 0.0F},
      {0.5F, 2.0F, 0.0F},
      {-0.5F, 2.0F, 0.0F},
  };
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.traversalTags = std::move(tags);
  surface.collisionMask = {};
  surface.blocksActor = false;
  surface.blocksProjectile = false;
  surface.openingId = "opening_1";
  return surface;
}

iggy3d::RoomSpatialSurface surfaceForCatalogTag(iggy3d::TraversalTag tag) {
  const std::string id(iggy3d::traversalTagId(tag));
  switch (tag) {
    case iggy3d::TraversalTag::Walkable:
      return walkableSurface({id});
    case iggy3d::TraversalTag::Blocker:
      return blockerSurface({id});
    case iggy3d::TraversalTag::ProjectileBlocker:
      return projectileBlockerSurface({id});
    case iggy3d::TraversalTag::Opening:
      return openingSurface({id});
    case iggy3d::TraversalTag::Clamber:
    case iggy3d::TraversalTag::ClamberCandidate:
    case iggy3d::TraversalTag::Vault:
    case iggy3d::TraversalTag::WireWalk:
    case iggy3d::TraversalTag::NoPlayer:
    case iggy3d::TraversalTag::DebugOnly:
      return blockerSurface({"blocker", id});
  }
  return blockerSurface({"blocker"});
}

bool catalogRoundTripsAndClassifiesTags() {
  const std::vector<std::string_view> expectedIds{
      "walkable",
      "blocker",
      "projectile_blocker",
      "opening",
      "clamber",
      "clamber_candidate",
      "vault",
      "wire_walk",
      "no_player",
      "debug_only",
  };

  bool ok = expect(iggy3d::allTraversalTags().size() == expectedIds.size(),
                   "catalog tag count");
  for (std::size_t index = 0; index < expectedIds.size(); ++index) {
    const iggy3d::TraversalTag tag = iggy3d::allTraversalTags()[index];
    const std::string_view id = iggy3d::traversalTagId(tag);
    ok = ok && expect(id == expectedIds[index], "catalog id order") &&
         expect(iggy3d::parseTraversalTag(id) == tag, "catalog parse round trip") &&
         expect(iggy3d::validTraversalTag(id), "catalog valid id");
  }

  ok = ok && expect(!iggy3d::parseTraversalTag("wall_jump").has_value(),
                    "unknown tag parse rejects") &&
       expect(!iggy3d::validTraversalTag("wall_jump"), "unknown tag invalid") &&
       expect(iggy3d::isStructuralTraversalTag(iggy3d::TraversalTag::Walkable),
              "walkable structural") &&
       expect(iggy3d::isStructuralTraversalTag(iggy3d::TraversalTag::Opening),
              "opening structural") &&
       expect(iggy3d::isMovementTraversalTag(iggy3d::TraversalTag::Clamber),
              "clamber movement") &&
       expect(iggy3d::isMovementTraversalTag(iggy3d::TraversalTag::Vault),
              "vault movement") &&
       expect(iggy3d::isMovementTraversalTag(iggy3d::TraversalTag::WireWalk),
              "wire walk movement") &&
       expect(!iggy3d::isMovementTraversalTag(iggy3d::TraversalTag::ClamberCandidate),
              "clamber candidate is not movement slot") &&
       expect(iggy3d::isAuthoringTraversalHintTag(iggy3d::TraversalTag::ClamberCandidate),
              "clamber candidate authoring hint");
  return ok;
}

bool editableRoomSemanticsUseCatalog() {
  bool ok = true;
  for (const iggy3d::TraversalTag tag : iggy3d::allTraversalTags()) {
    iggy3d::EditableRoomFloor floor;
    floor.id = "floor_" + std::string(iggy3d::traversalTagId(tag));
    floor.centerMeters = {0.0F, -0.05F, 0.0F};
    floor.sizeMeters = {2.0F, 0.1F, 2.0F};
    floor.semantics = iggy3d::defaultFloorSemantics("stone_floor");
    floor.semantics.traversalTags = {std::string(iggy3d::traversalTagId(tag))};

    iggy3d::EditableRoomSession session;
    const iggy3d::RoomEditResult result =
        session.submit(iggy3d::addFloorCommand(floor));
    ok = ok && expect(result.status == iggy3d::RoomEditStatus::Applied,
                      "editable room accepts catalog tag");
  }

  iggy3d::EditableRoomFloor badFloor;
  badFloor.id = "bad_floor";
  badFloor.centerMeters = {0.0F, -0.05F, 0.0F};
  badFloor.sizeMeters = {2.0F, 0.1F, 2.0F};
  badFloor.semantics = iggy3d::defaultFloorSemantics("stone_floor");
  badFloor.semantics.traversalTags = {"wall_jump"};
  iggy3d::EditableRoomSession session;
  const iggy3d::RoomEditResult result =
      session.submit(iggy3d::addFloorCommand(badFloor));
  ok = ok && expect(result.status == iggy3d::RoomEditStatus::InvalidPrimitive,
                    "editable room rejects unknown tag");
  return ok;
}

bool roomAssetAndAsciiWriterUseCatalog() {
  bool ok = true;
  for (const iggy3d::TraversalTag tag : iggy3d::allTraversalTags()) {
    iggy3d::RoomAsset room = baseRoomAsset();
    room.spatialSurfaces.push_back(surfaceForCatalogTag(tag));

    const std::string id(iggy3d::traversalTagId(tag));
    const iggy3d::AsciiRoomAssetTextResult write =
        iggy3d::writeAsciiRoomAssetText(room);
    ok = ok && expect(write.ok, "ascii room asset write ok") &&
         expect(write.text.find(quoted(id)) != std::string::npos,
                "ascii writer preserves catalog tag");

    const iggy3d::RoomAssetParseResult parse =
        iggy3d::parseRoomAssetText(write.text);
    ok = ok && expect(parse.ok, parse.reason) &&
         expect(!parse.room.spatialSurfaces.empty(), "parsed surface exists") &&
         expect(hasTag(parse.room.spatialSurfaces.front().traversalTags, id),
                "room asset parse preserves catalog tag");
  }

  const std::string badText = R"([room]
id = "bad_tags"
version = 1
units = "ft"
source = "unit"
source_file = "unit"
source_subset = "unit"

[[static_meshes]]
id = "floor_mesh"
mesh = "unit_mesh"
role = "floor"
position_ft = [0.0, 0.0, 0.0]
size_ft = [1.0, 0.1, 1.0]
material = "unit_material"

[[spatial_surfaces]]
id = "bad_surface"
source_static_mesh = "floor_mesh"
shape = "plane"
role = "walkable"
points_ft = [[-1.0, 0.0, -1.0], [1.0, 0.0, -1.0], [1.0, 0.0, 1.0], [-1.0, 0.0, 1.0]]
normal = [0.0, 1.0, 0.0]
traversal_tags = ["walkable", "wall_jump"]
collision_mask = ["actor"]
blocks_actor = false
blocks_projectile = false
opening_id = ""
runtime_owner_stable_name = ""

[[anchors]]
id = "spawn_anchor"
kind = "spawn"
position_ft = [0.0, 0.0, 0.0]
runtime_stable_name = "spawn_anchor"
)";
  const iggy3d::RoomAssetParseResult badParse =
      iggy3d::parseRoomAssetText(badText);
  ok = ok && expect(!badParse.ok, "room asset rejects unknown traversal tag") &&
       expect(badParse.reason == "room_unknown_traversal_tag",
              "unknown traversal tag reason");
  return ok;
}

}  // namespace

int main() {
  return catalogRoundTripsAndClassifiesTags() && editableRoomSemanticsUseCatalog() &&
                 roomAssetAndAsciiWriterUseCatalog()
             ? 0
             : 1;
}
