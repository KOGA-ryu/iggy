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

  const bool starterPassed =
      starterExitCode == 0 && starterReceiptValid &&
      hasField(starterFields, "app", "iggy3d") &&
      hasField(starterFields, "frontend_screen", "starter") &&
      hasField(starterFields, "starter_world_suppressed", "true") &&
      hasField(starterFields, "gameplay_active", "false") &&
      hasField(starterFields, "gameplay_view_visible", "false") &&
      hasField(starterFields, "input_owner", "starter") &&
      hasField(starterFields, "gameplay_input_suppressed", "true") &&
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

  const bool passed = starterPassed && gameplayPassed;

  std::cout << "smoke=product_menu_transition\n";
  std::cout << "starter_receipt_valid="
            << (starterReceiptValid ? "true" : "false") << "\n";
  std::cout << "gameplay_receipt_valid="
            << (gameplayReceiptValid ? "true" : "false") << "\n";
  std::cout << "starter_exit_code=" << starterExitCode << "\n";
  std::cout << "gameplay_exit_code=" << gameplayExitCode << "\n";
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
