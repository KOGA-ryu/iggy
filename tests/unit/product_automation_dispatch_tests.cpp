#include "app/iggy3d/automation/AutomationDispatch.hpp"

#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>

#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/save/Catalog.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

std::filesystem::path testRoot(std::string_view name) {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "iggy3d_automation_dispatch" /
      std::string{name};
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root, error);
  return root;
}

bool creativeNewWorldLaunchesThroughAutomationAppContext() {
  iggy3d::FrontendState frontend;
  frontend.screen = iggy3d::FrontendScreen::Starter;
  frontend.childScreen = iggy3d::FrontendScreen::Gameplay;
  frontend.selectedAction = iggy3d::FrontendAction::Continue;
  frontend.status = "starter_screen_ready";

  iggy3d::ProductAppOptions options;
  options.saveRoot = testRoot("creative_new_world");

  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::FrontendSettings settings;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::WorldSetupDraft draft =
      iggy3d::makeDefaultWorldSetupDraft("automation_seed");
  draft.worldName = "Automation Creative";
  iggy3d::ProductAppWindowState window;
  bool closeRequested = false;
  iggy3d::creative::CreativeAppState app;
  iggy3d::creative::Facade& facade = app.facade;
  facade.reset();

  iggy3d::ProductAutomationAppContext context{
      frontend, saves, options, settings, settingsTab, activeSession, draft,
      window, closeRequested, &app};

  const bool selected = iggy3d::applyProductAutomationAppCommand(
      {"frontend.select", "creative_new_world"}, context);
  const iggy3d::FrontendAction selectedActionAfterSelect =
      frontend.selectedAction;
  const bool executed = iggy3d::applyProductAutomationAppCommand(
      {"frontend.execute", "true"}, context);

  const iggy3d::ProductSaveBridgeResult creativeSaves =
      iggy3d::scanProductSaves(options.saveRoot,
                               "iggy3d.creative",
                               "creative.document");
  const bool hasCreativeEntry =
      creativeSaves.catalog.catalog.entries.size() == 1U;
  const iggy3d::ProductSaveCatalogEntry creativeEntry =
      hasCreativeEntry ? creativeSaves.catalog.catalog.entries.front()
                       : iggy3d::ProductSaveCatalogEntry{};
  const iggy3d::ProductContinueSelectionResult productContinue =
      iggy3d::selectProductContinueSave(creativeSaves.catalog.catalog);

  return expect(selected, "automation selected creative new world") &&
         expect(selectedActionAfterSelect ==
                    iggy3d::FrontendAction::CreativeNewWorld,
                "automation selected creative action") &&
         expect(executed, "automation executed creative new world") &&
         expect(activeSession.has_value(), "automation created runtime session") &&
         expect(frontend.screen == iggy3d::FrontendScreen::Gameplay,
                "automation entered gameplay") &&
         expect(window.interactionMode ==
                    iggy3d::ProductInteractionMode::Creative,
                "automation entered creative mode") &&
         expect(window.launchStatus == "product_creative_world_launched",
                "automation creative launch status") &&
         expect(window.activeCreativeSaveId != "none" &&
                    !window.activeCreativeSaveId.empty(),
                "automation records creative save id") &&
         expect(window.activeProductSaveId == "none",
                "automation does not set product save id") &&
         expect(facade.document().id() !=
                    iggy3d::creative::kInvalidDocumentId,
                "automation installed creative document id") &&
         expect(facade.document().name() == "Automation Creative",
                "automation installed creative document title") &&
         expect(facade.document().objectCount() == 0U,
                "automation creates empty creative document") &&
         expect(hasCreativeEntry, "automation writes one creative save") &&
         expect(creativeEntry.contentKind ==
                    iggy3d::ProductSaveContentKind::CreativeDocument,
                "automation save is creative document") &&
         expect(iggy3d::canOpenCreativeWorld(creativeEntry),
                "automation save can open as creative") &&
         expect(!iggy3d::canLoadProductSave(creativeEntry),
                "automation save is not product loadable") &&
         expect(!productContinue.selected,
                "automation creative save is ignored by product continue") &&
         expect(window.launchStatus != "product_creative_world_facade_missing",
                "automation facade was threaded into starter action");
}

}  // namespace

int main() {
  bool ok = true;
  ok &= creativeNewWorldLaunchesThroughAutomationAppContext();
  return ok ? 0 : 1;
}
