#include "app/iggy3d/ProductPrimitiveDrawList.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/ProductActiveRoomCollision.hpp"
#include "app/iggy3d/ProductRoomEditorOverlay.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneItem.hpp"
#include "projection/scene/SceneProjection.hpp"

namespace iggy3d {
namespace {

bool hasTag(const std::vector<std::string>& tags, std::string_view expected) {
  for (const std::string& tag : tags) {
    if (tag == expected) {
      return true;
    }
  }
  return false;
}

Aabb3 boundsFromPoints(const std::vector<Vec3>& points) {
  if (points.empty()) {
    return {};
  }
  Vec3 minPoint{std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()};
  Vec3 maxPoint{-std::numeric_limits<float>::max(),
                -std::numeric_limits<float>::max(),
                -std::numeric_limits<float>::max()};
  for (const Vec3 point : points) {
    minPoint.x = std::min(minPoint.x, point.x);
    minPoint.y = std::min(minPoint.y, point.y);
    minPoint.z = std::min(minPoint.z, point.z);
    maxPoint.x = std::max(maxPoint.x, point.x);
    maxPoint.y = std::max(maxPoint.y, point.y);
    maxPoint.z = std::max(maxPoint.z, point.z);
  }
  return makeAabb3(minPoint, maxPoint);
}

ProductPrimitiveDrawKind floorKindForSurface(const RoomSpatialSurface& surface) {
  if (hasTag(surface.traversalTags, "blocked_slope")) {
    return ProductPrimitiveDrawKind::BlockedSlopeTile;
  }
  if (hasTag(surface.traversalTags, "ramp")) {
    return ProductPrimitiveDrawKind::RampTile;
  }
  if (hasTag(surface.traversalTags, "elevated_floor")) {
    return ProductPrimitiveDrawKind::ElevatedFloorTile;
  }
  return ProductPrimitiveDrawKind::FloorTile;
}

ProductPrimitiveColor colorForRoomKind(ProductPrimitiveDrawKind kind) {
  switch (kind) {
    case ProductPrimitiveDrawKind::FloorTile:
      return {54, 78, 68};
    case ProductPrimitiveDrawKind::ElevatedFloorTile:
      return {92, 126, 102};
    case ProductPrimitiveDrawKind::RampTile:
      return {82, 139, 156};
    case ProductPrimitiveDrawKind::BlockedSlopeTile:
      return {184, 82, 74};
    case ProductPrimitiveDrawKind::WallTile:
      return {76, 86, 92};
    case ProductPrimitiveDrawKind::RoomEditorCursor:
      return {245, 214, 96};
    case ProductPrimitiveDrawKind::RoomEditorPlacementPreview:
      return {105, 205, 228};
    case ProductPrimitiveDrawKind::PlayerMarker:
    case ProductPrimitiveDrawKind::NpcMarker:
    case ProductPrimitiveDrawKind::PickupMarker:
    case ProductPrimitiveDrawKind::InteractableMarker:
    case ProductPrimitiveDrawKind::ObjectiveMarker:
    case ProductPrimitiveDrawKind::TacticalMarker:
    case ProductPrimitiveDrawKind::DebugMarker:
    case ProductPrimitiveDrawKind::PlayerFocusIndicator:
    case ProductPrimitiveDrawKind::DoorMarker:
      break;
  }
  return {112, 118, 120};
}

ProductPrimitiveDrawItem itemFromWalkableSurface(const RoomSpatialSurface& surface) {
  ProductPrimitiveDrawItem item;
  item.kind = floorKindForSurface(surface);
  item.stableName = surface.id;
  item.worldBounds = boundsFromPoints(surface.pointsMeters);
  item.worldPosition = center(item.worldBounds);
  item.visible = true;
  item.targetable = false;
  item.interactable = false;
  item.tactical = false;
  item.color = colorForRoomKind(item.kind);
  item.markerSize = 58.0F;
  return item;
}

ProductPrimitiveDrawItem itemFromWallMesh(const RoomStaticMeshAsset& mesh) {
  ProductPrimitiveDrawItem item;
  item.kind = ProductPrimitiveDrawKind::WallTile;
  item.stableName = mesh.id;
  item.worldPosition = mesh.positionMeters;
  item.worldBounds = aabbFromCenterExtents(mesh.positionMeters, mesh.sizeMeters * 0.5F);
  item.visible = true;
  item.color = colorForRoomKind(item.kind);
  item.markerSize = 62.0F;
  return item;
}

ProductPrimitiveDrawItem itemFromDoorMesh(const RoomStaticMeshAsset& mesh,
                                          std::string_view ownerStableName,
                                          bool open) {
  ProductPrimitiveDrawItem item;
  item.kind = ProductPrimitiveDrawKind::DoorMarker;
  item.stableName = std::string(ownerStableName);
  item.worldPosition = mesh.positionMeters;
  item.worldBounds = aabbFromCenterExtents(mesh.positionMeters, mesh.sizeMeters * 0.5F);
  item.visible = true;
  item.targetable = !open;
  item.interactable = !open;
  item.doorOpen = open;
  item.doorClosed = !open;
  item.color = open ? ProductPrimitiveColor{126, 201, 176}
                    : ProductPrimitiveColor{220, 178, 86};
  item.markerSize = open ? 18.0F : 24.0F;
  return item;
}

ProductPrimitiveDrawItem itemFromSceneItem(const SceneItem& item) {
  ProductPrimitiveDrawItem draw;
  draw.entityId = item.entityId;
  draw.stableName = item.stableName;
  draw.worldPosition = item.transform.position;
  draw.worldBounds = item.worldBounds;
  draw.visible = item.visible;
  draw.targetable = item.targetable;
  draw.interactable = item.interactable;
  draw.tactical = item.tactical;

  switch (item.kind) {
    case SceneItemKind::Player:
      draw.kind = ProductPrimitiveDrawKind::PlayerMarker;
      draw.color = {80, 170, 236};
      draw.markerSize = 26.0F;
      return draw;
    case SceneItemKind::Npc:
      draw.kind = ProductPrimitiveDrawKind::NpcMarker;
      draw.color = {210, 78, 76};
      draw.markerSize = 28.0F;
      return draw;
    case SceneItemKind::Pickup:
      draw.kind = ProductPrimitiveDrawKind::PickupMarker;
      draw.color = {229, 196, 72};
      draw.markerSize = 20.0F;
      return draw;
    case SceneItemKind::Interactable:
      if (item.entityKind == EntityKind::Door) {
        draw.kind = ProductPrimitiveDrawKind::DoorMarker;
        draw.color = {220, 178, 86};
        draw.markerSize = 24.0F;
        draw.doorOpen = false;
        draw.doorClosed = true;
        return draw;
      }
      draw.kind = ProductPrimitiveDrawKind::InteractableMarker;
      draw.color = {198, 142, 222};
      draw.markerSize = 22.0F;
      return draw;
    case SceneItemKind::ObjectiveMarker:
      draw.kind = ProductPrimitiveDrawKind::ObjectiveMarker;
      draw.color = {126, 201, 176};
      draw.markerSize = 18.0F;
      return draw;
    case SceneItemKind::TacticalMarker:
      draw.kind = ProductPrimitiveDrawKind::TacticalMarker;
      draw.color = {126, 201, 176};
      draw.markerSize = 18.0F;
      return draw;
    case SceneItemKind::DebugOnly:
      draw.kind = ProductPrimitiveDrawKind::DebugMarker;
      draw.color = {112, 118, 120};
      draw.markerSize = 14.0F;
      return draw;
  }
  return draw;
}

ProductPrimitiveDrawItem playerFocusIndicatorFor(const ProductPrimitiveDrawItem& player) {
  ProductPrimitiveDrawItem indicator = player;
  indicator.kind = ProductPrimitiveDrawKind::PlayerFocusIndicator;
  indicator.color = {226, 230, 211};
  indicator.markerSize = 36.0F;
  return indicator;
}

void updateCounts(ProductPrimitiveDrawList& list, const ProductPrimitiveDrawItem& item) {
  ++list.itemCount;
  switch (item.kind) {
    case ProductPrimitiveDrawKind::PlayerMarker:
      ++list.playerCount;
      list.playerVisible = true;
      break;
    case ProductPrimitiveDrawKind::NpcMarker:
    case ProductPrimitiveDrawKind::PickupMarker:
    case ProductPrimitiveDrawKind::InteractableMarker:
    case ProductPrimitiveDrawKind::TacticalMarker:
      ++list.targetMarkerCount;
      break;
    case ProductPrimitiveDrawKind::DoorMarker:
      ++list.doorMarkerCount;
      list.doorVisible = true;
      if (item.doorOpen) {
        ++list.openDoorMarkerCount;
        list.openDoorVisible = true;
      }
      if (item.doorClosed) {
        ++list.closedDoorMarkerCount;
        list.closedDoorVisible = true;
      }
      if (item.targetable || item.interactable) {
        ++list.targetMarkerCount;
      }
      break;
    case ProductPrimitiveDrawKind::ObjectiveMarker:
      ++list.objectiveMarkerCount;
      list.objectiveVisible = true;
      break;
    case ProductPrimitiveDrawKind::DebugMarker:
      ++list.debugMarkerCount;
      break;
    case ProductPrimitiveDrawKind::PlayerFocusIndicator:
      list.playerFocusIndicatorVisible = true;
      break;
    case ProductPrimitiveDrawKind::FloorTile:
      ++list.roomGeometryCount;
      ++list.floorTileCount;
      list.roomVisible = true;
      break;
    case ProductPrimitiveDrawKind::ElevatedFloorTile:
      ++list.roomGeometryCount;
      ++list.elevatedFloorTileCount;
      list.roomVisible = true;
      break;
    case ProductPrimitiveDrawKind::RampTile:
      ++list.roomGeometryCount;
      ++list.rampTileCount;
      list.roomVisible = true;
      break;
    case ProductPrimitiveDrawKind::BlockedSlopeTile:
      ++list.roomGeometryCount;
      ++list.blockedSlopeTileCount;
      list.roomVisible = true;
      break;
    case ProductPrimitiveDrawKind::WallTile:
      ++list.roomGeometryCount;
      ++list.wallTileCount;
      list.roomVisible = true;
      break;
    case ProductPrimitiveDrawKind::RoomEditorCursor:
      ++list.roomEditorCursorCount;
      list.roomEditorCursorVisible = true;
      break;
    case ProductPrimitiveDrawKind::RoomEditorPlacementPreview:
      ++list.roomEditorPlacementPreviewCount;
      list.roomEditorPlacementPreviewVisible = true;
      break;
  }
}

const RoomStaticMeshAsset* findStaticMesh(const RoomAsset& room, std::string_view id) {
  for (const RoomStaticMeshAsset& mesh : room.staticMeshes) {
    if (mesh.id == id) {
      return &mesh;
    }
  }
  return nullptr;
}

bool isDoorBlockerSurface(const RoomAsset& room, const RoomSpatialSurface& surface) {
  if (surface.runtimeOwnerStableName.empty() || surface.sourceStaticMeshId.empty()) {
    return false;
  }
  const RoomStaticMeshAsset* mesh = findStaticMesh(room, surface.sourceStaticMeshId);
  if (mesh == nullptr || mesh->role != "door") {
    return false;
  }
  return surface.blocksActor || surface.blocksProjectile ||
         surface.role == RoomSpatialSurfaceRole::Blocker ||
         surface.role == RoomSpatialSurfaceRole::ProjectileBlocker;
}

bool collisionIncludesRuntimeOwner(const ProductActiveRoomCollisionState& collision,
                                   std::string_view ownerStableName) {
  if (!collision.ready) {
    return true;
  }
  for (const CollisionSurfaceView& surface : collision.surfaces.surfaces()) {
    if (surface.runtimeOwnerStableName == ownerStableName) {
      return true;
    }
  }
  return false;
}

bool containsOwner(const std::vector<std::string>& owners, std::string_view owner) {
  for (const std::string& seen : owners) {
    if (seen == owner) {
      return true;
    }
  }
  return false;
}

void appendOpenDoorMarkers(const RoomAsset* room,
                           const ProductActiveRoomCollisionState* collision,
                           ProductPrimitiveDrawList& list) {
  if (room == nullptr || collision == nullptr || !collision->ready) {
    return;
  }

  std::vector<std::string> appendedOwners;
  for (const RoomSpatialSurface& surface : room->spatialSurfaces) {
    if (!isDoorBlockerSurface(*room, surface) ||
        containsOwner(appendedOwners, surface.runtimeOwnerStableName) ||
        collisionIncludesRuntimeOwner(*collision, surface.runtimeOwnerStableName)) {
      continue;
    }
    const RoomStaticMeshAsset* mesh = findStaticMesh(*room, surface.sourceStaticMeshId);
    if (mesh == nullptr) {
      continue;
    }

    ProductPrimitiveDrawItem item =
        itemFromDoorMesh(*mesh, surface.runtimeOwnerStableName, true);
    list.items.push_back(item);
    updateCounts(list, item);
    appendedOwners.push_back(surface.runtimeOwnerStableName);
  }
}

void appendRoomGeometry(const RoomAsset* room, ProductPrimitiveDrawList& list) {
  if (room == nullptr) {
    return;
  }

  for (const RoomSpatialSurface& surface : room->spatialSurfaces) {
    if (surface.role != RoomSpatialSurfaceRole::Walkable ||
        surface.pointsMeters.empty()) {
      continue;
    }
    ProductPrimitiveDrawItem item = itemFromWalkableSurface(surface);
    list.items.push_back(item);
    updateCounts(list, item);
  }

  for (const RoomStaticMeshAsset& mesh : room->staticMeshes) {
    if (mesh.role != "wall") {
      continue;
    }
    ProductPrimitiveDrawItem item = itemFromWallMesh(mesh);
    list.items.push_back(item);
    updateCounts(list, item);
  }
}

void appendRoomEditorOverlay(const ProductRoomEditorOverlay* overlay,
                             ProductPrimitiveDrawList& list) {
  if (overlay == nullptr || !overlay->visible || overlay->itemCount == 0U) {
    return;
  }

  ProductPrimitiveDrawItem item;
  item.kind = ProductPrimitiveDrawKind::RoomEditorCursor;
  item.stableName = "room_editor_cursor";
  item.worldPosition = overlay->worldPosition;
  item.worldBounds =
      aabbFromCenterExtents(overlay->worldPosition, {0.28F, 0.08F, 0.28F});
  item.visible = true;
  item.targetable = false;
  item.interactable = false;
  item.tactical = false;
  item.color = colorForRoomKind(item.kind);
  item.markerSize = 30.0F;
  list.items.push_back(item);
  updateCounts(list, item);
}

Vec3 previewWallCenter(const ProductRoomEditorPreviewOverlay& overlay) {
  return {(overlay.wallStartMeters.x + overlay.wallEndMeters.x) * 0.5F,
          overlay.wallBottomY + overlay.wallHeightMeters * 0.5F,
          (overlay.wallStartMeters.z + overlay.wallEndMeters.z) * 0.5F};
}

Vec3 previewWallExtents(const ProductRoomEditorPreviewOverlay& overlay) {
  const float lengthX = std::abs(overlay.wallEndMeters.x - overlay.wallStartMeters.x);
  const float lengthZ = std::abs(overlay.wallEndMeters.z - overlay.wallStartMeters.z);
  return {std::max(lengthX, overlay.wallThicknessMeters),
          overlay.wallHeightMeters,
          std::max(lengthZ, overlay.wallThicknessMeters)};
}

void appendRoomEditorPlacementPreview(
    const ProductRoomEditorPreviewOverlay* overlay,
    ProductPrimitiveDrawList& list) {
  // branch-gate: BG-1047
  if (overlay == nullptr || !overlay->visible || overlay->itemCount == 0U) {
    return;
  }

  ProductPrimitiveDrawItem item;
  item.kind = ProductPrimitiveDrawKind::RoomEditorPlacementPreview;
  item.stableName = overlay->candidateId;
  item.visible = true;
  item.targetable = false;
  item.interactable = false;
  item.tactical = false;
  item.color = colorForRoomKind(item.kind);
  // branch-gate: BG-1047
  item.markerSize = overlay->tool == ProductRoomEditorTool::Wall ? 58.0F : 52.0F;

  // branch-gate: BG-1047
  if (overlay->tool == ProductRoomEditorTool::Wall) {
    item.worldPosition = previewWallCenter(*overlay);
    item.worldBounds =
        aabbFromCenterExtents(item.worldPosition, previewWallExtents(*overlay) * 0.5F);
  } else {
    item.worldPosition = overlay->worldPosition;
    item.worldBounds =
        aabbFromCenterExtents(item.worldPosition, overlay->floorSizeMeters * 0.5F);
  }

  list.items.push_back(item);
  updateCounts(list, item);
}

}  // namespace

ProductPrimitiveDrawList buildProductPrimitiveDrawList(
    const SceneProjectionResult* scene,
    const DebugProjectionResult* debug,
    const RoomAsset* activeRoom,
    const ProductActiveRoomCollisionState* activeRoomCollision,
    const ProductRoomEditorOverlay* roomEditorOverlay,
    const ProductRoomEditorPreviewOverlay* roomEditorPreviewOverlay) {
  (void)debug;
  ProductPrimitiveDrawList list;
  list.gridVisible =
      scene != nullptr || activeRoom != nullptr ||
      (roomEditorOverlay != nullptr && roomEditorOverlay->visible) ||
      (roomEditorPreviewOverlay != nullptr && roomEditorPreviewOverlay->visible);
  appendRoomGeometry(activeRoom, list);
  appendOpenDoorMarkers(activeRoom, activeRoomCollision, list);
  appendRoomEditorOverlay(roomEditorOverlay, list);
  appendRoomEditorPlacementPreview(roomEditorPreviewOverlay, list);
  if (scene == nullptr) {
    return list;
  }

  list.roomVisible = list.roomVisible || scene->room.loaded || !scene->items.empty();
  list.objectiveVisible = scene->pickupCount > 0 || scene->interactableCount > 0 ||
                          scene->markerCount > 0 || scene->room.loaded;

  for (const SceneItem& sceneItem : scene->items) {
    if (!sceneItem.visible) {
      continue;
    }
    ProductPrimitiveDrawItem item = itemFromSceneItem(sceneItem);
    list.items.push_back(item);
    updateCounts(list, item);

    if (item.kind == ProductPrimitiveDrawKind::PlayerMarker) {
      ProductPrimitiveDrawItem focus = playerFocusIndicatorFor(item);
      list.items.push_back(focus);
      updateCounts(list, focus);
    }
  }

  return list;
}

}  // namespace iggy3d
