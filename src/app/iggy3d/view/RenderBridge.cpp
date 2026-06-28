#include "app/iggy3d/view/RenderBridge.hpp"

#include "core/ids/EntityId.hpp"

namespace iggy3d {
namespace {

bool isTargetItem(const ProductPrimitiveDrawItem& item) {
  return item.kind == ProductPrimitiveDrawKind::NpcMarker ||
         item.kind == ProductPrimitiveDrawKind::PickupMarker ||
         item.kind == ProductPrimitiveDrawKind::InteractableMarker ||
         item.kind == ProductPrimitiveDrawKind::TacticalMarker || item.targetable ||
         item.interactable;
}

void countPhysicsDebugKind(ProductRenderBridgeFrame& bridge,
                           ProductPrimitiveDrawKind kind) {
  // branch-gate: BG-1113
  switch (kind) {
    case ProductPrimitiveDrawKind::PhysicsAabbDebug:
      ++bridge.physicsDebugItemCount;
      ++bridge.physicsAabbDebugCount;
      bridge.physicsDebugVisible = true;
      return;
    case ProductPrimitiveDrawKind::PhysicsContactNormalDebug:
      ++bridge.physicsDebugItemCount;
      ++bridge.physicsContactNormalDebugCount;
      bridge.physicsDebugVisible = true;
      return;
    case ProductPrimitiveDrawKind::PhysicsBroadphasePairDebug:
      ++bridge.physicsDebugItemCount;
      ++bridge.physicsBroadphasePairDebugCount;
      bridge.physicsDebugVisible = true;
      return;
    case ProductPrimitiveDrawKind::PlayerMarker:
    case ProductPrimitiveDrawKind::NpcMarker:
    case ProductPrimitiveDrawKind::PickupMarker:
    case ProductPrimitiveDrawKind::InteractableMarker:
    case ProductPrimitiveDrawKind::ObjectiveMarker:
    case ProductPrimitiveDrawKind::TacticalMarker:
    case ProductPrimitiveDrawKind::DebugMarker:
    case ProductPrimitiveDrawKind::PlayerFocusIndicator:
    case ProductPrimitiveDrawKind::DoorMarker:
    case ProductPrimitiveDrawKind::FloorTile:
    case ProductPrimitiveDrawKind::ElevatedFloorTile:
    case ProductPrimitiveDrawKind::RampTile:
    case ProductPrimitiveDrawKind::BlockedSlopeTile:
    case ProductPrimitiveDrawKind::WallTile:
    case ProductPrimitiveDrawKind::PropTile:
    case ProductPrimitiveDrawKind::RoomEditorCursor:
    case ProductPrimitiveDrawKind::RoomEditorPlacementPreview:
    case ProductPrimitiveDrawKind::MapMakerGridDot:
    case ProductPrimitiveDrawKind::MapMakerCubePreview:
      return;
  }
}

}  // namespace

ProductRenderBridgeFrame buildProductRenderBridgeFrame(
    const ProductPrimitiveDrawList* drawList,
    const ProductViewportFrame* frame,
    const GameplayFeedback* feedback) {
  ProductRenderBridgeFrame bridge;
  bridge.viewFrameReady = drawList != nullptr && frame != nullptr;
  bridge.feedbackReady = feedback != nullptr && feedback->visible;
  bridge.ready = bridge.viewFrameReady;

  if (drawList != nullptr) {
    bridge.drawItemCount = drawList->itemCount;
    bridge.gridVisible = drawList->gridVisible;
    bridge.playerVisible = drawList->playerVisible;
    bridge.objectiveVisible = drawList->objectiveVisible;
    bridge.targetIndicatorVisible = drawList->playerFocusIndicatorVisible;
    bridge.physicsDebugVisible = drawList->physicsDebugVisible;
    bridge.physicsDebugItemCount = drawList->physicsDebugItemCount;
    bridge.physicsAabbDebugCount = drawList->physicsAabbDebugCount;
    bridge.physicsContactNormalDebugCount = drawList->physicsContactNormalDebugCount;
    bridge.physicsBroadphasePairDebugCount =
        drawList->physicsBroadphasePairDebugCount;
    bridge.mapMakerGridVisible = drawList->mapMakerGridVisible;
    bridge.mapMakerGridDotCount = drawList->mapMakerGridDotCount;
    bridge.mapMakerMajorGridDotCount = drawList->mapMakerMajorGridDotCount;
    bridge.mapMakerCubePreviewVisible = drawList->mapMakerCubePreviewVisible;
    bridge.mapMakerCubePreviewCount = drawList->mapMakerCubePreviewCount;
    bridge.propVisible = drawList->propTileCount > 0U;
    bridge.propTileCount = drawList->propTileCount;
  }

  ProductRenderBridgeFrame framedPhysicsCounts;
  std::uint64_t framedPropTileCount = 0;
  std::uint64_t framedMapMakerGridDotCount = 0;
  std::uint64_t framedMapMakerMajorGridDotCount = 0;
  std::uint64_t framedMapMakerCubePreviewCount = 0;
  if (frame != nullptr) {
    bridge.projectionMode = frame->projectionMode;
    bridge.frameItemCount = static_cast<std::uint64_t>(frame->framedItems.size());
    bridge.items.reserve(frame->framedItems.size());
    for (const ProductViewportFramedItem& framed : frame->framedItems) {
      const ProductPrimitiveDrawItem& item = framed.item;
      ProductRenderBridgeItem bridgeItem;
      bridgeItem.kind = item.kind;
      bridgeItem.entityId = toUint64(item.entityId);
      bridgeItem.stableName = item.stableName;
      bridgeItem.visible = item.visible;
      bridgeItem.onScreen = framed.onScreen;
      bridgeItem.targetable = item.targetable;
      bridgeItem.interactable = item.interactable;
      bridgeItem.screenX = framed.screenX;
      bridgeItem.screenY = framed.screenY;
      bridgeItem.depthMeters = framed.depthMeters;
      bridge.items.push_back(bridgeItem);
      if (bridgeItem.onScreen) {
        ++bridge.onScreenItemCount;
      }
      if (isTargetItem(item)) {
        ++bridge.targetItemCount;
      }
      countPhysicsDebugKind(framedPhysicsCounts, item.kind);
      if (item.kind == ProductPrimitiveDrawKind::RoomEditorCursor) {
        ++bridge.roomEditorCursorCount;
        bridge.roomEditorCursorVisible = true;
      }
      // branch-gate: BG-1048
      if (item.kind == ProductPrimitiveDrawKind::RoomEditorPlacementPreview) {
        ++bridge.roomEditorPlacementPreviewCount;
        bridge.roomEditorPlacementPreviewVisible = true;
      }
      // branch-gate: BG-1129
      if (item.kind == ProductPrimitiveDrawKind::PropTile) {
        ++framedPropTileCount;
      }
      // branch-gate: BG-1205
      if (item.kind == ProductPrimitiveDrawKind::MapMakerGridDot) {
        ++framedMapMakerGridDotCount;
        // branch-gate: BG-1205
        if (item.markerSize > 10.0F) {
          ++framedMapMakerMajorGridDotCount;
        }
      }
      // branch-gate: BG-1206
      if (item.kind == ProductPrimitiveDrawKind::MapMakerCubePreview) {
        ++framedMapMakerCubePreviewCount;
      }
    }
  }
  // branch-gate: BG-1048
  if (bridge.propTileCount == 0U) {
    bridge.propTileCount = framedPropTileCount;
    bridge.propVisible = framedPropTileCount > 0U;
  }
  // branch-gate: BG-1113
  if (bridge.physicsDebugItemCount == 0U) {
    bridge.physicsDebugVisible = framedPhysicsCounts.physicsDebugVisible;
    bridge.physicsDebugItemCount = framedPhysicsCounts.physicsDebugItemCount;
    bridge.physicsAabbDebugCount = framedPhysicsCounts.physicsAabbDebugCount;
    bridge.physicsContactNormalDebugCount =
        framedPhysicsCounts.physicsContactNormalDebugCount;
    bridge.physicsBroadphasePairDebugCount =
        framedPhysicsCounts.physicsBroadphasePairDebugCount;
  }
  // branch-gate: BG-1205
  if (bridge.mapMakerGridDotCount == 0U) {
    bridge.mapMakerGridDotCount = framedMapMakerGridDotCount;
    bridge.mapMakerMajorGridDotCount = framedMapMakerMajorGridDotCount;
    bridge.mapMakerGridVisible = framedMapMakerGridDotCount > 0U;
  }
  // branch-gate: BG-1206
  if (bridge.mapMakerCubePreviewCount == 0U) {
    bridge.mapMakerCubePreviewCount = framedMapMakerCubePreviewCount;
    bridge.mapMakerCubePreviewVisible = framedMapMakerCubePreviewCount > 0U;
  }

  if (feedback != nullptr) {
    for (const GameplayFeedbackLine& line : feedback->lines) {
      if (line.visible) {
        ++bridge.feedbackLineCount;
      }
    }
  }

  return bridge;
}

}  // namespace iggy3d
