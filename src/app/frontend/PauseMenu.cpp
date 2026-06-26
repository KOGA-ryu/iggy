#include "app/frontend/PauseMenu.hpp"

namespace iggy3d {

namespace {

MenuRowModel makePauseRow(FrontendAction action, const PauseMenuContext& context) {
  MenuRowModel row;
  row.action = action;
  row.command = pauseCommandName(action);
  switch (action) {
    case FrontendAction::Resume:
      row.enabled = context.pauseOpen;
      row.disabledReason = row.enabled ? "none" : "pause_not_open";
      break;
    case FrontendAction::EditRoom:
      row.enabled = context.pauseOpen && context.activeRoomEditable;
      row.disabledReason = row.enabled ? "none" : "active_room_unavailable";
      break;
    case FrontendAction::Save:
      row.enabled = context.runtimeSessionAvailable && context.saveRootWritable;
      row.disabledReason = row.enabled ? "none" : "save_unavailable";
      break;
    case FrontendAction::SaveAndExit:
      row.enabled = context.runtimeSessionAvailable && context.saveRootWritable;
      row.disabledReason = row.enabled ? "none" : "save_unavailable";
      break;
    case FrontendAction::LoadSave:
      row.enabled = context.compatibleSaveCount > 0U;
      row.disabledReason = row.enabled ? "none" : "no_compatible_save";
      break;
    case FrontendAction::Settings:
      row.enabled = true;
      row.disabledReason = "none";
      break;
    case FrontendAction::DevTools:
      row.enabled = context.developerToolsEnabled;
      row.disabledReason = row.enabled ? "none" : "developer_tools_disabled";
      break;
    case FrontendAction::ReturnToTitle:
    case FrontendAction::ExitGame:
      row.enabled = true;
      row.disabledReason = "none";
      break;
    case FrontendAction::None:
    case FrontendAction::Continue:
    case FrontendAction::NewWorld:
    case FrontendAction::Exit:
    case FrontendAction::Load:
    case FrontendAction::Delete:
    case FrontendAction::Back:
    case FrontendAction::Apply:
    case FrontendAction::RestoreDefaults:
    case FrontendAction::CreateAndEnter:
      row.enabled = false;
      row.disabledReason = "not_pause_action";
      break;
  }
  return row;
}

const MenuRowModel* findPauseRow(const PauseMenuModel& model,
                                 FrontendAction action) {
  for (const MenuRowModel& row : model.rows) {
    if (row.action == action) {
      return &row;
    }
  }
  return nullptr;
}

FrontendRouteResult ignoredPauseRoute(FrontendAction action,
                                      std::string_view status) {
  FrontendRouteResult result = makeIgnoredFrontendRouteResult(
      MenuOwner::Pause,
      FrontendScreen::Pause,
      FrontendScreen::Gameplay,
      action);
  result.gameplayInputSuppressed = true;
  result.status = status;
  result.receiptReason = status;
  return result;
}

FrontendRouteResult acceptedPauseRoute(MenuOwner owner,
                                       FrontendScreen nextScreen,
                                       FrontendScreen nextChildScreen,
                                       FrontendTransitionRequest transition,
                                       bool closeRequested,
                                       bool gameplayInputSuppressed,
                                       std::string_view status,
                                       FrontendAction action) {
  return makeAcceptedFrontendRouteResult(owner,
                                         nextScreen,
                                         nextChildScreen,
                                         transition,
                                         closeRequested,
                                         gameplayInputSuppressed,
                                         status,
                                         status,
                                         action);
}

}  // namespace

std::string_view pauseCommandName(FrontendAction action) {
  switch (action) {
    case FrontendAction::Resume:
      return "pause_resume";
    case FrontendAction::EditRoom:
      return "pause_edit_room";
    case FrontendAction::Save:
      return "pause_save";
    case FrontendAction::SaveAndExit:
      return "pause_save_and_exit";
    case FrontendAction::LoadSave:
      return "pause_load_save";
    case FrontendAction::Settings:
      return "pause_open_settings";
    case FrontendAction::DevTools:
      return "pause_open_dev_tools";
    case FrontendAction::ReturnToTitle:
      return "pause_return_to_title";
    case FrontendAction::ExitGame:
      return "pause_exit_game";
    default:
      return "none";
  }
}

PauseMenuModel buildPauseMenuModel(const PauseMenuContext& context,
                                   FrontendAction selected) {
  PauseMenuModel model;
  model.selected = selected == FrontendAction::None ? FrontendAction::Resume : selected;
  for (const FrontendAction action : pauseActionOrder()) {
    MenuRowModel row = makePauseRow(action, context);
    if (row.enabled) {
      ++model.enabledRowCount;
    }
    if (row.action == model.selected) {
      model.selectedEnabled = row.enabled;
      model.selectedDisabledReason = row.disabledReason;
      model.selectedCommand = row.command;
    }
    model.rows.push_back(row);
  }
  return model;
}

FrontendRouteResult routePauseAction(const PauseMenuModel& model,
                                     FrontendAction action) {
  const MenuRowModel* row = findPauseRow(model, action);
  if (row == nullptr) {
    return ignoredPauseRoute(action, "not_pause_action");
  }
  if (!row->enabled) {
    return ignoredPauseRoute(action, row->disabledReason);
  }

  switch (action) {
    case FrontendAction::Resume:
      return acceptedPauseRoute(MenuOwner::Gameplay,
                                FrontendScreen::Gameplay,
                                FrontendScreen::Gameplay,
                                FrontendTransitionRequest::None,
                                false,
                                false,
                                "pause_resume_requested",
                                action);
    case FrontendAction::EditRoom:
      return acceptedPauseRoute(MenuOwner::Gameplay,
                                FrontendScreen::Gameplay,
                                FrontendScreen::Gameplay,
                                FrontendTransitionRequest::None,
                                false,
                                false,
                                "pause_edit_room_requested",
                                action);
    case FrontendAction::Save:
      return acceptedPauseRoute(MenuOwner::Pause,
                                FrontendScreen::Pause,
                                FrontendScreen::Gameplay,
                                FrontendTransitionRequest::Save,
                                false,
                                true,
                                "pause_save_requested",
                                action);
    case FrontendAction::SaveAndExit:
      return acceptedPauseRoute(MenuOwner::Pause,
                                FrontendScreen::Pause,
                                FrontendScreen::Gameplay,
                                FrontendTransitionRequest::SaveAndExit,
                                false,
                                true,
                                "pause_save_and_exit_requested",
                                action);
    case FrontendAction::LoadSave:
      return acceptedPauseRoute(MenuOwner::Pause,
                                FrontendScreen::Pause,
                                FrontendScreen::LoadSave,
                                FrontendTransitionRequest::None,
                                false,
                                true,
                                "pause_load_save_opened",
                                action);
    case FrontendAction::Settings:
      return acceptedPauseRoute(MenuOwner::Settings,
                                FrontendScreen::Settings,
                                FrontendScreen::Pause,
                                FrontendTransitionRequest::None,
                                false,
                                true,
                                "pause_settings_opened",
                                action);
    case FrontendAction::DevTools:
      return acceptedPauseRoute(MenuOwner::DevTools,
                                FrontendScreen::DevOverlay,
                                FrontendScreen::Gameplay,
                                FrontendTransitionRequest::None,
                                false,
                                true,
                                "pause_dev_tools_opened",
                                action);
    case FrontendAction::ReturnToTitle:
      return acceptedPauseRoute(MenuOwner::Starter,
                                FrontendScreen::Starter,
                                FrontendScreen::Gameplay,
                                FrontendTransitionRequest::ReturnToTitle,
                                false,
                                true,
                                "pause_return_to_title_requested",
                                action);
    case FrontendAction::ExitGame:
      return acceptedPauseRoute(MenuOwner::Pause,
                                FrontendScreen::Pause,
                                FrontendScreen::Gameplay,
                                FrontendTransitionRequest::Exit,
                                true,
                                true,
                                "pause_exit_game_requested",
                                action);
    case FrontendAction::None:
    case FrontendAction::Continue:
    case FrontendAction::NewWorld:
    case FrontendAction::Exit:
    case FrontendAction::CreateAndEnter:
    case FrontendAction::Load:
    case FrontendAction::Delete:
    case FrontendAction::Back:
    case FrontendAction::Apply:
    case FrontendAction::RestoreDefaults:
      break;
  }

  return ignoredPauseRoute(action, "not_pause_action");
}

}  // namespace iggy3d
