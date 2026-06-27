#include "app/iggy3d/menu/InputRouter.hpp"

#include "app/iggy3d/menu/ProductMenuActionHandlers.hpp"

namespace iggy3d {

MenuOwner productInputOwnerFor(const FrontendState& frontend,
                               const ProductAppWindowState& window) {
  // branch-gate: BG-1024
  if (frontend.screen == FrontendScreen::Settings ||
      frontend.childScreen == FrontendScreen::Settings) {
    return MenuOwner::Settings;
  }
  // branch-gate: BG-1024
  if (frontend.screen == FrontendScreen::DevOverlay ||
      frontend.childScreen == FrontendScreen::StarterDevTools) {
    return MenuOwner::DevTools;
  }
  // branch-gate: BG-1024
  if (frontend.screen == FrontendScreen::Starter) {
    return MenuOwner::Starter;
  }
  // branch-gate: BG-1024
  if (frontend.screen == FrontendScreen::Pause) {
    return MenuOwner::Pause;
  }
  // branch-gate: BG-1024
  if (frontend.screen == FrontendScreen::Gameplay && window.gameplayActive) {
    // branch-gate: BG-1024
    if (window.roomEditing.ready) {
      return MenuOwner::Editor;
    }
    return MenuOwner::Gameplay;
  }
  return MenuOwner::None;
}

void applyProductOpeningMenuAction(InputAction action,
                                   ProductOpeningMenuInputContext context) {
  const ProductMenuActionResult systemPause =
      applyProductSystemPauseMenuAction(
          action,
          {context.frontend, context.window, context.closeRequested});
  // branch-gate: BG-1024
  if (systemPause.handled) {
    return;
  }

  // branch-gate: BG-1024
  if (context.frontend.screen == FrontendScreen::Pause) {
    ProductPauseMenuActionContext pauseContext{
        context.frontend, context.options, context.settingsTab,
        context.activeSession, context.window, context.closeRequested};
    (void)applyProductPauseMenuAction(action, pauseContext);
    return;
  }
  // branch-gate: BG-1024
  if (context.frontend.screen == FrontendScreen::DevOverlay) {
    return (void)applyProductDevOverlayMenuAction(
               action, {context.frontend, context.window});
  }
  // branch-gate: BG-1024
  if (context.frontend.childScreen == FrontendScreen::StarterDevTools) {
    return (void)applyProductStarterDevToolsMenuAction(
               action, {context.frontend, context.window});
  }
  // branch-gate: BG-1024
  if (context.frontend.childScreen == FrontendScreen::Settings) {
    return (void)applyProductSettingsMenuAction(
               action, {context.frontend, context.settingsTab, context.window});
  }
  // branch-gate: BG-1024
  if (context.frontend.childScreen == FrontendScreen::DeleteConfirm &&
      context.window.saveDeleteConfirmationOpen) {
    return (void)applyProductDeleteConfirmMenuAction(
               action, {context.frontend, context.options, context.window});
  }
  // branch-gate: BG-1024
  if (context.frontend.childScreen == FrontendScreen::NewWorld) {
    ProductNewWorldMenuActionContext newWorldContext{
        context.frontend, context.options, context.activeSession,
        context.worldSetupDraft, context.window};
    (void)applyProductNewWorldMenuAction(action, newWorldContext);
    return;
  }
  // branch-gate: BG-1024
  if (context.frontend.childScreen == FrontendScreen::LoadSave) {
    ProductLoadSaveMenuActionContext loadSaveContext{
        context.frontend, context.options, context.saves, context.activeSession,
        context.window};
    (void)applyProductLoadSaveMenuAction(action, loadSaveContext);
    return;
  }

  ProductStarterMenuActionContext starterContext{
      context.frontend, context.options, context.saves, context.settingsTab,
      context.activeSession, context.worldSetupDraft, context.window,
      context.closeRequested};
  (void)applyProductStarterMenuAction(action, starterContext);
}

void routeProductOpeningMenuInput(InputAction inputAction,
                                  ActionState& actionState,
                                  ProductOpeningMenuInputContext context) {
  // branch-gate: BG-1024
  if (inputAction == InputAction::None) {
    return;
  }

  // branch-gate: BG-1024
  if (context.frontend.screen == FrontendScreen::Gameplay &&
      inputAction == InputAction::MenuBack) {
    inputAction = InputAction::SystemPause;
  }

  recordAction(actionState, inputAction, true, true, false, 1.0F);

  InputRoutingContext routingContext;
  routingContext.owners.starter =
      context.frontend.screen == FrontendScreen::Starter;
  routingContext.owners.pause =
      context.frontend.screen == FrontendScreen::Pause;
  routingContext.owners.settings =
      context.frontend.screen == FrontendScreen::Settings ||
      context.frontend.childScreen == FrontendScreen::Settings;
  routingContext.owners.devTools =
      context.frontend.screen == FrontendScreen::DevOverlay ||
      context.frontend.childScreen == FrontendScreen::StarterDevTools;
  routingContext.owners.gameplay =
      context.frontend.screen == FrontendScreen::Gameplay &&
      context.window.gameplayActive;
  const InputRoutingResult routed = routeInputAction(routingContext, inputAction);
  context.window.inputOwner = routed.owner;
  context.window.lastInputAction = routed.action;
  context.window.lastInputAccepted = routed.accepted;
  context.window.gameplayInputSuppressed = routed.gameplaySuppressed;
  // branch-gate: BG-1024
  if (routed.accepted) {
    applyProductOpeningMenuAction(routed.action, context);
  }
}

}  // namespace iggy3d
