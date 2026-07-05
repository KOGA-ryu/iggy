// iggy3d_creative — SLICE 3 ("Move")
//
// A standalone executable that boots straight into a creative stage: a Vulkan
// window showing a ground grid at Y=0 with a fly camera, PLUS one authored crate
// object seeded into a CreativeDocument. The crate renders as a shaded prop cube;
// clicking it (or, in --capture mode, a synthesized click at its projected screen
// center) selects it, which draws a bright-yellow wireframe bounding box, a
// "W x H x D m" dimension label, and an inspector panel of the crate's metadata.
//
// SLICE 3 adds MOVE, driven ENTIRELY through the kernel's generic Move system:
// switching to Tool::Move and feeding the pointer PRESS/MOVE/RELEASE lifecycle to
// facade.dispatchToolInput() — the facade picks the object, snaps the world
// destination to the grid, and commits ONE Move mutation. There is NO
// crate-specific move math here; the crate's new transform/bounds are read back
// from the document via findObject() each frame, so the prop cube, yellow
// wireframe, dimension label and inspector all follow the moved object for free.
// Keys: '1' -> Select, '2' -> Move. In --capture mode an early frame selects the
// crate, then the Move lifecycle relocates it a few cells (world XZ 4,4).
//
// No menu, no ProductAppWindowState god-struct, no FrontendState, no room-loaded
// gate. It is a pure consumer of the already-built `iggy3d` library, reusing the
// product's window / renderer / grid / frame-builder / fly-camera / creative
// facade / UI-projection / wireframe APIs directly.

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <SDL3/SDL.h>

#include "app/PackageRuntimeLookup.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/DocumentWireframe.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"
#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "app/iggy3d/creative/ui/UiProjection.hpp"
#include "app/iggy3d/gameplay/ProjectionRefresh.hpp"
#include "app/iggy3d/map_maker/Grid.hpp"
#include "app/iggy3d/window/FramePresenter.hpp"
#include "app/platform/SdlVulkanSurface.hpp"
#include "app/platform/SdlWindow.hpp"
#include "core/math/Mat4.hpp"
#include "core/math/Vec3.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneItem.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"
#include "render/RendererApi.hpp"
#include "render/RendererConfig.hpp"
#include "render/debug/DebugHudText.hpp"
#include "render/vulkan/FrameCapture.hpp"
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

// Build the Vulkan backend wired to the SDL window's surface. Returns the
// VulkanBackend directly (not wrapped in RendererApi) so we can reach its
// frame-capture API for --capture. Mirrors RendererLifecycle.cpp:214-262.
std::unique_ptr<VulkanBackend> createCreativeRenderer(SdlWindow& window) {
  SdlVulkanSurfaceProvider sdlVulkanProvider;
  const SdlVulkanExtensionList extensions =
      sdlVulkanProvider.requiredInstanceExtensions(window);
  if (extensions.outcome != RenderOutcome::Ok) {
    SDL_Log("iggy3d_creative: vulkan instance extensions unavailable (%s)",
            std::string(extensions.reason.code).c_str());
    return nullptr;
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

  return std::make_unique<VulkanBackend>(std::move(backendInfo));
}

// Write the last presented frame to a PNG (+ raw/meta/hash siblings) via the
// engine's FrameCapture. Requires a drawable swapchain (a display), so this
// only works on a machine with a screen — the capture reads back the swapchain.
bool captureFrameToPng(VulkanBackend& backend, const std::string& pngPath) {
  const RenderOutcome wait = backend.waitIdle();
  if (wait != RenderOutcome::Ok || !backend.frameCaptureReady()) {
    SDL_Log("iggy3d_creative: capture unavailable (wait=%d ready=%d)",
            static_cast<int>(wait), backend.frameCaptureReady() ? 1 : 0);
    return false;
  }
  const vulkan::NormalizedCapture capture = backend.readLastFrameCapture();
  const std::filesystem::path png{pngPath};
  vulkan::FrameCaptureArtifacts artifacts;
  artifacts.screenshotPath = png;
  artifacts.rawPath = std::filesystem::path{png}.replace_extension(".rgba");
  artifacts.metaPath = std::filesystem::path{png}.replace_extension(".meta.kv");
  artifacts.hashPath = std::filesystem::path{png}.replace_extension(".sha256");
  const vulkan::FrameCaptureResult result =
      vulkan::writePacket7CaptureArtifacts(capture, artifacts);
  SDL_Log("iggy3d_creative: capture written=%d path='%s' %ux%u coverage=%.4f",
          result.written ? 1 : 0, png.generic_string().c_str(), capture.width,
          capture.height, result.nonBackgroundPixelCoverage);
  return result.written;
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

// A projected screen-space AABB of a world-space box, in pixels. `valid` is
// false when no corner survives the w>0 (in-front-of-camera) cull.
struct ScreenAabb {
  bool valid = false;
  float minX = 0.0F;
  float minY = 0.0F;
  float maxX = 0.0F;
  float maxY = 0.0F;
};

// Compute w = row3 . (point, 1) for a clip-from-world matrix. transformPoint()
// does the perspective divide internally but hides w, so we recompute it here to
// cull corners behind the camera (w <= 0) — otherwise the divide flips their sign
// and the projected AABB explodes across the whole screen (plan guard).
float clipW(const Mat4& clipFromWorld, Vec3 p) {
  return at(clipFromWorld, 3, 0) * p.x + at(clipFromWorld, 3, 1) * p.y +
         at(clipFromWorld, 3, 2) * p.z + at(clipFromWorld, 3, 3);
}

// Project the 8 corners of a world AABB through clipFromWorld into a pixel-space
// screen AABB. NDC xy in [-1,1] map to pixels ([0,w] x [0,h]) with Y flipped
// (NDC +Y is up, pixels +Y is down). Corners with w<=0 are dropped.
ScreenAabb projectBoxToScreen(const Mat4& clipFromWorld, Vec3 boxMin, Vec3 boxMax,
                              std::uint32_t widthPx, std::uint32_t heightPx) {
  ScreenAabb out;
  float minX = std::numeric_limits<float>::max();
  float minY = std::numeric_limits<float>::max();
  float maxX = std::numeric_limits<float>::lowest();
  float maxY = std::numeric_limits<float>::lowest();
  const float fw = static_cast<float>(widthPx);
  const float fh = static_cast<float>(heightPx);
  for (int corner = 0; corner < 8; ++corner) {
    const Vec3 world{
        (corner & 1) ? boxMax.x : boxMin.x,
        (corner & 2) ? boxMax.y : boxMin.y,
        (corner & 4) ? boxMax.z : boxMin.z,
    };
    const float w = clipW(clipFromWorld, world);
    if (!(std::isfinite(w)) || w <= 0.0F) {
      continue;  // Behind or on the camera plane — cull.
    }
    const Vec3 ndc = transformPoint(clipFromWorld, world);
    if (!std::isfinite(ndc.x) || !std::isfinite(ndc.y)) {
      continue;
    }
    const float px = (ndc.x * 0.5F + 0.5F) * fw;
    const float py = (1.0F - (ndc.y * 0.5F + 0.5F)) * fh;
    minX = std::min(minX, px);
    minY = std::min(minY, py);
    maxX = std::max(maxX, px);
    maxY = std::max(maxY, py);
    out.valid = true;
  }
  out.minX = minX;
  out.minY = minY;
  out.maxX = maxX;
  out.maxY = maxY;
  return out;
}

// Convert a creative bounds to render-space Vec3 min/max (double -> float).
Vec3 toVec3(const creative::CreativeVec3& v) {
  return {static_cast<float>(v.x), static_cast<float>(v.y),
          static_cast<float>(v.z)};
}

// Log an object's document-truth position + bounds.min so the move is provable
// BEFORE vs AFTER. Reads only — no position math. (bounds.min is the corner
// anchor the facade snaps for props with an explicit bounds override.)
void logObjectPlacement(const char* phase, const creative::CreativeObject* obj) {
  if (obj == nullptr) {
    SDL_Log("iggy3d_creative: MOVE %s object=<null>", phase);
    return;
  }
  SDL_Log("iggy3d_creative: MOVE %s pos=(%.3f, %.3f, %.3f) "
          "boundsMin=(%.3f, %.3f, %.3f) boundsMax=(%.3f, %.3f, %.3f)",
          phase, obj->transform.position.x, obj->transform.position.y,
          obj->transform.position.z, obj->bounds.min.x, obj->bounds.min.y,
          obj->bounds.min.z, obj->bounds.max.x, obj->bounds.max.y,
          obj->bounds.max.z);
}

// Log the facade tool-dispatch receipt, surfacing the generic Move-drag detail:
// stage/outcome enums, committed/changed flags, the snapped anchor the facade
// chose, and the document mutation status. This is the commit receipt the plan
// asks for — proof the move went through the kernel, not by hand.
void logMoveDispatch(const char* phase,
                     const creative::CreativeFacadeToolDispatchReceipt& r) {
  SDL_Log("iggy3d_creative: MOVE dispatch %s inputKind=%d accepted=%d changed=%d "
          "moveDragChanged=%d | drag.stage=%d drag.outcome=%d requested=%d "
          "accepted=%d committed=%d changed=%d snappedAnchor=(%.3f, %.3f, %.3f) "
          "documentStatus=%d msg='%s'",
          phase, static_cast<int>(r.inputKind), r.accepted ? 1 : 0,
          r.changed ? 1 : 0, r.moveDragChanged ? 1 : 0,
          static_cast<int>(r.moveDrag.stage),
          static_cast<int>(r.moveDrag.outcome), r.moveDrag.requested ? 1 : 0,
          r.moveDrag.accepted ? 1 : 0, r.moveDrag.committed ? 1 : 0,
          r.moveDrag.changed ? 1 : 0, r.moveDrag.snappedAnchor.x,
          r.moveDrag.snappedAnchor.y, r.moveDrag.snappedAnchor.z,
          static_cast<int>(r.moveDrag.documentStatus),
          r.moveDrag.message.c_str());
}

}  // namespace

int main(int argc, char** argv) {
  // Optional flags:
  //   --frames N        auto-exit after N presented frames (scriptable run)
  //   --capture <path>  render a few frames, write <path>.png, then exit
  std::uint64_t maxFrames = 0;  // 0 = run until window close.
  std::string capturePath;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--frames" && i + 1 < argc) {
      maxFrames = std::strtoull(argv[++i], nullptr, 10);
    } else if (arg == "--capture" && i + 1 < argc) {
      capturePath = argv[++i];
    }
  }
  // In capture mode, render a handful of frames to let the swapchain settle,
  // then grab the last one.
  if (!capturePath.empty() && maxFrames == 0U) {
    maxFrames = 8U;
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

  std::unique_ptr<VulkanBackend> backend = createCreativeRenderer(window);
  if (backend == nullptr ||
      backend->lifecycleState() != RendererLifecycleState::Ready) {
    SDL_Log("iggy3d_creative: renderer not ready (lifecycle=%d)",
            backend == nullptr ? -1
                               : static_cast<int>(backend->lifecycleState()));
    return 1;
  }

  // Relative mouse mode for a free-look fly camera (interactive only — don't
  // grab the mouse during a scripted --capture run).
  if (capturePath.empty()) {
    window.setRelativeMouseMode(true);
  }

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

  // ---- Seed ONE crate object into a CreativeDocument (once) --------------
  creative::CreativeAppState appState;
  {
    creative::CreativeDocument doc =
        creative::CreativeDocument::create("CrateWorld");
    (void)doc.assignId(1);
    const creative::CreativeFacadeDocumentInstallReceipt installReceipt =
        appState.facade.installDocument(std::move(doc));
    SDL_Log("iggy3d_creative: install document accepted=%d",
            installReceipt.accepted ? 1 : 0);
  }

  creative::CreativeDocumentCreateRequest request;
  request.kind = creative::CreativeObjectKind::Crate;
  request.name = "Crate 1";
  request.transform.position = {0.0, 0.5, 0.0};
  request.hasTransformOverride = true;
  request.bounds = {{-0.5, 0.0, -0.5}, {0.5, 1.0, 0.5}};
  request.hasBoundsOverride = true;
  request.visible = true;
  request.hasVisibleOverride = true;
  request.locked = false;
  request.hasLockedOverride = true;
  const creative::CreativeDocumentCreateReceipt createReceipt =
      appState.facade.createDocumentObject(request);
  const creative::CreativeObjectId crateObjectId = createReceipt.objectId;
  SDL_Log("iggy3d_creative: crate create accepted=%d objectId=%llu kind='%s'",
          createReceipt.accepted ? 1 : 0,
          static_cast<unsigned long long>(crateObjectId),
          std::string(creative::toString(createReceipt.objectKind)).c_str());

  // The wireframe projection request: a grid big enough to hold the origin
  // crate (world Y 0..1 fits in height=8; XZ clamp handles the negative corner).
  creative::CreativeSpatialProjectionRequest wireProjReq;
  wireProjReq.gridSize = {80, 8, 80};
  wireProjReq.cellSize = 1.0;
  wireProjReq.clampToGrid = true;
  wireProjReq.includeAuthoringOnly = false;

  // Selected-state logging: emit the selection + submit reason once.
  bool loggedSelection = false;

  // ---- MOVE state (SLICE 3) ----------------------------------------------
  // Interactive: edge-triggered key latches for '1' Select / '2' Move so a held
  // key switches the tool exactly once. Interactive drag latch tracks a left
  // button held while the Move tool is active.
  bool prevKey1 = false;
  bool prevKey2 = false;
  bool moveDragButtonDown = false;
  // --capture: run the generic Move lifecycle across a few frames, and log the
  // crate placement BEFORE the commit and AFTER the release exactly once.
  bool loggedMoveBefore = false;
  bool loggedMoveAfter = false;
  // The scripted Move destination. The kernel's v1 generic Move (TD-7,
  // Facade.cpp:586-590) is a SCREEN-plane drag: worldDestination.x -> the
  // object's world-X anchor, worldDestination.y -> its world-Y anchor, and the
  // DEPTH axis (world Z) HOLDS the start-anchor Z. So to relocate the crate a
  // few cells cleanly we set X=4 (slide it sideways) and hold Y at its authored
  // anchor height 0.5 (avoid sinking it into the ground); Z is ignored by the
  // facade. This is the kernel's mapping — we do NOT reinterpret the axes here.
  const creative::CreativeToolWorldPoint kCaptureMoveDestination{4.0, 0.5, 0.0};

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
      backend->resize(viewport);
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

    // ---- TOOL SWITCH (SLICE 3): '1' -> Select, '2' -> Move ------------------
    // Edge-triggered so a held key flips the active tool once. Driven ONLY
    // through the facade's generic setActiveTool — no per-tool special-casing.
    if (capturePath.empty() && keys != nullptr) {
      const bool key1 = keys[SDL_SCANCODE_1] != 0;
      const bool key2 = keys[SDL_SCANCODE_2] != 0;
      if (key1 && !prevKey1) {
        const bool ok = appState.facade.setActiveTool(creative::Tool::Select);
        SDL_Log("iggy3d_creative: setActiveTool(Select) accepted=%d", ok ? 1 : 0);
      }
      if (key2 && !prevKey2) {
        const bool ok = appState.facade.setActiveTool(creative::Tool::Move);
        SDL_Log("iggy3d_creative: setActiveTool(Move) accepted=%d", ok ? 1 : 0);
      }
      prevKey1 = key1;
      prevKey2 = key2;
    }

    // SCENE (local, must outlive submitFrame): rebuild the grid meshes each
    // frame from the cached snapshot, then append the crate as a prop cube.
    SceneProjectionResult scene{};
    appendGridDotsToScene(gridSnapshot, scene);

    const creative::CreativeObject* crate =
        appState.facade.findObject(crateObjectId);
    Vec3 crateBoxMin{-0.5F, 0.0F, -0.5F};
    Vec3 crateBoxMax{0.5F, 1.0F, 0.5F};
    if (crate != nullptr && crate->visible) {
      crateBoxMin = toVec3(crate->bounds.min);
      crateBoxMax = toVec3(crate->bounds.max);
      SceneRoomMeshItem crateMesh;
      crateMesh.id = "creative.crate_" + std::to_string(crateObjectId);
      crateMesh.role = "prop";  // Distinct color from role="grid".
      crateMesh.materialId = "creative_crate";
      crateMesh.position = {(crateBoxMin.x + crateBoxMax.x) * 0.5F,
                            (crateBoxMin.y + crateBoxMax.y) * 0.5F,
                            (crateBoxMin.z + crateBoxMax.z) * 0.5F};
      crateMesh.size = {crateBoxMax.x - crateBoxMin.x,
                        crateBoxMax.y - crateBoxMin.y,
                        crateBoxMax.z - crateBoxMin.z};
      scene.room.meshes.push_back(std::move(crateMesh));
      scene.room.staticMeshCount = scene.room.meshes.size();
      scene.room.propVisible = true;
    }
    DebugProjectionResult debug{};

    // FRAME (non-const so we can attach UI + wireframe + label below). This
    // gives frame.camera.clipFromWorld (world -> NDC) for click + label maths.
    FrameInput frame = makeProductVulkanFrame(
        scene, debug, frameIndex++, extent.width, extent.height, yawDegrees,
        pitchDegrees, /*cameraAnchorOverrideAvailable=*/true, flyPos);

    // ---- CLICK-TO-SELECT ---------------------------------------------------
    // Project the crate box to a screen-space pixel AABB; a click inside selects
    // it, a miss clears selection. In --capture mode, synthesize a click at the
    // crate's projected screen center on an early frame so the captured frame is
    // the SELECTED state.
    const ScreenAabb crateScreen = projectBoxToScreen(
        frame.camera.clipFromWorld, crateBoxMin, crateBoxMax, extent.width,
        extent.height);

    bool clickRequested = false;
    float clickX = 0.0F;
    float clickY = 0.0F;
    if (!capturePath.empty()) {
      // Synthesize the click at the crate's projected center once the swapchain
      // has settled (frame index ~2).
      if (frameIndex == 3U && crateScreen.valid) {
        clickRequested = true;
        clickX = (crateScreen.minX + crateScreen.maxX) * 0.5F;
        clickY = (crateScreen.minY + crateScreen.maxY) * 0.5F;
      }
    } else {
      // Interactive: hold Left-Alt to release fly-look and click to select.
      const bool* selKeys = SDL_GetKeyboardState(nullptr);
      const bool altHeld =
          selKeys != nullptr && (selKeys[SDL_SCANCODE_LALT] != 0);
      if (altHeld) {
        window.setRelativeMouseMode(false);
        float mx = 0.0F;
        float my = 0.0F;
        const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
        if ((buttons & SDL_BUTTON_LMASK) != 0U) {
          clickRequested = true;
          // Window is high-DPI; scale logical mouse coords to drawable pixels.
          const std::uint32_t logicalW = window.eventState().windowWidth;
          const std::uint32_t logicalH = window.eventState().windowHeight;
          const float scaleX =
              logicalW > 0 ? static_cast<float>(extent.width) /
                                 static_cast<float>(logicalW)
                           : 1.0F;
          const float scaleY =
              logicalH > 0 ? static_cast<float>(extent.height) /
                                 static_cast<float>(logicalH)
                           : 1.0F;
          clickX = mx * scaleX;
          clickY = my * scaleY;
        }
      } else {
        window.setRelativeMouseMode(true);
      }
    }

    if (clickRequested) {
      const bool hit = crateScreen.valid && clickX >= crateScreen.minX &&
                       clickX <= crateScreen.maxX && clickY >= crateScreen.minY &&
                       clickY <= crateScreen.maxY;
      creative::CreativeToolInputPacket packet;
      packet.kind = creative::CreativeToolInputKind::PointerPress;
      packet.pointer.button = creative::CreativeToolPointerButton::Primary;
      if (hit) {
        packet.pointer.target =
            creative::TargetRef{static_cast<creative::Id>(crateObjectId)};
      }  // A miss leaves target invalid -> Select clears selection.
      (void)appState.facade.dispatchToolInput(packet);
    }

    const creative::Id selectedId =
        appState.facade.selectionState().selectedTarget.value;
    const bool crateSelected =
        selectedId == static_cast<creative::Id>(crateObjectId);

    // ---- MOVE (SLICE 3) ----------------------------------------------------
    // Everything below drives the kernel's GENERIC Move: setActiveTool(Move) +
    // the PRESS/MOVE/RELEASE pointer lifecycle through dispatchToolInput. The
    // facade picks the object, snaps the world destination to the grid, and
    // commits ONE Move mutation. NO crate-specific position math lives here.
    if (!capturePath.empty()) {
      // --capture: after the crate is selected (frame 3), run the Move drag.
      //   frame 5: switch to Move + PRESS on the crate (BeginMove)
      //   frame 6: hold + PointerMove carrying worldDestination (PreviewMove)
      //   frame 7: RELEASE with the same worldDestination (CommitMove) -> snap
      if (frameIndex == 5U && crateSelected) {
        const bool ok = appState.facade.setActiveTool(creative::Tool::Move);
        SDL_Log("iggy3d_creative: setActiveTool(Move) accepted=%d", ok ? 1 : 0);
        if (!loggedMoveBefore) {
          logObjectPlacement("BEFORE", appState.facade.findObject(crateObjectId));
          loggedMoveBefore = true;
        }
        creative::CreativeToolInputPacket press;
        press.kind = creative::CreativeToolInputKind::PointerPress;
        press.pointer.button = creative::CreativeToolPointerButton::Primary;
        press.pointer.target =
            creative::TargetRef{static_cast<creative::Id>(crateObjectId)};
        const creative::CreativeFacadeToolDispatchReceipt r =
            appState.facade.dispatchToolInput(press);
        logMoveDispatch("PRESS", r);
      } else if (frameIndex == 6U) {
        creative::CreativeToolInputPacket move;
        move.kind = creative::CreativeToolInputKind::PointerMove;
        move.pointer.button = creative::CreativeToolPointerButton::Primary;
        move.pointer.hasWorldDestination = true;
        move.pointer.worldDestination = kCaptureMoveDestination;
        const creative::CreativeFacadeToolDispatchReceipt r =
            appState.facade.dispatchToolInput(move);
        logMoveDispatch("MOVE", r);
      } else if (frameIndex == 7U) {
        creative::CreativeToolInputPacket release;
        release.kind = creative::CreativeToolInputKind::PointerRelease;
        release.pointer.button = creative::CreativeToolPointerButton::Primary;
        release.pointer.hasWorldDestination = true;
        release.pointer.worldDestination = kCaptureMoveDestination;
        const creative::CreativeFacadeToolDispatchReceipt r =
            appState.facade.dispatchToolInput(release);
        logMoveDispatch("RELEASE", r);
        if (!loggedMoveAfter) {
          logObjectPlacement("AFTER", appState.facade.findObject(crateObjectId));
          loggedMoveAfter = true;
        }
      }
    } else if (appState.facade.toolState().activeTool == creative::Tool::Move &&
               crateSelected) {
      // Interactive Move: while the Move tool is active and the crate is
      // selected, a left-drag runs the same generic lifecycle. The destination
      // is the ground cell the fly camera is aimed at — a simple ray from the
      // camera eye along its forward to the Y=0 plane (kept basic per plan; the
      // facade still owns the pick + snap + commit). Hold Left-Alt (same as the
      // select path) to release fly-look while dragging.
      const bool* mvKeys = SDL_GetKeyboardState(nullptr);
      const bool altHeld = mvKeys != nullptr && (mvKeys[SDL_SCANCODE_LALT] != 0);
      if (altHeld) {
        float mx = 0.0F;
        float my = 0.0F;
        const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
        const bool lDown = (buttons & SDL_BUTTON_LMASK) != 0U;

        // Camera-forward ray -> Y=0 plane -> world XZ ground point.
        const Vec3 eye = frame.camera.worldEye;
        const Vec3 fwd = frame.camera.worldForward;
        creative::CreativeToolWorldPoint ground{eye.x, 0.0, eye.z};
        if (std::fabs(fwd.y) > 1.0e-4F) {
          const float t = -eye.y / fwd.y;  // eye.y + t*fwd.y == 0
          if (t > 0.0F) {
            ground.x = static_cast<double>(eye.x + fwd.x * t);
            ground.z = static_cast<double>(eye.z + fwd.z * t);
          }
        }

        if (lDown && !moveDragButtonDown) {
          moveDragButtonDown = true;
          creative::CreativeToolInputPacket press;
          press.kind = creative::CreativeToolInputKind::PointerPress;
          press.pointer.button = creative::CreativeToolPointerButton::Primary;
          press.pointer.target =
              creative::TargetRef{static_cast<creative::Id>(crateObjectId)};
          (void)appState.facade.dispatchToolInput(press);
        } else if (lDown && moveDragButtonDown) {
          creative::CreativeToolInputPacket move;
          move.kind = creative::CreativeToolInputKind::PointerMove;
          move.pointer.button = creative::CreativeToolPointerButton::Primary;
          move.pointer.hasWorldDestination = true;
          move.pointer.worldDestination = ground;
          (void)appState.facade.dispatchToolInput(move);
        } else if (!lDown && moveDragButtonDown) {
          moveDragButtonDown = false;
          creative::CreativeToolInputPacket release;
          release.kind = creative::CreativeToolInputKind::PointerRelease;
          release.pointer.button = creative::CreativeToolPointerButton::Primary;
          release.pointer.hasWorldDestination = true;
          release.pointer.worldDestination = ground;
          const creative::CreativeFacadeToolDispatchReceipt r =
              appState.facade.dispatchToolInput(release);
          logMoveDispatch("RELEASE", r);
        }
      } else if (moveDragButtonDown) {
        moveDragButtonDown = false;  // Alt released mid-drag: drop the latch.
      }
    }

    // ---- INSPECTOR UI (draw list -> menu frame rects + glyphs) -------------
    ProductCreativeUiProjectionRequest uiReq;
    uiReq.creative = &appState;  // model=nullptr -> facade.buildUiModel().
    uiReq.virtualWidth = 1280;
    uiReq.virtualHeight = 720;
    const ProductCreativeUiProjection uiProj =
        buildProductCreativeUiProjection(uiReq);

    ProductVulkanMenuFrameRequest menuReq;
    menuReq.uiDrawList = &uiProj.drawList;
    menuReq.frameIndex = frameIndex;
    menuReq.drawableWidth = extent.width;
    menuReq.drawableHeight = extent.height;
    ProductVulkanMenuFrame menuFrame =
        buildProductVulkanStarterMenuFrame(menuReq);

    // ---- BOUNDS BOX (wireframe) --------------------------------------------
    const creative::CreativeDocumentWireframeSegmentBuildResult segs =
        creative::buildCreativeDocumentWireframeSegments(
            appState.facade.document(), wireProjReq);
    ProductCreativeWireframeDebugLineBuildResult lines =
        buildProductCreativeWireframeDebugLines(segs.segmentList);
    // The renderer turns each line into a world-space tube of `thickness` METRES
    // (creativeDebugLineBox: size = |edge| x thickness x thickness), so keep it
    // thin (a few cm) or a 1 m box fills into a solid blob. Selected edges go
    // bright yellow + a touch fatter for emphasis (the kernel colors by style,
    // not by selection).
    for (ProductCreativeWireframeDebugLine& line : lines.lineList.lines) {
      const bool sel = crateSelected && line.objectId == crateObjectId;
      line.thickness = sel ? 0.06F : 0.03F;
      if (sel) {
        line.color = {1.0F, 1.0F, 0.0F, 1.0F};
      }
    }
    ProductCreativeWireframeDebugRenderFrame dbg =
        buildProductCreativeWireframeDebugRenderFrame(&lines.lineList);

    // ---- DIMENSION LABEL + glyph merge -------------------------------------
    // Merge the inspector-panel glyphs with the dimension-label glyphs into ONE
    // vector so a single .data() pointer stays valid for the whole frame.
    std::vector<DebugHudGlyphQuad> glyphs = menuFrame.textGlyphQuads;
    if (crateSelected) {
      const Vec3 center{(crateBoxMin.x + crateBoxMax.x) * 0.5F,
                        (crateBoxMin.y + crateBoxMax.y) * 0.5F,
                        (crateBoxMin.z + crateBoxMax.z) * 0.5F};
      const float w = clipW(frame.camera.clipFromWorld, center);
      if (std::isfinite(w) && w > 0.0F) {
        const Vec3 ndc = transformPoint(frame.camera.clipFromWorld, center);
        const float px = (ndc.x * 0.5F + 0.5F) * static_cast<float>(extent.width);
        const float py = (1.0F - (ndc.y * 0.5F + 0.5F)) *
                         static_cast<float>(extent.height);
        const float dimW = crateBoxMax.x - crateBoxMin.x;
        const float dimH = crateBoxMax.y - crateBoxMin.y;
        const float dimD = crateBoxMax.z - crateBoxMin.z;
        char labelBuf[64];
        std::snprintf(labelBuf, sizeof(labelBuf), "%.1f x %.1f x %.1f m",
                      static_cast<double>(dimW), static_cast<double>(dimH),
                      static_cast<double>(dimD));
        const DebugHudLayoutResult labelLayout = layoutDebugHudTextAt(
            labelBuf, static_cast<std::int32_t>(px),
            static_cast<std::int32_t>(py), extent.width, extent.height);
        glyphs.insert(glyphs.end(), labelLayout.quads.begin(),
                      labelLayout.quads.end());
      }
    }

    // ---- ATTACH overlays to the frame --------------------------------------
    frame.ui.visible = true;
    frame.ui.rects = menuFrame.rects.data();
    frame.ui.rectCount = menuFrame.rects.size();
    frame.ui.textGlyphQuads = glyphs.data();
    frame.ui.textGlyphQuadCount = glyphs.size();
    frame.creativeWireframeDebug = dbg.frame;

    const RenderSubmitResult submit = backend->submitFrame(frame);
    if (!loggedSelection) {
      loggedSelection = true;
      SDL_Log("iggy3d_creative: frame %llu submit outcome=%d reason='%s' "
              "meshes=%zu selectedTarget=%u crateSelected=%d wireLines=%zu "
              "uiRects=%zu glyphs=%zu",
              static_cast<unsigned long long>(frameIndex),
              static_cast<int>(submit.outcome),
              std::string(submit.reason.code).c_str(),
              scene.room.meshes.size(), selectedId, crateSelected ? 1 : 0,
              lines.lineList.lines.size(), menuFrame.rects.size(),
              glyphs.size());
    }

    if (maxFrames != 0U && frameIndex >= maxFrames) {
      SDL_Log("iggy3d_creative: FINAL frame %llu submit outcome=%d reason='%s' "
              "selectedTarget=%u crateSelected=%d wireLines=%zu",
              static_cast<unsigned long long>(frameIndex),
              static_cast<int>(submit.outcome),
              std::string(submit.reason.code).c_str(), selectedId,
              crateSelected ? 1 : 0, lines.lineList.lines.size());
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  bool captureOk = true;
  if (!capturePath.empty()) {
    captureOk = captureFrameToPng(*backend, capturePath);
  }
  backend->waitIdle();
  backend->shutdown();
  return captureOk ? 0 : 1;
}
