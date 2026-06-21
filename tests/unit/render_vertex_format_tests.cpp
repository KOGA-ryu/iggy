#include "render/vulkan/FirstRoomPipeline.hpp"

#include <cstddef>
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
  const iggy3d::vulkan::FirstRoomVertexFormat format =
      iggy3d::vulkan::firstRoomVertexFormat();
  ok = expect(sizeof(iggy3d::vulkan::FirstRoomVertex) == sizeof(float) * 6U,
              "vertex stride storage") &&
       ok;
  ok = expect(format.binding.stride == sizeof(iggy3d::vulkan::FirstRoomVertex),
              "binding stride") &&
       ok;
  ok = expect(format.position.location == 0U, "position location") && ok;
  ok = expect(format.color.location == 1U, "color location") && ok;
  ok = expect(format.position.offset == offsetof(iggy3d::vulkan::FirstRoomVertex, position),
              "position offset") &&
       ok;
  ok = expect(format.color.offset == offsetof(iggy3d::vulkan::FirstRoomVertex, color),
              "color offset") &&
       ok;
  ok = expect(format.position.format == VK_FORMAT_R32G32B32_SFLOAT, "position format") && ok;
  ok = expect(format.color.format == VK_FORMAT_R32G32B32_SFLOAT, "color format") && ok;
  ok = expect(iggy3d::vulkan::firstRoomVertexFormatMatchesShader(),
              "format matches shader") &&
       ok;
  return ok ? 0 : 1;
}
