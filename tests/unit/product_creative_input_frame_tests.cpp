#include "app/iggy3d/creative/bridge/InputFrame.hpp"

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  const std::uint64_t objectCountBefore = facade.document().objectCount();

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.creative = &app;
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

bool movePacketMapsPointer() {
  cr::TargetRef target;
  target.value = 55;
  const cr::CreativeToolInputPacket packet =
      iggy3d::productCreativePointerMovePacket(12.5F, 34.25F, target);

  return expect(packet.kind == cr::CreativeToolInputKind::PointerMove,
                "move packet kind") &&
         expect(packet.pointer.button == cr::CreativeToolPointerButton::Primary,
                "move packet primary") &&
         expect(packet.pointer.x == 12.5, "move packet x") &&
         expect(packet.pointer.y == 34.25, "move packet y") &&
         expect(packet.pointer.target.value == 55U, "move packet target");
}

bool releasePacketMapsPointer() {
  const cr::CreativeToolInputPacket packet =
      iggy3d::productCreativePointerReleasePacket(60.0F, 70.0F);

  return expect(packet.kind == cr::CreativeToolInputKind::PointerRelease,
                "release packet kind") &&
         expect(packet.pointer.button == cr::CreativeToolPointerButton::Primary,
                "release packet primary") &&
         expect(packet.pointer.x == 60.0, "release packet x") &&
         expect(packet.pointer.y == 70.0, "release packet y") &&
         expect(packet.pointer.target.value == cr::kInvalidId,
                "release packet target invalid");
}

bool pointerLifecycleWalksPressMoveRelease() {
  iggy3d::ProductCreativePointerLifecycleState state;

  // Frame 0: button up -> nothing.
  const iggy3d::ProductCreativePointerLifecycleEvent up0 =
      iggy3d::resolveProductCreativePointerLifecycle(state, {false, 5.0F, 6.0F});
  // Frame 1: button-down edge -> Press.
  const iggy3d::ProductCreativePointerLifecycleEvent press =
      iggy3d::resolveProductCreativePointerLifecycle(state, {true, 5.0F, 6.0F});
  // Frame 2: held, position unchanged -> None (no zero-delta spam).
  const iggy3d::ProductCreativePointerLifecycleEvent held =
      iggy3d::resolveProductCreativePointerLifecycle(state, {true, 5.0F, 6.0F});
  // Frame 3: held, moved -> Move at the new position.
  const iggy3d::ProductCreativePointerLifecycleEvent move =
      iggy3d::resolveProductCreativePointerLifecycle(state, {true, 9.0F, 8.0F});
  // Frame 4: button-up edge -> Release at the current position.
  const iggy3d::ProductCreativePointerLifecycleEvent release =
      iggy3d::resolveProductCreativePointerLifecycle(state, {false, 9.0F, 8.0F});
  // Frame 5: up again -> nothing, no phantom release.
  const iggy3d::ProductCreativePointerLifecycleEvent up5 =
      iggy3d::resolveProductCreativePointerLifecycle(state, {false, 9.0F, 8.0F});

  return expect(up0.phase == iggy3d::ProductCreativePointerLifecyclePhase::None,
                "lifecycle up0 none") &&
         expect(press.phase ==
                    iggy3d::ProductCreativePointerLifecyclePhase::Press,
                "lifecycle press") &&
         expect(held.phase == iggy3d::ProductCreativePointerLifecyclePhase::None,
                "lifecycle held unmoved none") &&
         expect(move.phase == iggy3d::ProductCreativePointerLifecyclePhase::Move,
                "lifecycle move") &&
         expect(move.x == 9.0F && move.y == 8.0F, "lifecycle move coords") &&
         expect(release.phase ==
                    iggy3d::ProductCreativePointerLifecyclePhase::Release,
                "lifecycle release") &&
         expect(release.x == 9.0F && release.y == 8.0F,
                "lifecycle release coords") &&
         expect(up5.phase == iggy3d::ProductCreativePointerLifecyclePhase::None,
                "lifecycle up5 no phantom release");
}

bool pointerLifecycleResetDropsHeldFlag() {
  iggy3d::ProductCreativePointerLifecycleState state;
  // Enter a held gesture.
  static_cast<void>(
      iggy3d::resolveProductCreativePointerLifecycle(state, {true, 1.0F, 2.0F}));
  // Reset (tool switch / mode exit) drops the held flag.
  iggy3d::resetProductCreativePointerLifecycle(state);
  // Button now up: without the reset this would have been a phantom Release.
  const iggy3d::ProductCreativePointerLifecycleEvent afterReset =
      iggy3d::resolveProductCreativePointerLifecycle(state, {false, 1.0F, 2.0F});

  return expect(!state.primaryButtonHeld, "reset clears held flag") &&
         expect(afterReset.phase ==
                    iggy3d::ProductCreativePointerLifecyclePhase::None,
                "reset prevents phantom release");
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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  const std::uint64_t objectCountBefore = facade.document().objectCount();

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.creative = &app;
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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  iggy3d::ProductCreativeInputActionsRequest request;
  request.window = &window;
  request.creative = &app;
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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  cr::TargetRef target;
  target.value = 101;

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.creative = &app;
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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  cr::TargetRef target;
  target.value = 202;

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.creative = &app;
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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Navigate));
  cr::TargetRef target;
  target.value = 303;

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.creative = &app;
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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.creative = &app;
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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));
  cr::TargetRef target;
  target.value = 303;

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.creative = &app;
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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));

  iggy3d::ProductCreativeInputFrameRequest beginRequest;
  beginRequest.window = &window;
  beginRequest.creative = &app;
  beginRequest.click = clickAt(10.0F, 10.0F);
  static_cast<void>(iggy3d::processProductCreativeInputFrame(beginRequest));

  iggy3d::ProductCreativeInputFrameRequest cancelRequest;
  cancelRequest.window = &window;
  cancelRequest.creative = &app;
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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.creative = &app;
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
  cr::CreativeAppState inactiveApp;
  cr::Facade& inactiveFacade = inactiveApp.facade;
  inactiveFacade.reset();
  static_cast<void>(inactiveFacade.setActiveTool(cr::Tool::Measure));
  iggy3d::ActionState inactiveActions;
  recordTestAction(inactiveActions,
                   iggy3d::InputAction::EditorCancelPreview,
                   true);
  iggy3d::ProductCreativeInputActionsRequest inactiveRequest;
  inactiveRequest.window = &inactiveWindow;
  inactiveRequest.creative = &inactiveApp;
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

  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  iggy3d::ProductCreativeInputActionsRequest nullActionsRequest;
  nullActionsRequest.window = &creative;
  nullActionsRequest.creative = &app;
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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  iggy3d::KeyboardInputState keyboard;
  iggy3d::KeyboardCreativeToolInputSample sample;
  sample.moveToolDown = true;

  iggy3d::ProductCreativeInputActionsRequest pressedRequest;
  pressedRequest.window = &window;
  pressedRequest.creative = &app;
  pressedRequest.toolKeys =
      iggy3d::recordKeyboardCreativeToolKeys(keyboard, sample);
  const iggy3d::ProductCreativeInputFrameReceipt pressedReceipt =
      iggy3d::processProductCreativeInputActions(pressedRequest);

  iggy3d::ProductCreativeInputActionsRequest heldRequest;
  heldRequest.window = &window;
  heldRequest.creative = &app;
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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  iggy3d::KeyboardCreativeToolKeyPresses presses;
  presses.movePressed = true;
  presses.measurePressed = true;
  iggy3d::ProductCreativeInputActionsRequest request;
  request.window = &window;
  request.creative = &app;
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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  iggy3d::KeyboardCreativeToolKeyPresses presses;
  presses.measurePressed = true;
  iggy3d::ProductCreativeInputActionsRequest request;
  request.window = &window;
  request.creative = &app;
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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  cr::TargetRef target;
  target.value = 404;
  iggy3d::ProductCreativeInputActionsRequest request;
  request.window = &window;
  request.creative = &app;
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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();

  iggy3d::ProductCreativeInputFrameRequest request;
  request.window = &window;
  request.creative = &app;
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
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));

  iggy3d::ProductCreativeInputFrameRequest beginRequest;
  beginRequest.window = &window;
  beginRequest.creative = &app;
  beginRequest.click = clickAt(5.0F, 6.0F);
  static_cast<void>(iggy3d::processProductCreativeInputFrame(beginRequest));

  iggy3d::ActionState heldCancelActions;
  recordTestAction(heldCancelActions,
                   iggy3d::InputAction::EditorCancelPreview,
                   false);
  iggy3d::ProductCreativeInputActionsRequest heldRequest;
  heldRequest.window = &window;
  heldRequest.creative = &app;
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
  pressedRequest.creative = &app;
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

iggy3d::ProductCreativePointerLifecycleEvent lifecycleMove(float x, float y) {
  return {iggy3d::ProductCreativePointerLifecyclePhase::Move, x, y};
}

iggy3d::ProductCreativePointerLifecycleEvent lifecycleRelease(float x, float y) {
  return {iggy3d::ProductCreativePointerLifecyclePhase::Release, x, y};
}

// FLAGSHIP: a full Measure gesture through the WINDOW/frame entry finally ENDS.
// Before TV1-F only PointerPress was synthesized, so Measure could Begin but the
// EndMeasurement intent (PointerRelease) was never reachable from the window.
bool measureGestureBeginsUpdatesAndEndsThroughFrameEntry() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));

  // Press -> Begin (the existing click path).
  iggy3d::ProductCreativeInputActionsRequest beginRequest;
  beginRequest.window = &window;
  beginRequest.creative = &app;
  beginRequest.click = clickAt(10.0F, 10.0F);
  const iggy3d::ProductCreativeInputFrameReceipt beginReceipt =
      iggy3d::processProductCreativeInputActions(beginRequest);
  const bool beganActive = facade.measurementState().active;

  // Move (held) -> Update: the measurement's current point tracks the pointer.
  iggy3d::ProductCreativeInputActionsRequest moveRequest;
  moveRequest.window = &window;
  moveRequest.creative = &app;
  moveRequest.pointerLifecycle = lifecycleMove(40.0F, 55.0F);
  const iggy3d::ProductCreativeInputFrameReceipt moveReceipt =
      iggy3d::processProductCreativeInputActions(moveRequest);
  const bool activeDuringMove = facade.measurementState().active;
  const double currentXAfterMove = facade.measurementState().currentPoint.x;
  const double currentYAfterMove = facade.measurementState().currentPoint.y;

  // Release -> End: the measurement finally ends (active clears, result stays).
  iggy3d::ProductCreativeInputActionsRequest releaseRequest;
  releaseRequest.window = &window;
  releaseRequest.creative = &app;
  releaseRequest.pointerLifecycle = lifecycleRelease(40.0F, 55.0F);
  const iggy3d::ProductCreativeInputFrameReceipt releaseReceipt =
      iggy3d::processProductCreativeInputActions(releaseRequest);

  return expect(beginReceipt.pointerDispatched, "flagship begin dispatched") &&
         expect(beganActive, "flagship measurement began active") &&
         expect(moveReceipt.pointerMoveDispatched, "flagship move dispatched") &&
         expect(moveReceipt.inputKind ==
                    cr::CreativeToolInputKind::PointerMove,
                "flagship move input kind") &&
         expect(moveReceipt.emittedIntentCount == 1U,
                "flagship move emitted update intent") &&
         expect(activeDuringMove, "flagship still measuring during move") &&
         expect(currentXAfterMove == 40.0, "flagship move updates current x") &&
         expect(currentYAfterMove == 55.0, "flagship move updates current y") &&
         expect(releaseReceipt.pointerReleaseDispatched,
                "flagship release dispatched") &&
         expect(releaseReceipt.inputKind ==
                    cr::CreativeToolInputKind::PointerRelease,
                "flagship release input kind") &&
         expect(releaseReceipt.emittedIntentCount == 1U,
                "flagship release emitted end intent") &&
         expect(!facade.measurementState().active,
                "flagship measurement ENDED after release") &&
         expect(facade.measurementState().hasMeasurement,
                "flagship measurement result retained after end");
}

// Esc mid-measure cancels the gesture; a subsequent Release is a harmless no-op
// (no measurement to end) and does NOT resurrect the cancelled measurement.
bool escMidMeasureCancelsThenReleaseIsInert() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));

  iggy3d::ProductCreativeInputActionsRequest beginRequest;
  beginRequest.window = &window;
  beginRequest.creative = &app;
  beginRequest.click = clickAt(12.0F, 12.0F);
  static_cast<void>(iggy3d::processProductCreativeInputActions(beginRequest));

  iggy3d::ActionState cancelActions;
  recordTestAction(cancelActions, iggy3d::InputAction::EditorCancelPreview,
                   true);
  iggy3d::ProductCreativeInputActionsRequest cancelRequest;
  cancelRequest.window = &window;
  cancelRequest.creative = &app;
  cancelRequest.actions = &cancelActions;
  const iggy3d::ProductCreativeInputFrameReceipt cancelReceipt =
      iggy3d::processProductCreativeInputActions(cancelRequest);
  const bool activeAfterCancel = facade.measurementState().active;
  const bool hasMeasurementAfterCancel =
      facade.measurementState().hasMeasurement;

  iggy3d::ProductCreativeInputActionsRequest releaseRequest;
  releaseRequest.window = &window;
  releaseRequest.creative = &app;
  releaseRequest.pointerLifecycle = lifecycleRelease(12.0F, 12.0F);
  const iggy3d::ProductCreativeInputFrameReceipt releaseReceipt =
      iggy3d::processProductCreativeInputActions(releaseRequest);

  return expect(cancelReceipt.cancelDispatched, "esc cancel dispatched") &&
         expect(!activeAfterCancel, "esc cancel clears active") &&
         expect(!hasMeasurementAfterCancel, "esc cancel clears measurement") &&
         expect(releaseReceipt.pointerReleaseDispatched,
                "post-cancel release dispatched") &&
         expect(releaseReceipt.emittedIntentCount == 0U,
                "post-cancel release emits no end intent") &&
         expect(!facade.measurementState().active,
                "post-cancel release leaves no measurement") &&
         expect(!facade.measurementState().hasMeasurement,
                "post-cancel release does not resurrect measurement");
}

// A Move dispatched to the Select tool is a harmless preview no-op (Move-ready
// plumbing per TV1-F#5: Press/Move/Release all reach the facade, but Select's
// move/release do not mutate the document or selection).
bool selectMoveAndReleaseAreHarmlessNoOps() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();  // Select is the default tool.

  iggy3d::ProductCreativeInputActionsRequest moveRequest;
  moveRequest.window = &window;
  moveRequest.creative = &app;
  moveRequest.pointerLifecycle = lifecycleMove(30.0F, 40.0F);
  const iggy3d::ProductCreativeInputFrameReceipt moveReceipt =
      iggy3d::processProductCreativeInputActions(moveRequest);

  iggy3d::ProductCreativeInputActionsRequest releaseRequest;
  releaseRequest.window = &window;
  releaseRequest.creative = &app;
  releaseRequest.pointerLifecycle = lifecycleRelease(30.0F, 40.0F);
  const iggy3d::ProductCreativeInputFrameReceipt releaseReceipt =
      iggy3d::processProductCreativeInputActions(releaseRequest);

  return expect(moveReceipt.pointerMoveDispatched, "select move dispatched") &&
         expect(moveReceipt.accepted, "select move accepted") &&
         expect(releaseReceipt.pointerReleaseDispatched,
                "select release dispatched") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "select move/release leaves selection untouched") &&
         expect(facade.document().objectCount() == 0U,
                "select move/release leaves document untouched");
}

// Navigate stays fully inert across the whole lifecycle (TD-8: camera in TV1-H).
bool navigateLifecycleStaysInert() {
  iggy3d::ProductAppWindowState window = creativeWindow();
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  facade.reset();
  static_cast<void>(facade.setActiveTool(cr::Tool::Navigate));

  iggy3d::ProductCreativeInputActionsRequest moveRequest;
  moveRequest.window = &window;
  moveRequest.creative = &app;
  moveRequest.pointerLifecycle = lifecycleMove(30.0F, 40.0F);
  const iggy3d::ProductCreativeInputFrameReceipt moveReceipt =
      iggy3d::processProductCreativeInputActions(moveRequest);

  iggy3d::ProductCreativeInputActionsRequest releaseRequest;
  releaseRequest.window = &window;
  releaseRequest.creative = &app;
  releaseRequest.pointerLifecycle = lifecycleRelease(30.0F, 40.0F);
  const iggy3d::ProductCreativeInputFrameReceipt releaseReceipt =
      iggy3d::processProductCreativeInputActions(releaseRequest);

  return expect(moveReceipt.pointerMoveDispatched, "navigate move dispatched") &&
         expect(moveReceipt.emittedIntentCount == 0U,
                "navigate move emits no intent") &&
         expect(releaseReceipt.pointerReleaseDispatched,
                "navigate release dispatched") &&
         expect(releaseReceipt.emittedIntentCount == 0U,
                "navigate release emits no intent") &&
         expect(!facade.measurementState().active,
                "navigate lifecycle no measurement") &&
         expect(facade.document().objectCount() == 0U,
                "navigate lifecycle document untouched");
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
  ok &= movePacketMapsPointer();
  ok &= releasePacketMapsPointer();
  ok &= pointerLifecycleWalksPressMoveRelease();
  ok &= pointerLifecycleResetDropsHeldFlag();
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
  ok &= measureGestureBeginsUpdatesAndEndsThroughFrameEntry();
  ok &= escMidMeasureCancelsThenReleaseIsInert();
  ok &= selectMoveAndReleaseAreHarmlessNoOps();
  ok &= navigateLifecycleStaysInert();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
