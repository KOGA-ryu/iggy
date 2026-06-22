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

bool writeControlFile(const std::filesystem::path& path) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << "dev_menu.open=true\n";
  output << "dev_menu.select=jump\n";
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
  const std::filesystem::path control = "/tmp/iggy3d_codex_control.in";
  const std::filesystem::path output = "/tmp/iggy3d_package_visual_codex_control.out";
  const bool controlWritten = writeControlFile(control);
  const std::string command =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer null --window --interactive --frames 3 --dev-menu --codex-control " +
      shellQuote(control) + " --print-render-receipt > " + shellQuote(output);

  const int exitCode =
      controlWritten && std::filesystem::exists(binary)
          ? exitCodeFromSystem(std::system(command.c_str()))
          : 1;
  std::map<std::string, std::string> fields;
  const bool receiptValid = parseReceiptFile(output, fields);
  const bool skipped = exitCode == 77 && receiptValid && hasField(fields, "result", "skip");
  const bool passed =
      exitCode == 0 && controlWritten && receiptValid && hasField(fields, "result", "pass") &&
      hasField(fields, "backend", "null") && hasField(fields, "input_backend", "scripted") &&
      hasField(fields, "interactive_mode", "true") &&
      hasField(fields, "dev_menu_enabled", "true") &&
      hasField(fields, "dev_menu_open", "true") &&
      hasField(fields, "dev_menu_selected_mechanic", "jump") &&
      hasField(fields, "dev_menu_execute_requested", "true") &&
      hasField(fields, "dev_menu_execution_status", "applied") &&
      hasField(fields, "codex_control_configured", "true") &&
      hasField(fields, "codex_control_read", "true") &&
      hasField(fields, "codex_control_applied", "true") &&
      hasField(fields, "codex_control_status", "applied") &&
      hasField(fields, "player_motor_active", "true") &&
      hasField(fields, "player_grounded", "false") &&
      hasField(fields, "player_motor_phase", "airborne") &&
      hasField(fields, "player_motor_reason", "player_motor_ok") &&
      hasField(fields, "jump_input_observed", "true") &&
      hasField(fields, "jump_accepted", "true") &&
      hasField(fields, "kinematic_movement_attempted", "true") &&
      hasField(fields, "kinematic_movement_accepted", "true") &&
      hasField(fields, "movement_reason", "movement_ok");
#else
  const int exitCode = 77;
  const bool controlWritten = false;
  const bool receiptValid = false;
  const bool skipped = true;
  const bool passed = false;
#endif

  std::cout << "smoke=package_visual_codex_control\n";
  std::cout << "control_written=" << (controlWritten ? "true" : "false") << "\n";
  std::cout << "receipt_valid=" << (receiptValid ? "true" : "false") << "\n";
  std::cout << "actual_exit_code=" << exitCode << "\n";
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
