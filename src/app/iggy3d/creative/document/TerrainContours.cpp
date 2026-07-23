#include "app/iggy3d/creative/document/TerrainContours.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool coordLess(CreativeTerrainCoord2 lhs,
                             CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z != rhs.z ? lhs.z < rhs.z : lhs.x < rhs.x;
}

[[nodiscard]] bool validColumns(
    std::span<const CreativeTerrainColumn> columns) noexcept {
  if (columns.size() > kCreativeTerrainRenderPatchCapacity) {
    return false;
  }
  for (std::size_t index = 0U; index < columns.size(); ++index) {
    if (columns[index].heightCells < kCreativeTerrainMinimumHeightCells ||
        columns[index].heightCells > kCreativeTerrainMaximumHeightCells ||
        (index > 0U &&
         !coordLess(columns[index - 1U].coord, columns[index].coord))) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] const CreativeTerrainColumn* findColumn(
    std::span<const CreativeTerrainColumn> columns,
    CreativeTerrainCoord2 coord) noexcept {
  const auto found = std::lower_bound(
      columns.begin(), columns.end(), coord,
      [](const CreativeTerrainColumn& column, CreativeTerrainCoord2 value) {
        return coordLess(column.coord, value);
      });
  return found != columns.end() && found->coord == coord ? &*found : nullptr;
}

struct AxisSlope {
  double value = 0.0;
  std::uint8_t sampleCount = 0U;
};

[[nodiscard]] const CreativeTerrainColumn* findOffsetColumn(
    std::span<const CreativeTerrainColumn> columns,
    CreativeTerrainCoord2 coord,
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
  return findColumn(columns, {static_cast<std::int32_t>(x),
                              static_cast<std::int32_t>(z)});
}

[[nodiscard]] AxisSlope sampleAxisSlope(
    std::span<const CreativeTerrainColumn> columns,
    const CreativeTerrainColumn& center,
    std::int32_t deltaX,
    std::int32_t deltaZ) noexcept {
  const CreativeTerrainColumn* negative = findOffsetColumn(
      columns, center.coord, -deltaX, -deltaZ);
  const CreativeTerrainColumn* positive = findOffsetColumn(
      columns, center.coord, deltaX, deltaZ);
  if (negative != nullptr && positive != nullptr) {
    return {(static_cast<double>(positive->heightCells) -
             static_cast<double>(negative->heightCells)) /
                2.0,
            2U};
  }
  if (positive != nullptr) {
    return {static_cast<double>(positive->heightCells) -
                static_cast<double>(center.heightCells),
            1U};
  }
  if (negative != nullptr) {
    return {static_cast<double>(center.heightCells) -
                static_cast<double>(negative->heightCells),
            1U};
  }
  return {};
}

[[nodiscard]] double distanceSquared(CreativeTerrainContourPoint first,
                                     CreativeTerrainContourPoint second) {
  const double x = second.x - first.x;
  const double z = second.z - first.z;
  return x * x + z * z;
}

[[nodiscard]] double distanceToSegment(
    CreativeTerrainContourPoint point,
    const CreativeTerrainContourSegment& segment,
    CreativeTerrainContourPoint& nearest) noexcept {
  const double x = segment.end.x - segment.start.x;
  const double z = segment.end.z - segment.start.z;
  const double lengthSquared = x * x + z * z;
  const double t = lengthSquared <= 1.0e-18
                       ? 0.0
                       : std::clamp(((point.x - segment.start.x) * x +
                                     (point.z - segment.start.z) * z) /
                                        lengthSquared,
                                    0.0, 1.0);
  nearest = {segment.start.x + x * t, segment.start.z + z * t};
  return std::sqrt(distanceSquared(point, nearest));
}

struct ContourSample {
  CreativeTerrainContourPoint point{};
  std::uint16_t heightCells = 0U;
};

[[nodiscard]] bool highAtLevel(const ContourSample& sample,
                               std::uint16_t levelCells) noexcept {
  return sample.heightCells >= levelCells;
}

[[nodiscard]] CreativeTerrainContourPoint interpolateContourPoint(
    const ContourSample& first,
    const ContourSample& second,
    std::uint16_t levelCells) noexcept {
  const double threshold = static_cast<double>(levelCells) - 0.5;
  const double heightDelta = static_cast<double>(second.heightCells) -
                             static_cast<double>(first.heightCells);
  const double t =
      (threshold - static_cast<double>(first.heightCells)) / heightDelta;
  return {first.point.x + (second.point.x - first.point.x) * t,
          first.point.z + (second.point.z - first.point.z) * t};
}

[[nodiscard]] CreativeTerrainContourPoint crossingForEdge(
    const std::array<ContourSample, 4U>& samples,
    std::uint8_t edge,
    std::uint16_t levelCells) noexcept {
  constexpr std::array<std::array<std::uint8_t, 2U>, 4U> endpoints{{
      {0U, 1U},
      {1U, 2U},
      {2U, 3U},
      {3U, 0U},
  }};
  return interpolateContourPoint(samples[endpoints[edge][0U]],
                                 samples[endpoints[edge][1U]], levelCells);
}

[[nodiscard]] bool appendContourSegment(
    CreativeTerrainContourPlan& plan,
    const std::array<ContourSample, 4U>& samples,
    std::uint8_t firstEdge,
    std::uint8_t secondEdge,
    std::uint16_t levelCells,
    const CreativeTerrainContourRequest& request) {
  if (plan.segments.size() >= request.maxSegmentCount) {
    return false;
  }
  plan.segments.push_back(
      {crossingForEdge(samples, firstEdge, levelCells),
       crossingForEdge(samples, secondEdge, levelCells), levelCells,
       (levelCells / request.intervalCells) % request.majorEvery == 0U});
  return true;
}

[[nodiscard]] bool appendContoursForLevel(
    CreativeTerrainContourPlan& plan,
    const std::array<ContourSample, 4U>& samples,
    std::uint16_t levelCells,
    const CreativeTerrainContourRequest& request) {
  std::array<std::uint8_t, 4U> crossingEdges{};
  std::size_t crossingCount = 0U;
  constexpr std::array<std::array<std::uint8_t, 2U>, 4U> edgeSamples{{
      {0U, 1U},
      {1U, 2U},
      {2U, 3U},
      {3U, 0U},
  }};
  for (std::size_t edge = 0U; edge < edgeSamples.size(); ++edge) {
    const auto endpoints = edgeSamples[edge];
    if (highAtLevel(samples[endpoints[0U]], levelCells) !=
        highAtLevel(samples[endpoints[1U]], levelCells)) {
      crossingEdges[crossingCount++] = static_cast<std::uint8_t>(edge);
    }
  }
  if (crossingCount == 0U) {
    return true;
  }
  if (crossingCount == 2U) {
    return appendContourSegment(plan, samples, crossingEdges[0U],
                                crossingEdges[1U], levelCells, request);
  }
  if (crossingCount != 4U) {
    return false;
  }

  ++plan.ambiguousCaseCount;
  std::uint8_t highMask = 0U;
  std::uint32_t heightSum = 0U;
  for (std::size_t index = 0U; index < samples.size(); ++index) {
    if (highAtLevel(samples[index], levelCells)) {
      highMask |= static_cast<std::uint8_t>(1U << index);
    }
    heightSum += samples[index].heightCells;
  }
  const bool centerHigh = heightSum >=
                          static_cast<std::uint32_t>(levelCells) * 4U - 2U;
  std::array<std::array<std::uint8_t, 2U>, 2U> pairs{};
  if ((highMask == 0x5U && centerHigh) ||
      (highMask == 0xAU && !centerHigh)) {
    pairs = {{{0U, 1U}, {2U, 3U}}};
  } else {
    pairs = {{{0U, 3U}, {1U, 2U}}};
  }
  if (plan.segments.size() + pairs.size() > request.maxSegmentCount) {
    return false;
  }
  for (const auto pair : pairs) {
    if (!appendContourSegment(plan, samples, pair[0U], pair[1U], levelCells,
                              request)) {
      return false;
    }
  }
  return true;
}

}  // namespace

std::string_view toString(CreativeTerrainContourPlanStatus status) noexcept {
  switch (status) {
    case CreativeTerrainContourPlanStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainContourPlanStatus::InvalidSurface:
      return "InvalidSurface";
    case CreativeTerrainContourPlanStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeTerrainContourPlanStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainContourPlanStatus::Empty:
      return "Empty";
    case CreativeTerrainContourPlanStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

std::string_view toString(CreativeTerrainSlopeBand band) noexcept {
  switch (band) {
    case CreativeTerrainSlopeBand::Unavailable:
      return "Unavailable";
    case CreativeTerrainSlopeBand::Flat:
      return "Flat";
    case CreativeTerrainSlopeBand::Gentle:
      return "Gentle";
    case CreativeTerrainSlopeBand::Steep:
      return "Steep";
    case CreativeTerrainSlopeBand::Extreme:
      return "Extreme";
    case CreativeTerrainSlopeBand::Count:
      return "Count";
  }
  return "Unknown";
}

std::string_view toString(CreativeTerrainCutFillKind kind) noexcept {
  switch (kind) {
    case CreativeTerrainCutFillKind::Unchanged:
      return "Unchanged";
    case CreativeTerrainCutFillKind::Cut:
      return "Cut";
    case CreativeTerrainCutFillKind::Fill:
      return "Fill";
    case CreativeTerrainCutFillKind::Count:
      return "Count";
  }
  return "Unknown";
}

std::string_view toString(CreativeTerrainAnalysisPlanStatus status) noexcept {
  switch (status) {
    case CreativeTerrainAnalysisPlanStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainAnalysisPlanStatus::InvalidSurface:
      return "InvalidSurface";
    case CreativeTerrainAnalysisPlanStatus::InvalidReference:
      return "InvalidReference";
    case CreativeTerrainAnalysisPlanStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeTerrainAnalysisPlanStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainAnalysisPlanStatus::Empty:
      return "Empty";
    case CreativeTerrainAnalysisPlanStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

CreativeTerrainContourPlan buildCreativeTerrainContourPlan(
    const CreativeTerrainSurfacePlan& surface,
    const CreativeTerrainContourRequest& request) {
  CreativeTerrainContourPlan plan;
  plan.requested = true;
  plan.sourceRevision = surface.sourceRevision;
  plan.sourceColumnCount = surface.columns.size();
  const bool surfaceShapeValid =
      (surface.status == CreativeTerrainSurfacePlanStatus::Empty &&
       surface.columns.empty()) ||
      (surface.status == CreativeTerrainSurfacePlanStatus::Ready &&
       !surface.columns.empty());
  if (!surface.requested || !surface.accepted || !surfaceShapeValid ||
      !validColumns(surface.columns)) {
    plan.status = CreativeTerrainContourPlanStatus::InvalidSurface;
    plan.reasonCode = "creative_terrain_contours_surface_invalid";
    return plan;
  }
  if (request.intervalCells == 0U ||
      request.intervalCells > kCreativeTerrainContourMaximumIntervalCells ||
      request.majorEvery == 0U ||
      request.majorEvery > kCreativeTerrainContourMaximumMajorEvery ||
      request.maxSegmentCount == 0U ||
      request.maxSegmentCount > kCreativeTerrainContourSegmentCapacity) {
    plan.status = CreativeTerrainContourPlanStatus::InvalidRequest;
    plan.reasonCode = "creative_terrain_contours_request_invalid";
    return plan;
  }
  if (surface.columns.empty()) {
    plan.accepted = true;
    plan.status = CreativeTerrainContourPlanStatus::Empty;
    plan.reasonCode = "creative_terrain_contours_empty";
    return plan;
  }

  std::array<bool, kCreativeTerrainMaximumHeightCells + 1U> usedLevels{};
  const std::span<const CreativeTerrainColumn> columns = surface.columns;
  for (const CreativeTerrainColumn& lowerLeft : columns) {
    if (lowerLeft.coord.x == std::numeric_limits<std::int32_t>::max() ||
        lowerLeft.coord.z == std::numeric_limits<std::int32_t>::max()) {
      continue;
    }
    const CreativeTerrainColumn* lowerRight = findColumn(
        columns, {lowerLeft.coord.x + 1, lowerLeft.coord.z});
    const CreativeTerrainColumn* upperRight = findColumn(
        columns, {lowerLeft.coord.x + 1, lowerLeft.coord.z + 1});
    const CreativeTerrainColumn* upperLeft = findColumn(
        columns, {lowerLeft.coord.x, lowerLeft.coord.z + 1});
    if (lowerRight == nullptr || upperRight == nullptr ||
        upperLeft == nullptr) {
      continue;
    }
    ++plan.evaluatedSquareCount;
    const double centerX = static_cast<double>(lowerLeft.coord.x) + 0.5;
    const double centerZ = static_cast<double>(lowerLeft.coord.z) + 0.5;
    const std::array<ContourSample, 4U> samples{{
        {{centerX, centerZ}, lowerLeft.heightCells},
        {{centerX + 1.0, centerZ}, lowerRight->heightCells},
        {{centerX + 1.0, centerZ + 1.0}, upperRight->heightCells},
        {{centerX, centerZ + 1.0}, upperLeft->heightCells},
    }};
    std::uint16_t minimum = samples.front().heightCells;
    std::uint16_t maximum = samples.front().heightCells;
    for (const ContourSample& sample : samples) {
      minimum = std::min(minimum, sample.heightCells);
      maximum = std::max(maximum, sample.heightCells);
    }
    if (minimum == maximum) {
      continue;
    }
    for (std::uint16_t level = request.intervalCells;
         level <= kCreativeTerrainMaximumHeightCells;
         level = static_cast<std::uint16_t>(level + request.intervalCells)) {
      const double threshold = static_cast<double>(level) - 0.5;
      if (threshold <= minimum || threshold >= maximum) {
        continue;
      }
      const std::size_t segmentCountBefore = plan.segments.size();
      if (!appendContoursForLevel(plan, samples, level, request)) {
        plan.segments.clear();
        plan.contourLevelCount = 0U;
        plan.status = CreativeTerrainContourPlanStatus::CapacityExceeded;
        plan.reasonCode = "creative_terrain_contours_capacity_exceeded";
        return plan;
      }
      if (plan.segments.size() != segmentCountBefore && !usedLevels[level]) {
        usedLevels[level] = true;
        ++plan.contourLevelCount;
      }
    }
  }

  plan.accepted = true;
  plan.status = plan.segments.empty()
                    ? CreativeTerrainContourPlanStatus::Empty
                    : CreativeTerrainContourPlanStatus::Ready;
  plan.reasonCode = plan.segments.empty()
                        ? "creative_terrain_contours_empty"
                        : "creative_terrain_contours_ready";
  return plan;
}

CreativeTerrainAnalysisPlan buildCreativeTerrainAnalysisPlan(
    const CreativeTerrainSurfacePlan& surface,
    const CreativeTerrainAnalysisRequest& request,
    const CreativeTerrainSurfacePlan* reference) {
  CreativeTerrainAnalysisPlan plan;
  plan.requested = true;
  plan.sourceRevision = surface.sourceRevision;
  plan.hasReference = reference != nullptr;
  plan.referenceRevision = reference == nullptr ? 0U : reference->sourceRevision;

  const bool surfaceShapeValid =
      (surface.status == CreativeTerrainSurfacePlanStatus::Empty &&
       surface.columns.empty()) ||
      (surface.status == CreativeTerrainSurfacePlanStatus::Ready &&
       !surface.columns.empty());
  if (!surface.requested || !surface.accepted || !surfaceShapeValid ||
      !validColumns(surface.columns)) {
    plan.status = CreativeTerrainAnalysisPlanStatus::InvalidSurface;
    plan.reasonCode = "creative_terrain_analysis_surface_invalid";
    return plan;
  }
  if (reference != nullptr) {
    const bool referenceShapeValid =
        (reference->status == CreativeTerrainSurfacePlanStatus::Empty &&
         reference->columns.empty()) ||
        (reference->status == CreativeTerrainSurfacePlanStatus::Ready &&
         !reference->columns.empty());
    if (!reference->requested || !reference->accepted ||
        !referenceShapeValid || !validColumns(reference->columns)) {
      plan.status = CreativeTerrainAnalysisPlanStatus::InvalidReference;
      plan.reasonCode = "creative_terrain_analysis_reference_invalid";
      return plan;
    }
  }
  if (!std::isfinite(request.flatMaximumDegrees) ||
      !std::isfinite(request.gentleMaximumDegrees) ||
      !std::isfinite(request.steepMaximumDegrees) ||
      request.flatMaximumDegrees < 0.0 ||
      request.flatMaximumDegrees > request.gentleMaximumDegrees ||
      request.gentleMaximumDegrees > request.steepMaximumDegrees ||
      request.steepMaximumDegrees > 90.0 || request.maxCellCount == 0U ||
      request.maxCellCount > kCreativeTerrainAnalysisCellCapacity ||
      request.maxLabelCount == 0U ||
      request.maxLabelCount > kCreativeTerrainContourLabelCapacity) {
    plan.status = CreativeTerrainAnalysisPlanStatus::InvalidRequest;
    plan.reasonCode = "creative_terrain_analysis_request_invalid";
    return plan;
  }

  plan.contours = buildCreativeTerrainContourPlan(surface, request.contours);
  if (!plan.contours.accepted &&
      plan.contours.status !=
          CreativeTerrainContourPlanStatus::CapacityExceeded) {
    plan.status = CreativeTerrainAnalysisPlanStatus::InvalidRequest;
    plan.reasonCode = "creative_terrain_analysis_contours_rejected";
    return plan;
  }

  std::array<std::size_t, kCreativeTerrainMaximumHeightCells + 1U>
      labelSegmentIndices{};
  std::array<double, kCreativeTerrainMaximumHeightCells + 1U>
      labelSegmentLengths{};
  labelSegmentIndices.fill(std::numeric_limits<std::size_t>::max());
  for (std::size_t index = 0U; index < plan.contours.segments.size(); ++index) {
    const CreativeTerrainContourSegment& segment = plan.contours.segments[index];
    if (!segment.major || segment.levelCells > kCreativeTerrainMaximumHeightCells) {
      continue;
    }
    const double length = distanceSquared(segment.start, segment.end);
    if (length > labelSegmentLengths[segment.levelCells]) {
      labelSegmentLengths[segment.levelCells] = length;
      labelSegmentIndices[segment.levelCells] = index;
    }
  }
  for (std::uint16_t level = 0U;
       level <= kCreativeTerrainMaximumHeightCells; ++level) {
    const std::size_t segmentIndex = labelSegmentIndices[level];
    if (segmentIndex == std::numeric_limits<std::size_t>::max()) {
      continue;
    }
    if (plan.labels.size() >= request.maxLabelCount) {
      plan.contours = {};
      plan.labels.clear();
      plan.cells.clear();
      plan.status = CreativeTerrainAnalysisPlanStatus::CapacityExceeded;
      plan.reasonCode = "creative_terrain_analysis_label_capacity";
      return plan;
    }
    const CreativeTerrainContourSegment& segment =
        plan.contours.segments[segmentIndex];
    plan.labels.push_back(
        {{(segment.start.x + segment.end.x) * 0.5,
          (segment.start.z + segment.end.z) * 0.5},
         level});
  }

  const std::span<const CreativeTerrainColumn> sourceColumns = surface.columns;
  const std::span<const CreativeTerrainColumn> referenceColumns =
      reference == nullptr
          ? std::span<const CreativeTerrainColumn>{}
          : std::span<const CreativeTerrainColumn>{reference->columns};
  std::size_t sourceIndex = 0U;
  std::size_t referenceIndex = 0U;
  while (sourceIndex < sourceColumns.size() ||
         referenceIndex < referenceColumns.size()) {
    if (plan.cells.size() >= request.maxCellCount) {
      plan.contours = {};
      plan.labels.clear();
      plan.cells.clear();
      plan.status = CreativeTerrainAnalysisPlanStatus::CapacityExceeded;
      plan.reasonCode = "creative_terrain_analysis_cell_capacity";
      return plan;
    }
    const bool haveSource = sourceIndex < sourceColumns.size();
    const bool haveReference = referenceIndex < referenceColumns.size();
    const CreativeTerrainCoord2 sourceCoord =
        haveSource ? sourceColumns[sourceIndex].coord : CreativeTerrainCoord2{};
    const CreativeTerrainCoord2 referenceCoord =
        haveReference ? referenceColumns[referenceIndex].coord
                      : CreativeTerrainCoord2{};
    const bool takeSource =
        haveSource && (!haveReference || coordLess(sourceCoord, referenceCoord));
    const bool takeReference =
        haveReference && (!haveSource || coordLess(referenceCoord, sourceCoord));
    const bool takeBoth = haveSource && haveReference &&
                          sourceCoord == referenceCoord;

    CreativeTerrainAnalysisCell cell;
    if (takeSource || takeBoth) {
      const CreativeTerrainColumn& column = sourceColumns[sourceIndex++];
      cell.coord = column.coord;
      cell.terrainPresent = true;
      cell.heightCells = column.heightCells;
      const AxisSlope xSlope = sampleAxisSlope(sourceColumns, column, 1, 0);
      const AxisSlope zSlope = sampleAxisSlope(sourceColumns, column, 0, 1);
      cell.slopeXCellsPerCell = xSlope.value;
      cell.slopeZCellsPerCell = zSlope.value;
      cell.neighborSampleCount =
          static_cast<std::uint8_t>(xSlope.sampleCount + zSlope.sampleCount);
      cell.slopeDegrees =
          std::atan(std::hypot(xSlope.value, zSlope.value)) * 180.0 /
          std::acos(-1.0);
      cell.slopeBand =
          cell.slopeDegrees <= request.flatMaximumDegrees
              ? CreativeTerrainSlopeBand::Flat
          : cell.slopeDegrees <= request.gentleMaximumDegrees
              ? CreativeTerrainSlopeBand::Gentle
          : cell.slopeDegrees <= request.steepMaximumDegrees
              ? CreativeTerrainSlopeBand::Steep
              : CreativeTerrainSlopeBand::Extreme;
      plan.maximumSlopeDegrees =
          std::max(plan.maximumSlopeDegrees, cell.slopeDegrees);
    }
    if (takeReference || takeBoth) {
      const CreativeTerrainColumn& column = referenceColumns[referenceIndex++];
      if (!cell.terrainPresent) {
        cell.coord = column.coord;
      }
      cell.referencePresent = true;
      cell.referenceHeightCells = column.heightCells;
    } else if (reference == nullptr && cell.terrainPresent) {
      cell.referencePresent = true;
      cell.referenceHeightCells = cell.heightCells;
    }
    const int delta = static_cast<int>(cell.heightCells) -
                      static_cast<int>(cell.referenceHeightCells);
    cell.deltaCells = static_cast<std::int16_t>(delta);
    if (delta < 0) {
      cell.cutFill = CreativeTerrainCutFillKind::Cut;
      ++plan.cutCellCount;
    } else if (delta > 0) {
      cell.cutFill = CreativeTerrainCutFillKind::Fill;
      ++plan.fillCellCount;
    }
    plan.cells.push_back(cell);
  }

  if (!sourceColumns.empty()) {
    plan.minimumHeightCells = sourceColumns.front().heightCells;
    plan.maximumHeightCells = sourceColumns.front().heightCells;
    for (const CreativeTerrainColumn& column : sourceColumns) {
      plan.minimumHeightCells =
          std::min(plan.minimumHeightCells, column.heightCells);
      plan.maximumHeightCells =
          std::max(plan.maximumHeightCells, column.heightCells);
    }
  }
  plan.accepted = true;
  plan.status = plan.cells.empty() ? CreativeTerrainAnalysisPlanStatus::Empty
                                   : CreativeTerrainAnalysisPlanStatus::Ready;
  plan.reasonCode = plan.cells.empty() ? "creative_terrain_analysis_empty"
                                       : "creative_terrain_analysis_ready";
  return plan;
}

CreativeTerrainAnalysisHit hitCreativeTerrainAnalysis(
    const CreativeTerrainAnalysisPlan& plan,
    CreativeTerrainContourPoint point,
    double contourToleranceCells,
    CreativeTerrainAnalysisHitMode mode) noexcept {
  CreativeTerrainAnalysisHit hit;
  hit.requested = true;
  if (!plan.accepted ||
      plan.status != CreativeTerrainAnalysisPlanStatus::Ready ||
      !std::isfinite(point.x) || !std::isfinite(point.z) ||
      !std::isfinite(contourToleranceCells) || contourToleranceCells < 0.0 ||
      mode >= CreativeTerrainAnalysisHitMode::Count) {
    hit.reasonCode = "creative_terrain_analysis_hit_invalid";
    return hit;
  }

  if (mode != CreativeTerrainAnalysisHitMode::HeightHandleOnly) {
    double nearestDistance = std::numeric_limits<double>::infinity();
    CreativeTerrainContourPoint nearestPoint{};
    std::size_t nearestIndex = std::numeric_limits<std::size_t>::max();
    for (std::size_t index = 0U; index < plan.contours.segments.size();
         ++index) {
      CreativeTerrainContourPoint candidatePoint;
      const double distance = distanceToSegment(
          point, plan.contours.segments[index], candidatePoint);
      if (distance < nearestDistance) {
        nearestDistance = distance;
        nearestPoint = candidatePoint;
        nearestIndex = index;
      }
    }
    if (nearestIndex != std::numeric_limits<std::size_t>::max() &&
        nearestDistance <= contourToleranceCells) {
      const CreativeTerrainContourSegment& segment =
          plan.contours.segments[nearestIndex];
      hit.accepted = true;
      hit.kind = CreativeTerrainAnalysisHitKind::Contour;
      hit.coord = {static_cast<std::int32_t>(std::floor(nearestPoint.x)),
                   static_cast<std::int32_t>(std::floor(nearestPoint.z))};
      hit.point = nearestPoint;
      hit.targetHeightCells = segment.levelCells;
      hit.contourSegmentIndex = nearestIndex;
      hit.distanceCells = nearestDistance;
      hit.reasonCode = "creative_terrain_analysis_hit_contour";
      return hit;
    }
  }
  if (mode == CreativeTerrainAnalysisHitMode::ContourOnly ||
      point.x < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      point.x > static_cast<double>(std::numeric_limits<std::int32_t>::max()) ||
      point.z < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      point.z > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
    hit.reasonCode = "creative_terrain_analysis_hit_miss";
    return hit;
  }
  const CreativeTerrainCoord2 coord{
      static_cast<std::int32_t>(std::floor(point.x)),
      static_cast<std::int32_t>(std::floor(point.z))};
  const auto found = std::lower_bound(
      plan.cells.begin(), plan.cells.end(), coord,
      [](const CreativeTerrainAnalysisCell& cell, CreativeTerrainCoord2 value) {
        return coordLess(cell.coord, value);
      });
  if (found == plan.cells.end() || found->coord != coord ||
      !found->terrainPresent) {
    hit.reasonCode = "creative_terrain_analysis_hit_miss";
    return hit;
  }
  hit.accepted = true;
  hit.kind = CreativeTerrainAnalysisHitKind::HeightHandle;
  hit.coord = coord;
  hit.point = {static_cast<double>(coord.x) + 0.5,
               static_cast<double>(coord.z) + 0.5};
  hit.targetHeightCells = found->heightCells;
  hit.distanceCells = std::sqrt(distanceSquared(point, hit.point));
  hit.reasonCode = "creative_terrain_analysis_hit_height";
  return hit;
}

}  // namespace iggy3d::creative
