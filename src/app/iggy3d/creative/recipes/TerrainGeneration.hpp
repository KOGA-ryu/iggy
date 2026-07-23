#pragma once

#include "app/iggy3d/creative/document/TerrainHeightField.hpp"
#include "app/iggy3d/creative/document/TerrainMaterialField.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

inline constexpr std::uint32_t kCreativeTerrainGeneratorRecipeVersion = 2U;
inline constexpr std::uint8_t kCreativeTerrainGeneratorMaximumOctaves = 8U;
inline constexpr double kCreativeTerrainGeneratorMinimumHorizontalScaleCells =
    1.0;
inline constexpr double kCreativeTerrainGeneratorMaximumHorizontalScaleCells =
    4096.0;
inline constexpr double kCreativeTerrainGeneratorMinimumPersistence = 0.05;
inline constexpr double kCreativeTerrainGeneratorMaximumPersistence = 1.0;
inline constexpr double kCreativeTerrainGeneratorMinimumLacunarity = 1.25;
inline constexpr double kCreativeTerrainGeneratorMaximumLacunarity = 4.0;
inline constexpr double kCreativeTerrainGeneratorMaximumSlopeDamping = 8.0;

enum class CreativeTerrainGeneratorKind : std::uint8_t {
  SlopeDampedFbm,
  Count,
};

enum class CreativeTerrainBiomeIntent : std::uint8_t {
  Temperate,
  Alpine,
  Arid,
  Wetland,
  Custom,
  Count,
};

struct CreativeTerrainGeneratorRecipe {
  std::uint32_t version = kCreativeTerrainGeneratorRecipeVersion;
  CreativeTerrainGeneratorKind kind =
      CreativeTerrainGeneratorKind::SlopeDampedFbm;
  std::uint64_t seed = 1U;
  CreativeTerrainHeightFieldBounds bounds{};
  std::uint16_t baseHeightCells = 8U;
  std::uint16_t reliefCells = 6U;
  double horizontalScaleCells = 24.0;
  std::uint8_t octaveCount = 5U;
  double persistence = 0.5;
  double lacunarity = 2.0;
  double slopeDamping = 0.35;
  bool paintMaterials = true;
  CreativeTerrainBiomeIntent biomeIntent =
      CreativeTerrainBiomeIntent::Temperate;
  CreativeTerrainMaterial lowlandMaterial = CreativeTerrainMaterial::Grass;
  CreativeTerrainMaterial highlandMaterial = CreativeTerrainMaterial::Stone;
  std::uint16_t materialTransitionHeightCells = 11U;

  [[nodiscard]] friend bool operator==(
      const CreativeTerrainGeneratorRecipe&,
      const CreativeTerrainGeneratorRecipe&) noexcept = default;
};

enum class CreativeTerrainGenerationStatus : std::uint8_t {
  NotRequested,
  UnsupportedVersion,
  InvalidKind,
  InvalidBounds,
  InvalidParameters,
  EvaluationFailed,
  HeightFieldRejected,
  MaterialFieldRejected,
  Ready,
};

struct CreativeTerrainGenerationReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainGenerationStatus status =
      CreativeTerrainGenerationStatus::NotRequested;
  std::uint64_t generatedCellCount = 0U;
  std::uint64_t evaluatedOctaveCount = 0U;
  std::uint64_t dampedContributionCount = 0U;
  std::uint64_t generatedMaterialOverrideCount = 0U;
  std::uint16_t minimumHeightCells = 0U;
  std::uint16_t maximumHeightCells = 0U;
  std::uint64_t heightHash = 0U;
  std::uint64_t materialHash = 0U;
  std::string_view reasonCode = "creative_terrain_generation_not_requested";
};

struct CreativeTerrainGenerationPlan {
  CreativeTerrainGeneratorRecipe recipe{};
  CreativeTerrainHeightField heightField;
  CreativeTerrainMaterialField materialField;
};

struct CreativeTerrainGenerationResult {
  CreativeTerrainGenerationPlan plan;
  CreativeTerrainGenerationReceipt receipt;
};

[[nodiscard]] bool isValidCreativeTerrainGeneratorRecipe(
    const CreativeTerrainGeneratorRecipe& recipe) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainGeneratorKind kind) noexcept;
[[nodiscard]] bool parseCreativeTerrainGeneratorKind(
    std::string_view value,
    CreativeTerrainGeneratorKind& output) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainBiomeIntent intent) noexcept;
[[nodiscard]] bool parseCreativeTerrainBiomeIntent(
    std::string_view value,
    CreativeTerrainBiomeIntent& output) noexcept;
// Applies a named material preset by writing all durable material fields.
// Custom leaves those fields untouched, so replay never depends on hidden
// biome lookup behavior.
void applyCreativeTerrainBiomeIntent(
    CreativeTerrainGeneratorRecipe& recipe,
    CreativeTerrainBiomeIntent intent) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainGenerationStatus status) noexcept;

// Evaluates the complete bounded field without mutating a document. Output is
// quantized before it enters the plan, so previews, tests, and later commits
// can share exactly the same terrain heights.
[[nodiscard]] CreativeTerrainGenerationResult
buildCreativeTerrainGenerationPlan(
    const CreativeTerrainGeneratorRecipe& recipe);

}  // namespace iggy3d::creative
