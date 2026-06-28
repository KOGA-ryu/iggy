#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Vec3.hpp"
#include "content/assets/RoomAsset.hpp"

namespace iggy3d {

struct DebugProjectionResult;
struct ProductMapMakerGridOverlay;
struct ProductActiveRoomCollisionState;
struct ProductRoomEditorOverlay;
struct ProductRoomEditorPreviewOverlay;
struct SceneProjectionResult;

enum class ProductPrimitiveDrawKind : std::uint8_t {
  PlayerMarker,
  NpcMarker,
  PickupMarker,
  InteractableMarker,
  ObjectiveMarker,
  TacticalMarker,
  DebugMarker,
  PlayerFocusIndicator,
  DoorMarker,
  FloorTile,
  ElevatedFloorTile,
  RampTile,
  BlockedSlopeTile,
  WallTile,
  PropTile,
  RoomEditorCursor,
  RoomEditorPlacementPreview,
  PhysicsAabbDebug,
  PhysicsContactNormalDebug,
  PhysicsBroadphasePairDebug,
  MapMakerGridDot,
};

struct ProductPrimitiveColor {
  std::uint8_t r = 255;
  std::uint8_t g = 255;
  std::uint8_t b = 255;
};

struct ProductPrimitiveDrawItem {
  ProductPrimitiveDrawKind kind = ProductPrimitiveDrawKind::DebugMarker;
  EntityId entityId;
  std::string stableName;
  Vec3 worldPosition;
  Aabb3 worldBounds;
  bool visible = false;
  bool targetable = false;
  bool interactable = false;
  bool tactical = false;
  bool doorOpen = false;
  bool doorClosed = false;
  ProductPrimitiveColor color;
  float markerSize = 14.0F;
};

struct ProductPrimitiveDrawList {
  std::vector<ProductPrimitiveDrawItem> items;
  bool gridVisible = false;
  bool roomVisible = false;
  bool playerVisible = false;
  bool objectiveVisible = false;
  bool playerFocusIndicatorVisible = false;
  bool doorVisible = false;
  bool openDoorVisible = false;
  bool closedDoorVisible = false;
  std::uint64_t itemCount = 0;
  std::uint64_t playerCount = 0;
  std::uint64_t targetMarkerCount = 0;
  std::uint64_t objectiveMarkerCount = 0;
  std::uint64_t debugMarkerCount = 0;
  std::uint64_t doorMarkerCount = 0;
  std::uint64_t openDoorMarkerCount = 0;
  std::uint64_t closedDoorMarkerCount = 0;
  std::uint64_t roomGeometryCount = 0;
  std::uint64_t floorTileCount = 0;
  std::uint64_t elevatedFloorTileCount = 0;
  std::uint64_t rampTileCount = 0;
  std::uint64_t blockedSlopeTileCount = 0;
  std::uint64_t wallTileCount = 0;
  std::uint64_t propTileCount = 0;
  bool roomEditorCursorVisible = false;
  std::uint64_t roomEditorCursorCount = 0;
  bool roomEditorPlacementPreviewVisible = false;
  std::uint64_t roomEditorPlacementPreviewCount = 0;
  bool physicsDebugVisible = false;
  std::uint64_t physicsDebugItemCount = 0;
  std::uint64_t physicsAabbDebugCount = 0;
  std::uint64_t physicsContactNormalDebugCount = 0;
  std::uint64_t physicsBroadphasePairDebugCount = 0;
  bool mapMakerGridVisible = false;
  std::uint64_t mapMakerGridDotCount = 0;
  std::uint64_t mapMakerMajorGridDotCount = 0;
};

ProductPrimitiveDrawList buildProductPrimitiveDrawList(
    const SceneProjectionResult* scene,
    const DebugProjectionResult* debug,
    const RoomAsset* activeRoom = nullptr,
    const ProductActiveRoomCollisionState* activeRoomCollision = nullptr,
    const ProductRoomEditorOverlay* roomEditorOverlay = nullptr,
    const ProductRoomEditorPreviewOverlay* roomEditorPreviewOverlay = nullptr,
    const ProductMapMakerGridOverlay* mapMakerGridOverlay = nullptr);

}  // namespace iggy3d
