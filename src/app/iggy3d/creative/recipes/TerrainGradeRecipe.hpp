#pragma once

#include "app/iggy3d/creative/document/TerrainHeightField.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

inline constexpr std::uint32_t kCreativeTerrainGradeRecipeVersion = 1U;
inline constexpr std::uint16_t kCreativeTerrainGradeMaximumHalfWidthCells =
    64U;
inline constexpr std::uint16_t kCreativeTerrainGradeMaximumFalloffCells = 64U;
inline constexpr std::int32_t kCreativeTerrainGradeMaximumCrossSlopePermille =
    1000;

// One durable, straight terrain corridor. Coordinates identify tile centers;
// the signed cross slope rises to the left of start -> end when positive.
struct CreativeTerrainGradeRecipe {
  std::uint32_t version = kCreativeTerrainGradeRecipeVersion;
  CreativeTerrainCoord2 start{};
  CreativeTerrainCoord2 end{1, 0};
  std::uint16_t startHeightCells = 4U;
  std::uint16_t endHeightCells = 4U;
  std::uint16_t halfWidthCells = 2U;
  std::int32_t crossSlopePermille = 0;
  std::uint16_t falloffCells = 2U;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainGradeRecipe,
      CreativeTerrainGradeRecipe) noexcept = default;
};

enum class CreativeTerrainGradeRecipeStatus : std::uint8_t {
  NotRequested,
  UnsupportedVersion,
  InvalidRecipe,
  InvalidSource,
  CapacityExceeded,
  HeightOutOfRange,
  OutputRejected,
  Ready,
};

struct CreativeTerrainGradeReadout {
  double lengthCells = 0.0;
  double longitudinalSlopePercent = 0.0;
  double crossSlopePercent = 0.0;
  double maximumCollisionSlopeDegrees = 0.0;
  double maximumWalkableSlopeDegrees = 40.0;
  std::string_view movementBand = "invalid";
  bool walkable = false;
};

struct CreativeTerrainGradeRecipeReceipt {
  bool requested = false;
  bool accepted = false;
  bool boundsExpanded = false;
  CreativeTerrainGradeRecipeStatus status =
      CreativeTerrainGradeRecipeStatus::NotRequested;
  std::uint64_t sourceColumnCount = 0U;
  std::uint64_t evaluatedCellCount = 0U;
  std::uint64_t corridorCellCount = 0U;
  std::uint64_t falloffCellCount = 0U;
  std::uint64_t modifiedCellCount = 0U;
  std::uint64_t outputCellCount = 0U;
  std::uint64_t heightHash = 0U;
  CreativeTerrainGradeReadout readout{};
  std::string_view reasonCode = "creative_terrain_grade_not_requested";
};

struct CreativeTerrainGradeRecipeResult {
  CreativeTerrainGradeRecipe recipe{};
  CreativeTerrainHeightField heightField;
  CreativeTerrainGradeRecipeReceipt receipt{};
};

[[nodiscard]] bool isValidCreativeTerrainGradeRecipe(
    const CreativeTerrainGradeRecipe& recipe) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainGradeRecipeStatus status) noexcept;

// Applies one corridor to the authored height field. canonicalSource must be
// the composed legacy + authored terrain at this point in operation replay.
// Output and readout are deterministic and bounded by the height-field cap.
[[nodiscard]] CreativeTerrainGradeRecipeResult buildCreativeTerrainGradeRecipe(
    const CreativeTerrainHeightField& existingAuthored,
    const CreativeTerrainSurfacePlan& canonicalSource,
    const CreativeTerrainGradeRecipe& recipe,
    double maximumWalkableSlopeDegrees = 40.0);

}  // namespace iggy3d::creative
