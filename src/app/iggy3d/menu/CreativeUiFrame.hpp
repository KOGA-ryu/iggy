#pragma once

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/menu/CreativeUiProjection.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d {

struct ProductCreativeUiFrameRequest {
  ProductAppWindowState* window = nullptr;
  const creative::Facade* facade = nullptr;
  std::uint32_t virtualWidth = 1280;
  std::uint32_t virtualHeight = 720;
  ProductUiThemeId theme = ProductUiThemeId::System;
};

struct ProductCreativeUiFrameReceipt {
  bool requested = false;
  bool active = false;
  bool projected = false;
  bool recorded = false;
  bool ready = false;
  bool partial = false;
  std::string_view status = "product_creative_ui_frame_not_requested";
  std::string_view reasonCode = "product_creative_ui_frame_not_requested";
  std::uint64_t primitiveCount = 0;
  std::uint64_t textCount = 0;
  std::uint64_t rectCount = 0;
  std::uint64_t rowCount = 0;
  std::uint64_t hitRegionCount = 0;
};

struct ProductCreativeUiFrame {
  ProductCreativeUiProjection projection;
  ProductCreativeUiFrameReceipt receipt;
};

[[nodiscard]] bool productCreativeUiActiveForWindow(
    const ProductAppWindowState& window) noexcept;
[[nodiscard]] ProductCreativeUiFrame buildProductCreativeUiFrame(
    const ProductCreativeUiFrameRequest& request);

}  // namespace iggy3d
