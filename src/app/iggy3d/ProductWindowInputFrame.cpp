#include "app/iggy3d/ProductWindowInputFrame.hpp"

#include "app/iggy3d/OpeningMenuView.hpp"
#include "app/iggy3d/ProductActiveRoomCollision.hpp"
#include "app/iggy3d/ProductCameraController.hpp"
#include "app/iggy3d/ProductControllerActionRouting.hpp"
#include "app/iggy3d/ProductGameplayController.hpp"
#include "app/iggy3d/ProductInteractionModeState.hpp"
#include "app/iggy3d/ProductRoomEditorActionController.hpp"
#include "app/iggy3d/product/Automation.hpp"
#include "app/iggy3d/product/AutomationRoomEditing.hpp"
#include "app/iggy3d/product/ProductMenuInputRouter.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputRouter.hpp"

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
    ProductWindowInputFrameContext& context,
    const GamepadControllerActionSample& sample,
    bool controllerModeChordRequested,
    ActionState& actions) {
  const ProductInputSurface surface =
      productInputSurfaceFor(context.frontend, context.window);
  ProductControllerActionRoutingResult result =
      productControllerActionRoutingSkipped(
          surface, context.window.interactionMode, "controller_action_chord_consumed");
  if (!controllerModeChordRequested) {  // branch-gate: BG-1059
    result = recordProductControllerMappedActions({
        surface,
        context.window.interactionMode,
        sample,
        context.inputFrame.controllerAction,
        actions,
    });
  }
  recordProductControllerActionRoutingResult(context.window, result);
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

void shutdownProductWindowInputFrameState(ProductWindowInputFrameState& state) {
  shutdownGamepadMenuState(state.gamepad);
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
      pollKeyboardRoomEditorActions(context.inputFrame.keyboard, gameplayActions);
      recordProductWindowControllerActions(
          context, gamepadControllerSample, controllerModeChordRequested,
          gameplayActions);
    } else {
      pollKeyboardGameplayActions(context.inputFrame.keyboard, gameplayActions);
      recordProductWindowControllerActions(
          context, gamepadControllerSample, controllerModeChordRequested,
          gameplayActions);
      pollMouseGameplayActions(context.inputFrame.mouse, gameplayActions);
    }

    ActionState acceptedGameplayActions;
    ActionState acceptedEditorActions;
    InputRoutingContext routingContext;
    routingContext.owners.editor = context.window.roomEditing.ready;
    routingContext.owners.gameplay = true;
    for (const ActionStateEntry& entry : gameplayActions.entries) {
      const InputRoutingResult routed = routeInputAction(routingContext, entry.action);
      context.window.inputOwner = routed.owner;
      context.window.lastInputAction = routed.action;
      context.window.lastInputAccepted = routed.accepted;
      context.window.gameplayInputSuppressed = routed.gameplaySuppressed;
      // branch-gate: BG-1029
      if (routed.accepted && routed.owner == MenuOwner::Editor &&
          inputActionGroup(entry.action) == InputActionGroup::Editor) {
        recordAction(acceptedEditorActions, entry.action, entry.down, entry.pressed,
                     entry.released, entry.value);
      // branch-gate: BG-1029
      } else if (routed.accepted && routed.owner == MenuOwner::Gameplay) {
        recordAction(acceptedGameplayActions, entry.action, entry.down, entry.pressed,
                     entry.released, entry.value);
      }
    }
    // branch-gate: BG-1029
    if (!acceptedEditorActions.entries.empty()) {
      applyProductWindowRoomEditorActions(context.window, acceptedEditorActions);
    } else {
      applyProductCameraActions(acceptedGameplayActions, context.window.viewport,
                                context.settings, "action_map");
      const SpatialSurfaceSet* collisionSurfaces =
          productActiveRoomCollisionSurfaces(context.window.activeRoomCollision);
      applyProductGameplayActions(*context.activeSession, acceptedGameplayActions,
                                  context.window, "action_map", collisionSurfaces);
    }
  }
}

}  // namespace iggy3d
