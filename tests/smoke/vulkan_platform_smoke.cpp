#include "render/RenderDiagnostics.hpp"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#if defined(IGGY3D_HAS_SDL3)
#include "app/platform/SdlWindow.hpp"
#endif

#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_HAS_VULKAN)
#include "app/platform/SdlVulkanSurface.hpp"
#endif

namespace {

bool strictSmoke() {
#if defined(IGGY3D_REQUIRE_VULKAN_SMOKE_ENABLED)
  return true;
#else
  return false;
#endif
}

std::string platformName() {
#if defined(__APPLE__)
  return "macos";
#elif defined(__linux__)
  return "linux";
#elif defined(_WIN32)
  return "windows";
#else
  return "unknown";
#endif
}

#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_HAS_VULKAN)
std::string joinNames(const std::vector<std::string>& names) {
  std::string joined;
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (i != 0U) {
      joined.push_back(',');
    }
    joined += names[i];
  }
  return joined;
}
#endif

iggy3d::RenderReceipt baseReceipt(std::string_view mode,
                                  std::string_view result,
                                  std::string_view reason) {
  iggy3d::RenderReceipt receipt;
  iggy3d::appendReceiptField(receipt, "receipt_version", "1");
  iggy3d::appendReceiptField(receipt, "repo", "iggy3d");
  iggy3d::appendReceiptField(receipt, "smoke", "vulkan_platform");
  iggy3d::appendReceiptField(receipt, "platform", platformName());
  iggy3d::appendReceiptField(receipt, "window_shell", "sdl3");
  iggy3d::appendReceiptField(receipt, "mode", mode);
  iggy3d::appendReceiptField(receipt, "surface_provider", "sdl3");
  iggy3d::appendReceiptField(receipt, "surface_create_attempted", false);
  iggy3d::appendReceiptField(receipt, "surface_created", false);
  iggy3d::appendReceiptField(receipt, "strict_vulkan", strictSmoke());
  iggy3d::appendReceiptField(receipt, "result", result);
  iggy3d::appendReceiptField(receipt, "reason_code", reason);
  return receipt;
}

int printReceipt(const iggy3d::RenderReceipt& receipt, bool ok) {
  std::cout << iggy3d::formatRenderReceipt(receipt);
  return ok ? 0 : 1;
}

}  // namespace

int main(int argc, const char* const* argv) {
  std::string mode = "window_only";
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg{argv[i]};
    if (arg == "--mode" && i + 1 < argc) {
      mode = argv[++i];
    } else if (arg == "--strict") {
      continue;
    }
  }

#if !defined(IGGY3D_HAS_SDL3)
  iggy3d::RenderReceipt receipt =
      baseReceipt(mode, strictSmoke() ? "fail" : "skip", "vulkan_platform_display_unavailable");
  return printReceipt(receipt, !strictSmoke());
#else
  iggy3d::SdlWindowCreateInfo create;
  create.title = "iggy3d platform smoke";
  create.width = 640U;
  create.height = 360U;
  create.vulkan = mode == "extension_query";
  iggy3d::SdlWindow window(create);
  window.pollEvents();
  const iggy3d::SdlDrawableExtent extent = window.drawableExtent();
  if (!window.isOpen()) {
    iggy3d::RenderReceipt receipt =
        baseReceipt(mode, strictSmoke() ? "fail" : "skip", "vulkan_platform_sdl_window_failed");
    iggy3d::appendReceiptField(receipt, "drawable", false);
    iggy3d::appendReceiptField(receipt, "window_width", static_cast<std::uint64_t>(0));
    iggy3d::appendReceiptField(receipt, "window_height", static_cast<std::uint64_t>(0));
    iggy3d::appendReceiptField(receipt, "drawable_width", static_cast<std::uint64_t>(0));
    iggy3d::appendReceiptField(receipt, "drawable_height", static_cast<std::uint64_t>(0));
    iggy3d::appendReceiptField(receipt, "required_instance_extensions", "");
    return printReceipt(receipt, !strictSmoke());
  }

  iggy3d::RenderReceipt receipt = baseReceipt(mode, "pass", "vulkan_platform_ok");
  iggy3d::appendReceiptField(receipt, "drawable", window.isDrawable());
  iggy3d::appendReceiptField(receipt, "window_width",
                             static_cast<std::uint64_t>(window.eventState().windowWidth));
  iggy3d::appendReceiptField(receipt, "window_height",
                             static_cast<std::uint64_t>(window.eventState().windowHeight));
  iggy3d::appendReceiptField(receipt, "drawable_width", static_cast<std::uint64_t>(extent.width));
  iggy3d::appendReceiptField(receipt, "drawable_height",
                             static_cast<std::uint64_t>(extent.height));

  if (mode == "extension_query") {
#if defined(IGGY3D_HAS_VULKAN)
    const iggy3d::SdlVulkanSurfaceProvider provider;
    const iggy3d::SdlVulkanExtensionList extensions =
        provider.requiredInstanceExtensions(window);
    if (extensions.outcome != iggy3d::RenderOutcome::Ok) {
      iggy3d::RenderReceipt failure =
          baseReceipt(mode, strictSmoke() ? "fail" : "skip",
                      "vulkan_platform_extensions_unavailable");
      iggy3d::appendReceiptField(failure, "drawable", window.isDrawable());
      iggy3d::appendReceiptField(failure, "required_instance_extensions", "");
      return printReceipt(failure, !strictSmoke());
    }
    iggy3d::appendReceiptField(receipt, "required_instance_extensions",
                               joinNames(extensions.names));
#else
    iggy3d::RenderReceipt failure =
        baseReceipt(mode, strictSmoke() ? "fail" : "skip",
                    "vulkan_platform_extensions_unavailable");
    iggy3d::appendReceiptField(failure, "drawable", window.isDrawable());
    iggy3d::appendReceiptField(failure, "required_instance_extensions", "");
    return printReceipt(failure, !strictSmoke());
#endif
  } else {
    iggy3d::appendReceiptField(receipt, "required_instance_extensions", "");
  }

  return printReceipt(receipt, true);
#endif
}
