#include "app/frontend/DevToolsMenu.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/frontend/StarterScreen.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool starterActionOrderIsExact() {
  const std::vector<iggy3d::FrontendAction>& actions = iggy3d::starterActionOrder();
  return expect(actions.size() == 6U, "starter action count") &&
         expect(actions[0] == iggy3d::FrontendAction::Continue, "continue first") &&
         expect(actions[1] == iggy3d::FrontendAction::NewWorld, "new world second") &&
         expect(actions[2] == iggy3d::FrontendAction::LoadSave, "load save third") &&
         expect(actions[3] == iggy3d::FrontendAction::Settings, "settings fourth") &&
         expect(actions[4] == iggy3d::FrontendAction::DevTools, "dev tools fifth") &&
         expect(actions[5] == iggy3d::FrontendAction::Exit, "exit sixth");
}

bool pauseActionOrderIsExact() {
  const std::vector<iggy3d::FrontendAction>& actions = iggy3d::pauseActionOrder();
  return expect(actions.size() == 8U, "pause action count") &&
         expect(actions[0] == iggy3d::FrontendAction::Resume, "resume first") &&
         expect(actions[1] == iggy3d::FrontendAction::Save, "save second") &&
         expect(actions[2] == iggy3d::FrontendAction::SaveAndExit,
                "save and exit third") &&
         expect(actions[3] == iggy3d::FrontendAction::LoadSave, "load save fourth") &&
         expect(actions[4] == iggy3d::FrontendAction::Settings, "settings fifth") &&
         expect(actions[5] == iggy3d::FrontendAction::DevTools, "dev tools sixth") &&
         expect(actions[6] == iggy3d::FrontendAction::ReturnToTitle,
                "return to title seventh") &&
         expect(actions[7] == iggy3d::FrontendAction::ExitGame, "exit game eighth");
}

bool continueDisablesWithoutSave() {
  const iggy3d::StarterScreenModel empty =
      iggy3d::buildStarterScreenModel(0U, iggy3d::FrontendAction::Continue);
  const iggy3d::StarterScreenModel withSave =
      iggy3d::buildStarterScreenModel(1U, iggy3d::FrontendAction::Continue);
  return expect(!empty.continueEnabled, "empty continue disabled") &&
         expect(empty.disabled == iggy3d::FrontendAction::Continue,
                "disabled action continue") &&
         expect(withSave.continueEnabled, "continue enabled with save") &&
         expect(withSave.disabled == iggy3d::FrontendAction::None,
                "no disabled action with save");
}

bool devToolsTabOrderIsExact() {
  const std::vector<iggy3d::FrontendDevToolsCategory>& tabs =
      iggy3d::devToolsCategoryOrder();
  return expect(tabs.size() == 9U, "dev tabs count") &&
         expect(tabs[0] == iggy3d::FrontendDevToolsCategory::Session, "session first") &&
         expect(tabs[1] == iggy3d::FrontendDevToolsCategory::Player, "player second") &&
         expect(tabs[2] == iggy3d::FrontendDevToolsCategory::Movement, "movement third") &&
         expect(tabs[3] == iggy3d::FrontendDevToolsCategory::WorldEditor,
                "world editor fourth") &&
         expect(tabs[4] == iggy3d::FrontendDevToolsCategory::Collision,
                "collision fifth") &&
         expect(tabs[5] == iggy3d::FrontendDevToolsCategory::Spells, "spells sixth") &&
         expect(tabs[6] == iggy3d::FrontendDevToolsCategory::Camera, "camera seventh") &&
         expect(tabs[7] == iggy3d::FrontendDevToolsCategory::Renderer,
                "renderer eighth") &&
         expect(tabs[8] == iggy3d::FrontendDevToolsCategory::Performance,
                "performance ninth");
}

bool settingsDraftIsFrontendOnly() {
  iggy3d::FrontendSettings settings = iggy3d::defaultFrontendSettings();
  settings.inputBackend = iggy3d::FrontendInputBackend::Gamepad;
  settings.lookSensitivity = 1.75F;
  settings.invertLook = true;
  iggy3d::restoreFrontendSettingsDefaults(settings);
  return expect(settings.inputBackend == iggy3d::FrontendInputBackend::Keyboard,
                "settings restore input") &&
         expect(settings.lookSensitivity == 1.0F, "settings restore sensitivity") &&
         expect(!settings.invertLook, "settings restore invert");
}

}  // namespace

int main() {
  const bool ok = starterActionOrderIsExact() && pauseActionOrderIsExact() &&
                  continueDisablesWithoutSave() && devToolsTabOrderIsExact() &&
                  settingsDraftIsFrontendOnly();
  return ok ? 0 : 1;
}
