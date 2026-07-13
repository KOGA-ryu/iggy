#include "EditorBootstrap.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

#include <cstdlib>

#include <SDL3/SDL.h>

#include "app/PackageRuntimeLookup.hpp"
#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "app/iggy3d/map_maker/Grid.hpp"
#include "app/platform/SdlVulkanSurface.hpp"
#include "render/RendererApi.hpp"
#include "render/vulkan/FrameCapture.hpp"

#include "EditorPlacement.hpp"

namespace iggy3d_creative_app {

void initializeCreativeEditorBootstrapData(
    CreativeEditorBootstrapData& output,
    bool captureMode) {
  // Fly camera state. Start pulled back and up, looking at the origin.
  output.editor.flyConfig.enabled = true;
  output.editor.flyConfig.speedMetersPerSecond = 8.0F;
  output.editor.flyConfig.sprintMultiplier = 3.0F;
  output.editor.flyConfig.inputStepSeconds = 1.0F / 60.0F;

  // Build the ground grid ONCE: a single layer at Y=0 (extentYMeters=0 so it
  // does not stack ~9 layers), anchored at the origin.
  iggy3d::ProductMapMakerGridConfig gridConfig;
  gridConfig.enabled = true;
  gridConfig.pitchMeters = 1.0F;
  gridConfig.majorStepMeters = 5.0F;
  gridConfig.extentXMeters = 40.0F;
  gridConfig.extentYMeters = 1.0F;  // Must be > 0 (config validity); the
                                    // ground layer is filtered in
                                    // standalone preview scene build.
  gridConfig.extentZMeters = 40.0F;
  gridConfig.planeY = 0.0F;
  gridConfig.anchorWorld = iggy3d::Vec3{0.0F, 0.0F, 0.0F};
  output.gridSnapshot =
      iggy3d::buildProductMapMakerGridSnapshot(gridConfig);
  SDL_Log("iggy3d_creative: grid visible=%d layers=%llu dots=%llu",
          output.gridSnapshot.visible ? 1 : 0,
          static_cast<unsigned long long>(output.gridSnapshot.layerCount),
          static_cast<unsigned long long>(output.gridSnapshot.dotCount));

  // ---- Seed initial CreativeDocument objects -----------------------------
  // A Floor tile sitting on Y=0 and a Crate resting on top of it. Both are
  // authored through the SAME generic createDocumentObject path; Floor needs no
  // new kernel work because CreativeObjectKind::Floor already ships a descriptor.
  {
    iggy3d::creative::CreativeDocument doc =
        iggy3d::creative::CreativeDocument::create("FloorAndCrateWorld");
    (void)doc.assignId(1);
    const iggy3d::creative::CreativeFacadeDocumentInstallReceipt installReceipt =
        output.appState.facade.installDocument(std::move(doc));
    SDL_Log("iggy3d_creative: install document accepted=%d",
            installReceipt.accepted ? 1 : 0);
  }

  // FLOOR 1: a 4 x 0.25 x 4 walkable tile whose top sits at Y=0.25 with its slab
  // straddling Y=0. Same authoring request struct as the crate — only kind and
  // extents differ; no per-kind create path.
  iggy3d::creative::CreativeDocumentCreateRequest floorRequest;
  floorRequest.kind = iggy3d::creative::CreativeObjectKind::Floor;
  floorRequest.name = "Floor 1";
  floorRequest.transform.position = {0.0, 0.125, 0.0};
  floorRequest.hasTransformOverride = true;
  floorRequest.bounds = {{-2.0, 0.0, -2.0}, {2.0, 0.25, 2.0}};
  floorRequest.hasBoundsOverride = true;
  floorRequest.visible = true;
  floorRequest.hasVisibleOverride = true;
  floorRequest.locked = false;
  floorRequest.hasLockedOverride = true;
  const iggy3d::creative::CreativeDocumentCreateReceipt floorReceipt =
      output.appState.facade.createDocumentObject(floorRequest);
  output.floorObjectId = floorReceipt.objectId;
  SDL_Log("iggy3d_creative: floor create accepted=%d objectId=%llu kind='%s'",
          floorReceipt.accepted ? 1 : 0,
          static_cast<unsigned long long>(output.floorObjectId),
          std::string(iggy3d::creative::toString(floorReceipt.objectKind))
              .c_str());

  // CRATE 1: a 1 m cube resting ON the floor (bottom at Y=0.25, top at Y=1.25),
  // offset in Z so it does not eclipse the floor tile's center from the camera.
  iggy3d::creative::CreativeDocumentCreateRequest crateRequest;
  crateRequest.kind = iggy3d::creative::CreativeObjectKind::Crate;
  crateRequest.name = "Crate 1";
  crateRequest.transform.position = {0.0, 0.375, 0.0};
  crateRequest.hasTransformOverride = true;
  crateRequest.bounds = {{-0.5, 0.25, -0.5}, {0.5, 1.25, 0.5}};
  crateRequest.hasBoundsOverride = true;
  crateRequest.visible = true;
  crateRequest.hasVisibleOverride = true;
  crateRequest.locked = false;
  crateRequest.hasLockedOverride = true;
  const iggy3d::creative::CreativeDocumentCreateReceipt crateReceipt =
      output.appState.facade.createDocumentObject(crateRequest);
  const iggy3d::creative::CreativeObjectId crateObjectId = crateReceipt.objectId;
  SDL_Log("iggy3d_creative: crate create accepted=%d objectId=%llu kind='%s'",
          crateReceipt.accepted ? 1 : 0,
          static_cast<unsigned long long>(crateObjectId),
          std::string(iggy3d::creative::toString(crateReceipt.objectKind))
              .c_str());
  (void)crateObjectId;  // Retained for the log; tooling keys off the SELECTION.

  // ---- SAVE LOCATION ------------------------------------------------------
  // A single fixed save slot for the standalone app: <HOME>/.iggy3d/
  // creative_standalone with saveId "scene". Created up front so the kernel's
  // creative-save always has a writable root. One slot is enough for the
  // standalone proof.
  if (const char* home = std::getenv("HOME"); home != nullptr) {
    output.saveRoot =
        std::filesystem::path{home} / ".iggy3d" / "creative_standalone";
  } else {
    output.saveRoot =
        std::filesystem::path{".iggy3d"} / "creative_standalone";
  }
  output.saveId = "scene";
  {
    std::error_code ec;
    std::filesystem::create_directories(output.saveRoot, ec);
    SDL_Log("iggy3d_creative: saveRoot='%s' saveId='%s' created=%d",
            output.saveRoot.generic_string().c_str(), output.saveId.c_str(),
            ec ? 0 : 1);
  }

  // The wireframe projection request: a grid big enough to hold the origin
  // crate (world Y 0..1 fits in height=8; XZ clamp handles the negative corner).
  output.wireProjectionRequest.gridSize = {80, 8, 80};
  output.wireProjectionRequest.cellSize = 1.0;
  output.wireProjectionRequest.clampToGrid = true;
  output.wireProjectionRequest.includeAuthoringOnly = false;

  // ---- MOVE / GIZMO state ------------------------------------------------
  // Live movement is selected from the hotbar and driven by the center target.
  // Scripted capture retains its deterministic axis-constrained Move proof.
  // Axis shaft length (m) and wireframe thickness (m). Kept short so the shafts
  // read as handles, not room-scale rays; thickness ~5 cm per the plan.
  output.gizmoAxisLengthMeters = 1.5F;
  output.gizmoThicknessMeters = 0.05F;
  // ---- PLACE state -------------------------------------------------------
  // Slot 1 starts as the first descriptor-backed material. The center ray and
  // targeted face provide its placement anchor; the grid pitch is the cell size.
  output.editor.brushPalette = buildBrushPaletteFromDescriptors();
  output.editor.placeBrush = firstBrushKind(output.editor.brushPalette);
  output.editor.interaction.hotbar =
      iggy3d::creative::makeDefaultCreativeHotbar(
          output.editor.brushPalette);
  output.editor.catalog.model =
      iggy3d::creative::makeCreativeCatalog(output.editor.brushPalette);
  output.editor.catalog.toolWheel =
      iggy3d::creative::makeCreativeToolWheel(output.editor.catalog.model);
  output.editor.placeMode = true;
  SDL_Log("iggy3d_creative: brush palette slots=%llu first='%s'",
          static_cast<unsigned long long>(output.editor.brushPalette.size()),
          std::string(iggy3d::creative::toString(output.editor.placeBrush))
              .c_str());
  output.editor.placeCellSize = static_cast<double>(gridConfig.pitchMeters);
  syncCreativeEditorQuickEdit(output.editor);
  // --capture uses the same initial material slot so its authored placement
  // schedule remains aligned with the interactive application.
  if (captureMode) {
    output.editor.placeBrush = firstBrushKind(output.editor.brushPalette);
  }
  // ---- SAVE / LOAD state ------------------------------------------------
  // Interactive: command-modifier S/N/O route save/new/load without occupying
  // Minecraft's world-action keys.
  // --capture round-trip proof: prove create/delete/move undo, add Point and
  // Line + Path markers, undo their moves, then SAVE/CLEAR/LOAD the
  // eight-object scene. The schedule, flags, ids, and snapshots live in the
  // capture script helper; main only executes the current frame's authored step.
}

iggy3d::RendererConfig makeCreativeVulkanRendererConfig() {
  iggy3d::PackageLookupConfig lookupConfig;
  lookupConfig.packageMode = iggy3d::PackageMode::BuildTreeProduct;
  lookupConfig.requireShaderRoot = true;
  lookupConfig.requireGraphicsRuntime = true;
  const iggy3d::PackageLookupResult lookup =
      iggy3d::resolvePackageRuntimeLookup(lookupConfig);

  iggy3d::RendererConfig config;
  config.renderer = iggy3d::RendererMode::Vulkan;
  config.rendererRequirement = iggy3d::RendererRequirement::Optional;
  config.allowSoftwareVulkan = true;
  config.staticMeshAssetRoot =
      std::filesystem::path{IGGY3D_CREATIVE_ASSET_ROOT_VALUE};
  if (lookup.outcome == iggy3d::RenderOutcome::Ok) {
    config.shaderRoot = lookup.lookup.shaderRoot;
    config.diagnosticsDir = lookup.lookup.diagnosticsDir;
  }
  SDL_Log("iggy3d_creative: shader lookup outcome=%d shaderRoot='%s'",
          static_cast<int>(lookup.outcome),
          config.shaderRoot.generic_string().c_str());
  return config;
}

std::unique_ptr<iggy3d::VulkanBackend> createCreativeRenderer(
    iggy3d::SdlWindow& window) {
  iggy3d::SdlVulkanSurfaceProvider sdlVulkanProvider;
  const iggy3d::SdlVulkanExtensionList extensions =
      sdlVulkanProvider.requiredInstanceExtensions(window);
  if (extensions.outcome != iggy3d::RenderOutcome::Ok) {
    SDL_Log("iggy3d_creative: vulkan instance extensions unavailable (%s)",
            std::string(extensions.reason.code).c_str());
    return nullptr;
  }

  iggy3d::VulkanBackendCreateInfo backendInfo;
  backendInfo.config = makeCreativeVulkanRendererConfig();
  const iggy3d::SdlDrawableExtent drawable = window.drawableExtent();
  backendInfo.drawableWidth = drawable.width == 0U ? 1280U : drawable.width;
  backendInfo.drawableHeight = drawable.height == 0U ? 720U : drawable.height;
  backendInfo.surfaceProvider.requiredInstanceExtensions = extensions.names;
  backendInfo.surfaceProvider.createSurface =
      [&sdlVulkanProvider,
       &window](VkInstance instance, VkSurfaceKHR* surface) {
        const iggy3d::SdlVulkanSurfaceCreateResult created =
            sdlVulkanProvider.createSurface(window, instance);
        if (surface != nullptr) {
          *surface = created.surface;
        }
        iggy3d::RenderReceipt receipt;
        return receipt;
      };

  return std::make_unique<iggy3d::VulkanBackend>(std::move(backendInfo));
}

bool captureFrameToPng(iggy3d::VulkanBackend& backend,
                       const std::string& pngPath) {
  const iggy3d::RenderOutcome wait = backend.waitIdle();
  if (wait != iggy3d::RenderOutcome::Ok || !backend.frameCaptureReady()) {
    SDL_Log("iggy3d_creative: capture unavailable (wait=%d ready=%d)",
            static_cast<int>(wait), backend.frameCaptureReady() ? 1 : 0);
    return false;
  }
  const iggy3d::vulkan::NormalizedCapture capture =
      backend.readLastFrameCapture();
  const std::filesystem::path png{pngPath};
  iggy3d::vulkan::FrameCaptureArtifacts artifacts;
  artifacts.screenshotPath = png;
  artifacts.rawPath = std::filesystem::path{png}.replace_extension(".rgba");
  artifacts.metaPath = std::filesystem::path{png}.replace_extension(".meta.kv");
  artifacts.hashPath = std::filesystem::path{png}.replace_extension(".sha256");
  const iggy3d::vulkan::FrameCaptureResult result =
      iggy3d::vulkan::writePacket7CaptureArtifacts(capture, artifacts);
  SDL_Log("iggy3d_creative: capture written=%d path='%s' %ux%u coverage=%.4f",
          result.written ? 1 : 0, png.generic_string().c_str(), capture.width,
          capture.height, result.nonBackgroundPixelCoverage);
  return result.written;
}

}  // namespace iggy3d_creative_app
