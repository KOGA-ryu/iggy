#include "app/iggy3d/window/CreativeInputFrame.hpp"

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/input/ActionState.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

iggy3d::ProductAppWindowState creativeWindow() {
  iggy3d::ProductAppWindowState window;
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  window.activeCreativeSaveId = "creative_save";
  window.activeCreativeWorldId = "world_001";
  window.activeCreativeDocumentId = 42U;
  return window;
}

iggy3d::MouseClick clickAt(float x, float y) {
  iggy3d::MouseClick click;
  click.clicked = true;
  click.x = x;
  click.y = y;
  return click;
}

void recordTestAction(iggy3d::ActionState& actions,
                      iggy3d::InputAction action,
                      bool pressed) {
  iggy3d::recordAction(actions, action, true, pressed, false, 1.0F);
}

iggy3d::KeyboardCreativeToolKeyPresses moveKeyPressed() {
  iggy3d::KeyboardCreativeToolKeyPresses presses;
  presses.movePressed = true;
  return presses;
}

bool activeRuleUsesCreativeDocumentIdentity() {
  iggy3d::ProductAppWindowState window;
  const bool playerActive = iggy3d::productCreativeInputActiveForWindow(window);
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  const bool legacyCreativeActive =
      iggy3d::productCreativeInputActiveForWindow(window);
  window.activeCreativeDocumentId = 42U;
  const bool documentCreativeActive =
      iggy3d::productCreativeInputActiveForWindow(window);

  return expect(!playerActive, "player inactive") &&
         expect(!legacyCreativeActive, "legacy creative inactive") &&
         expect(documentCreativeActive, "document creative active");
}

bool nullWindowReturnsWindowMissing() {
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame({});

  return expect(!receipt.requested, "null not requested") &&
         expect(!receipt.active, "null inactive") &&
         expect(!receipt.facadeAvailable, "null no facade") &&
         expect(!receipt.accepted, "null not accepted") &&
         expect(!receipt.changed, "null unchanged") &&
         expect(receipt.status == "product_creative_input_window_missing",
                "null status") &&
         expect(receipt.reasonCode == "product_creative_input_window_missing",
                "null reason");
}

bool inactiveWindowNoopsAndDoesNotMutateFacade() {
  iggy3d::ProductAppWindowState window;
  cr::Facade facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  const std::uint64_t objectCountBefore = facade.document().objectCount();

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  request.toolKeyRequested = true;
  request.toolKey = cr::Tool::Measure;
  request.click = clickAt(10.0F, 20.0F);
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame(request);

  return expect(!receipt.requested, "inactive not requested") &&
         expect(!receipt.active, "inactive active false") &&
         expect(!receipt.actionHandled, "inactive action not handled") &&
         expect(!receipt.pointerDispatched, "inactive pointer not dispatched") &&
         expect(receipt.status == "product_creative_input_inactive",
                "inactive status") &&
         expect(facade.toolState().activeTool == cr::Tool::Move,
                "inactive tool unchanged") &&
         expect(facade.document().objectCount() == objectCountBefore,
                "inactive document unchanged");
}

bool activeCreativeNullFacadeReportsMissing() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.toolKeyRequested = true;
  request.toolKey = cr::Tool::Move;
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame(request);

  return expect(receipt.requested, "missing facade requested") &&
         expect(receipt.active, "missing facade active") &&
         expect(!receipt.facadeAvailable, "missing facade unavailable") &&
         expect(!receipt.actionHandled, "missing facade action untouched") &&
         expect(receipt.status == "product_creative_input_facade_missing",
                "missing facade status");
}

bool toolKeysMapDirectlyToTools() {
  bool ok = true;
  struct Row {
    bool iggy3d::KeyboardCreativeToolKeyPresses::* pressed;
    cr::Tool tool;
    const char* label;
  };
  const Row rows[] = {
      {&iggy3d::KeyboardCreativeToolKeyPresses::selectPressed,
       cr::Tool::Select, "key 1 maps select"},
      {&iggy3d::KeyboardCreativeToolKeyPresses::movePressed,
       cr::Tool::Move, "key 2 maps move"},
      {&iggy3d::KeyboardCreativeToolKeyPresses::measurePressed,
       cr::Tool::Measure, "key 3 maps measure"},
      {&iggy3d::KeyboardCreativeToolKeyPresses::navigatePressed,
       cr::Tool::Navigate, "key 4 maps navigate"},
  };
  for (const Row& row : rows) {
    iggy3d::KeyboardCreativeToolKeyPresses presses;
    presses.*(row.pressed) = true;
    cr::Tool out = cr::Tool::Measure;
    ok &= expect(iggy3d::productCreativeToolKeyTarget(presses, out) &&
                     out == row.tool,
                 row.label);
  }

  cr::Tool out = cr::Tool::Measure;
  ok &= expect(!iggy3d::productCreativeToolKeyTarget({}, out) &&
                   out == cr::Tool::Measure,
               "no tool key pressed rejected");
  return ok;
}

bool heldToolKeyRecordsNoPressEdge() {
  iggy3d::KeyboardInputState keyboard;
  iggy3d::KeyboardCreativeToolInputSample sample;
  sample.moveToolDown = true;

  const iggy3d::KeyboardCreativeToolKeyPresses first =
      iggy3d::recordKeyboardCreativeToolKeys(keyboard, sample);
  const iggy3d::KeyboardCreativeToolKeyPresses held =
      iggy3d::recordKeyboardCreativeToolKeys(keyboard, sample);
  const iggy3d::KeyboardCreativeToolKeyPresses released =
      iggy3d::recordKeyboardCreativeToolKeys(keyboard, {});
  const iggy3d::KeyboardCreativeToolKeyPresses pressedAgain =
      iggy3d::recordKeyboardCreativeToolKeys(keyboard, sample);

  return expect(first.movePressed, "first move key press edge") &&
         expect(!first.selectPressed && !first.measurePressed &&
                    !first.navigatePressed,
                "first move key only") &&
         expect(!held.movePressed, "held move key no edge") &&
         expect(!released.movePressed, "released move key no edge") &&
         expect(pressedAgain.movePressed, "re-pressed move key edge");
}

bool clickPacketMapsMouseClick() {
  const iggy3d::MouseClick click = clickAt(12.5F, 34.25F);
  const cr::CreativeToolInputPacket packet =
      iggy3d::productCreativePointerPressPacket(click);

  return expect(packet.kind == cr::CreativeToolInputKind::PointerPress,
                "packet press") &&
         expect(packet.pointer.button == cr::CreativeToolPointerButton::Primary,
                "packet primary") &&
         expect(packet.pointer.x == 12.5, "packet x") &&
         expect(packet.pointer.y == 34.25, "packet y") &&
         expect(packet.pointer.modifiers == cr::kCreativeToolModifierNone,
                "packet modifiers none") &&
         expect(packet.pointer.target.value == cr::kInvalidId,
                "packet target invalid");
}

bool clickPacketPreservesPickedTarget() {
  const iggy3d::MouseClick click = clickAt(12.5F, 34.25F);
  cr::TargetRef target;
  target.value = 77;
  const cr::CreativeToolInputPacket packet =
      iggy3d::productCreativePointerPressPacket(click, target);

  return expect(packet.kind == cr::CreativeToolInputKind::PointerPress,
                "target packet press") &&
         expect(packet.pointer.target.value == 77U,
                "target packet target copied") &&
         expect(packet.pointer.x == 12.5, "target packet x") &&
         expect(packet.pointer.y == 34.25, "target packet y");
}

bool toolKeyChangesFacadeToolWithoutDocumentMutation() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();
  const std::uint64_t objectCountBefore = facade.document().objectCount();

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  request.toolKeyRequested = true;
  request.toolKey = cr::Tool::Move;
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame(request);

  return expect(receipt.requested, "tool key requested") &&
         expect(receipt.active, "tool key active") &&
         expect(receipt.facadeAvailable, "tool key facade") &&
         expect(receipt.actionHandled, "tool key handled") &&
         expect(receipt.toolChanged, "tool key changed") &&
         expect(receipt.accepted, "tool key accepted") &&
         expect(receipt.changed, "tool key receipt changed") &&
         expect(receipt.activeToolBefore == cr::Tool::Select,
                "tool before select") &&
         expect(receipt.activeToolAfter == cr::Tool::Move,
                "tool after move") &&
         expect(facade.toolState().activeTool == cr::Tool::Move,
                "facade active move") &&
         expect(facade.document().objectCount() == objectCountBefore,
                "tool key document unchanged");
}

bool repeatedToolKeyDoesNotSpamToolChanged() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();

  iggy3d::ProductCreativeInputActionsRequest request;
  request.window = &window;
  request.facade = &facade;
  request.toolKeys = moveKeyPressed();
  const iggy3d::ProductCreativeInputFrameReceipt first =
      iggy3d::processProductCreativeInputActions(request);
  const iggy3d::ProductCreativeInputFrameReceipt repeat =
      iggy3d::processProductCreativeInputActions(request);

  return expect(first.actionHandled, "first tool key handled") &&
         expect(first.toolChanged, "first tool key changed") &&
         expect(repeat.actionHandled, "repeat tool key handled") &&
         expect(!repeat.toolChanged, "repeat tool key no toolChanged") &&
         expect(!repeat.changed, "repeat tool key no change") &&
         expect(facade.toolState().activeTool == cr::Tool::Move,
                "repeat facade still move");
}

bool selectClickWithPickedTargetUpdatesSelection() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();
  cr::TargetRef target;
  target.value = 101;

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  request.click = clickAt(18.0F, 19.0F);
  request.pointerTarget = target;
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame(request);

  return expect(receipt.pointerDispatched, "select target pointer dispatched") &&
         expect(receipt.inputKind == cr::CreativeToolInputKind::PointerPress,
                "select target input kind") &&
         expect(receipt.emittedIntentCount == 1U,
                "select target emitted") &&
         expect(receipt.changed, "select target changed") &&
         expect(facade.selectionState().selectedTarget.value == 101U,
                "select target selected") &&
         expect(facade.state().selected.value == 101U,
                "select target old state selected") &&
         expect(facade.document().objectCount() == 0U,
                "select target document unchanged");
}

bool moveClickWithPickedTargetSelects() {
  // Move selects like Select until the drag slice (TV1-F/G) lands.
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  cr::TargetRef target;
  target.value = 202;

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  request.click = clickAt(28.0F, 29.0F);
  request.pointerTarget = target;
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame(request);

  return expect(receipt.pointerDispatched, "move target pointer dispatched") &&
         expect(receipt.changed, "move target changed") &&
         expect(facade.selectionState().selectedTarget.value == 202U,
                "move target selected") &&
         expect(facade.document().objectCount() == 0U,
                "move target document unchanged");
}

bool navigateClickIsInert() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Navigate));
  cr::TargetRef target;
  target.value = 303;

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  request.click = clickAt(28.0F, 29.0F);
  request.pointerTarget = target;
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame(request);

  return expect(receipt.pointerDispatched, "navigate pointer dispatched") &&
         expect(receipt.accepted, "navigate pointer accepted") &&
         expect(receipt.emittedIntentCount == 0U,
                "navigate pointer no intents") &&
         expect(!receipt.changed, "navigate pointer unchanged") &&
         expect(facade.selectionState().selectedTarget.value ==
                    cr::kInvalidId,
                "navigate pointer selection untouched") &&
         expect(!facade.ghostState().visible,
                "navigate pointer no ghost") &&
         expect(facade.document().objectCount() == 0U,
                "navigate pointer document unchanged");
}

bool clickWithMeasureActiveBeginsMeasurement() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  request.click = clickAt(48.0F, 96.5F);
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame(request);

  return expect(receipt.pointerDispatched, "measure pointer dispatched") &&
         expect(receipt.inputKind == cr::CreativeToolInputKind::PointerPress,
                "measure input kind press") &&
         expect(receipt.emittedIntentCount == 1U,
                "measure emitted begin intent") &&
         expect(receipt.accepted, "measure click accepted") &&
         expect(receipt.changed, "measure click changed") &&
         expect(receipt.activeToolBefore == cr::Tool::Measure,
                "measure before") &&
         expect(receipt.activeToolAfter == cr::Tool::Measure,
                "measure after") &&
         expect(facade.measurementState().active,
                "measurement active after click") &&
         expect(facade.measurementState().hasMeasurement,
                "measurement exists after click") &&
         expect(facade.measurementState().startPoint.x == 48.0,
                "measurement start x") &&
         expect(facade.measurementState().startPoint.y == 96.5,
                "measurement start y");
}

bool measureClickWithPickedTargetStoresMeasurementTarget() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));
  cr::TargetRef target;
  target.value = 303;

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  request.click = clickAt(38.0F, 39.0F);
  request.pointerTarget = target;
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame(request);

  return expect(receipt.pointerDispatched, "measure target pointer dispatched") &&
         expect(facade.measurementState().active,
                "measure target active") &&
         expect(facade.measurementState().startPoint.target.value == 303U,
                "measure target start target") &&
         expect(facade.measurementState().currentPoint.target.value == 303U,
                "measure target current target") &&
         expect(facade.selectionState().selectedTarget.value ==
                    cr::kInvalidId,
                "measure target selection unchanged");
}

bool editorCancelPreviewCancelsActiveMeasurement() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));

  iggy3d::ProductCreativeInputFrameRequest beginRequest;
  beginRequest.window = &window;
  beginRequest.facade = &facade;
  beginRequest.click = clickAt(10.0F, 10.0F);
  static_cast<void>(iggy3d::processProductCreativeInputFrame(beginRequest));

  iggy3d::ProductCreativeInputFrameRequest cancelRequest;
  cancelRequest.window = &window;
  cancelRequest.facade = &facade;
  cancelRequest.action = iggy3d::InputAction::EditorCancelPreview;
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame(cancelRequest);

  return expect(receipt.cancelDispatched, "cancel dispatched") &&
         expect(receipt.inputKind == cr::CreativeToolInputKind::Cancel,
                "cancel input kind") &&
         expect(receipt.emittedIntentCount == 1U, "cancel emitted intent") &&
         expect(receipt.accepted, "cancel accepted") &&
         expect(receipt.changed, "cancel changed") &&
         expect(!facade.measurementState().active,
                "measurement inactive after cancel") &&
         expect(!facade.measurementState().hasMeasurement,
                "measurement cleared after cancel");
}

bool noApplicableInputReturnsNoop() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame(request);

  return expect(receipt.requested, "noop requested") &&
         expect(receipt.active, "noop active") &&
         expect(receipt.facadeAvailable, "noop facade available") &&
         expect(!receipt.actionHandled, "noop action false") &&
         expect(!receipt.pointerDispatched, "noop pointer false") &&
         expect(!receipt.cancelDispatched, "noop cancel false") &&
         expect(!receipt.accepted, "noop not accepted") &&
         expect(!receipt.changed, "noop unchanged") &&
         expect(receipt.inputKind == cr::CreativeToolInputKind::Unknown,
                "noop input kind unknown") &&
         expect(receipt.emittedIntentCount == 0U, "noop emitted zero") &&
         expect(receipt.status == "product_creative_input_noop",
                "noop status");
}

bool batchNoopsForClosedInputs() {
  const iggy3d::ProductCreativeInputFrameReceipt nullWindowReceipt =
      iggy3d::processProductCreativeInputActions({});

  iggy3d::ProductAppWindowState inactiveWindow;
  cr::Facade inactiveFacade;
  inactiveFacade.reset();
  static_cast<void>(inactiveFacade.setActiveTool(cr::Tool::Measure));
  iggy3d::ActionState inactiveActions;
  recordTestAction(inactiveActions,
                   iggy3d::InputAction::EditorCancelPreview,
                   true);
  iggy3d::ProductCreativeInputActionsRequest inactiveRequest;
  inactiveRequest.window = &inactiveWindow;
  inactiveRequest.facade = &inactiveFacade;
  inactiveRequest.actions = &inactiveActions;
  inactiveRequest.toolKeys = moveKeyPressed();
  inactiveRequest.click = clickAt(2.0F, 3.0F);
  const iggy3d::ProductCreativeInputFrameReceipt inactiveReceipt =
      iggy3d::processProductCreativeInputActions(inactiveRequest);

  iggy3d::ProductAppWindowState creative = creativeWindow();
  iggy3d::ProductCreativeInputActionsRequest missingFacadeRequest;
  missingFacadeRequest.window = &creative;
  missingFacadeRequest.actions = &inactiveActions;
  const iggy3d::ProductCreativeInputFrameReceipt missingFacadeReceipt =
      iggy3d::processProductCreativeInputActions(missingFacadeRequest);

  cr::Facade facade;
  facade.reset();
  iggy3d::ProductCreativeInputActionsRequest nullActionsRequest;
  nullActionsRequest.window = &creative;
  nullActionsRequest.facade = &facade;
  const iggy3d::ProductCreativeInputFrameReceipt nullActionsReceipt =
      iggy3d::processProductCreativeInputActions(nullActionsRequest);

  return expect(nullWindowReceipt.status ==
                    "product_creative_input_window_missing",
                "batch null window") &&
         expect(inactiveReceipt.status == "product_creative_input_inactive",
                "batch inactive status") &&
         expect(inactiveFacade.toolState().activeTool == cr::Tool::Measure,
                "batch inactive facade unchanged") &&
         expect(missingFacadeReceipt.status ==
                    "product_creative_input_facade_missing",
                "batch missing facade") &&
         expect(nullActionsReceipt.requested, "batch null actions requested") &&
         expect(nullActionsReceipt.active, "batch null actions active") &&
         expect(nullActionsReceipt.facadeAvailable,
                "batch null actions facade") &&
         expect(!nullActionsReceipt.accepted, "batch null actions accepted") &&
         expect(!nullActionsReceipt.changed, "batch null actions changed") &&
         expect(nullActionsReceipt.status == "product_creative_input_noop",
                "batch null actions noop");
}

bool batchHeldToolKeyDoesNotRedispatch() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();
  iggy3d::KeyboardInputState keyboard;
  iggy3d::KeyboardCreativeToolInputSample sample;
  sample.moveToolDown = true;

  iggy3d::ProductCreativeInputActionsRequest pressedRequest;
  pressedRequest.window = &window;
  pressedRequest.facade = &facade;
  pressedRequest.toolKeys =
      iggy3d::recordKeyboardCreativeToolKeys(keyboard, sample);
  const iggy3d::ProductCreativeInputFrameReceipt pressedReceipt =
      iggy3d::processProductCreativeInputActions(pressedRequest);

  iggy3d::ProductCreativeInputActionsRequest heldRequest;
  heldRequest.window = &window;
  heldRequest.facade = &facade;
  heldRequest.toolKeys =
      iggy3d::recordKeyboardCreativeToolKeys(keyboard, sample);
  const iggy3d::ProductCreativeInputFrameReceipt heldReceipt =
      iggy3d::processProductCreativeInputActions(heldRequest);

  return expect(pressedReceipt.actionHandled, "pressed key handled") &&
         expect(pressedReceipt.toolChanged, "pressed key changed tool") &&
         expect(pressedReceipt.activeToolBefore == cr::Tool::Select,
                "pressed before select") &&
         expect(pressedReceipt.activeToolAfter == cr::Tool::Move,
                "pressed after move") &&
         expect(heldReceipt.status == "product_creative_input_noop",
                "held key noop") &&
         expect(!heldReceipt.toolChanged, "held key no toolChanged") &&
         expect(facade.toolState().activeTool == cr::Tool::Move,
                "held key facade still move");
}

bool batchProcessesMultiplePressedToolKeysInKeyOrder() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();

  iggy3d::KeyboardCreativeToolKeyPresses presses;
  presses.movePressed = true;
  presses.measurePressed = true;
  iggy3d::ProductCreativeInputActionsRequest request;
  request.window = &window;
  request.facade = &facade;
  request.toolKeys = presses;
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputActions(request);

  return expect(receipt.actionHandled, "multi key handled") &&
         expect(receipt.toolChanged, "multi key changed") &&
         expect(receipt.activeToolBefore == cr::Tool::Select,
                "multi before select") &&
         expect(receipt.activeToolAfter == cr::Tool::Measure,
                "multi after measure") &&
         expect(facade.toolState().activeTool == cr::Tool::Measure,
                "multi facade measure");
}

bool batchPointerRunsAfterActionsWithUpdatedTool() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();

  iggy3d::KeyboardCreativeToolKeyPresses presses;
  presses.measurePressed = true;
  iggy3d::ProductCreativeInputActionsRequest request;
  request.window = &window;
  request.facade = &facade;
  request.toolKeys = presses;
  request.click = clickAt(22.0F, 44.0F);
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputActions(request);

  return expect(receipt.actionHandled, "action before pointer handled") &&
         expect(receipt.pointerDispatched, "pointer dispatched after action") &&
         expect(receipt.inputKind == cr::CreativeToolInputKind::PointerPress,
                "pointer wins final input kind") &&
         expect(receipt.emittedIntentCount == 1U,
                "pointer wins final emitted count") &&
         expect(receipt.activeToolAfter == cr::Tool::Measure,
                "pointer uses measure tool") &&
         expect(facade.measurementState().active,
                "batch measurement active") &&
         expect(facade.measurementState().startPoint.x == 22.0,
                "batch measurement x") &&
         expect(facade.measurementState().startPoint.y == 44.0,
                "batch measurement y");
}

bool batchPointerUsesPickedTargetAfterToolAction() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();

  cr::TargetRef target;
  target.value = 404;
  iggy3d::ProductCreativeInputActionsRequest request;
  request.window = &window;
  request.facade = &facade;
  request.toolKeys = moveKeyPressed();
  request.click = clickAt(62.0F, 64.0F);
  request.pointerTarget = target;
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputActions(request);

  return expect(receipt.actionHandled, "batch target key handled") &&
         expect(receipt.pointerDispatched, "batch target pointer dispatched") &&
         expect(receipt.activeToolAfter == cr::Tool::Move,
                "batch target tool move") &&
         expect(facade.selectionState().selectedTarget.value == 404U,
                "batch target selected");
}

bool invalidPointerTargetKeepsSelectionInvalid() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.facade = &facade;
  request.click = clickAt(78.0F, 79.0F);
  const iggy3d::ProductCreativeInputFrameReceipt receipt =
      iggy3d::processProductCreativeInputFrame(request);

  return expect(receipt.pointerDispatched, "invalid target pointer") &&
         expect(facade.selectionState().selectedTarget.value ==
                    cr::kInvalidId,
                "invalid target selection invalid");
}

bool batchCancelOnlyWhenPressed() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::Facade facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));

  iggy3d::ProductCreativeInputFrameRequest beginRequest;
  beginRequest.window = &window;
  beginRequest.facade = &facade;
  beginRequest.click = clickAt(5.0F, 6.0F);
  static_cast<void>(iggy3d::processProductCreativeInputFrame(beginRequest));

  iggy3d::ActionState heldCancelActions;
  recordTestAction(heldCancelActions,
                   iggy3d::InputAction::EditorCancelPreview,
                   false);
  iggy3d::ProductCreativeInputActionsRequest heldRequest;
  heldRequest.window = &window;
  heldRequest.facade = &facade;
  heldRequest.actions = &heldCancelActions;
  const iggy3d::ProductCreativeInputFrameReceipt heldReceipt =
      iggy3d::processProductCreativeInputActions(heldRequest);
  const bool activeAfterHeldCancel = facade.measurementState().active;

  iggy3d::ActionState pressedCancelActions;
  recordTestAction(pressedCancelActions,
                   iggy3d::InputAction::EditorCancelPreview,
                   true);
  iggy3d::ProductCreativeInputActionsRequest pressedRequest;
  pressedRequest.window = &window;
  pressedRequest.facade = &facade;
  pressedRequest.actions = &pressedCancelActions;
  const iggy3d::ProductCreativeInputFrameReceipt pressedReceipt =
      iggy3d::processProductCreativeInputActions(pressedRequest);

  return expect(heldReceipt.status == "product_creative_input_noop",
                "held cancel noop") &&
         expect(activeAfterHeldCancel, "held cancel leaves measurement active") &&
         expect(facade.measurementState().active == false,
                "pressed cancel inactive") &&
         expect(pressedReceipt.cancelDispatched, "pressed cancel dispatched") &&
         expect(pressedReceipt.inputKind == cr::CreativeToolInputKind::Cancel,
                "pressed cancel kind") &&
         expect(!facade.measurementState().hasMeasurement,
                "pressed cancel clears measurement");
}

}  // namespace

int main() {
  bool ok = true;
  ok &= activeRuleUsesCreativeDocumentIdentity();
  ok &= nullWindowReturnsWindowMissing();
  ok &= inactiveWindowNoopsAndDoesNotMutateFacade();
  ok &= activeCreativeNullFacadeReportsMissing();
  ok &= toolKeysMapDirectlyToTools();
  ok &= heldToolKeyRecordsNoPressEdge();
  ok &= clickPacketMapsMouseClick();
  ok &= clickPacketPreservesPickedTarget();
  ok &= toolKeyChangesFacadeToolWithoutDocumentMutation();
  ok &= repeatedToolKeyDoesNotSpamToolChanged();
  ok &= selectClickWithPickedTargetUpdatesSelection();
  ok &= moveClickWithPickedTargetSelects();
  ok &= navigateClickIsInert();
  ok &= clickWithMeasureActiveBeginsMeasurement();
  ok &= measureClickWithPickedTargetStoresMeasurementTarget();
  ok &= editorCancelPreviewCancelsActiveMeasurement();
  ok &= noApplicableInputReturnsNoop();
  ok &= batchNoopsForClosedInputs();
  ok &= batchHeldToolKeyDoesNotRedispatch();
  ok &= batchProcessesMultiplePressedToolKeysInKeyOrder();
  ok &= batchPointerRunsAfterActionsWithUpdatedTool();
  ok &= batchPointerUsesPickedTargetAfterToolAction();
  ok &= invalidPointerTargetKeepsSelectionInvalid();
  ok &= batchCancelOnlyWhenPressed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
