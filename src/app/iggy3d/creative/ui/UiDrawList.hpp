#pragma once

#include "app/iggy3d/creative/ui/Ui.hpp"
#include "app/iggy3d/creative/ui/CreativeUiDrawTypes.hpp"

#include <cstdint>

namespace iggy3d {

struct ProductCreativeUiDrawListRequest {
  const creative::CreativeUiModel* model = nullptr;
  std::uint32_t virtualWidth = 1280;
  std::uint32_t virtualHeight = 720;
  CreativeUiThemeId theme = CreativeUiThemeId::System;
};

[[nodiscard]] CreativeUiDrawList buildProductCreativeUiDrawList(
    const ProductCreativeUiDrawListRequest& request);

}  // namespace iggy3d
