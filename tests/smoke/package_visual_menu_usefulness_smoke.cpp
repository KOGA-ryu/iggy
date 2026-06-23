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
                   const std::filesystem::path& output,
                   bool openingMenu) {
  std::string command = shellQuote(binary) + " --package " + shellQuote(fixture) +
                        " --renderer null --interactive --frames 1 --save-root " +
                        shellQuote(saveRoot) + " --codex-control " +
                        shellQuote(control);
  command += openingMenu ? " --opening-menu" : " --no-opening-menu";
  command += " --print-render-receipt > " + shellQuote(output);
  return exitCodeFromSystem(std::system(command.c_str())) == 0;
}

bool runCase(const std::filesystem::path& binary,
             const std::filesystem::path& fixture,
             const std::filesystem::path& saveRoot,
             std::string_view name,
             std::string_view controlText,
             bool openingMenu,
             std::map<std::string, std::string>& fields) {
  const std::filesystem::path control =
      std::filesystem::temp_directory_path() /
      ("iggy3d_menu_usefulness_" + std::string(name) + ".in");
  const std::filesystem::path output =
      std::filesystem::temp_directory_path() /
      ("iggy3d_menu_usefulness_" + std::string(name) + ".out");
  return writeControlFile(control, controlText) &&
         runVisualDemo(binary, fixture, saveRoot, control, output, openingMenu) &&
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

  bool starterRows = false;
  bool pauseRows = false;
  bool settingsRows = false;
  bool devRows = false;

#if defined(IGGY3D_VISUAL_DEMO_PATH)
  const std::filesystem::path binary{IGGY3D_VISUAL_DEMO_PATH};
  const std::filesystem::path fixture =
      std::filesystem::current_path() / "fixtures/demos/movement_playground/package.iggy3d.toml";
  const std::filesystem::path saveRoot =
      std::filesystem::temp_directory_path() / "iggy3d_menu_usefulness_saves";
  std::error_code error;
  std::filesystem::remove_all(saveRoot, error);

  std::map<std::string, std::string> fields;
  starterRows = runCase(binary, fixture, saveRoot, "starter",
                        "frontend.screen=starter\n", true, fields) &&
                hasField(fields, "frontend_screen", "starter") &&
                hasField(fields, "starter_world_suppressed", "true") &&
                hasField(fields, "starter_row_count", "7") &&
                hasField(fields, "starter_selected_action", "continue") &&
                hasField(fields, "starter_selected_enabled", "false") &&
                hasField(fields, "starter_selected_disabled_reason",
                         "no_compatible_save");

  fields.clear();
  pauseRows = runCase(binary, fixture, saveRoot, "pause",
                      "pause.open=true\npause.select=load_save\n", false, fields) &&
              hasField(fields, "frontend_screen", "pause") &&
              hasField(fields, "menu_owner", "pause") &&
              hasField(fields, "pause_row_count", "8") &&
              hasField(fields, "pause_selected_action", "load_save") &&
              hasField(fields, "pause_selected_enabled", "false") &&
              hasField(fields, "pause_selected_disabled_reason", "no_compatible_save") &&
              hasField(fields, "pause_action_command", "pause_load_save") &&
              hasField(fields, "gameplay_input_suppressed", "true");

  fields.clear();
  settingsRows =
      runCase(binary, fixture, saveRoot, "settings",
              "pause.open=true\npause.select=settings\npause.execute=true\n"
              "settings.tab=audio\n",
              false, fields) &&
      hasField(fields, "frontend_screen", "settings") &&
      hasField(fields, "settings_selected_row", "master_volume") &&
      hasField(fields, "settings_selected_enabled", "false") &&
      hasField(fields, "settings_selected_disabled_reason", "audio_unavailable") &&
      hasField(fields, "settings_persistence", "runtime_only");

  fields.clear();
  devRows = runCase(binary, fixture, saveRoot, "dev",
                    "pause.open=true\npause.select=dev_tools\npause.execute=true\n"
                    "dev_tools.category=input\n",
                    false, fields) &&
            hasField(fields, "frontend_screen", "dev_overlay") &&
            hasField(fields, "dev_tools_category", "input") &&
            hasField(fields, "dev_tools_selected_action", "none") &&
            hasField(fields, "dev_tools_selected_enabled", "true") &&
            hasField(fields, "dev_tools_selected_disabled_reason", "none") &&
            hasField(fields, "dev_tools_command_status", "read_only") &&
            hasField(fields, "dev_tools_runtime_readout_count", "4");
#endif

  const bool passed = starterRows && pauseRows && settingsRows && devRows;
  std::cout << "smoke=package_visual_menu_usefulness\n";
  std::cout << "backend=" << (visualBuilt ? "null" : "unavailable") << "\n";
  std::cout << "starter_rows=" << (starterRows ? "true" : "false") << "\n";
  std::cout << "pause_rows=" << (pauseRows ? "true" : "false") << "\n";
  std::cout << "settings_rows=" << (settingsRows ? "true" : "false") << "\n";
  std::cout << "dev_tools_rows=" << (devRows ? "true" : "false") << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result=" << (passed ? "pass" : (visualBuilt ? "fail" : "skip")) << "\n";
  std::cout << "reason_code="
            << (passed ? "visual_menu_usefulness_pass"
                       : (visualBuilt ? "visual_menu_usefulness_failed"
                                      : "visual_demo_unavailable"))
            << "\n";
  if (passed) {
    return 0;
  }
  return visualBuilt ? 1 : 77;
}
