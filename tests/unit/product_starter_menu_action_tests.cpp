#include "app/iggy3d/menu/ActionHandlers.hpp"

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/menu/InputRouter.hpp"
#include "app/iggy3d/Operations.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/iggy3d/creative/bridge/UiCommandFrame.hpp"
#include "app/iggy3d/creative/world/WorldService.hpp"

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
  iggy3d::creative::CreativeAppState* creativeApp = nullptr;

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
          harness.creativeApp,
      });
}

iggy3d::ProductMenuActionResult applyPauseConfirmAction(
    StarterHarness& harness,
    iggy3d::FrontendAction action,
    iggy3d::creative::CreativeAppState* app) {
  iggy3d::FrontendSettings settings;
  (void)iggy3d::applyProductSystemPauseMenuAction(
      iggy3d::InputAction::SystemPause,
      {harness.frontend, harness.window, harness.closeRequested});
  harness.frontend.selectedAction = action;
  return iggy3d::applyProductPauseMenuAction(
      iggy3d::InputAction::MenuConfirm,
      {
          harness.frontend,
          harness.options,
          harness.saves,
          harness.settingsTab,
          harness.activeSession,
          harness.window,
          harness.closeRequested,
          settings,
          app,
      });
}

iggy3d::ProductCreativeUiCommandFrameReceipt routeCreateRoomCommand(
    iggy3d::creative::CreativeAppState& app) {
  iggy3d::ProductCreativeUiInputFrameReceipt input;
  input.requested = true;
  input.clickPresent = true;
  input.drawListAvailable = true;
  input.routed = true;
  input.hit = true;
  input.consumed = true;
  input.enabled = true;
  input.semanticId = "creative.row.create.create_room";

  iggy3d::ProductCreativeUiCommandFrameRequest request;
  request.creative = &app;
  request.inputReceipt = input;
  return iggy3d::routeProductCreativeUiCommandFrame(request);
}

bool productContinueSelectsNoSave(const std::filesystem::path& root) {
  const iggy3d::ProductSaveBridgeResult saves =
      iggy3d::scanProductSaves(root, "", "");
  return !iggy3d::selectProductContinueSave(saves.catalog.catalog).selected;
}

void showMovementTuning(StarterHarness& harness) {
  harness.window.gameplayMovement.tuningVisible = true;
  harness.window.gameplayMovement.tuningStatus = "movement_tuning_visible";
  harness.window.gameplayMovement.tuningReasonCode =
      harness.window.gameplayMovement.tuningStatus;
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
         expect(newWorld.window.worldSetup.status == "world_setup_open",
                "new world records draft state") &&
         expect(!newWorld.window.gameplayMovement.tuningVisible,
                "new world clears movement tuning") &&
         expect(loadResult.handled && loadResult.accepted, "load accepted") &&
         expect(loadSave.frontend.childScreen == iggy3d::FrontendScreen::LoadSave,
                "load child") &&
         expect(loadSave.window.selectedProductSave.id == "save_unit",
                "load selected save") &&
         expect(!loadSave.window.gameplayMovement.tuningVisible,
                "load clears movement tuning") &&
         expect(settingsResult.handled && settingsResult.accepted,
                "settings accepted") &&
         expect(settings.frontend.childScreen == iggy3d::FrontendScreen::Settings,
                "settings child") &&
         expect(settings.settingsTab == iggy3d::FrontendSettingsTab::Input,
                "settings starts input tab") &&
         expect(!settings.window.gameplayMovement.tuningVisible,
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
         expect(!devTools.window.gameplayMovement.tuningVisible,
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
         expect(!deleteSave.window.saveDelete.confirmationOpen,
                "delete does not auto-open confirmation") &&
         expect(deleteSave.window.selectedProductSave.id == "save_unit",
                "delete pre-selects first save") &&
         expect(!deleteSave.window.gameplayMovement.tuningVisible,
                "delete clears movement tuning") &&
         expect(!deleteSave.closeRequested, "delete does not close") &&
         expect(exitResult.handled && exitResult.accepted, "exit accepted") &&
         expect(exit.closeRequested, "exit requests close") &&
         expect(!exit.window.gameplayMovement.tuningVisible,
                "exit clears movement tuning") &&
         expect(exit.frontend.status == "opening_menu_exit_requested",
                "exit status");
}

bool creativeNewWorldLaunchesThroughStarterActionAndKeepsContinueSeparate() {
  StarterHarness harness;
  iggy3d::creative::CreativeAppState app;
  iggy3d::creative::Facade& facade = app.facade;
  harness.creativeApp = &app;
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
         expect(!harness.window.gameplayMovement.tuningVisible,
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
         expect(!harness.window.gameplayMovement.tuningVisible,
                "missing facade clears movement tuning") &&
         expect(creativeSaves.catalog.catalog.entries.empty(),
                "missing facade writes no save");
}

bool creativeStarterLaunchCanOpenAndClosePauseWithMenuBackRoute() {
  StarterHarness harness;
  iggy3d::creative::CreativeAppState app;
  iggy3d::creative::Facade& facade = app.facade;
  harness.creativeApp = &app;
  harness.options.saveRoot = testRoot("creative_pause_resume");
  harness.frontend.selectedAction = iggy3d::FrontendAction::CreativeNewWorld;
  harness.draft.worldName = "Starter Creative Pause";
  iggy3d::FrontendSettings settings;

  const iggy3d::ProductMenuActionResult launched =
      applyStarterAction(harness, iggy3d::InputAction::MenuConfirm);
  const iggy3d::creative::CreativeDocumentId documentId = facade.document().id();

  iggy3d::ActionState openActions;
  iggy3d::routeProductOpeningMenuInput(
      iggy3d::InputAction::MenuBack,
      openActions,
      {
          harness.frontend,
          harness.saves,
          harness.options,
          harness.settingsTab,
          harness.activeSession,
          harness.draft,
          harness.window,
          harness.closeRequested,
          settings,
          &app,
      });

  const bool openedPause =
      expect(harness.frontend.screen == iggy3d::FrontendScreen::Pause,
             "creative menu back opens pause") &&
      expect(harness.window.inputOwner == iggy3d::MenuOwner::Pause,
             "creative pause owns input") &&
      expect(harness.window.gameplayInputSuppressed,
             "creative pause suppresses gameplay") &&
      expect(harness.window.interactionMode ==
                 iggy3d::ProductInteractionMode::Creative,
             "creative pause preserves creative mode");

  iggy3d::ActionState closeActions;
  iggy3d::routeProductOpeningMenuInput(
      iggy3d::InputAction::MenuBack,
      closeActions,
      {
          harness.frontend,
          harness.saves,
          harness.options,
          harness.settingsTab,
          harness.activeSession,
          harness.draft,
          harness.window,
          harness.closeRequested,
          settings,
          &app,
      });

  return expect(launched.handled && launched.accepted,
                "creative pause launch accepted") &&
         expect(documentId != iggy3d::creative::kInvalidDocumentId,
                "creative pause document id") &&
         openedPause &&
         expect(harness.frontend.screen == iggy3d::FrontendScreen::Gameplay,
                "creative second menu back resumes gameplay") &&
         expect(harness.window.inputOwner == iggy3d::MenuOwner::Gameplay,
                "creative resumed gameplay owns input") &&
         expect(!harness.window.gameplayInputSuppressed,
                "creative resumed gameplay unsuppressed") &&
         expect(harness.window.interactionMode ==
                    iggy3d::ProductInteractionMode::Creative,
                "creative resumed mode stays creative") &&
         expect(harness.window.gameplayActive,
                "creative resumed gameplay active") &&
         expect(harness.activeSession.has_value(),
                "creative resumed session alive") &&
         expect(facade.document().id() == documentId,
                "creative resumed facade document unchanged");
}

bool creativeOpenWorldLaunchesThroughStarterAction() {
  StarterHarness harness;
  iggy3d::creative::CreativeAppState app;
  iggy3d::creative::Facade& facade = app.facade;
  harness.creativeApp = &app;
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
         expect(!harness.window.gameplayMovement.tuningVisible,
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
         expect(harness.window.activeCreative.saveId == created.saveId,
                "creative open active creative save id") &&
         expect(harness.window.activeCreative.savePath ==
                    created.path.generic_string(),
                "creative open active creative save path") &&
         expect(harness.window.activeCreative.worldId == created.worldId,
                "creative open active creative world id") &&
         expect(harness.window.activeCreative.documentId == created.documentId,
                "creative open active creative document id") &&
         expect(harness.window.activeCreative.objectCount == 0U,
                "creative open active creative object count") &&
         expect(harness.window.activeCreative.nextObjectId ==
                    facade.document().nextObjectId(),
                "creative open active creative next id") &&
         expect(harness.window.activeProductSaveId == "none",
                "creative open does not set product save id") &&
         expect(!productContinue.selected,
                "creative open keeps product continue separate");
}

bool creativeOpenWorldWithoutCreativeSaveFailsClosed() {
  StarterHarness harness;
  iggy3d::creative::CreativeAppState app;
  iggy3d::creative::Facade& facade = app.facade;
  harness.creativeApp = &app;
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
         expect(!harness.window.gameplayMovement.tuningVisible,
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
         expect(!harness.window.gameplayMovement.tuningVisible,
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
  iggy3d::creative::CreativeAppState app;
  iggy3d::creative::Facade& facade = app.facade;
  harness.creativeApp = &app;
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
         expect(harness.window.activeCreative.saveId == "none",
                "creative open product-only no active creative id") &&
         expect(harness.window.activeProductSaveId == "none",
                "creative open product-only no active product id");
}

bool creativeWorldMinimumLifecycleLoopsThroughStarterCreateSaveExitAndOpen() {
  StarterHarness harness;
  iggy3d::creative::CreativeAppState app;
  iggy3d::creative::Facade& facade = app.facade;
  harness.creativeApp = &app;
  harness.options.saveRoot = testRoot("creative_lifecycle_loop");
  harness.frontend.selectedAction = iggy3d::FrontendAction::CreativeNewWorld;
  harness.draft.worldName = "Lifecycle Creative";

  const iggy3d::ProductMenuActionResult launched =
      applyStarterAction(harness, iggy3d::InputAction::MenuConfirm);
  const bool launchSessionPresent = harness.activeSession.has_value();
  const bool launchGameplayActive = harness.window.gameplayActive;
  const iggy3d::ProductInteractionMode launchInteractionMode =
      harness.window.interactionMode;
  const std::string launchStatus = harness.window.launchStatus;
  const std::string launchedSaveId = harness.window.activeCreative.saveId;
  const std::string launchedSavePath = harness.window.activeCreative.savePath;
  const std::string launchedWorldId = harness.window.activeCreative.worldId;
  const std::uint64_t launchedDocumentId =
      harness.window.activeCreative.documentId;
  const bool launchNoProductSaveId =
      harness.window.activeProductSaveId == "none";
  const std::uint64_t launchObjectCount = facade.document().objectCount();
  const bool productContinueAfterLaunch =
      productContinueSelectsNoSave(harness.options.saveRoot);

  const iggy3d::ProductCreativeUiCommandFrameReceipt createRoom =
      routeCreateRoomCommand(app);
  const iggy3d::creative::CreativeObject* createdRoom =
      facade.findObject(createRoom.createObjectId);
  const iggy3d::creative::CreativeObjectDirtyFlags dirtyBeforeSave =
      facade.document().dirtyFlags();

  const iggy3d::ProductMenuActionResult savedAndExited =
      applyPauseConfirmAction(harness,
                              iggy3d::FrontendAction::SaveAndExit,
                              &app);
  const bool saveExitReturnedStarter =
      harness.frontend.screen == iggy3d::FrontendScreen::Starter;
  const std::string saveExitFrontendStatus{harness.frontend.status};
  const bool saveExitSessionCleared = !harness.activeSession.has_value();
  const bool saveExitGameplayCleared = !harness.window.gameplayActive;
  const iggy3d::ProductInteractionMode saveExitInteractionMode =
      harness.window.interactionMode;
  const std::string saveExitLaunchStatus = harness.window.launchStatus;
  const iggy3d::creative::CreativeObjectDirtyFlags dirtyAfterSaveExit =
      facade.document().dirtyFlags();
  const bool saveExitNoProductSaveId =
      harness.window.activeProductSaveId == "none";
  const bool productContinueAfterSaveExit =
      productContinueSelectsNoSave(harness.options.saveRoot);
  const bool creativeIdentityCleared =
      harness.window.activeCreative.saveId == "none" &&
      harness.window.activeCreative.savePath == "none" &&
      harness.window.activeCreative.worldId == "none" &&
      harness.window.activeCreative.documentId ==
          iggy3d::creative::kInvalidDocumentId &&
      harness.window.activeCreative.objectCount == 0U &&
      harness.window.activeCreative.nextObjectId ==
          iggy3d::creative::kInvalidObjectId;

  iggy3d::creative::CreativeAppState reopenedApp;
  iggy3d::creative::Facade& reopenedFacade = reopenedApp.facade;
  harness.creativeApp = &reopenedApp;
  harness.frontend.selectedAction = iggy3d::FrontendAction::CreativeOpenWorld;
  const iggy3d::ProductMenuActionResult reopened =
      applyStarterAction(harness, iggy3d::InputAction::MenuConfirm);
  const std::span<const iggy3d::creative::CreativeObject> reopenedObjects =
      reopenedFacade.document().objects();
  const iggy3d::creative::CreativeObject* reopenedRoom =
      reopenedObjects.empty() ? nullptr : &reopenedObjects.front();
  const bool productContinueAfterReopen =
      productContinueSelectsNoSave(harness.options.saveRoot);

  return expect(launched.handled && launched.accepted,
                "lifecycle starter create handled") &&
         expect(launchSessionPresent,
                "lifecycle launch active session") &&
         expect(launchGameplayActive,
                "lifecycle launch gameplay active") &&
         expect(launchInteractionMode ==
                    iggy3d::ProductInteractionMode::Creative,
                "lifecycle launch creative mode") &&
         expect(launchStatus == "product_creative_world_launched",
                "lifecycle launch status") &&
         expect(launchedSaveId != "none", "lifecycle launch save id") &&
         expect(launchedSavePath != "none", "lifecycle launch save path") &&
         expect(launchedWorldId != "none", "lifecycle launch world id") &&
         expect(launchedDocumentId !=
                    iggy3d::creative::kInvalidDocumentId,
                "lifecycle launch document id") &&
         expect(launchNoProductSaveId,
                "lifecycle launch no product save id") &&
         expect(launchObjectCount == 0U,
                "lifecycle launch empty document") &&
         expect(productContinueAfterLaunch,
                "lifecycle product continue ignores creative launch") &&
         expect(createRoom.requested, "lifecycle create command requested") &&
         expect(createRoom.commandKind ==
                    iggy3d::ProductCreativeUiCommandKind::CreateObject,
                "lifecycle create command kind") &&
         expect(createRoom.accepted && createRoom.changed,
                "lifecycle create command applied") &&
         expect(createRoom.createRequested &&
                    createRoom.createAccepted &&
                    createRoom.createChanged,
                "lifecycle create receipt applied") &&
         expect(createRoom.createStatus ==
                    iggy3d::creative::CreativeDocumentCreateStatus::Created,
                "lifecycle create status") &&
         expect(createRoom.createObjectId !=
                    iggy3d::creative::kInvalidObjectId,
                "lifecycle create object id") &&
         expect(createRoom.createObjectKind ==
                    iggy3d::creative::CreativeObjectKind::Room,
                "lifecycle create object kind") &&
         expect(facade.document().objectCount() == 1U,
                "lifecycle created one room") &&
         expect(facade.document().nextObjectId() == 2U,
                "lifecycle next id after create") &&
         expect(createdRoom != nullptr, "lifecycle created room exists") &&
         expect(createdRoom != nullptr &&
                    createdRoom->kind ==
                        iggy3d::creative::CreativeObjectKind::Room,
                "lifecycle created room kind") &&
         expect(createdRoom != nullptr &&
                    createdRoom->bounds.min.x == 0.0 &&
                    createdRoom->bounds.min.y == 0.0 &&
                    createdRoom->bounds.min.z == 0.0,
                "lifecycle created room bounds min") &&
         expect(createdRoom != nullptr &&
                    createdRoom->bounds.max.x == 10.0 &&
                    createdRoom->bounds.max.y == 4.0 &&
                    createdRoom->bounds.max.z == 10.0,
                "lifecycle created room bounds max") &&
         expect(createdRoom != nullptr && createdRoom->visible,
                "lifecycle created room visible") &&
         expect(createdRoom != nullptr && !createdRoom->locked,
                "lifecycle created room unlocked") &&
         expect(dirtyBeforeSave != 0U, "lifecycle dirty before save") &&
         expect(savedAndExited.handled && savedAndExited.accepted,
                "lifecycle save exit handled") &&
         expect(saveExitReturnedStarter,
                "lifecycle save exit returns starter") &&
         expect(saveExitFrontendStatus == "returned_to_title",
                "lifecycle save exit returned status") &&
         expect(saveExitSessionCleared,
                "lifecycle save exit clears session") &&
         expect(saveExitGameplayCleared,
                "lifecycle save exit clears gameplay") &&
         expect(saveExitInteractionMode ==
                    iggy3d::ProductInteractionMode::Player,
                "lifecycle save exit player mode") &&
         expect(saveExitLaunchStatus == "product_creative_world_saved",
                "lifecycle save exit launch status") &&
         expect(dirtyAfterSaveExit == 0U,
                "lifecycle save exit drains dirty") &&
         expect(creativeIdentityCleared,
                "lifecycle save exit clears creative identity") &&
         expect(saveExitNoProductSaveId,
                "lifecycle save exit no product save id") &&
         expect(productContinueAfterSaveExit,
                "lifecycle product continue ignores saved creative") &&
         expect(reopened.handled && reopened.accepted,
                "lifecycle reopen handled") &&
         expect(harness.activeSession.has_value(),
                "lifecycle reopen active session") &&
         expect(harness.window.gameplayActive,
                "lifecycle reopen gameplay active") &&
         expect(harness.window.interactionMode ==
                    iggy3d::ProductInteractionMode::Creative,
                "lifecycle reopen creative mode") &&
         expect(harness.window.launchStatus ==
                    "product_creative_world_opened",
                "lifecycle reopen status") &&
         expect(reopenedFacade.document().id() == launchedDocumentId,
                "lifecycle reopened document id") &&
         expect(reopenedFacade.document().name() == "Lifecycle Creative",
                "lifecycle reopened document name") &&
         expect(reopenedFacade.document().objectCount() == 1U,
                "lifecycle reopened object count") &&
         expect(reopenedFacade.document().revision() == 0U,
                "lifecycle reopened revision clean") &&
         expect(reopenedFacade.document().dirtyFlags() == 0U,
                "lifecycle reopened dirty clean") &&
         expect(reopenedFacade.document().nextObjectId() == 2U,
                "lifecycle reopened next id") &&
         expect(reopenedRoom != nullptr, "lifecycle reopened room exists") &&
         expect(reopenedRoom != nullptr &&
                    reopenedRoom->id == createRoom.createObjectId,
                "lifecycle reopened room id") &&
         expect(reopenedRoom != nullptr &&
                    reopenedRoom->kind ==
                        iggy3d::creative::CreativeObjectKind::Room,
                "lifecycle reopened room kind") &&
         expect(reopenedRoom != nullptr &&
                    reopenedRoom->bounds.min.x == 0.0 &&
                    reopenedRoom->bounds.min.y == 0.0 &&
                    reopenedRoom->bounds.min.z == 0.0,
                "lifecycle reopened room bounds min") &&
         expect(reopenedRoom != nullptr &&
                    reopenedRoom->bounds.max.x == 10.0 &&
                    reopenedRoom->bounds.max.y == 4.0 &&
                    reopenedRoom->bounds.max.z == 10.0,
                "lifecycle reopened room bounds max") &&
         expect(reopenedRoom != nullptr && reopenedRoom->visible,
                "lifecycle reopened room visible") &&
         expect(reopenedRoom != nullptr && !reopenedRoom->locked,
                "lifecycle reopened room unlocked") &&
         expect(harness.window.activeCreative.saveId == launchedSaveId,
                "lifecycle reopen restores active creative save") &&
         expect(harness.window.activeCreative.savePath == launchedSavePath,
                "lifecycle reopen restores active creative path") &&
         expect(harness.window.activeCreative.worldId == launchedWorldId,
                "lifecycle reopen restores active creative world") &&
         expect(harness.window.activeCreative.documentId == launchedDocumentId,
                "lifecycle reopen restores active creative document") &&
         expect(harness.window.activeCreative.objectCount == 1U,
                "lifecycle reopen active creative object count") &&
         expect(harness.window.activeCreative.nextObjectId == 2U,
                "lifecycle reopen active creative next id") &&
         expect(harness.window.activeProductSaveId == "none",
                "lifecycle reopen no product save id") &&
         expect(productContinueAfterReopen,
                "lifecycle product continue ignores reopened creative");
}

}  // namespace

int main() {
  const bool ok = selectionAndBackAreStable() && disabledContinueStaysOnStarter() &&
                  childPanelActionsOpenExpectedSurfaces() &&
                  deleteAndExitActionsAreExplicitRows() &&
                  creativeNewWorldLaunchesThroughStarterActionAndKeepsContinueSeparate() &&
                  creativeNewWorldMissingFacadeFailsClosed() &&
                  creativeStarterLaunchCanOpenAndClosePauseWithMenuBackRoute() &&
                  creativeOpenWorldLaunchesThroughStarterAction() &&
                  creativeOpenWorldWithoutCreativeSaveFailsClosed() &&
                  creativeOpenWorldMissingFacadeFailsClosed() &&
                  creativeOpenWorldIgnoresProductOnlySaves() &&
                  creativeWorldMinimumLifecycleLoopsThroughStarterCreateSaveExitAndOpen();
  return ok ? 0 : 1;
}
