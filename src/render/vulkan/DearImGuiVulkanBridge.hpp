#pragma once

#include <cstdint>

#include "render/vulkan/ExternalUiRecordHook.hpp"
#include "render/vulkan/VulkanFunctions.hpp"
#include "render/vulkan/VulkanTypes.hpp"

union SDL_Event;

namespace iggy3d::vulkan {

class Swapchain;

struct DearImGuiVulkanBridgeCreateInfo {
  bool enabled = false;
  void* nativeWindow = nullptr;  // SDL_Window*; required when enabled
  VkInstance instance{};
  VkPhysicalDevice physicalDevice{};
  VkDevice device{};
  std::uint32_t graphicsQueueFamily = 0;
  VkQueue graphicsQueue{};
  std::uint32_t apiVersion = 0;
  VulkanDeviceFunctions deviceFunctions;
  const Swapchain* swapchain = nullptr;
};

// Owns the ImGui context plus its SDL3 + Vulkan platform backends. Lives
// inside VulkanBackend, wired like FrameCapture. When createInfo.enabled is
// false — the --capture construction gate — no ImGui context is ever created
// and every method is a no-op returning false.
//
// Frame protocol: the app drives NewFrame via VulkanBackend::
// beginExternalUiFrame() and calls ImGui::Render() itself before submit
// (never inside the record hook — a skipped submit must still leave the
// frame Rendered). The record hook fires inside command recording and draws
// finalized draw data into its own rendering block.
//
// Swapchain churn: the record hook compares the live swapchain generation +
// color format against what the UI pipeline was built with. A format change
// skips that frame's UI pass and rebuilds the Vulkan backend objects at the
// next beginFrame (device-idle there); a same-format recreate refreshes
// MinImageCount at the next beginFrame.
class DearImGuiVulkanBridge {
public:
  DearImGuiVulkanBridge() = default;
  ~DearImGuiVulkanBridge();

  DearImGuiVulkanBridge(const DearImGuiVulkanBridge&) = delete;
  DearImGuiVulkanBridge& operator=(const DearImGuiVulkanBridge&) = delete;

  bool initialize(const DearImGuiVulkanBridgeCreateInfo& createInfo);
  void shutdown();  // caller guarantees device idle
  bool enabled() const;
  bool frameActive() const;
  bool recordedLastFrame() const;
  // ImGui's input-capture intent (valid after the frame's NewFrame; reflects
  // last frame when read at begin-frame). False when the shell is disabled.
  bool wantsMouse() const;
  bool wantsKeyboard() const;
  void processEvent(const SDL_Event& event);
  bool beginFrame();
  ExternalUiRecordHook recordHook();

private:
  static bool recordThunk(void* user, const ExternalUiRecordTarget& target);
  bool record(const ExternalUiRecordTarget& target);
  bool initVulkanBackendObjects();

  DearImGuiVulkanBridgeCreateInfo createInfo_;
  bool initialized_ = false;
  bool frameActive_ = false;
  bool recordedLastFrame_ = false;
  bool rebuildPending_ = false;
  bool minImageCountUpdatePending_ = false;
  VkFormat pipelineColorFormat_{};
  std::uint32_t cachedGeneration_ = 0;
};

}  // namespace iggy3d::vulkan
