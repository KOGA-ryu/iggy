#pragma once

#include <cstdint>

#include "render/vulkan/VulkanTypes.hpp"

namespace iggy3d::vulkan {

// Injection point for an external UI layer (the Dear ImGui desktop shell).
// CommandRecording invokes the hook once per recorded frame AFTER the frame's
// own dynamic-rendering block has ended and BEFORE the capture-copy/present
// barriers; the color image is in COLOR_ATTACHMENT_OPTIMAL layout. The hook
// owner records a color-only LOAD_OP_LOAD rendering block of its own — the
// frame's block cannot host it because the three record paths disagree on the
// depth attachment (first-room has D32, empty/proxy have none) and a single
// UI pipeline must match exactly one rendering-info layout.
struct ExternalUiRecordTarget {
  VkCommandBuffer commandBuffer{};
  VkImage colorImage{};
  VkImageView colorImageView{};
  VkExtent2D extent{};
  std::uint32_t frameSlot = 0;
};

// Returns true when a UI pass was actually recorded into the command buffer.
// Both fields null => no external UI (the default everywhere).
struct ExternalUiRecordHook {
  bool (*record)(void* user, const ExternalUiRecordTarget& target) = nullptr;
  void* user = nullptr;
};

}  // namespace iggy3d::vulkan
