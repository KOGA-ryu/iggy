#pragma once

#include "app/iggy3d/menu/CreativeUiFrame.hpp"

#include <cstdint>

namespace iggy3d {

struct ProductCreativeUiWindowFrameRequest {
  ProductAppWindowState* window = nullptr;
  const creative::Facade* facade = nullptr;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
  std::uint32_t fallbackWidth = 1280;
  std::uint32_t fallbackHeight = 720;
  ProductUiThemeId theme = ProductUiThemeId::System;
};

[[nodiscard]] ProductCreativeUiFrame buildProductCreativeUiWindowFrame(
    const ProductCreativeUiWindowFrameRequest& request);

}  // namespace iggy3d
