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
  ok = expect(sizeof(iggy3d::vulkan::FirstRoomVertex) == sizeof(float) * 8U,
              "vertex stride storage") &&
       ok;
  ok = expect(format.binding.stride == sizeof(iggy3d::vulkan::FirstRoomVertex),
              "binding stride") &&
       ok;
  ok = expect(format.position.location == 0U, "position location") && ok;
  ok = expect(format.uv0.location == 1U, "uv0 location") && ok;
  ok = expect(format.color.location == 2U, "color location") && ok;
  ok = expect(format.position.offset == offsetof(iggy3d::vulkan::FirstRoomVertex, position),
              "position offset") &&
       ok;
  ok = expect(format.color.offset == offsetof(iggy3d::vulkan::FirstRoomVertex, color),
              "color offset") &&
       ok;
  ok = expect(format.uv0.offset == offsetof(iggy3d::vulkan::FirstRoomVertex, uv0),
              "uv0 offset") &&
       ok;
  ok = expect(format.position.format == VK_FORMAT_R32G32B32_SFLOAT, "position format") && ok;
  ok = expect(format.color.format == VK_FORMAT_R32G32B32_SFLOAT, "color format") && ok;
  ok = expect(format.uv0.format == VK_FORMAT_R32G32_SFLOAT, "uv0 format") && ok;
  ok = expect(iggy3d::vulkan::firstRoomVertexFormatMatchesShader(),
              "format matches shader") &&
       ok;
  const iggy3d::vulkan::StaticMeshInstanceVertexFormat instanceFormat =
      iggy3d::vulkan::staticMeshInstanceVertexFormat();
  ok = expect(sizeof(iggy3d::vulkan::StaticMeshInstanceVertex) ==
                  sizeof(float) * 11U,
              "static mesh vertex stride storage") &&
       expect(sizeof(iggy3d::vulkan::StaticMeshInstanceTransform) ==
                  sizeof(float) * 28U,
              "static mesh instance stride storage") &&
       expect(instanceFormat.bindings[0].inputRate ==
                  VK_VERTEX_INPUT_RATE_VERTEX &&
                  instanceFormat.bindings[1].inputRate ==
                      VK_VERTEX_INPUT_RATE_INSTANCE,
              "static mesh vertex and instance rates") &&
       expect(instanceFormat.attributes[3].location ==
                  iggy3d::vulkan::kStaticMeshNormalLocation &&
                  instanceFormat.attributes[4].location ==
                      iggy3d::vulkan::kStaticMeshModelColumn0Location &&
                  instanceFormat.attributes[10].location ==
                      iggy3d::vulkan::kStaticMeshNormalColumn2Location,
              "static mesh instance attribute locations") &&
       expect(iggy3d::vulkan::staticMeshInstanceVertexFormatMatchesShader(),
              "static mesh instance format matches shader") &&
       ok;
  iggy3d::vulkan::PipelineLayoutKey materialLayout;
  materialLayout.layout = "material_texture";
  materialLayout.descriptorSetLayoutCount =
      iggy3d::vulkan::kMaterialTextureDescriptorSetLayoutCount;
  ok = expect(
           iggy3d::vulkan::materialTexturePipelineLayoutKeyValid(
               materialLayout),
           "material layout owns one texture descriptor set") &&
       ok;
  ok = expect(iggy3d::vulkan::firstRoomPipelineVariant(
                  iggy3d::vulkan::FirstRoomDepthMode::ReadWrite) ==
                  iggy3d::vulkan::kFirstRoomPipelineVariant,
              "world preview uses depth read-write pipeline") &&
       expect(iggy3d::vulkan::firstRoomPipelineVariant(
                  iggy3d::vulkan::FirstRoomDepthMode::Disabled) ==
                  iggy3d::vulkan::kCreativeViewModelPipelineVariant,
              "held preview uses depth-disabled pipeline") &&
       expect(iggy3d::vulkan::kStaticMeshInstancePipelineVariant !=
                  iggy3d::vulkan::kStaticMeshInstanceMaterialPipelineVariant,
              "instanced material and color pipelines remain distinct") &&
       ok;
  return ok ? 0 : 1;
}
