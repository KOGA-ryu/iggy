#include "render/RenderDiagnostics.hpp"
#include "render/vulkan/DescriptorSets.hpp"

#include <iostream>
#include <string_view>

namespace {

iggy3d::RenderReceipt baseReceipt(std::string_view result, std::string_view reason) {
  iggy3d::RenderReceipt receipt;
  iggy3d::appendReceiptField(receipt, "receipt_version", "1");
  iggy3d::appendReceiptField(receipt, "repo", "iggy3d");
  iggy3d::appendReceiptField(receipt, "smoke", "vulkan_descriptor");
  iggy3d::appendReceiptField(receipt, "packet_order", "6");
  iggy3d::appendReceiptField(receipt, "backend", "vulkan");
  iggy3d::appendReceiptField(receipt, "descriptor_layout_family", "none");
  iggy3d::appendReceiptField(receipt, "descriptor_set_layout_count", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "descriptor_sets", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "result", result);
  iggy3d::appendReceiptField(receipt, "reason_code", reason);
  return receipt;
}

}  // namespace

int main() {
  const iggy3d::vulkan::DescriptorSetLayoutRecord baseline =
      iggy3d::vulkan::firstRoomDescriptorBaseline();
  iggy3d::RenderReceipt receipt =
      baseReceipt(baseline.bindingCount == 0U ? "pass" : "fail",
                  baseline.bindingCount == 0U ? "packet6_resource_ready"
                                               : "descriptor_layout_create_failed");
  const iggy3d::vulkan::MaterialDescriptorBinding material =
      iggy3d::vulkan::materialUnlitTexturedBinding();
  iggy3d::appendReceiptField(receipt, "descriptor_layout_family", "material_unlit_textured");
  iggy3d::appendReceiptField(receipt, "material_binding", static_cast<std::uint64_t>(material.binding));
  iggy3d::appendReceiptField(receipt, "material_descriptor_deferred", true);
  std::cout << iggy3d::formatRenderReceipt(receipt);
  return baseline.bindingCount == 0U ? 0 : 1;
}
