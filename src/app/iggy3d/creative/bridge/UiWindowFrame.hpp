#pragma once

#include "app/iggy3d/creative/ui/UiFrame.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d {

struct ProductCreativeUiWindowFrameRequest {
  ProductAppWindowState* window = nullptr;
  const creative::CreativeAppState* creative = nullptr;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
  std::uint32_t fallbackWidth = 1280;
  std::uint32_t fallbackHeight = 720;
  ProductUiThemeId theme = ProductUiThemeId::System;
  std::uint32_t logicalWidth = 0;
  std::uint32_t logicalHeight = 0;
};

struct ProductCreativeUiOverlayInputAvailabilityRequest {
  const ProductUiDrawList* drawList = nullptr;
  bool rendererCanRenderCreativeOverlay = false;
  bool windowDrawable = false;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
  bool gameplayOverlayRenderable = false;
};

struct ProductCreativeUiOverlayInputAvailability {
  bool requested = false;
  bool drawListAvailable = false;
  bool drawListReady = false;
  bool rendererCanRenderCreativeOverlay = false;
  bool windowDrawable = false;
  bool drawableAvailable = false;
  bool gameplayOverlayRenderable = false;
  bool inputAvailable = false;
  std::string_view status =
      "product_creative_ui_overlay_input_not_requested";
  std::string_view reasonCode =
      "product_creative_ui_overlay_input_not_requested";
};

[[nodiscard]] ProductCreativeUiFrame buildProductCreativeUiWindowFrame(
    const ProductCreativeUiWindowFrameRequest& request);
[[nodiscard]] ProductCreativeUiOverlayInputAvailability
resolveProductCreativeUiOverlayInputAvailability(
    const ProductCreativeUiOverlayInputAvailabilityRequest& request) noexcept;

}  // namespace iggy3d
