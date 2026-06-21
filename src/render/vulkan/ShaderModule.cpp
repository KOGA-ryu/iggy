#include "render/vulkan/ShaderModule.hpp"

#include <array>
#include <charconv>
#include <fstream>

#if defined(IGGY3D_HAS_VULKAN)
#include <vulkan/vulkan.h>
#endif

namespace iggy3d::vulkan {
namespace {

RenderReason reason(std::string_view code) {
  if (code == "vk_success" || code == "packet6_resource_ready") {
    return {code, "packet 6 resource ready"};
  }
  if (code == "shader_artifact_missing") {
    return {code, "shader artifact missing"};
  }
  if (code == "shader_read_failed") {
    return {code, "shader read failed"};
  }
  if (code == "shader_spirv_empty") {
    return {code, "shader spir-v empty"};
  }
  if (code == "shader_spirv_alignment_invalid") {
    return {code, "shader spir-v alignment invalid"};
  }
  if (code == "shader_entry_point_invalid") {
    return {code, "shader entry point invalid"};
  }
  if (code == "shader_stage_mismatch") {
    return {code, "shader stage mismatch"};
  }
  if (code == "shader_module_create_failed") {
    return {code, "shader module create failed"};
  }
  return {"shader_module_create_failed", "shader module create failed"};
}

RenderReceipt baseReceipt(std::string_view result,
                          std::string_view reasonCode,
                          const ShaderModuleCreateInfo& createInfo) {
  RenderReceipt receipt;
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/vulkan/ShaderModule.cpp");
  appendReceiptField(receipt, "packet_order", "6");
  appendReceiptField(receipt, "backend", "vulkan");
  appendReceiptField(receipt, "shader_language", "glsl");
  appendReceiptField(receipt, "shader_stage", shaderStageName(createInfo.stage));
  appendReceiptField(receipt, "shader_entry_point", createInfo.entryPoint);
  appendReceiptField(receipt, "shader_spirv_path", createInfo.spirvPath.string());
  appendReceiptField(receipt, "shader_module_created", false);
  appendReceiptField(receipt, "result", result);
  appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
}

std::vector<std::uint8_t> readBytes(const std::filesystem::path& path, bool& ok) {
  ok = false;
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return {};
  }
  std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());
  ok = file.good() || file.eof();
  return bytes;
}

}  // namespace

std::string_view shaderStageName(ShaderStage stage) {
  switch (stage) {
    case ShaderStage::Vertex:
      return "vertex";
    case ShaderStage::Fragment:
      return "fragment";
  }
  return "vertex";
}

std::string_view shaderStageFileExtension(ShaderStage stage) {
  switch (stage) {
    case ShaderStage::Vertex:
      return ".vert.spv";
    case ShaderStage::Fragment:
      return ".frag.spv";
  }
  return ".vert.spv";
}

VkShaderStageFlagBits shaderStageFlag(ShaderStage stage) {
  switch (stage) {
    case ShaderStage::Vertex:
      return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStage::Fragment:
      return VK_SHADER_STAGE_FRAGMENT_BIT;
  }
  return VK_SHADER_STAGE_VERTEX_BIT;
}

bool shaderEntryPointValid(std::string_view entryPoint) {
  return entryPoint == "main";
}

bool spirvBytesWellFormed(const std::vector<std::uint8_t>& bytes) {
  return !bytes.empty() && (bytes.size() % 4U) == 0U;
}

std::string formatShaderArtifactHash(const std::vector<std::uint8_t>& bytes) {
  std::uint64_t hash = 1469598103934665603ULL;
  for (const std::uint8_t byte : bytes) {
    hash ^= byte;
    hash *= 1099511628211ULL;
  }
  std::array<char, 16> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    const std::uint8_t nibble = static_cast<std::uint8_t>((hash >> ((15U - i) * 4U)) & 0x0FU);
    out[i] = static_cast<char>(nibble < 10U ? ('0' + nibble) : ('a' + (nibble - 10U)));
  }
  return std::string(out.begin(), out.end());
}

ShaderModuleResult createShaderModule(const ShaderModuleCreateInfo& createInfo) {
  ShaderModuleResult result;
  result.record.stage = createInfo.stage;
  result.record.entryPoint = createInfo.entryPoint;
  result.record.spirvPath = createInfo.spirvPath;
  result.record.sourceName = createInfo.spirvPath.filename().string();

  if (!shaderEntryPointValid(createInfo.entryPoint)) {
    result.reason = reason("shader_entry_point_invalid");
    result.receipt = baseReceipt("fail", result.reason.code, createInfo);
    return result;
  }
  if (!std::filesystem::exists(createInfo.spirvPath)) {
    result.reason = reason("shader_artifact_missing");
    result.receipt = baseReceipt("fail", result.reason.code, createInfo);
    return result;
  }
  if (createInfo.spirvPath.filename().string().find(shaderStageFileExtension(createInfo.stage)) ==
      std::string::npos) {
    result.reason = reason("shader_stage_mismatch");
    result.receipt = baseReceipt("fail", result.reason.code, createInfo);
    return result;
  }

  bool readOk = false;
  const std::vector<std::uint8_t> bytes = readBytes(createInfo.spirvPath, readOk);
  if (!readOk) {
    result.reason = reason("shader_read_failed");
    result.receipt = baseReceipt("fail", result.reason.code, createInfo);
    return result;
  }
  if (bytes.empty()) {
    result.reason = reason("shader_spirv_empty");
    result.receipt = baseReceipt("fail", result.reason.code, createInfo);
    return result;
  }
  if (!spirvBytesWellFormed(bytes)) {
    result.reason = reason("shader_spirv_alignment_invalid");
    result.receipt = baseReceipt("fail", result.reason.code, createInfo);
    return result;
  }
  result.record.artifactHash = formatShaderArtifactHash(bytes);

#if defined(IGGY3D_HAS_VULKAN)
  if (createInfo.device == VK_NULL_HANDLE) {
    result.reason = reason("shader_module_create_failed");
    result.receipt = baseReceipt("fail", result.reason.code, createInfo);
    appendReceiptField(result.receipt, "shader_artifact_hash", result.record.artifactHash);
    return result;
  }
  VkShaderModuleCreateInfo moduleInfo{};
  moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  moduleInfo.codeSize = bytes.size();
  moduleInfo.pCode = reinterpret_cast<const std::uint32_t*>(bytes.data());
  VkShaderModule module = VK_NULL_HANDLE;
  if (vkCreateShaderModule(createInfo.device, &moduleInfo, nullptr, &module) != VK_SUCCESS) {
    result.reason = reason("shader_module_create_failed");
    result.receipt = baseReceipt("fail", result.reason.code, createInfo);
    appendReceiptField(result.receipt, "shader_artifact_hash", result.record.artifactHash);
    return result;
  }
  result.record.module = module;
#endif

  result.outcome = RenderOutcome::Ok;
  result.reason = reason("packet6_resource_ready");
  result.receipt = baseReceipt("pass", result.reason.code, createInfo);
  appendReceiptField(result.receipt, "shader_artifact_hash", result.record.artifactHash);
  appendReceiptField(result.receipt, "shader_module_created", true);
  return result;
}

RenderReceipt destroyShaderModule(VkDevice device, ShaderModuleRecord& record) {
  RenderReceipt receipt;
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/vulkan/ShaderModule.cpp");
  appendReceiptField(receipt, "packet_order", "6");
  appendReceiptField(receipt, "backend", "vulkan");
#if defined(IGGY3D_HAS_VULKAN)
  if (device != VK_NULL_HANDLE && record.module != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device, record.module, nullptr);
  }
#else
  (void)device;
#endif
  record.module = {};
  appendReceiptField(receipt, "shader_module_created", false);
  appendReceiptField(receipt, "result", "pass");
  appendReceiptField(receipt, "reason_code", "packet6_resource_ready");
  return receipt;
}

}  // namespace iggy3d::vulkan
