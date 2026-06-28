#include "app/iggy3d/menu/InputRouter.hpp"

#include "app/iggy3d/menu/ActionHandlers.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"

#include <array>

namespace iggy3d {
namespace {

using OpeningMenuDispatchPredicate = bool (*)(
    const ProductOpeningMenuInputContext&);
using OpeningMenuDispatchHandler = void (*)(InputAction,
                                            ProductOpeningMenuInputContext);

struct OpeningMenuDispatchRow {
  OpeningMenuDispatchPredicate matches = nullptr;
  OpeningMenuDispatchHandler handler = nullptr;
};

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

bool dispatchMatchesPause(const ProductOpeningMenuInputContext& context) {
  return context.frontend.screen == FrontendScreen::Pause;
}

bool dispatchMatchesDevOverlay(const ProductOpeningMenuInputContext& context) {
  return context.frontend.screen == FrontendScreen::DevOverlay;
}

bool dispatchMatchesStarterDevTools(
    const ProductOpeningMenuInputContext& context) {
  return context.frontend.childScreen == FrontendScreen::StarterDevTools;
}

bool dispatchMatchesSettings(const ProductOpeningMenuInputContext& context) {
  const ProductActiveSurfaceFrame surface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(context.frontend, context.window));
  return surface.activeSurface == ProductFrontendSurface::Settings &&
         surface.inputOwner == MenuOwner::Settings;
}

bool dispatchMatchesDeleteConfirm(
    const ProductOpeningMenuInputContext& context) {
  return context.frontend.childScreen == FrontendScreen::DeleteConfirm &&
         context.window.saveDeleteConfirmationOpen;
}

bool dispatchMatchesNewWorld(const ProductOpeningMenuInputContext& context) {
  return context.frontend.childScreen == FrontendScreen::NewWorld;
}

bool dispatchMatchesLoadSave(const ProductOpeningMenuInputContext& context) {
  return context.frontend.childScreen == FrontendScreen::LoadSave;
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

  static constexpr std::array kDispatchRows{
      OpeningMenuDispatchRow{dispatchMatchesPause, dispatchPauseAction},
      OpeningMenuDispatchRow{dispatchMatchesDevOverlay, dispatchDevOverlayAction},
      OpeningMenuDispatchRow{dispatchMatchesStarterDevTools,
                             dispatchStarterDevToolsAction},
      OpeningMenuDispatchRow{dispatchMatchesSettings, dispatchSettingsAction},
      OpeningMenuDispatchRow{dispatchMatchesDeleteConfirm,
                             dispatchDeleteConfirmAction},
      OpeningMenuDispatchRow{dispatchMatchesNewWorld, dispatchNewWorldAction},
      OpeningMenuDispatchRow{dispatchMatchesLoadSave, dispatchLoadSaveAction},
  };

  for (const OpeningMenuDispatchRow& row : kDispatchRows) {
    if (row.matches(context)) {
      row.handler(action, context);
      return;
    }
  }

  dispatchStarterAction(action, context);
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

  const ProductActiveSurfaceFrame surface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(context.frontend, context.window));
  InputRoutingContext routingContext;
  routingContext.owners = menuOwnerStateForActiveOwner(surface.inputOwner);
  const InputRoutingResult routed = routeInputAction(routingContext, inputAction);
  context.window.inputOwner = routed.owner;
  context.window.lastInputAction = routed.action;
  context.window.lastInputAccepted = routed.accepted;
  context.window.gameplayInputSuppressed =
      surface.gameplayInputSuppressed || routed.gameplaySuppressed;
  // branch-gate: BG-1024
  if (routed.accepted) {
    applyProductOpeningMenuAction(routed.action, context);
  }
}

}  // namespace iggy3d
