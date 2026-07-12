#include "app/iggy3d/creative/tools/TerrainPaint.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <vector>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool coordLess(CreativeTerrainCoord2 lhs,
                             CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z != rhs.z ? lhs.z < rhs.z : lhs.x < rhs.x;
}

[[nodiscard]] std::size_t surfaceIndex(
    std::span<const CreativeTerrainColumn> columns,
    CreativeTerrainCoord2 coord) noexcept {
  const auto found = std::lower_bound(
      columns.begin(), columns.end(), coord,
      [](const CreativeTerrainColumn& column, CreativeTerrainCoord2 candidate) {
        return coordLess(column.coord, candidate);
      });
  return found != columns.end() && found->coord == coord
             ? static_cast<std::size_t>(std::distance(columns.begin(), found))
             : columns.size();
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

[[nodiscard]] CreativeTerrainPaintSource sourceForMaterial(
    CreativeTerrainMaterial material) noexcept {
  switch (material) {
    case CreativeTerrainMaterial::Grass:
      return CreativeTerrainPaintSource::Grass;
    case CreativeTerrainMaterial::Dirt:
      return CreativeTerrainPaintSource::Dirt;
    case CreativeTerrainMaterial::Stone:
      return CreativeTerrainPaintSource::Stone;
    case CreativeTerrainMaterial::Sand:
      return CreativeTerrainPaintSource::Sand;
    case CreativeTerrainMaterial::Count: break;
  }
  return CreativeTerrainPaintSource::Any;
}

[[nodiscard]] bool appendCell(CreativeTerrainPaintPlan& plan,
                              CreativeTerrainCoord2 coord,
                              std::size_t capacity) {
  if (plan.affectedCells.size() >= capacity) {
    plan.affectedCells.clear();
    plan.status = CreativeTerrainPaintPlanStatus::CapacityExceeded;
    plan.reasonCode = "creative_terrain_paint_capacity_exceeded";
    return false;
  }
  plan.affectedCells.push_back(coord);
  return true;
}

[[nodiscard]] bool buildBrushCells(
    CreativeTerrainPaintPlan& plan,
    const CreativeTerrainPaintRequest& request) {
  const std::size_t capacity = std::min(
      request.maxAffectedCellCount, kCreativeTerrainPaintBrushCellCapacity);
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
        return false;
      }
      const CreativeTerrainCoord2 coord{static_cast<std::int32_t>(x),
                                        static_cast<std::int32_t>(z)};
      if (surfaceIndex(request.surfaceColumns, coord) ==
          request.surfaceColumns.size()) {
        continue;
      }
      if (!appendCell(plan, coord, capacity)) {
        return false;
      }
    }
  }
  return true;
}

[[nodiscard]] bool buildConnectedCells(
    CreativeTerrainPaintPlan& plan,
    const CreativeTerrainPaintRequest& request) {
  const std::size_t seedIndex =
      surfaceIndex(request.surfaceColumns, request.center);
  if (seedIndex == request.surfaceColumns.size()) {
    return true;
  }
  const CreativeTerrainMaterial sourceMaterial =
      request.materialField->materialAt(request.center);
  plan.source = sourceForMaterial(sourceMaterial);
  std::vector<bool> visited(request.surfaceColumns.size(), false);
  std::vector<std::size_t> queue;
  queue.reserve(request.surfaceColumns.size());
  queue.push_back(seedIndex);
  visited[seedIndex] = true;
  constexpr std::array<CreativeTerrainCoord2, 4U> neighbors{{
      {0, -1},
      {-1, 0},
      {1, 0},
      {0, 1},
  }};
  for (std::size_t cursor = 0U; cursor < queue.size(); ++cursor) {
    const CreativeTerrainCoord2 coord =
        request.surfaceColumns[queue[cursor]].coord;
    if (!appendCell(plan, coord, request.maxAffectedCellCount)) {
      return false;
    }
    for (CreativeTerrainCoord2 offset : neighbors) {
      const std::int64_t x = static_cast<std::int64_t>(coord.x) + offset.x;
      const std::int64_t z = static_cast<std::int64_t>(coord.z) + offset.z;
      if (x < std::numeric_limits<std::int32_t>::min() ||
          x > std::numeric_limits<std::int32_t>::max() ||
          z < std::numeric_limits<std::int32_t>::min() ||
          z > std::numeric_limits<std::int32_t>::max()) {
        continue;
      }
      const CreativeTerrainCoord2 neighbor{static_cast<std::int32_t>(x),
                                           static_cast<std::int32_t>(z)};
      const std::size_t index = surfaceIndex(request.surfaceColumns, neighbor);
      if (index == request.surfaceColumns.size() || visited[index] ||
          request.materialField->materialAt(neighbor) != sourceMaterial) {
        continue;
      }
      visited[index] = true;
      queue.push_back(index);
    }
  }
  std::sort(plan.affectedCells.begin(), plan.affectedCells.end(), coordLess);
  return true;
}

[[nodiscard]] bool buildRegionCells(
    CreativeTerrainPaintPlan& plan,
    const CreativeTerrainPaintRequest& request) {
  for (const CreativeTerrainColumn& column : request.surfaceColumns) {
    if (column.coord.x < request.minimumCoord.x ||
        column.coord.x > request.maximumCoord.x ||
        column.coord.z < request.minimumCoord.z ||
        column.coord.z > request.maximumCoord.z ||
        !creativeTerrainPaintSourceMatches(
            request.source,
            request.materialField->materialAt(column.coord))) {
      continue;
    }
    if (!appendCell(plan, column.coord, request.maxAffectedCellCount)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool buildEdits(CreativeTerrainPaintPlan& plan,
                              const CreativeTerrainPaintRequest& request) {
  std::size_t finalOverrideCount =
      static_cast<std::size_t>(request.materialField->overrideCount());
  plan.edits.reserve(plan.affectedCells.size());
  for (CreativeTerrainCoord2 coord : plan.affectedCells) {
    const CreativeTerrainMaterial current =
        request.materialField->materialAt(coord);
    if (current == request.material) {
      continue;
    }
    const bool hasOverride = request.materialField->overrideAt(coord) != nullptr;
    CreativeTerrainMaterialEdit edit;
    edit.kind = request.material == CreativeTerrainMaterial::Grass
                    ? CreativeTerrainMaterialEditKind::Clear
                    : CreativeTerrainMaterialEditKind::Set;
    edit.coord = coord;
    edit.material = request.material;
    plan.edits.push_back(edit);
    if (edit.kind == CreativeTerrainMaterialEditKind::Set && !hasOverride) {
      ++finalOverrideCount;
    } else if (edit.kind == CreativeTerrainMaterialEditKind::Clear &&
               hasOverride) {
      --finalOverrideCount;
    }
  }
  if (finalOverrideCount > kCreativeTerrainMaterialOverrideCapacity) {
    plan.affectedCells.clear();
    plan.edits.clear();
    plan.status = CreativeTerrainPaintPlanStatus::CapacityExceeded;
    plan.reasonCode = "creative_terrain_paint_field_capacity_exceeded";
    return false;
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

std::string_view toString(CreativeTerrainPaintMode mode) noexcept {
  switch (mode) {
    case CreativeTerrainPaintMode::Brush: return "BRUSH";
    case CreativeTerrainPaintMode::Connected: return "CONNECTED";
    case CreativeTerrainPaintMode::Region: return "REGION";
    case CreativeTerrainPaintMode::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeTerrainPaintSource source) noexcept {
  switch (source) {
    case CreativeTerrainPaintSource::Any: return "ANY";
    case CreativeTerrainPaintSource::Grass: return "GRASS";
    case CreativeTerrainPaintSource::Dirt: return "DIRT";
    case CreativeTerrainPaintSource::Stone: return "STONE";
    case CreativeTerrainPaintSource::Sand: return "SAND";
    case CreativeTerrainPaintSource::Count: break;
  }
  return "INVALID";
}

bool creativeTerrainPaintSourceMatches(
    CreativeTerrainPaintSource source,
    CreativeTerrainMaterial material) noexcept {
  if (!isValidCreativeTerrainMaterial(material)) {
    return false;
  }
  switch (source) {
    case CreativeTerrainPaintSource::Any: return true;
    case CreativeTerrainPaintSource::Grass:
      return material == CreativeTerrainMaterial::Grass;
    case CreativeTerrainPaintSource::Dirt:
      return material == CreativeTerrainMaterial::Dirt;
    case CreativeTerrainPaintSource::Stone:
      return material == CreativeTerrainMaterial::Stone;
    case CreativeTerrainPaintSource::Sand:
      return material == CreativeTerrainMaterial::Sand;
    case CreativeTerrainPaintSource::Count: break;
  }
  return false;
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
    const CreativeTerrainPaintRequest& request) {
  CreativeTerrainPaintPlan plan;
  plan.requested = true;
  plan.mode = request.mode;
  plan.center = request.center;
  plan.minimumCoord = request.minimumCoord;
  plan.maximumCoord = request.maximumCoord;
  plan.material = request.material;
  plan.source = request.source;
  plan.radiusCells = request.radiusCells;
  const bool regionBoundsValid =
      request.minimumCoord.x <= request.maximumCoord.x &&
      request.minimumCoord.z <= request.maximumCoord.z;
  if (request.materialField == nullptr ||
      !request.materialField->validateInvariants() ||
      !isValidCreativeTerrainMaterial(request.material) ||
      request.mode >= CreativeTerrainPaintMode::Count ||
      request.source >= CreativeTerrainPaintSource::Count ||
      request.maxAffectedCellCount == 0U ||
      request.maxAffectedCellCount > kCreativeTerrainPaintCellCapacity ||
      !surfaceColumnsAreCanonical(request.surfaceColumns) ||
      (request.mode == CreativeTerrainPaintMode::Brush &&
       (request.radiusCells == 0U || request.radiusCells > 8U)) ||
      (request.mode == CreativeTerrainPaintMode::Region &&
       !regionBoundsValid)) {
    plan.status = CreativeTerrainPaintPlanStatus::InvalidRequest;
    plan.reasonCode = "creative_terrain_paint_invalid_request";
    return plan;
  }

  bool built = false;
  switch (request.mode) {
    case CreativeTerrainPaintMode::Brush:
      built = buildBrushCells(plan, request);
      break;
    case CreativeTerrainPaintMode::Connected:
      built = buildConnectedCells(plan, request);
      break;
    case CreativeTerrainPaintMode::Region:
      built = buildRegionCells(plan, request);
      break;
    case CreativeTerrainPaintMode::Count: break;
  }
  if (!built) {
    return plan;
  }
  if (plan.affectedCells.empty()) {
    plan.status = CreativeTerrainPaintPlanStatus::NoSurface;
    plan.reasonCode = "creative_terrain_paint_no_surface";
    return plan;
  }
  if (!buildEdits(plan, request)) {
    return plan;
  }
  plan.accepted = true;
  if (plan.edits.empty()) {
    plan.status = CreativeTerrainPaintPlanStatus::NoChange;
    plan.reasonCode = "creative_terrain_paint_no_change";
    return plan;
  }
  plan.status = CreativeTerrainPaintPlanStatus::Ready;
  plan.reasonCode = "creative_terrain_paint_ready";
  return plan;
}

}  // namespace iggy3d::creative
