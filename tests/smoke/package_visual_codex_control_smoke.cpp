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
  const bool jumpControlWritten = writeControlFile(jumpControl, "jump");
  const bool dashControlWritten = writeControlFile(dashControl, "dash");
  const bool spellControlWritten = writeControlFile(spellControl, "spell");
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
  std::map<std::string, std::string> jumpFields;
  std::map<std::string, std::string> dashFields;
  std::map<std::string, std::string> spellFields;
  const bool jumpReceiptValid = parseReceiptFile(jumpOutput, jumpFields);
  const bool dashReceiptValid = parseReceiptFile(dashOutput, dashFields);
  const bool spellReceiptValid = parseReceiptFile(spellOutput, spellFields);
  const bool skipped =
      (jumpExitCode == 77 && jumpReceiptValid && hasField(jumpFields, "result", "skip")) ||
      (dashExitCode == 77 && dashReceiptValid && hasField(dashFields, "result", "skip")) ||
      (spellExitCode == 77 && spellReceiptValid && hasField(spellFields, "result", "skip"));
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
      hasField(spellFields, "ability_runtime_owned_projectile", "true") &&
      spellProjectileStillVisible &&
      hasField(spellFields, "projectile_visual_projected", "true") &&
      numericFieldGreater(spellFields, "projectile_visual_count", 0.0F) &&
      numericFieldGreater(spellFields, "projectile_marker_count", 0.0F) &&
      numericFieldGreater(spellFields, "projectile_trail_rect_count", 0.0F) &&
      hasField(spellFields, "projectile_rendered", "true") &&
      hasField(spellFields, "projectile_record_mode", "overlay_rects");
  const bool passed = jumpPassed && dashPassed && spellPassed;
#else
  const int jumpExitCode = 77;
  const int dashExitCode = 77;
  const int spellExitCode = 77;
  const bool jumpControlWritten = false;
  const bool dashControlWritten = false;
  const bool spellControlWritten = false;
  const bool jumpReceiptValid = false;
  const bool dashReceiptValid = false;
  const bool spellReceiptValid = false;
  const bool skipped = true;
  const bool jumpPassed = false;
  const bool dashPassed = false;
  const bool spellPassed = false;
  const bool passed = false;
#endif

  std::cout << "smoke=package_visual_codex_control\n";
  std::cout << "jump_control_written=" << (jumpControlWritten ? "true" : "false") << "\n";
  std::cout << "dash_control_written=" << (dashControlWritten ? "true" : "false") << "\n";
  std::cout << "spell_control_written=" << (spellControlWritten ? "true" : "false") << "\n";
  std::cout << "jump_receipt_valid=" << (jumpReceiptValid ? "true" : "false") << "\n";
  std::cout << "dash_receipt_valid=" << (dashReceiptValid ? "true" : "false") << "\n";
  std::cout << "spell_receipt_valid=" << (spellReceiptValid ? "true" : "false") << "\n";
  std::cout << "jump_exit_code=" << jumpExitCode << "\n";
  std::cout << "dash_exit_code=" << dashExitCode << "\n";
  std::cout << "spell_exit_code=" << spellExitCode << "\n";
  std::cout << "jump_result=" << (jumpPassed ? "pass" : "fail") << "\n";
  std::cout << "dash_result=" << (dashPassed ? "pass" : "fail") << "\n";
  std::cout << "spell_result=" << (spellPassed ? "pass" : "fail") << "\n";
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
