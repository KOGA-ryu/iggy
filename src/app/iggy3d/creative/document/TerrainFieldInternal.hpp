#pragma once

#include "app/iggy3d/creative/document/TerrainField.hpp"

namespace iggy3d::creative::terrain_field_internal {

[[nodiscard]] inline bool coordLess(CreativeTerrainCoord2 lhs,
                                    CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z != rhs.z ? lhs.z < rhs.z : lhs.x < rhs.x;
}

[[nodiscard]] std::uint32_t terrainContributionWeight(
    const CreativeTerrainControlPoint& control,
    CreativeTerrainCoord2 coord) noexcept;
[[nodiscard]] CreativeTerrainHeightSample sampleTerrainHeightUnchecked(
    std::span<const CreativeTerrainControlPoint> controls,
    CreativeTerrainCoord2 coord) noexcept;

}  // namespace iggy3d::creative::terrain_field_internal
