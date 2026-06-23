#include "app/iggy3d/ProductRenderBridge.hpp"

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

}  // namespace

ProductRenderBridgeFrame buildProductRenderBridgeFrame(
    const ProductPrimitiveDrawList* drawList,
    const ProductViewportFrame* frame,
    const ProductGameplayFeedback* feedback) {
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
  }

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
    }
  }

  if (feedback != nullptr) {
    for (const ProductGameplayFeedbackLine& line : feedback->lines) {
      if (line.visible) {
        ++bridge.feedbackLineCount;
      }
    }
  }

  return bridge;
}

}  // namespace iggy3d
