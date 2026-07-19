#include "app/iggy3d/creative/document/TerrainContours.hpp"

#include <algorithm>
#include <array>
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

}  // namespace iggy3d::creative
