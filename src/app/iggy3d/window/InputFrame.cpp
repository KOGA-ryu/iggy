#include "app/iggy3d/window/InputFrame.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/view/OpeningMenuView.hpp"
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
#include "app/iggy3d/menu/ActionHandlers.hpp"
#include "app/iggy3d/menu/DrawList.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/menu/InputRouter.hpp"
#include "app/iggy3d/menu/Transitions.hpp"
#include "app/iggy3d/Operations.hpp"
#include "app/iggy3d/ProductCreativeBakedRoomRefresh.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputRouter.hpp"
#include "app/platform/SdlWindow.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

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
    bool* closeRequested,
    creative::CreativeAppState* creativeApp);

struct ProductCreativeDocumentRevisionSnapshot {
  bool observed = false;
  std::uint64_t documentId = 0;
  std::uint64_t revision = 0;
};

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
ProductWindowTopLevelToggleResult dispatchProductWindowMovementTuningToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings* settings,
    bool* closeRequested,
    creative::CreativeAppState* creativeApp);
ProductWindowTopLevelToggleResult dispatchProductWindowMapMakerToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings* settings,
    bool* closeRequested,
    creative::CreativeAppState* creativeApp);

ProductCreativeDocumentRevisionSnapshot
captureProductCreativeDocumentRevision(
    const creative::CreativeAppState* creativeApp) noexcept {
  ProductCreativeDocumentRevisionSnapshot snapshot;
  if (creativeApp == nullptr) {
    return snapshot;
  }

  const creative::CreativeDocument& document = creativeApp->facade.document();
  if (!document.isValid()) {
    return snapshot;
  }

  snapshot.observed = true;
  snapshot.documentId = document.id();
  snapshot.revision = document.revision();
  return snapshot;
}

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
  return window.gameplay.gameplayActive &&
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

MouseClick productWindowClickForHitTest(MouseClick click,
                                        const SdlWindow* sdlWindow,
                                        std::uint32_t virtualWidth,
                                        std::uint32_t virtualHeight) {
  // branch-gate: BG-1123
  if (sdlWindow == nullptr) {
    return click;
  }
  const SdlWindowEventState& eventState = sdlWindow->eventState();
  return normalizeProductWindowMenuClick(click,
                                         eventState.windowWidth,
                                         eventState.windowHeight,
                                         virtualWidth,
                                         virtualHeight);
}

MouseClick productWindowMenuClickForHitTest(MouseClick click,
                                            const SdlWindow* sdlWindow) {
  return productWindowClickForHitTest(click, sdlWindow, 1280U, 720U);
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
        applyProductRoomEditorActions(window.creativeAuthoring.roomEditing,
                                      window.creativeAuthoring.roomEditorCursor,
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
    ActionState& actions,
    const creative::CreativeAppState* creativeApp = nullptr) {
  const ProductInputSurface surface = productInputSurfaceFor(frontend, window);
  // branch-gate: BG-1205
  const bool mapMakerLive =
      productMapMakerLiveForSource(frontend, window, creativeApp);
  const ProductInteractionMode actionMode =
      mapMakerLive ? ProductInteractionMode::Player
                   : window.inputDevice.interactionMode;
  const ProductInteractionMode proofMode =
      // branch-gate: BG-1059
      controllerModeChordRequested ? window.inputDevice.interactionMode
                                   : actionMode;
  ProductControllerActionRoutingResult result =
      productControllerActionRoutingSkipped(
          surface, proofMode, "controller_action_chord_consumed");
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
    const ProductAppWindowState& window,
    const creative::CreativeAppState* creativeApp) {
  return frontend.screen == FrontendScreen::Gameplay && window.gameplay.gameplayActive &&
         !frontendBlocksGameplayInput(frontend) &&
         !productCreativeWorldActiveForSource(window, creativeApp);
}

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
  const ProductCreativeFlyAnchorStore& anchor =
      ensureFreshCreativeFlyAnchor(window, activeSession);
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
      applyProductCreativeFlyInput(config, input, anchor.positionMeters);
  window.viewport.creativeFlyActive = true;
  window.viewport.creativeFlyStatus = fly.reasonCode;
  window.viewport.creativeFlyReasonCode = fly.reasonCode;
  window.viewport.creativeFlySpeedMetersPerSecond = fly.speedMetersPerSecond;
  // branch-gate: BG-1205
  if (fly.applied) {
    recordCreativeFlyAnchorIntegrated(window, fly.finalPositionMeters);
  }
  return fly;
}

ProductControllerSampleInputResult applyProductWindowInputActionsImpl(
    const FrontendState& frontend,
    ProductAppWindowState& window,
    Session* activeSession,
    const FrontendSettings* settings,
    const ActionState& gameplayActions,
    std::string_view inputSource,
    const creative::CreativeAppState* creativeApp = nullptr) {
  ProductControllerSampleInputResult result;
  result.processed = true;
  result.status = "controller_sample_processed";
  result.reasonCode = result.status;

  ActionState acceptedGameplayActions;
  ActionState acceptedEditorActions;
  InputRoutingContext routingContext;
  routingContext.owners.editor = window.creativeAuthoring.roomEditing.ready;
  routingContext.owners.gameplay = true;
  for (const ActionStateEntry& entry : gameplayActions.entries) {
    const InputRoutingResult routed = routeInputAction(routingContext, entry.action);
    window.inputDevice.lastInputAction = routed.action;
    window.inputDevice.lastInputAccepted = routed.accepted;
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
  if (window.creativeAuthoring.roomEditing.ready) {
    window.gameplay.gameplayMovement.groundVelocityX = 0.0F;
    window.gameplay.gameplayMovement.groundVelocityZ = 0.0F;
    window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond = 0.0F;
    window.gameplay.gameplayWallRun.candidateAvailable = false;
    window.gameplay.gameplayWallRun.candidateStatus = "wall_run_grounded";
    window.gameplay.gameplayWallRun.candidateReasonCode =
        window.gameplay.gameplayWallRun.candidateStatus;
    window.gameplay.gameplayWallRun.side = "none";
    window.gameplay.gameplayWallRun.surfaceId = "none";
    window.gameplay.gameplayWallRun.normalX = 0.0F;
    window.gameplay.gameplayWallRun.normalY = 0.0F;
    window.gameplay.gameplayWallRun.normalZ = 0.0F;
    window.gameplay.gameplayWallRun.approachSpeedMetersPerSecond = 0.0F;
    window.gameplay.gameplayWallRun.active = false;
    window.gameplay.gameplayWallRun.status = "wall_run_inactive";
    window.gameplay.gameplayWallRun.reasonCode = window.gameplay.gameplayWallRun.status;
    window.gameplay.gameplayWallRun.remainingSeconds = 0.0F;
    window.gameplay.gameplayWallRun.durationSeconds = 0.0F;
    window.gameplay.gameplayWallRun.gravityMultiplier = 1.0F;
    window.gameplay.gameplayWallRun.speedMultiplier = 1.0F;
    // branch-gate: BG-1061
    if (!window.gameplay.gameplayJump.active) {
      window.gameplay.gameplayMovement.grounded = true;
      window.gameplay.gameplayMovement.state = ProductGameplayMovementState::IdleGrounded;
    }
    window.gameplay.gameplayJump.coyoteSecondsRemaining = 0.0F;
    window.gameplay.gameplayJump.bufferSecondsRemaining = 0.0F;
    window.gameplay.gameplayJump.held = false;
    window.gameplay.gameplayJump.cutApplied = false;
  }

  // branch-gate: BG-1061
  if (!acceptedEditorActions.entries.empty()) {
    applyProductWindowRoomEditorActions(window, acceptedEditorActions);
    result.actionApplied = true;
    result.actionAccepted = window.creativeAuthoring.roomEditorLastOperationAccepted;
    return result;
  }

  // branch-gate: BG-1061
  if (settings != nullptr) {
    FrontendSettings movementTunedSettings = *settings;
    movementTunedSettings.lookSensitivity =
        window.gameplay.gameplayMovement.tuning.lookSensitivity;
    movementTunedSettings.invertLook =
        productGameplayMovementTuningInvertLook(window.gameplay.gameplayMovement.tuning);
    applyProductCameraActions(acceptedGameplayActions, window.viewport,
                              movementTunedSettings,
                              inputSource);
  }
  // TODO(map-maker): Creative mode needs its own toolbelt/input owner here.
  // Movement is consumed for creative fly, but selected asset/tool input is not
  // modeled yet.
  // branch-gate: BG-1205
  bool mapMakerActionApplied = false;
  bool mapMakerActionAccepted = false;
  ActionState gameplayActionsForSession = acceptedGameplayActions;
  const bool mapMakerLive =
      productMapMakerLiveForSource(frontend, window, creativeApp);
  if (mapMakerLive) {
    const ProductCreativeFlyResult fly =
        applyProductWindowCreativeFlyActions(window, activeSession,
                                             acceptedGameplayActions);
    // The map_maker surface tags its status here; the shared fly wrapper stays
    // surface-neutral so the creative-document Navigate path (TV1-H) can reuse
    // it without stamping a map_maker label.
    window.viewport.mapMakerStatus = "map_maker_active";
    window.viewport.mapMakerReasonCode = window.viewport.mapMakerStatus;
    mapMakerActionApplied = !acceptedGameplayActions.entries.empty();
    mapMakerActionAccepted =
        mapMakerActionApplied &&
        (fly.applied || fly.reasonCode == "creative_fly_no_input");
    gameplayActionsForSession =
        gameplayActionsAfterMapMakerConsumesMovement(acceptedGameplayActions);
  }
  // branch-gate: BG-1061
  if (activeSession != nullptr) {
    activeRoomCollisionFreshness(window) =
        ensureActiveRoomCollisionFresh(window, activeSession);
    const SpatialSurfaceSet* collisionSurfaces =
        productActiveRoomCollisionSurfaces(activeRoomCollision(window));
    applyProductGameplayActions(*activeSession, gameplayActionsForSession, window,
                                inputSource, collisionSurfaces);
    result.actionApplied = !acceptedGameplayActions.entries.empty();
    result.actionAccepted = window.gameplay.gameplayCommand.accepted || mapMakerActionAccepted;
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
    bool* closeRequested,
    creative::CreativeAppState* creativeApp) {
  bool ignoredCloseRequested = false;
  bool& closeTarget =
      closeRequested == nullptr ? ignoredCloseRequested : *closeRequested;  // branch-gate: BG-1194
  const ProductMenuActionResult menuResult = applyProductSystemPauseMenuAction(
      action, {frontend, window, closeTarget, settings, creativeApp});
  return {menuResult.handled, menuResult.accepted, action};
}

ProductWindowTopLevelToggleResult dispatchProductWindowMovementTuningToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings*,
    bool*,
    creative::CreativeAppState*) {
  const ProductMovementTuningInputResult tuningResult =
      applyProductWindowMovementTuningInput(frontend, window, action);
  return {tuningResult.handled, tuningResult.accepted, action};
}

ProductWindowTopLevelToggleResult dispatchProductWindowMapMakerToggleAction(
    FrontendState& frontend,
    ProductAppWindowState& window,
    InputAction action,
    FrontendSettings*,
    bool*,
    creative::CreativeAppState* creativeApp) {
  const ProductMenuActionResult mapMakerResult =
      applyProductGameplayMapMakerToggleAction(action,
                                               {frontend, window, creativeApp});
  return {mapMakerResult.handled, mapMakerResult.accepted, action};
}

}  // namespace

ProductControllerSampleInputResult applyProductWindowInputActions(
    const FrontendState& frontend,
    ProductAppWindowState& window,
    Session* activeSession,
    const FrontendSettings* settings,
    const ActionState& gameplayActions,
    std::string_view inputSource,
    creative::CreativeAppState* creativeApp) {
  return applyProductWindowInputActionsImpl(frontend,
                                            window,
                                            activeSession,
                                            settings,
                                            gameplayActions,
                                            inputSource,
                                            creativeApp);
}

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
      context.frontend.saveBrowserMode = FrontendSaveBrowserMode::Load;
      context.window.saveSession.saveSlotBrowserMode =
          std::string(frontendSaveBrowserModeName(context.frontend.saveBrowserMode));
      routeProductOpeningMenuInput(InputAction::MenuConfirm, actionState,
                                   context);
      return;
    case OpeningMenuHitArea::LoadSaveDelete:
      context.frontend.saveBrowserMode = FrontendSaveBrowserMode::Delete;
      context.window.saveSession.saveSlotBrowserMode =
          std::string(frontendSaveBrowserModeName(context.frontend.saveBrowserMode));
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
  window.inputDevice.gamepadAvailable = state.gamepad.gamepadAvailable;
  window.inputDevice.gamepadName = state.gamepad.gamepadName;
  // branch-gate: BG-1029
  window.inputDevice.gamepadMapping =
      state.gamepad.gamepadAvailable ? "sdl_gamepad" : "unavailable";
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

MouseClick resolveProductWindowInputMouseClick(
    ProductWindowInputClickOverride clickOverride,
    MouseInputState& mouse) {
  if (clickOverride.enabled) {
    return clickOverride.click;
  }
  return pollMouseClick(mouse);
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
  (void)frontend;
  // branch-gate: BG-1055
  if (!window.creativeAuthoring.roomEditing.ready || !window.creativeAuthoring.roomEditorPreview.active) {
    return false;
  }
  const ProductRoomEditorPreviewInputResult cancelled =
      applyProductRoomEditorPreviewInputAction(window,
                                              InputAction::EditorCancelPreview);
  window.inputDevice.lastInputAction = InputAction::EditorCancelPreview;
  window.inputDevice.lastInputAccepted = cancelled.ok;
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
      window.gameplay.gameplayMovement.tuningStatus = result.status;
      window.gameplay.gameplayMovement.tuningReasonCode = result.reasonCode;
      frontend.status = result.status;
      return result;
    }

    window.gameplay.gameplayMovement.tuningVisible = !window.gameplay.gameplayMovement.tuningVisible;
    result.accepted = true;
    result.status = window.gameplay.gameplayMovement.tuningVisible ? "movement_tuning_visible"
                                                         : "movement_tuning_hidden";
    result.reasonCode = result.status;
    window.gameplay.gameplayMovement.tuningStatus = result.status;
    window.gameplay.gameplayMovement.tuningReasonCode = result.reasonCode;
    frontend.status = result.status;
    return result;
  }

  // branch-gate: BG-1212
  if (!gameplaySurfaceActive && window.gameplay.gameplayMovement.tuningVisible) {
    clearProductGameplayMovementTuning(window);
  }

  // branch-gate: BG-1212
  if (!gameplaySurfaceActive || !window.gameplay.gameplayMovement.tuningVisible) {
    return result;
  }

  // branch-gate: BG-1212
  if (action == InputAction::MenuConfirm || action == InputAction::MenuDown) {
    window.gameplay.gameplayMovement.tuningSelectedField =
        nextProductGameplayMovementTuningField(
            window.gameplay.gameplayMovement.tuningSelectedField);
    result.handled = true;
    result.accepted = true;
    result.status = "movement_tuning_field_selected";
  } else if (action == InputAction::MenuUp) {  // branch-gate: BG-1212
    window.gameplay.gameplayMovement.tuningSelectedField =
        previousProductGameplayMovementTuningField(
            window.gameplay.gameplayMovement.tuningSelectedField);
    result.handled = true;
    result.accepted = true;
    result.status = "movement_tuning_field_selected";
  // branch-gate: BG-1212
  } else if (action == InputAction::MenuLeft ||
             action == InputAction::MenuRight) {
    const int direction =
        action == InputAction::MenuLeft ? -1 : 1;  // branch-gate: BG-1212
    (void)adjustProductGameplayMovementTuning(
        window.gameplay.gameplayMovement.tuning,
        window.gameplay.gameplayMovement.tuningSelectedField,
        direction);
    result.handled = true;
    result.accepted = true;
    result.status = "movement_tuning_adjusted";
  }

  // branch-gate: BG-1212
  if (result.handled) {
    result.reasonCode = result.status;
    window.gameplay.gameplayMovement.tuningStatus = result.status;
    window.gameplay.gameplayMovement.tuningReasonCode = result.reasonCode;
    frontend.status = result.status;
  }
  return result;
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
  if (!gameplaySurfaceActive || !window.gameplay.gameplayMovement.tuningVisible) {
    resetMovementTuningRepeat(repeat);
    // branch-gate: BG-1212
    if (!gameplaySurfaceActive && window.gameplay.gameplayMovement.tuningVisible) {
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

  if (context.creativeApp != nullptr &&
      productCreativeDocumentEditorActiveForSource(context.window,
                                                   context.creativeApp)) {
    ProductControllerSampleInputResult result;
    result.processed = true;
    result.status = "controller_sample_creative_document_suppressed";
    result.reasonCode = result.status;
    return result;
  }

  ActionState controllerActions;
  recordProductWindowControllerActions(context.frontend,
                                       context.window,
                                       context.controllerAction,
                                       sample,
                                       controllerModeChordRequested,
                                       controllerActions,
                                       context.creativeApp);

  // branch-gate: BG-1061
  if (!context.window.gameplay.gameplayActive || context.activeSession == nullptr ||
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
                                        context.inputSource,
                                        context.creativeApp);
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
                                                context.window,
                                                context.creativeApp)) {
    result.status = "room_editor_mouse_pick_preview_surface_blocked";
    result.reasonCode = result.status;
    return result;
  }
  // branch-gate: BG-1063
  if (context.window.inputDevice.interactionMode !=
      ProductInteractionMode::Creative) {
    result.status = "room_editor_mouse_pick_preview_mode_blocked";
    result.reasonCode = result.status;
    return result;
  }
  // branch-gate: BG-1063
  if (!context.window.creativeAuthoring.roomEditing.ready) {
    result.status = "room_editor_not_ready";
    result.reasonCode = result.status;
    return result;
  }

  const ProductRoomEditorActionResult pickResult =
      applyProductRoomEditorMousePickAutomation(
          context.window.creativeAuthoring.roomEditing,
          context.window.creativeAuthoring.roomEditorCursor,
          context.click.x,
          context.click.y,
          context.viewportConfig,
          context.anchorWorld);
  result.handled = true;
  result.picked = pickResult.ok;
  result.status = pickResult.status;
  result.reasonCode = pickResult.reasonCode;
  context.window.inputDevice.lastInputAction = InputAction::EditorPreviewPlacement;
  context.window.inputDevice.lastInputAccepted = pickResult.ok;
  recordProductRoomEditorActionResult(context.window,
                                      pickResult,
                                      "room_editor.mouse_pick");
  // branch-gate: BG-1063
  if (!pickResult.ok) {
    return result;
  }

  const ProductRoomEditorPlacementPreviewResult preview =
      buildProductRoomEditorPreviewAutomation(context.window.creativeAuthoring.roomEditing,
                                              context.window.creativeAuthoring.roomEditorCursor);
  recordProductRoomEditorPreviewResult(context.window, preview);
  result.previewBuilt = true;
  result.accepted = preview.ok;
  result.status = preview.status;
  result.reasonCode = preview.reasonCode;
  return result;
}

struct ProductCreativeDocumentInputOrchestrationRequest {
  ProductWindowInputFrameContext& context;
  MouseClick click;
  bool higherPriorityMouseConsumed = false;
  bool frontendMouseOwnsInput = false;
};

struct ProductCreativeDocumentInputOrchestrationResult {
  MouseClick downstreamClick;
  creative::TargetRef pointerTarget;
  bool creativeDocumentActive = false;
  bool creativeDocumentInputHandled = false;
};

struct ProductCreativeDocumentRevisionPhaseState {
  ProductCreativeDocumentRevisionSnapshot revisionBefore;
  creative::CreativeDocument undoSnapshotBefore;
  bool undoSnapshotBeforeAvailable = false;
};

struct ProductCreativeUiCommandPhaseResult {
  ProductCreativeUiInputFrameReceipt inputReceipt;
  ProductCreativeUiCommandFrameReceipt commandReceipt;
};

struct ProductCreativeViewportInputPhaseResult {
  MouseClick downstreamClick;
  creative::TargetRef pointerTarget;
};

[[nodiscard]] bool canDispatchProductCreativeDocumentInput(
    const ProductWindowInputFrameContext& context,
    bool creativeDocumentActive) {
  return context.window.gameplay.gameplayActive && context.activeSession.has_value() &&
         !frontendBlocksGameplayInput(context.frontend) &&
         creativeDocumentActive;
}

[[nodiscard]] MouseClick productCreativeUiClickForFrame(
    ProductWindowInputFrameContext& context,
    const ProductCreativeDocumentInputOrchestrationRequest& request) {
  MouseClick creativeUiClick = productWindowClickForHitTest(
      request.click,
      context.sdlWindow,
      context.creativeUiDrawList == nullptr
          ? 1280U
          : context.creativeUiDrawList->virtualWidth,
      context.creativeUiDrawList == nullptr
          ? 720U
          : context.creativeUiDrawList->virtualHeight);
  // Branch-gate: BG-1029. Surfaces visually above gameplay own the click even
  // when the pointer misses a specific row, and a row hit consumes before the
  // Creative overlay can route a hidden/behind click.
  if (request.frontendMouseOwnsInput || request.higherPriorityMouseConsumed) {
    creativeUiClick.clicked = false;
  }
  return creativeUiClick;
}

[[nodiscard]] ProductCreativeDocumentRevisionPhaseState
beginProductCreativeDocumentRevisionPhase(
    const ProductWindowInputFrameContext& context) {
  ProductCreativeDocumentRevisionPhaseState phase;
  phase.revisionBefore =
      captureProductCreativeDocumentRevision(context.creativeApp);
  if (context.creativeApp != nullptr &&
      context.creativeApp->facade.document().isValid() &&
      context.creativeApp->facade.document().id() !=
          creative::kInvalidDocumentId) {
    phase.undoSnapshotBefore = context.creativeApp->facade.document();
    phase.undoSnapshotBeforeAvailable = true;
  }
  return phase;
}

ProductCreativeUiCommandPhaseResult processProductCreativeUiCommandPhase(
    ProductWindowInputFrameContext& context,
    MouseClick creativeUiClick) {
  ProductCreativeUiCommandPhaseResult result;
  const ProductCreativeUiInputFrameReceipt creativeUiInputReceipt =
      routeProductCreativeUiInputFrame(ProductCreativeUiInputFrameRequest{
          context.creativeUiDrawList,
          creativeUiClick,
      });
  recordProductCreativeUiInputFrame(context.window, creativeUiInputReceipt);
  result.inputReceipt = creativeUiInputReceipt;

  const ProductCreativeUiCommandFrameReceipt creativeUiCommandReceipt =
      routeProductCreativeUiCommandFrame(ProductCreativeUiCommandFrameRequest{
          context.creativeApp,
          creativeUiInputReceipt,
      });
  recordProductCreativeUiCommandFrame(context.window, creativeUiCommandReceipt);
  result.commandReceipt = creativeUiCommandReceipt;

  if (creativeUiCommandReceipt.commandKind ==
          ProductCreativeUiCommandKind::RebuildRoom &&
      context.creativeApp != nullptr) {
    ProductCreativeBakedActiveRoomRefreshRequest refreshRequest;
    refreshRequest.clearOnNoRenderable = true;
    const ProductCreativeBakedActiveRoomRefreshResult refresh =
        refreshProductCreativeBakedActiveRoom(refreshRequest,
                                             context.activeSession,
                                             context.window,
                                             *context.creativeApp);
    recordProductCreativeUiBakedRoomRefresh(context.window, refresh);
  }
  return result;
}

ProductCreativeViewportInputPhaseResult processProductCreativeViewportInputPhase(
    ProductWindowInputFrameContext& context,
    const ProductCreativeDocumentInputOrchestrationRequest& request) {
  ProductCreativeViewportInputPhaseResult result;
  result.downstreamClick = request.click;
  const ProductCreativeUiDownstreamClickReceipt downstreamClickReceipt =
      routeProductCreativeUiDownstreamClick(
          ProductCreativeUiDownstreamClickRequest{
              request.click,
          context.window.creativeAuthoring.creativeUiInput.consumed,
          request.higherPriorityMouseConsumed ||
              request.frontendMouseOwnsInput,
          });
  recordProductCreativeUiDownstreamClick(context.window,
                                         downstreamClickReceipt);
  result.downstreamClick = downstreamClickReceipt.downstreamClick;

  const ProductCreativeViewportPickFrameReceipt viewportPickReceipt =
      routeProductCreativeViewportPickFrame(
          ProductCreativeViewportPickFrameRequest{
              &context.window,
              context.creativeApp,
              result.downstreamClick,
              downstreamClickReceipt.suppressed,
              context.creativeViewportPickViewport,
              context.creativeViewportPickProjectionRequest,
              context.creativeViewportPickZ,
              context.creativeViewportPickDepthMode,
          });
  recordProductCreativeViewportPickFrame(context.window, viewportPickReceipt);
  if (viewportPickReceipt.picked) {
    result.pointerTarget = viewportPickReceipt.target;
  }
  return result;
}

struct ProductCreativePointerLifecyclePhaseResult {
  ProductCreativePointerLifecycleEvent lifecycle;
  creative::TargetRef lifecycleTarget;
};

ProductCreativePointerLifecyclePhaseResult
processProductCreativePointerLifecyclePhase(
    ProductWindowInputFrameContext& context,
    MouseClick click,
    const KeyboardCreativeToolKeyPresses& creativeToolKeys) {
  ProductCreativePointerLifecyclePhaseResult result;

  // TL-3 pointer gesture lifecycle. Press flows through the pick chain via
  // `downstreamClick`; here we synthesize the Move (held drag) and Release
  // (gesture END) from raw held-button state. Automation drives clicks through
  // the override socket and does not update the raw button state, so the
  // real-mouse lifecycle stays inert under an override (TV1-K owns the
  // injected-gesture channel). A tool switch this frame resets the held-state
  // first, so a button held across a switch cannot fire a phantom Release into
  // the newly-selected tool. Coordinates are raw window-pixel space — the SAME
  // space the pick/press already consume.
  creative::Tool toolKeyTarget = creative::Tool::Select;
  if (productCreativeToolKeyTarget(creativeToolKeys, toolKeyTarget)) {
    resetProductCreativePointerLifecycle(
        context.inputFrame.creativePointerLifecycle);
  }
  // branch-gate: BG-1029
  if (context.clickOverride.pointerLifecycle.enabled) {
    // Injected-gesture channel (test/automation): a scripted pointer sample
    // drives the SAME raw resolver so the window-plumbing lifecycle
    // (press-hold-release) is reachable headless, where there is no real SDL
    // mouse. A real mouse under an override stays inert (no lifecycle); only
    // an explicit scripted sample drives this.
    const ProductCreativePointerSample pointerSample{
        context.clickOverride.pointerLifecycle.primaryButtonDown,
        context.clickOverride.pointerLifecycle.x,
        context.clickOverride.pointerLifecycle.y};
    result.lifecycle = resolveProductCreativePointerLifecycle(
        context.inputFrame.creativePointerLifecycle, pointerSample);
  } else if (!context.clickOverride.enabled) {
    const ProductCreativePointerSample pointerSample{
        context.inputFrame.mouse.leftWasDown,
        click.x,
        click.y};
    result.lifecycle = resolveProductCreativePointerLifecycle(
        context.inputFrame.creativePointerLifecycle, pointerSample);
  }

  // TV1-G: a Move-drag Move/Release carries the destination anchor. Reuse the
  // pick's pointer->grid-cell conversion (TD-7: the drag plane is the SCREEN
  // plane, world XY; Z is resolved by the facade from the start anchor). The
  // dragged object is the facade's active drag target (filled on the Press) —
  // this is the pointerLifecycleTarget seam.
  const bool lifecycleCarriesGesture =
      result.lifecycle.phase == ProductCreativePointerLifecyclePhase::Move ||
      result.lifecycle.phase == ProductCreativePointerLifecyclePhase::Release;
  if (lifecycleCarriesGesture && context.creativeApp != nullptr) {
    const creative::CreativeToolState& toolState =
        context.creativeApp->facade.toolState();
    if (toolState.moveDragActive) {
      result.lifecycleTarget = toolState.moveDragTarget;
      const creative::CreativeGridCoord3 coord =
          creative::pointerToCreativeGridCoord(
              context.creativeViewportPickViewport,
              context.creativeViewportPickProjectionRequest.gridSize,
              result.lifecycle.x,
              result.lifecycle.y,
              context.creativeViewportPickZ);
      // TD-7: the creative viewport is a FRONT view (screen = world XY), so the
      // drag axes must match the projection for the object to track the cursor:
      // `coord.x` is world X (screen-horizontal) and `coord.y` is world Y
      // (screen-vertical). The held axis is DEPTH — world Z — which the facade
      // overrides with the object's start anchor. The `.z` field below is a
      // placeholder the facade replaces with the start anchor Z.
      result.lifecycle.hasWorldDestination = true;
      result.lifecycle.worldDestination = {
          static_cast<double>(coord.x),
          static_cast<double>(coord.y),
          0.0,
      };
    }
  }
  return result;
}

void processProductCreativeNavigateFlyPhase(
    ProductWindowInputFrameContext& context) {
  // TV1-H (TD-8): Navigate = fly camera, gated STRICTLY to the active tool being
  // Navigate in creative document mode. A tool-key press this frame has already
  // landed in the facade above, so we read the RESULT here. When Navigate is
  // active, WASD + Space/Ctrl + Shift drive the fly kernel (the SAME
  // applyProductCreativeFlyInput map_maker uses) and relative mouse motion
  // drives look; the mouse-capture policy re-engages relative capture for the
  // look (keyed on the mirror flag below). For every other tool the fly stays
  // dead and the free cursor drives UI/pick — movement is enabled ONLY here,
  // and only the fly-movement keys, never general gameplay keyboard input.
  const bool navigateActive =
      context.creativeApp != nullptr &&
      context.creativeApp->facade.toolState().activeTool ==
          creative::Tool::Navigate;
  context.window.creativeAuthoring.creativeNavigateActive = navigateActive;
  if (!navigateActive) {
    return;
  }

  // Seed the fly anchor from the current view on entering Navigate so the camera
  // starts where the fixed first-person anchor already is.
  ensureFreshCreativeFlyAnchor(context.window, &*context.activeSession);
  ActionState flyActions;
  pollKeyboardCreativeFlyActions(flyActions);
  pollMouseGameplayActions(context.inputFrame.mouse, flyActions);
  // Mouse-look first so the fly moves relative to the updated heading.
  applyProductCameraActions(flyActions,
                            context.window.viewport,
                            context.settings,
                            "creative_navigate");
  (void)applyProductWindowCreativeFlyActions(
      context.window, &*context.activeSession, flyActions);
}

bool processProductCreativeDocumentToolDispatchPhase(
    ProductWindowInputFrameContext& context,
    const ProductCreativeViewportInputPhaseResult& viewportInput) {
  // Keyboard stays dead for gameplay in creative document mode; the only keys
  // polled are the four direct tool keys (TL-2, keys 1/2/3/4).
  const KeyboardCreativeToolKeyPresses creativeToolKeys =
      pollKeyboardCreativeToolKeys(context.inputFrame.keyboard);
  const ProductCreativePointerLifecyclePhaseResult lifecycle =
      processProductCreativePointerLifecyclePhase(
          context, viewportInput.downstreamClick, creativeToolKeys);

  ActionState gameplayActions;
  ProductCreativeInputActionsRequest creativeRequest;
  creativeRequest.window = &context.window;
  creativeRequest.creative = context.creativeApp;
  creativeRequest.actions = &gameplayActions;
  creativeRequest.toolKeys = creativeToolKeys;
  creativeRequest.click = viewportInput.downstreamClick;
  creativeRequest.pointerTarget = viewportInput.pointerTarget;
  creativeRequest.pointerLifecycle = lifecycle.lifecycle;
  creativeRequest.pointerLifecycleTarget = lifecycle.lifecycleTarget;
  (void)processProductCreativeInputActions(creativeRequest);

  processProductCreativeNavigateFlyPhase(context);
  return true;
}

void finalizeProductCreativeDocumentInputPhase(
    ProductWindowInputFrameContext& context,
    bool creativeDocumentActive,
    const ProductCreativeUiCommandFrameReceipt& creativeUiCommandReceipt,
    const ProductCreativeDocumentRevisionPhaseState& revisionPhase) {
  const ProductCreativeDocumentRevisionSnapshot creativeRevisionAfter =
      captureProductCreativeDocumentRevision(context.creativeApp);
  recordProductCreativeDocumentRevisionFrame(
      context.window,
      revisionPhase.revisionBefore.observed && creativeRevisionAfter.observed,
      revisionPhase.revisionBefore.documentId,
      revisionPhase.revisionBefore.revision,
      creativeRevisionAfter.documentId,
      creativeRevisionAfter.revision);
  const bool creativeDocumentChanged =
      revisionPhase.revisionBefore.observed && creativeRevisionAfter.observed &&
      (revisionPhase.revisionBefore.documentId !=
           creativeRevisionAfter.documentId ||
       revisionPhase.revisionBefore.revision != creativeRevisionAfter.revision);
  const bool creativeDocumentRevisionChangedForUndo =
      revisionPhase.revisionBefore.observed && creativeRevisionAfter.observed &&
      revisionPhase.revisionBefore.documentId ==
          creativeRevisionAfter.documentId &&
      revisionPhase.revisionBefore.revision != creativeRevisionAfter.revision;
  const bool undoCommandApplied =
      creativeUiCommandReceipt.commandKind ==
          ProductCreativeUiCommandKind::UndoLastDocumentChange &&
      creativeUiCommandReceipt.accepted && creativeUiCommandReceipt.changed;
  if (creativeDocumentRevisionChangedForUndo && !undoCommandApplied &&
      revisionPhase.undoSnapshotBeforeAvailable &&
      context.creativeApp != nullptr) {
    creative::pushCreativeUndoSnapshot(context.creativeApp->undoStack,
                                       revisionPhase.undoSnapshotBefore);
  }
  if (creativeDocumentChanged && context.creativeApp != nullptr) {
    ProductCreativeBakedActiveRoomRefreshRequest refreshRequest;
    refreshRequest.clearOnNoRenderable = true;
    const ProductCreativeBakedActiveRoomRefreshResult refresh =
        refreshProductCreativeBakedActiveRoom(refreshRequest,
                                             context.activeSession,
                                             context.window,
                                             *context.creativeApp);
    recordProductCreativeBakedRoomAutoRefresh(context.window, refresh);
  }
  if (context.creativeApp != nullptr) {
    context.window.creativeAuthoring.creativeUndo.available =
        creative::creativeUndoAvailable(context.creativeApp->undoStack);
    context.window.creativeAuthoring.creativeUndo.depth =
        creative::creativeUndoDepth(context.creativeApp->undoStack);
  } else {
    context.window.creativeAuthoring.creativeUndo.available = false;
    context.window.creativeAuthoring.creativeUndo.depth = 0;
  }

  // If the creative-document dispatch path did not run this frame (frontend
  // menu open over the world, pause, no session), drop any held-state so a
  // gesture interrupted mid-drag cannot fire a phantom Release on resume. Also
  // clear the Navigate mirror (TV1-H) so a pause over a Navigate world releases
  // capture rather than staying captured behind the menu.
  if (!creativeDocumentActive || !context.window.gameplay.gameplayActive ||
      !context.activeSession.has_value() ||
      frontendBlocksGameplayInput(context.frontend)) {
    resetProductCreativePointerLifecycle(
        context.inputFrame.creativePointerLifecycle);
    context.window.creativeAuthoring.creativeNavigateActive = false;
  }
}

ProductCreativeDocumentInputOrchestrationResult
processProductCreativeDocumentInputOrchestration(
    ProductCreativeDocumentInputOrchestrationRequest request) {
  ProductWindowInputFrameContext& context = request.context;
  ProductCreativeDocumentInputOrchestrationResult result;
  result.downstreamClick = request.click;
  result.creativeDocumentActive =
      productCreativeDocumentEditorActiveForSource(context.window,
                                                   context.creativeApp);

  const ProductCreativeDocumentRevisionPhaseState revisionPhase =
      beginProductCreativeDocumentRevisionPhase(context);
  const MouseClick creativeUiClick =
      productCreativeUiClickForFrame(context, request);
  const ProductCreativeUiCommandPhaseResult commandPhase =
      processProductCreativeUiCommandPhase(context, creativeUiClick);
  const ProductCreativeViewportInputPhaseResult viewportInput =
      processProductCreativeViewportInputPhase(context, request);
  result.downstreamClick = viewportInput.downstreamClick;
  result.pointerTarget = viewportInput.pointerTarget;

  if (canDispatchProductCreativeDocumentInput(context,
                                              result.creativeDocumentActive)) {
    result.creativeDocumentInputHandled =
        processProductCreativeDocumentToolDispatchPhase(context, viewportInput);
  }

  finalizeProductCreativeDocumentInputPhase(context,
                                            result.creativeDocumentActive,
                                            commandPhase.commandReceipt,
                                            revisionPhase);

  return result;
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
