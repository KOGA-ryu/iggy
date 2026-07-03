#pragma once

#include "app/iggy3d/creative/Ui.hpp"
#include "app/iggy3d/menu/DrawList.hpp"

#include <cstdint>

namespace iggy3d {

struct ProductCreativeUiDrawListRequest {
  const creative::CreativeUiModel* model = nullptr;
  std::uint32_t virtualWidth = 1280;
  std::uint32_t virtualHeight = 720;
  ProductUiThemeId theme = ProductUiThemeId::System;
};

[[nodiscard]] ProductUiDrawList buildProductCreativeUiDrawList(
    const ProductCreativeUiDrawListRequest& request);

}  // namespace iggy3d
