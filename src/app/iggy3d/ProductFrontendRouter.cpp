#include "app/iggy3d/ProductFrontendRouter.hpp"

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

}  // namespace iggy3d
