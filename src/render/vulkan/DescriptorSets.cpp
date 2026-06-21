#include "render/vulkan/DescriptorSets.hpp"

#if defined(IGGY3D_HAS_VULKAN)
#include <vulkan/vulkan.h>
#endif

namespace iggy3d::vulkan {
namespace {

RenderReceipt baseReceipt(std::string_view result,
                          std::string_view reasonCode,
                          DescriptorLayoutFamily family) {
  RenderReceipt receipt;
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/vulkan/DescriptorSets.cpp");
  appendReceiptField(receipt, "packet_order", "6");
  appendReceiptField(receipt, "backend", "vulkan");
  appendReceiptField(receipt, "descriptor_layout_family", descriptorLayoutFamilyName(family));
  appendReceiptField(receipt, "descriptor_sets", static_cast<std::uint64_t>(0));
  appendReceiptField(receipt, "result", result);
  appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
}

}  // namespace

std::string_view descriptorLayoutFamilyName(DescriptorLayoutFamily family) {
  switch (family) {
    case DescriptorLayoutFamily::None:
      return "none";
    case DescriptorLayoutFamily::MaterialUnlitTextured:
      return "material_unlit_textured";
  }
  return "none";
}

DescriptorSetLayoutRecord firstRoomDescriptorBaseline() {
  DescriptorSetLayoutRecord record;
  record.family = DescriptorLayoutFamily::None;
  record.layoutName = "none";
  record.bindingCount = 0U;
  return record;
}

MaterialDescriptorBinding materialUnlitTexturedBinding() {
  return {};
}

DescriptorLayoutResult createMaterialUnlitTexturedDescriptorLayout(
    const DescriptorLayoutCreateInfo& createInfo) {
  DescriptorLayoutResult result;
  result.record.family = DescriptorLayoutFamily::MaterialUnlitTextured;
  result.record.layoutName = "descriptor_layout.material_unlit_textured.v1";
  result.record.setNumber = 1U;
  result.record.bindingCount = 1U;
#if defined(IGGY3D_HAS_VULKAN)
  if (createInfo.device == VK_NULL_HANDLE) {
    result.outcome = RenderOutcome::PipelineOrShaderFailure;
    result.reason = {"descriptor_layout_create_failed", "descriptor layout create failed"};
    result.receipt = baseReceipt("fail", result.reason.code, result.record.family);
    return result;
  }
  VkDescriptorSetLayoutBinding binding{};
  binding.binding = 0U;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binding.descriptorCount = 1U;
  binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1U;
  layoutInfo.pBindings = &binding;
  VkDescriptorSetLayout layout = VK_NULL_HANDLE;
  if (vkCreateDescriptorSetLayout(createInfo.device, &layoutInfo, nullptr, &layout) !=
      VK_SUCCESS) {
    result.outcome = RenderOutcome::PipelineOrShaderFailure;
    result.reason = {"descriptor_layout_create_failed", "descriptor layout create failed"};
    result.receipt = baseReceipt("fail", result.reason.code, result.record.family);
    return result;
  }
  result.record.layout = layout;
#else
  (void)createInfo;
#endif
  result.receipt = baseReceipt("pass", result.reason.code, result.record.family);
  appendReceiptField(result.receipt, "descriptor_set_layout_count", static_cast<std::uint64_t>(1));
  appendReceiptField(result.receipt, "descriptor_binding_count", static_cast<std::uint64_t>(1));
  appendReceiptField(result.receipt, "material_descriptor_deferred", true);
  return result;
}

RenderReceipt destroyDescriptorSetLayout(VkDevice device, DescriptorSetLayoutRecord& record) {
  RenderReceipt receipt = baseReceipt("pass", "packet6_resource_ready", record.family);
#if defined(IGGY3D_HAS_VULKAN)
  if (device != VK_NULL_HANDLE && record.layout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device, record.layout, nullptr);
  }
#else
  (void)device;
#endif
  record.layout = {};
  return receipt;
}

}  // namespace iggy3d::vulkan
