#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/input/ControlProfile.hpp"
#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorState;
struct CreativeEditorSelectionFrame;
struct CreativeEditorGizmoFrame;

struct CreativeEditorOverlayFrameRequest {
  iggy3d::creative::CreativeAppState& appState;
  CreativeEditorState& editor;
  const CreativeEditorSelectionFrame& selection;
  const CreativeEditorGizmoFrame& gizmoFrame;
  iggy3d::FrameInput& frame;
  const iggy3d::creative::CreativeSpatialProjectionRequest&
      wireProjectionRequest;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
  float gizmoThickness = 0.0F;
  bool captureMode = false;
  iggy3d::creative::CreativeInputContext inputContext =
      iggy3d::creative::CreativeInputContext::EditorViewport;
  iggy3d::creative::CreativeControlDevice activeControlDevice =
      iggy3d::creative::CreativeControlDevice::KeyboardMouse;
  const iggy3d::StaticMeshAssetCatalog* assetCatalog = nullptr;
};

struct CreativeEditorOverlayFrame {
  std::vector<iggy3d::RenderUiRect> uiRects;
  std::vector<iggy3d::DebugHudGlyphQuad> glyphs;
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> combinedWireLines;
  std::size_t placementGridLineCount = 0;
  std::size_t placementGridDotCount = 0;
  bool placementGridClipped = false;
  std::size_t documentWireLineCount = 0;
  std::size_t pointMarkerEdgeCount = 0;
  std::size_t lineMarkerEdgeCount = 0;
  std::size_t pathPointHandleEdgeCount = 0;
  std::size_t movingPlatformPathPreviewEdgeCount = 0;
  std::size_t structuralSpanEditEdgeCount = 0;
  std::size_t ghostEdgeCount = 0;
  std::size_t materialBrushPivotEdgeCount = 0;
  std::size_t materialBrushGuideLineCount = 0;
  std::size_t materialBrushEdgeCount = 0;
  std::size_t connectedFillEdgeCount = 0;
  std::size_t surfaceExtrudeEdgeCount = 0;
  std::size_t terrainEdgeCount = 0;
  std::size_t volumeEdgeCount = 0;
  std::size_t patternEdgeCount = 0;
  std::size_t transformPreviewEdgeCount = 0;
  std::size_t assetReplacementEdgeCount = 0;
  std::size_t assetScatterEdgeCount = 0;
  std::size_t attachmentSocketMarkerEdgeCount = 0;
  std::size_t placementFeedbackEdgeCount = 0;
  std::size_t logicLinkEdgeCount = 0;
  std::size_t logicLinkShaftCount = 0;
  std::size_t logicLinkArrowEdgeCount = 0;
  std::size_t logicLinkEndpointEdgeCount = 0;
  std::size_t logicLinkLabelGlyphCount = 0;
  std::size_t invalidLogicLinkCount = 0;
};

void buildAndAttachCreativeEditorOverlayFrame(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output);

void attachCreativeEditorPlacementPreviews(
    const CreativeEditorState& editor,
    bool captureMode,
    iggy3d::FrameInput& frame,
    const iggy3d::creative::CreativeDocument* document = nullptr,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog = nullptr);

struct StandaloneRoomBakePreviewScene {
  iggy3d::SceneProjectionResult scene;
  iggy3d::creative::CreativeRoomBakeResult roomBake;
  std::size_t standalonePreviewMeshCount = 0;
};

struct CreativeEditorVoxelChunkMeshCache {
  iggy3d::creative::CreativeVoxelChunkCoord coord{};
  std::uint64_t revision = 0;
  std::vector<iggy3d::creative::CreativeVoxelCuboid> cuboids;
};

struct CreativeEditorSceneCache {
  StandaloneRoomBakePreviewScene preview;
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  std::uint64_t documentRevision = 0;
  std::uint64_t refreshCount = 0;
  std::uint64_t voxelChunkMeshBuildCount = 0;
  std::uint64_t terrainRevision = 0;
  std::uint64_t terrainMaterialRevision = 0;
  std::uint64_t terrainSurfaceBuildCount = 0;
  std::uint64_t terrainMaterialBuildCount = 0;
  iggy3d::creative::CreativeVec3 terrainGridOrigin{};
  double terrainGridCellSizeMeters = 0.0;
  std::vector<CreativeEditorVoxelChunkMeshCache> voxelChunkMeshes;
  std::vector<iggy3d::creative::CreativeVoxelCuboid> terrainCuboids;
  std::vector<iggy3d::creative::CreativeTerrainSurfacePatch>
      terrainCollisionPatches;
  std::vector<iggy3d::SceneRoomSurfacePatchItem> terrainSurfacePatches;
  bool valid = false;
};

[[nodiscard]] StandaloneRoomBakePreviewScene buildStandaloneRoomBakePreviewScene(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog = nullptr);

[[nodiscard]] bool refreshCreativeEditorSceneCache(
    CreativeEditorSceneCache& cache,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog = nullptr);
void invalidateCreativeEditorSceneCache(
    CreativeEditorSceneCache& cache) noexcept;

void logStandaloneRoomBakeFinal(
    const StandaloneRoomBakePreviewScene& preview);

}  // namespace iggy3d_creative_app
