#include "app/iggy3d/ProductPrimitiveDrawList.hpp"

#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneItem.hpp"
#include "projection/scene/SceneProjection.hpp"

namespace iggy3d {
namespace {

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
  }
}

}  // namespace

ProductPrimitiveDrawList buildProductPrimitiveDrawList(
    const SceneProjectionResult* scene,
    const DebugProjectionResult* debug) {
  (void)debug;
  ProductPrimitiveDrawList list;
  list.gridVisible = scene != nullptr;
  if (scene == nullptr) {
    return list;
  }

  list.roomVisible = scene->room.loaded || !scene->items.empty();
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
