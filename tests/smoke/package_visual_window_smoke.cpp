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

bool integerFieldGreaterThan(const std::map<std::string, std::string>& fields,
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
  return value > threshold;
}

}  // namespace

int main() {
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  const std::filesystem::path binary{IGGY3D_VISUAL_DEMO_PATH};
  const std::filesystem::path fixture =
      std::filesystem::current_path() / "fixtures/demos/first_room/package.iggy3d.toml";
  const std::filesystem::path output = "/tmp/iggy3d_package_visual_window.out";
  const std::filesystem::path interactiveOutput =
      "/tmp/iggy3d_package_visual_window_interactive.out";
  const std::string command = shellQuote(binary) + " --package " + shellQuote(fixture) +
                              " --renderer null --frames 1 --window --print-render-receipt > " +
                              shellQuote(output);
  const std::string interactiveCommand =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer null --window --interactive --hold-seconds 1 --print-render-receipt > " +
      shellQuote(interactiveOutput);

  const int exitCode =
      std::filesystem::exists(binary) ? exitCodeFromSystem(std::system(command.c_str())) : 1;
  std::map<std::string, std::string> fields;
  const bool receiptValid = parseReceiptFile(output, fields);
  const int interactiveExitCode =
      std::filesystem::exists(binary) ? exitCodeFromSystem(std::system(interactiveCommand.c_str())) : 1;
  std::map<std::string, std::string> interactiveFields;
  const bool interactiveReceiptValid = parseReceiptFile(interactiveOutput, interactiveFields);
  const bool skipped = exitCode == 77 && receiptValid && hasField(fields, "result", "skip") &&
                       hasField(fields, "reason_code", "sdl3_unavailable") &&
                       hasField(fields, "window_mode", "window") &&
                       hasField(fields, "window_shell", "unavailable") &&
                       hasField(fields, "sdl3_available", "false") &&
                       hasField(fields, "frames_presented", "0");
  const bool passed = exitCode == 0 && receiptValid && hasField(fields, "result", "pass") &&
                      hasField(fields, "backend", "null") &&
                      hasField(fields, "window_mode", "window") &&
                      hasField(fields, "window_shell", "sdl3") &&
                      hasField(fields, "sdl3_available", "true") &&
                      hasField(fields, "window_created", "true") &&
                      hasField(fields, "draw_count", "0") &&
                      hasField(fields, "frames_presented", "1") &&
                      interactiveExitCode == 0 && interactiveReceiptValid &&
                      hasField(interactiveFields, "result", "pass") &&
                      hasField(interactiveFields, "backend", "null") &&
                      hasField(interactiveFields, "interactive_mode", "true") &&
                      hasField(interactiveFields, "window_mode", "window") &&
                      integerFieldGreaterThan(interactiveFields, "frames", 1ULL) &&
                      integerFieldGreaterThan(interactiveFields, "frames_presented", 1ULL);
#else
  const int exitCode = 77;
  const int interactiveExitCode = 77;
  const bool receiptValid = false;
  const bool interactiveReceiptValid = false;
  const bool skipped = true;
  const bool passed = false;
#endif

  std::cout << "smoke=package_visual_window\n";
  std::cout << "receipt_valid=" << (receiptValid ? "true" : "false") << "\n";
  std::cout << "interactive_receipt_valid=" << (interactiveReceiptValid ? "true" : "false")
            << "\n";
  std::cout << "actual_exit_code=" << exitCode << "\n";
  std::cout << "interactive_exit_code=" << interactiveExitCode << "\n";
  std::cout << "result=" << (passed ? "pass" : (skipped ? "skip" : "fail")) << "\n";
  std::cout << "reason_code="
            << (passed ? "packet_visual_window_pass"
                       : (skipped ? "sdl3_unavailable" : "visual_demo_window_failed"))
            << "\n";
  if (passed) {
    return 0;
  }
  return skipped ? 77 : 1;
}
