#include "app/iggy3d/window/InputFrame.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.hpp"
#include "app/iggy3d/gameplay/Controller.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/iggy3d/input/ControllerActionRouting.hpp"
#include "app/iggy3d/input/InteractionModeState.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/menu/InputRouter.hpp"
#include "app/iggy3d/menu/Transitions.hpp"
#include "app/iggy3d/view/CameraController.hpp"
#include "app/iggy3d/window/InputFrameStages.hpp"
#include "app/input/ActionState.hpp"

#include <algorithm>

namespace iggy3d {
namespace {

bool resolvedSurfaceAcceptsGameplayInput(const FrontendState& frontend,
                                         const ProductAppWindowState& window) {
  const ProductActiveSurfaceFrame surface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(frontend, window));
  return window.gameplay.gameplayActive &&
         surface.activeSurface == ProductFrontendSurface::Gameplay &&
         surface.inputOwner == MenuOwner::Gameplay &&
         !surface.gameplayInputSuppressed;
}

}  // namespace

InputAction movementTuningHeldAdjustmentAction(bool leftDown, bool rightDown) {
  // branch-gate: BG-1212
  if (leftDown == rightDown) {
    return InputAction::None;
  }
  // branch-gate: BG-1212
  return leftDown ? InputAction::MenuLeft : InputAction::MenuRight;
}

namespace {

void resetMovementTuningRepeat(ProductMovementTuningRepeatState& repeat) {
  repeat.heldDirection = 0;
  repeat.heldFrames = 0U;
}

}  // namespace

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

namespace {

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

}  // namespace

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

}  // namespace iggy3d
