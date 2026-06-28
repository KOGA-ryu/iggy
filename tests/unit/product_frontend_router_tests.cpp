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
    bool roomEditorReady = false) {
  iggy3d::ProductActiveSurfaceContext context;
  context.frontend.screen = screen;
  context.frontend.childScreen = child;
  context.gameplayActive = gameplayActive;
  context.hasActiveSession = hasActiveSession;
  context.roomEditorReady = roomEditorReady;
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

iggy3d::SettingsRouteContext settingsRouteContextFor(
    iggy3d::MenuOwner parentOwner,
    iggy3d::FrontendSettingsTab selectedTab,
    bool dirty) {
  const auto settings = iggy3d::defaultFrontendSettings();
  iggy3d::SettingsRouteContext context;
  context.parentOwner = parentOwner;
  context.selectedTab = selectedTab;
  context.selectedRow = iggy3d::defaultSettingsRowModel(selectedTab, settings);
  context.dirty = dirty;
  return context;
}

iggy3d::ProductFrontendRouteContext settingsContextFor(
    iggy3d::FrontendScreen screen,
    iggy3d::FrontendScreen child,
    const iggy3d::SettingsRouteContext& settings) {
  iggy3d::ProductFrontendRouteContext context = contextFor(screen, child);
  context.settingsContext = &settings;
  return context;
}

iggy3d::PauseMenuModel buildOpenPauseModel(std::uint64_t compatibleSaveCount = 1U) {
  return iggy3d::buildPauseMenuModel(
      iggy3d::PauseMenuContext{true, true, true, compatibleSaveCount, true, true, true},
      iggy3d::FrontendAction::Resume);
}

iggy3d::ProductFrontendRouteContext pauseContextFor(
    const iggy3d::PauseMenuModel& model) {
  iggy3d::ProductFrontendRouteContext context =
      contextFor(iggy3d::FrontendScreen::Pause,
                 iggy3d::FrontendScreen::Gameplay);
  context.pauseModel = &model;
  return context;
}

iggy3d::ProductFrontendRouteContext devToolsContextFor(
    iggy3d::FrontendScreen screen,
    iggy3d::FrontendScreen child,
    const iggy3d::DevToolsMenuModel& model) {
  iggy3d::ProductFrontendRouteContext context = contextFor(screen, child);
  context.devToolsModel = &model;
  return context;
}

iggy3d::SaveSlotPreview routerCompatibleSlot(std::string_view id) {
  iggy3d::SaveSlotPreview slot;
  slot.id = std::string(id);
  slot.enabled = true;
  slot.reason = "compatible";
  slot.displayTitle = "Title " + slot.id;
  slot.timestampLabel = "file_time_100";
  return slot;
}

iggy3d::SaveBrowserModel compatibleSaveBrowserModel() {
  iggy3d::SaveSlotList slots;
  slots.slots.push_back(routerCompatibleSlot("save_001"));
  slots.compatibleCount = 1U;
  return iggy3d::buildSaveBrowserModel(slots, "save_001");
}

iggy3d::ProductFrontendRouteContext saveBrowserContextFor(
    iggy3d::FrontendScreen screen,
    iggy3d::FrontendScreen child,
    const iggy3d::SaveBrowserModel& model) {
  iggy3d::ProductFrontendRouteContext context = contextFor(screen, child);
  context.saveBrowserModel = &model;
  return context;
}

iggy3d::ProductFrontendRouteContext worldSetupContextFor(
    const iggy3d::WorldSetupDraft& draft) {
  iggy3d::ProductFrontendRouteContext context =
      contextFor(iggy3d::FrontendScreen::Starter,
                 iggy3d::FrontendScreen::NewWorld);
  context.worldSetupDraft = &draft;
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

  window.roomEditing.ready = true;
  const auto editor = iggy3d::resolveProductActiveSurface(
      iggy3d::productActiveSurfaceContextForWindow(frontend, window));

  return expect(gameplay.inputOwner == iggy3d::MenuOwner::Gameplay,
                "window gameplay owner") &&
         expect(gameplay.inputSurface == iggy3d::ProductInputSurface::Gameplay,
                "window gameplay input surface") &&
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
                 iggy3d::FrontendScreen::DeleteConfirm),
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
                 iggy3d::FrontendScreen::DeleteConfirm),
      iggy3d::FrontendAction::CreateAndEnter);

  return expect(!frame.routed, "child non-back not routed") &&
         expect(!frame.route.accepted, "child non-back not accepted") &&
         expect(frame.route.requestedTransition ==
                    iggy3d::FrontendTransitionRequest::None,
                "child non-back no transition") &&
         expect(frame.route.status == "product_frontend_child_route_unavailable",
                "child non-back status");
}

bool worldSetupBackDelegatesThroughDraft() {
  const auto draft = iggy3d::makeDefaultWorldSetupDraft("seed_1");
  const auto context = worldSetupContextFor(draft);
  const auto frame = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::Back);

  return expect(frame.routed, "world setup back routed") &&
         expect(frame.hasWorldSetupRoute, "world setup route present") &&
         expect(frame.worldSetupRoute.draftDiscarded,
                "world setup draft discarded") &&
         expect(frame.route.accepted, "world setup back accepted") &&
         expect(frame.route.inputOwner == iggy3d::MenuOwner::Starter,
                "world setup back owner") &&
         expect(frame.route.nextScreen == iggy3d::FrontendScreen::Starter,
                "world setup back screen") &&
         expect(frame.route.nextChildScreen == iggy3d::FrontendScreen::Gameplay,
                "world setup back child") &&
         expect(frame.route.requestedTransition ==
                    iggy3d::FrontendTransitionRequest::None,
                "world setup back transition") &&
         expect(frame.route.gameplayInputSuppressed,
                "world setup back suppresses") &&
         expect(frame.route.status == "world_setup_back",
                "world setup back status") &&
         expect(frame.route.receiptReason == "world_setup_back",
                "world setup back reason") &&
         expect(frame.routeModelName == "world_setup",
                "world setup back model name");
}

bool worldSetupValidCreateCarriesRequestWithoutTransition() {
  auto draft = iggy3d::makeDefaultWorldSetupDraft("seed_42");
  draft.worldName = "  Router World  ";
  const auto context = worldSetupContextFor(draft);
  const auto frame = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::CreateAndEnter);
  const auto summary = iggy3d::summarizeProductFrontendRoute(
      context,
      frame,
      iggy3d::FrontendAction::CreateAndEnter);

  return expect(frame.routed, "world setup create routed") &&
         expect(frame.hasWorldSetupRoute, "world setup create result present") &&
         expect(frame.worldSetupRoute.createRequested,
                "world setup create requested") &&
         expect(frame.worldSetupRoute.createRequest.worldName == "Router World",
                "world setup create world name") &&
         expect(frame.worldSetupRoute.createRequest.seedText == "seed_42",
                "world setup create seed") &&
         expect(frame.worldSetupRoute.createRequest.resolvedSeed ==
                    iggy3d::resolveWorldSetupSeed("seed_42"),
                "world setup create resolved seed") &&
         expect(frame.route.accepted, "world setup create accepted") &&
         expect(frame.route.inputOwner == iggy3d::MenuOwner::Starter,
                "world setup create owner") &&
         expect(frame.route.nextScreen == iggy3d::FrontendScreen::Starter,
                "world setup create screen") &&
         expect(frame.route.nextChildScreen == iggy3d::FrontendScreen::NewWorld,
                "world setup create child") &&
         expect(frame.route.requestedTransition ==
                    iggy3d::FrontendTransitionRequest::None,
                "world setup create no transition") &&
         expect(frame.route.gameplayInputSuppressed,
                "world setup create suppresses") &&
         expect(frame.route.status == "world_setup_create_requested",
                "world setup create status") &&
         expect(summary.activeSurface == "world_setup",
                "world setup create summary surface") &&
         expect(summary.routeModelName == "world_setup",
                "world setup create summary model") &&
         expect(summary.transition == "none",
                "world setup create summary transition") &&
         expect(summary.reason == "world_setup_create_requested",
                "world setup create summary reason");
}

bool worldSetupInvalidCreateAndUnsupportedStayOnDraft() {
  auto draft = iggy3d::makeDefaultWorldSetupDraft("seed_1");
  draft.worldName = " ";
  const auto context = worldSetupContextFor(draft);
  const auto invalid = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::CreateAndEnter);
  const auto unsupported = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::Apply);

  return expect(invalid.routed, "world setup invalid routed") &&
         expect(invalid.hasWorldSetupRoute, "world setup invalid result present") &&
         expect(!invalid.route.accepted, "world setup invalid rejected") &&
         expect(!invalid.worldSetupRoute.createRequested,
                "world setup invalid no create request") &&
         expect(invalid.route.nextScreen == iggy3d::FrontendScreen::Starter,
                "world setup invalid screen") &&
         expect(invalid.route.nextChildScreen == iggy3d::FrontendScreen::NewWorld,
                "world setup invalid child") &&
         expect(invalid.route.status == "world_setup_invalid",
                "world setup invalid status") &&
         expect(invalid.route.receiptReason == "invalid_world_name",
                "world setup invalid reason") &&
         expect(unsupported.routed, "world setup unsupported routed") &&
         expect(!unsupported.route.accepted, "world setup unsupported rejected") &&
         expect(unsupported.route.status == "not_world_setup_action",
                "world setup unsupported status") &&
         expect(unsupported.route.receiptReason == "not_world_setup_action",
                "world setup unsupported reason") &&
         expect(unsupported.route.nextChildScreen == iggy3d::FrontendScreen::NewWorld,
                "world setup unsupported child");
}

bool missingWorldSetupDraftIsUnavailable() {
  const auto context = contextFor(iggy3d::FrontendScreen::Starter,
                                 iggy3d::FrontendScreen::NewWorld);
  const auto frame = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::CreateAndEnter);
  const auto summary = iggy3d::summarizeProductFrontendRoute(
      context,
      frame,
      iggy3d::FrontendAction::CreateAndEnter);

  return expect(!frame.routed, "missing world setup not routed") &&
         expect(!frame.hasWorldSetupRoute, "missing world setup no model result") &&
         expect(!frame.route.accepted, "missing world setup not accepted") &&
         expect(frame.route.status == "world_setup_model_unavailable",
                "missing world setup status") &&
         expect(frame.route.receiptReason == "world_setup_model_unavailable",
                "missing world setup reason") &&
         expect(!summary.routeModelAvailable,
                "missing world setup summary model unavailable") &&
         expect(summary.routeModelName == "world_setup",
                "missing world setup summary model name");
}

bool starterSettingsApplyDelegatesToSettingsHelper() {
  const auto settings = settingsRouteContextFor(
      iggy3d::MenuOwner::Starter,
      iggy3d::FrontendSettingsTab::Input,
      true);
  const auto frame = iggy3d::routeProductFrontendAction(
      settingsContextFor(iggy3d::FrontendScreen::Starter,
                         iggy3d::FrontendScreen::Settings,
                         settings),
      iggy3d::FrontendAction::Apply);

  return expect(frame.routed, "starter settings apply routed") &&
         expect(frame.route.accepted, "starter settings apply accepted") &&
         expect(frame.owner.inputOwner == iggy3d::MenuOwner::Settings,
                "settings owner preserved") &&
         expect(frame.owner.activeSurface == iggy3d::ProductFrontendSurface::Settings,
                "settings surface preserved") &&
         expect(frame.route.nextScreen == iggy3d::FrontendScreen::Settings,
                "starter settings apply stays settings") &&
         expect(frame.route.nextChildScreen == iggy3d::FrontendScreen::Starter,
                "starter settings apply child") &&
         expect(frame.route.status == "settings_apply_requested",
                "starter settings apply status") &&
         expect(frame.route.gameplayInputSuppressed,
                "starter settings apply suppresses gameplay");
}

bool starterSettingsBackReturnsToStarter() {
  const auto settings = settingsRouteContextFor(
      iggy3d::MenuOwner::Starter,
      iggy3d::FrontendSettingsTab::Input,
      false);
  const auto frame = iggy3d::routeProductFrontendAction(
      settingsContextFor(iggy3d::FrontendScreen::Starter,
                         iggy3d::FrontendScreen::Settings,
                         settings),
      iggy3d::FrontendAction::Back);

  return expect(frame.routed, "starter settings back routed") &&
         expect(frame.route.accepted, "starter settings back accepted") &&
         expect(frame.route.nextScreen == iggy3d::FrontendScreen::Starter,
                "starter settings back screen") &&
         expect(frame.route.nextChildScreen == iggy3d::FrontendScreen::Gameplay,
                "starter settings back child") &&
         expect(frame.route.status == "settings_back_requested",
                "starter settings back status");
}

bool pauseSettingsBackReturnsToPause() {
  const auto settings = settingsRouteContextFor(
      iggy3d::MenuOwner::Pause,
      iggy3d::FrontendSettingsTab::Input,
      false);
  const auto frame = iggy3d::routeProductFrontendAction(
      settingsContextFor(iggy3d::FrontendScreen::Settings,
                         iggy3d::FrontendScreen::Pause,
                         settings),
      iggy3d::FrontendAction::Back);

  return expect(frame.routed, "pause settings back routed") &&
         expect(frame.route.accepted, "pause settings back accepted") &&
         expect(frame.owner.parentOwner == iggy3d::MenuOwner::Pause,
                "pause settings owner parent") &&
         expect(frame.route.nextScreen == iggy3d::FrontendScreen::Pause,
                "pause settings back screen") &&
         expect(frame.route.nextChildScreen == iggy3d::FrontendScreen::Gameplay,
                "pause settings back child") &&
         expect(frame.route.status == "settings_back_requested",
                "pause settings back status");
}

bool settingsDisabledRowDelegatesReason() {
  const auto settings = settingsRouteContextFor(
      iggy3d::MenuOwner::Starter,
      iggy3d::FrontendSettingsTab::Audio,
      true);
  const auto frame = iggy3d::routeProductFrontendAction(
      settingsContextFor(iggy3d::FrontendScreen::Starter,
                         iggy3d::FrontendScreen::Settings,
                         settings),
      iggy3d::FrontendAction::Apply);

  return expect(frame.routed, "disabled row routed") &&
         expect(!frame.route.accepted, "disabled row not accepted") &&
         expect(frame.route.status == "audio_unavailable",
                "disabled row reason") &&
         expect(frame.route.receiptReason == "audio_unavailable",
                "disabled row receipt reason");
}

bool settingsCleanApplyReportsNoChanges() {
  const auto settings = settingsRouteContextFor(
      iggy3d::MenuOwner::Starter,
      iggy3d::FrontendSettingsTab::Input,
      false);
  const auto frame = iggy3d::routeProductFrontendAction(
      settingsContextFor(iggy3d::FrontendScreen::Starter,
                         iggy3d::FrontendScreen::Settings,
                         settings),
      iggy3d::FrontendAction::Apply);

  return expect(frame.routed, "clean apply routed") &&
         expect(!frame.route.accepted, "clean apply not accepted") &&
         expect(frame.route.status == "settings_no_changes",
                "clean apply status");
}

bool missingSettingsContextIsUnavailable() {
  const auto frame = iggy3d::routeProductFrontendAction(
      contextFor(iggy3d::FrontendScreen::Starter,
                 iggy3d::FrontendScreen::Settings),
      iggy3d::FrontendAction::Apply);

  return expect(!frame.routed, "missing settings not routed") &&
         expect(!frame.route.accepted, "missing settings not accepted") &&
         expect(frame.route.status == "settings_model_unavailable",
                "missing settings status") &&
         expect(frame.route.receiptReason == "settings_model_unavailable",
                "missing settings receipt") &&
         expect(frame.route.gameplayInputSuppressed,
                "missing settings suppresses gameplay");
}

bool settingsInvalidParentDelegatesReason() {
  const auto settings = settingsRouteContextFor(
      iggy3d::MenuOwner::None,
      iggy3d::FrontendSettingsTab::Input,
      true);
  const auto frame = iggy3d::routeProductFrontendAction(
      settingsContextFor(iggy3d::FrontendScreen::Starter,
                         iggy3d::FrontendScreen::Settings,
                         settings),
      iggy3d::FrontendAction::Apply);

  return expect(frame.routed, "invalid settings parent routed") &&
         expect(!frame.route.accepted, "invalid settings parent not accepted") &&
         expect(frame.route.status == "settings_invalid_parent",
                "invalid settings parent status");
}

bool pauseRouteDelegatesResume() {
  const auto model = buildOpenPauseModel();
  const auto context = pauseContextFor(model);
  const auto frame = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::Resume);
  const auto summary = iggy3d::summarizeProductFrontendRoute(
      context,
      frame,
      iggy3d::FrontendAction::Resume);

  return expect(frame.routed, "pause resume routed") &&
         expect(frame.route.accepted, "pause resume accepted") &&
         expect(frame.route.inputOwner == iggy3d::MenuOwner::Gameplay,
                "pause resume owner") &&
         expect(frame.route.nextScreen == iggy3d::FrontendScreen::Gameplay,
                "pause resume screen") &&
         expect(!frame.route.gameplayInputSuppressed,
                "pause resume unsuppressed") &&
         expect(frame.route.status == "pause_resume_requested",
                "pause resume status") &&
         expect(summary.routeModelAvailable, "pause resume model available") &&
         expect(summary.routeModelName == "pause",
                "pause resume summary model");
}

bool missingPauseModelIsUnavailable() {
  const auto context = contextFor(iggy3d::FrontendScreen::Pause,
                                 iggy3d::FrontendScreen::Gameplay);
  const auto frame = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::Resume);
  const auto summary = iggy3d::summarizeProductFrontendRoute(
      context,
      frame,
      iggy3d::FrontendAction::Resume);

  return expect(!frame.routed, "missing pause not routed") &&
         expect(!frame.route.accepted, "missing pause not accepted") &&
         expect(frame.route.status == "pause_model_unavailable",
                "missing pause status") &&
         expect(!summary.routeModelAvailable,
                "missing pause summary model unavailable") &&
         expect(summary.routeModelName == "pause",
                "missing pause summary model name");
}

bool disabledPauseLoadSaveDelegatesReason() {
  const auto model = buildOpenPauseModel(0U);
  const auto frame = iggy3d::routeProductFrontendAction(
      pauseContextFor(model),
      iggy3d::FrontendAction::LoadSave);

  return expect(frame.routed, "disabled pause load routed") &&
         expect(!frame.route.accepted, "disabled pause load not accepted") &&
         expect(frame.route.status == "no_compatible_save",
                "disabled pause load status");
}

bool pauseSettingsRouteSummaryIsReceiptReady() {
  const auto model = buildOpenPauseModel();
  const auto context = pauseContextFor(model);
  const auto frame = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::Settings);
  const auto summary = iggy3d::summarizeProductFrontendRoute(
      context,
      frame,
      iggy3d::FrontendAction::Settings);

  return expect(frame.routed, "pause settings routed") &&
         expect(frame.route.accepted, "pause settings accepted") &&
         expect(frame.route.inputOwner == iggy3d::MenuOwner::Settings,
                "pause settings owner") &&
         expect(frame.route.nextScreen == iggy3d::FrontendScreen::Settings,
                "pause settings screen") &&
         expect(frame.route.nextChildScreen == iggy3d::FrontendScreen::Pause,
                "pause settings child") &&
         expect(summary.activeSurface == "pause",
                "pause settings summary surface") &&
         expect(summary.status == "pause_settings_opened",
                "pause settings summary status") &&
         expect(summary.reason == "pause_settings_opened",
                "pause settings summary reason");
}

bool devToolsRoutesFromStarterAndGameplay() {
  const auto model =
      iggy3d::buildDevToolsMenuModel(iggy3d::FrontendDevToolsCategory::Session);
  const auto starterContext = devToolsContextFor(
      iggy3d::FrontendScreen::Starter,
      iggy3d::FrontendScreen::StarterDevTools,
      model);
  const auto gameplayContext = devToolsContextFor(
      iggy3d::FrontendScreen::DevOverlay,
      iggy3d::FrontendScreen::Gameplay,
      model);

  const auto starter = iggy3d::routeProductFrontendAction(
      starterContext,
      iggy3d::FrontendAction::Back);
  const auto gameplay = iggy3d::routeProductFrontendAction(
      gameplayContext,
      iggy3d::FrontendAction::Back);

  return expect(starter.routed, "starter dev tools routed") &&
         expect(starter.route.accepted, "starter dev tools accepted") &&
         expect(starter.route.inputOwner == iggy3d::MenuOwner::Starter,
                "starter dev tools owner") &&
         expect(starter.route.status == "dev_tools_closed_to_starter",
                "starter dev tools status") &&
         expect(gameplay.routed, "gameplay dev tools routed") &&
         expect(gameplay.route.accepted, "gameplay dev tools accepted") &&
         expect(gameplay.route.inputOwner == iggy3d::MenuOwner::Gameplay,
                "gameplay dev tools owner") &&
         expect(!gameplay.route.gameplayInputSuppressed,
                "gameplay dev tools unsuppressed") &&
         expect(gameplay.route.status == "dev_tools_closed_to_gameplay",
                "gameplay dev tools status");
}

bool missingDevToolsModelIsUnavailable() {
  const auto context = contextFor(iggy3d::FrontendScreen::DevOverlay,
                                 iggy3d::FrontendScreen::Gameplay);
  const auto frame = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::Back);
  const auto summary = iggy3d::summarizeProductFrontendRoute(
      context,
      frame,
      iggy3d::FrontendAction::Back);

  return expect(!frame.routed, "missing dev tools not routed") &&
         expect(!frame.route.accepted, "missing dev tools not accepted") &&
         expect(frame.route.status == "dev_tools_model_unavailable",
                "missing dev tools status") &&
         expect(!summary.routeModelAvailable,
                "missing dev tools summary model unavailable") &&
         expect(summary.routeModelName == "dev_tools",
                "missing dev tools summary model name");
}

bool devToolsApplySummaryIsReceiptReady() {
  const auto model =
      iggy3d::buildDevToolsMenuModel(iggy3d::FrontendDevToolsCategory::Renderer);
  const auto context = devToolsContextFor(
      iggy3d::FrontendScreen::DevOverlay,
      iggy3d::FrontendScreen::Gameplay,
      model);
  const auto frame = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::Apply);
  const auto summary = iggy3d::summarizeProductFrontendRoute(
      context,
      frame,
      iggy3d::FrontendAction::Apply);

  return expect(frame.routed, "dev tools apply routed") &&
         expect(frame.route.accepted, "dev tools apply accepted") &&
         expect(frame.route.status == "dev_tools_category_selected",
                "dev tools apply status") &&
         expect(summary.activeSurface == "dev_tools",
                "dev tools summary surface") &&
         expect(summary.routeModelAvailable,
                "dev tools summary model available") &&
         expect(summary.routeModelName == "dev_tools",
                "dev tools summary model name") &&
         expect(summary.reason == "dev_tools_category_selected",
                "dev tools summary reason");
}

bool saveBrowserRoutesFromStarterAndPause() {
  const auto model = compatibleSaveBrowserModel();
  const auto starterContext = saveBrowserContextFor(
      iggy3d::FrontendScreen::Starter,
      iggy3d::FrontendScreen::LoadSave,
      model);
  const auto pauseContext = saveBrowserContextFor(
      iggy3d::FrontendScreen::Pause,
      iggy3d::FrontendScreen::LoadSave,
      model);

  const auto starterBack = iggy3d::routeProductFrontendAction(
      starterContext,
      iggy3d::FrontendAction::Back);
  const auto pauseBack = iggy3d::routeProductFrontendAction(
      pauseContext,
      iggy3d::FrontendAction::Back);

  return expect(starterBack.routed, "starter save browser back routed") &&
         expect(starterBack.route.accepted,
                "starter save browser back accepted") &&
         expect(starterBack.route.status == "save_browser_closed_to_starter",
                "starter save browser back status") &&
         expect(starterBack.route.nextScreen == iggy3d::FrontendScreen::Starter,
                "starter save browser back screen") &&
         expect(pauseBack.routed, "pause save browser back routed") &&
         expect(pauseBack.route.accepted,
                "pause save browser back accepted") &&
         expect(pauseBack.route.status == "save_browser_closed_to_pause",
                "pause save browser back status") &&
         expect(pauseBack.route.nextScreen == iggy3d::FrontendScreen::Pause,
                "pause save browser back screen");
}

bool saveBrowserLoadAndDeleteSummaryAreReceiptReady() {
  const auto model = compatibleSaveBrowserModel();
  const auto context = saveBrowserContextFor(iggy3d::FrontendScreen::Starter,
                                             iggy3d::FrontendScreen::LoadSave,
                                             model);
  const auto load = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::Load);
  const auto loadSummary = iggy3d::summarizeProductFrontendRoute(
      context,
      load,
      iggy3d::FrontendAction::Load);
  const auto deleteRoute = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::Delete);

  return expect(load.routed, "save browser load routed") &&
         expect(load.route.accepted, "save browser load accepted") &&
         expect(load.route.requestedTransition ==
                    iggy3d::FrontendTransitionRequest::LaunchGameplay,
                "save browser load transition") &&
         expect(load.route.status == "save_browser_load_requested",
                "save browser load status") &&
         expect(loadSummary.activeSurface == "save_selector",
                "save browser load summary surface") &&
         expect(loadSummary.routeModelName == "save_browser",
                "save browser load summary model") &&
         expect(loadSummary.transition == "launch_gameplay",
                "save browser load summary transition") &&
         expect(loadSummary.status == "save_browser_load_requested",
                "save browser load summary status") &&
         expect(deleteRoute.routed, "save browser delete routed") &&
         expect(deleteRoute.route.accepted, "save browser delete accepted") &&
         expect(deleteRoute.route.nextScreen == iggy3d::FrontendScreen::Starter,
                "save browser delete screen") &&
         expect(deleteRoute.route.nextChildScreen ==
                    iggy3d::FrontendScreen::DeleteConfirm,
                "save browser delete child") &&
         expect(deleteRoute.route.status ==
                    "save_browser_delete_confirm_requested",
                "save browser delete status");
}

bool missingSaveBrowserModelIsUnavailable() {
  const auto context = contextFor(iggy3d::FrontendScreen::Starter,
                                 iggy3d::FrontendScreen::LoadSave);
  const auto frame = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::Load);
  const auto summary = iggy3d::summarizeProductFrontendRoute(
      context,
      frame,
      iggy3d::FrontendAction::Load);

  return expect(!frame.routed, "missing save browser not routed") &&
         expect(!frame.route.accepted,
                "missing save browser not accepted") &&
         expect(frame.route.status == "save_browser_model_unavailable",
                "missing save browser status") &&
         expect(!summary.routeModelAvailable,
                "missing save browser summary model unavailable") &&
         expect(summary.routeModelName == "save_browser",
                "missing save browser summary model name");
}

bool gameplayRemainsDeferred() {
  auto gameplay = contextFor(iggy3d::FrontendScreen::Gameplay,
                             iggy3d::FrontendScreen::Gameplay);
  gameplay.gameplayActive = true;
  gameplay.hasActiveSession = true;

  const auto game = iggy3d::routeProductFrontendAction(
      gameplay,
      iggy3d::FrontendAction::Apply);

  return expect(!game.routed, "gameplay deferred") &&
         expect(game.route.status == "product_frontend_route_unavailable",
                "gameplay deferred status");
}

bool starterNewWorldSummaryIsReceiptReady() {
  const auto model =
      iggy3d::buildStarterScreenModel(0U, iggy3d::FrontendAction::NewWorld);
  const auto context =
      starterContextFor(iggy3d::FrontendScreen::Gameplay, model);
  const auto frame = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::NewWorld);
  const auto summary = iggy3d::summarizeProductFrontendRoute(
      context,
      frame,
      iggy3d::FrontendAction::NewWorld);

  return expect(summary.routed, "summary new world routed") &&
         expect(summary.accepted, "summary new world accepted") &&
         expect(summary.inputOwner == "starter", "summary new world owner") &&
         expect(summary.activeSurface == "starter",
                "summary new world active surface") &&
         expect(summary.parentOwner == "none", "summary new world parent") &&
         expect(summary.inputAction == "new_world",
                "summary new world input action") &&
         expect(summary.screenBefore == "starter",
                "summary new world screen before") &&
         expect(summary.childBefore == "gameplay",
                "summary new world child before") &&
         expect(summary.screenAfter == "starter",
                "summary new world screen after") &&
         expect(summary.childAfter == "new_world",
                "summary new world child after") &&
         expect(summary.transition == "none", "summary new world transition") &&
         expect(!summary.closeRequested, "summary new world close") &&
         expect(summary.gameplayInputSuppressed,
                "summary new world suppression") &&
         expect(summary.ownerModelAvailable, "summary new world owner model") &&
         expect(summary.ownerModelName == "starter",
                "summary new world owner model name") &&
         expect(summary.routeModelAvailable, "summary new world route model") &&
         expect(summary.routeModelName == "starter",
                "summary new world route model name") &&
         expect(summary.status == "starter_new_world_opened",
                "summary new world status") &&
         expect(summary.reason == "starter_new_world_opened",
                "summary new world reason");
}

bool compatibleContinueSummaryShowsLaunch() {
  const auto model =
      iggy3d::buildStarterScreenModel(1U, iggy3d::FrontendAction::Continue);
  const auto context =
      starterContextFor(iggy3d::FrontendScreen::Gameplay, model);
  const auto frame = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::Continue);
  const auto summary = iggy3d::summarizeProductFrontendRoute(
      context,
      frame,
      iggy3d::FrontendAction::Continue);

  return expect(summary.routed, "summary continue routed") &&
         expect(summary.accepted, "summary continue accepted") &&
         expect(summary.inputOwner == "gameplay", "summary continue owner") &&
         expect(summary.activeSurface == "starter",
                "summary continue active surface") &&
         expect(summary.transition == "launch_gameplay",
                "summary continue transition") &&
         expect(!summary.gameplayInputSuppressed,
                "summary continue not suppressed") &&
         expect(summary.status == "starter_launch_continue",
                "summary continue status") &&
         expect(summary.reason == "starter_launch_continue",
                "summary continue reason");
}

bool settingsApplySummaryIsReceiptReady() {
  const auto settings = settingsRouteContextFor(
      iggy3d::MenuOwner::Starter,
      iggy3d::FrontendSettingsTab::Input,
      true);
  const auto context = settingsContextFor(iggy3d::FrontendScreen::Starter,
                                          iggy3d::FrontendScreen::Settings,
                                          settings);
  const auto frame = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::Apply);
  const auto summary = iggy3d::summarizeProductFrontendRoute(
      context,
      frame,
      iggy3d::FrontendAction::Apply);

  return expect(summary.routed, "summary settings routed") &&
         expect(summary.accepted, "summary settings accepted") &&
         expect(summary.inputOwner == "settings", "summary settings owner") &&
         expect(summary.activeSurface == "settings",
                "summary settings active surface") &&
         expect(summary.parentOwner == "starter", "summary settings parent") &&
         expect(summary.inputAction == "apply", "summary settings action") &&
         expect(summary.status == "settings_apply_requested",
                "summary settings status") &&
         expect(summary.reason == "settings_apply_requested",
                "summary settings reason") &&
         expect(summary.routeModelAvailable, "summary settings route model") &&
         expect(summary.routeModelName == "settings",
                "summary settings route model name");
}

bool missingStarterModelSummaryIsHonest() {
  const auto context = contextFor(iggy3d::FrontendScreen::Starter,
                                 iggy3d::FrontendScreen::Gameplay);
  const auto frame = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::NewWorld);
  const auto summary = iggy3d::summarizeProductFrontendRoute(
      context,
      frame,
      iggy3d::FrontendAction::NewWorld);

  return expect(!summary.routed, "summary missing starter not routed") &&
         expect(!summary.accepted, "summary missing starter not accepted") &&
         expect(!summary.routeModelAvailable,
                "summary missing starter route model unavailable") &&
         expect(summary.routeModelName == "starter",
                "summary missing starter route model name") &&
         expect(summary.status == "starter_model_unavailable",
                "summary missing starter status") &&
         expect(summary.reason == "starter_model_unavailable",
                "summary missing starter reason");
}

bool missingSettingsContextSummaryIsHonest() {
  const auto context = contextFor(iggy3d::FrontendScreen::Starter,
                                 iggy3d::FrontendScreen::Settings);
  const auto frame = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::Apply);
  const auto summary = iggy3d::summarizeProductFrontendRoute(
      context,
      frame,
      iggy3d::FrontendAction::Apply);

  return expect(!summary.routed, "summary missing settings not routed") &&
         expect(!summary.accepted, "summary missing settings not accepted") &&
         expect(!summary.routeModelAvailable,
                "summary missing settings route model unavailable") &&
         expect(summary.routeModelName == "settings",
                "summary missing settings route model name") &&
         expect(summary.status == "settings_model_unavailable",
                "summary missing settings status") &&
         expect(summary.reason == "settings_model_unavailable",
                "summary missing settings reason");
}

bool deferredGameplaySummaryDoesNotClaimMissingModel() {
  auto context = contextFor(iggy3d::FrontendScreen::Gameplay,
                            iggy3d::FrontendScreen::Gameplay);
  context.gameplayActive = true;
  context.hasActiveSession = true;
  const auto frame = iggy3d::routeProductFrontendAction(
      context,
      iggy3d::FrontendAction::Apply);
  const auto summary = iggy3d::summarizeProductFrontendRoute(
      context,
      frame,
      iggy3d::FrontendAction::Apply);

  return expect(!summary.routed, "summary gameplay not routed") &&
         expect(!summary.accepted, "summary gameplay not accepted") &&
         expect(summary.routeModelAvailable,
                "summary gameplay route model available") &&
         expect(summary.routeModelName == "gameplay",
                "summary gameplay route model name") &&
         expect(summary.status == "product_frontend_route_unavailable",
                "summary gameplay status") &&
         expect(summary.reason == "product_frontend_route_unavailable",
                "summary gameplay reason");
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
                  inputOwnerCacheSyncUsesResolvedActiveSurface() &&
                  starterRootDelegatesNewWorld() &&
                  starterRootContinueDelegatesLaunchWhenCompatible() &&
                  starterRootContinueReportsDisabledWithoutCompatibleSave() &&
                  starterRootDelegatesSettingsAndDevTools() &&
                  missingStarterModelIsUnavailable() &&
                  starterChildBackDelegatesToStarterHelper() &&
                  starterChildNonBackIsUnavailable() &&
                  worldSetupBackDelegatesThroughDraft() &&
                  worldSetupValidCreateCarriesRequestWithoutTransition() &&
                  worldSetupInvalidCreateAndUnsupportedStayOnDraft() &&
                  missingWorldSetupDraftIsUnavailable() &&
                  starterSettingsApplyDelegatesToSettingsHelper() &&
                  starterSettingsBackReturnsToStarter() &&
                  pauseSettingsBackReturnsToPause() &&
                  settingsDisabledRowDelegatesReason() &&
                  settingsCleanApplyReportsNoChanges() &&
                  missingSettingsContextIsUnavailable() &&
                  settingsInvalidParentDelegatesReason() &&
                  pauseRouteDelegatesResume() && missingPauseModelIsUnavailable() &&
                  disabledPauseLoadSaveDelegatesReason() &&
                  pauseSettingsRouteSummaryIsReceiptReady() &&
                  devToolsRoutesFromStarterAndGameplay() &&
                  missingDevToolsModelIsUnavailable() &&
                  devToolsApplySummaryIsReceiptReady() &&
                  saveBrowserRoutesFromStarterAndPause() &&
                  saveBrowserLoadAndDeleteSummaryAreReceiptReady() &&
                  missingSaveBrowserModelIsUnavailable() &&
                  gameplayRemainsDeferred() &&
                  starterNewWorldSummaryIsReceiptReady() &&
                  compatibleContinueSummaryShowsLaunch() &&
                  settingsApplySummaryIsReceiptReady() &&
                  missingStarterModelSummaryIsHonest() &&
                  missingSettingsContextSummaryIsHonest() &&
                  deferredGameplaySummaryDoesNotClaimMissingModel();
  return ok ? 0 : 1;
}
