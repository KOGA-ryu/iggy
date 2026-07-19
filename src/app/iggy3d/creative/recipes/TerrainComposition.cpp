#include "app/iggy3d/creative/recipes/TerrainComposition.hpp"

#include "core/hash/StableHash.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace iggy3d::creative {
namespace {

inline constexpr std::uint32_t kMaximumMaskWeight = 65535U;

[[nodiscard]] bool coordLess(CreativeTerrainCoord2 lhs,
                             CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z != rhs.z ? lhs.z < rhs.z : lhs.x < rhs.x;
}

[[nodiscard]] bool validCanonicalSource(
    const CreativeTerrainSurfacePlan& source) noexcept {
  if (!source.accepted ||
      (source.status != CreativeTerrainSurfacePlanStatus::Empty &&
       source.status != CreativeTerrainSurfacePlanStatus::Ready)) {
    return false;
  }
  for (std::size_t index = 0U; index < source.columns.size(); ++index) {
    const CreativeTerrainColumn& column = source.columns[index];
    if (column.heightCells < kCreativeTerrainMinimumHeightCells ||
        column.heightCells > kCreativeTerrainMaximumHeightCells ||
        (index > 0U &&
         !coordLess(source.columns[index - 1U].coord, column.coord))) {
      return false;
    }
  }
  return source.status == CreativeTerrainSurfacePlanStatus::Empty
             ? source.columns.empty()
             : !source.columns.empty();
}

[[nodiscard]] bool insideBounds(
    CreativeTerrainCoord2 coord,
    CreativeTerrainHeightFieldBounds bounds) noexcept {
  const std::int64_t offsetX =
      static_cast<std::int64_t>(coord.x) - bounds.minimum.x;
  const std::int64_t offsetZ =
      static_cast<std::int64_t>(coord.z) - bounds.minimum.z;
  return offsetX >= 0 && offsetZ >= 0 && offsetX < bounds.widthCells &&
         offsetZ < bounds.depthCells;
}

[[nodiscard]] bool unionBounds(
    const CreativeTerrainHeightField& existing,
    CreativeTerrainHeightFieldBounds generated,
    CreativeTerrainHeightFieldBounds& output,
    bool& expanded) noexcept {
  if (existing.cellCount() == 0U) {
    output = generated;
    expanded = false;
    return isValidCreativeTerrainHeightFieldBounds(output);
  }
  const CreativeTerrainHeightFieldBounds current = existing.bounds();
  const std::int64_t minimumX =
      std::min<std::int64_t>(current.minimum.x, generated.minimum.x);
  const std::int64_t minimumZ =
      std::min<std::int64_t>(current.minimum.z, generated.minimum.z);
  const std::int64_t maximumXExclusive = std::max(
      static_cast<std::int64_t>(current.minimum.x) + current.widthCells,
      static_cast<std::int64_t>(generated.minimum.x) + generated.widthCells);
  const std::int64_t maximumZExclusive = std::max(
      static_cast<std::int64_t>(current.minimum.z) + current.depthCells,
      static_cast<std::int64_t>(generated.minimum.z) + generated.depthCells);
  const std::int64_t width = maximumXExclusive - minimumX;
  const std::int64_t depth = maximumZExclusive - minimumZ;
  if (minimumX < std::numeric_limits<std::int32_t>::min() ||
      minimumX > std::numeric_limits<std::int32_t>::max() ||
      minimumZ < std::numeric_limits<std::int32_t>::min() ||
      minimumZ > std::numeric_limits<std::int32_t>::max() || width <= 0 ||
      depth <= 0 || width > std::numeric_limits<std::uint16_t>::max() ||
      depth > std::numeric_limits<std::uint16_t>::max()) {
    return false;
  }
  output = {{static_cast<std::int32_t>(minimumX),
             static_cast<std::int32_t>(minimumZ)},
            static_cast<std::uint16_t>(width),
            static_cast<std::uint16_t>(depth)};
  expanded = output != current;
  return isValidCreativeTerrainHeightFieldBounds(output);
}

[[nodiscard]] std::uint32_t rectangleMaskWeight(
    std::uint16_t x,
    std::uint16_t z,
    CreativeTerrainHeightFieldBounds bounds,
    std::uint16_t featherCells) noexcept {
  if (featherCells == 0U) {
    return kMaximumMaskWeight;
  }
  const std::uint32_t distance = std::min({
      static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(z),
      static_cast<std::uint32_t>(bounds.widthCells - 1U - x),
      static_cast<std::uint32_t>(bounds.depthCells - 1U - z),
  });
  if (distance >= featherCells) {
    return kMaximumMaskWeight;
  }
  return static_cast<std::uint32_t>(
      (static_cast<std::uint64_t>(distance + 1U) * kMaximumMaskWeight) /
      (static_cast<std::uint64_t>(featherCells) + 1U));
}

[[nodiscard]] std::uint32_t ellipseMaskWeight(
    std::uint16_t x,
    std::uint16_t z,
    CreativeTerrainHeightFieldBounds bounds,
    std::uint16_t featherCells) noexcept {
  const double radiusX = static_cast<double>(bounds.widthCells) * 0.5;
  const double radiusZ = static_cast<double>(bounds.depthCells) * 0.5;
  const double localX = (static_cast<double>(x) + 0.5 - radiusX) / radiusX;
  const double localZ = (static_cast<double>(z) + 0.5 - radiusZ) / radiusZ;
  const double radial = std::sqrt(localX * localX + localZ * localZ);
  if (!std::isfinite(radial) || radial >= 1.0) {
    return 0U;
  }
  if (featherCells == 0U) {
    return kMaximumMaskWeight;
  }
  const double inwardDistance =
      (1.0 - radial) * std::min(radiusX, radiusZ);
  const double normalized = std::clamp(
      inwardDistance / static_cast<double>(featherCells), 0.0, 1.0);
  return static_cast<std::uint32_t>(
      std::llround(normalized * kMaximumMaskWeight));
}

[[nodiscard]] std::uint32_t maskWeight(
    CreativeTerrainCompositionMask mask,
    std::uint16_t x,
    std::uint16_t z,
    CreativeTerrainHeightFieldBounds bounds,
    std::uint16_t featherCells) noexcept {
  switch (mask) {
    case CreativeTerrainCompositionMask::Rectangle:
      return rectangleMaskWeight(x, z, bounds, featherCells);
    case CreativeTerrainCompositionMask::Ellipse:
      return ellipseMaskWeight(x, z, bounds, featherCells);
    case CreativeTerrainCompositionMask::Count:
      break;
  }
  return 0U;
}

[[nodiscard]] std::uint16_t compositionTarget(
    CreativeTerrainCompositionMode mode,
    std::uint16_t source,
    std::uint16_t generated) noexcept {
  switch (mode) {
    case CreativeTerrainCompositionMode::Replace:
      return generated;
    case CreativeTerrainCompositionMode::Raise:
      return std::max(source, generated);
    case CreativeTerrainCompositionMode::Lower:
      return std::min(source, generated);
    case CreativeTerrainCompositionMode::Count:
      break;
  }
  return source;
}

[[nodiscard]] std::uint16_t blendHeight(std::uint16_t source,
                                        std::uint16_t target,
                                        std::uint32_t weight) noexcept {
  const std::uint64_t weighted =
      static_cast<std::uint64_t>(source) * (kMaximumMaskWeight - weight) +
      static_cast<std::uint64_t>(target) * weight +
      kMaximumMaskWeight / 2U;
  return static_cast<std::uint16_t>(weighted / kMaximumMaskWeight);
}

[[nodiscard]] std::uint64_t heightFieldHash(
    CreativeTerrainHeightFieldBounds bounds,
    const std::vector<std::uint16_t>& heights) noexcept {
  iggy3d::StableHasher hasher;
  hasher.addI64(bounds.minimum.x);
  hasher.addI64(bounds.minimum.z);
  hasher.addU64(bounds.widthCells);
  hasher.addU64(bounds.depthCells);
  for (const std::uint16_t height : heights) {
    hasher.addU64(height);
  }
  return hasher.value();
}

}  // namespace

bool isValidCreativeTerrainCompositionRecipe(
    const CreativeTerrainCompositionRecipe& recipe) noexcept {
  return recipe.version == kCreativeTerrainCompositionRecipeVersion &&
         recipe.mask < CreativeTerrainCompositionMask::Count &&
         recipe.mode < CreativeTerrainCompositionMode::Count &&
         recipe.featherCells <=
             kCreativeTerrainCompositionMaximumFeatherCells;
}

std::string_view toString(CreativeTerrainCompositionMask mask) noexcept {
  switch (mask) {
    case CreativeTerrainCompositionMask::Rectangle:
      return "Rectangle";
    case CreativeTerrainCompositionMask::Ellipse:
      return "Ellipse";
    case CreativeTerrainCompositionMask::Count:
      break;
  }
  return "Invalid";
}

std::string_view toString(CreativeTerrainCompositionMode mode) noexcept {
  switch (mode) {
    case CreativeTerrainCompositionMode::Replace:
      return "Replace";
    case CreativeTerrainCompositionMode::Raise:
      return "Raise";
    case CreativeTerrainCompositionMode::Lower:
      return "Lower";
    case CreativeTerrainCompositionMode::Count:
      break;
  }
  return "Invalid";
}

std::string_view toString(CreativeTerrainCompositionStatus status) noexcept {
  switch (status) {
    case CreativeTerrainCompositionStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainCompositionStatus::UnsupportedVersion:
      return "UnsupportedVersion";
    case CreativeTerrainCompositionStatus::InvalidMask:
      return "InvalidMask";
    case CreativeTerrainCompositionStatus::InvalidMode:
      return "InvalidMode";
    case CreativeTerrainCompositionStatus::InvalidFeather:
      return "InvalidFeather";
    case CreativeTerrainCompositionStatus::InvalidSource:
      return "InvalidSource";
    case CreativeTerrainCompositionStatus::InvalidGeneration:
      return "InvalidGeneration";
    case CreativeTerrainCompositionStatus::InvalidBounds:
      return "InvalidBounds";
    case CreativeTerrainCompositionStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainCompositionStatus::OutputRejected:
      return "OutputRejected";
    case CreativeTerrainCompositionStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

CreativeTerrainCompositionResult composeCreativeTerrainGeneration(
    const CreativeTerrainHeightField& existingAuthored,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainGenerationResult& generation,
    const CreativeTerrainCompositionRecipe& recipe) {
  CreativeTerrainCompositionResult result;
  result.recipe = recipe;
  result.receipt.requested = true;
  result.receipt.sourceColumnCount = canonicalSource.columns.size();
  result.receipt.generatedCellCount =
      generation.plan.heightField.cellCount();
  if (recipe.version != kCreativeTerrainCompositionRecipeVersion) {
    result.receipt.status =
        CreativeTerrainCompositionStatus::UnsupportedVersion;
    result.receipt.reasonCode =
        "creative_terrain_composition_version_unsupported";
    return result;
  }
  if (recipe.mask >= CreativeTerrainCompositionMask::Count) {
    result.receipt.status = CreativeTerrainCompositionStatus::InvalidMask;
    result.receipt.reasonCode = "creative_terrain_composition_mask_invalid";
    return result;
  }
  if (recipe.mode >= CreativeTerrainCompositionMode::Count) {
    result.receipt.status = CreativeTerrainCompositionStatus::InvalidMode;
    result.receipt.reasonCode = "creative_terrain_composition_mode_invalid";
    return result;
  }
  if (recipe.featherCells >
      kCreativeTerrainCompositionMaximumFeatherCells) {
    result.receipt.status = CreativeTerrainCompositionStatus::InvalidFeather;
    result.receipt.reasonCode =
        "creative_terrain_composition_feather_invalid";
    return result;
  }
  if (!existingAuthored.validateInvariants() ||
      !validCanonicalSource(canonicalSource)) {
    result.receipt.status = CreativeTerrainCompositionStatus::InvalidSource;
    result.receipt.reasonCode = "creative_terrain_composition_source_invalid";
    return result;
  }
  if (!generation.receipt.accepted ||
      generation.receipt.status != CreativeTerrainGenerationStatus::Ready ||
      !generation.plan.heightField.validateInvariants() ||
      generation.plan.heightField.cellCount() == 0U) {
    result.receipt.status =
        CreativeTerrainCompositionStatus::InvalidGeneration;
    result.receipt.reasonCode =
        "creative_terrain_composition_generation_invalid";
    return result;
  }

  CreativeTerrainHeightFieldBounds outputBounds;
  if (!unionBounds(existingAuthored,
                   generation.plan.heightField.bounds(), outputBounds,
                   result.receipt.boundsExpanded)) {
    const std::uint64_t width = outputBounds.widthCells;
    const std::uint64_t depth = outputBounds.depthCells;
    result.receipt.status =
        width * depth > kCreativeTerrainHeightFieldCellCapacity
            ? CreativeTerrainCompositionStatus::CapacityExceeded
            : CreativeTerrainCompositionStatus::InvalidBounds;
    result.receipt.reasonCode =
        result.receipt.status ==
                CreativeTerrainCompositionStatus::CapacityExceeded
            ? "creative_terrain_composition_capacity_exceeded"
            : "creative_terrain_composition_bounds_invalid";
    return result;
  }

  const std::uint64_t outputCellCount =
      static_cast<std::uint64_t>(outputBounds.widthCells) *
      outputBounds.depthCells;
  if (outputCellCount > kCreativeTerrainHeightFieldCellCapacity) {
    result.receipt.status = CreativeTerrainCompositionStatus::CapacityExceeded;
    result.receipt.reasonCode =
        "creative_terrain_composition_capacity_exceeded";
    return result;
  }

  const CreativeTerrainHeightField& generated =
      generation.plan.heightField;
  const CreativeTerrainHeightFieldBounds generatedBounds = generated.bounds();
  const CreativeTerrainHeightFieldBounds existingBounds =
      existingAuthored.bounds();
  std::vector<std::uint16_t> heights;
  heights.reserve(static_cast<std::size_t>(outputCellCount));
  std::size_t sourceIndex = 0U;
  for (std::uint16_t outputZ = 0U; outputZ < outputBounds.depthCells;
       ++outputZ) {
    for (std::uint16_t outputX = 0U; outputX < outputBounds.widthCells;
         ++outputX) {
      const CreativeTerrainCoord2 coord{
          outputBounds.minimum.x + static_cast<std::int32_t>(outputX),
          outputBounds.minimum.z + static_cast<std::int32_t>(outputZ),
      };
      while (sourceIndex < canonicalSource.columns.size() &&
             coordLess(canonicalSource.columns[sourceIndex].coord, coord)) {
        ++sourceIndex;
      }
      const std::uint16_t sourceHeight =
          sourceIndex < canonicalSource.columns.size() &&
                  canonicalSource.columns[sourceIndex].coord == coord
              ? canonicalSource.columns[sourceIndex].heightCells
              : kCreativeTerrainEmptyHeightCells;
      std::uint16_t outputHeight = sourceHeight;
      if (insideBounds(coord, generatedBounds)) {
        const std::uint16_t generatedX = static_cast<std::uint16_t>(
            static_cast<std::int64_t>(coord.x) - generatedBounds.minimum.x);
        const std::uint16_t generatedZ = static_cast<std::uint16_t>(
            static_cast<std::int64_t>(coord.z) - generatedBounds.minimum.z);
        const std::size_t generatedIndex =
            static_cast<std::size_t>(generatedZ) * generatedBounds.widthCells +
            generatedX;
        const std::uint32_t weight =
            maskWeight(recipe.mask, generatedX, generatedZ, generatedBounds,
                       recipe.featherCells);
        if (weight > 0U) {
          ++result.receipt.maskedCellCount;
          if (weight < kMaximumMaskWeight) {
            ++result.receipt.featheredCellCount;
          }
          const std::uint16_t target = compositionTarget(
              recipe.mode, sourceHeight, generated.heights()[generatedIndex]);
          outputHeight = blendHeight(sourceHeight, target, weight);
        }
      }
      if (outputHeight == sourceHeight) {
        ++result.receipt.preservedCellCount;
      } else {
        ++result.receipt.modifiedCellCount;
      }
      if (!insideBounds(coord, existingBounds) && sourceHeight != 0U) {
        ++result.receipt.materializedSourceCellCount;
      }
      heights.push_back(outputHeight);
    }
  }

  const CreativeTerrainHeightFieldReplaceReceipt replacement =
      result.heightField.replace(outputBounds, heights);
  if (!replacement.accepted) {
    result.receipt.status = CreativeTerrainCompositionStatus::OutputRejected;
    result.receipt.reasonCode =
        "creative_terrain_composition_output_rejected";
    return result;
  }
  result.receipt.accepted = true;
  result.receipt.status = CreativeTerrainCompositionStatus::Ready;
  result.receipt.outputCellCount = outputCellCount;
  result.receipt.heightHash = heightFieldHash(outputBounds, heights);
  result.receipt.reasonCode = "creative_terrain_composition_ready";
  return result;
}

}  // namespace iggy3d::creative
