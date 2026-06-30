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

}  // namespace iggy3d::vulkan
