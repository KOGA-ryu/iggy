#include "app/iggy3d/creative/tools/TerrainSculpt.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>

namespace iggy3d::creative {
namespace {

[[nodiscard]] constexpr bool validMode(
    CreativeTerrainSculptMode mode) noexcept {
  return static_cast<std::size_t>(mode) <
         static_cast<std::size_t>(CreativeTerrainSculptMode::Count);
}

[[nodiscard]] constexpr bool coordLess(CreativeTerrainCoord2 lhs,
                                       CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z < rhs.z || (lhs.z == rhs.z && lhs.x < rhs.x);
}

[[nodiscard]] bool insideRadius(CreativeTerrainCoord2 first,
                                CreativeTerrainCoord2 second,
                                std::uint16_t radiusCells) noexcept {
  const std::int64_t dx = static_cast<std::int64_t>(first.x) - second.x;
  const std::int64_t dz = static_cast<std::int64_t>(first.z) - second.z;
  const std::int64_t radius = radiusCells;
  if (dx < -radius || dx > radius || dz < -radius || dz > radius) {
    return false;
  }
  return dx * dx + dz * dz <= radius * radius;
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

[[nodiscard]] std::uint16_t moveToward(std::uint16_t current,
                                       std::uint16_t target,
                                       std::uint16_t strength) noexcept {
  const int currentValue = current;
  const int targetValue = target;
  const int delta = targetValue - currentValue;
  if (delta == 0) {
    return current;
  }
  const int step = std::min(std::abs(delta), static_cast<int>(strength));
  return static_cast<std::uint16_t>(currentValue + (delta > 0 ? step : -step));
}

[[nodiscard]] std::uint16_t smoothTargetHeight(
    std::span<const CreativeTerrainControlPoint> controls,
    const CreativeTerrainControlPoint& control,
    std::uint16_t radiusCells) noexcept {
  std::uint32_t heightSum = 0U;
  std::uint32_t neighborCount = 0U;
  for (const CreativeTerrainControlPoint& neighbor : controls) {
    if (!insideRadius(control.coord, neighbor.coord, radiusCells)) {
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

}  // namespace

std::string_view toString(CreativeTerrainSculptMode mode) noexcept {
  switch (mode) {
    case CreativeTerrainSculptMode::Flatten:
      return "FLATTEN";
    case CreativeTerrainSculptMode::Smooth:
      return "SMOOTH";
    case CreativeTerrainSculptMode::Count:
      break;
  }
  return "INVALID";
}

std::string_view toString(CreativeTerrainSculptRadius radius) noexcept {
  switch (radius) {
    case CreativeTerrainSculptRadius::OneCell:
      return "1 CELL";
    case CreativeTerrainSculptRadius::TwoCells:
      return "2 CELLS";
    case CreativeTerrainSculptRadius::FourCells:
      return "4 CELLS";
    case CreativeTerrainSculptRadius::EightCells:
      return "8 CELLS";
    case CreativeTerrainSculptRadius::Count:
      break;
  }
  return "INVALID";
}

std::string_view toString(CreativeTerrainSculptStrength strength) noexcept {
  switch (strength) {
    case CreativeTerrainSculptStrength::OneCell:
      return "1 CELL";
    case CreativeTerrainSculptStrength::TwoCells:
      return "2 CELLS";
    case CreativeTerrainSculptStrength::FourCells:
      return "4 CELLS";
    case CreativeTerrainSculptStrength::EightCells:
      return "8 CELLS";
    case CreativeTerrainSculptStrength::Count:
      break;
  }
  return "INVALID";
}

std::string_view toString(CreativeTerrainSculptPlanStatus status) noexcept {
  switch (status) {
    case CreativeTerrainSculptPlanStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainSculptPlanStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeTerrainSculptPlanStatus::NoControlsInBrush:
      return "NoControlsInBrush";
    case CreativeTerrainSculptPlanStatus::NoChange:
      return "NoChange";
    case CreativeTerrainSculptPlanStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainSculptPlanStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

std::uint16_t creativeTerrainSculptRadiusCells(
    CreativeTerrainSculptRadius radius) noexcept {
  constexpr std::array<std::uint16_t,
                       static_cast<std::size_t>(
                           CreativeTerrainSculptRadius::Count)>
      values{{1U, 2U, 4U, 8U}};
  const std::size_t index = static_cast<std::size_t>(radius);
  return index < values.size() ? values[index] : 0U;
}

std::uint16_t creativeTerrainSculptStrengthCells(
    CreativeTerrainSculptStrength strength) noexcept {
  constexpr std::array<std::uint16_t,
                       static_cast<std::size_t>(
                           CreativeTerrainSculptStrength::Count)>
      values{{1U, 2U, 4U, 8U}};
  const std::size_t index = static_cast<std::size_t>(strength);
  return index < values.size() ? values[index] : 0U;
}

CreativeTerrainSculptPlan buildCreativeTerrainSculptPlan(
    const CreativeTerrainSculptRequest& request) noexcept {
  CreativeTerrainSculptPlan plan;
  plan.requested = true;
  if (!validMode(request.mode) || !validControls(request.controls) ||
      request.radiusCells < kCreativeTerrainMinimumRadiusCells ||
      request.radiusCells > kCreativeTerrainMaximumRadiusCells ||
      request.strengthCells == 0U ||
      request.strengthCells > kCreativeTerrainMaximumHeightCells ||
      request.targetHeightCells < kCreativeTerrainMinimumHeightCells ||
      request.targetHeightCells > kCreativeTerrainMaximumHeightCells) {
    plan.status = CreativeTerrainSculptPlanStatus::InvalidRequest;
    plan.reasonCode = "creative_terrain_sculpt_invalid_request";
    return plan;
  }

  for (const CreativeTerrainControlPoint& control : request.controls) {
    if (!insideRadius(request.center, control.coord, request.radiusCells)) {
      continue;
    }
    ++plan.affectedControlCount;
    const std::uint16_t target =
        request.mode == CreativeTerrainSculptMode::Flatten
            ? request.targetHeightCells
            : smoothTargetHeight(request.controls, control,
                                 request.radiusCells);
    const std::uint16_t nextHeight =
        moveToward(control.heightCells, target, request.strengthCells);
    if (nextHeight == control.heightCells) {
      continue;
    }
    if (plan.editCount >= plan.edits.size()) {
      plan.editCount = 0U;
      plan.status = CreativeTerrainSculptPlanStatus::CapacityExceeded;
      plan.reasonCode = "creative_terrain_sculpt_capacity_exceeded";
      return plan;
    }
    CreativeTerrainControlPoint adjusted = control;
    adjusted.heightCells = nextHeight;
    plan.edits[plan.editCount++] =
        {CreativeTerrainEditKind::Upsert, adjusted};
  }

  if (plan.affectedControlCount == 0U) {
    plan.status = CreativeTerrainSculptPlanStatus::NoControlsInBrush;
    plan.reasonCode = "creative_terrain_sculpt_no_controls";
    return plan;
  }
  plan.accepted = true;
  if (plan.editCount == 0U) {
    plan.status = CreativeTerrainSculptPlanStatus::NoChange;
    plan.reasonCode = "creative_terrain_sculpt_no_change";
    return plan;
  }
  plan.status = CreativeTerrainSculptPlanStatus::Ready;
  plan.reasonCode = "creative_terrain_sculpt_ready";
  return plan;
}

}  // namespace iggy3d::creative
