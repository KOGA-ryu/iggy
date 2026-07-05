// iggy3d_creative — SLICE 1
//
// A standalone executable that boots straight into a blank creative stage:
// a Vulkan window showing a ground grid at Y=0 with a fly camera framed on the
// origin. No menu, no ProductAppWindowState god-struct, no FrontendState, no
// room-loaded gate. It is a pure consumer of the already-built `iggy3d`
// library, reusing the product's window / renderer / grid / frame-builder /
// fly-camera APIs directly.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>

#include <SDL3/SDL.h>

#include "app/PackageRuntimeLookup.hpp"
#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/gameplay/ProjectionRefresh.hpp"
#include "app/iggy3d/map_maker/Grid.hpp"
#include "app/platform/SdlVulkanSurface.hpp"
#include "app/platform/SdlWindow.hpp"
#include "core/math/Vec3.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneItem.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"
#include "render/RendererApi.hpp"
#include "render/RendererConfig.hpp"
#include "render/vulkan/InstanceDeviceSurface.hpp"
#include "render/vulkan/VulkanBackend.hpp"

namespace {

using namespace iggy3d;

// Resolve the Vulkan renderer config (shader root + diagnostics dir) the same
// way the product does, via PackageRuntimeLookup — WITHOUT any god-struct
// telemetry. Mirrors makeProductVulkanRendererConfig() in RendererLifecycle.cpp.
RendererConfig makeCreativeVulkanRendererConfig() {
  PackageLookupConfig lookupConfig;
  lookupConfig.packageMode = PackageMode::BuildTreeProduct;
  lookupConfig.requireShaderRoot = true;
  lookupConfig.requireGraphicsRuntime = true;
  const PackageLookupResult lookup = resolvePackageRuntimeLookup(lookupConfig);

  RendererConfig config;
  config.renderer = RendererMode::Vulkan;
  config.rendererRequirement = RendererRequirement::Optional;
  config.allowSoftwareVulkan = true;
  if (lookup.outcome == RenderOutcome::Ok) {
    config.shaderRoot = lookup.lookup.shaderRoot;
    config.diagnosticsDir = lookup.lookup.diagnosticsDir;
  }
  SDL_Log("iggy3d_creative: shader lookup outcome=%d shaderRoot='%s'",
          static_cast<int>(lookup.outcome),
          config.shaderRoot.generic_string().c_str());
  return config;
}

// Build the Vulkan backend + RendererApi wired to the SDL window's surface.
// Mirrors the backend construction in RendererLifecycle.cpp:214-262.
RendererApi createCreativeRenderer(SdlWindow& window) {
  SdlVulkanSurfaceProvider sdlVulkanProvider;
  const SdlVulkanExtensionList extensions =
      sdlVulkanProvider.requiredInstanceExtensions(window);
  if (extensions.outcome != RenderOutcome::Ok) {
    SDL_Log("iggy3d_creative: vulkan instance extensions unavailable (%s)",
            std::string(extensions.reason.code).c_str());
    return RendererApi{};
  }

  VulkanBackendCreateInfo backendInfo;
  backendInfo.config = makeCreativeVulkanRendererConfig();
  const SdlDrawableExtent drawable = window.drawableExtent();
  backendInfo.drawableWidth = drawable.width == 0U ? 1280U : drawable.width;
  backendInfo.drawableHeight = drawable.height == 0U ? 720U : drawable.height;
  backendInfo.surfaceProvider.requiredInstanceExtensions = extensions.names;
  backendInfo.surfaceProvider.createSurface =
      [&sdlVulkanProvider, &window](VkInstance instance, VkSurfaceKHR* surface) {
        const SdlVulkanSurfaceCreateResult created =
            sdlVulkanProvider.createSurface(window, instance);
        if (surface != nullptr) {
          *surface = created.surface;
        }
        RenderReceipt receipt;
        return receipt;
      };

  return RendererApi(std::make_unique<VulkanBackend>(std::move(backendInfo)));
}

// Local reimplementation of the ~20-line product loop
// (appendMapMakerGridDotsToScene + mapMakerDotSizeFor,
// ProjectionRefresh.cpp:420-453): turn grid dots into small "grid" cubes on the
// scene room. The renderer meshes role="grid" as small shaded cubes.
Vec3 gridDotSizeFor(const ProductMapMakerGridDot& dot, float pitchMeters) {
  const float minorSize = std::clamp(pitchMeters * 0.08F, 0.04F, 0.10F);
  const float majorSize = std::clamp(pitchMeters * 0.14F, 0.07F, 0.16F);
  const float size = dot.major ? majorSize : minorSize;
  return {size, size, size};
}

void appendGridDotsToScene(const ProductMapMakerGridSnapshot& grid,
                           SceneProjectionResult& scene) {
  if (!grid.visible || grid.dots.empty()) {
    return;
  }
  scene.room.meshes.reserve(scene.room.meshes.size() + grid.dots.size());
  std::uint64_t index = 0;
  for (const ProductMapMakerGridDot& dot : grid.dots) {
    // Keep only the ground layer: a small Y-extent still emits a few Y layers
    // (the snap rounds the half-extent out to y=-1,0,1), so filter to planeY.
    if (std::fabs(dot.worldPosition.y - grid.planeY) > grid.pitchMeters * 0.5F) {
      continue;
    }
    SceneRoomMeshItem mesh;
    mesh.id = dot.major ? "creative.grid_major_dot_" : "creative.grid_dot_";
    mesh.id += std::to_string(index);
    mesh.role = "grid";
    mesh.materialId =
        dot.major ? "map_maker_grid_major_dot" : "map_maker_grid_dot";
    mesh.position = dot.worldPosition;
    mesh.size = gridDotSizeFor(dot, grid.pitchMeters);
    scene.room.meshes.push_back(std::move(mesh));
    ++index;
  }
  scene.room.staticMeshCount = scene.room.meshes.size();
  scene.room.loaded = true;
}

}  // namespace

int main(int argc, char** argv) {
  // Optional: --frames N auto-exits after N presented frames (scriptable run).
  std::uint64_t maxFrames = 0;  // 0 = run until window close.
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--frames" && i + 1 < argc) {
      maxFrames = std::strtoull(argv[++i], nullptr, 10);
    }
  }

  // Window (Vulkan). The SdlWindow ctor initializes the SDL video subsystem.
  SdlWindowCreateInfo createInfo;
  createInfo.title = "iggy3d creative";
  createInfo.width = 1280;
  createInfo.height = 720;
  createInfo.resizable = true;
  createInfo.highDpi = true;
  createInfo.vulkan = true;
  SdlWindow window(createInfo);
  if (!window.isOpen()) {
    SDL_Log("iggy3d_creative: failed to open window");
    return 1;
  }

  RendererApi renderer = createCreativeRenderer(window);
  if (!renderer.hasBackend()) {
    SDL_Log("iggy3d_creative: failed to create vulkan renderer");
    return 1;
  }

  // Relative mouse mode for a free-look fly camera.
  window.setRelativeMouseMode(true);

  // Fly camera state. Start pulled back and up, looking at the origin.
  ProductCreativeFlyConfig flyConfig;
  flyConfig.enabled = true;
  flyConfig.speedMetersPerSecond = 8.0F;
  flyConfig.sprintMultiplier = 3.0F;
  flyConfig.inputStepSeconds = 1.0F / 60.0F;

  Vec3 flyPos{0.0F, 6.0F, 12.0F};
  float yawDegrees = 0.0F;
  float pitchDegrees = -25.0F;

  // Build the ground grid ONCE: a single layer at Y=0 (extentYMeters=0 so it
  // does not stack ~9 layers), anchored at the origin.
  ProductMapMakerGridConfig gridConfig;
  gridConfig.enabled = true;
  gridConfig.pitchMeters = 1.0F;
  gridConfig.majorStepMeters = 5.0F;
  gridConfig.extentXMeters = 40.0F;
  gridConfig.extentYMeters = 1.0F;  // Must be > 0 (config validity); the
                                    // ground layer is filtered in
                                    // appendGridDotsToScene (planeY only).
  gridConfig.extentZMeters = 40.0F;
  gridConfig.planeY = 0.0F;
  gridConfig.anchorWorld = Vec3{0.0F, 0.0F, 0.0F};
  const ProductMapMakerGridSnapshot gridSnapshot =
      buildProductMapMakerGridSnapshot(gridConfig);
  SDL_Log("iggy3d_creative: grid visible=%d layers=%llu dots=%llu",
          gridSnapshot.visible ? 1 : 0,
          static_cast<unsigned long long>(gridSnapshot.layerCount),
          static_cast<unsigned long long>(gridSnapshot.dotCount));

  std::uint64_t frameIndex = 0;
  std::uint32_t lastWidth = 0;
  std::uint32_t lastHeight = 0;

  constexpr float kMouseSensitivity = 0.12F;

  while (window.isOpen()) {
    window.pollEvents();
    if (window.eventState().quitRequested) {
      break;
    }
    if (!window.isDrawable()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
      continue;
    }

    const SdlDrawableExtent extent = window.drawableExtent();
    if (extent.width == 0U || extent.height == 0U) {
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
      continue;
    }
    if (extent.width != lastWidth || extent.height != lastHeight) {
      RenderViewport viewport;
      viewport.width = extent.width;
      viewport.height = extent.height;
      viewport.aspectRatio =
          static_cast<float>(extent.width) / static_cast<float>(extent.height);
      renderer.resize(viewport);
      lastWidth = extent.width;
      lastHeight = extent.height;
    }

    // INPUT: keyboard (WASD move, Space/LCtrl up/down, LShift sprint) +
    // relative mouse (look).
    const bool* keys = SDL_GetKeyboardState(nullptr);
    ProductCreativeFlyInput flyInput;
    if (keys != nullptr) {
      // Fly camera axes: direction = right*moveX + forward*moveY + up*moveZ.
      // So moveX = strafe (A/D), moveY = forward/back (W/S), moveZ = up/down
      // (Space/LCtrl).
      const float moveX = (keys[SDL_SCANCODE_D] ? 1.0F : 0.0F) -
                          (keys[SDL_SCANCODE_A] ? 1.0F : 0.0F);
      const float moveY = (keys[SDL_SCANCODE_W] ? 1.0F : 0.0F) -
                          (keys[SDL_SCANCODE_S] ? 1.0F : 0.0F);
      const float moveZ = (keys[SDL_SCANCODE_SPACE] ? 1.0F : 0.0F) -
                          (keys[SDL_SCANCODE_LCTRL] ? 1.0F : 0.0F);
      flyInput.moveX = moveX;
      flyInput.moveY = moveY;
      flyInput.moveZ = moveZ;
      flyInput.sprinting = keys[SDL_SCANCODE_LSHIFT];
    }

    float mouseDx = 0.0F;
    float mouseDy = 0.0F;
    SDL_GetRelativeMouseState(&mouseDx, &mouseDy);
    yawDegrees += mouseDx * kMouseSensitivity;
    pitchDegrees =
        std::clamp(pitchDegrees - mouseDy * kMouseSensitivity, -80.0F, 80.0F);
    flyInput.cameraYawDegrees = yawDegrees;
    flyInput.cameraPitchDegrees = pitchDegrees;

    // CAMERA: advance the fly position.
    const ProductCreativeFlyResult flyResult =
        applyProductCreativeFlyInput(flyConfig, flyInput, flyPos);
    if (flyResult.applied) {
      flyPos = flyResult.finalPositionMeters;
    }

    // SCENE (local, must outlive submitFrame): rebuild the grid meshes each
    // frame from the cached snapshot.
    SceneProjectionResult scene{};
    appendGridDotsToScene(gridSnapshot, scene);
    DebugProjectionResult debug{};

    const FrameInput frame = makeProductVulkanFrame(
        scene, debug, frameIndex++, extent.width, extent.height, yawDegrees,
        pitchDegrees, /*cameraAnchorOverrideAvailable=*/true, flyPos);

    const RenderSubmitResult submit = renderer.submitFrame(frame);
    if (frameIndex <= 1) {
      SDL_Log("iggy3d_creative: frame %llu submit outcome=%d reason='%s' "
              "meshes=%zu",
              static_cast<unsigned long long>(frameIndex),
              static_cast<int>(submit.outcome),
              std::string(submit.reason.code).c_str(),
              scene.room.meshes.size());
    }

    if (maxFrames != 0U && frameIndex >= maxFrames) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  renderer.waitIdle();
  renderer.shutdown();
  return 0;
}
