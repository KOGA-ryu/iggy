#include "app/iggy3d/ProductFrontendRouter.hpp"

#include "app/frontend/DevToolsMenu.hpp"
#include "app/frontend/PauseMenu.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/frontend/StarterScreen.hpp"

namespace iggy3d {
namespace {

ProductFrontendOwnerDecision ownerDecision(MenuOwner owner,
                                           ProductFrontendSurface surface,
                                           MenuOwner parentOwner,
                                           bool suppressed) {
  ProductFrontendOwnerDecision decision;
  decision.inputOwner = owner;
  decision.activeSurface = surface;
  decision.parentOwner = parentOwner;
  decision.gameplayInputSuppressed = suppressed;
  decision.modelName = productFrontendSurfaceName(surface);
  return decision;
}

ProductFrontendOwnerDecision unavailableGameplayDecision() {
  ProductFrontendOwnerDecision decision;
  decision.inputOwner = MenuOwner::None;
  decision.activeSurface = ProductFrontendSurface::None;
  decision.parentOwner = MenuOwner::None;
  decision.gameplayInputSuppressed = true;
  decision.modelAvailable = false;
  decision.modelName = "none";
  decision.status = "product_frontend_gameplay_unavailable";
  return decision;
}

ProductFrontendOwnerDecision starterChildDecision(FrontendScreen childScreen) {
  switch (childScreen) {
    case FrontendScreen::NewWorld:
      return ownerDecision(MenuOwner::Starter,
                           ProductFrontendSurface::WorldSetup,
                           MenuOwner::Starter,
                           true);
    case FrontendScreen::LoadSave:
      return ownerDecision(MenuOwner::Starter,
                           ProductFrontendSurface::SaveSelector,
                           MenuOwner::Starter,
                           true);
    case FrontendScreen::DeleteConfirm:
    case FrontendScreen::ExitConfirm:
      return ownerDecision(MenuOwner::Starter,
                           ProductFrontendSurface::ConfirmDialog,
                           MenuOwner::Starter,
                           true);
    case FrontendScreen::Settings:
      return ownerDecision(MenuOwner::Settings,
                           ProductFrontendSurface::Settings,
                           MenuOwner::Starter,
                           true);
    case FrontendScreen::StarterDevTools:
      return ownerDecision(MenuOwner::DevTools,
                           ProductFrontendSurface::DevTools,
                           MenuOwner::Starter,
                           true);
    case FrontendScreen::Gameplay:
      return ownerDecision(MenuOwner::Starter,
                           ProductFrontendSurface::Starter,
                           MenuOwner::None,
                           true);
    case FrontendScreen::BootStatus:
    case FrontendScreen::Starter:
    case FrontendScreen::Pause:
    case FrontendScreen::DevOverlay:
      break;
  }
  return ownerDecision(MenuOwner::Starter,
                       ProductFrontendSurface::Starter,
                       MenuOwner::None,
                       true);
}

bool isStarterChildSurface(ProductFrontendSurface surface) {
  return surface == ProductFrontendSurface::WorldSetup ||
         surface == ProductFrontendSurface::SaveSelector ||
         surface == ProductFrontendSurface::ConfirmDialog;
}

FrontendRouteResult unavailableRoute(const ProductFrontendRouteContext& context,
                                     const ProductFrontendOwnerDecision& owner,
                                     FrontendAction action,
                                     std::string_view status) {
  FrontendRouteResult route = makeIgnoredFrontendRouteResult(
      owner.inputOwner,
      context.frontend.screen,
      context.frontend.childScreen,
      action);
  route.gameplayInputSuppressed = owner.gameplayInputSuppressed;
  route.status = status;
  route.receiptReason = status;
  return route;
}

}  // namespace

std::string_view productFrontendSurfaceName(ProductFrontendSurface surface) {
  switch (surface) {
    case ProductFrontendSurface::None:
      return "none";
    case ProductFrontendSurface::BootStatus:
      return "boot_status";
    case ProductFrontendSurface::ConfirmDialog:
      return "confirm_dialog";
    case ProductFrontendSurface::SaveSelector:
      return "save_selector";
    case ProductFrontendSurface::WorldSetup:
      return "world_setup";
    case ProductFrontendSurface::Settings:
      return "settings";
    case ProductFrontendSurface::DevTools:
      return "dev_tools";
    case ProductFrontendSurface::Pause:
      return "pause";
    case ProductFrontendSurface::Starter:
      return "starter";
    case ProductFrontendSurface::Gameplay:
      return "gameplay";
    case ProductFrontendSurface::Editor:
      return "editor";
  }
  return "none";
}

ProductFrontendOwnerDecision chooseProductFrontendOwner(
    const ProductFrontendRouteContext& context) {
  const FrontendState& frontend = context.frontend;

  if (frontend.screen == FrontendScreen::BootStatus) {
    return ownerDecision(MenuOwner::None,
                         ProductFrontendSurface::BootStatus,
                         MenuOwner::None,
                         true);
  }

  if (frontend.screen == FrontendScreen::Starter) {
    return starterChildDecision(frontend.childScreen);
  }

  if (frontend.screen == FrontendScreen::Settings &&
      frontend.childScreen == FrontendScreen::Pause) {
    return ownerDecision(MenuOwner::Settings,
                         ProductFrontendSurface::Settings,
                         MenuOwner::Pause,
                         true);
  }

  if (frontend.screen == FrontendScreen::Pause) {
    return ownerDecision(MenuOwner::Pause,
                         ProductFrontendSurface::Pause,
                         MenuOwner::None,
                         true);
  }

  if (frontend.screen == FrontendScreen::DevOverlay) {
    return ownerDecision(MenuOwner::DevTools,
                         ProductFrontendSurface::DevTools,
                         MenuOwner::Gameplay,
                         true);
  }

  if (frontend.screen == FrontendScreen::Gameplay) {
    if (context.gameplayActive && context.hasActiveSession) {
      return ownerDecision(MenuOwner::Gameplay,
                           ProductFrontendSurface::Gameplay,
                           MenuOwner::None,
                           false);
    }
    return unavailableGameplayDecision();
  }

  ProductFrontendOwnerDecision decision;
  decision.gameplayInputSuppressed = true;
  decision.modelAvailable = false;
  decision.status = "product_frontend_surface_unavailable";
  return decision;
}

ProductFrontendRouteFrame routeProductFrontendAction(
    const ProductFrontendRouteContext& context,
    FrontendAction action) {
  ProductFrontendRouteFrame frame;
  frame.owner = chooseProductFrontendOwner(context);

  if (frame.owner.activeSurface == ProductFrontendSurface::Starter &&
      frame.owner.inputOwner == MenuOwner::Starter) {
    if (context.starterModel == nullptr) {
      frame.route =
          unavailableRoute(context, frame.owner, action, "starter_model_unavailable");
      frame.route.gameplayInputSuppressed = true;
      frame.routeModelAvailable = false;
      frame.routeModelName = "starter";
      return frame;
    }
    frame.route = routeStarterAction(*context.starterModel, action);
    frame.routeModelName = "starter";
    frame.routed = true;
    return frame;
  }

  if (context.frontend.screen == FrontendScreen::Starter &&
      isStarterChildSurface(frame.owner.activeSurface)) {
    if (action == FrontendAction::Back) {
      frame.route = routeStarterBackFromChild(context.frontend.childScreen);
      frame.routeModelName = "starter";
      frame.routed = true;
      return frame;
    }
    frame.route = unavailableRoute(context,
                                   frame.owner,
                                   action,
                                   "product_frontend_child_route_unavailable");
    frame.routeModelName = productFrontendSurfaceName(frame.owner.activeSurface);
    return frame;
  }

  if (frame.owner.activeSurface == ProductFrontendSurface::Settings &&
      frame.owner.inputOwner == MenuOwner::Settings) {
    if (context.settingsContext == nullptr) {
      frame.route =
          unavailableRoute(context, frame.owner, action, "settings_model_unavailable");
      frame.route.gameplayInputSuppressed = true;
      frame.routeModelAvailable = false;
      frame.routeModelName = "settings";
      return frame;
    }
    if (action == FrontendAction::Back) {
      frame.route = routeSettingsBackToParent(context.settingsContext->parentOwner);
    } else {
      frame.route = routeSettingsAction(*context.settingsContext, action);
    }
    frame.routeModelName = "settings";
    frame.routed = true;
    return frame;
  }

  if (frame.owner.activeSurface == ProductFrontendSurface::Pause &&
      frame.owner.inputOwner == MenuOwner::Pause) {
    if (context.pauseModel == nullptr) {
      frame.route =
          unavailableRoute(context, frame.owner, action, "pause_model_unavailable");
      frame.route.gameplayInputSuppressed = true;
      frame.routeModelAvailable = false;
      frame.routeModelName = "pause";
      return frame;
    }
    frame.route = routePauseAction(*context.pauseModel, action);
    frame.routeModelName = "pause";
    frame.routed = true;
    return frame;
  }

  if (frame.owner.activeSurface == ProductFrontendSurface::DevTools &&
      frame.owner.inputOwner == MenuOwner::DevTools) {
    if (context.devToolsModel == nullptr) {
      frame.route =
          unavailableRoute(context, frame.owner, action, "dev_tools_model_unavailable");
      frame.route.gameplayInputSuppressed = true;
      frame.routeModelAvailable = false;
      frame.routeModelName = "dev_tools";
      return frame;
    }
    frame.route =
        routeDevToolsAction(*context.devToolsModel, frame.owner.parentOwner, action);
    frame.routeModelName = "dev_tools";
    frame.routed = true;
    return frame;
  }

  frame.route = unavailableRoute(context,
                                 frame.owner,
                                 action,
                                 "product_frontend_route_unavailable");
  frame.routeModelName = productFrontendSurfaceName(frame.owner.activeSurface);
  return frame;
}

ProductFrontendRouteSummary summarizeProductFrontendRoute(
    const ProductFrontendRouteContext& context,
    const ProductFrontendRouteFrame& frame,
    FrontendAction inputAction) {
  ProductFrontendRouteSummary summary;
  summary.routed = frame.routed;
  summary.accepted = frame.route.accepted;
  summary.inputOwner = menuOwnerName(frame.route.inputOwner);
  summary.activeSurface = productFrontendSurfaceName(frame.owner.activeSurface);
  summary.parentOwner = menuOwnerName(frame.owner.parentOwner);
  summary.inputAction = frontendActionName(inputAction);
  summary.screenBefore = frontendScreenName(context.frontend.screen);
  summary.childBefore = frontendScreenName(context.frontend.childScreen);
  summary.screenAfter = frontendScreenName(frame.route.nextScreen);
  summary.childAfter = frontendScreenName(frame.route.nextChildScreen);
  summary.transition = frontendTransitionRequestName(frame.route.requestedTransition);
  summary.closeRequested = frame.route.closeRequested;
  summary.gameplayInputSuppressed = frame.route.gameplayInputSuppressed;
  summary.ownerModelAvailable = frame.owner.modelAvailable;
  summary.ownerModelName = frame.owner.modelName;
  summary.routeModelAvailable = frame.routeModelAvailable;
  summary.routeModelName = frame.routeModelName;
  summary.status = frame.route.status;
  summary.reason = frame.route.receiptReason;
  return summary;
}

}  // namespace iggy3d
