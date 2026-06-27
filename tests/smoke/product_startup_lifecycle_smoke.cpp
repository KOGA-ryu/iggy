#include "AutomationSmokeSupport.hpp"

#include <filesystem>
#include <iostream>

int main() {
  const std::filesystem::path binary = iggy3d::smoke::productAppBinary();
  const bool appAvailable = iggy3d::smoke::productAppAvailable(binary);

  int exitCode = 77;
  iggy3d::smoke::ReceiptFields fields;
  const std::filesystem::path saveRoot =
      iggy3d::smoke::cleanSaveRoot("startup_lifecycle");

  const bool freshStarter =
      appAvailable &&
      iggy3d::smoke::runProductReceiptCase(
          binary, "startup_lifecycle_fresh_starter",
          iggy3d::smoke::saveRootArg(saveRoot), fields, exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_child_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "input_owner", "starter") &&
      iggy3d::smoke::hasField(fields, "save_count", "0") &&
      iggy3d::smoke::hasField(fields, "compatible_save_count", "0") &&
      !std::filesystem::exists(saveRoot / "save_001.iggy3d.save");

  fields.clear();
  const bool createSaveAndExit =
      appAvailable && freshStarter &&
      iggy3d::smoke::runProductCase(
          binary,
          "startup_lifecycle_create_save_exit",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "world.title=Loop Chapter\nworld.create=true\n"
          "system.pause=true\npause.select=save_and_exit\npause.execute=true\n",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_child_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "input_owner", "starter") &&
      iggy3d::smoke::hasField(fields, "world_setup_title", "Loop Chapter") &&
      iggy3d::smoke::hasField(fields, "world_creation_status",
                              "world_creation_initial_save_written") &&
      iggy3d::smoke::hasField(fields, "world_creation_world_title",
                              "Loop Chapter") &&
      iggy3d::smoke::hasField(fields, "world_creation_initial_save_title",
                              "Loop Chapter") &&
      iggy3d::smoke::hasField(fields, "world_creation_initial_save_id",
                              "save_001") &&
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
      iggy3d::smoke::hasField(fields, "save_count", "1") &&
      iggy3d::smoke::hasField(fields, "compatible_save_count", "1") &&
      iggy3d::smoke::hasField(fields, "product_transition_returned_to_title",
                              "true") &&
      std::filesystem::exists(saveRoot / "save_001.iggy3d.save");

  fields.clear();
  const bool rebootStarter =
      appAvailable && createSaveAndExit &&
      iggy3d::smoke::runProductReceiptCase(
          binary, "startup_lifecycle_reboot_starter",
          iggy3d::smoke::saveRootArg(saveRoot), fields, exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "frontend_child_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "input_owner", "starter") &&
      iggy3d::smoke::hasField(fields, "save_count", "1") &&
      iggy3d::smoke::hasField(fields, "compatible_save_count", "1") &&
      iggy3d::smoke::hasField(fields, "world_creation_status", "not_requested") &&
      iggy3d::smoke::hasField(fields, "product_save_status", "not_requested") &&
      iggy3d::smoke::hasField(fields, "product_save_load_status",
                              "not_requested") &&
      std::filesystem::exists(saveRoot / "save_001.iggy3d.save");

  fields.clear();
  const bool continueResume =
      appAvailable && rebootStarter &&
      iggy3d::smoke::runProductCase(
          binary,
          "startup_lifecycle_continue_resume",
          "frontend.select=continue\nfrontend.execute=true\n",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(fields, "window_created", "false") &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "frontend_selected_action", "continue") &&
      iggy3d::smoke::hasField(fields, "frontend_launch_requested", "true") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "save_count", "1") &&
      iggy3d::smoke::hasField(fields, "compatible_save_count", "1") &&
      iggy3d::smoke::hasField(fields, "product_save_load_status",
                              "product_save_loaded") &&
      iggy3d::smoke::hasField(fields, "product_save_load_reason_code",
                              "product_save_loaded") &&
      iggy3d::smoke::hasField(fields, "product_save_load_source", "continue") &&
      iggy3d::smoke::hasField(fields, "product_save_load_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "product_save_load_selected_id",
                              "save_001") &&
      iggy3d::smoke::hasField(fields, "product_save_load_selected_enabled",
                              "true") &&
      iggy3d::smoke::hasField(fields, "product_save_load_session_loaded",
                              "true") &&
      iggy3d::smoke::positiveIntegerField(fields,
                                          "product_save_load_loaded_hash") &&
      iggy3d::smoke::hasField(fields, "active_product_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "product_transition_last_action",
                              "launch_gameplay") &&
      iggy3d::smoke::hasField(fields, "product_transition_status",
                              "gameplay_active");

  const bool passed = freshStarter && createSaveAndExit && rebootStarter &&
                      continueResume;
  std::cout << "smoke=product_startup_lifecycle\n";
  std::cout << "fresh_starter=" << (freshStarter ? "true" : "false") << "\n";
  std::cout << "create_save_and_exit="
            << (createSaveAndExit ? "true" : "false") << "\n";
  std::cout << "reboot_starter=" << (rebootStarter ? "true" : "false") << "\n";
  std::cout << "continue_resume=" << (continueResume ? "true" : "false")
            << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result=" << (passed ? "pass" : (appAvailable ? "fail" : "skip"))
            << "\n";
  std::cout << "reason_code="
            << (passed ? "product_startup_lifecycle_pass"
                       : (appAvailable ? "product_startup_lifecycle_failed"
                                       : "product_app_unavailable"))
            << "\n";
  if (passed) {
    return 0;
  }
  return appAvailable ? 1 : 77;
}
