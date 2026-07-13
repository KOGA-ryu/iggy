#include "EditorPreviewFrame.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>

#include "EditorPreviewProxies.hpp"
#include "projection/scene/SceneItem.hpp"
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
    bool usePrecomputedVoxelCuboids,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog) {
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
  bakeRequest.staticMeshAssetCatalog = assetCatalog;

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
    const iggy3d::ProductMapMakerGridSnapshot& gridSnapshot,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog) {
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
      terrainSurfacePatches, true, assetCatalog);
}

bool refreshCreativeEditorSceneCache(
    CreativeEditorSceneCache& cache,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::ProductMapMakerGridSnapshot& gridSnapshot,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog) {
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
      cache.terrainSurfacePatches, true, assetCatalog);
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

void logStandaloneRoomBakeFinal(
    const StandaloneRoomBakePreviewScene& preview) {
  const iggy3d::creative::CreativeRoomBakeReceipt& receipt =
      preview.roomBake.receipt;
  SDL_Log("iggy3d_creative: ROOM_BAKE final status='%s' reasonCode='%s' "
          "accepted=%d objectCount=%llu considered=%llu staticMeshes=%llu "
          "spatialSurfaces=%llu skippedHidden=%llu skippedEditorOnly=%llu "
          "skippedNoBounds=%llu skippedUnsupported=%llu "
          "skippedRoomMetadata=%llu assetBounds=%llu assetWalkable=%llu "
          "assetNoCollision=%llu assetMissing=%llu assetUnsupported=%llu "
          "assetInvalid=%llu assetWalkableTransformSkipped=%llu "
          "standalonePreviewMeshes=%zu "
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
          static_cast<unsigned long long>(
              receipt.bakedAssetBoundsCollisionCount),
          static_cast<unsigned long long>(
              receipt.bakedAssetWalkableSurfaceCount),
          static_cast<unsigned long long>(
              receipt.skippedAssetNoCollisionCount),
          static_cast<unsigned long long>(
              receipt.skippedMissingAssetMetadataCount),
          static_cast<unsigned long long>(
              receipt.skippedUnsupportedAssetCollisionCount),
          static_cast<unsigned long long>(
              receipt.skippedInvalidAssetMetadataCount),
          static_cast<unsigned long long>(
              receipt.skippedAssetWalkableTransformCount),
          preview.standalonePreviewMeshCount,
          preview.scene.room.meshes.size());
}

}  // namespace iggy3d_creative_app
