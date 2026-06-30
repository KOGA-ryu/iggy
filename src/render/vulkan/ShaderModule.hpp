#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "render/RenderDiagnostics.hpp"
#include "render/vulkan/VulkanTypes.hpp"

#if defined(IGGY3D_HAS_VULKAN)
#include <vulkan/vulkan.h>
#else
using VkShaderModule = void*;
enum VkShaderStageFlagBits : std::uint32_t {
  VK_SHADER_STAGE_VERTEX_BIT = 0x00000001U,
  VK_SHADER_STAGE_FRAGMENT_BIT = 0x00000010U,
};
#endif

namespace iggy3d::vulkan {

enum class ShaderStage : std::uint8_t {
  Vertex,
  Fragment,
};

struct ShaderStageInfo {
  ShaderStage stage = ShaderStage::Vertex;
  std::string entryPoint = "main";
  std::string sourceName;
};

struct ShaderModuleCreateInfo {
  VkDevice device{};
  std::filesystem::path spirvPath;
  ShaderStage stage = ShaderStage::Vertex;
  std::string entryPoint = "main";
  std::string debugName;
};

struct ShaderModuleRecord {
  VkShaderModule module{};
  ShaderStage stage = ShaderStage::Vertex;
  std::string entryPoint = "main";
  std::filesystem::path spirvPath;
  std::string sourceName;
  std::string artifactHash = "none";
};

struct ShaderModuleResult {
  ShaderModuleRecord record;
  RenderOutcome outcome = RenderOutcome::PipelineOrShaderFailure;
  RenderReason reason{"shader_artifact_missing", "shader artifact missing"};
  RenderReceipt receipt;
};

std::string_view shaderStageName(ShaderStage stage);
std::string_view shaderStageFileExtension(ShaderStage stage);
bool shaderEntryPointValid(std::string_view entryPoint);
bool spirvBytesWellFormed(const std::vector<std::uint8_t>& bytes);
std::string formatShaderArtifactHash(const std::vector<std::uint8_t>& bytes);
ShaderModuleResult createShaderModule(const ShaderModuleCreateInfo& createInfo);
RenderReceipt destroyShaderModule(VkDevice device, ShaderModuleRecord& record);

}  // namespace iggy3d::vulkan
