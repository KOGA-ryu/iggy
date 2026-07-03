#pragma once

#include "app/iggy3d/menu/DrawList.hpp"
#include "app/iggy3d/menu/UiHitRouter.hpp"
#include "app/input/MouseInput.hpp"

#include <cstddef>
#include <string>

namespace iggy3d {

struct ProductCreativeUiInputFrameRequest {
  const ProductUiDrawList* creativeUiDrawList = nullptr;
  MouseClick click;
};

struct ProductCreativeUiInputFrameReceipt {
  bool requested = false;
  bool clickPresent = false;
  bool drawListAvailable = false;
  bool routed = false;
  bool hit = false;
  bool consumed = false;
  bool enabled = false;
  ProductUiHitSurface surface = ProductUiHitSurface::None;
  UiHitKind kind = UiHitKind::None;
  FrontendAction action = FrontendAction::None;
  std::size_t layerIndex = 0;
  std::size_t regionIndex = 0;
  std::string semanticId;
  std::string status = "product_creative_ui_input_not_requested";
  std::string reasonCode = "product_creative_ui_input_not_requested";
};

[[nodiscard]] ProductCreativeUiInputFrameReceipt routeProductCreativeUiInputFrame(
    const ProductCreativeUiInputFrameRequest& request);

}  // namespace iggy3d
