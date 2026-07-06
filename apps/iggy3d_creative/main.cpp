// iggy3d_creative — SLICE 6 ("In-World Place — the Minecraft face")
//
// SLICE 6 adds a PLACE mode: the fly camera's aimed ground cell shows a GREEN
// GHOST preview of the current "brush" object kind, and a click drops a NEW
// object there (snapped to the 1 m grid), which immediately joins the world —
// it renders, and is Select/Move/Gizmo-able via the identical generic tooling
// from slice 5. Placement goes through the SAME generic createDocumentObject as
// the seeded Floor + Crate: ANY kind places the same way, no per-kind place
// code. The brush kind is just data in the create request.
//
// SLICE 6 additions (all generic; the brush kind never branches placement):
//   - BRUSH KIND: an app-side placeBrush. Key 'B' cycles through a
//     descriptor-derived brush palette. Each kind's footprint comes from
//     descriptor defaults, with standing structural surfaces normalized to the
//     standalone app's thin wall-panel editing affordance.
//   - PLACE MODE: key '3' activates Place ('1' Select, '2' Move already exist).
//     In Place mode the select/move hit-test is skipped; instead the camera ray
//     hits Y=0, the XZ is snapped to the nearest 1 m cell center, and a GREEN
//     (0,1,0,1) axis-aligned wireframe box (the brush footprint at that cell,
//     min.y=0..height) is appended to the SAME combined wireframe vector the
//     selection box + gizmo use — that box is the placement preview.
//   - PLACE: a left-click (interactive) or synthesized (capture) calls
//     facade.createDocumentObject(request{ kind=placeBrush, transform+bounds at
//     the aimed cell with min.y=0, visible }). The new object logs its id/kind/
//     pos and renders + becomes tool-able automatically (slice 5 iterates ALL
//     objects). Select/Move/Gizmo keep working in their own modes; only Place
//     mode swaps the click behavior to "drop a new object".
//   - --capture PROOF: start in Place mode, brush=Crate. The script places the
//     proof scene through the same generic createDocumentObject request path,
//     exercises snapshot undo for place/delete/move, then leaves Place mode
//     active with the ghost visible at the current aim.
//
// SLICE 5 ("Any Object Inherits the Tooling")
//
// A standalone executable that boots straight into a creative stage: a Vulkan
// window showing a ground grid at Y=0 with a fly camera, PLUS authored objects
// seeded into a CreativeDocument. SLICE 5 seeds TWO objects of DIFFERENT kinds —
// a Floor tile and a Crate resting on it — and GENERALIZES every tool from the
// old hardcoded crate to "the currently selected object". The whole point: a new
// object kind (Floor) inherits Select / Inspect / Move / Gizmo with ZERO new
// tooling code — Floor was already a kernel-ready CreativeObjectKind with a full
// descriptor, so it just needs createDocumentObject(kind=Floor) and then rides
// the identical generic code paths as the Crate.
//
// SLICE 5 changes vs slice 4, all generic (no per-kind branches in the tooling):
//   - SEED: Floor 1 (4x0.25x4 tile on Y=0) + Crate 1 (1m cube resting on it).
//   - RENDER: iterate facade.document().objects() and emit one SceneRoomMeshItem
//     per visible object, mapping descriptor facts -> render role so surfaces
//     and mesh proxies read as distinct material classes without per-kind tools.
//   - HIT-TEST: project EVERY object's bounds to a screen AABB, and a click picks
//     the hit object NEAREST the camera (smallest depth), selecting it via the
//     generic dispatchToolInput PointerPress + that object's TargetRef; a miss
//     clears selection.
//   - TOOLING: the yellow wireframe recolor, the 3-axis gizmo, and the Move (both
//     interactive and --capture) all key off the CURRENTLY SELECTED object id
//     (facade.selectionState().selectedTarget.value, looked up via findObject) —
//     NOT any hardcoded crate id and NOT any kind check. If nothing is selected we
//     draw no gizmo/box and skip Move.
//   - --capture PROOF: synthesize a click at the FLOOR's projected screen center
//     (so the FLOOR, not the crate, becomes selected), render its gizmo + yellow
//     box, then grab the X handle and slide the FLOOR +2 m along X via the SAME
//     generic constrained Move. The captured frame shows both objects (distinct
//     colors), the Floor selected (inspector KIND: FLOOR, gizmo on the floor),
//     moved on X only.
//
// Prior slices (unchanged mechanics, now generalized):
// SLICE 2 selection via CPU AABB raycast vs clipFromWorld; SLICE 3 Move via
// dispatchToolInput PRESS/MOVE/RELEASE + grid snap; SLICE 4 3-axis gizmo where a
// grabbed axis drives an axis-constrained Move (worldDestination pins the other
// two axes, moveHeldAxis holds one exact). All of that math is object-agnostic —
// it only ever needs a selected id, a center, and an anchor, which we now read
// from whichever object the selection points at.
//
// SLICE 3 added MOVE, driven ENTIRELY through the kernel's generic Move system:
// switching to Tool::Move and feeding the pointer PRESS/MOVE/RELEASE lifecycle to
// facade.dispatchToolInput() — the facade picks the object, snaps the world
// destination to the grid, and commits ONE Move mutation. There is NO
// crate-specific move math here; the crate's new transform/bounds are read back
// from the document via findObject() each frame, so the prop cube, yellow
// wireframe, dimension label and inspector all follow the moved object for free.
// Keys: '1' -> Select, '2' -> Move.
//
// SLICE 4 adds a 3-AXIS TRANSFORM GIZMO. When the crate is selected we draw three
// axis-aligned wireframe shafts at its center C = (min+max)/2 — X (RED), Y
// (GREEN), Z (BLUE) — as ADDITIONAL RenderCreativeWireframeDebugLine entries
// appended to the yellow selection-box lines (one combined vector points
// frame.creativeWireframeDebug at both). Grabbing one axis handle and dragging
// moves the object ALONG THAT AXIS ONLY, committed through the SAME generic Move:
// we build worldDestination with the grabbed axis carrying the dragged value and
// the OTHER TWO pinned to the start anchor S, then set moveHeldAxis to one of the
// two non-grabbed axes so the facade holds it exact after snap — the third
// non-grabbed axis is pinned to S and snaps to itself for a grid-aligned object.
// So there is STILL zero per-kind move math: single-axis motion falls out of the
// destination + held-axis choice alone. The gizmo geometry is strictly
// axis-aligned because the Vulkan renderer's creativeDebugLineBox only draws a
// segment that moves along EXACTLY ONE world axis (diagonal segments are skipped).
// In --capture mode an early frame selects the crate, then the Move lifecycle
// grabs the X handle and slides the crate +3 m along X (Y,Z pinned).
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
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <SDL3/SDL.h>

#include "app/PackageRuntimeLookup.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/camera/Fly.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/document/DocumentWireframe.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/mutation/Mutation.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"
#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "app/iggy3d/creative/ui/UiProjection.hpp"
#include "app/iggy3d/creative/world/WorldService.hpp"
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

// ---- SLICE 6: place-mode helpers --------------------------------------------

// A brush footprint: XZ extents (metres) + height (metres). The placed object
// sits ON Y=0, so bounds are min.y=0..height and the XZ footprint is centered
// on the aimed cell. This is derived from descriptor defaults; placement logic
// stays one generic createDocumentObject path.
struct BrushFootprint {
  float sizeX = 1.0F;
  float height = 1.0F;
  float sizeZ = 1.0F;
};

constexpr float kPointMarkerSizeMeters = 0.35F;
constexpr float kLineProxyThicknessMeters = 0.16F;
constexpr float kPathProxyThicknessMeters = 0.16F;
constexpr float kPathPointHandleSizeMeters = 0.30F;

bool positiveFinite(float value) {
  return std::isfinite(value) && value > 0.0F;
}

bool finiteCreativeVec3(const creative::CreativeVec3& value) {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

bool validPathPoints(const std::vector<creative::CreativePathPoint>& points) {
  if (points.size() < 2U) {
    return false;
  }
  return std::all_of(points.begin(), points.end(),
                     [](const creative::CreativePathPoint& point) {
                       return finiteCreativeVec3(point.position);
                     });
}

bool samePathPoints(const std::vector<creative::CreativePathPoint>& lhs,
                    const std::vector<creative::CreativePathPoint>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  constexpr double kEps = 1.0e-6;
  for (std::size_t index = 0; index < lhs.size(); ++index) {
    const creative::CreativeVec3& a = lhs[index].position;
    const creative::CreativeVec3& b = rhs[index].position;
    if (std::fabs(a.x - b.x) >= kEps || std::fabs(a.y - b.y) >= kEps ||
        std::fabs(a.z - b.z) >= kEps) {
      return false;
    }
  }
  return true;
}

std::string pathPointsSummary(
    const std::vector<creative::CreativePathPoint>& points) {
  std::string summary;
  for (std::size_t index = 0; index < points.size(); ++index) {
    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "%s(%.3f,%.3f,%.3f)",
                  index == 0U ? "" : "->", points[index].position.x,
                  points[index].position.y, points[index].position.z);
    summary += buffer;
  }
  return summary;
}

std::vector<creative::CreativePathPoint> translatePathPoints(
    const std::vector<creative::CreativePathPoint>& points,
    creative::CreativeVec3 delta) {
  std::vector<creative::CreativePathPoint> translated;
  translated.reserve(points.size());
  for (const creative::CreativePathPoint& point : points) {
    translated.push_back(creative::CreativePathPoint{
        {point.position.x + delta.x,
         point.position.y + delta.y,
         point.position.z + delta.z}});
  }
  return translated;
}

std::vector<creative::CreativePathPoint> movePathPoint(
    const std::vector<creative::CreativePathPoint>& points,
    std::size_t pointIndex,
    creative::CreativeVec3 delta) {
  std::vector<creative::CreativePathPoint> moved = points;
  if (pointIndex < moved.size()) {
    creative::CreativeVec3& position = moved[pointIndex].position;
    position.x += delta.x;
    position.y += delta.y;
    position.z += delta.z;
  }
  return moved;
}

std::vector<creative::CreativePathPoint> initialPathPointsForAnchor(
    Vec3 cellCenter) {
  const double cx = static_cast<double>(cellCenter.x);
  const double cz = static_cast<double>(cellCenter.z);
  return {
      creative::CreativePathPoint{{cx - 1.0, 0.0, cz - 0.5}},
      creative::CreativePathPoint{{cx + 1.0, 0.0, cz - 0.5}},
      creative::CreativePathPoint{{cx + 1.0, 0.0, cz + 1.5}},
  };
}

BrushFootprint descriptorBoundsFootprint(
    const creative::CreativeObjectDescriptor& descriptor) {
  const creative::CreativeBounds& bounds = descriptor.defaults.bounds;
  return {static_cast<float>(bounds.max.x - bounds.min.x),
          static_cast<float>(bounds.max.y - bounds.min.y),
          static_cast<float>(bounds.max.z - bounds.min.z)};
}

bool validBrushFootprint(BrushFootprint footprint) {
  return positiveFinite(footprint.sizeX) && positiveFinite(footprint.height) &&
         positiveFinite(footprint.sizeZ);
}

bool isHorizontalSurfaceFootprint(BrushFootprint footprint) {
  return footprint.height <= std::min(footprint.sizeX, footprint.sizeZ);
}

bool isStandingSurfaceFootprint(BrushFootprint footprint) {
  return footprint.height > std::min(footprint.sizeX, footprint.sizeZ);
}

bool descriptorSupportsBoxPlacement(
    const creative::CreativeObjectDescriptor& descriptor) {
  if (descriptor.kind == creative::CreativeObjectKind::Unknown ||
      !descriptor.hasTransform || !descriptor.hasBounds ||
      descriptor.projectionProfile !=
          creative::CreativeSpatialProjectionProfile::BoxProjection) {
    return false;
  }
  const BrushFootprint footprint = descriptorBoundsFootprint(descriptor);
  if (!validBrushFootprint(footprint)) {
    return false;
  }
  switch (descriptor.shapeKind) {
    case creative::CreativeObjectShapeKind::BoxVolume:
    case creative::CreativeObjectShapeKind::Surface:
    case creative::CreativeObjectShapeKind::MeshProxy:
      return true;
    case creative::CreativeObjectShapeKind::Unknown:
    case creative::CreativeObjectShapeKind::Line:
    case creative::CreativeObjectShapeKind::Point:
    case creative::CreativeObjectShapeKind::Path:
      return false;
  }
  return false;
}

bool descriptorSupportsLinePlacement(
    const creative::CreativeObjectDescriptor& descriptor) {
  if (descriptor.kind == creative::CreativeObjectKind::Unknown ||
      descriptor.shapeKind != creative::CreativeObjectShapeKind::Line ||
      !descriptor.hasTransform || !descriptor.hasBounds) {
    return false;
  }

  const BrushFootprint footprint = descriptorBoundsFootprint(descriptor);
  if (!validBrushFootprint(footprint)) {
    return false;
  }

  return descriptor.projectionProfile ==
             creative::CreativeSpatialProjectionProfile::BoxProjection ||
         descriptor.projectionProfile ==
             creative::CreativeSpatialProjectionProfile::LineProjection;
}

bool descriptorSupportsPointPlacement(
    const creative::CreativeObjectDescriptor& descriptor) {
  return descriptor.kind != creative::CreativeObjectKind::Unknown &&
         descriptor.shapeKind == creative::CreativeObjectShapeKind::Point &&
         descriptor.hasTransform && !descriptor.hasBounds;
}

bool descriptorSupportsPathPlacement(
    const creative::CreativeObjectDescriptor& descriptor) {
  return descriptor.kind != creative::CreativeObjectKind::Unknown &&
         descriptor.shapeKind == creative::CreativeObjectShapeKind::Path &&
         descriptor.projectionProfile ==
             creative::CreativeSpatialProjectionProfile::PathProjection;
}

bool descriptorSupportsBrushPlacement(
    const creative::CreativeObjectDescriptor& descriptor) {
  return descriptorSupportsBoxPlacement(descriptor) ||
         descriptorSupportsLinePlacement(descriptor) ||
         descriptorSupportsPointPlacement(descriptor) ||
         descriptorSupportsPathPlacement(descriptor);
}

bool descriptorAvailableInStandaloneBrushPalette(
    const creative::CreativeObjectDescriptor& descriptor) {
  return !descriptor.isEditorOnly;
}

BrushFootprint brushFootprintForDescriptor(
    const creative::CreativeObjectDescriptor& descriptor) {
  BrushFootprint footprint = descriptorBoundsFootprint(descriptor);
  if (!validBrushFootprint(footprint)) {
    return {};
  }

  if (descriptor.shapeKind == creative::CreativeObjectShapeKind::Surface &&
      descriptor.occupancyKind ==
          creative::CreativeSpatialOccupancyKind::Structural &&
      isStandingSurfaceFootprint(footprint)) {
    // Preserve the current wall brush proof while deriving the decision from
    // descriptor shape/occupancy. A future descriptor placement-footprint column
    // can delete these standalone editing constants.
    return {std::max(footprint.sizeX, footprint.sizeZ), 2.5F, 0.25F};
  }

  return footprint;
}

std::vector<creative::CreativeObjectKind> buildBrushPaletteFromDescriptors() {
  std::vector<creative::CreativeObjectKind> palette;
  std::uint64_t eligibleBeforeHygiene = 0;
  std::string removed;
  for (const creative::CreativeObjectDescriptor& descriptor :
       creative::allObjectDescriptors()) {
    if (!descriptorSupportsBrushPlacement(descriptor)) {
      continue;
    }

    ++eligibleBeforeHygiene;
    if (!descriptorAvailableInStandaloneBrushPalette(descriptor)) {
      if (!removed.empty()) {
        removed += ",";
      }
      removed += std::string(descriptor.name);
      continue;
    }

    palette.push_back(descriptor.kind);
  }
  SDL_Log("iggy3d_creative: brush palette hygiene before=%llu after=%llu "
          "removed=%llu predicate='!descriptor.isEditorOnly' removed='%s'",
          static_cast<unsigned long long>(eligibleBeforeHygiene),
          static_cast<unsigned long long>(palette.size()),
          static_cast<unsigned long long>(eligibleBeforeHygiene -
                                          palette.size()),
          removed.c_str());
  return palette;
}

creative::CreativeObjectKind firstBrushKind(
    const std::vector<creative::CreativeObjectKind>& palette) {
  return palette.empty() ? creative::CreativeObjectKind::Unknown
                         : palette.front();
}

creative::CreativeObjectKind nextBrushKind(
    const std::vector<creative::CreativeObjectKind>& palette,
    creative::CreativeObjectKind current) {
  if (palette.empty()) {
    return creative::CreativeObjectKind::Unknown;
  }

  const auto it = std::find(palette.begin(), palette.end(), current);
  if (it == palette.end()) {
    return palette.front();
  }
  const auto next = std::next(it);
  return next == palette.end() ? palette.front() : *next;
}

std::string_view renderRoleForDescriptor(
    const creative::CreativeObjectDescriptor& descriptor) {
  if (descriptor.shapeKind == creative::CreativeObjectShapeKind::Point) {
    // Existing renderer role with a distinct blue color. This is a coarse
    // standalone marker role until the render layer grows a named point style.
    return "spell";
  }
  if (descriptor.shapeKind == creative::CreativeObjectShapeKind::Line) {
    return "rail";
  }
  if (descriptor.shapeKind == creative::CreativeObjectShapeKind::Path) {
    return "rail";
  }
  const BrushFootprint footprint = descriptorBoundsFootprint(descriptor);
  if (descriptor.shapeKind == creative::CreativeObjectShapeKind::Surface &&
      descriptor.occupancyKind ==
          creative::CreativeSpatialOccupancyKind::Structural &&
      validBrushFootprint(footprint)) {
    if (isHorizontalSurfaceFootprint(footprint)) {
      return "floor";
    }
    if (isStandingSurfaceFootprint(footprint)) {
      return "wall";
    }
  }
  return "prop";
}

struct VisualBounds {
  Vec3 min;
  Vec3 max;
};

VisualBounds pointMarkerBounds(const creative::CreativeVec3& position) {
  const float half = kPointMarkerSizeMeters * 0.5F;
  const Vec3 center = toVec3(position);
  return {{center.x - half, center.y - half, center.z - half},
          {center.x + half, center.y + half, center.z + half}};
}

VisualBounds pathPointHandleBounds(const creative::CreativeVec3& position) {
  const float half = kPathPointHandleSizeMeters * 0.5F;
  const Vec3 center = toVec3(position);
  return {{center.x - half, center.y - half, center.z - half},
          {center.x + half, center.y + half, center.z + half}};
}

enum class VisualMajorAxis { X, Y, Z };

VisualMajorAxis majorAxisForBounds(VisualBounds bounds) {
  const float extentX = bounds.max.x - bounds.min.x;
  const float extentY = bounds.max.y - bounds.min.y;
  const float extentZ = bounds.max.z - bounds.min.z;
  if (extentY > extentX && extentY >= extentZ) {
    return VisualMajorAxis::Y;
  }
  if (extentZ > extentX && extentZ > extentY) {
    return VisualMajorAxis::Z;
  }
  return VisualMajorAxis::X;
}

VisualBounds lineProxyBounds(VisualBounds authoredBounds) {
  const VisualMajorAxis majorAxis = majorAxisForBounds(authoredBounds);
  const Vec3 center{(authoredBounds.min.x + authoredBounds.max.x) * 0.5F,
                    (authoredBounds.min.y + authoredBounds.max.y) * 0.5F,
                    (authoredBounds.min.z + authoredBounds.max.z) * 0.5F};
  const float halfThickness = kLineProxyThicknessMeters * 0.5F;
  VisualBounds proxy{{center.x - halfThickness, center.y - halfThickness,
                      center.z - halfThickness},
                     {center.x + halfThickness, center.y + halfThickness,
                      center.z + halfThickness}};
  switch (majorAxis) {
    case VisualMajorAxis::X:
      proxy.min.x = authoredBounds.min.x;
      proxy.max.x = authoredBounds.max.x;
      break;
    case VisualMajorAxis::Y:
      proxy.min.y = authoredBounds.min.y;
      proxy.max.y = authoredBounds.max.y;
      break;
    case VisualMajorAxis::Z:
      proxy.min.z = authoredBounds.min.z;
      proxy.max.z = authoredBounds.max.z;
      break;
  }
  return proxy;
}

VisualBounds pathProxyBounds(
    const std::vector<creative::CreativePathPoint>& pathPoints) {
  if (!validPathPoints(pathPoints)) {
    return {{-0.5F, 0.0F, -0.5F}, {0.5F, kPathProxyThicknessMeters, 0.5F}};
  }

  const float halfThickness = kPathProxyThicknessMeters * 0.5F;
  Vec3 min{static_cast<float>(pathPoints.front().position.x),
           static_cast<float>(pathPoints.front().position.y),
           static_cast<float>(pathPoints.front().position.z)};
  Vec3 max = min;
  for (const creative::CreativePathPoint& point : pathPoints) {
    const Vec3 p = toVec3(point.position);
    min.x = std::min(min.x, p.x);
    min.y = std::min(min.y, p.y);
    min.z = std::min(min.z, p.z);
    max.x = std::max(max.x, p.x);
    max.y = std::max(max.y, p.y);
    max.z = std::max(max.z, p.z);
  }
  return {{min.x - halfThickness, min.y - halfThickness,
           min.z - halfThickness},
          {max.x + halfThickness, max.y + halfThickness,
           max.z + halfThickness}};
}

VisualBounds pathSegmentProxyBounds(creative::CreativeVec3 start,
                                    creative::CreativeVec3 end) {
  const float halfThickness = kPathProxyThicknessMeters * 0.5F;
  const Vec3 a = toVec3(start);
  const Vec3 b = toVec3(end);
  return {{std::min(a.x, b.x) - halfThickness,
           std::min(a.y, b.y) - halfThickness,
           std::min(a.z, b.z) - halfThickness},
          {std::max(a.x, b.x) + halfThickness,
           std::max(a.y, b.y) + halfThickness,
           std::max(a.z, b.z) + halfThickness}};
}

bool pathSegmentAxisAligned(creative::CreativeVec3 start,
                            creative::CreativeVec3 end) {
  constexpr double kEps = 1.0e-6;
  const bool sameX = std::fabs(start.x - end.x) < kEps;
  const bool sameY = std::fabs(start.y - end.y) < kEps;
  const bool sameZ = std::fabs(start.z - end.z) < kEps;
  return (sameX && sameY && !sameZ) || (sameX && sameZ && !sameY) ||
         (sameY && sameZ && !sameX);
}

VisualBounds visualBoundsForObject(const creative::CreativeObject& object) {
  const creative::CreativeObjectDescriptor& descriptor =
      creative::describeObject(object.kind);
  if (descriptor.shapeKind == creative::CreativeObjectShapeKind::Point) {
    return pointMarkerBounds(object.transform.position);
  }
  if (descriptor.shapeKind == creative::CreativeObjectShapeKind::Path) {
    return pathProxyBounds(object.pathPoints);
  }
  const VisualBounds authoredBounds{toVec3(object.bounds.min),
                                    toVec3(object.bounds.max)};
  if (descriptor.shapeKind == creative::CreativeObjectShapeKind::Line) {
    return lineProxyBounds(authoredBounds);
  }
  return authoredBounds;
}

Vec3 visualBoundsCenter(VisualBounds bounds) {
  return {(bounds.min.x + bounds.max.x) * 0.5F,
          (bounds.min.y + bounds.max.y) * 0.5F,
          (bounds.min.z + bounds.max.z) * 0.5F};
}

void appendPathProxyMeshesToScene(const creative::CreativeObject& object,
                                  SceneProjectionResult& scene,
                                  std::string_view role) {
  if (!validPathPoints(object.pathPoints)) {
    return;
  }
  for (std::size_t index = 0; index < object.pathPoints.size() - 1U; ++index) {
    const creative::CreativeVec3 start = object.pathPoints[index].position;
    const creative::CreativeVec3 end = object.pathPoints[index + 1U].position;
    if (!pathSegmentAxisAligned(start, end)) {
      continue;
    }
    const VisualBounds segmentBounds = pathSegmentProxyBounds(start, end);
    SceneRoomMeshItem mesh;
    mesh.id = "creative.object_" + std::to_string(object.id) + ".path_" +
              std::to_string(index);
    mesh.role = std::string(role);
    mesh.materialId = "creative_object";
    mesh.position = visualBoundsCenter(segmentBounds);
    mesh.size = {segmentBounds.max.x - segmentBounds.min.x,
                 segmentBounds.max.y - segmentBounds.min.y,
                 segmentBounds.max.z - segmentBounds.min.z};
    scene.room.meshes.push_back(std::move(mesh));
  }
}

std::size_t appendStandalonePreviewProxiesToScene(
    const creative::CreativeDocument& document,
    SceneProjectionResult& scene) {
  std::size_t appended = 0;
  for (const creative::CreativeObject& obj : document.objects()) {
    if (!obj.visible) {
      continue;
    }
    const creative::CreativeObjectDescriptor& descriptor =
        creative::describeObject(obj.kind);
    const std::string_view role = renderRoleForDescriptor(descriptor);
    if (descriptor.shapeKind == creative::CreativeObjectShapeKind::Path) {
      const std::size_t before = scene.room.meshes.size();
      appendPathProxyMeshesToScene(obj, scene, role);
      appended += scene.room.meshes.size() - before;
      continue;
    }
    if (descriptor.shapeKind != creative::CreativeObjectShapeKind::Point &&
        descriptor.shapeKind != creative::CreativeObjectShapeKind::Line) {
      continue;
    }

    const VisualBounds visualBounds = visualBoundsForObject(obj);
    const Vec3 boxMin = visualBounds.min;
    const Vec3 boxMax = visualBounds.max;
    SceneRoomMeshItem mesh;
    mesh.id = "creative.preview_object_" + std::to_string(obj.id);
    mesh.role = std::string(role);
    mesh.materialId = "creative_preview_object";
    mesh.position = {(boxMin.x + boxMax.x) * 0.5F,
                     (boxMin.y + boxMax.y) * 0.5F,
                     (boxMin.z + boxMax.z) * 0.5F};
    mesh.size = {boxMax.x - boxMin.x, boxMax.y - boxMin.y,
                 boxMax.z - boxMin.z};
    scene.room.meshes.push_back(std::move(mesh));
    ++appended;
  }
  if (appended > 0U) {
    scene.room.staticMeshCount = scene.room.meshes.size();
    scene.room.loaded = true;
  }
  return appended;
}

// Snap a world XZ ground point to the nearest 1 m cell CENTER: floor to the cell
// then add half a cell. cellSize matches the grid pitch (1 m). Y is fixed at the
// caller's placement plane (always 0 here), so we only snap XZ.
Vec3 snapGroundToCellCenter(double worldX, double worldZ, double cellSize) {
  const double cx = std::floor(worldX / cellSize) * cellSize + cellSize * 0.5;
  const double cz = std::floor(worldZ / cellSize) * cellSize + cellSize * 0.5;
  return {static_cast<float>(cx), 0.0F, static_cast<float>(cz)};
}

// Drop a NEW object of `brush` at the snapped ground cell center via the SAME
// generic createDocumentObject the seed uses — the brush kind is data in the
// request, NOT a place branch. The object sits ON Y=0: bounds are the brush
// footprint centered in XZ on the cell with min.y=0..height, and the transform
// position is the cell center at half-height. Logs the new id/kind/pos.
creative::CreativeDocumentCreateReceipt placeBrushObject(
    creative::Facade& facade, creative::CreativeObjectKind brush,
    Vec3 cellCenter, std::uint64_t ordinal) {
  const creative::CreativeObjectDescriptor& descriptor =
      creative::describeObject(brush);
  const double cx = static_cast<double>(cellCenter.x);
  const double cz = static_cast<double>(cellCenter.z);

  creative::CreativeDocumentCreateRequest request;
  request.kind = brush;  // <-- the ONLY per-kind input: pure data, no branch.
  request.name = std::string(creative::toString(brush)) + " placed#" +
                 std::to_string(ordinal);
  if (descriptor.shapeKind == creative::CreativeObjectShapeKind::Path &&
      descriptor.projectionProfile ==
          creative::CreativeSpatialProjectionProfile::PathProjection) {
    request.hasPathOverride = true;
    request.pathPoints = initialPathPointsForAnchor(cellCenter);
  } else if (descriptor.shapeKind == creative::CreativeObjectShapeKind::Point &&
             descriptor.hasTransform && !descriptor.hasBounds) {
    // Point-shape authored truth is the transform anchor. The marker box used
    // for render/hit/wire feedback is app-local visualization only.
    request.transform.position = {cx, 0.0, cz};
    request.hasTransformOverride = true;
  } else {
    const BrushFootprint fp = brushFootprintForDescriptor(descriptor);
    const double halfX = static_cast<double>(fp.sizeX) * 0.5;
    const double halfZ = static_cast<double>(fp.sizeZ) * 0.5;
    const double height = static_cast<double>(fp.height);
    // Position = cell center at half-height so the box straddles the footprint
    // and rests on Y=0.
    request.transform.position = {cx, height * 0.5, cz};
    request.hasTransformOverride = true;
    // Footprint centered in XZ on the cell, min.y=0 so it sits ON the ground.
    request.bounds = {{cx - halfX, 0.0, cz - halfZ},
                      {cx + halfX, height, cz + halfZ}};
    request.hasBoundsOverride = true;
  }
  request.visible = true;
  request.hasVisibleOverride = true;
  request.locked = false;
  request.hasLockedOverride = true;

  const creative::CreativeDocumentCreateReceipt receipt =
      facade.createDocumentObject(request);
  SDL_Log("iggy3d_creative: PLACE dropped objectId=%llu kind='%s' "
          "pos=(%.3f, %.3f, %.3f) bounds=[(%.3f,%.3f,%.3f)..(%.3f,%.3f,%.3f)] "
          "shape='%s' transformOverride=%d boundsOverride=%d pathOverride=%d "
          "pathPointCount=%zu pathPoints='%s' accepted=%d",
          static_cast<unsigned long long>(receipt.objectId),
          std::string(creative::toString(receipt.objectKind)).c_str(),
          request.transform.position.x, request.transform.position.y,
          request.transform.position.z, request.bounds.min.x,
          request.bounds.min.y, request.bounds.min.z, request.bounds.max.x,
          request.bounds.max.y, request.bounds.max.z,
          std::string(creative::toString(descriptor.shapeKind)).c_str(),
          request.hasTransformOverride ? 1 : 0,
          request.hasBoundsOverride ? 1 : 0,
          request.hasPathOverride ? 1 : 0,
          request.pathPoints.size(),
          pathPointsSummary(request.pathPoints).c_str(),
          receipt.accepted ? 1 : 0);
  return receipt;
}

// Slice C: app-local, document-level undo. The kernel does not yet expose
// inverse receipts/history, so the standalone app stores exact document values
// before a destructive app command and restores them through Facade::installDocument.
struct StandaloneUndoStack {
  std::vector<creative::CreativeDocument> documents;
  std::size_t maxDepth = 32;
};

void clearUndoStack(StandaloneUndoStack& undoStack, std::string_view source) {
  const std::size_t depthBefore = undoStack.documents.size();
  undoStack.documents.clear();
  if (depthBefore > 0U) {
    SDL_Log("iggy3d_creative: UNDO cleared source='%s' depthBefore=%zu "
            "depthAfter=0",
            std::string(source).c_str(), depthBefore);
  }
}

void pushUndoSnapshot(StandaloneUndoStack& undoStack,
                      const creative::Facade& facade,
                      std::string_view source) {
  if (undoStack.documents.size() >= undoStack.maxDepth) {
    undoStack.documents.erase(undoStack.documents.begin());
  }
  const creative::CreativeDocument& document = facade.document();
  undoStack.documents.push_back(document);
  SDL_Log("iggy3d_creative: UNDO pushed source='%s' depth=%zu "
          "objectCount=%llu revision=%llu dirtyFlags=%llu nextObjectId=%llu",
          std::string(source).c_str(), undoStack.documents.size(),
          static_cast<unsigned long long>(document.objectCount()),
          static_cast<unsigned long long>(document.revision()),
          static_cast<unsigned long long>(document.dirtyFlags()),
          static_cast<unsigned long long>(document.nextObjectId()));
}

bool undoLastSnapshot(creative::CreativeAppState& appState,
                      StandaloneUndoStack& undoStack,
                      std::string_view source) {
  const std::size_t depthBefore = undoStack.documents.size();
  const std::uint64_t objectCountBefore =
      static_cast<std::uint64_t>(appState.facade.document().objectCount());
  if (undoStack.documents.empty()) {
    SDL_Log("iggy3d_creative: UNDO empty source='%s' objectCount=%llu",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectCountBefore));
    return false;
  }

  creative::CreativeDocument snapshot = undoStack.documents.back();
  const creative::CreativeFacadeDocumentInstallReceipt installReceipt =
      appState.facade.installDocument(std::move(snapshot));
  if (installReceipt.accepted) {
    undoStack.documents.pop_back();
  }
  const std::uint64_t objectCountAfter =
      static_cast<std::uint64_t>(appState.facade.document().objectCount());
  const creative::Id selectionAfter =
      appState.facade.selectionState().selectedTarget.value;
  SDL_Log("iggy3d_creative: UNDO applied source='%s' accepted=%d changed=%d "
          "depthBefore=%zu depthAfter=%zu objectCountBefore=%llu "
          "objectCountAfter=%llu revisionAfter=%llu dirtyFlagsAfter=%llu "
          "selectionAfter=%u reasonCode='%s'",
          std::string(source).c_str(), installReceipt.accepted ? 1 : 0,
          installReceipt.changed ? 1 : 0, depthBefore,
          undoStack.documents.size(),
          static_cast<unsigned long long>(objectCountBefore),
          static_cast<unsigned long long>(objectCountAfter),
          static_cast<unsigned long long>(appState.facade.document().revision()),
          static_cast<unsigned long long>(appState.facade.document().dirtyFlags()),
          selectionAfter, std::string(installReceipt.reasonCode).c_str());
  return installReceipt.accepted;
}

void discardUndoSnapshot(StandaloneUndoStack& undoStack,
                         std::size_t depthBefore,
                         std::string_view source,
                         std::string_view reasonCode) {
  if (undoStack.documents.size() <= depthBefore) {
    return;
  }
  undoStack.documents.pop_back();
  SDL_Log("iggy3d_creative: UNDO discarded source='%s' depth=%zu "
          "reasonCode='%s'",
          std::string(source).c_str(), undoStack.documents.size(),
          std::string(reasonCode).c_str());
}

creative::CreativeDocumentCreateReceipt placeBrushObjectWithUndo(
    creative::Facade& facade, StandaloneUndoStack& undoStack,
    creative::CreativeObjectKind brush, Vec3 cellCenter, std::uint64_t ordinal,
    std::string_view source) {
  const std::size_t undoDepthBefore = undoStack.documents.size();
  pushUndoSnapshot(undoStack, facade, source);
  creative::CreativeDocumentCreateReceipt receipt =
      placeBrushObject(facade, brush, cellCenter, ordinal);
  if (!receipt.accepted || !receipt.objectCreated) {
    discardUndoSnapshot(undoStack, undoDepthBefore, source, receipt.reasonCode);
  }
  SDL_Log("iggy3d_creative: UNDO create source='%s' objectId=%llu "
          "accepted=%d created=%d objectCount=%llu depthBefore=%zu "
          "depthAfter=%zu reasonCode='%s'",
          std::string(source).c_str(),
          static_cast<unsigned long long>(receipt.objectId),
          receipt.accepted ? 1 : 0, receipt.objectCreated ? 1 : 0,
          static_cast<unsigned long long>(facade.document().objectCount()),
          undoDepthBefore, undoStack.documents.size(),
          std::string(receipt.reasonCode).c_str());
  return receipt;
}

// Delete the currently selected object through the facade's generic document
// removal seam. The facade owns invalidating selection/tool/ghost/measurement
// references; this app only asks to remove the selected target id and logs proof.
creative::CreativeDocumentRemoveReceipt deleteSelectedObject(
    creative::CreativeAppState& appState, std::string_view source,
    StandaloneUndoStack* undoStack = nullptr) {
  const creative::Id selectedId =
      appState.facade.selectionState().selectedTarget.value;
  const std::uint64_t objectCountBefore =
      static_cast<std::uint64_t>(appState.facade.document().objectCount());
  if (selectedId == 0U) {
    SDL_Log("iggy3d_creative: DELETE no selection source='%s' "
            "objectCount=%llu",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectCountBefore));
    return {};
  }

  const auto objectId = static_cast<creative::CreativeObjectId>(selectedId);
  const creative::CreativeObject* object = appState.facade.findObject(objectId);
  if (object == nullptr) {
    SDL_Log("iggy3d_creative: DELETE missing selection source='%s' "
            "objectId=%llu objectCount=%llu",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectId),
            static_cast<unsigned long long>(objectCountBefore));
    return {};
  }

  const creative::CreativeObjectKind kind = object->kind;
  const std::size_t undoDepthBefore =
      undoStack != nullptr ? undoStack->documents.size() : 0U;
  if (undoStack != nullptr) {
    pushUndoSnapshot(*undoStack, appState.facade, source);
  }
  creative::CreativeDocumentRemoveReceipt receipt =
      appState.facade.removeDocumentObject(objectId);
  if ((!receipt.accepted || !receipt.objectRemoved) && undoStack != nullptr &&
      undoStack->documents.size() > undoDepthBefore) {
    undoStack->documents.pop_back();
    SDL_Log("iggy3d_creative: UNDO discarded source='%s' depth=%zu "
            "reasonCode='%s'",
            std::string(source).c_str(), undoStack->documents.size(),
            std::string(receipt.reasonCode).c_str());
  }
  const std::uint64_t objectCountAfter =
      static_cast<std::uint64_t>(appState.facade.document().objectCount());
  const creative::Id selectionAfter =
      appState.facade.selectionState().selectedTarget.value;
  SDL_Log("iggy3d_creative: DELETE removed objectId=%llu kind='%s' "
          "accepted=%d changed=%d removed=%d status='%s' reasonCode='%s' "
          "objectCountBefore=%llu objectCountAfter=%llu selectionAfter=%u",
          static_cast<unsigned long long>(objectId),
          std::string(creative::toString(kind)).c_str(),
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          receipt.objectRemoved ? 1 : 0,
          std::string(creative::toString(receipt.status)).c_str(),
          std::string(receipt.reasonCode).c_str(),
          static_cast<unsigned long long>(objectCountBefore),
          static_cast<unsigned long long>(objectCountAfter), selectionAfter);
  return receipt;
}

bool selectObjectForCapture(creative::Facade& facade,
                            creative::CreativeObjectId objectId,
                            std::string_view source) {
  if (objectId == creative::kInvalidObjectId) {
    SDL_Log("iggy3d_creative: CAPTURE select skipped source='%s' "
            "objectId=0",
            std::string(source).c_str());
    return false;
  }
  creative::CreativeToolInputPacket packet;
  packet.kind = creative::CreativeToolInputKind::PointerPress;
  packet.pointer.button = creative::CreativeToolPointerButton::Primary;
  packet.pointer.target = creative::TargetRef{static_cast<creative::Id>(objectId)};
  const creative::CreativeFacadeToolDispatchReceipt receipt =
      facade.dispatchToolInput(packet);
  SDL_Log("iggy3d_creative: CAPTURE selected source='%s' objectId=%llu "
          "accepted=%d changed=%d selectedTarget=%u",
          std::string(source).c_str(), static_cast<unsigned long long>(objectId),
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          facade.selectionState().selectedTarget.value);
  return receipt.accepted;
}

// Append the 12 AXIS-ALIGNED edges of a world box [boxMin, boxMax] as wireframe
// lines of the given color into `out`. Each edge moves along exactly one world
// axis, which is the only geometry the renderer's creativeDebugLineBox draws —
// the same reason the gizmo shafts are single-axis. Used for the green Place
// ghost preview, appended to the SAME combined vector as the selection box.
void appendWireframeBoxEdges(std::vector<RenderCreativeWireframeDebugLine>& out,
                            Vec3 boxMin, Vec3 boxMax, RenderLineColor color,
                            float thickness) {
  // 8 corners indexed by (x bit0, y bit1, z bit2).
  const auto corner = [&](int c) -> Vec3 {
    return {(c & 1) ? boxMax.x : boxMin.x, (c & 2) ? boxMax.y : boxMin.y,
            (c & 4) ? boxMax.z : boxMin.z};
  };
  // 12 edges: pairs of corner indices differing in exactly one axis bit.
  static constexpr int kEdges[12][2] = {
      {0, 1}, {2, 3}, {4, 5}, {6, 7},  // along X
      {0, 2}, {1, 3}, {4, 6}, {5, 7},  // along Y
      {0, 4}, {1, 5}, {2, 6}, {3, 7},  // along Z
  };
  out.reserve(out.size() + 12);
  for (const auto& e : kEdges) {
    RenderCreativeWireframeDebugLine line;
    line.start = corner(e[0]);
    line.end = corner(e[1]);
    line.color = color;
    line.objectId = 0;  // Ghost is not a document object.
    line.thickness = thickness;
    out.push_back(line);
  }
}

void appendPathPolylineLines(
    std::vector<RenderCreativeWireframeDebugLine>& out,
    const std::vector<creative::CreativePathPoint>& pathPoints,
    RenderLineColor color,
    float thickness,
    creative::CreativeObjectId objectId = creative::kInvalidObjectId) {
  if (!validPathPoints(pathPoints)) {
    return;
  }
  out.reserve(out.size() + pathPoints.size() - 1U);
  for (std::size_t index = 0; index < pathPoints.size() - 1U; ++index) {
    const creative::CreativeVec3 start = pathPoints[index].position;
    const creative::CreativeVec3 end = pathPoints[index + 1U].position;
    if (!pathSegmentAxisAligned(start, end)) {
      continue;
    }
    RenderCreativeWireframeDebugLine line;
    line.start = toVec3(start);
    line.end = toVec3(end);
    line.color = color;
    line.objectId = objectId;
    line.thickness = thickness;
    out.push_back(line);
  }
}

// ---- SLICE 4: gizmo helpers -------------------------------------------------

// A single world point projected to pixel space (same NDC->pixel maths as
// projectBoxToScreen). `valid` is false when the point is behind the camera
// (w<=0) or projects to a non-finite NDC — a grabbed axis needs a valid tip.
struct ScreenPoint {
  bool valid = false;
  float x = 0.0F;
  float y = 0.0F;
};

// Project ONE world point through clipFromWorld into pixel space, culling w<=0
// (behind the camera) so a flipped-sign divide can't smear the point off-screen.
ScreenPoint projectPointToScreen(const Mat4& clipFromWorld, Vec3 world,
                                 std::uint32_t widthPx, std::uint32_t heightPx) {
  ScreenPoint out;
  const float w = clipW(clipFromWorld, world);
  if (!std::isfinite(w) || w <= 0.0F) {
    return out;
  }
  const Vec3 ndc = transformPoint(clipFromWorld, world);
  if (!std::isfinite(ndc.x) || !std::isfinite(ndc.y)) {
    return out;
  }
  out.x = (ndc.x * 0.5F + 0.5F) * static_cast<float>(widthPx);
  out.y = (1.0F - (ndc.y * 0.5F + 0.5F)) * static_cast<float>(heightPx);
  out.valid = true;
  return out;
}

// Distance in pixels from point p to the finite screen segment [a, b]. Used to
// hit-test a click against each projected axis shaft; the nearest axis within a
// threshold is the grabbed handle.
float pointToSegmentDistancePx(float px, float py, float ax, float ay, float bx,
                               float by) {
  const float dx = bx - ax;
  const float dy = by - ay;
  const float lenSq = dx * dx + dy * dy;
  float t = 0.0F;
  if (lenSq > 1.0e-6F) {
    t = ((px - ax) * dx + (py - ay) * dy) / lenSq;
    t = std::clamp(t, 0.0F, 1.0F);
  }
  const float cx = ax + t * dx;
  const float cy = ay + t * dy;
  const float ex = px - cx;
  const float ey = py - cy;
  return std::sqrt(ex * ex + ey * ey);
}

// Which gizmo axis a click grabbed (or None when the click missed every shaft).
enum class GizmoAxis { None, X, Y, Z };

// A gizmo axis shaft used for both drawing (world start/dir/color) and hit-test.
struct GizmoAxisShaft {
  GizmoAxis axis = GizmoAxis::None;
  Vec3 tip{0.0F, 0.0F, 0.0F};            // C + dir * L (world-space shaft end).
  RenderLineColor color{1.0F, 1.0F, 1.0F, 1.0F};
};

// Map a gizmo axis to the move-held axis for a SINGLE non-grabbed axis. Moving
// along the grabbed axis means holding one of the OTHER two exact (via
// moveHeldAxis) while pinning the third to the start anchor in worldDestination.
// Convention: grabbed X -> hold Y, grabbed Y -> hold X, grabbed Z -> hold X.
creative::CreativeToolMoveHeldAxis heldAxisForGrabbedAxis(GizmoAxis grabbed) {
  switch (grabbed) {
    case GizmoAxis::X:
      return creative::CreativeToolMoveHeldAxis::Y;
    case GizmoAxis::Y:
      return creative::CreativeToolMoveHeldAxis::X;
    case GizmoAxis::Z:
      return creative::CreativeToolMoveHeldAxis::X;
    case GizmoAxis::None:
    default:
      return creative::CreativeToolMoveHeldAxis::Y;
  }
}

const char* gizmoAxisName(GizmoAxis axis) {
  switch (axis) {
    case GizmoAxis::X:
      return "X";
    case GizmoAxis::Y:
      return "Y";
    case GizmoAxis::Z:
      return "Z";
    case GizmoAxis::None:
    default:
      return "None";
  }
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

void logUndoMovePlacement(const char* phase,
                          creative::CreativeObjectId objectId,
                          const creative::CreativeObject* obj) {
  if (obj == nullptr) {
    SDL_Log("iggy3d_creative: UNDO move %s objectId=%llu object=<null>",
            phase, static_cast<unsigned long long>(objectId));
    return;
  }
  SDL_Log("iggy3d_creative: UNDO move %s objectId=%llu pos=(%.3f, %.3f, %.3f) "
          "bounds=[(%.3f, %.3f, %.3f)..(%.3f, %.3f, %.3f)]",
          phase, static_cast<unsigned long long>(objectId),
          obj->transform.position.x, obj->transform.position.y,
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

bool isAppliedMoveCommit(
    const creative::CreativeFacadeToolDispatchReceipt& receipt) {
  return receipt.moveDrag.stage == creative::CreativeFacadeMoveDragStage::Commit &&
         receipt.moveDrag.outcome ==
             creative::CreativeFacadeMoveDragOutcome::Applied &&
         receipt.moveDrag.committed && receipt.moveDrag.changed;
}

creative::CreativeFacadeToolDispatchReceipt dispatchMoveReleaseWithUndo(
    creative::CreativeAppState& appState, StandaloneUndoStack& undoStack,
    const creative::CreativeToolInputPacket& release,
    creative::CreativeObjectId objectId, std::string_view source) {
  const std::size_t undoDepthBefore = undoStack.documents.size();
  logUndoMovePlacement("before", objectId, appState.facade.findObject(objectId));
  pushUndoSnapshot(undoStack, appState.facade, source);
  const creative::CreativeFacadeToolDispatchReceipt receipt =
      appState.facade.dispatchToolInput(release);
  const bool applied = isAppliedMoveCommit(receipt);
  if (!applied) {
    discardUndoSnapshot(undoStack, undoDepthBefore, source,
                        receipt.moveDrag.message);
  }
  logUndoMovePlacement("after", objectId, appState.facade.findObject(objectId));
  SDL_Log("iggy3d_creative: UNDO move commit source='%s' objectId=%llu "
          "applied=%d accepted=%d changed=%d moveDragChanged=%d "
          "depthBefore=%zu depthAfter=%zu stage=%d outcome=%d "
          "documentStatus=%d reasonCode='%s'",
          std::string(source).c_str(),
          static_cast<unsigned long long>(objectId), applied ? 1 : 0,
          receipt.accepted ? 1 : 0, receipt.changed ? 1 : 0,
          receipt.moveDrag.changed ? 1 : 0, undoDepthBefore,
          undoStack.documents.size(), static_cast<int>(receipt.moveDrag.stage),
          static_cast<int>(receipt.moveDrag.outcome),
          static_cast<int>(receipt.moveDrag.documentStatus),
          receipt.moveDrag.message.c_str());
  return receipt;
}

creative::CreativeDocumentMutationReceipt movePathObjectWithUndo(
    creative::CreativeAppState& appState,
    StandaloneUndoStack& undoStack,
    creative::CreativeObjectId objectId,
    creative::CreativeVec3 delta,
    std::string_view source) {
  const creative::CreativeObject* beforeObject =
      appState.facade.findObject(objectId);
  const std::size_t undoDepthBefore = undoStack.documents.size();
  if (beforeObject == nullptr) {
    SDL_Log("iggy3d_creative: PATH move skipped source='%s' objectId=%llu "
            "reason='missing_object'",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectId));
    return {};
  }
  const creative::CreativeObjectDescriptor& descriptor =
      creative::describeObject(beforeObject->kind);
  if (descriptor.shapeKind != creative::CreativeObjectShapeKind::Path) {
    SDL_Log("iggy3d_creative: PATH move skipped source='%s' objectId=%llu "
            "kind='%s' shape='%s'",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectId),
            std::string(creative::toString(beforeObject->kind)).c_str(),
            std::string(creative::toString(descriptor.shapeKind)).c_str());
    return {};
  }

  const std::vector<creative::CreativePathPoint> beforePoints =
      beforeObject->pathPoints;
  const std::vector<creative::CreativePathPoint> afterPoints =
      translatePathPoints(beforePoints, delta);
  SDL_Log("iggy3d_creative: PATH before move objectId=%llu kind='%s' "
          "shape='%s' projection='%s' pathPointCount=%zu pathPoints='%s' "
          "delta=(%.3f, %.3f, %.3f)",
          static_cast<unsigned long long>(objectId),
          std::string(creative::toString(beforeObject->kind)).c_str(),
          std::string(creative::toString(descriptor.shapeKind)).c_str(),
          std::string(creative::toString(descriptor.projectionProfile)).c_str(),
          beforePoints.size(), pathPointsSummary(beforePoints).c_str(), delta.x,
          delta.y, delta.z);

  pushUndoSnapshot(undoStack, appState.facade, source);
  const creative::CreativeDocumentMutationReceipt receipt =
      creative::applyDocumentMutation(
          appState.facade.documentForPersistence(),
          objectId,
          creative::CreativeMutationKind::SetPatrolRoute,
          creative::makePathPointsPayload(afterPoints));
  if (receipt.status != creative::CreativeDocumentMutationStatus::Applied ||
      !receipt.changed) {
    discardUndoSnapshot(undoStack, undoDepthBefore, source, receipt.message);
  }

  const creative::CreativeObject* afterObject =
      appState.facade.findObject(objectId);
  SDL_Log("iggy3d_creative: PATH move commit source='%s' objectId=%llu "
          "status='%s' allowed=%d changed=%d revisionBefore=%llu "
          "revisionAfter=%llu dirtyFlags=%llu depthBefore=%zu depthAfter=%zu "
          "before='%s' after='%s'",
          std::string(source).c_str(),
          static_cast<unsigned long long>(objectId),
          std::string(creative::toString(receipt.status)).c_str(),
          receipt.allowed ? 1 : 0, receipt.changed ? 1 : 0,
          static_cast<unsigned long long>(receipt.revisionBefore),
          static_cast<unsigned long long>(receipt.revisionAfter),
          static_cast<unsigned long long>(receipt.dirtyFlags), undoDepthBefore,
          undoStack.documents.size(), pathPointsSummary(beforePoints).c_str(),
          afterObject != nullptr
              ? pathPointsSummary(afterObject->pathPoints).c_str()
              : "<missing>");
  return receipt;
}

creative::CreativeDocumentMutationReceipt movePathPointWithUndo(
    creative::CreativeAppState& appState,
    StandaloneUndoStack& undoStack,
    creative::CreativeObjectId objectId,
    std::size_t pointIndex,
    creative::CreativeVec3 delta,
    std::string_view source) {
  const creative::CreativeObject* beforeObject =
      appState.facade.findObject(objectId);
  const std::size_t undoDepthBefore = undoStack.documents.size();
  if (beforeObject == nullptr) {
    SDL_Log("iggy3d_creative: PATH_HANDLE move skipped source='%s' objectId=%llu "
            "reason='missing_object'",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectId));
    return {};
  }

  const creative::CreativeObjectDescriptor& descriptor =
      creative::describeObject(beforeObject->kind);
  if (descriptor.shapeKind != creative::CreativeObjectShapeKind::Path) {
    SDL_Log("iggy3d_creative: PATH_HANDLE move skipped source='%s' objectId=%llu "
            "kind='%s' shape='%s'",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectId),
            std::string(creative::toString(beforeObject->kind)).c_str(),
            std::string(creative::toString(descriptor.shapeKind)).c_str());
    return {};
  }
  if (pointIndex >= beforeObject->pathPoints.size()) {
    SDL_Log("iggy3d_creative: PATH_HANDLE move skipped source='%s' objectId=%llu "
            "pointIndex=%zu pointCount=%zu reason='invalid_point_index'",
            std::string(source).c_str(),
            static_cast<unsigned long long>(objectId), pointIndex,
            beforeObject->pathPoints.size());
    return {};
  }

  const std::vector<creative::CreativePathPoint> beforePoints =
      beforeObject->pathPoints;
  const std::vector<creative::CreativePathPoint> afterPoints =
      movePathPoint(beforePoints, pointIndex, delta);
  SDL_Log("iggy3d_creative: PATH_HANDLE before move objectId=%llu kind='%s' "
          "shape='%s' pointIndex=%zu delta=(%.3f, %.3f, %.3f) before='%s' "
          "requested='%s'",
          static_cast<unsigned long long>(objectId),
          std::string(creative::toString(beforeObject->kind)).c_str(),
          std::string(creative::toString(descriptor.shapeKind)).c_str(),
          pointIndex, delta.x, delta.y, delta.z,
          pathPointsSummary(beforePoints).c_str(),
          pathPointsSummary(afterPoints).c_str());

  pushUndoSnapshot(undoStack, appState.facade, source);
  const creative::CreativeDocumentMutationReceipt receipt =
      creative::applyDocumentMutation(
          appState.facade.documentForPersistence(),
          objectId,
          creative::CreativeMutationKind::SetPatrolRoute,
          creative::makePathPointsPayload(afterPoints));
  if (receipt.status != creative::CreativeDocumentMutationStatus::Applied ||
      !receipt.changed) {
    discardUndoSnapshot(undoStack, undoDepthBefore, source, receipt.message);
  }

  const creative::CreativeObject* afterObject =
      appState.facade.findObject(objectId);
  SDL_Log("iggy3d_creative: PATH_HANDLE move commit source='%s' objectId=%llu "
          "pointIndex=%zu status='%s' allowed=%d changed=%d "
          "revisionBefore=%llu revisionAfter=%llu dirtyFlags=%llu "
          "depthBefore=%zu depthAfter=%zu before='%s' after='%s'",
          std::string(source).c_str(),
          static_cast<unsigned long long>(objectId), pointIndex,
          std::string(creative::toString(receipt.status)).c_str(),
          receipt.allowed ? 1 : 0, receipt.changed ? 1 : 0,
          static_cast<unsigned long long>(receipt.revisionBefore),
          static_cast<unsigned long long>(receipt.revisionAfter),
          static_cast<unsigned long long>(receipt.dirtyFlags), undoDepthBefore,
          undoStack.documents.size(), pathPointsSummary(beforePoints).c_str(),
          afterObject != nullptr
              ? pathPointsSummary(afterObject->pathPoints).c_str()
              : "<missing>");
  return receipt;
}

// ---- SLICE 7: save / load helpers -------------------------------------------

// A minimal per-object snapshot captured from the live document: id + kind +
// position + bounds. Used to log the document BEFORE save and AFTER load and to
// prove the round-trip is lossless (kinds + positions + bounds identical).
struct ObjectSnapshotEntry {
  creative::CreativeObjectId id = creative::kInvalidObjectId;
  creative::CreativeObjectKind kind = creative::CreativeObjectKind::Unknown;
  creative::CreativeVec3 position{};
  creative::CreativeVec3 boundsMin{};
  creative::CreativeVec3 boundsMax{};
  std::vector<creative::CreativePathPoint> pathPoints;
};

// Snapshot EVERY object currently in the document (order preserved). Read-only —
// no hardcoded ids, so it survives a clear/load with no dangling references.
std::vector<ObjectSnapshotEntry> snapshotDocument(
    const creative::CreativeDocument& doc) {
  std::vector<ObjectSnapshotEntry> out;
  out.reserve(doc.objectCount());
  for (const creative::CreativeObject& obj : doc.objects()) {
    ObjectSnapshotEntry e;
    e.id = obj.id;
    e.kind = obj.kind;
    e.position = obj.transform.position;
    e.boundsMin = obj.bounds.min;
    e.boundsMax = obj.bounds.max;
    e.pathPoints = obj.pathPoints;
    out.push_back(e);
  }
  return out;
}

// Log a full snapshot of the document (one line per object) under a phase tag.
void logDocumentSnapshot(const char* phase,
                         const std::vector<ObjectSnapshotEntry>& snap) {
  SDL_Log("iggy3d_creative: SNAPSHOT %s objectCount=%zu", phase, snap.size());
  for (const ObjectSnapshotEntry& e : snap) {
    SDL_Log("iggy3d_creative: SNAPSHOT %s   id=%llu kind='%s' "
            "pos=(%.3f, %.3f, %.3f) "
            "bounds=[(%.3f,%.3f,%.3f)..(%.3f,%.3f,%.3f)] "
            "pathPointCount=%zu pathPoints='%s'",
            phase, static_cast<unsigned long long>(e.id),
            std::string(creative::toString(e.kind)).c_str(), e.position.x,
            e.position.y, e.position.z, e.boundsMin.x, e.boundsMin.y,
            e.boundsMin.z, e.boundsMax.x, e.boundsMax.y, e.boundsMax.z,
            e.pathPoints.size(), pathPointsSummary(e.pathPoints).c_str());
  }
}

// True when two snapshots have identical kinds + positions + bounds in order
// (the ids may legitimately re-mint on load, so id equality is NOT required).
bool snapshotsMatch(const std::vector<ObjectSnapshotEntry>& before,
                    const std::vector<ObjectSnapshotEntry>& after) {
  if (before.size() != after.size()) {
    return false;
  }
  const auto vecEq = [](const creative::CreativeVec3& a,
                        const creative::CreativeVec3& b) {
    constexpr double kEps = 1.0e-6;
    return std::fabs(a.x - b.x) < kEps && std::fabs(a.y - b.y) < kEps &&
           std::fabs(a.z - b.z) < kEps;
  };
  for (std::size_t i = 0; i < before.size(); ++i) {
    if (before[i].kind != after[i].kind ||
        !vecEq(before[i].position, after[i].position) ||
        !vecEq(before[i].boundsMin, after[i].boundsMin) ||
        !vecEq(before[i].boundsMax, after[i].boundsMax) ||
        !samePathPoints(before[i].pathPoints, after[i].pathPoints)) {
      return false;
    }
  }
  return true;
}

// Persist the live document to disk via the kernel's creative-save. saveCreative-
// World takes a MUTABLE document pointer because it drains dirty flags, so we
// point it at a local COPY of facade.document() (the copy is safe to mutate and
// the on-disk bytes are identical). Logs the full save receipt.
CreativeWorldSaveResult saveStandaloneScene(
    const creative::Facade& facade, const std::filesystem::path& saveRoot,
    const std::string& saveId) {
  creative::CreativeDocument docCopy = facade.document();  // Copy: save drains.
  CreativeWorldSaveRequest request;
  request.saveRoot = saveRoot;
  request.saveId = saveId;
  request.document = &docCopy;  // Mutable pointer at the local copy.
  request.worldTitle = "standalone";
  request.saveTitle = "scene";
  const CreativeWorldSaveResult result = saveCreativeWorld(request);
  SDL_Log("iggy3d_creative: SAVE accepted=%d saved=%d objectCount=%llu "
          "path='%s' reasonCode='%s'",
          result.accepted ? 1 : 0, result.saved ? 1 : 0,
          static_cast<unsigned long long>(result.objectCount),
          result.path.generic_string().c_str(), result.reasonCode.c_str());
  return result;
}

// Restore the document from disk via the kernel's creative-open and install it
// (MOVE the returned document into the facade). On accept the facade's install
// resets transient state (selection, ghost, move-drag), so no stale ids dangle.
// Logs the open receipt. Returns whether the load was accepted + installed.
bool loadStandaloneScene(creative::CreativeAppState& appState,
                         const std::filesystem::path& saveRoot,
                         const std::string& saveId) {
  CreativeWorldOpenRequest request;
  request.saveRoot = saveRoot;
  request.saveId = saveId;
  CreativeWorldOpenResult result = openCreativeWorld(request);
  SDL_Log("iggy3d_creative: LOAD accepted=%d objectCount=%llu reasonCode='%s'",
          result.accepted ? 1 : 0,
          static_cast<unsigned long long>(result.objectCount),
          result.reasonCode.c_str());
  if (!result.accepted) {
    return false;
  }
  const creative::CreativeFacadeDocumentInstallReceipt installReceipt =
      appState.facade.installDocument(std::move(result.document));
  SDL_Log("iggy3d_creative: LOAD install accepted=%d objectCount=%llu",
          installReceipt.accepted ? 1 : 0,
          static_cast<unsigned long long>(
              appState.facade.document().objectCount()));
  return installReceipt.accepted;
}

// Install a fresh, empty document (NEW/CLEAR). The facade's install resets all
// transient state including the selection, so any prior selection + hardcoded
// seed ids are dropped — the render/hit-test iterate document().objects(), which
// is now empty. Logs the resulting object count (expect 0).
void clearToBlankScene(creative::CreativeAppState& appState) {
  creative::CreativeDocument blank = creative::CreativeDocument::create("blank");
  (void)blank.assignId(1);
  const creative::CreativeFacadeDocumentInstallReceipt installReceipt =
      appState.facade.installDocument(std::move(blank));
  SDL_Log("iggy3d_creative: NEW/CLEAR install accepted=%d objectCount=%llu",
          installReceipt.accepted ? 1 : 0,
          static_cast<unsigned long long>(
              appState.facade.document().objectCount()));
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

  // ---- Seed TWO objects of DIFFERENT kinds into a CreativeDocument -------
  // A Floor tile sitting on Y=0 and a Crate resting on top of it. Both are
  // authored through the SAME generic createDocumentObject path; Floor needs no
  // new kernel work because CreativeObjectKind::Floor already ships a descriptor.
  creative::CreativeAppState appState;
  {
    creative::CreativeDocument doc =
        creative::CreativeDocument::create("FloorAndCrateWorld");
    (void)doc.assignId(1);
    const creative::CreativeFacadeDocumentInstallReceipt installReceipt =
        appState.facade.installDocument(std::move(doc));
    SDL_Log("iggy3d_creative: install document accepted=%d",
            installReceipt.accepted ? 1 : 0);
  }

  // FLOOR 1: a 4 x 0.25 x 4 walkable tile whose top sits at Y=0.25 with its slab
  // straddling Y=0. Same authoring request struct as the crate — only kind and
  // extents differ; no per-kind create path.
  creative::CreativeDocumentCreateRequest floorRequest;
  floorRequest.kind = creative::CreativeObjectKind::Floor;
  floorRequest.name = "Floor 1";
  floorRequest.transform.position = {0.0, 0.125, 0.0};
  floorRequest.hasTransformOverride = true;
  floorRequest.bounds = {{-2.0, 0.0, -2.0}, {2.0, 0.25, 2.0}};
  floorRequest.hasBoundsOverride = true;
  floorRequest.visible = true;
  floorRequest.hasVisibleOverride = true;
  floorRequest.locked = false;
  floorRequest.hasLockedOverride = true;
  const creative::CreativeDocumentCreateReceipt floorReceipt =
      appState.facade.createDocumentObject(floorRequest);
  const creative::CreativeObjectId floorObjectId = floorReceipt.objectId;
  SDL_Log("iggy3d_creative: floor create accepted=%d objectId=%llu kind='%s'",
          floorReceipt.accepted ? 1 : 0,
          static_cast<unsigned long long>(floorObjectId),
          std::string(creative::toString(floorReceipt.objectKind)).c_str());

  // CRATE 1: a 1 m cube resting ON the floor (bottom at Y=0.25, top at Y=1.25),
  // offset in Z so it does not eclipse the floor tile's center from the camera.
  creative::CreativeDocumentCreateRequest crateRequest;
  crateRequest.kind = creative::CreativeObjectKind::Crate;
  crateRequest.name = "Crate 1";
  crateRequest.transform.position = {0.0, 0.375, 0.0};
  crateRequest.hasTransformOverride = true;
  crateRequest.bounds = {{-0.5, 0.25, -0.5}, {0.5, 1.25, 0.5}};
  crateRequest.hasBoundsOverride = true;
  crateRequest.visible = true;
  crateRequest.hasVisibleOverride = true;
  crateRequest.locked = false;
  crateRequest.hasLockedOverride = true;
  const creative::CreativeDocumentCreateReceipt crateReceipt =
      appState.facade.createDocumentObject(crateRequest);
  const creative::CreativeObjectId crateObjectId = crateReceipt.objectId;
  SDL_Log("iggy3d_creative: crate create accepted=%d objectId=%llu kind='%s'",
          crateReceipt.accepted ? 1 : 0,
          static_cast<unsigned long long>(crateObjectId),
          std::string(creative::toString(crateReceipt.objectKind)).c_str());
  (void)crateObjectId;  // Retained for the log; tooling keys off the SELECTION.

  // ---- SAVE LOCATION (SLICE 7) -------------------------------------------
  // A single fixed save slot for the standalone app: <HOME>/.iggy3d/
  // creative_standalone with saveId "scene". Created up front so the kernel's
  // creative-save always has a writable root. One slot is enough for this slice.
  std::filesystem::path saveRoot;
  if (const char* home = std::getenv("HOME"); home != nullptr) {
    saveRoot = std::filesystem::path{home} / ".iggy3d" / "creative_standalone";
  } else {
    saveRoot = std::filesystem::path{".iggy3d"} / "creative_standalone";
  }
  const std::string saveId = "scene";
  {
    std::error_code ec;
    std::filesystem::create_directories(saveRoot, ec);
    SDL_Log("iggy3d_creative: saveRoot='%s' saveId='%s' created=%d",
            saveRoot.generic_string().c_str(), saveId.c_str(),
            ec ? 0 : 1);
  }

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
  // ---- GIZMO state (SLICE 4) ---------------------------------------------
  // Axis shaft length (m) and wireframe thickness (m). Kept short so the shafts
  // read as handles, not room-scale rays; thickness ~5 cm per the plan.
  constexpr float kGizmoAxisLength = 1.5F;
  constexpr float kGizmoThickness = 0.05F;
  // Handle hit-test threshold (px): a click within this pixel distance of a
  // projected shaft grabs that axis; the nearest axis within range wins.
  constexpr float kGizmoHandleThresholdPx = 35.0F;
  // Interactive: which axis is currently grabbed (None = not dragging a handle),
  // the object's start corner anchor S captured at grab time, and the pixel/world
  // frame captured at grab so a cursor drag maps to a world offset along the axis.
  GizmoAxis interactiveGrabbedAxis = GizmoAxis::None;
  Vec3 interactiveGrabAnchorS{0.0F, 0.0F, 0.0F};
  float interactiveGrabCursorX = 0.0F;
  float interactiveGrabCursorY = 0.0F;
  ScreenPoint interactiveGrabCenterScreen;
  ScreenPoint interactiveGrabTipScreen;
  bool interactivePathMoveActive = false;
  creative::CreativeObjectId interactivePathMoveObjectId =
      creative::kInvalidObjectId;
  creative::CreativeToolWorldPoint interactivePathMoveStartGround{};
  bool interactivePathPointMoveActive = false;
  creative::CreativeObjectId interactivePathPointMoveObjectId =
      creative::kInvalidObjectId;
  std::size_t interactivePathPointMoveIndex = 0U;
  creative::CreativeToolWorldPoint interactivePathPointMoveStartGround{};
  // --capture: log the grabbed axis exactly once.
  bool loggedGizmoGrab = false;

  // ---- PLACE state (SLICE 6) ---------------------------------------------
  // placeMode is an APP-level mode (not a kernel Tool) toggled by '3'. When on,
  // the click drops a NEW object at the aimed cell instead of running the
  // select/move hit-test. '1'/'2' leave place mode and set the kernel tool.
  // placeBrush is the current descriptor-backed brush kind. The grid pitch
  // (1 m) is the placement cell size for snapping.
  bool placeMode = false;
  const std::vector<creative::CreativeObjectKind> brushPalette =
      buildBrushPaletteFromDescriptors();
  creative::CreativeObjectKind placeBrush = firstBrushKind(brushPalette);
  SDL_Log("iggy3d_creative: brush palette slots=%llu first='%s'",
          static_cast<unsigned long long>(brushPalette.size()),
          std::string(creative::toString(placeBrush)).c_str());
  const double placeCellSize = static_cast<double>(gridConfig.pitchMeters);
  bool prevKey3 = false;
  bool prevKeyB = false;
  bool placeButtonDown = false;  // Interactive left-button edge latch in Place.
  std::uint64_t placedCount = 0;  // Objects dropped via Place (for the count log).
  // --capture: in Place mode we start ON so the proof frames can drop objects.
  if (!capturePath.empty()) {
    placeMode = true;
    placeBrush = firstBrushKind(brushPalette);
  }
  // --capture placement script: place box/surface proof objects, undo one extra
  // create, then add Point and Line representatives before save/load. Each entry
  // is a (worldX, worldZ, kind) target fed to the SAME createDocumentObject path.
  struct CapturePlacement {
    std::uint64_t frame;
    double worldX;
    double worldZ;
    creative::CreativeObjectKind kind;
    bool deleteProofTarget = false;
    bool moveProofTarget = false;
    bool createUndoProofTarget = false;
    bool pointProofTarget = false;
    bool lineProofTarget = false;
    bool pathProofTarget = false;
  };
  const std::array<CapturePlacement, 7> capturePlacements{{
      {3U, 2.0, 2.0, creative::CreativeObjectKind::Crate, false, true,
       false, false, false, false},
      {4U, 4.0, 2.0, creative::CreativeObjectKind::Crate, true, false,
       false, false, false, false},
      {5U, -2.0, 2.0, creative::CreativeObjectKind::Wall, false, false,
       false, false, false, false},
      {6U, 6.0, 2.0, creative::CreativeObjectKind::Crate, false, false,
       true, false, false, false},
      {14U, 6.0, 2.0, creative::CreativeObjectKind::PointLight, false, false,
       false, true, false, false},
      {18U, 9.0, 4.0, creative::CreativeObjectKind::Beam, false, false,
       false, false, true, false},
      {22U, 10.0, 1.0, creative::CreativeObjectKind::PatrolRoute, false, false,
       false, false, false, true},
  }};

  // ---- SAVE / LOAD state (SLICE 7) ---------------------------------------
  // Interactive: edge latches for F5 (save), F6 (new/clear), F9 (load).
  bool prevKeyF5 = false;
  bool prevKeyF6 = false;
  bool prevKeyF9 = false;
  bool prevKeyDelete = false;
  bool prevKeyBackspace = false;
  bool prevKeyZ = false;
  StandaloneUndoStack undoStack;
  // --capture round-trip proof: prove create/delete/move undo, add Point and
  // Line + Path markers, undo their moves, then SAVE/CLEAR/LOAD the eight-object scene. The
  // BEFORE snapshot (taken pre-clear, post-undo) holds the restored proof scene
  // to compare against AFTER.
  std::vector<ObjectSnapshotEntry> roundtripBefore;
  std::size_t roundtripCountAfterClear = 0;
  bool roundtripSaved = false;
  bool roundtripCleared = false;
  bool roundtripLoaded = false;
  bool captureDeleteNoSelectionAttempted = false;
  bool captureCreateUndoAttempted = false;
  bool captureDeleteAttempted = false;
  bool captureUndoAttempted = false;
  bool captureMoveBeginAttempted = false;
  bool captureMovePreviewAttempted = false;
  bool captureMoveCommitAttempted = false;
  bool captureMoveUndoAttempted = false;
  bool capturePointMoveBeginAttempted = false;
  bool capturePointMoveCommitAttempted = false;
  bool capturePointMoveUndoAttempted = false;
  bool capturePointHitProxyLogged = false;
  bool captureLineMoveBeginAttempted = false;
  bool captureLineMoveCommitAttempted = false;
  bool captureLineMoveUndoAttempted = false;
  bool captureLineHitProxyLogged = false;
  bool capturePathMoveAttempted = false;
  bool capturePathMoveUndoAttempted = false;
  bool capturePathHitProxyLogged = false;
  bool capturePathPointMoveAttempted = false;
  bool capturePathPointMoveUndoAttempted = false;
  bool capturePathPointHandleLogged = false;
  constexpr std::uint64_t kCaptureDeleteNoSelectionFrame = 2U;
  constexpr std::uint64_t kCaptureCreateUndoFrame = 7U;
  constexpr std::uint64_t kCaptureDeleteFrame = 8U;
  constexpr std::uint64_t kCaptureUndoFrame = 9U;
  constexpr std::uint64_t kCaptureMoveBeginFrame = 10U;
  constexpr std::uint64_t kCaptureMovePreviewFrame = 11U;
  constexpr std::uint64_t kCaptureMoveCommitFrame = 12U;
  constexpr std::uint64_t kCaptureMoveUndoFrame = 13U;
  constexpr std::uint64_t kCapturePointMoveBeginFrame = 15U;
  constexpr std::uint64_t kCapturePointMoveCommitFrame = 16U;
  constexpr std::uint64_t kCapturePointMoveUndoFrame = 17U;
  constexpr std::uint64_t kCaptureLineMoveBeginFrame = 19U;
  constexpr std::uint64_t kCaptureLineMoveCommitFrame = 20U;
  constexpr std::uint64_t kCaptureLineMoveUndoFrame = 21U;
  constexpr std::uint64_t kCapturePathMoveFrame = 23U;
  constexpr std::uint64_t kCapturePathMoveUndoFrame = 24U;
  constexpr std::uint64_t kCapturePathPointMoveFrame = 25U;
  constexpr std::uint64_t kCapturePathPointMoveUndoFrame = 26U;
  constexpr std::uint64_t kCaptureSaveFrame = 27U;
  constexpr std::uint64_t kCaptureClearFrame = 28U;
  constexpr std::uint64_t kCaptureLoadFrame = 29U;
  creative::CreativeObjectId captureCreateUndoTargetId =
      creative::kInvalidObjectId;
  creative::CreativeObjectId captureDeleteTargetId = creative::kInvalidObjectId;
  creative::CreativeObjectId captureMoveTargetId = creative::kInvalidObjectId;
  creative::CreativeObjectId capturePointTargetId = creative::kInvalidObjectId;
  creative::CreativeObjectId captureLineTargetId = creative::kInvalidObjectId;
  creative::CreativeObjectId capturePathTargetId = creative::kInvalidObjectId;
  creative::CreativeToolWorldPoint captureMoveDestination{};
  creative::CreativeToolWorldPoint capturePointMoveDestination{};
  creative::CreativeToolWorldPoint captureLineMoveDestination{};

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
      const bool key3 = keys[SDL_SCANCODE_3] != 0;
      const bool keyB = keys[SDL_SCANCODE_B] != 0;
      const bool keyDelete = keys[SDL_SCANCODE_DELETE] != 0;
      const bool keyBackspace = keys[SDL_SCANCODE_BACKSPACE] != 0;
      const bool keyZ = keys[SDL_SCANCODE_Z] != 0;
      const SDL_Keymod modState = SDL_GetModState();
      const bool undoModifier =
          (modState & (SDL_KMOD_GUI | SDL_KMOD_CTRL)) != 0U;
      if (key1 && !prevKey1) {
        placeMode = false;  // '1' Select leaves Place mode.
        const bool ok = appState.facade.setActiveTool(creative::Tool::Select);
        SDL_Log("iggy3d_creative: setActiveTool(Select) accepted=%d placeMode=0",
                ok ? 1 : 0);
      }
      if (key2 && !prevKey2) {
        placeMode = false;  // '2' Move leaves Place mode.
        const bool ok = appState.facade.setActiveTool(creative::Tool::Move);
        SDL_Log("iggy3d_creative: setActiveTool(Move) accepted=%d placeMode=0",
                ok ? 1 : 0);
      }
      if (key3 && !prevKey3) {
        placeMode = true;  // '3' Place: app-level mode, not a kernel Tool.
        SDL_Log("iggy3d_creative: placeMode=1 brush='%s'",
                std::string(creative::toString(placeBrush)).c_str());
      }
      if (keyB && !prevKeyB) {
        placeBrush = nextBrushKind(brushPalette, placeBrush);
        SDL_Log("iggy3d_creative: brush cycled -> '%s'",
                std::string(creative::toString(placeBrush)).c_str());
      }
      if ((keyDelete && !prevKeyDelete) ||
          (keyBackspace && !prevKeyBackspace)) {
        (void)deleteSelectedObject(
            appState, keyDelete ? "delete_key" : "backspace_key", &undoStack);
      }
      if (keyZ && !prevKeyZ && undoModifier) {
        (void)undoLastSnapshot(appState, undoStack, "keyboard_undo");
      }
      // ---- SAVE / LOAD keys (SLICE 7): F5 save, F6 new/clear, F9 load -------
      const bool keyF5 = keys[SDL_SCANCODE_F5] != 0;
      const bool keyF6 = keys[SDL_SCANCODE_F6] != 0;
      const bool keyF9 = keys[SDL_SCANCODE_F9] != 0;
      if (keyF5 && !prevKeyF5) {
        const CreativeWorldSaveResult saveResult =
            saveStandaloneScene(appState.facade, saveRoot, saveId);
        if (saveResult.accepted && saveResult.saved) {
          clearUndoStack(undoStack, "save_success");
        }
      }
      if (keyF6 && !prevKeyF6) {
        clearToBlankScene(appState);  // Fresh blank document; facade resets
                                      // selection so no stale seed id dangles.
        clearUndoStack(undoStack, "new_clear");
      }
      if (keyF9 && !prevKeyF9) {
        const bool loaded = loadStandaloneScene(appState, saveRoot, saveId);
        if (loaded) {
          clearUndoStack(undoStack, "load_success");
        }
      }
      prevKey1 = key1;
      prevKey2 = key2;
      prevKey3 = key3;
      prevKeyB = keyB;
      prevKeyDelete = keyDelete;
      prevKeyBackspace = keyBackspace;
      prevKeyZ = keyZ;
      prevKeyF5 = keyF5;
      prevKeyF6 = keyF6;
      prevKeyF9 = keyF9;
    }

    // SCENE (local, must outlive submitFrame): bake supported room geometry
    // through the same CreativeDocument -> RoomAsset adapter that gameplay will
    // eventually consume, then project that RoomAsset through the runtime scene
    // path. Standalone-only editor proxies remain for Point/Line/Path because
    // they are intentionally not RoomAsset static geometry in bake v1.
    creative::CreativeRoomBakeRequest bakeRequest;
    bakeRequest.document = &appState.facade.document();
    bakeRequest.roomId = "iggy3d_creative_preview";
    bakeRequest.sourceName = "apps/iggy3d_creative";
    bakeRequest.sourceSubset = "standalone_preview";
    const creative::CreativeRoomBakeResult roomBake =
        creative::buildRoomAssetFromCreativeDocument(bakeRequest);

    SessionState emptyRuntimeState;
    SceneProjectionResult scene =
        buildSceneProjection(emptyRuntimeState, &roomBake.room);
    appendGridDotsToScene(gridSnapshot, scene);
    const std::size_t standalonePreviewMeshCount =
        appendStandalonePreviewProxiesToScene(appState.facade.document(), scene);
    if (!scene.room.meshes.empty()) {
      scene.room.staticMeshCount = scene.room.meshes.size();
      scene.room.loaded = true;
    }
    DebugProjectionResult debug{};

    // FRAME (non-const so we can attach UI + wireframe + label below). This
    // gives frame.camera.clipFromWorld (world -> NDC) for click + label maths.
    FrameInput frame = makeProductVulkanFrame(
        scene, debug, frameIndex++, extent.width, extent.height, yawDegrees,
        pitchDegrees, /*cameraAnchorOverrideAvailable=*/true, flyPos);

    // ---- AIM -> GROUND CELL (SLICE 6, Place mode) --------------------------
    // Cast the camera-forward ray to the Y=0 plane (eye + forward*t), giving a
    // world XZ ground point, then snap XZ to the nearest 1 m cell center. This
    // is the SAME camera-ray -> Y=0 math the interactive ground-plane Move uses;
    // it feeds both the green ghost preview and the drop position. When the ray
    // is (near) parallel to the ground we fall back to the point under the eye.
    const Vec3 aimEye = frame.camera.worldEye;
    const Vec3 aimFwd = frame.camera.worldForward;
    double aimGroundX = static_cast<double>(aimEye.x);
    double aimGroundZ = static_cast<double>(aimEye.z);
    if (std::fabs(aimFwd.y) > 1.0e-4F) {
      const float t = -aimEye.y / aimFwd.y;  // eye.y + t*fwd.y == 0
      if (t > 0.0F) {
        aimGroundX = static_cast<double>(aimEye.x + aimFwd.x * t);
        aimGroundZ = static_cast<double>(aimEye.z + aimFwd.z * t);
      }
    }
    const Vec3 aimCellCenter =
        snapGroundToCellCenter(aimGroundX, aimGroundZ, placeCellSize);

    // ---- CLICK-TO-SELECT (generic over ALL objects) ------------------------
    // Project EVERY visible object's world bounds to a screen-space pixel AABB,
    // recording its center clip-w as a camera-depth key. A click picks the hit
    // object NEAREST the camera (smallest depth); a miss clears selection. This
    // is fully generic: it never mentions crate vs floor, only object ids.
    struct ObjectScreenHit {
      creative::CreativeObjectId id = creative::kInvalidObjectId;
      ScreenAabb aabb;      // Projected pixel AABB for the pointer-in-box test.
      float centerDepth = std::numeric_limits<float>::max();  // clip-w at center.
    };
    std::vector<ObjectScreenHit> objectHits;
    // Remember the FLOOR's world bounds so --capture can aim its click at a point
    // on the floor OUTSIDE the crate's footprint (the crate sits over the floor's
    // center, so a center-click would land on the nearer crate — nearest wins).
    bool haveFloorBounds = false;
    Vec3 floorBoxMin{};
    Vec3 floorBoxMax{};
    for (const creative::CreativeObject& obj : appState.facade.document().objects()) {
      if (!obj.visible) {
        continue;
      }
      const VisualBounds visualBounds = visualBoundsForObject(obj);
      const Vec3 boxMin = visualBounds.min;
      const Vec3 boxMax = visualBounds.max;
      ObjectScreenHit hit;
      hit.id = obj.id;
      hit.aabb = projectBoxToScreen(frame.camera.clipFromWorld, boxMin, boxMax,
                                    extent.width, extent.height);
      const Vec3 center = visualBoundsCenter(visualBounds);
      hit.centerDepth = clipW(frame.camera.clipFromWorld, center);
      objectHits.push_back(hit);
      if (obj.id == floorObjectId) {
        haveFloorBounds = true;
        floorBoxMin = boxMin;
        floorBoxMax = boxMax;
      }
      if (!capturePath.empty() && !capturePointHitProxyLogged &&
          obj.id == capturePointTargetId) {
        SDL_Log("iggy3d_creative: POINT hit proxy objectId=%llu "
                "aabbValid=%d marker=[(%.3f, %.3f, %.3f).."
                "(%.3f, %.3f, %.3f)] screen=[%.1f, %.1f..%.1f, %.1f]",
                static_cast<unsigned long long>(capturePointTargetId),
                hit.aabb.valid ? 1 : 0, boxMin.x, boxMin.y, boxMin.z,
                boxMax.x, boxMax.y, boxMax.z, hit.aabb.minX, hit.aabb.minY,
                hit.aabb.maxX, hit.aabb.maxY);
        capturePointHitProxyLogged = true;
      }
      if (!capturePath.empty() && !captureLineHitProxyLogged &&
          obj.id == captureLineTargetId) {
        SDL_Log("iggy3d_creative: LINE hit proxy objectId=%llu "
                "aabbValid=%d visual=[(%.3f, %.3f, %.3f).."
                "(%.3f, %.3f, %.3f)] screen=[%.1f, %.1f..%.1f, %.1f]",
                static_cast<unsigned long long>(captureLineTargetId),
                hit.aabb.valid ? 1 : 0, boxMin.x, boxMin.y, boxMin.z,
                boxMax.x, boxMax.y, boxMax.z, hit.aabb.minX, hit.aabb.minY,
                hit.aabb.maxX, hit.aabb.maxY);
        captureLineHitProxyLogged = true;
      }
      if (!capturePath.empty() && !capturePathHitProxyLogged &&
          obj.id == capturePathTargetId) {
        SDL_Log("iggy3d_creative: PATH hit proxy objectId=%llu "
                "aabbValid=%d visual=[(%.3f, %.3f, %.3f).."
                "(%.3f, %.3f, %.3f)] screen=[%.1f, %.1f..%.1f, %.1f] "
                "pathPointCount=%zu pathPoints='%s'",
                static_cast<unsigned long long>(capturePathTargetId),
                hit.aabb.valid ? 1 : 0, boxMin.x, boxMin.y, boxMin.z,
                boxMax.x, boxMax.y, boxMax.z, hit.aabb.minX, hit.aabb.minY,
                hit.aabb.maxX, hit.aabb.maxY, obj.pathPoints.size(),
                pathPointsSummary(obj.pathPoints).c_str());
        capturePathHitProxyLogged = true;
      }
    }

    bool clickRequested = false;
    float clickX = 0.0F;
    float clickY = 0.0F;
    if (!capturePath.empty() && placeMode) {
      // Place-mode capture: no select-click is synthesized; the placement script
      // below drops objects directly. Leave clickRequested false.
    } else if (!capturePath.empty()) {
      // Synthesize a click on the FLOOR once the swapchain has settled (frame ~2),
      // aimed at a point on the floor's TOP surface near a corner — a point the
      // crate does NOT cover — so the generic nearest-hit picker selects the FLOOR
      // and not the crate. We project that single world point through the SAME
      // clipFromWorld the hit-test uses.
      if (frameIndex == 3U && haveFloorBounds) {
        // 80% out toward the +X/+Z corner of the floor top, well past the crate's
        // XZ footprint. This is only which pixel we click; the pick logic itself
        // is unchanged and object-agnostic.
        const Vec3 floorTopCorner{
            floorBoxMin.x + (floorBoxMax.x - floorBoxMin.x) * 0.85F,
            floorBoxMax.y,
            floorBoxMin.z + (floorBoxMax.z - floorBoxMin.z) * 0.85F};
        const ScreenPoint p = projectPointToScreen(
            frame.camera.clipFromWorld, floorTopCorner, extent.width,
            extent.height);
        if (p.valid) {
          clickRequested = true;
          clickX = p.x;
          clickY = p.y;
        }
      }
    } else if (!placeMode) {
      // Interactive: hold Left-Alt to release fly-look and click to select.
      // Skipped in Place mode — the interactive Place block below owns the click.
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
      // Nearest hit wins: scan every object's projected AABB for a pointer-in-box
      // hit and keep the one with the smallest center depth (closest to camera).
      creative::CreativeObjectId pickedId = creative::kInvalidObjectId;
      float pickedDepth = std::numeric_limits<float>::max();
      for (const ObjectScreenHit& h : objectHits) {
        if (!h.aabb.valid || clickX < h.aabb.minX || clickX > h.aabb.maxX ||
            clickY < h.aabb.minY || clickY > h.aabb.maxY) {
          continue;
        }
        if (h.centerDepth < pickedDepth) {
          pickedDepth = h.centerDepth;
          pickedId = h.id;
        }
      }
      creative::CreativeToolInputPacket packet;
      packet.kind = creative::CreativeToolInputKind::PointerPress;
      packet.pointer.button = creative::CreativeToolPointerButton::Primary;
      if (pickedId != creative::kInvalidObjectId) {
        packet.pointer.target =
            creative::TargetRef{static_cast<creative::Id>(pickedId)};
      }  // A miss leaves target invalid -> Select clears selection.
      (void)appState.facade.dispatchToolInput(packet);
    }

    // ---- PLACE (SLICE 6) ---------------------------------------------------
    // In Place mode a click drops a NEW object of the current brush kind at the
    // aimed cell, snapped to the grid, via the SAME generic createDocumentObject.
    // The new object joins the document immediately, so next frame it renders and
    // is Select/Move/Gizmo-able with ZERO extra code. There is NO per-kind place
    // branch; placeBrushObject() reads descriptor-derived footprint geometry.
    if (placeMode) {
      if (!capturePath.empty()) {
        // --capture: run the scripted placements. Each entry sets the brush kind
        // then drops at its target world XZ, snapped to the cell center — proving
        // the aim->cell->createDocumentObject pipeline the interactive path uses.
        for (const CapturePlacement& p : capturePlacements) {
          if (frameIndex == p.frame) {
            placeBrush = p.kind;  // Brush is data; switching kinds is not a branch.
            const Vec3 cell = snapGroundToCellCenter(p.worldX, p.worldZ,
                                                     placeCellSize);
            const creative::CreativeDocumentCreateReceipt receipt =
                placeBrushObjectWithUndo(appState.facade, undoStack, placeBrush,
                                         cell, ++placedCount,
                                         p.createUndoProofTarget
                                             ? "capture_place_create_target"
                                             : "capture_place");
            if (p.createUndoProofTarget && receipt.accepted) {
              captureCreateUndoTargetId = receipt.objectId;
              SDL_Log("iggy3d_creative: CREATE_UNDO capture target objectId=%llu "
                      "kind='%s'",
                      static_cast<unsigned long long>(
                          captureCreateUndoTargetId),
                      std::string(creative::toString(receipt.objectKind)).c_str());
            }
            if (p.deleteProofTarget && receipt.accepted) {
              captureDeleteTargetId = receipt.objectId;
              SDL_Log("iggy3d_creative: DELETE capture target objectId=%llu "
                      "kind='%s'",
                      static_cast<unsigned long long>(captureDeleteTargetId),
                      std::string(creative::toString(receipt.objectKind)).c_str());
            }
            if (p.moveProofTarget && receipt.accepted) {
              captureMoveTargetId = receipt.objectId;
              SDL_Log("iggy3d_creative: MOVE_UNDO capture target objectId=%llu "
                      "kind='%s'",
                      static_cast<unsigned long long>(captureMoveTargetId),
                      std::string(creative::toString(receipt.objectKind)).c_str());
            }
            if (p.pointProofTarget && receipt.accepted) {
              capturePointTargetId = receipt.objectId;
              SDL_Log("iggy3d_creative: POINT capture target objectId=%llu "
                      "kind='%s' shape='%s' boundsOverride=%d",
                      static_cast<unsigned long long>(capturePointTargetId),
                      std::string(creative::toString(receipt.objectKind)).c_str(),
                      std::string(creative::toString(
                          creative::describeObject(receipt.objectKind).shapeKind))
                          .c_str(),
                      0);
            }
            if (p.lineProofTarget && receipt.accepted) {
              captureLineTargetId = receipt.objectId;
              const creative::CreativeObject* lineTarget =
                  appState.facade.findObject(captureLineTargetId);
              const VisualBounds lineVisual =
                  lineTarget != nullptr
                      ? visualBoundsForObject(*lineTarget)
                      : VisualBounds{};
              SDL_Log("iggy3d_creative: LINE capture target objectId=%llu "
                      "kind='%s' shape='%s' projection='%s' occupancy='%s' "
                      "boundsOverride=%d visual=[(%.3f, %.3f, %.3f).."
                      "(%.3f, %.3f, %.3f)]",
                      static_cast<unsigned long long>(captureLineTargetId),
                      std::string(creative::toString(receipt.objectKind)).c_str(),
                      std::string(creative::toString(
                          creative::describeObject(receipt.objectKind).shapeKind))
                          .c_str(),
                      std::string(creative::toString(
                          creative::describeObject(receipt.objectKind)
                              .projectionProfile))
                          .c_str(),
                      std::string(creative::toString(
                          creative::describeObject(receipt.objectKind)
                              .occupancyKind))
                          .c_str(),
                      lineTarget != nullptr ? 1 : 0, lineVisual.min.x,
                      lineVisual.min.y, lineVisual.min.z, lineVisual.max.x,
                      lineVisual.max.y, lineVisual.max.z);
            }
            if (p.pathProofTarget && receipt.accepted) {
              capturePathTargetId = receipt.objectId;
              const creative::CreativeObject* pathTarget =
                  appState.facade.findObject(capturePathTargetId);
              const VisualBounds pathVisual =
                  pathTarget != nullptr
                      ? visualBoundsForObject(*pathTarget)
                      : VisualBounds{};
              SDL_Log("iggy3d_creative: PATH capture target objectId=%llu "
                      "kind='%s' shape='%s' projection='%s' occupancy='%s' "
                      "pathOverride=1 pathPointCount=%zu pathPoints='%s' "
                      "visual=[(%.3f, %.3f, %.3f)..(%.3f, %.3f, %.3f)]",
                      static_cast<unsigned long long>(capturePathTargetId),
                      std::string(creative::toString(receipt.objectKind)).c_str(),
                      std::string(creative::toString(
                          creative::describeObject(receipt.objectKind).shapeKind))
                          .c_str(),
                      std::string(creative::toString(
                          creative::describeObject(receipt.objectKind)
                              .projectionProfile))
                          .c_str(),
                      std::string(creative::toString(
                          creative::describeObject(receipt.objectKind)
                              .occupancyKind))
                          .c_str(),
                      pathTarget != nullptr ? pathTarget->pathPoints.size() : 0U,
                      pathTarget != nullptr
                          ? pathPointsSummary(pathTarget->pathPoints).c_str()
                          : "",
                      pathVisual.min.x, pathVisual.min.y, pathVisual.min.z,
                      pathVisual.max.x, pathVisual.max.y, pathVisual.max.z);
            }
          }
        }
      } else {
        // Interactive: hold Left-Alt (release fly-look) and left-click to drop at
        // the aimed cell. Edge-triggered so one click drops exactly one object.
        const bool* plKeys = SDL_GetKeyboardState(nullptr);
        const bool altHeld =
            plKeys != nullptr && (plKeys[SDL_SCANCODE_LALT] != 0);
        if (altHeld) {
          window.setRelativeMouseMode(false);
          float mx = 0.0F;
          float my = 0.0F;
          const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
          const bool lDown = (buttons & SDL_BUTTON_LMASK) != 0U;
          if (lDown && !placeButtonDown) {
            placeButtonDown = true;
            (void)placeBrushObjectWithUndo(appState.facade, undoStack,
                                           placeBrush, aimCellCenter,
                                           ++placedCount,
                                           "place_interactive");
          } else if (!lDown) {
            placeButtonDown = false;
          }
        } else {
          window.setRelativeMouseMode(true);
          placeButtonDown = false;
        }
      }
    }

    // ---- DELETE PROOF (SLICE B, --capture) --------------------------------
    // Capture proves correction paths: no-selection delete is a clean no-op,
    // extra placement undo removes the new object, then a placed Crate is
    // selected through the generic tool input packet and removed through
    // Facade::removeDocumentObject. Slice C then restores that exact pre-delete
    // document through the app-local snapshot undo before save/open.
    if (!capturePath.empty()) {
      if (frameIndex == kCaptureDeleteNoSelectionFrame &&
          !captureDeleteNoSelectionAttempted) {
        (void)deleteSelectedObject(appState, "capture_no_selection", &undoStack);
        captureDeleteNoSelectionAttempted = true;
      } else if (frameIndex == kCaptureCreateUndoFrame &&
                 !captureCreateUndoAttempted) {
        SDL_Log("iggy3d_creative: CREATE_UNDO attempting target objectId=%llu "
                "objectCountBefore=%llu",
                static_cast<unsigned long long>(captureCreateUndoTargetId),
                static_cast<unsigned long long>(
                    appState.facade.document().objectCount()));
        (void)undoLastSnapshot(appState, undoStack, "capture_create_undo");
        SDL_Log("iggy3d_creative: CREATE_UNDO objectCountAfter=%llu "
                "targetPresentAfter=%d",
                static_cast<unsigned long long>(
                    appState.facade.document().objectCount()),
                appState.facade.findObject(captureCreateUndoTargetId) != nullptr
                    ? 1
                    : 0);
        captureCreateUndoAttempted = true;
      } else if (frameIndex == kCaptureDeleteFrame && !captureDeleteAttempted) {
        (void)selectObjectForCapture(appState.facade, captureDeleteTargetId,
                                     "capture_delete");
        (void)deleteSelectedObject(appState, "capture_delete", &undoStack);
        captureDeleteAttempted = true;
      } else if (frameIndex == kCaptureUndoFrame && !captureUndoAttempted) {
        (void)undoLastSnapshot(appState, undoStack, "capture_undo");
        captureUndoAttempted = true;
      }
    }

    // ---- MOVE UNDO PROOF (C2, --capture) ----------------------------------
    // Drive one real Move through the facade packet lifecycle, keep the snapshot
    // only when the release commits a changed move, then undo it before save.
    if (!capturePath.empty()) {
      if (frameIndex == kCaptureMoveBeginFrame && !captureMoveBeginAttempted) {
        placeMode = false;
        (void)appState.facade.setActiveTool(creative::Tool::Move);
        (void)selectObjectForCapture(appState.facade, captureMoveTargetId,
                                     "capture_move");
        const creative::CreativeObject* moveTarget =
            appState.facade.findObject(captureMoveTargetId);
        if (moveTarget != nullptr) {
          captureMoveDestination = {moveTarget->transform.position.x + 2.0,
                                    moveTarget->transform.position.y,
                                    moveTarget->transform.position.z};
          logUndoMovePlacement("capture_begin", captureMoveTargetId,
                               moveTarget);
          creative::CreativeToolInputPacket press;
          press.kind = creative::CreativeToolInputKind::PointerPress;
          press.pointer.button = creative::CreativeToolPointerButton::Primary;
          press.pointer.target =
              creative::TargetRef{static_cast<creative::Id>(
                  captureMoveTargetId)};
          const creative::CreativeFacadeToolDispatchReceipt receipt =
              appState.facade.dispatchToolInput(press);
          logMoveDispatch("CAPTURE_MOVE_PRESS", receipt);
        }
        captureMoveBeginAttempted = true;
      } else if (frameIndex == kCaptureMovePreviewFrame &&
                 !captureMovePreviewAttempted) {
        creative::CreativeToolInputPacket move;
        move.kind = creative::CreativeToolInputKind::PointerMove;
        move.pointer.button = creative::CreativeToolPointerButton::Primary;
        move.pointer.hasWorldDestination = true;
        move.pointer.worldDestination = captureMoveDestination;
        move.pointer.moveHeldAxis = heldAxisForGrabbedAxis(GizmoAxis::X);
        const creative::CreativeFacadeToolDispatchReceipt receipt =
            appState.facade.dispatchToolInput(move);
        logMoveDispatch("CAPTURE_MOVE", receipt);
        captureMovePreviewAttempted = true;
      } else if (frameIndex == kCaptureMoveCommitFrame &&
                 !captureMoveCommitAttempted) {
        creative::CreativeToolInputPacket release;
        release.kind = creative::CreativeToolInputKind::PointerRelease;
        release.pointer.button = creative::CreativeToolPointerButton::Primary;
        release.pointer.hasWorldDestination = true;
        release.pointer.worldDestination = captureMoveDestination;
        release.pointer.moveHeldAxis = heldAxisForGrabbedAxis(GizmoAxis::X);
        const creative::CreativeFacadeToolDispatchReceipt receipt =
            dispatchMoveReleaseWithUndo(appState, undoStack, release,
                                        captureMoveTargetId,
                                        "capture_move_commit");
        logMoveDispatch("CAPTURE_MOVE_RELEASE", receipt);
        captureMoveCommitAttempted = true;
      } else if (frameIndex == kCaptureMoveUndoFrame &&
                 !captureMoveUndoAttempted) {
        (void)undoLastSnapshot(appState, undoStack, "capture_move_undo");
        placeMode = true;
        placeBrush = creative::CreativeObjectKind::Wall;
        logUndoMovePlacement("capture_undo_after", captureMoveTargetId,
                             appState.facade.findObject(captureMoveTargetId));
        captureMoveUndoAttempted = true;
      }
    }

    // ---- POINT SHAPE PROOF (D1, --capture) --------------------------------
    // PointLight is only the deterministic representative brush here. Rendering,
    // hit bounds, selection and Move use descriptor shape facts, not this kind.
    if (!capturePath.empty()) {
      if (frameIndex == kCapturePointMoveBeginFrame &&
          !capturePointMoveBeginAttempted) {
        placeMode = false;
        (void)appState.facade.setActiveTool(creative::Tool::Move);
        (void)selectObjectForCapture(appState.facade, capturePointTargetId,
                                     "capture_point_move");
        const creative::CreativeObject* pointTarget =
            appState.facade.findObject(capturePointTargetId);
        if (pointTarget != nullptr) {
          const VisualBounds markerBounds = visualBoundsForObject(*pointTarget);
          capturePointMoveDestination = {
              pointTarget->transform.position.x + 1.0,
              pointTarget->transform.position.y,
              pointTarget->transform.position.z};
          SDL_Log("iggy3d_creative: POINT before move objectId=%llu "
                  "pos=(%.3f, %.3f, %.3f) marker=[(%.3f, %.3f, %.3f).."
                  "(%.3f, %.3f, %.3f)]",
                  static_cast<unsigned long long>(capturePointTargetId),
                  pointTarget->transform.position.x,
                  pointTarget->transform.position.y,
                  pointTarget->transform.position.z, markerBounds.min.x,
                  markerBounds.min.y, markerBounds.min.z, markerBounds.max.x,
                  markerBounds.max.y, markerBounds.max.z);
          creative::CreativeToolInputPacket press;
          press.kind = creative::CreativeToolInputKind::PointerPress;
          press.pointer.button = creative::CreativeToolPointerButton::Primary;
          press.pointer.target =
              creative::TargetRef{static_cast<creative::Id>(
                  capturePointTargetId)};
          const creative::CreativeFacadeToolDispatchReceipt receipt =
              appState.facade.dispatchToolInput(press);
          logMoveDispatch("POINT_MOVE_PRESS", receipt);
        }
        capturePointMoveBeginAttempted = true;
      } else if (frameIndex == kCapturePointMoveCommitFrame &&
                 !capturePointMoveCommitAttempted) {
        creative::CreativeToolInputPacket release;
        release.kind = creative::CreativeToolInputKind::PointerRelease;
        release.pointer.button = creative::CreativeToolPointerButton::Primary;
        release.pointer.hasWorldDestination = true;
        release.pointer.worldDestination = capturePointMoveDestination;
        release.pointer.moveHeldAxis = heldAxisForGrabbedAxis(GizmoAxis::X);
        const creative::CreativeFacadeToolDispatchReceipt receipt =
            dispatchMoveReleaseWithUndo(appState, undoStack, release,
                                        capturePointTargetId,
                                        "capture_point_move_commit");
        logMoveDispatch("POINT_MOVE_RELEASE", receipt);
        const creative::CreativeObject* pointTarget =
            appState.facade.findObject(capturePointTargetId);
        if (pointTarget != nullptr) {
          const VisualBounds markerBounds = visualBoundsForObject(*pointTarget);
          SDL_Log("iggy3d_creative: POINT after move objectId=%llu "
                  "pos=(%.3f, %.3f, %.3f) marker=[(%.3f, %.3f, %.3f).."
                  "(%.3f, %.3f, %.3f)]",
                  static_cast<unsigned long long>(capturePointTargetId),
                  pointTarget->transform.position.x,
                  pointTarget->transform.position.y,
                  pointTarget->transform.position.z, markerBounds.min.x,
                  markerBounds.min.y, markerBounds.min.z, markerBounds.max.x,
                  markerBounds.max.y, markerBounds.max.z);
        }
        capturePointMoveCommitAttempted = true;
      } else if (frameIndex == kCapturePointMoveUndoFrame &&
                 !capturePointMoveUndoAttempted) {
        (void)undoLastSnapshot(appState, undoStack, "capture_point_move_undo");
        placeMode = true;
        placeBrush = creative::CreativeObjectKind::Wall;
        const creative::CreativeObject* pointTarget =
            appState.facade.findObject(capturePointTargetId);
        if (pointTarget != nullptr) {
          const VisualBounds markerBounds = visualBoundsForObject(*pointTarget);
          SDL_Log("iggy3d_creative: POINT after undo objectId=%llu "
                  "pos=(%.3f, %.3f, %.3f) marker=[(%.3f, %.3f, %.3f).."
                  "(%.3f, %.3f, %.3f)]",
                  static_cast<unsigned long long>(capturePointTargetId),
                  pointTarget->transform.position.x,
                  pointTarget->transform.position.y,
                  pointTarget->transform.position.z, markerBounds.min.x,
                  markerBounds.min.y, markerBounds.min.z, markerBounds.max.x,
                  markerBounds.max.y, markerBounds.max.z);
        }
        capturePointMoveUndoAttempted = true;
      }
    }

    // ---- LINE SHAPE PROOF (D2, --capture) ---------------------------------
    // Beam is only the deterministic representative here. Rendering, hit bounds,
    // selection and Move use descriptor Line shape facts and bounds-backed
    // document truth, not this kind name.
    if (!capturePath.empty()) {
      if (frameIndex == kCaptureLineMoveBeginFrame &&
          !captureLineMoveBeginAttempted) {
        placeMode = false;
        (void)appState.facade.setActiveTool(creative::Tool::Move);
        (void)selectObjectForCapture(appState.facade, captureLineTargetId,
                                     "capture_line_move");
        const creative::CreativeObject* lineTarget =
            appState.facade.findObject(captureLineTargetId);
        if (lineTarget != nullptr) {
          const VisualBounds authoredBounds{toVec3(lineTarget->bounds.min),
                                            toVec3(lineTarget->bounds.max)};
          const VisualBounds lineVisual = visualBoundsForObject(*lineTarget);
          captureLineMoveDestination = {
              lineTarget->transform.position.x,
              lineTarget->transform.position.y,
              lineTarget->transform.position.z + 1.0};
          SDL_Log("iggy3d_creative: LINE before move objectId=%llu "
                  "pos=(%.3f, %.3f, %.3f) authored=[(%.3f, %.3f, %.3f).."
                  "(%.3f, %.3f, %.3f)] visual=[(%.3f, %.3f, %.3f).."
                  "(%.3f, %.3f, %.3f)]",
                  static_cast<unsigned long long>(captureLineTargetId),
                  lineTarget->transform.position.x,
                  lineTarget->transform.position.y,
                  lineTarget->transform.position.z, authoredBounds.min.x,
                  authoredBounds.min.y, authoredBounds.min.z,
                  authoredBounds.max.x, authoredBounds.max.y,
                  authoredBounds.max.z, lineVisual.min.x, lineVisual.min.y,
                  lineVisual.min.z, lineVisual.max.x, lineVisual.max.y,
                  lineVisual.max.z);
          creative::CreativeToolInputPacket press;
          press.kind = creative::CreativeToolInputKind::PointerPress;
          press.pointer.button = creative::CreativeToolPointerButton::Primary;
          press.pointer.target =
              creative::TargetRef{static_cast<creative::Id>(
                  captureLineTargetId)};
          const creative::CreativeFacadeToolDispatchReceipt receipt =
              appState.facade.dispatchToolInput(press);
          logMoveDispatch("LINE_MOVE_PRESS", receipt);
        }
        captureLineMoveBeginAttempted = true;
      } else if (frameIndex == kCaptureLineMoveCommitFrame &&
                 !captureLineMoveCommitAttempted) {
        creative::CreativeToolInputPacket release;
        release.kind = creative::CreativeToolInputKind::PointerRelease;
        release.pointer.button = creative::CreativeToolPointerButton::Primary;
        release.pointer.hasWorldDestination = true;
        release.pointer.worldDestination = captureLineMoveDestination;
        release.pointer.moveHeldAxis = heldAxisForGrabbedAxis(GizmoAxis::Z);
        const creative::CreativeFacadeToolDispatchReceipt receipt =
            dispatchMoveReleaseWithUndo(appState, undoStack, release,
                                        captureLineTargetId,
                                        "capture_line_move_commit");
        logMoveDispatch("LINE_MOVE_RELEASE", receipt);
        const creative::CreativeObject* lineTarget =
            appState.facade.findObject(captureLineTargetId);
        if (lineTarget != nullptr) {
          const VisualBounds authoredBounds{toVec3(lineTarget->bounds.min),
                                            toVec3(lineTarget->bounds.max)};
          const VisualBounds lineVisual = visualBoundsForObject(*lineTarget);
          SDL_Log("iggy3d_creative: LINE after move objectId=%llu "
                  "pos=(%.3f, %.3f, %.3f) authored=[(%.3f, %.3f, %.3f).."
                  "(%.3f, %.3f, %.3f)] visual=[(%.3f, %.3f, %.3f).."
                  "(%.3f, %.3f, %.3f)]",
                  static_cast<unsigned long long>(captureLineTargetId),
                  lineTarget->transform.position.x,
                  lineTarget->transform.position.y,
                  lineTarget->transform.position.z, authoredBounds.min.x,
                  authoredBounds.min.y, authoredBounds.min.z,
                  authoredBounds.max.x, authoredBounds.max.y,
                  authoredBounds.max.z, lineVisual.min.x, lineVisual.min.y,
                  lineVisual.min.z, lineVisual.max.x, lineVisual.max.y,
                  lineVisual.max.z);
        }
        captureLineMoveCommitAttempted = true;
      } else if (frameIndex == kCaptureLineMoveUndoFrame &&
                 !captureLineMoveUndoAttempted) {
        (void)undoLastSnapshot(appState, undoStack, "capture_line_move_undo");
        placeMode = true;
        placeBrush = creative::CreativeObjectKind::Wall;
        const creative::CreativeObject* lineTarget =
            appState.facade.findObject(captureLineTargetId);
        if (lineTarget != nullptr) {
          const VisualBounds authoredBounds{toVec3(lineTarget->bounds.min),
                                            toVec3(lineTarget->bounds.max)};
          const VisualBounds lineVisual = visualBoundsForObject(*lineTarget);
          SDL_Log("iggy3d_creative: LINE after undo objectId=%llu "
                  "pos=(%.3f, %.3f, %.3f) authored=[(%.3f, %.3f, %.3f).."
                  "(%.3f, %.3f, %.3f)] visual=[(%.3f, %.3f, %.3f).."
                  "(%.3f, %.3f, %.3f)]",
                  static_cast<unsigned long long>(captureLineTargetId),
                  lineTarget->transform.position.x,
                  lineTarget->transform.position.y,
                  lineTarget->transform.position.z, authoredBounds.min.x,
                  authoredBounds.min.y, authoredBounds.min.z,
                  authoredBounds.max.x, authoredBounds.max.y,
                  authoredBounds.max.z, lineVisual.min.x, lineVisual.min.y,
                  lineVisual.min.z, lineVisual.max.x, lineVisual.max.y,
                  lineVisual.max.z);
        }
        captureLineMoveUndoAttempted = true;
      }
    }

    // ---- PATH SHAPE PROOF (D3f, --capture) --------------------------------
    // PatrolRoute is only the deterministic representative. Placement stores
    // ordered pathPoints; whole-route move translates those points through the
    // public SetPatrolRoute mutation payload, then snapshot undo restores them.
    if (!capturePath.empty()) {
      if (frameIndex == kCapturePathMoveFrame && !capturePathMoveAttempted) {
        placeMode = false;
        (void)selectObjectForCapture(appState.facade, capturePathTargetId,
                                     "capture_path_move");
        const creative::CreativeDocumentMutationReceipt receipt =
            movePathObjectWithUndo(appState, undoStack, capturePathTargetId,
                                   {1.0, 0.0, 1.0},
                                   "capture_path_move_commit");
        SDL_Log("iggy3d_creative: PATH mutation receipt status='%s' changed=%d "
                "revisionBefore=%llu revisionAfter=%llu dirtyFlags=%llu",
                std::string(creative::toString(receipt.status)).c_str(),
                receipt.changed ? 1 : 0,
                static_cast<unsigned long long>(receipt.revisionBefore),
                static_cast<unsigned long long>(receipt.revisionAfter),
                static_cast<unsigned long long>(receipt.dirtyFlags));
        capturePathMoveAttempted = true;
      } else if (frameIndex == kCapturePathMoveUndoFrame &&
                 !capturePathMoveUndoAttempted) {
        (void)undoLastSnapshot(appState, undoStack, "capture_path_move_undo");
        placeMode = true;
        placeBrush = creative::CreativeObjectKind::Wall;
        const creative::CreativeObject* pathTarget =
            appState.facade.findObject(capturePathTargetId);
        if (pathTarget != nullptr) {
          SDL_Log("iggy3d_creative: PATH after undo objectId=%llu "
                  "pathPointCount=%zu pathPoints='%s'",
                  static_cast<unsigned long long>(capturePathTargetId),
                  pathTarget->pathPoints.size(),
                  pathPointsSummary(pathTarget->pathPoints).c_str());
        }
        capturePathMoveUndoAttempted = true;
      } else if (frameIndex == kCapturePathPointMoveFrame &&
                 !capturePathPointMoveAttempted) {
        placeMode = false;
        (void)selectObjectForCapture(appState.facade, capturePathTargetId,
                                     "capture_path_point_move");
        constexpr std::size_t kCapturePathPointIndex = 2U;
        const creative::CreativeDocumentMutationReceipt receipt =
            movePathPointWithUndo(appState,
                                  undoStack,
                                  capturePathTargetId,
                                  kCapturePathPointIndex,
                                  {0.0, 0.0, 1.0},
                                  "capture_path_point_move_commit");
        SDL_Log("iggy3d_creative: PATH_HANDLE mutation receipt status='%s' "
                "changed=%d revisionBefore=%llu revisionAfter=%llu "
                "dirtyFlags=%llu pointIndex=%zu",
                std::string(creative::toString(receipt.status)).c_str(),
                receipt.changed ? 1 : 0,
                static_cast<unsigned long long>(receipt.revisionBefore),
                static_cast<unsigned long long>(receipt.revisionAfter),
                static_cast<unsigned long long>(receipt.dirtyFlags),
                kCapturePathPointIndex);
        capturePathPointMoveAttempted = true;
      } else if (frameIndex == kCapturePathPointMoveUndoFrame &&
                 !capturePathPointMoveUndoAttempted) {
        (void)undoLastSnapshot(appState,
                               undoStack,
                               "capture_path_point_move_undo");
        placeMode = true;
        placeBrush = creative::CreativeObjectKind::Wall;
        const creative::CreativeObject* pathTarget =
            appState.facade.findObject(capturePathTargetId);
        if (pathTarget != nullptr) {
          SDL_Log("iggy3d_creative: PATH_HANDLE after undo objectId=%llu "
                  "pathPointCount=%zu pathPoints='%s'",
                  static_cast<unsigned long long>(capturePathTargetId),
                  pathTarget->pathPoints.size(),
                  pathPointsSummary(pathTarget->pathPoints).c_str());
        }
        capturePathPointMoveUndoAttempted = true;
      }
    }

    // ---- SAVE / LOAD ROUND-TRIP (SLICE 7, --capture) -----------------------
    // On fixed frames, drive the lossless round-trip and log the proof. The
    // placements above grow the scene, create/delete/move undo return it to the
    // intended eight-object proof, then SAVE/CLEAR/LOAD proves persistence. The
    // whole sequence reuses the kernel's creative-save/open — no new
    // serialization here. All reads go through document().objects(), so no
    // hardcoded id can dangle after the clear or the load.
    if (!capturePath.empty()) {
      if (frameIndex == kCaptureSaveFrame && !roundtripSaved) {
        // Snapshot BEFORE (post-place, pre-clear) then SAVE the live document.
        roundtripBefore = snapshotDocument(appState.facade.document());
        logDocumentSnapshot("BEFORE", roundtripBefore);
        const CreativeWorldSaveResult saveResult =
            saveStandaloneScene(appState.facade, saveRoot, saveId);
        if (saveResult.accepted && saveResult.saved) {
          clearUndoStack(undoStack, "capture_save_success");
        }
        roundtripSaved = true;
      } else if (frameIndex == kCaptureClearFrame && !roundtripCleared) {
        // CLEAR: install a blank document (facade resets selection); expect 0.
        clearToBlankScene(appState);
        clearUndoStack(undoStack, "capture_clear");
        roundtripCountAfterClear = appState.facade.document().objectCount();
        roundtripCleared = true;
      } else if (frameIndex == kCaptureLoadFrame && !roundtripLoaded) {
        // LOAD: restore from disk and install; snapshot AFTER + ROUNDTRIP line.
        const bool loaded = loadStandaloneScene(appState, saveRoot, saveId);
        if (loaded) {
          clearUndoStack(undoStack, "capture_load_success");
        }
        const std::vector<ObjectSnapshotEntry> roundtripAfter =
            snapshotDocument(appState.facade.document());
        logDocumentSnapshot("AFTER", roundtripAfter);
        const bool match = snapshotsMatch(roundtripBefore, roundtripAfter);
        SDL_Log("iggy3d_creative: ROUNDTRIP objectCount before=%zu afterClear=%zu "
                "afterLoad=%zu match=%d",
                roundtripBefore.size(), roundtripCountAfterClear,
                roundtripAfter.size(), match ? 1 : 0);
        roundtripLoaded = true;
      }
    }

    // ---- RESOLVE THE SELECTION (generic) -----------------------------------
    // Everything downstream — the yellow box, the gizmo, the dimension label, and
    // the Move — keys off the CURRENTLY SELECTED object id, looked up via the same
    // findObject the inspector uses. No hardcoded crate id, no kind check. When
    // nothing is selected we draw no gizmo/box and skip Move.
    const creative::Id selectedId =
        appState.facade.selectionState().selectedTarget.value;
    const creative::CreativeObject* selected =
        selectedId != 0
            ? appState.facade.findObject(
                  static_cast<creative::CreativeObjectId>(selectedId))
            : nullptr;
    const bool hasSelection = selected != nullptr && selected->visible;
    // Selected object's world bounds (defaults are unused when !hasSelection).
    Vec3 selBoxMin{-0.5F, 0.0F, -0.5F};
    Vec3 selBoxMax{0.5F, 1.0F, 0.5F};
    if (hasSelection) {
      const VisualBounds selectedVisualBounds = visualBoundsForObject(*selected);
      selBoxMin = selectedVisualBounds.min;
      selBoxMax = selectedVisualBounds.max;
    }

    // ---- GIZMO GEOMETRY (SLICE 4) ------------------------------------------
    // Build the 3 axis shafts at the selected object's center C = (min+max)/2.
    // Each shaft is a single AXIS-ALIGNED world segment (start=C, end=C+dir*L),
    // which is the ONLY geometry the renderer's creativeDebugLineBox will draw
    // (it silently skips any segment moving along more than one world axis). The
    // gizmo wireframe lines are appended to the yellow selection-box lines below.
    const Vec3 gizmoCenter{(selBoxMin.x + selBoxMax.x) * 0.5F,
                           (selBoxMin.y + selBoxMax.y) * 0.5F,
                           (selBoxMin.z + selBoxMax.z) * 0.5F};
    std::array<GizmoAxisShaft, 3> gizmoShafts{};
    gizmoShafts[0] = {GizmoAxis::X,
                      {gizmoCenter.x + kGizmoAxisLength, gizmoCenter.y,
                       gizmoCenter.z},
                      {1.0F, 0.0F, 0.0F, 1.0F}};
    gizmoShafts[1] = {GizmoAxis::Y,
                      {gizmoCenter.x, gizmoCenter.y + kGizmoAxisLength,
                       gizmoCenter.z},
                      {0.0F, 1.0F, 0.0F, 1.0F}};
    gizmoShafts[2] = {GizmoAxis::Z,
                      {gizmoCenter.x, gizmoCenter.y,
                       gizmoCenter.z + kGizmoAxisLength},
                      {0.0F, 0.0F, 1.0F, 1.0F}};

    // ---- HANDLE HIT-TEST (SLICE 4) -----------------------------------------
    // Project C and each axis tip to pixels; a click's nearest shaft within the
    // pixel threshold names the grabbed axis. Reused by both --capture (to grab
    // the X handle) and the interactive left-press-near-a-handle path below.
    const ScreenPoint gizmoCenterScreen = projectPointToScreen(
        frame.camera.clipFromWorld, gizmoCenter, extent.width, extent.height);
    std::array<ScreenPoint, 3> gizmoTipScreen{};
    for (std::size_t i = 0; i < 3; ++i) {
      gizmoTipScreen[i] = projectPointToScreen(frame.camera.clipFromWorld,
                                               gizmoShafts[i].tip, extent.width,
                                               extent.height);
    }
    // Given a pixel (px,py), return the nearest grabbed axis (or None). Both the
    // center and the candidate tip must project in front of the camera.
    const auto pickGizmoAxis = [&](float px, float py) -> GizmoAxis {
      if (!gizmoCenterScreen.valid) {
        return GizmoAxis::None;
      }
      GizmoAxis best = GizmoAxis::None;
      float bestDist = kGizmoHandleThresholdPx;
      for (std::size_t i = 0; i < 3; ++i) {
        if (!gizmoTipScreen[i].valid) {
          continue;
        }
        const float d = pointToSegmentDistancePx(
            px, py, gizmoCenterScreen.x, gizmoCenterScreen.y,
            gizmoTipScreen[i].x, gizmoTipScreen[i].y);
        if (d < bestDist) {
          bestDist = d;
          best = gizmoShafts[i].axis;
        }
      }
      return best;
    };

    struct PathPointHandleHit {
      creative::CreativeObjectId objectId = creative::kInvalidObjectId;
      std::size_t pointIndex = 0U;
      ScreenAabb aabb{};
      float centerDepth = std::numeric_limits<float>::max();
      creative::CreativeVec3 position{};
    };
    std::vector<PathPointHandleHit> pathPointHandleHits;
    const creative::CreativeObjectId selectedPathHandleObjectId =
        static_cast<creative::CreativeObjectId>(selectedId);
    const bool selectedIsPathForHandles =
        hasSelection &&
        creative::describeObject(selected->kind).shapeKind ==
            creative::CreativeObjectShapeKind::Path &&
        validPathPoints(selected->pathPoints);
    if (selectedIsPathForHandles) {
      pathPointHandleHits.reserve(selected->pathPoints.size());
      for (std::size_t index = 0; index < selected->pathPoints.size(); ++index) {
        const creative::CreativeVec3 position =
            selected->pathPoints[index].position;
        const VisualBounds handleBounds = pathPointHandleBounds(position);
        PathPointHandleHit handle;
        handle.objectId = selectedPathHandleObjectId;
        handle.pointIndex = index;
        handle.position = position;
        handle.aabb = projectBoxToScreen(frame.camera.clipFromWorld,
                                         handleBounds.min,
                                         handleBounds.max,
                                         extent.width,
                                         extent.height);
        handle.centerDepth =
            clipW(frame.camera.clipFromWorld, visualBoundsCenter(handleBounds));
        pathPointHandleHits.push_back(handle);
      }
    }

    const auto pickPathPointHandle =
        [&](float px, float py, PathPointHandleHit& out) -> bool {
      bool found = false;
      float bestDepth = std::numeric_limits<float>::max();
      for (const PathPointHandleHit& handle : pathPointHandleHits) {
        if (!handle.aabb.valid || px < handle.aabb.minX ||
            px > handle.aabb.maxX || py < handle.aabb.minY ||
            py > handle.aabb.maxY) {
          continue;
        }
        if (!found || handle.centerDepth < bestDepth) {
          found = true;
          bestDepth = handle.centerDepth;
          out = handle;
        }
      }
      return found;
    };

    if (!capturePath.empty() && !capturePathPointHandleLogged &&
        selectedIsPathForHandles &&
        selectedPathHandleObjectId == capturePathTargetId) {
      for (const PathPointHandleHit& handle : pathPointHandleHits) {
        SDL_Log("iggy3d_creative: PATH_HANDLE hit proxy objectId=%llu "
                "pointIndex=%zu aabbValid=%d position=(%.3f, %.3f, %.3f) "
                "screen=[%.1f, %.1f..%.1f, %.1f]",
                static_cast<unsigned long long>(handle.objectId),
                handle.pointIndex, handle.aabb.valid ? 1 : 0,
                handle.position.x, handle.position.y, handle.position.z,
                handle.aabb.minX, handle.aabb.minY, handle.aabb.maxX,
                handle.aabb.maxY);
      }
      capturePathPointHandleLogged = true;
    }
    // Start anchor S for an axis-constrained move = the object's corner anchor,
    // exactly as the facade captures it on BeginMove (objectCornerAnchor): for a
    // Crate (hasTransform=true) that is transform.position, NOT bounds.min. Using
    // the SAME anchor the facade holds keeps the pinned axes grid-aligned so they
    // snap to themselves; reading bounds.min instead would desync the pinned axes
    // and let the snap drag a "held" axis off S. Constrained-move worldDestination
    // is built FROM S: grabbed axis carries the dragged value, other two pinned.
    Vec3 gizmoAnchorS{gizmoCenter.x, gizmoCenter.y, gizmoCenter.z};
    if (hasSelection) {
      gizmoAnchorS = toVec3(selected->transform.position);
    }

    // ---- MOVE (SLICE 3) ----------------------------------------------------
    // Everything below drives the kernel's GENERIC Move: setActiveTool(Move) +
    // the PRESS/MOVE/RELEASE pointer lifecycle through dispatchToolInput. The
    // facade picks the object, snaps the world destination to the grid, and
    // commits ONE Move mutation. NO per-object position math lives here, and the
    // target is ALWAYS the currently selected id — for slice 5's capture that is
    // the FLOOR, which rides the identical path the crate did in slice 4.
    const creative::CreativeObjectId selectedObjectId =
        static_cast<creative::CreativeObjectId>(selectedId);
    if (!capturePath.empty() && !placeMode) {
      // --capture (SLICE 5): after the FLOOR is selected (frame 3), grab the X
      // gizmo handle and run an AXIS-CONSTRAINED Move along +X by 2 m.
      //   frame 5: hit-test the X shaft (grab X) + switch to Move + PRESS
      //   frame 6: PointerMove carrying worldDestination = {S.x+2, S.y, S.z},
      //            moveHeldAxis=Y (PreviewMove) — X follows, Y held, Z pinned
      //   frame 7: RELEASE with the same worldDestination (CommitMove) -> snap
      // The single-axis motion is entirely a product of the destination + held
      // axis; there is NO per-object move math. worldDestination pins Y,Z to the
      // start anchor S so only X (= S.x + 2) can change after the facade snaps.
      const creative::CreativeToolWorldPoint xAxisDestination{
          static_cast<double>(gizmoAnchorS.x) + 2.0,
          static_cast<double>(gizmoAnchorS.y),
          static_cast<double>(gizmoAnchorS.z)};
      if (frameIndex == 5U && hasSelection) {
        // Synthesize a grab of the X handle: click the projected midpoint of the
        // X shaft [screen(C), screen(Xtip)] and confirm the hit-test picks X.
        GizmoAxis grabbed = GizmoAxis::None;
        if (gizmoCenterScreen.valid && gizmoTipScreen[0].valid) {
          const float hx = (gizmoCenterScreen.x + gizmoTipScreen[0].x) * 0.5F;
          const float hy = (gizmoCenterScreen.y + gizmoTipScreen[0].y) * 0.5F;
          grabbed = pickGizmoAxis(hx, hy);
        }
        if (!loggedGizmoGrab) {
          SDL_Log("iggy3d_creative: GIZMO grabbed axis=%s (expected X) on "
                  "selected id=%u kind='%s'",
                  gizmoAxisName(grabbed), selectedId,
                  std::string(creative::toString(selected->kind)).c_str());
          loggedGizmoGrab = true;
        }
        const bool ok = appState.facade.setActiveTool(creative::Tool::Move);
        SDL_Log("iggy3d_creative: setActiveTool(Move) accepted=%d", ok ? 1 : 0);
        if (!loggedMoveBefore) {
          logObjectPlacement("BEFORE",
                             appState.facade.findObject(selectedObjectId));
          loggedMoveBefore = true;
        }
        creative::CreativeToolInputPacket press;
        press.kind = creative::CreativeToolInputKind::PointerPress;
        press.pointer.button = creative::CreativeToolPointerButton::Primary;
        press.pointer.target =
            creative::TargetRef{static_cast<creative::Id>(selectedObjectId)};
        const creative::CreativeFacadeToolDispatchReceipt r =
            appState.facade.dispatchToolInput(press);
        logMoveDispatch("PRESS", r);
      } else if (frameIndex == 6U) {
        creative::CreativeToolInputPacket move;
        move.kind = creative::CreativeToolInputKind::PointerMove;
        move.pointer.button = creative::CreativeToolPointerButton::Primary;
        move.pointer.hasWorldDestination = true;
        move.pointer.worldDestination = xAxisDestination;
        move.pointer.moveHeldAxis = heldAxisForGrabbedAxis(GizmoAxis::X);
        const creative::CreativeFacadeToolDispatchReceipt r =
            appState.facade.dispatchToolInput(move);
        logMoveDispatch("MOVE", r);
      } else if (frameIndex == 7U) {
        creative::CreativeToolInputPacket release;
        release.kind = creative::CreativeToolInputKind::PointerRelease;
        release.pointer.button = creative::CreativeToolPointerButton::Primary;
        release.pointer.hasWorldDestination = true;
        release.pointer.worldDestination = xAxisDestination;
        release.pointer.moveHeldAxis = heldAxisForGrabbedAxis(GizmoAxis::X);
        const creative::CreativeFacadeToolDispatchReceipt r =
            dispatchMoveReleaseWithUndo(appState, undoStack, release,
                                        selectedObjectId,
                                        "capture_move_release");
        logMoveDispatch("RELEASE", r);
        if (!loggedMoveAfter) {
          logObjectPlacement("AFTER",
                             appState.facade.findObject(selectedObjectId));
          loggedMoveAfter = true;
        }
      }
    } else if (!placeMode &&
               appState.facade.toolState().activeTool == creative::Tool::Move &&
               hasSelection) {
      // Interactive Move (SLICE 5): while the Move tool is active and ANY object
      // is selected, hold Left-Alt (releases fly-look) and left-press. A press
      // NEAR a gizmo handle grabs that axis and maps cursor motion ALONG THAT
      // AXIS ONLY; a press away from every handle falls back to the ground-plane
      // move (camera-forward ray -> Y=0 plane). Either way the facade still owns
      // the pick + snap + commit — no per-object move math here.
      const bool* mvKeys = SDL_GetKeyboardState(nullptr);
      const bool altHeld = mvKeys != nullptr && (mvKeys[SDL_SCANCODE_LALT] != 0);
      if (altHeld) {
        float mx = 0.0F;
        float my = 0.0F;
        const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
        const bool lDown = (buttons & SDL_BUTTON_LMASK) != 0U;
        // High-DPI: scale logical cursor coords to drawable pixels (as select).
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
        const float cursorPx = mx * scaleX;
        const float cursorPy = my * scaleY;

        // Camera-forward ray -> Y=0 plane -> world XZ ground point (fallback).
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

        const bool selectedIsPath =
            selected != nullptr &&
            creative::describeObject(selected->kind).shapeKind ==
                creative::CreativeObjectShapeKind::Path;
        if (selectedIsPath) {
          if (lDown && !interactivePathMoveActive &&
              !interactivePathPointMoveActive) {
            PathPointHandleHit handle;
            if (pickPathPointHandle(cursorPx, cursorPy, handle)) {
              interactivePathPointMoveActive = true;
              interactivePathPointMoveObjectId = handle.objectId;
              interactivePathPointMoveIndex = handle.pointIndex;
              interactivePathPointMoveStartGround = ground;
              SDL_Log("iggy3d_creative: PATH_HANDLE interactive move begin "
                      "objectId=%llu pointIndex=%zu ground=(%.3f, %.3f, %.3f) "
                      "position=(%.3f, %.3f, %.3f) pathPoints='%s'",
                      static_cast<unsigned long long>(handle.objectId),
                      handle.pointIndex, ground.x, ground.y, ground.z,
                      handle.position.x, handle.position.y, handle.position.z,
                      pathPointsSummary(selected->pathPoints).c_str());
            } else {
              interactivePathMoveActive = true;
              interactivePathMoveObjectId = selectedObjectId;
              interactivePathMoveStartGround = ground;
              SDL_Log("iggy3d_creative: PATH interactive move begin objectId=%llu "
                      "ground=(%.3f, %.3f, %.3f) pathPoints='%s'",
                      static_cast<unsigned long long>(selectedObjectId),
                      ground.x, ground.y, ground.z,
                      pathPointsSummary(selected->pathPoints).c_str());
            }
          } else if (!lDown && interactivePathPointMoveActive) {
            const creative::CreativeVec3 delta{
                ground.x - interactivePathPointMoveStartGround.x,
                0.0,
                ground.z - interactivePathPointMoveStartGround.z};
            const creative::CreativeDocumentMutationReceipt receipt =
                movePathPointWithUndo(appState,
                                      undoStack,
                                      interactivePathPointMoveObjectId,
                                      interactivePathPointMoveIndex,
                                      delta,
                                      "path_point_move_interactive_release");
            SDL_Log("iggy3d_creative: PATH_HANDLE interactive move release "
                    "objectId=%llu pointIndex=%zu status='%s' changed=%d "
                    "delta=(%.3f, %.3f, %.3f)",
                    static_cast<unsigned long long>(
                        interactivePathPointMoveObjectId),
                    interactivePathPointMoveIndex,
                    std::string(creative::toString(receipt.status)).c_str(),
                    receipt.changed ? 1 : 0, delta.x, delta.y, delta.z);
            interactivePathPointMoveActive = false;
            interactivePathPointMoveObjectId = creative::kInvalidObjectId;
            interactivePathPointMoveIndex = 0U;
          } else if (!lDown && interactivePathMoveActive) {
            const creative::CreativeVec3 delta{
                ground.x - interactivePathMoveStartGround.x,
                0.0,
                ground.z - interactivePathMoveStartGround.z};
            const creative::CreativeDocumentMutationReceipt receipt =
                movePathObjectWithUndo(appState,
                                       undoStack,
                                       interactivePathMoveObjectId,
                                       delta,
                                       "path_move_interactive_release");
            SDL_Log("iggy3d_creative: PATH interactive move release objectId=%llu "
                    "status='%s' changed=%d delta=(%.3f, %.3f, %.3f)",
                    static_cast<unsigned long long>(
                        interactivePathMoveObjectId),
                    std::string(creative::toString(receipt.status)).c_str(),
                    receipt.changed ? 1 : 0, delta.x, delta.y, delta.z);
            interactivePathMoveActive = false;
            interactivePathMoveObjectId = creative::kInvalidObjectId;
          }
        } else {
        // Build the pointer packet's worldDestination + moveHeldAxis. When an
        // axis handle is grabbed, project the cursor delta since grab onto the
        // shaft's screen direction, scale it to world length along the axis, and
        // set worldDestination = S with only the grabbed axis advanced (the other
        // two pinned to S), moveHeldAxis = one of the two non-grabbed axes.
        const auto buildConstrainedDestination =
            [&](creative::CreativeToolWorldPoint& dest,
                creative::CreativeToolMoveHeldAxis& held) {
              if (interactiveGrabbedAxis == GizmoAxis::None) {
                dest = ground;  // Free ground-plane move.
                held = creative::CreativeToolMoveHeldAxis::Y;
                return;
              }
              // Screen-space shaft direction at grab time (center -> tip).
              float sdx = interactiveGrabTipScreen.x - interactiveGrabCenterScreen.x;
              float sdy = interactiveGrabTipScreen.y - interactiveGrabCenterScreen.y;
              const float slen = std::sqrt(sdx * sdx + sdy * sdy);
              float along = 0.0F;
              if (slen > 1.0e-3F) {
                sdx /= slen;
                sdy /= slen;
                const float cdx = cursorPx - interactiveGrabCursorX;
                const float cdy = cursorPy - interactiveGrabCursorY;
                // pixels moved along the shaft / pixels per shaft * world length.
                const float alongPx = cdx * sdx + cdy * sdy;
                along = (alongPx / slen) * kGizmoAxisLength;
              }
              dest.x = static_cast<double>(interactiveGrabAnchorS.x);
              dest.y = static_cast<double>(interactiveGrabAnchorS.y);
              dest.z = static_cast<double>(interactiveGrabAnchorS.z);
              switch (interactiveGrabbedAxis) {
                case GizmoAxis::X:
                  dest.x += static_cast<double>(along);
                  break;
                case GizmoAxis::Y:
                  dest.y += static_cast<double>(along);
                  break;
                case GizmoAxis::Z:
                  dest.z += static_cast<double>(along);
                  break;
                case GizmoAxis::None:
                default:
                  break;
              }
              held = heldAxisForGrabbedAxis(interactiveGrabbedAxis);
            };

        if (lDown && !moveDragButtonDown) {
          moveDragButtonDown = true;
          // Grab an axis handle if the press landed near one; else free move.
          interactiveGrabbedAxis = pickGizmoAxis(cursorPx, cursorPy);
          interactiveGrabAnchorS = gizmoAnchorS;
          interactiveGrabCursorX = cursorPx;
          interactiveGrabCursorY = cursorPy;
          interactiveGrabCenterScreen = gizmoCenterScreen;
          if (interactiveGrabbedAxis == GizmoAxis::X) {
            interactiveGrabTipScreen = gizmoTipScreen[0];
          } else if (interactiveGrabbedAxis == GizmoAxis::Y) {
            interactiveGrabTipScreen = gizmoTipScreen[1];
          } else if (interactiveGrabbedAxis == GizmoAxis::Z) {
            interactiveGrabTipScreen = gizmoTipScreen[2];
          }
          SDL_Log("iggy3d_creative: GIZMO grabbed axis=%s",
                  gizmoAxisName(interactiveGrabbedAxis));
          creative::CreativeToolInputPacket press;
          press.kind = creative::CreativeToolInputKind::PointerPress;
          press.pointer.button = creative::CreativeToolPointerButton::Primary;
          press.pointer.target =
              creative::TargetRef{static_cast<creative::Id>(selectedObjectId)};
          (void)appState.facade.dispatchToolInput(press);
        } else if (lDown && moveDragButtonDown) {
          creative::CreativeToolInputPacket move;
          move.kind = creative::CreativeToolInputKind::PointerMove;
          move.pointer.button = creative::CreativeToolPointerButton::Primary;
          move.pointer.hasWorldDestination = true;
          buildConstrainedDestination(move.pointer.worldDestination,
                                      move.pointer.moveHeldAxis);
          (void)appState.facade.dispatchToolInput(move);
        } else if (!lDown && moveDragButtonDown) {
          moveDragButtonDown = false;
          creative::CreativeToolInputPacket release;
          release.kind = creative::CreativeToolInputKind::PointerRelease;
          release.pointer.button = creative::CreativeToolPointerButton::Primary;
          release.pointer.hasWorldDestination = true;
          buildConstrainedDestination(release.pointer.worldDestination,
                                      release.pointer.moveHeldAxis);
          const creative::CreativeFacadeToolDispatchReceipt r =
              dispatchMoveReleaseWithUndo(appState, undoStack, release,
                                          selectedObjectId,
                                          "move_interactive_release");
          logMoveDispatch("RELEASE", r);
          interactiveGrabbedAxis = GizmoAxis::None;
        }
        }
      } else if (moveDragButtonDown) {
        moveDragButtonDown = false;  // Alt released mid-drag: drop the latch.
        interactiveGrabbedAxis = GizmoAxis::None;
      } else if (interactivePathMoveActive) {
        SDL_Log("iggy3d_creative: PATH interactive move cancelled objectId=%llu",
                static_cast<unsigned long long>(interactivePathMoveObjectId));
        interactivePathMoveActive = false;
        interactivePathMoveObjectId = creative::kInvalidObjectId;
      } else if (interactivePathPointMoveActive) {
        SDL_Log("iggy3d_creative: PATH_HANDLE interactive move cancelled "
                "objectId=%llu pointIndex=%zu",
                static_cast<unsigned long long>(
                    interactivePathPointMoveObjectId),
                interactivePathPointMoveIndex);
        interactivePathPointMoveActive = false;
        interactivePathPointMoveObjectId = creative::kInvalidObjectId;
        interactivePathPointMoveIndex = 0U;
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
      // Recolor the SELECTED object's edges — matched by id, whatever the kind.
      const bool sel = hasSelection &&
                       line.objectId == static_cast<creative::CreativeObjectId>(
                                            selectedId);
      line.thickness = sel ? 0.06F : 0.03F;
      if (sel) {
        line.color = {1.0F, 1.0F, 0.0F, 1.0F};
      }
    }
    ProductCreativeWireframeDebugRenderFrame dbg =
        buildProductCreativeWireframeDebugRenderFrame(&lines.lineList);

    // ---- GIZMO WIREFRAME (SLICE 4) -----------------------------------------
    // Build ONE combined line vector: the document wireframe lines that draw the
    // yellow selection box (dbg.lines, already converted to render lines) PLUS
    // the 3 axis-aligned gizmo shafts. Point frame.creativeWireframeDebug at THIS
    // vector so the renderer draws both. The vector must outlive submitFrame(),
    // so it lives here in the frame-loop body. When nothing is selected we skip
    // the gizmo and the selection box is empty, so this is just dbg.lines.
    std::vector<RenderCreativeWireframeDebugLine> combinedWireLines;
    combinedWireLines.reserve(dbg.lines.size() + 48);
    std::size_t documentWireLineCount = 0;
    for (const RenderCreativeWireframeDebugLine& line : dbg.lines) {
      const creative::CreativeObject* object =
          line.objectId != creative::kInvalidObjectId
              ? appState.facade.findObject(line.objectId)
              : nullptr;
      if (object != nullptr &&
          creative::describeObject(object->kind).shapeKind ==
              creative::CreativeObjectShapeKind::Line) {
        continue;
      }
      combinedWireLines.push_back(line);
    }
    documentWireLineCount = combinedWireLines.size();
    std::size_t pointMarkerEdgeCount = 0;
    std::size_t lineMarkerEdgeCount = 0;
    std::size_t pathPointHandleEdgeCount = 0;
    for (const creative::CreativeObject& obj :
         appState.facade.document().objects()) {
      const creative::CreativeObjectDescriptor& descriptor =
          creative::describeObject(obj.kind);
      if (!obj.visible) {
        continue;
      }
      const bool sel =
          hasSelection &&
          obj.id == static_cast<creative::CreativeObjectId>(selectedId);
      if (descriptor.shapeKind != creative::CreativeObjectShapeKind::Point &&
          descriptor.shapeKind != creative::CreativeObjectShapeKind::Line) {
        continue;
      }
      const VisualBounds markerBounds = visualBoundsForObject(obj);
      const std::size_t before = combinedWireLines.size();
      appendWireframeBoxEdges(
          combinedWireLines, markerBounds.min, markerBounds.max,
          sel ? RenderLineColor{1.0F, 1.0F, 0.0F, 1.0F}
              : descriptor.shapeKind == creative::CreativeObjectShapeKind::Line
                    ? RenderLineColor{0.86F, 0.68F, 0.28F, 1.0F}
                    : RenderLineColor{0.34F, 0.62F, 0.88F, 1.0F},
          sel ? 0.06F : 0.035F);
      for (std::size_t i = before; i < combinedWireLines.size(); ++i) {
        combinedWireLines[i].objectId = obj.id;
      }
      if (descriptor.shapeKind == creative::CreativeObjectShapeKind::Line) {
        lineMarkerEdgeCount += combinedWireLines.size() - before;
      } else {
        pointMarkerEdgeCount += combinedWireLines.size() - before;
      }
    }
    if (selectedIsPathForHandles) {
      for (const creative::CreativePathPoint& point : selected->pathPoints) {
        const VisualBounds handleBounds = pathPointHandleBounds(point.position);
        const std::size_t before = combinedWireLines.size();
        appendWireframeBoxEdges(combinedWireLines,
                                handleBounds.min,
                                handleBounds.max,
                                RenderLineColor{0.20F, 0.88F, 1.0F, 1.0F},
                                0.035F);
        for (std::size_t i = before; i < combinedWireLines.size(); ++i) {
          combinedWireLines[i].objectId =
              static_cast<creative::CreativeObjectId>(selectedId);
        }
        pathPointHandleEdgeCount += combinedWireLines.size() - before;
      }
    }
    if (hasSelection) {
      for (const GizmoAxisShaft& shaft : gizmoShafts) {
        RenderCreativeWireframeDebugLine gizmoLine;
        gizmoLine.start = gizmoCenter;
        gizmoLine.end = shaft.tip;  // Axis-aligned: only one component differs.
        gizmoLine.color = shaft.color;
        gizmoLine.objectId =
            static_cast<creative::CreativeObjectId>(selectedId);
        gizmoLine.thickness = kGizmoThickness;
        combinedWireLines.push_back(gizmoLine);
      }
    }
    // ---- GHOST PREVIEW (SLICE 6) -------------------------------------------
    // In Place mode, draw a GREEN (0,1,0,1) axis-aligned wireframe box at the
    // aimed cell sized to the current brush footprint (min.y=0..height, XZ
    // centered on the cell) — the placement preview. It rides the SAME combined
    // wireframe vector as the selection box + gizmo, so it needs no new render
    // path. It is NOT a document object (objectId=0); it vanishes on the drop's
    // next frame if the aim moves.
    std::size_t ghostEdgeCount = 0;
    if (placeMode) {
      const creative::CreativeObjectDescriptor& brushDescriptor =
          creative::describeObject(placeBrush);
      Vec3 ghostMin{};
      Vec3 ghostMax{};
      if (brushDescriptor.shapeKind == creative::CreativeObjectShapeKind::Path) {
        const std::vector<creative::CreativePathPoint> ghostPath =
            initialPathPointsForAnchor(aimCellCenter);
        const std::size_t before = combinedWireLines.size();
        appendPathPolylineLines(combinedWireLines,
                                ghostPath,
                                RenderLineColor{0.0F, 1.0F, 0.0F, 1.0F},
                                kGizmoThickness);
        ghostEdgeCount = combinedWireLines.size() - before;
      } else if (brushDescriptor.shapeKind ==
                 creative::CreativeObjectShapeKind::Point) {
        const VisualBounds markerBounds = pointMarkerBounds(
            creative::CreativeVec3{aimCellCenter.x, 0.0, aimCellCenter.z});
        ghostMin = markerBounds.min;
        ghostMax = markerBounds.max;
      } else {
        const BrushFootprint fp = brushFootprintForDescriptor(brushDescriptor);
        const VisualBounds authoredGhost{
            {aimCellCenter.x - fp.sizeX * 0.5F, 0.0F,
             aimCellCenter.z - fp.sizeZ * 0.5F},
            {aimCellCenter.x + fp.sizeX * 0.5F, fp.height,
             aimCellCenter.z + fp.sizeZ * 0.5F}};
        const VisualBounds ghostBounds =
            brushDescriptor.shapeKind == creative::CreativeObjectShapeKind::Line
                ? lineProxyBounds(authoredGhost)
                : authoredGhost;
        ghostMin = ghostBounds.min;
        ghostMax = ghostBounds.max;
      }
      if (brushDescriptor.shapeKind != creative::CreativeObjectShapeKind::Path) {
        const std::size_t before = combinedWireLines.size();
        appendWireframeBoxEdges(combinedWireLines, ghostMin, ghostMax,
                                RenderLineColor{0.0F, 1.0F, 0.0F, 1.0F},
                                kGizmoThickness);
        ghostEdgeCount = combinedWireLines.size() - before;
      }
    }
    (void)ghostEdgeCount;
    RenderCreativeWireframeDebugFrame combinedWireFrame;
    combinedWireFrame.available = true;
    combinedWireFrame.visible = !combinedWireLines.empty();
    combinedWireFrame.lines = combinedWireLines.data();
    combinedWireFrame.lineCount = combinedWireLines.size();

    // ---- DIMENSION LABEL + glyph merge -------------------------------------
    // Merge the inspector-panel glyphs with the dimension-label glyphs into ONE
    // vector so a single .data() pointer stays valid for the whole frame.
    std::vector<DebugHudGlyphQuad> glyphs = menuFrame.textGlyphQuads;
    if (hasSelection) {
      const Vec3 center{(selBoxMin.x + selBoxMax.x) * 0.5F,
                        (selBoxMin.y + selBoxMax.y) * 0.5F,
                        (selBoxMin.z + selBoxMax.z) * 0.5F};
      const float w = clipW(frame.camera.clipFromWorld, center);
      if (std::isfinite(w) && w > 0.0F) {
        const Vec3 ndc = transformPoint(frame.camera.clipFromWorld, center);
        const float px = (ndc.x * 0.5F + 0.5F) * static_cast<float>(extent.width);
        const float py = (1.0F - (ndc.y * 0.5F + 0.5F)) *
                         static_cast<float>(extent.height);
        const float dimW = selBoxMax.x - selBoxMin.x;
        const float dimH = selBoxMax.y - selBoxMin.y;
        const float dimD = selBoxMax.z - selBoxMin.z;
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
    // SLICE 4: the combined vector (selection box + gizmo shafts), NOT dbg.frame.
    frame.creativeWireframeDebug = combinedWireFrame;

    const RenderSubmitResult submit = backend->submitFrame(frame);
    if (!loggedSelection) {
      loggedSelection = true;
      SDL_Log("iggy3d_creative: frame %llu submit outcome=%d reason='%s' "
              "meshes=%zu selectedTarget=%u hasSelection=%d selBoxLines=%zu "
              "pointMarkerLines=%zu lineMarkerLines=%zu pathHandleLines=%zu "
              "gizmoLines=%zu combinedWireLines=%zu uiRects=%zu glyphs=%zu",
              static_cast<unsigned long long>(frameIndex),
              static_cast<int>(submit.outcome),
              std::string(submit.reason.code).c_str(),
              scene.room.meshes.size(), selectedId, hasSelection ? 1 : 0,
              documentWireLineCount, pointMarkerEdgeCount, lineMarkerEdgeCount,
              pathPointHandleEdgeCount,
              combinedWireLines.size() - documentWireLineCount -
                  pointMarkerEdgeCount - lineMarkerEdgeCount -
                  pathPointHandleEdgeCount,
              combinedWireLines.size(), menuFrame.rects.size(), glyphs.size());
    }

    if (maxFrames != 0U && frameIndex >= maxFrames) {
      SDL_Log("iggy3d_creative: ROOM_BAKE final status='%s' reasonCode='%s' "
              "accepted=%d objectCount=%llu considered=%llu staticMeshes=%llu "
              "spatialSurfaces=%llu skippedHidden=%llu skippedEditorOnly=%llu "
              "skippedNoBounds=%llu skippedUnsupported=%llu "
              "skippedRoomMetadata=%llu standalonePreviewMeshes=%zu "
              "sceneMeshes=%zu",
              std::string(creative::toString(roomBake.receipt.status)).c_str(),
              roomBake.receipt.reasonCode.c_str(),
              roomBake.receipt.accepted ? 1 : 0,
              static_cast<unsigned long long>(roomBake.receipt.objectCount),
              static_cast<unsigned long long>(
                  roomBake.receipt.consideredObjectCount),
              static_cast<unsigned long long>(
                  roomBake.receipt.bakedStaticMeshCount),
              static_cast<unsigned long long>(
                  roomBake.receipt.bakedSpatialSurfaceCount),
              static_cast<unsigned long long>(
                  roomBake.receipt.skippedHiddenCount),
              static_cast<unsigned long long>(
                  roomBake.receipt.skippedEditorOnlyCount),
              static_cast<unsigned long long>(
                  roomBake.receipt.skippedNoBoundsCount),
              static_cast<unsigned long long>(
                  roomBake.receipt.skippedUnsupportedShapeCount),
              static_cast<unsigned long long>(
                  roomBake.receipt.skippedRoomMetadataCount),
              standalonePreviewMeshCount,
              scene.room.meshes.size());
      // Name the SELECTED object + kind so the capture is self-documenting: for
      // slice 5 this is expected to be the FLOOR.
      const char* selKind =
          hasSelection
              ? creative::toString(selected->kind).data()
              : "<none>";
      SDL_Log("iggy3d_creative: FINAL frame %llu submit outcome=%d reason='%s' "
              "selectedTarget=%u selectedKind='%s' hasSelection=%d selBoxLines=%zu "
              "pointMarkerLines=%zu lineMarkerLines=%zu pathHandleLines=%zu "
              "gizmoLines=%zu combinedWireLines=%zu placeMode=%d brush='%s' "
              "ghostEdges=%zu placed=%llu objectCount=%llu",
              static_cast<unsigned long long>(frameIndex),
              static_cast<int>(submit.outcome),
              std::string(submit.reason.code).c_str(), selectedId, selKind,
              hasSelection ? 1 : 0, documentWireLineCount,
              pointMarkerEdgeCount, lineMarkerEdgeCount,
              pathPointHandleEdgeCount,
              combinedWireLines.size() - documentWireLineCount -
                  pointMarkerEdgeCount - lineMarkerEdgeCount -
                  pathPointHandleEdgeCount,
              combinedWireLines.size(), placeMode ? 1 : 0,
              std::string(creative::toString(placeBrush)).c_str(),
              ghostEdgeCount, static_cast<unsigned long long>(placedCount),
              static_cast<unsigned long long>(
                  appState.facade.document().objectCount()));
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
