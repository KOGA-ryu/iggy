#include "AutomationSmokeSupport.hpp"

#include "app/iggy3d/gameplay/MovementTuning.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
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

std::string fixed3(float value) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(3) << value;
  return out.str();
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

  const iggy3d::ProductGameplayMovementTuning& tuning =
      iggy3d::productGameplayMovementTuning();
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
  const bool scriptedControlsPassed =
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
      hasField(fields, "position_hud_visible", "false") &&
      hasField(fields, "position_hud_status", "not_requested") &&
      hasField(fields,
               "gameplay_movement_profile",
               std::string{tuning.walkProfile}) &&
      hasField(fields,
               "gameplay_movement_max_speed_mps",
               fixed3(tuning.walkSpeedMetersPerSecond)) &&
      hasField(fields,
               "gameplay_movement_tuning_ground_acceleration_mps2",
               fixed3(tuning.groundAccelerationMetersPerSecondSquared)) &&
      hasField(fields,
               "gameplay_movement_tuning_ground_deceleration_mps2",
               fixed3(tuning.groundDecelerationMetersPerSecondSquared)) &&
      hasField(fields,
               "gameplay_movement_tuning_air_control",
               fixed3(tuning.airControlMultiplier)) &&
      hasField(fields,
               "gameplay_movement_tuning_coyote_time_s",
               fixed3(tuning.coyoteTimeSeconds)) &&
      hasField(fields,
               "gameplay_movement_tuning_jump_buffer_s",
               fixed3(tuning.jumpBufferSeconds)) &&
      hasField(fields,
               "gameplay_movement_tuning_jump_cut_multiplier",
               fixed3(tuning.jumpCutMultiplier)) &&
      hasField(fields,
               "gameplay_movement_tuning_fall_gravity_multiplier",
               fixed3(tuning.fallGravityMultiplier)) &&
      hasField(fields,
               "gameplay_movement_tuning_look_sensitivity",
               fixed3(tuning.lookSensitivity)) &&
      hasField(fields,
               "gameplay_movement_tuning_invert_look",
               "false") &&
      hasField(fields,
               "gameplay_movement_tuning_wall_run_min_speed_mps",
               fixed3(tuning.wallRunMinSpeedMetersPerSecond)) &&
      hasField(fields,
               "gameplay_movement_tuning_wall_run_max_normal_y",
               fixed3(tuning.wallRunMaxWallNormalY)) &&
      hasField(fields, "target_discovered", "false") &&
      hasField(fields, "gameplay_reach_gate", "not_attempted") &&
      hasField(fields, "attack_executed", "false") &&
      hasField(fields, "look_input_used", "true") &&
      hasField(fields, "camera_controller_active", "true") &&
      hasField(fields, "camera_input_source", "scripted") &&
      hasField(fields, "camera_yaw_degrees", "6.000") &&
      hasField(fields, "camera_pitch_degrees", "-2.000") &&
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

  const std::filesystem::path clamberSaveRoot =
      iggy3d::smoke::cleanSaveRoot("gameplay_controls_movement_gym_clamber");
  iggy3d::smoke::ReceiptFields clamberFields;
  int clamberExitCode = 77;
  const bool clamberControlsPassed =
      appBuilt && std::filesystem::exists(binary) &&
      iggy3d::smoke::runProductCase(
          binary,
          "gameplay_controls_movement_gym_clamber",
          "frontend.select=new_world\n"
          "frontend.execute=true\n"
          "world.dungeon_id=movement_gym\n"
          "world.create=true\n"
          "gameplay.player_position=250,0,51.2\n"
          "gameplay.jump=true\n",
          iggy3d::smoke::saveRootArg(clamberSaveRoot),
          clamberFields,
          clamberExitCode) &&
      clamberExitCode == 0 && iggy3d::smoke::productReceipt(clamberFields) &&
      iggy3d::smoke::automationApplied(clamberFields) &&
      iggy3d::smoke::hasField(clamberFields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(clamberFields, "window_created", "false") &&
      iggy3d::smoke::hasField(clamberFields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(clamberFields, "active_room_id",
                              "movement_gym") &&
      iggy3d::smoke::hasField(clamberFields, "automation_control_last_key",
                              "gameplay.jump") &&
      iggy3d::smoke::hasField(clamberFields, "automation_control_last_action",
                              "game.jump") &&
      iggy3d::smoke::hasField(clamberFields, "automation_control_last_owner",
                              "gameplay") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_input_source",
                              "automation") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_input_used", "true") &&
      iggy3d::smoke::hasField(clamberFields, "input_owner", "gameplay") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_input_suppressed",
                              "false") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_jump_requested",
                              "true") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_jump_accepted",
                              "false") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_jump_active",
                              "false") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_jump_status",
                              "traversal") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_jump_reason_code",
                              "traversal_intent_applied") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_traversal_requested",
                              "true") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_traversal_consumed",
                              "true") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_traversal_accepted",
                              "true") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_traversal_mechanic",
                              "clamber") &&
      iggy3d::smoke::hasField(
          clamberFields,
          "gameplay_traversal_slot_id",
          "object_clamber_ledge_r5_c25:object_clamber_ledge_r5_c25_walkable_top") &&
      iggy3d::smoke::hasField(clamberFields,
                              "gameplay_traversal_target_id",
                              "object_clamber_ledge_r5_c25") &&
      iggy3d::smoke::hasField(
          clamberFields,
          "gameplay_traversal_landing_surface_id",
          "object_clamber_ledge_r5_c25_walkable_top") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_traversal_start_x",
                              "250.000") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_traversal_start_y",
                              "0.000") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_traversal_start_z",
                              "51.200") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_traversal_final_x",
                              "250.000") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_traversal_final_y",
                              "1.700") &&
      iggy3d::smoke::hasField(clamberFields, "gameplay_traversal_final_z",
                              "50.000") &&
      iggy3d::smoke::hasField(clamberFields, "player_position_changed",
                              "true") &&
      iggy3d::smoke::hasField(clamberFields, "product_draw_prop_visible",
                              "true") &&
      iggy3d::smoke::hasField(clamberFields, "product_draw_prop_tile_count",
                              "10");

  const std::filesystem::path wallJumpSaveRoot =
      iggy3d::smoke::cleanSaveRoot("gameplay_controls_movement_gym_wall_jump");
  iggy3d::smoke::ReceiptFields wallJumpFields;
  int wallJumpExitCode = 77;
  const bool wallJumpControlsPassed =
      appBuilt && std::filesystem::exists(binary) &&
      iggy3d::smoke::runProductCase(
          binary,
          "gameplay_controls_movement_gym_wall_jump",
          "frontend.select=new_world\n"
          "frontend.execute=true\n"
          "world.dungeon_id=movement_gym\n"
          "world.create=true\n"
          "gameplay.player_position=310,0.8,19.65\n"
          "gameplay.jump=true\n",
          iggy3d::smoke::saveRootArg(wallJumpSaveRoot),
          wallJumpFields,
          wallJumpExitCode) &&
      wallJumpExitCode == 0 && iggy3d::smoke::productReceipt(wallJumpFields) &&
      iggy3d::smoke::automationApplied(wallJumpFields) &&
      iggy3d::smoke::hasField(wallJumpFields, "window_mode", "no_window") &&
      iggy3d::smoke::hasField(wallJumpFields, "window_created", "false") &&
      iggy3d::smoke::hasField(wallJumpFields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(wallJumpFields, "active_room_id",
                              "movement_gym") &&
      iggy3d::smoke::hasField(wallJumpFields, "automation_control_last_key",
                              "gameplay.jump") &&
      iggy3d::smoke::hasField(wallJumpFields, "automation_control_last_action",
                              "game.jump") &&
      iggy3d::smoke::hasField(wallJumpFields, "automation_control_last_owner",
                              "gameplay") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_input_source",
                              "automation") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_input_used", "true") &&
      iggy3d::smoke::hasField(wallJumpFields, "input_owner", "gameplay") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_input_suppressed",
                              "false") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_jump_requested",
                              "true") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_jump_accepted",
                              "true") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_jump_active",
                              "true") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_jump_status",
                              "wall_jump") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_jump_reason_code",
                              "gameplay_jump_wall_jump") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_jump_velocity_mps",
                              fixed3(tuning.jumpImpulseMetersPerSecond)) &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_jump_start_y",
                              "0.800") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_jump_final_y",
                              fixed3(0.8F + tuning.wallJumpRiseMeters)) &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_traversal_requested",
                              "true") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_traversal_consumed",
                              "true") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_traversal_accepted",
                              "true") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_traversal_mechanic",
                              "wall_jump") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_traversal_slot_id",
                              "wall_r2_c31_actor_blocker") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_traversal_target_id",
                              "wall_r2_c31_actor_blocker") &&
      iggy3d::smoke::hasField(wallJumpFields,
                              "gameplay_traversal_landing_surface_id",
                              "wall_r2_c31_actor_blocker") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_traversal_start_x",
                              "310.000") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_traversal_start_y",
                              "0.800") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_traversal_start_z",
                              "19.650") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_traversal_final_x",
                              "310.000") &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_traversal_final_y",
                              fixed3(0.8F + tuning.wallJumpRiseMeters)) &&
      iggy3d::smoke::hasField(wallJumpFields, "gameplay_traversal_final_z",
                              fixed3(19.65F - tuning.wallJumpPushMeters)) &&
      iggy3d::smoke::hasField(wallJumpFields, "player_position_changed",
                              "true") &&
      iggy3d::smoke::hasField(wallJumpFields, "product_draw_prop_visible",
                              "true") &&
      iggy3d::smoke::hasField(wallJumpFields, "product_draw_prop_tile_count",
                              "10");

  const std::filesystem::path layeredWalkSaveRoot =
      iggy3d::smoke::cleanSaveRoot("gameplay_controls_layered_jump_gym_walk");
  iggy3d::smoke::ReceiptFields layeredWalkFields;
  int layeredWalkExitCode = 77;
  const bool layeredWalkControlsPassed =
      appBuilt && std::filesystem::exists(binary) &&
      iggy3d::smoke::runProductCase(
          binary,
          "gameplay_controls_layered_jump_gym_walk",
          "frontend.select=new_world\n"
          "frontend.execute=true\n"
          "world.dungeon_id=layered_jump_gym\n"
          "world.create=true\n"
          "game.move_y=1\n",
          iggy3d::smoke::saveRootArg(layeredWalkSaveRoot),
          layeredWalkFields,
          layeredWalkExitCode) &&
      layeredWalkExitCode == 0 &&
      iggy3d::smoke::productReceipt(layeredWalkFields) &&
      iggy3d::smoke::automationApplied(layeredWalkFields) &&
      iggy3d::smoke::hasField(layeredWalkFields, "window_mode",
                              "no_window") &&
      iggy3d::smoke::hasField(layeredWalkFields, "window_created",
                              "false") &&
      iggy3d::smoke::hasField(layeredWalkFields, "frontend_screen",
                              "gameplay") &&
      iggy3d::smoke::hasField(layeredWalkFields, "gameplay_active",
                              "true") &&
      iggy3d::smoke::hasField(layeredWalkFields, "active_room_id",
                              "layered_jump_gym") &&
      iggy3d::smoke::hasField(layeredWalkFields,
                              "automation_control_last_key", "game.move_y") &&
      iggy3d::smoke::hasField(layeredWalkFields,
                              "automation_control_last_action",
                              "game.move_y") &&
      iggy3d::smoke::hasField(layeredWalkFields,
                              "automation_control_last_owner", "gameplay") &&
      iggy3d::smoke::hasField(layeredWalkFields, "gameplay_input_source",
                              "automation") &&
      iggy3d::smoke::hasField(layeredWalkFields, "gameplay_input_used",
                              "true") &&
      iggy3d::smoke::hasField(layeredWalkFields, "input_owner",
                              "gameplay") &&
      iggy3d::smoke::hasField(layeredWalkFields, "gameplay_movement_status",
                              "moved") &&
      iggy3d::smoke::hasField(layeredWalkFields, "gameplay_movement_blocked",
                              "false") &&
      iggy3d::smoke::hasField(layeredWalkFields,
                              "gameplay_movement_reason_code",
                              "movement_ok") &&
      iggy3d::smoke::hasField(layeredWalkFields,
                              "gameplay_movement_blocked_reason",
                              "movement_ok") &&
      iggy3d::smoke::hasField(layeredWalkFields,
                              "gameplay_movement_ground_snap_applied",
                              "true") &&
      iggy3d::smoke::hasField(layeredWalkFields,
                              "gameplay_movement_final_y", "0.000") &&
      iggy3d::smoke::hasField(layeredWalkFields, "position_hud_visible",
                              "false") &&
      iggy3d::smoke::hasField(layeredWalkFields, "position_hud_status",
                              "not_requested") &&
      iggy3d::smoke::hasField(layeredWalkFields, "player_position_changed",
                              "true");

  const std::filesystem::path layeredResetSaveRoot =
      iggy3d::smoke::cleanSaveRoot("gameplay_controls_layered_jump_gym_reset");
  iggy3d::smoke::ReceiptFields layeredResetFields;
  int layeredResetExitCode = 77;
  const bool layeredResetControlsPassed =
      appBuilt && std::filesystem::exists(binary) &&
      iggy3d::smoke::runProductCase(
          binary,
          "gameplay_controls_layered_jump_gym_reset",
          "frontend.select=new_world\n"
          "frontend.execute=true\n"
          "world.dungeon_id=layered_jump_gym\n"
          "world.create=true\n"
          "gameplay.player_position=3,0.05,0\n"
          "game.move_y=1\n",
          iggy3d::smoke::saveRootArg(layeredResetSaveRoot),
          layeredResetFields,
          layeredResetExitCode) &&
      layeredResetExitCode == 0 &&
      iggy3d::smoke::productReceipt(layeredResetFields) &&
      iggy3d::smoke::automationApplied(layeredResetFields) &&
      iggy3d::smoke::hasField(layeredResetFields, "window_mode",
                              "no_window") &&
      iggy3d::smoke::hasField(layeredResetFields, "window_created",
                              "false") &&
      iggy3d::smoke::hasField(layeredResetFields, "frontend_screen",
                              "gameplay") &&
      iggy3d::smoke::hasField(layeredResetFields, "gameplay_active",
                              "true") &&
      iggy3d::smoke::hasField(layeredResetFields, "active_room_id",
                              "layered_jump_gym") &&
      iggy3d::smoke::hasField(layeredResetFields,
                              "active_room_authored_marker_count", "16") &&
      iggy3d::smoke::hasField(layeredResetFields,
                              "automation_control_last_key", "game.move_y") &&
      iggy3d::smoke::hasField(layeredResetFields,
                              "automation_control_last_action",
                              "game.move_y") &&
      iggy3d::smoke::hasField(layeredResetFields,
                              "automation_control_last_owner", "gameplay") &&
      iggy3d::smoke::hasField(layeredResetFields, "gameplay_input_source",
                              "automation") &&
      iggy3d::smoke::hasField(layeredResetFields, "gameplay_input_used",
                              "true") &&
      iggy3d::smoke::hasField(layeredResetFields, "input_owner",
                              "gameplay") &&
      iggy3d::smoke::hasField(layeredResetFields,
                              "gameplay_reset_triggered", "true") &&
      iggy3d::smoke::hasField(layeredResetFields, "gameplay_reset_status",
                              "reset") &&
      iggy3d::smoke::hasField(layeredResetFields,
                              "gameplay_reset_reason_code",
                              "gameplay_reset_zone") &&
      iggy3d::smoke::hasField(layeredResetFields,
                              "gameplay_reset_spawn_anchor_id",
                              "marker_player_spawn_r0_c0") &&
      iggy3d::smoke::hasField(layeredResetFields,
                              "gameplay_reset_source_anchor_id",
                              "marker_reset_zone_r0_c3") &&
      iggy3d::smoke::hasField(layeredResetFields, "gameplay_reset_start_y",
                              "0.050") &&
      iggy3d::smoke::hasField(layeredResetFields, "gameplay_reset_final_y",
                              "0.050") &&
      iggy3d::smoke::hasField(layeredResetFields, "product_draw_prop_visible",
                              "true") &&
      iggy3d::smoke::hasField(layeredResetFields,
                              "product_draw_prop_tile_count", "14") &&
      iggy3d::smoke::hasField(layeredResetFields,
                              "product_render_bridge_prop_visible", "true") &&
      iggy3d::smoke::hasField(layeredResetFields,
                              "product_render_bridge_prop_tile_count", "14");

  const bool passed =
      scriptedControlsPassed && clamberControlsPassed && wallJumpControlsPassed &&
      layeredWalkControlsPassed &&
      layeredResetControlsPassed;

  std::cout << "smoke=product_gameplay_controls\n";
  std::cout << "receipt_valid=" << (receiptValid ? "true" : "false") << "\n";
  std::cout << "actual_exit_code=" << exitCode << "\n";
  std::cout << "movement_gym_clamber="
            << (clamberControlsPassed ? "true" : "false") << "\n";
  std::cout << "movement_gym_clamber_exit_code=" << clamberExitCode << "\n";
  std::cout << "movement_gym_wall_jump="
            << (wallJumpControlsPassed ? "true" : "false") << "\n";
  std::cout << "movement_gym_wall_jump_exit_code=" << wallJumpExitCode << "\n";
  std::cout << "layered_jump_gym_walk="
            << (layeredWalkControlsPassed ? "true" : "false") << "\n";
  std::cout << "layered_jump_gym_walk_exit_code=" << layeredWalkExitCode
            << "\n";
  std::cout << "layered_jump_gym_reset="
            << (layeredResetControlsPassed ? "true" : "false") << "\n";
  std::cout << "layered_jump_gym_reset_exit_code=" << layeredResetExitCode
            << "\n";
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
