#include "app/frontend/StarterScreen.hpp"

namespace iggy3d {

StarterScreenModel buildStarterScreenModel(std::uint64_t compatibleSaveCount,
                                           FrontendAction selected) {
  StarterScreenModel model;
  model.actions = starterActionOrder();
  model.compatibleSaveCount = compatibleSaveCount;
  model.continueEnabled = compatibleSaveCount > 0U;
  model.disabled = model.continueEnabled ? FrontendAction::None : FrontendAction::Continue;
  model.selected = selected;
  if (model.selected == FrontendAction::None) {
    model.selected = FrontendAction::Continue;
  }
  return model;
}

FrontendScreen starterChildScreenForAction(FrontendAction action) {
  switch (action) {
    case FrontendAction::NewWorld:
      return FrontendScreen::NewWorld;
    case FrontendAction::LoadSave:
      return FrontendScreen::LoadSave;
    case FrontendAction::Settings:
      return FrontendScreen::Settings;
    case FrontendAction::DevTools:
      return FrontendScreen::StarterDevTools;
    case FrontendAction::Exit:
      return FrontendScreen::ExitConfirm;
    case FrontendAction::Continue:
    case FrontendAction::None:
    case FrontendAction::CreateAndEnter:
    case FrontendAction::Load:
    case FrontendAction::Delete:
    case FrontendAction::Back:
    case FrontendAction::Apply:
    case FrontendAction::RestoreDefaults:
    case FrontendAction::Resume:
    case FrontendAction::Save:
    case FrontendAction::SaveAndExit:
    case FrontendAction::ReturnToTitle:
    case FrontendAction::ExitGame:
      return FrontendScreen::Starter;
  }
  return FrontendScreen::Starter;
}

std::string_view starterActionLabel(FrontendAction action) {
  switch (action) {
    case FrontendAction::Continue:
      return "Continue";
    case FrontendAction::NewWorld:
      return "New World";
    case FrontendAction::LoadSave:
      return "Load Save";
    case FrontendAction::Settings:
      return "Settings";
    case FrontendAction::DevTools:
      return "Dev Tools";
    case FrontendAction::Exit:
      return "Exit";
    default:
      return "";
  }
}

}  // namespace iggy3d
