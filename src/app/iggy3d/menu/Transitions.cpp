#include "app/iggy3d/menu/Transitions.hpp"

#include <string>
#include <utility>

namespace iggy3d {
namespace {

void setTransition(ProductAppWindowState& window,
                   std::string action,
                   std::string status,
                   bool returnedToGameplay,
                   bool returnedToTitle,
                   bool sessionPreserved) {
  window.productTransitionLastAction = std::move(action);
  window.productTransitionStatus = std::move(status);
  window.productTransitionReturnedToGameplay = returnedToGameplay;
  window.productTransitionReturnedToTitle = returnedToTitle;
  window.productTransitionSessionPreserved = sessionPreserved;
}

}  // namespace

void clearProductMapMakerMode(ProductAppWindowState& window) {
  window.mapMakerActive = false;
  window.interactionMode = ProductInteractionMode::Player;
  window.viewport.creativeFlyActive = false;
  window.viewport.creativeFlyStatus = "creative_fly_not_requested";
  window.viewport.creativeFlyReasonCode = window.viewport.creativeFlyStatus;
  window.viewport.creativeFlySpeedMetersPerSecond = 0.0F;
  window.mapMakerStatus = "map_maker_inactive";
  window.mapMakerReasonCode = window.mapMakerStatus;
}

void initializeProductStarterTransition(FrontendState& frontend,
                                        ProductAppWindowState& window,
                                        bool hasCompatibleSave) {
  completeFrontendBoot(frontend, true, true);
  frontend.screen = FrontendScreen::Starter;
  frontend.childScreen = FrontendScreen::Gameplay;
  frontend.selectedAction =
      hasCompatibleSave ? FrontendAction::Continue : FrontendAction::NewWorld;
  frontend.status = "opening_menu_ready";
  frontend.inputOwned = true;
  window.inputOwner = MenuOwner::Starter;
  window.gameplayInputSuppressed = true;
  setTransition(window, "startup", "starter_ready", false, false, false);
}

void enterProductGameplayTransition(FrontendState& frontend,
                                    ProductAppWindowState& window,
                                    FrontendAction launchAction) {
  enterFrontendGameplay(frontend, launchAction);
  frontend.status = "gameplay_active";
  window.inputOwner = MenuOwner::Gameplay;
  window.gameplayInputSuppressed = false;
  setTransition(window, "launch_gameplay", "gameplay_active", true, false, true);
}

void openProductPauseTransition(FrontendState& frontend,
                                ProductAppWindowState& window,
                                FrontendAction selectedAction) {
  clearProductMapMakerMode(window);
  openFrontendPause(frontend, selectedAction);
  window.inputOwner = MenuOwner::Pause;
  window.gameplayInputSuppressed = true;
  setTransition(window, "open_pause", "pause_ready", false, false,
                window.gameplayActive);
}

void openProductPauseSettingsTransition(FrontendState& frontend,
                                        ProductAppWindowState& window,
                                        FrontendSettingsTab& settingsTab) {
  clearProductMapMakerMode(window);
  frontend.screen = FrontendScreen::Settings;
  frontend.childScreen = FrontendScreen::Pause;
  settingsTab = FrontendSettingsTab::Input;
  frontend.inputOwned = true;
  frontend.status = "pause_settings_selected";
  window.inputOwner = MenuOwner::Settings;
  window.gameplayInputSuppressed = true;
  setTransition(window, "open_settings", "settings_from_pause", false, false,
                window.gameplayActive);
}

void openProductPauseDevToolsTransition(FrontendState& frontend,
                                        ProductAppWindowState& window,
                                        FrontendDevToolsCategory category) {
  clearProductMapMakerMode(window);
  openFrontendDevOverlay(frontend, category);
  frontend.status = "pause_dev_tools_selected";
  window.inputOwner = MenuOwner::DevTools;
  window.gameplayInputSuppressed = true;
  setTransition(window, "open_dev_tools", "dev_overlay_from_pause", false, false,
                window.gameplayActive);
}

void closeProductOverlayToGameplayTransition(FrontendState& frontend,
                                             ProductAppWindowState& window) {
  closeFrontendOverlayToGameplay(frontend);
  window.inputOwner = MenuOwner::Gameplay;
  window.gameplayInputSuppressed = false;
  setTransition(window, "resume_gameplay", "gameplay_resumed", true, false,
                window.gameplayActive);
}

void returnProductToTitleTransition(FrontendState& frontend,
                                    ProductAppWindowState& window) {
  window.gameplayActive = false;
  window.runtimeSessionCreated = false;
  clearProductMapMakerMode(window);
  frontend.screen = FrontendScreen::Starter;
  frontend.childScreen = FrontendScreen::Gameplay;
  frontend.selectedAction = FrontendAction::NewWorld;
  frontend.returnToTitleRequested = true;
  frontend.inputOwned = true;
  frontend.pauseMenuOpen = false;
  frontend.devToolsOpen = false;
  frontend.status = "returned_to_title";
  window.inputOwner = MenuOwner::Starter;
  window.gameplayInputSuppressed = true;
  setTransition(window, "return_to_title", "returned_to_title", false, true, false);
}

}  // namespace iggy3d
