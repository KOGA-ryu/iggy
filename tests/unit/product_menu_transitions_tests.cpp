#include "app/iggy3d/menu/Transitions.hpp"

#include <iostream>

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    return false;
  }
  return true;
}

void activateMapMaker(iggy3d::ProductAppWindowState& window) {
  window.mapMakerActive = true;
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  window.viewport.creativeFlyActive = true;
  window.viewport.creativeFlyStatus = "creative_fly_applied";
  window.viewport.creativeFlyReasonCode = window.viewport.creativeFlyStatus;
  window.viewport.creativeFlySpeedMetersPerSecond = 8.0F;
  window.mapMakerStatus = "map_maker_active";
  window.mapMakerReasonCode = window.mapMakerStatus;
}

}  // namespace

int main() {
  bool ok = true;

  iggy3d::FrontendState frontend;
  iggy3d::ProductAppWindowState window;
  initializeProductStarterTransition(frontend, window, false);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::Starter,
               "starter screen initialized");
  ok &= expect(frontend.selectedAction == iggy3d::FrontendAction::NewWorld,
               "starter selects new world without compatible save");
  ok &= expect(window.inputOwner == iggy3d::MenuOwner::Starter,
               "starter owns input");
  ok &= expect(window.gameplayInputSuppressed, "starter suppresses gameplay input");
  ok &= expect(window.productTransitionStatus == "starter_ready",
               "starter transition status");

  window.gameplayActive = true;
  window.runtimeSessionCreated = true;
  enterProductGameplayTransition(frontend, window,
                                 iggy3d::FrontendAction::CreateAndEnter);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::Gameplay,
               "gameplay screen after launch");
  ok &= expect(!frontend.inputOwned, "gameplay owns no frontend input");
  ok &= expect(window.inputOwner == iggy3d::MenuOwner::Gameplay,
               "gameplay input owner");
  ok &= expect(!window.gameplayInputSuppressed, "gameplay input accepted");
  ok &= expect(window.productTransitionReturnedToGameplay,
               "gameplay transition returned to gameplay");
  ok &= expect(window.productTransitionSessionPreserved,
               "gameplay launch preserves session");

  activateMapMaker(window);
  openProductPauseTransition(frontend, window, iggy3d::FrontendAction::Resume);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::Pause,
               "pause screen opened");
  ok &= expect(frontend.pauseMenuOpen, "pause menu open");
  ok &= expect(window.inputOwner == iggy3d::MenuOwner::Pause,
               "pause owns input");
  ok &= expect(window.gameplayInputSuppressed, "pause suppresses gameplay input");
  ok &= expect(window.productTransitionSessionPreserved,
               "pause keeps session active");
  ok &= expect(!window.mapMakerActive, "pause clears map maker active");
  ok &= expect(window.interactionMode == iggy3d::ProductInteractionMode::Player,
               "pause returns interaction mode to player");
  ok &= expect(!window.viewport.creativeFlyActive,
               "pause clears creative fly active");

  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  openProductPauseSettingsTransition(frontend, window, settingsTab);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::Settings,
               "settings opened from pause");
  ok &= expect(frontend.childScreen == iggy3d::FrontendScreen::Pause,
               "settings parent is pause");
  ok &= expect(settingsTab == iggy3d::FrontendSettingsTab::Input,
               "settings opens input tab");
  ok &= expect(window.inputOwner == iggy3d::MenuOwner::Settings,
               "settings owns input");
  ok &= expect(!window.mapMakerActive, "settings keeps map maker cleared");
  ok &= expect(window.interactionMode == iggy3d::ProductInteractionMode::Player,
               "settings keeps player interaction mode");

  openProductPauseTransition(frontend, window, iggy3d::FrontendAction::Settings);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::Pause,
               "settings back returns to pause");
  ok &= expect(frontend.selectedAction == iggy3d::FrontendAction::Settings,
               "pause remembers settings row");
  ok &= expect(!window.mapMakerActive, "settings back keeps map maker cleared");

  openProductPauseDevToolsTransition(frontend, window,
                                     iggy3d::FrontendDevToolsCategory::Session);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::DevOverlay,
               "dev overlay opened");
  ok &= expect(frontend.devToolsOpen, "dev tools open");
  ok &= expect(window.inputOwner == iggy3d::MenuOwner::DevTools,
               "dev tools own input");
  ok &= expect(window.gameplayInputSuppressed, "dev tools suppress gameplay input");
  ok &= expect(!window.mapMakerActive, "dev tools keeps map maker cleared");

  closeProductOverlayToGameplayTransition(frontend, window);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::Gameplay,
               "resume returns to gameplay");
  ok &= expect(!frontend.inputOwned, "resume releases frontend input");
  ok &= expect(window.inputOwner == iggy3d::MenuOwner::Gameplay,
               "resume restores gameplay owner");
  ok &= expect(window.productTransitionReturnedToGameplay,
               "resume transition status");
  ok &= expect(!window.mapMakerActive, "resume does not restore map maker");
  ok &= expect(window.interactionMode == iggy3d::ProductInteractionMode::Player,
               "resume stays player mode");

  openProductPauseTransition(frontend, window, iggy3d::FrontendAction::ReturnToTitle);
  activateMapMaker(window);
  returnProductToTitleTransition(frontend, window);
  ok &= expect(frontend.screen == iggy3d::FrontendScreen::Starter,
               "return to title opens starter");
  ok &= expect(frontend.returnToTitleRequested, "return to title requested");
  ok &= expect(!window.gameplayActive, "return to title clears gameplay active");
  ok &= expect(!window.runtimeSessionCreated, "return to title clears session flag");
  ok &= expect(window.inputOwner == iggy3d::MenuOwner::Starter,
               "return to title restores starter owner");
  ok &= expect(window.interactionMode == iggy3d::ProductInteractionMode::Player,
               "return to title resets interaction mode");
  ok &= expect(!window.mapMakerActive, "return to title clears map maker active");
  ok &= expect(!window.viewport.creativeFlyActive,
               "return to title clears creative fly active");
  ok &= expect(window.productTransitionReturnedToTitle,
               "return to title transition status");
  ok &= expect(!window.productTransitionSessionPreserved,
               "return to title does not preserve session");

  if (!ok) {
    return 1;
  }
  std::cout << "product_menu_transitions_tests=pass\n";
  return 0;
}
