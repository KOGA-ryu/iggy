#include "app/iggy3d/creative/tools/TerrainPaint.hpp"

#include <algorithm>
#include <limits>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool coordLess(CreativeTerrainCoord2 lhs,
                             CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z != rhs.z ? lhs.z < rhs.z : lhs.x < rhs.x;
}

[[nodiscard]] bool surfaceContains(
    std::span<const CreativeTerrainColumn> columns,
    CreativeTerrainCoord2 coord) noexcept {
  const auto found = std::lower_bound(
      columns.begin(), columns.end(), coord,
      [](const CreativeTerrainColumn& column, CreativeTerrainCoord2 candidate) {
        return coordLess(column.coord, candidate);
      });
  return found != columns.end() && found->coord == coord;
}

[[nodiscard]] bool surfaceColumnsAreCanonical(
    std::span<const CreativeTerrainColumn> columns) noexcept {
  for (std::size_t index = 0U; index < columns.size(); ++index) {
    if (columns[index].heightCells == 0U ||
        (index > 0U &&
         !coordLess(columns[index - 1U].coord, columns[index].coord))) {
      return false;
    }
  }
  return true;
}

}  // namespace

std::uint16_t creativeTerrainPaintRadiusCells(
    CreativeTerrainPaintRadius radius) noexcept {
  switch (radius) {
    case CreativeTerrainPaintRadius::OneCell: return 1U;
    case CreativeTerrainPaintRadius::TwoCells: return 2U;
    case CreativeTerrainPaintRadius::FourCells: return 4U;
    case CreativeTerrainPaintRadius::EightCells: return 8U;
    case CreativeTerrainPaintRadius::Count: break;
  }
  return 0U;
}

std::string_view toString(CreativeTerrainPaintRadius radius) noexcept {
  switch (radius) {
    case CreativeTerrainPaintRadius::OneCell: return "1 CELL";
    case CreativeTerrainPaintRadius::TwoCells: return "2 CELLS";
    case CreativeTerrainPaintRadius::FourCells: return "4 CELLS";
    case CreativeTerrainPaintRadius::EightCells: return "8 CELLS";
    case CreativeTerrainPaintRadius::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeTerrainPaintPlanStatus status) noexcept {
  switch (status) {
    case CreativeTerrainPaintPlanStatus::NotRequested: return "NotRequested";
    case CreativeTerrainPaintPlanStatus::InvalidRequest: return "InvalidRequest";
    case CreativeTerrainPaintPlanStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainPaintPlanStatus::NoSurface: return "NoSurface";
    case CreativeTerrainPaintPlanStatus::NoChange: return "NoChange";
    case CreativeTerrainPaintPlanStatus::Ready: return "Ready";
  }
  return "Unknown";
}

CreativeTerrainPaintPlan buildCreativeTerrainPaintPlan(
    const CreativeTerrainPaintRequest& request) noexcept {
  CreativeTerrainPaintPlan plan;
  plan.requested = true;
  plan.center = request.center;
  plan.material = request.material;
  plan.radiusCells = request.radiusCells;
  if (request.materialField == nullptr ||
      !request.materialField->validateInvariants() ||
      !isValidCreativeTerrainMaterial(request.material) ||
      request.radiusCells == 0U || request.radiusCells > 8U ||
      !surfaceColumnsAreCanonical(request.surfaceColumns)) {
    plan.status = CreativeTerrainPaintPlanStatus::InvalidRequest;
    plan.reasonCode = "creative_terrain_paint_invalid_request";
    return plan;
  }

  const std::int64_t radius = request.radiusCells;
  const std::int64_t radiusSquared = radius * radius;
  for (std::int64_t dz = -radius; dz <= radius; ++dz) {
    for (std::int64_t dx = -radius; dx <= radius; ++dx) {
      if (dx * dx + dz * dz > radiusSquared) {
        continue;
      }
      const std::int64_t x = static_cast<std::int64_t>(request.center.x) + dx;
      const std::int64_t z = static_cast<std::int64_t>(request.center.z) + dz;
      if (x < std::numeric_limits<std::int32_t>::min() ||
          x > std::numeric_limits<std::int32_t>::max() ||
          z < std::numeric_limits<std::int32_t>::min() ||
          z > std::numeric_limits<std::int32_t>::max()) {
        plan.status = CreativeTerrainPaintPlanStatus::InvalidRequest;
        plan.reasonCode = "creative_terrain_paint_coordinate_overflow";
        return plan;
      }
      const CreativeTerrainCoord2 coord{static_cast<std::int32_t>(x),
                                        static_cast<std::int32_t>(z)};
      if (!surfaceContains(request.surfaceColumns, coord)) {
        continue;
      }
      ++plan.surfaceCellCount;
      if (request.materialField->materialAt(coord) == request.material) {
        continue;
      }
      if (plan.editCount >= plan.edits.size()) {
        plan.editCount = 0U;
        plan.status = CreativeTerrainPaintPlanStatus::CapacityExceeded;
        plan.reasonCode = "creative_terrain_paint_capacity_exceeded";
        return plan;
      }
      CreativeTerrainMaterialEdit& edit = plan.edits[plan.editCount++];
      edit.kind = request.material == CreativeTerrainMaterial::Grass
                      ? CreativeTerrainMaterialEditKind::Clear
                      : CreativeTerrainMaterialEditKind::Set;
      edit.coord = coord;
      edit.material = request.material;
    }
  }
  if (plan.surfaceCellCount == 0U) {
    plan.status = CreativeTerrainPaintPlanStatus::NoSurface;
    plan.reasonCode = "creative_terrain_paint_no_surface";
    return plan;
  }
  plan.accepted = true;
  if (plan.editCount == 0U) {
    plan.status = CreativeTerrainPaintPlanStatus::NoChange;
    plan.reasonCode = "creative_terrain_paint_no_change";
    return plan;
  }
  plan.status = CreativeTerrainPaintPlanStatus::Ready;
  plan.reasonCode = "creative_terrain_paint_ready";
  return plan;
}

}  // namespace iggy3d::creative
