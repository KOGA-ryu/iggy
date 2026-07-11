#include "render/vulkan/VulkanResult.hpp"

#include <iostream>
#include <set>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool lowerSnake(std::string_view code) {
  if (code.empty()) {
    return false;
  }
  for (const char c : code) {
    const bool lower = c >= 'a' && c <= 'z';
    const bool digit = c >= '0' && c <= '9';
    if (!lower && !digit && c != '_') {
      return false;
    }
  }
  return true;
}

bool reasonCodesAreStable() {
  std::vector<std::string_view> codes = iggy3d::vulkan::packet4VulkanResultReasonCodes();
  const std::vector<std::string_view> startup =
      iggy3d::vulkan::packet4VulkanStartupReasonCodes();
  const std::vector<std::string_view> smoke = iggy3d::vulkan::packet4VulkanSmokeReasonCodes();
  codes.insert(codes.end(), startup.begin(), startup.end());
  codes.insert(codes.end(), smoke.begin(), smoke.end());

  std::set<std::string_view> unique;
  bool ok = true;
  for (const std::string_view code : codes) {
    ok = expect(lowerSnake(code), "reason code lower snake") && ok;
    ok = expect(unique.insert(code).second, "reason code unique") && ok;
  }
  return ok;
}

bool requiredFamiliesArePresent() {
  const std::vector<std::string_view> startupCodes =
      iggy3d::vulkan::packet4VulkanStartupReasonCodes();
  const std::set<std::string_view> startup{startupCodes.begin(), startupCodes.end()};
  return expect(startup.count("vulkan_loader_missing") == 1U, "loader reason") &&
         expect(startup.count("missing_graphics_queue") == 1U, "graphics reason") &&
         expect(startup.count("missing_present_queue") == 1U, "present reason") &&
         expect(startup.count("dynamic_rendering_required_missing") == 1U,
                "dynamic reason") &&
         expect(startup.count("debug_utils_required_missing") == 1U, "debug utils reason") &&
         expect(startup.count("vulkan_backend_no_swapchain_yet") == 1U,
                "no swapchain reason");
}

bool renderReceiptAcceptsReasons() {
  iggy3d::RenderReceipt receipt;
  iggy3d::appendReceiptField(receipt, "reason_code", "vulkan_backend_no_swapchain_yet");
  return expect(iggy3d::hasReceiptField(receipt, "reason_code",
                                        "vulkan_backend_no_swapchain_yet"),
                "receipt reason");
}

}  // namespace

int main() {
  bool ok = true;
  ok = reasonCodesAreStable() && ok;
  ok = requiredFamiliesArePresent() && ok;
  ok = renderReceiptAcceptsReasons() && ok;
  return ok ? 0 : 1;
}
