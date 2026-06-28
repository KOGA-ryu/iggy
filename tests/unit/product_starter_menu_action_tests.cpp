#include "app/iggy3d/menu/ActionHandlers.hpp"

#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::SaveSlotPreview compatibleSlot(std::string_view id) {
  iggy3d::SaveSlotPreview slot;
  slot.id = std::string{id};
  slot.enabled = true;
  slot.reason = "compatible";
  slot.displayTitle = std::string{id};
  return slot;
}

struct StarterHarness {
  iggy3d::FrontendState frontend;
  iggy3d::ProductAppOptions options;
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::WorldSetupDraft draft = iggy3d::makeDefaultWorldSetupDraft("starter_seed");
  iggy3d::ProductAppWindowState window;
  bool closeRequested = false;

  StarterHarness() {
    frontend.screen = iggy3d::FrontendScreen::Starter;
    frontend.childScreen = iggy3d::FrontendScreen::Gameplay;
    frontend.selectedAction = iggy3d::FrontendAction::Continue;
    frontend.status = "starter_screen_ready";
  }
};

iggy3d::ProductMenuActionResult applyStarterAction(
    StarterHarness& harness,
    iggy3d::InputAction action) {
  return iggy3d::applyProductStarterMenuAction(
      action,
      {
          harness.frontend,
          harness.options,
          harness.saves,
          harness.settingsTab,
          harness.activeSession,
          harness.draft,
          harness.window,
          harness.closeRequested,
      });
}

bool selectionAndBackAreStable() {
  StarterHarness harness;

  const iggy3d::ProductMenuActionResult down =
      applyStarterAction(harness, iggy3d::InputAction::MenuDown);
  const bool downOk =
      expect(down.handled, "down handled") &&
      expect(down.accepted, "down accepted") &&
      expect(harness.frontend.selectedAction == iggy3d::FrontendAction::NewWorld,
             "down selects new world") &&
      expect(harness.frontend.status == "opening_menu_selection_changed",
             "down status");

  const iggy3d::ProductMenuActionResult up =
      applyStarterAction(harness, iggy3d::InputAction::MenuUp);
  const bool upOk =
      expect(up.handled, "up handled") &&
      expect(up.accepted, "up accepted") &&
      expect(harness.frontend.selectedAction == iggy3d::FrontendAction::Continue,
             "up returns to continue");

  const iggy3d::ProductMenuActionResult back =
      applyStarterAction(harness, iggy3d::InputAction::MenuBack);
  return downOk && upOk && expect(back.handled, "back handled") &&
         expect(back.accepted, "back accepted") &&
         expect(harness.closeRequested, "back requests close") &&
         expect(harness.frontend.status == "opening_menu_back_requested",
                "back status");
}

bool disabledContinueStaysOnStarter() {
  StarterHarness harness;
  harness.frontend.selectedAction = iggy3d::FrontendAction::Continue;

  const iggy3d::ProductMenuActionResult result =
      applyStarterAction(harness, iggy3d::InputAction::MenuConfirm);

  return expect(result.handled, "continue handled") &&
         expect(result.accepted, "continue accepted") &&
         expect(harness.frontend.screen == iggy3d::FrontendScreen::Starter,
                "continue stays starter") &&
         expect(harness.frontend.childScreen == iggy3d::FrontendScreen::Gameplay,
                "continue keeps root child") &&
         expect(harness.frontend.status == "opening_menu_action_disabled",
                "continue disabled status") &&
         expect(!harness.closeRequested, "continue does not close");
}

bool childPanelActionsOpenExpectedSurfaces() {
  StarterHarness newWorld;
  newWorld.frontend.selectedAction = iggy3d::FrontendAction::NewWorld;
  const iggy3d::ProductMenuActionResult newWorldResult =
      applyStarterAction(newWorld, iggy3d::InputAction::MenuConfirm);

  StarterHarness loadSave;
  loadSave.frontend.selectedAction = iggy3d::FrontendAction::LoadSave;
  loadSave.saves.slots.slots.push_back(compatibleSlot("save_unit"));
  loadSave.saves.slots.compatibleCount = 1U;
  const iggy3d::ProductMenuActionResult loadResult =
      applyStarterAction(loadSave, iggy3d::InputAction::MenuConfirm);

  StarterHarness settings;
  settings.frontend.selectedAction = iggy3d::FrontendAction::Settings;
  const iggy3d::ProductMenuActionResult settingsResult =
      applyStarterAction(settings, iggy3d::InputAction::MenuConfirm);

  StarterHarness devTools;
  devTools.frontend.selectedAction = iggy3d::FrontendAction::DevTools;
  const iggy3d::ProductMenuActionResult devToolsResult =
      applyStarterAction(devTools, iggy3d::InputAction::MenuConfirm);

  return expect(newWorldResult.handled && newWorldResult.accepted,
                "new world accepted") &&
         expect(newWorld.frontend.childScreen == iggy3d::FrontendScreen::NewWorld,
                "new world child") &&
         expect(newWorld.frontend.selectedAction ==
                    iggy3d::FrontendAction::CreateAndEnter,
                "new world selects create") &&
         expect(newWorld.window.worldSetupStatus == "world_setup_open",
                "new world records draft state") &&
         expect(loadResult.handled && loadResult.accepted, "load accepted") &&
         expect(loadSave.frontend.childScreen == iggy3d::FrontendScreen::LoadSave,
                "load child") &&
         expect(loadSave.window.selectedProductSaveId == "save_unit",
                "load selected save") &&
         expect(settingsResult.handled && settingsResult.accepted,
                "settings accepted") &&
         expect(settings.frontend.childScreen == iggy3d::FrontendScreen::Settings,
                "settings child") &&
         expect(settings.settingsTab == iggy3d::FrontendSettingsTab::Input,
                "settings starts input tab") &&
         expect(devToolsResult.handled && devToolsResult.accepted,
                "dev tools accepted") &&
         expect(devTools.frontend.childScreen ==
                    iggy3d::FrontendScreen::StarterDevTools,
                "dev tools child") &&
         expect(devTools.frontend.devToolsOpen, "dev tools open") &&
         expect(devTools.frontend.devToolsCategory ==
                    iggy3d::FrontendDevToolsCategory::Session,
                "dev tools starts session");
}

bool deleteAndExitActionsAreExplicitRows() {
  StarterHarness deleteSave;
  deleteSave.frontend.selectedAction = iggy3d::FrontendAction::Delete;
  deleteSave.saves.slots.slots.push_back(compatibleSlot("save_unit"));
  deleteSave.saves.slots.compatibleCount = 1U;
  const iggy3d::ProductMenuActionResult deleteResult =
      applyStarterAction(deleteSave, iggy3d::InputAction::MenuConfirm);

  StarterHarness exit;
  exit.frontend.selectedAction = iggy3d::FrontendAction::Exit;
  const iggy3d::ProductMenuActionResult exitResult =
      applyStarterAction(exit, iggy3d::InputAction::MenuConfirm);

  return expect(deleteResult.handled && deleteResult.accepted,
                "delete accepted") &&
         expect(deleteSave.frontend.childScreen ==
                    iggy3d::FrontendScreen::DeleteConfirm,
                "delete confirm child") &&
         expect(deleteSave.window.saveDeleteConfirmationOpen,
                "delete confirmation open") &&
         expect(deleteSave.window.saveDeleteCandidateId == "save_unit",
                "delete candidate") &&
         expect(!deleteSave.closeRequested, "delete does not close") &&
         expect(exitResult.handled && exitResult.accepted, "exit accepted") &&
         expect(exit.closeRequested, "exit requests close") &&
         expect(exit.frontend.status == "opening_menu_exit_requested",
                "exit status");
}

}  // namespace

int main() {
  const bool ok = selectionAndBackAreStable() && disabledContinueStaysOnStarter() &&
                  childPanelActionsOpenExpectedSurfaces() &&
                  deleteAndExitActionsAreExplicitRows();
  return ok ? 0 : 1;
}
