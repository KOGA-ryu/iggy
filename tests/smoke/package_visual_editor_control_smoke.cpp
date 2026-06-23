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

bool writeAddControlFile(const std::filesystem::path& path) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << "editor.open=true\n";
  output << "debug_overlay.open=true\n";
  output << "editor.tool=place_wall\n";
  output << "editor.preset=clamber_wall\n";
  output << "editor.cursor=2,0,-2\n";
  output << "editor.apply_frames=0\n";
  return static_cast<bool>(output);
}

bool writeDeleteControlFile(const std::filesystem::path& path) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << "editor.open=true\n";
  output << "editor.tool=place_wall\n";
  output << "editor.preset=solid_wall\n";
  output << "editor.cursor=1,0,-1\n";
  output << "editor.apply_frames=0\n";
  output << "editor.delete_frames=1\n";
  return static_cast<bool>(output);
}

bool writeProbeControlFile(const std::filesystem::path& path) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << "editor.open=true\n";
  output << "debug_overlay.open=true\n";
  output << "editor.tool=place_wall\n";
  output << "editor.preset=clamber_wall\n";
  output << "editor.apply_frames=0\n";
  output << "look.yaw=0\n";
  output << "look.pitch=-0.5\n";
  return static_cast<bool>(output);
}

bool writeProbeDeleteControlFile(const std::filesystem::path& path) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << "editor.open=true\n";
  output << "debug_overlay.open=true\n";
  output << "editor.tool=place_wall\n";
  output << "editor.preset=clamber_wall\n";
  output << "editor.apply_frames=0\n";
  output << "editor.delete_frames=1\n";
  output << "look.yaw=0\n";
  output << "look.pitch=-0.5\n";
  return static_cast<bool>(output);
}

bool runVisualDemo(const std::filesystem::path& binary,
                   const std::filesystem::path& fixture,
                   const std::filesystem::path& control,
                   const std::filesystem::path& output,
                   std::uint32_t frames) {
  const std::string command =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer null --interactive --frames " + std::to_string(frames) +
      " --codex-control " + shellQuote(control) +
      " --print-render-receipt > " + shellQuote(output);
  return std::system(command.c_str()) == 0;
}

}  // namespace

int main() {
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  constexpr bool visualBuilt = true;
#else
  constexpr bool visualBuilt = false;
#endif
  bool addPassed = false;
  bool deletePassed = false;
  bool probePassed = false;
  bool probeDeletePassed = false;
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  const std::filesystem::path binary{IGGY3D_VISUAL_DEMO_PATH};
  const std::filesystem::path fixture =
      std::filesystem::current_path() / "fixtures/demos/movement_playground/package.iggy3d.toml";
  const std::filesystem::path addControl = "/tmp/iggy3d_visual_editor_add_control.in";
  const std::filesystem::path addOutput = "/tmp/iggy3d_visual_editor_add_control.out";
  const std::filesystem::path deleteControl = "/tmp/iggy3d_visual_editor_delete_control.in";
  const std::filesystem::path deleteOutput = "/tmp/iggy3d_visual_editor_delete_control.out";
  const std::filesystem::path probeControl = "/tmp/iggy3d_visual_editor_probe_control.in";
  const std::filesystem::path probeOutput = "/tmp/iggy3d_visual_editor_probe_control.out";
  const std::filesystem::path probeDeleteControl =
      "/tmp/iggy3d_visual_editor_probe_delete_control.in";
  const std::filesystem::path probeDeleteOutput =
      "/tmp/iggy3d_visual_editor_probe_delete_control.out";

  std::map<std::string, std::string> fields;
  addPassed = std::filesystem::exists(binary) && writeAddControlFile(addControl) &&
              runVisualDemo(binary, fixture, addControl, addOutput, 2U) &&
              parseReceiptFile(addOutput, fields) && hasField(fields, "result", "pass") &&
              hasField(fields, "backend", "null") &&
              hasField(fields, "editor_enabled", "true") &&
              hasField(fields, "editor_open", "true") &&
              hasField(fields, "editor_tool", "place_wall") &&
              hasField(fields, "editor_preset", "clamber_wall") &&
              hasField(fields, "editor_apply_requested", "true") &&
              hasField(fields, "editor_last_command", "add_wall") &&
              hasField(fields, "editor_last_status", "room_edit_applied") &&
              hasField(fields, "editor_selected_id", "edit_wall_1") &&
              hasField(fields, "editor_probe_available", "true") &&
              hasField(fields, "editor_placement_valid", "true") &&
              hasField(fields, "editor_probe_status", "manual_cursor") &&
              hasField(fields, "editor_ghost_visible", "true") &&
              hasField(fields, "editor_ghost_role", "editor_ghost_valid") &&
              hasField(fields, "editor_wall_count", "1") &&
              hasField(fields, "editor_bake_ok", "true") &&
              hasField(fields, "debug_traversal_preview_candidate_available", "true") &&
              hasField(fields, "debug_traversal_preview_target_id", "edit_wall_1") &&
              numericFieldGreater(fields, "editor_runtime_surface_count", 0.0F) &&
              numericFieldGreater(fields, "editor_runtime_traversal_slot_count", 0.0F);

  fields.clear();
  deletePassed = std::filesystem::exists(binary) && writeDeleteControlFile(deleteControl) &&
                 runVisualDemo(binary, fixture, deleteControl, deleteOutput, 3U) &&
                 parseReceiptFile(deleteOutput, fields) && hasField(fields, "result", "pass") &&
                 hasField(fields, "editor_enabled", "true") &&
                 hasField(fields, "editor_open", "true") &&
                 hasField(fields, "editor_apply_requested", "true") &&
                 hasField(fields, "editor_delete_requested", "true") &&
                 hasField(fields, "editor_last_command", "delete") &&
                 hasField(fields, "editor_last_status", "room_edit_applied") &&
                 hasField(fields, "editor_selected_id", "none") &&
                 hasField(fields, "editor_probe_available", "true") &&
                 hasField(fields, "editor_placement_valid", "true") &&
                 hasField(fields, "editor_ghost_visible", "true") &&
                 hasField(fields, "editor_wall_count", "0") &&
                 hasField(fields, "editor_apply_count", "1") &&
                 hasField(fields, "editor_delete_count", "1") &&
                 hasField(fields, "editor_bake_ok", "true");

  fields.clear();
  probePassed = std::filesystem::exists(binary) && writeProbeControlFile(probeControl) &&
                runVisualDemo(binary, fixture, probeControl, probeOutput, 2U) &&
                parseReceiptFile(probeOutput, fields) && hasField(fields, "result", "pass") &&
                hasField(fields, "editor_enabled", "true") &&
                hasField(fields, "editor_open", "true") &&
                hasField(fields, "editor_apply_requested", "true") &&
                hasField(fields, "editor_last_command", "add_wall") &&
                hasField(fields, "editor_last_status", "room_edit_applied") &&
                hasField(fields, "editor_selected_id", "edit_wall_1") &&
                hasField(fields, "editor_probe_available", "true") &&
                hasField(fields, "editor_probe_hit", "true") &&
                hasField(fields, "editor_placement_valid", "true") &&
                hasField(fields, "editor_probe_status", "collision_hit") &&
                hasField(fields, "editor_ghost_visible", "true") &&
                hasField(fields, "editor_ghost_role", "editor_ghost_valid") &&
                hasField(fields, "editor_wall_count", "1") &&
                numericFieldGreater(fields, "editor_probe_distance_meters", 0.0F) &&
                numericFieldGreater(fields, "editor_runtime_surface_count", 0.0F);

  fields.clear();
  probeDeletePassed =
      std::filesystem::exists(binary) && writeProbeDeleteControlFile(probeDeleteControl) &&
      runVisualDemo(binary, fixture, probeDeleteControl, probeDeleteOutput, 3U) &&
      parseReceiptFile(probeDeleteOutput, fields) && hasField(fields, "result", "pass") &&
      hasField(fields, "editor_enabled", "true") &&
      hasField(fields, "editor_open", "true") &&
      hasField(fields, "editor_apply_requested", "true") &&
      hasField(fields, "editor_delete_requested", "true") &&
      hasField(fields, "editor_last_command", "delete") &&
      hasField(fields, "editor_last_status", "room_edit_applied") &&
      hasField(fields, "editor_selected_id", "none") &&
      hasField(fields, "editor_selection_source", "reticle") &&
      hasField(fields, "editor_probe_available", "true") &&
      hasField(fields, "editor_probe_hit", "true") &&
      hasField(fields, "editor_placement_valid", "true") &&
      hasField(fields, "editor_wall_count", "0") &&
      hasField(fields, "editor_apply_count", "1") &&
      hasField(fields, "editor_delete_count", "1");
#endif
  const bool passed = addPassed && deletePassed && probePassed && probeDeletePassed;
  std::cout << "smoke=package_visual_editor_control\n";
  std::cout << "backend=" << (visualBuilt ? "null" : "unavailable") << "\n";
  std::cout << "editor_add_wall_passed=" << (addPassed ? "true" : "false") << "\n";
  std::cout << "editor_delete_wall_passed=" << (deletePassed ? "true" : "false") << "\n";
  std::cout << "editor_probe_wall_passed=" << (probePassed ? "true" : "false") << "\n";
  std::cout << "editor_probe_delete_passed="
            << (probeDeletePassed ? "true" : "false") << "\n";
  std::cout << "result=" << (passed ? "pass" : (visualBuilt ? "fail" : "skip")) << "\n";
  std::cout << "reason_code=" << (passed ? "visual_editor_control_pass"
                                          : (visualBuilt ? "visual_editor_control_failed"
                                                         : "visual_demo_unavailable"))
            << "\n";
  if (passed) {
    return 0;
  }
  return visualBuilt ? 1 : 77;
}
