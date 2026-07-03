#pragma once

#include "app/iggy3d/creative/Ui.hpp"
#include "app/iggy3d/menu/DrawList.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d {

namespace creative {
class Facade;
}  // namespace creative

struct ProductCreativeUiProjectionRequest {
  const creative::CreativeUiModel* model = nullptr;
  const creative::Facade* facade = nullptr;
  std::uint32_t virtualWidth = 1280;
  std::uint32_t virtualHeight = 720;
  ProductUiThemeId theme = ProductUiThemeId::System;
};

struct ProductCreativeUiProjectionReceipt {
  bool requested = false;
  bool ready = false;
  bool partial = false;
  std::string_view status = "product_creative_ui_not_requested";
  std::string_view reasonCode = "product_creative_ui_not_requested";
  bool usedFacade = false;
  bool usedModel = false;
  std::uint32_t virtualWidth = 0;
  std::uint32_t virtualHeight = 0;
  ProductUiThemeId theme = ProductUiThemeId::System;
  std::uint64_t panelCount = 0;
  std::uint64_t modelRowCount = 0;
  std::uint64_t primitiveCount = 0;
  std::uint64_t textCount = 0;
  std::uint64_t rectCount = 0;
  std::uint64_t rowCount = 0;
  std::uint64_t disabledRowCount = 0;
  std::uint64_t hitRegionCount = 0;
};

struct ProductCreativeUiProjection {
  ProductUiDrawList drawList;
  ProductCreativeUiProjectionReceipt receipt;
};

[[nodiscard]] ProductCreativeUiProjection buildProductCreativeUiProjection(
    const ProductCreativeUiProjectionRequest& request);

}  // namespace iggy3d
