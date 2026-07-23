#include "app/iggy3d/creative/document/TerrainMaterialField.hpp"

#include <algorithm>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool coordLess(CreativeTerrainCoord2 lhs,
                             CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z != rhs.z ? lhs.z < rhs.z : lhs.x < rhs.x;
}

using ConstIterator =
    std::vector<CreativeTerrainMaterialOverride>::const_iterator;

[[nodiscard]] ConstIterator lowerBound(
    const std::vector<CreativeTerrainMaterialOverride>& values,
    CreativeTerrainCoord2 coord) noexcept {
  return std::lower_bound(
      values.begin(), values.end(), coord,
      [](const CreativeTerrainMaterialOverride& value,
         CreativeTerrainCoord2 candidate) {
        return coordLess(value.coord, candidate);
      });
}

[[nodiscard]] bool validEdit(const CreativeTerrainMaterialEdit& edit) noexcept {
  if (edit.kind == CreativeTerrainMaterialEditKind::Clear) {
    return true;
  }
  if (edit.kind == CreativeTerrainMaterialEditKind::Set) {
    return isValidCreativeTerrainMaterial(edit.material) &&
           edit.material != CreativeTerrainMaterial::Grass;
  }
  return edit.kind == CreativeTerrainMaterialEditKind::SetWeights &&
         isValidCreativeTerrainMaterialWeights(edit.weights);
}

[[nodiscard]] bool canonicalGrass(
    const CreativeTerrainMaterialWeights& weights) noexcept {
  return weights ==
         creativeTerrainMaterialSolidWeights(CreativeTerrainMaterial::Grass);
}

[[nodiscard]] bool editKeepsOverride(
    const CreativeTerrainMaterialEdit& edit) noexcept {
  return edit.kind == CreativeTerrainMaterialEditKind::Set ||
         (edit.kind == CreativeTerrainMaterialEditKind::SetWeights &&
          !canonicalGrass(edit.weights));
}

[[nodiscard]] CreativeTerrainMaterialOverride overrideForEdit(
    const CreativeTerrainMaterialEdit& edit) noexcept {
  return edit.kind == CreativeTerrainMaterialEditKind::SetWeights
             ? CreativeTerrainMaterialOverride{edit.coord, edit.weights}
             : CreativeTerrainMaterialOverride{edit.coord, edit.material};
}

}  // namespace

bool isValidCreativeTerrainMaterialWeights(
    const CreativeTerrainMaterialWeights& weights) noexcept {
  std::uint16_t total = 0U;
  for (std::uint8_t weight : weights) {
    total = static_cast<std::uint16_t>(total + weight);
  }
  return total == kCreativeTerrainMaterialWeightTotal;
}

CreativeTerrainMaterial dominantCreativeTerrainMaterial(
    const CreativeTerrainMaterialWeights& weights) noexcept {
  if (!isValidCreativeTerrainMaterialWeights(weights)) {
    return CreativeTerrainMaterial::Count;
  }
  std::size_t dominant = 0U;
  for (std::size_t index = 1U; index < weights.size(); ++index) {
    if (weights[index] > weights[dominant]) {
      dominant = index;
    }
  }
  return static_cast<CreativeTerrainMaterial>(dominant);
}

CreativeVec3 creativeTerrainMaterialRenderColor(
    const CreativeTerrainMaterialWeights& weights) noexcept {
  constexpr std::array<CreativeVec3, kCreativeTerrainMaterialCount> colors{{
      {0.22, 0.52, 0.20},
      {0.42, 0.25, 0.10},
      {0.42, 0.44, 0.46},
      {0.78, 0.68, 0.38},
  }};
  if (!isValidCreativeTerrainMaterialWeights(weights)) {
    return colors.front();
  }
  CreativeVec3 result{};
  for (std::size_t index = 0U; index < weights.size(); ++index) {
    const double amount = static_cast<double>(weights[index]) /
                          static_cast<double>(kCreativeTerrainMaterialWeightTotal);
    result.x += colors[index].x * amount;
    result.y += colors[index].y * amount;
    result.z += colors[index].z * amount;
  }
  return result;
}

CreativeTerrainMaterialOverride::CreativeTerrainMaterialOverride(
    CreativeTerrainCoord2 sourceCoord,
    CreativeTerrainMaterialWeights sourceWeights) noexcept
    : coord(sourceCoord),
      material(dominantCreativeTerrainMaterial(sourceWeights)),
      weights(sourceWeights) {}

CreativeTerrainMaterialEdit makeCreativeTerrainMaterialWeightEdit(
    CreativeTerrainCoord2 coord,
    const CreativeTerrainMaterialWeights& weights) noexcept {
  CreativeTerrainMaterialEdit edit;
  edit.kind = CreativeTerrainMaterialEditKind::SetWeights;
  edit.coord = coord;
  edit.material = dominantCreativeTerrainMaterial(weights);
  edit.weights = weights;
  return edit;
}

bool isValidCreativeTerrainMaterial(CreativeTerrainMaterial material) noexcept {
  return material >= CreativeTerrainMaterial::Grass &&
         material < CreativeTerrainMaterial::Count;
}

std::string_view toString(CreativeTerrainMaterial material) noexcept {
  switch (material) {
    case CreativeTerrainMaterial::Grass: return "Grass";
    case CreativeTerrainMaterial::Dirt: return "Dirt";
    case CreativeTerrainMaterial::Stone: return "Stone";
    case CreativeTerrainMaterial::Sand: return "Sand";
    case CreativeTerrainMaterial::Count: break;
  }
  return "Unknown";
}

bool parseCreativeTerrainMaterial(std::string_view value,
                                  CreativeTerrainMaterial& output) noexcept {
  for (std::uint8_t index = 0U;
       index < static_cast<std::uint8_t>(CreativeTerrainMaterial::Count);
       ++index) {
    const CreativeTerrainMaterial candidate =
        static_cast<CreativeTerrainMaterial>(index);
    if (toString(candidate) == value) {
      output = candidate;
      return true;
    }
  }
  return false;
}

std::string_view toString(
    CreativeTerrainMaterialMutationStatus status) noexcept {
  switch (status) {
    case CreativeTerrainMaterialMutationStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainMaterialMutationStatus::InvalidField:
      return "InvalidField";
    case CreativeTerrainMaterialMutationStatus::InvalidEdit:
      return "InvalidEdit";
    case CreativeTerrainMaterialMutationStatus::DuplicateCoordinate:
      return "DuplicateCoordinate";
    case CreativeTerrainMaterialMutationStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainMaterialMutationStatus::NoChange: return "NoChange";
    case CreativeTerrainMaterialMutationStatus::Applied: return "Applied";
  }
  return "Unknown";
}

std::string_view creativeTerrainMaterialRenderRole(
    CreativeTerrainMaterial material) noexcept {
  switch (material) {
    case CreativeTerrainMaterial::Grass: return "terrain_grass";
    case CreativeTerrainMaterial::Dirt: return "terrain_dirt";
    case CreativeTerrainMaterial::Stone: return "terrain_stone";
    case CreativeTerrainMaterial::Sand: return "terrain_sand";
    case CreativeTerrainMaterial::Count: break;
  }
  return "terrain_grass";
}

bool CreativeTerrainMaterialField::isValid() const noexcept {
  return valid_;
}

bool CreativeTerrainMaterialField::validateInvariants() const noexcept {
  if (!valid_ || overrides_.size() > kCreativeTerrainMaterialOverrideCapacity) {
    return false;
  }
  for (std::size_t index = 0U; index < overrides_.size(); ++index) {
    const CreativeTerrainMaterialOverride& value = overrides_[index];
    if (!isValidCreativeTerrainMaterialWeights(value.weights) ||
        value.material != dominantCreativeTerrainMaterial(value.weights) ||
        canonicalGrass(value.weights) ||
        (index > 0U && !coordLess(overrides_[index - 1U].coord, value.coord))) {
      return false;
    }
  }
  return true;
}

std::uint64_t CreativeTerrainMaterialField::revision() const noexcept {
  return revision_;
}

std::uint64_t CreativeTerrainMaterialField::overrideCount() const noexcept {
  return overrides_.size();
}

std::span<const CreativeTerrainMaterialOverride>
CreativeTerrainMaterialField::overrides() const noexcept {
  return overrides_;
}

const CreativeTerrainMaterialOverride* CreativeTerrainMaterialField::overrideAt(
    CreativeTerrainCoord2 coord) const noexcept {
  const ConstIterator found = lowerBound(overrides_, coord);
  return found != overrides_.end() && found->coord == coord ? &*found : nullptr;
}

CreativeTerrainMaterial CreativeTerrainMaterialField::materialAt(
    CreativeTerrainCoord2 coord) const noexcept {
  const CreativeTerrainMaterialOverride* found = overrideAt(coord);
  return found == nullptr ? CreativeTerrainMaterial::Grass : found->material;
}

CreativeTerrainMaterialWeights CreativeTerrainMaterialField::weightsAt(
    CreativeTerrainCoord2 coord) const noexcept {
  const CreativeTerrainMaterialOverride* found = overrideAt(coord);
  return found == nullptr
             ? creativeTerrainMaterialSolidWeights(CreativeTerrainMaterial::Grass)
             : found->weights;
}

CreativeTerrainMaterialMutationReceipt CreativeTerrainMaterialField::apply(
    std::span<const CreativeTerrainMaterialEdit> edits) {
  CreativeTerrainMaterialMutationReceipt receipt;
  receipt.requested = true;
  receipt.revisionBefore = revision_;
  receipt.revisionAfter = revision_;
  receipt.attemptedEditCount = edits.size();
  receipt.overrideCountBefore = overrides_.size();
  receipt.overrideCountAfter = overrides_.size();
  if (!validateInvariants()) {
    receipt.status = CreativeTerrainMaterialMutationStatus::InvalidField;
    receipt.reasonCode = "creative_terrain_material_field_invalid";
    return receipt;
  }
  if (edits.size() > kCreativeTerrainMaterialOverrideCapacity) {
    receipt.status = CreativeTerrainMaterialMutationStatus::CapacityExceeded;
    receipt.reasonCode = "creative_terrain_material_edit_capacity_exceeded";
    return receipt;
  }

  std::vector<CreativeTerrainMaterialEdit> ordered{edits.begin(), edits.end()};
  for (const CreativeTerrainMaterialEdit& edit : ordered) {
    if (!validEdit(edit)) {
      receipt.status = CreativeTerrainMaterialMutationStatus::InvalidEdit;
      receipt.reasonCode = "creative_terrain_material_edit_invalid";
      return receipt;
    }
  }
  std::sort(ordered.begin(), ordered.end(),
            [](const CreativeTerrainMaterialEdit& lhs,
               const CreativeTerrainMaterialEdit& rhs) {
              return coordLess(lhs.coord, rhs.coord);
            });
  for (std::size_t index = 1U; index < ordered.size(); ++index) {
    if (ordered[index - 1U].coord == ordered[index].coord) {
      receipt.status =
          CreativeTerrainMaterialMutationStatus::DuplicateCoordinate;
      receipt.reasonCode = "creative_terrain_material_duplicate_coordinate";
      return receipt;
    }
  }

  std::vector<CreativeTerrainMaterialOverride> staged;
  staged.reserve(std::min(
      kCreativeTerrainMaterialOverrideCapacity + 1U,
      overrides_.size() + ordered.size()));
  std::size_t overrideIndex = 0U;
  std::size_t editIndex = 0U;
  while (overrideIndex < overrides_.size() || editIndex < ordered.size()) {
    if (editIndex == ordered.size() ||
        (overrideIndex < overrides_.size() &&
         coordLess(overrides_[overrideIndex].coord,
                   ordered[editIndex].coord))) {
      staged.push_back(overrides_[overrideIndex++]);
      continue;
    }
    const CreativeTerrainMaterialEdit& edit = ordered[editIndex];
    if (overrideIndex == overrides_.size() ||
        coordLess(edit.coord, overrides_[overrideIndex].coord)) {
      if (editKeepsOverride(edit)) {
        staged.push_back(overrideForEdit(edit));
        ++receipt.changedOverrideCount;
      }
      ++editIndex;
      continue;
    }
    const CreativeTerrainMaterialOverride& existing =
        overrides_[overrideIndex];
    if (!editKeepsOverride(edit)) {
      ++receipt.changedOverrideCount;
    } else {
      const CreativeTerrainMaterialOverride replacement = overrideForEdit(edit);
      staged.push_back(replacement);
      if (!(existing == replacement)) {
        ++receipt.changedOverrideCount;
      }
    }
    ++overrideIndex;
    ++editIndex;
  }
  if (staged.size() > kCreativeTerrainMaterialOverrideCapacity) {
    receipt.changedOverrideCount = 0U;
    receipt.status = CreativeTerrainMaterialMutationStatus::CapacityExceeded;
    receipt.reasonCode = "creative_terrain_material_capacity_exceeded";
    return receipt;
  }
  receipt.accepted = true;
  if (staged == overrides_) {
    receipt.status = CreativeTerrainMaterialMutationStatus::NoChange;
    receipt.reasonCode = "creative_terrain_material_no_change";
    return receipt;
  }
  overrides_ = std::move(staged);
  ++revision_;
  receipt.changed = true;
  receipt.revisionAfter = revision_;
  receipt.overrideCountAfter = overrides_.size();
  receipt.status = CreativeTerrainMaterialMutationStatus::Applied;
  receipt.reasonCode = "creative_terrain_material_applied";
  return receipt;
}

void CreativeTerrainMaterialField::clear() noexcept {
  overrides_.clear();
  revision_ = 0U;
  valid_ = true;
}

}  // namespace iggy3d::creative
