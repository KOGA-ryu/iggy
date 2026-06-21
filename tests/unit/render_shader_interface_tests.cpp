#include "render/vulkan/DescriptorSets.hpp"
#include "render/vulkan/PipelineLayout.hpp"
#include "render/vulkan/ShaderModule.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

}  // namespace

int main() {
  bool ok = true;
  ok = expect(iggy3d::vulkan::shaderStageName(iggy3d::vulkan::ShaderStage::Vertex) ==
                  "vertex",
              "vertex stage name") &&
       ok;
  ok = expect(iggy3d::vulkan::shaderStageName(iggy3d::vulkan::ShaderStage::Fragment) ==
                  "fragment",
              "fragment stage name") &&
       ok;
  ok = expect(iggy3d::vulkan::shaderEntryPointValid("main"), "main entry valid") && ok;
  ok = expect(!iggy3d::vulkan::shaderEntryPointValid("not_main"),
              "non-main entry rejected") &&
       ok;
  ok = expect(sizeof(iggy3d::vulkan::FirstRoomPushConstants) == 64U,
              "push constant size") &&
       ok;
  const auto range = iggy3d::vulkan::firstRoomPushConstantRange();
  ok = expect(range.offset == 0U, "push constant offset") && ok;
  ok = expect(range.size == 64U, "push constant range size") && ok;
  const iggy3d::vulkan::DescriptorSetLayoutRecord firstRoom =
      iggy3d::vulkan::firstRoomDescriptorBaseline();
  ok = expect(firstRoom.bindingCount == 0U, "first-room zero descriptors") && ok;
  ok = expect(iggy3d::vulkan::descriptorLayoutFamilyName(firstRoom.family) == "none",
              "first-room descriptor family") &&
       ok;
  return ok ? 0 : 1;
}
