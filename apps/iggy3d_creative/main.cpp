// iggy3d_creative — SLICE 5 ("Any Object Inherits the Tooling")
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
//     per visible object, mapping kind -> render role (Floor->"floor" gray,
//     Crate/default->"prop" brown) so distinct kinds read as distinct colors.
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

// Map an object KIND to a renderer room role. The renderer's colorForRoomRole
// gives each role a distinct color: "floor" -> dark gray-blue, "prop" -> brown.
// This is the ONLY place the code inspects a kind, and it drives colour ONLY —
// none of the Select/Inspect/Move/Gizmo tooling ever branches on kind. A Floor
// therefore looks different from a Crate but behaves identically under the tools.
const char* renderRoleForKind(creative::CreativeObjectKind kind) {
  switch (kind) {
    case creative::CreativeObjectKind::Floor:
      return "floor";
    case creative::CreativeObjectKind::Crate:
    default:
      return "prop";
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
  // --capture: log the grabbed axis exactly once.
  bool loggedGizmoGrab = false;

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
    // frame from the cached snapshot, then append EVERY visible document object
    // as a shaded box — generically, one SceneRoomMeshItem per object. There is
    // NO per-kind mesh code: the object's kind only picks a render role (color)
    // via renderRoleForKind; geometry comes straight from its bounds. A new kind
    // renders for free the moment it lands in the document.
    SceneProjectionResult scene{};
    appendGridDotsToScene(gridSnapshot, scene);

    bool anyPropVisible = false;
    for (const creative::CreativeObject& obj : appState.facade.document().objects()) {
      if (!obj.visible) {
        continue;  // Skip hidden objects (still authored, just not drawn).
      }
      const Vec3 boxMin = toVec3(obj.bounds.min);
      const Vec3 boxMax = toVec3(obj.bounds.max);
      SceneRoomMeshItem mesh;
      mesh.id = "creative.object_" + std::to_string(obj.id);
      mesh.role = renderRoleForKind(obj.kind);  // Color by kind — the only
                                                // place kind is inspected.
      mesh.materialId = "creative_object";
      mesh.position = {(boxMin.x + boxMax.x) * 0.5F,
                       (boxMin.y + boxMax.y) * 0.5F,
                       (boxMin.z + boxMax.z) * 0.5F};
      mesh.size = {boxMax.x - boxMin.x, boxMax.y - boxMin.y,
                   boxMax.z - boxMin.z};
      scene.room.meshes.push_back(std::move(mesh));
      anyPropVisible = true;
    }
    scene.room.staticMeshCount = scene.room.meshes.size();
    scene.room.propVisible = anyPropVisible;
    DebugProjectionResult debug{};

    // FRAME (non-const so we can attach UI + wireframe + label below). This
    // gives frame.camera.clipFromWorld (world -> NDC) for click + label maths.
    FrameInput frame = makeProductVulkanFrame(
        scene, debug, frameIndex++, extent.width, extent.height, yawDegrees,
        pitchDegrees, /*cameraAnchorOverrideAvailable=*/true, flyPos);

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
      const Vec3 boxMin = toVec3(obj.bounds.min);
      const Vec3 boxMax = toVec3(obj.bounds.max);
      ObjectScreenHit hit;
      hit.id = obj.id;
      hit.aabb = projectBoxToScreen(frame.camera.clipFromWorld, boxMin, boxMax,
                                    extent.width, extent.height);
      const Vec3 center{(boxMin.x + boxMax.x) * 0.5F,
                        (boxMin.y + boxMax.y) * 0.5F,
                        (boxMin.z + boxMax.z) * 0.5F};
      hit.centerDepth = clipW(frame.camera.clipFromWorld, center);
      objectHits.push_back(hit);
      if (obj.id == floorObjectId) {
        haveFloorBounds = true;
        floorBoxMin = boxMin;
        floorBoxMax = boxMax;
      }
    }

    bool clickRequested = false;
    float clickX = 0.0F;
    float clickY = 0.0F;
    if (!capturePath.empty()) {
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
      selBoxMin = toVec3(selected->bounds.min);
      selBoxMax = toVec3(selected->bounds.max);
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
    if (!capturePath.empty()) {
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
            appState.facade.dispatchToolInput(release);
        logMoveDispatch("RELEASE", r);
        if (!loggedMoveAfter) {
          logObjectPlacement("AFTER",
                             appState.facade.findObject(selectedObjectId));
          loggedMoveAfter = true;
        }
      }
    } else if (appState.facade.toolState().activeTool == creative::Tool::Move &&
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
              appState.facade.dispatchToolInput(release);
          logMoveDispatch("RELEASE", r);
          interactiveGrabbedAxis = GizmoAxis::None;
        }
      } else if (moveDragButtonDown) {
        moveDragButtonDown = false;  // Alt released mid-drag: drop the latch.
        interactiveGrabbedAxis = GizmoAxis::None;
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
    std::vector<RenderCreativeWireframeDebugLine> combinedWireLines = dbg.lines;
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
              "gizmoLines=%zu combinedWireLines=%zu uiRects=%zu glyphs=%zu",
              static_cast<unsigned long long>(frameIndex),
              static_cast<int>(submit.outcome),
              std::string(submit.reason.code).c_str(),
              scene.room.meshes.size(), selectedId, hasSelection ? 1 : 0,
              dbg.lines.size(), combinedWireLines.size() - dbg.lines.size(),
              combinedWireLines.size(), menuFrame.rects.size(),
              glyphs.size());
    }

    if (maxFrames != 0U && frameIndex >= maxFrames) {
      // Name the SELECTED object + kind so the capture is self-documenting: for
      // slice 5 this is expected to be the FLOOR.
      const char* selKind =
          hasSelection
              ? creative::toString(selected->kind).data()
              : "<none>";
      SDL_Log("iggy3d_creative: FINAL frame %llu submit outcome=%d reason='%s' "
              "selectedTarget=%u selectedKind='%s' hasSelection=%d selBoxLines=%zu "
              "gizmoLines=%zu combinedWireLines=%zu",
              static_cast<unsigned long long>(frameIndex),
              static_cast<int>(submit.outcome),
              std::string(submit.reason.code).c_str(), selectedId, selKind,
              hasSelection ? 1 : 0, dbg.lines.size(),
              combinedWireLines.size() - dbg.lines.size(),
              combinedWireLines.size());
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
