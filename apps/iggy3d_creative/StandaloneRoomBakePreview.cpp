#include "StandaloneRoomBakePreview.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

#include <SDL3/SDL.h>

#include "EditorPreviewProxies.hpp"
#include "projection/scene/SceneItem.hpp"
#include "runtime/session/SessionState.hpp"

namespace iggy3d_creative_app {
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

}  // namespace

StandaloneRoomBakePreviewScene buildStandaloneRoomBakePreviewScene(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::ProductMapMakerGridSnapshot& gridSnapshot) {
  iggy3d::creative::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = &document;
  bakeRequest.roomId = "iggy3d_creative_preview";
  bakeRequest.sourceName = "apps/iggy3d_creative";
  bakeRequest.sourceSubset = "standalone_preview";

  StandaloneRoomBakePreviewScene preview;
  preview.roomBake =
      iggy3d::creative::buildRoomAssetFromCreativeDocument(bakeRequest);

  iggy3d::SessionState emptyRuntimeState;
  preview.scene = iggy3d::buildSceneProjection(emptyRuntimeState,
                                               &preview.roomBake.room);
  appendGridDotsToScene(gridSnapshot, preview.scene);
  preview.standalonePreviewMeshCount = appendStandalonePreviewProxiesToScene(
      document, preview.roomBake.staticMeshSources, preview.scene);
  if (!preview.scene.room.meshes.empty()) {
    preview.scene.room.staticMeshCount = preview.scene.room.meshes.size();
    preview.scene.room.loaded = true;
  }
  return preview;
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

}  // namespace iggy3d_creative_app
