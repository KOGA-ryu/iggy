#include "render/vulkan/FirstRoomPipeline.hpp"

#include <cstddef>

#if defined(IGGY3D_HAS_VULKAN)
#include <vulkan/vulkan.h>
#endif

namespace iggy3d::vulkan {
namespace {

RenderReason reason(std::string_view code) {
  if (code == "packet6_resource_ready") {
    return {code, "packet 6 resource ready"};
  }
  if (code == "vertex_format_mismatch") {
    return {code, "vertex format mismatch"};
  }
  return {"pipeline_create_failed", "pipeline create failed"};
}

RenderReceipt baseReceipt(std::string_view result,
                          std::string_view reasonCode,
                          std::string_view variant,
                          FirstRoomDepthMode depthMode) {
  RenderReceipt receipt;
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/vulkan/FirstRoomPipeline.cpp");
  appendReceiptField(receipt, "packet_order", "6");
  appendReceiptField(receipt, "backend", "vulkan");
  appendReceiptField(receipt, "pipeline_family", "first_room");
  appendReceiptField(receipt, "pipeline_variant", variant);
  appendReceiptField(receipt, "pipeline_layout",
                     variant == kStaticMeshMaterialPipelineVariant
                         ? "material_texture"
                         : "push_constants_only");
  appendReceiptField(
      receipt, "descriptor_set_layout_count",
      static_cast<std::uint64_t>(
          variant == kStaticMeshMaterialPipelineVariant ? 1U : 0U));
  appendReceiptField(receipt, "push_constant_clip_from_model_size",
                     static_cast<std::uint64_t>(kFirstRoomPushConstantSize));
  appendReceiptField(receipt, "vertex_format", kFirstRoomVertexFormatName);
  appendReceiptField(receipt, "rendering_path", "dynamic");
  appendReceiptField(receipt, "depth_test",
                     depthMode == FirstRoomDepthMode::ReadWrite ? "enabled"
                                                                : "disabled");
  appendReceiptField(receipt, "cull_mode", "back");
  appendReceiptField(receipt, "front_face", "counter_clockwise");
  appendReceiptField(receipt, "pipeline_created", false);
  appendReceiptField(receipt, "result", result);
  appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
}

}  // namespace

FirstRoomVertexFormat firstRoomVertexFormat() {
  FirstRoomVertexFormat format;
  format.binding.binding = 0U;
  format.binding.stride = static_cast<std::uint32_t>(sizeof(FirstRoomVertex));
  format.binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  format.position.location = kFirstRoomPositionLocation;
  format.position.binding = 0U;
  format.position.format = VK_FORMAT_R32G32B32_SFLOAT;
  format.position.offset = static_cast<std::uint32_t>(offsetof(FirstRoomVertex, position));
  format.uv0.location = kFirstRoomUv0Location;
  format.uv0.binding = 0U;
  format.uv0.format = VK_FORMAT_R32G32_SFLOAT;
  format.uv0.offset = static_cast<std::uint32_t>(offsetof(FirstRoomVertex, uv0));
  format.color.location = kFirstRoomColorLocation;
  format.color.binding = 0U;
  format.color.format = VK_FORMAT_R32G32B32_SFLOAT;
  format.color.offset = static_cast<std::uint32_t>(offsetof(FirstRoomVertex, color));
  return format;
}

bool firstRoomVertexFormatMatchesShader() {
  const FirstRoomVertexFormat format = firstRoomVertexFormat();
  return format.binding.stride == sizeof(FirstRoomVertex) &&
         format.position.location == kFirstRoomPositionLocation &&
         format.uv0.location == kFirstRoomUv0Location &&
         format.color.location == kFirstRoomColorLocation &&
         format.position.offset == 0U &&
         format.color.offset == sizeof(float) * 3U &&
         format.uv0.offset == sizeof(float) * 6U;
}

FirstRoomPipelineResult createFirstRoomPipeline(const FirstRoomPipelineCreateInfo& createInfo) {
  FirstRoomPipelineResult result;
  result.record.colorFormat = createInfo.colorFormat;
  result.record.depthFormat = createInfo.depthFormat;
  result.record.depthMode = createInfo.depthMode;
  result.record.flavor = createInfo.flavor;
  result.record.variant = createInfo.flavor ==
                                  FirstRoomPipelineFlavor::MaterialTextured
                              ? std::string(kStaticMeshMaterialPipelineVariant)
                              : std::string(firstRoomPipelineVariant(
                                    createInfo.depthMode));
  const bool layoutMatches =
      createInfo.flavor == FirstRoomPipelineFlavor::MaterialTextured
          ? materialTexturePipelineLayoutKeyValid(createInfo.layout.key)
          : firstRoomPipelineLayoutKeyValid(createInfo.layout.key);
  if (!firstRoomVertexFormatMatchesShader() || !layoutMatches) {
    result.reason = reason("vertex_format_mismatch");
    result.receipt = baseReceipt("fail", result.reason.code,
                                 result.record.variant,
                                 result.record.depthMode);
    return result;
  }
#if defined(IGGY3D_HAS_VULKAN)
  if (createInfo.device == VK_NULL_HANDLE || createInfo.vertexShader.module == VK_NULL_HANDLE ||
      createInfo.fragmentShader.module == VK_NULL_HANDLE ||
      createInfo.layout.layout == VK_NULL_HANDLE) {
    result.reason = reason("pipeline_create_failed");
    result.receipt = baseReceipt("fail", result.reason.code,
                                 result.record.variant,
                                 result.record.depthMode);
    return result;
  }

  const FirstRoomVertexFormat vertexFormat = firstRoomVertexFormat();
  VkPipelineShaderStageCreateInfo stages[2]{};
  stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  stages[0].module = createInfo.vertexShader.module;
  stages[0].pName = createInfo.vertexShader.entryPoint.c_str();
  stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  stages[1].module = createInfo.fragmentShader.module;
  stages[1].pName = createInfo.fragmentShader.entryPoint.c_str();

  VkPipelineVertexInputStateCreateInfo vertexInput{};
  vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInput.vertexBindingDescriptionCount = 1U;
  vertexInput.pVertexBindingDescriptions = &vertexFormat.binding;
  VkVertexInputAttributeDescription attributes[3]{
      vertexFormat.position, vertexFormat.uv0, vertexFormat.color};
  vertexInput.vertexAttributeDescriptionCount = 3U;
  vertexInput.pVertexAttributeDescriptions = attributes;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1U;
  viewportState.scissorCount = 1U;

  VkPipelineRasterizationStateCreateInfo rasterization{};
  rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterization.polygonMode = VK_POLYGON_MODE_FILL;
  rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterization.lineWidth = 1.0F;

  VkPipelineMultisampleStateCreateInfo multisample{};
  multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo depth{};
  depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  const bool depthEnabled =
      createInfo.depthMode == FirstRoomDepthMode::ReadWrite;
  depth.depthTestEnable = depthEnabled ? VK_TRUE : VK_FALSE;
  depth.depthWriteEnable = depthEnabled ? VK_TRUE : VK_FALSE;
  depth.depthCompareOp = VK_COMPARE_OP_LESS;

  VkPipelineColorBlendAttachmentState colorAttachment{};
  colorAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                   VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  VkPipelineColorBlendStateCreateInfo colorBlend{};
  colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlend.attachmentCount = 1U;
  colorBlend.pAttachments = &colorAttachment;

  VkDynamicState dynamicStates[2]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamic{};
  dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic.dynamicStateCount = 2U;
  dynamic.pDynamicStates = dynamicStates;

  VkPipelineRenderingCreateInfo rendering{};
  rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  rendering.colorAttachmentCount = 1U;
  rendering.pColorAttachmentFormats = &createInfo.colorFormat;
  rendering.depthAttachmentFormat = createInfo.depthFormat;

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext = &rendering;
  pipelineInfo.stageCount = 2U;
  pipelineInfo.pStages = stages;
  pipelineInfo.pVertexInputState = &vertexInput;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterization;
  pipelineInfo.pMultisampleState = &multisample;
  pipelineInfo.pDepthStencilState = &depth;
  pipelineInfo.pColorBlendState = &colorBlend;
  pipelineInfo.pDynamicState = &dynamic;
  pipelineInfo.layout = createInfo.layout.layout;

  VkPipeline pipeline = VK_NULL_HANDLE;
  if (vkCreateGraphicsPipelines(createInfo.device, VK_NULL_HANDLE, 1U, &pipelineInfo, nullptr,
                                &pipeline) != VK_SUCCESS) {
    result.reason = reason("pipeline_create_failed");
    result.receipt = baseReceipt("fail", result.reason.code,
                                 result.record.variant,
                                 result.record.depthMode);
    return result;
  }
  result.record.pipeline = pipeline;
#endif
  result.outcome = RenderOutcome::Ok;
  result.reason = reason("packet6_resource_ready");
  result.receipt = baseReceipt("pass", result.reason.code,
                               result.record.variant,
                               result.record.depthMode);
  appendReceiptField(result.receipt, "pipeline_created", true);
  return result;
}

RenderReceipt destroyFirstRoomPipeline(VkDevice device, FirstRoomPipelineRecord& record) {
  RenderReceipt receipt = baseReceipt("pass", "packet6_resource_ready",
                                      record.variant, record.depthMode);
#if defined(IGGY3D_HAS_VULKAN)
  if (device != VK_NULL_HANDLE && record.pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(device, record.pipeline, nullptr);
  }
#else
  (void)device;
#endif
  record.pipeline = {};
  return receipt;
}

}  // namespace iggy3d::vulkan
