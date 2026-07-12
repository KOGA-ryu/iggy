#include "app/iggy3d/creative/tools/TerrainPath.hpp"

#include "app/iggy3d/creative/tools/TerrainBrushKernel.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>

namespace iggy3d::creative {
namespace {

template <typename Enum>
[[nodiscard]] constexpr bool validEnum(Enum value, Enum count) noexcept {
  return static_cast<std::size_t>(value) < static_cast<std::size_t>(count);
}

[[nodiscard]] constexpr bool coordLess(CreativeTerrainCoord2 lhs,
                                       CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z < rhs.z || (lhs.z == rhs.z && lhs.x < rhs.x);
}

struct CenterlineCell {
  CreativeTerrainCoord2 coord{};
  std::uint64_t progressQ16 = 0U;
  std::uint16_t fallbackHeightCells = 4U;
};

[[nodiscard]] CreativeTerrainPathPlan rejectedPlan(
    CreativeTerrainPathPlan plan,
    CreativeTerrainPathPlanStatus status,
    std::string_view reasonCode) noexcept {
  plan.accepted = false;
  plan.status = status;
  plan.controlCount = 0U;
  plan.editCount = 0U;
  plan.reasonCode = reasonCode;
  return plan;
}

[[nodiscard]] bool validRequest(
    const CreativeTerrainPathRequest& request) noexcept {
  if (request.field == nullptr || !request.field->validateInvariants() ||
      request.points.size() < 2U ||
      request.points.size() > kCreativeTerrainPathPointCapacity ||
      !validEnum(request.kind, CreativeTerrainPathKind::Count) ||
      !validEnum(request.elevation, CreativeTerrainPathElevation::Count) ||
      request.halfWidthCells > 3U ||
      (request.amplitudeCells != 1U && request.amplitudeCells != 2U &&
       request.amplitudeCells != 4U && request.amplitudeCells != 8U)) {
    return false;
  }
  for (std::size_t index = 0U; index < request.points.size(); ++index) {
    const CreativeTerrainPathPoint point = request.points[index];
    if (point.heightCells < kCreativeTerrainMinimumHeightCells ||
        point.heightCells > kCreativeTerrainMaximumHeightCells ||
        (index > 0U && point.coord == request.points[index - 1U].coord)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool segmentFacts(CreativeTerrainCoord2 from,
                                CreativeTerrainCoord2 to,
                                std::int32_t& dx,
                                std::int32_t& dz,
                                std::uint32_t& steps,
                                std::uint64_t& lengthQ16) noexcept {
  const std::int64_t deltaX = static_cast<std::int64_t>(to.x) - from.x;
  const std::int64_t deltaZ = static_cast<std::int64_t>(to.z) - from.z;
  const std::uint64_t absoluteX = static_cast<std::uint64_t>(std::abs(deltaX));
  const std::uint64_t absoluteZ = static_cast<std::uint64_t>(std::abs(deltaZ));
  if (absoluteX > kCreativeTerrainPathMaximumSegmentCells ||
      absoluteZ > kCreativeTerrainPathMaximumSegmentCells) {
    return false;
  }
  dx = static_cast<std::int32_t>(deltaX);
  dz = static_cast<std::int32_t>(deltaZ);
  steps = static_cast<std::uint32_t>(std::max(absoluteX, absoluteZ));
  if (steps == 0U) {
    return false;
  }
  const std::uint64_t squared = absoluteX * absoluteX + absoluteZ * absoluteZ;
  lengthQ16 = creativeTerrainIntegerSquareRoot(squared << 32U);
  return lengthQ16 > 0U;
}

[[nodiscard]] std::uint16_t interpolatedHeight(std::uint16_t from,
                                               std::uint16_t to,
                                               std::uint64_t numerator,
                                               std::uint64_t denominator) noexcept {
  if (denominator == 0U) {
    return from;
  }
  const std::int64_t delta = static_cast<std::int64_t>(to) - from;
  const std::int64_t offset = creativeTerrainRoundDivideSymmetric(
      delta * static_cast<std::int64_t>(numerator),
      static_cast<std::int64_t>(denominator));
  return static_cast<std::uint16_t>(std::clamp<std::int64_t>(
      static_cast<std::int64_t>(from) + offset,
      kCreativeTerrainMinimumHeightCells, kCreativeTerrainMaximumHeightCells));
}

[[nodiscard]] bool appendCenterlineSegment(
    std::array<CenterlineCell, kCreativeTerrainControlCapacity>& centerline,
    std::size_t& centerlineCount,
    const CreativeTerrainPathPoint& from,
    const CreativeTerrainPathPoint& to,
    std::uint64_t progressBeforeQ16,
    std::uint64_t segmentLengthQ16,
    bool skipFirst) noexcept {
  const CreativeTerrainGridLine line =
      rasterizeCreativeTerrainGridLine(from.coord, to.coord);
  if (!line.accepted || line.count < 2U) {
    return false;
  }
  const std::uint64_t steps = line.count - 1U;
  for (std::size_t index = 0U; index < line.items().size(); ++index) {
    if (!(skipFirst && index == 0U)) {
      if (centerlineCount >= centerline.size()) {
        return false;
      }
      CenterlineCell& cell = centerline[centerlineCount++];
      cell.coord = line.items()[index];
      cell.progressQ16 = progressBeforeQ16 +
                         (segmentLengthQ16 * index + steps / 2U) / steps;
      cell.fallbackHeightCells = interpolatedHeight(
          from.heightCells, to.heightCells, index, steps);
    }
  }
  return true;
}

[[nodiscard]] bool containsCoord(
    std::span<const CreativeTerrainCoord2> coords,
    CreativeTerrainCoord2 coord) noexcept {
  return std::find(coords.begin(), coords.end(), coord) != coords.end();
}

[[nodiscard]] bool offsetCoord(CreativeTerrainCoord2 center,
                               std::int32_t dx,
                               std::int32_t dz,
                               CreativeTerrainCoord2& output) noexcept {
  const std::int64_t x = static_cast<std::int64_t>(center.x) + dx;
  const std::int64_t z = static_cast<std::int64_t>(center.z) + dz;
  if (x < std::numeric_limits<std::int32_t>::min() ||
      x > std::numeric_limits<std::int32_t>::max() ||
      z < std::numeric_limits<std::int32_t>::min() ||
      z > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  output = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(z)};
  return true;
}

[[nodiscard]] std::int32_t crossSectionWeightQ15(
    CreativeTerrainPathKind kind,
    std::uint64_t distanceSquared,
    std::uint16_t halfWidthCells) noexcept {
  std::int32_t bell = kCreativeTerrainQ15One;
  if (halfWidthCells > 0U) {
    const std::uint64_t distanceQ16 =
        creativeTerrainIntegerSquareRoot(distanceSquared << 32U);
    const std::uint32_t normalizedQ16 = static_cast<std::uint32_t>(
        std::min<std::uint64_t>(
            kCreativeTerrainQ16One,
            (distanceQ16 + halfWidthCells / 2U) / halfWidthCells));
    bell = creativeTerrainCosineBellQ15(normalizedQ16);
  }
  switch (kind) {
    case CreativeTerrainPathKind::Road:
      return kCreativeTerrainQ15One;
    case CreativeTerrainPathKind::River:
      return -bell;
    case CreativeTerrainPathKind::Ridge:
      return bell;
    case CreativeTerrainPathKind::Trench:
      return -kCreativeTerrainQ15One;
    case CreativeTerrainPathKind::Count:
      break;
  }
  return 0;
}

}  // namespace

std::string_view toString(CreativeTerrainPathKind value) noexcept {
  constexpr std::array names{"ROAD", "RIVER", "RIDGE", "TRENCH"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

std::string_view toString(CreativeTerrainPathElevation value) noexcept {
  constexpr std::array names{"FOLLOW", "LEVEL", "GRADE"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

std::string_view toString(CreativeTerrainPathWidth value) noexcept {
  constexpr std::array names{"1 CELL", "3 CELLS", "5 CELLS", "7 CELLS"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

std::string_view toString(CreativeTerrainPathAmplitude value) noexcept {
  constexpr std::array names{"1 CELL", "2 CELLS", "4 CELLS", "8 CELLS"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "INVALID";
}

std::string_view toString(CreativeTerrainPathPlanStatus value) noexcept {
  constexpr std::array names{"NotRequested",      "InvalidRequest",
                             "CoordinateOverflow", "PathTooLong",
                             "CapacityExceeded",  "NoChange",
                             "Ready"};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < names.size() ? names[index] : "Unknown";
}

std::uint16_t creativeTerrainPathHalfWidthCells(
    CreativeTerrainPathWidth value) noexcept {
  constexpr std::array<std::uint16_t, 4U> values{{0U, 1U, 2U, 3U}};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < values.size() ? values[index] : 0U;
}

std::uint16_t creativeTerrainPathWidthCells(
    CreativeTerrainPathWidth value) noexcept {
  const std::size_t index = static_cast<std::size_t>(value);
  return index < static_cast<std::size_t>(CreativeTerrainPathWidth::Count)
             ? static_cast<std::uint16_t>(index * 2U + 1U)
             : 0U;
}

std::uint16_t creativeTerrainPathAmplitudeCells(
    CreativeTerrainPathAmplitude value) noexcept {
  constexpr std::array<std::uint16_t, 4U> values{{1U, 2U, 4U, 8U}};
  const std::size_t index = static_cast<std::size_t>(value);
  return index < values.size() ? values[index] : 0U;
}

bool creativeTerrainPathUsesDepth(CreativeTerrainPathKind value) noexcept {
  return value == CreativeTerrainPathKind::River ||
         value == CreativeTerrainPathKind::Trench;
}

CreativeTerrainPathPlan buildCreativeTerrainPathPlan(
    const CreativeTerrainPathRequest& request) noexcept {
  CreativeTerrainPathPlan plan;
  plan.requested = true;
  if (!validRequest(request)) {
    return rejectedPlan(plan, CreativeTerrainPathPlanStatus::InvalidRequest,
                        "creative_terrain_path_invalid_request");
  }

  std::array<std::uint64_t, kCreativeTerrainPathPointCapacity> segmentLengths{};
  std::uint64_t totalLengthQ16 = 0U;
  for (std::size_t index = 1U; index < request.points.size(); ++index) {
    std::int32_t dx = 0;
    std::int32_t dz = 0;
    std::uint32_t steps = 0U;
    std::uint64_t lengthQ16 = 0U;
    if (!segmentFacts(request.points[index - 1U].coord,
                      request.points[index].coord, dx, dz, steps,
                      lengthQ16)) {
      return rejectedPlan(plan, CreativeTerrainPathPlanStatus::PathTooLong,
                          "creative_terrain_path_segment_too_long");
    }
    segmentLengths[index - 1U] = lengthQ16;
    if (totalLengthQ16 >
        std::numeric_limits<std::uint64_t>::max() - lengthQ16) {
      return rejectedPlan(plan,
                          CreativeTerrainPathPlanStatus::CoordinateOverflow,
                          "creative_terrain_path_length_overflow");
    }
    totalLengthQ16 += lengthQ16;
  }

  std::array<CenterlineCell, kCreativeTerrainControlCapacity> centerline{};
  std::size_t centerlineCount = 0U;
  std::uint64_t progressQ16 = 0U;
  for (std::size_t index = 1U; index < request.points.size(); ++index) {
    if (!appendCenterlineSegment(centerline, centerlineCount,
                                 request.points[index - 1U],
                                 request.points[index], progressQ16,
                                 segmentLengths[index - 1U], index > 1U)) {
      return rejectedPlan(plan, CreativeTerrainPathPlanStatus::PathTooLong,
                          "creative_terrain_path_centerline_capacity_exceeded");
    }
    progressQ16 += segmentLengths[index - 1U];
  }
  plan.centerlineCount = static_cast<std::uint16_t>(centerlineCount);

  std::array<CreativeTerrainCoord2, kCreativeTerrainControlCapacity> candidates{};
  std::size_t candidateCount = 0U;
  const std::int32_t halfWidth = request.halfWidthCells;
  for (std::size_t index = 0U; index < centerlineCount; ++index) {
    for (std::int32_t dz = -halfWidth; dz <= halfWidth; ++dz) {
      for (std::int32_t dx = -halfWidth; dx <= halfWidth; ++dx) {
        if (dx * dx + dz * dz > halfWidth * halfWidth) {
          continue;
        }
        CreativeTerrainCoord2 coord{};
        if (!offsetCoord(centerline[index].coord, dx, dz, coord)) {
          return rejectedPlan(
              plan, CreativeTerrainPathPlanStatus::CoordinateOverflow,
              "creative_terrain_path_coordinate_overflow");
        }
        if (containsCoord(std::span{candidates.data(), candidateCount}, coord)) {
          continue;
        }
        if (candidateCount >= candidates.size()) {
          return rejectedPlan(
              plan, CreativeTerrainPathPlanStatus::CapacityExceeded,
              "creative_terrain_path_candidate_capacity_exceeded");
        }
        candidates[candidateCount++] = coord;
      }
    }
  }
  std::sort(candidates.begin(), candidates.begin() + candidateCount, coordLess);

  std::size_t newControlCount = 0U;
  for (std::size_t index = 0U; index < candidateCount; ++index) {
    if (request.field->controlAt(candidates[index]) == nullptr) {
      ++newControlCount;
    }
  }
  if (request.field->controlCount() + newControlCount >
      kCreativeTerrainControlCapacity) {
    return rejectedPlan(plan, CreativeTerrainPathPlanStatus::CapacityExceeded,
                        "creative_terrain_path_field_capacity_exceeded");
  }

  for (std::size_t candidateIndex = 0U; candidateIndex < candidateCount;
       ++candidateIndex) {
    const CreativeTerrainCoord2 coord = candidates[candidateIndex];
    const CenterlineCell* nearest = nullptr;
    std::uint64_t nearestSquared = std::numeric_limits<std::uint64_t>::max();
    for (std::size_t centerIndex = 0U; centerIndex < centerlineCount;
         ++centerIndex) {
      const std::int64_t dx =
          static_cast<std::int64_t>(coord.x) - centerline[centerIndex].coord.x;
      const std::int64_t dz =
          static_cast<std::int64_t>(coord.z) - centerline[centerIndex].coord.z;
      if (std::abs(dx) > halfWidth || std::abs(dz) > halfWidth) {
        continue;
      }
      const std::uint64_t squared = static_cast<std::uint64_t>(dx * dx + dz * dz);
      if (nearest == nullptr || squared < nearestSquared ||
          (squared == nearestSquared &&
           centerline[centerIndex].progressQ16 < nearest->progressQ16)) {
        nearest = &centerline[centerIndex];
        nearestSquared = squared;
      }
    }
    if (nearest == nullptr) {
      return rejectedPlan(plan, CreativeTerrainPathPlanStatus::InvalidRequest,
                          "creative_terrain_path_nearest_centerline_missing");
    }

    std::uint16_t baseline = nearest->fallbackHeightCells;
    switch (request.elevation) {
      case CreativeTerrainPathElevation::Follow: {
        const CreativeTerrainHeightSample sample =
            sampleCreativeTerrainHeight(*request.field, nearest->coord);
        if (sample.present) {
          baseline = sample.heightCells;
        }
        break;
      }
      case CreativeTerrainPathElevation::Level:
        baseline = request.points.front().heightCells;
        break;
      case CreativeTerrainPathElevation::Grade:
        baseline = interpolatedHeight(request.points.front().heightCells,
                                      request.points.back().heightCells,
                                      nearest->progressQ16, totalLengthQ16);
        break;
      case CreativeTerrainPathElevation::Count:
        break;
    }

    const std::int32_t weight = crossSectionWeightQ15(
        request.kind, nearestSquared, request.halfWidthCells);
    const std::int64_t delta = creativeTerrainRoundDivideSymmetric(
        static_cast<std::int64_t>(request.amplitudeCells) * weight,
        kCreativeTerrainQ15One);
    const std::uint16_t height = static_cast<std::uint16_t>(
        std::clamp<std::int64_t>(static_cast<std::int64_t>(baseline) + delta,
                                 kCreativeTerrainMinimumHeightCells,
                                 kCreativeTerrainMaximumHeightCells));
    const CreativeTerrainControlPoint* existing =
        request.field->controlAt(coord);
    const std::uint16_t radius =
        existing != nullptr
            ? existing->radiusCells
            : static_cast<std::uint16_t>(request.halfWidthCells + 1U);
    const CreativeTerrainControlPoint control{coord, height, radius};
    if (!isValidCreativeTerrainControlPoint(control)) {
      return rejectedPlan(plan,
                          CreativeTerrainPathPlanStatus::CoordinateOverflow,
                          "creative_terrain_path_control_invalid");
    }
    plan.controls[plan.controlCount++] = control;
    if (existing == nullptr || *existing != control) {
      plan.edits[plan.editCount++] = {CreativeTerrainEditKind::Upsert, control};
    }
  }

  plan.minimumCoord = candidates.front();
  plan.maximumCoord = candidates.front();
  for (std::size_t index = 1U; index < candidateCount; ++index) {
    plan.minimumCoord.x = std::min(plan.minimumCoord.x, candidates[index].x);
    plan.minimumCoord.z = std::min(plan.minimumCoord.z, candidates[index].z);
    plan.maximumCoord.x = std::max(plan.maximumCoord.x, candidates[index].x);
    plan.maximumCoord.z = std::max(plan.maximumCoord.z, candidates[index].z);
  }
  plan.accepted = true;
  if (plan.editCount == 0U) {
    plan.status = CreativeTerrainPathPlanStatus::NoChange;
    plan.reasonCode = "creative_terrain_path_no_change";
    return plan;
  }
  plan.status = CreativeTerrainPathPlanStatus::Ready;
  plan.reasonCode = "creative_terrain_path_ready";
  return plan;
}

}  // namespace iggy3d::creative
