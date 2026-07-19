#include "EditorWorldLayoutTopography.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <span>
#include <utility>

#include "app/iggy3d/creative/document/TerrainHeightField.hpp"

namespace iggy3d_creative_app {
namespace {

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
    std::uint64_t sourceKey) noexcept {
  return state.cacheValid &&
         state.cachedSourceOverride == sourceOverride &&
         state.cachedSourceKey == sourceKey &&
         state.cachedDocumentId == document.id() &&
         state.cachedDocumentRevision == document.revision() &&
         state.cachedTerrainRevision == document.terrainField().revision() &&
         state.cachedTerrainHeightRevision ==
             document.terrainHeightField().revision() &&
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
    std::uint16_t majorEvery) {
  CreativeEditorWorldLayoutTopographyPlan plan;
  plan.requested = true;
  plan.documentId = document.id();
  plan.documentRevision = document.revision();
  plan.terrainRevision = document.terrainField().revision();
  plan.terrainHeightRevision = document.terrainHeightField().revision();
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
          document.terrainField(), document.terrainHeightField());
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
    std::uint64_t sourceKey) {
  if (!state.visible ||
      cacheMatches(state, document, sourceOverride, sourceKey)) {
    return false;
  }
  state.plan = buildCreativeEditorWorldLayoutTopography(
      document, state.intervalCells, state.majorEvery);
  state.cacheValid = true;
  state.cachedSourceOverride = sourceOverride;
  state.cachedSourceKey = sourceKey;
  state.cachedDocumentId = document.id();
  state.cachedDocumentRevision = document.revision();
  state.cachedTerrainRevision = document.terrainField().revision();
  state.cachedTerrainHeightRevision =
      document.terrainHeightField().revision();
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
