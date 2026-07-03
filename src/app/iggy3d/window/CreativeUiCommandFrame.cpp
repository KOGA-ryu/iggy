#include "app/iggy3d/window/CreativeUiCommandFrame.hpp"

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/window/CreativeInputFrame.hpp"

#include <array>
#include <string_view>

namespace iggy3d {
namespace {

struct ProductCreativeUiCommandRow {
  std::string_view semanticId;
  ProductCreativeUiCommandKind commandKind =
      ProductCreativeUiCommandKind::None;
};

constexpr std::array<ProductCreativeUiCommandRow, 1>
    kProductCreativeUiCommandRows = {{
        {"creative.row.tools.active_tool",
         ProductCreativeUiCommandKind::CycleNextTool},
    }};

[[nodiscard]] const ProductCreativeUiCommandRow* findCommandRow(
    std::string_view semanticId) noexcept {
  for (const ProductCreativeUiCommandRow& row :
       kProductCreativeUiCommandRows) {
    if (row.semanticId == semanticId) {
      return &row;
    }
  }
  return nullptr;
}

void setNoopStatus(ProductCreativeUiCommandFrameReceipt& receipt,
                  std::string_view status) {
  receipt.status = std::string(status);
  receipt.reasonCode = std::string(status);
}

}  // namespace

ProductCreativeUiCommandFrameReceipt routeProductCreativeUiCommandFrame(
    const ProductCreativeUiCommandFrameRequest& request) {
  ProductCreativeUiCommandFrameReceipt receipt;
  receipt.requested = true;
  receipt.inputConsumed = request.inputReceipt.consumed;
  receipt.inputEnabled = request.inputReceipt.enabled;
  receipt.semanticId = request.inputReceipt.semanticId;

  if (request.facade == nullptr) {
    setNoopStatus(receipt, "product_creative_ui_command_facade_missing");
    return receipt;
  }

  creative::Facade& facade = *request.facade;
  receipt.facadeAvailable = true;
  receipt.toolBefore = facade.toolState().activeTool;
  receipt.toolAfter = receipt.toolBefore;

  if (!receipt.inputConsumed) {
    setNoopStatus(receipt, "product_creative_ui_command_not_consumed");
    return receipt;
  }

  if (!receipt.inputEnabled) {
    setNoopStatus(receipt, "product_creative_ui_command_disabled");
    return receipt;
  }

  const ProductCreativeUiCommandRow* row =
      findCommandRow(request.inputReceipt.semanticId);
  if (row == nullptr) {
    setNoopStatus(receipt, "product_creative_ui_command_unknown_semantic");
    return receipt;
  }

  receipt.commandKind = row->commandKind;
  if (receipt.commandKind == ProductCreativeUiCommandKind::CycleNextTool) {
    receipt.accepted = true;
    receipt.changed = facade.setActiveTool(
        nextProductCreativeTool(receipt.toolBefore));
    receipt.toolAfter = facade.toolState().activeTool;
    setNoopStatus(receipt, "product_creative_ui_command_applied");
    return receipt;
  }

  setNoopStatus(receipt, "product_creative_ui_command_unknown_command");
  return receipt;
}

}  // namespace iggy3d
