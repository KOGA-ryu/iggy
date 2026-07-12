#include "app/iggy3d/creative/tools/TerrainGrade.hpp"

#include <algorithm>
#include <cstdint>

namespace iggy3d::creative {
namespace {

[[nodiscard]] constexpr std::int64_t magnitude(
    std::int64_t value) noexcept {
  return value < 0 ? -value : value;
}

[[nodiscard]] constexpr std::int64_t divideNearest(
    std::int64_t numerator,
    std::int64_t denominator) noexcept {
  if (numerator < 0) {
    return -((-numerator + denominator / 2) / denominator);
  }
  return (numerator + denominator / 2) / denominator;
}

[[nodiscard]] constexpr bool validHeight(std::uint16_t height) noexcept {
  return height >= kCreativeTerrainMinimumHeightCells &&
         height <= kCreativeTerrainMaximumHeightCells;
}

[[nodiscard]] constexpr bool validRadius(std::uint16_t radius) noexcept {
  return radius >= kCreativeTerrainMinimumRadiusCells &&
         radius <= kCreativeTerrainMaximumRadiusCells;
}

}  // namespace

std::string_view toString(CreativeTerrainGradePlanStatus status) noexcept {
  switch (status) {
    case CreativeTerrainGradePlanStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainGradePlanStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeTerrainGradePlanStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainGradePlanStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

CreativeTerrainGradePlan buildCreativeTerrainGradePlan(
    const CreativeTerrainGradeRequest& request) noexcept {
  CreativeTerrainGradePlan plan;
  plan.requested = true;
  if (!validHeight(request.startHeightCells) ||
      !validHeight(request.endHeightCells) ||
      !validRadius(request.radiusCells)) {
    plan.status = CreativeTerrainGradePlanStatus::InvalidRequest;
    plan.reasonCode = "creative_terrain_grade_invalid_request";
    return plan;
  }

  const std::int64_t startX = request.start.x;
  const std::int64_t startZ = request.start.z;
  const std::int64_t endX = request.end.x;
  const std::int64_t endZ = request.end.z;
  const std::int64_t deltaX = magnitude(endX - startX);
  const std::int64_t deltaZ = magnitude(endZ - startZ);
  const std::int64_t segmentCount = std::max(deltaX, deltaZ);
  if (segmentCount >=
      static_cast<std::int64_t>(kCreativeTerrainGradeEditCapacity)) {
    plan.status = CreativeTerrainGradePlanStatus::CapacityExceeded;
    plan.reasonCode = "creative_terrain_grade_capacity_exceeded";
    return plan;
  }

  std::int64_t x = startX;
  std::int64_t z = startZ;
  const std::int64_t stepX = startX < endX ? 1 : -1;
  const std::int64_t stepZ = startZ < endZ ? 1 : -1;
  std::int64_t error = deltaX - deltaZ;
  const std::int64_t heightDelta =
      static_cast<std::int64_t>(request.endHeightCells) -
      static_cast<std::int64_t>(request.startHeightCells);

  for (std::int64_t index = 0; index <= segmentCount; ++index) {
    const std::int64_t height =
        segmentCount == 0
            ? request.endHeightCells
            : static_cast<std::int64_t>(request.startHeightCells) +
                  divideNearest(heightDelta * index, segmentCount);
    const CreativeTerrainControlPoint control{
        {static_cast<std::int32_t>(x), static_cast<std::int32_t>(z)},
        static_cast<std::uint16_t>(height), request.radiusCells};
    if (!isValidCreativeTerrainControlPoint(control)) {
      plan.editCount = 0U;
      plan.status = CreativeTerrainGradePlanStatus::InvalidRequest;
      plan.reasonCode = "creative_terrain_grade_invalid_coordinate";
      return plan;
    }
    plan.edits[plan.editCount++] =
        {CreativeTerrainEditKind::Upsert, control};
    if (x == endX && z == endZ) {
      break;
    }
    const std::int64_t twiceError = error * 2;
    if (twiceError > -deltaZ) {
      error -= deltaZ;
      x += stepX;
    }
    if (twiceError < deltaX) {
      error += deltaX;
      z += stepZ;
    }
  }

  plan.accepted = true;
  plan.status = CreativeTerrainGradePlanStatus::Ready;
  plan.reasonCode = "creative_terrain_grade_ready";
  return plan;
}

}  // namespace iggy3d::creative
