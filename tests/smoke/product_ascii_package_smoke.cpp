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

bool expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

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

bool asciiPackageSelectionFields(const std::map<std::string, std::string>& fields) {
  return hasField(fields, "selected_package_id", "iggy3d.ascii_training_room") &&
         hasField(fields, "selected_scenario_id", "ascii_training_room.runtime_loop");
}

bool asciiPackageLoadedFields(const std::map<std::string, std::string>& fields) {
  return hasField(fields, "package_load_status", "ok") &&
         asciiPackageSelectionFields(fields);
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

  const std::filesystem::path packagePath =
      "fixtures/demos/ascii_training_room/package.iggy3d.toml";
  const std::filesystem::path saveRoot =
      "/tmp/iggy3d_product_ascii_package_saves";
  const std::filesystem::path scriptedSaveRoot =
      "/tmp/iggy3d_product_ascii_package_scripted_saves";
  std::filesystem::remove_all(saveRoot);
  std::filesystem::remove_all(scriptedSaveRoot);
  std::filesystem::create_directories(saveRoot);
  std::filesystem::create_directories(scriptedSaveRoot);

  const std::string packageArg = " --package " + shellQuote(packagePath);
  const std::string saveRootArg = " --save-root " + shellQuote(saveRoot);
  const std::string scriptedSaveRootArg =
      " --save-root " + shellQuote(scriptedSaveRoot);

  int newWorldExitCode = 77;
  std::map<std::string, std::string> newWorldFields;
  const bool newWorldReceiptValid =
      appBuilt && std::filesystem::exists(binary) &&
      runReceiptCommand(binary,
                        "/tmp/iggy3d_product_ascii_package_new_world.out",
                        std::string{"--no-window"} + packageArg +
                            " --auto-new-world" + saveRootArg +
                            " --print-render-receipt",
                        newWorldFields,
                        newWorldExitCode);

  int starterExitCode = 77;
  std::map<std::string, std::string> starterFields;
  const bool starterReceiptValid =
      appBuilt && std::filesystem::exists(binary) &&
      runReceiptCommand(binary,
                        "/tmp/iggy3d_product_ascii_package_starter.out",
                        std::string{"--no-window"} + packageArg + saveRootArg +
                            " --print-render-receipt",
                        starterFields,
                        starterExitCode);

  const std::filesystem::path continueControl =
      "/tmp/iggy3d_product_ascii_package_continue.in";
  const bool continueControlWritten =
      writeControlFile(continueControl, "frontend.select=continue\nfrontend.execute=true\n");
  int continueExitCode = 77;
  std::map<std::string, std::string> continueFields;
  const bool continueReceiptValid =
      continueControlWritten && appBuilt && std::filesystem::exists(binary) &&
      runReceiptCommand(binary,
                        "/tmp/iggy3d_product_ascii_package_continue.out",
                        std::string{"--no-window"} + packageArg + saveRootArg +
                            " --automation-control " + shellQuote(continueControl) +
                            " --print-render-receipt",
                        continueFields,
                        continueExitCode);

  int scriptedExitCode = 77;
  std::map<std::string, std::string> scriptedFields;
  const bool scriptedReceiptValid =
      appBuilt && std::filesystem::exists(binary) &&
      runReceiptCommand(binary,
                        "/tmp/iggy3d_product_ascii_package_scripted.out",
                        std::string{"--no-window"} + packageArg +
                            " --scripted-gameplay-smoke" + scriptedSaveRootArg +
                            " --print-render-receipt",
                        scriptedFields,
                        scriptedExitCode);

  const std::filesystem::path saveFile = saveRoot / "save_001.iggy3d.save";
  const bool newWorldPassed =
      newWorldExitCode == 0 && newWorldReceiptValid &&
      hasField(newWorldFields, "app", "iggy3d") &&
      hasField(newWorldFields, "result", "pass") &&
      hasField(newWorldFields, "window_mode", "no_window") &&
      hasField(newWorldFields, "window_created", "false") &&
      asciiPackageLoadedFields(newWorldFields) &&
      hasField(newWorldFields, "frontend_screen", "gameplay") &&
      hasField(newWorldFields, "runtime_session_created", "true") &&
      hasField(newWorldFields, "gameplay_active", "true") &&
      hasField(newWorldFields, "world_creation_status",
               "world_creation_initial_save_written") &&
      hasField(newWorldFields, "world_creation_initial_save_written", "true") &&
      hasField(newWorldFields, "product_save_status", "product_save_written") &&
      hasField(newWorldFields, "active_product_save_id", "save_001") &&
      std::filesystem::exists(saveFile);

  const bool starterPassed =
      starterExitCode == 0 && starterReceiptValid &&
      hasField(starterFields, "frontend_screen", "starter") &&
      hasField(starterFields, "save_count", "1") &&
      hasField(starterFields, "compatible_save_count", "1") &&
      asciiPackageSelectionFields(starterFields);

  const bool continuePassed =
      continueExitCode == 0 && continueReceiptValid &&
      hasField(continueFields, "frontend_screen", "gameplay") &&
      hasField(continueFields, "gameplay_active", "true") &&
      hasField(continueFields, "product_save_load_status", "product_save_loaded") &&
      hasField(continueFields, "product_save_load_source", "continue") &&
      hasField(continueFields, "product_save_load_session_loaded", "true") &&
      hasField(continueFields, "active_product_save_id", "save_001") &&
      asciiPackageLoadedFields(continueFields);

  const bool scriptedPassed =
      scriptedExitCode == 0 && scriptedReceiptValid &&
      hasField(scriptedFields, "app", "iggy3d") &&
      hasField(scriptedFields, "result", "pass") &&
      hasField(scriptedFields, "window_mode", "no_window") &&
      hasField(scriptedFields, "window_created", "false") &&
      asciiPackageLoadedFields(scriptedFields) &&
      hasField(scriptedFields, "frontend_screen", "gameplay") &&
      hasField(scriptedFields, "runtime_session_created", "true") &&
      hasField(scriptedFields, "gameplay_active", "true") &&
      hasField(scriptedFields, "scripted_gameplay_smoke", "true") &&
      hasField(scriptedFields, "scene_item_count", "5") &&
      hasField(scriptedFields, "player_visible", "true") &&
      hasField(scriptedFields, "objective_visible", "true") &&
      hasField(scriptedFields, "product_draw_item_count", "6") &&
      hasField(scriptedFields, "product_render_bridge_ready", "true") &&
      hasField(scriptedFields, "target_discovered", "true") &&
      hasField(scriptedFields, "gameplay_command_kind", "attack") &&
      hasField(scriptedFields, "gameplay_command_status", "accepted") &&
      hasField(scriptedFields, "gameplay_command_accepted", "true") &&
      hasField(scriptedFields, "gameplay_reach_gate", "pass") &&
      hasField(scriptedFields, "gameplay_last_rejection", "none") &&
      hasField(scriptedFields, "attack_executed", "true") &&
      hasField(scriptedFields, "product_feedback_visible", "true") &&
      hasField(scriptedFields, "product_feedback_command_kind", "attack") &&
      hasField(scriptedFields, "product_feedback_command_status", "accepted") &&
      hasField(scriptedFields, "product_feedback_rejection_reason", "none") &&
      hasField(scriptedFields, "product_feedback_attack_visible", "true");

  const bool ok = expect(appBuilt, "app target available") &&
                  expect(std::filesystem::exists(binary), "app binary exists") &&
                  expect(newWorldReceiptValid, "new world receipt valid and unique") &&
                  expect(newWorldPassed, "new world ascii package pass") &&
                  expect(starterReceiptValid, "starter receipt valid and unique") &&
                  expect(starterPassed, "starter scan ascii package pass") &&
                  expect(continueControlWritten, "continue control written") &&
                  expect(continueReceiptValid, "continue receipt valid and unique") &&
                  expect(continuePassed, "continue ascii package pass") &&
                  expect(scriptedReceiptValid, "scripted receipt valid and unique") &&
                  expect(scriptedPassed, "scripted ascii package pass");
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
