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
              const std::string& key,
              const std::string& value) {
  const auto found = fields.find(key);
  return found != fields.end() && found->second == value;
}

bool numericFieldAtLeast(const std::map<std::string, std::string>& fields,
                         const std::string& key,
                         unsigned long minimum) {
  const auto found = fields.find(key);
  if (found == fields.end()) {
    return false;
  }
  char* end = nullptr;
  const unsigned long value = std::strtoul(found->second.c_str(), &end, 10);
  return end != found->second.c_str() && *end == '\0' && value >= minimum;
}

bool writeControlFile(const std::filesystem::path& path,
                      std::string_view selectedAction,
                      bool execute) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << "opening_menu.open=true\n";
  output << "opening_menu.select=" << selectedAction << "\n";
  if (execute) {
    output << "opening_menu.execute=true\n";
  }
  return static_cast<bool>(output);
}

bool runVisualDemo(const std::filesystem::path& binary,
                   const std::filesystem::path& fixture,
                   const std::filesystem::path& worldRoot,
                   const std::filesystem::path& control,
                   const std::filesystem::path& output) {
  const std::string command =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer null --interactive --frames 1 --opening-menu --world-root " +
      shellQuote(worldRoot) + " --codex-control " + shellQuote(control) +
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
  bool hudPassed = false;
  bool createPassed = false;
  bool deletePassed = false;
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  const std::filesystem::path binary{IGGY3D_VISUAL_DEMO_PATH};
  const std::filesystem::path fixture =
      std::filesystem::current_path() / "fixtures/demos/movement_playground/package.iggy3d.toml";
  const std::filesystem::path worldRoot =
      std::filesystem::temp_directory_path() / "iggy3d_opening_menu_worlds";
  const std::filesystem::path hudControl = "/tmp/iggy3d_opening_menu_hud_control.in";
  const std::filesystem::path hudOutput = "/tmp/iggy3d_opening_menu_hud.out";
  const std::filesystem::path createControl = "/tmp/iggy3d_opening_menu_create_control.in";
  const std::filesystem::path createOutput = "/tmp/iggy3d_opening_menu_create.out";
  const std::filesystem::path deleteControl = "/tmp/iggy3d_opening_menu_delete_control.in";
  const std::filesystem::path deleteOutput = "/tmp/iggy3d_opening_menu_delete.out";
  std::error_code error;
  std::filesystem::remove_all(worldRoot, error);

  std::map<std::string, std::string> fields;
  hudPassed = std::filesystem::exists(binary) &&
              writeControlFile(hudControl, "existing_saves", false) &&
              runVisualDemo(binary, fixture, worldRoot, hudControl, hudOutput) &&
              parseReceiptFile(hudOutput, fields) && hasField(fields, "result", "pass") &&
              hasField(fields, "backend", "null") &&
              hasField(fields, "opening_menu_enabled", "true") &&
              hasField(fields, "opening_menu_open", "true") &&
              hasField(fields, "opening_menu_hud_visible", "true") &&
              numericFieldAtLeast(fields, "opening_menu_hud_line_count", 8UL) &&
              hasField(fields, "opening_menu_selected_action", "existing_saves") &&
              hasField(fields, "opening_menu_world_slot_count", "0") &&
              hasField(fields, "opening_menu_selected_world_id", "none");

  createPassed = writeControlFile(createControl, "new_world", true) &&
                 runVisualDemo(binary, fixture, worldRoot, createControl, createOutput) &&
                 parseReceiptFile(createOutput, fields) &&
                 hasField(fields, "result", "pass") &&
                 hasField(fields, "opening_menu_open", "false") &&
                 hasField(fields, "opening_menu_last_action", "new_world") &&
                 hasField(fields, "opening_menu_status", "world_slot_created") &&
                 hasField(fields, "opening_menu_created_world", "true") &&
                 hasField(fields, "opening_menu_world_slot_count", "1") &&
                 hasField(fields, "opening_menu_selected_world_id", "world_001") &&
                 std::filesystem::exists(worldRoot / "world_001.iggy3d.world.toml");

  deletePassed = writeControlFile(deleteControl, "delete_selected", true) &&
                 runVisualDemo(binary, fixture, worldRoot, deleteControl, deleteOutput) &&
                 parseReceiptFile(deleteOutput, fields) &&
                 hasField(fields, "result", "pass") &&
                 hasField(fields, "opening_menu_last_action", "delete_selected") &&
                 hasField(fields, "opening_menu_status", "world_slot_deleted") &&
                 hasField(fields, "opening_menu_deleted_world", "true") &&
                 hasField(fields, "opening_menu_world_slot_count", "0") &&
                 !std::filesystem::exists(worldRoot / "world_001.iggy3d.world.toml");
#endif

  const bool passed = hudPassed && createPassed && deletePassed;
  std::cout << "smoke=package_visual_opening_menu\n";
  std::cout << "backend=" << (visualBuilt ? "null" : "unavailable") << "\n";
  std::cout << "opening_menu_hud=" << (hudPassed ? "true" : "false") << "\n";
  std::cout << "opening_menu_create=" << (createPassed ? "true" : "false") << "\n";
  std::cout << "opening_menu_delete=" << (deletePassed ? "true" : "false") << "\n";
  std::cout << "result=" << (passed ? "pass" : (visualBuilt ? "fail" : "skip")) << "\n";
  std::cout << "reason_code=" << (passed ? "visual_opening_menu_pass"
                                         : (visualBuilt ? "visual_opening_menu_failed"
                                                        : "visual_demo_unavailable"))
            << "\n";
  if (passed) {
    return 0;
  }
  return visualBuilt ? 1 : 77;
}
