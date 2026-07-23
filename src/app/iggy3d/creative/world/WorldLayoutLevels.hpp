#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

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

enum class CreativeWorldLayoutLevelDatumEditScope : std::uint8_t {
  Selected,
  SelectedAndAbove,
  SelectedAndBelow,
  All,
  Count,
};

struct CreativeWorldLayoutLevelDatumEditRequest {
  std::size_t levelIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutLevelDatumEditScope scope =
      CreativeWorldLayoutLevelDatumEditScope::Selected;
  double requestedFloorTopLayer = 0.0;
};

struct CreativeWorldLayoutLevelDatumEditPlan {
  bool accepted = false;
  bool changed = false;
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t selectedLevelIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutLevelDatumEditScope scope =
      CreativeWorldLayoutLevelDatumEditScope::Selected;
  double selectedFloorTopLayerBefore = 0.0;
  double snappedFloorTopLayer = 0.0;
  double deltaCells = 0.0;
  std::size_t affectedLevelCount = 0U;
  std::size_t affectedBoxCount = 0U;
  std::size_t adjustedWallCount = 0U;
  bool buildingRootAdjusted = false;
  std::string_view reasonCode =
      "creative_world_layout_level_datum_edit_not_requested";
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

// Exterior facades inherit the complete storey band up to the next occupied
// floor datum. The top storey inherits its authored wall height.
[[nodiscard]] double creativeWorldLayoutLevelFacadeHeightCells(
    const CreativeWorldLayout& layout, std::size_t levelIndex) noexcept;

// Finds the nearest distinct visible floor datum without depending on level
// table order. When several buildings share the target datum, the current
// building wins; otherwise the lowest table index keeps the result stable.
// Runs in O(level count) time without allocating.
[[nodiscard]] CreativeWorldLayoutLevelNavigationResult
navigateCreativeWorldLayoutLevel(
    const CreativeWorldLayout& layout, std::size_t activeLevelIndex,
    CreativeWorldLayoutLevelNavigationDirection direction) noexcept;

// Plans one building-local datum transaction in
// O(levels^2 + (boxes + walls) * levels) time without allocating. Sources
// attached by vertical ownership move or resize with their affected level
// endpoints. Application validates the complete plan before writing anything.
[[nodiscard]] CreativeWorldLayoutLevelDatumEditPlan
planCreativeWorldLayoutLevelDatumEdit(
    const CreativeWorldLayout& layout,
    CreativeWorldLayoutLevelDatumEditRequest request) noexcept;

[[nodiscard]] bool applyCreativeWorldLayoutLevelDatumEditPlan(
    CreativeWorldLayout& layout,
    const CreativeWorldLayoutLevelDatumEditPlan& plan) noexcept;

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
