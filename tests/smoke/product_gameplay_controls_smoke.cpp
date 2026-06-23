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
  const std::string command =
      shellQuote(binary) +
      " --no-window --scripted-gameplay-smoke --print-render-receipt > " +
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
      hasField(fields, "gameplay_input_source", "scripted") &&
      hasField(fields, "gameplay_input_used", "true") &&
      hasField(fields, "gameplay_command_submitted", "true") &&
      hasField(fields, "gameplay_command_kind", "attack") &&
      hasField(fields, "gameplay_command_status", "accepted") &&
      hasField(fields, "gameplay_command_accepted", "true") &&
      hasField(fields, "gameplay_tick_advanced", "true") &&
      hasField(fields, "player_position_changed", "true") &&
      hasField(fields, "target_discovered", "true") &&
      hasField(fields, "gameplay_reach_gate", "pass") &&
      hasField(fields, "attack_executed", "true") &&
      hasField(fields, "look_input_used", "true") &&
      hasField(fields, "camera_controller_active", "true") &&
      hasField(fields, "camera_input_source", "scripted") &&
      hasField(fields, "camera_yaw_degrees", "6.000") &&
      hasField(fields, "camera_pitch_degrees", "2.000") &&
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
