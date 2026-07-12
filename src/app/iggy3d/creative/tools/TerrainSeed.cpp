#include "app/iggy3d/creative/tools/TerrainSeed.hpp"

#include <array>
#include <cstdint>
#include <limits>

namespace iggy3d::creative {
namespace {

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

[[nodiscard]] bool validRequest(
    const CreativeTerrainSeedRequest& request) noexcept {
  return request.field != nullptr && request.field->validateInvariants() &&
         request.operation < CreativeTerrainSeedOperation::Count &&
         request.seedRadiusCells >= 1U && request.seedRadiusCells <= 8U &&
         request.spacingCells >= 1U && request.spacingCells <= 4U &&
         request.fallbackHeightCells >= kCreativeTerrainMinimumHeightCells &&
         request.fallbackHeightCells <= kCreativeTerrainMaximumHeightCells &&
         request.controlRadiusCells >= kCreativeTerrainMinimumRadiusCells &&
         request.controlRadiusCells <= kCreativeTerrainMaximumRadiusCells;
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

[[nodiscard]] CreativeTerrainSeedPlan invalidPlan(
    CreativeTerrainSeedPlan plan) noexcept {
  plan.editCount = 0U;
  plan.status = CreativeTerrainSeedPlanStatus::InvalidRequest;
  plan.reasonCode = "creative_terrain_seed_invalid_request";
  return plan;
}

[[nodiscard]] CreativeTerrainSeedPlan buildSeedMissingPlan(
    const CreativeTerrainSeedRequest& request,
    CreativeTerrainSeedPlan plan) noexcept {
  const std::int32_t radius = request.seedRadiusCells;
  const std::int32_t spacing = request.spacingCells;
  for (std::int32_t dz = -radius; dz <= radius; ++dz) {
    for (std::int32_t dx = -radius; dx <= radius; ++dx) {
      if (dx % spacing != 0 || dz % spacing != 0 ||
          !insideRadius({}, {dx, dz}, request.seedRadiusCells)) {
        continue;
      }
      ++plan.candidateCount;
      CreativeTerrainCoord2 coord{};
      if (!offsetCoord(request.center, dx, dz, coord)) {
        return invalidPlan(plan);
      }
      if (request.field->controlAt(coord) != nullptr) {
        continue;
      }
      const CreativeTerrainHeightSample sampled =
          sampleCreativeTerrainHeight(*request.field, coord);
      const CreativeTerrainControlPoint control{
          coord,
          sampled.present ? sampled.heightCells : request.fallbackHeightCells,
          request.controlRadiusCells};
      if (!isValidCreativeTerrainControlPoint(control)) {
        return invalidPlan(plan);
      }
      if (request.field->controlCount() + plan.editCount >=
          kCreativeTerrainControlCapacity) {
        plan.editCount = 0U;
        plan.status = CreativeTerrainSeedPlanStatus::CapacityExceeded;
        plan.reasonCode = "creative_terrain_seed_capacity_exceeded";
        return plan;
      }
      plan.edits[plan.editCount++] =
          {CreativeTerrainEditKind::Upsert, control};
    }
  }
  return plan;
}

[[nodiscard]] CreativeTerrainSeedPlan buildClearPlan(
    const CreativeTerrainSeedRequest& request,
    CreativeTerrainSeedPlan plan) noexcept {
  for (const CreativeTerrainControlPoint& control : request.field->controls()) {
    if (!insideRadius(request.center, control.coord,
                      request.seedRadiusCells)) {
      continue;
    }
    ++plan.candidateCount;
    plan.edits[plan.editCount++] =
        {CreativeTerrainEditKind::Remove, control};
  }
  return plan;
}

}  // namespace

std::string_view toString(CreativeTerrainRodStampMode mode) noexcept {
  switch (mode) {
    case CreativeTerrainRodStampMode::Single:
      return "SINGLE";
    case CreativeTerrainRodStampMode::Seed:
      return "SEED";
    case CreativeTerrainRodStampMode::Count:
      break;
  }
  return "INVALID";
}

std::string_view toString(CreativeTerrainSeedRadius radius) noexcept {
  switch (radius) {
    case CreativeTerrainSeedRadius::OneCell:
      return "1 CELL";
    case CreativeTerrainSeedRadius::TwoCells:
      return "2 CELLS";
    case CreativeTerrainSeedRadius::FourCells:
      return "4 CELLS";
    case CreativeTerrainSeedRadius::EightCells:
      return "8 CELLS";
    case CreativeTerrainSeedRadius::Count:
      break;
  }
  return "INVALID";
}

std::string_view toString(CreativeTerrainSeedSpacing spacing) noexcept {
  switch (spacing) {
    case CreativeTerrainSeedSpacing::OneCell:
      return "1 CELL";
    case CreativeTerrainSeedSpacing::TwoCells:
      return "2 CELLS";
    case CreativeTerrainSeedSpacing::FourCells:
      return "4 CELLS";
    case CreativeTerrainSeedSpacing::Count:
      break;
  }
  return "INVALID";
}

std::string_view toString(CreativeTerrainSeedPlanStatus status) noexcept {
  switch (status) {
    case CreativeTerrainSeedPlanStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainSeedPlanStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeTerrainSeedPlanStatus::NoChange:
      return "NoChange";
    case CreativeTerrainSeedPlanStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainSeedPlanStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

std::uint16_t creativeTerrainSeedRadiusCells(
    CreativeTerrainSeedRadius radius) noexcept {
  constexpr std::array<std::uint16_t,
                       static_cast<std::size_t>(CreativeTerrainSeedRadius::Count)>
      values{{1U, 2U, 4U, 8U}};
  const std::size_t index = static_cast<std::size_t>(radius);
  return index < values.size() ? values[index] : 0U;
}

std::uint16_t creativeTerrainSeedSpacingCells(
    CreativeTerrainSeedSpacing spacing) noexcept {
  constexpr std::array<
      std::uint16_t,
      static_cast<std::size_t>(CreativeTerrainSeedSpacing::Count)>
      values{{1U, 2U, 4U}};
  const std::size_t index = static_cast<std::size_t>(spacing);
  return index < values.size() ? values[index] : 0U;
}

CreativeTerrainSeedPlan buildCreativeTerrainSeedPlan(
    const CreativeTerrainSeedRequest& request) noexcept {
  CreativeTerrainSeedPlan plan;
  plan.requested = true;
  if (!validRequest(request)) {
    return invalidPlan(plan);
  }
  switch (request.operation) {
    case CreativeTerrainSeedOperation::SeedMissing:
      plan = buildSeedMissingPlan(request, plan);
      break;
    case CreativeTerrainSeedOperation::Clear:
      plan = buildClearPlan(request, plan);
      break;
    case CreativeTerrainSeedOperation::Count:
      return invalidPlan(plan);
  }
  if (plan.status == CreativeTerrainSeedPlanStatus::InvalidRequest ||
      plan.status == CreativeTerrainSeedPlanStatus::CapacityExceeded) {
    return plan;
  }
  plan.accepted = true;
  if (plan.items().empty()) {
    plan.status = CreativeTerrainSeedPlanStatus::NoChange;
    plan.reasonCode = "creative_terrain_seed_no_change";
    return plan;
  }
  plan.status = CreativeTerrainSeedPlanStatus::Ready;
  plan.reasonCode = "creative_terrain_seed_ready";
  return plan;
}

}  // namespace iggy3d::creative
