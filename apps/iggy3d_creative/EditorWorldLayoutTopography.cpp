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

[[nodiscard]] const cr::CreativeTerrainHeightField& topographyHeightField(
    const cr::CreativeDocument& document,
    const cr::CreativeTerrainHeightField* heightFieldOverride) noexcept {
  return heightFieldOverride == nullptr ? document.terrainHeightField()
                                        : *heightFieldOverride;
}

[[nodiscard]] bool coordLess(cr::CreativeTerrainCoord2 lhs,
                             cr::CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z != rhs.z ? lhs.z < rhs.z : lhs.x < rhs.x;
}

[[nodiscard]] const cr::CreativeTerrainAnalysisCell* findCell(
    std::span<const cr::CreativeTerrainAnalysisCell> cells,
    cr::CreativeTerrainCoord2 coord) noexcept {
  const auto found = std::lower_bound(
      cells.begin(), cells.end(), coord,
      [](const cr::CreativeTerrainAnalysisCell& cell,
         cr::CreativeTerrainCoord2 value) {
        return coordLess(cell.coord, value);
      });
  return found != cells.end() && found->coord == coord ? &*found : nullptr;
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

}  // namespace

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

  cr::CreativeTerrainSurfacePlan referenceSurface =
      cr::buildCreativeComposedTerrainSurfacePlan(
          document.terrainField(), document.terrainHeightField());
  cr::CreativeTerrainSurfacePlan surface =
      heightFieldOverride == nullptr
          ? referenceSurface
          : cr::buildCreativeComposedTerrainSurfacePlan(
                document.terrainField(), heightField);
  if (!referenceSurface.accepted || !surface.accepted) {
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

  cr::CreativeTerrainAnalysisRequest analysisRequest;
  analysisRequest.contours =
      {intervalCells, majorEvery, cr::kCreativeTerrainContourSegmentCapacity};
  plan.analysis = cr::buildCreativeTerrainAnalysisPlan(
      surface, analysisRequest,
      heightFieldOverride == nullptr ? nullptr : &referenceSurface);
  if (!plan.analysis.accepted) {
    plan.status = plan.analysis.status ==
                          cr::CreativeTerrainAnalysisPlanStatus::CapacityExceeded
                      ? CreativeEditorWorldLayoutTopographyStatus::CapacityExceeded
                      : CreativeEditorWorldLayoutTopographyStatus::SurfaceRejected;
    plan.reasonCode = plan.analysis.reasonCode;
    return plan;
  }
  plan.accepted = true;
  if (plan.analysis.status == cr::CreativeTerrainAnalysisPlanStatus::Empty) {
    plan.status = CreativeEditorWorldLayoutTopographyStatus::Empty;
    plan.reasonCode = "creative_editor_world_layout_topography_empty";
    return plan;
  }

  plan.minimumHeightCells = plan.analysis.minimumHeightCells;
  plan.maximumHeightCells = plan.analysis.maximumHeightCells;
  plan.status = CreativeEditorWorldLayoutTopographyStatus::Ready;
  plan.reasonCode = plan.analysis.contours.accepted
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
  const cr::CreativeTerrainAnalysisCell* center =
      findCell(plan.analysis.cells, sample.coord);
  if (center == nullptr || !center->terrainPresent) {
    return sample;
  }

  sample.present = true;
  sample.heightCells = center->heightCells;
  sample.slopeXCellsPerCell = center->slopeXCellsPerCell;
  sample.slopeZCellsPerCell = center->slopeZCellsPerCell;
  sample.neighborSampleCount = center->neighborSampleCount;
  sample.slopeMagnitude =
      std::hypot(center->slopeXCellsPerCell, center->slopeZCellsPerCell);
  sample.slopeDegrees = center->slopeDegrees;
  sample.slopeBand = center->slopeBand;
  sample.cutFill = center->cutFill;
  sample.deltaCells = center->deltaCells;
  return sample;
}

CreativeEditorWorldLayoutTerrainAnalysisEditPlan
planCreativeEditorWorldLayoutTerrainAnalysisEdit(
    const CreativeEditorWorldLayoutTopographyPlan& plan,
    double xCells,
    double zCells,
    double contourToleranceCells,
    cr::CreativeTerrainAnalysisHitMode mode) noexcept {
  CreativeEditorWorldLayoutTerrainAnalysisEditPlan edit;
  edit.requested = true;
  edit.hit = cr::hitCreativeTerrainAnalysis(
      plan.analysis, {xCells, zCells}, contourToleranceCells, mode);
  if (!edit.hit.accepted) {
    edit.reasonCode = edit.hit.reasonCode;
    return edit;
  }

  cr::CreativeTerrainRegionRecipe recipe;
  recipe.mode = cr::CreativeTerrainRegionMode::Flatten;
  recipe.targetHeightCells = edit.hit.targetHeightCells;
  if (edit.hit.kind == cr::CreativeTerrainAnalysisHitKind::Contour) {
    constexpr std::int64_t radius = 2;
    const std::int64_t minimumX =
        static_cast<std::int64_t>(edit.hit.coord.x) - radius;
    const std::int64_t minimumZ =
        static_cast<std::int64_t>(edit.hit.coord.z) - radius;
    if (minimumX < std::numeric_limits<std::int32_t>::min() ||
        minimumX > std::numeric_limits<std::int32_t>::max() ||
        minimumZ < std::numeric_limits<std::int32_t>::min() ||
        minimumZ > std::numeric_limits<std::int32_t>::max()) {
      edit.reasonCode =
          "creative_editor_world_layout_terrain_analysis_bounds_overflow";
      return edit;
    }
    recipe.bounds = {{static_cast<std::int32_t>(minimumX),
                      static_cast<std::int32_t>(minimumZ)},
                     5U, 5U};
    recipe.mask = cr::CreativeTerrainCompositionMask::Ellipse;
    recipe.featherCells = 1U;
  } else if (edit.hit.kind ==
             cr::CreativeTerrainAnalysisHitKind::HeightHandle) {
    recipe.bounds = {edit.hit.coord, 1U, 1U};
    recipe.mask = cr::CreativeTerrainCompositionMask::Rectangle;
    recipe.featherCells = 0U;
  } else {
    edit.reasonCode =
        "creative_editor_world_layout_terrain_analysis_target_invalid";
    return edit;
  }
  if (!cr::isValidCreativeTerrainRegionRecipe(recipe)) {
    edit.reasonCode =
        "creative_editor_world_layout_terrain_analysis_recipe_invalid";
    return edit;
  }
  edit.accepted = true;
  edit.recipe = recipe;
  edit.reasonCode =
      "creative_editor_world_layout_terrain_analysis_edit_ready";
  return edit;
}

bool selectCreativeEditorWorldLayoutTerrainAnalysisEdit(
    CreativeEditorWorldLayoutTerrainRegionState& state,
    const CreativeEditorWorldLayoutTerrainAnalysisEditPlan& edit) noexcept {
  if (!state.editingEnabled || !edit.accepted ||
      !cr::isValidCreativeTerrainRegionRecipe(edit.recipe)) {
    state.statusMessage = "Terrain analysis target unavailable";
    return false;
  }

  const cr::CreativeTerrainHeightFieldBounds bounds = edit.recipe.bounds;
  const std::int64_t maximumX =
      static_cast<std::int64_t>(bounds.minimum.x) + bounds.widthCells - 1;
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(bounds.minimum.z) + bounds.depthCells - 1;
  if (maximumX > std::numeric_limits<std::int32_t>::max() ||
      maximumZ > std::numeric_limits<std::int32_t>::max()) {
    state.statusMessage = "Terrain analysis target unavailable";
    return false;
  }

  state.selecting = false;
  state.manipulation = {};
  state.editingOperationId = cr::kInvalidCreativeTerrainOperationId;
  state.recipe = edit.recipe;
  state.regionValid = true;
  state.anchor = bounds.minimum;
  state.cursor = {static_cast<std::int32_t>(maximumX),
                  static_cast<std::int32_t>(maximumZ)};
  state.statusMessage =
      edit.hit.kind == cr::CreativeTerrainAnalysisHitKind::Contour
          ? "Contour region selected"
          : "Terrain height selected";
  return true;
}

}  // namespace iggy3d_creative_app
