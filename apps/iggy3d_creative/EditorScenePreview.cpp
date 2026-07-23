#include "EditorPreviewFrame.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
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
    double bottomY,
    std::vector<iggy3d::SceneRoomSurfacePatchItem>& output) {
  output.clear();
  if (!plan.accepted || !std::isfinite(bottomY)) {
    return false;
  }
  struct EdgeSpec {
    std::int32_t neighborX = 0;
    std::int32_t neighborZ = 0;
    std::size_t topFirst = 0U;
    std::size_t topSecond = 0U;
    std::size_t lowerFirst = 0U;
    std::size_t lowerSecond = 0U;
  };
  constexpr std::array edges{
      EdgeSpec{0, -1, 0U, 1U, 3U, 2U},
      EdgeSpec{1, 0, 1U, 2U, 0U, 3U},
      EdgeSpec{0, 1, 2U, 3U, 1U, 0U},
      EdgeSpec{-1, 0, 3U, 0U, 2U, 1U},
  };
  const auto patchLess = [](const cr::CreativeTerrainSurfacePatch& patch,
                            cr::CreativeTerrainCoord2 coord) {
    return patch.coord.z != coord.z ? patch.coord.z < coord.z
                                    : patch.coord.x < coord.x;
  };
  const auto findPatch = [&](cr::CreativeTerrainCoord2 coord)
      -> const cr::CreativeTerrainSurfacePatch* {
    const auto found = std::lower_bound(plan.patches.begin(),
                                        plan.patches.end(), coord, patchLess);
    return found != plan.patches.end() && found->coord == coord ? &*found
                                                                : nullptr;
  };
  std::size_t hardFaceCount = 0U;
  for (const cr::CreativeTerrainSurfacePatch& patch : plan.patches) {
    hardFaceCount += static_cast<std::size_t>(
        std::popcount(static_cast<unsigned int>(patch.hardEdgeMask)));
  }
  output.reserve(plan.patches.size() + hardFaceCount);
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
    const cr::CreativeCoreVec3Conversion tint =
        cr::creativeVec3ToCoreChecked(patch.materialColor);
    if (!tint.converted) {
      output.clear();
      return false;
    }
    item.tint = tint.value;
    item.hasTint = true;
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

  for (const cr::CreativeTerrainSurfacePatch& patch : plan.patches) {
    for (std::size_t edgeIndex = 0U; edgeIndex < edges.size(); ++edgeIndex) {
      if ((patch.hardEdgeMask & (1U << edgeIndex)) == 0U) {
        continue;
      }
      const EdgeSpec edge = edges[edgeIndex];
      const std::int64_t neighborX =
          static_cast<std::int64_t>(patch.coord.x) + edge.neighborX;
      const std::int64_t neighborZ =
          static_cast<std::int64_t>(patch.coord.z) + edge.neighborZ;
      const cr::CreativeTerrainSurfacePatch* neighbor = nullptr;
      if (neighborX >= std::numeric_limits<std::int32_t>::min() &&
          neighborX <= std::numeric_limits<std::int32_t>::max() &&
          neighborZ >= std::numeric_limits<std::int32_t>::min() &&
          neighborZ <= std::numeric_limits<std::int32_t>::max()) {
        neighbor = findPatch({static_cast<std::int32_t>(neighborX),
                              static_cast<std::int32_t>(neighborZ)});
      }

      std::array<cr::CreativeVec3, 4U> corners{
          patch.corners[edge.topFirst], patch.corners[edge.topSecond],
          patch.corners[edge.topSecond], patch.corners[edge.topFirst]};
      if (neighbor != nullptr) {
        corners[2U] = neighbor->corners[edge.lowerSecond];
        corners[3U] = neighbor->corners[edge.lowerFirst];
      } else {
        corners[2U].y = bottomY;
        corners[3U].y = bottomY;
      }

      iggy3d::SceneRoomSurfacePatchItem item;
      item.role = cr::creativeTerrainMaterialRenderRole(patch.material);
      cr::CreativeVec3 center{};
      for (const cr::CreativeVec3 corner : corners) {
        center.x += corner.x;
        center.y += corner.y;
        center.z += corner.z;
      }
      center.x *= 0.25;
      center.y *= 0.25;
      center.z *= 0.25;
      const cr::CreativeCoreVec3Conversion convertedCenter =
          cr::creativeVec3ToCoreChecked(center);
      const cr::CreativeCoreVec3Conversion convertedTint =
          cr::creativeVec3ToCoreChecked(
              {patch.materialColor.x * 0.72,
               patch.materialColor.y * 0.72,
               patch.materialColor.z * 0.72});
      if (!convertedCenter.converted || !convertedTint.converted) {
        output.clear();
        return false;
      }
      item.center = convertedCenter.value;
      item.tint = convertedTint.value;
      item.hasTint = true;
      for (std::size_t cornerIndex = 0U; cornerIndex < corners.size();
           ++cornerIndex) {
        const cr::CreativeCoreVec3Conversion converted =
            cr::creativeVec3ToCoreChecked(corners[cornerIndex]);
        if (!converted.converted) {
          output.clear();
          return false;
        }
        item.corners[cornerIndex] = converted.value;
      }
      output.push_back(item);
    }
  }
  return true;
}

void refreshTerrainSurfacePlan(CreativeEditorSceneCache& cache,
                               const cr::CreativeDocument& document) {
  const cr::CreativeGridSettings grid = document.gridSettings();
  const cr::CreativeTerrainHeightField& heightField =
      document.terrainHeightField();
  const std::uint64_t heightHash =
      cr::hashCreativeTerrainHeightField(heightField);
  const std::uint64_t hardEdgeHash =
      cr::hashCreativeTerrainHardEdges(document.terrainHardEdges());
  const std::uint64_t materialHash =
      cr::hashCreativeTerrainMaterialField(document.terrainMaterialField());
  const bool geometryCurrent =
      cache.valid && cache.documentId == document.id() &&
      cache.terrainRevision == document.terrainField().revision() &&
      cache.terrainHeightRevision == heightField.revision() &&
      cache.terrainHeightHash == heightHash &&
      cache.terrainHeightCellCount == heightField.cellCount() &&
      cache.terrainHardEdgeHash == hardEdgeHash &&
      cache.terrainHardEdgeCount == document.terrainHardEdges().size() &&
      cr::creativeVec3ExactlyEqual(cache.terrainGridOrigin, grid.origin) &&
      cache.terrainGridCellSizeMeters == grid.cellSizeMeters;
  const bool materialCurrent =
      cache.valid && cache.documentId == document.id() &&
      cache.terrainMaterialRevision ==
          document.terrainMaterialField().revision() &&
      cache.terrainMaterialHash == materialHash;
  if (geometryCurrent && materialCurrent) {
    return;
  }
  if (geometryCurrent) {
    std::vector<cr::CreativeTerrainSurfacePatch> displayPatches =
        cache.terrainCollisionPatches;
    for (cr::CreativeTerrainSurfacePatch& patch : displayPatches) {
      const cr::CreativeTerrainMaterialWeights weights =
          document.terrainMaterialField().weightsAt(patch.coord);
      patch.material = cr::dominantCreativeTerrainMaterial(weights);
      patch.materialWeights = weights;
      patch.materialColor = cr::creativeTerrainMaterialRenderColor(weights);
    }
    cr::CreativeTerrainRenderPlan displayPlan;
    displayPlan.accepted = true;
    displayPlan.patches = std::move(displayPatches);
    static_cast<void>(convertTerrainSurfacePatches(
        displayPlan, grid.origin.y, cache.terrainSurfacePatches));
    cache.terrainMaterialRevision =
        document.terrainMaterialField().revision();
    cache.terrainMaterialHash = materialHash;
    ++cache.terrainMaterialBuildCount;
    return;
  }
  cr::CreativeTerrainSurfacePlan plan =
      cr::buildCreativeComposedTerrainSurfacePlan(
          document.terrainField(), heightField, document.terrainHardEdges());
  cache.terrainCuboids = plan.accepted ? plan.cuboids
                                      : std::vector<cr::CreativeVoxelCuboid>{};
  const cr::CreativeTerrainRenderPlan renderPlan =
      cr::buildCreativeTerrainRenderPlan(
          plan, document.terrainMaterialField(), grid.origin,
          grid.cellSizeMeters);
  if (convertTerrainSurfacePatches(renderPlan, grid.origin.y,
                                   cache.terrainSurfacePatches)) {
    cache.terrainCollisionPatches = renderPlan.patches;
  } else {
    cache.terrainCollisionPatches.clear();
  }
  cache.terrainRevision = document.terrainField().revision();
  cache.terrainHeightRevision = heightField.revision();
  cache.terrainHeightHash = heightHash;
  cache.terrainHeightCellCount = heightField.cellCount();
  cache.terrainHardEdgeHash = hardEdgeHash;
  cache.terrainHardEdgeCount = document.terrainHardEdges().size();
  cache.terrainMaterialRevision =
      document.terrainMaterialField().revision();
  cache.terrainMaterialHash = materialHash;
  cache.terrainGridOrigin = grid.origin;
  cache.terrainGridCellSizeMeters = grid.cellSizeMeters;
  cache.composedTerrainSurface = std::move(plan);
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
          document.terrainField(), document.terrainHeightField(),
          document.terrainHardEdges());
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
    if (convertTerrainSurfacePatches(renderPlan, grid.origin.y,
                                     terrainSurfacePatches)) {
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
    cache.composedTerrainSurface = {};
    cache.terrainCuboids.clear();
    cache.terrainCollisionPatches.clear();
    cache.terrainSurfacePatches.clear();
    cache.terrainRevision = 0;
    cache.terrainHeightRevision = 0;
    cache.terrainHeightHash = 0;
    cache.terrainHeightCellCount = 0;
    cache.terrainHardEdgeHash = 0;
    cache.terrainHardEdgeCount = 0;
    cache.terrainMaterialRevision = 0;
    cache.terrainMaterialHash = 0;
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
    const cr::CreativeTerrainHeightField& candidateHeight,
    std::uint64_t candidateHeightHash,
    const cr::CreativeTerrainMaterialField& candidateMaterial,
    std::uint64_t candidateMaterialHash,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog) {
  const cr::CreativeGridSettings grid = document.gridSettings();
  const bool sourceValid =
      sourceSceneCache.valid && sourceSceneCache.documentId == document.id() &&
      sourceSceneCache.documentRevision == document.revision();
  const bool candidateValid =
      candidateHeight.validateInvariants() &&
      candidateHeight.cellCount() > 0U &&
      candidateMaterial.validateInvariants();
  if (!sourceValid || !candidateValid) {
    invalidateCreativeEditorGeneratedTerrainPreview(cache);
    return false;
  }
  if (cache.valid && cache.documentId == document.id() &&
      cache.documentRevision == document.revision() &&
      cache.heightHash == candidateHeightHash &&
      cache.materialHash == candidateMaterialHash &&
      cache.sourceSceneRefreshCount == sourceSceneCache.refreshCount &&
      cr::creativeVec3ExactlyEqual(cache.terrainGridOrigin, grid.origin) &&
      cache.terrainGridCellSizeMeters == grid.cellSizeMeters) {
    return false;
  }

  const cr::CreativeTerrainSurfacePlan sourceSurface =
      cr::buildCreativeComposedTerrainSurfacePlan(
          document.terrainField(), document.terrainHeightField(),
          document.terrainHardEdges());
  cr::CreativeTerrainSurfacePlan composed =
      cr::replaceCreativeTerrainSurfaceRegion(
          sourceSurface, candidateHeight);
  if (!composed.accepted) {
    invalidateCreativeEditorGeneratedTerrainPreview(cache);
    return false;
  }
  const cr::CreativeTerrainRenderPlan render =
      cr::buildCreativeTerrainRenderPlan(
          composed, candidateMaterial, grid.origin,
          grid.cellSizeMeters);
  std::vector<iggy3d::SceneRoomSurfacePatchItem> surfacePatches;
  if (!convertTerrainSurfacePatches(render, grid.origin.y, surfacePatches)) {
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
  cache.materialHash = candidateMaterialHash;
  cache.sourceSceneRefreshCount = sourceSceneCache.refreshCount;
  cache.terrainGridOrigin = grid.origin;
  cache.terrainGridCellSizeMeters = grid.cellSizeMeters;
  ++cache.refreshCount;
  cache.valid = true;
  return true;
}

bool refreshCreativeEditorVolumeScenePreview(
    CreativeEditorVolumeScenePreviewCache& cache,
    const cr::CreativeDocument& stagedDocument,
    std::uint64_t operationRefreshCount,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog) {
  if (operationRefreshCount == 0U || !stagedDocument.isValid() ||
      stagedDocument.id() == cr::kInvalidDocumentId) {
    invalidateCreativeEditorVolumeScenePreview(cache);
    return false;
  }
  if (cache.valid &&
      cache.operationRefreshCount == operationRefreshCount) {
    return false;
  }

  invalidateCreativeEditorSceneCache(cache.scene);
  static_cast<void>(refreshCreativeEditorSceneCache(
      cache.scene, stagedDocument, assetCatalog));
  if (!cache.scene.valid || !cache.scene.preview.roomBake.receipt.accepted) {
    invalidateCreativeEditorVolumeScenePreview(cache);
    return false;
  }
  cache.operationRefreshCount = operationRefreshCount;
  cache.valid = true;
  return true;
}

void invalidateCreativeEditorSceneCache(
    CreativeEditorSceneCache& cache) noexcept {
  cache.valid = false;
  cache.documentId = iggy3d::creative::kInvalidDocumentId;
  cache.documentRevision = 0;
  cache.voxelChunkMeshes.clear();
  cache.composedTerrainSurface = {};
  cache.terrainCuboids.clear();
  cache.terrainCollisionPatches.clear();
  cache.terrainSurfacePatches.clear();
  cache.terrainRevision = 0;
  cache.terrainHeightRevision = 0;
  cache.terrainHeightHash = 0;
  cache.terrainHeightCellCount = 0;
  cache.terrainHardEdgeHash = 0;
  cache.terrainHardEdgeCount = 0;
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
  cache.materialHash = 0U;
  cache.sourceSceneRefreshCount = 0U;
  cache.terrainGridOrigin = {};
  cache.terrainGridCellSizeMeters = 0.0;
  cache.valid = false;
}

void invalidateCreativeEditorVolumeScenePreview(
    CreativeEditorVolumeScenePreviewCache& cache) noexcept {
  invalidateCreativeEditorSceneCache(cache.scene);
  cache.operationRefreshCount = 0U;
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
