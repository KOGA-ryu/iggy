#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/input/ControlProfile.hpp"
#include "app/iggy3d/creative/play/PlayerSpawn.hpp"
#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"
#include "app/iggy3d/creative/world/WorldLayoutDimensions.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"
#include "EditorPlacementFeedback.hpp"
#include "EditorPlacementClearance.hpp"
#include "EditorPlayerSpawnPreview.hpp"

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
  const CreativePlacementClearanceCache* placementClearanceCache = nullptr;
  const iggy3d::creative::CreativeDocument* terrainDocument = nullptr;
  const iggy3d::creative::CreativeTerrainSurfacePlan* terrainSurface = nullptr;
  std::uint64_t terrainSurfaceKey = 0U;
  const CreativePlayerSpawnPreviewGeometry* playerSpawnPreview = nullptr;
};

struct CreativeEditorOverlayFrame {
  std::vector<iggy3d::RenderUiRect> uiRects;
  std::vector<iggy3d::DebugHudGlyphQuad> glyphs;
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> combinedWireLines;
  std::size_t placementGridLineCount = 0;
  std::size_t placementGridDotCount = 0;
  std::size_t placementGridGuideLineCount = 0;
  std::size_t placementGridAnchorGuideLineCount = 0;
  std::size_t placementGridAnchorCandidateLineCount = 0;
  std::size_t placementGridContactGuideLineCount = 0;
  std::size_t placementGridTargetMarkerCount = 0;
  std::size_t placementInvalidTargetEdgeCount = 0;
  std::size_t placementBlockerEdgeCount = 0;
  bool placementGridClipped = false;
  std::size_t documentWireLineCount = 0;
  bool generatedScopeActive = false;
  std::size_t generatedScopeObjectCount = 0;
  std::size_t generatedScopeVisibleObjectCount = 0;
  std::size_t generatedScopeEdgeCount = 0;
  bool architectureScaleGuideActive = false;
  std::size_t architectureScaleGuideLineCount = 0;
  std::size_t worldLayoutRoofHandleEdgeCount = 0;
  iggy3d::creative::CreativeWorldLayoutBuildingDimensions
      architecturalDimensions{};
  std::size_t pointMarkerEdgeCount = 0;
  std::size_t lineMarkerEdgeCount = 0;
  std::size_t pathPointHandleEdgeCount = 0;
  std::size_t measurementEdgeCount = 0;
  std::size_t movingPlatformPathPreviewEdgeCount = 0;
  bool playerSpawnPreviewActive = false;
  bool playerSpawnPreviewAccepted = false;
  iggy3d::creative::CreativePlayerSpawnStatus playerSpawnPreviewStatus =
      iggy3d::creative::CreativePlayerSpawnStatus::NotRequested;
  std::string_view playerSpawnPreviewReasonCode =
      "creative_player_spawn_not_requested";
  std::size_t playerSpawnPreviewEdgeCount = 0;
  std::size_t playerSpawnPreviewLabelGlyphCount = 0;
  std::size_t structuralSpanEditEdgeCount = 0;
  std::size_t roomPlacementEdgeCount = 0;
  std::size_t ghostEdgeCount = 0;
  std::size_t materialBrushPivotEdgeCount = 0;
  std::size_t materialBrushGuideLineCount = 0;
  std::size_t materialBrushEdgeCount = 0;
  std::size_t connectedFillEdgeCount = 0;
  std::size_t surfaceExtrudeEdgeCount = 0;
  bool terrainSourceImpactActive = false;
  std::size_t terrainSourceImpactControlCount = 0;
  std::size_t terrainSourceImpactMaterialCellCount = 0;
  std::size_t terrainSourceImpactEdgeCount = 0;
  bool terrainSourceImpactClipped = false;
  std::size_t terrainContourEdgeCount = 0;
  std::size_t terrainEdgeCount = 0;
  std::size_t volumeEdgeCount = 0;
  std::size_t volumeExteriorEdgeCount = 0;
  std::size_t volumeInteriorEdgeCount = 0;
  std::size_t volumeChangedMemberEdgeCount = 0;
  std::size_t volumeUnchangedMemberEdgeCount = 0;
  std::size_t volumeProtectedMemberEdgeCount = 0;
  std::size_t volumeDependentSourceEdgeCount = 0;
  std::size_t volumeBlockedMemberEdgeCount = 0;
  std::size_t volumeHandleEdgeCount = 0;
  std::size_t patternEdgeCount = 0;
  std::size_t transformPreviewEdgeCount = 0;
  std::size_t assetReplacementEdgeCount = 0;
  std::size_t assetScatterEdgeCount = 0;
  std::size_t attachmentSocketMarkerEdgeCount = 0;
  std::size_t assetCollisionPreviewEdgeCount = 0;
  std::size_t placementFeedbackEdgeCount = 0;
  std::size_t logicLinkEdgeCount = 0;
  std::size_t logicLinkShaftCount = 0;
  std::size_t logicLinkArrowEdgeCount = 0;
  std::size_t logicLinkEndpointEdgeCount = 0;
  std::size_t logicLinkLabelGlyphCount = 0;
  std::size_t invalidLogicLinkCount = 0;
  CreativeEditorPlacementVisualizationReceipt placementVisualization{};
};

void buildAndAttachCreativeEditorOverlayFrame(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output);

CreativeEditorPlacementVisualizationReceipt
attachCreativeEditorPlacementPreviews(
    const CreativeEditorState& editor,
    bool captureMode,
    iggy3d::FrameInput& frame,
    const iggy3d::creative::CreativeDocument* document = nullptr,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog = nullptr,
    const CreativePlacementClearanceCache* clearanceCache = nullptr);

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
  std::uint64_t terrainHeightRevision = 0;
  std::uint64_t terrainHeightHash = 0;
  std::uint64_t terrainHeightCellCount = 0;
  std::uint64_t terrainHardEdgeHash = 0;
  std::uint64_t terrainHardEdgeCount = 0;
  std::uint64_t terrainMaterialRevision = 0;
  std::uint64_t terrainMaterialHash = 0;
  std::uint64_t terrainSurfaceBuildCount = 0;
  std::uint64_t terrainMaterialBuildCount = 0;
  iggy3d::creative::CreativeVec3 terrainGridOrigin{};
  double terrainGridCellSizeMeters = 0.0;
  std::vector<CreativeEditorVoxelChunkMeshCache> voxelChunkMeshes;
  iggy3d::creative::CreativeTerrainSurfacePlan composedTerrainSurface;
  std::vector<iggy3d::creative::CreativeVoxelCuboid> terrainCuboids;
  std::vector<iggy3d::creative::CreativeTerrainSurfacePatch>
      terrainCollisionPatches;
  std::vector<iggy3d::SceneRoomSurfacePatchItem> terrainSurfacePatches;
  CreativePlacementClearanceCache placementClearance;
  bool valid = false;
};

// A complete transient scene built only when the source document, grid, or
// generated height hash changes. The generated region replaces source terrain
// before meshing, so preview frames cannot contain overlapping terrain owners.
struct CreativeEditorGeneratedTerrainPreviewCache {
  StandaloneRoomBakePreviewScene preview;
  iggy3d::creative::CreativeTerrainSurfacePlan composedSurface;
  std::vector<iggy3d::creative::CreativeTerrainSurfacePatch>
      terrainCollisionPatches;
  std::vector<iggy3d::SceneRoomSurfacePatchItem> terrainSurfacePatches;
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  std::uint64_t documentRevision = 0U;
  std::uint64_t heightHash = 0U;
  std::uint64_t materialHash = 0U;
  std::uint64_t sourceSceneRefreshCount = 0U;
  std::uint64_t refreshCount = 0U;
  iggy3d::creative::CreativeVec3 terrainGridOrigin{};
  double terrainGridCellSizeMeters = 0.0;
  bool valid = false;
};

// Renders an exact staged volume operation through the same room-bake path as
// committed document geometry. The operation refresh key is required because
// distinct staged previews can share the same document id and revision.
struct CreativeEditorVolumeScenePreviewCache {
  CreativeEditorSceneCache scene;
  std::uint64_t operationRefreshCount = 0U;
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

[[nodiscard]] bool refreshCreativeEditorGeneratedTerrainPreview(
    CreativeEditorGeneratedTerrainPreviewCache& cache,
    const CreativeEditorSceneCache& sourceSceneCache,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::creative::CreativeTerrainHeightField& candidateHeight,
    std::uint64_t candidateHeightHash,
    const iggy3d::creative::CreativeTerrainMaterialField& candidateMaterial,
    std::uint64_t candidateMaterialHash,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog = nullptr);
void invalidateCreativeEditorGeneratedTerrainPreview(
    CreativeEditorGeneratedTerrainPreviewCache& cache) noexcept;

[[nodiscard]] bool refreshCreativeEditorVolumeScenePreview(
    CreativeEditorVolumeScenePreviewCache& cache,
    const iggy3d::creative::CreativeDocument& stagedDocument,
    std::uint64_t operationRefreshCount,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog = nullptr);
void invalidateCreativeEditorVolumeScenePreview(
    CreativeEditorVolumeScenePreviewCache& cache) noexcept;

void logStandaloneRoomBakeFinal(
    const StandaloneRoomBakePreviewScene& preview);

}  // namespace iggy3d_creative_app
