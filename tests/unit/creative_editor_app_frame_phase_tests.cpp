#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

namespace {

namespace fs = std::filesystem;

std::string readFile(const fs::path& path) {
  std::ifstream input(path, std::ios::binary);
  return {std::istreambuf_iterator<char>{input},
          std::istreambuf_iterator<char>{}};
}

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool framePhasesHaveStableOrderAndOwnership() {
  const std::string mainSource =
      readFile("apps/iggy3d_creative/main.cpp");
  constexpr std::array orderedCalls{
      std::string_view{"runCreativeEditorAppInputPhase("},
      std::string_view{"runCreativeEditorAppToolPhase("},
      std::string_view{"prepareCreativeEditorAppPreviewPhase("},
      std::string_view{"processCreativeEditorWorldInteractionFrame("},
      std::string_view{"refreshCreativeEditorAppPreviewAfterInteraction("},
      std::string_view{"runCreativeEditorAppRenderPhase("},
  };
  bool clean = true;
  std::size_t previous = 0U;
  for (std::size_t index = 0U; index < orderedCalls.size(); ++index) {
    const std::size_t found = mainSource.find(orderedCalls[index]);
    clean = expect(found != std::string::npos,
                   "main retains every frame phase call") &&
            clean;
    if (index > 0U) {
      clean = expect(found > previous,
                     "main exposes stable frame phase order") &&
              clean;
    }
    previous = found;
  }

  struct OwnerRule {
    std::string_view token;
    std::string_view owner;
  };
  constexpr std::array ownerRules{
      OwnerRule{"beginCreativeEditorFrameInput(",
                "apps/iggy3d_creative/EditorAppInputPhase.cpp"},
      OwnerRule{"buildCreativeEditorDesktopMenuBar(",
                "apps/iggy3d_creative/EditorAppToolPhase.cpp"},
      OwnerRule{"refreshCreativeEditorVolumeScenePreview(",
                "apps/iggy3d_creative/EditorAppPreviewPhase.cpp"},
      OwnerRule{"buildAndAttachCreativeEditorOverlayFrame(",
                "apps/iggy3d_creative/EditorAppRenderPhase.cpp"},
      OwnerRule{"submitCreativeEditorFrame(",
                "apps/iggy3d_creative/EditorAppRenderPhase.cpp"},
  };
  for (const OwnerRule& rule : ownerRules) {
    clean = expect(mainSource.find(rule.token) == std::string::npos,
                   "main does not retain phase-owned implementation") &&
            clean;
    clean = expect(readFile(rule.owner).find(rule.token) != std::string::npos,
                   "phase implementation remains in its declared owner") &&
            clean;
  }

  constexpr std::array phaseFiles{
      std::string_view{"apps/iggy3d_creative/EditorAppInputPhase.cpp"},
      std::string_view{"apps/iggy3d_creative/EditorAppToolPhase.cpp"},
      std::string_view{"apps/iggy3d_creative/EditorAppPreviewPhase.cpp"},
      std::string_view{"apps/iggy3d_creative/EditorAppRenderPhase.cpp"},
  };
  for (std::string_view path : phaseFiles) {
    const std::string source = readFile(path);
    clean = expect(source.find("new ") == std::string::npos &&
                       source.find("make_unique") == std::string::npos &&
                       source.find("make_shared") == std::string::npos,
                   "frame phases add no explicit per-frame heap ownership") &&
            clean;
  }
  return clean;
}

}  // namespace

int main() {
  return framePhasesHaveStableOrderAndOwnership() ? EXIT_SUCCESS
                                                  : EXIT_FAILURE;
}
