#include "app/iggy3d/window/CreativeInputFrame.hpp"

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/input/ActionState.hpp"

#include <array>

namespace iggy3d {
namespace {

struct ToolKeyRow {
  bool KeyboardCreativeToolKeyPresses::* pressed = nullptr;
  creative::Tool tool = creative::Tool::Select;
};

// Direct tool keys 1/2/3/4 (TL-2: no cycling anywhere).
constexpr std::array<ToolKeyRow, 4> kToolKeyRows = {{
    {&KeyboardCreativeToolKeyPresses::selectPressed, creative::Tool::Select},
    {&KeyboardCreativeToolKeyPresses::movePressed, creative::Tool::Move},
    {&KeyboardCreativeToolKeyPresses::measurePressed, creative::Tool::Measure},
    {&KeyboardCreativeToolKeyPresses::navigatePressed,
     creative::Tool::Navigate},
}};

void mergeDispatchReceipt(ProductCreativeInputFrameReceipt& receipt,
                          const creative::CreativeFacadeToolDispatchReceipt&
                              dispatchReceipt) noexcept {
  receipt.accepted = receipt.accepted || dispatchReceipt.accepted;
  receipt.changed = receipt.changed || dispatchReceipt.changed;
  receipt.inputKind = dispatchReceipt.inputKind;
  receipt.emittedIntentCount = dispatchReceipt.emittedIntentCount;
}

void mergeInputFrameReceipt(ProductCreativeInputFrameReceipt& receipt,
                            const ProductCreativeInputFrameReceipt& next)
    noexcept {
  receipt.actionHandled = receipt.actionHandled || next.actionHandled;
  receipt.toolChanged = receipt.toolChanged || next.toolChanged;
  receipt.pointerDispatched =
      receipt.pointerDispatched || next.pointerDispatched;
  receipt.cancelDispatched = receipt.cancelDispatched || next.cancelDispatched;
  receipt.accepted = receipt.accepted || next.accepted;
  receipt.changed = receipt.changed || next.changed;
  if (next.inputKind != creative::CreativeToolInputKind::Unknown ||
      next.emittedIntentCount != 0U) {
    receipt.inputKind = next.inputKind;
    receipt.emittedIntentCount = next.emittedIntentCount;
  }
}

[[nodiscard]] creative::CreativeToolInputPacket cancelPacket() noexcept {
  creative::CreativeToolInputPacket packet;
  packet.kind = creative::CreativeToolInputKind::Cancel;
  return packet;
}

}  // namespace

bool productCreativeInputActiveForWindow(
    const ProductAppWindowState& window) noexcept {
  return productCreativeDocumentEditorActiveForWindow(window);
}

bool productCreativeToolKeyTarget(
    const KeyboardCreativeToolKeyPresses& presses,
    creative::Tool& out) noexcept {
  for (const ToolKeyRow& row : kToolKeyRows) {
    if (presses.*(row.pressed)) {
      out = row.tool;
      return true;
    }
  }
  return false;
}

creative::CreativeToolInputPacket productCreativePointerPressPacket(
    const MouseClick& click,
    creative::TargetRef target) noexcept {
  creative::CreativeToolInputPacket packet;
  packet.kind = creative::CreativeToolInputKind::PointerPress;
  packet.pointer.x = static_cast<double>(click.x);
  packet.pointer.y = static_cast<double>(click.y);
  packet.pointer.button = creative::CreativeToolPointerButton::Primary;
  packet.pointer.modifiers = creative::kCreativeToolModifierNone;
  packet.pointer.target = target;
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

  if (request.toolKeyRequested) {
    receipt.actionHandled = true;
    // setActiveTool no-ops on the same tool, so key repeat cannot spam
    // toolChanged receipts.
    receipt.toolChanged = facade.setActiveTool(request.toolKey);
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
        facade.dispatchToolInput(productCreativePointerPressPacket(
            request.click, request.pointerTarget));
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

ProductCreativeInputFrameReceipt processProductCreativeInputActions(
    const ProductCreativeInputActionsRequest& request) {
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

  bool dispatched = false;
  for (const ToolKeyRow& row : kToolKeyRows) {
    if (!(request.toolKeys.*(row.pressed))) {
      continue;
    }

    ProductCreativeInputFrameRequest frameRequest;
    frameRequest.window = request.window;
    frameRequest.facade = request.facade;
    frameRequest.toolKeyRequested = true;
    frameRequest.toolKey = row.tool;
    const ProductCreativeInputFrameReceipt frameReceipt =
        processProductCreativeInputFrame(frameRequest);
    mergeInputFrameReceipt(receipt, frameReceipt);
    dispatched = true;
  }

  if (request.actions != nullptr) {
    for (const ActionStateEntry& entry : request.actions->entries) {
      if (!entry.pressed ||
          entry.action != InputAction::EditorCancelPreview) {
        continue;
      }

      ProductCreativeInputFrameRequest frameRequest;
      frameRequest.window = request.window;
      frameRequest.facade = request.facade;
      frameRequest.action = entry.action;
      const ProductCreativeInputFrameReceipt frameReceipt =
          processProductCreativeInputFrame(frameRequest);
      mergeInputFrameReceipt(receipt, frameReceipt);
      dispatched = true;
    }
  }

  if (request.click.clicked) {
    ProductCreativeInputFrameRequest frameRequest;
    frameRequest.window = request.window;
    frameRequest.facade = request.facade;
    frameRequest.click = request.click;
    frameRequest.pointerTarget = request.pointerTarget;
    const ProductCreativeInputFrameReceipt frameReceipt =
        processProductCreativeInputFrame(frameRequest);
    mergeInputFrameReceipt(receipt, frameReceipt);
    dispatched = true;
  }

  receipt.activeToolAfter = facade.toolState().activeTool;
  if (dispatched) {
    receipt.status = "product_creative_input_processed";
    receipt.reasonCode = "product_creative_input_processed";
  } else {
    receipt.status = "product_creative_input_noop";
    receipt.reasonCode = "product_creative_input_noop";
  }

  return receipt;
}

}  // namespace iggy3d
