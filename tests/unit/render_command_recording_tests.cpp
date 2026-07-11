#include "render/vulkan/CommandRecording.hpp"

#include <array>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool creativePreviewCommandsKeepTargetBeforeHeld() {
  std::array<iggy3d::vulkan::CreativePreviewDrawInfo, 2> draws{};
  draws[0].geometryDrawIndex = 0U;
  draws[0].depthDisabled = true;
  draws[1].geometryDrawIndex = 3U;
  draws[1].depthDisabled = false;
  const iggy3d::vulkan::CreativePreviewCommandPlan plan =
      iggy3d::vulkan::buildCreativePreviewCommandPlan(
          draws.data(), draws.size(), 5U);
  return expect(plan.stepCount == 2U, "two preview draws planned") &&
         expect(plan.steps[0].sourceDrawIndex == 1U &&
                    !plan.steps[0].depthDisabled,
                "target uses depth pass first") &&
         expect(plan.steps[1].sourceDrawIndex == 0U &&
                    plan.steps[1].depthDisabled,
                "held object uses depth-disabled pass second");
}

bool creativePreviewCommandsAreBoundedAndValidateGeometry() {
  std::array<iggy3d::vulkan::CreativePreviewDrawInfo, 2> draws{};
  draws[0].geometryDrawIndex = 4U;
  draws[1].geometryDrawIndex = 1U;
  draws[1].depthDisabled = true;
  const iggy3d::vulkan::CreativePreviewCommandPlan filtered =
      iggy3d::vulkan::buildCreativePreviewCommandPlan(
          draws.data(), draws.size(), 3U);
  const iggy3d::vulkan::CreativePreviewCommandPlan absent =
      iggy3d::vulkan::buildCreativePreviewCommandPlan(nullptr, 2U, 3U);
  const iggy3d::vulkan::CreativePreviewCommandPlan overCapacity =
      iggy3d::vulkan::buildCreativePreviewCommandPlan(
          draws.data(), draws.size() + 1U, 3U);
  return expect(filtered.stepCount == 1U &&
                    filtered.steps[0].sourceDrawIndex == 1U &&
                    filtered.steps[0].depthDisabled,
                "invalid geometry draw is omitted") &&
         expect(absent.stepCount == 0U,
                "missing preview draws produce no commands") &&
         expect(overCapacity.stepCount == 0U,
                "over-capacity preview input produces no commands");
}

}  // namespace

int main() {
  return creativePreviewCommandsKeepTargetBeforeHeld() &&
                 creativePreviewCommandsAreBoundedAndValidateGeometry()
             ? 0
             : 1;
}
