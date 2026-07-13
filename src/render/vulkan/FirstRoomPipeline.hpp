#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
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
constexpr VkFormat VK_FORMAT_R32G32B32A32_SFLOAT = 109U;
constexpr VkFormat VK_FORMAT_R32G32_SFLOAT = 103U;
constexpr VkFormat VK_FORMAT_B8G8R8A8_SRGB = 50U;
constexpr VkFormat VK_FORMAT_D32_SFLOAT = 126U;
constexpr VkVertexInputRate VK_VERTEX_INPUT_RATE_VERTEX = 0U;
constexpr VkVertexInputRate VK_VERTEX_INPUT_RATE_INSTANCE = 1U;
#endif

namespace iggy3d::vulkan {

struct FirstRoomVertex {
  float position[3]{};
  float color[3]{};
  float uv0[2]{};
};

struct StaticMeshInstanceVertex {
  float position[3]{};
  float uv0[2]{};
  float baseColor[3]{};
  float normal[3]{};
};

struct StaticMeshInstanceTransform {
  std::array<float, 4> modelColumn0{};
  std::array<float, 4> modelColumn1{};
  std::array<float, 4> modelColumn2{};
  std::array<float, 4> modelColumn3{};
  std::array<float, 4> normalColumn0{};
  std::array<float, 4> normalColumn1{};
  std::array<float, 4> normalColumn2{};
};

inline constexpr std::uint32_t kInvalidMaterialTextureIndex =
    std::numeric_limits<std::uint32_t>::max();

struct IndexedDrawRange {
  std::uint32_t firstIndex = 0;
  std::uint32_t indexCount = 0;
  std::uint32_t materialTextureIndex = kInvalidMaterialTextureIndex;
};

struct StaticMeshInstanceBatch {
  std::uint32_t firstIndex = 0;
  std::uint32_t indexCount = 0;
  std::uint32_t materialTextureIndex = kInvalidMaterialTextureIndex;
  std::uint32_t firstInstance = 0;
  std::uint32_t instanceCount = 0;
};

constexpr std::uint32_t kFirstRoomPositionLocation = 0U;
constexpr std::uint32_t kFirstRoomUv0Location = 1U;
constexpr std::uint32_t kFirstRoomColorLocation = 2U;
constexpr std::uint32_t kStaticMeshNormalLocation = 3U;
constexpr std::uint32_t kStaticMeshModelColumn0Location = 4U;
constexpr std::uint32_t kStaticMeshModelColumn1Location = 5U;
constexpr std::uint32_t kStaticMeshModelColumn2Location = 6U;
constexpr std::uint32_t kStaticMeshModelColumn3Location = 7U;
constexpr std::uint32_t kStaticMeshNormalColumn0Location = 8U;
constexpr std::uint32_t kStaticMeshNormalColumn1Location = 9U;
constexpr std::uint32_t kStaticMeshNormalColumn2Location = 10U;
constexpr std::string_view kFirstRoomVertexFormatName =
    "FirstRoomVertex_Pos3_Color3_Uv2";
constexpr std::string_view kFirstRoomPipelineVariant =
    "first_room.vertex_color.opaque.depth.backface";
constexpr std::string_view kCreativeViewModelPipelineVariant =
    "creative_view_model.vertex_color.opaque.no_depth.backface";
constexpr std::string_view kStaticMeshMaterialPipelineVariant =
    "first_room.material_unlit_textured.opaque.depth.backface";
constexpr std::string_view kStaticMeshInstancePipelineVariant =
    "static_mesh.instanced.vertex_color.opaque.depth.backface";
constexpr std::string_view kStaticMeshInstanceMaterialPipelineVariant =
    "static_mesh.instanced.material_unlit_textured.opaque.depth.backface";

enum class FirstRoomPipelineFlavor : std::uint8_t {
  VertexColor,
  MaterialTextured,
};

enum class FirstRoomDepthMode : std::uint8_t {
  ReadWrite,
  Disabled,
};

[[nodiscard]] constexpr std::string_view firstRoomPipelineVariant(
    FirstRoomDepthMode mode) noexcept {
  return mode == FirstRoomDepthMode::Disabled
             ? kCreativeViewModelPipelineVariant
             : kFirstRoomPipelineVariant;
}

struct FirstRoomVertexFormat {
  VkVertexInputBindingDescription binding{};
  VkVertexInputAttributeDescription position{};
  VkVertexInputAttributeDescription uv0{};
  VkVertexInputAttributeDescription color{};
};

struct StaticMeshInstanceVertexFormat {
  std::array<VkVertexInputBindingDescription, 2> bindings{};
  std::array<VkVertexInputAttributeDescription, 11> attributes{};
};

struct FirstRoomPipelineCreateInfo {
  VkDevice device{};
  VkFormat colorFormat = VK_FORMAT_B8G8R8A8_SRGB;
  VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
  ShaderModuleRecord vertexShader;
  ShaderModuleRecord fragmentShader;
  PipelineLayoutRecord layout;
  FirstRoomDepthMode depthMode = FirstRoomDepthMode::ReadWrite;
  FirstRoomPipelineFlavor flavor = FirstRoomPipelineFlavor::VertexColor;
  bool instanced = false;
};

struct FirstRoomPipelineRecord {
  VkPipeline pipeline{};
  std::string variant = std::string(kFirstRoomPipelineVariant);
  VkFormat colorFormat = VK_FORMAT_B8G8R8A8_SRGB;
  VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
  FirstRoomDepthMode depthMode = FirstRoomDepthMode::ReadWrite;
  FirstRoomPipelineFlavor flavor = FirstRoomPipelineFlavor::VertexColor;
  bool instanced = false;
};

struct FirstRoomPipelineResult {
  FirstRoomPipelineRecord record;
  RenderOutcome outcome = RenderOutcome::PipelineOrShaderFailure;
  RenderReason reason{"pipeline_create_failed", "pipeline create failed"};
  RenderReceipt receipt;
};

FirstRoomVertexFormat firstRoomVertexFormat();
bool firstRoomVertexFormatMatchesShader();
StaticMeshInstanceVertexFormat staticMeshInstanceVertexFormat();
bool staticMeshInstanceVertexFormatMatchesShader();
FirstRoomPipelineResult createFirstRoomPipeline(const FirstRoomPipelineCreateInfo& createInfo);
RenderReceipt destroyFirstRoomPipeline(VkDevice device, FirstRoomPipelineRecord& record);

}  // namespace iggy3d::vulkan
