#pragma once

#include "app/iggy3d/menu/DrawList.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace iggy3d {

enum class ProductUiHitSurface : std::uint8_t {
  None,
  StarterMenu,
  PauseMenu,
  CreativeOverlay,
  Notebook,
};

struct ProductUiHitLayer {
  ProductUiHitSurface surface = ProductUiHitSurface::None;
  const ProductUiDrawList* drawList = nullptr;
  bool enabled = true;
};

struct ProductUiHitRouteRequest {
  const ProductUiHitLayer* layers = nullptr;
  std::size_t layerCount = 0;
  float x = 0.0F;
  float y = 0.0F;
};

struct ProductUiHitRouteReceipt {
  bool requested = false;
  bool hit = false;
  bool consumed = false;
  bool enabled = false;
  ProductUiHitSurface surface = ProductUiHitSurface::None;
  UiHitKind kind = UiHitKind::None;
  FrontendAction action = FrontendAction::None;
  std::size_t layerIndex = 0;
  std::size_t regionIndex = 0;
  std::string semanticId;
  std::string message;
};

[[nodiscard]] ProductUiHitRouteReceipt routeProductUiHit(
    const ProductUiHitRouteRequest& request);

}  // namespace iggy3d
