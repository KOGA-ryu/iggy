#include "render/vulkan/VulkanFeatureSupport.hpp"

#if defined(IGGY3D_ENABLE_VULKAN) && defined(IGGY3D_HAS_VULKAN)
#include "render/vulkan/InstanceDeviceSurface.hpp"
#endif

#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::vulkan::FakeVulkanDeviceCandidate baseCandidate(std::string name = "gpu") {
  iggy3d::vulkan::FakeVulkanDeviceCandidate candidate;
  candidate.identity.name = std::move(name);
  candidate.identity.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
  candidate.identity.apiVersion = (1U << 22U) | (3U << 12U);
  candidate.queues.hasGraphics = true;
  candidate.queues.hasPresent = true;
  candidate.queues.hasTransfer = true;
  candidate.queues.graphicsFamily = 0;
  candidate.queues.presentFamily = 0;
  candidate.queues.transferFamily = 1;
  candidate.queues.graphicsAndPresentSame = true;
  return candidate;
}

bool rejectsHardGates() {
  iggy3d::vulkan::VulkanFeatureBaselineRequest request;
  bool ok = true;

  iggy3d::vulkan::FakeVulkanDeviceCandidate noGraphics = baseCandidate();
  noGraphics.queues.hasGraphics = false;
  ok = expect(!iggy3d::vulkan::evaluateFakeVulkanDeviceGates(noGraphics, request).passed,
              "missing graphics rejected") &&
       ok;

  iggy3d::vulkan::FakeVulkanDeviceCandidate noPresent = baseCandidate();
  noPresent.queues.hasPresent = false;
  const iggy3d::vulkan::VulkanDeviceGateResult present =
      iggy3d::vulkan::evaluateFakeVulkanDeviceGates(noPresent, request);
  ok = expect(!present.passed, "missing present rejected") &&
       expect(iggy3d::vulkan::formatDeviceRejectionReasons(present.rejectReasons).find(
                  "missing_present_queue") != std::string::npos,
              "missing present reason") &&
       ok;

  iggy3d::vulkan::FakeVulkanDeviceCandidate noSwapchain = baseCandidate();
  noSwapchain.hasSwapchainExtension = false;
  ok = expect(!iggy3d::vulkan::evaluateFakeVulkanDeviceGates(noSwapchain, request).passed,
              "missing swapchain rejected") &&
       ok;

  iggy3d::vulkan::FakeVulkanDeviceCandidate noDynamic = baseCandidate();
  noDynamic.hasDynamicRenderingCore13 = false;
  noDynamic.hasDynamicRenderingKhr = false;
  ok = expect(!iggy3d::vulkan::evaluateFakeVulkanDeviceGates(noDynamic, request).passed,
              "missing dynamic rejected") &&
       ok;
  return ok;
}

bool softwarePolicyIsExplicit() {
  iggy3d::vulkan::FakeVulkanDeviceCandidate software = baseCandidate("software");
  software.identity.deviceType = VK_PHYSICAL_DEVICE_TYPE_CPU;
  iggy3d::vulkan::VulkanFeatureBaselineRequest request;
  const iggy3d::vulkan::VulkanDeviceGateResult rejected =
      iggy3d::vulkan::evaluateFakeVulkanDeviceGates(software, request);
  request.allowSoftwareDevice = true;
  const iggy3d::vulkan::VulkanDeviceGateResult accepted =
      iggy3d::vulkan::evaluateFakeVulkanDeviceGates(software, request);
  return expect(!rejected.passed, "software rejected") &&
         expect(accepted.passed, "software allowed explicitly");
}

bool scoringIsDeterministic() {
  iggy3d::vulkan::VulkanFeatureBaselineRequest request;
  iggy3d::vulkan::FakeVulkanDeviceCandidate b = baseCandidate("b_gpu");
  b.stableIndex = 1;
  iggy3d::vulkan::FakeVulkanDeviceCandidate a = baseCandidate("a_gpu");
  a.stableIndex = 2;
  const std::vector<iggy3d::vulkan::FakeVulkanDeviceCandidate> candidates{b, a};
  const iggy3d::vulkan::FakeVulkanDeviceSelectionResult selected =
      iggy3d::vulkan::selectFakeVulkanDevice(candidates, request);
  const iggy3d::vulkan::VulkanDeviceScore score =
      iggy3d::vulkan::scoreVulkanDevice(a.identity, a.queues, selected.gates);
  return expect(selected.selected, "device selected") &&
         expect(selected.selectedIndex == 1U, "tie breaks by name") &&
         expect(score.value == 1060, "score value");
}

bool noDevicesIsDistinct() {
  iggy3d::vulkan::VulkanFeatureBaselineRequest request;
  const iggy3d::vulkan::FakeVulkanDeviceSelectionResult selected =
      iggy3d::vulkan::selectFakeVulkanDevice({}, request);
  return expect(!selected.selected, "no device selected") &&
         expect(selected.reason.code == "no_physical_devices", "no devices reason");
}

bool initializeFailurePreservesReceipt() {
#if defined(IGGY3D_ENABLE_VULKAN) && defined(IGGY3D_HAS_VULKAN)
  iggy3d::vulkan::InstanceDeviceSurface surface;
  iggy3d::vulkan::InstanceDeviceSurfaceCreateInfo createInfo;
  createInfo.surfaceProvider.createSurface = [](VkInstance, VkSurfaceKHR* createdSurface) {
    if (createdSurface != nullptr) {
      *createdSurface = VK_NULL_HANDLE;
    }
    iggy3d::RenderReceipt receipt;
    iggy3d::appendReceiptField(receipt, "result", "fail");
    iggy3d::appendReceiptField(receipt, "reason_code", "fake_surface_failure");
    return receipt;
  };

  const iggy3d::RenderReceipt receipt = surface.initialize(createInfo);
  return expect(iggy3d::hasReceiptField(receipt, "result", "fail"),
                "initialize failure stays fail") &&
         expect(iggy3d::hasReceiptField(receipt, "reason_code", "missing_present_queue"),
                "initialize failure reason preserved") &&
         expect(!iggy3d::hasReceiptField(receipt, "result", "pass"),
                "initialize failure not overwritten by shutdown pass") &&
         expect(!iggy3d::hasReceiptField(receipt, "reason_code", "vulkan_smoke_pass"),
                "initialize failure not overwritten by shutdown reason") &&
         expect(!surface.ready(), "failed initialize not ready") &&
         expect(surface.handles().instance == VK_NULL_HANDLE, "failed initialize cleared instance") &&
         expect(surface.handles().surface == VK_NULL_HANDLE, "failed initialize cleared surface") &&
         expect(surface.handles().device == VK_NULL_HANDLE, "failed initialize cleared device");
#else
  return true;
#endif
}

}  // namespace

int main() {
  bool ok = true;
  ok = rejectsHardGates() && ok;
  ok = softwarePolicyIsExplicit() && ok;
  ok = scoringIsDeterministic() && ok;
  ok = noDevicesIsDistinct() && ok;
  ok = initializeFailurePreservesReceipt() && ok;
  return ok ? 0 : 1;
}
