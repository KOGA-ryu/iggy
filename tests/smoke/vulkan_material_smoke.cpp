#include "render/RenderDiagnostics.hpp"
#include "render/vulkan/ShaderModule.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>

namespace {

bool strictLane() {
#if defined(IGGY3D_REQUIRE_VULKAN_SMOKE_ENABLED)
  return true;
#else
  return false;
#endif
}

iggy3d::RenderReceipt baseReceipt(std::string_view result, std::string_view reason) {
  iggy3d::RenderReceipt receipt;
  iggy3d::appendReceiptField(receipt, "receipt_version", "1");
  iggy3d::appendReceiptField(receipt, "repo", "iggy3d");
  iggy3d::appendReceiptField(receipt, "smoke", "vulkan_material");
  iggy3d::appendReceiptField(receipt, "packet_order", "6");
  iggy3d::appendReceiptField(receipt, "backend", "vulkan");
  iggy3d::appendReceiptField(receipt, "shader_language", "glsl");
  iggy3d::appendReceiptField(receipt, "shader_target_env", IGGY3D_SHADER_TARGET_ENV_VALUE);
  iggy3d::appendReceiptField(receipt, "descriptor_layout_family", "material_unlit_textured");
  iggy3d::appendReceiptField(receipt, "result", result);
  iggy3d::appendReceiptField(receipt, "reason_code", reason);
  return receipt;
}

}  // namespace

int main() {
#if !defined(IGGY3D_SHADER_COMPILER_AVAILABLE)
  std::cout << iggy3d::formatRenderReceipt(
      baseReceipt(strictLane() ? "fail" : "skip",
                  strictLane() ? "packet6_smoke_strict_dependency_missing"
                               : "packet6_smoke_skipped_shader_compiler_missing"));
  return strictLane() ? 1 : 77;
#else
  const std::filesystem::path shaderRoot{IGGY3D_SHADER_BINARY_ROOT_VALUE};
  const std::filesystem::path vertexPath = shaderRoot / "material_unlit_textured.vert.spv";
  const std::filesystem::path fragmentPath = shaderRoot / "material_unlit_textured.frag.spv";
  if (!std::filesystem::exists(vertexPath) || !std::filesystem::exists(fragmentPath)) {
    std::cout << iggy3d::formatRenderReceipt(baseReceipt("fail", "shader_artifact_missing"));
    return 1;
  }
  const bool namesOk =
      vertexPath.filename().string().find(iggy3d::vulkan::shaderStageFileExtension(
          iggy3d::vulkan::ShaderStage::Vertex)) != std::string::npos &&
      fragmentPath.filename().string().find(iggy3d::vulkan::shaderStageFileExtension(
          iggy3d::vulkan::ShaderStage::Fragment)) != std::string::npos;
  iggy3d::RenderReceipt receipt =
      baseReceipt(namesOk ? "pass" : "fail",
                  namesOk ? "material_descriptor_deferred" : "shader_stage_mismatch");
  iggy3d::appendReceiptField(receipt, "shader_spirv_path", vertexPath.string());
  iggy3d::appendReceiptField(receipt, "material_runtime_upload", "deferred");
  std::cout << iggy3d::formatRenderReceipt(receipt);
  return namesOk ? 0 : 1;
#endif
}
