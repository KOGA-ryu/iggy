#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <system_error>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/wait.h>
#endif

namespace {

std::string shellQuote(const std::filesystem::path& path) {
  std::string value = path.string();
  std::string quoted = "'";
  for (const char character : value) {
    if (character == '\'') {
      quoted += "'\\''";
    } else {
      quoted.push_back(character);
    }
  }
  quoted += "'";
  return quoted;
}

int exitCodeFromSystem(int status) {
  if (status == -1) {
    return 1;
  }
#if defined(__unix__) || defined(__APPLE__)
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  return 1;
#else
  return status;
#endif
}

bool writeControlFile(const std::filesystem::path& path, std::string_view content) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << content;
  return static_cast<bool>(output);
}

bool parseReceiptFile(const std::filesystem::path& path,
                      std::map<std::string, std::string>& fields) {
  fields.clear();
  std::ifstream input(path);
  if (!input) {
    return false;
  }
  std::string line;
  while (std::getline(input, line)) {
    const std::size_t equals = line.find('=');
    if (equals == std::string::npos || equals == 0U) {
      return false;
    }
    if (!fields.emplace(line.substr(0, equals), line.substr(equals + 1U)).second) {
      return false;
    }
  }
  return true;
}

bool hasField(const std::map<std::string, std::string>& fields,
              std::string_view key,
              std::string_view value) {
  const auto found = fields.find(std::string(key));
  return found != fields.end() && found->second == value;
}

bool positiveIntegerField(const std::map<std::string, std::string>& fields,
                          std::string_view key) {
  const auto found = fields.find(std::string(key));
  if (found == fields.end() || found->second.empty()) {
    return false;
  }
  unsigned long long value = 0ULL;
  for (const char character : found->second) {
    if (character < '0' || character > '9') {
      return false;
    }
    value = value * 10ULL + static_cast<unsigned long long>(character - '0');
  }
  return value > 0ULL;
}

bool runProductCase(const std::filesystem::path& binary,
                    std::string_view name,
                    std::string_view controlText,
                    std::string_view extraArgs,
                    std::map<std::string, std::string>& fields,
                    int& exitCode) {
  const std::filesystem::path control =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_automation_" + std::string(name) + ".in");
  const std::filesystem::path output =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_automation_" + std::string(name) + ".out");
  if (!writeControlFile(control, controlText)) {
    return false;
  }
  const std::string command =
      shellQuote(binary) + " --no-window " + std::string(extraArgs) +
      " --automation-control " + shellQuote(control) +
      " --print-render-receipt > " + shellQuote(output);
  exitCode = exitCodeFromSystem(std::system(command.c_str()));
  return parseReceiptFile(output, fields);
}

std::filesystem::path cleanSaveRoot(std::string_view name) {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_automation_" + std::string(name) + "_saves");
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);
  return root;
}

bool productReceipt(const std::map<std::string, std::string>& fields) {
  return hasField(fields, "app", "iggy3d") &&
         !hasField(fields, "app", "iggy3d_visual_demo") &&
         hasField(fields, "result", "pass");
}

bool automationApplied(const std::map<std::string, std::string>& fields) {
  return hasField(fields, "automation_control_requested", "true") &&
         hasField(fields, "automation_control_loaded", "true") &&
         hasField(fields, "automation_control_status", "applied") &&
         hasField(fields, "automation_control_scope", "frontend_menu");
}

}  // namespace

int main() {
#if defined(IGGY3D_PRODUCT_APP_PATH)
  constexpr bool appBuilt = true;
  const std::filesystem::path binary{IGGY3D_PRODUCT_APP_PATH};
#else
  constexpr bool appBuilt = false;
  const std::filesystem::path binary;
#endif
  const bool appAvailable = appBuilt && std::filesystem::exists(binary);

  int exitCode = 77;
  std::map<std::string, std::string> fields;

  const bool starterSettings =
      appAvailable &&
      runProductCase(binary,
                     "starter_settings",
                     "frontend.select=settings\nfrontend.execute=true\nsettings.tab=audio\n",
                     "",
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
      hasField(fields, "frontend_screen", "starter") &&
      hasField(fields, "frontend_child_screen", "settings") &&
      hasField(fields, "frontend_selected_action", "settings") &&
      hasField(fields, "settings_selected_tab", "audio") &&
      hasField(fields, "input_owner", "settings") &&
      hasField(fields, "gameplay_input_suppressed", "true");

  fields.clear();
  const bool starterDevTools =
      appAvailable &&
      runProductCase(binary,
                     "starter_dev_tools",
                     "frontend.select=dev_tools\nfrontend.execute=true\n"
                     "dev_tools.category=input\n",
                     "",
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
      hasField(fields, "frontend_screen", "starter") &&
      hasField(fields, "frontend_child_screen", "starter_dev_tools") &&
      hasField(fields, "dev_tools_open", "true") &&
      hasField(fields, "dev_tools_category", "input") &&
      hasField(fields, "input_owner", "dev_tools") &&
      hasField(fields, "gameplay_input_suppressed", "true");

  fields.clear();
  const std::filesystem::path newWorldSaveRoot = cleanSaveRoot("new_world");
  const bool newWorld =
      appAvailable &&
      runProductCase(binary,
                     "new_world",
                     "frontend.select=new_world\nfrontend.execute=true\n",
                     std::string{"--save-root "} + shellQuote(newWorldSaveRoot),
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
      hasField(fields, "frontend_screen", "gameplay") &&
      hasField(fields, "frontend_selected_action", "create_and_enter") &&
      hasField(fields, "frontend_launch_requested", "true") &&
      hasField(fields, "gameplay_active", "true") &&
      hasField(fields, "world_creation_status",
               "world_creation_initial_save_written") &&
      hasField(fields, "world_creation_reason_code",
               "world_creation_initial_save_written") &&
      hasField(fields, "world_creation_world_id", "world_0001") &&
      hasField(fields, "world_creation_initial_save_requested", "true") &&
      hasField(fields, "world_creation_initial_save_written", "true") &&
      hasField(fields, "world_creation_initial_save_id", "save_001") &&
      hasField(fields, "world_creation_route_after_create", "gameplay") &&
      hasField(fields, "product_save_status", "product_save_written") &&
      hasField(fields, "product_save_reason_code", "product_save_written") &&
      hasField(fields, "product_save_durable_reason", "durable_save_file_written") &&
      hasField(fields, "product_save_source", "initial_world") &&
      hasField(fields, "product_save_save_id", "save_001") &&
      hasField(fields, "product_save_session_saved", "true") &&
      hasField(fields, "active_product_save_id", "save_001") &&
      std::filesystem::exists(newWorldSaveRoot / "save_001.iggy3d.save") &&
      hasField(fields, "product_transition_last_action", "launch_gameplay") &&
      hasField(fields, "product_transition_status", "gameplay_active");

  fields.clear();
  const bool continueLoad =
      appAvailable &&
      runProductCase(binary,
                     "continue_load",
                     "frontend.select=continue\nfrontend.execute=true\n",
                     std::string{"--save-root "} + shellQuote(newWorldSaveRoot),
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
      hasField(fields, "frontend_screen", "gameplay") &&
      hasField(fields, "frontend_selected_action", "continue") &&
      hasField(fields, "frontend_launch_requested", "true") &&
      hasField(fields, "save_count", "1") &&
      hasField(fields, "compatible_save_count", "1") &&
      hasField(fields, "gameplay_active", "true") &&
      hasField(fields, "product_save_load_status", "product_save_loaded") &&
      hasField(fields, "product_save_load_reason_code", "product_save_loaded") &&
      hasField(fields, "product_save_load_save_id", "save_001") &&
      hasField(fields, "product_save_load_source", "continue") &&
      hasField(fields, "product_save_load_selected_id", "save_001") &&
      hasField(fields, "product_save_load_selected_enabled", "true") &&
      hasField(fields, "product_save_load_session_loaded", "true") &&
      positiveIntegerField(fields, "product_save_load_loaded_hash") &&
      hasField(fields, "world_creation_status", "not_requested") &&
      hasField(fields, "product_save_status", "not_requested") &&
      hasField(fields, "active_product_save_id", "save_001") &&
      hasField(fields, "product_transition_last_action", "launch_gameplay") &&
      hasField(fields, "product_transition_status", "gameplay_active");

  fields.clear();
  const bool loadSaveSelector =
      appAvailable &&
      runProductCase(binary,
                     "load_save_selector",
                     "frontend.select=load_save\nfrontend.execute=true\nmenu.confirm=true\n",
                     std::string{"--save-root "} + shellQuote(newWorldSaveRoot),
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
      hasField(fields, "frontend_screen", "gameplay") &&
      hasField(fields, "frontend_selected_action", "load") &&
      hasField(fields, "frontend_launch_requested", "true") &&
      hasField(fields, "save_count", "1") &&
      hasField(fields, "compatible_save_count", "1") &&
      hasField(fields, "gameplay_active", "true") &&
      hasField(fields, "product_save_load_status", "product_save_loaded") &&
      hasField(fields, "product_save_load_reason_code", "product_save_loaded") &&
      hasField(fields, "product_save_load_save_id", "save_001") &&
      hasField(fields, "product_save_load_source", "load_save_selector") &&
      hasField(fields, "product_save_load_selected_id", "save_001") &&
      hasField(fields, "product_save_load_selected_enabled", "true") &&
      hasField(fields, "product_save_load_session_loaded", "true") &&
      positiveIntegerField(fields, "product_save_load_loaded_hash") &&
      hasField(fields, "world_creation_status", "not_requested") &&
      hasField(fields, "product_save_status", "not_requested") &&
      hasField(fields, "active_product_save_id", "save_001") &&
      hasField(fields, "product_transition_last_action", "launch_gameplay") &&
      hasField(fields, "product_transition_status", "gameplay_active");

  std::error_code copyError;
  const bool secondSaveReady = std::filesystem::copy_file(
      newWorldSaveRoot / "save_001.iggy3d.save",
      newWorldSaveRoot / "save_002.iggy3d.save",
      std::filesystem::copy_options::overwrite_existing,
      copyError);

  fields.clear();
  const bool loadSaveSelectorSelected =
      appAvailable && secondSaveReady &&
      runProductCase(binary,
                     "load_save_selector_selected",
                     "frontend.select=load_save\nfrontend.execute=true\n"
                     "save.select=save_002\nmenu.confirm=true\n",
                     std::string{"--save-root "} + shellQuote(newWorldSaveRoot),
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
      hasField(fields, "frontend_screen", "gameplay") &&
      hasField(fields, "frontend_selected_action", "load") &&
      hasField(fields, "frontend_launch_requested", "true") &&
      hasField(fields, "save_count", "2") &&
      hasField(fields, "compatible_save_count", "2") &&
      hasField(fields, "gameplay_active", "true") &&
      hasField(fields, "selected_save_id", "save_002") &&
      hasField(fields, "selected_save_enabled", "true") &&
      hasField(fields, "selected_save_status", "selected") &&
      hasField(fields, "product_save_load_status", "product_save_loaded") &&
      hasField(fields, "product_save_load_reason_code", "product_save_loaded") &&
      hasField(fields, "product_save_load_save_id", "save_002") &&
      hasField(fields, "product_save_load_source", "load_save_selector") &&
      hasField(fields, "product_save_load_selected_id", "save_002") &&
      hasField(fields, "product_save_load_selected_enabled", "true") &&
      hasField(fields, "product_save_load_session_loaded", "true") &&
      positiveIntegerField(fields, "product_save_load_loaded_hash") &&
      hasField(fields, "world_creation_status", "not_requested") &&
      hasField(fields, "product_save_status", "not_requested") &&
      hasField(fields, "active_product_save_id", "save_002") &&
      hasField(fields, "product_transition_last_action", "launch_gameplay") &&
      hasField(fields, "product_transition_status", "gameplay_active");

  const bool corruptSaveReady =
      writeControlFile(newWorldSaveRoot / "corrupt.iggy3d.save",
                       "not_an_iggy3d_save_envelope\n");

  fields.clear();
  const bool loadSaveSelectorCorruptRejected =
      appAvailable && corruptSaveReady &&
      runProductCase(binary,
                     "load_save_selector_corrupt_rejected",
                     "frontend.select=load_save\nfrontend.execute=true\n"
                     "save.select=corrupt\nmenu.confirm=true\n",
                     std::string{"--save-root "} + shellQuote(newWorldSaveRoot),
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
      hasField(fields, "frontend_screen", "starter") &&
      hasField(fields, "frontend_child_screen", "load_save") &&
      hasField(fields, "frontend_selected_action", "load") &&
      hasField(fields, "frontend_launch_requested", "false") &&
      hasField(fields, "save_count", "3") &&
      hasField(fields, "compatible_save_count", "2") &&
      hasField(fields, "gameplay_active", "false") &&
      hasField(fields, "selected_save_id", "corrupt") &&
      hasField(fields, "selected_save_enabled", "false") &&
      hasField(fields, "selected_save_status", "disabled") &&
      hasField(fields, "product_save_load_status", "save_file_decode_failed") &&
      hasField(fields, "product_save_load_reason_code", "save_file_decode_failed") &&
      hasField(fields, "product_save_load_source", "load_save_selector") &&
      hasField(fields, "product_save_load_selected_id", "corrupt") &&
      hasField(fields, "product_save_load_selected_enabled", "false") &&
      hasField(fields, "product_save_load_session_loaded", "false") &&
      hasField(fields, "product_save_load_loaded_hash", "0") &&
      hasField(fields, "world_creation_status", "not_requested") &&
      hasField(fields, "product_save_status", "not_requested");

  fields.clear();
  const bool loadSaveDeleteConfirmOpen =
      appAvailable &&
      runProductCase(binary,
                     "load_save_delete_confirm_open",
                     "frontend.select=load_save\nfrontend.execute=true\n"
                     "save.select=save_001\nsave.delete=true\n",
                     std::string{"--save-root "} + shellQuote(newWorldSaveRoot),
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
      hasField(fields, "frontend_screen", "starter") &&
      hasField(fields, "frontend_child_screen", "delete_confirm") &&
      hasField(fields, "frontend_selected_action", "delete") &&
      hasField(fields, "gameplay_active", "false") &&
      hasField(fields, "selected_save_id", "save_001") &&
      hasField(fields, "selected_save_enabled", "true") &&
      hasField(fields, "selected_save_status", "selected") &&
      hasField(fields, "save_delete_confirmation_open", "true") &&
      hasField(fields, "save_delete_candidate_id", "save_001") &&
      hasField(fields, "save_delete_candidate_enabled", "true") &&
      hasField(fields, "save_delete_status", "confirm_open") &&
      hasField(fields, "save_delete_reason_code", "confirm_open") &&
      hasField(fields, "save_delete_type", "soft") &&
      hasField(fields, "save_delete_recoverable", "false") &&
      hasField(fields, "save_delete_executed", "false") &&
      std::filesystem::exists(newWorldSaveRoot / "save_001.iggy3d.save");

  fields.clear();
  const bool loadSaveDeleteCancel =
      appAvailable &&
      runProductCase(binary,
                     "load_save_delete_cancel",
                     "frontend.select=load_save\nfrontend.execute=true\n"
                     "save.select=save_001\nsave.delete=true\nmenu.back=true\n",
                     std::string{"--save-root "} + shellQuote(newWorldSaveRoot),
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
      hasField(fields, "frontend_screen", "starter") &&
      hasField(fields, "frontend_child_screen", "load_save") &&
      hasField(fields, "frontend_selected_action", "delete") &&
      hasField(fields, "gameplay_active", "false") &&
      hasField(fields, "selected_save_id", "save_001") &&
      hasField(fields, "save_delete_confirmation_open", "false") &&
      hasField(fields, "save_delete_candidate_id", "save_001") &&
      hasField(fields, "save_delete_status", "cancelled") &&
      hasField(fields, "save_delete_reason_code", "cancelled") &&
      hasField(fields, "save_delete_type", "soft") &&
      hasField(fields, "save_delete_recoverable", "false") &&
      hasField(fields, "save_delete_executed", "false") &&
      std::filesystem::exists(newWorldSaveRoot / "save_001.iggy3d.save") &&
      !std::filesystem::exists(newWorldSaveRoot / "deleted" /
                               "save_001.iggy3d.save");

  fields.clear();
  const bool loadSaveDeleteConfirmSoftDeleted =
      appAvailable &&
      runProductCase(binary,
                     "load_save_delete_confirm_soft_deleted",
                     "frontend.select=load_save\nfrontend.execute=true\n"
                     "save.select=save_001\nsave.delete=true\nmenu.confirm=true\n",
                     std::string{"--save-root "} + shellQuote(newWorldSaveRoot),
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
      hasField(fields, "frontend_screen", "starter") &&
      hasField(fields, "frontend_child_screen", "load_save") &&
      hasField(fields, "frontend_selected_action", "delete") &&
      hasField(fields, "gameplay_active", "false") &&
      hasField(fields, "selected_save_id", "save_001") &&
      hasField(fields, "selected_save_enabled", "false") &&
      hasField(fields, "selected_save_status", "missing") &&
      hasField(fields, "save_delete_confirmation_open", "false") &&
      hasField(fields, "save_delete_candidate_id", "save_001") &&
      hasField(fields, "save_delete_status", "product_save_soft_deleted") &&
      hasField(fields, "save_delete_reason_code", "product_save_soft_deleted") &&
      hasField(fields, "save_delete_type", "soft") &&
      hasField(fields, "save_delete_recoverable", "true") &&
      hasField(fields, "save_delete_executed", "true") &&
      !std::filesystem::exists(newWorldSaveRoot / "save_001.iggy3d.save") &&
      std::filesystem::exists(newWorldSaveRoot / "deleted" /
                              "save_001.iggy3d.save");

  const std::filesystem::path recoverSaveRoot =
      cleanSaveRoot("recover_soft_deleted");
  std::error_code recoverSetupError;
  std::filesystem::create_directories(recoverSaveRoot / "deleted",
                                      recoverSetupError);
  recoverSetupError.clear();
  const bool recoverSetup =
      loadSaveDeleteConfirmSoftDeleted &&
      std::filesystem::copy_file(
          newWorldSaveRoot / "deleted" / "save_001.iggy3d.save",
          recoverSaveRoot / "deleted" / "save_001.iggy3d.save",
          std::filesystem::copy_options::overwrite_existing,
          recoverSetupError) &&
      !recoverSetupError;

  fields.clear();
  const bool loadSaveRecoverSoftDeleted =
      appAvailable && recoverSetup &&
      runProductCase(binary,
                     "load_save_recover_soft_deleted",
                     "frontend.select=load_save\nfrontend.execute=true\n"
                     "save.show_deleted=true\nsave.deleted_select=save_001\n"
                     "save.recover=true\n",
                     std::string{"--save-root "} + shellQuote(recoverSaveRoot),
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
      hasField(fields, "frontend_screen", "starter") &&
      hasField(fields, "frontend_child_screen", "load_save") &&
      hasField(fields, "frontend_selected_action", "load_save") &&
      hasField(fields, "gameplay_active", "false") &&
      hasField(fields, "save_count", "1") &&
      hasField(fields, "compatible_save_count", "1") &&
      hasField(fields, "selected_save_id", "save_001") &&
      hasField(fields, "selected_save_enabled", "true") &&
      hasField(fields, "selected_save_status", "selected") &&
      hasField(fields, "deleted_save_browser_open", "false") &&
      hasField(fields, "deleted_save_count", "0") &&
      hasField(fields, "deleted_compatible_save_count", "0") &&
      hasField(fields, "deleted_selected_save_id", "none") &&
      hasField(fields, "deleted_selected_save_enabled", "false") &&
      hasField(fields, "deleted_selected_save_status", "empty") &&
      hasField(fields, "save_recover_status", "product_save_recovered") &&
      hasField(fields, "save_recover_reason_code", "product_save_recovered") &&
      hasField(fields, "save_recover_executed", "true") &&
      hasField(fields, "save_recover_save_id", "save_001") &&
      hasField(fields, "save_recover_snapshot_missing", "true") &&
      std::filesystem::exists(recoverSaveRoot / "save_001.iggy3d.save") &&
      !std::filesystem::exists(recoverSaveRoot / "deleted" /
                               "save_001.iggy3d.save");

  fields.clear();
  const std::filesystem::path pauseSaveActionRoot = cleanSaveRoot("pause_save");
  const bool pauseSave =
      appAvailable &&
      runProductCase(binary,
                     "pause_save",
                     "system.pause=true\npause.select=save\npause.execute=true\n",
                     std::string{"--auto-new-world --save-root "} +
                         shellQuote(pauseSaveActionRoot),
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
      hasField(fields, "frontend_screen", "pause") &&
      hasField(fields, "frontend_selected_action", "save") &&
      hasField(fields, "gameplay_active", "true") &&
      hasField(fields, "pause_menu_open", "true") &&
      hasField(fields, "input_owner", "pause") &&
      hasField(fields, "product_save_status", "product_save_written") &&
      hasField(fields, "product_save_reason_code", "product_save_written") &&
      hasField(fields, "product_save_durable_reason", "durable_save_file_written") &&
      hasField(fields, "product_save_source", "pause_save") &&
      hasField(fields, "product_save_save_id", "save_001") &&
      hasField(fields, "product_save_session_saved", "true") &&
      hasField(fields, "active_product_save_id", "save_001") &&
      std::filesystem::exists(pauseSaveActionRoot / "save_001.iggy3d.save");

  fields.clear();
  const std::filesystem::path saveAndExitRoot = cleanSaveRoot("pause_save_and_exit");
  const bool pauseSaveAndExit =
      appAvailable &&
      runProductCase(binary,
                     "pause_save_and_exit",
                     "system.pause=true\npause.select=save_and_exit\npause.execute=true\n",
                     std::string{"--auto-new-world --save-root "} +
                         shellQuote(saveAndExitRoot),
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
      hasField(fields, "frontend_screen", "starter") &&
      hasField(fields, "gameplay_active", "false") &&
      hasField(fields, "input_owner", "starter") &&
      hasField(fields, "product_save_status", "product_save_written") &&
      hasField(fields, "product_save_reason_code", "product_save_written") &&
      hasField(fields, "product_save_durable_reason", "durable_save_file_written") &&
      hasField(fields, "product_save_source", "pause_save_and_exit") &&
      hasField(fields, "product_save_save_id", "save_001") &&
      hasField(fields, "product_save_session_saved", "true") &&
      hasField(fields, "active_product_save_id", "save_001") &&
      hasField(fields, "product_transition_returned_to_title", "true") &&
      std::filesystem::exists(saveAndExitRoot / "save_001.iggy3d.save");

  fields.clear();
  const std::filesystem::path pauseSaveRoot = cleanSaveRoot("pause_from_gameplay");
  const bool pauseFromGameplay =
      appAvailable &&
      runProductCase(binary,
                     "pause_from_gameplay",
                     "system.pause=true\n",
                     std::string{"--auto-new-world --save-root "} +
                         shellQuote(pauseSaveRoot),
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
      hasField(fields, "frontend_screen", "pause") &&
      hasField(fields, "world_creation_initial_save_written", "true") &&
      std::filesystem::exists(pauseSaveRoot / "save_001.iggy3d.save") &&
      hasField(fields, "pause_menu_open", "true") &&
      hasField(fields, "input_owner", "pause") &&
      hasField(fields, "input_action_last", "system.pause") &&
      hasField(fields, "input_action_accepted", "true") &&
      hasField(fields, "gameplay_input_suppressed", "true");

  fields.clear();
  const std::filesystem::path returnSaveRoot = cleanSaveRoot("return_to_title");
  const bool returnToTitle =
      appAvailable &&
      runProductCase(binary,
                     "return_to_title",
                     "system.pause=true\npause.select=return_to_title\n"
                     "pause.execute=true\n",
                     std::string{"--auto-new-world --save-root "} +
                         shellQuote(returnSaveRoot),
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
      hasField(fields, "frontend_screen", "starter") &&
      hasField(fields, "world_creation_initial_save_written", "true") &&
      std::filesystem::exists(returnSaveRoot / "save_001.iggy3d.save") &&
      hasField(fields, "frontend_return_to_title_requested", "true") &&
      hasField(fields, "product_transition_returned_to_title", "true") &&
      hasField(fields, "gameplay_active", "false");

  fields.clear();
  const bool invalidValue =
      appAvailable &&
      runProductCase(binary,
                     "invalid_value",
                     "menu.input=teleport\n",
                     "",
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) &&
      hasField(fields, "automation_control_requested", "true") &&
      hasField(fields, "automation_control_loaded", "false") &&
      hasField(fields, "automation_control_status", "invalid_value") &&
      hasField(fields, "automation_control_scope", "frontend_menu");

  const bool passed = starterSettings && starterDevTools && newWorld && continueLoad &&
                      loadSaveSelector && loadSaveSelectorSelected &&
                      loadSaveSelectorCorruptRejected && loadSaveDeleteConfirmOpen &&
                      loadSaveDeleteCancel && loadSaveDeleteConfirmSoftDeleted &&
                      loadSaveRecoverSoftDeleted &&
                      pauseSave && pauseSaveAndExit && pauseFromGameplay &&
                      returnToTitle && invalidValue;
  std::cout << "smoke=product_automation_menu\n";
  std::cout << "starter_settings=" << (starterSettings ? "true" : "false") << "\n";
  std::cout << "starter_dev_tools=" << (starterDevTools ? "true" : "false") << "\n";
  std::cout << "new_world=" << (newWorld ? "true" : "false") << "\n";
  std::cout << "continue_load=" << (continueLoad ? "true" : "false") << "\n";
  std::cout << "load_save_selector="
            << (loadSaveSelector ? "true" : "false") << "\n";
  std::cout << "load_save_selector_selected="
            << (loadSaveSelectorSelected ? "true" : "false") << "\n";
  std::cout << "load_save_selector_corrupt_rejected="
            << (loadSaveSelectorCorruptRejected ? "true" : "false") << "\n";
  std::cout << "load_save_delete_confirm_open="
            << (loadSaveDeleteConfirmOpen ? "true" : "false") << "\n";
  std::cout << "load_save_delete_cancel="
            << (loadSaveDeleteCancel ? "true" : "false") << "\n";
  std::cout << "load_save_delete_confirm_soft_deleted="
            << (loadSaveDeleteConfirmSoftDeleted ? "true" : "false") << "\n";
  std::cout << "load_save_recover_soft_deleted="
            << (loadSaveRecoverSoftDeleted ? "true" : "false") << "\n";
  std::cout << "pause_save=" << (pauseSave ? "true" : "false") << "\n";
  std::cout << "pause_save_and_exit="
            << (pauseSaveAndExit ? "true" : "false") << "\n";
  std::cout << "pause_from_gameplay=" << (pauseFromGameplay ? "true" : "false")
            << "\n";
  std::cout << "return_to_title=" << (returnToTitle ? "true" : "false") << "\n";
  std::cout << "invalid_value=" << (invalidValue ? "true" : "false") << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result=" << (passed ? "pass" : (appAvailable ? "fail" : "skip"))
            << "\n";
  std::cout << "reason_code="
            << (passed ? "product_automation_menu_pass"
                       : (appAvailable ? "product_automation_menu_failed"
                                       : "product_app_unavailable"))
            << "\n";
  if (passed) {
    return 0;
  }
  return appAvailable ? 1 : 77;
}
