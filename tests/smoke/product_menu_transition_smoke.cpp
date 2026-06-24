#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

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

bool writeControlFile(const std::filesystem::path& path, const std::string& content) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << content;
  return static_cast<bool>(output);
}

bool hasField(const std::map<std::string, std::string>& fields,
              const std::string& key,
              const std::string& value) {
  const auto found = fields.find(key);
  return found != fields.end() && found->second == value;
}

bool positiveIntegerField(const std::map<std::string, std::string>& fields,
                          const std::string& key) {
  const auto found = fields.find(key);
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

bool runReceiptCommand(const std::filesystem::path& binary,
                       const std::filesystem::path& output,
                       const std::string& arguments,
                       std::map<std::string, std::string>& fields,
                       int& exitCode) {
  const std::string command =
      shellQuote(binary) + " " + arguments + " > " + shellQuote(output);
  exitCode = exitCodeFromSystem(std::system(command.c_str()));
  return parseReceiptFile(output, fields);
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

  const std::filesystem::path saveRoot =
      "/tmp/iggy3d_product_menu_transition_saves";
  std::filesystem::remove_all(saveRoot);
  std::filesystem::create_directories(saveRoot);

  int starterExitCode = 77;
  std::map<std::string, std::string> starterFields;
  const bool starterReceiptValid =
      appBuilt && std::filesystem::exists(binary) &&
      runReceiptCommand(binary,
                        "/tmp/iggy3d_product_menu_transition_starter.out",
                        std::string{"--no-window --save-root "} + shellQuote(saveRoot) +
                            " --print-render-receipt",
                        starterFields,
                        starterExitCode);

  int gameplayExitCode = 77;
  std::map<std::string, std::string> gameplayFields;
  const bool gameplayReceiptValid =
      appBuilt && std::filesystem::exists(binary) &&
      runReceiptCommand(binary,
                        "/tmp/iggy3d_product_menu_transition_gameplay.out",
                        std::string{"--no-window --auto-new-world --save-root "} +
                            shellQuote(saveRoot) + " --print-render-receipt",
                        gameplayFields,
                        gameplayExitCode);

  int savedStarterExitCode = 77;
  std::map<std::string, std::string> savedStarterFields;
  const bool savedStarterReceiptValid =
      appBuilt && std::filesystem::exists(binary) &&
      runReceiptCommand(binary,
                        "/tmp/iggy3d_product_menu_transition_saved_starter.out",
                        std::string{"--no-window --save-root "} + shellQuote(saveRoot) +
                            " --print-render-receipt",
                        savedStarterFields,
                        savedStarterExitCode);

  const std::filesystem::path continueControl =
      "/tmp/iggy3d_product_menu_transition_continue.in";
  const bool continueControlWritten = writeControlFile(
      continueControl, "frontend.select=continue\nfrontend.execute=true\n");
  int continueExitCode = 77;
  std::map<std::string, std::string> continueFields;
  const bool continueReceiptValid =
      continueControlWritten && appBuilt && std::filesystem::exists(binary) &&
      runReceiptCommand(binary,
                        "/tmp/iggy3d_product_menu_transition_continue.out",
                        std::string{"--no-window --save-root "} + shellQuote(saveRoot) +
                            " --automation-control " + shellQuote(continueControl) +
                            " --print-render-receipt",
                        continueFields,
                        continueExitCode);

  const bool starterPassed =
      starterExitCode == 0 && starterReceiptValid &&
      hasField(starterFields, "app", "iggy3d") &&
      hasField(starterFields, "frontend_screen", "starter") &&
      hasField(starterFields, "starter_world_suppressed", "true") &&
      hasField(starterFields, "gameplay_active", "false") &&
      hasField(starterFields, "gameplay_view_visible", "false") &&
      hasField(starterFields, "input_owner", "starter") &&
      hasField(starterFields, "gameplay_input_suppressed", "true") &&
      hasField(starterFields, "world_creation_status", "not_requested") &&
      hasField(starterFields, "world_creation_initial_save_requested", "false") &&
      hasField(starterFields, "world_creation_initial_save_written", "false") &&
      hasField(starterFields, "world_creation_initial_save_id", "none") &&
      hasField(starterFields, "product_save_status", "not_requested") &&
      hasField(starterFields, "product_save_load_status", "not_requested") &&
      hasField(starterFields, "product_save_load_session_loaded", "false") &&
      hasField(starterFields, "product_transition_last_action", "startup") &&
      hasField(starterFields, "product_transition_status", "starter_ready") &&
      hasField(starterFields, "product_render_bridge_ready", "false") &&
      hasField(starterFields, "product_view_frame_ready", "false") &&
      hasField(starterFields, "product_view_frame_item_count", "0") &&
      hasField(starterFields, "product_feedback_bridge_ready", "false");

  const bool gameplayPassed =
      gameplayExitCode == 0 && gameplayReceiptValid &&
      hasField(gameplayFields, "app", "iggy3d") &&
      hasField(gameplayFields, "frontend_screen", "gameplay") &&
      hasField(gameplayFields, "gameplay_active", "true") &&
      hasField(gameplayFields, "input_owner", "gameplay") &&
      hasField(gameplayFields, "gameplay_input_suppressed", "false") &&
      hasField(gameplayFields, "world_creation_status",
               "world_creation_initial_save_written") &&
      hasField(gameplayFields, "world_creation_reason_code",
               "world_creation_initial_save_written") &&
      hasField(gameplayFields, "world_creation_world_id", "world_0001") &&
      hasField(gameplayFields, "world_creation_initial_save_requested", "true") &&
      hasField(gameplayFields, "world_creation_initial_save_written", "true") &&
      hasField(gameplayFields, "world_creation_initial_save_id", "save_001") &&
      hasField(gameplayFields, "world_creation_route_after_create", "gameplay") &&
      hasField(gameplayFields, "product_save_status", "product_save_written") &&
      hasField(gameplayFields, "product_save_reason_code", "product_save_written") &&
      hasField(gameplayFields, "product_save_durable_reason",
               "durable_save_file_written") &&
      hasField(gameplayFields, "product_save_load_status", "not_requested") &&
      hasField(gameplayFields, "product_save_load_session_loaded", "false") &&
      std::filesystem::exists(saveRoot / "save_001.iggy3d.save") &&
      hasField(gameplayFields, "product_transition_last_action", "launch_gameplay") &&
      hasField(gameplayFields, "product_transition_status", "gameplay_active") &&
      hasField(gameplayFields, "product_transition_returned_to_gameplay", "true") &&
      hasField(gameplayFields, "product_transition_session_preserved", "true") &&
      hasField(gameplayFields, "product_draw_grid_visible", "true") &&
      hasField(gameplayFields, "product_view_projection", "primitive_first_person") &&
      positiveIntegerField(gameplayFields, "product_draw_item_count") &&
      hasField(gameplayFields, "product_render_bridge_ready", "true") &&
      hasField(gameplayFields, "product_view_frame_ready", "true") &&
      positiveIntegerField(gameplayFields, "product_view_frame_item_count") &&
      positiveIntegerField(gameplayFields, "product_view_frame_on_screen_item_count") &&
      positiveIntegerField(gameplayFields, "product_view_frame_target_item_count") &&
      hasField(gameplayFields, "product_feedback_bridge_ready", "true") &&
      positiveIntegerField(gameplayFields, "product_feedback_bridge_line_count");

  const bool savedStarterPassed =
      savedStarterExitCode == 0 && savedStarterReceiptValid &&
      hasField(savedStarterFields, "frontend_screen", "starter") &&
      hasField(savedStarterFields, "save_count", "1") &&
      hasField(savedStarterFields, "compatible_save_count", "1") &&
      hasField(savedStarterFields, "world_creation_status", "not_requested") &&
      hasField(savedStarterFields, "product_save_status", "not_requested") &&
      hasField(savedStarterFields, "product_save_load_status", "not_requested") &&
      hasField(savedStarterFields, "product_save_load_session_loaded", "false") &&
      hasField(savedStarterFields, "gameplay_active", "false");

  const bool continuePassed =
      continueExitCode == 0 && continueReceiptValid &&
      hasField(continueFields, "frontend_screen", "gameplay") &&
      hasField(continueFields, "frontend_selected_action", "continue") &&
      hasField(continueFields, "frontend_launch_requested", "true") &&
      hasField(continueFields, "save_count", "1") &&
      hasField(continueFields, "compatible_save_count", "1") &&
      hasField(continueFields, "gameplay_active", "true") &&
      hasField(continueFields, "input_owner", "gameplay") &&
      hasField(continueFields, "product_save_load_status", "product_save_loaded") &&
      hasField(continueFields, "product_save_load_reason_code", "product_save_loaded") &&
      hasField(continueFields, "product_save_load_save_id", "save_001") &&
      hasField(continueFields, "product_save_load_session_loaded", "true") &&
      positiveIntegerField(continueFields, "product_save_load_loaded_hash") &&
      hasField(continueFields, "world_creation_status", "not_requested") &&
      hasField(continueFields, "product_save_status", "not_requested") &&
      hasField(continueFields, "product_transition_last_action", "launch_gameplay") &&
      hasField(continueFields, "product_transition_status", "gameplay_active") &&
      hasField(continueFields, "product_render_bridge_ready", "true") &&
      hasField(continueFields, "product_view_frame_ready", "true") &&
      positiveIntegerField(continueFields, "product_draw_item_count");

  const bool passed =
      starterPassed && gameplayPassed && savedStarterPassed && continuePassed;

  std::cout << "smoke=product_menu_transition\n";
  std::cout << "starter_receipt_valid="
            << (starterReceiptValid ? "true" : "false") << "\n";
  std::cout << "gameplay_receipt_valid="
            << (gameplayReceiptValid ? "true" : "false") << "\n";
  std::cout << "saved_starter_receipt_valid="
            << (savedStarterReceiptValid ? "true" : "false") << "\n";
  std::cout << "continue_receipt_valid="
            << (continueReceiptValid ? "true" : "false") << "\n";
  std::cout << "starter_exit_code=" << starterExitCode << "\n";
  std::cout << "gameplay_exit_code=" << gameplayExitCode << "\n";
  std::cout << "saved_starter_exit_code=" << savedStarterExitCode << "\n";
  std::cout << "continue_exit_code=" << continueExitCode << "\n";
  std::cout << "starter_passed=" << (starterPassed ? "true" : "false") << "\n";
  std::cout << "gameplay_passed=" << (gameplayPassed ? "true" : "false") << "\n";
  std::cout << "saved_starter_passed="
            << (savedStarterPassed ? "true" : "false") << "\n";
  std::cout << "continue_passed=" << (continuePassed ? "true" : "false") << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result=" << (passed ? "pass" : (appBuilt ? "fail" : "skip")) << "\n";
  std::cout << "reason_code="
            << (passed ? "product_menu_transition_pass"
                       : (appBuilt ? "product_menu_transition_failed"
                                   : "product_app_unavailable"))
            << "\n";
  if (passed) {
    return 0;
  }
  return appBuilt ? 1 : 77;
}
