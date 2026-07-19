#pragma once

#include "app/iggy3d/creative/document/TerrainHeightField.hpp"
#include "app/iggy3d/creative/recipes/TerrainGeneration.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

inline constexpr std::uint32_t kCreativeTerrainCompositionRecipeVersion = 1U;
inline constexpr std::uint16_t kCreativeTerrainCompositionMaximumFeatherCells =
    4096U;

enum class CreativeTerrainCompositionMask : std::uint8_t {
  Rectangle,
  Ellipse,
  Count,
};

enum class CreativeTerrainCompositionMode : std::uint8_t {
  Replace,
  Raise,
  Lower,
  Count,
};

struct CreativeTerrainCompositionRecipe {
  std::uint32_t version = kCreativeTerrainCompositionRecipeVersion;
  CreativeTerrainCompositionMask mask =
      CreativeTerrainCompositionMask::Rectangle;
  CreativeTerrainCompositionMode mode =
      CreativeTerrainCompositionMode::Replace;
  std::uint16_t featherCells = 4U;
};

enum class CreativeTerrainCompositionStatus : std::uint8_t {
  NotRequested,
  UnsupportedVersion,
  InvalidMask,
  InvalidMode,
  InvalidFeather,
  InvalidSource,
  InvalidGeneration,
  InvalidBounds,
  CapacityExceeded,
  OutputRejected,
  Ready,
};

struct CreativeTerrainCompositionReceipt {
  bool requested = false;
  bool accepted = false;
  bool boundsExpanded = false;
  CreativeTerrainCompositionStatus status =
      CreativeTerrainCompositionStatus::NotRequested;
  std::uint64_t sourceColumnCount = 0U;
  std::uint64_t generatedCellCount = 0U;
  std::uint64_t outputCellCount = 0U;
  std::uint64_t maskedCellCount = 0U;
  std::uint64_t featheredCellCount = 0U;
  std::uint64_t modifiedCellCount = 0U;
  std::uint64_t preservedCellCount = 0U;
  std::uint64_t materializedSourceCellCount = 0U;
  std::uint64_t heightHash = 0U;
  std::string_view reasonCode =
      "creative_terrain_composition_not_requested";
};

struct CreativeTerrainCompositionResult {
  CreativeTerrainCompositionRecipe recipe{};
  CreativeTerrainHeightField heightField;
  CreativeTerrainCompositionReceipt receipt;
};

[[nodiscard]] bool isValidCreativeTerrainCompositionRecipe(
    const CreativeTerrainCompositionRecipe& recipe) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainCompositionMask mask) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainCompositionMode mode) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainCompositionStatus status) noexcept;

// Materializes the union of the existing authored bounds and generated bounds,
// sampling canonical source terrain for every cell before applying the mask.
// The returned field is complete document-candidate truth: preview and Apply
// must consume this exact field rather than recomposing independently.
// Traversal is deterministic row-major O(output cells + source columns), and
// capacity is rejected before output allocation so failure leaves no partial
// candidate.
[[nodiscard]] CreativeTerrainCompositionResult composeCreativeTerrainGeneration(
    const CreativeTerrainHeightField& existingAuthored,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainGenerationResult& generation,
    const CreativeTerrainCompositionRecipe& recipe);

}  // namespace iggy3d::creative
