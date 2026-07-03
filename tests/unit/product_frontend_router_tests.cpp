#include "app/iggy3d/menu/FrontendRouter.hpp"

#include "app/frontend/DevToolsMenu.hpp"
#include "app/frontend/PauseMenu.hpp"
#include "app/frontend/SaveBrowser.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/frontend/StarterScreen.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"

#include <array>
#include <iostream>
#include <string>
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

iggy3d::ProductActiveSurfaceContext activeSurfaceContextFor(
    iggy3d::FrontendScreen screen,
    iggy3d::FrontendScreen child,
    bool gameplayActive = false,
    bool hasActiveSession = false,
    bool roomEditorReady = false,
    iggy3d::ProductInteractionMode interactionMode =
        iggy3d::ProductInteractionMode::Player) {
  iggy3d::ProductActiveSurfaceContext context;
  context.frontend.screen = screen;
  context.frontend.childScreen = child;
  context.gameplayActive = gameplayActive;
  context.hasActiveSession = hasActiveSession;
  context.roomEditorReady = roomEditorReady;
  context.interactionMode = interactionMode;
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
  const auto pauseLoadSave = iggy3d::chooseProductFrontendOwner(
      contextFor(iggy3d::FrontendScreen::Pause,
                 iggy3d::FrontendScreen::LoadSave));
  const auto pauseDeleteConfirm = iggy3d::chooseProductFrontendOwner(
      contextFor(iggy3d::FrontendScreen::Pause,
                 iggy3d::FrontendScreen::DeleteConfirm));
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
         expect(pauseLoadSave.inputOwner == iggy3d::MenuOwner::Pause,
                "pause load save owner") &&
         expect(pauseLoadSave.activeSurface ==
                    iggy3d::ProductFrontendSurface::SaveSelector,
                "pause load save surface") &&
         expect(pauseLoadSave.parentOwner == iggy3d::MenuOwner::Pause,
                "pause load save parent") &&
         expect(pauseDeleteConfirm.activeSurface ==
                    iggy3d::ProductFrontendSurface::ConfirmDialog,
                "pause delete confirm surface") &&
         expect(pauseDeleteConfirm.parentOwner == iggy3d::MenuOwner::Pause,
                "pause delete confirm parent") &&
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

bool activeMouseCapturePolicyNamesAreStable() {
  return expect(iggy3d::productActiveMouseCapturePolicyName(
                    iggy3d::ProductActiveMouseCapturePolicy::Released) ==
                    "released",
                "released mouse capture policy name") &&
         expect(iggy3d::productActiveMouseCapturePolicyName(
                    iggy3d::ProductActiveMouseCapturePolicy::RelativeGameplay) ==
                    "relative_gameplay",
                "relative gameplay mouse capture policy name");
}

bool activeSurfaceMatrixCoversCurrentRoutes() {
  struct ActiveSurfaceCase {
    const char* name = "";
    iggy3d::FrontendScreen screen = iggy3d::FrontendScreen::BootStatus;
    iggy3d::FrontendScreen child = iggy3d::FrontendScreen::Gameplay;
    bool gameplayActive = false;
    bool hasActiveSession = false;
    bool roomEditorReady = false;
    iggy3d::ProductFrontendSurface activeSurface =
        iggy3d::ProductFrontendSurface::None;
    iggy3d::ProductFrontendSurface parentSurface =
        iggy3d::ProductFrontendSurface::None;
    iggy3d::ProductInputSurface inputSurface = iggy3d::ProductInputSurface::None;
    iggy3d::MenuOwner inputOwner = iggy3d::MenuOwner::None;
    iggy3d::MenuOwner parentOwner = iggy3d::MenuOwner::None;
    bool gameplayInputSuppressed = true;
    iggy3d::ProductActiveMouseCapturePolicy mouseCapturePolicy =
        iggy3d::ProductActiveMouseCapturePolicy::Released;
    bool acceptsMenuActions = false;
    bool acceptsPlayerActions = false;
    bool acceptsEditorActions = false;
  };

  constexpr std::array cases{
      ActiveSurfaceCase{
          "boot_status",
          iggy3d::FrontendScreen::BootStatus,
          iggy3d::FrontendScreen::Gameplay,
          false,
          false,
          false,
          iggy3d::ProductFrontendSurface::BootStatus,
          iggy3d::ProductFrontendSurface::None,
          iggy3d::ProductInputSurface::None,
          iggy3d::MenuOwner::None,
          iggy3d::MenuOwner::None,
          true,
      },
      ActiveSurfaceCase{
          "starter_root",
          iggy3d::FrontendScreen::Starter,
          iggy3d::FrontendScreen::Gameplay,
          false,
          false,
          false,
          iggy3d::ProductFrontendSurface::Starter,
          iggy3d::ProductFrontendSurface::None,
          iggy3d::ProductInputSurface::Starter,
          iggy3d::MenuOwner::Starter,
          iggy3d::MenuOwner::None,
          true,
          iggy3d::ProductActiveMouseCapturePolicy::Released,
          true,
      },
      ActiveSurfaceCase{
          "starter_world_setup",
          iggy3d::FrontendScreen::Starter,
          iggy3d::FrontendScreen::NewWorld,
          false,
          false,
          false,
          iggy3d::ProductFrontendSurface::WorldSetup,
          iggy3d::ProductFrontendSurface::Starter,
          iggy3d::ProductInputSurface::WorldSetup,
          iggy3d::MenuOwner::Starter,
          iggy3d::MenuOwner::Starter,
          true,
          iggy3d::ProductActiveMouseCapturePolicy::Released,
          true,
      },
      ActiveSurfaceCase{
          "starter_save_selector",
          iggy3d::FrontendScreen::Starter,
          iggy3d::FrontendScreen::LoadSave,
          false,
          false,
          false,
          iggy3d::ProductFrontendSurface::SaveSelector,
          iggy3d::ProductFrontendSurface::Starter,
          iggy3d::ProductInputSurface::SaveBrowser,
          iggy3d::MenuOwner::Starter,
          iggy3d::MenuOwner::Starter,
          true,
          iggy3d::ProductActiveMouseCapturePolicy::Released,
          true,
      },
      ActiveSurfaceCase{
          "starter_delete_confirm",
          iggy3d::FrontendScreen::Starter,
          iggy3d::FrontendScreen::DeleteConfirm,
          false,
          false,
          false,
          iggy3d::ProductFrontendSurface::ConfirmDialog,
          iggy3d::ProductFrontendSurface::Starter,
          iggy3d::ProductInputSurface::SaveBrowser,
          iggy3d::MenuOwner::Starter,
          iggy3d::MenuOwner::Starter,
          true,
          iggy3d::ProductActiveMouseCapturePolicy::Released,
          true,
      },
      ActiveSurfaceCase{
          "starter_exit_confirm",
          iggy3d::FrontendScreen::Starter,
          iggy3d::FrontendScreen::ExitConfirm,
          false,
          false,
          false,
          iggy3d::ProductFrontendSurface::ConfirmDialog,
          iggy3d::ProductFrontendSurface::Starter,
          iggy3d::ProductInputSurface::Starter,
          iggy3d::MenuOwner::Starter,
          iggy3d::MenuOwner::Starter,
          true,
          iggy3d::ProductActiveMouseCapturePolicy::Released,
          true,
      },
      ActiveSurfaceCase{
          "starter_settings",
          iggy3d::FrontendScreen::Starter,
          iggy3d::FrontendScreen::Settings,
          false,
          false,
          false,
          iggy3d::ProductFrontendSurface::Settings,
          iggy3d::ProductFrontendSurface::Starter,
          iggy3d::ProductInputSurface::Settings,
          iggy3d::MenuOwner::Settings,
          iggy3d::MenuOwner::Starter,
          true,
          iggy3d::ProductActiveMouseCapturePolicy::Released,
          true,
      },
      ActiveSurfaceCase{
          "starter_dev_tools",
          iggy3d::FrontendScreen::Starter,
          iggy3d::FrontendScreen::StarterDevTools,
          false,
          false,
          false,
          iggy3d::ProductFrontendSurface::DevTools,
          iggy3d::ProductFrontendSurface::Starter,
          iggy3d::ProductInputSurface::DevTools,
          iggy3d::MenuOwner::DevTools,
          iggy3d::MenuOwner::Starter,
          true,
          iggy3d::ProductActiveMouseCapturePolicy::Released,
          true,
      },
      ActiveSurfaceCase{
          "pause_root",
          iggy3d::FrontendScreen::Pause,
          iggy3d::FrontendScreen::Gameplay,
          true,
          true,
          false,
          iggy3d::ProductFrontendSurface::Pause,
          iggy3d::ProductFrontendSurface::None,
          iggy3d::ProductInputSurface::Pause,
          iggy3d::MenuOwner::Pause,
          iggy3d::MenuOwner::None,
          true,
          iggy3d::ProductActiveMouseCapturePolicy::Released,
          true,
      },
      ActiveSurfaceCase{
          "pause_save_selector",
          iggy3d::FrontendScreen::Pause,
          iggy3d::FrontendScreen::LoadSave,
          true,
          true,
          false,
          iggy3d::ProductFrontendSurface::SaveSelector,
          iggy3d::ProductFrontendSurface::Pause,
          iggy3d::ProductInputSurface::SaveBrowser,
          iggy3d::MenuOwner::Pause,
          iggy3d::MenuOwner::Pause,
          true,
          iggy3d::ProductActiveMouseCapturePolicy::Released,
          true,
      },
      ActiveSurfaceCase{
          "pause_delete_confirm",
          iggy3d::FrontendScreen::Pause,
          iggy3d::FrontendScreen::DeleteConfirm,
          true,
          true,
          false,
          iggy3d::ProductFrontendSurface::ConfirmDialog,
          iggy3d::ProductFrontendSurface::Pause,
          iggy3d::ProductInputSurface::SaveBrowser,
          iggy3d::MenuOwner::Pause,
          iggy3d::MenuOwner::Pause,
          true,
          iggy3d::ProductActiveMouseCapturePolicy::Released,
          true,
      },
      ActiveSurfaceCase{
          "pause_settings",
          iggy3d::FrontendScreen::Settings,
          iggy3d::FrontendScreen::Pause,
          true,
          true,
          false,
          iggy3d::ProductFrontendSurface::Settings,
          iggy3d::ProductFrontendSurface::Pause,
          iggy3d::ProductInputSurface::Settings,
          iggy3d::MenuOwner::Settings,
          iggy3d::MenuOwner::Pause,
          true,
          iggy3d::ProductActiveMouseCapturePolicy::Released,
          true,
      },
      ActiveSurfaceCase{
          "dev_overlay",
          iggy3d::FrontendScreen::DevOverlay,
          iggy3d::FrontendScreen::Gameplay,
          true,
          true,
          false,
          iggy3d::ProductFrontendSurface::DevTools,
          iggy3d::ProductFrontendSurface::Gameplay,
          iggy3d::ProductInputSurface::DevTools,
          iggy3d::MenuOwner::DevTools,
          iggy3d::MenuOwner::Gameplay,
          true,
          iggy3d::ProductActiveMouseCapturePolicy::Released,
          true,
      },
      ActiveSurfaceCase{
          "gameplay",
          iggy3d::FrontendScreen::Gameplay,
          iggy3d::FrontendScreen::Gameplay,
          true,
          true,
          false,
          iggy3d::ProductFrontendSurface::Gameplay,
          iggy3d::ProductFrontendSurface::None,
          iggy3d::ProductInputSurface::Gameplay,
          iggy3d::MenuOwner::Gameplay,
          iggy3d::MenuOwner::None,
          false,
          iggy3d::ProductActiveMouseCapturePolicy::RelativeGameplay,
          false,
          true,
      },
      ActiveSurfaceCase{
          "room_editor",
          iggy3d::FrontendScreen::Gameplay,
          iggy3d::FrontendScreen::Gameplay,
          true,
          true,
          true,
          iggy3d::ProductFrontendSurface::Editor,
          iggy3d::ProductFrontendSurface::Gameplay,
          iggy3d::ProductInputSurface::RoomEditor,
          iggy3d::MenuOwner::Editor,
          iggy3d::MenuOwner::Gameplay,
          true,
          iggy3d::ProductActiveMouseCapturePolicy::Released,
          true,
          false,
          true,
      },
      ActiveSurfaceCase{
          "gameplay_without_session",
          iggy3d::FrontendScreen::Gameplay,
          iggy3d::FrontendScreen::Gameplay,
          true,
          false,
          false,
          iggy3d::ProductFrontendSurface::None,
          iggy3d::ProductFrontendSurface::None,
          iggy3d::ProductInputSurface::None,
          iggy3d::MenuOwner::None,
          iggy3d::MenuOwner::None,
          true,
      },
  };

  bool ok = true;
  for (const ActiveSurfaceCase& row : cases) {
    const iggy3d::ProductActiveSurfaceFrame frame =
        iggy3d::resolveProductActiveSurface(activeSurfaceContextFor(
            row.screen,
            row.child,
            row.gameplayActive,
            row.hasActiveSession,
            row.roomEditorReady));
    const std::string prefix = std::string(row.name) + " ";
    ok = expect(frame.activeSurface == row.activeSurface,
                (prefix + "active surface").c_str()) &&
         ok;
    ok = expect(frame.parentSurface == row.parentSurface,
                (prefix + "parent surface").c_str()) &&
         ok;
    ok = expect(frame.inputSurface == row.inputSurface,
                (prefix + "input surface").c_str()) &&
         ok;
    ok = expect(frame.inputOwner == row.inputOwner,
                (prefix + "input owner").c_str()) &&
         ok;
    ok = expect(frame.parentOwner == row.parentOwner,
                (prefix + "parent owner").c_str()) &&
         ok;
    ok = expect(frame.gameplayInputSuppressed == row.gameplayInputSuppressed,
                (prefix + "suppression").c_str()) &&
         ok;
    ok = expect(frame.mouseCapturePolicy == row.mouseCapturePolicy,
                (prefix + "mouse capture policy").c_str()) &&
         ok;
    ok = expect(frame.acceptsSystemActions,
                (prefix + "system actions").c_str()) &&
         ok;
    ok = expect(frame.acceptsMenuActions == row.acceptsMenuActions,
                (prefix + "menu actions").c_str()) &&
         ok;
    ok = expect(frame.acceptsPlayerActions == row.acceptsPlayerActions,
                (prefix + "player actions").c_str()) &&
         ok;
    ok = expect(frame.acceptsEditorActions == row.acceptsEditorActions,
                (prefix + "editor actions").c_str()) &&
         ok;
  }
  return ok;
}

bool activeSurfaceWindowContextPreservesLegacyGameplayGate() {
  iggy3d::FrontendState frontend;
  frontend.screen = iggy3d::FrontendScreen::Gameplay;
  frontend.childScreen = iggy3d::FrontendScreen::Gameplay;

  iggy3d::ProductAppWindowState window;
  window.gameplayActive = true;
  const auto gameplay = iggy3d::resolveProductActiveSurface(
      iggy3d::productActiveSurfaceContextForWindow(frontend, window));

  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  const auto creativeGameplay = iggy3d::resolveProductActiveSurface(
      iggy3d::productActiveSurfaceContextForWindow(frontend, window));

  window.roomEditing.ready = true;
  const auto editor = iggy3d::resolveProductActiveSurface(
      iggy3d::productActiveSurfaceContextForWindow(frontend, window));

  return expect(gameplay.inputOwner == iggy3d::MenuOwner::Gameplay,
                "window gameplay owner") &&
         expect(gameplay.inputSurface == iggy3d::ProductInputSurface::Gameplay,
                "window gameplay input surface") &&
         expect(gameplay.mouseCapturePolicy ==
                    iggy3d::ProductActiveMouseCapturePolicy::RelativeGameplay,
                "player gameplay keeps relative mouse capture policy") &&
         expect(creativeGameplay.inputOwner == iggy3d::MenuOwner::Gameplay,
                "window creative gameplay owner") &&
         expect(creativeGameplay.mouseCapturePolicy ==
                    iggy3d::ProductActiveMouseCapturePolicy::Released,
                "creative gameplay releases mouse capture policy") &&
         expect(editor.inputOwner == iggy3d::MenuOwner::Editor,
                "window editor owner") &&
         expect(editor.inputSurface == iggy3d::ProductInputSurface::RoomEditor,
                "window editor input surface");
}

bool inputOwnerCacheSyncUsesResolvedActiveSurface() {
  struct CacheCase {
    iggy3d::FrontendScreen screen = iggy3d::FrontendScreen::Starter;
    iggy3d::FrontendScreen child = iggy3d::FrontendScreen::Gameplay;
    bool gameplayActive = false;
    bool roomEditorReady = false;
    iggy3d::MenuOwner expectedOwner = iggy3d::MenuOwner::None;
    bool expectedSuppressed = true;
    const char* label = "";
  };

  constexpr CacheCase cases[] = {
      {iggy3d::FrontendScreen::Starter,
       iggy3d::FrontendScreen::Gameplay,
       false,
       false,
       iggy3d::MenuOwner::Starter,
       true,
       "starter"},
      {iggy3d::FrontendScreen::Gameplay,
       iggy3d::FrontendScreen::Gameplay,
       true,
       false,
       iggy3d::MenuOwner::Gameplay,
       false,
       "gameplay"},
      {iggy3d::FrontendScreen::Settings,
       iggy3d::FrontendScreen::Pause,
       true,
       false,
       iggy3d::MenuOwner::Settings,
       true,
       "pause-settings"},
      {iggy3d::FrontendScreen::DevOverlay,
       iggy3d::FrontendScreen::Gameplay,
       true,
       false,
       iggy3d::MenuOwner::DevTools,
       true,
       "devtools"},
      {iggy3d::FrontendScreen::Gameplay,
       iggy3d::FrontendScreen::Gameplay,
       true,
       true,
       iggy3d::MenuOwner::Editor,
       true,
       "editor"},
  };

  bool ok = true;
  for (const CacheCase& row : cases) {
    iggy3d::FrontendState frontend;
    frontend.screen = row.screen;
    frontend.childScreen = row.child;
    iggy3d::ProductAppWindowState window;
    window.gameplayActive = row.gameplayActive;
    window.roomEditing.ready = row.roomEditorReady;
    window.inputOwner = iggy3d::MenuOwner::Gameplay;
    window.gameplayInputSuppressed = false;

    const iggy3d::ProductActiveSurfaceFrame surface =
        iggy3d::syncProductWindowInputOwnerFromActiveSurface(frontend, window);
    const std::string prefix = std::string(row.label) + " cache sync ";
    ok = expect(surface.inputOwner == row.expectedOwner,
                (prefix + "surface owner").c_str()) &&
         ok;
    ok = expect(surface.gameplayInputSuppressed == row.expectedSuppressed,
                (prefix + "surface suppression").c_str()) &&
         ok;
    ok = expect(window.inputOwner == row.expectedOwner,
                (prefix + "window owner").c_str()) &&
         ok;
    ok = expect(window.gameplayInputSuppressed == row.expectedSuppressed,
                (prefix + "window suppression").c_str()) &&
         ok;
  }
  return ok;
}

}  // namespace

int main() {
  const bool ok = surfaceNamesAreStable() && bootStatusSuppressesWithoutOwner() &&
                  starterRootOwnsInput() && starterChildrenWinBeforeStarter() &&
                  pauseSettingsAndOverlaysOwnInput() &&
                  gameplayRequiresActiveSession() &&
                  activeMouseCapturePolicyNamesAreStable() &&
                  activeSurfaceMatrixCoversCurrentRoutes() &&
                  activeSurfaceWindowContextPreservesLegacyGameplayGate() &&
                  inputOwnerCacheSyncUsesResolvedActiveSurface();
  return ok ? 0 : 1;
}
