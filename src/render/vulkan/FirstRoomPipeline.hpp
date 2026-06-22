#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "render/RenderDiagnostics.hpp"
#include "render/vulkan/PipelineLayout.hpp"
#include "render/vulkan/ShaderModule.hpp"
#include "render/vulkan/VulkanTypes.hpp"

#if defined(IGGY3D_HAS_VULKAN)
#include <vulkan/vulkan.h>
#else
using VkPipeline = void*;
using VkPipelineCache = void*;
using VkFormat = std::uint32_t;
using VkVertexInputRate = std::uint32_t;
using VkPrimitiveTopology = std::uint32_t;
using VkCullModeFlags = std::uint32_t;
using VkFrontFace = std::uint32_t;
using VkCompareOp = std::uint32_t;
struct VkVertexInputBindingDescription {
  std::uint32_t binding{};
  std::uint32_t stride{};
  VkVertexInputRate inputRate{};
};
struct VkVertexInputAttributeDescription {
  std::uint32_t location{};
  std::uint32_t binding{};
  VkFormat format{};
  std::uint32_t offset{};
};
constexpr VkFormat VK_FORMAT_R32G32B32_SFLOAT = 106U;
constexpr VkFormat VK_FORMAT_B8G8R8A8_SRGB = 50U;
constexpr VkFormat VK_FORMAT_D32_SFLOAT = 126U;
constexpr VkVertexInputRate VK_VERTEX_INPUT_RATE_VERTEX = 0U;
#endif

namespace iggy3d::vulkan {

struct FirstRoomVertex {
  float position[3]{};
  float color[3]{};
};

struct IndexedDrawRange {
  std::uint32_t firstIndex = 0;
  std::uint32_t indexCount = 0;
};

constexpr std::uint32_t kFirstRoomPositionLocation = 0U;
constexpr std::uint32_t kFirstRoomColorLocation = 1U;
constexpr std::string_view kFirstRoomVertexFormatName = "FirstRoomVertex_Pos3_Color3";
constexpr std::string_view kFirstRoomPipelineVariant =
    "first_room.vertex_color.opaque.depth.backface";

struct FirstRoomVertexFormat {
  VkVertexInputBindingDescription binding{};
  VkVertexInputAttributeDescription position{};
  VkVertexInputAttributeDescription color{};
};

struct FirstRoomPipelineCreateInfo {
  VkDevice device{};
  VkFormat colorFormat = VK_FORMAT_B8G8R8A8_SRGB;
  VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
  ShaderModuleRecord vertexShader;
  ShaderModuleRecord fragmentShader;
  PipelineLayoutRecord layout;
};

struct FirstRoomPipelineRecord {
  VkPipeline pipeline{};
  std::string variant = std::string(kFirstRoomPipelineVariant);
  VkFormat colorFormat = VK_FORMAT_B8G8R8A8_SRGB;
  VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
};

struct FirstRoomPipelineResult {
  FirstRoomPipelineRecord record;
  RenderOutcome outcome = RenderOutcome::PipelineOrShaderFailure;
  RenderReason reason{"pipeline_create_failed", "pipeline create failed"};
  RenderReceipt receipt;
};

FirstRoomVertexFormat firstRoomVertexFormat();
bool firstRoomVertexFormatMatchesShader();
FirstRoomPipelineResult createFirstRoomPipeline(const FirstRoomPipelineCreateInfo& createInfo);
RenderReceipt destroyFirstRoomPipeline(VkDevice device, FirstRoomPipelineRecord& record);

}  // namespace iggy3d::vulkan
