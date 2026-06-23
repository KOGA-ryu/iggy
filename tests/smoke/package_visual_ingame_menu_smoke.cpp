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

bool writeControlFile(const std::filesystem::path& path, std::string_view content) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << content;
  return static_cast<bool>(output);
}

bool runVisualDemo(const std::filesystem::path& binary,
                   const std::filesystem::path& fixture,
                   const std::filesystem::path& saveRoot,
                   const std::filesystem::path& control,
                   const std::filesystem::path& output) {
  const std::string command = shellQuote(binary) + " --package " + shellQuote(fixture) +
                              " --renderer null --interactive --frames 1 --no-opening-menu"
                              " --save-root " +
                              shellQuote(saveRoot) + " --codex-control " +
                              shellQuote(control) +
                              " --print-render-receipt > " + shellQuote(output);
  return exitCodeFromSystem(std::system(command.c_str())) == 0;
}

bool runCase(const std::filesystem::path& binary,
             const std::filesystem::path& fixture,
             const std::filesystem::path& saveRoot,
             std::string_view name,
             std::string_view controlText,
             std::map<std::string, std::string>& fields) {
  const std::filesystem::path control =
      std::filesystem::temp_directory_path() / ("iggy3d_" + std::string(name) + ".in");
  const std::filesystem::path output =
      std::filesystem::temp_directory_path() / ("iggy3d_" + std::string(name) + ".out");
  return writeControlFile(control, controlText) &&
         runVisualDemo(binary, fixture, saveRoot, control, output) &&
         parseReceiptFile(output, fields) && hasField(fields, "result", "pass") &&
         hasField(fields, "window_launch_count", "0");
}

}  // namespace

int main() {
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  constexpr bool visualBuilt = true;
#else
  constexpr bool visualBuilt = false;
#endif

  bool pauseBlocks = false;
  bool pauseNavigation = false;
  bool pauseSettings = false;
  bool settingsRestore = false;
  bool devTools = false;
  bool returnToTitle = false;
  bool pauseSave = false;
  bool pauseSaveExit = false;

#if defined(IGGY3D_VISUAL_DEMO_PATH)
  const std::filesystem::path binary{IGGY3D_VISUAL_DEMO_PATH};
  const std::filesystem::path fixture =
      std::filesystem::current_path() / "fixtures/demos/movement_playground/package.iggy3d.toml";
  const std::filesystem::path saveRoot =
      std::filesystem::temp_directory_path() / "iggy3d_ingame_menu_saves";
  std::error_code error;
  std::filesystem::remove_all(saveRoot, error);

  std::map<std::string, std::string> fields;

  pauseBlocks =
      runCase(binary, fixture, saveRoot, "ingame_pause_blocks",
              "pause.open=true\nmove.forward=1\nattack=true\n", fields) &&
      hasField(fields, "frontend_screen", "pause") &&
      hasField(fields, "menu_owner", "pause") &&
      hasField(fields, "pause_menu_open", "true") &&
      hasField(fields, "pause_menu_visible", "true") &&
      hasField(fields, "pause_menu_hud_line_count", "11") &&
      hasField(fields, "frontend_input_owned", "true") &&
      hasField(fields, "gameplay_input_suppressed", "true") &&
      hasField(fields, "gamepad_options_opens", "pause") &&
      hasField(fields, "gamepad_create_options_quit", "true");

  pauseNavigation =
      runCase(binary, fixture, saveRoot, "ingame_pause_navigation",
              "pause.open=true\npause.next=true\npause.execute=true\n", fields) &&
      hasField(fields, "frontend_screen", "pause") &&
      hasField(fields, "pause_selected_action", "save") &&
      hasField(fields, "pause_action_executed", "true") &&
      hasField(fields, "opening_menu_saved_current", "true") &&
      hasField(fields, "gameplay_input_suppressed", "true");

  pauseSettings =
      runCase(binary, fixture, saveRoot, "ingame_pause_settings",
              "pause.open=true\npause.select=settings\npause.execute=true\n"
              "settings.tab=controls\nsettings.input_backend=gamepad\nsettings.apply=true\n",
              fields) &&
      hasField(fields, "frontend_screen", "settings") &&
      hasField(fields, "settings_open", "true") &&
      hasField(fields, "settings_parent", "pause") &&
      hasField(fields, "settings_tab", "controls") &&
      hasField(fields, "settings_selected_row", "look_sensitivity") &&
      hasField(fields, "settings_apply_requested", "true") &&
      hasField(fields, "settings_input_backend", "gamepad") &&
      hasField(fields, "menu_owner", "settings") &&
      hasField(fields, "gameplay_input_suppressed", "true");

  settingsRestore =
      runCase(binary, fixture, saveRoot, "ingame_settings_restore",
              "pause.open=true\npause.select=settings\npause.execute=true\n"
              "settings.input_backend=gamepad\nsettings.restore_defaults=true\n",
              fields) &&
      hasField(fields, "frontend_screen", "settings") &&
      hasField(fields, "settings_restore_defaults_requested", "true") &&
      hasField(fields, "settings_input_backend", "keyboard") &&
      hasField(fields, "settings_look_sensitivity", "1.000");

  devTools =
      runCase(binary, fixture, saveRoot, "ingame_dev_tools",
              "pause.open=true\npause.select=dev_tools\npause.execute=true\n"
              "dev_tools.category=renderer\n",
              fields) &&
      hasField(fields, "frontend_screen", "dev_overlay") &&
      hasField(fields, "dev_tools_open", "true") &&
      hasField(fields, "dev_tools_category", "renderer") &&
      hasField(fields, "dev_tools_parent", "pause") &&
      hasField(fields, "dev_tools_input_blocking", "true") &&
      hasField(fields, "dev_tools_readout_visible", "true") &&
      hasField(fields, "gameplay_input_suppressed", "true");

  returnToTitle =
      runCase(binary, fixture, saveRoot, "ingame_return_title",
              "pause.open=true\npause.select=return_to_title\npause.execute=true\n",
              fields) &&
      hasField(fields, "frontend_screen", "starter") &&
      hasField(fields, "frontend_return_to_title_requested", "true") &&
      hasField(fields, "frontend_status", "frontend_return_to_title") &&
      hasField(fields, "menu_owner", "starter");

  pauseSave =
      runCase(binary, fixture, saveRoot, "ingame_pause_save",
              "pause.open=true\npause.select=save\npause.execute=true\n", fields) &&
      hasField(fields, "frontend_screen", "pause") &&
      hasField(fields, "pause_action_executed", "true") &&
      hasField(fields, "opening_menu_saved_current", "true");

  pauseSaveExit =
      runCase(binary, fixture, saveRoot, "ingame_pause_save_exit",
              "pause.open=true\npause.select=save_and_exit\npause.execute=true\n",
              fields) &&
      hasField(fields, "frontend_screen", "pause") &&
      hasField(fields, "pause_selected_action", "save_and_exit") &&
      hasField(fields, "pause_action_executed", "true") &&
      hasField(fields, "opening_menu_saved_and_exit", "true") &&
      hasField(fields, "opening_menu_exit_requested", "true");
#endif

  const bool passed = pauseBlocks && pauseNavigation && pauseSettings && settingsRestore &&
                      devTools && returnToTitle && pauseSave && pauseSaveExit;
  std::cout << "smoke=package_visual_ingame_menu\n";
  std::cout << "backend=" << (visualBuilt ? "null" : "unavailable") << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "pause_blocks_gameplay=" << (pauseBlocks ? "true" : "false") << "\n";
  std::cout << "pause_navigation=" << (pauseNavigation ? "true" : "false") << "\n";
  std::cout << "pause_settings=" << (pauseSettings ? "true" : "false") << "\n";
  std::cout << "settings_restore_defaults=" << (settingsRestore ? "true" : "false")
            << "\n";
  std::cout << "dev_tools_from_pause=" << (devTools ? "true" : "false") << "\n";
  std::cout << "return_to_title=" << (returnToTitle ? "true" : "false") << "\n";
  std::cout << "pause_save=" << (pauseSave ? "true" : "false") << "\n";
  std::cout << "pause_save_and_exit=" << (pauseSaveExit ? "true" : "false") << "\n";
  std::cout << "result=" << (passed ? "pass" : "fail") << "\n";
  std::cout << "reason_code="
            << (passed ? "visual_ingame_menu_pass"
                       : (visualBuilt ? "visual_ingame_menu_failed"
                                      : "visual_demo_unavailable"))
            << "\n";
  return passed ? 0 : 1;
}
