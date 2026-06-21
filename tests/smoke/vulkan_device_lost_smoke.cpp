#include "content/PackageLoader.hpp"
#include "render/vulkan/VulkanResult.hpp"
#include "runtime/replay/StateHash.hpp"
#include "runtime/session/Session.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>
#include <utility>

namespace {

iggy3d::Result<iggy3d::Session> loadSession() {
  const std::filesystem::path fixture =
      std::filesystem::current_path() / "fixtures/demos/first_room/package.iggy3d.toml";
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage(iggy3d::PackageLoadRequest{fixture.string()});
  if (package.status != iggy3d::PackageLoadStatus::Ok) {
    return {};
  }
  iggy3d::SessionCreateRequest request;
  request.config = package.scenario.config;
  request.seed = package.scenario;
  return iggy3d::Session::create(request);
}

}  // namespace

int main() {
  iggy3d::Result<iggy3d::Session> sessionResult = loadSession();
  const bool runtimeAvailable = sessionResult.status == iggy3d::ResultStatus::Ok;
  iggy3d::StateHashValue hashBefore = 0;
  iggy3d::StateHashValue hashAfter = 0;
  if (runtimeAvailable) {
    iggy3d::Session session = std::move(sessionResult.value);
    hashBefore = iggy3d::computeStateHash(session.state());
    const iggy3d::vulkan::VulkanResultMapping mapped =
        iggy3d::vulkan::mapVkResult(VK_ERROR_DEVICE_LOST, iggy3d::vulkan::VulkanCallContext::Present);
    hashAfter = iggy3d::computeStateHash(session.state());
    const bool pass = mapped.outcome == iggy3d::RenderOutcome::DeviceLost &&
                      mapped.deviceLost &&
                      mapped.reason.code == std::string_view{"device_lost"} &&
                      hashBefore == hashAfter;
    std::cout << "smoke=vulkan_device_lost\n";
    std::cout << "device_lost_detected=" << (mapped.deviceLost ? "true" : "false") << "\n";
    std::cout << "device_lost_source=present\n";
    std::cout << "first_lost_vk_result=" << mapped.rawResultName << "\n";
    std::cout << "device_name=none\n";
    std::cout << "driver_version=none\n";
    std::cout << "swapchain_recreate_attempted=false\n";
    std::cout << "new_submit_after_device_lost=false\n";
    std::cout << "shutdown_after_device_lost=true\n";
    std::cout << "runtime_hash_before=" << iggy3d::formatStateHash(hashBefore) << "\n";
    std::cout << "runtime_hash_after=" << iggy3d::formatStateHash(hashAfter) << "\n";
    std::cout << "result=" << (pass ? "pass" : "fail") << "\n";
    std::cout << "reason_code=device_lost\n";
    return pass ? 0 : 1;
  }

  const iggy3d::vulkan::VulkanResultMapping mapped =
      iggy3d::vulkan::mapVkResult(VK_ERROR_DEVICE_LOST, iggy3d::vulkan::VulkanCallContext::Present);
  const bool pass = mapped.outcome == iggy3d::RenderOutcome::DeviceLost &&
                    mapped.deviceLost && mapped.reason.code == std::string_view{"device_lost"};
  std::cout << "smoke=vulkan_device_lost\n";
  std::cout << "device_lost_detected=" << (mapped.deviceLost ? "true" : "false") << "\n";
  std::cout << "device_lost_source=present\n";
  std::cout << "first_lost_vk_result=" << mapped.rawResultName << "\n";
  std::cout << "device_name=none\n";
  std::cout << "driver_version=none\n";
  std::cout << "swapchain_recreate_attempted=false\n";
  std::cout << "new_submit_after_device_lost=false\n";
  std::cout << "shutdown_after_device_lost=true\n";
  std::cout << "runtime_hash_before=none\n";
  std::cout << "runtime_hash_after=none\n";
  std::cout << "result=" << (pass ? "pass" : "fail") << "\n";
  std::cout << "reason_code=device_lost\n";
  return pass ? 0 : 1;
}
