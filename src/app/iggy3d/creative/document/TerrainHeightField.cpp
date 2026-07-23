#include "app/iggy3d/creative/document/TerrainHeightField.hpp"

#include "app/iggy3d/creative/document/TerrainFieldInternal.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <utility>

namespace iggy3d::creative {
namespace {

[[nodiscard]] std::size_t fieldCellCount(
    CreativeTerrainHeightFieldBounds bounds) noexcept {
  return static_cast<std::size_t>(bounds.widthCells) * bounds.depthCells;
}

[[nodiscard]] bool validHeight(std::uint16_t height) noexcept {
  return height == kCreativeTerrainEmptyHeightCells ||
         (height >= kCreativeTerrainMinimumHeightCells &&
          height <= kCreativeTerrainMaximumHeightCells);
}

[[nodiscard]] bool coordLess(CreativeTerrainCoord2 lhs,
                             CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z != rhs.z ? lhs.z < rhs.z : lhs.x < rhs.x;
}

[[nodiscard]] bool hardEdgeLess(CreativeTerrainHardEdge lhs,
                                CreativeTerrainHardEdge rhs) noexcept {
  return lhs.first != rhs.first ? coordLess(lhs.first, rhs.first)
                                : coordLess(lhs.second, rhs.second);
}

[[nodiscard]] bool coordInsideBounds(
    CreativeTerrainCoord2 coord,
    CreativeTerrainHeightFieldBounds bounds) noexcept {
  const std::int64_t offsetX =
      static_cast<std::int64_t>(coord.x) - bounds.minimum.x;
  const std::int64_t offsetZ =
      static_cast<std::int64_t>(coord.z) - bounds.minimum.z;
  return offsetX >= 0 && offsetX < bounds.widthCells && offsetZ >= 0 &&
         offsetZ < bounds.depthCells;
}

[[nodiscard]] bool validSurfaceColumns(
    std::span<const CreativeTerrainColumn> columns) noexcept {
  for (std::size_t index = 0U; index < columns.size(); ++index) {
    if (!validHeight(columns[index].heightCells) ||
        columns[index].heightCells == kCreativeTerrainEmptyHeightCells ||
        (index > 0U &&
         !coordLess(columns[index - 1U].coord, columns[index].coord))) {
      return false;
    }
  }
  return true;
}

void rebuildTerrainCuboids(CreativeTerrainSurfacePlan& plan) {
  plan.cuboids.clear();
  for (std::size_t rowBegin = 0U; rowBegin < plan.columns.size();) {
    std::size_t rowEnd = rowBegin + 1U;
    while (rowEnd < plan.columns.size() &&
           plan.columns[rowEnd].coord.z ==
               plan.columns[rowBegin].coord.z) {
      ++rowEnd;
    }
    terrain_field_internal::appendTerrainRowCuboids(
        std::span<const CreativeTerrainColumn>{
            plan.columns.data() + rowBegin, rowEnd - rowBegin},
        plan.cuboids);
    rowBegin = rowEnd;
  }
}

}  // namespace

CreativeTerrainHardEdge canonicalCreativeTerrainHardEdge(
    CreativeTerrainCoord2 first,
    CreativeTerrainCoord2 second) noexcept {
  return coordLess(second, first) ? CreativeTerrainHardEdge{second, first}
                                  : CreativeTerrainHardEdge{first, second};
}

bool isValidCreativeTerrainHardEdge(CreativeTerrainHardEdge edge) noexcept {
  const std::int64_t deltaX =
      static_cast<std::int64_t>(edge.second.x) - edge.first.x;
  const std::int64_t deltaZ =
      static_cast<std::int64_t>(edge.second.z) - edge.first.z;
  return coordLess(edge.first, edge.second) &&
         std::abs(deltaX) + std::abs(deltaZ) == 1;
}

bool validateCreativeTerrainHardEdges(
    std::span<const CreativeTerrainHardEdge> edges) noexcept {
  if (edges.size() > kCreativeTerrainHardEdgeCapacity) {
    return false;
  }
  for (std::size_t index = 0U; index < edges.size(); ++index) {
    if (!isValidCreativeTerrainHardEdge(edges[index]) ||
        (index > 0U && !hardEdgeLess(edges[index - 1U], edges[index]))) {
      return false;
    }
  }
  return true;
}

bool validateCreativeTerrainHardEdgesForSurface(
    std::span<const CreativeTerrainColumn> columns,
    std::span<const CreativeTerrainHardEdge> edges) noexcept {
  if (!validSurfaceColumns(columns) ||
      !validateCreativeTerrainHardEdges(edges)) {
    return false;
  }
  const auto heightAt = [&](CreativeTerrainCoord2 coord) {
    const auto found = std::lower_bound(
        columns.begin(), columns.end(), coord,
        [](const CreativeTerrainColumn& column, CreativeTerrainCoord2 value) {
          return coordLess(column.coord, value);
        });
    return found != columns.end() && found->coord == coord
               ? found->heightCells
               : kCreativeTerrainEmptyHeightCells;
  };
  return std::all_of(
      edges.begin(), edges.end(), [&](CreativeTerrainHardEdge edge) {
        return heightAt(edge.first) != heightAt(edge.second);
      });
}

bool isValidCreativeTerrainHeightFieldBounds(
    CreativeTerrainHeightFieldBounds bounds) noexcept {
  if (bounds.widthCells == 0U || bounds.depthCells == 0U ||
      fieldCellCount(bounds) > kCreativeTerrainHeightFieldCellCapacity) {
    return false;
  }
  const std::int64_t maximumX =
      static_cast<std::int64_t>(bounds.minimum.x) + bounds.widthCells - 1;
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(bounds.minimum.z) + bounds.depthCells - 1;
  return maximumX <= std::numeric_limits<std::int32_t>::max() &&
         maximumZ <= std::numeric_limits<std::int32_t>::max();
}

std::string_view toString(
    CreativeTerrainHeightFieldReplaceStatus status) noexcept {
  switch (status) {
    case CreativeTerrainHeightFieldReplaceStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainHeightFieldReplaceStatus::InvalidField:
      return "InvalidField";
    case CreativeTerrainHeightFieldReplaceStatus::InvalidBounds:
      return "InvalidBounds";
    case CreativeTerrainHeightFieldReplaceStatus::InvalidHeights:
      return "InvalidHeights";
    case CreativeTerrainHeightFieldReplaceStatus::NoChange:
      return "NoChange";
    case CreativeTerrainHeightFieldReplaceStatus::Applied:
      return "Applied";
  }
  return "Unknown";
}

bool CreativeTerrainHeightField::isValid() const noexcept {
  return valid_;
}

bool CreativeTerrainHeightField::validateInvariants() const noexcept {
  if (!valid_) {
    return false;
  }
  if (heights_.empty()) {
    return bounds_ == CreativeTerrainHeightFieldBounds{};
  }
  return isValidCreativeTerrainHeightFieldBounds(bounds_) &&
         heights_.size() == fieldCellCount(bounds_) &&
         std::all_of(heights_.begin(), heights_.end(), validHeight);
}

std::uint64_t CreativeTerrainHeightField::revision() const noexcept {
  return revision_;
}

CreativeTerrainHeightFieldBounds CreativeTerrainHeightField::bounds()
    const noexcept {
  return bounds_;
}

std::uint64_t CreativeTerrainHeightField::cellCount() const noexcept {
  return heights_.size();
}

std::uint64_t CreativeTerrainHeightField::presentCellCount() const noexcept {
  return static_cast<std::uint64_t>(std::count_if(
      heights_.begin(), heights_.end(), [](std::uint16_t height) {
        return height != kCreativeTerrainEmptyHeightCells;
      }));
}

std::span<const std::uint16_t> CreativeTerrainHeightField::heights()
    const noexcept {
  return heights_;
}

bool CreativeTerrainHeightField::contains(
    CreativeTerrainCoord2 coord) const noexcept {
  if (heights_.empty()) {
    return false;
  }
  const std::int64_t offsetX =
      static_cast<std::int64_t>(coord.x) - bounds_.minimum.x;
  const std::int64_t offsetZ =
      static_cast<std::int64_t>(coord.z) - bounds_.minimum.z;
  return offsetX >= 0 && offsetX < bounds_.widthCells && offsetZ >= 0 &&
         offsetZ < bounds_.depthCells;
}

std::optional<std::uint16_t> CreativeTerrainHeightField::heightAt(
    CreativeTerrainCoord2 coord) const noexcept {
  if (!contains(coord)) {
    return std::nullopt;
  }
  const std::size_t offsetX = static_cast<std::size_t>(
      static_cast<std::int64_t>(coord.x) - bounds_.minimum.x);
  const std::size_t offsetZ = static_cast<std::size_t>(
      static_cast<std::int64_t>(coord.z) - bounds_.minimum.z);
  return heights_[offsetZ * bounds_.widthCells + offsetX];
}

CreativeTerrainHeightFieldReplaceReceipt CreativeTerrainHeightField::replace(
    CreativeTerrainHeightFieldBounds bounds,
    std::span<const std::uint16_t> heights) {
  CreativeTerrainHeightFieldReplaceReceipt receipt;
  receipt.requested = true;
  receipt.revisionBefore = revision_;
  receipt.revisionAfter = revision_;
  receipt.cellCountBefore = heights_.size();
  receipt.cellCountAfter = heights_.size();
  if (!validateInvariants()) {
    receipt.status = CreativeTerrainHeightFieldReplaceStatus::InvalidField;
    receipt.reasonCode = "creative_terrain_height_field_invalid";
    return receipt;
  }
  if (bounds == CreativeTerrainHeightFieldBounds{} && heights.empty()) {
    receipt.accepted = true;
    if (heights_.empty()) {
      receipt.status = CreativeTerrainHeightFieldReplaceStatus::NoChange;
      receipt.reasonCode = "creative_terrain_height_field_no_change";
      return receipt;
    }
    bounds_ = {};
    heights_.clear();
    ++revision_;
    receipt.changed = true;
    receipt.status = CreativeTerrainHeightFieldReplaceStatus::Applied;
    receipt.revisionAfter = revision_;
    receipt.cellCountAfter = 0U;
    receipt.reasonCode = "creative_terrain_height_field_cleared";
    return receipt;
  }
  if (!isValidCreativeTerrainHeightFieldBounds(bounds)) {
    receipt.status = CreativeTerrainHeightFieldReplaceStatus::InvalidBounds;
    receipt.reasonCode = "creative_terrain_height_field_bounds_invalid";
    return receipt;
  }
  if (heights.size() != fieldCellCount(bounds) ||
      !std::all_of(heights.begin(), heights.end(), validHeight)) {
    receipt.status = CreativeTerrainHeightFieldReplaceStatus::InvalidHeights;
    receipt.reasonCode = "creative_terrain_height_field_heights_invalid";
    return receipt;
  }
  if (bounds == bounds_ && std::equal(heights.begin(), heights.end(),
                                     heights_.begin(), heights_.end())) {
    receipt.accepted = true;
    receipt.status = CreativeTerrainHeightFieldReplaceStatus::NoChange;
    receipt.reasonCode = "creative_terrain_height_field_no_change";
    return receipt;
  }

  bounds_ = bounds;
  heights_.assign(heights.begin(), heights.end());
  ++revision_;
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeTerrainHeightFieldReplaceStatus::Applied;
  receipt.revisionAfter = revision_;
  receipt.cellCountAfter = heights_.size();
  receipt.reasonCode = "creative_terrain_height_field_applied";
  return receipt;
}

void CreativeTerrainHeightField::clear() noexcept {
  bounds_ = {};
  heights_.clear();
  revision_ = 0U;
  valid_ = true;
}

CreativeTerrainSurfacePlan buildCreativeTerrainHeightSurfacePlan(
    const CreativeTerrainHeightField& field) {
  CreativeTerrainSurfacePlan plan;
  plan.requested = true;
  plan.sourceRevision = field.revision();
  if (!field.validateInvariants()) {
    plan.status = CreativeTerrainSurfacePlanStatus::InvalidField;
    plan.reasonCode = "creative_terrain_height_surface_field_invalid";
    return plan;
  }
  if (field.presentCellCount() == 0U) {
    plan.accepted = true;
    plan.status = CreativeTerrainSurfacePlanStatus::Empty;
    plan.reasonCode = "creative_terrain_height_surface_empty";
    return plan;
  }

  const CreativeTerrainHeightFieldBounds bounds = field.bounds();
  const std::uint64_t presentCellCount = field.presentCellCount();
  plan.columns.reserve(static_cast<std::size_t>(presentCellCount));
  for (std::uint16_t z = 0U; z < bounds.depthCells; ++z) {
    const std::size_t rowBegin = plan.columns.size();
    for (std::uint16_t x = 0U; x < bounds.widthCells; ++x) {
      const CreativeTerrainCoord2 coord{
          bounds.minimum.x + static_cast<std::int32_t>(x),
          bounds.minimum.z + static_cast<std::int32_t>(z)};
      const std::uint16_t height = field.heightAt(coord).value_or(
          kCreativeTerrainEmptyHeightCells);
      if (height == kCreativeTerrainEmptyHeightCells) {
        continue;
      }
      plan.columns.push_back({coord, height});
    }
    if (plan.columns.size() != rowBegin) {
      terrain_field_internal::appendTerrainRowCuboids(
          std::span<const CreativeTerrainColumn>{
              plan.columns.data() + rowBegin,
              plan.columns.size() - rowBegin},
          plan.cuboids);
    }
  }

  plan.contributionCount = plan.columns.size();
  plan.accepted = true;
  plan.status = CreativeTerrainSurfacePlanStatus::Ready;
  plan.reasonCode = "creative_terrain_height_surface_ready";
  return plan;
}

CreativeTerrainSurfacePlan replaceCreativeTerrainSurfaceRegion(
    const CreativeTerrainSurfacePlan& base,
    const CreativeTerrainHeightField& replacement) {
  CreativeTerrainSurfacePlan plan;
  plan.requested = true;
  plan.sourceRevision = replacement.revision();
  if (!replacement.validateInvariants() ||
      !isValidCreativeTerrainHeightFieldBounds(replacement.bounds()) ||
      !base.accepted ||
      (base.status != CreativeTerrainSurfacePlanStatus::Empty &&
       base.status != CreativeTerrainSurfacePlanStatus::Ready) ||
      (base.status == CreativeTerrainSurfacePlanStatus::Empty &&
       !base.columns.empty()) ||
      !validSurfaceColumns(base.columns) ||
      !validateCreativeTerrainHardEdges(base.hardEdges)) {
    plan.status = CreativeTerrainSurfacePlanStatus::InvalidField;
    plan.reasonCode = "creative_terrain_height_composition_invalid";
    return plan;
  }

  const CreativeTerrainSurfacePlan replacementPlan =
      buildCreativeTerrainHeightSurfacePlan(replacement);
  if (!replacementPlan.accepted) {
    plan.status = CreativeTerrainSurfacePlanStatus::InvalidField;
    plan.reasonCode = "creative_terrain_height_replacement_invalid";
    return plan;
  }

  plan.columns.reserve(base.columns.size() + replacementPlan.columns.size());
  for (const CreativeTerrainColumn& column : base.columns) {
    if (!coordInsideBounds(column.coord, replacement.bounds())) {
      plan.columns.push_back(column);
    }
  }
  plan.columns.insert(plan.columns.end(), replacementPlan.columns.begin(),
                      replacementPlan.columns.end());
  std::sort(plan.columns.begin(), plan.columns.end(),
            [](const CreativeTerrainColumn& lhs,
               const CreativeTerrainColumn& rhs) {
              return coordLess(lhs.coord, rhs.coord);
            });
  rebuildTerrainCuboids(plan);
  for (const CreativeTerrainHardEdge edge : base.hardEdges) {
    if (!coordInsideBounds(edge.first, replacement.bounds()) &&
        !coordInsideBounds(edge.second, replacement.bounds())) {
      plan.hardEdges.push_back(edge);
    }
  }
  plan.contributionCount = plan.columns.size();
  plan.accepted = true;
  plan.status = plan.columns.empty()
                    ? CreativeTerrainSurfacePlanStatus::Empty
                    : CreativeTerrainSurfacePlanStatus::Ready;
  plan.reasonCode = plan.columns.empty()
                        ? "creative_terrain_height_composition_empty"
                        : "creative_terrain_height_composition_ready";
  return plan;
}

CreativeTerrainSurfacePlan buildCreativeComposedTerrainSurfacePlan(
    const CreativeTerrainField& legacy,
    const CreativeTerrainHeightField& authored,
    std::span<const CreativeTerrainHardEdge> hardEdges) {
  CreativeTerrainSurfacePlan base =
      buildCreativeTerrainSurfacePlan(legacy);
  if (!base.accepted || !validateCreativeTerrainHardEdges(hardEdges)) {
    if (!validateCreativeTerrainHardEdges(hardEdges)) {
      base.accepted = false;
      base.status = CreativeTerrainSurfacePlanStatus::InvalidField;
      base.reasonCode = "creative_terrain_hard_edges_invalid";
    }
    return base;
  }
  CreativeTerrainSurfacePlan plan = authored.cellCount() == 0U
                                        ? std::move(base)
                                        : replaceCreativeTerrainSurfaceRegion(
                                              base, authored);
  if (!plan.accepted) {
    return plan;
  }
  if (!validateCreativeTerrainHardEdgesForSurface(plan.columns, hardEdges)) {
    plan.accepted = false;
    plan.status = CreativeTerrainSurfacePlanStatus::InvalidField;
    plan.columns.clear();
    plan.cuboids.clear();
    plan.reasonCode = "creative_terrain_hard_edges_do_not_match_surface";
    return plan;
  }
  plan.hardEdges.assign(hardEdges.begin(), hardEdges.end());
  return plan;
}

CreativeTerrainRenderPlan buildCreativeTerrainHeightRenderPlan(
    const CreativeTerrainHeightField& field,
    CreativeVec3 gridOrigin,
    double cellSize,
    std::size_t maxPatchCount) {
  return buildCreativeTerrainRenderPlan(
      buildCreativeTerrainHeightSurfacePlan(field), gridOrigin, cellSize,
      maxPatchCount);
}

}  // namespace iggy3d::creative
