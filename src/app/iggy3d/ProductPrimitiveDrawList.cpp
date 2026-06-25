#include "app/iggy3d/ProductPrimitiveDrawList.hpp"

#include <algorithm>
#include <limits>
#include <string_view>

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
    case ProductPrimitiveDrawKind::PlayerMarker:
    case ProductPrimitiveDrawKind::NpcMarker:
    case ProductPrimitiveDrawKind::PickupMarker:
    case ProductPrimitiveDrawKind::InteractableMarker:
    case ProductPrimitiveDrawKind::ObjectiveMarker:
    case ProductPrimitiveDrawKind::TacticalMarker:
    case ProductPrimitiveDrawKind::DebugMarker:
    case ProductPrimitiveDrawKind::PlayerFocusIndicator:
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

}  // namespace

ProductPrimitiveDrawList buildProductPrimitiveDrawList(
    const SceneProjectionResult* scene,
    const DebugProjectionResult* debug,
    const RoomAsset* activeRoom) {
  (void)debug;
  ProductPrimitiveDrawList list;
  list.gridVisible = scene != nullptr || activeRoom != nullptr;
  appendRoomGeometry(activeRoom, list);
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
