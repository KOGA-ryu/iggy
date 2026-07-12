#include "app/iggy3d/creative/tools/TerrainGrade.hpp"

#include "app/iggy3d/creative/tools/TerrainBrushKernel.hpp"

#include <algorithm>
#include <cstdint>

namespace iggy3d::creative {
namespace {

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

  const CreativeTerrainGridLine line =
      rasterizeCreativeTerrainGridLine(request.start, request.end);
  if (!line.accepted) {
    plan.status = CreativeTerrainGradePlanStatus::CapacityExceeded;
    plan.reasonCode = "creative_terrain_grade_capacity_exceeded";
    return plan;
  }

  const std::int64_t segmentCount = line.count - 1U;
  const std::int64_t heightDelta =
      static_cast<std::int64_t>(request.endHeightCells) -
      static_cast<std::int64_t>(request.startHeightCells);

  for (std::size_t index = 0U; index < line.items().size(); ++index) {
    const std::int64_t height =
        segmentCount == 0
            ? request.endHeightCells
            : static_cast<std::int64_t>(request.startHeightCells) +
                  creativeTerrainRoundDivideSymmetric(
                      heightDelta * static_cast<std::int64_t>(index),
                      segmentCount);
    const CreativeTerrainControlPoint control{
        line.items()[index],
        static_cast<std::uint16_t>(height), request.radiusCells};
    if (!isValidCreativeTerrainControlPoint(control)) {
      plan.editCount = 0U;
      plan.status = CreativeTerrainGradePlanStatus::InvalidRequest;
      plan.reasonCode = "creative_terrain_grade_invalid_coordinate";
      return plan;
    }
    plan.edits[plan.editCount++] =
        {CreativeTerrainEditKind::Upsert, control};
  }

  plan.accepted = true;
  plan.status = CreativeTerrainGradePlanStatus::Ready;
  plan.reasonCode = "creative_terrain_grade_ready";
  return plan;
}

}  // namespace iggy3d::creative
