#pragma once

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/window/CreativeUiInputFrame.hpp"

#include <cstdint>
#include <string>

namespace iggy3d {

namespace creative {
class Facade;
}  // namespace creative

enum class ProductCreativeUiCommandKind : std::uint8_t {
  None,
  CycleNextTool,
};

struct ProductCreativeUiCommandFrameRequest {
  creative::Facade* facade = nullptr;
  ProductCreativeUiInputFrameReceipt inputReceipt;
};

struct ProductCreativeUiCommandFrameReceipt {
  bool requested = false;
  bool facadeAvailable = false;
  bool inputConsumed = false;
  bool inputEnabled = false;
  bool accepted = false;
  bool changed = false;
  ProductCreativeUiCommandKind commandKind =
      ProductCreativeUiCommandKind::None;
  creative::Tool toolBefore = creative::Tool::Select;
  creative::Tool toolAfter = creative::Tool::Select;
  std::string semanticId;
  std::string status = "product_creative_ui_command_not_requested";
  std::string reasonCode = "product_creative_ui_command_not_requested";
};

[[nodiscard]] ProductCreativeUiCommandFrameReceipt
routeProductCreativeUiCommandFrame(
    const ProductCreativeUiCommandFrameRequest& request);

}  // namespace iggy3d
