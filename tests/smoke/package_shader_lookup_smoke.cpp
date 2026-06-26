#include "app/PackageRuntimeLookup.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

std::string firstMissing(bool firstRoomVertex,
                         bool firstRoomFragment,
                         bool materialVertex,
                         bool materialFragment) {
  if (!firstRoomVertex) {
    return "first_room.vert.spv";
  }
  if (!firstRoomFragment) {
    return "first_room.frag.spv";
  }
  if (!materialVertex) {
    return "material_unlit_textured.vert.spv";
  }
  if (!materialFragment) {
    return "material_unlit_textured.frag.spv";
  }
  return "none";
}

}  // namespace

int main(int argc, char** argv) {
  const std::filesystem::path root =
#if defined(IGGY3D_SHADER_BINARY_ROOT_VALUE)
      IGGY3D_SHADER_BINARY_ROOT_VALUE;
#else
      "";
#endif

  if (root.empty() || !std::filesystem::is_directory(root)) {
    return 77;
  }

  iggy3d::PackageLookupConfig config;
  config.packageMode = iggy3d::PackageMode::BuildTreeProduct;
  config.shaderRootOverride = root;
  config.requireShaderRoot = true;
  config.requireGraphicsRuntime = false;
  if (argc > 0 && argv != nullptr && argv[0] != nullptr) {
    config.executablePathOverride = std::filesystem::absolute(argv[0]);
  }
  const iggy3d::PackageLookupResult lookup = iggy3d::resolvePackageRuntimeLookup(config);
  if (lookup.outcome != iggy3d::RenderOutcome::Ok) {
    std::cout << "smoke=package_shader_lookup\n";
    std::cout << "package_mode=build_tree_product\n";
    std::cout << "shader_root=" << root.string() << "\n";
    std::cout << "shader_root_source=override\n";
    std::cout << "shader_artifact_count=0\n";
    std::cout << "first_room_vertex_shader_found=false\n";
    std::cout << "first_room_fragment_shader_found=false\n";
    std::cout << "material_vertex_shader_found=false\n";
    std::cout << "material_fragment_shader_found=false\n";
    std::cout << "missing_shader=lookup_failed\n";
    std::cout << "result=fail\n";
    std::cout << "reason_code=" << lookup.reason.code << "\n";
    return 1;
  }

  const std::filesystem::path shaderRoot = lookup.lookup.shaderRoot;
  const bool vertexFound = std::filesystem::exists(shaderRoot / "first_room.vert.spv");
  const bool fragmentFound = std::filesystem::exists(shaderRoot / "first_room.frag.spv");
  const bool materialVertexFound =
      std::filesystem::exists(shaderRoot / "material_unlit_textured.vert.spv");
  const bool materialFragmentFound =
      std::filesystem::exists(shaderRoot / "material_unlit_textured.frag.spv");
  const bool allFound = vertexFound && fragmentFound && materialVertexFound && materialFragmentFound;
  std::cout << "smoke=package_shader_lookup\n";
  std::cout << "package_mode=" << iggy3d::packageModeName(lookup.lookup.packageMode) << "\n";
  std::cout << "shader_root=" << shaderRoot.string() << "\n";
  std::cout << "shader_root_source=" << lookup.lookup.shaderRootSource << "\n";
  std::cout << "shader_artifact_count=" << (allFound ? 4 : 0) << "\n";
  std::cout << "first_room_vertex_shader_found=" << (vertexFound ? "true" : "false") << "\n";
  std::cout << "first_room_fragment_shader_found=" << (fragmentFound ? "true" : "false") << "\n";
  std::cout << "material_vertex_shader_found=" << (materialVertexFound ? "true" : "false") << "\n";
  std::cout << "material_fragment_shader_found=" << (materialFragmentFound ? "true" : "false") << "\n";
  std::cout << "missing_shader="
            << firstMissing(vertexFound, fragmentFound, materialVertexFound, materialFragmentFound)
            << "\n";
  std::cout << "result=" << (allFound ? "pass" : "fail") << "\n";
  std::cout << "reason_code=" << (allFound ? "packet7_package_smoke_pass"
                                           : "shader_artifact_missing")
            << "\n";
  return allFound ? 0 : 1;
}
