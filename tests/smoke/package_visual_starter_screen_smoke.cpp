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
  const std::string menuFlag = openingMenu ? " --opening-menu " : " --no-opening-menu ";
  const std::string command = shellQuote(binary) + " --package " + shellQuote(fixture) +
                              " --renderer null --interactive --frames 1" + menuFlag +
                              " --save-root " + shellQuote(saveRoot) +
                              " --codex-control " + shellQuote(control) +
                              " --print-render-receipt > " + shellQuote(output);
  return exitCodeFromSystem(std::system(command.c_str())) == 0;
}

}  // namespace

int main() {
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  constexpr bool visualBuilt = true;
#else
  constexpr bool visualBuilt = false;
#endif

  bool starterVisible = false;
  bool continueBlocked = false;
  bool settingsPanel = false;
  bool starterDevTools = false;
  bool newWorldLaunch = false;
  bool pauseSaveExit = false;
  bool devOverlay = false;
  bool returnToTitle = false;

#if defined(IGGY3D_VISUAL_DEMO_PATH)
  const std::filesystem::path binary{IGGY3D_VISUAL_DEMO_PATH};
  const std::filesystem::path fixture =
      std::filesystem::current_path() / "fixtures/demos/movement_playground/package.iggy3d.toml";
  const std::filesystem::path saveRoot =
      std::filesystem::temp_directory_path() / "iggy3d_starter_screen_saves";
  std::error_code error;
  std::filesystem::remove_all(saveRoot, error);

  const std::filesystem::path starterControl = "/tmp/iggy3d_starter_visible.in";
  const std::filesystem::path starterOutput = "/tmp/iggy3d_starter_visible.out";
  const std::filesystem::path continueControl = "/tmp/iggy3d_starter_continue.in";
  const std::filesystem::path continueOutput = "/tmp/iggy3d_starter_continue.out";
  const std::filesystem::path settingsControl = "/tmp/iggy3d_starter_settings.in";
  const std::filesystem::path settingsOutput = "/tmp/iggy3d_starter_settings.out";
  const std::filesystem::path starterDevToolsControl =
      "/tmp/iggy3d_starter_dev_tools.in";
  const std::filesystem::path starterDevToolsOutput =
      "/tmp/iggy3d_starter_dev_tools.out";
  const std::filesystem::path newWorldControl = "/tmp/iggy3d_starter_new_world.in";
  const std::filesystem::path newWorldOutput = "/tmp/iggy3d_starter_new_world.out";
  const std::filesystem::path pauseControl = "/tmp/iggy3d_starter_pause_save_exit.in";
  const std::filesystem::path pauseOutput = "/tmp/iggy3d_starter_pause_save_exit.out";
  const std::filesystem::path devControl = "/tmp/iggy3d_starter_dev_overlay.in";
  const std::filesystem::path devOutput = "/tmp/iggy3d_starter_dev_overlay.out";
  const std::filesystem::path titleControl = "/tmp/iggy3d_starter_return_title.in";
  const std::filesystem::path titleOutput = "/tmp/iggy3d_starter_return_title.out";

  std::map<std::string, std::string> fields;
  starterVisible =
      writeControlFile(starterControl, "frontend.screen=starter\n") &&
      runVisualDemo(binary, fixture, saveRoot, starterControl, starterOutput, true) &&
      parseReceiptFile(starterOutput, fields) && hasField(fields, "result", "pass") &&
      hasField(fields, "frontend_screen", "starter") &&
      hasField(fields, "frontend_selected_action", "continue") &&
      hasField(fields, "frontend_disabled_action", "continue") &&
      hasField(fields, "frontend_input_owned", "true") &&
      hasField(fields, "starter_header_visible", "true") &&
      hasField(fields, "starter_action_list_visible", "true") &&
      hasField(fields, "starter_detail_panel_visible", "true") &&
      hasField(fields, "starter_status_strip_visible", "true") &&
      hasField(fields, "starter_world_suppressed", "true") &&
      hasField(fields, "draw_count", "0") &&
      hasField(fields, "save_count", "0") && hasField(fields, "compatible_save_count", "0");

  continueBlocked =
      writeControlFile(continueControl,
                       "frontend.screen=starter\nfrontend.execute=true\n") &&
      runVisualDemo(binary, fixture, saveRoot, continueControl, continueOutput, true) &&
      parseReceiptFile(continueOutput, fields) && hasField(fields, "result", "pass") &&
      hasField(fields, "frontend_screen", "starter") &&
      hasField(fields, "frontend_selected_action", "continue") &&
      hasField(fields, "frontend_status", "frontend_continue_disabled") &&
      hasField(fields, "frontend_launch_requested", "false") &&
      hasField(fields, "opening_menu_open", "true");

  settingsPanel =
      writeControlFile(settingsControl,
                       "frontend.screen=starter\nfrontend.select=settings\n") &&
      runVisualDemo(binary, fixture, saveRoot, settingsControl, settingsOutput, true) &&
      parseReceiptFile(settingsOutput, fields) && hasField(fields, "result", "pass") &&
      hasField(fields, "frontend_screen", "starter") &&
      hasField(fields, "frontend_selected_action", "settings") &&
      hasField(fields, "frontend_child_screen", "settings") &&
      hasField(fields, "frontend_input_owned", "true");

  starterDevTools =
      writeControlFile(starterDevToolsControl,
                       "frontend.screen=starter\nfrontend.select=dev_tools\n") &&
      runVisualDemo(binary, fixture, saveRoot, starterDevToolsControl,
                    starterDevToolsOutput, true) &&
      parseReceiptFile(starterDevToolsOutput, fields) &&
      hasField(fields, "result", "pass") &&
      hasField(fields, "frontend_screen", "starter") &&
      hasField(fields, "frontend_selected_action", "dev_tools") &&
      hasField(fields, "frontend_child_screen", "starter_dev_tools") &&
      hasField(fields, "frontend_input_owned", "true");

  newWorldLaunch =
      writeControlFile(newWorldControl,
                       "frontend.screen=starter\nfrontend.select=new_world\n"
                       "frontend.execute=true\n") &&
      runVisualDemo(binary, fixture, saveRoot, newWorldControl, newWorldOutput, true) &&
      parseReceiptFile(newWorldOutput, fields) && hasField(fields, "result", "pass") &&
      hasField(fields, "frontend_screen", "gameplay") &&
      hasField(fields, "frontend_selected_action", "create_and_enter") &&
      hasField(fields, "frontend_status", "frontend_launch_new_world") &&
      hasField(fields, "frontend_launch_requested", "true") &&
      hasField(fields, "starter_world_suppressed", "false") &&
      hasField(fields, "opening_menu_created_save", "true") &&
      hasField(fields, "save_count", "1") &&
      std::filesystem::exists(saveRoot / "save_001.iggy3d.save");

  pauseSaveExit =
      writeControlFile(pauseControl,
                       "pause.open=true\npause.select=save_and_exit\n"
                       "pause.execute=true\n") &&
      runVisualDemo(binary, fixture, saveRoot, pauseControl, pauseOutput, false) &&
      parseReceiptFile(pauseOutput, fields) && hasField(fields, "result", "pass") &&
      hasField(fields, "frontend_screen", "pause") &&
      hasField(fields, "frontend_selected_action", "save_and_exit") &&
      hasField(fields, "pause_menu_open", "true") &&
      hasField(fields, "frontend_input_owned", "true") &&
      hasField(fields, "opening_menu_saved_and_exit", "true") &&
      hasField(fields, "opening_menu_exit_requested", "true");

  devOverlay =
      writeControlFile(devControl, "dev_tools.open=true\ndev_tools.category=session\n") &&
      runVisualDemo(binary, fixture, saveRoot, devControl, devOutput, false) &&
      parseReceiptFile(devOutput, fields) && hasField(fields, "result", "pass") &&
      hasField(fields, "frontend_screen", "dev_overlay") &&
      hasField(fields, "dev_tools_open", "true") &&
      hasField(fields, "dev_tools_category", "session") &&
      hasField(fields, "frontend_input_owned", "true");

  returnToTitle =
      writeControlFile(titleControl,
                       "pause.open=true\npause.select=return_to_title\n"
                       "pause.execute=true\n") &&
      runVisualDemo(binary, fixture, saveRoot, titleControl, titleOutput, false) &&
      parseReceiptFile(titleOutput, fields) && hasField(fields, "result", "pass") &&
      hasField(fields, "frontend_screen", "starter") &&
      hasField(fields, "frontend_return_to_title_requested", "true") &&
      hasField(fields, "frontend_status", "frontend_return_to_title") &&
      hasField(fields, "opening_menu_open", "true");
#endif

  const bool passed =
      starterVisible && continueBlocked && settingsPanel && starterDevTools && newWorldLaunch &&
      pauseSaveExit && devOverlay && returnToTitle;
  std::cout << "smoke=package_visual_starter_screen\n";
  std::cout << "backend=" << (visualBuilt ? "null" : "unavailable") << "\n";
  std::cout << "starter_visible=" << (starterVisible ? "true" : "false") << "\n";
  std::cout << "continue_blocked=" << (continueBlocked ? "true" : "false") << "\n";
  std::cout << "settings_panel=" << (settingsPanel ? "true" : "false") << "\n";
  std::cout << "starter_dev_tools=" << (starterDevTools ? "true" : "false") << "\n";
  std::cout << "new_world_launch=" << (newWorldLaunch ? "true" : "false") << "\n";
  std::cout << "pause_save_exit=" << (pauseSaveExit ? "true" : "false") << "\n";
  std::cout << "dev_overlay=" << (devOverlay ? "true" : "false") << "\n";
  std::cout << "return_to_title=" << (returnToTitle ? "true" : "false") << "\n";
  std::cout << "result=" << (passed ? "pass" : "fail") << "\n";
  std::cout << "reason_code="
            << (passed ? "visual_starter_screen_pass"
                       : (visualBuilt ? "visual_starter_screen_failed"
                                      : "visual_demo_unavailable"))
            << "\n";
  return passed ? 0 : 1;
}
