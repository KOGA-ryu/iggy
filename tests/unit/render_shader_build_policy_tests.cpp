#include "render/vulkan/VulkanResult.hpp"

#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool fileExists(const std::filesystem::path& path) {
  return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
}

}  // namespace

int main() {
  bool ok = true;
  const std::filesystem::path sourceRoot{IGGY3D_SHADER_SOURCE_ROOT_VALUE};
  const std::string binaryRoot{IGGY3D_SHADER_BINARY_ROOT_VALUE};
  ok = expect(fileExists(sourceRoot / "first_room.vert.glsl"), "first-room vertex shader") &&
       ok;
  ok = expect(fileExists(sourceRoot / "first_room.frag.glsl"), "first-room fragment shader") &&
       ok;
  ok = expect(fileExists(sourceRoot / "material_unlit_textured.vert.glsl"),
              "material vertex shader") &&
       ok;
  ok = expect(fileExists(sourceRoot / "material_unlit_textured.frag.glsl"),
              "material fragment shader") &&
       ok;
  ok = expect(fileExists(sourceRoot / "static_mesh_instanced.vert.glsl"),
              "instanced static mesh vertex shader") &&
       expect(fileExists(sourceRoot /
                         "static_mesh_instanced_textured.vert.glsl"),
              "instanced textured static mesh vertex shader") &&
       ok;
  ok = expect(binaryRoot.find("generated/shaders/vulkan") != std::string::npos,
              "generated shader root") &&
       ok;
  ok = expect(binaryRoot.find("shaders/vulkan/src") == std::string::npos,
              "generated output not source root") &&
       ok;
  ok = expect(std::string{IGGY3D_SHADER_TARGET_ENV_VALUE} == "vulkan1.3",
              "shader target env") &&
       ok;
  const std::vector<std::string_view> packet6Codes =
      iggy3d::vulkan::packet6VulkanReasonCodes();
  const std::set<std::string_view> codes{packet6Codes.begin(), packet6Codes.end()};
  ok = expect(codes.count("shader_compiler_missing") == 1U, "compiler missing reason") &&
       ok;
  ok = expect(codes.count("packet6_smoke_skipped_shader_compiler_missing") == 1U,
              "optional shader skip reason") &&
       ok;
  return ok ? 0 : 1;
}
