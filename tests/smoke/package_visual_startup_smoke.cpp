#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

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

}  // namespace

int main() {
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  constexpr bool visualBuilt = true;
#else
  constexpr bool visualBuilt = false;
#endif
  bool startupPassed = false;
  bool receiptValid = false;
#if defined(IGGY3D_VISUAL_DEMO_PATH)
  const std::filesystem::path fixture =
      std::filesystem::current_path() / "fixtures/demos/first_room/package.iggy3d.toml";
  const std::filesystem::path output = "/tmp/iggy3d_package_visual_startup.out";
  const std::string command = shellQuote(std::filesystem::path{IGGY3D_VISUAL_DEMO_PATH}) +
                              " --fixture " + shellQuote(fixture) +
                              " --renderer null --frames 1 --print-render-receipt "
                              "> " + shellQuote(output);
  startupPassed = std::system(command.c_str()) == 0;
  std::map<std::string, std::string> fields;
  receiptValid = startupPassed && parseReceiptFile(output, fields) &&
                 fields["result"] == "pass" && fields["backend"] == "null" &&
                 fields["frames_presented"] == "1";
#endif
  std::cout << "smoke=package_visual_startup\n";
  std::cout << "package_mode=build_tree_visual\n";
  std::cout << "executable_dir=unavailable\n";
  std::cout << "resource_root=unavailable\n";
  std::cout << "resource_root_source=build_tree\n";
  std::cout << "shader_root=unavailable\n";
  std::cout << "shader_root_source=build_tree\n";
  std::cout << "diagnostics_dir=unavailable\n";
  std::cout << "renderer_request=null\n";
  std::cout << "backend=" << (visualBuilt ? "null" : "unavailable") << "\n";
  std::cout << "frames_requested=1\n";
  std::cout << "frames_presented=" << (receiptValid ? 1 : 0) << "\n";
  std::cout << "visual_receipt_valid=" << (receiptValid ? "true" : "false") << "\n";
  std::cout << "result=" << (receiptValid ? "pass" : (visualBuilt ? "fail" : "skip")) << "\n";
  std::cout << "reason_code=" << (receiptValid ? "packet7_package_smoke_pass"
                                                : (visualBuilt ? "visual_demo_startup_failed"
                                                               : "visual_demo_unavailable"))
            << "\n";
  if (receiptValid) {
    return 0;
  }
  return visualBuilt ? 1 : 77;
}
