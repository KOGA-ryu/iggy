#pragma once

#include "app/iggy3d/creative/recipes/TerrainComposition.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

inline constexpr std::uint32_t kCreativeTerrainRegionRecipeVersion = 1U;
inline constexpr std::uint16_t kCreativeTerrainRegionMaximumAmountCells =
    kCreativeTerrainMaximumHeightCells;

enum class CreativeTerrainRegionMode : std::uint8_t {
  Raise,
  Lower,
  Flatten,
  Smooth,
  Noise,
  Erase,
  Count,
};

// One durable local terrain edit. Bounds are dense tile coordinates rather
// than legacy rod coordinates; all quantities are quantized terrain cells.
struct CreativeTerrainRegionRecipe {
  std::uint32_t version = kCreativeTerrainRegionRecipeVersion;
  CreativeTerrainHeightFieldBounds bounds{{0, 0}, 1U, 1U};
  CreativeTerrainCompositionMask mask =
      CreativeTerrainCompositionMask::Rectangle;
  CreativeTerrainRegionMode mode = CreativeTerrainRegionMode::Raise;
  std::uint16_t amountCells = 1U;
  std::uint16_t targetHeightCells = 4U;
  std::uint16_t noiseReliefCells = 4U;
  double noiseScaleCells = 12.0;
  std::uint16_t featherCells = 0U;
  std::uint64_t seed = 1U;

  [[nodiscard]] friend bool operator==(
      const CreativeTerrainRegionRecipe&,
      const CreativeTerrainRegionRecipe&) noexcept = default;
};

enum class CreativeTerrainRegionRecipeStatus : std::uint8_t {
  NotRequested,
  UnsupportedVersion,
  InvalidRecipe,
  InvalidSource,
  CapacityExceeded,
  NoiseRejected,
  OutputRejected,
  Ready,
};

struct CreativeTerrainRegionRecipeReceipt {
  bool requested = false;
  bool accepted = false;
  bool boundsExpanded = false;
  CreativeTerrainRegionRecipeStatus status =
      CreativeTerrainRegionRecipeStatus::NotRequested;
  std::uint64_t sourceColumnCount = 0U;
  std::uint64_t evaluatedCellCount = 0U;
  std::uint64_t maskedCellCount = 0U;
  std::uint64_t featheredCellCount = 0U;
  std::uint64_t modifiedCellCount = 0U;
  std::uint64_t materializedCellCount = 0U;
  std::uint64_t outputCellCount = 0U;
  std::uint64_t heightHash = 0U;
  std::string_view reasonCode = "creative_terrain_region_not_requested";
};

struct CreativeTerrainRegionRecipeResult {
  CreativeTerrainRegionRecipe recipe{};
  CreativeTerrainHeightField heightField;
  CreativeTerrainRegionRecipeReceipt receipt{};
};

[[nodiscard]] bool isValidCreativeTerrainRegionRecipe(
    const CreativeTerrainRegionRecipe& recipe) noexcept;
[[nodiscard]] bool creativeTerrainRegionModeUsesAmount(
    CreativeTerrainRegionMode mode) noexcept;
[[nodiscard]] bool creativeTerrainRegionModeUsesTargetHeight(
    CreativeTerrainRegionMode mode) noexcept;
[[nodiscard]] bool creativeTerrainRegionModeUsesNoise(
    CreativeTerrainRegionMode mode) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainRegionMode mode) noexcept;
[[nodiscard]] bool parseCreativeTerrainRegionMode(
    std::string_view text,
    CreativeTerrainRegionMode& output) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainRegionRecipeStatus status) noexcept;

// Applies one bounded local edit to composed terrain truth. Smooth reads an
// immutable 3x3 source snapshot; raise/lower do not create terrain in holes;
// flatten/noise may materialize it. Output is the exact dense candidate shared
// by preview, operation replay, collision, and commit.
[[nodiscard]] CreativeTerrainRegionRecipeResult
buildCreativeTerrainRegionRecipe(
    const CreativeTerrainHeightField& existingAuthored,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainRegionRecipe& recipe);

}  // namespace iggy3d::creative
