#include "app/iggy3d/view/ScenePrimitiveView.hpp"

#if defined(IGGY3D_HAS_SDL3)

#include <SDL3/SDL.h>

#include <algorithm>
#include <string_view>

#include "app/iggy3d/debug/TopDownMapOverlay.hpp"
#include "app/iggy3d/view/SdlDraw.hpp"
#include "app/iggy3d/view/ViewportFraming.hpp"

namespace iggy3d {
namespace {

void drawMarker(SDL_Renderer& renderer,
                const ProductViewportFramedItem& framed) {
  const ProductPrimitiveDrawItem& item = framed.item;
  setColor(renderer, item.color.r, item.color.g, item.color.b);
  const float x = framed.screenX;
  const float y = framed.screenY;
  const float size = item.markerSize;
  fillRect(renderer, x - size * 0.5F, y - size * 0.5F, size, size);
}

void drawPhysicsAabbDebugMarker(SDL_Renderer& renderer,
                                const ProductViewportFramedItem& framed) {
  const ProductPrimitiveDrawItem& item = framed.item;
  const float x = framed.screenX;
  const float y = framed.screenY;
  const float size = item.markerSize;
  const float half = size * 0.5F;
  constexpr float kLineWidth = 3.0F;

  setColor(renderer, item.color.r, item.color.g, item.color.b);
  fillRect(renderer, x - half, y - half, size, kLineWidth);
  fillRect(renderer, x - half, y + half - kLineWidth, size, kLineWidth);
  fillRect(renderer, x - half, y - half, kLineWidth, size);
  fillRect(renderer, x + half - kLineWidth, y - half, kLineWidth, size);
  SDL_RenderLine(&renderer, x - half + 6.0F, y, x + half - 6.0F, y);
  SDL_RenderLine(&renderer, x, y - half + 6.0F, x, y + half - 6.0F);
}

void drawPhysicsContactNormalDebugMarker(
    SDL_Renderer& renderer,
    const ProductViewportFramedItem& framed) {
  const ProductPrimitiveDrawItem& item = framed.item;
  const float x = framed.screenX;
  const float y = framed.screenY;
  const float half = item.markerSize * 0.5F;

  setColor(renderer, item.color.r, item.color.g, item.color.b);
  SDL_RenderLine(&renderer, x, y - half, x + half, y);
  SDL_RenderLine(&renderer, x + half, y, x, y + half);
  SDL_RenderLine(&renderer, x, y + half, x - half, y);
  SDL_RenderLine(&renderer, x - half, y, x, y - half);
  fillRect(renderer, x - 2.0F, y - 2.0F, 4.0F, 4.0F);
}

void drawPhysicsBroadphasePairDebugMarker(
    SDL_Renderer& renderer,
    const ProductViewportFramedItem& framed) {
  const ProductPrimitiveDrawItem& item = framed.item;
  const float x = framed.screenX;
  const float y = framed.screenY;
  const float half = item.markerSize * 0.5F;

  setColor(renderer, item.color.r, item.color.g, item.color.b);
  SDL_RenderLine(&renderer, x - half, y, x + half, y);
  SDL_RenderLine(&renderer, x, y - half, x, y + half);
  fillRect(renderer, x - 2.0F, y - 2.0F, 4.0F, 4.0F);
}

void drawFocusIndicator(SDL_Renderer& renderer,
                        const ProductViewportFramedItem& framed) {
  const ProductPrimitiveDrawItem& item = framed.item;
  const float x = framed.screenX;
  const float y = framed.screenY;
  setColor(renderer, item.color.r, item.color.g, item.color.b);
  fillRect(renderer, x - 18.0F, y - 2.0F, 36.0F, 4.0F);
  fillRect(renderer, x - 2.0F, y - 18.0F, 4.0F, 36.0F);
}

void drawRoomEditorCursor(SDL_Renderer& renderer,
                          const ProductViewportFramedItem& framed) {
  const ProductPrimitiveDrawItem& item = framed.item;
  const float x = framed.screenX;
  const float y = framed.screenY;
  const float size = item.markerSize;
  const float half = size * 0.5F;

  setColor(renderer, item.color.r, item.color.g, item.color.b);
  fillRect(renderer, x - half, y - 2.0F, size, 4.0F);
  fillRect(renderer, x - 2.0F, y - half, 4.0F, size);
  setColor(renderer, 32, 42, 44);
  fillRect(renderer, x - half, y - half, size, 3.0F);
  fillRect(renderer, x - half, y + half - 3.0F, size, 3.0F);
  fillRect(renderer, x - half, y - half, 3.0F, size);
  fillRect(renderer, x + half - 3.0F, y - half, 3.0F, size);
}

void drawDoorMarker(SDL_Renderer& renderer,
                    const ProductViewportFramedItem& framed) {
  const ProductPrimitiveDrawItem& item = framed.item;
  const float x = framed.screenX;
  const float y = framed.screenY;
  const float size = item.markerSize;
  const float half = size * 0.5F;

  setColor(renderer, item.color.r, item.color.g, item.color.b);
  if (item.doorOpen) {
    fillRect(renderer, x - half, y - 3.0F, size, 6.0F);
    fillRect(renderer, x + half - 4.0F, y - half, 4.0F, size);
    return;
  }

  fillRect(renderer, x - half, y - half, size, size);
  setColor(renderer, 94, 74, 54);
  fillRect(renderer, x - 2.0F, y - half + 4.0F, 4.0F, size - 8.0F);
  fillRect(renderer, x + half - 7.0F, y - 2.0F, 4.0F, 4.0F);
}

void drawRoomTile(SDL_Renderer& renderer,
                  const ProductViewportFramedItem& framed) {
  const ProductPrimitiveDrawItem& item = framed.item;
  const float x = framed.screenX;
  const float y = framed.screenY;
  const float size = item.markerSize;
  const float half = size * 0.5F;

  setColor(renderer, item.color.r, item.color.g, item.color.b);
  fillRect(renderer, x - half, y - half, size, size);

  switch (item.kind) {
    case ProductPrimitiveDrawKind::ElevatedFloorTile:
      setColor(renderer, 137, 168, 143);
      fillRect(renderer, x - half + 8.0F, y - half + 8.0F, size - 16.0F,
               size - 16.0F);
      return;
    case ProductPrimitiveDrawKind::RampTile:
      setColor(renderer, 183, 213, 210);
      SDL_RenderLine(&renderer, x - half + 8.0F, y + half - 8.0F,
                     x + half - 8.0F, y - half + 8.0F);
      SDL_RenderLine(&renderer, x - half + 16.0F, y + half - 8.0F,
                     x + half - 8.0F, y - half + 16.0F);
      return;
    case ProductPrimitiveDrawKind::BlockedSlopeTile:
      setColor(renderer, 246, 184, 130);
      SDL_RenderLine(&renderer, x - half + 8.0F, y - half + 8.0F,
                     x + half - 8.0F, y + half - 8.0F);
      SDL_RenderLine(&renderer, x + half - 8.0F, y - half + 8.0F,
                     x - half + 8.0F, y + half - 8.0F);
      return;
    case ProductPrimitiveDrawKind::WallTile:
      setColor(renderer, 116, 128, 132);
      fillRect(renderer, x - half, y - half, size, 5.0F);
      fillRect(renderer, x - half, y + half - 5.0F, size, 5.0F);
      fillRect(renderer, x - half, y - half, 5.0F, size);
      fillRect(renderer, x + half - 5.0F, y - half, 5.0F, size);
      return;
    case ProductPrimitiveDrawKind::PropTile:
      setColor(renderer, 198, 142, 82);
      fillRect(renderer, x - half + 8.0F, y - half + 8.0F, size - 16.0F,
               size - 16.0F);
      setColor(renderer, 88, 58, 34);
      SDL_RenderLine(&renderer, x - half + 8.0F, y - half + 8.0F,
                     x + half - 8.0F, y + half - 8.0F);
      SDL_RenderLine(&renderer, x + half - 8.0F, y - half + 8.0F,
                     x - half + 8.0F, y + half - 8.0F);
      return;
    case ProductPrimitiveDrawKind::FloorTile:
    case ProductPrimitiveDrawKind::PlayerMarker:
    case ProductPrimitiveDrawKind::NpcMarker:
    case ProductPrimitiveDrawKind::PickupMarker:
    case ProductPrimitiveDrawKind::InteractableMarker:
    case ProductPrimitiveDrawKind::ObjectiveMarker:
    case ProductPrimitiveDrawKind::TacticalMarker:
    case ProductPrimitiveDrawKind::DebugMarker:
    case ProductPrimitiveDrawKind::PlayerFocusIndicator:
    case ProductPrimitiveDrawKind::DoorMarker:
    case ProductPrimitiveDrawKind::RoomEditorCursor:
    case ProductPrimitiveDrawKind::RoomEditorPlacementPreview:
    case ProductPrimitiveDrawKind::PhysicsAabbDebug:
    case ProductPrimitiveDrawKind::PhysicsContactNormalDebug:
    case ProductPrimitiveDrawKind::PhysicsBroadphasePairDebug:
    case ProductPrimitiveDrawKind::MapMakerGridDot:
    case ProductPrimitiveDrawKind::MapMakerCubePreview:
      break;
  }
}

void drawPrimitiveItem(SDL_Renderer& renderer,
                       const ProductViewportFramedItem& framed) {
  const ProductPrimitiveDrawItem& item = framed.item;
  if (!item.visible) {
    return;
  }

  switch (item.kind) {
    case ProductPrimitiveDrawKind::PlayerFocusIndicator:
      drawFocusIndicator(renderer, framed);
      return;
    case ProductPrimitiveDrawKind::DoorMarker:
      drawDoorMarker(renderer, framed);
      return;
    case ProductPrimitiveDrawKind::RoomEditorCursor:
      drawRoomEditorCursor(renderer, framed);
      return;
    case ProductPrimitiveDrawKind::RoomEditorPlacementPreview:
      drawMarker(renderer, framed);
      return;
    case ProductPrimitiveDrawKind::PhysicsAabbDebug:
      drawPhysicsAabbDebugMarker(renderer, framed);
      return;
    case ProductPrimitiveDrawKind::PhysicsContactNormalDebug:
      drawPhysicsContactNormalDebugMarker(renderer, framed);
      return;
    case ProductPrimitiveDrawKind::PhysicsBroadphasePairDebug:
      drawPhysicsBroadphasePairDebugMarker(renderer, framed);
      return;
    case ProductPrimitiveDrawKind::MapMakerGridDot:
      return;
    case ProductPrimitiveDrawKind::MapMakerCubePreview:
      drawRoomTile(renderer, framed);
      return;
    case ProductPrimitiveDrawKind::FloorTile:
    case ProductPrimitiveDrawKind::ElevatedFloorTile:
    case ProductPrimitiveDrawKind::RampTile:
    case ProductPrimitiveDrawKind::BlockedSlopeTile:
    case ProductPrimitiveDrawKind::WallTile:
    case ProductPrimitiveDrawKind::PropTile:
      drawRoomTile(renderer, framed);
      return;
    case ProductPrimitiveDrawKind::PlayerMarker:
    case ProductPrimitiveDrawKind::NpcMarker:
    case ProductPrimitiveDrawKind::PickupMarker:
    case ProductPrimitiveDrawKind::InteractableMarker:
    case ProductPrimitiveDrawKind::ObjectiveMarker:
    case ProductPrimitiveDrawKind::TacticalMarker:
    case ProductPrimitiveDrawKind::DebugMarker:
      drawMarker(renderer, framed);
      return;
  }
}

void drawGrid(SDL_Renderer& renderer) {
  setColor(renderer, 18, 28, 29);
  fillRect(renderer, 80.0F, 130.0F, 1120.0F, 480.0F);
  setColor(renderer, 32, 48, 48);
  for (int i = 0; i <= 14; ++i) {
    const float x = 80.0F + static_cast<float>(i) * 80.0F;
    fillRect(renderer, x, 130.0F, 2.0F, 480.0F);
  }
  for (int i = 0; i <= 6; ++i) {
    const float y = 130.0F + static_cast<float>(i) * 80.0F;
    fillRect(renderer, 80.0F, y, 1120.0F, 2.0F);
  }
}

std::string_view topDownMapTitle(const TopDownMapOverlay* overlay) {
  // branch-gate: BG-1071
  if (overlay == nullptr || overlay->purpose == "hidden") {
    return "TOP-DOWN MAP";
  }
  // branch-gate: BG-1071
  if (overlay->purpose == "minimap") {
    return "MINIMAP";
  }
  // branch-gate: BG-1071
  if (overlay->purpose == "editor_overview") {
    return "EDITOR OVERVIEW";
  }
  return "TOP-DOWN DEBUG FALLBACK";
}

bool topDownMapUsesCompactLayout(const TopDownMapOverlay* overlay) {
  return overlay != nullptr && overlay->size == "compact";
}

Vec3 topDownMapAnchorFor(const ProductViewportFrame* frame) {
  // branch-gate: BG-1119
  if (frame == nullptr) {
    return {};
  }
  for (const ProductViewportFramedItem& item : frame->framedItems) {
    // branch-gate: BG-1119
    if (item.item.kind == ProductPrimitiveDrawKind::PlayerMarker &&
        item.item.visible) {
      return item.item.worldPosition;
    }
  }
  return {};
}

ProductViewportFramedItem topDownMappedItem(
    const ProductViewportFramedItem& item,
    Vec3 anchor,
    float originX,
    float originY,
    float width,
    float height,
    float pixelsPerMeter,
    float markerScale) {
  ProductViewportFramedItem mapped = item;
  mapped.screenX = originX + width * 0.5F +
                   (item.item.worldPosition.x - anchor.x) * pixelsPerMeter;
  mapped.screenY = originY + height * 0.5F +
                   (item.item.worldPosition.z - anchor.z) * pixelsPerMeter;
  mapped.item.markerSize = std::max(3.0F, item.item.markerSize * markerScale);
  mapped.onScreen = item.item.visible && mapped.screenX >= originX &&
                    mapped.screenX <= originX + width &&
                    mapped.screenY >= originY &&
                    mapped.screenY <= originY + height;
  return mapped;
}

}  // namespace

void drawFirstPersonPrimitiveViewport(SDL_Renderer& renderer,
                                      const ProductViewportFrame* frame) {
  setColor(renderer, 11, 17, 20);
  fillRect(renderer, 80.0F, 130.0F, 1120.0F, 480.0F);
  setColor(renderer, 18, 32, 36);
  fillRect(renderer, 80.0F, 130.0F, 1120.0F, 190.0F);
  setColor(renderer, 22, 28, 27);
  fillRect(renderer, 80.0F, 320.0F, 1120.0F, 290.0F);
  setColor(renderer, 42, 58, 58);
  fillRect(renderer, 80.0F, 319.0F, 1120.0F, 2.0F);

  // branch-gate: BG-1119
  if (frame == nullptr) {
    return;
  }

  for (const ProductViewportFramedItem& item : frame->framedItems) {
    // branch-gate: BG-1119
    if (!item.onScreen) {
      continue;
    }
    drawPrimitiveItem(renderer, item);
  }
}

void drawTopDownMapPrimitives(SDL_Renderer& renderer,
                              const ProductViewportFrame* frame,
                              const TopDownMapOverlay* overlay) {
  // branch-gate: BG-1071
  if (overlay == nullptr || !overlay->visible || frame == nullptr) {
    return;
  }

  const Vec3 anchor = topDownMapAnchorFor(frame);

  // branch-gate: BG-1071
  if (topDownMapUsesCompactLayout(overlay)) {
    constexpr float kMinimapX = 890.0F;
    constexpr float kMinimapY = 116.0F;
    constexpr float kMinimapWidth = 270.0F;
    constexpr float kMinimapHeight = 126.0F;
    setColor(renderer, 18, 24, 27);
    fillRect(renderer, 874.0F, 74.0F, 318.0F, 190.0F);
    setColor(renderer, 126, 201, 176);
    drawText(renderer, topDownMapTitle(overlay), 890.0F, 86.0F, 2.0F);
    setColor(renderer, 32, 48, 48);
    fillRect(renderer, kMinimapX, kMinimapY, kMinimapWidth, kMinimapHeight);
    for (const ProductViewportFramedItem& item : frame->framedItems) {
      drawPrimitiveItem(renderer,
                        topDownMappedItem(item,
                                          anchor,
                                          kMinimapX,
                                          kMinimapY,
                                          kMinimapWidth,
                                          kMinimapHeight,
                                          18.0F,
                                          0.24F));
    }
    return;
  }

  // branch-gate: BG-1071
  if (frame->gridVisible) {
    drawGrid(renderer);
  }
  for (const ProductViewportFramedItem& item : frame->framedItems) {
    drawPrimitiveItem(renderer,
                      topDownMappedItem(item,
                                        anchor,
                                        80.0F,
                                        130.0F,
                                        1120.0F,
                                        480.0F,
                                        80.0F,
                                        1.0F));
  }
}

}  // namespace iggy3d

#endif
