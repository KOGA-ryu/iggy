#include "app/iggy3d/menu/ActionHandlers.hpp"

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/Operations.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/iggy3d/world/CreativeWorldService.hpp"

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
  iggy3d::creative::Facade* creativeFacade = nullptr;

  StarterHarness() {
    frontend.screen = iggy3d::FrontendScreen::Starter;
    frontend.childScreen = iggy3d::FrontendScreen::Gameplay;
    frontend.selectedAction = iggy3d::FrontendAction::Continue;
    frontend.status = "starter_screen_ready";
  }
};

std::filesystem::path testRoot(std::string_view name) {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "iggy3d_starter_action" /
      std::string{name};
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root, error);
  return root;
}

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
          harness.creativeFacade,
      });
}

void showMovementTuning(StarterHarness& harness) {
  harness.window.gameplayMovementTuningVisible = true;
  harness.window.gameplayMovementTuningStatus = "movement_tuning_visible";
  harness.window.gameplayMovementTuningReasonCode =
      harness.window.gameplayMovementTuningStatus;
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
  showMovementTuning(newWorld);
  const iggy3d::ProductMenuActionResult newWorldResult =
      applyStarterAction(newWorld, iggy3d::InputAction::MenuConfirm);

  StarterHarness loadSave;
  loadSave.frontend.selectedAction = iggy3d::FrontendAction::LoadSave;
  loadSave.saves.slots.slots.push_back(compatibleSlot("save_unit"));
  loadSave.saves.slots.compatibleCount = 1U;
  showMovementTuning(loadSave);
  const iggy3d::ProductMenuActionResult loadResult =
      applyStarterAction(loadSave, iggy3d::InputAction::MenuConfirm);

  StarterHarness settings;
  settings.frontend.selectedAction = iggy3d::FrontendAction::Settings;
  showMovementTuning(settings);
  const iggy3d::ProductMenuActionResult settingsResult =
      applyStarterAction(settings, iggy3d::InputAction::MenuConfirm);

  StarterHarness devTools;
  devTools.frontend.selectedAction = iggy3d::FrontendAction::DevTools;
  showMovementTuning(devTools);
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
         expect(!newWorld.window.gameplayMovementTuningVisible,
                "new world clears movement tuning") &&
         expect(loadResult.handled && loadResult.accepted, "load accepted") &&
         expect(loadSave.frontend.childScreen == iggy3d::FrontendScreen::LoadSave,
                "load child") &&
         expect(loadSave.window.selectedProductSaveId == "save_unit",
                "load selected save") &&
         expect(!loadSave.window.gameplayMovementTuningVisible,
                "load clears movement tuning") &&
         expect(settingsResult.handled && settingsResult.accepted,
                "settings accepted") &&
         expect(settings.frontend.childScreen == iggy3d::FrontendScreen::Settings,
                "settings child") &&
         expect(settings.settingsTab == iggy3d::FrontendSettingsTab::Input,
                "settings starts input tab") &&
         expect(!settings.window.gameplayMovementTuningVisible,
                "settings clears movement tuning") &&
         expect(devToolsResult.handled && devToolsResult.accepted,
                "dev tools accepted") &&
         expect(devTools.frontend.childScreen ==
                    iggy3d::FrontendScreen::StarterDevTools,
                "dev tools child") &&
         expect(iggy3d::frontendDevToolsOpen(devTools.frontend),
                "dev tools open") &&
         expect(devTools.frontend.devToolsCategory ==
                    iggy3d::FrontendDevToolsCategory::Session,
                "dev tools starts session") &&
         expect(!devTools.window.gameplayMovementTuningVisible,
                "dev tools clears movement tuning");
}

bool deleteAndExitActionsAreExplicitRows() {
  StarterHarness deleteSave;
  deleteSave.frontend.selectedAction = iggy3d::FrontendAction::Delete;
  deleteSave.saves.slots.slots.push_back(compatibleSlot("save_unit"));
  deleteSave.saves.slots.compatibleCount = 1U;
  showMovementTuning(deleteSave);
  const iggy3d::ProductMenuActionResult deleteResult =
      applyStarterAction(deleteSave, iggy3d::InputAction::MenuConfirm);

  StarterHarness exit;
  exit.frontend.selectedAction = iggy3d::FrontendAction::Exit;
  showMovementTuning(exit);
  const iggy3d::ProductMenuActionResult exitResult =
      applyStarterAction(exit, iggy3d::InputAction::MenuConfirm);

  return expect(deleteResult.handled && deleteResult.accepted,
                "delete accepted") &&
         // Delete now opens the selectable save browser (LoadSave in Delete
         // mode) so the operator chooses which map; the confirmation only opens
         // once they confirm a selected slot inside the browser.
         expect(deleteSave.frontend.childScreen ==
                    iggy3d::FrontendScreen::LoadSave,
                "delete opens selectable browser") &&
         expect(deleteSave.frontend.saveBrowserMode ==
                    iggy3d::FrontendSaveBrowserMode::Delete,
                "delete opens explicit delete browser mode") &&
         expect(deleteSave.window.saveSlotBrowserMode == "delete",
                "delete records browser mode") &&
         expect(deleteSave.frontend.selectedAction ==
                    iggy3d::FrontendAction::Delete,
                "delete keeps starter action proof") &&
         expect(deleteSave.frontend.status == "delete_world_browser_open",
                "delete browser status") &&
         expect(!deleteSave.window.saveDeleteConfirmationOpen,
                "delete does not auto-open confirmation") &&
         expect(deleteSave.window.selectedProductSaveId == "save_unit",
                "delete pre-selects first save") &&
         expect(!deleteSave.window.gameplayMovementTuningVisible,
                "delete clears movement tuning") &&
         expect(!deleteSave.closeRequested, "delete does not close") &&
         expect(exitResult.handled && exitResult.accepted, "exit accepted") &&
         expect(exit.closeRequested, "exit requests close") &&
         expect(!exit.window.gameplayMovementTuningVisible,
                "exit clears movement tuning") &&
         expect(exit.frontend.status == "opening_menu_exit_requested",
                "exit status");
}

bool creativeNewWorldLaunchesThroughStarterActionAndKeepsContinueSeparate() {
  StarterHarness harness;
  iggy3d::creative::Facade facade;
  harness.creativeFacade = &facade;
  harness.options.saveRoot = testRoot("creative_launch");
  harness.frontend.selectedAction = iggy3d::FrontendAction::CreativeNewWorld;
  harness.draft.worldName = "Starter Creative";
  showMovementTuning(harness);

  const iggy3d::ProductMenuActionResult result =
      applyStarterAction(harness, iggy3d::InputAction::MenuConfirm);
  const iggy3d::ProductSaveBridgeResult creativeSaves =
      iggy3d::scanProductSaves(harness.options.saveRoot,
                               "iggy3d.creative",
                               "creative.document");
  const bool hasCreativeEntry =
      creativeSaves.catalog.catalog.entries.size() == 1U;
  const iggy3d::ProductSaveCatalogEntry creativeEntry =
      hasCreativeEntry ? creativeSaves.catalog.catalog.entries.front()
                       : iggy3d::ProductSaveCatalogEntry{};
  const iggy3d::ProductContinueSelectionResult productContinue =
      iggy3d::selectProductContinueSave(creativeSaves.catalog.catalog);
  const iggy3d::ProductContinueSelectionResult contextContinue =
      iggy3d::selectProductContinueSave(harness.saves.catalog.catalog);

  return expect(result.handled && result.accepted,
                "creative starter handled") &&
         expect(harness.activeSession.has_value(),
                "creative starter runtime session") &&
         expect(harness.window.gameplayActive,
                "creative starter gameplay active") &&
         expect(harness.frontend.screen == iggy3d::FrontendScreen::Gameplay,
                "creative starter frontend gameplay") &&
         expect(harness.window.interactionMode ==
                    iggy3d::ProductInteractionMode::Creative,
                "creative starter interaction mode") &&
         expect(harness.window.launchStatus ==
                    "product_creative_world_launched",
                "creative starter launch status") &&
         expect(!harness.window.gameplayMovementTuningVisible,
                "creative starter clears movement tuning") &&
         expect(facade.document().id() !=
                    iggy3d::creative::kInvalidDocumentId,
                "creative starter document id") &&
         expect(facade.document().name() == "Starter Creative",
                "creative starter document title") &&
         expect(facade.document().objectCount() == 0U,
                "creative starter object count") &&
         expect(facade.document().revision() == 0U,
                "creative starter revision clean") &&
         expect(facade.document().dirtyFlags() == 0U,
                "creative starter dirty clean") &&
         expect(hasCreativeEntry, "creative starter save entry") &&
         expect(creativeEntry.contentKind ==
                    iggy3d::ProductSaveContentKind::CreativeDocument,
                "creative starter catalog kind") &&
         expect(iggy3d::canOpenCreativeWorld(creativeEntry),
                "creative starter can open creative") &&
         expect(!iggy3d::canLoadProductSave(creativeEntry),
                "creative starter not product loadable") &&
         expect(!productContinue.selected,
                "creative starter continue ignores creative scan") &&
         expect(!contextContinue.selected,
                "creative starter refreshed context keeps continue disabled");
}

bool creativeNewWorldMissingFacadeFailsClosed() {
  StarterHarness harness;
  harness.options.saveRoot = testRoot("creative_missing_facade");
  harness.frontend.selectedAction = iggy3d::FrontendAction::CreativeNewWorld;
  harness.draft.worldName = "Missing Facade Creative";
  showMovementTuning(harness);

  const iggy3d::ProductMenuActionResult result =
      applyStarterAction(harness, iggy3d::InputAction::MenuConfirm);
  const iggy3d::ProductSaveBridgeResult creativeSaves =
      iggy3d::scanProductSaves(harness.options.saveRoot,
                               "iggy3d.creative",
                               "creative.document");

  return expect(result.handled && result.accepted,
                "missing facade handled") &&
         expect(!harness.activeSession.has_value(),
                "missing facade no session") &&
         expect(!harness.window.gameplayActive,
                "missing facade no gameplay") &&
         expect(harness.window.interactionMode ==
                    iggy3d::ProductInteractionMode::Player,
                "missing facade player mode") &&
         expect(harness.window.launchStatus ==
                    "product_creative_world_facade_missing",
                "missing facade launch status") &&
         expect(harness.frontend.status ==
                    "product_creative_world_facade_missing",
                "missing facade frontend status") &&
         expect(!harness.window.gameplayMovementTuningVisible,
                "missing facade clears movement tuning") &&
         expect(creativeSaves.catalog.catalog.entries.empty(),
                "missing facade writes no save");
}

bool creativeOpenWorldLaunchesThroughStarterAction() {
  StarterHarness harness;
  iggy3d::creative::Facade facade;
  harness.creativeFacade = &facade;
  harness.options.saveRoot = testRoot("creative_open");

  iggy3d::CreativeWorldCreateRequest create;
  create.saveRoot = harness.options.saveRoot;
  create.title = "Saved Creative";
  create.requestedAtUtc = "2026-07-03T18:00:00Z";
  const iggy3d::CreativeWorldCreateResult created =
      iggy3d::createCreativeWorld(create);

  harness.frontend.selectedAction = iggy3d::FrontendAction::CreativeOpenWorld;
  showMovementTuning(harness);
  const iggy3d::ProductMenuActionResult result =
      applyStarterAction(harness, iggy3d::InputAction::MenuConfirm);
  const iggy3d::ProductSaveBridgeResult creativeSaves =
      iggy3d::scanProductSaves(harness.options.saveRoot,
                               "iggy3d.creative",
                               "creative.document");
  const iggy3d::ProductContinueSelectionResult productContinue =
      iggy3d::selectProductContinueSave(creativeSaves.catalog.catalog);

  return expect(created.accepted, "creative open setup create accepted") &&
         expect(result.handled && result.accepted,
                "creative open handled") &&
         expect(harness.activeSession.has_value(),
                "creative open runtime session") &&
         expect(harness.window.gameplayActive,
                "creative open gameplay active") &&
         expect(harness.frontend.screen == iggy3d::FrontendScreen::Gameplay,
                "creative open frontend gameplay") &&
         expect(harness.window.interactionMode ==
                    iggy3d::ProductInteractionMode::Creative,
                "creative open interaction mode") &&
         expect(harness.window.launchStatus ==
                    "product_creative_world_opened",
                "creative open launch status") &&
         expect(!harness.window.gameplayMovementTuningVisible,
                "creative open clears movement tuning") &&
         expect(facade.document().id() == created.documentId,
                "creative open document id") &&
         expect(facade.document().name() == "Saved Creative",
                "creative open document title") &&
         expect(facade.document().objectCount() == 0U,
                "creative open object count") &&
         expect(facade.document().revision() == 0U,
                "creative open revision clean") &&
         expect(facade.document().dirtyFlags() == 0U,
                "creative open dirty clean") &&
         expect(harness.window.activeCreativeSaveId == created.saveId,
                "creative open active creative save id") &&
         expect(harness.window.activeCreativeSavePath ==
                    created.path.generic_string(),
                "creative open active creative save path") &&
         expect(harness.window.activeCreativeWorldId == created.worldId,
                "creative open active creative world id") &&
         expect(harness.window.activeCreativeDocumentId == created.documentId,
                "creative open active creative document id") &&
         expect(harness.window.activeCreativeObjectCount == 0U,
                "creative open active creative object count") &&
         expect(harness.window.activeCreativeNextObjectId ==
                    facade.document().nextObjectId(),
                "creative open active creative next id") &&
         expect(harness.window.activeProductSaveId == "none",
                "creative open does not set product save id") &&
         expect(!productContinue.selected,
                "creative open keeps product continue separate");
}

bool creativeOpenWorldWithoutCreativeSaveFailsClosed() {
  StarterHarness harness;
  iggy3d::creative::Facade facade;
  harness.creativeFacade = &facade;
  harness.options.saveRoot = testRoot("creative_open_empty");
  harness.frontend.selectedAction = iggy3d::FrontendAction::CreativeOpenWorld;
  showMovementTuning(harness);

  const iggy3d::ProductMenuActionResult result =
      applyStarterAction(harness, iggy3d::InputAction::MenuConfirm);

  return expect(result.handled && result.accepted,
                "creative open empty handled") &&
         expect(!harness.activeSession.has_value(),
                "creative open empty no session") &&
         expect(!harness.window.gameplayActive,
                "creative open empty no gameplay") &&
         expect(harness.window.interactionMode ==
                    iggy3d::ProductInteractionMode::Player,
                "creative open empty stays player") &&
         expect(harness.window.launchStatus ==
                    "creative_open_no_active_saves",
                "creative open empty launch status") &&
         expect(harness.frontend.status ==
                    "opening_menu_creative_open_world_unavailable",
                "creative open empty frontend status") &&
         expect(!harness.window.gameplayMovementTuningVisible,
                "creative open empty clears movement tuning") &&
         expect(facade.document().id() ==
                    iggy3d::creative::kInvalidDocumentId,
                "creative open empty no install");
}

bool creativeOpenWorldMissingFacadeFailsClosed() {
  StarterHarness harness;
  harness.options.saveRoot = testRoot("creative_open_missing_facade");

  iggy3d::CreativeWorldCreateRequest create;
  create.saveRoot = harness.options.saveRoot;
  create.title = "Saved Creative Missing Facade";
  create.requestedAtUtc = "2026-07-03T18:15:00Z";
  const iggy3d::CreativeWorldCreateResult created =
      iggy3d::createCreativeWorld(create);

  harness.frontend.selectedAction = iggy3d::FrontendAction::CreativeOpenWorld;
  showMovementTuning(harness);
  const iggy3d::ProductMenuActionResult result =
      applyStarterAction(harness, iggy3d::InputAction::MenuConfirm);

  return expect(created.accepted,
                "creative open missing facade setup create accepted") &&
         expect(result.handled && result.accepted,
                "creative open missing facade handled") &&
         expect(!harness.activeSession.has_value(),
                "creative open missing facade no session") &&
         expect(!harness.window.gameplayActive,
                "creative open missing facade no gameplay") &&
         expect(harness.window.interactionMode ==
                    iggy3d::ProductInteractionMode::Player,
                "creative open missing facade stays player") &&
         expect(harness.window.launchStatus ==
                    "product_creative_world_facade_missing",
                "creative open missing facade launch status") &&
         expect(harness.frontend.status ==
                    "product_creative_world_facade_missing",
                "creative open missing facade frontend status") &&
         expect(!harness.window.gameplayMovementTuningVisible,
                "creative open missing facade clears movement tuning");
}

bool creativeOpenWorldIgnoresProductOnlySaves() {
  StarterHarness setup;
  setup.options.saveRoot = testRoot("creative_open_product_only");
  setup.draft.worldName = "Product Only";
  iggy3d::launchProductNewWorld(setup.options,
                                setup.draft,
                                setup.frontend,
                                setup.activeSession,
                                setup.window);

  StarterHarness harness;
  iggy3d::creative::Facade facade;
  harness.creativeFacade = &facade;
  harness.options.saveRoot = setup.options.saveRoot;
  harness.frontend.selectedAction = iggy3d::FrontendAction::CreativeOpenWorld;
  showMovementTuning(harness);

  const iggy3d::ProductMenuActionResult result =
      applyStarterAction(harness, iggy3d::InputAction::MenuConfirm);
  const iggy3d::ProductSaveBridgeResult productSaves =
      iggy3d::scanProductSaves(harness.options.saveRoot, "", "");
  const iggy3d::ProductContinueSelectionResult productContinue =
      iggy3d::selectProductContinueSave(productSaves.catalog.catalog);

  return expect(setup.activeSession.has_value(),
                "product-only setup session created") &&
         expect(result.handled && result.accepted,
                "creative open product-only handled") &&
         expect(!harness.activeSession.has_value(),
                "creative open product-only no session") &&
         expect(!harness.window.gameplayActive,
                "creative open product-only no gameplay") &&
         expect(harness.window.launchStatus ==
                    "creative_open_no_openable_saves",
                "creative open product-only launch status") &&
         expect(harness.frontend.status ==
                    "opening_menu_creative_open_world_unavailable",
                "creative open product-only frontend status") &&
         expect(productContinue.selected,
                "product continue still sees product save") &&
         expect(productContinue.selectedSaveId != "none",
                "product continue selected product save") &&
         expect(harness.window.activeCreativeSaveId == "none",
                "creative open product-only no active creative id") &&
         expect(harness.window.activeProductSaveId == "none",
                "creative open product-only no active product id");
}

}  // namespace

int main() {
  const bool ok = selectionAndBackAreStable() && disabledContinueStaysOnStarter() &&
                  childPanelActionsOpenExpectedSurfaces() &&
                  deleteAndExitActionsAreExplicitRows() &&
                  creativeNewWorldLaunchesThroughStarterActionAndKeepsContinueSeparate() &&
                  creativeNewWorldMissingFacadeFailsClosed() &&
                  creativeOpenWorldLaunchesThroughStarterAction() &&
                  creativeOpenWorldWithoutCreativeSaveFailsClosed() &&
                  creativeOpenWorldMissingFacadeFailsClosed() &&
                  creativeOpenWorldIgnoresProductOnlySaves();
  return ok ? 0 : 1;
}
