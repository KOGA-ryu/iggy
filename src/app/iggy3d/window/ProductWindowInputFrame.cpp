#include "app/iggy3d/window/ProductWindowInputFrame.hpp"

#include "app/iggy3d/OpeningMenuView.hpp"
#include "app/iggy3d/gameplay/ProductActiveRoomCollision.hpp"
#include "app/iggy3d/ProductCameraController.hpp"
#include "app/iggy3d/ProductControllerActionRouting.hpp"
#include "app/iggy3d/gameplay/ProductGameplayController.hpp"
#include "app/iggy3d/ProductInteractionModeState.hpp"
#include "app/iggy3d/window/ProductMouseCapturePolicy.hpp"
#include "app/iggy3d/room_editor/ProductRoomEditorActionController.hpp"
#include "app/iggy3d/room_editor/ProductRoomEditorPreview.hpp"
#include "app/iggy3d/product/Automation.hpp"
#include "app/iggy3d/product/AutomationRoomEditing.hpp"
#include "app/iggy3d/product/ProductMenuInputRouter.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputRouter.hpp"
#include "app/platform/SdlWindow.hpp"

namespace iggy3d {
namespace {

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

}  // namespace

void initializeProductWindowInputFrameState(ProductWindowInputFrameState& state,
                                            ProductAppWindowState& window) {
  initializeGamepadMenuState(state.gamepad);
  window.gamepadAvailable = state.gamepad.gamepadAvailable;
  window.gamepadName = state.gamepad.gamepadName;
  // branch-gate: BG-1029
  window.gamepadMapping = state.gamepad.gamepadAvailable ? "sdl_gamepad" : "unavailable";
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
      context.closeRequested};
  routeProductOpeningMenuInput(pollKeyboardMenuAction(context.inputFrame.keyboard),
                               actionState, menuContext);
  // branch-gate: BG-1029
  if (context.frontend.childScreen == FrontendScreen::NewWorld) {
    const char paintGlyph = pollKeyboardAsciiRoomPaintGlyph(context.inputFrame.keyboard);
    // branch-gate: BG-1029
    if (paintGlyph != '\0') {
      applyDungeonDraftPaintGlyph(context.worldSetupDraft, context.window, paintGlyph);
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
    routeProductOpeningMenuInput(gamepadAction, actionState, menuContext);
  }

  const MouseClick click = pollMouseClick(context.inputFrame.mouse);
  // branch-gate: BG-1029
  if (click.clicked) {
    const OpeningMenuHitTestResult hit =
        openingMenuActionAt(context.frontend, click.x, click.y);
    // branch-gate: BG-1029
    if (hit.hit) {
      context.window.mouseMenuSelectUsed = true;
      // branch-gate: BG-1029
      if (hit.area == OpeningMenuHitArea::StarterAction) {
        context.frontend.selectedAction = hit.action;
        routeProductOpeningMenuInput(mouseClickAction(click), actionState, menuContext);
      // branch-gate: BG-1029
      } else if (hit.area == OpeningMenuHitArea::DevToolsCategory) {
        context.frontend.devToolsCategory = hit.devToolsCategory;
        context.frontend.status = "dev_tools_category_selected";
      // branch-gate: BG-1029
      } else if (hit.area == OpeningMenuHitArea::SettingsTab) {
        context.settingsTab = hit.settingsTab;
        context.frontend.status = "settings_tab_selected";
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

}  // namespace iggy3d
