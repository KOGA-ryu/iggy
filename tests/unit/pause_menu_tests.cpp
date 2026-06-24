#include "app/frontend/PauseMenu.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool rowOrderAndCommandsAreExact() {
  const iggy3d::PauseMenuModel model = iggy3d::buildPauseMenuModel(
      iggy3d::PauseMenuContext{true, true, true, 1U, true},
      iggy3d::FrontendAction::SaveAndExit);
  return expect(model.rows.size() == 8U, "pause row count") &&
         expect(model.rows[0].action == iggy3d::FrontendAction::Resume,
                "resume first") &&
         expect(model.rows[1].action == iggy3d::FrontendAction::Save, "save second") &&
         expect(model.rows[2].action == iggy3d::FrontendAction::SaveAndExit,
                "save exit third") &&
         expect(model.rows[3].action == iggy3d::FrontendAction::LoadSave,
                "load fourth") &&
         expect(model.rows[4].action == iggy3d::FrontendAction::Settings,
                "settings fifth") &&
         expect(model.rows[5].action == iggy3d::FrontendAction::DevTools,
                "dev sixth") &&
         expect(model.rows[6].action == iggy3d::FrontendAction::ReturnToTitle,
                "return seventh") &&
         expect(model.rows[7].action == iggy3d::FrontendAction::ExitGame,
                "exit eighth") &&
         expect(model.selectedCommand == "pause_save_and_exit",
                "selected command");
}

bool disabledReasonsAreDeterministic() {
  const iggy3d::PauseMenuModel model = iggy3d::buildPauseMenuModel(
      iggy3d::PauseMenuContext{true, true, true, 0U, false},
      iggy3d::FrontendAction::LoadSave);
  return expect(!model.rows[3].enabled, "load disabled without save") &&
         expect(model.rows[3].disabledReason == "no_compatible_save",
                "load disabled reason") &&
         expect(!model.rows[5].enabled, "dev disabled") &&
         expect(model.rows[5].disabledReason == "developer_tools_disabled",
                "dev disabled reason") &&
         expect(!model.selectedEnabled, "selected load disabled") &&
         expect(model.selectedDisabledReason == "no_compatible_save",
                "selected disabled reason");
}

bool resumeRouteReturnsToGameplay() {
  const iggy3d::PauseMenuModel model = iggy3d::buildPauseMenuModel(
      iggy3d::PauseMenuContext{true, true, true, 1U, true},
      iggy3d::FrontendAction::Resume);
  const auto route = iggy3d::routePauseAction(model, iggy3d::FrontendAction::Resume);
  return expect(route.accepted, "resume accepted") &&
         expect(route.inputOwner == iggy3d::MenuOwner::Gameplay,
                "resume owner") &&
         expect(route.nextScreen == iggy3d::FrontendScreen::Gameplay,
                "resume screen") &&
         expect(route.nextChildScreen == iggy3d::FrontendScreen::Gameplay,
                "resume child") &&
         expect(route.requestedTransition ==
                    iggy3d::FrontendTransitionRequest::None,
                "resume transition") &&
         expect(!route.gameplayInputSuppressed, "resume unsuppressed") &&
         expect(route.status == "pause_resume_requested",
                "resume status");
}

bool saveRoutesRequestTransitions() {
  const iggy3d::PauseMenuModel model = iggy3d::buildPauseMenuModel(
      iggy3d::PauseMenuContext{true, true, true, 1U, true},
      iggy3d::FrontendAction::Save);
  const auto save = iggy3d::routePauseAction(model, iggy3d::FrontendAction::Save);
  const auto saveAndExit =
      iggy3d::routePauseAction(model, iggy3d::FrontendAction::SaveAndExit);
  return expect(save.accepted, "save accepted") &&
         expect(save.requestedTransition == iggy3d::FrontendTransitionRequest::Save,
                "save transition") &&
         expect(save.status == "pause_save_requested", "save status") &&
         expect(saveAndExit.accepted, "save exit accepted") &&
         expect(saveAndExit.requestedTransition ==
                    iggy3d::FrontendTransitionRequest::SaveAndExit,
                "save exit transition") &&
         expect(saveAndExit.status == "pause_save_and_exit_requested",
                "save exit status");
}

bool childRoutesOpenExpectedScreens() {
  const iggy3d::PauseMenuModel model = iggy3d::buildPauseMenuModel(
      iggy3d::PauseMenuContext{true, true, true, 1U, true},
      iggy3d::FrontendAction::Settings);
  const auto settings =
      iggy3d::routePauseAction(model, iggy3d::FrontendAction::Settings);
  const auto devTools =
      iggy3d::routePauseAction(model, iggy3d::FrontendAction::DevTools);
  const auto loadSave =
      iggy3d::routePauseAction(model, iggy3d::FrontendAction::LoadSave);
  return expect(settings.accepted, "settings accepted") &&
         expect(settings.inputOwner == iggy3d::MenuOwner::Settings,
                "settings owner") &&
         expect(settings.nextScreen == iggy3d::FrontendScreen::Settings,
                "settings screen") &&
         expect(settings.nextChildScreen == iggy3d::FrontendScreen::Pause,
                "settings parent child") &&
         expect(settings.status == "pause_settings_opened",
                "settings status") &&
         expect(devTools.accepted, "dev accepted") &&
         expect(devTools.inputOwner == iggy3d::MenuOwner::DevTools,
                "dev owner") &&
         expect(devTools.nextScreen == iggy3d::FrontendScreen::DevOverlay,
                "dev screen") &&
         expect(devTools.status == "pause_dev_tools_opened",
                "dev status") &&
         expect(loadSave.accepted, "load save accepted") &&
         expect(loadSave.nextScreen == iggy3d::FrontendScreen::Pause,
                "load save screen") &&
         expect(loadSave.nextChildScreen == iggy3d::FrontendScreen::LoadSave,
                "load save child") &&
         expect(loadSave.status == "pause_load_save_opened",
                "load save status");
}

bool titleAndExitRoutesRequestTransitions() {
  const iggy3d::PauseMenuModel model = iggy3d::buildPauseMenuModel(
      iggy3d::PauseMenuContext{true, true, true, 1U, true},
      iggy3d::FrontendAction::ReturnToTitle);
  const auto title =
      iggy3d::routePauseAction(model, iggy3d::FrontendAction::ReturnToTitle);
  const auto exit =
      iggy3d::routePauseAction(model, iggy3d::FrontendAction::ExitGame);
  return expect(title.accepted, "title accepted") &&
         expect(title.inputOwner == iggy3d::MenuOwner::Starter,
                "title owner") &&
         expect(title.nextScreen == iggy3d::FrontendScreen::Starter,
                "title screen") &&
         expect(title.requestedTransition ==
                    iggy3d::FrontendTransitionRequest::ReturnToTitle,
                "title transition") &&
         expect(title.status == "pause_return_to_title_requested",
                "title status") &&
         expect(exit.accepted, "exit accepted") &&
         expect(exit.closeRequested, "exit close") &&
         expect(exit.requestedTransition == iggy3d::FrontendTransitionRequest::Exit,
                "exit transition") &&
         expect(exit.status == "pause_exit_game_requested",
                "exit status");
}

bool disabledLoadSaveAndUnsupportedActionsAreIgnored() {
  const iggy3d::PauseMenuModel model = iggy3d::buildPauseMenuModel(
      iggy3d::PauseMenuContext{true, true, true, 0U, true},
      iggy3d::FrontendAction::LoadSave);
  const auto loadSave =
      iggy3d::routePauseAction(model, iggy3d::FrontendAction::LoadSave);
  const auto unsupported =
      iggy3d::routePauseAction(model, iggy3d::FrontendAction::Apply);
  return expect(!loadSave.accepted, "disabled load save not accepted") &&
         expect(loadSave.status == "no_compatible_save",
                "disabled load save status") &&
         expect(loadSave.inputOwner == iggy3d::MenuOwner::Pause,
                "disabled load save owner") &&
         expect(loadSave.nextScreen == iggy3d::FrontendScreen::Pause,
                "disabled load save screen") &&
         expect(loadSave.gameplayInputSuppressed,
                "disabled load save suppresses") &&
         expect(!unsupported.accepted, "unsupported not accepted") &&
         expect(unsupported.status == "not_pause_action",
                "unsupported status");
}

}  // namespace

int main() {
  const bool ok = rowOrderAndCommandsAreExact() && disabledReasonsAreDeterministic() &&
                  resumeRouteReturnsToGameplay() && saveRoutesRequestTransitions() &&
                  childRoutesOpenExpectedScreens() &&
                  titleAndExitRoutesRequestTransitions() &&
                  disabledLoadSaveAndUnsupportedActionsAreIgnored();
  return ok ? 0 : 1;
}
