#include "app/frontend/FrontendState.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool bootTransitionsOnlyWhenReady() {
  iggy3d::FrontendState state;
  iggy3d::completeFrontendBoot(state, true, false);
  bool ok = expect(state.screen == iggy3d::FrontendScreen::BootStatus,
                   "partial boot stays on boot") &&
            expect(state.status == "boot_failed", "partial boot status");
  iggy3d::completeFrontendBoot(state, true, true);
  ok = expect(state.screen == iggy3d::FrontendScreen::Starter,
              "complete boot enters starter") &&
       expect(state.selectedAction == iggy3d::FrontendAction::Continue,
              "starter selects continue") &&
       expect(iggy3d::frontendBlocksGameplayInput(state), "starter blocks gameplay") && ok;
  return ok;
}

bool overlaysAreSeparateStates() {
  iggy3d::FrontendState state;
  iggy3d::completeFrontendBoot(state, true, true);
  iggy3d::enterFrontendGameplay(state, iggy3d::FrontendAction::CreateAndEnter);
  bool ok = expect(state.screen == iggy3d::FrontendScreen::Gameplay, "gameplay entered") &&
            expect(!iggy3d::frontendBlocksGameplayInput(state), "gameplay does not block");
  iggy3d::openFrontendPause(state, iggy3d::FrontendAction::SaveAndExit);
  ok = expect(state.screen == iggy3d::FrontendScreen::Pause, "pause is separate") &&
       expect(iggy3d::frontendPauseMenuOpen(state), "pause open") &&
       expect(!iggy3d::frontendDevToolsOpen(state), "dev closed in pause") &&
       expect(iggy3d::frontendBlocksGameplayInput(state), "pause blocks gameplay") && ok;
  iggy3d::openFrontendDevOverlay(state, iggy3d::FrontendDevToolsCategory::Session);
  ok = expect(state.screen == iggy3d::FrontendScreen::DevOverlay, "dev overlay screen") &&
       expect(iggy3d::frontendDevToolsOpen(state), "dev overlay open") &&
       expect(!iggy3d::frontendPauseMenuOpen(state), "pause closed for dev overlay") &&
       expect(iggy3d::frontendBlocksGameplayInput(state), "dev overlay blocks gameplay") && ok;
  iggy3d::closeFrontendOverlayToGameplay(state);
  ok = expect(state.screen == iggy3d::FrontendScreen::Gameplay, "closed to gameplay") &&
       expect(!iggy3d::frontendBlocksGameplayInput(state), "closed overlay releases input") && ok;
  return ok;
}

bool pauseSettingsAndDevToolsBlockGameplay() {
  iggy3d::FrontendState state;
  iggy3d::completeFrontendBoot(state, true, true);
  iggy3d::enterFrontendGameplay(state, iggy3d::FrontendAction::CreateAndEnter);
  iggy3d::openFrontendPause(state, iggy3d::FrontendAction::Settings);
  bool ok = expect(state.screen == iggy3d::FrontendScreen::Pause,
                   "pause before settings") &&
            expect(iggy3d::frontendBlocksGameplayInput(state),
                   "pause owns input");
  state.screen = iggy3d::FrontendScreen::Settings;
  state.childScreen = iggy3d::FrontendScreen::Settings;
  state.selectedAction = iggy3d::FrontendAction::Settings;
  state.inputOwned = true;
  state.status = "settings_menu_ready";
  ok = expect(state.screen == iggy3d::FrontendScreen::Settings,
              "settings screen open") &&
       expect(iggy3d::frontendBlocksGameplayInput(state),
              "settings owns input") && ok;
  iggy3d::openFrontendPause(state, iggy3d::FrontendAction::Resume);
  ok = expect(state.screen == iggy3d::FrontendScreen::Pause,
              "settings back returns pause") &&
       expect(iggy3d::frontendBlocksGameplayInput(state),
              "pause still blocks after settings") && ok;
  iggy3d::openFrontendDevOverlay(state, iggy3d::FrontendDevToolsCategory::Renderer);
  ok = expect(state.screen == iggy3d::FrontendScreen::DevOverlay,
              "dev tools from pause") &&
       expect(iggy3d::frontendDevToolsOpen(state), "dev overlay open") &&
       expect(iggy3d::frontendBlocksGameplayInput(state),
              "dev tools block gameplay") && ok;
  iggy3d::closeFrontendOverlayToGameplay(state);
  ok = expect(!iggy3d::frontendBlocksGameplayInput(state),
              "resume releases input") && ok;
  return ok;
}

bool menuAndConfirmScreensBlockGameplay() {
  struct BlockingCase {
    iggy3d::FrontendScreen screen;
    iggy3d::FrontendScreen childScreen;
    const char* name;
  };
  constexpr BlockingCase cases[] = {
      {iggy3d::FrontendScreen::BootStatus,
       iggy3d::FrontendScreen::Gameplay,
       "boot"},
      {iggy3d::FrontendScreen::Starter,
       iggy3d::FrontendScreen::Gameplay,
       "starter"},
      {iggy3d::FrontendScreen::NewWorld,
       iggy3d::FrontendScreen::Gameplay,
       "new world"},
      {iggy3d::FrontendScreen::LoadSave,
       iggy3d::FrontendScreen::Gameplay,
       "load save"},
      {iggy3d::FrontendScreen::Settings,
       iggy3d::FrontendScreen::Pause,
       "pause settings"},
      {iggy3d::FrontendScreen::StarterDevTools,
       iggy3d::FrontendScreen::Gameplay,
       "starter dev tools"},
      {iggy3d::FrontendScreen::Pause,
       iggy3d::FrontendScreen::Gameplay,
       "pause"},
      {iggy3d::FrontendScreen::DevOverlay,
       iggy3d::FrontendScreen::Gameplay,
       "dev overlay"},
      {iggy3d::FrontendScreen::ExitConfirm,
       iggy3d::FrontendScreen::Gameplay,
       "exit confirm"},
      {iggy3d::FrontendScreen::DeleteConfirm,
       iggy3d::FrontendScreen::Gameplay,
       "delete confirm"},
  };

  bool ok = true;
  for (const BlockingCase& testCase : cases) {
    iggy3d::FrontendState state;
    state.screen = testCase.screen;
    state.childScreen = testCase.childScreen;
    state.inputOwned = false;
    ok = expect(iggy3d::frontendBlocksGameplayInput(state), testCase.name) &&
         ok;
  }

  iggy3d::FrontendState gameplay;
  iggy3d::enterFrontendGameplay(gameplay, iggy3d::FrontendAction::CreateAndEnter);
  ok = expect(!iggy3d::frontendBlocksGameplayInput(gameplay),
              "active gameplay does not block") &&
       ok;
  gameplay.inputOwned = true;
  ok = expect(iggy3d::frontendBlocksGameplayInput(gameplay),
              "input-owned gameplay blocks") &&
       ok;
  return ok;
}

bool openPredicatesDeriveFromScreenState() {
  iggy3d::FrontendState starter;
  starter.screen = iggy3d::FrontendScreen::Starter;
  starter.childScreen = iggy3d::FrontendScreen::Gameplay;
  bool ok = expect(!iggy3d::frontendPauseMenuOpen(starter),
                   "starter pause closed") &&
            expect(!iggy3d::frontendDevToolsOpen(starter),
                   "starter dev closed");

  iggy3d::FrontendState pause;
  pause.screen = iggy3d::FrontendScreen::Pause;
  pause.childScreen = iggy3d::FrontendScreen::Gameplay;
  ok = expect(iggy3d::frontendPauseMenuOpen(pause),
              "pause derives open from screen") &&
       expect(!iggy3d::frontendDevToolsOpen(pause),
              "pause dev tools closed") && ok;

  iggy3d::FrontendState devTools;
  devTools.screen = iggy3d::FrontendScreen::DevOverlay;
  devTools.childScreen = iggy3d::FrontendScreen::Gameplay;
  ok = expect(!iggy3d::frontendPauseMenuOpen(devTools),
              "dev tools pause closed") &&
       expect(iggy3d::frontendDevToolsOpen(devTools),
              "dev tools derives open from screen") && ok;

  iggy3d::FrontendState starterDevTools;
  starterDevTools.screen = iggy3d::FrontendScreen::Starter;
  starterDevTools.childScreen = iggy3d::FrontendScreen::StarterDevTools;
  ok = expect(iggy3d::frontendDevToolsOpen(starterDevTools),
              "starter child dev tools derives open from child screen") && ok;
  return ok;
}

bool returnToTitleSuppressesGameplay() {
  iggy3d::FrontendState state;
  iggy3d::completeFrontendBoot(state, true, true);
  iggy3d::enterFrontendGameplay(state, iggy3d::FrontendAction::CreateAndEnter);
  state.screen = iggy3d::FrontendScreen::Starter;
  state.selectedAction = iggy3d::FrontendAction::Continue;
  state.returnToTitleRequested = true;
  state.inputOwned = true;
  state.status = "frontend_return_to_title";
  return expect(state.returnToTitleRequested, "return title requested") &&
         expect(state.screen == iggy3d::FrontendScreen::Starter,
                "return title screen") &&
         expect(iggy3d::frontendBlocksGameplayInput(state),
                "starter suppresses gameplay");
}

bool stableNamesArePacketNames() {
  return expect(iggy3d::frontendScreenName(iggy3d::FrontendScreen::Starter) ==
                    "starter",
                "starter name") &&
         expect(iggy3d::frontendScreenName(iggy3d::FrontendScreen::DevOverlay) ==
                    "dev_overlay",
                "dev overlay name") &&
         expect(iggy3d::frontendActionName(iggy3d::FrontendAction::SaveAndExit) ==
                    "save_and_exit",
                "save and exit name") &&
         expect(iggy3d::frontendActionName(iggy3d::FrontendAction::EditRoom) ==
                    "edit_room",
                "edit room name") &&
         expect(iggy3d::frontendActionName(iggy3d::FrontendAction::LeaveEditor) ==
                    "leave_editor",
                "leave editor name") &&
         expect(iggy3d::frontendDevToolsCategoryName(
                    iggy3d::FrontendDevToolsCategory::WorldEditor) == "world_editor",
                "world editor category name");
}

}  // namespace

int main() {
  const bool ok = bootTransitionsOnlyWhenReady() && overlaysAreSeparateStates() &&
                  pauseSettingsAndDevToolsBlockGameplay() &&
                  menuAndConfirmScreensBlockGameplay() &&
                  openPredicatesDeriveFromScreenState() &&
                  returnToTitleSuppressesGameplay() &&
                  stableNamesArePacketNames();
  return ok ? 0 : 1;
}
