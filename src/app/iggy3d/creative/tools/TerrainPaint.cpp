#include "app/iggy3d/creative/tools/TerrainPaint.hpp"

#include <algorithm>
#include <array>
#include <cmath>
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

[[nodiscard]] const CreativeTerrainColumn* surfaceColumn(
    std::span<const CreativeTerrainColumn> columns,
    CreativeTerrainCoord2 coord) noexcept {
  const std::size_t index = surfaceIndex(columns, coord);
  return index == columns.size() ? nullptr : &columns[index];
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
    plan.previewCells.clear();
    plan.edits.clear();
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
      if (request.mask == CreativeTerrainPaintMask::Circle &&
          dx * dx + dz * dz > radiusSquared) {
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
  if (!creativeTerrainPaintSourceMatches(request.source, sourceMaterial)) {
    return true;
  }
  plan.source = request.source == CreativeTerrainPaintSource::Any
                    ? sourceForMaterial(sourceMaterial)
                    : request.source;
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

[[nodiscard]] double terrainSlopeDegrees(
    std::span<const CreativeTerrainColumn> columns,
    const CreativeTerrainColumn& center) noexcept {
  const auto sample = [&](std::int32_t dx,
                          std::int32_t dz) -> const CreativeTerrainColumn* {
    const std::int64_t x = static_cast<std::int64_t>(center.coord.x) + dx;
    const std::int64_t z = static_cast<std::int64_t>(center.coord.z) + dz;
    if (x < std::numeric_limits<std::int32_t>::min() ||
        x > std::numeric_limits<std::int32_t>::max() ||
        z < std::numeric_limits<std::int32_t>::min() ||
        z > std::numeric_limits<std::int32_t>::max()) {
      return nullptr;
    }
    return surfaceColumn(columns, {static_cast<std::int32_t>(x),
                                   static_cast<std::int32_t>(z)});
  };
  const CreativeTerrainColumn* negativeX = sample(-1, 0);
  const CreativeTerrainColumn* positiveX = sample(1, 0);
  const CreativeTerrainColumn* negativeZ = sample(0, -1);
  const CreativeTerrainColumn* positiveZ = sample(0, 1);
  const auto axisSlope = [&](const CreativeTerrainColumn* negative,
                             const CreativeTerrainColumn* positive) {
    if (negative != nullptr && positive != nullptr) {
      return (static_cast<double>(positive->heightCells) -
              static_cast<double>(negative->heightCells)) /
             2.0;
    }
    if (positive != nullptr) {
      return static_cast<double>(positive->heightCells) - center.heightCells;
    }
    if (negative != nullptr) {
      return static_cast<double>(center.heightCells) - negative->heightCells;
    }
    return 0.0;
  };
  const double magnitude =
      std::hypot(axisSlope(negativeX, positiveX),
                 axisSlope(negativeZ, positiveZ));
  return std::atan(magnitude) * 180.0 / std::acos(-1.0);
}

[[nodiscard]] bool slopeMatches(CreativeTerrainPaintSlopeFilter filter,
                                double degrees) noexcept {
  switch (filter) {
    case CreativeTerrainPaintSlopeFilter::Any: return true;
    case CreativeTerrainPaintSlopeFilter::UpTo5Degrees:
      return degrees <= 5.0;
    case CreativeTerrainPaintSlopeFilter::UpTo15Degrees:
      return degrees <= 15.0;
    case CreativeTerrainPaintSlopeFilter::UpTo30Degrees:
      return degrees <= 30.0;
    case CreativeTerrainPaintSlopeFilter::UpTo45Degrees:
      return degrees <= 45.0;
    case CreativeTerrainPaintSlopeFilter::Above45Degrees:
      return degrees > 45.0;
    case CreativeTerrainPaintSlopeFilter::Count: break;
  }
  return false;
}

[[nodiscard]] bool heightMatches(CreativeTerrainPaintHeightFilter filter,
                                 std::uint16_t height) noexcept {
  switch (filter) {
    case CreativeTerrainPaintHeightFilter::Any: return true;
    case CreativeTerrainPaintHeightFilter::Cells1To8:
      return height <= 8U;
    case CreativeTerrainPaintHeightFilter::Cells9To16:
      return height >= 9U && height <= 16U;
    case CreativeTerrainPaintHeightFilter::Cells17To32:
      return height >= 17U && height <= 32U;
    case CreativeTerrainPaintHeightFilter::Cells33To64:
      return height >= 33U && height <= 64U;
    case CreativeTerrainPaintHeightFilter::Count: break;
  }
  return false;
}

[[nodiscard]] std::uint8_t percentByte(std::uint8_t percent) noexcept {
  return static_cast<std::uint8_t>(
      (static_cast<std::uint16_t>(percent) * 255U + 50U) / 100U);
}

[[nodiscard]] std::uint8_t paintInfluence(
    const CreativeTerrainPaintRequest& request,
    CreativeTerrainCoord2 coord) noexcept {
  const std::uint8_t opacity =
      percentByte(creativeTerrainPaintOpacityPercent(request.opacity));
  if (request.mode != CreativeTerrainPaintMode::Brush) {
    return opacity;
  }
  const double dx = static_cast<double>(coord.x) - request.center.x;
  const double dz = static_cast<double>(coord.z) - request.center.z;
  const double distance = request.mask == CreativeTerrainPaintMask::Square
                              ? std::max(std::abs(dx), std::abs(dz))
                              : std::hypot(dx, dz);
  const double hardness =
      creativeTerrainPaintHardnessPercent(request.hardness) / 100.0;
  double falloff = 1.0;
  if (hardness < 1.0 && distance > 0.0) {
    const double outerRadius = static_cast<double>(request.radiusCells) + 0.5;
    const double normalized = std::clamp(distance / outerRadius, 0.0, 1.0);
    if (normalized > hardness) {
      falloff = 1.0 - (normalized - hardness) / (1.0 - hardness);
    }
  }
  const std::uint16_t scaled = static_cast<std::uint16_t>(
      std::lround(std::clamp(falloff, 0.0, 1.0) * opacity));
  return static_cast<std::uint8_t>(std::min<std::uint16_t>(scaled, 255U));
}

[[nodiscard]] CreativeTerrainMaterialWeights distributeWeights(
    const std::array<std::uint32_t, kCreativeTerrainMaterialCount>& numerators,
    std::uint32_t denominator,
    std::uint16_t expectedTotal) noexcept {
  CreativeTerrainMaterialWeights result{};
  std::array<std::uint32_t, kCreativeTerrainMaterialCount> remainders{};
  std::uint16_t total = 0U;
  for (std::size_t index = 0U; index < result.size(); ++index) {
    result[index] = static_cast<std::uint8_t>(numerators[index] / denominator);
    remainders[index] = numerators[index] % denominator;
    total = static_cast<std::uint16_t>(total + result[index]);
  }
  while (total < expectedTotal) {
    std::size_t best = 0U;
    for (std::size_t index = 1U; index < remainders.size(); ++index) {
      if (remainders[index] > remainders[best]) {
        best = index;
      }
    }
    ++result[best];
    remainders[best] = 0U;
    ++total;
  }
  return result;
}

[[nodiscard]] CreativeTerrainMaterialWeights replaceWeights(
    const CreativeTerrainMaterialWeights& before,
    CreativeTerrainMaterial material,
    std::uint8_t influence) noexcept {
  const std::size_t target = static_cast<std::size_t>(material);
  std::array<std::uint32_t, kCreativeTerrainMaterialCount> numerators{};
  for (std::size_t index = 0U; index < before.size(); ++index) {
    numerators[index] =
        static_cast<std::uint32_t>(before[index]) * (255U - influence);
  }
  numerators[target] += static_cast<std::uint32_t>(255U) * influence;
  return distributeWeights(numerators, 255U, 255U);
}

[[nodiscard]] CreativeTerrainMaterialWeights addWeights(
    const CreativeTerrainMaterialWeights& before,
    CreativeTerrainMaterial material,
    std::uint8_t influence) noexcept {
  const std::size_t target = static_cast<std::size_t>(material);
  const std::uint16_t targetWeight = std::min<std::uint16_t>(
      255U, static_cast<std::uint16_t>(before[target]) + influence);
  const std::uint16_t remaining = 255U - targetWeight;
  const std::uint16_t sourceRemaining = 255U - before[target];
  if (sourceRemaining == 0U) {
    return before;
  }
  std::array<std::uint32_t, kCreativeTerrainMaterialCount> numerators{};
  for (std::size_t index = 0U; index < before.size(); ++index) {
    if (index != target) {
      numerators[index] =
          static_cast<std::uint32_t>(before[index]) * remaining;
    }
  }
  CreativeTerrainMaterialWeights result =
      distributeWeights(numerators, sourceRemaining, remaining);
  result[target] = static_cast<std::uint8_t>(targetWeight);
  return result;
}

[[nodiscard]] bool buildEdits(CreativeTerrainPaintPlan& plan,
                              const CreativeTerrainPaintRequest& request) {
  std::size_t finalOverrideCount =
      static_cast<std::size_t>(request.materialField->overrideCount());
  std::vector<CreativeTerrainCoord2> candidates =
      std::move(plan.affectedCells);
  plan.affectedCells.clear();
  plan.affectedCells.reserve(candidates.size());
  plan.previewCells.reserve(candidates.size());
  plan.edits.reserve(candidates.size());
  const CreativeTerrainMaterialWeights grass =
      creativeTerrainMaterialSolidWeights(CreativeTerrainMaterial::Grass);
  for (CreativeTerrainCoord2 coord : candidates) {
    const CreativeTerrainColumn* column =
        surfaceColumn(request.surfaceColumns, coord);
    if (column == nullptr) {
      continue;
    }
    const CreativeTerrainMaterial current =
        request.materialField->materialAt(coord);
    const double slope = terrainSlopeDegrees(request.surfaceColumns, *column);
    if (!creativeTerrainPaintSourceMatches(request.source, current) ||
        !slopeMatches(request.slopeFilter, slope) ||
        !heightMatches(request.heightFilter, column->heightCells)) {
      continue;
    }
    const std::uint8_t influence = paintInfluence(request, coord);
    if (influence == 0U) {
      continue;
    }
    const CreativeTerrainMaterialWeights before =
        request.materialField->weightsAt(coord);
    const CreativeTerrainMaterialWeights after =
        request.blend == CreativeTerrainPaintBlend::Additive
            ? addWeights(before, request.material, influence)
            : replaceWeights(before, request.material, influence);
    plan.affectedCells.push_back(coord);
    plan.previewCells.push_back(
        {coord, before, after, column->heightCells, influence, slope});
    if (after == before) {
      continue;
    }
    const bool hadOverride = request.materialField->overrideAt(coord) != nullptr;
    const bool hasOverride = after != grass;
    plan.edits.push_back(hasOverride
                             ? makeCreativeTerrainMaterialWeightEdit(coord, after)
                             : CreativeTerrainMaterialEdit{
                                   CreativeTerrainMaterialEditKind::Clear,
                                   coord, CreativeTerrainMaterial::Grass});
    if (hasOverride && !hadOverride) {
      ++finalOverrideCount;
    } else if (!hasOverride && hadOverride) {
      --finalOverrideCount;
    }
  }
  if (finalOverrideCount > kCreativeTerrainMaterialOverrideCapacity) {
    plan.affectedCells.clear();
    plan.previewCells.clear();
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

std::uint8_t creativeTerrainPaintHardnessPercent(
    CreativeTerrainPaintHardness hardness) noexcept {
  switch (hardness) {
    case CreativeTerrainPaintHardness::Soft: return 0U;
    case CreativeTerrainPaintHardness::Balanced: return 50U;
    case CreativeTerrainPaintHardness::Firm: return 75U;
    case CreativeTerrainPaintHardness::Solid: return 100U;
    case CreativeTerrainPaintHardness::Count: break;
  }
  return 0U;
}

std::uint8_t creativeTerrainPaintOpacityPercent(
    CreativeTerrainPaintOpacity opacity) noexcept {
  switch (opacity) {
    case CreativeTerrainPaintOpacity::Percent25: return 25U;
    case CreativeTerrainPaintOpacity::Percent50: return 50U;
    case CreativeTerrainPaintOpacity::Percent75: return 75U;
    case CreativeTerrainPaintOpacity::Percent100: return 100U;
    case CreativeTerrainPaintOpacity::Count: break;
  }
  return 0U;
}

std::string_view toString(CreativeTerrainPaintHardness hardness) noexcept {
  switch (hardness) {
    case CreativeTerrainPaintHardness::Soft: return "SOFT";
    case CreativeTerrainPaintHardness::Balanced: return "50%";
    case CreativeTerrainPaintHardness::Firm: return "75%";
    case CreativeTerrainPaintHardness::Solid: return "SOLID";
    case CreativeTerrainPaintHardness::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeTerrainPaintOpacity opacity) noexcept {
  switch (opacity) {
    case CreativeTerrainPaintOpacity::Percent25: return "25%";
    case CreativeTerrainPaintOpacity::Percent50: return "50%";
    case CreativeTerrainPaintOpacity::Percent75: return "75%";
    case CreativeTerrainPaintOpacity::Percent100: return "100%";
    case CreativeTerrainPaintOpacity::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeTerrainPaintMask mask) noexcept {
  switch (mask) {
    case CreativeTerrainPaintMask::Circle: return "CIRCLE";
    case CreativeTerrainPaintMask::Square: return "SQUARE";
    case CreativeTerrainPaintMask::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeTerrainPaintBlend blend) noexcept {
  switch (blend) {
    case CreativeTerrainPaintBlend::Replace: return "REPLACE";
    case CreativeTerrainPaintBlend::Additive: return "ADDITIVE";
    case CreativeTerrainPaintBlend::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeTerrainPaintSlopeFilter filter) noexcept {
  switch (filter) {
    case CreativeTerrainPaintSlopeFilter::Any: return "ANY SLOPE";
    case CreativeTerrainPaintSlopeFilter::UpTo5Degrees: return "<= 5 DEG";
    case CreativeTerrainPaintSlopeFilter::UpTo15Degrees: return "<= 15 DEG";
    case CreativeTerrainPaintSlopeFilter::UpTo30Degrees: return "<= 30 DEG";
    case CreativeTerrainPaintSlopeFilter::UpTo45Degrees: return "<= 45 DEG";
    case CreativeTerrainPaintSlopeFilter::Above45Degrees: return "> 45 DEG";
    case CreativeTerrainPaintSlopeFilter::Count: break;
  }
  return "INVALID";
}

std::string_view toString(CreativeTerrainPaintHeightFilter filter) noexcept {
  switch (filter) {
    case CreativeTerrainPaintHeightFilter::Any: return "ANY HEIGHT";
    case CreativeTerrainPaintHeightFilter::Cells1To8: return "1-8 CELLS";
    case CreativeTerrainPaintHeightFilter::Cells9To16: return "9-16 CELLS";
    case CreativeTerrainPaintHeightFilter::Cells17To32: return "17-32 CELLS";
    case CreativeTerrainPaintHeightFilter::Cells33To64: return "33-64 CELLS";
    case CreativeTerrainPaintHeightFilter::Count: break;
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
  plan.hardness = request.hardness;
  plan.opacity = request.opacity;
  plan.mask = request.mask;
  plan.blend = request.blend;
  plan.slopeFilter = request.slopeFilter;
  plan.heightFilter = request.heightFilter;
  const bool regionBoundsValid =
      request.minimumCoord.x <= request.maximumCoord.x &&
      request.minimumCoord.z <= request.maximumCoord.z;
  if (request.materialField == nullptr ||
      !request.materialField->validateInvariants() ||
      !isValidCreativeTerrainMaterial(request.material) ||
      request.mode >= CreativeTerrainPaintMode::Count ||
      request.source >= CreativeTerrainPaintSource::Count ||
      request.hardness >= CreativeTerrainPaintHardness::Count ||
      request.opacity >= CreativeTerrainPaintOpacity::Count ||
      request.mask >= CreativeTerrainPaintMask::Count ||
      request.blend >= CreativeTerrainPaintBlend::Count ||
      request.slopeFilter >= CreativeTerrainPaintSlopeFilter::Count ||
      request.heightFilter >= CreativeTerrainPaintHeightFilter::Count ||
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
  if (plan.affectedCells.empty()) {
    plan.status = CreativeTerrainPaintPlanStatus::NoSurface;
    plan.reasonCode = "creative_terrain_paint_no_matching_surface";
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
