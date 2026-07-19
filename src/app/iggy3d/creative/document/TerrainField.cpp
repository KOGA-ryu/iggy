#include "app/iggy3d/creative/document/TerrainField.hpp"

#include "app/iggy3d/creative/document/TerrainFieldInternal.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace iggy3d::creative::terrain_field_internal {

[[nodiscard]] std::uint32_t terrainContributionWeight(
    const CreativeTerrainControlPoint& control,
    CreativeTerrainCoord2 coord) noexcept {
  const std::int64_t dx = static_cast<std::int64_t>(coord.x) - control.coord.x;
  const std::int64_t dz = static_cast<std::int64_t>(coord.z) - control.coord.z;
  const std::int64_t radius = control.radiusCells;
  const std::int64_t radiusSquared = radius * radius;
  const std::int64_t distanceSquared = dx * dx + dz * dz;
  if (distanceSquared > radiusSquared) {
    return 0U;
  }
  const std::uint32_t base = static_cast<std::uint32_t>(
      radiusSquared - distanceSquared + 1);
  return base * base;
}

[[nodiscard]] CreativeTerrainHeightSample sampleTerrainHeightUnchecked(
    std::span<const CreativeTerrainControlPoint> controls,
    CreativeTerrainCoord2 coord) noexcept {
  CreativeTerrainHeightSample sample;
  sample.coord = coord;
  std::uint64_t weightedHeight = 0U;
  for (const CreativeTerrainControlPoint& control : controls) {
    const std::uint32_t weight = terrainContributionWeight(control, coord);
    if (weight == 0U) {
      continue;
    }
    sample.totalWeight += weight;
    weightedHeight += static_cast<std::uint64_t>(weight) * control.heightCells;
    ++sample.contributingControlCount;
  }
  if (sample.totalWeight == 0U) {
    return sample;
  }
  const std::uint64_t roundedHeight =
      (weightedHeight + sample.totalWeight / 2U) / sample.totalWeight;
  sample.heightCells = static_cast<std::uint16_t>(
      std::clamp<std::uint64_t>(roundedHeight,
                                kCreativeTerrainMinimumHeightCells,
                                kCreativeTerrainMaximumHeightCells));
  sample.present = true;
  return sample;
}

namespace {

[[nodiscard]] std::int32_t floorDivByVoxelChunk(
    std::int32_t value) noexcept {
  std::int32_t quotient = value / kCreativeVoxelChunkEdge;
  if (value % kCreativeVoxelChunkEdge < 0) {
    --quotient;
  }
  return quotient;
}

}  // namespace

void appendTerrainRowCuboids(
    std::span<const CreativeTerrainColumn> row,
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

}  // namespace iggy3d::creative::terrain_field_internal

namespace iggy3d::creative {

using terrain_field_internal::coordLess;
using terrain_field_internal::sampleTerrainHeightUnchecked;

namespace {

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

CreativeTerrainHeightSample sampleCreativeTerrainHeight(
    const CreativeTerrainField& field,
    CreativeTerrainCoord2 coord) noexcept {
  if (!field.validateInvariants()) {
    CreativeTerrainHeightSample invalid;
    invalid.coord = coord;
    return invalid;
  }
  return sampleTerrainHeightUnchecked(field.controls(), coord);
}

}  // namespace iggy3d::creative
