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

}  // namespace

int main() {
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  const std::filesystem::path binary{IGGY3D_VISUAL_DEMO_PATH};
  const std::filesystem::path fixture =
      std::filesystem::current_path() / "fixtures/demos/first_room/package.iggy3d.toml";
  const std::filesystem::path output = "/tmp/iggy3d_package_visual_window.out";
  const std::string command = shellQuote(binary) + " --package " + shellQuote(fixture) +
                              " --renderer null --frames 1 --window --print-render-receipt > " +
                              shellQuote(output);

  const int exitCode =
      std::filesystem::exists(binary) ? exitCodeFromSystem(std::system(command.c_str())) : 1;
  std::map<std::string, std::string> fields;
  const bool receiptValid = parseReceiptFile(output, fields);
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
                      hasField(fields, "frames_presented", "1");
#else
  const int exitCode = 77;
  const bool receiptValid = false;
  const bool skipped = true;
  const bool passed = false;
#endif

  std::cout << "smoke=package_visual_window\n";
  std::cout << "receipt_valid=" << (receiptValid ? "true" : "false") << "\n";
  std::cout << "actual_exit_code=" << exitCode << "\n";
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
