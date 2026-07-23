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
using terrain_field_internal::appendTerrainRowCuboids;
using terrain_field_internal::terrainContributionWeight;

namespace {

struct TerrainContribution {
  CreativeTerrainCoord2 coord{};
  std::uint32_t weight = 0;
  std::uint64_t weightedHeight = 0;
};

struct TerrainPatchEdgeSpec {
  std::int32_t neighborX = 0;
  std::int32_t neighborZ = 0;
};

constexpr std::array<TerrainPatchEdgeSpec, 4U> kTerrainPatchEdges{{
    {0, -1},
    {1, 0},
    {0, 1},
    {-1, 0},
}};

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

[[nodiscard]] bool hardEdgeLess(CreativeTerrainHardEdge lhs,
                                CreativeTerrainHardEdge rhs) noexcept {
  return lhs.first != rhs.first ? coordLess(lhs.first, rhs.first)
                                : coordLess(lhs.second, rhs.second);
}

[[nodiscard]] bool hasTerrainHardEdge(
    std::span<const CreativeTerrainHardEdge> hardEdges,
    CreativeTerrainCoord2 first,
    CreativeTerrainCoord2 second) noexcept {
  const CreativeTerrainHardEdge candidate =
      canonicalCreativeTerrainHardEdge(first, second);
  const auto found =
      std::lower_bound(hardEdges.begin(), hardEdges.end(), candidate,
                       hardEdgeLess);
  return found != hardEdges.end() && *found == candidate;
}

[[nodiscard]] bool cornerTouchesHardEdge(
    std::span<const CreativeTerrainHardEdge> hardEdges,
    CreativeTerrainCoord2 coord,
    std::size_t cornerIndex) noexcept {
  constexpr std::array<std::array<std::size_t, 2U>, 4U> cornerEdges{{
      {0U, 3U},
      {0U, 1U},
      {1U, 2U},
      {2U, 3U},
  }};
  for (const std::size_t edgeIndex : cornerEdges[cornerIndex]) {
    const TerrainPatchEdgeSpec edge = kTerrainPatchEdges[edgeIndex];
    const std::int64_t neighborX =
        static_cast<std::int64_t>(coord.x) + edge.neighborX;
    const std::int64_t neighborZ =
        static_cast<std::int64_t>(coord.z) + edge.neighborZ;
    if (neighborX < std::numeric_limits<std::int32_t>::min() ||
        neighborX > std::numeric_limits<std::int32_t>::max() ||
        neighborZ < std::numeric_limits<std::int32_t>::min() ||
        neighborZ > std::numeric_limits<std::int32_t>::max()) {
      continue;
    }
    if (hasTerrainHardEdge(
            hardEdges, coord,
            {static_cast<std::int32_t>(neighborX),
             static_cast<std::int32_t>(neighborZ)})) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] double terrainPatchCornerHeightCells(
    std::span<const CreativeTerrainColumn> columns,
    std::span<const CreativeTerrainHardEdge> hardEdges,
    const CreativeTerrainColumn& requestingColumn,
    std::size_t cornerIndex,
    std::int64_t cornerX,
    std::int64_t cornerZ) noexcept {
  return cornerTouchesHardEdge(hardEdges, requestingColumn.coord, cornerIndex)
             ? static_cast<double>(requestingColumn.heightCells)
             : terrainCornerHeightCells(columns, cornerX, cornerZ);
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
    appendTerrainRowCuboids(
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
  if (!validateCreativeTerrainHardEdgesForSurface(surface.columns,
                                                  surface.hardEdges)) {
    plan.status = CreativeTerrainRenderPlanStatus::InvalidField;
    plan.reasonCode = "creative_terrain_render_hard_edges_invalid";
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
    const CreativeTerrainMaterialWeights materialWeights =
        materials == nullptr
            ? creativeTerrainMaterialSolidWeights(
                  CreativeTerrainMaterial::Grass)
            : materials->weightsAt(column.coord);
    patch.material = dominantCreativeTerrainMaterial(materialWeights);
    patch.materialWeights = materialWeights;
    patch.materialColor = creativeTerrainMaterialRenderColor(materialWeights);
    patch.center = {
        gridOrigin.x + (static_cast<double>(x) + 0.5) * cellSize,
        0.0,
        gridOrigin.z + (static_cast<double>(z) + 0.5) * cellSize,
    };
    double cornerHeightSum = 0.0;
    for (std::size_t index = 0U; index < corners.size(); ++index) {
      const std::int64_t cornerX = corners[index][0];
      const std::int64_t cornerZ = corners[index][1];
      patch.corners[index] = {
          gridOrigin.x + static_cast<double>(cornerX) * cellSize,
          gridOrigin.y +
              terrainPatchCornerHeightCells(
                  columns, surface.hardEdges, column, index, cornerX,
                  cornerZ) *
                  cellSize,
          gridOrigin.z + static_cast<double>(cornerZ) * cellSize,
      };
      if (!isFiniteCreativeVec3(patch.corners[index])) {
        plan.patches.clear();
        plan.status = CreativeTerrainRenderPlanStatus::ArithmeticOverflow;
        plan.reasonCode = "creative_terrain_render_arithmetic_overflow";
        return plan;
      }
      cornerHeightSum += patch.corners[index].y;
    }
    patch.center.y = cornerHeightSum / static_cast<double>(corners.size());
    for (std::size_t edgeIndex = 0U;
         edgeIndex < kTerrainPatchEdges.size(); ++edgeIndex) {
      const TerrainPatchEdgeSpec edge = kTerrainPatchEdges[edgeIndex];
      const std::int64_t neighborX = x + edge.neighborX;
      const std::int64_t neighborZ = z + edge.neighborZ;
      if (neighborX < std::numeric_limits<std::int32_t>::min() ||
          neighborX > std::numeric_limits<std::int32_t>::max() ||
          neighborZ < std::numeric_limits<std::int32_t>::min() ||
          neighborZ > std::numeric_limits<std::int32_t>::max()) {
        continue;
      }
      const CreativeTerrainCoord2 neighborCoord{
          static_cast<std::int32_t>(neighborX),
          static_cast<std::int32_t>(neighborZ)};
      if (!hasTerrainHardEdge(surface.hardEdges, column.coord,
                              neighborCoord)) {
        continue;
      }
      const CreativeTerrainColumn* neighbor =
          findTerrainColumn(columns, neighborX, neighborZ);
      if (neighbor == nullptr || column.heightCells > neighbor->heightCells) {
        patch.hardEdgeMask |= static_cast<std::uint8_t>(1U << edgeIndex);
      }
    }
    if (!isFiniteCreativeVec3(patch.center)) {
      plan.patches.clear();
      plan.status = CreativeTerrainRenderPlanStatus::ArithmeticOverflow;
      plan.reasonCode = "creative_terrain_render_arithmetic_overflow";
      return plan;
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

CreativeTerrainRenderPlan buildCreativeTerrainRenderPlan(
    const CreativeTerrainField& field,
    CreativeTerrainPatchRegion region,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount) {
  CreativeTerrainRenderPlan plan;
  plan.requested = true;
  plan.sourceRevision = field.revision();
  if (!field.validateInvariants()) {
    plan.status = CreativeTerrainRenderPlanStatus::InvalidField;
    plan.reasonCode = "creative_terrain_render_region_field_invalid";
    return plan;
  }
  if (!isFiniteCreativeVec3(gridOrigin) || !std::isfinite(cellSize) ||
      cellSize <= 0.0 || maxPatchCount == 0U ||
      region.maximum.x < region.minimum.x ||
      region.maximum.z < region.minimum.z) {
    plan.status = CreativeTerrainRenderPlanStatus::InvalidRequest;
    plan.reasonCode = "creative_terrain_render_region_request_invalid";
    return plan;
  }

  const std::uint64_t width =
      static_cast<std::uint64_t>(static_cast<std::int64_t>(region.maximum.x) -
                                 region.minimum.x + 1);
  const std::uint64_t depth =
      static_cast<std::uint64_t>(static_cast<std::int64_t>(region.maximum.z) -
                                 region.minimum.z + 1);
  if (width > maxPatchCount || depth > maxPatchCount ||
      width * depth > maxPatchCount) {
    plan.status = CreativeTerrainRenderPlanStatus::CapacityExceeded;
    plan.reasonCode = "creative_terrain_render_region_capacity_exceeded";
    return plan;
  }

  const std::int64_t supportMinimumX =
      static_cast<std::int64_t>(region.minimum.x) - 1;
  const std::int64_t supportMinimumZ =
      static_cast<std::int64_t>(region.minimum.z) - 1;
  const std::int64_t supportMaximumX =
      static_cast<std::int64_t>(region.maximum.x) + 1;
  const std::int64_t supportMaximumZ =
      static_cast<std::int64_t>(region.maximum.z) + 1;
  if (supportMinimumX < std::numeric_limits<std::int32_t>::min() ||
      supportMinimumZ < std::numeric_limits<std::int32_t>::min() ||
      supportMaximumX > std::numeric_limits<std::int32_t>::max() ||
      supportMaximumZ > std::numeric_limits<std::int32_t>::max()) {
    plan.status = CreativeTerrainRenderPlanStatus::ArithmeticOverflow;
    plan.reasonCode = "creative_terrain_render_region_arithmetic_overflow";
    return plan;
  }

  std::vector<CreativeTerrainColumn> supportColumns;
  const std::uint64_t supportWidth = width + 2U;
  const std::uint64_t supportDepth = depth + 2U;
  supportColumns.reserve(
      static_cast<std::size_t>(supportWidth * supportDepth));
  for (std::int64_t z = supportMinimumZ; z <= supportMaximumZ; ++z) {
    for (std::int64_t x = supportMinimumX; x <= supportMaximumX; ++x) {
      const CreativeTerrainCoord2 coord{static_cast<std::int32_t>(x),
                                        static_cast<std::int32_t>(z)};
      const CreativeTerrainHeightSample sample =
          sampleCreativeTerrainHeight(field, coord);
      if (sample.present) {
        supportColumns.push_back({coord, sample.heightCells});
      }
    }
  }

  plan.patches.reserve(static_cast<std::size_t>(width * depth));
  const std::span<const CreativeTerrainColumn> columns = supportColumns;
  for (std::int64_t z = region.minimum.z; z <= region.maximum.z; ++z) {
    for (std::int64_t x = region.minimum.x; x <= region.maximum.x; ++x) {
      const CreativeTerrainColumn* column = findTerrainColumn(columns, x, z);
      if (column == nullptr) {
        continue;
      }
      const std::array<std::array<std::int64_t, 2U>, 4U> corners{{
          {x, z},
          {x + 1, z},
          {x + 1, z + 1},
          {x, z + 1},
      }};
      CreativeTerrainSurfacePatch patch;
      patch.coord = column->coord;
      patch.material = CreativeTerrainMaterial::Grass;
      patch.materialWeights = creativeTerrainMaterialSolidWeights(
          CreativeTerrainMaterial::Grass);
      patch.materialColor =
          creativeTerrainMaterialRenderColor(patch.materialWeights);
      patch.center = {
          gridOrigin.x + (static_cast<double>(x) + 0.5) * cellSize,
          0.0,
          gridOrigin.z + (static_cast<double>(z) + 0.5) * cellSize,
      };
      double cornerHeightSum = 0.0;
      for (std::size_t index = 0U; index < corners.size(); ++index) {
        const std::int64_t cornerX = corners[index][0];
        const std::int64_t cornerZ = corners[index][1];
        patch.corners[index] = {
            gridOrigin.x + static_cast<double>(cornerX) * cellSize,
            gridOrigin.y +
                terrainCornerHeightCells(columns, cornerX, cornerZ) *
                    cellSize,
            gridOrigin.z + static_cast<double>(cornerZ) * cellSize,
        };
        if (!isFiniteCreativeVec3(patch.corners[index])) {
          plan.patches.clear();
          plan.status = CreativeTerrainRenderPlanStatus::ArithmeticOverflow;
          plan.reasonCode =
              "creative_terrain_render_region_arithmetic_overflow";
          return plan;
        }
        cornerHeightSum += patch.corners[index].y;
      }
      patch.center.y = cornerHeightSum / static_cast<double>(corners.size());
      if (!isFiniteCreativeVec3(patch.center)) {
        plan.patches.clear();
        plan.status = CreativeTerrainRenderPlanStatus::ArithmeticOverflow;
        plan.reasonCode = "creative_terrain_render_region_arithmetic_overflow";
        return plan;
      }
      plan.patches.push_back(patch);
    }
  }

  plan.sourceColumnCount = plan.patches.size();
  plan.accepted = true;
  plan.status = plan.patches.empty() ? CreativeTerrainRenderPlanStatus::Empty
                                    : CreativeTerrainRenderPlanStatus::Ready;
  plan.reasonCode = plan.patches.empty()
                        ? "creative_terrain_render_region_empty"
                        : "creative_terrain_render_region_ready";
  return plan;
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

CreativeTerrainMutationPreviewReceipt buildCreativeTerrainMutationPreview(
    const CreativeTerrainField& field,
    std::span<const CreativeTerrainControlEdit> edits,
    CreativeTerrainPatchRegion region,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount) {
  CreativeTerrainMutationPreviewReceipt receipt;
  receipt.requested = true;
  receipt.regionLimited = true;
  receipt.regionMinimum = region.minimum;
  receipt.regionMaximum = region.maximum;
  if (!field.validateInvariants()) {
    receipt.status = CreativeTerrainMutationPreviewStatus::InvalidField;
    receipt.reasonCode = "creative_terrain_preview_region_field_invalid";
    return receipt;
  }

  CreativeTerrainField previewField = field;
  if (!edits.empty()) {
    receipt.mutation = previewField.apply(edits);
    if (!receipt.mutation.accepted) {
      receipt.status = CreativeTerrainMutationPreviewStatus::MutationRejected;
      receipt.reasonCode = "creative_terrain_preview_region_mutation_rejected";
      return receipt;
    }
  }
  receipt.render = buildCreativeTerrainRenderPlan(
      previewField, region, gridOrigin, cellSize, maxPatchCount);
  if (!receipt.render.accepted) {
    receipt.status = CreativeTerrainMutationPreviewStatus::RenderRejected;
    receipt.reasonCode = "creative_terrain_preview_region_render_rejected";
    return receipt;
  }

  const std::uint64_t width =
      static_cast<std::uint64_t>(static_cast<std::int64_t>(region.maximum.x) -
                                 region.minimum.x + 1);
  const std::uint64_t depth =
      static_cast<std::uint64_t>(static_cast<std::int64_t>(region.maximum.z) -
                                 region.minimum.z + 1);
  receipt.candidatePatchCoordinateCount = width * depth;
  receipt.sampledColumnCoordinateCount = (width + 2U) * (depth + 2U);
  receipt.accepted = true;
  receipt.status = CreativeTerrainMutationPreviewStatus::Ready;
  receipt.reasonCode = "creative_terrain_preview_region_ready";
  return receipt;
}

}  // namespace iggy3d::creative
