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
  return expect(actions.size() == 7U, "starter action count") &&
         expect(actions[0] == iggy3d::FrontendAction::Continue, "continue first") &&
         expect(actions[1] == iggy3d::FrontendAction::NewWorld, "new world second") &&
         expect(actions[2] == iggy3d::FrontendAction::LoadSave, "load save third") &&
         expect(actions[3] == iggy3d::FrontendAction::Delete, "delete fourth") &&
         expect(actions[4] == iggy3d::FrontendAction::Settings, "settings fifth") &&
         expect(actions[5] == iggy3d::FrontendAction::DevTools, "dev tools sixth") &&
         expect(actions[6] == iggy3d::FrontendAction::Exit, "exit seventh");
}

bool pauseActionOrderIsExact() {
  const std::vector<iggy3d::FrontendAction>& actions = iggy3d::pauseActionOrder();
  return expect(actions.size() == 9U, "pause action count") &&
         expect(actions[0] == iggy3d::FrontendAction::Resume, "resume first") &&
         expect(actions[1] == iggy3d::FrontendAction::EditRoom,
                "edit room second") &&
         expect(actions[2] == iggy3d::FrontendAction::Save, "save third") &&
         expect(actions[3] == iggy3d::FrontendAction::SaveAndExit,
                "save and exit fourth") &&
         expect(actions[4] == iggy3d::FrontendAction::LoadSave, "load save fifth") &&
         expect(actions[5] == iggy3d::FrontendAction::Settings, "settings sixth") &&
         expect(actions[6] == iggy3d::FrontendAction::DevTools, "dev tools seventh") &&
         expect(actions[7] == iggy3d::FrontendAction::ReturnToTitle,
                "return to title eighth") &&
         expect(actions[8] == iggy3d::FrontendAction::ExitGame, "exit game ninth");
}

bool continueDisablesWithoutSave() {
  const iggy3d::StarterScreenModel empty =
      iggy3d::buildStarterScreenModel(0U, iggy3d::FrontendAction::Continue);
  const iggy3d::StarterScreenModel withSave =
      iggy3d::buildStarterScreenModel(1U, iggy3d::FrontendAction::Continue);
  return expect(!empty.continueEnabled, "empty continue disabled") &&
         expect(empty.disabled == iggy3d::FrontendAction::Continue,
                "disabled action continue") &&
         expect(!empty.selectedEnabled, "selected continue disabled") &&
         expect(empty.selectedDisabledReason == "no_compatible_save",
                "continue disabled reason") &&
         expect(empty.selectedCommand == "starter_continue",
                "continue command") &&
         expect(withSave.continueEnabled, "continue enabled with save") &&
         expect(withSave.disabled == iggy3d::FrontendAction::None,
                "no disabled action with save") &&
         expect(withSave.selectedEnabled, "continue selected enabled");
}

bool disabledSaveBackedActionsReturnIgnoredRoutes() {
  const iggy3d::StarterScreenModel model =
      iggy3d::buildStarterScreenModel(0U, iggy3d::FrontendAction::Continue);
  const auto continueRoute =
      iggy3d::routeStarterAction(model, iggy3d::FrontendAction::Continue);
  const auto loadRoute =
      iggy3d::routeStarterAction(model, iggy3d::FrontendAction::LoadSave);
  const auto deleteRoute =
      iggy3d::routeStarterAction(model, iggy3d::FrontendAction::Delete);
  return expect(!continueRoute.accepted, "disabled continue ignored") &&
         expect(continueRoute.inputOwner == iggy3d::MenuOwner::Starter,
                "disabled continue owner") &&
         expect(continueRoute.nextScreen == iggy3d::FrontendScreen::Starter,
                "disabled continue screen") &&
         expect(continueRoute.nextChildScreen == iggy3d::FrontendScreen::Gameplay,
                "disabled continue child") &&
         expect(continueRoute.requestedTransition ==
                    iggy3d::FrontendTransitionRequest::None,
                "disabled continue transition") &&
         expect(!continueRoute.closeRequested, "disabled continue close") &&
         expect(continueRoute.gameplayInputSuppressed,
                "disabled continue suppresses gameplay") &&
         expect(continueRoute.status == "no_compatible_save",
                "disabled continue status") &&
         expect(continueRoute.receiptReason == "no_compatible_save",
                "disabled continue reason") &&
         expect(continueRoute.selectedAction == iggy3d::FrontendAction::Continue,
                "disabled continue action") &&
         expect(!loadRoute.accepted, "disabled load ignored") &&
         expect(loadRoute.status == "no_compatible_save", "disabled load status") &&
         expect(loadRoute.selectedAction == iggy3d::FrontendAction::LoadSave,
                "disabled load action") &&
         expect(!deleteRoute.accepted, "disabled delete ignored") &&
         expect(deleteRoute.status == "no_compatible_save",
                "disabled delete status") &&
         expect(deleteRoute.selectedAction == iggy3d::FrontendAction::Delete,
                "disabled delete action");
}

bool enabledStarterActionsReturnRouteResults() {
  const iggy3d::StarterScreenModel model =
      iggy3d::buildStarterScreenModel(2U, iggy3d::FrontendAction::Continue);
  const auto continueRoute =
      iggy3d::routeStarterAction(model, iggy3d::FrontendAction::Continue);
  const auto newWorldRoute =
      iggy3d::routeStarterAction(model, iggy3d::FrontendAction::NewWorld);
  const auto loadRoute =
      iggy3d::routeStarterAction(model, iggy3d::FrontendAction::LoadSave);
  const auto deleteRoute =
      iggy3d::routeStarterAction(model, iggy3d::FrontendAction::Delete);
  const auto settingsRoute =
      iggy3d::routeStarterAction(model, iggy3d::FrontendAction::Settings);
  const auto devToolsRoute =
      iggy3d::routeStarterAction(model, iggy3d::FrontendAction::DevTools);
  const auto exitRoute =
      iggy3d::routeStarterAction(model, iggy3d::FrontendAction::Exit);

  return expect(continueRoute.accepted, "continue accepted") &&
         expect(continueRoute.inputOwner == iggy3d::MenuOwner::Gameplay,
                "continue gameplay owner") &&
         expect(continueRoute.nextScreen == iggy3d::FrontendScreen::Gameplay,
                "continue next gameplay") &&
         expect(continueRoute.nextChildScreen == iggy3d::FrontendScreen::Gameplay,
                "continue child gameplay") &&
         expect(continueRoute.requestedTransition ==
                    iggy3d::FrontendTransitionRequest::LaunchGameplay,
                "continue launch transition") &&
         expect(!continueRoute.gameplayInputSuppressed,
                "continue releases gameplay input") &&
         expect(newWorldRoute.accepted, "new world accepted") &&
         expect(newWorldRoute.nextScreen == iggy3d::FrontendScreen::Starter,
                "new world stays starter") &&
         expect(newWorldRoute.nextChildScreen == iggy3d::FrontendScreen::NewWorld,
                "new world child") &&
         expect(newWorldRoute.requestedTransition ==
                    iggy3d::FrontendTransitionRequest::None,
                "new world no transition") &&
         expect(loadRoute.nextChildScreen == iggy3d::FrontendScreen::LoadSave,
                "load child") &&
         expect(deleteRoute.nextChildScreen == iggy3d::FrontendScreen::DeleteConfirm,
                "delete confirm child") &&
         expect(settingsRoute.nextChildScreen == iggy3d::FrontendScreen::Settings,
                "settings child") &&
         expect(devToolsRoute.nextChildScreen == iggy3d::FrontendScreen::StarterDevTools,
                "dev tools child") &&
         expect(exitRoute.accepted, "exit accepted") &&
         expect(exitRoute.inputOwner == iggy3d::MenuOwner::Starter,
                "exit owner starter") &&
         expect(exitRoute.nextChildScreen == iggy3d::FrontendScreen::ExitConfirm,
                "exit confirm child") &&
         expect(exitRoute.requestedTransition == iggy3d::FrontendTransitionRequest::Exit,
                "exit transition") &&
         expect(exitRoute.closeRequested, "exit close requested") &&
         expect(exitRoute.gameplayInputSuppressed, "exit suppresses gameplay") &&
         expect(exitRoute.status == "starter_exit_requested",
                "exit status");
}

bool starterChildBackReturnsToStarter() {
  const std::vector<iggy3d::FrontendScreen> children = {
      iggy3d::FrontendScreen::NewWorld,
      iggy3d::FrontendScreen::LoadSave,
      iggy3d::FrontendScreen::DeleteConfirm,
      iggy3d::FrontendScreen::Settings,
      iggy3d::FrontendScreen::StarterDevTools,
      iggy3d::FrontendScreen::ExitConfirm,
  };
  bool ok = true;
  for (const iggy3d::FrontendScreen child : children) {
    const auto route = iggy3d::routeStarterBackFromChild(child);
    ok = expect(route.accepted, "child back accepted") &&
         expect(route.inputOwner == iggy3d::MenuOwner::Starter,
                "child back owner") &&
         expect(route.nextScreen == iggy3d::FrontendScreen::Starter,
                "child back screen") &&
         expect(route.nextChildScreen == iggy3d::FrontendScreen::Gameplay,
                "child back closes child") &&
         expect(route.requestedTransition == iggy3d::FrontendTransitionRequest::None,
                "child back no transition") &&
         expect(!route.closeRequested, "child back no close") &&
         expect(route.gameplayInputSuppressed, "child back suppresses gameplay") &&
         expect(route.status == "starter_child_returned",
                "child back status") &&
         expect(route.selectedAction == iggy3d::FrontendAction::Back,
                "child back action") && ok;
  }
  return ok;
}

bool devToolsTabOrderIsExact() {
  const std::vector<iggy3d::FrontendDevToolsCategory>& tabs =
      iggy3d::devToolsCategoryOrder();
  return expect(tabs.size() == 10U, "dev tabs count") &&
         expect(tabs[0] == iggy3d::FrontendDevToolsCategory::Session, "session first") &&
         expect(tabs[1] == iggy3d::FrontendDevToolsCategory::Input, "input second") &&
         expect(tabs[2] == iggy3d::FrontendDevToolsCategory::Player, "player third") &&
         expect(tabs[3] == iggy3d::FrontendDevToolsCategory::Movement, "movement fourth") &&
         expect(tabs[4] == iggy3d::FrontendDevToolsCategory::WorldEditor,
                "world editor fifth") &&
         expect(tabs[5] == iggy3d::FrontendDevToolsCategory::Collision,
                "collision sixth") &&
         expect(tabs[6] == iggy3d::FrontendDevToolsCategory::Spells, "spells seventh") &&
         expect(tabs[7] == iggy3d::FrontendDevToolsCategory::Camera, "camera eighth") &&
         expect(tabs[8] == iggy3d::FrontendDevToolsCategory::Renderer,
                "renderer ninth") &&
         expect(tabs[9] == iggy3d::FrontendDevToolsCategory::Performance,
                "performance tenth");
}

bool settingsDraftIsFrontendOnly() {
  iggy3d::FrontendSettings settings = iggy3d::defaultFrontendSettings();
  settings.inputBackend = iggy3d::FrontendInputBackend::Gamepad;
  settings.lookSensitivity = 1.75F;
  settings.invertLook = true;
  settings.controllerLookSensitivity = 0.5F;
  iggy3d::restoreFrontendSettingsDefaults(settings);
  return expect(settings.inputBackend == iggy3d::FrontendInputBackend::Keyboard,
                "settings restore input") &&
         expect(settings.lookSensitivity == 1.0F, "settings restore sensitivity") &&
         expect(!settings.invertLook, "settings restore invert") &&
         expect(settings.controllerLookSensitivity == 1.0F,
                "settings restore controller sensitivity");
}

bool starterAndPauseShareSettingsTabs() {
  const std::vector<iggy3d::FrontendSettingsTab>& tabs = iggy3d::settingsTabOrder();
  return expect(tabs.size() == 8U, "shared settings tab count") &&
         expect(tabs[0] == iggy3d::FrontendSettingsTab::Input, "shared input tab") &&
         expect(tabs[4] == iggy3d::FrontendSettingsTab::VideoDisplay,
                "shared video display tab") &&
         expect(tabs[7] == iggy3d::FrontendSettingsTab::Developer,
                "shared developer tab");
}

}  // namespace

int main() {
  const bool ok = starterActionOrderIsExact() && pauseActionOrderIsExact() &&
                  continueDisablesWithoutSave() &&
                  disabledSaveBackedActionsReturnIgnoredRoutes() &&
                  enabledStarterActionsReturnRouteResults() &&
                  starterChildBackReturnsToStarter() && devToolsTabOrderIsExact() &&
                  settingsDraftIsFrontendOnly() && starterAndPauseShareSettingsTabs();
  return ok ? 0 : 1;
}
