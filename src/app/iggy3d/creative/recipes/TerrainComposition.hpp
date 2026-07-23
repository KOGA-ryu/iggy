#pragma once

#include "app/iggy3d/creative/document/TerrainHeightField.hpp"
#include "app/iggy3d/creative/document/TerrainMaterialField.hpp"
#include "app/iggy3d/creative/recipes/TerrainGeneration.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

inline constexpr std::uint32_t kCreativeTerrainCompositionRecipeVersion = 2U;
inline constexpr std::uint16_t kCreativeTerrainCompositionMaximumFeatherCells =
    4096U;
inline constexpr std::uint32_t kCreativeTerrainCompositionMaximumMaskWeight =
    65535U;
inline constexpr std::size_t
    kCreativeTerrainCompositionProtectedRegionCapacity = 16U;

enum class CreativeTerrainCompositionMask : std::uint8_t {
  Rectangle,
  Ellipse,
  Count,
};

enum class CreativeTerrainCompositionMode : std::uint8_t {
  Replace,
  Raise,
  Lower,
  Smooth,
  Count,
};

struct CreativeTerrainCompositionProtectedRegion {
  CreativeTerrainHeightFieldBounds bounds{};
  CreativeTerrainCompositionMask mask =
      CreativeTerrainCompositionMask::Rectangle;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainCompositionProtectedRegion,
      CreativeTerrainCompositionProtectedRegion) noexcept = default;
};

struct CreativeTerrainCompositionRecipe {
  std::uint32_t version = kCreativeTerrainCompositionRecipeVersion;
  CreativeTerrainCompositionMask mask =
      CreativeTerrainCompositionMask::Rectangle;
  CreativeTerrainCompositionMode mode =
      CreativeTerrainCompositionMode::Replace;
  std::uint16_t featherCells = 4U;
  std::array<CreativeTerrainCompositionProtectedRegion,
             kCreativeTerrainCompositionProtectedRegionCapacity>
      protectedRegions{};
  std::uint8_t protectedRegionCount = 0U;

  [[nodiscard]] friend bool operator==(
      CreativeTerrainCompositionRecipe,
      CreativeTerrainCompositionRecipe) noexcept = default;
};

enum class CreativeTerrainProtectedRegionMutationStatus : std::uint8_t {
  NotRequested,
  InvalidRecipe,
  InvalidRegion,
  Duplicate,
  CapacityExceeded,
  NotFound,
  NoChange,
  Applied,
};

struct CreativeTerrainProtectedRegionMutationReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeTerrainProtectedRegionMutationStatus status =
      CreativeTerrainProtectedRegionMutationStatus::NotRequested;
  std::size_t regionIndex = 0U;
  std::uint64_t countBefore = 0U;
  std::uint64_t countAfter = 0U;
  std::string_view reasonCode =
      "creative_terrain_protected_region_mutation_not_requested";
};

enum class CreativeTerrainCompositionStatus : std::uint8_t {
  NotRequested,
  UnsupportedVersion,
  InvalidMask,
  InvalidMode,
  InvalidFeather,
  InvalidProtectedRegion,
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
  std::uint64_t protectedCellCount = 0U;
  std::uint64_t modifiedCellCount = 0U;
  std::uint64_t materialModifiedCellCount = 0U;
  std::uint64_t preservedCellCount = 0U;
  std::uint64_t materializedSourceCellCount = 0U;
  std::uint64_t heightHash = 0U;
  std::uint64_t materialHash = 0U;
  std::string_view reasonCode =
      "creative_terrain_composition_not_requested";
};

struct CreativeTerrainCompositionResult {
  CreativeTerrainCompositionRecipe recipe{};
  CreativeTerrainHeightField heightField;
  CreativeTerrainMaterialField materialField;
  CreativeTerrainCompositionReceipt receipt;
};

[[nodiscard]] bool isValidCreativeTerrainCompositionRecipe(
    const CreativeTerrainCompositionRecipe& recipe) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainCompositionMask mask) noexcept;
[[nodiscard]] bool parseCreativeTerrainCompositionMask(
    std::string_view value,
    CreativeTerrainCompositionMask& output) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainCompositionMode mode) noexcept;
[[nodiscard]] bool parseCreativeTerrainCompositionMode(
    std::string_view value,
    CreativeTerrainCompositionMode& output) noexcept;
// Returns a deterministic 0..65535 inclusion weight for one local cell. Region
// and full-generation recipes share this kernel so 2D/3D mask previews cannot
// drift from committed terrain.
[[nodiscard]] std::uint32_t creativeTerrainCompositionMaskWeight(
    CreativeTerrainCompositionMask mask,
    std::uint16_t localX,
    std::uint16_t localZ,
    CreativeTerrainHeightFieldBounds bounds,
    std::uint16_t featherCells) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainCompositionStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainProtectedRegionMutationStatus status) noexcept;
[[nodiscard]] CreativeTerrainProtectedRegionMutationReceipt
addCreativeTerrainCompositionProtectedRegion(
    CreativeTerrainCompositionRecipe& recipe,
    CreativeTerrainCompositionProtectedRegion region) noexcept;
[[nodiscard]] CreativeTerrainProtectedRegionMutationReceipt
removeCreativeTerrainCompositionProtectedRegion(
    CreativeTerrainCompositionRecipe& recipe,
    std::size_t regionIndex) noexcept;
[[nodiscard]] CreativeTerrainProtectedRegionMutationReceipt
clearCreativeTerrainCompositionProtectedRegions(
    CreativeTerrainCompositionRecipe& recipe) noexcept;

// Materializes the union of the existing authored bounds and generated bounds,
// sampling canonical source terrain for every cell before applying the mask.
// The returned field is complete document-candidate truth: preview and Apply
// must consume this exact field rather than recomposing independently.
// Smooth is one deterministic 3x3 box-filter pass over present source cells;
// empty centers stay empty. Traversal is deterministic row-major O(output
// cells + source columns), and capacity is rejected before output allocation
// so failure leaves no partial candidate.
[[nodiscard]] CreativeTerrainCompositionResult composeCreativeTerrainGeneration(
    const CreativeTerrainHeightField& existingAuthored,
    const CreativeTerrainMaterialField& existingMaterial,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainGenerationResult& generation,
    const CreativeTerrainCompositionRecipe& recipe);
[[nodiscard]] CreativeTerrainCompositionResult composeCreativeTerrainGeneration(
    const CreativeTerrainHeightField& existingAuthored,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainGenerationResult& generation,
    const CreativeTerrainCompositionRecipe& recipe);

}  // namespace iggy3d::creative
