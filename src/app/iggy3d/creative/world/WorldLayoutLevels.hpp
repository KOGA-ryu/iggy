#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <cstdint>

namespace iggy3d::creative {

struct CreativeWorldLayoutResolvedRoomGeometry {
  bool valid = false;
  std::size_t levelIndex = kInvalidCreativeWorldLayoutIndex;
  double floorTopLayer = 0.0;
  std::uint16_t wallHeightCells = 0U;
  std::uint16_t floorThicknessLayers = 0U;
  std::uint16_t upperSurfaceThicknessLayers = 0U;
  CreativeObjectKind upperSurfaceKind = CreativeObjectKind::Unknown;
};

// Validates the normalized building -> level -> room ownership graph. Levels
// within one building must have distinct elevations.
[[nodiscard]] std::size_t firstInvalidCreativeWorldLayoutLevelIndex(
    const CreativeWorldLayout& layout) noexcept;

[[nodiscard]] bool validCreativeWorldLayoutLevelOwnership(
    const CreativeWorldLayout& layout) noexcept;

[[nodiscard]] const CreativeWorldLayoutLevel*
creativeWorldLayoutLevelForRoom(const CreativeWorldLayout& layout,
                                std::size_t roomIndex) noexcept;

[[nodiscard]] bool creativeWorldLayoutLevelHasRooms(
    const CreativeWorldLayout& layout, std::size_t levelIndex) noexcept;

[[nodiscard]] bool creativeWorldLayoutLevelIsTopmostOccupied(
    const CreativeWorldLayout& layout, std::size_t levelIndex) noexcept;

[[nodiscard]] CreativeWorldLayoutResolvedRoomGeometry
resolveCreativeWorldLayoutRoomGeometry(const CreativeWorldLayout& layout,
                                       std::size_t roomIndex) noexcept;

}  // namespace iggy3d::creative
