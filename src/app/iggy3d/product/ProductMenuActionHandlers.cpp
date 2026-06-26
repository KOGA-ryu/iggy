#include "app/iggy3d/product/ProductMenuActionHandlers.hpp"

#include <array>
#include <cstddef>

#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/ProductMenuTransitions.hpp"
#include "app/iggy3d/ProductRoomAuthoringController.hpp"
#include "app/iggy3d/ProductSaveFlow.hpp"
#include "app/iggy3d/product/AutomationRoomEditing.hpp"

namespace iggy3d {
namespace {

FrontendAction nextPauseSelection(FrontendAction current, InputAction action) {
  const auto& actions = pauseActionOrder();
  std::size_t index = 0;
  // branch-gate: BG-1017
  for (std::size_t i = 0; i < actions.size(); ++i) {
    // branch-gate: BG-1017
    if (actions[i] == current) {
      index = i;
      break;
    }
  }
  // branch-gate: BG-1017
  if (action == InputAction::MenuUp) {
    index = index == 0 ? actions.size() - 1 : index - 1;  // branch-gate: BG-1017
  } else if (action == InputAction::MenuDown) {  // branch-gate: BG-1017
    index = (index + 1) % actions.size();
  }
  return actions[index];
}

ProductMenuActionResult handlePauseConfirm(ProductPauseMenuActionContext& context) {
  FrontendState& frontend = context.frontend;
  ProductAppWindowState& window = context.window;
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::Resume) {
    closeProductOverlayToGameplayTransition(frontend, window);
    return {true, true};
  }
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::EditRoom) {
    const ProductRoomEditingStartResult started =
        startProductRoomAuthoringFromActiveRoom({window.activeRoom});
    recordProductRoomEditingStart(window, started, "pause_edit_room");
    frontend.status =
        started.ok ? "pause_edit_room_requested" : "pause_edit_room_failed";  // branch-gate: BG-1017
    // branch-gate: BG-1017
    if (started.ok) {
      closeProductOverlayToGameplayTransition(frontend, window);
    }
    return {true, started.ok};
  }
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::Settings) {
    openProductPauseSettingsTransition(frontend, window, context.settingsTab);
    return {true, true};
  }
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::DevTools) {
    openProductPauseDevToolsTransition(frontend, window,
                                       FrontendDevToolsCategory::Session);
    return {true, true};
  }
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::Save) {
    executeProductPauseSaveFlow(ProductPauseSaveFlowKind::Save, context.options,
                                frontend, context.activeSession, window);
    return {true, true};
  }
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::SaveAndExit) {
    executeProductPauseSaveFlow(ProductPauseSaveFlowKind::SaveAndExit,
                                context.options, frontend,
                                context.activeSession, window);
    return {true, true};
  }
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::ReturnToTitle) {
    returnProductToTitleTransition(frontend, window);
    context.activeSession.reset();
    return {true, true};
  }
  // branch-gate: BG-1017
  if (frontend.selectedAction == FrontendAction::ExitGame) {
    frontend.status = "pause_exit_game_requested";
    context.closeRequested = true;
    return {true, true};
  }
  frontend.status = "pause_action_selected";
  return {true, true};
}

}  // namespace

ProductMenuActionResult applyProductPauseMenuAction(
    InputAction action,
    ProductPauseMenuActionContext& context) {
  FrontendState& frontend = context.frontend;
  ProductAppWindowState& window = context.window;
  // branch-gate: BG-1017
  if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
    frontend.selectedAction = nextPauseSelection(frontend.selectedAction, action);
    frontend.status = "pause_menu_selection_changed";
    return {true, true};
  }
  // branch-gate: BG-1017
  if (action == InputAction::MenuBack) {
    closeProductOverlayToGameplayTransition(frontend, window);
    return {true, true};
  }
  // branch-gate: BG-1017
  if (action != InputAction::MenuConfirm) {
    return {true, false};
  }
  return handlePauseConfirm(context);
}

}  // namespace iggy3d
