#include "app/iggy3d/menu/InputRouter.hpp"

#include "app/iggy3d/menu/ActionHandlers.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"

namespace iggy3d {
namespace {

MenuOwnerState menuOwnerStateForActiveOwner(MenuOwner owner) {
  MenuOwnerState state;
  // branch-gate: BG-1024
  switch (owner) {
    case MenuOwner::Starter:
      state.starter = true;
      break;
    case MenuOwner::Pause:
      state.pause = true;
      break;
    case MenuOwner::Settings:
      state.settings = true;
      break;
    case MenuOwner::DevTools:
      state.devTools = true;
      break;
    case MenuOwner::Editor:
      state.editor = true;
      break;
    case MenuOwner::Gameplay:
      state.gameplay = true;
      break;
    case MenuOwner::None:
      break;
  }
  return state;
}

void dispatchPauseAction(InputAction action,
                         ProductOpeningMenuInputContext context) {
  ProductPauseMenuActionContext pauseContext{
      context.frontend, context.options, context.settingsTab,
      context.activeSession, context.window, context.closeRequested,
      context.settings};
  (void)applyProductPauseMenuAction(action, pauseContext);
}

void dispatchDevOverlayAction(InputAction action,
                              ProductOpeningMenuInputContext context) {
  (void)applyProductDevOverlayMenuAction(action,
                                         {context.frontend, context.window});
}

void dispatchStarterDevToolsAction(InputAction action,
                                   ProductOpeningMenuInputContext context) {
  (void)applyProductStarterDevToolsMenuAction(
      action, {context.frontend, context.window});
}

void dispatchSettingsAction(InputAction action,
                            ProductOpeningMenuInputContext context) {
  (void)applyProductSettingsMenuAction(
      action, {context.frontend, context.settingsTab, context.window});
}

void dispatchDeleteConfirmAction(InputAction action,
                                 ProductOpeningMenuInputContext context) {
  (void)applyProductDeleteConfirmMenuAction(
      action, {context.frontend, context.options, context.window});
}

void dispatchExitConfirmAction(InputAction action,
                               ProductOpeningMenuInputContext context) {
  // branch-gate: BG-1024
  if (action == InputAction::MenuBack) {
    context.frontend.childScreen = FrontendScreen::Gameplay;
    context.frontend.status = "exit_confirm_cancelled";
    return;
  }
  // branch-gate: BG-1024
  if (action == InputAction::MenuConfirm) {
    context.frontend.status = "opening_menu_exit_requested";
    context.closeRequested = true;
  }
}

void dispatchConfirmDialogAction(const ProductActiveSurfaceFrame& surface,
                                 InputAction action,
                                 ProductOpeningMenuInputContext context) {
  // branch-gate: BG-1024
  if (surface.inputSurface == ProductInputSurface::SaveBrowser &&
      context.window.saveDeleteConfirmationOpen) {
    dispatchDeleteConfirmAction(action, context);
    return;
  }
  // branch-gate: BG-1024
  if (surface.inputSurface == ProductInputSurface::Starter) {
    dispatchExitConfirmAction(action, context);
  }
}

void dispatchNewWorldAction(InputAction action,
                            ProductOpeningMenuInputContext context) {
  ProductNewWorldMenuActionContext newWorldContext{
      context.frontend, context.options, context.activeSession,
      context.worldSetupDraft, context.window};
  (void)applyProductNewWorldMenuAction(action, newWorldContext);
}

void dispatchLoadSaveAction(InputAction action,
                            ProductOpeningMenuInputContext context) {
  ProductLoadSaveMenuActionContext loadSaveContext{
      context.frontend, context.options, context.saves, context.activeSession,
      context.window};
  (void)applyProductLoadSaveMenuAction(action, loadSaveContext);
}

void dispatchStarterAction(InputAction action,
                           ProductOpeningMenuInputContext context) {
  ProductStarterMenuActionContext starterContext{
      context.frontend, context.options, context.saves, context.settingsTab,
      context.activeSession, context.worldSetupDraft, context.window,
      context.closeRequested};
  (void)applyProductStarterMenuAction(action, starterContext);
}

void dispatchOpeningMenuActionForSurface(
    const ProductActiveSurfaceFrame& surface,
    InputAction action,
    ProductOpeningMenuInputContext context) {
  // branch-gate: BG-1024
  switch (surface.activeSurface) {
    case ProductFrontendSurface::Pause:
      dispatchPauseAction(action, context);
      return;
    case ProductFrontendSurface::DevTools:
      // branch-gate: BG-1024
      if (surface.parentOwner == MenuOwner::Starter) {
        dispatchStarterDevToolsAction(action, context);
        return;
      }
      dispatchDevOverlayAction(action, context);
      return;
    case ProductFrontendSurface::Settings:
      dispatchSettingsAction(action, context);
      return;
    case ProductFrontendSurface::ConfirmDialog:
      dispatchConfirmDialogAction(surface, action, context);
      return;
    case ProductFrontendSurface::WorldSetup:
      dispatchNewWorldAction(action, context);
      return;
    case ProductFrontendSurface::SaveSelector:
      dispatchLoadSaveAction(action, context);
      return;
    case ProductFrontendSurface::Starter:
      dispatchStarterAction(action, context);
      return;
    case ProductFrontendSurface::None:
    case ProductFrontendSurface::BootStatus:
    case ProductFrontendSurface::Gameplay:
    case ProductFrontendSurface::Editor:
      return;
  }
}

}  // namespace

MenuOwner productInputOwnerFor(const FrontendState& frontend,
                               const ProductAppWindowState& window) {
  const ProductActiveSurfaceFrame surface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(frontend, window));
  return surface.inputOwner;
}

void applyProductOpeningMenuAction(InputAction action,
                                   ProductOpeningMenuInputContext context) {
  const ProductMenuActionResult systemPause =
      applyProductSystemPauseMenuAction(
          action,
          {context.frontend, context.window, context.closeRequested,
           &context.settings});
  // branch-gate: BG-1024
  if (systemPause.handled) {
    return;
  }

  const ProductActiveSurfaceFrame surface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(context.frontend, context.window));
  dispatchOpeningMenuActionForSurface(surface, action, context);
}

void routeProductOpeningMenuInput(InputAction inputAction,
                                  ActionState& actionState,
                                  ProductOpeningMenuInputContext context) {
  // branch-gate: BG-1024
  if (inputAction == InputAction::None) {
    return;
  }

  const ProductActiveSurfaceFrame surface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(context.frontend, context.window));
  // branch-gate: BG-1024
  if (surface.activeSurface == ProductFrontendSurface::Gameplay &&
      inputAction == InputAction::MenuBack) {
    inputAction = InputAction::SystemPause;
  }

  recordAction(actionState, inputAction, true, true, false, 1.0F);

  InputRoutingContext routingContext;
  routingContext.owners = menuOwnerStateForActiveOwner(surface.inputOwner);
  const InputRoutingResult routed = routeInputAction(routingContext, inputAction);
  context.window.lastInputAction = routed.action;
  context.window.lastInputAccepted = routed.accepted;
  syncProductWindowInputOwnerFromActiveSurface(context.window, surface);
  // branch-gate: BG-1024
  if (routed.accepted) {
    applyProductOpeningMenuAction(routed.action, context);
  }
}

}  // namespace iggy3d
