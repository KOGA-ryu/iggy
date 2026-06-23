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
  model.selectedEnabled = starterActionEnabled(model.selected, compatibleSaveCount);
  model.selectedDisabledReason =
      starterActionDisabledReason(model.selected, compatibleSaveCount);
  model.selectedCommand = starterActionCommand(model.selected);
  return model;
}

FrontendScreen starterChildScreenForAction(FrontendAction action) {
  switch (action) {
    case FrontendAction::NewWorld:
      return FrontendScreen::NewWorld;
    case FrontendAction::LoadSave:
      return FrontendScreen::LoadSave;
    case FrontendAction::Delete:
      return FrontendScreen::DeleteConfirm;
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
      return "Existing Saves";
    case FrontendAction::Delete:
      return "Delete Save";
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

std::string_view starterActionCommand(FrontendAction action) {
  switch (action) {
    case FrontendAction::Continue:
      return "starter_continue";
    case FrontendAction::NewWorld:
      return "starter_new_world";
    case FrontendAction::LoadSave:
      return "starter_existing_saves";
    case FrontendAction::Delete:
      return "starter_delete_save";
    case FrontendAction::Settings:
      return "starter_open_settings";
    case FrontendAction::DevTools:
      return "starter_open_dev_tools";
    case FrontendAction::Exit:
      return "starter_exit";
    default:
      return "none";
  }
}

std::string_view starterActionDisabledReason(FrontendAction action,
                                             std::uint64_t compatibleSaveCount) {
  if (starterActionEnabled(action, compatibleSaveCount)) {
    return "none";
  }
  switch (action) {
    case FrontendAction::Continue:
    case FrontendAction::LoadSave:
    case FrontendAction::Delete:
      return "no_compatible_save";
    default:
      return "not_starter_action";
  }
}

bool starterActionEnabled(FrontendAction action, std::uint64_t compatibleSaveCount) {
  switch (action) {
    case FrontendAction::Continue:
    case FrontendAction::LoadSave:
    case FrontendAction::Delete:
      return compatibleSaveCount > 0U;
    case FrontendAction::NewWorld:
    case FrontendAction::Settings:
    case FrontendAction::DevTools:
    case FrontendAction::Exit:
      return true;
    default:
      return false;
  }
}

}  // namespace iggy3d
