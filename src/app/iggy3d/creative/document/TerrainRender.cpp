#include "app/iggy3d/creative/document/TerrainField.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/TerrainFieldInternal.hpp"
#include "app/iggy3d/creative/document/TerrainMaterialField.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace iggy3d::creative {

using terrain_field_internal::coordLess;
using terrain_field_internal::terrainContributionWeight;

namespace {

struct TerrainContribution {
  CreativeTerrainCoord2 coord{};
  std::uint32_t weight = 0;
  std::uint64_t weightedHeight = 0;
};

[[nodiscard]] bool contributionLess(const TerrainContribution& lhs,
                                    const TerrainContribution& rhs) noexcept {
  return coordLess(lhs.coord, rhs.coord);
}

[[nodiscard]] const CreativeTerrainColumn* findTerrainColumn(
    std::span<const CreativeTerrainColumn> columns,
    std::int64_t x,
    std::int64_t z) noexcept {
  if (x < std::numeric_limits<std::int32_t>::min() ||
      x > std::numeric_limits<std::int32_t>::max() ||
      z < std::numeric_limits<std::int32_t>::min() ||
      z > std::numeric_limits<std::int32_t>::max()) {
    return nullptr;
  }
  const CreativeTerrainCoord2 coord{static_cast<std::int32_t>(x),
                                    static_cast<std::int32_t>(z)};
  const auto found = std::lower_bound(
      columns.begin(), columns.end(), coord,
      [](const CreativeTerrainColumn& column, CreativeTerrainCoord2 value) {
        return coordLess(column.coord, value);
      });
  return found != columns.end() && found->coord == coord ? &*found : nullptr;
}

[[nodiscard]] double terrainCornerHeightCells(
    std::span<const CreativeTerrainColumn> columns,
    std::int64_t cornerX,
    std::int64_t cornerZ) noexcept {
  constexpr std::array<std::array<std::int32_t, 2U>, 4U> offsets{{
      {-1, -1},
      {0, -1},
      {-1, 0},
      {0, 0},
  }};
  std::uint64_t heightSum = 0U;
  std::uint32_t count = 0U;
  for (const auto& offset : offsets) {
    const CreativeTerrainColumn* column = findTerrainColumn(
        columns, cornerX + offset[0], cornerZ + offset[1]);
    if (column == nullptr) {
      continue;
    }
    heightSum += column->heightCells;
    ++count;
  }
  return count == 0U ? 0.0
                     : static_cast<double>(heightSum) /
                           static_cast<double>(count);
}

[[nodiscard]] std::int32_t floorDivByVoxelChunk(
    std::int32_t value) noexcept {
  std::int32_t quotient = value / kCreativeVoxelChunkEdge;
  if (value % kCreativeVoxelChunkEdge < 0) {
    --quotient;
  }
  return quotient;
}

void appendRowCuboids(std::span<const CreativeTerrainColumn> row,
                      std::vector<CreativeVoxelCuboid>& output) {
  if (row.empty()) {
    return;
  }
  std::size_t begin = 0U;
  while (begin < row.size()) {
    std::size_t end = begin + 1U;
    while (end < row.size() &&
           row[end].heightCells == row[begin].heightCells &&
           row[end].coord.x == row[end - 1U].coord.x + 1) {
      ++end;
    }
    const CreativeTerrainColumn& first = row[begin];
    CreativeVoxelCuboid cuboid;
    cuboid.chunk = {floorDivByVoxelChunk(first.coord.x), 0,
                    floorDivByVoxelChunk(first.coord.z)};
    cuboid.minCell = {first.coord.x, 0, first.coord.z};
    cuboid.maxCellExclusive = {row[end - 1U].coord.x + 1,
                               first.heightCells, first.coord.z + 1};
    cuboid.material = CreativeObjectKind::TerrainPatch;
    output.push_back(cuboid);
    begin = end;
  }
}

}  // namespace

std::string_view toString(CreativeTerrainSurfacePlanStatus status) noexcept {
  switch (status) {
    case CreativeTerrainSurfacePlanStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainSurfacePlanStatus::InvalidField:
      return "InvalidField";
    case CreativeTerrainSurfacePlanStatus::Empty:
      return "Empty";
    case CreativeTerrainSurfacePlanStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

std::string_view toString(CreativeTerrainRenderPlanStatus status) noexcept {
  switch (status) {
    case CreativeTerrainRenderPlanStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainRenderPlanStatus::InvalidField:
      return "InvalidField";
    case CreativeTerrainRenderPlanStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeTerrainRenderPlanStatus::SurfacePlanFailed:
      return "SurfacePlanFailed";
    case CreativeTerrainRenderPlanStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainRenderPlanStatus::ArithmeticOverflow:
      return "ArithmeticOverflow";
    case CreativeTerrainRenderPlanStatus::Empty:
      return "Empty";
    case CreativeTerrainRenderPlanStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

std::string_view toString(
    CreativeTerrainMutationPreviewStatus status) noexcept {
  switch (status) {
    case CreativeTerrainMutationPreviewStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainMutationPreviewStatus::InvalidField:
      return "InvalidField";
    case CreativeTerrainMutationPreviewStatus::MutationRejected:
      return "MutationRejected";
    case CreativeTerrainMutationPreviewStatus::RenderRejected:
      return "RenderRejected";
    case CreativeTerrainMutationPreviewStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

CreativeTerrainSurfacePlan buildCreativeTerrainSurfacePlan(
    const CreativeTerrainField& field) {
  CreativeTerrainSurfacePlan plan;
  plan.requested = true;
  plan.sourceRevision = field.revision();
  if (!field.validateInvariants()) {
    plan.status = CreativeTerrainSurfacePlanStatus::InvalidField;
    plan.reasonCode = "creative_terrain_surface_field_invalid";
    return plan;
  }
  if (field.controls().empty()) {
    plan.accepted = true;
    plan.status = CreativeTerrainSurfacePlanStatus::Empty;
    plan.reasonCode = "creative_terrain_surface_empty";
    return plan;
  }

  constexpr std::size_t maximumContributionsPerControl =
      (kCreativeTerrainMaximumRadiusCells * 2U + 1U) *
      (kCreativeTerrainMaximumRadiusCells * 2U + 1U);
  std::vector<TerrainContribution> contributions;
  contributions.reserve(field.controls().size() *
                        maximumContributionsPerControl);
  for (const CreativeTerrainControlPoint& control : field.controls()) {
    const std::int32_t radius = control.radiusCells;
    const std::int32_t radiusSquared = radius * radius;
    for (std::int32_t dz = -radius; dz <= radius; ++dz) {
      for (std::int32_t dx = -radius; dx <= radius; ++dx) {
        const std::int32_t distanceSquared = dx * dx + dz * dz;
        if (distanceSquared > radiusSquared) {
          continue;
        }
        const std::uint32_t weight = terrainContributionWeight(
            control, {control.coord.x + dx, control.coord.z + dz});
        contributions.push_back(
            {{control.coord.x + dx, control.coord.z + dz}, weight,
             static_cast<std::uint64_t>(weight) * control.heightCells});
      }
    }
  }
  plan.contributionCount = contributions.size();
  std::sort(contributions.begin(), contributions.end(), contributionLess);

  plan.columns.reserve(contributions.size());
  for (std::size_t begin = 0U; begin < contributions.size();) {
    std::size_t end = begin + 1U;
    std::uint64_t totalWeight = contributions[begin].weight;
    std::uint64_t weightedHeight = contributions[begin].weightedHeight;
    while (end < contributions.size() &&
           contributions[end].coord == contributions[begin].coord) {
      totalWeight += contributions[end].weight;
      weightedHeight += contributions[end].weightedHeight;
      ++end;
    }
    const std::uint64_t roundedHeight =
        (weightedHeight + totalWeight / 2U) / totalWeight;
    plan.columns.push_back(
        {contributions[begin].coord,
         static_cast<std::uint16_t>(std::clamp<std::uint64_t>(
             roundedHeight, kCreativeTerrainMinimumHeightCells,
             kCreativeTerrainMaximumHeightCells))});
    begin = end;
  }

  for (std::size_t begin = 0U; begin < plan.columns.size();) {
    std::size_t end = begin + 1U;
    while (end < plan.columns.size() &&
           plan.columns[end].coord.z == plan.columns[begin].coord.z) {
      ++end;
    }
    appendRowCuboids(
        std::span<const CreativeTerrainColumn>{plan.columns.data() + begin,
                                               end - begin},
        plan.cuboids);
    begin = end;
  }

  plan.accepted = true;
  plan.status = CreativeTerrainSurfacePlanStatus::Ready;
  plan.reasonCode = "creative_terrain_surface_ready";
  return plan;
}

CreativeTerrainRenderPlan buildCreativeTerrainRenderPlan(
    const CreativeTerrainField& field,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount) {
  return buildCreativeTerrainRenderPlan(
      buildCreativeTerrainSurfacePlan(field), gridOrigin, cellSize,
      maxPatchCount);
}

static CreativeTerrainRenderPlan buildCreativeTerrainRenderPlanWithMaterials(
    const CreativeTerrainSurfacePlan& surface,
    const CreativeTerrainMaterialField* materials,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount) {
  CreativeTerrainRenderPlan plan;
  plan.requested = true;
  plan.sourceRevision = surface.sourceRevision;
  plan.sourceMaterialRevision = materials == nullptr ? 0U : materials->revision();
  if (surface.status == CreativeTerrainSurfacePlanStatus::InvalidField) {
    plan.status = CreativeTerrainRenderPlanStatus::InvalidField;
    plan.reasonCode = "creative_terrain_render_field_invalid";
    return plan;
  }
  if (!isFiniteCreativeVec3(gridOrigin) || !std::isfinite(cellSize) ||
      cellSize <= 0.0 || maxPatchCount == 0U) {
    plan.status = CreativeTerrainRenderPlanStatus::InvalidRequest;
    plan.reasonCode = "creative_terrain_render_request_invalid";
    return plan;
  }

  if (!surface.accepted) {
    plan.status = CreativeTerrainRenderPlanStatus::SurfacePlanFailed;
    plan.reasonCode = "creative_terrain_render_surface_plan_failed";
    return plan;
  }
  plan.sourceColumnCount = surface.columns.size();
  if (surface.columns.empty()) {
    plan.accepted = true;
    plan.status = CreativeTerrainRenderPlanStatus::Empty;
    plan.reasonCode = "creative_terrain_render_empty";
    return plan;
  }
  if (surface.columns.size() > maxPatchCount) {
    plan.status = CreativeTerrainRenderPlanStatus::CapacityExceeded;
    plan.reasonCode = "creative_terrain_render_patch_capacity_exceeded";
    return plan;
  }

  plan.patches.reserve(surface.columns.size());
  const std::span<const CreativeTerrainColumn> columns = surface.columns;
  for (const CreativeTerrainColumn& column : columns) {
    const std::int64_t x = column.coord.x;
    const std::int64_t z = column.coord.z;
    const std::array<std::array<std::int64_t, 2U>, 4U> corners{{
        {x, z},
        {x + 1, z},
        {x + 1, z + 1},
        {x, z + 1},
    }};
    CreativeTerrainSurfacePatch patch;
    patch.coord = column.coord;
    patch.material = materials == nullptr
                         ? CreativeTerrainMaterial::Grass
                         : materials->materialAt(column.coord);
    patch.center = {
        gridOrigin.x + (static_cast<double>(x) + 0.5) * cellSize,
        gridOrigin.y + static_cast<double>(column.heightCells) * cellSize,
        gridOrigin.z + (static_cast<double>(z) + 0.5) * cellSize,
    };
    if (!isFiniteCreativeVec3(patch.center)) {
      plan.patches.clear();
      plan.status = CreativeTerrainRenderPlanStatus::ArithmeticOverflow;
      plan.reasonCode = "creative_terrain_render_arithmetic_overflow";
      return plan;
    }
    for (std::size_t index = 0U; index < corners.size(); ++index) {
      const std::int64_t cornerX = corners[index][0];
      const std::int64_t cornerZ = corners[index][1];
      patch.corners[index] = {
          gridOrigin.x + static_cast<double>(cornerX) * cellSize,
          gridOrigin.y +
              terrainCornerHeightCells(columns, cornerX, cornerZ) * cellSize,
          gridOrigin.z + static_cast<double>(cornerZ) * cellSize,
      };
      if (!isFiniteCreativeVec3(patch.corners[index])) {
        plan.patches.clear();
        plan.status = CreativeTerrainRenderPlanStatus::ArithmeticOverflow;
        plan.reasonCode = "creative_terrain_render_arithmetic_overflow";
        return plan;
      }
    }
    plan.patches.push_back(patch);
  }

  plan.accepted = true;
  plan.status = CreativeTerrainRenderPlanStatus::Ready;
  plan.reasonCode = "creative_terrain_render_ready";
  return plan;
}

CreativeTerrainRenderPlan buildCreativeTerrainRenderPlan(
    const CreativeTerrainSurfacePlan& surface,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount) {
  return buildCreativeTerrainRenderPlanWithMaterials(
      surface, nullptr, gridOrigin, cellSize, maxPatchCount);
}

CreativeTerrainRenderPlan buildCreativeTerrainRenderPlan(
    const CreativeTerrainSurfacePlan& surface,
    const CreativeTerrainMaterialField& materials,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount) {
  return buildCreativeTerrainRenderPlanWithMaterials(
      surface, &materials, gridOrigin, cellSize, maxPatchCount);
}

CreativeTerrainMutationPreviewReceipt buildCreativeTerrainMutationPreview(
    const CreativeTerrainField& field,
    std::span<const CreativeTerrainControlEdit> edits,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount) {
  CreativeTerrainMutationPreviewReceipt receipt;
  receipt.requested = true;
  if (!field.validateInvariants()) {
    receipt.status = CreativeTerrainMutationPreviewStatus::InvalidField;
    receipt.reasonCode = "creative_terrain_preview_field_invalid";
    return receipt;
  }

  CreativeTerrainField previewField = field;
  if (!edits.empty()) {
    receipt.mutation = previewField.apply(edits);
    if (!receipt.mutation.accepted) {
      receipt.status =
          CreativeTerrainMutationPreviewStatus::MutationRejected;
      receipt.reasonCode = "creative_terrain_preview_mutation_rejected";
      return receipt;
    }
  }
  receipt.render = buildCreativeTerrainRenderPlan(
      previewField, gridOrigin, cellSize, maxPatchCount);
  if (!receipt.render.accepted) {
    receipt.status = CreativeTerrainMutationPreviewStatus::RenderRejected;
    receipt.reasonCode = "creative_terrain_preview_render_rejected";
    return receipt;
  }
  receipt.accepted = true;
  receipt.status = CreativeTerrainMutationPreviewStatus::Ready;
  receipt.reasonCode = "creative_terrain_preview_ready";
  return receipt;
}

}  // namespace iggy3d::creative
