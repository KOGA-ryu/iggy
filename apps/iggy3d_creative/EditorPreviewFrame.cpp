#include "EditorPreviewFrame.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <utility>

#include <SDL3/SDL.h>

#include "EditorActionHints.hpp"
#include "EditorConnectedFill.hpp"
#include "EditorFrame.hpp"
#include "EditorCatalog.hpp"
#include "EditorControls.hpp"
#include "EditorToolOptions.hpp"
#include "EditorTransform.hpp"
#include "EditorGizmo.hpp"
#include "EditorInteraction.hpp"
#include "EditorPlacement.hpp"
#include "EditorPattern.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorShapePreview.hpp"
#include "EditorState.hpp"
#include "EditorSurfaceExtrude.hpp"
#include "EditorTerrain.hpp"
#include "EditorVolume.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/DocumentWireframe.hpp"
#include "app/iggy3d/creative/render/CreativeOverlayFrame.hpp"
#include "app/iggy3d/creative/render/CreativeScreenProjection.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"
#include "core/math/EulerRotation.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneItem.hpp"
#include "render/debug/DebugHudText.hpp"
#include "runtime/session/SessionState.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace creative = iggy3d::creative;
using namespace iggy3d;
namespace {

iggy3d::Vec3 gridDotSizeFor(const iggy3d::ProductMapMakerGridDot& dot,
                            float pitchMeters) {
  const float minorSize = std::clamp(pitchMeters * 0.08F, 0.04F, 0.10F);
  const float majorSize = std::clamp(pitchMeters * 0.14F, 0.07F, 0.16F);
  const float size = dot.major ? majorSize : minorSize;
  return {size, size, size};
}

void appendGridDotsToScene(const iggy3d::ProductMapMakerGridSnapshot& grid,
                           iggy3d::SceneProjectionResult& scene) {
  if (!grid.visible || grid.dots.empty()) {
    return;
  }
  scene.room.meshes.reserve(scene.room.meshes.size() + grid.dots.size());
  std::uint64_t index = 0;
  for (const iggy3d::ProductMapMakerGridDot& dot : grid.dots) {
    // Keep only the ground layer: a small Y-extent still emits a few Y layers
    // (the snap rounds the half-extent out to y=-1,0,1), so filter to planeY.
    if (std::fabs(dot.worldPosition.y - grid.planeY) > grid.pitchMeters * 0.5F) {
      continue;
    }
    iggy3d::SceneRoomMeshItem mesh;
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

[[nodiscard]] RenderLineColor volumeOperationColor(
    cr::CreativeVolumeOperationKind operation) noexcept {
  constexpr std::array<RenderLineColor,
                       static_cast<std::size_t>(
                           cr::CreativeVolumeOperationKind::Count)>
      colors{
          RenderLineColor{0.18F, 0.95F, 0.34F, 1.0F},
          RenderLineColor{0.10F, 0.90F, 0.95F, 1.0F},
          RenderLineColor{1.0F, 0.72F, 0.12F, 1.0F},
          RenderLineColor{1.0F, 0.20F, 0.18F, 1.0F},
          RenderLineColor{0.30F, 0.55F, 1.0F, 1.0F},
      };
  const std::size_t index = static_cast<std::size_t>(operation);
  return index < colors.size() ? colors[index] : colors.front();
}

[[nodiscard]] Mat4 modelMatrix(Vec3 position,
                               Vec3 rotationRadians,
                               Vec3 scale) {
  const Vec3 axisX = rotateEulerXyz({1.0F, 0.0F, 0.0F}, rotationRadians);
  const Vec3 axisY = rotateEulerXyz({0.0F, 1.0F, 0.0F}, rotationRadians);
  const Vec3 axisZ = rotateEulerXyz({0.0F, 0.0F, 1.0F}, rotationRadians);
  Mat4 matrix = identityMat4();
  matrix.m[0] = axisX.x * scale.x;
  matrix.m[4] = axisX.y * scale.x;
  matrix.m[8] = axisX.z * scale.x;
  matrix.m[1] = axisY.x * scale.y;
  matrix.m[5] = axisY.y * scale.y;
  matrix.m[9] = axisY.z * scale.y;
  matrix.m[2] = axisZ.x * scale.z;
  matrix.m[6] = axisZ.y * scale.z;
  matrix.m[10] = axisZ.z * scale.z;
  matrix.m[3] = position.x;
  matrix.m[7] = position.y;
  matrix.m[11] = position.z;
  return matrix;
}

[[nodiscard]] bool previewBoundsTransform(
    const creative::CreativeBounds& bounds,
    float inset,
    Vec3& center,
    Vec3& size) {
  const creative::CreativeBoundsMetrics metrics =
      creative::measureCreativeBounds(bounds);
  const creative::CreativeCoreVec3Conversion coreCenter =
      creative::creativeVec3ToCoreChecked(metrics.center);
  const creative::CreativeCoreVec3Conversion coreSize =
      creative::creativeVec3ToCoreChecked(metrics.size);
  if (!metrics.valid || !coreCenter.converted || !coreSize.converted ||
      !std::isfinite(inset)) {
    return false;
  }
  center = coreCenter.value;
  size = coreSize.value * inset;
  return isFinite(size) && size.x > 0.0F &&
         size.y > 0.0F && size.z > 0.0F;
}

void appendCreativePreview(RenderCreativePreviewFrame& previews,
                           RenderCreativePreviewRole role,
                           const Mat4& clipFromModel,
                           bool includePathWireframe = false) {
  if (previews.itemCount >= previews.items.size()) {
    return;
  }
  previews.items[previews.itemCount++] = {
      role, clipFromModel, includePathWireframe};
}

struct MaterialBrushPreviewPlan {
  bool visible = false;
  bool removing = false;
  bool admitted = false;
  bool hasGuideAnchor = false;
  bool showSymmetryPivot = false;
  cr::CreativeMaterialBrushGuide guide =
      cr::CreativeMaterialBrushGuide::Free;
  cr::CreativeGridCoord3 guideAnchor{};
  cr::CreativeGridCoord3 symmetryPivot{};
  cr::CreativeGridCoord3 center{};
  cr::CreativeMaterialBrushStampPlan stamp{};
  std::array<cr::CreativeGridCoord3,
             cr::kCreativeMaterialBrushSymmetryCapacity>
      plannedDirectCells{};
  std::array<cr::CreativeGridCoord3,
             cr::kCreativeMaterialBrushSymmetryCapacity>
      plannedMirroredCells{};
  std::array<cr::CreativeGridCoord3,
             cr::kCreativeMaterialBrushSymmetryCapacity>
      eligibleDirectCells{};
  std::array<cr::CreativeGridCoord3,
             cr::kCreativeMaterialBrushSymmetryCapacity>
      eligibleMirroredCells{};
  std::uint16_t plannedDirectCellCount = 0U;
  std::uint16_t plannedMirroredCellCount = 0U;
  std::uint16_t eligibleDirectCellCount = 0U;
  std::uint16_t eligibleMirroredCellCount = 0U;

  [[nodiscard]] std::span<const cr::CreativeGridCoord3> renderedDirectCells()
      const noexcept {
    return admitted
               ? std::span<const cr::CreativeGridCoord3>{
                     eligibleDirectCells.data(), eligibleDirectCellCount}
               : std::span<const cr::CreativeGridCoord3>{
                     plannedDirectCells.data(), plannedDirectCellCount};
  }

  [[nodiscard]] std::span<const cr::CreativeGridCoord3>
  renderedMirroredCells() const noexcept {
    return admitted
               ? std::span<const cr::CreativeGridCoord3>{
                     eligibleMirroredCells.data(), eligibleMirroredCellCount}
               : std::span<const cr::CreativeGridCoord3>{
                     plannedMirroredCells.data(), plannedMirroredCellCount};
  }
};

[[nodiscard]] bool materialBrushCellVisited(
    const CreativeMaterialStrokeState& stroke,
    cr::CreativeGridCoord3 cell) noexcept {
  for (std::size_t index = 0U; index < stroke.visitedCount; ++index) {
    const cr::CreativeGridCoord3 visited = stroke.visited[index].cell;
    if (visited.x == cell.x && visited.y == cell.y && visited.z == cell.z) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] MaterialBrushPreviewPlan materialBrushPreviewPlan(
    const CreativeEditorState& editor,
    const cr::CreativeDocument& document) noexcept {
  MaterialBrushPreviewPlan output;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (held.kind != cr::CreativeHeldItemKind::MaterialBrush) {
    return output;
  }
  const CreativeMaterialStrokeState& stroke =
      editor.interaction.materialStroke;
  const CreativeMaterialBrushGestureConfig config =
      stroke.hasBrushAnchor
          ? stroke.brushConfig
          : creativeMaterialBrushGestureConfig(editor.toolSettings);
  cr::CreativeGridCoord3 lockedPivot{};
  const bool hasLockedPivot = creativeMaterialBrushLockedPivot(
      editor.interaction.materialBrushPivot, document.id(), lockedPivot);
  if (config.symmetry != cr::CreativeMaterialBrushSymmetry::Off) {
    if (stroke.hasSymmetryPivot) {
      output.showSymmetryPivot = true;
      output.symmetryPivot = stroke.symmetryPivot;
    } else if (hasLockedPivot) {
      output.showSymmetryPivot = true;
      output.symmetryPivot = lockedPivot;
    }
  }
  if (!editor.interaction.target.grid.valid) {
    return output;
  }
  output.removing = editor.interaction.materialStroke.repeat.active &&
                    editor.interaction.materialStroke.repeat.kind ==
                        CreativeMaterialStrokeKind::Remove;
  if (output.removing && !editor.interaction.target.voxelHit) {
    return output;
  }
  const cr::CreativeGridCoord3 rawCenter =
      output.removing ? editor.interaction.target.voxelCell
                      : editor.interaction.target.grid.adjacentCell;
  const cr::CreativeGridCoord3 anchor =
      stroke.hasBrushAnchor ? stroke.brushAnchor : rawCenter;
  cr::CreativeGridCoord3 center{};
  if (!cr::guideCreativeMaterialBrushCenter(config.guide, anchor, rawCenter,
                                            center)) {
    return output;
  }
  output.hasGuideAnchor = stroke.hasBrushAnchor;
  if (!output.showSymmetryPivot) {
    output.symmetryPivot = anchor;
  }
  output.guide = config.guide;
  output.guideAnchor = anchor;
  output.center = center;
  output.stamp = cr::planCreativeMaterialBrushStamp(
      creativeMaterialBrushStampRequest(config, center));
  output.visible = output.stamp.accepted;
  if (!output.visible) {
    return output;
  }

  const cr::CreativeMaterialBrushSymmetryPlan symmetry =
      cr::planCreativeMaterialBrushSymmetry(
          {config.symmetry, output.symmetryPivot,
           output.stamp.generatedCells()});
  if (!symmetry.accepted) {
    for (cr::CreativeGridCoord3 cell : output.stamp.generatedCells()) {
      output.plannedDirectCells[output.plannedDirectCellCount++] = cell;
    }
    return output;
  }

  const cr::CreativeVoxelField& field = document.voxelField();
  for (std::size_t index = 0U; index < symmetry.cellCount; ++index) {
    const cr::CreativeGridCoord3 cell = symmetry.cells[index];
    const bool mirrored = symmetry.cellIsMirrored(index);
    if (mirrored) {
      output.plannedMirroredCells[output.plannedMirroredCellCount++] = cell;
    } else {
      output.plannedDirectCells[output.plannedDirectCellCount++] = cell;
    }
    const cr::CreativeObjectKind currentMaterial = field.materialAt(cell);
    const bool occupied = currentMaterial != cr::CreativeObjectKind::Unknown;
    const bool allowed =
        output.removing
            ? occupied
            : currentMaterial != held.objectKind &&
                  cr::creativeMaterialBrushPaintAllows(
                      config.mask, currentMaterial,
                      config.replaceSourceKind);
    if (!allowed || materialBrushCellVisited(
                        editor.interaction.materialStroke, cell)) {
      continue;
    }
    if (mirrored) {
      output.eligibleMirroredCells[output.eligibleMirroredCellCount++] = cell;
    } else {
      output.eligibleDirectCells[output.eligibleDirectCellCount++] = cell;
    }
  }
  const std::size_t remainingCapacity =
      editor.interaction.materialStroke.visitedCount <=
              editor.interaction.materialStroke.visited.size()
          ? editor.interaction.materialStroke.visited.size() -
                editor.interaction.materialStroke.visitedCount
          : 0U;
  const bool materialValid =
      output.removing || cr::creativeVolumeBrushSupported(held.objectKind);
  const std::size_t eligibleCellCount =
      output.eligibleDirectCellCount + output.eligibleMirroredCellCount;
  output.admitted = materialValid &&
                    !editor.interaction.materialStroke.capacityReached &&
                    eligibleCellCount > 0U &&
                    eligibleCellCount <= remainingCapacity;
  return output;
}

[[nodiscard]] bool voxelChunkCoordLess(
    cr::CreativeVoxelChunkCoord lhs,
    cr::CreativeVoxelChunkCoord rhs) noexcept {
  if (lhs.z != rhs.z) return lhs.z < rhs.z;
  if (lhs.y != rhs.y) return lhs.y < rhs.y;
  return lhs.x < rhs.x;
}

[[nodiscard]] std::vector<cr::CreativeVoxelCuboid>
refreshVoxelChunkMeshPlans(CreativeEditorSceneCache& cache,
                           const cr::CreativeDocument& document) {
  std::vector<CreativeEditorVoxelChunkMeshCache> next;
  next.reserve(static_cast<std::size_t>(document.voxelField().chunkCount()));
  for (const cr::CreativeVoxelChunk& chunk : document.voxelField().chunks()) {
    const auto found = std::lower_bound(
        cache.voxelChunkMeshes.begin(), cache.voxelChunkMeshes.end(),
        chunk.coord,
        [](const CreativeEditorVoxelChunkMeshCache& entry,
           cr::CreativeVoxelChunkCoord coord) {
          return voxelChunkCoordLess(entry.coord, coord);
        });
    if (found != cache.voxelChunkMeshes.end() &&
        found->coord == chunk.coord && found->revision == chunk.revision) {
      next.push_back(*found);
      continue;
    }

    CreativeEditorVoxelChunkMeshCache rebuilt;
    rebuilt.coord = chunk.coord;
    rebuilt.revision = chunk.revision;
    rebuilt.cuboids = cr::buildCreativeVoxelCuboids(chunk);
    next.push_back(std::move(rebuilt));
    ++cache.voxelChunkMeshBuildCount;
  }
  cache.voxelChunkMeshes = std::move(next);

  std::size_t cuboidCount = 0;
  for (const CreativeEditorVoxelChunkMeshCache& entry :
       cache.voxelChunkMeshes) {
    cuboidCount += entry.cuboids.size();
  }
  std::vector<cr::CreativeVoxelCuboid> cuboids;
  cuboids.reserve(cuboidCount);
  for (const CreativeEditorVoxelChunkMeshCache& entry :
       cache.voxelChunkMeshes) {
    cuboids.insert(cuboids.end(), entry.cuboids.begin(), entry.cuboids.end());
  }
  return cuboids;
}

[[nodiscard]] bool convertTerrainSurfacePatches(
    const cr::CreativeTerrainRenderPlan& plan,
    std::vector<iggy3d::SceneRoomSurfacePatchItem>& output) {
  output.clear();
  if (!plan.accepted) {
    return false;
  }
  output.reserve(plan.patches.size());
  for (const cr::CreativeTerrainSurfacePatch& patch : plan.patches) {
    iggy3d::SceneRoomSurfacePatchItem item;
    item.role = cr::creativeTerrainMaterialRenderRole(patch.material);
    const cr::CreativeCoreVec3Conversion center =
        cr::creativeVec3ToCoreChecked(patch.center);
    if (!center.converted) {
      output.clear();
      return false;
    }
    item.center = center.value;
    for (std::size_t index = 0U; index < patch.corners.size(); ++index) {
      const cr::CreativeCoreVec3Conversion corner =
          cr::creativeVec3ToCoreChecked(patch.corners[index]);
      if (!corner.converted) {
        output.clear();
        return false;
      }
      item.corners[index] = corner.value;
    }
    output.push_back(item);
  }
  return true;
}

void refreshTerrainSurfacePlan(CreativeEditorSceneCache& cache,
                               const cr::CreativeDocument& document) {
  const cr::CreativeGridSettings grid = document.gridSettings();
  const bool geometryCurrent =
      cache.valid && cache.documentId == document.id() &&
      cache.terrainRevision == document.terrainField().revision() &&
      cr::creativeVec3ExactlyEqual(cache.terrainGridOrigin, grid.origin) &&
      cache.terrainGridCellSizeMeters == grid.cellSizeMeters;
  const bool materialCurrent =
      cache.valid && cache.documentId == document.id() &&
      cache.terrainMaterialRevision ==
          document.terrainMaterialField().revision();
  if (geometryCurrent && materialCurrent) {
    return;
  }
  if (geometryCurrent) {
    std::vector<cr::CreativeTerrainSurfacePatch> displayPatches =
        cache.terrainCollisionPatches;
    for (cr::CreativeTerrainSurfacePatch& patch : displayPatches) {
      patch.material =
          document.terrainMaterialField().materialAt(patch.coord);
    }
    cr::CreativeTerrainRenderPlan displayPlan;
    displayPlan.accepted = true;
    displayPlan.patches = std::move(displayPatches);
    static_cast<void>(convertTerrainSurfacePatches(
        displayPlan, cache.terrainSurfacePatches));
    cache.terrainMaterialRevision =
        document.terrainMaterialField().revision();
    ++cache.terrainMaterialBuildCount;
    return;
  }
  const cr::CreativeTerrainSurfacePlan plan =
      cr::buildCreativeTerrainSurfacePlan(document.terrainField());
  cache.terrainCuboids = plan.accepted ? plan.cuboids
                                      : std::vector<cr::CreativeVoxelCuboid>{};
  const cr::CreativeTerrainRenderPlan renderPlan =
      cr::buildCreativeTerrainRenderPlan(
          plan, document.terrainMaterialField(), grid.origin,
          grid.cellSizeMeters);
  if (convertTerrainSurfacePatches(renderPlan,
                                   cache.terrainSurfacePatches)) {
    cache.terrainCollisionPatches = renderPlan.patches;
  } else {
    cache.terrainCollisionPatches.clear();
  }
  cache.terrainRevision = document.terrainField().revision();
  cache.terrainMaterialRevision =
      document.terrainMaterialField().revision();
  cache.terrainGridOrigin = grid.origin;
  cache.terrainGridCellSizeMeters = grid.cellSizeMeters;
  ++cache.terrainSurfaceBuildCount;
  ++cache.terrainMaterialBuildCount;
}

[[nodiscard]] StandaloneRoomBakePreviewScene
buildStandaloneRoomBakePreviewSceneWithVoxelPlans(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::ProductMapMakerGridSnapshot& gridSnapshot,
    std::span<const cr::CreativeVoxelCuboid> voxelCuboids,
    std::span<const cr::CreativeTerrainSurfacePatch> terrainCollisionPatches,
    std::span<const iggy3d::SceneRoomSurfacePatchItem> terrainSurfacePatches,
    bool usePrecomputedVoxelCuboids) {
  iggy3d::creative::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = &document;
  bakeRequest.roomId = "iggy3d_creative_preview";
  bakeRequest.sourceName = "apps/iggy3d_creative";
  bakeRequest.sourceSubset = "standalone_preview";
  bakeRequest.usePrecomputedVoxelCuboids = usePrecomputedVoxelCuboids;
  bakeRequest.precomputedVoxelCuboids = voxelCuboids;
  bakeRequest.usePrecomputedTerrainSurfacePatches =
      !terrainCollisionPatches.empty();
  bakeRequest.precomputedTerrainSurfacePatches = terrainCollisionPatches;

  StandaloneRoomBakePreviewScene preview;
  preview.roomBake =
      iggy3d::creative::buildRoomAssetFromCreativeDocument(bakeRequest);

  iggy3d::SessionState emptyRuntimeState;
  preview.scene = iggy3d::buildSceneProjection(emptyRuntimeState,
                                               &preview.roomBake.room);
  preview.scene.room.surfacePatches.assign(terrainSurfacePatches.begin(),
                                           terrainSurfacePatches.end());
  appendGridDotsToScene(gridSnapshot, preview.scene);
  preview.standalonePreviewMeshCount = appendStandalonePreviewProxiesToScene(
      document, preview.roomBake.staticMeshSources, preview.scene);
  if (!preview.scene.room.meshes.empty() ||
      !preview.scene.room.surfacePatches.empty()) {
    preview.scene.room.staticMeshCount = preview.scene.room.meshes.size();
    preview.scene.room.loaded = true;
  }
  return preview;
}

}  // namespace

StandaloneRoomBakePreviewScene buildStandaloneRoomBakePreviewScene(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::ProductMapMakerGridSnapshot& gridSnapshot) {
  std::vector<cr::CreativeVoxelCuboid> cuboids =
      cr::buildCreativeVoxelCuboids(document.voxelField());
  const cr::CreativeTerrainSurfacePlan terrain =
      cr::buildCreativeTerrainSurfacePlan(document.terrainField());
  std::vector<cr::CreativeTerrainSurfacePatch> terrainCollisionPatches;
  std::vector<iggy3d::SceneRoomSurfacePatchItem> terrainSurfacePatches;
  if (terrain.accepted) {
    cuboids.insert(cuboids.end(), terrain.cuboids.begin(),
                   terrain.cuboids.end());
    const cr::CreativeGridSettings grid = document.gridSettings();
    const cr::CreativeTerrainRenderPlan renderPlan =
        cr::buildCreativeTerrainRenderPlan(
            terrain, document.terrainMaterialField(), grid.origin,
            grid.cellSizeMeters);
    if (convertTerrainSurfacePatches(renderPlan, terrainSurfacePatches)) {
      terrainCollisionPatches = renderPlan.patches;
    }
  }
  return buildStandaloneRoomBakePreviewSceneWithVoxelPlans(
      document, gridSnapshot, cuboids, terrainCollisionPatches,
      terrainSurfacePatches, true);
}

bool refreshCreativeEditorSceneCache(
    CreativeEditorSceneCache& cache,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::ProductMapMakerGridSnapshot& gridSnapshot) {
  if (cache.valid && cache.documentId == document.id() &&
      cache.documentRevision == document.revision()) {
    return false;
  }
  if (cache.documentId != document.id()) {
    cache.voxelChunkMeshes.clear();
    cache.terrainCuboids.clear();
    cache.terrainCollisionPatches.clear();
    cache.terrainSurfacePatches.clear();
    cache.terrainRevision = 0;
    cache.terrainMaterialRevision = 0;
    cache.terrainGridOrigin = {};
    cache.terrainGridCellSizeMeters = 0.0;
  }
  std::vector<cr::CreativeVoxelCuboid> voxelCuboids =
      refreshVoxelChunkMeshPlans(cache, document);
  refreshTerrainSurfacePlan(cache, document);
  voxelCuboids.insert(voxelCuboids.end(), cache.terrainCuboids.begin(),
                      cache.terrainCuboids.end());
  cache.preview = buildStandaloneRoomBakePreviewSceneWithVoxelPlans(
      document, gridSnapshot, voxelCuboids, cache.terrainCollisionPatches,
      cache.terrainSurfacePatches, true);
  cache.documentId = document.id();
  cache.documentRevision = document.revision();
  ++cache.refreshCount;
  cache.valid = true;
  return true;
}

void invalidateCreativeEditorSceneCache(
    CreativeEditorSceneCache& cache) noexcept {
  cache.valid = false;
  cache.documentId = iggy3d::creative::kInvalidDocumentId;
  cache.documentRevision = 0;
  cache.voxelChunkMeshes.clear();
  cache.terrainCuboids.clear();
  cache.terrainCollisionPatches.clear();
  cache.terrainSurfacePatches.clear();
  cache.terrainRevision = 0;
  cache.terrainMaterialRevision = 0;
  cache.terrainGridOrigin = {};
  cache.terrainGridCellSizeMeters = 0.0;
}

void attachCreativeEditorPlacementPreviews(
    const CreativeEditorState& editor,
    bool captureMode,
    FrameInput& frame,
    const cr::CreativeDocument* document) {
  frame.creativePreview = {};
  const bool modalOpen = editor.catalog.model.open ||
                         editor.catalog.toolWheel.open ||
                         editor.toolOptions.open ||
                         editor.transform.active;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const bool materialPlacement =
      held.kind == cr::CreativeHeldItemKind::Material;
  const bool materialBrush =
      held.kind == cr::CreativeHeldItemKind::MaterialBrush;
  if (captureMode || modalOpen ||
      (!materialPlacement && !materialBrush) ||
      held.objectKind == cr::CreativeObjectKind::Unknown) {
    return;
  }

  const CreativeBrushPlacementPlan heldPlan =
      planBrushPlacement(held.objectKind, {});
  Vec3 heldCenter{};
  Vec3 heldSize{};
  const cr::CreativeBounds heldBounds =
      creativeBrushHeldPreviewBounds(heldPlan);
  if (!heldPlan.valid ||
      !previewBoundsTransform(heldBounds, 1.0F, heldCenter,
                              heldSize)) {
    return;
  }

  const CreativeEditorPlacementFeedback& feedback =
      editor.interaction.placementFeedback;
  const bool mutationAcceptedThisFrame =
      feedback.frameIndex == editor.frameIndex &&
      feedback.status == CreativeEditorPlacementFeedbackStatus::Placed;
  if (materialPlacement && editor.interaction.target.grid.valid &&
      !mutationAcceptedThisFrame) {
    const CreativeBrushPlacementAdmission admission = admitBrushPlacement(
        held.objectKind, editor.interaction.target.grid,
        editor.toolSettings.placementYaw);
    const CreativeBrushPlacementPlan& targetPlan = admission.plan;
    const cr::CreativeBounds& targetBounds =
        targetPlan.valid ? targetPlan.previewBounds
                         : editor.interaction.target.grid.adjacentCellBounds;
    Vec3 targetCenter{};
    Vec3 targetSize{};
    if (previewBoundsTransform(targetBounds, 1.0F, targetCenter,
                               targetSize)) {
      const bool rejectedThisFrame =
          feedback.frameIndex == editor.frameIndex &&
          feedback.status == CreativeEditorPlacementFeedbackStatus::Rejected;
      const bool duplicate =
          document != nullptr && admission.allowed &&
          creativeBrushPlacementTargetOccupied(*document, targetPlan);
      const bool targetInvalid =
          !admission.allowed || duplicate || rejectedThisFrame ||
          editor.interaction.materialStroke.capacityReached;
      appendCreativePreview(
          frame.creativePreview,
          targetInvalid ? RenderCreativePreviewRole::PlacementInvalid
                        : RenderCreativePreviewRole::PlacementValid,
          frame.camera.clipFromWorld *
              modelMatrix(
                  targetCenter,
                  targetPlan.valid
                      ? cr::creativeVec3ToCoreChecked(
                            targetPlan.transform.rotationEulerRadians)
                            .value
                      : Vec3{},
                  targetSize),
          targetPlan.valid &&
              targetPlan.shapeKind == cr::CreativeObjectShapeKind::Path);
    }
  }

  constexpr float kHeldLongestDimension = 0.32F;
  constexpr float kHeldMinimumAxis = 0.06F;
  const float longest = std::max({heldSize.x, heldSize.y, heldSize.z});
  if (!std::isfinite(longest) || longest <= 0.0F) {
    return;
  }
  const float heldScale = kHeldLongestDimension / longest;
  heldSize = {std::max(kHeldMinimumAxis, heldSize.x * heldScale),
              std::max(kHeldMinimumAxis, heldSize.y * heldScale),
              std::max(kHeldMinimumAxis, heldSize.z * heldScale)};
  constexpr float kDegreesToRadians = 0.01745329251994329577F;
  const Vec3 heldRotation{20.0F * kDegreesToRadians,
                          -35.0F * kDegreesToRadians,
                          8.0F * kDegreesToRadians};
  appendCreativePreview(
      frame.creativePreview, RenderCreativePreviewRole::Held,
      frame.camera.clipFromView *
          modelMatrix({0.42F, -0.32F, -0.82F}, heldRotation, heldSize));
}

void logStandaloneRoomBakeFinal(
    const StandaloneRoomBakePreviewScene& preview) {
  const iggy3d::creative::CreativeRoomBakeReceipt& receipt =
      preview.roomBake.receipt;
  SDL_Log("iggy3d_creative: ROOM_BAKE final status='%s' reasonCode='%s' "
          "accepted=%d objectCount=%llu considered=%llu staticMeshes=%llu "
          "spatialSurfaces=%llu skippedHidden=%llu skippedEditorOnly=%llu "
          "skippedNoBounds=%llu skippedUnsupported=%llu "
          "skippedRoomMetadata=%llu standalonePreviewMeshes=%zu "
          "sceneMeshes=%zu",
          std::string(iggy3d::creative::toString(receipt.status)).c_str(),
          receipt.reasonCode.c_str(), receipt.accepted ? 1 : 0,
          static_cast<unsigned long long>(receipt.objectCount),
          static_cast<unsigned long long>(receipt.consideredObjectCount),
          static_cast<unsigned long long>(receipt.bakedStaticMeshCount),
          static_cast<unsigned long long>(receipt.bakedSpatialSurfaceCount),
          static_cast<unsigned long long>(receipt.skippedHiddenCount),
          static_cast<unsigned long long>(receipt.skippedEditorOnlyCount),
          static_cast<unsigned long long>(receipt.skippedNoBoundsCount),
          static_cast<unsigned long long>(
              receipt.skippedUnsupportedShapeCount),
          static_cast<unsigned long long>(receipt.skippedRoomMetadataCount),
          preview.standalonePreviewMeshCount,
          preview.scene.room.meshes.size());
}

namespace {

void resetCreativeEditorOverlayFrame(CreativeEditorOverlayFrame& output) {
  output.uiRects.clear();
  output.glyphs.clear();
  output.combinedWireLines.clear();
  output.documentWireLineCount = 0;
  output.pointMarkerEdgeCount = 0;
  output.lineMarkerEdgeCount = 0;
  output.pathPointHandleEdgeCount = 0;
  output.ghostEdgeCount = 0;
  output.materialBrushPivotEdgeCount = 0;
  output.materialBrushGuideLineCount = 0;
  output.materialBrushEdgeCount = 0;
  output.connectedFillEdgeCount = 0;
  output.surfaceExtrudeEdgeCount = 0;
  output.terrainEdgeCount = 0;
  output.volumeEdgeCount = 0;
  output.patternEdgeCount = 0;
  output.transformPreviewEdgeCount = 0;
  output.placementFeedbackEdgeCount = 0;
}

void attachCreativeEditorOverlayFrame(FrameInput& frame,
                                      CreativeEditorOverlayFrame& output) {
  frame.ui.visible = true;
  frame.ui.rects = output.uiRects.data();
  frame.ui.rectCount = output.uiRects.size();
  frame.ui.textGlyphQuads = output.glyphs.data();
  frame.ui.textGlyphQuadCount = output.glyphs.size();

  RenderCreativeWireframeDebugFrame combinedWireFrame;
  combinedWireFrame.available = true;
  combinedWireFrame.visible = !output.combinedWireLines.empty();
  combinedWireFrame.lines = output.combinedWireLines.data();
  combinedWireFrame.lineCount = output.combinedWireLines.size();
  frame.creativeWireframeDebug = combinedWireFrame;
}

void appendCreativeEditorPlacementFeedbackWireframe(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  CreativeEditorState& editor = request.editor;
  std::vector<RenderCreativeWireframeDebugLine>& lines =
      output.combinedWireLines;
  const CreativeEditorPlacementFeedback& feedback =
      editor.interaction.placementFeedback;
  if (feedback.status != CreativeEditorPlacementFeedbackStatus::Placed ||
      !creativeEditorPlacementFeedbackVisible(feedback, editor.frameIndex)) {
    return;
  }
  const creative::CreativeObject* placedObject =
      request.appState.facade.findObject(feedback.objectId);
  if (placedObject != nullptr) {
    const VisualBounds placedBounds = visualBoundsForObject(*placedObject);
    const std::size_t before = lines.size();
    appendStandaloneWireframeBoxEdges(
        lines, placedBounds.min, placedBounds.max,
        RenderLineColor{0.25F, 1.0F, 0.35F, 1.0F},
        std::max(0.075F, request.gizmoThickness * 1.4F));
    for (std::size_t index = before; index < lines.size(); ++index) {
      lines[index].objectId = placedObject->id;
    }
    output.placementFeedbackEdgeCount = lines.size() - before;
    return;
  }
  if (!feedback.voxelPlaced) {
    return;
  }
  Vec3 center{};
  Vec3 size{};
  if (!previewBoundsTransform(feedback.voxelBounds, 1.0F, center, size)) {
    return;
  }
  const Vec3 minimum =
      cr::creativeVec3ToCoreChecked(feedback.voxelBounds.min).value;
  const Vec3 maximum =
      cr::creativeVec3ToCoreChecked(feedback.voxelBounds.max).value;
  const std::size_t before = lines.size();
  appendStandaloneWireframeBoxEdges(
      lines, minimum, maximum,
      RenderLineColor{0.25F, 1.0F, 0.35F, 1.0F},
      std::max(0.075F, request.gizmoThickness * 1.4F));
  output.placementFeedbackEdgeCount = lines.size() - before;
}

void appendCreativeEditorMaterialBrushWireframe(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  CreativeEditorState& editor = request.editor;
  cr::CreativeAppState& appState = request.appState;
  std::vector<RenderCreativeWireframeDebugLine>& lines =
      output.combinedWireLines;
  const MaterialBrushPreviewPlan preview =
      materialBrushPreviewPlan(editor, appState.facade.document());
  if (editor.transform.active ||
      (!preview.visible && !preview.showSymmetryPivot)) {
    return;
  }
  const RenderLineColor directColor =
      preview.removing
          ? RenderLineColor{1.0F, 0.2F, 0.16F, 1.0F}
          : preview.admitted
                ? RenderLineColor{0.22F, 1.0F, 0.34F, 1.0F}
                : RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F};
  const RenderLineColor mirroredColor =
      preview.admitted ? RenderLineColor{0.18F, 0.9F, 1.0F, 1.0F}
                       : RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F};
  const cr::CreativeGridSettings grid = appState.facade.document().gridSettings();
  const std::size_t pivotBefore = lines.size();
  if (preview.showSymmetryPivot) {
    static_cast<void>(appendCreativeMaterialBrushPivotMarker(
        lines, preview.symmetryPivot, grid,
        std::max(0.045F, request.gizmoThickness * 1.4F)));
  }
  output.materialBrushPivotEdgeCount = lines.size() - pivotBefore;
  const std::size_t guideBefore = lines.size();
  if (preview.visible && preview.hasGuideAnchor) {
    static_cast<void>(appendCreativeMaterialBrushGuideLine(
        lines, preview.guide, preview.guideAnchor, preview.center, grid,
        std::max(0.045F, request.gizmoThickness * 1.4F)));
  }
  output.materialBrushGuideLineCount = lines.size() - guideBefore;
  const std::size_t before = lines.size();
  if (preview.visible) {
    appendCreativeMaterialBrushCellOutlines(
        lines, preview.renderedDirectCells(), grid, directColor,
        request.gizmoThickness);
    appendCreativeMaterialBrushCellOutlines(
        lines, preview.renderedMirroredCells(), grid, mirroredColor,
        request.gizmoThickness);
  }
  output.materialBrushEdgeCount = lines.size() - before;
}

[[nodiscard]] bool creativeEditorWorldPreviewBlocked(
    const CreativeEditorOverlayFrameRequest& request) noexcept {
  const CreativeEditorState& editor = request.editor;
  return request.captureMode || editor.catalog.model.open ||
         editor.catalog.toolWheel.open || editor.toolOptions.open ||
         editor.controls.open || editor.transform.active ||
         editor.transform.controlsOpen;
}

void appendCreativeEditorConnectedFillWireframe(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  CreativeEditorState& editor = request.editor;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (creativeEditorWorldPreviewBlocked(request) ||
      held.kind != cr::CreativeHeldItemKind::ConnectedFill ||
      !editor.interaction.target.voxelHit) {
    return;
  }
  const cr::CreativeConnectedFillPlan& plan =
      resolveCreativeEditorConnectedFillPlan(
          editor.interaction.connectedFill,
          request.appState.facade.document(),
          editor.interaction.target.voxelCell,
          editor.toolSettings.connectedFillLimit);
  const bool valid =
      plan.accepted && cr::creativeVolumeBrushSupported(held.objectKind) &&
      plan.sourceMaterial != held.objectKind;
  const RenderLineColor color =
      valid ? RenderLineColor{0.12F, 0.92F, 1.0F, 1.0F}
            : RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F};
  const std::array<cr::CreativeGridCoord3, 1U> rejectedSeed{
      editor.interaction.target.voxelCell};
  const std::span<const cr::CreativeGridCoord3> cells =
      plan.accepted ? plan.generatedCells()
                    : std::span<const cr::CreativeGridCoord3>{rejectedSeed};
  const std::size_t before = output.combinedWireLines.size();
  appendCreativeMaterialBrushCellOutlines(
      output.combinedWireLines, cells,
      request.appState.facade.document().gridSettings(), color,
      request.gizmoThickness);
  output.connectedFillEdgeCount = output.combinedWireLines.size() - before;
}

void appendCreativeEditorSurfaceExtrudeWireframe(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  CreativeEditorState& editor = request.editor;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (creativeEditorWorldPreviewBlocked(request) ||
      held.kind != cr::CreativeHeldItemKind::SurfaceExtrude ||
      !editor.interaction.target.voxelHit) {
    return;
  }
  cr::CreativeGridCoord3 outward{};
  const bool faceValid = creativeSurfaceFaceOffset(
      editor.interaction.target.grid.faceNormal, outward);
  const cr::CreativeSurfaceExtrudePlan* plan = nullptr;
  if (faceValid) {
    plan = &resolveCreativeEditorSurfaceExtrudePlan(
        editor.interaction.surfaceExtrude,
        request.appState.facade.document(),
        editor.interaction.target.voxelCell, outward,
        cr::CreativeSurfaceExtrudeKind::Extrude,
        editor.toolSettings.surfaceExtrudeDepth,
        editor.toolSettings.surfaceExtrudeLimit);
  }
  const bool valid = plan != nullptr && plan->accepted &&
                     cr::creativeVolumeBrushSupported(held.objectKind);
  const RenderLineColor color =
      valid ? RenderLineColor{0.12F, 0.82F, 1.0F, 1.0F}
            : RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F};
  const std::array<cr::CreativeGridCoord3, 1U> rejectedSeed{
      editor.interaction.target.voxelCell};
  const std::span<const cr::CreativeGridCoord3> cells =
      valid ? plan->generatedCells()
            : std::span<const cr::CreativeGridCoord3>{rejectedSeed};
  const std::size_t before = output.combinedWireLines.size();
  appendCreativeMaterialBrushCellOutlines(
      output.combinedWireLines, cells,
      request.appState.facade.document().gridSettings(), color,
      request.gizmoThickness);
  output.surfaceExtrudeEdgeCount = output.combinedWireLines.size() - before;
}

struct CreativeEditorVolumePreviewFacts {
  creative::CreativeVolumeSelection selection{};
  creative::CreativeShapeBrushPlanReceipt shapePlan{};
  bool selectionVisible = false;
  bool usesShapePlan = false;
  bool terrainRegion = false;
  bool terrainStamp = false;
};

[[nodiscard]] bool terrainStampHasPositionableFootprint(
    const creative::CreativeTerrainStampPlan& plan) noexcept {
  switch (plan.status) {
    case creative::CreativeTerrainStampPlanStatus::CapacityExceeded:
    case creative::CreativeTerrainStampPlanStatus::HeightOutOfRange:
    case creative::CreativeTerrainStampPlanStatus::NoChange:
    case creative::CreativeTerrainStampPlanStatus::Ready:
      return plan.transformedWidthCells > 0U &&
             plan.transformedDepthCells > 0U;
    case creative::CreativeTerrainStampPlanStatus::NotRequested:
    case creative::CreativeTerrainStampPlanStatus::InvalidStamp:
    case creative::CreativeTerrainStampPlanStatus::InvalidDestination:
    case creative::CreativeTerrainStampPlanStatus::InvalidRequest:
    case creative::CreativeTerrainStampPlanStatus::CoordinateOverflow:
      return false;
  }
  return false;
}

CreativeEditorVolumePreviewFacts appendCreativeEditorVolumeAndToolWireframes(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  CreativeEditorVolumePreviewFacts facts;
  CreativeEditorState& editor = request.editor;
  std::vector<RenderCreativeWireframeDebugLine>& lines =
      output.combinedWireLines;
  if (editor.volume.active && !editor.transform.active) {
    const creative::CreativeHotbarEntry& held =
        creative::selectedCreativeHotbarEntry(editor.interaction.hotbar);
    facts.terrainRegion =
        held.kind == creative::CreativeHeldItemKind::TerrainRegion;
    facts.terrainStamp =
        facts.terrainRegion && editor.terrain.region.stamp.active;
    if (facts.terrainStamp) {
      const CreativeTerrainStampPreviewCache& preview =
          editor.terrain.region.stamp.preview;
      if (preview.valid &&
          terrainStampHasPositionableFootprint(preview.plan)) {
        facts.selection.phase =
            creative::CreativeVolumeSelectionPhase::Complete;
        facts.selection.firstCell = {preview.plan.targetMinimum.x, 0,
                                     preview.plan.targetMinimum.z};
        facts.selection.secondCell = {preview.plan.targetMaximum.x, 0,
                                      preview.plan.targetMaximum.z};
        const creative::CreativeGridSettings grid =
            request.appState.facade.document().gridSettings();
        facts.selection.origin = grid.origin;
        facts.selection.cellSize = grid.cellSizeMeters;
      }
    } else {
      facts.selection = creativeEditorVolumePreviewSelection(editor.volume);
    }
    const bool regionHidden =
        facts.terrainRegion &&
        (request.captureMode || editor.catalog.model.open ||
         editor.catalog.toolWheel.open || editor.toolOptions.open ||
         editor.controls.open || editor.transform.controlsOpen);
    if (creative::creativeVolumeSelectionValid(facts.selection) &&
        !regionHidden) {
      facts.selectionVisible = true;
      facts.usesShapePlan =
          !facts.terrainRegion &&
          (editor.volume.operation ==
               creative::CreativeVolumeOperationKind::Fill ||
           editor.volume.operation ==
               creative::CreativeVolumeOperationKind::Hollow);
      if (facts.usesShapePlan) {
        creative::CreativeShapeBrushPlanRequest planRequest;
        planRequest.kind = editor.toolSettings.shapeBrushKind;
        planRequest.axis = editor.toolSettings.shapeBrushAxis;
        planRequest.firstCell = facts.selection.firstCell;
        planRequest.secondCell = facts.selection.secondCell;
        planRequest.hollow =
            editor.volume.operation ==
            creative::CreativeVolumeOperationKind::Hollow;
        planRequest.maxCandidateCellCount = kCreativeEditorVolumeCellLimit;
        planRequest.maxGeneratedCellCount = kCreativeEditorVolumeCellLimit;
        facts.shapePlan = creative::planCreativeShapeBrush(planRequest);
      }
      const std::size_t before = lines.size();
      RenderLineColor color = volumeOperationColor(editor.volume.operation);
      if (facts.terrainRegion) {
        const bool accepted =
            facts.terrainStamp
                ? editor.terrain.region.stamp.preview.valid &&
                      editor.terrain.region.stamp.preview.plan.accepted &&
                      editor.terrain.region.stamp.preview.renderAccepted
                : editor.terrain.region.preview.valid &&
                      editor.terrain.region.preview.plan.accepted &&
                      editor.terrain.region.preview.renderAccepted;
        const bool replacing =
            facts.terrainStamp
                ? editor.terrain.region.stamp.preview.mode ==
                      creative::CreativeTerrainStampMode::Replace
                : editor.terrain.region.preview.operation ==
                      creative::CreativeTerrainRegionOperation::Erase;
        color = !accepted
                    ? RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F}
                    : replacing
                          ? RenderLineColor{1.0F, 0.46F, 0.12F, 1.0F}
                          : RenderLineColor{0.20F, 1.0F, 0.35F, 1.0F};
      } else if (facts.usesShapePlan && !facts.shapePlan.accepted) {
        color = RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F};
      }
      appendCreativeShapeBrushOutline(
          lines, facts.selection,
          facts.usesShapePlan ? editor.toolSettings.shapeBrushKind
                              : creative::CreativeShapeBrushKind::Box,
          editor.toolSettings.shapeBrushAxis, color, request.gizmoThickness);
      output.volumeEdgeCount = lines.size() - before;
    }
  }
  if (!editor.transform.active) {
    output.patternEdgeCount = appendCreativeEditorArrayPreview(
        request.appState, editor, request.gizmoThickness, lines);
  }
  output.transformPreviewEdgeCount =
      appendCreativeEditorSelectionTransformPreview(
          editor.transform, request.gizmoThickness, lines);
  const std::size_t terrainBefore = lines.size();
  appendCreativeEditorTerrainOverlay(
      request.appState.facade.document(), editor, request.gizmoThickness, lines,
      request.captureMode);
  output.terrainEdgeCount = lines.size() - terrainBefore;
  return facts;
}

void appendCreativeEditorVolumePreviewLabel(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output,
    const CreativeEditorVolumePreviewFacts& facts) {
  if (output.volumeEdgeCount == 0U || !facts.selectionVisible) {
    return;
  }
  CreativeEditorState& editor = request.editor;
  const creative::CreativeGridBounds3 gridBounds =
      creative::creativeVolumeGridBounds(facts.selection);
  const creative::CreativeBounds bounds =
      creative::creativeVolumeWorldBounds(facts.selection);
  const creative::CreativeBoundsMetrics metrics =
      creative::measureCreativeBounds(bounds);
  const Vec3 labelPosition = creative::creativeVec3ToCoreChecked(
                                 {metrics.center.x, bounds.max.y,
                                  metrics.center.z})
                                 .value;
  const creative::CreativeScreenPoint screenPoint =
      creative::projectCreativeWorldPointToScreen(
          request.frame.camera.clipFromWorld, labelPosition,
          request.drawableWidth, request.drawableHeight);
  if (!screenPoint.valid) {
    return;
  }
  char label[128];
  if (facts.terrainStamp) {
    const CreativeTerrainStampPreviewCache& preview =
        editor.terrain.region.stamp.preview;
    std::snprintf(
        label, sizeof(label), "STAMP %s %s Y%+d | %d x %d | %u rods | %s",
        std::string(creative::toString(preview.mode)).c_str(),
        std::string(creative::toString(preview.elevationMode)).c_str(),
        preview.plan.appliedHeightOffsetCells,
        gridBounds.max.x - gridBounds.min.x,
        gridBounds.max.z - gridBounds.min.z,
        static_cast<unsigned>(preview.plan.finalControlCount),
        std::string(creative::toString(preview.plan.status)).c_str());
  } else if (facts.terrainRegion) {
    const CreativeTerrainRegionPreviewCache& preview =
        editor.terrain.region.preview;
    std::snprintf(
        label, sizeof(label), "TERRAIN %s %d x %d | %u rods | %s",
        std::string(creative::toString(
                        editor.toolSettings.terrainRegionOperation))
            .c_str(),
        gridBounds.max.x - gridBounds.min.x,
        gridBounds.max.z - gridBounds.min.z,
        static_cast<unsigned>(preview.plan.affectedControlCount),
        std::string(creative::toString(preview.plan.status)).c_str());
  } else {
    const std::uint64_t plannedCellCount =
      facts.usesShapePlan && facts.shapePlan.accepted
          ? facts.shapePlan.generatedCellCount
          : facts.usesShapePlan
                ? 0U
                : creative::creativeVolumeCellCount(facts.selection);
    const std::string shapeLabel =
      facts.usesShapePlan
          ? std::string(creative::toString(editor.toolSettings.shapeBrushKind))
          : std::string{"BOX"};
    const std::string axisLabel =
      facts.usesShapePlan &&
              editor.toolSettings.shapeBrushKind ==
                  creative::CreativeShapeBrushKind::Cylinder
          ? " " + std::string(
                      creative::toString(editor.toolSettings.shapeBrushAxis))
          : std::string{};
    if (editor.volume.lastReceipt.requested) {
      std::snprintf(
          label, sizeof(label), "%s %s%s %d x %d x %d | %llu cells | %s",
          std::string(creative::toString(editor.volume.operation)).c_str(),
          shapeLabel.c_str(), axisLabel.c_str(),
          gridBounds.max.x - gridBounds.min.x,
          gridBounds.max.y - gridBounds.min.y,
          gridBounds.max.z - gridBounds.min.z,
          static_cast<unsigned long long>(plannedCellCount),
          std::string(creative::toString(editor.volume.lastReceipt.status))
              .c_str());
    } else {
      std::snprintf(
          label, sizeof(label), "%s %s%s %d x %d x %d | %llu cells",
          std::string(creative::toString(editor.volume.operation)).c_str(),
          shapeLabel.c_str(), axisLabel.c_str(),
          gridBounds.max.x - gridBounds.min.x,
          gridBounds.max.y - gridBounds.min.y,
          gridBounds.max.z - gridBounds.min.z,
          static_cast<unsigned long long>(plannedCellCount));
    }
  }
  const DebugHudLayoutResult layout = layoutDebugHudTextAt(
      label, static_cast<std::int32_t>(screenPoint.x),
      static_cast<std::int32_t>(screenPoint.y), request.drawableWidth,
      request.drawableHeight);
  output.glyphs.insert(output.glyphs.end(), layout.quads.begin(),
                       layout.quads.end());
}

void appendCreativeEditorSelectionDimensionLabel(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output,
    bool hasSelection) {
  if (!hasSelection) {
    return;
  }
  const Vec3 center{
      (request.selection.boxMin.x + request.selection.boxMax.x) * 0.5F,
      (request.selection.boxMin.y + request.selection.boxMax.y) * 0.5F,
      (request.selection.boxMin.z + request.selection.boxMax.z) * 0.5F};
  const creative::CreativeScreenPoint screenPoint =
      creative::projectCreativeWorldPointToScreen(
          request.frame.camera.clipFromWorld, center, request.drawableWidth,
          request.drawableHeight);
  if (!screenPoint.valid) {
    return;
  }
  const float width = request.selection.boxMax.x - request.selection.boxMin.x;
  const float height = request.selection.boxMax.y - request.selection.boxMin.y;
  const float depth = request.selection.boxMax.z - request.selection.boxMin.z;
  char label[64];
  std::snprintf(label, sizeof(label), "%.1f x %.1f x %.1f m",
                static_cast<double>(width), static_cast<double>(height),
                static_cast<double>(depth));
  const DebugHudLayoutResult layout = layoutDebugHudTextAt(
      label, static_cast<std::int32_t>(screenPoint.x),
      static_cast<std::int32_t>(screenPoint.y), request.drawableWidth,
      request.drawableHeight);
  output.glyphs.insert(output.glyphs.end(), layout.quads.begin(),
                       layout.quads.end());
}

void appendCreativeEditorHudOverlays(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output,
    const CreativeEditorVolumePreviewFacts& volumeFacts,
    bool hasSelection) {
  CreativeEditorState& editor = request.editor;
  appendCreativeEditorInteractionOverlay(
      editor, request.drawableWidth, request.drawableHeight,
      request.gizmoThickness, output.uiRects, output.glyphs,
      output.combinedWireLines);
  appendCreativeEditorTransformOverlay(
      editor.transform, request.drawableWidth, request.drawableHeight,
      output.uiRects, output.glyphs);
  appendCreativeEditorCatalogOverlay(
      request.appState, editor, request.drawableWidth, request.drawableHeight,
      output.uiRects, output.glyphs);
  appendCreativeEditorToolOptionsOverlay(
      editor, request.drawableWidth, request.drawableHeight, output.uiRects,
      output.glyphs);
  appendCreativeEditorControlsOverlay(
      editor, request.drawableWidth, request.drawableHeight, output.uiRects,
      output.glyphs);
  appendCreativeEditorVolumePreviewLabel(request, output, volumeFacts);
  appendCreativeEditorSelectionDimensionLabel(request, output, hasSelection);
  appendCreativeEditorActionHintsOverlay(
      editor, request.inputContext, request.activeControlDevice,
      request.captureMode, request.drawableWidth, request.drawableHeight,
      output.uiRects, output.glyphs);
}

}  // namespace

void buildAndAttachCreativeEditorOverlayFrame(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  cr::CreativeAppState& appState = request.appState;
  CreativeEditorState& editor = request.editor;
  FrameInput& frame = request.frame;
  const cr::CreativeSpatialProjectionRequest& wireProjReq =
      request.wireProjectionRequest;
  const float gizmoThickness = request.gizmoThickness;

  attachCreativeEditorPlacementPreviews(
      editor, request.captureMode, frame,
      &request.appState.facade.document());

  const creative::Id selectedId = request.selection.selectedId;
  const creative::CreativeObject* selected = request.selection.selected;
  const bool hasSelection =
      request.selection.hasSelection && !editor.volume.active;
  const auto objectSelected = [&](creative::CreativeObjectId objectId) {
    return std::find(request.selection.selectedObjectIds.begin(),
                     request.selection.selectedObjectIds.end(),
                     objectId) != request.selection.selectedObjectIds.end();
  };
  const std::array<GizmoAxisShaft, 3>& gizmoShafts =
      request.gizmoFrame.shafts;
  const bool selectedIsPathForHandles =
      request.gizmoFrame.selectedIsPathForHandles;

  resetCreativeEditorOverlayFrame(output);

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
    const bool sel = hasSelection && objectSelected(line.objectId);
    line.thickness = sel ? 0.06F : 0.03F;
    if (sel) {
      line.color = {1.0F, 1.0F, 0.0F, 1.0F};
    }
  }
  CreativeWireframeDebugRenderFrame dbg =
      buildCreativeWireframeDebugRenderFrame(&lines.lineList);

  // ---- GIZMO WIREFRAME ----------------------------------------------------
  // Build ONE combined line vector: the document wireframe lines that draw the
  // yellow selection box (dbg.lines, already converted to render lines) PLUS
  // the 3 axis-aligned gizmo shafts. Point frame.creativeWireframeDebug at THIS
  // vector so the renderer draws both. The vector must outlive submitFrame(),
  // so it lives here in the frame-loop body. When nothing is selected we skip
  // the gizmo and the selection box is empty, so this is just dbg.lines.
  std::vector<RenderCreativeWireframeDebugLine>& combinedWireLines =
      output.combinedWireLines;
  combinedWireLines.reserve(dbg.lines.size() + 48);
  std::size_t& documentWireLineCount = output.documentWireLineCount;
  std::size_t& pointMarkerEdgeCount = output.pointMarkerEdgeCount;
  std::size_t& lineMarkerEdgeCount = output.lineMarkerEdgeCount;
  std::size_t& pathPointHandleEdgeCount = output.pathPointHandleEdgeCount;
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
  for (const creative::CreativeObject& obj :
       appState.facade.document().objects()) {
    const creative::CreativeObjectDescriptor& descriptor =
        creative::describeObject(obj.kind);
    if (!obj.visible) {
      continue;
    }
    const bool sel = hasSelection && objectSelected(obj.id);
    if (descriptor.shapeKind != creative::CreativeObjectShapeKind::Point &&
        descriptor.shapeKind != creative::CreativeObjectShapeKind::Line) {
      continue;
    }
    const VisualBounds markerBounds = visualBoundsForObject(obj);
    const std::size_t before = combinedWireLines.size();
    appendStandaloneWireframeBoxEdges(
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
      appendStandaloneWireframeBoxEdges(
          combinedWireLines,
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
      gizmoLine.start = request.gizmoFrame.center;
      gizmoLine.end = shaft.tip;  // Axis-aligned: only one component differs.
      gizmoLine.color = shaft.color;
      gizmoLine.objectId =
          static_cast<creative::CreativeObjectId>(selectedId);
      gizmoLine.thickness = gizmoThickness;
      combinedWireLines.push_back(gizmoLine);
    }
  }
  appendCreativeEditorPlacementFeedbackWireframe(request, output);
  // ---- MATERIAL BRUSH PREVIEW --------------------------------------------
  appendCreativeEditorMaterialBrushWireframe(request, output);

  // ---- CONNECTED FILL PREVIEW -------------------------------------------
  appendCreativeEditorConnectedFillWireframe(request, output);

  // ---- SURFACE EXTRUDE PREVIEW ------------------------------------------
  appendCreativeEditorSurfaceExtrudeWireframe(request, output);

  // ---- VOLUME PREVIEW ----------------------------------------------------
  const CreativeEditorVolumePreviewFacts volumeFacts =
      appendCreativeEditorVolumeAndToolWireframes(request, output);
  appendCreativeEditorHudOverlays(request, output, volumeFacts, hasSelection);

  // Attach only after every backing vector has reached its final size.
  attachCreativeEditorOverlayFrame(frame, output);
}

}  // namespace iggy3d_creative_app
