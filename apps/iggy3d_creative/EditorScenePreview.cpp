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

[[nodiscard]] std::vector<cr::CreativeVoxelCuboid> collectVoxelCuboids(
    std::span<const CreativeEditorVoxelChunkMeshCache> chunks) {
  std::size_t cuboidCount = 0U;
  for (const CreativeEditorVoxelChunkMeshCache& entry : chunks) {
    cuboidCount += entry.cuboids.size();
  }
  std::vector<cr::CreativeVoxelCuboid> cuboids;
  cuboids.reserve(cuboidCount);
  for (const CreativeEditorVoxelChunkMeshCache& entry : chunks) {
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
  const cr::CreativeTerrainHeightField& heightField =
      document.terrainHeightField();
  const bool geometryCurrent =
      cache.valid && cache.documentId == document.id() &&
      cache.terrainRevision == document.terrainField().revision() &&
      cache.terrainHeightRevision == heightField.revision() &&
      cache.terrainHeightCellCount == heightField.cellCount() &&
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
      cr::buildCreativeComposedTerrainSurfacePlan(
          document.terrainField(), heightField);
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
  cache.terrainHeightRevision = heightField.revision();
  cache.terrainHeightCellCount = heightField.cellCount();
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
  bakeRequest.usePrecomputedTerrainSurfacePatches = true;
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
    const iggy3d::StaticMeshAssetCatalog* assetCatalog) {
  std::vector<cr::CreativeVoxelCuboid> cuboids =
      cr::buildCreativeVoxelCuboids(document.voxelField());
  const cr::CreativeTerrainSurfacePlan terrain =
      cr::buildCreativeComposedTerrainSurfacePlan(
          document.terrainField(), document.terrainHeightField());
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
      document, cuboids, terrainCollisionPatches,
      terrainSurfacePatches, true, assetCatalog);
}

bool refreshCreativeEditorSceneCache(
    CreativeEditorSceneCache& cache,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog) {
  static_cast<void>(refreshCreativePlacementClearanceCache(
      cache.placementClearance, document));
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
    cache.terrainHeightRevision = 0;
    cache.terrainHeightCellCount = 0;
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
      document, voxelCuboids, cache.terrainCollisionPatches,
      cache.terrainSurfacePatches, true, assetCatalog);
  cache.documentId = document.id();
  cache.documentRevision = document.revision();
  ++cache.refreshCount;
  cache.valid = true;
  return true;
}

bool refreshCreativeEditorGeneratedTerrainPreview(
    CreativeEditorGeneratedTerrainPreviewCache& cache,
    const CreativeEditorSceneCache& sourceSceneCache,
    const cr::CreativeDocument& document,
    const cr::CreativeTerrainHeightField& candidate,
    std::uint64_t candidateHeightHash,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog) {
  const cr::CreativeGridSettings grid = document.gridSettings();
  const bool sourceValid =
      sourceSceneCache.valid && sourceSceneCache.documentId == document.id() &&
      sourceSceneCache.documentRevision == document.revision();
  const bool candidateValid =
      candidate.validateInvariants() && candidate.cellCount() > 0U;
  if (!sourceValid || !candidateValid) {
    invalidateCreativeEditorGeneratedTerrainPreview(cache);
    return false;
  }
  if (cache.valid && cache.documentId == document.id() &&
      cache.documentRevision == document.revision() &&
      cache.heightHash == candidateHeightHash &&
      cache.sourceSceneRefreshCount == sourceSceneCache.refreshCount &&
      cr::creativeVec3ExactlyEqual(cache.terrainGridOrigin, grid.origin) &&
      cache.terrainGridCellSizeMeters == grid.cellSizeMeters) {
    return false;
  }

  const cr::CreativeTerrainSurfacePlan sourceSurface =
      cr::buildCreativeComposedTerrainSurfacePlan(
          document.terrainField(), document.terrainHeightField());
  cr::CreativeTerrainSurfacePlan composed =
      cr::replaceCreativeTerrainSurfaceRegion(
          sourceSurface, candidate);
  if (!composed.accepted) {
    invalidateCreativeEditorGeneratedTerrainPreview(cache);
    return false;
  }
  const cr::CreativeTerrainRenderPlan render =
      cr::buildCreativeTerrainRenderPlan(
          composed, document.terrainMaterialField(), grid.origin,
          grid.cellSizeMeters);
  std::vector<iggy3d::SceneRoomSurfacePatchItem> surfacePatches;
  if (!convertTerrainSurfacePatches(render, surfacePatches)) {
    invalidateCreativeEditorGeneratedTerrainPreview(cache);
    return false;
  }

  std::vector<cr::CreativeVoxelCuboid> voxelCuboids =
      collectVoxelCuboids(sourceSceneCache.voxelChunkMeshes);
  voxelCuboids.insert(voxelCuboids.end(), composed.cuboids.begin(),
                      composed.cuboids.end());
  StandaloneRoomBakePreviewScene preview =
      buildStandaloneRoomBakePreviewSceneWithVoxelPlans(
          document, voxelCuboids, render.patches, surfacePatches, true,
          assetCatalog);
  if (!preview.roomBake.receipt.accepted) {
    invalidateCreativeEditorGeneratedTerrainPreview(cache);
    return false;
  }

  cache.preview = std::move(preview);
  cache.composedSurface = std::move(composed);
  cache.terrainCollisionPatches = render.patches;
  cache.terrainSurfacePatches = std::move(surfacePatches);
  cache.documentId = document.id();
  cache.documentRevision = document.revision();
  cache.heightHash = candidateHeightHash;
  cache.sourceSceneRefreshCount = sourceSceneCache.refreshCount;
  cache.terrainGridOrigin = grid.origin;
  cache.terrainGridCellSizeMeters = grid.cellSizeMeters;
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
  cache.terrainHeightRevision = 0;
  cache.terrainHeightCellCount = 0;
  cache.terrainMaterialRevision = 0;
  cache.terrainGridOrigin = {};
  cache.terrainGridCellSizeMeters = 0.0;
  invalidateCreativePlacementClearanceCache(cache.placementClearance);
}

void invalidateCreativeEditorGeneratedTerrainPreview(
    CreativeEditorGeneratedTerrainPreviewCache& cache) noexcept {
  cache.preview = {};
  cache.composedSurface = {};
  cache.terrainCollisionPatches.clear();
  cache.terrainSurfacePatches.clear();
  cache.documentId = cr::kInvalidDocumentId;
  cache.documentRevision = 0U;
  cache.heightHash = 0U;
  cache.sourceSceneRefreshCount = 0U;
  cache.terrainGridOrigin = {};
  cache.terrainGridCellSizeMeters = 0.0;
  cache.valid = false;
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
