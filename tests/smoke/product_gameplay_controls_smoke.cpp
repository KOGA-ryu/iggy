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

}  // namespace

int main() {
#if defined(IGGY3D_PRODUCT_APP_PATH)
  constexpr bool appBuilt = true;
  const std::filesystem::path binary{IGGY3D_PRODUCT_APP_PATH};
#else
  constexpr bool appBuilt = false;
  const std::filesystem::path binary;
#endif

  const std::filesystem::path output = "/tmp/iggy3d_product_gameplay_controls.out";
  const std::filesystem::path saveRoot =
      "/tmp/iggy3d_product_gameplay_controls_saves";
  std::filesystem::remove_all(saveRoot);
  std::filesystem::create_directories(saveRoot);
  const std::string command =
      shellQuote(binary) +
      " --no-window --scripted-gameplay-smoke --save-root " +
      shellQuote(saveRoot) + " --print-render-receipt > " +
      shellQuote(output);
  const int exitCode =
      appBuilt && std::filesystem::exists(binary)
          ? exitCodeFromSystem(std::system(command.c_str()))
          : 77;

  std::map<std::string, std::string> fields;
  const bool receiptValid = parseReceiptFile(output, fields);
  const bool passed =
      exitCode == 0 && receiptValid && hasField(fields, "app", "iggy3d") &&
      hasField(fields, "frontend_screen", "gameplay") &&
      hasField(fields, "scripted_gameplay_smoke", "true") &&
      hasField(fields, "world_creation_status",
               "world_creation_initial_save_written") &&
      hasField(fields, "world_creation_reason_code",
               "world_creation_initial_save_written") &&
      hasField(fields, "world_creation_world_id", "world_0001") &&
      hasField(fields, "world_creation_initial_save_requested", "true") &&
      hasField(fields, "world_creation_initial_save_written", "true") &&
      hasField(fields, "world_creation_initial_save_id", "save_001") &&
      hasField(fields, "world_creation_route_after_create", "gameplay") &&
      hasField(fields, "product_save_status", "product_save_written") &&
      hasField(fields, "product_save_reason_code", "product_save_written") &&
      hasField(fields, "product_save_durable_reason",
               "durable_save_file_written") &&
      std::filesystem::exists(saveRoot / "save_001.iggy3d.save") &&
      hasField(fields, "gameplay_input_source", "scripted") &&
      hasField(fields, "gameplay_input_used", "true") &&
      hasField(fields, "gameplay_command_submitted", "false") &&
      hasField(fields, "gameplay_command_kind", "attack") &&
      hasField(fields, "gameplay_command_status", "no_target") &&
      hasField(fields, "gameplay_command_accepted", "false") &&
      hasField(fields, "gameplay_tick_advanced", "false") &&
      hasField(fields, "player_position_changed", "false") &&
      hasField(fields, "physics_movement_planner_enabled", "false") &&
      hasField(fields, "physics_movement_planner_requested", "false") &&
      hasField(fields, "physics_movement_planner_used", "false") &&
      hasField(fields, "physics_movement_planner_status",
               "physics_movement_planner_disabled") &&
      hasField(fields, "physics_movement_planner_reason_code",
               "physics_movement_planner_disabled") &&
      hasField(fields, "mouse_capture_requested", "false") &&
      hasField(fields, "mouse_capture_active", "false") &&
      hasField(fields, "mouse_capture_status", "mouse_capture_not_requested") &&
      hasField(fields, "mouse_capture_reason_code", "mouse_capture_no_window") &&
      hasField(fields, "mouse_capture_mode", "no_window") &&
      hasField(fields, "mouse_capture_input_owner", "gameplay") &&
      hasField(fields, "movement_debug_hud_visible", "false") &&
      hasField(fields, "movement_debug_hud_line_count", "0") &&
      hasField(fields, "movement_debug_hud_dev_tools_enabled", "true") &&
      hasField(fields, "movement_debug_hud_debug_overlay_enabled", "false") &&
      hasField(fields, "movement_debug_hud_status", "not_requested") &&
      hasField(fields, "movement_debug_hud_reason_code", "not_requested") &&
      hasField(fields, "npc_behavior_debug_hud_visible", "false") &&
      hasField(fields, "npc_behavior_debug_hud_status", "not_requested") &&
      hasField(fields, "physics_debug_hud_visible", "false") &&
      hasField(fields, "physics_debug_hud_status", "not_requested") &&
      hasField(fields, "gameplay_movement_profile", "manual_first_person") &&
      hasField(fields, "gameplay_movement_max_speed_mps", "1.600") &&
      hasField(fields, "target_discovered", "false") &&
      hasField(fields, "gameplay_reach_gate", "not_attempted") &&
      hasField(fields, "attack_executed", "false") &&
      hasField(fields, "look_input_used", "true") &&
      hasField(fields, "camera_controller_active", "true") &&
      hasField(fields, "camera_input_source", "scripted") &&
      hasField(fields, "camera_yaw_degrees", "6.000") &&
      hasField(fields, "camera_pitch_degrees", "2.000") &&
      hasField(fields, "product_draw_grid_visible", "true") &&
      hasField(fields, "product_draw_player_visible", "true") &&
      hasField(fields, "product_draw_room_visible", "true") &&
      hasField(fields, "product_draw_objective_visible", "true") &&
      hasField(fields, "product_draw_target_indicator_visible", "true") &&
      positiveIntegerField(fields, "product_draw_item_count") &&
      hasField(fields, "product_draw_debug_marker_count", "0") &&
      hasField(fields, "product_view_projection", "primitive_first_person") &&
      hasField(fields, "product_view_yaw_applied", "true") &&
      hasField(fields, "product_view_pitch_applied", "true") &&
      hasField(fields, "product_view_player_anchor_found", "true") &&
      hasField(fields, "product_render_bridge_ready", "true") &&
      hasField(fields, "product_view_frame_ready", "true") &&
      positiveIntegerField(fields, "product_view_frame_item_count") &&
      positiveIntegerField(fields, "product_view_frame_on_screen_item_count") &&
      positiveIntegerField(fields, "product_view_frame_target_item_count") &&
      hasField(fields, "product_feedback_bridge_ready", "true") &&
      positiveIntegerField(fields, "product_feedback_bridge_line_count") &&
      hasField(fields, "product_feedback_visible", "true") &&
      hasField(fields, "product_feedback_target_status", "no_target") &&
      hasField(fields, "product_feedback_reach_status", "not_attempted") &&
      hasField(fields, "product_feedback_command_kind", "attack") &&
      hasField(fields, "product_feedback_command_status", "no_target") &&
      hasField(fields, "product_feedback_rejection_reason", "none") &&
      hasField(fields, "product_feedback_attack_visible", "true") &&
      hasField(fields, "product_feedback_interaction_visible", "false") &&
      hasField(fields, "product_transition_last_action", "launch_gameplay") &&
      hasField(fields, "product_transition_status", "gameplay_active") &&
      hasField(fields, "product_transition_returned_to_gameplay", "true") &&
      hasField(fields, "product_transition_session_preserved", "true") &&
      hasField(fields, "renderer_mutated_runtime", "false") &&
      positiveIntegerField(fields, "scene_item_count") &&
      positiveIntegerField(fields, "debug_item_count") &&
      hasField(fields, "result", "pass");

  std::cout << "smoke=product_gameplay_controls\n";
  std::cout << "receipt_valid=" << (receiptValid ? "true" : "false") << "\n";
  std::cout << "actual_exit_code=" << exitCode << "\n";
  std::cout << "window_launch_count=0\n";
  std::cout << "result=" << (passed ? "pass" : (appBuilt ? "fail" : "skip")) << "\n";
  std::cout << "reason_code="
            << (passed ? "product_gameplay_controls_pass"
                       : (appBuilt ? "product_gameplay_controls_failed"
                                   : "product_app_unavailable"))
            << "\n";
  if (passed) {
    return 0;
  }
  return appBuilt ? 1 : 77;
}
