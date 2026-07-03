#include "app/frontend/StarterScreen.hpp"

namespace iggy3d {
namespace {

FrontendRouteResult acceptedStarterChildRoute(FrontendScreen childScreen,
                                              std::string_view status,
                                              FrontendAction action) {
  return makeAcceptedFrontendRouteResult(MenuOwner::Starter,
                                         FrontendScreen::Starter,
                                         childScreen,
                                         FrontendTransitionRequest::None,
                                         false,
                                         true,
                                         status,
                                         status,
                                         action);
}

}  // namespace

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

FrontendRouteResult routeStarterAction(const StarterScreenModel& model,
                                       FrontendAction action) {
  if (!starterActionEnabled(action, model.compatibleSaveCount)) {
    FrontendRouteResult result = makeIgnoredFrontendRouteResult(
        MenuOwner::Starter,
        FrontendScreen::Starter,
        FrontendScreen::Gameplay,
        action);
    result.status = starterActionDisabledReason(action, model.compatibleSaveCount);
    result.receiptReason = result.status;
    return result;
  }

  switch (action) {
    case FrontendAction::Continue:
      return makeAcceptedFrontendRouteResult(MenuOwner::Gameplay,
                                             FrontendScreen::Gameplay,
                                             FrontendScreen::Gameplay,
                                             FrontendTransitionRequest::LaunchGameplay,
                                             false,
                                             false,
                                             "starter_launch_continue",
                                             "starter_launch_continue",
                                             action);
    case FrontendAction::NewWorld:
      return acceptedStarterChildRoute(FrontendScreen::NewWorld,
                                       "starter_new_world_opened",
                                       action);
    case FrontendAction::CreativeNewWorld:
      return makeAcceptedFrontendRouteResult(MenuOwner::Gameplay,
                                             FrontendScreen::Gameplay,
                                             FrontendScreen::Gameplay,
                                             FrontendTransitionRequest::LaunchGameplay,
                                             false,
                                             false,
                                             "starter_creative_new_world_launch",
                                             "starter_creative_new_world_launch",
                                             action);
    case FrontendAction::LoadSave:
      return acceptedStarterChildRoute(FrontendScreen::LoadSave,
                                       "starter_load_save_opened",
                                       action);
    case FrontendAction::Delete:
      // The Delete entry opens the selectable save browser (LoadSave) so the
      // operator picks WHICH map; the DELETE SELECTED button inside the browser
      // is what reaches the DeleteConfirm dialog. Keeping this model in step
      // with the live handler (confirmStarterDelete) avoids a routing lie.
      return acceptedStarterChildRoute(FrontendScreen::LoadSave,
                                       "starter_delete_select_opened",
                                       action);
    case FrontendAction::Settings:
      return acceptedStarterChildRoute(FrontendScreen::Settings,
                                       "starter_settings_opened",
                                       action);
    case FrontendAction::DevTools:
      return acceptedStarterChildRoute(FrontendScreen::StarterDevTools,
                                       "starter_dev_tools_opened",
                                       action);
    case FrontendAction::Exit:
      return makeAcceptedFrontendRouteResult(MenuOwner::Starter,
                                             FrontendScreen::Starter,
                                             FrontendScreen::ExitConfirm,
                                             FrontendTransitionRequest::Exit,
                                             true,
                                             true,
                                             "starter_exit_requested",
                                             "starter_exit_requested",
                                             action);
    case FrontendAction::None:
    case FrontendAction::CreateAndEnter:
    case FrontendAction::Load:
    case FrontendAction::Back:
    case FrontendAction::Apply:
    case FrontendAction::RestoreDefaults:
    case FrontendAction::Resume:
    case FrontendAction::EditRoom:
    case FrontendAction::LeaveEditor:
    case FrontendAction::Save:
    case FrontendAction::SaveAndExit:
    case FrontendAction::ReturnToTitle:
    case FrontendAction::ExitGame:
      break;
  }

  FrontendRouteResult result = makeIgnoredFrontendRouteResult(
      MenuOwner::Starter,
      FrontendScreen::Starter,
      FrontendScreen::Gameplay,
      action);
  result.status = "not_starter_action";
  result.receiptReason = "not_starter_action";
  return result;
}

FrontendRouteResult routeStarterBackFromChild(FrontendScreen childScreen) {
  return makeAcceptedFrontendRouteResult(MenuOwner::Starter,
                                         FrontendScreen::Starter,
                                         FrontendScreen::Gameplay,
                                         FrontendTransitionRequest::None,
                                         false,
                                         true,
                                         "starter_child_returned",
                                         "starter_child_returned",
                                         childScreen == FrontendScreen::Gameplay
                                             ? FrontendAction::None
                                             : FrontendAction::Back);
}

std::string_view starterActionLabel(FrontendAction action) {
  switch (action) {
    case FrontendAction::Continue:
      return "Continue";
    case FrontendAction::NewWorld:
      return "Build Map";
    case FrontendAction::CreativeNewWorld:
      return "Creative World";
    case FrontendAction::LoadSave:
      return "Existing Maps";
    case FrontendAction::Delete:
      return "Delete Map";
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
    case FrontendAction::CreativeNewWorld:
      return "starter_creative_new_world";
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
    case FrontendAction::CreativeNewWorld:
    case FrontendAction::Settings:
    case FrontendAction::DevTools:
    case FrontendAction::Exit:
      return true;
    default:
      return false;
  }
}

}  // namespace iggy3d
