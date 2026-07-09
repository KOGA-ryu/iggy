#include "app/iggy3d/window/InputFrame.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/view/OpeningMenuHitTest.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/iggy3d/view/CameraController.hpp"
#include "app/iggy3d/input/ControllerActionRouting.hpp"
#include "app/iggy3d/gameplay/Controller.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/input/InteractionModeState.hpp"
#include "app/iggy3d/window/MouseCapturePolicy.hpp"
#include "app/iggy3d/window/InputFrameStages.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/bridge/InputFrame.hpp"
#include "app/iggy3d/creative/bridge/UiCommandFrame.hpp"
#include "app/iggy3d/creative/bridge/UiInputFrame.hpp"
#include "app/iggy3d/creative/bridge/ViewportPickFrame.hpp"
#include "app/iggy3d/view/CreativeFlyAnchorStore.hpp"
#include "app/iggy3d/room_editor/ActionController.hpp"
#include "app/iggy3d/room_editor/Preview.hpp"
#include "app/iggy3d/automation/Automation.hpp"
#include "app/iggy3d/automation/AutomationRoomEditing.hpp"
#include "app/iggy3d/creative/BakedActiveRoomRefresh.hpp"
#include "app/iggy3d/menu/ActionHandlers.hpp"
#include "app/iggy3d/menu/DrawList.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/menu/InputRouter.hpp"
#include "app/iggy3d/menu/Transitions.hpp"
#include "app/iggy3d/save/SaveSlotOperations.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputRouter.hpp"
#include "app/platform/SdlWindow.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

namespace iggy3d {

InputAction movementTuningHeldAdjustmentAction(bool leftDown, bool rightDown);
void recordProductWindowControllerActions(
    const FrontendState& frontend,
    ProductAppWindowState& window,
    ProductControllerActionRoutingState& controllerAction,
    const GamepadControllerActionSample& sample,
    bool controllerModeChordRequested,
    ActionState& actions,
    const creative::CreativeAppState* creativeApp);

namespace {

struct ProductWindowFunctionKeyBinding {
  bool SdlWindowEventState::* pressed = nullptr;
  InputAction action = InputAction::None;
  bool KeyboardInputState::* wasDown = nullptr;
};

using ProductWindowTopLevelToggleHandler = ProductWindowTopLevelToggleResult (*)(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings* settings,
    bool* closeRequested,
    creative::CreativeAppState* creativeApp);

struct ProductWindowTopLevelToggleRow {
  InputAction action = InputAction::None;
  ProductWindowTopLevelToggleHandler handler = nullptr;
  const char* eligibility = "";
};

ProductWindowTopLevelToggleResult dispatchProductWindowSystemToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings* settings,
    bool* closeRequested,
    creative::CreativeAppState* creativeApp);
static constexpr std::array kProductWindowFunctionKeyBindings{
    ProductWindowFunctionKeyBinding{&SdlWindowEventState::f3Pressed,
                                    InputAction::DevDebugOverlay,
                                    &KeyboardInputState::debugOverlayWasDown},
    ProductWindowFunctionKeyBinding{
        &SdlWindowEventState::f4Pressed,
        InputAction::MovementTuningToggle,
        &KeyboardInputState::movementTuningToggleWasDown},
    ProductWindowFunctionKeyBinding{&SdlWindowEventState::f1Pressed,
                                    InputAction::DevToggle,
                                    &KeyboardInputState::devToggleWasDown},
    ProductWindowFunctionKeyBinding{
        &SdlWindowEventState::f2Pressed,
        InputAction::DevCollisionOverlay,
        &KeyboardInputState::devCollisionOverlayWasDown},
    ProductWindowFunctionKeyBinding{&SdlWindowEventState::mPressed,
                                    InputAction::MapMakerToggle,
                                    &KeyboardInputState::mapMakerToggleWasDown},
};

static constexpr std::array kProductWindowTopLevelToggleRows{
    ProductWindowTopLevelToggleRow{InputAction::DevDebugOverlay,
                                   dispatchProductWindowSystemToggleAction,
                                   "gameplay_owned"},
    ProductWindowTopLevelToggleRow{InputAction::MovementTuningToggle,
                                   dispatchProductWindowMovementTuningToggleAction,
                                   "gameplay_owned"},
    ProductWindowTopLevelToggleRow{InputAction::DevToggle,
                                   dispatchProductWindowSystemToggleAction,
                                   "screen_aware_overlay"},
    ProductWindowTopLevelToggleRow{InputAction::DevCollisionOverlay,
                                   dispatchProductWindowSystemToggleAction,
                                   "screen_aware_overlay"},
    ProductWindowTopLevelToggleRow{InputAction::MapMakerToggle,
                                   dispatchProductWindowMapMakerToggleAction,
                                   "gameplay_owned"},
};

bool productWindowFocused(const SdlWindow* sdlWindow) {
  return sdlWindow == nullptr || sdlWindow->eventState().focused;
}

void recordProductMouseCaptureResult(ProductAppWindowState& window,
                                     const ProductMouseCapturePolicy& policy,
                                     const SdlMouseCaptureResult* platform) {
  window.inputDevice.mouseCapture.requested = policy.requested;
  window.inputDevice.mouseCapture.active = false;
  window.inputDevice.mouseCapture.status = policy.status;
  window.inputDevice.mouseCapture.reasonCode = policy.reasonCode;
  window.inputDevice.mouseCapture.mode = policy.mode;
  window.inputDevice.mouseCapture.inputOwner = policy.inputOwner;
  // branch-gate: BG-1076
  if (platform != nullptr) {
    window.inputDevice.mouseCapture.requested = platform->requested;
    window.inputDevice.mouseCapture.active = platform->active;
    window.inputDevice.mouseCapture.status = platform->status;
    window.inputDevice.mouseCapture.reasonCode = platform->reasonCode;
  }
}

void updateProductWindowMouseCapture(const FrontendState& frontend,
                                     ProductAppWindowState& window,
                                     SdlWindow* sdlWindow,
                                     const creative::CreativeAppState*
                                         creativeApp) {
  const ProductActiveSurfaceFrame surface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(frontend, window));
  const ProductMouseCapturePolicy policy = buildProductMouseCapturePolicy({
      window.gameplay.gameplayActive,
      window.inputDevice.interactionMode,
      surface.inputOwner,
      surface.gameplayInputSuppressed,
      productWindowFocused(sdlWindow),
      sdlWindow != nullptr,
      productCreativeDocumentEditorActiveForSource(window, creativeApp),
      window.creativeAuthoring.creativeNavigateActive,
  });
  // branch-gate: BG-1076
  if (sdlWindow == nullptr) {
    recordProductMouseCaptureResult(window, policy, nullptr);
    return;
  }
  const SdlMouseCaptureResult platform =
      sdlWindow->setRelativeMouseMode(policy.requested);
  recordProductMouseCaptureResult(window, policy, &platform);
}

ProductWindowTopLevelToggleResult dispatchProductWindowSystemToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings* settings,
    bool* closeRequested,
    creative::CreativeAppState* creativeApp) {
  bool ignoredCloseRequested = false;
  bool& closeTarget =
      closeRequested == nullptr ? ignoredCloseRequested : *closeRequested;  // branch-gate: BG-1194
  const ProductMenuActionResult menuResult = applyProductSystemPauseMenuAction(
      action, {frontend, window, closeTarget, settings, creativeApp});
  return {menuResult.handled, menuResult.accepted, action};
}

}  // namespace

void initializeProductWindowInputFrameState(ProductWindowInputFrameState& state,
                                            ProductAppWindowState& window) {
  initializeGamepadMenuState(state.gamepad);
  window.inputDevice.gamepadAvailable = state.gamepad.gamepadAvailable;
  window.inputDevice.gamepadName = state.gamepad.gamepadName;
  // branch-gate: BG-1029
  window.inputDevice.gamepadMapping =
      state.gamepad.gamepadAvailable ? "sdl_gamepad" : "unavailable";
}

void shutdownProductWindowInputFrameState(ProductWindowInputFrameState& state,
                                          SdlWindow* sdlWindow,
                                          ProductAppWindowState* window) {
  // branch-gate: BG-1076
  if (sdlWindow != nullptr) {
    const SdlMouseCaptureResult platform =
        sdlWindow->setRelativeMouseMode(false);
    // branch-gate: BG-1076
    if (window != nullptr) {
      ProductMouseCapturePolicy policy;
      policy.reasonCode = "mouse_capture_shutdown";
      recordProductMouseCaptureResult(*window, policy, &platform);
    }
  }
  shutdownGamepadMenuState(state.gamepad);
}

ProductWindowTopLevelToggleResult dispatchProductWindowTopLevelToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings* settings,
    bool* closeRequested,
    creative::CreativeAppState* creativeApp) {
  for (const ProductWindowTopLevelToggleRow& row : kProductWindowTopLevelToggleRows) {
    // branch-gate: BG-1194
    if (row.action == action) {
      return row.handler(frontend, window, action, settings, closeRequested,
                         creativeApp);
    }
  }
  return {};
}

void processProductWindowInputFrame(ProductWindowInputFrameContext context) {
  ActionState actionState;
  ProductOpeningMenuInputContext menuContext{
      context.frontend, context.saves, context.options, context.settingsTab,
      context.activeSession, context.worldSetupDraft, context.window,
      context.closeRequested, context.settings, context.creativeApp};
  InputAction functionKeyAction = InputAction::None;
  // branch-gate: BG-1194
  if (context.sdlWindow != nullptr) {
    const SdlWindowEventState& eventState = context.sdlWindow->eventState();
    functionKeyAction = productWindowFunctionKeyAction(eventState);
    recordProductWindowFunctionKeyKeyboardState(context.inputFrame.keyboard,
                                                eventState);
  }
  InputAction keyboardMenuAction = functionKeyAction;
  // branch-gate: BG-1194
  if (keyboardMenuAction == InputAction::None) {
    keyboardMenuAction = pollKeyboardMenuAction(context.inputFrame.keyboard);
  }
  const ProductWindowTopLevelToggleResult topLevelToggle =
      dispatchProductWindowTopLevelToggleAction(context.frontend,
                                                context.window,
                                                keyboardMenuAction,
                                                &context.settings,
                                                &context.closeRequested,
                                                context.creativeApp);
  // branch-gate: BG-1194
  if (topLevelToggle.handled) {
    recordAction(actionState,
                 topLevelToggle.action,
                 true,
                 topLevelToggle.accepted,
                 false,
                 1.0F);
    keyboardMenuAction = InputAction::None;
  }
  routeProductWindowMenuInput(keyboardMenuAction, actionState, menuContext);
  // branch-gate: BG-1029
  if (context.frontend.childScreen == FrontendScreen::NewWorld) {
    const char paintGlyph = pollKeyboardAsciiRoomPaintGlyph(context.inputFrame.keyboard);
    // branch-gate: BG-1029
    if (paintGlyph != '\0') {
      selectDungeonDraftPaintGlyph(context.window, paintGlyph);
    }
  }

  const GamepadControllerActionSample gamepadControllerSample =
      pollGamepadControllerActionSample(context.inputFrame.gamepad);
  const ProductInteractionModeToggleResult modeToggle =
      applyProductInteractionModeFrameToggle({
          context.frontend,
          context.window,
          context.inputFrame.controllerModeChord,
          productControllerModeChordSampleFromGamepad(gamepadControllerSample),
      });
  const bool controllerModeChordRequested = modeToggle.toggleRequested;

  const InputAction gamepadAction = pollGamepadMenuAction(context.inputFrame.gamepad);
  // branch-gate: BG-1029
  if (gamepadAction != InputAction::None) {
    context.window.frontendShell.gamepadMenuSelectUsed = true;
    const ProductWindowTopLevelToggleResult gamepadToggle =
        dispatchProductWindowTopLevelToggleAction(context.frontend,
                                                  context.window,
                                                  gamepadAction,
                                                  &context.settings,
                                                  &context.closeRequested,
                                                  context.creativeApp);
    // branch-gate: BG-1194
    if (gamepadToggle.handled) {
      recordAction(actionState,
                   gamepadToggle.action,
                   true,
                   gamepadToggle.accepted,
                   false,
                   1.0F);
    } else {
      routeProductWindowMenuInput(gamepadAction, actionState, menuContext);
    }
  }

  const bool tuningLeftHeld =
      context.inputFrame.keyboard.leftWasDown || context.inputFrame.gamepad.leftWasDown;
  const bool tuningRightHeld =
      context.inputFrame.keyboard.rightWasDown || context.inputFrame.gamepad.rightWasDown;
  const InputAction heldTuningAction =
      movementTuningHeldAdjustmentAction(tuningLeftHeld, tuningRightHeld);
  const ProductMovementTuningInputResult heldTuningInput =
      applyProductWindowMovementTuningHeldInput(
          context.frontend,
          context.window,
          context.inputFrame.movementTuningRepeat,
          tuningLeftHeld,
          tuningRightHeld);
  // branch-gate: BG-1212
  if (heldTuningInput.handled) {
    recordAction(actionState,
                 heldTuningAction,
                 true,
                 heldTuningInput.accepted,
                 false,
                 1.0F);
  }

  const MouseClick click = resolveProductWindowInputMouseClick(
      context.clickOverride, context.inputFrame.mouse);
  bool higherPriorityMouseConsumed = false;
  const bool frontendMouseOwnsInput =
      click.clicked && frontendBlocksGameplayInput(context.frontend);
  // branch-gate: BG-1029. Skip the legacy opening/starter-menu hit band during
  // raw creative-document gameplay: its hardcoded row band (openingMenuActionAt)
  // is a raw-coordinate check that fires even when no starter rows are drawn, so
  // it steals clicks landing on creative overlay rows (e.g. a Create row that
  // reflows into the phantom band). But any real frontend menu over a creative
  // world (pause and its Settings/Load/Delete children) still routes its mouse
  // hits HERE, so keep running the block whenever the frontend owns the mouse
  // (frontendMouseOwnsInput) — openingMenuActionAt routes the correct rows then.
  if (click.clicked &&
      (!productCreativeDocumentEditorActiveForSource(context.window,
                                                    context.creativeApp) ||
       frontendMouseOwnsInput)) {
    const MouseClick menuClick =
          productWindowMenuClickForHitTest(click, context.sdlWindow);
    // Build the hit-test request through the SAME factory the frame draw path
    // uses (buildProductStarterUiDrawListRequest), so hit-testing consumes the
    // identical request that produced what was drawn.
    ProductUiDrawListRequest uiRequest =
        buildProductStarterUiDrawListRequest(
            context.frontend,
            context.saves,
            context.worldSetupDraft,
            context.settingsTab,
            {context.window.creativeAuthoring.worldSetup.dungeonDraftEditMode,
             context.window.creativeAuthoring.worldSetup.dungeonDraftModified,
             context.window.creativeAuthoring.worldSetup.dungeonDraftCursorRow,
             context.window.creativeAuthoring.worldSetup.dungeonDraftCursorColumn,
             context.window.creativeAuthoring.worldSetup.dungeonDraftSelectedGlyph,
             context.window.creativeAuthoring.worldSetup.dungeonDraftLastGlyph,
             context.window.saveSession.selectedProductSave.id,
             context.window.saveSession.saveDelete.candidateId});
    uiRequest.gameplayActive = context.window.gameplay.gameplayActive;
    uiRequest.saveRootWritable = !context.options.saveRoot.empty();
    uiRequest.developerToolsEnabled = true;
    uiRequest.activeRoomEditable = context.window.creativeAuthoring.roomEditing.ready;
    uiRequest.roomEditingReady = context.window.creativeAuthoring.roomEditing.ready;
    const OpeningMenuHitTestResult hit =
        openingMenuActionAt(uiRequest, menuClick.x, menuClick.y);
    // branch-gate: BG-1029
    if (hit.hit) {
      context.window.frontendShell.mouseMenuSelectUsed = true;
      dispatchProductOpeningMenuMouseHit(hit, click, actionState, menuContext);
      higherPriorityMouseConsumed = true;
    }
  }
  const ProductCreativeDocumentInputOrchestrationResult creativeInput =
      processProductCreativeDocumentInputOrchestration(
          ProductCreativeDocumentInputOrchestrationRequest{
              context,
              click,
              higherPriorityMouseConsumed,
              frontendMouseOwnsInput,
          });

  // branch-gate: BG-1029
  if (context.window.gameplay.gameplayActive && context.activeSession.has_value() &&
      !frontendBlocksGameplayInput(context.frontend)) {
    ActionState gameplayActions;
    if (!creativeInput.creativeDocumentInputHandled) {
      context.window.creativeAuthoring.creativeNavigateActive = false;
      if (context.window.creativeAuthoring.roomEditing.ready) {
        (void)processProductWindowEditorMousePickPreview({
            context.frontend,
            context.window,
            creativeInput.downstreamClick,
            productRoomEditorMousePickViewportConfig(context.window),
            productRoomEditorMousePickAnchor(&*context.activeSession),
            context.creativeApp,
        });
        pollKeyboardRoomEditorActions(context.inputFrame.keyboard,
                                      gameplayActions);
        recordProductWindowControllerActions(
            context.frontend, context.window, context.inputFrame.controllerAction,
            gamepadControllerSample, controllerModeChordRequested,
            gameplayActions, context.creativeApp);
      } else {
        // branch-gate: BG-1212
        if (!context.window.gameplay.gameplayMovement.tuningVisible) {
          pollKeyboardGameplayActions(context.inputFrame.keyboard,
                                      gameplayActions);
        }
        recordProductWindowControllerActions(
            context.frontend, context.window, context.inputFrame.controllerAction,
            gamepadControllerSample, controllerModeChordRequested,
            gameplayActions, context.creativeApp);
        pollMouseGameplayActions(context.inputFrame.mouse, gameplayActions);
      }
      ProductCreativeInputActionsRequest creativeRequest;
      creativeRequest.window = &context.window;
      creativeRequest.creative = context.creativeApp;
      creativeRequest.actions = &gameplayActions;
      creativeRequest.click = creativeInput.downstreamClick;
      creativeRequest.pointerTarget = creativeInput.pointerTarget;
      (void)processProductCreativeInputActions(creativeRequest);
      (void)applyProductWindowInputActions(context.frontend,
                                           context.window,
                                           &*context.activeSession,
                                           &context.settings, gameplayActions,
                                           "action_map",
                                           context.creativeApp);
    }
  }
  updateProductWindowMouseCapture(context.frontend,
                                  context.window,
                                  context.sdlWindow,
                                  context.creativeApp);
}

InputAction productWindowFunctionKeyAction(const SdlWindowEventState& eventState) {
  for (const ProductWindowFunctionKeyBinding& binding :
       kProductWindowFunctionKeyBindings) {
    // branch-gate: BG-1194
    if (eventState.*(binding.pressed)) {
      return binding.action;
    }
  }
  return InputAction::None;
}

void recordProductWindowFunctionKeyKeyboardState(
    KeyboardInputState& keyboard,
    const SdlWindowEventState& eventState) {
  for (const ProductWindowFunctionKeyBinding& binding :
       kProductWindowFunctionKeyBindings) {
    // branch-gate: BG-1194
    if (eventState.*(binding.pressed)) {
      keyboard.*(binding.wasDown) = true;
    }
  }
}

}  // namespace iggy3d
