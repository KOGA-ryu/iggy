#include "EditorWorldLayoutTopography.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <span>
#include <string>
#include <utility>

#include "app/iggy3d/creative/document/TerrainHeightField.hpp"

namespace iggy3d_creative_app {
namespace {

struct TerrainRegionOperationSpec {
  cr::CreativeTerrainCompositionMode composition =
      cr::CreativeTerrainCompositionMode::Replace;
  bool usesNoise = false;
};

constexpr std::array<TerrainRegionOperationSpec, 5U>
    kTerrainRegionOperationSpecs{{
        {cr::CreativeTerrainCompositionMode::Replace, false},
        {cr::CreativeTerrainCompositionMode::Raise, false},
        {cr::CreativeTerrainCompositionMode::Lower, false},
        {cr::CreativeTerrainCompositionMode::Smooth, false},
        {cr::CreativeTerrainCompositionMode::Replace, true},
    }};

static_assert(kTerrainRegionOperationSpecs.size() ==
              static_cast<std::size_t>(
                  CreativeEditorWorldLayoutTerrainRegionOperation::Count));

[[nodiscard]] const cr::CreativeTerrainHeightField& topographyHeightField(
    const cr::CreativeDocument& document,
    const cr::CreativeTerrainHeightField* heightFieldOverride) noexcept {
  return heightFieldOverride == nullptr ? document.terrainHeightField()
                                        : *heightFieldOverride;
}

[[nodiscard]] bool terrainCellFromPoint(double xCells,
                                        double zCells,
                                        cr::CreativeTerrainCoord2& output)
    noexcept {
  if (!std::isfinite(xCells) || !std::isfinite(zCells)) {
    return false;
  }
  const double x = std::floor(xCells);
  const double z = std::floor(zCells);
  if (x < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      x > static_cast<double>(std::numeric_limits<std::int32_t>::max()) ||
      z < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      z > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
    return false;
  }
  output = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(z)};
  return true;
}

[[nodiscard]] bool terrainRegionBounds(
    cr::CreativeTerrainCoord2 first,
    cr::CreativeTerrainCoord2 second,
    cr::CreativeTerrainHeightFieldBounds& output) noexcept {
  const std::int64_t minimumX = std::min(first.x, second.x);
  const std::int64_t minimumZ = std::min(first.z, second.z);
  const std::int64_t width =
      std::llabs(static_cast<std::int64_t>(second.x) - first.x) + 1;
  const std::int64_t depth =
      std::llabs(static_cast<std::int64_t>(second.z) - first.z) + 1;
  if (width > std::numeric_limits<std::uint16_t>::max() ||
      depth > std::numeric_limits<std::uint16_t>::max() ||
      static_cast<std::uint64_t>(width) *
              static_cast<std::uint64_t>(depth) >
          cr::kCreativeTerrainHeightFieldCellCapacity) {
    return false;
  }
  output = {{static_cast<std::int32_t>(minimumX),
             static_cast<std::int32_t>(minimumZ)},
            static_cast<std::uint16_t>(width),
            static_cast<std::uint16_t>(depth)};
  return cr::isValidCreativeTerrainHeightFieldBounds(output);
}

[[nodiscard]] bool coordLess(cr::CreativeTerrainCoord2 lhs,
                             cr::CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z != rhs.z ? lhs.z < rhs.z : lhs.x < rhs.x;
}

[[nodiscard]] const cr::CreativeTerrainColumn* findColumn(
    std::span<const cr::CreativeTerrainColumn> columns,
    cr::CreativeTerrainCoord2 coord) noexcept {
  const auto found = std::lower_bound(
      columns.begin(), columns.end(), coord,
      [](const cr::CreativeTerrainColumn& column,
         cr::CreativeTerrainCoord2 value) {
        return coordLess(column.coord, value);
      });
  return found != columns.end() && found->coord == coord ? &*found : nullptr;
}

[[nodiscard]] bool validRequest(std::uint16_t intervalCells,
                                std::uint16_t majorEvery) noexcept {
  return intervalCells > 0U &&
         intervalCells <= cr::kCreativeTerrainContourMaximumIntervalCells &&
         majorEvery > 0U &&
         majorEvery <= cr::kCreativeTerrainContourMaximumMajorEvery;
}

[[nodiscard]] bool cacheMatches(
    const CreativeEditorWorldLayoutTopographyState& state,
    const cr::CreativeDocument& document,
    bool sourceOverride,
    std::uint64_t sourceKey,
    const cr::CreativeTerrainHeightField* heightFieldOverride) noexcept {
  const cr::CreativeTerrainHeightField& heightField =
      topographyHeightField(document, heightFieldOverride);
  return state.cacheValid &&
         state.cachedSourceOverride == sourceOverride &&
         state.cachedSourceKey == sourceKey &&
         state.cachedDocumentId == document.id() &&
         state.cachedDocumentRevision == document.revision() &&
         state.cachedTerrainRevision == document.terrainField().revision() &&
         state.cachedTerrainHeightRevision ==
             heightField.revision() &&
         state.cachedIntervalCells == state.intervalCells &&
         state.cachedMajorEvery == state.majorEvery;
}

[[nodiscard]] bool coordinateFromPoint(double value,
                                       std::int32_t& output) noexcept {
  if (!std::isfinite(value)) {
    return false;
  }
  const double floored = std::floor(value);
  if (floored < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      floored > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
    return false;
  }
  output = static_cast<std::int32_t>(floored);
  return true;
}

struct AxisSlope {
  double value = 0.0;
  std::uint8_t sampleCount = 0U;
};

[[nodiscard]] const cr::CreativeTerrainColumn* findOffsetColumn(
    std::span<const cr::CreativeTerrainColumn> columns,
    cr::CreativeTerrainCoord2 coord,
    std::int32_t deltaX,
    std::int32_t deltaZ) noexcept {
  const std::int64_t x = static_cast<std::int64_t>(coord.x) + deltaX;
  const std::int64_t z = static_cast<std::int64_t>(coord.z) + deltaZ;
  if (x < std::numeric_limits<std::int32_t>::min() ||
      x > std::numeric_limits<std::int32_t>::max() ||
      z < std::numeric_limits<std::int32_t>::min() ||
      z > std::numeric_limits<std::int32_t>::max()) {
    return nullptr;
  }
  return findColumn(columns,
                    {static_cast<std::int32_t>(x),
                     static_cast<std::int32_t>(z)});
}

[[nodiscard]] AxisSlope sampleAxisSlope(
    std::span<const cr::CreativeTerrainColumn> columns,
    cr::CreativeTerrainCoord2 coord,
    std::uint16_t centerHeight,
    std::int32_t deltaX,
    std::int32_t deltaZ) noexcept {
  const cr::CreativeTerrainColumn* negative =
      findOffsetColumn(columns, coord, -deltaX, -deltaZ);
  const cr::CreativeTerrainColumn* positive =
      findOffsetColumn(columns, coord, deltaX, deltaZ);
  if (negative != nullptr && positive != nullptr) {
    return {(static_cast<double>(positive->heightCells) -
             static_cast<double>(negative->heightCells)) /
                2.0,
            2U};
  }
  if (positive != nullptr) {
    return {static_cast<double>(positive->heightCells) - centerHeight, 1U};
  }
  if (negative != nullptr) {
    return {static_cast<double>(centerHeight) - negative->heightCells, 1U};
  }
  return {};
}

}  // namespace

std::string_view toString(
    CreativeEditorWorldLayoutTerrainRegionOperation operation) noexcept {
  switch (operation) {
    case CreativeEditorWorldLayoutTerrainRegionOperation::Flatten:
      return "Flatten";
    case CreativeEditorWorldLayoutTerrainRegionOperation::Raise:
      return "Raise";
    case CreativeEditorWorldLayoutTerrainRegionOperation::Lower:
      return "Lower";
    case CreativeEditorWorldLayoutTerrainRegionOperation::Smooth:
      return "Smooth";
    case CreativeEditorWorldLayoutTerrainRegionOperation::Noise:
      return "Noise";
    case CreativeEditorWorldLayoutTerrainRegionOperation::Count:
      break;
  }
  return "Invalid";
}

bool beginCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    double xCells,
    double zCells) noexcept {
  cr::CreativeTerrainCoord2 coord;
  if (!state.editingEnabled || !terrainCellFromPoint(xCells, zCells, coord)) {
    return false;
  }
  state.selecting = true;
  state.regionValid = terrainRegionBounds(coord, coord, state.bounds);
  state.anchor = coord;
  state.cursor = coord;
  return state.regionValid;
}

bool updateCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    double xCells,
    double zCells) noexcept {
  cr::CreativeTerrainCoord2 coord;
  if (!state.selecting || !terrainCellFromPoint(xCells, zCells, coord)) {
    return false;
  }
  state.cursor = coord;
  state.regionValid = terrainRegionBounds(state.anchor, state.cursor,
                                          state.bounds);
  return state.regionValid;
}

bool finishCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    double xCells,
    double zCells) noexcept {
  if (!state.selecting) {
    return false;
  }
  static_cast<void>(updateCreativeEditorWorldLayoutTerrainRegion(
      state, xCells, zCells));
  state.selecting = false;
  return state.regionValid;
}

void clearCreativeEditorWorldLayoutTerrainRegionSelection(
    CreativeEditorWorldLayoutTerrainRegionState& state) noexcept {
  state.selecting = false;
  state.regionValid = false;
  state.anchor = {};
  state.cursor = {};
  state.bounds = {};
}

CreativeEditorWorldLayoutTerrainRegionRecipePlan
planCreativeEditorWorldLayoutTerrainRegion(
    const CreativeEditorWorldLayoutTerrainRegionState& state) noexcept {
  CreativeEditorWorldLayoutTerrainRegionRecipePlan plan;
  plan.requested = true;
  if (!state.editingEnabled || !state.regionValid ||
      state.operation >=
          CreativeEditorWorldLayoutTerrainRegionOperation::Count ||
      state.mask >= cr::CreativeTerrainCompositionMask::Count ||
      state.targetHeightCells < cr::kCreativeTerrainMinimumHeightCells ||
      state.targetHeightCells > cr::kCreativeTerrainMaximumHeightCells ||
      state.noiseReliefCells > cr::kCreativeTerrainMaximumHeightCells ||
      !std::isfinite(state.noiseScaleCells) ||
      state.noiseScaleCells <
          cr::kCreativeTerrainGeneratorMinimumHorizontalScaleCells ||
      state.noiseScaleCells >
          cr::kCreativeTerrainGeneratorMaximumHorizontalScaleCells ||
      state.featherCells >
          cr::kCreativeTerrainCompositionMaximumFeatherCells ||
      !cr::isValidCreativeTerrainHeightFieldBounds(state.bounds)) {
    plan.reasonCode =
        "creative_editor_world_layout_terrain_region_request_invalid";
    return plan;
  }

  const TerrainRegionOperationSpec& spec = kTerrainRegionOperationSpecs[
      static_cast<std::size_t>(state.operation)];
  plan.generation = makeDefaultCreativeEditorTerrainGeneratorRecipe();
  plan.generation.bounds = state.bounds;
  plan.generation.seed = state.seed;
  plan.generation.baseHeightCells = state.targetHeightCells;
  plan.generation.reliefCells = spec.usesNoise ? state.noiseReliefCells : 0U;
  plan.generation.horizontalScaleCells = state.noiseScaleCells;
  plan.composition.mask = state.mask;
  plan.composition.mode = spec.composition;
  plan.composition.featherCells = state.featherCells;
  if (!cr::isValidCreativeTerrainGeneratorRecipe(plan.generation) ||
      !cr::isValidCreativeTerrainCompositionRecipe(plan.composition)) {
    plan.reasonCode =
        "creative_editor_world_layout_terrain_region_recipe_invalid";
    return plan;
  }
  plan.accepted = true;
  plan.reasonCode = "creative_editor_world_layout_terrain_region_ready";
  return plan;
}

CreativeEditorTerrainGenerationPreviewReceipt
previewCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    CreativeEditorTerrainGenerationState& terrainGeneration,
    const cr::CreativeDocument& document) {
  CreativeEditorTerrainGenerationPreviewReceipt receipt;
  receipt.requested = true;
  const CreativeEditorWorldLayoutTerrainRegionRecipePlan plan =
      planCreativeEditorWorldLayoutTerrainRegion(state);
  if (!plan.accepted) {
    if (state.ownsPreview) {
      static_cast<void>(cancelCreativeEditorTerrainGeneration(
          terrainGeneration, "Terrain region preview rejected"));
      state.ownsPreview = false;
    }
    receipt.reasonCode = plan.reasonCode;
    state.statusMessage = "Terrain region preview rejected";
    return receipt;
  }
  if (terrainGeneration.previewActive && !state.ownsPreview) {
    receipt.reasonCode =
        "creative_editor_world_layout_terrain_region_preview_owned_elsewhere";
    state.statusMessage = "Another terrain preview is active";
    return receipt;
  }

  static_cast<void>(beginNewCreativeEditorTerrainOperation(terrainGeneration));
  terrainGeneration.recipe = plan.generation;
  terrainGeneration.compositionRecipe = plan.composition;
  terrainGeneration.draftDirty = true;
  receipt = previewCreativeEditorTerrainGeneration(
      terrainGeneration, document, false);
  state.ownsPreview = receipt.accepted;
  if (!receipt.accepted) {
    static_cast<void>(cancelCreativeEditorTerrainGeneration(
        terrainGeneration, "Terrain region preview rejected"));
  }
  state.statusMessage = receipt.accepted ? "Terrain region preview ready"
                                         : "Terrain region preview rejected";
  return receipt;
}

CreativeEditorTerrainGenerationApplyReceipt
applyCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    CreativeEditorTerrainGenerationState& terrainGeneration,
    cr::CreativeAppState& appState) {
  CreativeEditorTerrainGenerationApplyReceipt receipt;
  receipt.requested = true;
  if (!state.ownsPreview) {
    receipt.reasonCode =
        "creative_editor_world_layout_terrain_region_preview_inactive";
    state.statusMessage = "No terrain region preview to apply";
    return receipt;
  }
  receipt = applyCreativeEditorTerrainGeneration(appState, terrainGeneration);
  if (receipt.accepted) {
    state.ownsPreview = false;
    clearCreativeEditorWorldLayoutTerrainRegionSelection(state);
  }
  state.statusMessage = receipt.accepted ? "Terrain region applied"
                                         : "Terrain region apply failed";
  return receipt;
}

bool cancelCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    CreativeEditorTerrainGenerationState& terrainGeneration,
    std::string_view reason) {
  const bool changed = state.selecting || state.regionValid ||
                       state.ownsPreview;
  if (state.ownsPreview) {
    static_cast<void>(cancelCreativeEditorTerrainGeneration(
        terrainGeneration, reason));
  }
  state.ownsPreview = false;
  clearCreativeEditorWorldLayoutTerrainRegionSelection(state);
  state.statusMessage = reason.empty() ? "Terrain region canceled"
                                       : std::string(reason);
  return changed;
}

bool synchronizeCreativeEditorWorldLayoutTerrainRegion(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    const CreativeEditorTerrainGenerationState& terrainGeneration,
    const cr::CreativeDocument& document) {
  if (!state.ownsPreview ||
      creativeEditorTerrainGenerationPreviewMatches(terrainGeneration,
                                                     document)) {
    return false;
  }
  state.ownsPreview = false;
  clearCreativeEditorWorldLayoutTerrainRegionSelection(state);
  state.statusMessage = "Terrain region preview canceled: document changed";
  return true;
}

std::string_view toString(
    CreativeEditorWorldLayoutTopographyStatus status) noexcept {
  switch (status) {
    case CreativeEditorWorldLayoutTopographyStatus::NotRequested:
      return "NotRequested";
    case CreativeEditorWorldLayoutTopographyStatus::InvalidDocument:
      return "InvalidDocument";
    case CreativeEditorWorldLayoutTopographyStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeEditorWorldLayoutTopographyStatus::SurfaceRejected:
      return "SurfaceRejected";
    case CreativeEditorWorldLayoutTopographyStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeEditorWorldLayoutTopographyStatus::Empty:
      return "Empty";
    case CreativeEditorWorldLayoutTopographyStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

CreativeEditorWorldLayoutTopographyPlan
buildCreativeEditorWorldLayoutTopography(
    const cr::CreativeDocument& document,
    std::uint16_t intervalCells,
    std::uint16_t majorEvery,
    const cr::CreativeTerrainHeightField* heightFieldOverride) {
  CreativeEditorWorldLayoutTopographyPlan plan;
  plan.requested = true;
  plan.documentId = document.id();
  plan.documentRevision = document.revision();
  plan.terrainRevision = document.terrainField().revision();
  const cr::CreativeTerrainHeightField& heightField =
      topographyHeightField(document, heightFieldOverride);
  plan.terrainHeightRevision = heightField.revision();
  if (!document.isValid() || document.id() == cr::kInvalidDocumentId) {
    plan.status = CreativeEditorWorldLayoutTopographyStatus::InvalidDocument;
    plan.reasonCode =
        "creative_editor_world_layout_topography_document_invalid";
    return plan;
  }
  if (!validRequest(intervalCells, majorEvery)) {
    plan.status = CreativeEditorWorldLayoutTopographyStatus::InvalidRequest;
    plan.reasonCode =
        "creative_editor_world_layout_topography_request_invalid";
    return plan;
  }

  cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeComposedTerrainSurfacePlan(
          document.terrainField(), heightField);
  if (!surface.accepted) {
    plan.status = CreativeEditorWorldLayoutTopographyStatus::SurfaceRejected;
    plan.reasonCode =
        "creative_editor_world_layout_topography_surface_rejected";
    return plan;
  }
  if (surface.columns.size() >
      kCreativeEditorWorldLayoutTopographyCellCapacity) {
    plan.status =
        CreativeEditorWorldLayoutTopographyStatus::CapacityExceeded;
    plan.reasonCode =
        "creative_editor_world_layout_topography_cell_capacity_exceeded";
    return plan;
  }

  plan.contours = cr::buildCreativeTerrainContourPlan(
      surface,
      {intervalCells, majorEvery, cr::kCreativeTerrainContourSegmentCapacity});
  plan.columns = std::move(surface.columns);
  plan.accepted = true;
  if (plan.columns.empty()) {
    plan.status = CreativeEditorWorldLayoutTopographyStatus::Empty;
    plan.reasonCode = "creative_editor_world_layout_topography_empty";
    return plan;
  }

  plan.minimumHeightCells = plan.columns.front().heightCells;
  plan.maximumHeightCells = plan.columns.front().heightCells;
  for (const cr::CreativeTerrainColumn& column : plan.columns) {
    plan.minimumHeightCells =
        std::min(plan.minimumHeightCells, column.heightCells);
    plan.maximumHeightCells =
        std::max(plan.maximumHeightCells, column.heightCells);
  }
  plan.status = CreativeEditorWorldLayoutTopographyStatus::Ready;
  plan.reasonCode = plan.contours.accepted
                        ? "creative_editor_world_layout_topography_ready"
                        : "creative_editor_world_layout_topography_ready_without_contours";
  return plan;
}

bool refreshCreativeEditorWorldLayoutTopography(
    CreativeEditorWorldLayoutTopographyState& state,
    const cr::CreativeDocument& document,
    bool sourceOverride,
    std::uint64_t sourceKey,
    const cr::CreativeTerrainHeightField* heightFieldOverride) {
  if (!state.visible ||
      cacheMatches(state, document, sourceOverride, sourceKey,
                   heightFieldOverride)) {
    return false;
  }
  state.plan = buildCreativeEditorWorldLayoutTopography(
      document, state.intervalCells, state.majorEvery, heightFieldOverride);
  state.cacheValid = true;
  state.cachedSourceOverride = sourceOverride;
  state.cachedSourceKey = sourceKey;
  state.cachedDocumentId = document.id();
  state.cachedDocumentRevision = document.revision();
  state.cachedTerrainRevision = document.terrainField().revision();
  state.cachedTerrainHeightRevision =
      topographyHeightField(document, heightFieldOverride).revision();
  state.cachedIntervalCells = state.intervalCells;
  state.cachedMajorEvery = state.majorEvery;
  ++state.buildCount;
  return true;
}

CreativeEditorWorldLayoutTopographySample
sampleCreativeEditorWorldLayoutTopography(
    const CreativeEditorWorldLayoutTopographyPlan& plan,
    double xCells,
    double zCells) noexcept {
  CreativeEditorWorldLayoutTopographySample sample;
  if (!plan.accepted ||
      plan.status != CreativeEditorWorldLayoutTopographyStatus::Ready) {
    return sample;
  }
  if (!coordinateFromPoint(xCells, sample.coord.x) ||
      !coordinateFromPoint(zCells, sample.coord.z)) {
    return {};
  }
  const std::span<const cr::CreativeTerrainColumn> columns = plan.columns;
  const cr::CreativeTerrainColumn* center = findColumn(columns, sample.coord);
  if (center == nullptr) {
    return sample;
  }

  sample.present = true;
  sample.heightCells = center->heightCells;
  const AxisSlope xSlope = sampleAxisSlope(
      columns, sample.coord, center->heightCells, 1, 0);
  const AxisSlope zSlope = sampleAxisSlope(
      columns, sample.coord, center->heightCells, 0, 1);
  sample.slopeXCellsPerCell = xSlope.value;
  sample.slopeZCellsPerCell = zSlope.value;
  sample.neighborSampleCount =
      static_cast<std::uint8_t>(xSlope.sampleCount + zSlope.sampleCount);
  sample.slopeMagnitude = std::hypot(xSlope.value, zSlope.value);
  sample.slopeDegrees =
      std::atan(sample.slopeMagnitude) * 180.0 / std::acos(-1.0);
  return sample;
}

}  // namespace iggy3d_creative_app
