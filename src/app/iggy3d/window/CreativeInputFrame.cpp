#include "app/iggy3d/window/CreativeInputFrame.hpp"

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/Facade.hpp"

#include <array>
#include <cstddef>

namespace iggy3d {
namespace {

enum class ToolActionMode : unsigned char {
  Direct,
  Next,
  Previous,
};

struct ToolActionRow {
  InputAction action = InputAction::None;
  ToolActionMode mode = ToolActionMode::Direct;
  creative::Tool tool = creative::Tool::Select;
};

constexpr std::array<creative::Tool, 3> kCreativeToolOrder = {
    creative::Tool::Select,
    creative::Tool::Inspect,
    creative::Tool::Measure,
};

constexpr std::array<ToolActionRow, 5> kToolActionRows = {{
    {InputAction::EditorNextTool, ToolActionMode::Next, creative::Tool::Select},
    {InputAction::EditorPreviousTool,
     ToolActionMode::Previous,
     creative::Tool::Select},
    {InputAction::EditorSelect, ToolActionMode::Direct, creative::Tool::Select},
    {InputAction::EditorSelectFloorTool,
     ToolActionMode::Direct,
     creative::Tool::Select},
    {InputAction::EditorSelectWallTool,
     ToolActionMode::Direct,
     creative::Tool::Inspect},
}};

[[nodiscard]] std::size_t toolIndex(creative::Tool tool) noexcept {
  for (std::size_t index = 0; index < kCreativeToolOrder.size(); ++index) {
    if (kCreativeToolOrder[index] == tool) {
      return index;
    }
  }
  return 0;
}

void mergeDispatchReceipt(ProductCreativeInputFrameReceipt& receipt,
                          const creative::CreativeFacadeToolDispatchReceipt&
                              dispatchReceipt) noexcept {
  receipt.accepted = receipt.accepted || dispatchReceipt.accepted;
  receipt.changed = receipt.changed || dispatchReceipt.changed;
  receipt.inputKind = dispatchReceipt.inputKind;
  receipt.emittedIntentCount = dispatchReceipt.emittedIntentCount;
}

[[nodiscard]] creative::CreativeToolInputPacket cancelPacket() noexcept {
  creative::CreativeToolInputPacket packet;
  packet.kind = creative::CreativeToolInputKind::Cancel;
  return packet;
}

}  // namespace

bool productCreativeInputActiveForWindow(
    const ProductAppWindowState& window) noexcept {
  return window.interactionMode == ProductInteractionMode::Creative;
}

creative::Tool nextProductCreativeTool(creative::Tool tool) noexcept {
  const std::size_t index = toolIndex(tool);
  return kCreativeToolOrder[(index + 1U) % kCreativeToolOrder.size()];
}

creative::Tool previousProductCreativeTool(creative::Tool tool) noexcept {
  const std::size_t index = toolIndex(tool);
  return kCreativeToolOrder[(index + kCreativeToolOrder.size() - 1U) %
                            kCreativeToolOrder.size()];
}

bool productCreativeToolActionTarget(InputAction action,
                                     creative::Tool current,
                                     creative::Tool& out) noexcept {
  for (const ToolActionRow& row : kToolActionRows) {
    if (row.action != action) {
      continue;
    }
    if (row.mode == ToolActionMode::Next) {
      out = nextProductCreativeTool(current);
      return true;
    }
    if (row.mode == ToolActionMode::Previous) {
      out = previousProductCreativeTool(current);
      return true;
    }
    out = row.tool;
    return true;
  }
  return false;
}

creative::CreativeToolInputPacket productCreativePointerPressPacket(
    const MouseClick& click) noexcept {
  creative::CreativeToolInputPacket packet;
  packet.kind = creative::CreativeToolInputKind::PointerPress;
  packet.pointer.x = static_cast<double>(click.x);
  packet.pointer.y = static_cast<double>(click.y);
  packet.pointer.button = creative::CreativeToolPointerButton::Primary;
  packet.pointer.modifiers = creative::kCreativeToolModifierNone;
  packet.pointer.target.value = creative::kInvalidId;
  return packet;
}

ProductCreativeInputFrameReceipt processProductCreativeInputFrame(
    const ProductCreativeInputFrameRequest& request) {
  ProductCreativeInputFrameReceipt receipt;

  if (request.window == nullptr) {
    receipt.status = "product_creative_input_window_missing";
    receipt.reasonCode = "product_creative_input_window_missing";
    return receipt;
  }

  receipt.active = productCreativeInputActiveForWindow(*request.window);
  if (!receipt.active) {
    receipt.status = "product_creative_input_inactive";
    receipt.reasonCode = "product_creative_input_inactive";
    return receipt;
  }

  receipt.requested = true;
  if (request.facade == nullptr) {
    receipt.status = "product_creative_input_facade_missing";
    receipt.reasonCode = "product_creative_input_facade_missing";
    return receipt;
  }

  creative::Facade& facade = *request.facade;
  receipt.facadeAvailable = true;
  receipt.activeToolBefore = facade.toolState().activeTool;
  receipt.activeToolAfter = receipt.activeToolBefore;

  creative::Tool targetTool = receipt.activeToolBefore;
  if (productCreativeToolActionTarget(request.action,
                                      receipt.activeToolBefore,
                                      targetTool)) {
    receipt.actionHandled = true;
    receipt.toolChanged = facade.setActiveTool(targetTool);
    receipt.accepted = true;
    receipt.changed = receipt.changed || receipt.toolChanged;
  }

  if (request.action == InputAction::EditorCancelPreview) {
    const creative::CreativeFacadeToolDispatchReceipt dispatchReceipt =
        facade.dispatchToolInput(cancelPacket());
    receipt.cancelDispatched = true;
    mergeDispatchReceipt(receipt, dispatchReceipt);
  }

  if (request.click.clicked) {
    const creative::CreativeFacadeToolDispatchReceipt dispatchReceipt =
        facade.dispatchToolInput(productCreativePointerPressPacket(request.click));
    receipt.pointerDispatched = true;
    mergeDispatchReceipt(receipt, dispatchReceipt);
  }

  receipt.activeToolAfter = facade.toolState().activeTool;
  if (receipt.actionHandled || receipt.cancelDispatched ||
      receipt.pointerDispatched) {
    receipt.status = "product_creative_input_processed";
    receipt.reasonCode = "product_creative_input_processed";
  } else {
    receipt.status = "product_creative_input_noop";
    receipt.reasonCode = "product_creative_input_noop";
  }

  return receipt;
}

}  // namespace iggy3d
