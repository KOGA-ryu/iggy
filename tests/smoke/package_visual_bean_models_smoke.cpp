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

bool numericFieldAtLeast(const std::map<std::string, std::string>& fields,
                         const std::string& key,
                         unsigned long minimum) {
  const auto found = fields.find(key);
  if (found == fields.end()) {
    return false;
  }
  char* end = nullptr;
  const unsigned long value = std::strtoul(found->second.c_str(), &end, 10);
  return end != found->second.c_str() && *end == '\0' && value >= minimum;
}

bool writeControlFile(const std::filesystem::path& path) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << "codex_probe.visible=true\n";
  output << "codex_probe.position=1.500,0.000,-2.500\n";
  return static_cast<bool>(output);
}

}  // namespace

int main() {
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  constexpr bool visualBuilt = true;
#else
  constexpr bool visualBuilt = false;
#endif
  bool beanPassed = false;
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  const std::filesystem::path binary{IGGY3D_VISUAL_DEMO_PATH};
  const std::filesystem::path fixture =
      std::filesystem::current_path() / "fixtures/demos/movement_playground/package.iggy3d.toml";
  const std::filesystem::path control = "/tmp/iggy3d_codex_bean_models_control.in";
  const std::filesystem::path output = "/tmp/iggy3d_package_visual_bean_models.out";
  const bool controlWritten = writeControlFile(control);
  const std::string command =
      shellQuote(binary) + " --package " + shellQuote(fixture) +
      " --renderer null --interactive --frames 2 --dev-menu --codex-control " +
      shellQuote(control) + " --print-render-receipt > " + shellQuote(output);
  const int exitCode = controlWritten && std::filesystem::exists(binary)
                           ? exitCodeFromSystem(std::system(command.c_str()))
                           : 1;
  std::map<std::string, std::string> fields;
  beanPassed = exitCode == 0 && parseReceiptFile(output, fields) &&
               hasField(fields, "result", "pass") &&
               hasField(fields, "backend", "null") &&
               hasField(fields, "codex_control_applied", "true") &&
               hasField(fields, "bean_player_model_ready", "true") &&
               hasField(fields, "bean_npc_model_ready", "true") &&
               hasField(fields, "bean_codex_probe_model_ready", "true") &&
               hasField(fields, "bean_player_visible", "true") &&
               hasField(fields, "bean_npc_visible", "true") &&
               hasField(fields, "bean_codex_probe_visible", "true") &&
               hasField(fields, "bean_codex_probe_x", "1.500") &&
               hasField(fields, "bean_codex_probe_y", "0.000") &&
               hasField(fields, "bean_codex_probe_z", "-2.500") &&
               numericFieldAtLeast(fields, "bean_model_count", 4UL);
#endif
  std::cout << "smoke=package_visual_bean_models\n";
  std::cout << "backend=" << (visualBuilt ? "null" : "unavailable") << "\n";
  std::cout << "bean_models_visible=" << (beanPassed ? "true" : "false") << "\n";
  std::cout << "result=" << (beanPassed ? "pass" : (visualBuilt ? "fail" : "skip")) << "\n";
  std::cout << "reason_code=" << (beanPassed ? "visual_bean_models_pass"
                                             : (visualBuilt ? "visual_bean_models_failed"
                                                            : "visual_demo_unavailable"))
            << "\n";
  if (beanPassed) {
    return 0;
  }
  return visualBuilt ? 1 : 77;
}
