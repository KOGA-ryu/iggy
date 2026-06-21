#include "render/RenderDiagnostics.hpp"
#include "render/vulkan/DebugValidation.hpp"

#include <iostream>

int main() {
  iggy3d::vulkan::DebugValidation validation;
  iggy3d::vulkan::DebugValidationConfig config;
#if defined(IGGY3D_REQUIRE_VALIDATION_LAYERS_ENABLED)
  config.validationMode = iggy3d::vulkan::ValidationRequestMode::Required;
  config.strictVulkan = true;
#else
  config.validationMode = iggy3d::vulkan::ValidationRequestMode::Optional;
#endif
  config.debugMessengerRequested = true;
  iggy3d::vulkan::DebugValidationPlan plan = validation.buildPlan(config);
  iggy3d::RenderReceipt receipt = plan.receipt;
  iggy3d::appendReceiptField(receipt, "smoke", "vulkan_validation");
  const bool pass = !iggy3d::hasReceiptField(receipt, "result", "fail");
  if (!pass) {
    std::cout << iggy3d::formatRenderReceipt(receipt);
    return config.strictVulkan ? 1 : 77;
  }
  std::cout << iggy3d::formatRenderReceipt(receipt);
  return 0;
}
