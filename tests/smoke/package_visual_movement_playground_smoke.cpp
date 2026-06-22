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

bool integerFieldAtLeast(const std::map<std::string, std::string>& fields,
                         const std::string& key,
                         unsigned long long threshold) {
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
  return value >= threshold;
}

}  // namespace

int main() {
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  const std::filesystem::path binary{IGGY3D_VISUAL_DEMO_PATH};
  const std::filesystem::path fixture =
      std::filesystem::current_path() / "fixtures/demos/movement_playground/package.iggy3d.toml";
  const std::filesystem::path output = "/tmp/iggy3d_package_visual_movement_playground.out";
  const std::filesystem::path slopeOutput =
      "/tmp/iggy3d_package_visual_movement_playground_slope.out";
  const std::filesystem::path slopeControl =
      "/tmp/iggy3d_package_visual_movement_playground_slope.control";
  const std::filesystem::path vaultOutput =
      "/tmp/iggy3d_package_visual_movement_playground_vault.out";
  const std::filesystem::path vaultControl =
      "/tmp/iggy3d_package_visual_movement_playground_vault.control";
  const std::filesystem::path clamberOutput =
      "/tmp/iggy3d_package_visual_movement_playground_clamber.out";
  const std::filesystem::path clamberControl =
      "/tmp/iggy3d_package_visual_movement_playground_clamber.control";
  const std::filesystem::path wireOutput =
      "/tmp/iggy3d_package_visual_movement_playground_wire.out";
  const std::filesystem::path wireControl =
      "/tmp/iggy3d_package_visual_movement_playground_wire.control";
  const std::filesystem::path intentOutput =
      "/tmp/iggy3d_package_visual_movement_playground_intent.out";
  const std::filesystem::path intentControl =
      "/tmp/iggy3d_package_visual_movement_playground_intent.control";
  const std::filesystem::path previewOutput =
      "/tmp/iggy3d_package_visual_movement_playground_preview.out";
  const std::filesystem::path previewControl =
      "/tmp/iggy3d_package_visual_movement_playground_preview.control";
  {
    std::ofstream control(slopeControl);
    control << "debug_overlay.open=true\n";
    control << "player.position_ft=38.0,2.2838,0.0\n";
    control << "move.forward=1\n";
  }
  {
    std::ofstream control(vaultControl);
    control << "dev_menu.open=true\n";
    control << "debug_overlay.open=true\n";
    control << "dev_menu.select=vault\n";
    control << "mechanic.execute=true\n";
    control << "player.position_ft=0.0,0.0,-16.5\n";
    control << "look.yaw=0.0\n";
  }
  {
    std::ofstream control(clamberControl);
    control << "dev_menu.open=true\n";
    control << "debug_overlay.open=true\n";
    control << "dev_menu.select=clamber\n";
    control << "mechanic.execute=true\n";
    control << "player.position_ft=-12.0,0.0,-16.5\n";
    control << "look.yaw=0.0\n";
  }
  {
    std::ofstream control(wireControl);
    control << "dev_menu.open=true\n";
    control << "debug_overlay.open=true\n";
    control << "dev_menu.select=wire_walk\n";
    control << "mechanic.execute=true\n";
    control << "player.position_ft=0.0,0.0,-30.5\n";
    control << "look.yaw=0.0\n";
  }
  {
    std::ofstream control(intentControl);
    control << "debug_overlay.open=true\n";
    control << "jump=true\n";
    control << "player.position_ft=-12.0,0.0,-16.5\n";
    control << "look.yaw=0.0\n";
  }
  {
    std::ofstream control(previewControl);
    control << "debug_overlay.open=true\n";
    control << "player.position_ft=-12.0,0.0,-16.5\n";
    control << "look.yaw=0.0\n";
  }
  const std::string command =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer vulkan --frames 3 --window --interactive --scripted-kinematic-input "
      "--scripted-crouch-input --print-render-receipt > " +
      shellQuote(output);
  const std::string slopeCommand =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer vulkan --frames 2 --window --interactive --input keyboard "
      "--codex-control " +
      shellQuote(slopeControl) + " --print-render-receipt > " + shellQuote(slopeOutput);
  const std::string vaultCommand =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer vulkan --frames 1 --window --interactive --input keyboard "
      "--codex-control " +
      shellQuote(vaultControl) + " --print-render-receipt > " + shellQuote(vaultOutput);
  const std::string clamberCommand =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer vulkan --frames 1 --window --interactive --input keyboard "
      "--codex-control " +
      shellQuote(clamberControl) + " --print-render-receipt > " +
      shellQuote(clamberOutput);
  const std::string wireCommand =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer vulkan --frames 1 --window --interactive --input keyboard "
      "--codex-control " +
      shellQuote(wireControl) + " --print-render-receipt > " + shellQuote(wireOutput);
  const std::string intentCommand =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer vulkan --frames 1 --window --interactive --input keyboard "
      "--codex-control " +
      shellQuote(intentControl) + " --print-render-receipt > " +
      shellQuote(intentOutput);
  const std::string previewCommand =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer vulkan --frames 1 --window --interactive --input keyboard "
      "--codex-control " +
      shellQuote(previewControl) + " --print-render-receipt > " +
      shellQuote(previewOutput);

  const int exitCode =
      std::filesystem::exists(binary) ? exitCodeFromSystem(std::system(command.c_str())) : 1;
  std::map<std::string, std::string> fields;
  const bool receiptValid = parseReceiptFile(output, fields);
  const int slopeExitCode =
      std::filesystem::exists(binary) ? exitCodeFromSystem(std::system(slopeCommand.c_str())) : 1;
  std::map<std::string, std::string> slopeFields;
  const bool slopeReceiptValid = parseReceiptFile(slopeOutput, slopeFields);
  const int vaultExitCode =
      std::filesystem::exists(binary) ? exitCodeFromSystem(std::system(vaultCommand.c_str())) : 1;
  std::map<std::string, std::string> vaultFields;
  const bool vaultReceiptValid = parseReceiptFile(vaultOutput, vaultFields);
  const int clamberExitCode = std::filesystem::exists(binary)
                                  ? exitCodeFromSystem(std::system(clamberCommand.c_str()))
                                  : 1;
  std::map<std::string, std::string> clamberFields;
  const bool clamberReceiptValid = parseReceiptFile(clamberOutput, clamberFields);
  const int wireExitCode =
      std::filesystem::exists(binary) ? exitCodeFromSystem(std::system(wireCommand.c_str())) : 1;
  std::map<std::string, std::string> wireFields;
  const bool wireReceiptValid = parseReceiptFile(wireOutput, wireFields);
  const int intentExitCode = std::filesystem::exists(binary)
                                 ? exitCodeFromSystem(std::system(intentCommand.c_str()))
                                 : 1;
  std::map<std::string, std::string> intentFields;
  const bool intentReceiptValid = parseReceiptFile(intentOutput, intentFields);
  const int previewExitCode = std::filesystem::exists(binary)
                                  ? exitCodeFromSystem(std::system(previewCommand.c_str()))
                                  : 1;
  std::map<std::string, std::string> previewFields;
  const bool previewReceiptValid = parseReceiptFile(previewOutput, previewFields);
  const bool skipped =
      (exitCode == 77 && receiptValid && hasField(fields, "result", "skip")) ||
      (slopeExitCode == 77 && slopeReceiptValid && hasField(slopeFields, "result", "skip")) ||
      (vaultExitCode == 77 && vaultReceiptValid && hasField(vaultFields, "result", "skip")) ||
      (clamberExitCode == 77 && clamberReceiptValid &&
       hasField(clamberFields, "result", "skip")) ||
      (wireExitCode == 77 && wireReceiptValid && hasField(wireFields, "result", "skip")) ||
      (intentExitCode == 77 && intentReceiptValid && hasField(intentFields, "result", "skip")) ||
      (previewExitCode == 77 && previewReceiptValid &&
       hasField(previewFields, "result", "skip"));
  const bool passed =
      exitCode == 0 && receiptValid && hasField(fields, "backend", "vulkan") &&
      hasField(fields, "rendering_path", "package_room_meshes") &&
      hasField(fields, "record_mode", "room_mesh_draws") &&
      hasField(fields, "room_asset_loaded", "true") &&
      hasField(fields, "room_asset_id", "movement_playground") &&
      hasField(fields, "source_subset", "movement_playground_v1") &&
      integerFieldAtLeast(fields, "room_static_mesh_count", 100ULL) &&
      integerFieldAtLeast(fields, "room_anchor_count", 5ULL) &&
      integerFieldAtLeast(fields, "mesh_draw_count", 100ULL) &&
      integerFieldAtLeast(fields, "indexed_draw_count", 100ULL) &&
      hasField(fields, "vertex_buffer_uploaded", "true") &&
      hasField(fields, "index_buffer_uploaded", "true") &&
      hasField(fields, "input_backend", "scripted") &&
      hasField(fields, "kinematic_movement_attempted", "true") &&
      hasField(fields, "kinematic_movement_accepted", "true") &&
      hasField(fields, "crouch_available", "true") &&
      hasField(fields, "crouch_active", "true") &&
      hasField(fields, "crouch_input_observed", "true") &&
      hasField(fields, "stance", "crouched") &&
      hasField(fields, "eye_height_meters", "1.050") &&
      hasField(fields, "actor_height_meters", "1.200") &&
      hasField(fields, "movement_speed_meters_per_second", "2.350") &&
      hasField(fields, "floor_visible", "true") &&
      hasField(fields, "wall_visible", "true") &&
      hasField(fields, "first_room_visible", "true") &&
      slopeExitCode == 0 && slopeReceiptValid &&
      hasField(slopeFields, "result", "pass") &&
      hasField(slopeFields, "backend", "vulkan") &&
      hasField(slopeFields, "input_backend", "scripted") &&
      hasField(slopeFields, "codex_control_configured", "true") &&
      hasField(slopeFields, "codex_control_read", "true") &&
      hasField(slopeFields, "codex_control_applied", "true") &&
      hasField(slopeFields, "ground_surface_id", "moderate_slope_walkable") &&
      hasField(slopeFields, "ground_sample_valid", "true") &&
      hasField(slopeFields, "ground_contact", "true") &&
      hasField(slopeFields, "ground_walkable", "true") &&
      hasField(slopeFields, "careful_footing", "true") &&
      hasField(slopeFields, "movement_policy_band", "moderate") &&
      hasField(slopeFields, "kinematic_movement_attempted", "true") &&
      hasField(slopeFields, "kinematic_movement_accepted", "true") &&
      hasField(slopeFields, "movement_distance_meters", "0.056") &&
      hasField(slopeFields, "movement_horizontal_distance_meters", "0.053") &&
      hasField(slopeFields, "movement_vertical_delta_meters", "-0.019") &&
      hasField(slopeFields, "movement_grade_percent", "-36.385") &&
      hasField(slopeFields, "slope_travel_direction", "downhill") &&
      hasField(slopeFields, "movement_start_y", "0.696") &&
      hasField(slopeFields, "movement_start_z", "0.000") &&
      hasField(slopeFields, "movement_destination_y", "0.677") &&
      hasField(slopeFields, "movement_destination_z", "-0.053") &&
      hasField(slopeFields, "movement_final_y", "0.677") &&
      hasField(slopeFields, "movement_final_z", "-0.053") &&
      hasField(slopeFields, "ground_normal_y", "0.940") &&
      hasField(slopeFields, "ground_normal_z", "-0.342") &&
      hasField(slopeFields, "slope_angle_degrees", "20.000") &&
      hasField(slopeFields, "slope_up_dot", "0.940") &&
      hasField(slopeFields, "speed_multiplier", "0.750") &&
      hasField(slopeFields, "stamina_cost_multiplier", "1.350") &&
      hasField(slopeFields, "step_penalty_multiplier", "1.200") &&
      hasField(slopeFields, "debug_overlay_open", "true") &&
      hasField(slopeFields, "debug_movement_policy_band", "moderate") &&
      hasField(slopeFields, "debug_ground_surface_id", "moderate_slope_walkable") &&
      hasField(slopeFields, "debug_ground_contact", "true") &&
      hasField(slopeFields, "debug_ground_walkable", "true") &&
      hasField(slopeFields, "debug_movement_horizontal_distance_meters", "0.053") &&
      hasField(slopeFields, "debug_movement_vertical_delta_meters", "-0.019") &&
      hasField(slopeFields, "debug_movement_grade_percent", "-36.385") &&
      hasField(slopeFields, "debug_slope_travel_direction", "downhill") &&
      hasField(slopeFields, "debug_moved_this_frame_meters", "0.056") &&
      hasField(slopeFields, "debug_position_y", "0.677") &&
      hasField(slopeFields, "debug_position_z", "-0.053") &&
      vaultExitCode == 0 && vaultReceiptValid &&
      hasField(vaultFields, "result", "pass") &&
      hasField(vaultFields, "backend", "vulkan") &&
      hasField(vaultFields, "input_backend", "scripted") &&
      hasField(vaultFields, "dev_menu_enabled", "true") &&
      hasField(vaultFields, "dev_menu_open", "true") &&
      hasField(vaultFields, "dev_menu_selected_mechanic", "vault") &&
      hasField(vaultFields, "dev_menu_execute_requested", "true") &&
      hasField(vaultFields, "dev_menu_execution_status", "applied") &&
      hasField(vaultFields, "traversal_attempted", "true") &&
      hasField(vaultFields, "traversal_accepted", "true") &&
      hasField(vaultFields, "traversal_mechanic", "vault") &&
      hasField(vaultFields, "traversal_reason", "traversal_applied") &&
      hasField(vaultFields, "traversal_target_id", "vault_rail") &&
      hasField(vaultFields, "traversal_landing_surface_id", "main_floor_walkable") &&
      hasField(vaultFields, "traversal_distance_meters", "1.403") &&
      hasField(vaultFields, "traversal_horizontal_distance_meters", "1.403") &&
      hasField(vaultFields, "traversal_vertical_delta_meters", "0.015") &&
      hasField(vaultFields, "traversal_grade_percent", "1.086") &&
      hasField(vaultFields, "traversal_direction", "uphill") &&
      hasField(vaultFields, "traversal_start_z", "-5.029") &&
      hasField(vaultFields, "traversal_final_y", "0.015") &&
      hasField(vaultFields, "traversal_final_z", "-6.432") &&
      hasField(vaultFields, "debug_position_y", "0.015") &&
      hasField(vaultFields, "debug_position_z", "-6.432") &&
      clamberExitCode == 0 && clamberReceiptValid &&
      hasField(clamberFields, "result", "pass") &&
      hasField(clamberFields, "backend", "vulkan") &&
      hasField(clamberFields, "input_backend", "scripted") &&
      hasField(clamberFields, "dev_menu_enabled", "true") &&
      hasField(clamberFields, "dev_menu_open", "true") &&
      hasField(clamberFields, "dev_menu_selected_mechanic", "clamber") &&
      hasField(clamberFields, "dev_menu_execute_requested", "true") &&
      hasField(clamberFields, "dev_menu_execution_status", "applied") &&
      hasField(clamberFields, "traversal_attempted", "true") &&
      hasField(clamberFields, "traversal_accepted", "true") &&
      hasField(clamberFields, "traversal_mechanic", "clamber") &&
      hasField(clamberFields, "traversal_reason", "traversal_applied") &&
      hasField(clamberFields, "traversal_slot_id",
               "clamber_block:clamber_top_walkable") &&
      hasField(clamberFields, "traversal_slot_kind", "clamber") &&
      hasField(clamberFields, "traversal_slot_height_band", "clamber_low") &&
      hasField(clamberFields, "traversal_target_id", "clamber_block") &&
      hasField(clamberFields, "traversal_landing_surface_id",
               "clamber_top_walkable") &&
      hasField(clamberFields, "traversal_slot_ledge_height_meters", "0.610") &&
      hasField(clamberFields, "traversal_slot_usable_width_meters", "1.829") &&
      hasField(clamberFields, "traversal_slot_start_range_meters", "0.152") &&
      hasField(clamberFields, "traversal_slot_facing_dot", "1.000") &&
      hasField(clamberFields, "traversal_distance_meters", "0.762") &&
      hasField(clamberFields, "traversal_horizontal_distance_meters", "0.457") &&
      hasField(clamberFields, "traversal_vertical_delta_meters", "0.610") &&
      hasField(clamberFields, "traversal_grade_percent", "133.333") &&
      hasField(clamberFields, "traversal_direction", "uphill") &&
      hasField(clamberFields, "traversal_start_x", "-3.658") &&
      hasField(clamberFields, "traversal_start_z", "-5.029") &&
      hasField(clamberFields, "traversal_final_x", "-3.658") &&
      hasField(clamberFields, "traversal_final_y", "0.610") &&
      hasField(clamberFields, "traversal_final_z", "-5.486") &&
      hasField(clamberFields, "ground_surface_id", "clamber_top_walkable") &&
      hasField(clamberFields, "debug_position_y", "0.610") &&
      hasField(clamberFields, "debug_position_z", "-5.486") &&
      hasField(clamberFields, "debug_traversal_available", "true") &&
      hasField(clamberFields, "debug_traversal_attempted", "true") &&
      hasField(clamberFields, "debug_traversal_accepted", "true") &&
      hasField(clamberFields, "debug_traversal_mechanic", "clamber") &&
      hasField(clamberFields, "debug_traversal_reason", "traversal_applied") &&
      hasField(clamberFields, "debug_traversal_slot_id",
               "clamber_block:clamber_top_walkable") &&
      hasField(clamberFields, "debug_traversal_slot_height_band", "clamber_low") &&
      hasField(clamberFields, "debug_traversal_slot_facing_dot", "1.000") &&
      wireExitCode == 0 && wireReceiptValid &&
      hasField(wireFields, "result", "pass") &&
      hasField(wireFields, "backend", "vulkan") &&
      hasField(wireFields, "input_backend", "scripted") &&
      hasField(wireFields, "dev_menu_enabled", "true") &&
      hasField(wireFields, "dev_menu_open", "true") &&
      hasField(wireFields, "dev_menu_selected_mechanic", "wire_walk") &&
      hasField(wireFields, "dev_menu_execute_requested", "true") &&
      hasField(wireFields, "dev_menu_execution_status", "applied") &&
      hasField(wireFields, "traversal_attempted", "true") &&
      hasField(wireFields, "traversal_accepted", "true") &&
      hasField(wireFields, "traversal_mechanic", "wire_walk") &&
      hasField(wireFields, "traversal_reason", "traversal_applied") &&
      hasField(wireFields, "traversal_slot_id", "wire_rail") &&
      hasField(wireFields, "traversal_slot_kind", "wire_walk") &&
      hasField(wireFields, "traversal_slot_height_band", "wire_balance") &&
      hasField(wireFields, "traversal_target_id", "wire_rail") &&
      hasField(wireFields, "traversal_landing_surface_id", "wire_rail") &&
      hasField(wireFields, "traversal_slot_ledge_height_meters", "0.952") &&
      hasField(wireFields, "traversal_slot_usable_width_meters", "6.096") &&
      hasField(wireFields, "traversal_slot_start_range_meters", "0.419") &&
      hasField(wireFields, "traversal_slot_facing_dot", "1.000") &&
      hasField(wireFields, "traversal_distance_meters", "1.057") &&
      hasField(wireFields, "traversal_horizontal_distance_meters", "0.457") &&
      hasField(wireFields, "traversal_vertical_delta_meters", "0.952") &&
      hasField(wireFields, "traversal_grade_percent", "208.333") &&
      hasField(wireFields, "traversal_direction", "uphill") &&
      hasField(wireFields, "traversal_start_z", "-9.296") &&
      hasField(wireFields, "traversal_final_y", "0.952") &&
      hasField(wireFields, "traversal_final_z", "-9.754") &&
      hasField(wireFields, "debug_position_y", "0.950") &&
      hasField(wireFields, "debug_position_z", "-9.754") &&
      hasField(wireFields, "debug_traversal_preview_available", "true") &&
      hasField(wireFields, "debug_traversal_preview_ready", "true") &&
      hasField(wireFields, "debug_traversal_preview_mechanic", "wire_walk") &&
      hasField(wireFields, "debug_traversal_preview_slot_id", "wire_rail") &&
      hasField(wireFields, "debug_traversal_preview_slot_height_band", "wire_balance") &&
      hasField(wireFields, "debug_traversal_available", "true") &&
      hasField(wireFields, "debug_traversal_attempted", "true") &&
      hasField(wireFields, "debug_traversal_accepted", "true") &&
      hasField(wireFields, "debug_traversal_mechanic", "wire_walk") &&
      hasField(wireFields, "debug_traversal_slot_id", "wire_rail") &&
      hasField(wireFields, "debug_traversal_slot_height_band", "wire_balance") &&
      hasField(wireFields, "debug_traversal_slot_facing_dot", "1.000") &&
      previewExitCode == 0 && previewReceiptValid &&
      hasField(previewFields, "result", "pass") &&
      hasField(previewFields, "backend", "vulkan") &&
      hasField(previewFields, "input_backend", "scripted") &&
      hasField(previewFields, "dev_menu_execute_requested", "false") &&
      integerFieldAtLeast(previewFields, "debug_hud_line_count", 11ULL) &&
      hasField(previewFields, "debug_traversal_preview_available", "true") &&
      hasField(previewFields, "debug_traversal_preview_ready", "true") &&
      hasField(previewFields, "debug_traversal_preview_candidate_available", "true") &&
      hasField(previewFields, "debug_traversal_preview_status",
               "traversal_preview_ready") &&
      hasField(previewFields, "debug_traversal_preview_hud_code", "READY") &&
      hasField(previewFields, "debug_traversal_preview_mechanic", "clamber") &&
      hasField(previewFields, "debug_traversal_preview_slot_id",
               "clamber_block:clamber_top_walkable") &&
      hasField(previewFields, "debug_traversal_preview_slot_kind", "clamber") &&
      hasField(previewFields, "debug_traversal_preview_slot_height_band", "clamber_low") &&
      hasField(previewFields, "debug_traversal_preview_target_id", "clamber_block") &&
      hasField(previewFields, "debug_traversal_preview_landing_surface_id",
               "clamber_top_walkable") &&
      hasField(previewFields, "debug_traversal_preview_slot_ledge_height_meters", "0.610") &&
      hasField(previewFields, "debug_traversal_preview_slot_usable_width_meters", "1.829") &&
      hasField(previewFields, "debug_traversal_preview_slot_start_range_meters", "0.152") &&
      hasField(previewFields, "debug_traversal_preview_slot_facing_dot", "1.000") &&
      hasField(previewFields, "debug_traversal_available", "false") &&
      intentExitCode == 0 && intentReceiptValid &&
      hasField(intentFields, "result", "pass") &&
      hasField(intentFields, "backend", "vulkan") &&
      hasField(intentFields, "input_backend", "scripted") &&
      hasField(intentFields, "dev_menu_execute_requested", "false") &&
      hasField(intentFields, "jump_input_observed", "true") &&
      hasField(intentFields, "jump_accepted", "false") &&
      hasField(intentFields, "traversal_intent_requested", "true") &&
      hasField(intentFields, "traversal_intent_consumed", "true") &&
      hasField(intentFields, "traversal_intent_accepted", "true") &&
      hasField(intentFields, "traversal_intent_fallback_jump_allowed", "false") &&
      hasField(intentFields, "traversal_intent_trigger", "jump") &&
      hasField(intentFields, "traversal_intent_status", "traversal_intent_applied") &&
      hasField(intentFields, "traversal_intent_selected_mechanic", "clamber") &&
      hasField(intentFields, "traversal_attempted", "true") &&
      hasField(intentFields, "traversal_accepted", "true") &&
      hasField(intentFields, "traversal_mechanic", "clamber") &&
      hasField(intentFields, "traversal_reason", "traversal_applied") &&
      hasField(intentFields, "traversal_slot_id",
               "clamber_block:clamber_top_walkable") &&
      hasField(intentFields, "traversal_slot_kind", "clamber") &&
      hasField(intentFields, "traversal_slot_height_band", "clamber_low") &&
      hasField(intentFields, "traversal_final_y", "0.610") &&
      hasField(intentFields, "traversal_final_z", "-5.486") &&
      hasField(intentFields, "ground_surface_id", "clamber_top_walkable") &&
      hasField(intentFields, "debug_position_y", "0.610") &&
      hasField(intentFields, "debug_position_z", "-5.486") &&
      integerFieldAtLeast(intentFields, "debug_hud_line_count", 11ULL) &&
      hasField(intentFields, "debug_traversal_available", "true") &&
      hasField(intentFields, "debug_traversal_intent_requested", "true") &&
      hasField(intentFields, "debug_traversal_intent_consumed", "true") &&
      hasField(intentFields, "debug_traversal_intent_accepted", "true") &&
      hasField(intentFields, "debug_traversal_intent_fallback_jump_allowed", "false") &&
      hasField(intentFields, "debug_traversal_intent_trigger", "jump") &&
      hasField(intentFields, "debug_traversal_intent_status",
               "traversal_intent_applied") &&
      hasField(intentFields, "debug_traversal_intent_selected_mechanic",
               "clamber") &&
      hasField(intentFields, "debug_traversal_attempted", "true") &&
      hasField(intentFields, "debug_traversal_accepted", "true") &&
      hasField(intentFields, "debug_traversal_mechanic", "clamber") &&
      hasField(intentFields, "debug_traversal_reason", "traversal_applied") &&
      hasField(intentFields, "debug_traversal_slot_id",
               "clamber_block:clamber_top_walkable") &&
      hasField(intentFields, "debug_traversal_slot_kind", "clamber") &&
      hasField(intentFields, "debug_traversal_slot_height_band", "clamber_low") &&
      hasField(intentFields, "debug_traversal_target_id", "clamber_block") &&
      hasField(intentFields, "debug_traversal_landing_surface_id",
               "clamber_top_walkable") &&
      hasField(intentFields, "debug_traversal_slot_ledge_height_meters", "0.610") &&
      hasField(intentFields, "debug_traversal_slot_usable_width_meters", "1.829") &&
      hasField(intentFields, "debug_traversal_slot_start_range_meters", "0.152") &&
      hasField(intentFields, "debug_traversal_slot_facing_dot", "1.000") &&
      hasField(fields, "result", "pass");
#else
  const int exitCode = 77;
  const int slopeExitCode = 77;
  const int vaultExitCode = 77;
  const int clamberExitCode = 77;
  const int wireExitCode = 77;
  const int intentExitCode = 77;
  const int previewExitCode = 77;
  const bool receiptValid = false;
  const bool slopeReceiptValid = false;
  const bool vaultReceiptValid = false;
  const bool clamberReceiptValid = false;
  const bool wireReceiptValid = false;
  const bool intentReceiptValid = false;
  const bool previewReceiptValid = false;
  const bool skipped = true;
  const bool passed = false;
#endif

  std::cout << "smoke=package_visual_movement_playground\n";
  std::cout << "receipt_valid=" << (receiptValid ? "true" : "false") << "\n";
  std::cout << "slope_receipt_valid=" << (slopeReceiptValid ? "true" : "false") << "\n";
  std::cout << "vault_receipt_valid=" << (vaultReceiptValid ? "true" : "false") << "\n";
  std::cout << "clamber_receipt_valid=" << (clamberReceiptValid ? "true" : "false") << "\n";
  std::cout << "wire_receipt_valid=" << (wireReceiptValid ? "true" : "false") << "\n";
  std::cout << "intent_receipt_valid=" << (intentReceiptValid ? "true" : "false") << "\n";
  std::cout << "preview_receipt_valid=" << (previewReceiptValid ? "true" : "false") << "\n";
  std::cout << "actual_exit_code=" << exitCode << "\n";
  std::cout << "slope_exit_code=" << slopeExitCode << "\n";
  std::cout << "vault_exit_code=" << vaultExitCode << "\n";
  std::cout << "clamber_exit_code=" << clamberExitCode << "\n";
  std::cout << "wire_exit_code=" << wireExitCode << "\n";
  std::cout << "intent_exit_code=" << intentExitCode << "\n";
  std::cout << "preview_exit_code=" << previewExitCode << "\n";
  std::cout << "result=" << (passed ? "pass" : (skipped ? "skip" : "fail")) << "\n";
  std::cout << "reason_code="
            << (passed ? "package_visual_movement_playground_pass"
                       : (skipped ? "package_visual_movement_playground_skip"
                                  : "package_visual_movement_playground_failed"))
            << "\n";
  if (passed) {
    return 0;
  }
  return skipped ? 77 : 1;
}
