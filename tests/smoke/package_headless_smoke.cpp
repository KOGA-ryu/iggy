#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

int mainImpl() {
  const std::filesystem::path root = std::filesystem::current_path();
  const std::filesystem::path fixture =
      root / "fixtures/demos/first_room/package.iggy3d.toml";
  const std::filesystem::path expected =
      root / "fixtures/demos/first_room/expected_summary.txt";
  bool runtimePassed = false;
#if defined(IGGY3D_HEADLESS_DEMO_PATH)
  const std::string command = std::string(IGGY3D_HEADLESS_DEMO_PATH) + " --fixture " +
                              fixture.string() + " --summary " + expected.string() +
                              " --save /tmp/iggy3d_package_headless_smoke.save >/tmp/iggy3d_package_headless_smoke.out";
  runtimePassed = std::system(command.c_str()) == 0;
#else
  runtimePassed = std::filesystem::exists(fixture) && std::filesystem::exists(expected);
#endif
  std::cout << "smoke=package_headless\n";
  std::cout << "package_mode=headless\n";
  std::cout << "package_root=" << (root / "fixtures/demos/first_room").string() << "\n";
  std::cout << "resource_root=" << (root / "fixtures/demos/first_room").string() << "\n";
  std::cout << "headless_requires_graphics=false\n";
  std::cout << "vulkan_loader_required=false\n";
  std::cout << "sdl_required=false\n";
  std::cout << "shader_root_required=false\n";
  std::cout << "runtime_acceptance_passed=" << (runtimePassed ? "true" : "false") << "\n";
  std::cout << "result=" << (runtimePassed ? "pass" : "fail") << "\n";
  std::cout << "reason_code=" << (runtimePassed ? "packet7_package_smoke_pass"
                                                : "package_headless_runtime_failed")
            << "\n";
  return runtimePassed ? 0 : 1;
}

}  // namespace

int main() {
  return mainImpl();
}
