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
  fields.pauseMenuOpen = state.pauseMenuOpen;
  fields.devToolsOpen = state.devToolsOpen;
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
}

}  // namespace iggy3d
