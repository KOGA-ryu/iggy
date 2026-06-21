#include "app/PackageRuntimeLookup.hpp"
#include "content/PackageLoader.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"
#include "render/RendererApi.hpp"
#include "runtime/session/Session.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

namespace {

struct VisualOptions {
  std::filesystem::path fixturePath = "demos/first_room/package.iggy3d.toml";
  std::filesystem::path shaderRoot;
  std::filesystem::path diagnosticsDir;
  iggy3d::RendererBackendKind backend = iggy3d::RendererBackendKind::Null;
  bool autoBackend = false;
  bool requireRenderer = false;
  bool strictVulkan = false;
  bool printReceipt = false;
  std::uint32_t frames = 1U;
};

struct ParseResult {
  bool ok = true;
  std::string reason = "visual_demo_ok";
  VisualOptions options;
};

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

ParseResult parseOptions(int argc, const char* const* argv) {
  ParseResult result;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg{argv[i]};
    if (arg == "--fixture" && hasValue(i, argc)) {
      result.options.fixturePath = argv[++i];
    } else if (arg == "--renderer" && hasValue(i, argc)) {
      const std::string_view renderer{argv[++i]};
      if (renderer == "null") {
        result.options.backend = iggy3d::RendererBackendKind::Null;
        result.options.autoBackend = false;
      } else if (renderer == "vulkan") {
        result.options.backend = iggy3d::RendererBackendKind::Vulkan;
        result.options.autoBackend = false;
      } else if (renderer == "auto") {
        result.options.backend = iggy3d::RendererBackendKind::Null;
        result.options.autoBackend = true;
      } else {
        result.ok = false;
        result.reason = "visual_demo_config_invalid";
      }
    } else if (arg == "--renderer=null") {
      result.options.backend = iggy3d::RendererBackendKind::Null;
      result.options.autoBackend = false;
    } else if (arg == "--renderer=vulkan") {
      result.options.backend = iggy3d::RendererBackendKind::Vulkan;
      result.options.autoBackend = false;
    } else if (arg == "--renderer=auto") {
      result.options.backend = iggy3d::RendererBackendKind::Null;
      result.options.autoBackend = true;
    } else if (arg == "--require-renderer") {
      result.options.requireRenderer = true;
    } else if (arg == "--strict-vulkan") {
      result.options.strictVulkan = true;
      result.options.backend = iggy3d::RendererBackendKind::Vulkan;
      result.options.requireRenderer = true;
    } else if (arg == "--frames" && hasValue(i, argc)) {
      const std::string_view count{argv[++i]};
      std::uint32_t value = 0U;
      for (const char c : count) {
        if (c < '0' || c > '9') {
          result.ok = false;
          result.reason = "visual_demo_config_invalid";
          return result;
        }
        value = value * 10U + static_cast<std::uint32_t>(c - '0');
      }
      result.options.frames = value == 0U ? 1U : value;
    } else if (arg == "--shader-root" && hasValue(i, argc)) {
      result.options.shaderRoot = argv[++i];
    } else if (arg == "--diagnostics-dir" && hasValue(i, argc)) {
      result.options.diagnosticsDir = argv[++i];
    } else if (arg == "--print-render-receipt") {
      result.options.printReceipt = true;
    } else if (arg == "--validation=off" || arg == "--validation=optional" ||
               arg == "--validation=required" || arg == "--sync-validation=off" ||
               arg == "--sync-validation=optional" || arg == "--sync-validation=required") {
      continue;
    } else {
      result.ok = false;
      result.reason = "visual_demo_config_invalid";
      return result;
    }
  }
  return result;
}

iggy3d::RenderReceipt baseReceipt(std::string_view result, std::string_view reasonCode) {
  iggy3d::RenderReceipt receipt;
  iggy3d::appendReceiptField(receipt, "receipt_version", "1");
  iggy3d::appendReceiptField(receipt, "repo", "iggy3d");
  iggy3d::appendReceiptField(receipt, "app", "iggy3d_visual_demo");
  iggy3d::appendReceiptField(receipt, "packet_order", "3");
  iggy3d::appendReceiptField(receipt, "result", result);
  iggy3d::appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
}

std::filesystem::path resolveFixturePath(const std::filesystem::path& fixturePath,
                                         const iggy3d::PackageRuntimeLookup& lookup) {
  if (fixturePath.is_absolute()) {
    return fixturePath.lexically_normal();
  }
  auto firstComponent = fixturePath.begin();
  if (firstComponent != fixturePath.end() && firstComponent->string() == "fixtures") {
    return (lookup.packageRoot / fixturePath).lexically_normal();
  }
  return (lookup.resourceRoot / fixturePath).lexically_normal();
}

int printFailure(std::string_view reasonCode) {
  std::cout << iggy3d::formatRenderReceipt(baseReceipt("fail", reasonCode));
  return 1;
}

iggy3d::FrameInput makeFrame(const iggy3d::SceneProjectionResult& scene,
                             const iggy3d::DebugProjectionResult& debug,
                             std::uint64_t frameIndex) {
  iggy3d::FrameInput frame;
  frame.viewport = {1280U, 720U, 1280.0F / 720.0F};
  frame.clock = {scene.sourceTick == iggy3d::kInvalidCommandTick ? 0U : scene.sourceTick,
                 frameIndex, 0.0F, 1.0F / 60.0F};
  frame.camera.mode = iggy3d::RenderCameraMode::ThirdPerson;
  frame.camera.worldEye = {0.0F, 2.0F, 5.0F};
  frame.camera.worldForward = {0.0F, 0.0F, -1.0F};
  frame.camera.worldUp = {0.0F, 1.0F, 0.0F};
  frame.camera.nearPlane = 0.1F;
  frame.camera.farPlane = 200.0F;
  frame.projections.scene = &scene;
  frame.projections.debug = &debug;
  return frame;
}

}  // namespace

int main(int argc, const char* const* argv) {
  const ParseResult parsed = parseOptions(argc, argv);
  if (!parsed.ok) {
    return printFailure(parsed.reason);
  }

  iggy3d::PackageLookupConfig lookupConfig;
  lookupConfig.packageMode = iggy3d::PackageMode::BuildTreeVisual;
  lookupConfig.shaderRootOverride = parsed.options.shaderRoot;
  lookupConfig.diagnosticsDirOverride = parsed.options.diagnosticsDir;
  lookupConfig.requireShaderRoot =
      parsed.options.strictVulkan || parsed.options.backend == iggy3d::RendererBackendKind::Vulkan;
  lookupConfig.requireGraphicsRuntime = parsed.options.requireRenderer;
  const iggy3d::PackageLookupResult lookup = iggy3d::resolvePackageRuntimeLookup(lookupConfig);
  if (lookup.outcome != iggy3d::RenderOutcome::Ok) {
    return printFailure(lookup.reason.code);
  }

  const std::filesystem::path fixturePath =
      resolveFixturePath(parsed.options.fixturePath, lookup.lookup);
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage(iggy3d::PackageLoadRequest{fixturePath.string()});
  if (package.status != iggy3d::PackageLoadStatus::Ok) {
    return printFailure("visual_demo_package_lookup_failed");
  }

  iggy3d::SessionCreateRequest create;
  create.config = package.scenario.config;
  create.seed = package.scenario;
  iggy3d::Result<iggy3d::Session> sessionResult = iggy3d::Session::create(create);
  if (sessionResult.status != iggy3d::ResultStatus::Ok) {
    return printFailure("visual_demo_package_lookup_failed");
  }
  iggy3d::Session session = std::move(sessionResult.value);

  iggy3d::RendererCreateInfo rendererCreate;
  rendererCreate.backend = parsed.options.autoBackend ? iggy3d::RendererBackendKind::Null
                                                      : parsed.options.backend;
  rendererCreate.config.renderer =
      rendererCreate.backend == iggy3d::RendererBackendKind::Vulkan ? iggy3d::RendererMode::Vulkan
                                                                    : iggy3d::RendererMode::Null;
  rendererCreate.config.shaderRoot = lookup.lookup.shaderRoot;
  rendererCreate.config.diagnosticsDir = lookup.lookup.diagnosticsDir;
  rendererCreate.config.strictVulkan = parsed.options.strictVulkan;
  rendererCreate.config.rendererRequirement =
      parsed.options.requireRenderer ? iggy3d::RendererRequirement::Required
                                     : iggy3d::RendererRequirement::Optional;

  iggy3d::RendererApi renderer = iggy3d::createRenderer(rendererCreate);
  if (!renderer.hasBackend() && parsed.options.requireRenderer) {
    return printFailure("visual_demo_renderer_unavailable");
  }

  iggy3d::RenderSubmitResult submit;
  for (std::uint32_t frameIndex = 0U; frameIndex < parsed.options.frames; ++frameIndex) {
    const iggy3d::SceneProjectionResult scene = iggy3d::buildSceneProjection(session.state());
    const iggy3d::DebugProjectionResult debug = iggy3d::buildDebugProjection(session.state());
    submit = renderer.submitFrame(makeFrame(scene, debug, frameIndex + 1U));
    if (submit.outcome != iggy3d::RenderOutcome::Ok) {
      return printFailure("visual_demo_frame_failed");
    }
  }

  iggy3d::RenderReceipt receipt = submit.receipt;
  iggy3d::appendReceiptField(receipt, "visual_demo", "bounded");
  iggy3d::appendReceiptField(receipt, "package_mode", iggy3d::packageModeName(lookup.lookup.packageMode));
  iggy3d::appendReceiptField(receipt, "resource_root_source", lookup.lookup.resourceRootSource);
  iggy3d::appendReceiptField(receipt, "shader_root_source", lookup.lookup.shaderRootSource);
  iggy3d::appendReceiptField(receipt, "diagnostics_dir_source", lookup.lookup.diagnosticsDirSource);
  iggy3d::appendReceiptField(receipt, "frames", static_cast<std::uint64_t>(parsed.options.frames));
  iggy3d::appendReceiptField(receipt, "reason_code", "visual_demo_ok");

  if (parsed.options.printReceipt) {
    std::cout << iggy3d::formatRenderReceipt(receipt);
  } else {
    std::cout << "visual_demo.status=ok\n";
    std::cout << "reason_code=visual_demo_ok\n";
  }
  return 0;
}
