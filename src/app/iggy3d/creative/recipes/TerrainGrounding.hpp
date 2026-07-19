#pragma once

#include "app/iggy3d/creative/document/TerrainHeightField.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeTerrainGroundingStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  InvalidTerrain,
  FootprintTooLarge,
  MissingSurface,
  ReliefExceeded,
  Ready,
};

struct CreativeTerrainGroundingRequest {
  const CreativeTerrainSurfacePlan* terrain = nullptr;
  CreativeTerrainCoord2 minimum{};
  CreativeTerrainCoord2 maximum{};
  double authoredGroundLayer = 0.0;
  std::uint16_t maximumReliefCells = 4U;
};

struct CreativeTerrainGroundingPlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainGroundingStatus status =
      CreativeTerrainGroundingStatus::NotRequested;
  std::size_t sampleCount = 0U;
  std::uint16_t minimumHeightCells = 0U;
  std::uint16_t maximumHeightCells = 0U;
  std::uint16_t reliefCells = 0U;
  double targetGroundLayer = 0.0;
  double verticalOffsetLayers = 0.0;
  std::string_view reasonCode = "creative_terrain_grounding_not_requested";
};

// Resolves one rigid building elevation from every terrain cell under its
// exclusive footprint. Holes fail closed; accepted plans place the authored
// ground plane on the highest sampled terrain column.
[[nodiscard]] CreativeTerrainGroundingPlan planCreativeTerrainGrounding(
    const CreativeTerrainGroundingRequest& request) noexcept;

}  // namespace iggy3d::creative
