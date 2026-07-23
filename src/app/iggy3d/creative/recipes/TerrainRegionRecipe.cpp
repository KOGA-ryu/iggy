#include "app/iggy3d/creative/recipes/TerrainRegionRecipe.hpp"

#include "core/hash/StableHash.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace iggy3d::creative {
namespace {

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

[[nodiscard]] std::uint16_t sourceHeightAt(
    const CreativeTerrainHeightField& existing,
    const CreativeTerrainSurfacePlan& canonical,
    CreativeTerrainCoord2 coord) noexcept {
  if (existing.contains(coord)) {
    return existing.heightAt(coord).value_or(kCreativeTerrainEmptyHeightCells);
  }
  const auto found = std::lower_bound(
      canonical.columns.begin(), canonical.columns.end(), coord,
      [](const CreativeTerrainColumn& column, CreativeTerrainCoord2 candidate) {
        return coordLess(column.coord, candidate);
      });
  return found != canonical.columns.end() && found->coord == coord
             ? found->heightCells
             : kCreativeTerrainEmptyHeightCells;
}

[[nodiscard]] bool unionBounds(
    const CreativeTerrainHeightField& existing,
    CreativeTerrainHeightFieldBounds region,
    CreativeTerrainHeightFieldBounds& output,
    bool& expanded) noexcept {
  if (existing.cellCount() == 0U) {
    output = region;
    expanded = false;
    return isValidCreativeTerrainHeightFieldBounds(output);
  }
  const CreativeTerrainHeightFieldBounds current = existing.bounds();
  const std::int64_t minimumX =
      std::min<std::int64_t>(current.minimum.x, region.minimum.x);
  const std::int64_t minimumZ =
      std::min<std::int64_t>(current.minimum.z, region.minimum.z);
  const std::int64_t maximumXExclusive = std::max(
      static_cast<std::int64_t>(current.minimum.x) + current.widthCells,
      static_cast<std::int64_t>(region.minimum.x) + region.widthCells);
  const std::int64_t maximumZExclusive = std::max(
      static_cast<std::int64_t>(current.minimum.z) + current.depthCells,
      static_cast<std::int64_t>(region.minimum.z) + region.depthCells);
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

[[nodiscard]] bool insideBounds(
    CreativeTerrainCoord2 coord,
    CreativeTerrainHeightFieldBounds bounds,
    std::uint16_t& localX,
    std::uint16_t& localZ) noexcept {
  const std::int64_t offsetX =
      static_cast<std::int64_t>(coord.x) - bounds.minimum.x;
  const std::int64_t offsetZ =
      static_cast<std::int64_t>(coord.z) - bounds.minimum.z;
  if (offsetX < 0 || offsetZ < 0 || offsetX >= bounds.widthCells ||
      offsetZ >= bounds.depthCells) {
    return false;
  }
  localX = static_cast<std::uint16_t>(offsetX);
  localZ = static_cast<std::uint16_t>(offsetZ);
  return true;
}

[[nodiscard]] std::uint16_t moveToward(std::uint16_t current,
                                       std::uint16_t target,
                                       std::uint16_t amount) noexcept {
  if (current == target) {
    return current;
  }
  if (current < target) {
    return static_cast<std::uint16_t>(
        current + std::min<std::uint16_t>(amount, target - current));
  }
  return static_cast<std::uint16_t>(
      current - std::min<std::uint16_t>(amount, current - target));
}

[[nodiscard]] std::uint16_t smoothTarget(
    const CreativeTerrainHeightField& existing,
    const CreativeTerrainSurfacePlan& canonical,
    CreativeTerrainCoord2 center) noexcept {
  std::uint32_t sum = 0U;
  std::uint32_t count = 0U;
  for (std::int32_t z = -1; z <= 1; ++z) {
    for (std::int32_t x = -1; x <= 1; ++x) {
      const std::int64_t sampleX = static_cast<std::int64_t>(center.x) + x;
      const std::int64_t sampleZ = static_cast<std::int64_t>(center.z) + z;
      if (sampleX < std::numeric_limits<std::int32_t>::min() ||
          sampleX > std::numeric_limits<std::int32_t>::max() ||
          sampleZ < std::numeric_limits<std::int32_t>::min() ||
          sampleZ > std::numeric_limits<std::int32_t>::max()) {
        continue;
      }
      const std::uint16_t height = sourceHeightAt(
          existing, canonical,
          {static_cast<std::int32_t>(sampleX),
           static_cast<std::int32_t>(sampleZ)});
      if (height != kCreativeTerrainEmptyHeightCells) {
        sum += height;
        ++count;
      }
    }
  }
  return count == 0U
             ? kCreativeTerrainEmptyHeightCells
             : static_cast<std::uint16_t>((sum + count / 2U) / count);
}

[[nodiscard]] std::uint16_t blendHeight(std::uint16_t source,
                                        std::uint16_t target,
                                        std::uint32_t weight) noexcept {
  const std::uint64_t weighted =
      static_cast<std::uint64_t>(source) *
          (kCreativeTerrainCompositionMaximumMaskWeight - weight) +
      static_cast<std::uint64_t>(target) * weight +
      kCreativeTerrainCompositionMaximumMaskWeight / 2U;
  return static_cast<std::uint16_t>(std::min<std::uint64_t>(
      weighted / kCreativeTerrainCompositionMaximumMaskWeight,
      kCreativeTerrainMaximumHeightCells));
}

[[nodiscard]] std::uint64_t heightHash(
    CreativeTerrainHeightFieldBounds bounds,
    const std::vector<std::uint16_t>& heights) noexcept {
  StableHasher hasher;
  hasher.addI64(bounds.minimum.x);
  hasher.addI64(bounds.minimum.z);
  hasher.addU64(bounds.widthCells);
  hasher.addU64(bounds.depthCells);
  for (const std::uint16_t height : heights) {
    hasher.addU64(height);
  }
  return hasher.value();
}

[[nodiscard]] CreativeTerrainGeneratorRecipe noiseRecipe(
    const CreativeTerrainRegionRecipe& recipe) noexcept {
  CreativeTerrainGeneratorRecipe generation;
  generation.seed = recipe.seed;
  generation.bounds = recipe.bounds;
  generation.baseHeightCells = recipe.targetHeightCells;
  generation.reliefCells = recipe.noiseReliefCells;
  generation.horizontalScaleCells = recipe.noiseScaleCells;
  return generation;
}

}  // namespace

bool isValidCreativeTerrainRegionRecipe(
    const CreativeTerrainRegionRecipe& recipe) noexcept {
  return recipe.version == kCreativeTerrainRegionRecipeVersion &&
         isValidCreativeTerrainHeightFieldBounds(recipe.bounds) &&
         recipe.mask < CreativeTerrainCompositionMask::Count &&
         recipe.mode < CreativeTerrainRegionMode::Count &&
         recipe.amountCells >= 1U &&
         recipe.amountCells <= kCreativeTerrainRegionMaximumAmountCells &&
         recipe.targetHeightCells >= kCreativeTerrainMinimumHeightCells &&
         recipe.targetHeightCells <= kCreativeTerrainMaximumHeightCells &&
         recipe.noiseReliefCells <= kCreativeTerrainMaximumHeightCells &&
         std::isfinite(recipe.noiseScaleCells) &&
         recipe.noiseScaleCells >=
             kCreativeTerrainGeneratorMinimumHorizontalScaleCells &&
         recipe.noiseScaleCells <=
             kCreativeTerrainGeneratorMaximumHorizontalScaleCells &&
         recipe.featherCells <=
             kCreativeTerrainCompositionMaximumFeatherCells;
}

bool creativeTerrainRegionModeUsesAmount(
    CreativeTerrainRegionMode mode) noexcept {
  return mode == CreativeTerrainRegionMode::Raise ||
         mode == CreativeTerrainRegionMode::Lower ||
         mode == CreativeTerrainRegionMode::Smooth;
}

bool creativeTerrainRegionModeUsesTargetHeight(
    CreativeTerrainRegionMode mode) noexcept {
  return mode == CreativeTerrainRegionMode::Flatten ||
         mode == CreativeTerrainRegionMode::Noise;
}

bool creativeTerrainRegionModeUsesNoise(
    CreativeTerrainRegionMode mode) noexcept {
  return mode == CreativeTerrainRegionMode::Noise;
}

std::string_view toString(CreativeTerrainRegionMode mode) noexcept {
  switch (mode) {
    case CreativeTerrainRegionMode::Raise:
      return "Raise";
    case CreativeTerrainRegionMode::Lower:
      return "Lower";
    case CreativeTerrainRegionMode::Flatten:
      return "Flatten";
    case CreativeTerrainRegionMode::Smooth:
      return "Smooth";
    case CreativeTerrainRegionMode::Noise:
      return "Noise";
    case CreativeTerrainRegionMode::Erase:
      return "Erase";
    case CreativeTerrainRegionMode::Count:
      break;
  }
  return "Invalid";
}

bool parseCreativeTerrainRegionMode(
    std::string_view text,
    CreativeTerrainRegionMode& output) noexcept {
  constexpr CreativeTerrainRegionMode modes[]{
      CreativeTerrainRegionMode::Raise,   CreativeTerrainRegionMode::Lower,
      CreativeTerrainRegionMode::Flatten, CreativeTerrainRegionMode::Smooth,
      CreativeTerrainRegionMode::Noise,   CreativeTerrainRegionMode::Erase,
  };
  for (const CreativeTerrainRegionMode mode : modes) {
    if (text == toString(mode)) {
      output = mode;
      return true;
    }
  }
  return false;
}

std::string_view toString(CreativeTerrainRegionRecipeStatus status) noexcept {
  switch (status) {
    case CreativeTerrainRegionRecipeStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainRegionRecipeStatus::UnsupportedVersion:
      return "UnsupportedVersion";
    case CreativeTerrainRegionRecipeStatus::InvalidRecipe:
      return "InvalidRecipe";
    case CreativeTerrainRegionRecipeStatus::InvalidSource:
      return "InvalidSource";
    case CreativeTerrainRegionRecipeStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainRegionRecipeStatus::NoiseRejected:
      return "NoiseRejected";
    case CreativeTerrainRegionRecipeStatus::OutputRejected:
      return "OutputRejected";
    case CreativeTerrainRegionRecipeStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

CreativeTerrainRegionRecipeResult buildCreativeTerrainRegionRecipe(
    const CreativeTerrainHeightField& existingAuthored,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainRegionRecipe& recipe) {
  CreativeTerrainRegionRecipeResult result;
  result.recipe = recipe;
  CreativeTerrainRegionRecipeReceipt& receipt = result.receipt;
  receipt.requested = true;
  receipt.sourceColumnCount = canonicalSource.columns.size();
  if (recipe.version != kCreativeTerrainRegionRecipeVersion) {
    receipt.status = CreativeTerrainRegionRecipeStatus::UnsupportedVersion;
    receipt.reasonCode = "creative_terrain_region_version_unsupported";
    return result;
  }
  if (!isValidCreativeTerrainRegionRecipe(recipe)) {
    receipt.status = CreativeTerrainRegionRecipeStatus::InvalidRecipe;
    receipt.reasonCode = "creative_terrain_region_recipe_invalid";
    return result;
  }
  if (!existingAuthored.validateInvariants() ||
      !validCanonicalSource(canonicalSource)) {
    receipt.status = CreativeTerrainRegionRecipeStatus::InvalidSource;
    receipt.reasonCode = "creative_terrain_region_source_invalid";
    return result;
  }

  CreativeTerrainHeightFieldBounds outputBounds;
  if (!unionBounds(existingAuthored, recipe.bounds, outputBounds,
                   receipt.boundsExpanded)) {
    receipt.status = CreativeTerrainRegionRecipeStatus::CapacityExceeded;
    receipt.reasonCode = "creative_terrain_region_capacity_exceeded";
    return result;
  }

  std::optional<CreativeTerrainGenerationResult> noise;
  if (recipe.mode == CreativeTerrainRegionMode::Noise) {
    noise = buildCreativeTerrainGenerationPlan(noiseRecipe(recipe));
    if (!noise->receipt.accepted) {
      receipt.status = CreativeTerrainRegionRecipeStatus::NoiseRejected;
      receipt.reasonCode = "creative_terrain_region_noise_rejected";
      return result;
    }
  }

  const std::uint64_t cellCount =
      static_cast<std::uint64_t>(outputBounds.widthCells) *
      outputBounds.depthCells;
  std::vector<std::uint16_t> heights;
  heights.reserve(static_cast<std::size_t>(cellCount));
  for (std::uint16_t z = 0U; z < outputBounds.depthCells; ++z) {
    for (std::uint16_t x = 0U; x < outputBounds.widthCells; ++x) {
      const CreativeTerrainCoord2 coord{
          outputBounds.minimum.x + static_cast<std::int32_t>(x),
          outputBounds.minimum.z + static_cast<std::int32_t>(z),
      };
      const std::uint16_t source =
          sourceHeightAt(existingAuthored, canonicalSource, coord);
      std::uint16_t output = source;
      std::uint16_t localX = 0U;
      std::uint16_t localZ = 0U;
      if (insideBounds(coord, recipe.bounds, localX, localZ)) {
        ++receipt.evaluatedCellCount;
        const std::uint32_t weight = creativeTerrainCompositionMaskWeight(
            recipe.mask, localX, localZ, recipe.bounds, recipe.featherCells);
        if (weight > 0U) {
          ++receipt.maskedCellCount;
          if (weight < kCreativeTerrainCompositionMaximumMaskWeight) {
            ++receipt.featheredCellCount;
          }
          std::uint16_t target = source;
          switch (recipe.mode) {
            case CreativeTerrainRegionMode::Raise:
              if (source != kCreativeTerrainEmptyHeightCells) {
                target = static_cast<std::uint16_t>(std::min<std::uint32_t>(
                    kCreativeTerrainMaximumHeightCells,
                    static_cast<std::uint32_t>(source) + recipe.amountCells));
              }
              break;
            case CreativeTerrainRegionMode::Lower:
              if (source != kCreativeTerrainEmptyHeightCells) {
                target = static_cast<std::uint16_t>(std::max<std::int32_t>(
                    kCreativeTerrainMinimumHeightCells,
                    static_cast<std::int32_t>(source) - recipe.amountCells));
              }
              break;
            case CreativeTerrainRegionMode::Flatten:
              target = recipe.targetHeightCells;
              break;
            case CreativeTerrainRegionMode::Smooth:
              if (source != kCreativeTerrainEmptyHeightCells) {
                target = moveToward(
                    source,
                    smoothTarget(existingAuthored, canonicalSource, coord),
                    recipe.amountCells);
              }
              break;
            case CreativeTerrainRegionMode::Noise:
              target = noise->plan.heightField.heights()[
                  static_cast<std::size_t>(localZ) * recipe.bounds.widthCells +
                  localX];
              break;
            case CreativeTerrainRegionMode::Erase:
              target = kCreativeTerrainEmptyHeightCells;
              break;
            case CreativeTerrainRegionMode::Count:
              break;
          }
          output = blendHeight(source, target, weight);
        }
      }
      receipt.modifiedCellCount += output != source ? 1U : 0U;
      receipt.materializedCellCount +=
          source == kCreativeTerrainEmptyHeightCells &&
                  output != kCreativeTerrainEmptyHeightCells
              ? 1U
              : 0U;
      heights.push_back(output);
    }
  }

  const CreativeTerrainHeightFieldReplaceReceipt replaced =
      result.heightField.replace(outputBounds, heights);
  if (!replaced.accepted) {
    receipt.status = CreativeTerrainRegionRecipeStatus::OutputRejected;
    receipt.reasonCode = "creative_terrain_region_output_rejected";
    return result;
  }
  receipt.accepted = true;
  receipt.status = CreativeTerrainRegionRecipeStatus::Ready;
  receipt.outputCellCount = cellCount;
  receipt.heightHash = heightHash(outputBounds, heights);
  receipt.reasonCode = "creative_terrain_region_ready";
  return result;
}

}  // namespace iggy3d::creative
