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
  std::vector<cr::CreativeTerrainColumn> columns;
  cr::CreativeTerrainContourPlan contours;
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
};

enum class CreativeEditorWorldLayoutTerrainRegionOperation : std::uint8_t {
  Flatten,
  Raise,
  Lower,
  Smooth,
  Noise,
  Count,
};

struct CreativeEditorWorldLayoutTerrainRegionRecipePlan {
  bool requested = false;
  bool accepted = false;
  cr::CreativeTerrainGeneratorRecipe generation;
  cr::CreativeTerrainCompositionRecipe composition;
  std::string_view reasonCode =
      "creative_editor_world_layout_terrain_region_not_requested";
};

struct CreativeEditorWorldLayoutTerrainRegionState {
  bool editingEnabled = false;
  bool selecting = false;
  bool regionValid = false;
  bool ownsPreview = false;
  CreativeEditorWorldLayoutTerrainRegionOperation operation =
      CreativeEditorWorldLayoutTerrainRegionOperation::Flatten;
  cr::CreativeTerrainCompositionMask mask =
      cr::CreativeTerrainCompositionMask::Rectangle;
  cr::CreativeTerrainCoord2 anchor{};
  cr::CreativeTerrainCoord2 cursor{};
  cr::CreativeTerrainHeightFieldBounds bounds{};
  std::uint16_t targetHeightCells = 8U;
  std::uint16_t noiseReliefCells = 4U;
  double noiseScaleCells = 12.0;
  std::uint16_t featherCells = 0U;
  std::uint64_t seed = 1U;
  std::string statusMessage = "terrain region ready";
};

struct CreativeEditorWorldLayoutTopographyState {
  bool visible = true;
  bool elevationBandsVisible = true;
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
[[nodiscard]] std::string_view toString(
    CreativeEditorWorldLayoutTerrainRegionOperation operation) noexcept;

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

[[nodiscard]] CreativeEditorWorldLayoutTerrainRegionRecipePlan
planCreativeEditorWorldLayoutTerrainRegion(
    const CreativeEditorWorldLayoutTerrainRegionState& state) noexcept;

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

}  // namespace iggy3d_creative_app
