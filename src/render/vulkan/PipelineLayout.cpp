#include "render/vulkan/PipelineLayout.hpp"

#if defined(IGGY3D_HAS_VULKAN)
#include <vulkan/vulkan.h>
#endif

namespace iggy3d::vulkan {
namespace {

RenderReason reason(std::string_view code) {
  if (code == "packet6_resource_ready") {
    return {code, "packet 6 resource ready"};
  }
  if (code == "pipeline_push_constant_mismatch") {
    return {code, "pipeline push constant mismatch"};
  }
  return {"pipeline_layout_create_failed", "pipeline layout create failed"};
}

RenderReceipt baseReceipt(std::string_view result, std::string_view reasonCode) {
  RenderReceipt receipt;
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/vulkan/PipelineLayout.cpp");
  appendReceiptField(receipt, "packet_order", "6");
  appendReceiptField(receipt, "backend", "vulkan");
  appendReceiptField(receipt, "pipeline_family", "first_room");
  appendReceiptField(receipt, "pipeline_layout", "push_constants_only");
  appendReceiptField(receipt, "descriptor_set_layout_count",
                     static_cast<std::uint64_t>(kFirstRoomDescriptorSetLayoutCount));
  appendReceiptField(receipt, "push_constant_clip_from_model_size",
                     static_cast<std::uint64_t>(kFirstRoomPushConstantSize));
  appendReceiptField(receipt, "result", result);
  appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
}

}  // namespace

bool firstRoomPipelineLayoutKeyValid(const PipelineLayoutKey& key) {
  return key.family == "first_room" && key.layout == "push_constants_only" &&
         key.descriptorSetLayoutCount == kFirstRoomDescriptorSetLayoutCount &&
         key.pushConstantSize == kFirstRoomPushConstantSize;
}

VkPushConstantRange firstRoomPushConstantRange() {
  VkPushConstantRange range{};
  range.stageFlags = static_cast<VkShaderStageFlags>(0x00000001U);
  range.offset = 0U;
  range.size = kFirstRoomPushConstantSize;
  return range;
}

PipelineLayoutResult createFirstRoomPipelineLayout(const PipelineLayoutCreateInfo& createInfo) {
  PipelineLayoutResult result;
  result.record.key = createInfo.key;
  result.record.pushConstantRange = firstRoomPushConstantRange();
  if (!firstRoomPipelineLayoutKeyValid(createInfo.key)) {
    result.reason = reason("pipeline_push_constant_mismatch");
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
#if defined(IGGY3D_HAS_VULKAN)
  if (createInfo.device == VK_NULL_HANDLE) {
    result.reason = reason("pipeline_layout_create_failed");
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
  const VkPushConstantRange range = firstRoomPushConstantRange();
  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount = 0U;
  layoutInfo.pSetLayouts = nullptr;
  layoutInfo.pushConstantRangeCount = 1U;
  layoutInfo.pPushConstantRanges = &range;
  VkPipelineLayout layout = VK_NULL_HANDLE;
  if (vkCreatePipelineLayout(createInfo.device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
    result.reason = reason("pipeline_layout_create_failed");
    result.receipt = baseReceipt("fail", result.reason.code);
    return result;
  }
  result.record.layout = layout;
#endif
  result.outcome = RenderOutcome::Ok;
  result.reason = reason("packet6_resource_ready");
  result.receipt = baseReceipt("pass", result.reason.code);
  return result;
}

RenderReceipt destroyPipelineLayout(VkDevice device, PipelineLayoutRecord& record) {
  RenderReceipt receipt = baseReceipt("pass", "packet6_resource_ready");
#if defined(IGGY3D_HAS_VULKAN)
  if (device != VK_NULL_HANDLE && record.layout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device, record.layout, nullptr);
  }
#else
  (void)device;
#endif
  record.layout = {};
  return receipt;
}

}  // namespace iggy3d::vulkan
