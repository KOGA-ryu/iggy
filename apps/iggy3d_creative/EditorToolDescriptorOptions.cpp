#include "EditorToolDescriptor.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

bool creativeEditorToolOptionVisible(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept {
  const cr::CreativeTerrainRegionMode operation =
      topography.region.recipe.mode;
  if (operation >= cr::CreativeTerrainRegionMode::Count) {
    return false;
  }
  const bool noise =
      operation == cr::CreativeTerrainRegionMode::Noise;
  switch (option.binding) {
    case CreativeEditorToolOptionBinding::TerrainOperation:
    case CreativeEditorToolOptionBinding::TerrainMask:
    case CreativeEditorToolOptionBinding::TerrainFeather:
      return true;
    case CreativeEditorToolOptionBinding::TerrainAmount:
      return cr::creativeTerrainRegionModeUsesAmount(operation);
    case CreativeEditorToolOptionBinding::TerrainTargetHeight:
      return cr::creativeTerrainRegionModeUsesTargetHeight(operation);
    case CreativeEditorToolOptionBinding::TerrainNoiseRelief:
    case CreativeEditorToolOptionBinding::TerrainNoiseScale:
    case CreativeEditorToolOptionBinding::TerrainSeed:
      return noise;
    case CreativeEditorToolOptionBinding::Count:
      break;
  }
  return false;
}

std::string_view creativeEditorToolOptionFullLabel(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept {
  if (!option.label.empty() ||
      option.binding != CreativeEditorToolOptionBinding::TerrainTargetHeight) {
    return option.label;
  }
  switch (topography.region.recipe.mode) {
    case cr::CreativeTerrainRegionMode::Flatten:
      return "Height";
    case cr::CreativeTerrainRegionMode::Noise:
      return "Base height";
    case cr::CreativeTerrainRegionMode::Raise:
    case cr::CreativeTerrainRegionMode::Lower:
    case cr::CreativeTerrainRegionMode::Smooth:
    case cr::CreativeTerrainRegionMode::Erase:
    case cr::CreativeTerrainRegionMode::Count:
      break;
  }
  return "";
}

std::string_view creativeEditorToolOptionCompactLabel(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept {
  if (!option.compactLabel.empty() ||
      option.binding != CreativeEditorToolOptionBinding::TerrainTargetHeight) {
    return option.compactLabel;
  }
  switch (topography.region.recipe.mode) {
    case cr::CreativeTerrainRegionMode::Flatten:
      return "Height";
    case cr::CreativeTerrainRegionMode::Noise:
      return "Base";
    case cr::CreativeTerrainRegionMode::Raise:
    case cr::CreativeTerrainRegionMode::Lower:
    case cr::CreativeTerrainRegionMode::Smooth:
    case cr::CreativeTerrainRegionMode::Erase:
    case cr::CreativeTerrainRegionMode::Count:
      break;
  }
  return "";
}

double creativeEditorToolOptionScalarValue(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept {
  const CreativeEditorWorldLayoutTerrainRegionState& region = topography.region;
  const cr::CreativeTerrainRegionRecipe& recipe = region.recipe;
  switch (option.binding) {
    case CreativeEditorToolOptionBinding::TerrainAmount:
      return static_cast<double>(recipe.amountCells);
    case CreativeEditorToolOptionBinding::TerrainTargetHeight:
      return static_cast<double>(recipe.targetHeightCells);
    case CreativeEditorToolOptionBinding::TerrainNoiseRelief:
      return static_cast<double>(recipe.noiseReliefCells);
    case CreativeEditorToolOptionBinding::TerrainNoiseScale:
      return recipe.noiseScaleCells;
    case CreativeEditorToolOptionBinding::TerrainFeather:
      return static_cast<double>(recipe.featherCells);
    case CreativeEditorToolOptionBinding::TerrainOperation:
    case CreativeEditorToolOptionBinding::TerrainMask:
    case CreativeEditorToolOptionBinding::TerrainSeed:
    case CreativeEditorToolOptionBinding::Count:
      break;
  }
  return 0.0;
}

bool setCreativeEditorToolOptionScalarValue(
    const CreativeEditorToolOptionSpec& option,
    CreativeEditorWorldLayoutTopographyState& topography,
    double value) noexcept {
  CreativeEditorWorldLayoutTerrainRegionState& region = topography.region;
  cr::CreativeTerrainRegionRecipe& recipe = region.recipe;
  const double clamped = std::clamp(value, option.minimum, option.maximum);
  switch (option.binding) {
    case CreativeEditorToolOptionBinding::TerrainAmount: {
      const auto cells = static_cast<std::uint16_t>(std::llround(clamped));
      const bool changed = recipe.amountCells != cells;
      recipe.amountCells = cells;
      return changed;
    }
    case CreativeEditorToolOptionBinding::TerrainTargetHeight: {
      const auto cells = static_cast<std::uint16_t>(std::llround(clamped));
      const bool changed = recipe.targetHeightCells != cells;
      recipe.targetHeightCells = cells;
      return changed;
    }
    case CreativeEditorToolOptionBinding::TerrainNoiseRelief: {
      const auto cells = static_cast<std::uint16_t>(std::llround(clamped));
      const bool changed = recipe.noiseReliefCells != cells;
      recipe.noiseReliefCells = cells;
      return changed;
    }
    case CreativeEditorToolOptionBinding::TerrainNoiseScale: {
      const bool changed = recipe.noiseScaleCells != clamped;
      recipe.noiseScaleCells = clamped;
      return changed;
    }
    case CreativeEditorToolOptionBinding::TerrainFeather: {
      const auto cells = static_cast<std::uint16_t>(std::llround(clamped));
      const bool changed = recipe.featherCells != cells;
      recipe.featherCells = cells;
      return changed;
    }
    case CreativeEditorToolOptionBinding::TerrainOperation:
    case CreativeEditorToolOptionBinding::TerrainMask:
    case CreativeEditorToolOptionBinding::TerrainSeed:
    case CreativeEditorToolOptionBinding::Count:
      break;
  }
  return false;
}

std::uint8_t creativeEditorToolOptionChoiceValue(
    const CreativeEditorToolOptionSpec& option,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept {
  switch (option.binding) {
    case CreativeEditorToolOptionBinding::TerrainOperation:
      return static_cast<std::uint8_t>(topography.region.recipe.mode);
    case CreativeEditorToolOptionBinding::TerrainMask:
      return static_cast<std::uint8_t>(topography.region.recipe.mask);
    case CreativeEditorToolOptionBinding::TerrainAmount:
    case CreativeEditorToolOptionBinding::TerrainTargetHeight:
    case CreativeEditorToolOptionBinding::TerrainNoiseRelief:
    case CreativeEditorToolOptionBinding::TerrainNoiseScale:
    case CreativeEditorToolOptionBinding::TerrainSeed:
    case CreativeEditorToolOptionBinding::TerrainFeather:
    case CreativeEditorToolOptionBinding::Count:
      break;
  }
  return 0U;
}

bool setCreativeEditorToolOptionChoiceValue(
    const CreativeEditorToolOptionSpec& option,
    CreativeEditorWorldLayoutTopographyState& topography,
    std::uint8_t value) noexcept {
  switch (option.binding) {
    case CreativeEditorToolOptionBinding::TerrainOperation: {
      if (value >=
          static_cast<std::uint8_t>(cr::CreativeTerrainRegionMode::Count)) {
        return false;
      }
      const auto operation = static_cast<cr::CreativeTerrainRegionMode>(value);
      const bool changed = topography.region.recipe.mode != operation;
      topography.region.recipe.mode = operation;
      return changed;
    }
    case CreativeEditorToolOptionBinding::TerrainMask: {
      if (value >= static_cast<std::uint8_t>(
                       cr::CreativeTerrainCompositionMask::Count)) {
        return false;
      }
      const auto mask = static_cast<cr::CreativeTerrainCompositionMask>(value);
      const bool changed = topography.region.recipe.mask != mask;
      topography.region.recipe.mask = mask;
      return changed;
    }
    case CreativeEditorToolOptionBinding::TerrainAmount:
    case CreativeEditorToolOptionBinding::TerrainTargetHeight:
    case CreativeEditorToolOptionBinding::TerrainNoiseRelief:
    case CreativeEditorToolOptionBinding::TerrainNoiseScale:
    case CreativeEditorToolOptionBinding::TerrainSeed:
    case CreativeEditorToolOptionBinding::TerrainFeather:
    case CreativeEditorToolOptionBinding::Count:
      break;
  }
  return false;
}

void advanceCreativeEditorToolOptionSeed(
    CreativeEditorWorldLayoutTopographyState& topography) noexcept {
  CreativeEditorWorldLayoutTerrainRegionState& region = topography.region;
  region.recipe.seed =
      region.recipe.seed == std::numeric_limits<std::uint64_t>::max()
          ? 0U
          : region.recipe.seed + 1U;
}

bool creativeEditorToolActionEnabled(
    const CreativeEditorToolActionSpec& action,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept {
  const CreativeEditorWorldLayoutTerrainRegionState& region = topography.region;
  switch (action.rule) {
    case CreativeEditorToolActionRule::IdleValidRegion:
      return region.regionValid && !region.selecting;
    case CreativeEditorToolActionRule::OwnedPreview:
      return region.ownsPreview;
    case CreativeEditorToolActionRule::RegionActivity:
      return region.selecting || region.regionValid || region.ownsPreview;
    case CreativeEditorToolActionRule::Count:
      break;
  }
  return false;
}

}  // namespace iggy3d_creative_app
