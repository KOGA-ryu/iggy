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
       expect(state.pauseMenuOpen, "pause open") &&
       expect(!state.devToolsOpen, "dev closed in pause") &&
       expect(iggy3d::frontendBlocksGameplayInput(state), "pause blocks gameplay") && ok;
  iggy3d::openFrontendDevOverlay(state, iggy3d::FrontendDevToolsCategory::Session);
  ok = expect(state.screen == iggy3d::FrontendScreen::DevOverlay, "dev overlay screen") &&
       expect(state.devToolsOpen, "dev overlay open") &&
       expect(!state.pauseMenuOpen, "pause closed for dev overlay") &&
       expect(iggy3d::frontendBlocksGameplayInput(state), "dev overlay blocks gameplay") && ok;
  iggy3d::closeFrontendOverlayToGameplay(state);
  ok = expect(state.screen == iggy3d::FrontendScreen::Gameplay, "closed to gameplay") &&
       expect(!iggy3d::frontendBlocksGameplayInput(state), "closed overlay releases input") && ok;
  return ok;
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
         expect(iggy3d::frontendDevToolsCategoryName(
                    iggy3d::FrontendDevToolsCategory::WorldEditor) == "world_editor",
                "world editor category name");
}

}  // namespace

int main() {
  const bool ok = bootTransitionsOnlyWhenReady() && overlaysAreSeparateStates() &&
                  stableNamesArePacketNames();
  return ok ? 0 : 1;
}
