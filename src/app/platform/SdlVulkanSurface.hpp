#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "app/platform/SdlWindow.hpp"
#include "render/RenderDiagnostics.hpp"

namespace iggy3d {

struct SdlVulkanExtensionList {
  std::vector<std::string> names;
  RenderOutcome outcome = RenderOutcome::Unsupported;
  RenderReason reason{"sdl_vulkan_extensions_unavailable",
                      "sdl vulkan extensions unavailable"};
};

struct SdlVulkanSurfaceCreateResult {
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  RenderOutcome outcome = RenderOutcome::Unsupported;
  RenderReason reason{"sdl_vulkan_surface_create_failed", "sdl vulkan surface create failed"};
};

class SdlVulkanSurfaceProvider {
public:
  SdlVulkanExtensionList requiredInstanceExtensions(const SdlWindow& window) const;
  SdlVulkanSurfaceCreateResult createSurface(const SdlWindow& window, VkInstance instance) const;
};

}  // namespace iggy3d
