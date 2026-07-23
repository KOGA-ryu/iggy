#pragma once

#include "app/iggy3d/creative/recipes/TerrainGradeRecipe.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

struct CreativeTerrainGradeRect {
  CreativeTerrainCoord2 minimum{};
  CreativeTerrainCoord2 maximum{};

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainGradeRect,
      CreativeTerrainGradeRect) noexcept = default;
};

enum class CreativeTerrainGradeAdapterKind : std::uint8_t {
  PathSegment,
  BuildingPadApproach,
  BridgeApproaches,
  Count,
};

enum class CreativeTerrainGradeAdapterStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  CoordinateOverflow,
  RecipeRejected,
  Ready,
};

struct CreativeTerrainGradeAdapterPlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainGradeAdapterKind kind =
      CreativeTerrainGradeAdapterKind::Count;
  CreativeTerrainGradeAdapterStatus status =
      CreativeTerrainGradeAdapterStatus::NotRequested;
  std::array<CreativeTerrainGradeRecipe, 2U> recipes{};
  std::size_t recipeCount = 0U;
  std::string_view reasonCode =
      "creative_terrain_grade_adapter_not_requested";
};

struct CreativeTerrainGradePathSegmentRequest {
  CreativeTerrainCoord2 start{};
  CreativeTerrainCoord2 end{};
  std::uint16_t startHeightCells = 4U;
  std::uint16_t endHeightCells = 4U;
  std::uint16_t halfWidthCells = 1U;
  std::int32_t crossSlopePermille = 0;
  std::uint16_t falloffCells = 2U;
};

struct CreativeTerrainGradeBuildingPadApproachRequest {
  CreativeTerrainGradeRect padFootprint{};
  CreativeTerrainCoord2 terrainEndpoint{};
  CreativeTerrainCoord2 padEndpoint{};
  std::uint16_t terrainHeightCells = 4U;
  std::uint16_t padHeightCells = 4U;
  std::uint16_t halfWidthCells = 1U;
  std::int32_t crossSlopePermille = 0;
  std::uint16_t falloffCells = 2U;
};

struct CreativeTerrainGradeBridgeApproachRequest {
  CreativeTerrainGradeRect bridgeFootprint{};
  std::uint16_t firstTerrainHeightCells = 4U;
  std::uint16_t deckHeightCells = 4U;
  std::uint16_t secondTerrainHeightCells = 4U;
  std::uint16_t approachLengthCells = 4U;
  std::int32_t crossSlopePermille = 0;
  std::uint16_t falloffCells = 2U;
};

[[nodiscard]] std::string_view toString(
    CreativeTerrainGradeAdapterStatus status) noexcept;
[[nodiscard]] CreativeTerrainGradeAdapterPlan
planCreativeTerrainGradePathSegment(
    const CreativeTerrainGradePathSegmentRequest& request) noexcept;
[[nodiscard]] CreativeTerrainGradeAdapterPlan
planCreativeTerrainGradeBuildingPadApproach(
    const CreativeTerrainGradeBuildingPadApproachRequest& request) noexcept;
[[nodiscard]] CreativeTerrainGradeAdapterPlan
planCreativeTerrainGradeBridgeApproaches(
    const CreativeTerrainGradeBridgeApproachRequest& request) noexcept;

}  // namespace iggy3d::creative
