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

enum class CreativeWorldLayoutLevelNavigationDirection : std::uint8_t {
  Lower,
  Higher,
  Count,
};

enum class CreativeWorldLayoutLevelNavigationStatus : std::uint8_t {
  Ready,
  Boundary,
  InvalidActiveLevel,
  InvalidLayout,
  InvalidDirection,
  Count,
};

struct CreativeWorldLayoutLevelNavigationResult {
  CreativeWorldLayoutLevelNavigationStatus status =
      CreativeWorldLayoutLevelNavigationStatus::InvalidActiveLevel;
  std::size_t targetLevelIndex = kInvalidCreativeWorldLayoutIndex;
  double targetFloorTopLayer = 0.0;
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

// Finds the nearest distinct visible floor datum without depending on level
// table order. When several buildings share the target datum, the current
// building wins; otherwise the lowest table index keeps the result stable.
// Runs in O(level count) time without allocating.
[[nodiscard]] CreativeWorldLayoutLevelNavigationResult
navigateCreativeWorldLayoutLevel(
    const CreativeWorldLayout& layout, std::size_t activeLevelIndex,
    CreativeWorldLayoutLevelNavigationDirection direction) noexcept;

// Resolves whether a semantic source participates in one storey. Structural
// spans use half-open vertical bands so adjacent storeys never claim the same
// per-storey wall/opening, while a deliberately tall exterior span can belong
// to every storey it crosses.
[[nodiscard]] bool creativeWorldLayoutSourceTouchesLevel(
    const CreativeWorldLayout& layout, CreativeWorldLayoutTable table,
    std::size_t sourceIndex, std::size_t levelIndex) noexcept;

// Finds the source-owned level at a visible floor datum. Runs in O(level count)
// without allocating and returns the lowest matching table index for stability.
[[nodiscard]] std::size_t creativeWorldLayoutSourceLevelAtDatum(
    const CreativeWorldLayout& layout, CreativeWorldLayoutTable table,
    std::size_t sourceIndex, double floorTopLayer) noexcept;

[[nodiscard]] CreativeWorldLayoutResolvedRoomGeometry
resolveCreativeWorldLayoutRoomGeometry(const CreativeWorldLayout& layout,
                                       std::size_t roomIndex) noexcept;

}  // namespace iggy3d::creative
