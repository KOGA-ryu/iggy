// T-0 capture regression guard (docs/creative_desktop_ui_plan.md §6).
//
// Nothing else in ctest pins `i3dc --capture`: the whole unit suite can pass
// while capture output silently changes. This smoke (a) probes for a usable
// Vulkan device exactly like the other smokes (exit 77 to self-skip),
// (b) spawns the real i3dc binary twice in capture mode, and (c) asserts the
// artifact quartet exists for both runs, the two frame hashes match
// (run-to-run determinism on this machine — deliberately NOT a checked-in
// golden hash, which would vary by device), and external_ui_recorded=0 in
// both .meta.kv files (the desktop shell must never be constructed under
// --capture — plan DL-1).

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_HAS_VULKAN)
#include "app/platform/SdlVulkanSurface.hpp"
#include "app/platform/SdlWindow.hpp"
#include "render/vulkan/VulkanBackend.hpp"
#endif

namespace {

constexpr int kSkipExitCode = 77;

std::string readTextFile(const std::filesystem::path& path) {
  std::ifstream stream(path);
  if (!stream.is_open()) {
    return {};
  }
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_HAS_VULKAN)
// Same probe shape as the vulkan_* smokes: no window/device => skip, not fail.
int probeVulkanDevice() {
  iggy3d::SdlWindowCreateInfo create;
  create.title = "iggy3d capture stability probe";
  create.width = 320U;
  create.height = 180U;
  create.vulkan = true;
  iggy3d::SdlWindow window(create);
  window.pollEvents();
  if (!window.isOpen() || !window.isDrawable()) {
    return kSkipExitCode;
  }
  iggy3d::SdlVulkanSurfaceProvider sdlProvider;
  const iggy3d::SdlVulkanExtensionList extensions =
      sdlProvider.requiredInstanceExtensions(window);
  if (extensions.outcome != iggy3d::RenderOutcome::Ok) {
    return kSkipExitCode;
  }
  const iggy3d::SdlDrawableExtent extent = window.drawableExtent();
  iggy3d::VulkanBackendCreateInfo backendInfo;
  backendInfo.config.allowSoftwareVulkan = true;
  backendInfo.drawableWidth = extent.width;
  backendInfo.drawableHeight = extent.height;
  backendInfo.surfaceProvider.requiredInstanceExtensions = extensions.names;
  backendInfo.surfaceProvider.createSurface =
      [&sdlProvider, &window](VkInstance instance, VkSurfaceKHR* surface) {
        const iggy3d::SdlVulkanSurfaceCreateResult created =
            sdlProvider.createSurface(window, instance);
        if (created.outcome == iggy3d::RenderOutcome::Ok && surface != nullptr) {
          *surface = created.surface;
        }
        iggy3d::RenderReceipt receipt;
        return receipt;
      };
  iggy3d::VulkanBackend backend(std::move(backendInfo));
  const bool ready =
      backend.lifecycleState() == iggy3d::RendererLifecycleState::Ready;
  backend.shutdown();
  return ready ? 0 : kSkipExitCode;
}
#endif

struct CaptureRunFacts {
  bool artifactsComplete = false;
  std::string frameHash;
  std::string externalUiRecorded;  // value of the .meta.kv key, "" if absent
};

CaptureRunFacts runCapture(const std::filesystem::path& pngPath) {
  CaptureRunFacts facts;
  const std::string command =
      std::string(I3DC_BINARY_PATH) + " --capture \"" +
      pngPath.generic_string() + "\"";
  const int exitCode = std::system(command.c_str());
  if (exitCode != 0) {
    std::cout << "capture_stability: i3dc exit=" << exitCode << " cmd=" << command
              << "\n";
    return facts;
  }
  const std::filesystem::path raw =
      std::filesystem::path(pngPath).replace_extension(".rgba");
  const std::filesystem::path meta =
      std::filesystem::path(pngPath).replace_extension(".meta.kv");
  const std::filesystem::path hash =
      std::filesystem::path(pngPath).replace_extension(".sha256");
  facts.artifactsComplete =
      std::filesystem::exists(pngPath) && std::filesystem::exists(raw) &&
      std::filesystem::exists(meta) && std::filesystem::exists(hash);
  facts.frameHash = readTextFile(hash);
  const std::string metaText = readTextFile(meta);
  const std::string key = "external_ui_recorded=";
  const std::size_t keyPos = metaText.find(key);
  if (keyPos != std::string::npos) {
    const std::size_t valueStart = keyPos + key.size();
    const std::size_t valueEnd = metaText.find('\n', valueStart);
    facts.externalUiRecorded = metaText.substr(
        valueStart,
        valueEnd == std::string::npos ? std::string::npos : valueEnd - valueStart);
  }
  return facts;
}

}  // namespace

int main() {
#if !defined(IGGY3D_HAS_SDL3) || !defined(IGGY3D_HAS_VULKAN)
  std::cout << "capture_stability: skip (no SDL3/Vulkan build)\n";
  return kSkipExitCode;
#else
  const int probe = probeVulkanDevice();
  if (probe != 0) {
    std::cout << "capture_stability: skip (no usable Vulkan device)\n";
    return probe;
  }

  const std::filesystem::path scratchRoot = "build/creative_capture_stability";
  std::filesystem::remove_all(scratchRoot);
  std::filesystem::create_directories(scratchRoot);

  const CaptureRunFacts runA = runCapture(scratchRoot / "capA.png");
  const CaptureRunFacts runB = runCapture(scratchRoot / "capB.png");

  bool pass = true;
  if (!runA.artifactsComplete || !runB.artifactsComplete) {
    std::cout << "capture_stability: FAIL artifact quartet incomplete (A="
              << runA.artifactsComplete << " B=" << runB.artifactsComplete
              << ")\n";
    pass = false;
  }
  if (runA.frameHash.empty() || runA.frameHash != runB.frameHash) {
    std::cout << "capture_stability: FAIL hash mismatch\n  A=" << runA.frameHash
              << "  B=" << runB.frameHash;
    pass = false;
  }
  if (runA.externalUiRecorded != "0" || runB.externalUiRecorded != "0") {
    std::cout << "capture_stability: FAIL external_ui_recorded (A='"
              << runA.externalUiRecorded << "' B='" << runB.externalUiRecorded
              << "', expected '0')\n";
    pass = false;
  }
  if (pass) {
    std::cout << "capture_stability: pass hash=" << runA.frameHash;
  }
  return pass ? 0 : 1;
#endif
}
