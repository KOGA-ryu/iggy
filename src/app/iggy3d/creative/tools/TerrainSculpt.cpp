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

[[nodiscard]] constexpr bool validFalloff(
    CreativeTerrainSculptFalloff falloff) noexcept {
  return static_cast<std::size_t>(falloff) <
         static_cast<std::size_t>(CreativeTerrainSculptFalloff::Count);
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

[[nodiscard]] constexpr std::uint64_t integerSquareRoot(
    std::uint64_t value) noexcept {
  std::uint64_t result = 0U;
  std::uint64_t bit = std::uint64_t{1U} << 62U;
  while (bit > value) {
    bit >>= 2U;
  }
  while (bit != 0U) {
    if (value >= result + bit) {
      value -= result + bit;
      result = (result >> 1U) + bit;
    } else {
      result >>= 1U;
    }
    bit >>= 2U;
  }
  return result;
}

[[nodiscard]] std::uint32_t sculptFalloffWeight(
    CreativeTerrainSculptFalloff falloff,
    CreativeTerrainCoord2 center,
    CreativeTerrainCoord2 coord,
    std::uint16_t radiusCells) noexcept {
  constexpr std::uint32_t kWeightScale = 65'536U;
  constexpr std::uint32_t kDistanceScale = 256U;
  if (falloff == CreativeTerrainSculptFalloff::Uniform) {
    return kWeightScale;
  }

  const std::int64_t dx = static_cast<std::int64_t>(center.x) - coord.x;
  const std::int64_t dz = static_cast<std::int64_t>(center.z) - coord.z;
  const std::uint64_t distanceSquared =
      static_cast<std::uint64_t>(dx * dx + dz * dz);
  const std::uint64_t radiusSquared =
      static_cast<std::uint64_t>(radiusCells) * radiusCells;
  if (distanceSquared >= radiusSquared) {
    return 0U;
  }

  const std::uint32_t distance = static_cast<std::uint32_t>(
      integerSquareRoot(distanceSquared * kDistanceScale * kDistanceScale));
  const std::uint32_t radius = radiusCells * kDistanceScale;
  const std::uint32_t linear = static_cast<std::uint32_t>(
      (static_cast<std::uint64_t>(radius - distance) * kWeightScale +
       radius / 2U) /
      radius);
  if (falloff == CreativeTerrainSculptFalloff::Linear) {
    return linear;
  }

  const std::uint64_t scaleSquared =
      static_cast<std::uint64_t>(kWeightScale) * kWeightScale;
  const std::uint64_t smooth = static_cast<std::uint64_t>(linear) * linear *
                               (3U * kWeightScale - 2U * linear);
  return static_cast<std::uint32_t>((smooth + scaleSquared / 2U) /
                                    scaleSquared);
}

[[nodiscard]] std::uint16_t effectiveSculptStrength(
    const CreativeTerrainSculptRequest& request,
    CreativeTerrainCoord2 coord) noexcept {
  constexpr std::uint32_t kWeightScale = 65'536U;
  const std::uint32_t weight = sculptFalloffWeight(
      request.falloff, request.center, coord, request.radiusCells);
  return static_cast<std::uint16_t>(
      (static_cast<std::uint64_t>(request.strengthCells) * weight +
       kWeightScale / 2U) /
      kWeightScale);
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

[[nodiscard]] std::uint16_t sculptedHeight(
    const CreativeTerrainSculptRequest& request,
    const CreativeTerrainControlPoint& control) noexcept {
  const std::uint16_t strength =
      effectiveSculptStrength(request, control.coord);
  if (strength == 0U) {
    return control.heightCells;
  }
  switch (request.mode) {
    case CreativeTerrainSculptMode::Raise:
      return static_cast<std::uint16_t>(
          std::min(static_cast<int>(kCreativeTerrainMaximumHeightCells),
                   static_cast<int>(control.heightCells) + strength));
    case CreativeTerrainSculptMode::Lower:
      return static_cast<std::uint16_t>(
          std::max(static_cast<int>(kCreativeTerrainMinimumHeightCells),
                   static_cast<int>(control.heightCells) - strength));
    case CreativeTerrainSculptMode::Flatten:
      return moveToward(control.heightCells, request.targetHeightCells,
                        strength);
    case CreativeTerrainSculptMode::Smooth:
      return moveToward(
          control.heightCells,
          smoothTargetHeight(request.controls, control, request.radiusCells),
          strength);
    case CreativeTerrainSculptMode::Count:
      break;
  }
  return control.heightCells;
}

}  // namespace

std::string_view toString(CreativeTerrainSculptMode mode) noexcept {
  switch (mode) {
    case CreativeTerrainSculptMode::Raise:
      return "RAISE";
    case CreativeTerrainSculptMode::Lower:
      return "LOWER";
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

std::string_view toString(CreativeTerrainSculptFalloff falloff) noexcept {
  switch (falloff) {
    case CreativeTerrainSculptFalloff::Uniform:
      return "UNIFORM";
    case CreativeTerrainSculptFalloff::Linear:
      return "LINEAR";
    case CreativeTerrainSculptFalloff::Smooth:
      return "SMOOTH";
    case CreativeTerrainSculptFalloff::Count:
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

bool creativeTerrainSculptUsesTargetHeight(
    CreativeTerrainSculptMode mode) noexcept {
  switch (mode) {
    case CreativeTerrainSculptMode::Flatten:
      return true;
    case CreativeTerrainSculptMode::Raise:
    case CreativeTerrainSculptMode::Lower:
    case CreativeTerrainSculptMode::Smooth:
    case CreativeTerrainSculptMode::Count:
      return false;
  }
  return false;
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
  if (!validMode(request.mode) || !validFalloff(request.falloff) ||
      !validControls(request.controls) ||
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
    const std::uint16_t nextHeight = sculptedHeight(request, control);
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
