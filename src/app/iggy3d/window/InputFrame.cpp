#include "app/iggy3d/window/InputFrame.hpp"

#include "app/iggy3d/view/OpeningMenuView.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/view/CameraController.hpp"
#include "app/iggy3d/input/ControllerActionRouting.hpp"
#include "app/iggy3d/gameplay/Controller.hpp"
#include "app/iggy3d/input/InteractionModeState.hpp"
#include "app/iggy3d/window/MouseCapturePolicy.hpp"
#include "app/iggy3d/room_editor/ActionController.hpp"
#include "app/iggy3d/room_editor/Preview.hpp"
#include "app/iggy3d/automation/Automation.hpp"
#include "app/iggy3d/automation/AutomationRoomEditing.hpp"
#include "app/iggy3d/menu/ActionHandlers.hpp"
#include "app/iggy3d/menu/InputRouter.hpp"
#include "app/iggy3d/Operations.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputRouter.hpp"
#include "app/platform/SdlWindow.hpp"

namespace iggy3d {
namespace {

MouseClick productWindowMenuClickForHitTest(MouseClick click,
                                            const SdlWindow* sdlWindow) {
  // branch-gate: BG-1123
  if (sdlWindow == nullptr) {
    return click;
  }
  const SdlWindowEventState& eventState = sdlWindow->eventState();
  return normalizeProductWindowMenuClick(click,
                                         eventState.windowWidth,
                                         eventState.windowHeight);
}

void applyProductWindowRoomEditorActions(ProductAppWindowState& window,
                                         const ActionState& actions) {
  for (const ActionStateEntry& entry : actions.entries) {
    // branch-gate: BG-1055
    if (isProductRoomEditorPreviewInputAction(entry.action)) {
      (void)applyProductRoomEditorPreviewInputAction(window, entry.action);
      continue;
    }

    ActionState singleAction;
    recordAction(singleAction, entry.action, entry.down, entry.pressed,
                 entry.released, entry.value);
    const ProductRoomEditorActionResult result =
        applyProductRoomEditorActions(window.roomEditing,
                                      window.roomEditorCursor,
                                      singleAction,
                                      ProductRoomAuthoringInputSource::Hotkey);
    recordProductRoomEditorActionResult(window, result);
  }
}

void recordProductWindowControllerActions(
    const FrontendState& frontend,
    ProductAppWindowState& window,
    ProductControllerActionRoutingState& controllerAction,
    const GamepadControllerActionSample& sample,
    bool controllerModeChordRequested,
    ActionState& actions) {
  const ProductInputSurface surface = productInputSurfaceFor(frontend, window);
  ProductControllerActionRoutingResult result =
      productControllerActionRoutingSkipped(
          surface, window.interactionMode, "controller_action_chord_consumed");
  if (!controllerModeChordRequested) {  // branch-gate: BG-1059
    result = recordProductControllerMappedActions({
        surface,
        window.interactionMode,
        sample,
        controllerAction,
        actions,
    });
  }
  recordProductControllerActionRoutingResult(window, result);
}

bool productWindowEditorMousePickSurfaceReady(
    const FrontendState& frontend,
    const ProductAppWindowState& window) {
  return frontend.screen == FrontendScreen::Gameplay && window.gameplayActive &&
         !frontendBlocksGameplayInput(frontend);
}

bool productWindowFocused(const SdlWindow* sdlWindow) {
  return sdlWindow == nullptr || sdlWindow->eventState().focused;
}

void recordProductMouseCaptureResult(ProductAppWindowState& window,
                                     const ProductMouseCapturePolicy& policy,
                                     const SdlMouseCaptureResult* platform) {
  window.mouseCaptureRequested = policy.requested;
  window.mouseCaptureActive = false;
  window.mouseCaptureStatus = policy.status;
  window.mouseCaptureReasonCode = policy.reasonCode;
  window.mouseCaptureMode = policy.mode;
  window.mouseCaptureInputOwner = policy.inputOwner;
  // branch-gate: BG-1076
  if (platform != nullptr) {
    window.mouseCaptureRequested = platform->requested;
    window.mouseCaptureActive = platform->active;
    window.mouseCaptureStatus = platform->status;
    window.mouseCaptureReasonCode = platform->reasonCode;
  }
}

void updateProductWindowMouseCapture(const FrontendState& frontend,
                                     ProductAppWindowState& window,
                                     SdlWindow* sdlWindow) {
  const MenuOwner owner = productInputOwnerFor(frontend, window);
  const ProductMouseCapturePolicy policy = buildProductMouseCapturePolicy({
      window.gameplayActive,
      window.interactionMode,
      owner,
      frontendBlocksGameplayInput(frontend),
      productWindowFocused(sdlWindow),
      sdlWindow != nullptr,
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

ProductControllerSampleInputResult applyProductWindowInputActions(
    ProductAppWindowState& window,
    Session* activeSession,
    const FrontendSettings* settings,
    const ActionState& gameplayActions,
    std::string_view inputSource) {
  ProductControllerSampleInputResult result;
  result.processed = true;
  result.status = "controller_sample_processed";
  result.reasonCode = result.status;

  ActionState acceptedGameplayActions;
  ActionState acceptedEditorActions;
  InputRoutingContext routingContext;
  routingContext.owners.editor = window.roomEditing.ready;
  routingContext.owners.gameplay = true;
  for (const ActionStateEntry& entry : gameplayActions.entries) {
    const InputRoutingResult routed = routeInputAction(routingContext, entry.action);
    window.inputOwner = routed.owner;
    window.lastInputAction = routed.action;
    window.lastInputAccepted = routed.accepted;
    window.gameplayInputSuppressed = routed.gameplaySuppressed;
    // branch-gate: BG-1061
    if (routed.accepted && routed.owner == MenuOwner::Editor &&
        inputActionGroup(entry.action) == InputActionGroup::Editor) {
      recordAction(acceptedEditorActions, entry.action, entry.down, entry.pressed,
                   entry.released, entry.value);
    // branch-gate: BG-1061
    } else if (routed.accepted && routed.owner == MenuOwner::Gameplay) {
      recordAction(acceptedGameplayActions, entry.action, entry.down, entry.pressed,
                   entry.released, entry.value);
    }
  }

  // branch-gate: BG-1061
  if (!acceptedEditorActions.entries.empty()) {
    applyProductWindowRoomEditorActions(window, acceptedEditorActions);
    result.actionApplied = true;
    result.actionAccepted = window.roomEditorLastOperationAccepted;
    return result;
  }

  // branch-gate: BG-1061
  if (settings != nullptr) {
    applyProductCameraActions(acceptedGameplayActions, window.viewport, *settings,
                              inputSource);
  }
  // branch-gate: BG-1061
  if (activeSession != nullptr) {
    const SpatialSurfaceSet* collisionSurfaces =
        productActiveRoomCollisionSurfaces(window.activeRoomCollision);
    applyProductGameplayActions(*activeSession, acceptedGameplayActions, window,
                                inputSource, collisionSurfaces);
    result.actionApplied = !acceptedGameplayActions.entries.empty();
    result.actionAccepted = window.gameplayCommandAccepted;
  }
  return result;
}

void routeProductWindowMenuInput(InputAction inputAction,
                                 ActionState& actionState,
                                 ProductOpeningMenuInputContext context) {
  // branch-gate: BG-1029
  if (inputAction == InputAction::MenuBack &&
      cancelProductRoomEditorPendingPreviewFromBack(context.window)) {
    recordAction(actionState,
                 InputAction::EditorCancelPreview,
                 true,
                 true,
                 false,
                 1.0F);
    return;
  }
  routeProductOpeningMenuInput(inputAction, actionState, context);
}

}  // namespace

void initializeProductWindowInputFrameState(ProductWindowInputFrameState& state,
                                            ProductAppWindowState& window) {
  initializeGamepadMenuState(state.gamepad);
  window.gamepadAvailable = state.gamepad.gamepadAvailable;
  window.gamepadName = state.gamepad.gamepadName;
  // branch-gate: BG-1029
  window.gamepadMapping = state.gamepad.gamepadAvailable ? "sdl_gamepad" : "unavailable";
}

MouseClick normalizeProductWindowMenuClick(MouseClick click,
                                           std::uint32_t windowWidth,
                                           std::uint32_t windowHeight,
                                           std::uint32_t virtualWidth,
                                           std::uint32_t virtualHeight) {
  // branch-gate: BG-1123
  if (!click.clicked || windowWidth == 0U || windowHeight == 0U ||
      virtualWidth == 0U || virtualHeight == 0U) {
    return click;
  }
  click.x = click.x * static_cast<float>(virtualWidth) /
            static_cast<float>(windowWidth);
  click.y = click.y * static_cast<float>(virtualHeight) /
            static_cast<float>(windowHeight);
  return click;
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

bool cancelProductRoomEditorPendingPreviewFromBack(ProductAppWindowState& window) {
  // branch-gate: BG-1055
  if (!window.roomEditing.ready || !window.roomEditorPreviewActive) {
    return false;
  }
  const ProductRoomEditorPreviewInputResult cancelled =
      applyProductRoomEditorPreviewInputAction(window,
                                              InputAction::EditorCancelPreview);
  window.inputOwner = MenuOwner::Editor;
  window.lastInputAction = InputAction::EditorCancelPreview;
  window.lastInputAccepted = cancelled.ok;
  window.gameplayInputSuppressed = true;
  return cancelled.ok;
}

ProductControllerSampleInputResult processProductControllerActionSample(
    ProductControllerSampleInputContext context,
    GamepadControllerActionSample sample) {
  const ProductInteractionModeToggleResult modeToggle =
      applyProductInteractionModeFrameToggle({
          context.frontend,
          context.window,
          context.controllerModeChord,
          productControllerModeChordSampleFromGamepad(sample),
      });
  const bool controllerModeChordRequested = modeToggle.toggleRequested;

  ActionState controllerActions;
  recordProductWindowControllerActions(context.frontend,
                                       context.window,
                                       context.controllerAction,
                                       sample,
                                       controllerModeChordRequested,
                                       controllerActions);

  // branch-gate: BG-1061
  if (!context.window.gameplayActive || context.activeSession == nullptr ||
      frontendBlocksGameplayInput(context.frontend)) {
    ProductControllerSampleInputResult result;
    result.processed = true;
    result.status = "controller_sample_processed";
    result.reasonCode = result.status;
    return result;
  }

  return applyProductWindowInputActions(context.window,
                                        context.activeSession,
                                        context.settings,
                                        controllerActions,
                                        context.inputSource);
}

ProductWindowEditorMousePickPreviewResult processProductWindowEditorMousePickPreview(
    ProductWindowEditorMousePickPreviewContext context) {
  ProductWindowEditorMousePickPreviewResult result;
  // branch-gate: BG-1063
  if (!context.click.clicked) {
    return result;
  }
  // branch-gate: BG-1063
  if (!productWindowEditorMousePickSurfaceReady(context.frontend,
                                               context.window)) {
    result.status = "room_editor_mouse_pick_preview_surface_blocked";
    result.reasonCode = result.status;
    return result;
  }
  // branch-gate: BG-1063
  if (context.window.interactionMode != ProductInteractionMode::Creative) {
    result.status = "room_editor_mouse_pick_preview_mode_blocked";
    result.reasonCode = result.status;
    return result;
  }
  // branch-gate: BG-1063
  if (!context.window.roomEditing.ready) {
    result.status = "room_editor_not_ready";
    result.reasonCode = result.status;
    return result;
  }

  const ProductRoomEditorActionResult pickResult =
      applyProductRoomEditorMousePickAutomation(
          context.window.roomEditing,
          context.window.roomEditorCursor,
          context.click.x,
          context.click.y,
          context.viewportConfig,
          context.anchorWorld);
  result.handled = true;
  result.picked = pickResult.ok;
  result.status = pickResult.status;
  result.reasonCode = pickResult.reasonCode;
  context.window.inputOwner = MenuOwner::Editor;
  context.window.lastInputAction = InputAction::EditorPreviewPlacement;
  context.window.lastInputAccepted = pickResult.ok;
  context.window.gameplayInputSuppressed = true;
  recordProductRoomEditorActionResult(context.window,
                                      pickResult,
                                      "room_editor.mouse_pick");
  // branch-gate: BG-1063
  if (!pickResult.ok) {
    return result;
  }

  const ProductRoomEditorPlacementPreviewResult preview =
      buildProductRoomEditorPreviewAutomation(context.window.roomEditing,
                                              context.window.roomEditorCursor);
  recordProductRoomEditorPreviewResult(context.window, preview);
  result.previewBuilt = true;
  result.accepted = preview.ok;
  result.status = preview.status;
  result.reasonCode = preview.reasonCode;
  return result;
}

void processProductWindowInputFrame(ProductWindowInputFrameContext context) {
  ActionState actionState;
  ProductOpeningMenuInputContext menuContext{
      context.frontend, context.saves, context.options, context.settingsTab,
      context.activeSession, context.worldSetupDraft, context.window,
      context.closeRequested, &context.settings};
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
    context.window.gamepadMenuSelectUsed = true;
    routeProductWindowMenuInput(gamepadAction, actionState, menuContext);
  }

  const MouseClick click = pollMouseClick(context.inputFrame.mouse);
  // branch-gate: BG-1029
  if (click.clicked) {
    const MouseClick menuClick =
        productWindowMenuClickForHitTest(click, context.sdlWindow);
    const OpeningMenuHitTestResult hit =
        openingMenuActionAt(context.frontend, menuClick.x, menuClick.y);
    // branch-gate: BG-1029
    if (hit.hit) {
      context.window.mouseMenuSelectUsed = true;
      // branch-gate: BG-1029
      if (hit.area == OpeningMenuHitArea::StarterAction) {
        context.frontend.selectedAction = hit.action;
        recordAction(actionState,
                     mouseClickAction(click),
                     true,
                     true,
                     false,
                     1.0F);
        (void)applyProductStarterMenuAction(
            InputAction::MenuConfirm,
            {context.frontend,
             context.options,
             context.saves,
             context.settingsTab,
             context.activeSession,
             context.worldSetupDraft,
             context.window,
             context.closeRequested});
      // branch-gate: BG-1029
      } else if (hit.area == OpeningMenuHitArea::DevToolsCategory) {
        context.frontend.devToolsCategory = hit.devToolsCategory;
        context.frontend.status = "dev_tools_category_selected";
      // branch-gate: BG-1142
      } else if (hit.area == OpeningMenuHitArea::DevToolsBack) {
        routeProductOpeningMenuInput(InputAction::MenuBack, actionState, menuContext);
      // branch-gate: BG-1029
      } else if (hit.area == OpeningMenuHitArea::SettingsTab) {
        context.settingsTab = hit.settingsTab;
        context.frontend.status = "settings_tab_selected";
      // branch-gate: BG-1142
      } else if (hit.area == OpeningMenuHitArea::SettingsBack) {
        routeProductOpeningMenuInput(InputAction::MenuBack, actionState, menuContext);
      // branch-gate: BG-1029
      } else if (hit.area == OpeningMenuHitArea::NewWorldCreate) {
        routeProductOpeningMenuInput(InputAction::MenuConfirm, actionState, menuContext);
      // branch-gate: BG-1029
      } else if (hit.area == OpeningMenuHitArea::NewWorldBack) {
        routeProductOpeningMenuInput(InputAction::MenuBack, actionState, menuContext);
      // branch-gate: BG-1138
      } else if (hit.area == OpeningMenuHitArea::NewWorldPreviousDungeon) {
        routeProductOpeningMenuInput(InputAction::MenuLeft, actionState, menuContext);
      // branch-gate: BG-1138
      } else if (hit.area == OpeningMenuHitArea::NewWorldNextDungeon) {
        routeProductOpeningMenuInput(InputAction::MenuRight, actionState, menuContext);
      // branch-gate: BG-1029
      } else if (hit.area == OpeningMenuHitArea::LoadSaveSlot) {
        // branch-gate: BG-1122
        if (hit.saveSlotIndex < context.saves.slots.slots.size()) {
          (void)selectProductSaveSlotById(
              context.saves.slots,
              context.saves.slots.slots[hit.saveSlotIndex].id,
              context.window);
          context.frontend.status = "load_save_selection_changed";
        }
      // branch-gate: BG-1029
      } else if (hit.area == OpeningMenuHitArea::LoadSaveLoad) {
        context.frontend.selectedAction = FrontendAction::LoadSave;
        routeProductOpeningMenuInput(InputAction::MenuConfirm, actionState, menuContext);
      // branch-gate: BG-1029
      } else if (hit.area == OpeningMenuHitArea::LoadSaveDelete) {
        context.frontend.selectedAction = FrontendAction::Delete;
        routeProductOpeningMenuInput(InputAction::MenuConfirm, actionState, menuContext);
      // branch-gate: BG-1029
      } else if (hit.area == OpeningMenuHitArea::LoadSaveBack) {
        routeProductOpeningMenuInput(InputAction::MenuBack, actionState, menuContext);
      // branch-gate: BG-1029
      } else if (hit.area == OpeningMenuHitArea::DeleteConfirmConfirm) {
        routeProductOpeningMenuInput(InputAction::MenuConfirm, actionState, menuContext);
      // branch-gate: BG-1029
      } else if (hit.area == OpeningMenuHitArea::DeleteConfirmBack) {
        routeProductOpeningMenuInput(InputAction::MenuBack, actionState, menuContext);
      }
    }
  }

  // branch-gate: BG-1029
  if (context.window.gameplayActive && context.activeSession.has_value() &&
      !frontendBlocksGameplayInput(context.frontend)) {
    ActionState gameplayActions;
    // branch-gate: BG-1029
    if (context.window.roomEditing.ready) {
      (void)processProductWindowEditorMousePickPreview({
          context.frontend,
          context.window,
          click,
          productRoomEditorMousePickViewportConfig(context.window),
          productRoomEditorMousePickAnchor(&*context.activeSession),
      });
      pollKeyboardRoomEditorActions(context.inputFrame.keyboard, gameplayActions);
      recordProductWindowControllerActions(
          context.frontend, context.window, context.inputFrame.controllerAction,
          gamepadControllerSample, controllerModeChordRequested,
          gameplayActions);
    } else {
      pollKeyboardGameplayActions(context.inputFrame.keyboard, gameplayActions);
      recordProductWindowControllerActions(
          context.frontend, context.window, context.inputFrame.controllerAction,
          gamepadControllerSample, controllerModeChordRequested,
          gameplayActions);
      pollMouseGameplayActions(context.inputFrame.mouse, gameplayActions);
    }
    (void)applyProductWindowInputActions(context.window, &*context.activeSession,
                                         &context.settings, gameplayActions,
                                         "action_map");
  }
  updateProductWindowMouseCapture(context.frontend,
                                  context.window,
                                  context.sdlWindow);
}

InputAction productWindowFunctionKeyAction(const SdlWindowEventState& eventState) {
  // branch-gate: BG-1194
  if (eventState.f3Pressed) {
    return InputAction::DevDebugOverlay;
  }
  // branch-gate: BG-1194
  if (eventState.f1Pressed || eventState.f2Pressed) {
    return InputAction::DevToggle;
  }
  return InputAction::None;
}

void recordProductWindowFunctionKeyKeyboardState(
    KeyboardInputState& keyboard,
    const SdlWindowEventState& eventState) {
  // branch-gate: BG-1194
  if (eventState.f1Pressed || eventState.f2Pressed) {
    keyboard.devToggleWasDown = true;
  }
  // branch-gate: BG-1194
  if (eventState.f3Pressed) {
    keyboard.debugOverlayWasDown = true;
  }
}

}  // namespace iggy3d
