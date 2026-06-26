#include "app/iggy3d/ProductWindowInputFrame.hpp"

#include "app/iggy3d/OpeningMenuView.hpp"
#include "app/iggy3d/ProductActiveRoomCollision.hpp"
#include "app/iggy3d/ProductCameraController.hpp"
#include "app/iggy3d/ProductGameplayController.hpp"
#include "app/iggy3d/ProductRoomEditorActionController.hpp"
#include "app/iggy3d/product/Automation.hpp"
#include "app/iggy3d/product/AutomationRoomEditing.hpp"
#include "app/iggy3d/product/ProductMenuInputRouter.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputRouter.hpp"

namespace iggy3d {

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
      pollGamepadRoomEditorActions(context.inputFrame.gamepad, gameplayActions);
    } else {
      pollKeyboardGameplayActions(context.inputFrame.keyboard, gameplayActions);
      pollGamepadGameplayActions(context.inputFrame.gamepad, gameplayActions);
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
      const ProductRoomEditorActionResult result =
          applyProductRoomEditorActions(context.window.roomEditing,
                                        context.window.roomEditorCursor,
                                        acceptedEditorActions,
                                        ProductRoomAuthoringInputSource::Hotkey);
      recordProductRoomEditorActionResult(context.window, result);
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
