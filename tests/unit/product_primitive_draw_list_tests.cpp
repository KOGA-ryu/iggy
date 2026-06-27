#include "app/iggy3d/ProductPrimitiveDrawList.hpp"

#include <iostream>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "app/iggy3d/gameplay/ProductActiveRoomCollision.hpp"
#include "app/iggy3d/room_editor/Presentation.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    return false;
  }
  return true;
}

bool near(float lhs, float rhs) {
  return std::fabs(lhs - rhs) <= 0.0001F;
}

bool expectVec3(iggy3d::Vec3 actual, iggy3d::Vec3 expected, const char* message) {
  return expect(near(actual.x, expected.x) && near(actual.y, expected.y) &&
                    near(actual.z, expected.z),
                message);
}

bool expectAabb(iggy3d::Aabb3 actual,
                iggy3d::Aabb3 expected,
                const char* message) {
  return expectVec3(actual.min, expected.min, message) &&
         expectVec3(actual.max, expected.max, message);
}

bool expectColor(iggy3d::ProductPrimitiveColor actual,
                 iggy3d::ProductPrimitiveColor expected,
                 const char* message) {
  return expect(actual.r == expected.r && actual.g == expected.g &&
                    actual.b == expected.b,
                message);
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

iggy3d::RoomStaticMeshAsset doorMesh(const char* id, float x) {
  iggy3d::RoomStaticMeshAsset mesh;
  mesh.id = id;
  mesh.meshId = "door_panel";
  mesh.materialId = "debug_door";
  mesh.role = "door";
  mesh.positionMeters = {x, 1.0F, 0.0F};
  mesh.sizeMeters = {0.2F, 2.0F, 1.0F};
  return mesh;
}

iggy3d::RoomSpatialSurface doorBlocker(const char* id,
                                       const char* meshId,
                                       const char* ownerStableName,
                                       float x) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = id;
  surface.sourceStaticMeshId = meshId;
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {{x - 0.1F, 0.0F, -0.5F},
                          {x + 0.1F, 0.0F, -0.5F},
                          {x + 0.1F, 2.0F, 0.5F},
                          {x - 0.1F, 2.0F, 0.5F}};
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.collisionMask = {"actor", "projectile"};
  surface.blocksActor = true;
  surface.blocksProjectile = true;
  surface.runtimeOwnerStableName = ownerStableName;
  return surface;
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

  iggy3d::DebugProjectionResult physicsDebug;
  iggy3d::DebugProjectionItem physicsAabb;
  physicsAabb.kind = iggy3d::DebugProjectionKind::PhysicsAabb;
  physicsAabb.actor = iggy3d::EntityId{41};
  physicsAabb.hasBounds = true;
  physicsAabb.worldBounds =
      iggy3d::aabbFromCenterExtents({10.0F, 1.0F, 2.0F}, {1.0F, 2.0F, 3.0F});
  physicsAabb.valueCode = "solid";
  physicsDebug.items.push_back(physicsAabb);

  iggy3d::DebugProjectionItem physicsContact;
  physicsContact.kind = iggy3d::DebugProjectionKind::PhysicsContactNormal;
  physicsContact.actor = iggy3d::EntityId{42};
  physicsContact.hasWorldPoint = true;
  physicsContact.worldPoint = {11.0F, 1.5F, 2.5F};
  physicsContact.valueCode = "sensor";
  physicsDebug.items.push_back(physicsContact);

  iggy3d::DebugProjectionItem physicsPair;
  physicsPair.kind = iggy3d::DebugProjectionKind::PhysicsBroadphasePair;
  physicsPair.actor = iggy3d::EntityId{43};
  physicsPair.hasWorldPoint = true;
  physicsPair.worldPoint = {12.0F, 2.5F, 3.5F};
  physicsPair.valueCode = "solid";
  physicsDebug.items.push_back(physicsPair);

  physicsDebug.items.push_back({});  // Non-physics projection item.
  iggy3d::DebugProjectionItem missingAabb = physicsAabb;
  missingAabb.hasBounds = false;
  physicsDebug.items.push_back(missingAabb);
  iggy3d::DebugProjectionItem missingContact = physicsContact;
  missingContact.hasWorldPoint = false;
  physicsDebug.items.push_back(missingContact);
  iggy3d::DebugProjectionItem missingPair = physicsPair;
  missingPair.hasWorldPoint = false;
  physicsDebug.items.push_back(missingPair);

  const iggy3d::ProductPrimitiveDrawList physicsDebugList =
      iggy3d::buildProductPrimitiveDrawList(&scene, &physicsDebug);
  ok &= expect(physicsDebugList.items.size() == 8U,
               "physics debug appends three valid items after scene items");
  ok &= expect(physicsDebugList.itemCount == 8U,
               "physics debug items count in draw list");
  ok &= expect(physicsDebugList.debugMarkerCount == 3U,
               "physics debug items increment aggregate debug marker count");
  ok &= expect(physicsDebugList.physicsDebugVisible,
               "physics debug visible when items append");
  ok &= expect(physicsDebugList.physicsDebugItemCount == 3U,
               "physics debug aggregate item count");
  ok &= expect(physicsDebugList.physicsAabbDebugCount == 1U,
               "physics AABB debug count");
  ok &= expect(physicsDebugList.physicsContactNormalDebugCount == 1U,
               "physics contact normal debug count");
  ok &= expect(physicsDebugList.physicsBroadphasePairDebugCount == 1U,
               "physics broadphase pair debug count");
  ok &= expect(physicsDebugList.items[4].kind ==
                   iggy3d::ProductPrimitiveDrawKind::ObjectiveMarker,
               "scene item order remains before physics debug");
  const iggy3d::ProductPrimitiveDrawItem& aabbItem = physicsDebugList.items[5];
  const iggy3d::ProductPrimitiveDrawItem& contactItem = physicsDebugList.items[6];
  const iggy3d::ProductPrimitiveDrawItem& pairItem = physicsDebugList.items[7];
  ok &= expect(aabbItem.kind == iggy3d::ProductPrimitiveDrawKind::PhysicsAabbDebug,
               "physics AABB maps to product primitive kind");
  ok &= expect(contactItem.kind ==
                   iggy3d::ProductPrimitiveDrawKind::PhysicsContactNormalDebug,
               "physics contact normal maps to product primitive kind");
  ok &= expect(pairItem.kind ==
                   iggy3d::ProductPrimitiveDrawKind::PhysicsBroadphasePairDebug,
               "physics broadphase pair maps to product primitive kind");
  ok &= expect(aabbItem.entityId.value == 41U, "AABB actor id copied");
  ok &= expect(aabbItem.stableName == "physics.aabb", "AABB fallback label");
  ok &= expectVec3(aabbItem.worldPosition, {10.0F, 1.0F, 2.0F},
                   "AABB position is bounds center");
  ok &= expectAabb(aabbItem.worldBounds, physicsAabb.worldBounds,
                   "AABB bounds copied");
  ok &= expect(aabbItem.markerSize == 34.0F, "solid AABB marker size");
  ok &= expectColor(aabbItem.color, {105, 205, 228}, "solid AABB color");
  ok &= expect(contactItem.stableName == "physics.contact_normal",
               "contact fallback label");
  ok &= expectVec3(contactItem.worldPosition, physicsContact.worldPoint,
                   "contact world point copied");
  ok &= expectAabb(contactItem.worldBounds,
                   iggy3d::aabbFromCenterExtents(physicsContact.worldPoint,
                                                 {0.08F, 0.08F, 0.08F}),
                   "contact point bounds");
  ok &= expect(contactItem.markerSize == 18.0F, "contact marker size");
  ok &= expectColor(contactItem.color, {245, 214, 96}, "sensor contact color");
  ok &= expect(pairItem.stableName == "physics.broadphase_pair",
               "pair fallback label");
  ok &= expectVec3(pairItem.worldPosition, physicsPair.worldPoint,
                   "pair world point copied");
  ok &= expectAabb(pairItem.worldBounds,
                   iggy3d::aabbFromCenterExtents(physicsPair.worldPoint,
                                                 {0.10F, 0.10F, 0.10F}),
                   "pair point bounds");
  ok &= expect(pairItem.markerSize == 14.0F, "pair marker size");
  ok &= expectColor(pairItem.color, {166, 184, 177}, "solid pair color");

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

  iggy3d::SceneProjectionResult doorScene;
  doorScene.items.push_back(
      sceneItem(iggy3d::SceneItemKind::Interactable, 7, "marker_door_r1_c2"));
  doorScene.items.back().entityKind = iggy3d::EntityKind::Door;
  doorScene.items.back().targetable = true;
  doorScene.items.back().interactable = true;
  const iggy3d::ProductPrimitiveDrawList closedDoorSceneList =
      iggy3d::buildProductPrimitiveDrawList(&doorScene, nullptr);
  ok &= expect(closedDoorSceneList.doorVisible, "closed scene door visible");
  ok &= expect(closedDoorSceneList.closedDoorVisible, "closed scene door marked closed");
  ok &= expect(!closedDoorSceneList.openDoorVisible,
               "closed scene door not marked open");
  ok &= expect(closedDoorSceneList.doorMarkerCount == 1U,
               "closed scene door count");
  ok &= expect(closedDoorSceneList.closedDoorMarkerCount == 1U,
               "closed scene door closed count");
  ok &= expect(closedDoorSceneList.openDoorMarkerCount == 0U,
               "closed scene door open count");
  ok &= expect(closedDoorSceneList.targetMarkerCount == 1U,
               "closed scene door remains target marker");
  ok &= expect(closedDoorSceneList.items[0].kind ==
                   iggy3d::ProductPrimitiveDrawKind::DoorMarker,
               "door entity maps to door marker");
  ok &= expect(closedDoorSceneList.items[0].doorClosed,
               "door entity item is closed");

  iggy3d::RoomAsset doorRoom;
  doorRoom.staticMeshes.push_back(doorMesh("marker_door_r1_c2_panel", 5.0F));
  doorRoom.spatialSurfaces.push_back(doorBlocker("marker_door_r1_c2_door_blocker",
                                                 "marker_door_r1_c2_panel",
                                                 "marker_door_r1_c2",
                                                 5.0F));

  iggy3d::ProductActiveRoomCollisionState closedCollision;
  closedCollision.ready = true;
  closedCollision.surfaces = iggy3d::buildSpatialSurfaceSet(doorRoom);
  const iggy3d::ProductPrimitiveDrawList closedDoorRoomList =
      iggy3d::buildProductPrimitiveDrawList(nullptr, nullptr, &doorRoom,
                                            &closedCollision);
  ok &= expect(closedDoorRoomList.doorMarkerCount == 0U,
               "closed room door is not duplicated without scene entity");

  iggy3d::ProductActiveRoomCollisionState openCollision;
  openCollision.ready = true;
  const iggy3d::ProductPrimitiveDrawList openDoorRoomList =
      iggy3d::buildProductPrimitiveDrawList(nullptr, nullptr, &doorRoom,
                                            &openCollision);
  ok &= expect(openDoorRoomList.doorVisible, "open room door visible");
  ok &= expect(openDoorRoomList.openDoorVisible, "open room door marked open");
  ok &= expect(!openDoorRoomList.closedDoorVisible,
               "open room door not marked closed");
  ok &= expect(openDoorRoomList.doorMarkerCount == 1U,
               "open room door count");
  ok &= expect(openDoorRoomList.openDoorMarkerCount == 1U,
               "open room door open count");
  ok &= expect(openDoorRoomList.closedDoorMarkerCount == 0U,
               "open room door closed count");
  ok &= expect(openDoorRoomList.items[0].kind ==
                   iggy3d::ProductPrimitiveDrawKind::DoorMarker,
               "open room door maps to door marker");
  ok &= expect(openDoorRoomList.items[0].doorOpen,
               "open room door item is open");

  iggy3d::ProductRoomEditorOverlay cursorOverlay;
  cursorOverlay.visible = true;
  cursorOverlay.itemCount = 1;
  cursorOverlay.worldPosition = {6.0F, 0.08F, 0.0F};
  iggy3d::ProductRoomEditorPreviewOverlay previewOverlay;
  previewOverlay.visible = true;
  previewOverlay.itemCount = 1;
  previewOverlay.candidateId = "edit_floor_1";
  previewOverlay.tool = iggy3d::ProductRoomEditorTool::Floor;
  previewOverlay.worldPosition = {7.0F, -0.05F, 0.0F};
  previewOverlay.floorSizeMeters = {1.0F, 0.10F, 1.0F};
  const iggy3d::ProductPrimitiveDrawList previewRoomList =
      iggy3d::buildProductPrimitiveDrawList(nullptr,
                                            nullptr,
                                            &room,
                                            nullptr,
                                            &cursorOverlay,
                                            &previewOverlay);
  ok &= expect(previewRoomList.itemCount == 7U,
               "room plus cursor plus preview item count");
  ok &= expect(previewRoomList.roomGeometryCount == 5U,
               "preview does not affect room geometry count");
  ok &= expect(previewRoomList.floorTileCount == 1U,
               "preview does not affect real floor tile count");
  ok &= expect(previewRoomList.wallTileCount == 1U,
               "preview does not affect real wall tile count");
  ok &= expect(previewRoomList.roomEditorCursorCount == 1U,
               "cursor overlay remains separate");
  ok &= expect(previewRoomList.roomEditorPlacementPreviewCount == 1U,
               "preview overlay counted separately");
  ok &= expect(previewRoomList.roomEditorPlacementPreviewVisible,
               "preview overlay visible");
  ok &= expect(previewRoomList.items.back().kind ==
                   iggy3d::ProductPrimitiveDrawKind::RoomEditorPlacementPreview,
               "preview overlay draw kind");
  ok &= expect(previewRoomList.items.back().stableName == "edit_floor_1",
               "preview overlay stable name");

  if (!ok) {
    return 1;
  }
  std::cout << "product_primitive_draw_list_tests=pass\n";
  return 0;
}
