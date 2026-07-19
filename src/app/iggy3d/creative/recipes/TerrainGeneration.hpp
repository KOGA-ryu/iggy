#pragma once

#include "app/iggy3d/creative/document/TerrainHeightField.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

inline constexpr std::uint32_t kCreativeTerrainGeneratorRecipeVersion = 1U;
inline constexpr std::uint8_t kCreativeTerrainGeneratorMaximumOctaves = 8U;

enum class CreativeTerrainGeneratorKind : std::uint8_t {
  SlopeDampedFbm,
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
};

enum class CreativeTerrainGenerationStatus : std::uint8_t {
  NotRequested,
  UnsupportedVersion,
  InvalidKind,
  InvalidBounds,
  InvalidParameters,
  EvaluationFailed,
  HeightFieldRejected,
  Ready,
};

struct CreativeTerrainGenerationReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainGenerationStatus status =
      CreativeTerrainGenerationStatus::NotRequested;
  std::uint64_t generatedCellCount = 0U;
  std::uint64_t dampedContributionCount = 0U;
  std::uint16_t minimumHeightCells = 0U;
  std::uint16_t maximumHeightCells = 0U;
  std::uint64_t heightHash = 0U;
  std::string_view reasonCode = "creative_terrain_generation_not_requested";
};

struct CreativeTerrainGenerationPlan {
  CreativeTerrainGeneratorRecipe recipe{};
  CreativeTerrainHeightField heightField;
};

struct CreativeTerrainGenerationResult {
  CreativeTerrainGenerationPlan plan;
  CreativeTerrainGenerationReceipt receipt;
};

[[nodiscard]] bool isValidCreativeTerrainGeneratorRecipe(
    const CreativeTerrainGeneratorRecipe& recipe) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainGeneratorKind kind) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainGenerationStatus status) noexcept;

// Evaluates the complete bounded field without mutating a document. Output is
// quantized before it enters the plan, so previews, tests, and later commits
// can share exactly the same terrain heights.
[[nodiscard]] CreativeTerrainGenerationResult
buildCreativeTerrainGenerationPlan(
    const CreativeTerrainGeneratorRecipe& recipe);

}  // namespace iggy3d::creative
