#include "app/platform/SdlVulkanSurface.hpp"

#include <SDL3/SDL_vulkan.h>

#include <algorithm>

namespace iggy3d {
namespace {

RenderReason sdlVulkanReason(std::string_view code) {
  if (code == "sdl_vulkan_extensions_ok") {
    return {code, "sdl vulkan extensions ok"};
  }
  if (code == "sdl_vulkan_surface_create_ok") {
    return {code, "sdl vulkan surface create ok"};
  }
  if (code == "sdl_vulkan_window_not_drawable") {
    return {code, "sdl vulkan window not drawable"};
  }
  if (code == "sdl_vulkan_surface_create_failed") {
    return {code, "sdl vulkan surface create failed"};
  }
  return {"sdl_vulkan_extensions_unavailable", "sdl vulkan extensions unavailable"};
}

}  // namespace

SdlVulkanExtensionList SdlVulkanSurfaceProvider::requiredInstanceExtensions(
    const SdlWindow& window) const {
  SdlVulkanExtensionList result;
  if (!window.isOpen() || window.nativeWindow() == nullptr) {
    result.outcome = RenderOutcome::Unsupported;
    result.reason = sdlVulkanReason("sdl_vulkan_extensions_unavailable");
    return result;
  }

  Uint32 extensionCount = 0;
  const char* const* extensions =
      SDL_Vulkan_GetInstanceExtensions(&extensionCount);
  if (extensions == nullptr || extensionCount == 0U) {
    result.outcome = RenderOutcome::Unsupported;
    result.reason = sdlVulkanReason("sdl_vulkan_extensions_unavailable");
    return result;
  }

  result.names.reserve(extensionCount);
  for (Uint32 i = 0; i < extensionCount; ++i) {
    if (extensions[i] != nullptr) {
      result.names.emplace_back(extensions[i]);
    }
  }
  std::sort(result.names.begin(), result.names.end());
  result.names.erase(std::unique(result.names.begin(), result.names.end()), result.names.end());
  result.outcome = result.names.empty() ? RenderOutcome::Unsupported : RenderOutcome::Ok;
  result.reason = result.names.empty()
                      ? sdlVulkanReason("sdl_vulkan_extensions_unavailable")
                      : sdlVulkanReason("sdl_vulkan_extensions_ok");
  return result;
}

SdlVulkanSurfaceCreateResult SdlVulkanSurfaceProvider::createSurface(
    const SdlWindow& window,
    VkInstance instance) const {
  SdlVulkanSurfaceCreateResult result;
  if (!window.isDrawable() || window.nativeWindow() == nullptr) {
    result.outcome = RenderOutcome::Unsupported;
    result.reason = sdlVulkanReason("sdl_vulkan_window_not_drawable");
    return result;
  }
  if (instance == VK_NULL_HANDLE) {
    result.outcome = RenderOutcome::Unsupported;
    result.reason = sdlVulkanReason("sdl_vulkan_surface_create_failed");
    return result;
  }

  VkSurfaceKHR surface = VK_NULL_HANDLE;
  if (!SDL_Vulkan_CreateSurface(window.nativeWindow(), instance, nullptr, &surface)) {
    result.outcome = RenderOutcome::Unsupported;
    result.reason = sdlVulkanReason("sdl_vulkan_surface_create_failed");
    result.surface = VK_NULL_HANDLE;
    return result;
  }

  result.surface = surface;
  result.outcome = RenderOutcome::Ok;
  result.reason = sdlVulkanReason("sdl_vulkan_surface_create_ok");
  return result;
}

}  // namespace iggy3d
