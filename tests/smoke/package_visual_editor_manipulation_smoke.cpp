#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <string_view>

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
              std::string_view key,
              std::string_view value) {
  const auto found = fields.find(std::string(key));
  return found != fields.end() && found->second == value;
}

bool numericFieldGreater(const std::map<std::string, std::string>& fields,
                         std::string_view key,
                         float minimum) {
  const auto found = fields.find(std::string(key));
  if (found == fields.end()) {
    return false;
  }
  char* end = nullptr;
  const float value = std::strtof(found->second.c_str(), &end);
  return end != found->second.c_str() && *end == '\0' && value > minimum;
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
  std::string command =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer null --interactive --frames 1 --save-root " + shellQuote(saveRoot) +
      " --codex-control " + shellQuote(control);
  command += openingMenu ? " --opening-menu" : " --no-opening-menu";
  command += " --print-render-receipt > " + shellQuote(output);
  return std::system(command.c_str()) == 0;
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
      ("iggy3d_visual_editor_manipulation_" + std::string(name) + ".in");
  const std::filesystem::path output =
      std::filesystem::temp_directory_path() /
      ("iggy3d_visual_editor_manipulation_" + std::string(name) + ".out");
  return writeControlFile(control, controlText) &&
         runVisualDemo(binary, fixture, saveRoot, control, output, openingMenu) &&
         parseReceiptFile(output, fields) && hasField(fields, "result", "pass") &&
         hasField(fields, "backend", "null") &&
         hasField(fields, "window_launch_count", "0");
}

}  // namespace

int main() {
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  constexpr bool visualBuilt = true;
#else
  constexpr bool visualBuilt = false;
#endif

  bool floorPassed = false;
  bool wallPassed = false;
  bool loadPassed = false;

#if defined(IGGY3D_VISUAL_DEMO_PATH)
  const std::filesystem::path binary{IGGY3D_VISUAL_DEMO_PATH};
  const std::filesystem::path fixture =
      std::filesystem::current_path() / "fixtures/demos/movement_playground/package.iggy3d.toml";
  const std::filesystem::path floorSaveRoot =
      std::filesystem::temp_directory_path() / "iggy3d_visual_editor_manipulation_floor_saves";
  const std::filesystem::path wallSaveRoot =
      std::filesystem::temp_directory_path() / "iggy3d_visual_editor_manipulation_wall_saves";

  std::error_code error;
  std::filesystem::remove_all(floorSaveRoot, error);
  error.clear();
  std::filesystem::remove_all(wallSaveRoot, error);

  std::map<std::string, std::string> fields;
  floorPassed = std::filesystem::exists(binary) &&
                runCase(binary, fixture, floorSaveRoot, "floor",
                        "editor.open=true\n"
                        "editor.add_basic_room=true\n"
                        "editor.select=edit_floor_1\n"
                        "editor.transform_frames=0\n"
                        "editor.move_selected=1,0,0.5\n"
                        "editor.resize_floor=3,4\n"
                        "save.current=true\n",
                        false, fields) &&
                hasField(fields, "editor_selected_id", "edit_floor_1") &&
                hasField(fields, "editor_selected_type", "floor") &&
                hasField(fields, "editor_selected_center_x", "1.000") &&
                hasField(fields, "editor_selected_center_z", "-0.500") &&
                hasField(fields, "editor_selected_size_x", "3.000") &&
                hasField(fields, "editor_selected_size_z", "4.000") &&
                hasField(fields, "editor_last_transform_command", "resize_floor") &&
                hasField(fields, "editor_last_transform_status", "room_edit_applied") &&
                hasField(fields, "editor_transform_count", "2") &&
                hasField(fields, "editor_runtime_room_rebuilt", "true") &&
                hasField(fields, "editor_authored_room_saved", "true") &&
                hasField(fields, "editor_save_floor_count", "1") &&
                hasField(fields, "editor_save_wall_count", "1") &&
                numericFieldGreater(fields, "editor_runtime_surface_count", 0.0F);

  fields.clear();
  wallPassed = std::filesystem::exists(binary) &&
               runCase(binary, fixture, wallSaveRoot, "wall",
                       "editor.open=true\n"
                       "editor.add_basic_room=true\n"
                       "editor.select=edit_wall_1\n"
                       "editor.transform_frames=0\n"
                       "editor.move_selected=0.5,0,0.25\n"
                       "editor.stretch_wall_end=-4.8152,0,-14.5516\n"
                       "editor.rotate_wall_90=left\n"
                       "editor.set_wall_height=2.25\n"
                       "editor.set_wall_thickness=0.35\n"
                       "save.current=true\n",
                       false, fields) &&
               hasField(fields, "editor_selected_id", "edit_wall_1") &&
               hasField(fields, "editor_selected_type", "wall") &&
               hasField(fields, "editor_selected_wall_height", "2.250") &&
               hasField(fields, "editor_selected_wall_thickness", "0.350") &&
               hasField(fields, "editor_last_transform_command", "set_wall_thickness") &&
               hasField(fields, "editor_last_transform_status", "room_edit_applied") &&
               hasField(fields, "editor_transform_count", "5") &&
               hasField(fields, "editor_runtime_room_rebuilt", "true") &&
               hasField(fields, "editor_authored_room_saved", "true") &&
               numericFieldGreater(fields, "editor_runtime_surface_count", 0.0F) &&
               numericFieldGreater(fields, "editor_runtime_traversal_slot_count", 0.0F);

  fields.clear();
  loadPassed = wallPassed &&
               runCase(binary, fixture, wallSaveRoot, "load",
                       "frontend.select=load_save\n"
                       "frontend.execute=true\n",
                       true, fields) &&
               hasField(fields, "editor_authored_room_loaded", "true") &&
               hasField(fields, "editor_loaded_floor_count", "1") &&
               hasField(fields, "editor_loaded_wall_count", "1") &&
               hasField(fields, "editor_floor_count", "1") &&
               hasField(fields, "editor_wall_count", "1") &&
               hasField(fields, "editor_selected_type", "none") &&
               hasField(fields, "editor_selection_reset_after_load", "true") &&
               hasField(fields, "editor_runtime_room_rebuilt", "true") &&
               hasField(fields, "editor_roundtrip_collision_surfaces_restored", "true") &&
               hasField(fields, "editor_roundtrip_traversal_slots_restored", "true") &&
               hasField(fields, "editor_next_floor_index", "2") &&
               hasField(fields, "editor_next_wall_index", "2");
#endif

  const bool passed = floorPassed && wallPassed && loadPassed;
  std::cout << "smoke=package_visual_editor_manipulation\n";
  std::cout << "backend=" << (visualBuilt ? "null" : "unavailable") << "\n";
  std::cout << "editor_floor_transform_passed=" << (floorPassed ? "true" : "false")
            << "\n";
  std::cout << "editor_wall_transform_passed=" << (wallPassed ? "true" : "false")
            << "\n";
  std::cout << "editor_manipulation_load_passed=" << (loadPassed ? "true" : "false")
            << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result=" << (passed ? "pass" : (visualBuilt ? "fail" : "skip")) << "\n";
  std::cout << "reason_code="
            << (passed ? "visual_editor_manipulation_pass"
                       : (visualBuilt ? "visual_editor_manipulation_failed"
                                      : "visual_demo_unavailable"))
            << "\n";
  if (passed) {
    return 0;
  }
  return visualBuilt ? 1 : 77;
}
