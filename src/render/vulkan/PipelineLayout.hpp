#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "render/RenderDiagnostics.hpp"
#include "render/vulkan/VulkanTypes.hpp"

#if defined(IGGY3D_HAS_VULKAN)
#include <vulkan/vulkan.h>
#else
using VkPipelineLayout = void*;
using VkDescriptorSetLayout = void*;
using VkShaderStageFlags = std::uint32_t;
struct VkPushConstantRange {
  VkShaderStageFlags stageFlags{};
  std::uint32_t offset{};
  std::uint32_t size{};
};
#endif

namespace iggy3d::vulkan {

constexpr std::uint32_t kFirstRoomPushConstantSize = 64U;
constexpr std::uint32_t kFirstRoomDescriptorSetLayoutCount = 0U;
constexpr std::uint32_t kMaterialTextureDescriptorSetLayoutCount = 1U;

struct FirstRoomPushConstants {
  std::array<float, 16> clipFromModel{};
};

struct PipelineLayoutKey {
  std::string family = "first_room";
  std::string layout = "push_constants_only";
  std::uint32_t descriptorSetLayoutCount = kFirstRoomDescriptorSetLayoutCount;
  std::uint32_t pushConstantSize = kFirstRoomPushConstantSize;
};

struct PipelineLayoutCreateInfo {
  VkDevice device{};
  PipelineLayoutKey key;
  const VkDescriptorSetLayout* descriptorSetLayouts = nullptr;
};

struct PipelineLayoutRecord {
  VkPipelineLayout layout{};
  PipelineLayoutKey key;
  VkPushConstantRange pushConstantRange{};
};

struct PipelineLayoutResult {
  PipelineLayoutRecord record;
  RenderOutcome outcome = RenderOutcome::PipelineOrShaderFailure;
  RenderReason reason{"pipeline_layout_create_failed", "pipeline layout create failed"};
  RenderReceipt receipt;
};

static_assert(sizeof(FirstRoomPushConstants) == kFirstRoomPushConstantSize);

bool firstRoomPipelineLayoutKeyValid(const PipelineLayoutKey& key);
bool materialTexturePipelineLayoutKeyValid(const PipelineLayoutKey& key);
VkPushConstantRange firstRoomPushConstantRange();
PipelineLayoutResult createFirstRoomPipelineLayout(const PipelineLayoutCreateInfo& createInfo);
RenderReceipt destroyPipelineLayout(VkDevice device, PipelineLayoutRecord& record);

}  // namespace iggy3d::vulkan
