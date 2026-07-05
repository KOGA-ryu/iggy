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
  receipt.pointerMoveDispatched =
      receipt.pointerMoveDispatched || next.pointerMoveDispatched;
  receipt.pointerReleaseDispatched =
      receipt.pointerReleaseDispatched || next.pointerReleaseDispatched;
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

creative::CreativeToolInputPacket productCreativePointerMovePacket(
    float x,
    float y,
    creative::TargetRef target,
    bool hasWorldDestination,
    creative::CreativeToolWorldPoint worldDestination) noexcept {
  creative::CreativeToolInputPacket packet;
  packet.kind = creative::CreativeToolInputKind::PointerMove;
  packet.pointer.x = static_cast<double>(x);
  packet.pointer.y = static_cast<double>(y);
  // The primary button stays held through a drag Move; the tool core reads the
  // button to know a gesture is in flight (vs a hover), so report it Primary.
  packet.pointer.button = creative::CreativeToolPointerButton::Primary;
  packet.pointer.modifiers = creative::kCreativeToolModifierNone;
  packet.pointer.target = target;
  packet.pointer.hasWorldDestination = hasWorldDestination;
  packet.pointer.worldDestination = worldDestination;
  return packet;
}

creative::CreativeToolInputPacket productCreativePointerReleasePacket(
    float x,
    float y,
    creative::TargetRef target,
    bool hasWorldDestination,
    creative::CreativeToolWorldPoint worldDestination) noexcept {
  creative::CreativeToolInputPacket packet;
  packet.kind = creative::CreativeToolInputKind::PointerRelease;
  packet.pointer.x = static_cast<double>(x);
  packet.pointer.y = static_cast<double>(y);
  // Release names the button that came up; the pointer is no longer held after.
  packet.pointer.button = creative::CreativeToolPointerButton::Primary;
  packet.pointer.modifiers = creative::kCreativeToolModifierNone;
  packet.pointer.target = target;
  packet.pointer.hasWorldDestination = hasWorldDestination;
  packet.pointer.worldDestination = worldDestination;
  return packet;
}

void resetProductCreativePointerLifecycle(
    ProductCreativePointerLifecycleState& state) noexcept {
  state.primaryButtonHeld = false;
  state.lastPointerX = 0.0F;
  state.lastPointerY = 0.0F;
}

ProductCreativePointerLifecycleEvent resolveProductCreativePointerLifecycle(
    ProductCreativePointerLifecycleState& state,
    const ProductCreativePointerSample& sample) noexcept {
  ProductCreativePointerLifecycleEvent event;
  event.x = sample.x;
  event.y = sample.y;

  const bool wasHeld = state.primaryButtonHeld;
  const bool nowDown = sample.primaryButtonDown;

  if (!wasHeld && nowDown) {
    // Button-down edge. The pick chain owns Press so it can resolve a target;
    // the lifecycle only records that the gesture is now in flight.
    event.phase = ProductCreativePointerLifecyclePhase::Press;
  } else if (wasHeld && !nowDown) {
    // Button-up edge closes the gesture.
    event.phase = ProductCreativePointerLifecyclePhase::Release;
  } else if (wasHeld && nowDown) {
    // Held: only a real position change is a Move (no zero-delta spam).
    if (sample.x != state.lastPointerX || sample.y != state.lastPointerY) {
      event.phase = ProductCreativePointerLifecyclePhase::Move;
    }
  }

  state.primaryButtonHeld = nowDown;
  state.lastPointerX = sample.x;
  state.lastPointerY = sample.y;
  return event;
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

  // TL-3 lifecycle continuation: Press already flowed through the pick chain
  // (via `click`), so here we only synthesize the Move (held drag) and the
  // Release that finally ENDS a gesture (Measure end, future Move commit).
  switch (request.pointerLifecycle.phase) {
    case ProductCreativePointerLifecyclePhase::Move: {
      const creative::CreativeFacadeToolDispatchReceipt dispatchReceipt =
          facade.dispatchToolInput(productCreativePointerMovePacket(
              request.pointerLifecycle.x,
              request.pointerLifecycle.y,
              request.pointerLifecycleTarget,
              request.pointerLifecycle.hasWorldDestination,
              request.pointerLifecycle.worldDestination));
      receipt.pointerMoveDispatched = true;
      mergeDispatchReceipt(receipt, dispatchReceipt);
      break;
    }
    case ProductCreativePointerLifecyclePhase::Release: {
      const creative::CreativeFacadeToolDispatchReceipt dispatchReceipt =
          facade.dispatchToolInput(productCreativePointerReleasePacket(
              request.pointerLifecycle.x,
              request.pointerLifecycle.y,
              request.pointerLifecycleTarget,
              request.pointerLifecycle.hasWorldDestination,
              request.pointerLifecycle.worldDestination));
      receipt.pointerReleaseDispatched = true;
      mergeDispatchReceipt(receipt, dispatchReceipt);
      break;
    }
    case ProductCreativePointerLifecyclePhase::Press:
    case ProductCreativePointerLifecyclePhase::None:
      // Press is owned by the pick chain above; None emits nothing.
      break;
  }

  receipt.activeToolAfter = facade.toolState().activeTool;
  if (receipt.actionHandled || receipt.cancelDispatched ||
      receipt.pointerDispatched || receipt.pointerMoveDispatched ||
      receipt.pointerReleaseDispatched) {
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

  const bool hasLifecyclePacket =
      request.pointerLifecycle.phase ==
          ProductCreativePointerLifecyclePhase::Move ||
      request.pointerLifecycle.phase ==
          ProductCreativePointerLifecyclePhase::Release;
  if (request.click.clicked || hasLifecyclePacket) {
    ProductCreativeInputFrameRequest frameRequest;
    frameRequest.window = request.window;
    frameRequest.facade = request.facade;
    frameRequest.click = request.click;
    frameRequest.pointerTarget = request.pointerTarget;
    frameRequest.pointerLifecycle = request.pointerLifecycle;
    frameRequest.pointerLifecycleTarget = request.pointerLifecycleTarget;
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
