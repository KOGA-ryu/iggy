#include "app/iggy3d/view/PrimitiveDrawList.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/room_editor/Presentation.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneItem.hpp"
#include "projection/scene/SceneProjection.hpp"

namespace iggy3d {
namespace {

struct PhysicsDebugStyle {
  ProductPrimitiveColor color;
  float markerSize = 14.0F;
};

constexpr ProductPrimitiveColor kPhysicsDebugSensorColor{245, 214, 96};
constexpr ProductPrimitiveColor kPhysicsAabbSolidColor{105, 205, 228};
constexpr ProductPrimitiveColor kPhysicsContactNormalSolidColor{236, 118, 86};
constexpr ProductPrimitiveColor kPhysicsBroadphasePairSolidColor{166, 184, 177};

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
    case ProductPrimitiveDrawKind::PropTile:
      return {151, 102, 58};
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
    case ProductPrimitiveDrawKind::PhysicsAabbDebug:
    case ProductPrimitiveDrawKind::PhysicsContactNormalDebug:
    case ProductPrimitiveDrawKind::PhysicsBroadphasePairDebug:
      break;
  }
  return {112, 118, 120};
}

bool physicsDebugItemIsSensor(const DebugProjectionItem& item) {
  return item.valueCode == "sensor" || (item.hasScalar && item.scalarValue > 0.5F);
}

std::string stableNameOrFallback(const DebugProjectionItem& item,
                                 std::string_view fallback) {
  // branch-gate: BG-1112
  if (!item.labelCode.empty()) {
    return item.labelCode;
  }
  return std::string(fallback);
}

PhysicsDebugStyle physicsAabbDebugStyle(bool sensor) {
  constexpr std::array<PhysicsDebugStyle, 2U> styles{
      PhysicsDebugStyle{kPhysicsAabbSolidColor, 34.0F},
      PhysicsDebugStyle{kPhysicsDebugSensorColor, 28.0F},
  };
  return styles[static_cast<std::size_t>(sensor)];
}

PhysicsDebugStyle physicsContactNormalDebugStyle(bool sensor) {
  constexpr std::array<PhysicsDebugStyle, 2U> styles{
      PhysicsDebugStyle{kPhysicsContactNormalSolidColor, 18.0F},
      PhysicsDebugStyle{kPhysicsDebugSensorColor, 18.0F},
  };
  return styles[static_cast<std::size_t>(sensor)];
}

PhysicsDebugStyle physicsBroadphasePairDebugStyle(bool sensor) {
  constexpr std::array<PhysicsDebugStyle, 2U> styles{
      PhysicsDebugStyle{kPhysicsBroadphasePairSolidColor, 14.0F},
      PhysicsDebugStyle{kPhysicsDebugSensorColor, 14.0F},
  };
  return styles[static_cast<std::size_t>(sensor)];
}

ProductPrimitiveDrawItem basePhysicsDebugItem(
    const DebugProjectionItem& debugItem,
    ProductPrimitiveDrawKind kind,
    std::string_view fallbackStableName,
    PhysicsDebugStyle style) {
  ProductPrimitiveDrawItem item;
  item.kind = kind;
  item.entityId = debugItem.actor;
  item.stableName = stableNameOrFallback(debugItem, fallbackStableName);
  item.visible = true;
  item.color = style.color;
  item.markerSize = style.markerSize;
  return item;
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

ProductPrimitiveDrawItem itemFromPropMesh(const RoomStaticMeshAsset& mesh) {
  ProductPrimitiveDrawItem item;
  item.kind = ProductPrimitiveDrawKind::PropTile;
  item.stableName = mesh.id;
  item.worldPosition = mesh.positionMeters;
  item.worldBounds = aabbFromCenterExtents(mesh.positionMeters, mesh.sizeMeters * 0.5F);
  item.visible = true;
  // branch-gate: BG-1159
  item.color = mesh.role == "ledge" ? ProductPrimitiveColor{76, 132, 178}
                                    : colorForRoomKind(item.kind);
  // branch-gate: BG-1159
  item.markerSize = mesh.role == "ledge" ? 52.0F : 42.0F;
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
    case ProductPrimitiveDrawKind::PhysicsAabbDebug:
      ++list.debugMarkerCount;
      ++list.physicsDebugItemCount;
      ++list.physicsAabbDebugCount;
      list.physicsDebugVisible = true;
      break;
    case ProductPrimitiveDrawKind::PhysicsContactNormalDebug:
      ++list.debugMarkerCount;
      ++list.physicsDebugItemCount;
      ++list.physicsContactNormalDebugCount;
      list.physicsDebugVisible = true;
      break;
    case ProductPrimitiveDrawKind::PhysicsBroadphasePairDebug:
      ++list.debugMarkerCount;
      ++list.physicsDebugItemCount;
      ++list.physicsBroadphasePairDebugCount;
      list.physicsDebugVisible = true;
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
    case ProductPrimitiveDrawKind::PropTile:
      ++list.roomGeometryCount;
      ++list.propTileCount;
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
    if (mesh.role == "wall") {
      ProductPrimitiveDrawItem item = itemFromWallMesh(mesh);
      list.items.push_back(item);
      updateCounts(list, item);
    }
    // branch-gate: BG-1128
    if (mesh.role == "prop" || mesh.role == "ledge") {
      ProductPrimitiveDrawItem item = itemFromPropMesh(mesh);
      list.items.push_back(item);
      updateCounts(list, item);
    }
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
  if (overlay->tool == ProductRoomEditorTool::Object) {
    item.markerSize = 46.0F;
  }

  // branch-gate: BG-1047
  if (overlay->tool == ProductRoomEditorTool::Wall) {
    item.worldPosition = previewWallCenter(*overlay);
    item.worldBounds =
        aabbFromCenterExtents(item.worldPosition, previewWallExtents(*overlay) * 0.5F);
  // branch-gate: BG-1128
  } else if (overlay->tool == ProductRoomEditorTool::Object) {
    item.worldPosition = overlay->worldPosition;
    item.worldBounds =
        aabbFromCenterExtents(item.worldPosition, overlay->objectSizeMeters * 0.5F);
  } else {
    item.worldPosition = overlay->worldPosition;
    item.worldBounds =
        aabbFromCenterExtents(item.worldPosition, overlay->floorSizeMeters * 0.5F);
  }

  list.items.push_back(item);
  updateCounts(list, item);
}

void appendPhysicsAabbDebugItem(const DebugProjectionItem& debugItem,
                                ProductPrimitiveDrawList& list) {
  // branch-gate: BG-1112
  if (!debugItem.hasBounds) {
    return;
  }

  const bool sensor = physicsDebugItemIsSensor(debugItem);
  ProductPrimitiveDrawItem item = basePhysicsDebugItem(
      debugItem,
      ProductPrimitiveDrawKind::PhysicsAabbDebug,
      "physics.aabb",
      physicsAabbDebugStyle(sensor));
  item.worldBounds = debugItem.worldBounds;
  item.worldPosition = center(item.worldBounds);
  list.items.push_back(item);
  updateCounts(list, item);
}

void appendPhysicsContactNormalDebugItem(const DebugProjectionItem& debugItem,
                                         ProductPrimitiveDrawList& list) {
  // branch-gate: BG-1112
  if (!debugItem.hasWorldPoint) {
    return;
  }

  const bool sensor = physicsDebugItemIsSensor(debugItem);
  ProductPrimitiveDrawItem item = basePhysicsDebugItem(
      debugItem,
      ProductPrimitiveDrawKind::PhysicsContactNormalDebug,
      "physics.contact_normal",
      physicsContactNormalDebugStyle(sensor));
  item.worldPosition = debugItem.worldPoint;
  item.worldBounds =
      aabbFromCenterExtents(item.worldPosition, {0.08F, 0.08F, 0.08F});
  list.items.push_back(item);
  updateCounts(list, item);
}

void appendPhysicsBroadphasePairDebugItem(const DebugProjectionItem& debugItem,
                                          ProductPrimitiveDrawList& list) {
  // branch-gate: BG-1112
  if (!debugItem.hasWorldPoint) {
    return;
  }

  const bool sensor = physicsDebugItemIsSensor(debugItem);
  ProductPrimitiveDrawItem item = basePhysicsDebugItem(
      debugItem,
      ProductPrimitiveDrawKind::PhysicsBroadphasePairDebug,
      "physics.broadphase_pair",
      physicsBroadphasePairDebugStyle(sensor));
  item.worldPosition = debugItem.worldPoint;
  item.worldBounds =
      aabbFromCenterExtents(item.worldPosition, {0.10F, 0.10F, 0.10F});
  list.items.push_back(item);
  updateCounts(list, item);
}

void appendDebugProjectionItem(const DebugProjectionItem& debugItem,
                               ProductPrimitiveDrawList& list) {
  // branch-gate: BG-1112
  switch (debugItem.kind) {
    case DebugProjectionKind::PhysicsAabb:
      appendPhysicsAabbDebugItem(debugItem, list);
      return;
    case DebugProjectionKind::PhysicsContactNormal:
      appendPhysicsContactNormalDebugItem(debugItem, list);
      return;
    case DebugProjectionKind::PhysicsBroadphasePair:
      appendPhysicsBroadphasePairDebugItem(debugItem, list);
      return;
    case DebugProjectionKind::TargetCandidate:
    case DebugProjectionKind::ReachRadius:
    case DebugProjectionKind::CommandRejected:
    case DebugProjectionKind::ClockMode:
    case DebugProjectionKind::CameraMode:
    case DebugProjectionKind::ObjectiveState:
    case DebugProjectionKind::StateHash:
    case DebugProjectionKind::ReplayDivergence:
    case DebugProjectionKind::RuntimeTelemetry:
    case DebugProjectionKind::NpcBehavior:
      return;
  }
}

void appendDebugProjectionItems(const DebugProjectionResult* debug,
                                ProductPrimitiveDrawList& list) {
  // branch-gate: BG-1112
  if (debug == nullptr) {
    return;
  }

  for (const DebugProjectionItem& item : debug->items) {
    appendDebugProjectionItem(item, list);
  }
}

}  // namespace

ProductPrimitiveDrawList buildProductPrimitiveDrawList(
    const SceneProjectionResult* scene,
    const DebugProjectionResult* debug,
    const RoomAsset* activeRoom,
    const ProductActiveRoomCollisionState* activeRoomCollision,
    const ProductRoomEditorOverlay* roomEditorOverlay,
    const ProductRoomEditorPreviewOverlay* roomEditorPreviewOverlay) {
  ProductPrimitiveDrawList list;
  list.gridVisible =
      scene != nullptr || activeRoom != nullptr ||
      (roomEditorOverlay != nullptr && roomEditorOverlay->visible) ||
      (roomEditorPreviewOverlay != nullptr && roomEditorPreviewOverlay->visible);
  appendRoomGeometry(activeRoom, list);
  appendOpenDoorMarkers(activeRoom, activeRoomCollision, list);
  appendRoomEditorOverlay(roomEditorOverlay, list);
  appendRoomEditorPlacementPreview(roomEditorPreviewOverlay, list);

  // branch-gate: BG-1112
  if (scene != nullptr) {
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
  }

  appendDebugProjectionItems(debug, list);

  return list;
}

}  // namespace iggy3d
