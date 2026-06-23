#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

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

bool numericFieldGreater(const std::map<std::string, std::string>& fields,
                         const std::string& key,
                         float minimum) {
  const auto found = fields.find(key);
  if (found == fields.end()) {
    return false;
  }
  char* end = nullptr;
  const float value = std::strtof(found->second.c_str(), &end);
  return end != found->second.c_str() && *end == '\0' && value > minimum;
}

bool writeCreateAndSaveControlFile(const std::filesystem::path& path) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << "editor.open=true\n";
  output << "debug_overlay.open=true\n";
  output << "editor.add_basic_room=true\n";
  output << "save.current=true\n";
  return static_cast<bool>(output);
}

bool writeLoadControlFile(const std::filesystem::path& path) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << "frontend.select=load_save\n";
  output << "frontend.execute=true\n";
  return static_cast<bool>(output);
}

bool runVisualDemo(const std::filesystem::path& binary,
                   const std::filesystem::path& fixture,
                   const std::filesystem::path& saveRoot,
                   const std::filesystem::path& control,
                   const std::filesystem::path& output,
                   bool openingMenu) {
  std::string command =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer null --interactive --frames 1 --save-root " + shellQuote(saveRoot) +
      " --codex-control " + shellQuote(control);
  command += openingMenu ? " --opening-menu" : " --no-opening-menu";
  command += " --print-render-receipt > " + shellQuote(output);
  return std::system(command.c_str()) == 0;
}

}  // namespace

int main() {
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  constexpr bool visualBuilt = true;
#else
  constexpr bool visualBuilt = false;
#endif
  bool savePassed = false;
  bool loadPassed = false;
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  const std::filesystem::path binary{IGGY3D_VISUAL_DEMO_PATH};
  const std::filesystem::path fixture =
      std::filesystem::current_path() / "fixtures/demos/movement_playground/package.iggy3d.toml";
  const std::filesystem::path saveRoot =
      std::filesystem::temp_directory_path() / "iggy3d_visual_editor_save_load_saves";
  const std::filesystem::path saveControl =
      std::filesystem::temp_directory_path() / "iggy3d_visual_editor_save_control.in";
  const std::filesystem::path saveOutput =
      std::filesystem::temp_directory_path() / "iggy3d_visual_editor_save_control.out";
  const std::filesystem::path loadControl =
      std::filesystem::temp_directory_path() / "iggy3d_visual_editor_load_control.in";
  const std::filesystem::path loadOutput =
      std::filesystem::temp_directory_path() / "iggy3d_visual_editor_load_control.out";

  std::error_code removeError;
  std::filesystem::remove_all(saveRoot, removeError);

  std::map<std::string, std::string> fields;
  savePassed = std::filesystem::exists(binary) && writeCreateAndSaveControlFile(saveControl) &&
               runVisualDemo(binary, fixture, saveRoot, saveControl, saveOutput, false) &&
               parseReceiptFile(saveOutput, fields) && hasField(fields, "result", "pass") &&
               hasField(fields, "backend", "null") &&
               hasField(fields, "editor_authored_room_saved", "true") &&
               hasField(fields, "editor_save_status", "authored_room_saved") &&
               hasField(fields, "editor_save_floor_count", "1") &&
               hasField(fields, "editor_save_wall_count", "1") &&
               hasField(fields, "editor_floor_count", "1") &&
               hasField(fields, "editor_wall_count", "1") &&
               hasField(fields, "editor_runtime_room_rebuilt", "true") &&
               numericFieldGreater(fields, "editor_runtime_surface_count", 0.0F) &&
               numericFieldGreater(fields, "editor_runtime_traversal_slot_count", 0.0F) &&
               hasField(fields, "editor_next_floor_index", "2") &&
               hasField(fields, "editor_next_wall_index", "2") &&
               hasField(fields, "window_launch_count", "0");

  fields.clear();
  loadPassed = savePassed && writeLoadControlFile(loadControl) &&
               runVisualDemo(binary, fixture, saveRoot, loadControl, loadOutput, true) &&
               parseReceiptFile(loadOutput, fields) && hasField(fields, "result", "pass") &&
               hasField(fields, "backend", "null") &&
               hasField(fields, "editor_authored_room_loaded", "true") &&
               hasField(fields, "editor_load_status", "authored_room_loaded") &&
               hasField(fields, "editor_loaded_floor_count", "1") &&
               hasField(fields, "editor_loaded_wall_count", "1") &&
               hasField(fields, "editor_floor_count", "1") &&
               hasField(fields, "editor_wall_count", "1") &&
               hasField(fields, "editor_runtime_room_rebuilt", "true") &&
               numericFieldGreater(fields, "editor_runtime_surface_count", 0.0F) &&
               hasField(fields, "editor_roundtrip_collision_surfaces_restored", "true") &&
               hasField(fields, "editor_roundtrip_traversal_slots_restored", "true") &&
               hasField(fields, "editor_next_floor_index", "2") &&
               hasField(fields, "editor_next_wall_index", "2") &&
               hasField(fields, "editor_selection_reset_after_load", "true") &&
               hasField(fields, "window_launch_count", "0");
#endif

  const bool passed = savePassed && loadPassed;
  std::cout << "smoke=package_visual_editor_save_load\n";
  std::cout << "backend=" << (visualBuilt ? "null" : "unavailable") << "\n";
  std::cout << "editor_authored_room_saved=" << (savePassed ? "true" : "false") << "\n";
  std::cout << "editor_authored_room_loaded=" << (loadPassed ? "true" : "false") << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result=" << (passed ? "pass" : (visualBuilt ? "fail" : "skip")) << "\n";
  std::cout << "reason_code=" << (passed ? "visual_editor_save_load_pass"
                                          : (visualBuilt ? "visual_editor_save_load_failed"
                                                         : "visual_demo_unavailable"))
            << "\n";
  if (passed) {
    return 0;
  }
  return visualBuilt ? 1 : 77;
}
