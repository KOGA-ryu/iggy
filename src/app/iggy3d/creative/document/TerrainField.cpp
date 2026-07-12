#include "app/iggy3d/creative/document/TerrainField.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool coordLess(CreativeTerrainCoord2 lhs,
                             CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z != rhs.z ? lhs.z < rhs.z : lhs.x < rhs.x;
}

[[nodiscard]] bool controlLess(const CreativeTerrainControlPoint& lhs,
                               const CreativeTerrainControlPoint& rhs) noexcept {
  return coordLess(lhs.coord, rhs.coord);
}

[[nodiscard]] bool editLess(const CreativeTerrainControlEdit& lhs,
                            const CreativeTerrainControlEdit& rhs) noexcept {
  return coordLess(lhs.control.coord, rhs.control.coord);
}

[[nodiscard]] bool coordinateAllowsMaximumRadius(
    CreativeTerrainCoord2 coord) noexcept {
  constexpr std::int32_t margin = kCreativeTerrainMaximumRadiusCells;
  constexpr std::int32_t minimum =
      std::numeric_limits<std::int32_t>::min() + margin;
  constexpr std::int32_t maximum =
      std::numeric_limits<std::int32_t>::max() - margin;
  return coord.x >= minimum && coord.x <= maximum && coord.z >= minimum &&
         coord.z <= maximum;
}

using ControlIterator = std::vector<CreativeTerrainControlPoint>::iterator;
using ConstControlIterator =
    std::vector<CreativeTerrainControlPoint>::const_iterator;

[[nodiscard]] ControlIterator findControl(
    std::vector<CreativeTerrainControlPoint>& controls,
    CreativeTerrainCoord2 coord) noexcept {
  const ControlIterator found = std::lower_bound(
      controls.begin(), controls.end(), coord,
      [](const CreativeTerrainControlPoint& control, CreativeTerrainCoord2 value) {
        return coordLess(control.coord, value);
      });
  return found != controls.end() && found->coord == coord ? found
                                                          : controls.end();
}

[[nodiscard]] ConstControlIterator findControl(
    const std::vector<CreativeTerrainControlPoint>& controls,
    CreativeTerrainCoord2 coord) noexcept {
  const ConstControlIterator found = std::lower_bound(
      controls.begin(), controls.end(), coord,
      [](const CreativeTerrainControlPoint& control, CreativeTerrainCoord2 value) {
        return coordLess(control.coord, value);
      });
  return found != controls.end() && found->coord == coord ? found
                                                          : controls.end();
}

struct TerrainContribution {
  CreativeTerrainCoord2 coord{};
  std::uint32_t weight = 0;
  std::uint64_t weightedHeight = 0;
};

[[nodiscard]] bool contributionLess(const TerrainContribution& lhs,
                                    const TerrainContribution& rhs) noexcept {
  return coordLess(lhs.coord, rhs.coord);
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

bool isValidCreativeTerrainControlPoint(
    CreativeTerrainControlPoint control) noexcept {
  return coordinateAllowsMaximumRadius(control.coord) &&
         control.heightCells >= kCreativeTerrainMinimumHeightCells &&
         control.heightCells <= kCreativeTerrainMaximumHeightCells &&
         control.radiusCells >= kCreativeTerrainMinimumRadiusCells &&
         control.radiusCells <= kCreativeTerrainMaximumRadiusCells;
}

std::string_view toString(CreativeTerrainMutationStatus status) noexcept {
  switch (status) {
    case CreativeTerrainMutationStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainMutationStatus::InvalidField:
      return "InvalidField";
    case CreativeTerrainMutationStatus::InvalidEdit:
      return "InvalidEdit";
    case CreativeTerrainMutationStatus::DuplicateCoordinate:
      return "DuplicateCoordinate";
    case CreativeTerrainMutationStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainMutationStatus::NoChange:
      return "NoChange";
    case CreativeTerrainMutationStatus::Applied:
      return "Applied";
  }
  return "Unknown";
}

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

bool CreativeTerrainField::isValid() const noexcept {
  return valid_;
}

bool CreativeTerrainField::validateInvariants() const noexcept {
  if (!valid_ || controls_.size() > kCreativeTerrainControlCapacity) {
    return false;
  }
  for (std::size_t index = 0; index < controls_.size(); ++index) {
    if (!isValidCreativeTerrainControlPoint(controls_[index]) ||
        (index > 0U &&
         !coordLess(controls_[index - 1U].coord, controls_[index].coord))) {
      return false;
    }
  }
  return true;
}

std::uint64_t CreativeTerrainField::revision() const noexcept {
  return revision_;
}

std::uint64_t CreativeTerrainField::controlCount() const noexcept {
  return controls_.size();
}

std::span<const CreativeTerrainControlPoint> CreativeTerrainField::controls()
    const noexcept {
  return controls_;
}

const CreativeTerrainControlPoint* CreativeTerrainField::controlAt(
    CreativeTerrainCoord2 coord) const noexcept {
  const ConstControlIterator found = findControl(controls_, coord);
  return found == controls_.end() ? nullptr : &*found;
}

CreativeTerrainMutationReceipt CreativeTerrainField::apply(
    std::span<const CreativeTerrainControlEdit> edits) {
  CreativeTerrainMutationReceipt receipt;
  receipt.requested = true;
  receipt.revisionBefore = revision_;
  receipt.revisionAfter = revision_;
  receipt.attemptedEditCount = edits.size();
  receipt.controlCountBefore = controls_.size();
  receipt.controlCountAfter = controls_.size();
  if (!validateInvariants()) {
    receipt.status = CreativeTerrainMutationStatus::InvalidField;
    receipt.reasonCode = "creative_terrain_field_invalid";
    return receipt;
  }

  std::vector<CreativeTerrainControlEdit> ordered{edits.begin(), edits.end()};
  for (const CreativeTerrainControlEdit& edit : ordered) {
    if (edit.kind >= CreativeTerrainEditKind::Count ||
        !coordinateAllowsMaximumRadius(edit.control.coord) ||
        (edit.kind == CreativeTerrainEditKind::Upsert &&
         !isValidCreativeTerrainControlPoint(edit.control))) {
      receipt.status = CreativeTerrainMutationStatus::InvalidEdit;
      receipt.reasonCode = "creative_terrain_edit_invalid";
      return receipt;
    }
  }
  std::sort(ordered.begin(), ordered.end(), editLess);
  for (std::size_t index = 1U; index < ordered.size(); ++index) {
    if (ordered[index - 1U].control.coord == ordered[index].control.coord) {
      receipt.status = CreativeTerrainMutationStatus::DuplicateCoordinate;
      receipt.reasonCode = "creative_terrain_duplicate_coordinate";
      return receipt;
    }
  }

  std::vector<CreativeTerrainControlPoint> staged = controls_;
  for (const CreativeTerrainControlEdit& edit : ordered) {
    ControlIterator found = findControl(staged, edit.control.coord);
    if (edit.kind == CreativeTerrainEditKind::Remove) {
      if (found != staged.end()) {
        staged.erase(found);
        ++receipt.changedControlCount;
      }
      continue;
    }
    if (found == staged.end()) {
      const ControlIterator insertion = std::lower_bound(
          staged.begin(), staged.end(), edit.control, controlLess);
      staged.insert(insertion, edit.control);
      ++receipt.changedControlCount;
    } else if (!(*found == edit.control)) {
      *found = edit.control;
      ++receipt.changedControlCount;
    }
  }
  if (staged.size() > kCreativeTerrainControlCapacity) {
    receipt.changedControlCount = 0U;
    receipt.status = CreativeTerrainMutationStatus::CapacityExceeded;
    receipt.reasonCode = "creative_terrain_control_capacity_exceeded";
    return receipt;
  }
  if (receipt.changedControlCount == 0U) {
    receipt.accepted = true;
    receipt.status = CreativeTerrainMutationStatus::NoChange;
    receipt.reasonCode = "creative_terrain_no_change";
    return receipt;
  }

  controls_ = std::move(staged);
  ++revision_;
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeTerrainMutationStatus::Applied;
  receipt.revisionAfter = revision_;
  receipt.controlCountAfter = controls_.size();
  receipt.reasonCode = "creative_terrain_applied";
  return receipt;
}

void CreativeTerrainField::clear() noexcept {
  controls_.clear();
  revision_ = 0;
  valid_ = true;
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
        const std::uint32_t base = static_cast<std::uint32_t>(
            radiusSquared - distanceSquared + 1);
        const std::uint32_t weight = base * base;
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

}  // namespace iggy3d::creative
