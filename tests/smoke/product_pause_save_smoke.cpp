#include "AutomationSmokeSupport.hpp"

#include <filesystem>
#include <iostream>

int main() {
  const std::filesystem::path binary = iggy3d::smoke::productAppBinary();
  const bool appAvailable = iggy3d::smoke::productAppAvailable(binary);

  int exitCode = 77;
  iggy3d::smoke::ReceiptFields fields;

  const std::filesystem::path pauseSaveRoot =
      iggy3d::smoke::cleanSaveRoot("pause_save");
  const bool pauseSave =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "pause_save",
          "system.pause=true\npause.select=save\npause.execute=true\n",
          std::string{"--auto-new-world "} +
              iggy3d::smoke::saveRootArg(pauseSaveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "pause") &&
      iggy3d::smoke::hasField(fields, "frontend_selected_action", "save") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "pause_menu_open", "true") &&
      iggy3d::smoke::hasField(fields, "input_owner", "pause") &&
      iggy3d::smoke::hasField(fields, "product_save_status",
                              "product_save_written") &&
      iggy3d::smoke::hasField(fields, "product_save_reason_code",
                              "product_save_written") &&
      iggy3d::smoke::hasField(fields, "product_save_durable_reason",
                              "durable_save_file_written") &&
      iggy3d::smoke::hasField(fields, "product_save_source", "pause_save") &&
      iggy3d::smoke::hasField(fields, "product_save_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "product_save_session_saved", "true") &&
      iggy3d::smoke::hasField(fields, "active_product_save_id", "save_001") &&
      std::filesystem::exists(pauseSaveRoot / "save_001.iggy3d.save");

  fields.clear();
  const std::filesystem::path saveAndExitRoot =
      iggy3d::smoke::cleanSaveRoot("pause_save_and_exit");
  const bool pauseSaveAndExit =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "pause_save_and_exit",
          "system.pause=true\npause.select=save_and_exit\npause.execute=true\n",
          std::string{"--auto-new-world "} +
              iggy3d::smoke::saveRootArg(saveAndExitRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "interaction_mode", "player") &&
      iggy3d::smoke::hasField(fields, "input_owner", "starter") &&
      iggy3d::smoke::hasField(fields, "product_save_status",
                              "product_save_written") &&
      iggy3d::smoke::hasField(fields, "product_save_reason_code",
                              "product_save_written") &&
      iggy3d::smoke::hasField(fields, "product_save_durable_reason",
                              "durable_save_file_written") &&
      iggy3d::smoke::hasField(fields, "product_save_source",
                              "pause_save_and_exit") &&
      iggy3d::smoke::hasField(fields, "product_save_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "product_save_session_saved", "true") &&
      iggy3d::smoke::hasField(fields, "active_product_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "product_transition_returned_to_title",
                              "true") &&
      std::filesystem::exists(saveAndExitRoot / "save_001.iggy3d.save");

  fields.clear();
  const std::filesystem::path editSaveAndExitRoot =
      iggy3d::smoke::cleanSaveRoot("pause_edit_room_save_and_exit");
  const bool pauseEditRoomSaveAndExit =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "pause_edit_room_save_and_exit",
          "system.pause=true\n"
          "menu.down=true\n"
          "pause.execute=true\n"
          "menu.back=true\n"
          "pause.select=save_and_exit\n"
          "menu.confirm=true\n",
          std::string{"--auto-new-world "} +
              iggy3d::smoke::saveRootArg(editSaveAndExitRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "interaction_mode", "player") &&
      iggy3d::smoke::hasField(fields, "input_owner", "starter") &&
      iggy3d::smoke::hasField(fields, "room_editing_last_operation",
                              "pause_edit_room") &&
      iggy3d::smoke::hasField(fields, "product_save_status",
                              "product_save_written") &&
      iggy3d::smoke::hasField(fields, "product_save_source",
                              "pause_save_and_exit") &&
      iggy3d::smoke::hasField(fields, "active_product_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "product_transition_returned_to_title",
                              "true") &&
      std::filesystem::exists(editSaveAndExitRoot / "save_001.iggy3d.save");

  fields.clear();
  const std::filesystem::path pauseFromGameplayRoot =
      iggy3d::smoke::cleanSaveRoot("pause_from_gameplay");
  const bool pauseFromGameplay =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "pause_from_gameplay",
          "system.pause=true\n",
          std::string{"--auto-new-world "} +
              iggy3d::smoke::saveRootArg(pauseFromGameplayRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "pause") &&
      iggy3d::smoke::hasField(fields, "world_creation_initial_save_written",
                              "true") &&
      std::filesystem::exists(pauseFromGameplayRoot / "save_001.iggy3d.save") &&
      iggy3d::smoke::hasField(fields, "pause_menu_open", "true") &&
      iggy3d::smoke::hasField(fields, "input_owner", "pause") &&
      iggy3d::smoke::hasField(fields, "input_action_last", "system.pause") &&
      iggy3d::smoke::hasField(fields, "input_action_accepted", "true") &&
      iggy3d::smoke::hasField(fields, "gameplay_input_suppressed", "true");

  fields.clear();
  const std::filesystem::path returnToTitleRoot =
      iggy3d::smoke::cleanSaveRoot("return_to_title");
  const bool returnToTitle =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "return_to_title",
          "system.pause=true\npause.select=return_to_title\npause.execute=true\n",
          std::string{"--auto-new-world "} +
              iggy3d::smoke::saveRootArg(returnToTitleRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "world_creation_initial_save_written",
                              "true") &&
      std::filesystem::exists(returnToTitleRoot / "save_001.iggy3d.save") &&
      iggy3d::smoke::hasField(fields, "frontend_return_to_title_requested",
                              "true") &&
      iggy3d::smoke::hasField(fields, "product_transition_returned_to_title",
                              "true") &&
      iggy3d::smoke::hasField(fields, "interaction_mode", "player") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false");

  fields.clear();
  const std::filesystem::path editReturnToTitleRoot =
      iggy3d::smoke::cleanSaveRoot("pause_edit_room_return_to_title");
  const bool returnToTitleAfterEditRoom =
      appAvailable &&
      iggy3d::smoke::runProductCase(
          binary,
          "pause_edit_room_return_to_title",
          "system.pause=true\n"
          "menu.down=true\n"
          "pause.execute=true\n"
          "menu.back=true\n"
          "pause.select=return_to_title\n"
          "menu.confirm=true\n",
          std::string{"--auto-new-world "} +
              iggy3d::smoke::saveRootArg(editReturnToTitleRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_return_to_title_requested",
                              "true") &&
      iggy3d::smoke::hasField(fields, "product_transition_returned_to_title",
                              "true") &&
      iggy3d::smoke::hasField(fields, "interaction_mode", "player") &&
      iggy3d::smoke::hasField(fields, "room_editing_last_operation",
                              "pause_edit_room") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false");

  const bool passed = pauseSave && pauseSaveAndExit &&
                      pauseEditRoomSaveAndExit && pauseFromGameplay &&
                      returnToTitle && returnToTitleAfterEditRoom;
  std::cout << "smoke=product_pause_save\n";
  std::cout << "pause_save=" << (pauseSave ? "true" : "false") << "\n";
  std::cout << "pause_save_and_exit="
            << (pauseSaveAndExit ? "true" : "false") << "\n";
  std::cout << "pause_edit_room_save_and_exit="
            << (pauseEditRoomSaveAndExit ? "true" : "false") << "\n";
  std::cout << "pause_from_gameplay="
            << (pauseFromGameplay ? "true" : "false") << "\n";
  std::cout << "return_to_title=" << (returnToTitle ? "true" : "false")
            << "\n";
  std::cout << "pause_edit_room_return_to_title="
            << (returnToTitleAfterEditRoom ? "true" : "false") << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result=" << (passed ? "pass" : (appAvailable ? "fail" : "skip"))
            << "\n";
  std::cout << "reason_code="
            << (passed ? "product_pause_save_pass"
                       : (appAvailable ? "product_pause_save_failed"
                                       : "product_app_unavailable"))
            << "\n";
  if (passed) {
    return 0;
  }
  return appAvailable ? 1 : 77;
}
