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

bool fieldsEqual(const std::map<std::string, std::string>& fields,
                 const std::string& left,
                 const std::string& right) {
  const auto leftFound = fields.find(left);
  const auto rightFound = fields.find(right);
  return leftFound != fields.end() && rightFound != fields.end() &&
         leftFound->second == rightFound->second;
}

bool writeControlFile(const std::filesystem::path& path, const std::string& mechanic) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << "dev_menu.open=true\n";
  output << "debug_overlay.open=true\n";
  output << "dev_menu.select=" << mechanic << "\n";
  output << "mechanic.execute=true\n";
  output << "move.forward=1\n";
  output << "look.yaw_delta=0.100\n";
  output << "look.pitch_delta=0.050\n";
  return static_cast<bool>(output);
}

bool writeSpellHitControlFile(const std::filesystem::path& path) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << "dev_menu.open=true\n";
  output << "dev_menu.select=spell\n";
  output << "mechanic.execute=true\n";
  output << "look.yaw=0.359\n";
  output << "look.pitch=-0.105\n";
  return static_cast<bool>(output);
}

bool writeSpellRecastControlFile(const std::filesystem::path& path) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << "dev_menu.open=true\n";
  output << "dev_menu.select=spell\n";
  output << "mechanic.execute_frames=0,2,42\n";
  output << "look.yaw=0.359\n";
  output << "look.pitch=-0.105\n";
  return static_cast<bool>(output);
}

bool writeSpellDefeatControlFile(const std::filesystem::path& path) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << "dev_menu.open=true\n";
  output << "dev_menu.select=spell\n";
  output << "mechanic.execute_frames=0,2,42\n";
  output << "look.yaw=0.359\n";
  output << "look.pitch=-0.105\n";
  return static_cast<bool>(output);
}

bool writeSpellResetControlFile(const std::filesystem::path& path) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << "dev_menu.open=true\n";
  output << "dev_menu.select=spell\n";
  output << "mechanic.execute_frames=0,2,42\n";
  output << "reset_frames=82\n";
  output << "look.yaw=0.359\n";
  output << "look.pitch=-0.105\n";
  return static_cast<bool>(output);
}

bool writePauseStepControlFile(const std::filesystem::path& path) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << "tactical_toggle_frames=0\n";
  output << "pause_frames=2\n";
  output << "step_frames=4\n";
  output << "resume_frames=6\n";
  return static_cast<bool>(output);
}

bool writeSpellSaveLoadControlFile(const std::filesystem::path& path) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << "dev_menu.open=true\n";
  output << "dev_menu.select=spell\n";
  output << "mechanic.execute=true\n";
  output << "save_load_frames=50\n";
  output << "look.yaw=0.359\n";
  output << "look.pitch=-0.105\n";
  return static_cast<bool>(output);
}

bool writeAcceptanceDemoControlFile(const std::filesystem::path& path) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << "acceptance_demo=true\n";
  return static_cast<bool>(output);
}

}  // namespace

int main() {
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  const std::filesystem::path binary{IGGY3D_VISUAL_DEMO_PATH};
  const std::filesystem::path fixture =
      std::filesystem::current_path() / "fixtures/demos/movement_playground/package.iggy3d.toml";
  const std::filesystem::path jumpControl = "/tmp/iggy3d_codex_jump_control.in";
  const std::filesystem::path jumpOutput = "/tmp/iggy3d_package_visual_codex_jump_control.out";
  const std::filesystem::path dashControl = "/tmp/iggy3d_codex_dash_control.in";
  const std::filesystem::path dashOutput = "/tmp/iggy3d_package_visual_codex_dash_control.out";
  const std::filesystem::path spellControl = "/tmp/iggy3d_codex_spell_control.in";
  const std::filesystem::path spellOutput = "/tmp/iggy3d_package_visual_codex_spell_control.out";
  const std::filesystem::path spellHitControl = "/tmp/iggy3d_codex_spell_hit_control.in";
  const std::filesystem::path spellHitOutput =
      "/tmp/iggy3d_package_visual_codex_spell_hit_control.out";
  const std::filesystem::path spellRecastControl =
      "/tmp/iggy3d_codex_spell_recast_control.in";
  const std::filesystem::path spellRecastOutput =
      "/tmp/iggy3d_package_visual_codex_spell_recast_control.out";
  const std::filesystem::path spellDefeatControl =
      "/tmp/iggy3d_codex_spell_defeat_control.in";
  const std::filesystem::path spellDefeatOutput =
      "/tmp/iggy3d_package_visual_codex_spell_defeat_control.out";
  const std::filesystem::path spellResetControl =
      "/tmp/iggy3d_codex_spell_reset_control.in";
  const std::filesystem::path spellResetOutput =
      "/tmp/iggy3d_package_visual_codex_spell_reset_control.out";
  const std::filesystem::path pauseStepControl =
      "/tmp/iggy3d_codex_pause_step_control.in";
  const std::filesystem::path pauseStepOutput =
      "/tmp/iggy3d_package_visual_codex_pause_step_control.out";
  const std::filesystem::path spellSaveLoadControl =
      "/tmp/iggy3d_codex_spell_save_load_control.in";
  const std::filesystem::path spellSaveLoadOutput =
      "/tmp/iggy3d_package_visual_codex_spell_save_load_control.out";
  const std::filesystem::path acceptanceDemoControl =
      "/tmp/iggy3d_codex_acceptance_demo_control.in";
  const std::filesystem::path acceptanceDemoOutput =
      "/tmp/iggy3d_package_visual_codex_acceptance_demo_control.out";
  const bool jumpControlWritten = writeControlFile(jumpControl, "jump");
  const bool dashControlWritten = writeControlFile(dashControl, "dash");
  const bool spellControlWritten = writeControlFile(spellControl, "spell");
  const bool spellHitControlWritten = writeSpellHitControlFile(spellHitControl);
  const bool spellRecastControlWritten = writeSpellRecastControlFile(spellRecastControl);
  const bool spellDefeatControlWritten = writeSpellDefeatControlFile(spellDefeatControl);
  const bool spellResetControlWritten = writeSpellResetControlFile(spellResetControl);
  const bool pauseStepControlWritten = writePauseStepControlFile(pauseStepControl);
  const bool spellSaveLoadControlWritten =
      writeSpellSaveLoadControlFile(spellSaveLoadControl);
  const bool acceptanceDemoControlWritten =
      writeAcceptanceDemoControlFile(acceptanceDemoControl);
  const std::string jumpCommand =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer vulkan --window --interactive --frames 3 --dev-menu --codex-control " +
      shellQuote(jumpControl) + " --print-render-receipt > " + shellQuote(jumpOutput);
  const std::string dashCommand =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer vulkan --window --interactive --frames 3 --dev-menu --codex-control " +
      shellQuote(dashControl) + " --print-render-receipt > " + shellQuote(dashOutput);
  const std::string spellCommand =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer vulkan --window --interactive --frames 3 --dev-menu --codex-control " +
      shellQuote(spellControl) + " --print-render-receipt > " + shellQuote(spellOutput);
  const std::string spellHitCommand =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer null --interactive --frames 50 --dev-menu --codex-control " +
      shellQuote(spellHitControl) + " --print-render-receipt > " +
      shellQuote(spellHitOutput);
  const std::string spellRecastCommand =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer null --interactive --frames 45 --dev-menu --codex-control " +
      shellQuote(spellRecastControl) + " --print-render-receipt > " +
      shellQuote(spellRecastOutput);
  const std::string spellDefeatCommand =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer null --interactive --frames 90 --dev-menu --codex-control " +
      shellQuote(spellDefeatControl) + " --print-render-receipt > " +
      shellQuote(spellDefeatOutput);
  const std::string spellResetCommand =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer null --interactive --frames 90 --dev-menu --codex-control " +
      shellQuote(spellResetControl) + " --print-render-receipt > " +
      shellQuote(spellResetOutput);
  const std::string pauseStepCommand =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer null --interactive --frames 8 --dev-menu --codex-control " +
      shellQuote(pauseStepControl) + " --print-render-receipt > " +
      shellQuote(pauseStepOutput);
  const std::string spellSaveLoadCommand =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer null --interactive --frames 55 --dev-menu --codex-control " +
      shellQuote(spellSaveLoadControl) + " --print-render-receipt > " +
      shellQuote(spellSaveLoadOutput);
  const std::string acceptanceDemoCommand =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer null --interactive --frames 71 --dev-menu --codex-control " +
      shellQuote(acceptanceDemoControl) + " --print-render-receipt > " +
      shellQuote(acceptanceDemoOutput);

  const int jumpExitCode =
      jumpControlWritten && std::filesystem::exists(binary)
          ? exitCodeFromSystem(std::system(jumpCommand.c_str()))
          : 1;
  const int dashExitCode =
      dashControlWritten && std::filesystem::exists(binary)
          ? exitCodeFromSystem(std::system(dashCommand.c_str()))
          : 1;
  const int spellExitCode =
      spellControlWritten && std::filesystem::exists(binary)
          ? exitCodeFromSystem(std::system(spellCommand.c_str()))
          : 1;
  const int spellHitExitCode =
      spellHitControlWritten && std::filesystem::exists(binary)
          ? exitCodeFromSystem(std::system(spellHitCommand.c_str()))
          : 1;
  const int spellRecastExitCode =
      spellRecastControlWritten && std::filesystem::exists(binary)
          ? exitCodeFromSystem(std::system(spellRecastCommand.c_str()))
          : 1;
  const int spellDefeatExitCode =
      spellDefeatControlWritten && std::filesystem::exists(binary)
          ? exitCodeFromSystem(std::system(spellDefeatCommand.c_str()))
          : 1;
  const int spellResetExitCode =
      spellResetControlWritten && std::filesystem::exists(binary)
          ? exitCodeFromSystem(std::system(spellResetCommand.c_str()))
          : 1;
  const int pauseStepExitCode =
      pauseStepControlWritten && std::filesystem::exists(binary)
          ? exitCodeFromSystem(std::system(pauseStepCommand.c_str()))
          : 1;
  const int spellSaveLoadExitCode =
      spellSaveLoadControlWritten && std::filesystem::exists(binary)
          ? exitCodeFromSystem(std::system(spellSaveLoadCommand.c_str()))
          : 1;
  const int acceptanceDemoExitCode =
      acceptanceDemoControlWritten && std::filesystem::exists(binary)
          ? exitCodeFromSystem(std::system(acceptanceDemoCommand.c_str()))
          : 1;
  std::map<std::string, std::string> jumpFields;
  std::map<std::string, std::string> dashFields;
  std::map<std::string, std::string> spellFields;
  std::map<std::string, std::string> spellHitFields;
  std::map<std::string, std::string> spellRecastFields;
  std::map<std::string, std::string> spellDefeatFields;
  std::map<std::string, std::string> spellResetFields;
  std::map<std::string, std::string> pauseStepFields;
  std::map<std::string, std::string> spellSaveLoadFields;
  std::map<std::string, std::string> acceptanceDemoFields;
  const bool jumpReceiptValid = parseReceiptFile(jumpOutput, jumpFields);
  const bool dashReceiptValid = parseReceiptFile(dashOutput, dashFields);
  const bool spellReceiptValid = parseReceiptFile(spellOutput, spellFields);
  const bool spellHitReceiptValid = parseReceiptFile(spellHitOutput, spellHitFields);
  const bool spellRecastReceiptValid =
      parseReceiptFile(spellRecastOutput, spellRecastFields);
  const bool spellDefeatReceiptValid =
      parseReceiptFile(spellDefeatOutput, spellDefeatFields);
  const bool spellResetReceiptValid =
      parseReceiptFile(spellResetOutput, spellResetFields);
  const bool pauseStepReceiptValid =
      parseReceiptFile(pauseStepOutput, pauseStepFields);
  const bool spellSaveLoadReceiptValid =
      parseReceiptFile(spellSaveLoadOutput, spellSaveLoadFields);
  const bool acceptanceDemoReceiptValid =
      parseReceiptFile(acceptanceDemoOutput, acceptanceDemoFields);
  const bool skipped =
      (jumpExitCode == 77 && jumpReceiptValid && hasField(jumpFields, "result", "skip")) ||
      (dashExitCode == 77 && dashReceiptValid && hasField(dashFields, "result", "skip")) ||
      (spellExitCode == 77 && spellReceiptValid && hasField(spellFields, "result", "skip")) ||
      (spellHitExitCode == 77 && spellHitReceiptValid &&
       hasField(spellHitFields, "result", "skip")) ||
      (spellRecastExitCode == 77 && spellRecastReceiptValid &&
       hasField(spellRecastFields, "result", "skip")) ||
      (spellDefeatExitCode == 77 && spellDefeatReceiptValid &&
       hasField(spellDefeatFields, "result", "skip")) ||
      (spellResetExitCode == 77 && spellResetReceiptValid &&
       hasField(spellResetFields, "result", "skip")) ||
      (pauseStepExitCode == 77 && pauseStepReceiptValid &&
       hasField(pauseStepFields, "result", "skip")) ||
      (spellSaveLoadExitCode == 77 && spellSaveLoadReceiptValid &&
       hasField(spellSaveLoadFields, "result", "skip")) ||
      (acceptanceDemoExitCode == 77 && acceptanceDemoReceiptValid &&
       hasField(acceptanceDemoFields, "result", "skip"));
  const bool jumpPassed =
      jumpExitCode == 0 && jumpControlWritten && jumpReceiptValid &&
      hasField(jumpFields, "result", "pass") && hasField(jumpFields, "backend", "vulkan") &&
      hasField(jumpFields, "input_backend", "scripted") &&
      hasField(jumpFields, "interactive_mode", "true") &&
      hasField(jumpFields, "dev_menu_enabled", "true") &&
      hasField(jumpFields, "dev_menu_open", "true") &&
      hasField(jumpFields, "dev_menu_selected_mechanic", "jump") &&
      hasField(jumpFields, "dev_menu_execute_requested", "true") &&
      hasField(jumpFields, "dev_menu_execution_status", "applied") &&
      hasField(jumpFields, "codex_control_configured", "true") &&
      hasField(jumpFields, "codex_control_read", "true") &&
      hasField(jumpFields, "codex_control_applied", "true") &&
      hasField(jumpFields, "codex_control_status", "applied") &&
      hasField(jumpFields, "player_motor_active", "true") &&
      hasField(jumpFields, "player_grounded", "false") &&
      hasField(jumpFields, "player_motor_phase", "airborne") &&
      hasField(jumpFields, "player_motor_reason", "player_motor_ok") &&
      hasField(jumpFields, "jump_input_observed", "true") &&
      hasField(jumpFields, "jump_accepted", "true") &&
      hasField(jumpFields, "air_move_intent_observed", "true") &&
      hasField(jumpFields, "air_control_active", "true") &&
      hasField(jumpFields, "horizontal_velocity_state", "positive") &&
      hasField(jumpFields, "ground_sample_valid", "true") &&
      hasField(jumpFields, "ground_contact", "false") &&
      hasField(jumpFields, "ground_walkable", "true") &&
      hasField(jumpFields, "ground_surface_id", "main_floor_walkable") &&
      hasField(jumpFields, "movement_policy_band", "flat") &&
      hasField(jumpFields, "ground_normal_y", "1.000") &&
      hasField(jumpFields, "slope_angle_degrees", "0.000") &&
      hasField(jumpFields, "slope_up_dot", "1.000") &&
      hasField(jumpFields, "speed_multiplier", "1.000") &&
      hasField(jumpFields, "debug_overlay_open", "true") &&
      hasField(jumpFields, "debug_overlay_reason", "debug_overlay_ok") &&
      hasField(jumpFields, "debug_overlay_visible", "true") &&
      hasField(jumpFields, "debug_overlay_surface", "vulkan_hud+window_title") &&
      hasField(jumpFields, "debug_title_fallback_active", "true") &&
      hasField(jumpFields, "debug_hud_projected", "true") &&
      numericFieldGreater(jumpFields, "debug_hud_line_count", 5.0F) &&
      numericFieldGreater(jumpFields, "debug_hud_glyph_count", 0.0F) &&
      hasField(jumpFields, "debug_hud_rendered", "true") &&
      hasField(jumpFields, "debug_hud_record_mode", "glyph_quads") &&
      hasField(jumpFields, "debug_player_position_available", "true") &&
      hasField(jumpFields, "debug_speed_available", "true") &&
      hasField(jumpFields, "debug_player_phase", "airborne") &&
      hasField(jumpFields, "debug_ground_sample_valid", "true") &&
      hasField(jumpFields, "debug_ground_contact", "false") &&
      hasField(jumpFields, "debug_ground_walkable", "true") &&
      hasField(jumpFields, "debug_ground_surface_id", "main_floor_walkable") &&
      hasField(jumpFields, "debug_movement_policy_band", "flat") &&
      hasField(jumpFields, "debug_ground_normal_y", "1.000") &&
      hasField(jumpFields, "debug_slope_angle_degrees", "0.000") &&
      hasField(jumpFields, "debug_slope_up_dot", "1.000") &&
      numericFieldGreater(jumpFields, "debug_horizontal_speed_meters_per_second", 0.0F) &&
      numericFieldGreater(jumpFields, "debug_distance_from_spawn_meters", 0.0F) &&
      hasField(jumpFields, "kinematic_movement_attempted", "true") &&
      hasField(jumpFields, "kinematic_movement_accepted", "true") &&
      hasField(jumpFields, "movement_reason", "movement_ok");
  const bool dashPassed =
      dashExitCode == 0 && dashControlWritten && dashReceiptValid &&
      hasField(dashFields, "result", "pass") &&
      hasField(dashFields, "dev_menu_selected_mechanic", "dash") &&
      hasField(dashFields, "dev_menu_execute_requested", "true") &&
      hasField(dashFields, "dev_menu_execution_status", "applied") &&
      hasField(dashFields, "player_motor_active", "true") &&
      hasField(dashFields, "dash_input_observed", "true") &&
      hasField(dashFields, "dash_accepted", "true") &&
      hasField(dashFields, "dash_active", "true") &&
      hasField(dashFields, "dash_cooldown_state", "cooling") &&
      hasField(dashFields, "horizontal_velocity_state", "positive") &&
      hasField(dashFields, "ground_sample_valid", "true") &&
      hasField(dashFields, "ground_contact", "true") &&
      hasField(dashFields, "ground_walkable", "true") &&
      hasField(dashFields, "ground_surface_id", "main_floor_walkable") &&
      hasField(dashFields, "movement_policy_band", "flat") &&
      hasField(dashFields, "ground_normal_y", "1.000") &&
      hasField(dashFields, "slope_angle_degrees", "0.000") &&
      hasField(dashFields, "slope_up_dot", "1.000") &&
      hasField(dashFields, "debug_overlay_open", "true") &&
      hasField(dashFields, "debug_overlay_reason", "debug_overlay_ok") &&
      hasField(dashFields, "debug_overlay_visible", "true") &&
      hasField(dashFields, "debug_overlay_surface", "vulkan_hud+window_title") &&
      hasField(dashFields, "debug_title_fallback_active", "true") &&
      hasField(dashFields, "debug_hud_projected", "true") &&
      numericFieldGreater(dashFields, "debug_hud_line_count", 5.0F) &&
      numericFieldGreater(dashFields, "debug_hud_glyph_count", 0.0F) &&
      hasField(dashFields, "debug_hud_rendered", "true") &&
      hasField(dashFields, "debug_hud_record_mode", "glyph_quads") &&
      hasField(dashFields, "debug_player_position_available", "true") &&
      hasField(dashFields, "debug_speed_available", "true") &&
      hasField(dashFields, "debug_player_phase", "grounded") &&
      hasField(dashFields, "debug_ground_sample_valid", "true") &&
      hasField(dashFields, "debug_ground_contact", "true") &&
      hasField(dashFields, "debug_ground_walkable", "true") &&
      hasField(dashFields, "debug_ground_surface_id", "main_floor_walkable") &&
      hasField(dashFields, "debug_movement_policy_band", "flat") &&
      hasField(dashFields, "debug_ground_normal_y", "1.000") &&
      hasField(dashFields, "debug_slope_angle_degrees", "0.000") &&
      hasField(dashFields, "debug_slope_up_dot", "1.000") &&
      numericFieldGreater(dashFields, "debug_horizontal_speed_meters_per_second", 0.0F) &&
      numericFieldGreater(dashFields, "debug_distance_from_spawn_meters", 0.0F);
  const bool spellProjectileStillVisible =
      hasField(spellFields, "spell_projectile_active", "true") ||
      hasField(spellFields, "spell_projectile_impact", "true");
  const bool spellPassed =
      spellExitCode == 0 && spellControlWritten && spellReceiptValid &&
      hasField(spellFields, "result", "pass") &&
      hasField(spellFields, "dev_menu_selected_mechanic", "spell") &&
      hasField(spellFields, "dev_menu_execute_requested", "true") &&
      hasField(spellFields, "dev_menu_execution_status", "applied") &&
      hasField(spellFields, "codex_control_configured", "true") &&
      hasField(spellFields, "codex_control_read", "true") &&
      hasField(spellFields, "codex_control_applied", "true") &&
      hasField(spellFields, "codex_control_status", "applied") &&
      hasField(spellFields, "spell_input_observed", "true") &&
      hasField(spellFields, "spell_projectile_spawned", "true") &&
      hasField(spellFields, "spell_projectile_visible", "true") &&
      hasField(spellFields, "ability_id", "arcane_bolt") &&
      hasField(spellFields, "ability_cast_requested", "true") &&
      hasField(spellFields, "ability_cast_accepted", "true") &&
      hasField(spellFields, "ability_cast_status", "accepted") &&
      hasField(spellFields, "ability_cast_reason", "ability_command_accepted") &&
      hasField(spellFields, "ability_resource_remaining", "2") &&
      hasField(spellFields, "ability_resource_cost", "1") &&
      hasField(spellFields, "ability_resource_max", "3") &&
      hasField(spellFields, "ability_resource_state", "recharging") &&
      hasField(spellFields, "ability_resource_next_recharge_tick", "20") &&
      hasField(spellFields, "ability_resource_recharge_remaining_ticks", "17") &&
      hasField(spellFields, "ability_cooldown_state", "cooling_down") &&
      hasField(spellFields, "ability_cooldown_ready_tick", "8") &&
      hasField(spellFields, "ability_cooldown_remaining_ticks", "5") &&
      hasField(spellFields, "ability_runtime_owned_projectile", "true") &&
      spellProjectileStillVisible &&
      hasField(spellFields, "projectile_visual_projected", "true") &&
      numericFieldGreater(spellFields, "projectile_visual_count", 0.0F) &&
      numericFieldGreater(spellFields, "projectile_marker_count", 0.0F) &&
      numericFieldGreater(spellFields, "projectile_trail_rect_count", 0.0F) &&
      hasField(spellFields, "projectile_rendered", "true") &&
      hasField(spellFields, "projectile_record_mode", "overlay_rects");
  const bool spellHitPassed =
      spellHitExitCode == 0 && spellHitControlWritten && spellHitReceiptValid &&
      hasField(spellHitFields, "result", "pass") &&
      hasField(spellHitFields, "backend", "null") &&
      hasField(spellHitFields, "input_backend", "scripted") &&
      hasField(spellHitFields, "interactive_mode", "true") &&
      hasField(spellHitFields, "dev_menu_selected_mechanic", "spell") &&
      hasField(spellHitFields, "dev_menu_execute_requested", "true") &&
      hasField(spellHitFields, "dev_menu_execution_status", "applied") &&
      hasField(spellHitFields, "spell_input_observed", "true") &&
      hasField(spellHitFields, "spell_projectile_spawned", "true") &&
      hasField(spellHitFields, "spell_projectile_impact", "true") &&
      hasField(spellHitFields, "spell_projectile_reason", "ability_entity_impact") &&
      hasField(spellHitFields, "spell_projectile_hit_surface_id",
               "entity:training_dummy") &&
      hasField(spellHitFields, "ability_id", "arcane_bolt") &&
      hasField(spellHitFields, "ability_cast_accepted", "true") &&
      hasField(spellHitFields, "ability_resource_remaining", "3") &&
      hasField(spellHitFields, "ability_resource_cost", "1") &&
      hasField(spellHitFields, "ability_resource_max", "3") &&
      hasField(spellHitFields, "ability_resource_state", "full") &&
      hasField(spellHitFields, "ability_resource_next_recharge_tick", "0") &&
      hasField(spellHitFields, "ability_resource_recharge_remaining_ticks", "0") &&
      hasField(spellHitFields, "ability_cooldown_state", "ready") &&
      hasField(spellHitFields, "ability_cooldown_ready_tick", "8") &&
      hasField(spellHitFields, "ability_cooldown_remaining_ticks", "0") &&
      hasField(spellHitFields, "ability_tick_status", "impact") &&
      hasField(spellHitFields, "ability_tick_reason", "ability_entity_impact") &&
      hasField(spellHitFields, "ability_impact_kind", "entity") &&
      hasField(spellHitFields, "ability_hit_entity", "true") &&
      hasField(spellHitFields, "ability_hit_stable_name", "training_dummy") &&
      hasField(spellHitFields, "ability_damage_applied", "true") &&
      hasField(spellHitFields, "ability_damage_amount", "3") &&
      hasField(spellHitFields, "ability_target_defeated", "false") &&
      hasField(spellHitFields, "training_dummy_combat_tracked", "true") &&
      hasField(spellHitFields, "training_dummy_hit_points", "3") &&
      hasField(spellHitFields, "training_dummy_defeated", "false");
  const bool spellRecastPassed =
      spellRecastExitCode == 0 && spellRecastControlWritten && spellRecastReceiptValid &&
      hasField(spellRecastFields, "result", "pass") &&
      hasField(spellRecastFields, "backend", "null") &&
      hasField(spellRecastFields, "input_backend", "scripted") &&
      hasField(spellRecastFields, "interactive_mode", "true") &&
      hasField(spellRecastFields, "dev_menu_selected_mechanic", "spell") &&
      hasField(spellRecastFields, "dev_menu_execute_requested", "true") &&
      hasField(spellRecastFields, "dev_menu_execution_status", "applied") &&
      hasField(spellRecastFields, "spell_input_observed", "true") &&
      hasField(spellRecastFields, "ability_id", "arcane_bolt") &&
      hasField(spellRecastFields, "ability_cast_requested", "true") &&
      hasField(spellRecastFields, "ability_cast_accepted", "true") &&
      hasField(spellRecastFields, "ability_cast_status", "accepted") &&
      hasField(spellRecastFields, "ability_cast_reason", "ability_command_accepted") &&
      hasField(spellRecastFields, "ability_cast_request_count", "3") &&
      hasField(spellRecastFields, "ability_cast_accept_count", "2") &&
      hasField(spellRecastFields, "ability_cast_reject_count", "1") &&
      hasField(spellRecastFields, "ability_slot_busy_rejected", "true") &&
      hasField(spellRecastFields, "ability_slot_busy_reject_count", "1") &&
      hasField(spellRecastFields, "ability_cooldown_rejected", "false") &&
      hasField(spellRecastFields, "ability_resource_rejected", "false") &&
      hasField(spellRecastFields, "ability_recast_accepted", "true") &&
      hasField(spellRecastFields, "ability_resource_remaining", "2") &&
      hasField(spellRecastFields, "ability_resource_cost", "1") &&
      hasField(spellRecastFields, "ability_resource_max", "3") &&
      hasField(spellRecastFields, "ability_resource_state", "recharging") &&
      hasField(spellRecastFields, "ability_resource_next_recharge_tick", "58") &&
      hasField(spellRecastFields, "ability_resource_recharge_remaining_ticks", "17") &&
      hasField(spellRecastFields, "ability_cooldown_state", "cooling_down") &&
      hasField(spellRecastFields, "ability_cooldown_ready_tick", "46") &&
      hasField(spellRecastFields, "ability_cooldown_remaining_ticks", "5") &&
      hasField(spellRecastFields, "ability_runtime_owned_projectile", "true") &&
      hasField(spellRecastFields, "spell_projectile_active", "true") &&
      hasField(spellRecastFields, "spell_projectile_visible", "true") &&
      hasField(spellRecastFields, "training_dummy_combat_tracked", "true") &&
      hasField(spellRecastFields, "training_dummy_hit_points", "3") &&
      hasField(spellRecastFields, "training_dummy_defeated", "false");
  const bool spellDefeatPassed =
      spellDefeatExitCode == 0 && spellDefeatControlWritten && spellDefeatReceiptValid &&
      hasField(spellDefeatFields, "result", "pass") &&
      hasField(spellDefeatFields, "backend", "null") &&
      hasField(spellDefeatFields, "input_backend", "scripted") &&
      hasField(spellDefeatFields, "interactive_mode", "true") &&
      hasField(spellDefeatFields, "dev_menu_selected_mechanic", "spell") &&
      hasField(spellDefeatFields, "dev_menu_execute_requested", "true") &&
      hasField(spellDefeatFields, "dev_menu_execution_status", "applied") &&
      hasField(spellDefeatFields, "spell_input_observed", "true") &&
      hasField(spellDefeatFields, "spell_projectile_spawned", "true") &&
      hasField(spellDefeatFields, "spell_projectile_active", "false") &&
      hasField(spellDefeatFields, "spell_projectile_impact", "true") &&
      hasField(spellDefeatFields, "spell_projectile_reason", "ability_entity_impact") &&
      hasField(spellDefeatFields, "spell_projectile_hit_surface_id",
               "entity:training_dummy") &&
      hasField(spellDefeatFields, "ability_id", "arcane_bolt") &&
      hasField(spellDefeatFields, "ability_cast_request_count", "3") &&
      hasField(spellDefeatFields, "ability_cast_accept_count", "2") &&
      hasField(spellDefeatFields, "ability_cast_reject_count", "1") &&
      hasField(spellDefeatFields, "ability_slot_busy_rejected", "true") &&
      hasField(spellDefeatFields, "ability_recast_accepted", "true") &&
      hasField(spellDefeatFields, "ability_resource_remaining", "3") &&
      hasField(spellDefeatFields, "ability_resource_state", "full") &&
      hasField(spellDefeatFields, "ability_resource_next_recharge_tick", "0") &&
      hasField(spellDefeatFields, "ability_cooldown_state", "ready") &&
      hasField(spellDefeatFields, "ability_tick_status", "impact") &&
      hasField(spellDefeatFields, "ability_tick_reason", "ability_entity_impact") &&
      hasField(spellDefeatFields, "ability_impact_kind", "entity") &&
      hasField(spellDefeatFields, "ability_hit_entity", "true") &&
      hasField(spellDefeatFields, "ability_hit_stable_name", "training_dummy") &&
      hasField(spellDefeatFields, "ability_damage_applied", "true") &&
      hasField(spellDefeatFields, "ability_damage_amount", "3") &&
      hasField(spellDefeatFields, "ability_target_defeated", "true") &&
      hasField(spellDefeatFields, "training_dummy_combat_tracked", "true") &&
      hasField(spellDefeatFields, "training_dummy_hit_points", "0") &&
      hasField(spellDefeatFields, "training_dummy_defeated", "true");
  const bool spellResetPassed =
      spellResetExitCode == 0 && spellResetControlWritten && spellResetReceiptValid &&
      hasField(spellResetFields, "result", "pass") &&
      hasField(spellResetFields, "backend", "null") &&
      hasField(spellResetFields, "input_backend", "scripted") &&
      hasField(spellResetFields, "interactive_mode", "true") &&
      hasField(spellResetFields, "reset_executed", "true") &&
      hasField(spellResetFields, "reset_baseline_restored", "true") &&
      hasField(spellResetFields, "reset_command_log_cleared", "true") &&
      hasField(spellResetFields, "reset_next_command_id", "1") &&
      hasField(spellResetFields, "spell_input_observed", "true") &&
      hasField(spellResetFields, "spell_projectile_spawned", "false") &&
      hasField(spellResetFields, "spell_projectile_active", "false") &&
      hasField(spellResetFields, "spell_projectile_impact", "false") &&
      hasField(spellResetFields, "ability_cast_request_count", "3") &&
      hasField(spellResetFields, "ability_cast_accept_count", "2") &&
      hasField(spellResetFields, "ability_cast_reject_count", "1") &&
      hasField(spellResetFields, "ability_slot_busy_rejected", "true") &&
      hasField(spellResetFields, "ability_recast_accepted", "true") &&
      hasField(spellResetFields, "ability_resource_remaining", "3") &&
      hasField(spellResetFields, "ability_resource_state", "full") &&
      hasField(spellResetFields, "ability_resource_next_recharge_tick", "0") &&
      hasField(spellResetFields, "ability_cooldown_state", "ready") &&
      hasField(spellResetFields, "ability_runtime_owned_projectile", "false") &&
      hasField(spellResetFields, "ability_target_defeated", "false") &&
      hasField(spellResetFields, "training_dummy_combat_tracked", "true") &&
      hasField(spellResetFields, "training_dummy_hit_points", "6") &&
      hasField(spellResetFields, "training_dummy_defeated", "false");
  const bool pauseStepPassed =
      pauseStepExitCode == 0 && pauseStepControlWritten && pauseStepReceiptValid &&
      hasField(pauseStepFields, "result", "pass") &&
      hasField(pauseStepFields, "backend", "null") &&
      hasField(pauseStepFields, "input_backend", "scripted") &&
      hasField(pauseStepFields, "interactive_mode", "true") &&
      hasField(pauseStepFields, "codex_control_configured", "true") &&
      hasField(pauseStepFields, "codex_control_read", "true") &&
      hasField(pauseStepFields, "codex_control_applied", "true") &&
      hasField(pauseStepFields, "codex_control_status", "applied") &&
      hasField(pauseStepFields, "runtime_clock_mode", "slow") &&
      hasField(pauseStepFields, "tactical_toggle_executed", "true") &&
      hasField(pauseStepFields, "tactical_mode_active", "true") &&
      hasField(pauseStepFields, "pause_executed", "true") &&
      hasField(pauseStepFields, "pause_tick_frozen", "true") &&
      hasField(pauseStepFields, "step_executed", "true") &&
      hasField(pauseStepFields, "step_tick_advanced_once", "true") &&
      hasField(pauseStepFields, "resume_executed", "true");
  const bool spellSaveLoadPassed =
      spellSaveLoadExitCode == 0 && spellSaveLoadControlWritten &&
      spellSaveLoadReceiptValid && hasField(spellSaveLoadFields, "result", "pass") &&
      hasField(spellSaveLoadFields, "backend", "null") &&
      hasField(spellSaveLoadFields, "input_backend", "scripted") &&
      hasField(spellSaveLoadFields, "interactive_mode", "true") &&
      hasField(spellSaveLoadFields, "codex_control_configured", "true") &&
      hasField(spellSaveLoadFields, "codex_control_read", "true") &&
      hasField(spellSaveLoadFields, "codex_control_applied", "true") &&
      hasField(spellSaveLoadFields, "codex_control_status", "applied") &&
      hasField(spellSaveLoadFields, "save_load_requested", "true") &&
      hasField(spellSaveLoadFields, "save_load_roundtrip_passed", "true") &&
      hasField(spellSaveLoadFields, "save_load_hash_matched", "true") &&
      hasField(spellSaveLoadFields, "save_load_session_replaced", "true") &&
      hasField(spellSaveLoadFields, "save_load_status", "ok") &&
      numericFieldGreater(spellSaveLoadFields, "save_load_encoded_bytes", 0.0F) &&
      fieldsEqual(spellSaveLoadFields, "save_load_saved_hash", "save_load_loaded_hash") &&
      hasField(spellSaveLoadFields, "spell_input_observed", "true") &&
      hasField(spellSaveLoadFields, "spell_projectile_impact", "false") &&
      hasField(spellSaveLoadFields, "ability_runtime_owned_projectile", "false") &&
      hasField(spellSaveLoadFields, "training_dummy_combat_tracked", "true") &&
      hasField(spellSaveLoadFields, "training_dummy_hit_points", "3") &&
      hasField(spellSaveLoadFields, "training_dummy_defeated", "false");
  const bool acceptanceDemoPassed =
      acceptanceDemoExitCode == 0 && acceptanceDemoControlWritten &&
      acceptanceDemoReceiptValid && hasField(acceptanceDemoFields, "result", "pass") &&
      hasField(acceptanceDemoFields, "backend", "null") &&
      hasField(acceptanceDemoFields, "input_backend", "scripted") &&
      hasField(acceptanceDemoFields, "interactive_mode", "true") &&
      hasField(acceptanceDemoFields, "codex_control_configured", "true") &&
      hasField(acceptanceDemoFields, "codex_control_read", "true") &&
      hasField(acceptanceDemoFields, "codex_control_applied", "true") &&
      hasField(acceptanceDemoFields, "codex_control_status", "applied") &&
      hasField(acceptanceDemoFields, "acceptance_demo_requested", "true") &&
      hasField(acceptanceDemoFields, "acceptance_demo_complete", "true") &&
      hasField(acceptanceDemoFields, "acceptance_pre_reset_saved_state", "true") &&
      hasField(acceptanceDemoFields, "target_discovered", "true") &&
      hasField(acceptanceDemoFields, "initial_reach_gate", "fail") &&
      hasField(acceptanceDemoFields, "reach_gate", "pass") &&
      hasField(acceptanceDemoFields, "retry_executed", "true") &&
      hasField(acceptanceDemoFields, "interaction_executed", "true") &&
      hasField(acceptanceDemoFields, "objective_complete", "true") &&
      hasField(acceptanceDemoFields, "tactical_toggle_executed", "true") &&
      hasField(acceptanceDemoFields, "tactical_mode_active", "true") &&
      hasField(acceptanceDemoFields, "pause_executed", "true") &&
      hasField(acceptanceDemoFields, "pause_tick_frozen", "true") &&
      hasField(acceptanceDemoFields, "step_executed", "true") &&
      hasField(acceptanceDemoFields, "step_tick_advanced_once", "true") &&
      hasField(acceptanceDemoFields, "resume_executed", "true") &&
      hasField(acceptanceDemoFields, "spell_input_observed", "true") &&
      hasField(acceptanceDemoFields, "ability_cast_accepted", "true") &&
      hasField(acceptanceDemoFields, "save_load_requested", "true") &&
      hasField(acceptanceDemoFields, "save_load_roundtrip_passed", "true") &&
      hasField(acceptanceDemoFields, "save_load_hash_matched", "true") &&
      hasField(acceptanceDemoFields, "save_load_session_replaced", "true") &&
      hasField(acceptanceDemoFields, "save_load_status", "ok") &&
      numericFieldGreater(acceptanceDemoFields, "save_load_encoded_bytes", 0.0F) &&
      fieldsEqual(acceptanceDemoFields, "save_load_saved_hash", "save_load_loaded_hash") &&
      hasField(acceptanceDemoFields, "reset_executed", "true") &&
      hasField(acceptanceDemoFields, "reset_baseline_restored", "true") &&
      hasField(acceptanceDemoFields, "reset_command_log_cleared", "true") &&
      hasField(acceptanceDemoFields, "reset_next_command_id", "1") &&
      hasField(acceptanceDemoFields, "ability_runtime_owned_projectile", "false") &&
      hasField(acceptanceDemoFields, "training_dummy_combat_tracked", "true") &&
      hasField(acceptanceDemoFields, "training_dummy_hit_points", "3") &&
      hasField(acceptanceDemoFields, "training_dummy_defeated", "false");
  const bool passed = jumpPassed && dashPassed && spellPassed && spellHitPassed &&
                      spellRecastPassed && spellDefeatPassed && spellResetPassed &&
                      pauseStepPassed && spellSaveLoadPassed && acceptanceDemoPassed;
#else
  const int jumpExitCode = 77;
  const int dashExitCode = 77;
  const int spellExitCode = 77;
  const int spellHitExitCode = 77;
  const int spellRecastExitCode = 77;
  const int spellDefeatExitCode = 77;
  const int spellResetExitCode = 77;
  const int pauseStepExitCode = 77;
  const int spellSaveLoadExitCode = 77;
  const int acceptanceDemoExitCode = 77;
  const bool jumpControlWritten = false;
  const bool dashControlWritten = false;
  const bool spellControlWritten = false;
  const bool spellHitControlWritten = false;
  const bool spellRecastControlWritten = false;
  const bool spellDefeatControlWritten = false;
  const bool spellResetControlWritten = false;
  const bool pauseStepControlWritten = false;
  const bool spellSaveLoadControlWritten = false;
  const bool acceptanceDemoControlWritten = false;
  const bool jumpReceiptValid = false;
  const bool dashReceiptValid = false;
  const bool spellReceiptValid = false;
  const bool spellHitReceiptValid = false;
  const bool spellRecastReceiptValid = false;
  const bool spellDefeatReceiptValid = false;
  const bool spellResetReceiptValid = false;
  const bool pauseStepReceiptValid = false;
  const bool spellSaveLoadReceiptValid = false;
  const bool acceptanceDemoReceiptValid = false;
  const bool skipped = true;
  const bool jumpPassed = false;
  const bool dashPassed = false;
  const bool spellPassed = false;
  const bool spellHitPassed = false;
  const bool spellRecastPassed = false;
  const bool spellDefeatPassed = false;
  const bool spellResetPassed = false;
  const bool pauseStepPassed = false;
  const bool spellSaveLoadPassed = false;
  const bool acceptanceDemoPassed = false;
  const bool passed = false;
#endif

  std::cout << "smoke=package_visual_codex_control\n";
  std::cout << "jump_control_written=" << (jumpControlWritten ? "true" : "false") << "\n";
  std::cout << "dash_control_written=" << (dashControlWritten ? "true" : "false") << "\n";
  std::cout << "spell_control_written=" << (spellControlWritten ? "true" : "false") << "\n";
  std::cout << "spell_hit_control_written=" << (spellHitControlWritten ? "true" : "false")
            << "\n";
  std::cout << "spell_recast_control_written="
            << (spellRecastControlWritten ? "true" : "false") << "\n";
  std::cout << "spell_defeat_control_written="
            << (spellDefeatControlWritten ? "true" : "false") << "\n";
  std::cout << "spell_reset_control_written="
            << (spellResetControlWritten ? "true" : "false") << "\n";
  std::cout << "pause_step_control_written="
            << (pauseStepControlWritten ? "true" : "false") << "\n";
  std::cout << "spell_save_load_control_written="
            << (spellSaveLoadControlWritten ? "true" : "false") << "\n";
  std::cout << "acceptance_demo_control_written="
            << (acceptanceDemoControlWritten ? "true" : "false") << "\n";
  std::cout << "jump_receipt_valid=" << (jumpReceiptValid ? "true" : "false") << "\n";
  std::cout << "dash_receipt_valid=" << (dashReceiptValid ? "true" : "false") << "\n";
  std::cout << "spell_receipt_valid=" << (spellReceiptValid ? "true" : "false") << "\n";
  std::cout << "spell_hit_receipt_valid=" << (spellHitReceiptValid ? "true" : "false")
            << "\n";
  std::cout << "spell_recast_receipt_valid="
            << (spellRecastReceiptValid ? "true" : "false") << "\n";
  std::cout << "spell_defeat_receipt_valid="
            << (spellDefeatReceiptValid ? "true" : "false") << "\n";
  std::cout << "spell_reset_receipt_valid="
            << (spellResetReceiptValid ? "true" : "false") << "\n";
  std::cout << "pause_step_receipt_valid="
            << (pauseStepReceiptValid ? "true" : "false") << "\n";
  std::cout << "spell_save_load_receipt_valid="
            << (spellSaveLoadReceiptValid ? "true" : "false") << "\n";
  std::cout << "acceptance_demo_receipt_valid="
            << (acceptanceDemoReceiptValid ? "true" : "false") << "\n";
  std::cout << "jump_exit_code=" << jumpExitCode << "\n";
  std::cout << "dash_exit_code=" << dashExitCode << "\n";
  std::cout << "spell_exit_code=" << spellExitCode << "\n";
  std::cout << "spell_hit_exit_code=" << spellHitExitCode << "\n";
  std::cout << "spell_recast_exit_code=" << spellRecastExitCode << "\n";
  std::cout << "spell_defeat_exit_code=" << spellDefeatExitCode << "\n";
  std::cout << "spell_reset_exit_code=" << spellResetExitCode << "\n";
  std::cout << "pause_step_exit_code=" << pauseStepExitCode << "\n";
  std::cout << "spell_save_load_exit_code=" << spellSaveLoadExitCode << "\n";
  std::cout << "acceptance_demo_exit_code=" << acceptanceDemoExitCode << "\n";
  std::cout << "jump_result=" << (jumpPassed ? "pass" : "fail") << "\n";
  std::cout << "dash_result=" << (dashPassed ? "pass" : "fail") << "\n";
  std::cout << "spell_result=" << (spellPassed ? "pass" : "fail") << "\n";
  std::cout << "spell_hit_result=" << (spellHitPassed ? "pass" : "fail") << "\n";
  std::cout << "spell_recast_result=" << (spellRecastPassed ? "pass" : "fail") << "\n";
  std::cout << "spell_defeat_result=" << (spellDefeatPassed ? "pass" : "fail") << "\n";
  std::cout << "spell_reset_result=" << (spellResetPassed ? "pass" : "fail") << "\n";
  std::cout << "pause_step_result=" << (pauseStepPassed ? "pass" : "fail") << "\n";
  std::cout << "spell_save_load_result=" << (spellSaveLoadPassed ? "pass" : "fail")
            << "\n";
  std::cout << "acceptance_demo_result=" << (acceptanceDemoPassed ? "pass" : "fail")
            << "\n";
  std::cout << "result=" << (passed ? "pass" : (skipped ? "skip" : "fail")) << "\n";
  std::cout << "reason_code="
            << (passed ? "package_visual_codex_control_pass"
                       : (skipped ? "package_visual_codex_control_skip"
                                  : "package_visual_codex_control_failed"))
            << "\n";
  if (passed) {
    return 0;
  }
  return skipped ? 77 : 1;
}
