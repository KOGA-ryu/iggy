#include "app/iggy3d/ProductFrontendRouter.hpp"

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

}  // namespace

int main() {
  const bool ok = surfaceNamesAreStable() && bootStatusSuppressesWithoutOwner() &&
                  starterRootOwnsInput() && starterChildrenWinBeforeStarter() &&
                  pauseSettingsAndOverlaysOwnInput() &&
                  gameplayRequiresActiveSession();
  return ok ? 0 : 1;
}
