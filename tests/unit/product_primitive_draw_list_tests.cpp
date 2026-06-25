#include "app/iggy3d/ProductPrimitiveDrawList.hpp"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    return false;
  }
  return true;
}

iggy3d::SceneItem sceneItem(iggy3d::SceneItemKind kind,
                            std::uint64_t id,
                            const char* name) {
  iggy3d::SceneItem item;
  item.kind = kind;
  item.entityId = iggy3d::EntityId{id};
  item.stableName = name;
  item.visible = true;
  item.transform.position = {static_cast<float>(id), 0.0F, 0.0F};
  return item;
}

iggy3d::RoomSpatialSurface walkableSurface(const char* id,
                                           float x,
                                           std::vector<std::string> tags = {}) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = id;
  surface.sourceStaticMeshId = id;
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {{x - 0.5F, 0.0F, -0.5F},
                          {x + 0.5F, 0.0F, -0.5F},
                          {x + 0.5F, 0.0F, 0.5F},
                          {x - 0.5F, 0.0F, 0.5F}};
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = std::move(tags);
  surface.collisionMask = {"actor"};
  return surface;
}

iggy3d::RoomStaticMeshAsset wallMesh(const char* id, float x) {
  iggy3d::RoomStaticMeshAsset mesh;
  mesh.id = id;
  mesh.meshId = "wall_segment";
  mesh.materialId = "debug_wall";
  mesh.role = "wall";
  mesh.positionMeters = {x, 1.25F, 0.0F};
  mesh.sizeMeters = {1.0F, 2.5F, 1.0F};
  return mesh;
}

}  // namespace

int main() {
  iggy3d::SceneProjectionResult scene;
  scene.items.push_back(sceneItem(iggy3d::SceneItemKind::Player, 1, "player"));
  scene.items.push_back(sceneItem(iggy3d::SceneItemKind::Npc, 2, "training_dummy"));
  scene.items.push_back(sceneItem(iggy3d::SceneItemKind::Pickup, 3, "gold_key"));
  scene.items.push_back(
      sceneItem(iggy3d::SceneItemKind::ObjectiveMarker, 4, "objective_marker"));
  scene.items.push_back(sceneItem(iggy3d::SceneItemKind::DebugOnly, 5, "hidden_debug"));
  scene.items.back().visible = false;
  scene.items.push_back(sceneItem(iggy3d::SceneItemKind::DebugOnly, 6, "visible_debug"));
  scene.playerCount = 1;
  scene.pickupCount = 1;
  scene.markerCount = 1;

  iggy3d::DebugProjectionResult debug;
  debug.items.push_back({});
  debug.items.push_back({});

  const iggy3d::ProductPrimitiveDrawList list =
      iggy3d::buildProductPrimitiveDrawList(&scene, &debug);

  bool ok = true;
  ok &= expect(list.gridVisible, "grid visible with scene");
  ok &= expect(list.roomVisible, "room visible with scene items");
  ok &= expect(list.playerVisible, "player visible");
  ok &= expect(list.objectiveVisible, "objective visible from pickup/marker counts");
  ok &= expect(list.playerFocusIndicatorVisible, "player focus indicator visible");
  ok &= expect(list.items.size() == 6U, "five visible scene items plus focus");
  ok &= expect(list.itemCount == 6U, "draw item count includes focus");
  ok &= expect(list.playerCount == 1U, "player count");
  ok &= expect(list.targetMarkerCount == 2U, "npc and pickup target count");
  ok &= expect(list.objectiveMarkerCount == 1U, "objective count");
  ok &= expect(list.debugMarkerCount == 1U, "only actual debug draw items count");
  ok &= expect(list.items[0].kind == iggy3d::ProductPrimitiveDrawKind::PlayerMarker,
               "player maps to player marker");
  ok &= expect(list.items[1].kind ==
                   iggy3d::ProductPrimitiveDrawKind::PlayerFocusIndicator,
               "player focus follows player");
  ok &= expect(list.items[2].kind == iggy3d::ProductPrimitiveDrawKind::NpcMarker,
               "npc maps to npc marker");
  ok &= expect(list.items[3].kind == iggy3d::ProductPrimitiveDrawKind::PickupMarker,
               "pickup maps to pickup marker");
  ok &= expect(list.items[4].kind == iggy3d::ProductPrimitiveDrawKind::ObjectiveMarker,
               "objective maps to objective marker");
  ok &= expect(list.items[5].kind == iggy3d::ProductPrimitiveDrawKind::DebugMarker,
               "visible debug scene item maps to debug marker");
  ok &= expect(list.items[0].color.r == 80 && list.items[0].color.g == 170 &&
                   list.items[0].color.b == 236 && list.items[0].markerSize == 26.0F,
               "player visual role preserved");

  const iggy3d::ProductPrimitiveDrawList empty =
      iggy3d::buildProductPrimitiveDrawList(nullptr, nullptr);
  ok &= expect(!empty.gridVisible, "null scene has no draw-list grid claim");
  ok &= expect(empty.items.empty(), "null scene has no items");
  ok &= expect(empty.itemCount == 0U, "null scene item count");

  scene.items.pop_back();
  const iggy3d::ProductPrimitiveDrawList noDebugDrawItems =
      iggy3d::buildProductPrimitiveDrawList(&scene, &debug);
  ok &= expect(noDebugDrawItems.debugMarkerCount == 0U,
               "debug projection records alone do not claim drawn debug markers");

  iggy3d::RoomAsset room;
  room.spatialSurfaces.push_back(walkableSurface("floor_flat", 0.0F));
  room.spatialSurfaces.push_back(
      walkableSurface("floor_elevated", 1.0F, {"walkable", "elevated_floor"}));
  room.spatialSurfaces.push_back(
      walkableSurface("floor_ramp", 2.0F, {"walkable", "ramp"}));
  room.spatialSurfaces.push_back(
      walkableSurface("floor_blocked", 3.0F, {"walkable", "blocked_slope"}));
  room.staticMeshes.push_back(wallMesh("wall_1", 4.0F));

  const iggy3d::ProductPrimitiveDrawList roomList =
      iggy3d::buildProductPrimitiveDrawList(nullptr, nullptr, &room);
  ok &= expect(roomList.gridVisible, "room geometry shows grid");
  ok &= expect(roomList.roomVisible, "room geometry marks room visible");
  ok &= expect(roomList.items.size() == 5U, "room geometry item count");
  ok &= expect(roomList.itemCount == 5U, "room geometry draw count");
  ok &= expect(roomList.roomGeometryCount == 5U, "room geometry count");
  ok &= expect(roomList.floorTileCount == 1U, "floor tile count");
  ok &= expect(roomList.elevatedFloorTileCount == 1U, "elevated tile count");
  ok &= expect(roomList.rampTileCount == 1U, "ramp tile count");
  ok &= expect(roomList.blockedSlopeTileCount == 1U, "blocked slope tile count");
  ok &= expect(roomList.wallTileCount == 1U, "wall tile count");
  ok &= expect(roomList.items[0].kind == iggy3d::ProductPrimitiveDrawKind::FloorTile,
               "flat floor kind");
  ok &= expect(roomList.items[1].kind ==
                   iggy3d::ProductPrimitiveDrawKind::ElevatedFloorTile,
               "elevated floor kind");
  ok &= expect(roomList.items[2].kind == iggy3d::ProductPrimitiveDrawKind::RampTile,
               "ramp kind");
  ok &= expect(roomList.items[3].kind ==
                   iggy3d::ProductPrimitiveDrawKind::BlockedSlopeTile,
               "blocked slope kind");
  ok &= expect(roomList.items[4].kind == iggy3d::ProductPrimitiveDrawKind::WallTile,
               "wall kind");
  ok &= expect(roomList.items[2].color.g > roomList.items[0].color.g,
               "ramp receives distinct color");

  if (!ok) {
    return 1;
  }
  std::cout << "product_primitive_draw_list_tests=pass\n";
  return 0;
}
