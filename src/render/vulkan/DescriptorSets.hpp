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

struct DescriptorPoolRecord {
  VkDescriptorPool pool{};
  std::string poolName;
  std::uint32_t maxSets = 0U;
};

struct MaterialDescriptorBinding {
  std::uint32_t setNumber = 1U;
  std::uint32_t binding = 0U;
  std::string bindingName = "combined_image_sampler_base_color";
};

struct MaterialDescriptorRecord {
  VkDescriptorSet set{};
  std::string materialId;
  std::string textureId;
  std::string samplerKey;
};

struct DescriptorLayoutCreateInfo {
  VkDevice device{};
  DescriptorSetLayoutKey key;
};

struct DescriptorLayoutResult {
  DescriptorSetLayoutRecord record;
  RenderOutcome outcome = RenderOutcome::Ok;
  RenderReason reason{"packet6_resource_ready", "packet 6 resource ready"};
  RenderReceipt receipt;
};

std::string_view descriptorLayoutFamilyName(DescriptorLayoutFamily family);
DescriptorSetLayoutRecord firstRoomDescriptorBaseline();
MaterialDescriptorBinding materialUnlitTexturedBinding();
DescriptorLayoutResult createMaterialUnlitTexturedDescriptorLayout(
    const DescriptorLayoutCreateInfo& createInfo);
RenderReceipt destroyDescriptorSetLayout(VkDevice device, DescriptorSetLayoutRecord& record);

}  // namespace iggy3d::vulkan
