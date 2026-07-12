#include "app/iggy3d/creative/tools/TerrainRegion.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace iggy3d::creative {
namespace {

[[nodiscard]] constexpr bool validOperation(
    CreativeTerrainRegionOperation operation) noexcept {
  return static_cast<std::size_t>(operation) <
         static_cast<std::size_t>(CreativeTerrainRegionOperation::Count);
}

[[nodiscard]] constexpr bool coordLess(CreativeTerrainCoord2 lhs,
                                       CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z < rhs.z || (lhs.z == rhs.z && lhs.x < rhs.x);
}

[[nodiscard]] bool validControls(
    std::span<const CreativeTerrainControlPoint> controls) noexcept {
  if (controls.size() > kCreativeTerrainControlCapacity) {
    return false;
  }
  for (std::size_t index = 0U; index < controls.size(); ++index) {
    if (!isValidCreativeTerrainControlPoint(controls[index]) ||
        (index > 0U &&
         !coordLess(controls[index - 1U].coord, controls[index].coord))) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool insideRadius(CreativeTerrainCoord2 center,
                                CreativeTerrainCoord2 coord,
                                std::uint16_t radiusCells) noexcept {
  const std::int64_t dx = static_cast<std::int64_t>(center.x) - coord.x;
  const std::int64_t dz = static_cast<std::int64_t>(center.z) - coord.z;
  const std::int64_t radius = radiusCells;
  if (dx < -radius || dx > radius || dz < -radius || dz > radius) {
    return false;
  }
  return dx * dx + dz * dz <= radius * radius;
}

[[nodiscard]] std::uint16_t moveToward(std::uint16_t current,
                                       std::uint16_t target,
                                       std::uint16_t amount) noexcept {
  const int delta = static_cast<int>(target) - static_cast<int>(current);
  if (delta == 0) {
    return current;
  }
  const int step = std::min(std::abs(delta), static_cast<int>(amount));
  return static_cast<std::uint16_t>(
      static_cast<int>(current) + (delta > 0 ? step : -step));
}

[[nodiscard]] std::uint16_t smoothTargetHeight(
    std::span<const CreativeTerrainControlPoint> controls,
    const CreativeTerrainControlPoint& control) noexcept {
  std::uint32_t heightSum = 0U;
  std::uint32_t neighborCount = 0U;
  for (const CreativeTerrainControlPoint& neighbor : controls) {
    if (!insideRadius(control.coord, neighbor.coord, control.radiusCells)) {
      continue;
    }
    heightSum += neighbor.heightCells;
    ++neighborCount;
  }
  return neighborCount == 0U
             ? control.heightCells
             : static_cast<std::uint16_t>(
                   (heightSum + neighborCount / 2U) / neighborCount);
}

[[nodiscard]] std::uint16_t editedHeight(
    const CreativeTerrainRegionRequest& request,
    const CreativeTerrainControlPoint& control) noexcept {
  switch (request.operation) {
    case CreativeTerrainRegionOperation::Raise:
      return static_cast<std::uint16_t>(std::min<std::uint32_t>(
          kCreativeTerrainMaximumHeightCells,
          static_cast<std::uint32_t>(control.heightCells) +
              request.amountCells));
    case CreativeTerrainRegionOperation::Lower:
      return static_cast<std::uint16_t>(std::max<std::int32_t>(
          kCreativeTerrainMinimumHeightCells,
          static_cast<std::int32_t>(control.heightCells) -
              request.amountCells));
    case CreativeTerrainRegionOperation::Flatten:
      return request.targetHeightCells;
    case CreativeTerrainRegionOperation::Smooth:
      return moveToward(control.heightCells,
                        smoothTargetHeight(request.controls, control),
                        request.amountCells);
    case CreativeTerrainRegionOperation::Erase:
    case CreativeTerrainRegionOperation::Count:
      return control.heightCells;
  }
  return control.heightCells;
}

}  // namespace

std::string_view toString(
    CreativeTerrainRegionOperation operation) noexcept {
  switch (operation) {
    case CreativeTerrainRegionOperation::Raise: return "RAISE";
    case CreativeTerrainRegionOperation::Lower: return "LOWER";
    case CreativeTerrainRegionOperation::Flatten: return "FLATTEN";
    case CreativeTerrainRegionOperation::Smooth: return "SMOOTH";
    case CreativeTerrainRegionOperation::Erase: return "ERASE";
    case CreativeTerrainRegionOperation::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeTerrainRegionAmount amount) noexcept {
  switch (amount) {
    case CreativeTerrainRegionAmount::OneCell: return "1 CELL";
    case CreativeTerrainRegionAmount::TwoCells: return "2 CELLS";
    case CreativeTerrainRegionAmount::FourCells: return "4 CELLS";
    case CreativeTerrainRegionAmount::EightCells: return "8 CELLS";
    case CreativeTerrainRegionAmount::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeTerrainRegionPlanStatus status) noexcept {
  switch (status) {
    case CreativeTerrainRegionPlanStatus::NotRequested: return "NotRequested";
    case CreativeTerrainRegionPlanStatus::InvalidRequest: return "InvalidRequest";
    case CreativeTerrainRegionPlanStatus::NoControlsInRegion:
      return "NoControlsInRegion";
    case CreativeTerrainRegionPlanStatus::NoChange: return "NoChange";
    case CreativeTerrainRegionPlanStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainRegionPlanStatus::Ready: return "Ready";
  }
  return "Unknown";
}

bool creativeTerrainRegionUsesAmount(
    CreativeTerrainRegionOperation operation) noexcept {
  switch (operation) {
    case CreativeTerrainRegionOperation::Raise:
    case CreativeTerrainRegionOperation::Lower:
    case CreativeTerrainRegionOperation::Smooth:
      return true;
    case CreativeTerrainRegionOperation::Flatten:
    case CreativeTerrainRegionOperation::Erase:
    case CreativeTerrainRegionOperation::Count:
      return false;
  }
  return false;
}

bool creativeTerrainRegionUsesTargetHeight(
    CreativeTerrainRegionOperation operation) noexcept {
  return operation == CreativeTerrainRegionOperation::Flatten;
}

std::uint16_t creativeTerrainRegionAmountCells(
    CreativeTerrainRegionAmount amount) noexcept {
  constexpr std::array<std::uint16_t,
                       static_cast<std::size_t>(
                           CreativeTerrainRegionAmount::Count)>
      values{{1U, 2U, 4U, 8U}};
  const std::size_t index = static_cast<std::size_t>(amount);
  return index < values.size() ? values[index] : 0U;
}

bool creativeTerrainCoordInsideRegion(
    CreativeTerrainCoord2 coord,
    CreativeTerrainCoord2 minimumCoord,
    CreativeTerrainCoord2 maximumCoord) noexcept {
  return coord.x >= minimumCoord.x && coord.x <= maximumCoord.x &&
         coord.z >= minimumCoord.z && coord.z <= maximumCoord.z;
}

CreativeTerrainRegionPlan buildCreativeTerrainRegionPlan(
    const CreativeTerrainRegionRequest& request) noexcept {
  CreativeTerrainRegionPlan plan;
  plan.requested = true;
  plan.minimumCoord = request.minimumCoord;
  plan.maximumCoord = request.maximumCoord;
  if (!validOperation(request.operation) || !validControls(request.controls) ||
      request.minimumCoord.x > request.maximumCoord.x ||
      request.minimumCoord.z > request.maximumCoord.z ||
      request.amountCells == 0U ||
      request.amountCells > kCreativeTerrainMaximumHeightCells ||
      request.targetHeightCells < kCreativeTerrainMinimumHeightCells ||
      request.targetHeightCells > kCreativeTerrainMaximumHeightCells) {
    plan.status = CreativeTerrainRegionPlanStatus::InvalidRequest;
    plan.reasonCode = "creative_terrain_region_invalid_request";
    return plan;
  }

  for (const CreativeTerrainControlPoint& control : request.controls) {
    if (!creativeTerrainCoordInsideRegion(
            control.coord, request.minimumCoord, request.maximumCoord)) {
      continue;
    }
    ++plan.affectedControlCount;
    CreativeTerrainControlEdit edit;
    edit.control = control;
    if (request.operation == CreativeTerrainRegionOperation::Erase) {
      edit.kind = CreativeTerrainEditKind::Remove;
    } else {
      edit.kind = CreativeTerrainEditKind::Upsert;
      edit.control.heightCells = editedHeight(request, control);
      if (edit.control.heightCells == control.heightCells) {
        continue;
      }
    }
    if (plan.editCount >= plan.edits.size()) {
      plan.editCount = 0U;
      plan.status = CreativeTerrainRegionPlanStatus::CapacityExceeded;
      plan.reasonCode = "creative_terrain_region_capacity_exceeded";
      return plan;
    }
    plan.edits[plan.editCount++] = edit;
  }

  if (plan.affectedControlCount == 0U) {
    plan.status = CreativeTerrainRegionPlanStatus::NoControlsInRegion;
    plan.reasonCode = "creative_terrain_region_no_controls";
    return plan;
  }
  plan.accepted = true;
  if (plan.editCount == 0U) {
    plan.status = CreativeTerrainRegionPlanStatus::NoChange;
    plan.reasonCode = "creative_terrain_region_no_change";
    return plan;
  }
  plan.status = CreativeTerrainRegionPlanStatus::Ready;
  plan.reasonCode = "creative_terrain_region_ready";
  return plan;
}

}  // namespace iggy3d::creative
