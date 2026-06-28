#include "app/frontend/FrontendReceipt.hpp"

namespace iggy3d {

FrontendReceiptFields receiptFieldsFromFrontendState(const FrontendState& state) {
  FrontendReceiptFields fields;
  fields.screen = state.screen;
  fields.selectedAction = state.selectedAction;
  fields.status = std::string(state.status);
  fields.launchRequested = state.launchRequested;
  fields.returnToTitleRequested = state.returnToTitleRequested;
  fields.inputOwned = frontendBlocksGameplayInput(state);
  fields.childScreen = state.childScreen;
  fields.disabledAction = state.disabledAction;
  fields.pauseMenuOpen = frontendPauseMenuOpen(state);
  fields.devToolsOpen = frontendDevToolsOpen(state);
  fields.devToolsCategory = state.devToolsCategory;
  return fields;
}

void appendFrontendReceiptFields(RenderReceipt& receipt,
                                 const FrontendReceiptFields& fields) {
  appendReceiptField(receipt, "frontend_screen", frontendScreenName(fields.screen));
  appendReceiptField(receipt, "frontend_selected_action",
                     frontendActionName(fields.selectedAction));
  appendReceiptField(receipt, "frontend_status", fields.status);
  appendReceiptField(receipt, "frontend_launch_requested", fields.launchRequested);
  appendReceiptField(receipt, "frontend_return_to_title_requested",
                     fields.returnToTitleRequested);
  appendReceiptField(receipt, "frontend_input_owned", fields.inputOwned);
  appendReceiptField(receipt, "frontend_child_screen",
                     fields.childScreen == FrontendScreen::Gameplay
                         ? std::string_view("none")
                         : frontendScreenName(fields.childScreen));
  appendReceiptField(receipt, "frontend_disabled_action",
                     frontendActionName(fields.disabledAction));
  appendReceiptField(receipt, "pause_menu_open", fields.pauseMenuOpen);
  appendReceiptField(receipt, "dev_tools_open", fields.devToolsOpen);
  appendReceiptField(receipt, "dev_tools_category",
                     frontendDevToolsCategoryName(fields.devToolsCategory));
  appendReceiptField(receipt, "settings_input_backend", fields.settingsInputBackend);
  appendReceiptField(receipt, "settings_look_sensitivity", fields.settingsLookSensitivity);
  appendReceiptField(receipt, "settings_invert_look", fields.settingsInvertLook);
  appendReceiptField(receipt, "selected_save_id", fields.selectedSaveId);
  appendReceiptField(receipt, "selected_package_id", fields.selectedPackageId);
  appendReceiptField(receipt, "selected_scenario_id", fields.selectedScenarioId);
  appendReceiptField(receipt, "save_root",
                     fields.saveRoot.empty() ? std::string("none") : fields.saveRoot.string());
  appendReceiptField(receipt, "save_count", fields.saveCount);
  appendReceiptField(receipt, "compatible_save_count", fields.compatibleSaveCount);
  appendReceiptField(receipt, "corrupt_save_count", fields.corruptSaveCount);
  appendReceiptField(receipt, "selected_save_compatible", fields.selectedSaveCompatible);
  appendReceiptField(receipt, "selected_save_package_id", fields.selectedSavePackageId);
  appendReceiptField(receipt, "selected_save_scenario_id", fields.selectedSaveScenarioId);
  appendReceiptField(receipt, "selected_save_tick", fields.selectedSaveTick);
  appendReceiptField(receipt, "selected_save_hash", fields.selectedSaveHash);
  appendReceiptField(receipt, "selected_save_authored_floor_count",
                     fields.selectedSaveAuthoredFloorCount);
  appendReceiptField(receipt, "selected_save_authored_wall_count",
                     fields.selectedSaveAuthoredWallCount);
  appendReceiptField(receipt, "starter_header_visible", fields.starterHeaderVisible);
  appendReceiptField(receipt, "starter_action_list_visible",
                     fields.starterActionListVisible);
  appendReceiptField(receipt, "starter_detail_panel_visible",
                     fields.starterDetailPanelVisible);
  appendReceiptField(receipt, "starter_status_strip_visible",
                     fields.starterStatusStripVisible);
  appendReceiptField(receipt, "starter_row_count", fields.starterRowCount);
  appendReceiptField(receipt, "starter_selected_action",
                     frontendActionName(fields.selectedAction));
  appendReceiptField(receipt, "starter_selected_enabled",
                     fields.starterSelectedEnabled);
  appendReceiptField(receipt, "starter_selected_disabled_reason",
                     fields.starterSelectedDisabledReason);
  appendReceiptField(receipt, "menu_owner", fields.menuOwner);
  appendReceiptField(receipt, "gameplay_input_suppressed",
                     fields.gameplayInputSuppressed);
  appendReceiptField(receipt, "pause_selected_action",
                     frontendActionName(fields.pauseSelectedAction));
  appendReceiptField(receipt, "pause_selected_enabled",
                     fields.pauseSelectedEnabled);
  appendReceiptField(receipt, "pause_selected_disabled_reason",
                     fields.pauseSelectedDisabledReason);
  appendReceiptField(receipt, "pause_action_command", fields.pauseActionCommand);
  appendReceiptField(receipt, "pause_action_executed", fields.pauseActionExecuted);
  appendReceiptField(receipt, "pause_action_status", fields.pauseActionStatus);
  appendReceiptField(receipt, "pause_parent_screen", fields.pauseParentScreen);
  appendReceiptField(receipt, "pause_row_count", fields.pauseRowCount);
  appendReceiptField(receipt, "pause_enabled_row_count", fields.pauseEnabledRowCount);
  appendReceiptField(receipt, "settings_open", fields.settingsOpen);
  appendReceiptField(receipt, "settings_parent", fields.settingsParent);
  appendReceiptField(receipt, "settings_tab", frontendSettingsTabName(fields.settingsTab));
  appendReceiptField(receipt, "settings_selected_row", fields.settingsSelectedRow);
  appendReceiptField(receipt, "settings_selected_enabled",
                     fields.settingsSelectedEnabled);
  appendReceiptField(receipt, "settings_selected_disabled_reason",
                     fields.settingsSelectedDisabledReason);
  appendReceiptField(receipt, "settings_apply_requested",
                     fields.settingsApplyRequested);
  appendReceiptField(receipt, "settings_restore_defaults_requested",
                     fields.settingsRestoreDefaultsRequested);
  appendReceiptField(receipt, "settings_back_requested", fields.settingsBackRequested);
  appendReceiptField(receipt, "settings_renderer", fields.settingsRenderer);
  appendReceiptField(receipt, "settings_window_mode", fields.settingsWindowMode);
  appendReceiptField(receipt, "settings_controller_look_sensitivity",
                     fields.settingsControllerLookSensitivity);
  appendReceiptField(receipt, "settings_audio_available", fields.settingsAudioAvailable);
  appendReceiptField(receipt, "settings_accessibility_high_contrast",
                     fields.settingsAccessibilityHighContrast);
  appendReceiptField(receipt, "settings_accessibility_reduced_motion",
                     fields.settingsAccessibilityReducedMotion);
  appendReceiptField(receipt, "settings_developer_tools_enabled",
                     fields.settingsDeveloperToolsEnabled);
  appendReceiptField(receipt, "settings_persistence", fields.settingsPersistence);
  appendReceiptField(receipt, "dev_tools_parent", fields.devToolsParent);
  appendReceiptField(receipt, "dev_tools_input_blocking",
                     fields.devToolsInputBlocking);
  appendReceiptField(receipt, "dev_tools_readout_visible",
                     fields.devToolsReadoutVisible);
  appendReceiptField(receipt, "dev_tools_selected_action",
                     fields.devToolsSelectedAction);
  appendReceiptField(receipt, "dev_tools_selected_enabled",
                     fields.devToolsSelectedEnabled);
  appendReceiptField(receipt, "dev_tools_selected_disabled_reason",
                     fields.devToolsSelectedDisabledReason);
  appendReceiptField(receipt, "dev_tools_command_status",
                     fields.devToolsCommandStatus);
  appendReceiptField(receipt, "dev_tools_runtime_readout_count",
                     fields.devToolsRuntimeReadoutCount);
  appendReceiptField(receipt, "gamepad_options_opens", fields.gamepadOptionsOpens);
  appendReceiptField(receipt, "gamepad_create_options_quit",
                     fields.gamepadCreateOptionsQuit);
  appendReceiptField(receipt, "window_launch_count", fields.windowLaunchCount);
}

}  // namespace iggy3d
