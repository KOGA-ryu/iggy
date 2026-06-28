#include "app/iggy3d/window/InputFrame.hpp"

#include "app/iggy3d/view/OpeningMenuView.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/view/CameraController.hpp"
#include "app/iggy3d/input/ControllerActionRouting.hpp"
#include "app/iggy3d/gameplay/Controller.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "app/iggy3d/map_maker/CreativeFly.hpp"
#include "app/iggy3d/input/InteractionModeState.hpp"
#include "app/iggy3d/window/MouseCapturePolicy.hpp"
#include "app/iggy3d/room_editor/ActionController.hpp"
#include "app/iggy3d/room_editor/Preview.hpp"
#include "app/iggy3d/automation/Automation.hpp"
#include "app/iggy3d/automation/AutomationRoomEditing.hpp"
#include "app/iggy3d/menu/ActionHandlers.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/menu/InputRouter.hpp"
#include "app/iggy3d/menu/Transitions.hpp"
#include "app/iggy3d/Operations.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputRouter.hpp"
#include "app/platform/SdlWindow.hpp"

#include <algorithm>
#include <array>

namespace iggy3d {
namespace {

struct ProductWindowFunctionKeyBinding {
  bool SdlWindowEventState::* pressed = nullptr;
  InputAction action = InputAction::None;
  bool KeyboardInputState::* wasDown = nullptr;
};

struct OpeningMenuNavigationHitRow {
  OpeningMenuHitArea area = OpeningMenuHitArea::None;
  InputAction action = InputAction::None;
};

using ProductWindowTopLevelToggleHandler = ProductWindowTopLevelToggleResult (*)(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings* settings,
    bool* closeRequested);

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
    bool* closeRequested);
ProductWindowTopLevelToggleResult dispatchProductWindowMovementTuningToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings* settings,
    bool* closeRequested);
ProductWindowTopLevelToggleResult dispatchProductWindowMapMakerToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings* settings,
    bool* closeRequested);

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

static constexpr std::array kOpeningMenuNavigationHitRows{
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::DevToolsBack,
                                InputAction::MenuBack},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::SettingsBack,
                                InputAction::MenuBack},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::NewWorldCreate,
                                InputAction::MenuConfirm},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::NewWorldBack,
                                InputAction::MenuBack},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::NewWorldPreviousDungeon,
                                InputAction::MenuLeft},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::NewWorldNextDungeon,
                                InputAction::MenuRight},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::LoadSaveBack,
                                InputAction::MenuBack},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::DeleteConfirmConfirm,
                                InputAction::MenuConfirm},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::DeleteConfirmBack,
                                InputAction::MenuBack},
};

bool resolvedSurfaceAcceptsGameplayInput(const FrontendState& frontend,
                                         const ProductAppWindowState& window) {
  const ProductActiveSurfaceFrame surface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(frontend, window));
  return window.gameplayActive &&
         surface.activeSurface == ProductFrontendSurface::Gameplay &&
         surface.inputOwner == MenuOwner::Gameplay &&
         !surface.gameplayInputSuppressed;
}

InputAction openingMenuNavigationActionFor(OpeningMenuHitArea area) {
  for (const OpeningMenuNavigationHitRow& row : kOpeningMenuNavigationHitRows) {
    // branch-gate: BG-1029
    if (row.area == area) {
      return row.action;
    }
  }
  return InputAction::None;
}

InputAction movementTuningHeldAdjustmentAction(bool leftDown, bool rightDown) {
  // branch-gate: BG-1212
  if (leftDown == rightDown) {
    return InputAction::None;
  }
  // branch-gate: BG-1212
  return leftDown ? InputAction::MenuLeft : InputAction::MenuRight;
}

void resetMovementTuningRepeat(ProductMovementTuningRepeatState& repeat) {
  repeat.heldDirection = 0;
  repeat.heldFrames = 0U;
}

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
  // branch-gate: BG-1205
  const bool mapMakerLive = productMapMakerLiveForWindow(frontend, window);
  const ProductInteractionMode actionMode =
      mapMakerLive ? ProductInteractionMode::Player : window.interactionMode;
  ProductControllerActionRoutingResult result =
      productControllerActionRoutingSkipped(
          surface, actionMode, "controller_action_chord_consumed");
  if (!controllerModeChordRequested) {  // branch-gate: BG-1059
    result = recordProductControllerMappedActions({
        surface,
        actionMode,
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
  const ProductActiveSurfaceFrame surface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(frontend, window));
  const ProductMouseCapturePolicy policy = buildProductMouseCapturePolicy({
      window.gameplayActive,
      window.interactionMode,
      surface.inputOwner,
      surface.gameplayInputSuppressed,
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

Vec3 activePlayerPositionOrOrigin(const Session* activeSession) {
  // branch-gate: BG-1205
  if (activeSession == nullptr) {
    return {};
  }
  const EntityId playerActor = activeSession->state().players.actorForSlot(0);
  const EntityState* player = activeSession->state().world.findById(playerActor);
  // branch-gate: BG-1205
  if (player == nullptr) {
    return {};
  }
  return player->transform.position;
}

void ensureCreativeFlyAnchor(ProductAppWindowState& window,
                             const Session* activeSession) {
  // branch-gate: BG-1205
  if (window.viewport.creativeFlyAnchorValid) {
    return;
  }
  window.viewport.creativeFlyPositionMeters =
      activePlayerPositionOrOrigin(activeSession);
  window.viewport.creativeFlyAnchorValid = true;
}

bool mapMakerConsumesGameplayAction(InputAction action) {
  constexpr std::array kConsumedActions{
      InputAction::PlayerMoveX,
      InputAction::PlayerMoveY,
      InputAction::PlayerJump,
      InputAction::PlayerCrouch,
      InputAction::PlayerSprint,
      InputAction::PlayerDash,
  };
  return std::find(kConsumedActions.begin(), kConsumedActions.end(), action) !=
         kConsumedActions.end();
}

ActionState gameplayActionsAfterMapMakerConsumesMovement(
    const ActionState& actions) {
  ActionState filtered;
  for (const ActionStateEntry& entry : actions.entries) {
    // branch-gate: BG-1205
    if (!mapMakerConsumesGameplayAction(entry.action)) {
      recordAction(filtered, entry.action, entry.down, entry.pressed,
                   entry.released, entry.value);
    }
  }
  return filtered;
}

ProductCreativeFlyResult applyProductWindowCreativeFlyActions(
    ProductAppWindowState& window,
    const Session* activeSession,
    const ActionState& actions) {
  ensureCreativeFlyAnchor(window, activeSession);
  ProductCreativeFlyConfig config;
  config.enabled = true;
  ProductCreativeFlyInput input;
  input.moveX = actionAxisValue(actions, InputAction::PlayerMoveX);
  input.moveY = actionAxisValue(actions, InputAction::PlayerMoveY);
  // branch-gate: BG-1205
  const float flyUp = actionIsDown(actions, InputAction::PlayerJump) ? 1.0F : 0.0F;
  // branch-gate: BG-1205
  const float flyDown =
      actionIsDown(actions, InputAction::PlayerCrouch) ? 1.0F : 0.0F;
  input.moveZ = flyUp - flyDown;
  input.sprinting = actionIsDown(actions, InputAction::PlayerSprint);
  input.cameraYawDegrees = window.viewport.cameraYawDegrees;
  input.cameraPitchDegrees = window.viewport.cameraPitchDegrees;
  const ProductCreativeFlyResult fly =
      applyProductCreativeFlyInput(config, input,
                                   window.viewport.creativeFlyPositionMeters);
  window.viewport.creativeFlyActive = true;
  window.viewport.creativeFlyStatus = fly.reasonCode;
  window.viewport.creativeFlyReasonCode = fly.reasonCode;
  window.viewport.creativeFlySpeedMetersPerSecond = fly.speedMetersPerSecond;
  window.mapMakerStatus = "map_maker_active";
  window.mapMakerReasonCode = window.mapMakerStatus;
  // branch-gate: BG-1205
  if (fly.applied) {
    window.viewport.creativeFlyPositionMeters = fly.finalPositionMeters;
  }
  return fly;
}

ProductControllerSampleInputResult applyProductWindowInputActions(
    const FrontendState& frontend,
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
    window.lastInputAction = routed.action;
    window.lastInputAccepted = routed.accepted;
    syncProductWindowInputOwnerFromActiveSurface(frontend, window);
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
  // TODO(map-maker): Creative mode needs its own toolbelt/input owner here.
  // Movement is consumed for creative fly, but selected asset/tool input is not
  // modeled yet.
  // branch-gate: BG-1205
  bool mapMakerActionApplied = false;
  bool mapMakerActionAccepted = false;
  ActionState gameplayActionsForSession = acceptedGameplayActions;
  const bool mapMakerLive = productMapMakerLiveForWindow(frontend, window);
  if (mapMakerLive) {
    const ProductCreativeFlyResult fly =
        applyProductWindowCreativeFlyActions(window, activeSession,
                                             acceptedGameplayActions);
    mapMakerActionApplied = !acceptedGameplayActions.entries.empty();
    mapMakerActionAccepted =
        mapMakerActionApplied &&
        (fly.applied || fly.reasonCode == "creative_fly_no_input");
    gameplayActionsForSession =
        gameplayActionsAfterMapMakerConsumesMovement(acceptedGameplayActions);
  }
  // branch-gate: BG-1061
  if (activeSession != nullptr) {
    const SpatialSurfaceSet* collisionSurfaces =
        productActiveRoomCollisionSurfaces(window.activeRoomCollision);
    applyProductGameplayActions(*activeSession, gameplayActionsForSession, window,
                                inputSource, collisionSurfaces);
    result.actionApplied = !acceptedGameplayActions.entries.empty();
    result.actionAccepted = window.gameplayCommandAccepted || mapMakerActionAccepted;
  } else if (mapMakerLive) {  // branch-gate: BG-1205
    result.actionApplied = mapMakerActionApplied;
    result.actionAccepted = mapMakerActionAccepted;
  }
  return result;
}

void routeProductWindowMenuInput(InputAction inputAction,
                                 ActionState& actionState,
                                 ProductOpeningMenuInputContext context) {
  // branch-gate: BG-1029
  if (inputAction == InputAction::MenuBack &&
      cancelProductRoomEditorPendingPreviewFromBack(context.frontend,
                                                    context.window)) {
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

ProductWindowTopLevelToggleResult dispatchProductWindowSystemToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings* settings,
    bool* closeRequested) {
  bool ignoredCloseRequested = false;
  bool& closeTarget =
      closeRequested == nullptr ? ignoredCloseRequested : *closeRequested;  // branch-gate: BG-1194
  const ProductMenuActionResult menuResult = applyProductSystemPauseMenuAction(
      action, {frontend, window, closeTarget, settings});
  return {menuResult.handled, menuResult.accepted, action};
}

ProductWindowTopLevelToggleResult dispatchProductWindowMovementTuningToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings*,
    bool*) {
  const ProductMovementTuningInputResult tuningResult =
      applyProductWindowMovementTuningInput(frontend, window, action);
  return {tuningResult.handled, tuningResult.accepted, action};
}

ProductWindowTopLevelToggleResult dispatchProductWindowMapMakerToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings*,
    bool*) {
  const ProductMenuActionResult mapMakerResult =
      applyProductGameplayMapMakerToggleAction(action, {frontend, window});
  return {mapMakerResult.handled, mapMakerResult.accepted, action};
}

}  // namespace

void dispatchProductOpeningMenuMouseHit(
    const OpeningMenuHitTestResult& hit,
    const MouseClick& click,
    ActionState& actionState,
    ProductOpeningMenuInputContext context) {
  const InputAction navigationAction = openingMenuNavigationActionFor(hit.area);
  // branch-gate: BG-1029
  if (navigationAction != InputAction::None) {
    routeProductOpeningMenuInput(navigationAction, actionState, context);
    return;
  }

  // branch-gate: BG-1029
  switch (hit.area) {
    case OpeningMenuHitArea::StarterAction:
      context.frontend.selectedAction = hit.action;
      recordAction(actionState,
                   mouseClickAction(click),
                   true,
                   true,
                   false,
                   1.0F);
      routeProductOpeningMenuInput(InputAction::MenuConfirm, actionState,
                                   context);
      return;
    case OpeningMenuHitArea::DevToolsCategory:
      context.frontend.devToolsCategory = hit.devToolsCategory;
      context.frontend.status = "dev_tools_category_selected";
      return;
    case OpeningMenuHitArea::SettingsTab:
      context.settingsTab = hit.settingsTab;
      context.frontend.status = "settings_tab_selected";
      return;
    case OpeningMenuHitArea::LoadSaveSlot:
      // branch-gate: BG-1122
      if (hit.saveSlotIndex < context.saves.slots.slots.size()) {
        (void)selectProductSaveSlotById(
            context.saves.slots,
            context.saves.slots.slots[hit.saveSlotIndex].id,
            context.window);
        context.frontend.status = "load_save_selection_changed";
      }
      return;
    case OpeningMenuHitArea::LoadSaveLoad:
      context.frontend.selectedAction = FrontendAction::LoadSave;
      routeProductOpeningMenuInput(InputAction::MenuConfirm, actionState,
                                   context);
      return;
    case OpeningMenuHitArea::LoadSaveDelete:
      context.frontend.selectedAction = FrontendAction::Delete;
      routeProductOpeningMenuInput(InputAction::MenuConfirm, actionState,
                                   context);
      return;
    default:
      context.frontend.status = "opening_menu_hit_area_unhandled";
      return;
  }
}

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

bool cancelProductRoomEditorPendingPreviewFromBack(
    const FrontendState& frontend,
    ProductAppWindowState& window) {
  // branch-gate: BG-1055
  if (!window.roomEditing.ready || !window.roomEditorPreviewActive) {
    return false;
  }
  const ProductRoomEditorPreviewInputResult cancelled =
      applyProductRoomEditorPreviewInputAction(window,
                                              InputAction::EditorCancelPreview);
  window.lastInputAction = InputAction::EditorCancelPreview;
  window.lastInputAccepted = cancelled.ok;
  syncProductWindowInputOwnerFromActiveSurface(frontend, window);
  return cancelled.ok;
}

ProductMovementTuningInputResult applyProductWindowMovementTuningInput(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action) {
  ProductMovementTuningInputResult result;
  const bool gameplaySurfaceActive =
      resolvedSurfaceAcceptsGameplayInput(frontend, window);
  // branch-gate: BG-1212
  if (action == InputAction::MovementTuningToggle) {
    result.handled = true;
    // branch-gate: BG-1212
    if (!gameplaySurfaceActive) {
      clearProductGameplayMovementTuning(window);
      result.status = "movement_tuning_gameplay_inactive";
      result.reasonCode = result.status;
      window.gameplayMovementTuningStatus = result.status;
      window.gameplayMovementTuningReasonCode = result.reasonCode;
      frontend.status = result.status;
      return result;
    }

    window.gameplayMovementTuningVisible = !window.gameplayMovementTuningVisible;
    result.accepted = true;
    result.status = window.gameplayMovementTuningVisible ? "movement_tuning_visible"
                                                         : "movement_tuning_hidden";
    result.reasonCode = result.status;
    window.gameplayMovementTuningStatus = result.status;
    window.gameplayMovementTuningReasonCode = result.reasonCode;
    frontend.status = result.status;
    return result;
  }

  // branch-gate: BG-1212
  if (!gameplaySurfaceActive && window.gameplayMovementTuningVisible) {
    clearProductGameplayMovementTuning(window);
  }

  // branch-gate: BG-1212
  if (!gameplaySurfaceActive || !window.gameplayMovementTuningVisible) {
    return result;
  }

  // branch-gate: BG-1212
  if (action == InputAction::MenuConfirm || action == InputAction::MenuDown) {
    window.gameplayMovementTuningSelectedField =
        nextProductGameplayMovementTuningField(
            window.gameplayMovementTuningSelectedField);
    result.handled = true;
    result.accepted = true;
    result.status = "movement_tuning_field_selected";
  } else if (action == InputAction::MenuUp) {  // branch-gate: BG-1212
    window.gameplayMovementTuningSelectedField =
        previousProductGameplayMovementTuningField(
            window.gameplayMovementTuningSelectedField);
    result.handled = true;
    result.accepted = true;
    result.status = "movement_tuning_field_selected";
  // branch-gate: BG-1212
  } else if (action == InputAction::MenuLeft ||
             action == InputAction::MenuRight) {
    const int direction =
        action == InputAction::MenuLeft ? -1 : 1;  // branch-gate: BG-1212
    (void)adjustProductGameplayMovementTuning(
        window.gameplayMovementTuning,
        window.gameplayMovementTuningSelectedField,
        direction);
    result.handled = true;
    result.accepted = true;
    result.status = "movement_tuning_adjusted";
  }

  // branch-gate: BG-1212
  if (result.handled) {
    result.reasonCode = result.status;
    window.gameplayMovementTuningStatus = result.status;
    window.gameplayMovementTuningReasonCode = result.reasonCode;
    frontend.status = result.status;
  }
  return result;
}

ProductWindowTopLevelToggleResult dispatchProductWindowTopLevelToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings* settings,
    bool* closeRequested) {
  for (const ProductWindowTopLevelToggleRow& row : kProductWindowTopLevelToggleRows) {
    // branch-gate: BG-1194
    if (row.action == action) {
      return row.handler(frontend, window, action, settings, closeRequested);
    }
  }
  return {};
}

ProductMovementTuningInputResult applyProductWindowMovementTuningHeldInput(
    FrontendState& frontend,
    ProductAppWindowState& window,
    ProductMovementTuningRepeatState& repeat,
    bool leftDown,
    bool rightDown,
    ProductMovementTuningRepeatPolicy policy) {
  ProductMovementTuningInputResult result;
  const bool gameplaySurfaceActive =
      resolvedSurfaceAcceptsGameplayInput(frontend, window);
  // branch-gate: BG-1212
  if (!gameplaySurfaceActive || !window.gameplayMovementTuningVisible) {
    resetMovementTuningRepeat(repeat);
    // branch-gate: BG-1212
    if (!gameplaySurfaceActive && window.gameplayMovementTuningVisible) {
      (void)applyProductWindowMovementTuningInput(frontend,
                                                  window,
                                                  InputAction::None);
    }
    return result;
  }

  const InputAction action = movementTuningHeldAdjustmentAction(leftDown, rightDown);
  // branch-gate: BG-1212
  if (action == InputAction::None) {
    resetMovementTuningRepeat(repeat);
    return result;
  }

  const int direction =
      action == InputAction::MenuLeft ? -1 : 1;  // branch-gate: BG-1212
  // branch-gate: BG-1212
  if (repeat.heldDirection != direction) {
    repeat.heldDirection = direction;
    repeat.heldFrames = 1U;
    return result;
  }

  ++repeat.heldFrames;
  const std::uint32_t interval =
      std::max<std::uint32_t>(1U, policy.repeatIntervalFrames);
  // branch-gate: BG-1212
  if (repeat.heldFrames < policy.initialDelayFrames ||
      (repeat.heldFrames - policy.initialDelayFrames) % interval != 0U) {
    return result;
  }

  return applyProductWindowMovementTuningInput(frontend, window, action);
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

  return applyProductWindowInputActions(context.frontend,
                                        context.window,
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
  context.window.lastInputAction = InputAction::EditorPreviewPlacement;
  context.window.lastInputAccepted = pickResult.ok;
  syncProductWindowInputOwnerFromActiveSurface(context.frontend, context.window);
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
      context.closeRequested, context.settings};
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
                                                &context.closeRequested);
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
    context.window.gamepadMenuSelectUsed = true;
    const ProductWindowTopLevelToggleResult gamepadToggle =
        dispatchProductWindowTopLevelToggleAction(context.frontend,
                                                  context.window,
                                                  gamepadAction,
                                                  &context.settings,
                                                  &context.closeRequested);
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
      dispatchProductOpeningMenuMouseHit(hit, click, actionState, menuContext);
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
      // branch-gate: BG-1212
      if (!context.window.gameplayMovementTuningVisible) {
        pollKeyboardGameplayActions(context.inputFrame.keyboard, gameplayActions);
      }
      recordProductWindowControllerActions(
          context.frontend, context.window, context.inputFrame.controllerAction,
          gamepadControllerSample, controllerModeChordRequested,
          gameplayActions);
      pollMouseGameplayActions(context.inputFrame.mouse, gameplayActions);
    }
    (void)applyProductWindowInputActions(context.frontend,
                                         context.window,
                                         &*context.activeSession,
                                         &context.settings, gameplayActions,
                                         "action_map");
  }
  updateProductWindowMouseCapture(context.frontend,
                                  context.window,
                                  context.sdlWindow);
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
