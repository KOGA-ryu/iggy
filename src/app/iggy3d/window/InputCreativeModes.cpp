#include "app/iggy3d/window/InputFrame.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/creative/BakedActiveRoomRefresh.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/bridge/InputFrame.hpp"
#include "app/iggy3d/creative/bridge/UiCommandFrame.hpp"
#include "app/iggy3d/creative/bridge/UiInputFrame.hpp"
#include "app/iggy3d/creative/bridge/ViewportPickFrame.hpp"
#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/input/InteractionModeState.hpp"
#include "app/iggy3d/automation/Automation.hpp"
#include "app/iggy3d/automation/AutomationRoomEditing.hpp"
#include "app/iggy3d/menu/ActionHandlers.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/room_editor/ActionController.hpp"
#include "app/iggy3d/room_editor/Preview.hpp"
#include "app/iggy3d/view/CameraController.hpp"
#include "app/iggy3d/view/CreativeFlyAnchorStore.hpp"
#include "app/iggy3d/window/InputFrameStages.hpp"
#include "app/input/ActionState.hpp"
#include "app/platform/SdlWindow.hpp"

#include <algorithm>
#include <array>

namespace iggy3d {
namespace {

struct ProductCreativeDocumentRevisionSnapshot {
  bool observed = false;
  std::uint64_t documentId = 0;
  std::uint64_t revision = 0;
};

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

}  // namespace

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

namespace {

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

bool productWindowEditorMousePickSurfaceReady(
    const FrontendState& frontend,
    const ProductAppWindowState& window,
    const creative::CreativeAppState* creativeApp) {
  return frontend.screen == FrontendScreen::Gameplay && window.gameplay.gameplayActive &&
         !frontendBlocksGameplayInput(frontend) &&
         !productCreativeWorldActiveForSource(window, creativeApp);
}

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
  MouseClick creativeUiClick = productWindowMenuClickForHitTest(
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

}  // namespace

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

}  // namespace iggy3d
