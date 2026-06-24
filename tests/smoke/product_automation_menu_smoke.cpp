#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <string_view>

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
  const bool newWorld =
      appAvailable &&
      runProductCase(binary,
                     "new_world",
                     "frontend.select=new_world\nfrontend.execute=true\n",
                     "",
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
      hasField(fields, "frontend_screen", "gameplay") &&
      hasField(fields, "frontend_selected_action", "create_and_enter") &&
      hasField(fields, "frontend_launch_requested", "true") &&
      hasField(fields, "gameplay_active", "true") &&
      hasField(fields, "product_transition_last_action", "launch_gameplay") &&
      hasField(fields, "product_transition_status", "gameplay_active");

  fields.clear();
  const bool pauseFromGameplay =
      appAvailable &&
      runProductCase(binary,
                     "pause_from_gameplay",
                     "system.pause=true\n",
                     "--auto-new-world",
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
      hasField(fields, "frontend_screen", "pause") &&
      hasField(fields, "pause_menu_open", "true") &&
      hasField(fields, "input_owner", "pause") &&
      hasField(fields, "input_action_last", "system.pause") &&
      hasField(fields, "input_action_accepted", "true") &&
      hasField(fields, "gameplay_input_suppressed", "true");

  fields.clear();
  const bool returnToTitle =
      appAvailable &&
      runProductCase(binary,
                     "return_to_title",
                     "system.pause=true\npause.select=return_to_title\n"
                     "pause.execute=true\n",
                     "--auto-new-world",
                     fields,
                     exitCode) &&
      exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
      hasField(fields, "frontend_screen", "starter") &&
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

  const bool passed = starterSettings && starterDevTools && newWorld &&
                      pauseFromGameplay && returnToTitle && invalidValue;
  std::cout << "smoke=product_automation_menu\n";
  std::cout << "starter_settings=" << (starterSettings ? "true" : "false") << "\n";
  std::cout << "starter_dev_tools=" << (starterDevTools ? "true" : "false") << "\n";
  std::cout << "new_world=" << (newWorld ? "true" : "false") << "\n";
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
