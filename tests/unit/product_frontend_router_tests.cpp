#include "app/iggy3d/ProductFrontendRouter.hpp"

#include "app/frontend/StarterScreen.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::ProductFrontendRouteContext contextFor(iggy3d::FrontendScreen screen,
                                               iggy3d::FrontendScreen child) {
  iggy3d::ProductFrontendRouteContext context;
  context.frontend.screen = screen;
  context.frontend.childScreen = child;
  return context;
}

iggy3d::ProductFrontendRouteContext starterContextFor(
    iggy3d::FrontendScreen child,
    const iggy3d::StarterScreenModel& model) {
  iggy3d::ProductFrontendRouteContext context =
      contextFor(iggy3d::FrontendScreen::Starter, child);
  context.starterModel = &model;
  return context;
}

bool surfaceNamesAreStable() {
  return expect(iggy3d::productFrontendSurfaceName(
                    iggy3d::ProductFrontendSurface::BootStatus) == "boot_status",
                "boot status name") &&
         expect(iggy3d::productFrontendSurfaceName(
                    iggy3d::ProductFrontendSurface::ConfirmDialog) ==
                    "confirm_dialog",
                "confirm dialog name") &&
         expect(iggy3d::productFrontendSurfaceName(
                    iggy3d::ProductFrontendSurface::SaveSelector) ==
                    "save_selector",
                "save selector name") &&
         expect(iggy3d::productFrontendSurfaceName(
                    iggy3d::ProductFrontendSurface::WorldSetup) ==
                    "world_setup",
                "world setup name") &&
         expect(iggy3d::productFrontendSurfaceName(
                    iggy3d::ProductFrontendSurface::Gameplay) == "gameplay",
                "gameplay name");
}

bool bootStatusSuppressesWithoutOwner() {
  const auto decision = iggy3d::chooseProductFrontendOwner(
      contextFor(iggy3d::FrontendScreen::BootStatus,
                 iggy3d::FrontendScreen::Gameplay));
  return expect(decision.inputOwner == iggy3d::MenuOwner::None, "boot owner") &&
         expect(decision.activeSurface == iggy3d::ProductFrontendSurface::BootStatus,
                "boot surface") &&
         expect(decision.parentOwner == iggy3d::MenuOwner::None, "boot parent") &&
         expect(decision.gameplayInputSuppressed, "boot suppresses gameplay") &&
         expect(decision.modelAvailable, "boot model available") &&
         expect(decision.modelName == "boot_status", "boot model name");
}

bool starterRootOwnsInput() {
  const auto decision = iggy3d::chooseProductFrontendOwner(
      contextFor(iggy3d::FrontendScreen::Starter,
                 iggy3d::FrontendScreen::Gameplay));
  return expect(decision.inputOwner == iggy3d::MenuOwner::Starter,
                "starter owner") &&
         expect(decision.activeSurface == iggy3d::ProductFrontendSurface::Starter,
                "starter surface") &&
         expect(decision.parentOwner == iggy3d::MenuOwner::None,
                "starter parent") &&
         expect(decision.gameplayInputSuppressed, "starter suppresses");
}

bool starterChildrenWinBeforeStarter() {
  const auto newWorld = iggy3d::chooseProductFrontendOwner(
      contextFor(iggy3d::FrontendScreen::Starter,
                 iggy3d::FrontendScreen::NewWorld));
  const auto loadSave = iggy3d::chooseProductFrontendOwner(
      contextFor(iggy3d::FrontendScreen::Starter,
                 iggy3d::FrontendScreen::LoadSave));
  const auto settings = iggy3d::chooseProductFrontendOwner(
      contextFor(iggy3d::FrontendScreen::Starter,
                 iggy3d::FrontendScreen::Settings));
  const auto devTools = iggy3d::chooseProductFrontendOwner(
      contextFor(iggy3d::FrontendScreen::Starter,
                 iggy3d::FrontendScreen::StarterDevTools));
  const auto deleteConfirm = iggy3d::chooseProductFrontendOwner(
      contextFor(iggy3d::FrontendScreen::Starter,
                 iggy3d::FrontendScreen::DeleteConfirm));
  const auto exitConfirm = iggy3d::chooseProductFrontendOwner(
      contextFor(iggy3d::FrontendScreen::Starter,
                 iggy3d::FrontendScreen::ExitConfirm));

  return expect(newWorld.inputOwner == iggy3d::MenuOwner::Starter,
                "new world owner") &&
         expect(newWorld.activeSurface == iggy3d::ProductFrontendSurface::WorldSetup,
                "new world surface") &&
         expect(newWorld.parentOwner == iggy3d::MenuOwner::Starter,
                "new world parent") &&
         expect(loadSave.activeSurface == iggy3d::ProductFrontendSurface::SaveSelector,
                "load save surface") &&
         expect(loadSave.parentOwner == iggy3d::MenuOwner::Starter,
                "load save parent") &&
         expect(settings.inputOwner == iggy3d::MenuOwner::Settings,
                "starter settings owner") &&
         expect(settings.activeSurface == iggy3d::ProductFrontendSurface::Settings,
                "starter settings surface") &&
         expect(settings.parentOwner == iggy3d::MenuOwner::Starter,
                "starter settings parent") &&
         expect(devTools.inputOwner == iggy3d::MenuOwner::DevTools,
                "starter dev tools owner") &&
         expect(devTools.activeSurface == iggy3d::ProductFrontendSurface::DevTools,
                "starter dev tools surface") &&
         expect(deleteConfirm.activeSurface ==
                    iggy3d::ProductFrontendSurface::ConfirmDialog,
                "delete confirm surface") &&
         expect(exitConfirm.activeSurface ==
                    iggy3d::ProductFrontendSurface::ConfirmDialog,
                "exit confirm surface") &&
         expect(exitConfirm.parentOwner == iggy3d::MenuOwner::Starter,
                "confirm parent");
}

bool pauseSettingsAndOverlaysOwnInput() {
  const auto pauseSettings = iggy3d::chooseProductFrontendOwner(
      contextFor(iggy3d::FrontendScreen::Settings,
                 iggy3d::FrontendScreen::Pause));
  const auto pause = iggy3d::chooseProductFrontendOwner(
      contextFor(iggy3d::FrontendScreen::Pause,
                 iggy3d::FrontendScreen::Gameplay));
  const auto dev = iggy3d::chooseProductFrontendOwner(
      contextFor(iggy3d::FrontendScreen::DevOverlay,
                 iggy3d::FrontendScreen::Gameplay));

  return expect(pauseSettings.inputOwner == iggy3d::MenuOwner::Settings,
                "pause settings owner") &&
         expect(pauseSettings.activeSurface ==
                    iggy3d::ProductFrontendSurface::Settings,
                "pause settings surface") &&
         expect(pauseSettings.parentOwner == iggy3d::MenuOwner::Pause,
                "pause settings parent") &&
         expect(pauseSettings.gameplayInputSuppressed,
                "pause settings suppresses") &&
         expect(pause.inputOwner == iggy3d::MenuOwner::Pause, "pause owner") &&
         expect(pause.activeSurface == iggy3d::ProductFrontendSurface::Pause,
                "pause surface") &&
         expect(dev.inputOwner == iggy3d::MenuOwner::DevTools,
                "dev overlay owner") &&
         expect(dev.activeSurface == iggy3d::ProductFrontendSurface::DevTools,
                "dev overlay surface") &&
         expect(dev.parentOwner == iggy3d::MenuOwner::Gameplay,
                "dev overlay parent") &&
         expect(dev.gameplayInputSuppressed, "dev overlay suppresses");
}

bool gameplayRequiresActiveSession() {
  auto active = contextFor(iggy3d::FrontendScreen::Gameplay,
                           iggy3d::FrontendScreen::Gameplay);
  active.gameplayActive = true;
  active.hasActiveSession = true;
  const auto activeDecision = iggy3d::chooseProductFrontendOwner(active);

  auto missing = contextFor(iggy3d::FrontendScreen::Gameplay,
                            iggy3d::FrontendScreen::Gameplay);
  missing.gameplayActive = true;
  missing.hasActiveSession = false;
  const auto missingDecision = iggy3d::chooseProductFrontendOwner(missing);

  return expect(activeDecision.inputOwner == iggy3d::MenuOwner::Gameplay,
                "active gameplay owner") &&
         expect(activeDecision.activeSurface == iggy3d::ProductFrontendSurface::Gameplay,
                "active gameplay surface") &&
         expect(!activeDecision.gameplayInputSuppressed,
                "active gameplay not suppressed") &&
         expect(activeDecision.modelAvailable, "active gameplay model") &&
         expect(missingDecision.inputOwner == iggy3d::MenuOwner::None,
                "missing session owner none") &&
         expect(missingDecision.activeSurface == iggy3d::ProductFrontendSurface::None,
                "missing session surface none") &&
         expect(missingDecision.gameplayInputSuppressed,
                "missing session suppresses") &&
         expect(!missingDecision.modelAvailable,
                "missing session unavailable") &&
         expect(missingDecision.status == "product_frontend_gameplay_unavailable",
                "missing session status");
}

bool starterRootDelegatesNewWorld() {
  const auto model =
      iggy3d::buildStarterScreenModel(0U, iggy3d::FrontendAction::NewWorld);
  const auto frame = iggy3d::routeProductFrontendAction(
      starterContextFor(iggy3d::FrontendScreen::Gameplay, model),
      iggy3d::FrontendAction::NewWorld);

  return expect(frame.routed, "new world routed") &&
         expect(frame.owner.activeSurface == iggy3d::ProductFrontendSurface::Starter,
                "new world owner surface") &&
         expect(frame.route.accepted, "new world accepted") &&
         expect(frame.route.nextScreen == iggy3d::FrontendScreen::Starter,
                "new world next screen") &&
         expect(frame.route.nextChildScreen == iggy3d::FrontendScreen::NewWorld,
                "new world child") &&
         expect(frame.route.status == "starter_new_world_opened",
                "new world status");
}

bool starterRootContinueDelegatesLaunchWhenCompatible() {
  const auto model =
      iggy3d::buildStarterScreenModel(1U, iggy3d::FrontendAction::Continue);
  const auto frame = iggy3d::routeProductFrontendAction(
      starterContextFor(iggy3d::FrontendScreen::Gameplay, model),
      iggy3d::FrontendAction::Continue);

  return expect(frame.routed, "continue routed") &&
         expect(frame.route.accepted, "continue accepted") &&
         expect(frame.route.requestedTransition ==
                    iggy3d::FrontendTransitionRequest::LaunchGameplay,
                "continue launch transition") &&
         expect(frame.route.inputOwner == iggy3d::MenuOwner::Gameplay,
                "continue route owner gameplay") &&
         expect(!frame.route.gameplayInputSuppressed,
                "continue gameplay unsuppressed") &&
         expect(frame.route.status == "starter_launch_continue",
                "continue status");
}

bool starterRootContinueReportsDisabledWithoutCompatibleSave() {
  const auto model =
      iggy3d::buildStarterScreenModel(0U, iggy3d::FrontendAction::Continue);
  const auto frame = iggy3d::routeProductFrontendAction(
      starterContextFor(iggy3d::FrontendScreen::Gameplay, model),
      iggy3d::FrontendAction::Continue);

  return expect(frame.routed, "disabled continue routed") &&
         expect(!frame.route.accepted, "disabled continue not accepted") &&
         expect(frame.route.requestedTransition ==
                    iggy3d::FrontendTransitionRequest::None,
                "disabled continue no transition") &&
         expect(frame.route.status == "no_compatible_save",
                "disabled continue reason");
}

bool starterRootDelegatesSettingsAndDevTools() {
  const auto settingsModel =
      iggy3d::buildStarterScreenModel(0U, iggy3d::FrontendAction::Settings);
  const auto settings = iggy3d::routeProductFrontendAction(
      starterContextFor(iggy3d::FrontendScreen::Gameplay, settingsModel),
      iggy3d::FrontendAction::Settings);

  const auto devModel =
      iggy3d::buildStarterScreenModel(0U, iggy3d::FrontendAction::DevTools);
  const auto devTools = iggy3d::routeProductFrontendAction(
      starterContextFor(iggy3d::FrontendScreen::Gameplay, devModel),
      iggy3d::FrontendAction::DevTools);

  return expect(settings.routed, "settings routed") &&
         expect(settings.route.accepted, "settings accepted") &&
         expect(settings.route.nextChildScreen == iggy3d::FrontendScreen::Settings,
                "settings child") &&
         expect(settings.route.status == "starter_settings_opened",
                "settings status") &&
         expect(devTools.routed, "dev tools routed") &&
         expect(devTools.route.accepted, "dev tools accepted") &&
         expect(devTools.route.nextChildScreen ==
                    iggy3d::FrontendScreen::StarterDevTools,
                "dev tools child") &&
         expect(devTools.route.status == "starter_dev_tools_opened",
                "dev tools status");
}

bool missingStarterModelIsUnavailable() {
  const auto frame = iggy3d::routeProductFrontendAction(
      contextFor(iggy3d::FrontendScreen::Starter,
                 iggy3d::FrontendScreen::Gameplay),
      iggy3d::FrontendAction::NewWorld);

  return expect(!frame.routed, "missing model not routed") &&
         expect(!frame.route.accepted, "missing model not accepted") &&
         expect(frame.owner.activeSurface == iggy3d::ProductFrontendSurface::Starter,
                "missing model owner surface") &&
         expect(frame.route.status == "starter_model_unavailable",
                "missing model status") &&
         expect(frame.route.receiptReason == "starter_model_unavailable",
                "missing model receipt") &&
         expect(frame.route.gameplayInputSuppressed,
                "missing model suppresses gameplay");
}

bool starterChildBackDelegatesToStarterHelper() {
  const auto frame = iggy3d::routeProductFrontendAction(
      contextFor(iggy3d::FrontendScreen::Starter,
                 iggy3d::FrontendScreen::NewWorld),
      iggy3d::FrontendAction::Back);

  return expect(frame.routed, "child back routed") &&
         expect(frame.route.accepted, "child back accepted") &&
         expect(frame.route.nextScreen == iggy3d::FrontendScreen::Starter,
                "child back screen") &&
         expect(frame.route.nextChildScreen == iggy3d::FrontendScreen::Gameplay,
                "child back closed child") &&
         expect(frame.route.status == "starter_child_returned",
                "child back status");
}

bool starterChildNonBackIsUnavailable() {
  const auto frame = iggy3d::routeProductFrontendAction(
      contextFor(iggy3d::FrontendScreen::Starter,
                 iggy3d::FrontendScreen::NewWorld),
      iggy3d::FrontendAction::CreateAndEnter);

  return expect(!frame.routed, "child non-back not routed") &&
         expect(!frame.route.accepted, "child non-back not accepted") &&
         expect(frame.route.requestedTransition ==
                    iggy3d::FrontendTransitionRequest::None,
                "child non-back no transition") &&
         expect(frame.route.status == "product_frontend_child_route_unavailable",
                "child non-back status");
}

bool settingsDelegationIsDeferred() {
  const auto frame = iggy3d::routeProductFrontendAction(
      contextFor(iggy3d::FrontendScreen::Starter,
                 iggy3d::FrontendScreen::Settings),
      iggy3d::FrontendAction::Apply);

  return expect(!frame.routed, "settings not routed") &&
         expect(!frame.route.accepted, "settings not accepted") &&
         expect(frame.owner.inputOwner == iggy3d::MenuOwner::Settings,
                "settings owner preserved") &&
         expect(frame.owner.activeSurface == iggy3d::ProductFrontendSurface::Settings,
                "settings surface preserved") &&
         expect(frame.route.status == "product_frontend_route_unavailable",
                "settings deferred status") &&
         expect(frame.route.gameplayInputSuppressed,
                "settings suppresses gameplay");
}

}  // namespace

int main() {
  const bool ok = surfaceNamesAreStable() && bootStatusSuppressesWithoutOwner() &&
                  starterRootOwnsInput() && starterChildrenWinBeforeStarter() &&
                  pauseSettingsAndOverlaysOwnInput() &&
                  gameplayRequiresActiveSession() && starterRootDelegatesNewWorld() &&
                  starterRootContinueDelegatesLaunchWhenCompatible() &&
                  starterRootContinueReportsDisabledWithoutCompatibleSave() &&
                  starterRootDelegatesSettingsAndDevTools() &&
                  missingStarterModelIsUnavailable() &&
                  starterChildBackDelegatesToStarterHelper() &&
                  starterChildNonBackIsUnavailable() && settingsDelegationIsDeferred();
  return ok ? 0 : 1;
}
