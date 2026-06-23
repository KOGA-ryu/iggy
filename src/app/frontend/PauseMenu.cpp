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

}  // namespace

std::string_view pauseCommandName(FrontendAction action) {
  switch (action) {
    case FrontendAction::Resume:
      return "pause_resume";
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

}  // namespace iggy3d
