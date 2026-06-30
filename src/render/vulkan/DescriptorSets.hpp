#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "render/RenderDiagnostics.hpp"
#include "render/vulkan/VulkanTypes.hpp"

#if defined(IGGY3D_HAS_VULKAN)
#include <vulkan/vulkan.h>
#else
using VkDescriptorSetLayout = void*;
using VkDescriptorPool = void*;
using VkDescriptorSet = void*;
#endif

namespace iggy3d::vulkan {

enum class DescriptorLayoutFamily : std::uint8_t {
  None,
  MaterialUnlitTextured,
};

struct DescriptorSetLayoutKey {
  DescriptorLayoutFamily family = DescriptorLayoutFamily::None;
  std::string layoutName = "none";
  std::uint32_t setNumber = 0U;
};

struct DescriptorSetLayoutRecord {
  VkDescriptorSetLayout layout{};
  DescriptorLayoutFamily family = DescriptorLayoutFamily::None;
  std::string layoutName = "none";
  std::uint32_t setNumber = 0U;
  std::uint32_t bindingCount = 0U;
};

struct MaterialDescriptorBinding {
  std::uint32_t setNumber = 1U;
  std::uint32_t binding = 0U;
  std::string bindingName = "combined_image_sampler_base_color";
};

std::string_view descriptorLayoutFamilyName(DescriptorLayoutFamily family);
DescriptorSetLayoutRecord firstRoomDescriptorBaseline();
MaterialDescriptorBinding materialUnlitTexturedBinding();

}  // namespace iggy3d::vulkan
