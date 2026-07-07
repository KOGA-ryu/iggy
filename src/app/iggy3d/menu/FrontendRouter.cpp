#include "app/iggy3d/menu/FrontendRouter.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"

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

ProductFrontendSurface surfaceForParentOwner(MenuOwner owner) {
  // branch-gate: BG-1058
  switch (owner) {
    case MenuOwner::Starter:
      return ProductFrontendSurface::Starter;
    case MenuOwner::Pause:
      return ProductFrontendSurface::Pause;
    case MenuOwner::Settings:
      return ProductFrontendSurface::Settings;
    case MenuOwner::DevTools:
      return ProductFrontendSurface::DevTools;
    case MenuOwner::Editor:
      return ProductFrontendSurface::Editor;
    case MenuOwner::Gameplay:
      return ProductFrontendSurface::Gameplay;
    case MenuOwner::None:
      return ProductFrontendSurface::None;
  }
  return ProductFrontendSurface::None;
}

bool mapMakerExplicitlyEnabled(const ProductAppWindowState& window) {
  return window.mapMakerStatus == "map_maker_enabled" ||
         window.mapMakerStatus == "map_maker_active";
}

ProductInputSurface inputSurfaceForActiveSurface(
    const ProductActiveSurfaceContext& context,
    ProductFrontendSurface surface) {
  // branch-gate: BG-1058
  switch (surface) {
    case ProductFrontendSurface::BootStatus:
    case ProductFrontendSurface::None:
      return ProductInputSurface::None;
    case ProductFrontendSurface::WorldSetup:
      return ProductInputSurface::WorldSetup;
    case ProductFrontendSurface::SaveSelector:
      return ProductInputSurface::SaveBrowser;
    case ProductFrontendSurface::ConfirmDialog:
      // branch-gate: BG-1058
      if (context.frontend.screen == FrontendScreen::DeleteConfirm ||
          context.frontend.childScreen == FrontendScreen::DeleteConfirm) {
        return ProductInputSurface::SaveBrowser;
      }
      return ProductInputSurface::Starter;
    case ProductFrontendSurface::Settings:
      return ProductInputSurface::Settings;
    case ProductFrontendSurface::DevTools:
      return ProductInputSurface::DevTools;
    case ProductFrontendSurface::Pause:
      return ProductInputSurface::Pause;
    case ProductFrontendSurface::Starter:
      return ProductInputSurface::Starter;
    case ProductFrontendSurface::Gameplay:
      return ProductInputSurface::Gameplay;
    case ProductFrontendSurface::Editor:
      return ProductInputSurface::RoomEditor;
  }
  return ProductInputSurface::None;
}

void recordAllowedActionGroups(ProductActiveSurfaceFrame& frame) {
  frame.acceptsSystemActions = true;
  // branch-gate: BG-1058
  switch (frame.inputOwner) {
    case MenuOwner::Starter:
    case MenuOwner::Pause:
    case MenuOwner::Settings:
    case MenuOwner::DevTools:
      frame.acceptsMenuActions = true;
      return;
    case MenuOwner::Editor:
      frame.acceptsMenuActions = true;
      frame.acceptsEditorActions = true;
      return;
    case MenuOwner::Gameplay:
      frame.acceptsPlayerActions = true;
      return;
    case MenuOwner::None:
      return;
  }
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

ProductFrontendOwnerDecision pauseChildDecision(FrontendScreen childScreen) {
  switch (childScreen) {
    case FrontendScreen::LoadSave:
      return ownerDecision(MenuOwner::Pause,
                           ProductFrontendSurface::SaveSelector,
                           MenuOwner::Pause,
                           true);
    case FrontendScreen::DeleteConfirm:
      return ownerDecision(MenuOwner::Pause,
                           ProductFrontendSurface::ConfirmDialog,
                           MenuOwner::Pause,
                           true);
    case FrontendScreen::Gameplay:
      return ownerDecision(MenuOwner::Pause,
                           ProductFrontendSurface::Pause,
                           MenuOwner::None,
                           true);
    case FrontendScreen::BootStatus:
    case FrontendScreen::Starter:
    case FrontendScreen::NewWorld:
    case FrontendScreen::Settings:
    case FrontendScreen::StarterDevTools:
    case FrontendScreen::Pause:
    case FrontendScreen::DevOverlay:
    case FrontendScreen::ExitConfirm:
      break;
  }
  return ownerDecision(MenuOwner::Pause,
                       ProductFrontendSurface::Pause,
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

std::string_view productActiveMouseCapturePolicyName(
    ProductActiveMouseCapturePolicy policy) {
  // branch-gate: BG-1058
  switch (policy) {
    case ProductActiveMouseCapturePolicy::Released:
      return "released";
    case ProductActiveMouseCapturePolicy::RelativeGameplay:
      return "relative_gameplay";
  }
  return "released";
}

std::string_view productCreativeSurfaceKindName(
    ProductCreativeSurfaceKind kind) {
  // branch-gate: BG-1058
  switch (kind) {
    case ProductCreativeSurfaceKind::None:
      return "none";
    case ProductCreativeSurfaceKind::LegacyMapMaker:
      return "legacy_map_maker";
    case ProductCreativeSurfaceKind::CreativeDocument:
      return "creative_document";
  }
  return "none";
}

ProductActiveSurfaceContext productActiveSurfaceContextForWindow(
    const FrontendState& frontend,
    const ProductAppWindowState& window) {
  ProductActiveSurfaceContext context;
  context.frontend = frontend;
  context.gameplayActive = window.gameplayActive;
  context.hasActiveSession = window.gameplayActive;
  context.roomEditorReady =
      window.roomEditing.ready && !productCreativeWorldActiveForWindow(window);
  context.interactionMode = window.interactionMode;
  return context;
}

ProductActiveSurfaceFrame resolveProductActiveSurface(
    const ProductActiveSurfaceContext& context) {
  ProductFrontendRouteContext routeContext;
  routeContext.frontend = context.frontend;
  routeContext.gameplayActive = context.gameplayActive;
  routeContext.hasActiveSession = context.hasActiveSession;

  const ProductFrontendOwnerDecision owner =
      chooseProductFrontendOwner(routeContext);

  ProductActiveSurfaceFrame frame;
  frame.activeSurface = owner.activeSurface;
  frame.parentOwner = owner.parentOwner;
  frame.parentSurface = surfaceForParentOwner(owner.parentOwner);
  frame.inputOwner = owner.inputOwner;
  frame.gameplayInputSuppressed = owner.gameplayInputSuppressed;
  frame.status = owner.status;

  // branch-gate: BG-1058
  if (frame.activeSurface == ProductFrontendSurface::Gameplay &&
      frame.inputOwner == MenuOwner::Gameplay && context.roomEditorReady) {
    frame.activeSurface = ProductFrontendSurface::Editor;
    frame.parentOwner = MenuOwner::Gameplay;
    frame.parentSurface = ProductFrontendSurface::Gameplay;
    frame.inputOwner = MenuOwner::Editor;
    frame.gameplayInputSuppressed = true;
    frame.status = "product_active_surface_editor";
  }

  frame.inputSurface = inputSurfaceForActiveSurface(context, frame.activeSurface);
  // branch-gate: BG-1058
  if (menuOwnerBlocksGameplay(frame.inputOwner)) {
    frame.gameplayInputSuppressed = true;
  }
  // branch-gate: BG-1058
  if (context.gameplayActive && frame.inputOwner == MenuOwner::Gameplay &&
      !frame.gameplayInputSuppressed &&
      context.interactionMode == ProductInteractionMode::Player) {
    frame.mouseCapturePolicy =
        ProductActiveMouseCapturePolicy::RelativeGameplay;
  }
  recordAllowedActionGroups(frame);
  return frame;
}

bool productMapMakerLiveForWindow(const FrontendState& frontend,
                                  const ProductAppWindowState& window) {
  return productCreativeSurfaceKindForWindow(frontend, window) ==
         ProductCreativeSurfaceKind::LegacyMapMaker;
}

bool productMapMakerLiveForSource(const FrontendState& frontend,
                                  const ProductAppWindowState& window,
                                  const creative::CreativeAppState* creativeApp) {
  return productCreativeSurfaceKindForSource(frontend, window, creativeApp) ==
         ProductCreativeSurfaceKind::LegacyMapMaker;
}

bool productCreativeWorldActiveForWindow(const ProductAppWindowState& window) {
  (void)window;
  return false;
}

bool productCreativeWorldActiveForIdentity(
    const creative::CreativeActiveIdentity& identity) {
  return identity.worldActive();
}

bool productCreativeWorldActiveForSource(
    const ProductAppWindowState& window,
    const creative::CreativeAppState* creativeApp) {
  if (creativeApp != nullptr) {
    return productCreativeWorldActiveForIdentity(creativeApp->identity);
  }
  return productCreativeWorldActiveForWindow(window);
}

bool productCreativeDocumentEditorActiveForWindow(
    const ProductAppWindowState& window) {
  return window.interactionMode == ProductInteractionMode::Creative &&
         !window.roomEditing.ready && !mapMakerExplicitlyEnabled(window);
}

bool productCreativeDocumentEditorActiveForSource(
    const ProductAppWindowState& window,
    const creative::CreativeAppState* creativeApp) {
  if (creativeApp != nullptr) {
    return window.interactionMode == ProductInteractionMode::Creative &&
           productCreativeWorldActiveForIdentity(creativeApp->identity);
  }
  return productCreativeDocumentEditorActiveForWindow(window);
}

ProductCreativeSurfaceKind productCreativeSurfaceKindForWindow(
    const FrontendState& frontend,
    const ProductAppWindowState& window) {
  const ProductActiveSurfaceFrame surface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(frontend, window));
  if (window.interactionMode == ProductInteractionMode::Creative &&
      !window.roomEditing.ready &&
      mapMakerExplicitlyEnabled(window) &&
      surface.activeSurface == ProductFrontendSurface::Gameplay &&
      surface.inputOwner == MenuOwner::Gameplay &&
      !surface.gameplayInputSuppressed) {
    return ProductCreativeSurfaceKind::LegacyMapMaker;
  }

  return ProductCreativeSurfaceKind::None;
}

ProductCreativeSurfaceKind productCreativeSurfaceKindForSource(
    const FrontendState& frontend,
    const ProductAppWindowState& window,
    const creative::CreativeAppState* creativeApp) {
  if (productCreativeDocumentEditorActiveForSource(window, creativeApp)) {
    return ProductCreativeSurfaceKind::CreativeDocument;
  }
  return productCreativeSurfaceKindForWindow(frontend, window);
}

ProductActiveSurfaceFrame syncProductWindowInputOwnerFromActiveSurface(
    const FrontendState& frontend,
    ProductAppWindowState& window) {
  const ProductActiveSurfaceFrame surface = resolveProductActiveSurface(
      productActiveSurfaceContextForWindow(frontend, window));
  return surface;
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
    return pauseChildDecision(frontend.childScreen);
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
