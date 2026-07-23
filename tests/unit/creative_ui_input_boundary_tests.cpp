#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

namespace {

namespace fs = std::filesystem;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

std::string readFile(const fs::path& path) {
  std::ifstream input(path, std::ios::binary);
  return {std::istreambuf_iterator<char>{input},
          std::istreambuf_iterator<char>{}};
}

bool directInputPollingHasOneOwnerPerLayer() {
  const fs::path appRoot = "apps/iggy3d_creative";
  const fs::path uiOwner = appRoot / "EditorDesktopPanels.cpp";
  const fs::path deviceOwner = appRoot / "EditorFrame.cpp";
  constexpr std::string_view kUiPollingTokens[]{
      "ImGui::IsMouseClicked",
      "ImGui::IsMouseDown",
      "ImGui::IsMouseReleased",
      "ImGui::IsMouseDoubleClicked",
      "ImGui::IsMouseDragging",
      "ImGui::IsKeyPressed",
      "io.MousePos",
      "io.MouseDelta",
      "io.MouseWheel",
      "io.AppFocusLost",
      "io.KeyShift",
      "io.KeyCtrl",
      "io.KeySuper",
  };
  constexpr std::string_view kDevicePollingTokens[]{
      "SDL_GetKeyboardState",
      "SDL_GetMouseState",
      "SDL_SCANCODE_",
      "creativeControllerButtonDown",
  };

  bool clean = true;
  for (const fs::directory_entry& entry :
       fs::recursive_directory_iterator(appRoot)) {
    if (!entry.is_regular_file() ||
        (entry.path().extension() != ".cpp" &&
         entry.path().extension() != ".hpp")) {
      continue;
    }
    const std::string source = readFile(entry.path());
    if (entry.path() != uiOwner) {
      for (std::string_view token : kUiPollingTokens) {
        if (source.find(token) != std::string::npos) {
          std::cerr << "FAIL: direct UI input poll " << token << " in "
                    << entry.path().string() << '\n';
          clean = false;
        }
      }
    }
    if (entry.path() != deviceOwner) {
      for (std::string_view token : kDevicePollingTokens) {
        if (source.find(token) != std::string::npos) {
          std::cerr << "FAIL: direct device input poll " << token << " in "
                    << entry.path().string() << '\n';
          clean = false;
        }
      }
    }
  }

  const std::string uiSource = readFile(uiOwner);
  const std::string deviceSource = readFile(deviceOwner);
  return expect(clean, "input polling stays inside the two adapters") &&
         expect(uiSource.find("sampleCreativeEditorUiInputFrame") !=
                    std::string::npos &&
                    uiSource.find("creativeInputActionPressed") !=
                        std::string::npos,
                "UI adapter samples pointer and semantic command edges") &&
         expect(deviceSource.find("kSdlKeyMappings") != std::string::npos &&
                    deviceSource.find("kControllerKeyMappings") !=
                        std::string::npos,
                "device adapter keeps keyboard and controller maps declarative");
}

}  // namespace

int main() {
  return directInputPollingHasOneOwnerPerLayer() ? EXIT_SUCCESS : EXIT_FAILURE;
}
