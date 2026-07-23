#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/TerrainContours.hpp"
#include "EditorTerrainGeneration.hpp"

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

inline constexpr std::size_t
    kCreativeEditorWorldLayoutTopographyCellCapacity =
        cr::kCreativeTerrainRenderPatchCapacity;

enum class CreativeEditorWorldLayoutTopographyStatus : std::uint8_t {
  NotRequested,
  InvalidDocument,
  InvalidRequest,
  SurfaceRejected,
  CapacityExceeded,
  Empty,
  Ready,
};

struct CreativeEditorWorldLayoutTopographyPlan {
  bool requested = false;
  bool accepted = false;
  CreativeEditorWorldLayoutTopographyStatus status =
      CreativeEditorWorldLayoutTopographyStatus::NotRequested;
  cr::CreativeDocumentId documentId = cr::kInvalidDocumentId;
  std::uint64_t documentRevision = 0U;
  std::uint64_t terrainRevision = 0U;
  std::uint64_t terrainHeightRevision = 0U;
  std::uint16_t minimumHeightCells = 0U;
  std::uint16_t maximumHeightCells = 0U;
  cr::CreativeTerrainAnalysisPlan analysis;
  std::string_view reasonCode =
      "creative_editor_world_layout_topography_not_requested";
};

struct CreativeEditorWorldLayoutTopographySample {
  bool present = false;
  cr::CreativeTerrainCoord2 coord{};
  std::uint16_t heightCells = 0U;
  double slopeXCellsPerCell = 0.0;
  double slopeZCellsPerCell = 0.0;
  double slopeMagnitude = 0.0;
  double slopeDegrees = 0.0;
  std::uint8_t neighborSampleCount = 0U;
  cr::CreativeTerrainSlopeBand slopeBand =
      cr::CreativeTerrainSlopeBand::Unavailable;
  cr::CreativeTerrainCutFillKind cutFill =
      cr::CreativeTerrainCutFillKind::Unchanged;
  std::int16_t deltaCells = 0;
};

struct CreativeEditorWorldLayoutTerrainRegionRecipePlan {
  bool requested = false;
  bool accepted = false;
  cr::CreativeTerrainRegionRecipe recipe;
  std::string_view reasonCode =
      "creative_editor_world_layout_terrain_region_not_requested";
};

struct CreativeEditorWorldLayoutTerrainAnalysisEditPlan {
  bool requested = false;
  bool accepted = false;
  cr::CreativeTerrainAnalysisHit hit;
  cr::CreativeTerrainRegionRecipe recipe;
  std::string_view reasonCode =
      "creative_editor_world_layout_terrain_analysis_edit_not_requested";
};

enum class CreativeEditorWorldLayoutTerrainRegionHandle : std::uint8_t {
  None,
  Body,
  MinimumX,
  MaximumX,
  MinimumZ,
  MaximumZ,
  MinimumXMinimumZ,
  MaximumXMinimumZ,
  MinimumXMaximumZ,
  MaximumXMaximumZ,
  Count,
};

struct CreativeEditorWorldLayoutTerrainRegionManipulation {
  bool active = false;
  bool changed = false;
  CreativeEditorWorldLayoutTerrainRegionHandle handle =
      CreativeEditorWorldLayoutTerrainRegionHandle::None;
  cr::CreativeTerrainCoord2 pointerStart{};
  cr::CreativeTerrainHeightFieldBounds boundsStart{};
};

[[nodiscard]] constexpr cr::CreativeTerrainRegionRecipe
makeCreativeEditorWorldLayoutTerrainRegionRecipe() noexcept {
  cr::CreativeTerrainRegionRecipe recipe;
  recipe.mode = cr::CreativeTerrainRegionMode::Flatten;
  recipe.targetHeightCells = 8U;
  return recipe;
}

struct CreativeEditorWorldLayoutTerrainRegionState {
  bool editingEnabled = false;
  bool selecting = false;
  bool regionValid = false;
  bool ownsPreview = false;
  cr::CreativeTerrainRegionRecipe recipe =
      makeCreativeEditorWorldLayoutTerrainRegionRecipe();
  cr::CreativeTerrainCoord2 anchor{};
  cr::CreativeTerrainCoord2 cursor{};
  CreativeEditorWorldLayoutTerrainRegionManipulation manipulation;
  cr::CreativeTerrainOperationId editingOperationId =
      cr::kInvalidCreativeTerrainOperationId;
  std::string statusMessage = "terrain region ready";
};

struct CreativeEditorWorldLayoutTopographyState {
  bool visible = true;
  bool elevationBandsVisible = true;
  bool slopeBandsVisible = false;
  bool cutFillVisible = true;
  bool contourLabelsVisible = true;
  std::uint16_t intervalCells = 2U;
  std::uint16_t majorEvery = 5U;
  CreativeEditorWorldLayoutTerrainRegionState region;

  bool cacheValid = false;
  bool cachedSourceOverride = false;
  std::uint64_t cachedSourceKey = 0U;
  cr::CreativeDocumentId cachedDocumentId = cr::kInvalidDocumentId;
  std::uint64_t cachedDocumentRevision = 0U;
  std::uint64_t cachedTerrainRevision = 0U;
  std::uint64_t cachedTerrainHeightRevision = 0U;
  std::uint16_t cachedIntervalCells = 0U;
  std::uint16_t cachedMajorEvery = 0U;
  std::uint64_t buildCount = 0U;
  CreativeEditorWorldLayoutTopographyPlan plan;
};

[[nodiscard]] std::string_view toString(
    CreativeEditorWorldLayoutTopographyStatus status) noexcept;

[[nodiscard]] bool beginCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    double xCells,
    double zCells) noexcept;
[[nodiscard]] bool updateCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    double xCells,
    double zCells) noexcept;
[[nodiscard]] bool finishCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    double xCells,
    double zCells) noexcept;
void clearCreativeEditorWorldLayoutTerrainRegionSelection(
    CreativeEditorWorldLayoutTerrainRegionState& state) noexcept;

[[nodiscard]] bool setCreativeEditorWorldLayoutTerrainRegionBounds(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    cr::CreativeTerrainHeightFieldBounds bounds) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutTerrainRegionHandle
hitCreativeEditorWorldLayoutTerrainRegionHandle(
    const CreativeEditorWorldLayoutTerrainRegionState& state,
    double xCells,
    double zCells,
    double toleranceCells) noexcept;
[[nodiscard]] bool beginCreativeEditorWorldLayoutTerrainRegionManipulation(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    CreativeEditorWorldLayoutTerrainRegionHandle handle,
    double xCells,
    double zCells) noexcept;
[[nodiscard]] bool updateCreativeEditorWorldLayoutTerrainRegionManipulation(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    double xCells,
    double zCells) noexcept;
[[nodiscard]] bool finishCreativeEditorWorldLayoutTerrainRegionManipulation(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    double xCells,
    double zCells) noexcept;
[[nodiscard]] bool cancelCreativeEditorWorldLayoutTerrainRegionManipulation(
    CreativeEditorWorldLayoutTerrainRegionState& state) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutTerrainRegionRecipePlan
planCreativeEditorWorldLayoutTerrainRegion(
    const CreativeEditorWorldLayoutTerrainRegionState& state) noexcept;

[[nodiscard]] bool selectCreativeEditorWorldLayoutTerrainRegionOperation(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    CreativeEditorTerrainGenerationState& terrainGeneration,
    const cr::CreativeDocument& document,
    cr::CreativeTerrainOperationId operationId);

[[nodiscard]] CreativeEditorTerrainGenerationPreviewReceipt
previewCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    CreativeEditorTerrainGenerationState& terrainGeneration,
    const cr::CreativeDocument& document);

[[nodiscard]] CreativeEditorTerrainGenerationApplyReceipt
applyCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    CreativeEditorTerrainGenerationState& terrainGeneration,
    cr::CreativeAppState& appState);

[[nodiscard]] bool cancelCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    CreativeEditorTerrainGenerationState& terrainGeneration,
    std::string_view reason = "Terrain region preview canceled");

[[nodiscard]] bool synchronizeCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    const CreativeEditorTerrainGenerationState& terrainGeneration,
    const cr::CreativeDocument& document);

[[nodiscard]] CreativeEditorWorldLayoutTopographyPlan
buildCreativeEditorWorldLayoutTopography(
    const cr::CreativeDocument& document,
    std::uint16_t intervalCells = 2U,
    std::uint16_t majorEvery = 5U,
    const cr::CreativeTerrainHeightField* heightFieldOverride = nullptr);

// Returns true only when the cached plan was rebuilt. Hidden display state does
// no terrain composition work.
[[nodiscard]] bool refreshCreativeEditorWorldLayoutTopography(
    CreativeEditorWorldLayoutTopographyState& state,
    const cr::CreativeDocument& document,
    bool sourceOverride = false,
    std::uint64_t sourceKey = 0U,
    const cr::CreativeTerrainHeightField* heightFieldOverride = nullptr);

[[nodiscard]] CreativeEditorWorldLayoutTopographySample
sampleCreativeEditorWorldLayoutTopography(
    const CreativeEditorWorldLayoutTopographyPlan& plan,
    double xCells,
    double zCells) noexcept;

// Resolves a display-space contour or height handle into the same durable
// Region recipe edited by both Creative frontends. No render primitive is ever
// mutated directly.
[[nodiscard]] CreativeEditorWorldLayoutTerrainAnalysisEditPlan
planCreativeEditorWorldLayoutTerrainAnalysisEdit(
    const CreativeEditorWorldLayoutTopographyPlan& plan,
    double xCells,
    double zCells,
    double contourToleranceCells,
    cr::CreativeTerrainAnalysisHitMode mode) noexcept;

// Adopts a headless analysis selection as a new Region operation draft. This
// is the sole state transition shared by plan-view contour/height gestures.
[[nodiscard]] bool selectCreativeEditorWorldLayoutTerrainAnalysisEdit(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    const CreativeEditorWorldLayoutTerrainAnalysisEditPlan& edit) noexcept;

}  // namespace iggy3d_creative_app
