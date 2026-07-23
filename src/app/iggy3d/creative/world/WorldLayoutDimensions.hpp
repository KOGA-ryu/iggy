#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"
#include "runtime/movement/MovementDefaults.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

inline constexpr double kCreativeArchitecturalHumanReferenceHeightMeters =
    iggy3d::kDefaultPlayerStandingHeightMeters;

enum class CreativeWorldLayoutDimensionStatus : std::uint8_t {
  NotRequested,
  InvalidGrid,
  InvalidBuilding,
  InvalidLevel,
  InvalidOpening,
  EmptyBuilding,
  InvalidFootprint,
  InvalidRoof,
  Unrepresentable,
  Ready,
};

struct CreativeWorldLayoutLevelDimensions {
  bool accepted = false;
  CreativeWorldLayoutDimensionStatus status =
      CreativeWorldLayoutDimensionStatus::NotRequested;
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t levelIndex = kInvalidCreativeWorldLayoutIndex;
  bool topmostOccupied = false;
  bool hasUpperLevel = false;
  std::size_t upperLevelIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeObjectKind upperSurfaceKind = CreativeObjectKind::Unknown;
  std::uint16_t floorThicknessLayers = 0U;
  std::uint16_t upperSurfaceThicknessLayers = 0U;
  double floorBottomMeters = 0.0;
  double floorTopMeters = 0.0;
  double wallBaseMeters = 0.0;
  double wallTopMeters = 0.0;
  double wallHeightMeters = 0.0;
  double floorThicknessMeters = 0.0;
  double nextFloorBottomMeters = 0.0;
  double nextFloorTopMeters = 0.0;
  double floorToFloorMeters = 0.0;
  double clearHeightMeters = 0.0;
  double upperSurfaceSupportMeters = 0.0;
  // A non-top ceiling is backed against the underside of the next floor slab.
  // A top-level roof remains supported at the authored wall top.
  double upperSurfaceTopMeters = 0.0;
  double upperSurfaceThicknessMeters = 0.0;
  std::string_view reasonCode =
      "creative_world_layout_dimensions_not_requested";
};

struct CreativeWorldLayoutBuildingDimensions {
  bool accepted = false;
  CreativeWorldLayoutDimensionStatus status =
      CreativeWorldLayoutDimensionStatus::NotRequested;
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t occupiedLevelCount = 0U;
  bool uniformFloorToFloor = true;
  bool uniformWallHeight = true;
  bool uniformFloorThickness = true;
  double footprintMinimumXMeters = 0.0;
  double footprintMaximumXMeters = 0.0;
  double footprintMinimumZMeters = 0.0;
  double footprintMaximumZMeters = 0.0;
  double footprintWidthMeters = 0.0;
  double footprintDepthMeters = 0.0;
  double lowestFloorBottomMeters = 0.0;
  double lowestFloorTopMeters = 0.0;
  double exteriorFacadeBaseMeters = 0.0;
  double exteriorFacadeTopMeters = 0.0;
  double exteriorFacadeHeightMeters = 0.0;
  double roofBaseMeters = 0.0;
  double roofTopMeters = 0.0;
  double totalHeightMeters = 0.0;
  double minimumFloorToFloorMeters = 0.0;
  double maximumFloorToFloorMeters = 0.0;
  double minimumWallHeightMeters = 0.0;
  double maximumWallHeightMeters = 0.0;
  double minimumFloorThicknessMeters = 0.0;
  double maximumFloorThicknessMeters = 0.0;
  std::string_view reasonCode =
      "creative_world_layout_dimensions_not_requested";
};

struct CreativeWorldLayoutOpeningDimensions {
  bool accepted = false;
  CreativeWorldLayoutDimensionStatus status =
      CreativeWorldLayoutDimensionStatus::NotRequested;
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t openingIndex = kInvalidCreativeWorldLayoutIndex;
  double wallBaseMeters = 0.0;
  double wallTopMeters = 0.0;
  double centerOffsetMeters = 0.0;
  double widthMeters = 0.0;
  double cutoutBottomOffsetMeters = 0.0;
  double cutoutBottomMeters = 0.0;
  double cutoutHeightMeters = 0.0;
  double cutoutTopMeters = 0.0;
  double insertBottomOffsetMeters = 0.0;
  double insertBottomMeters = 0.0;
  double insertHeightMeters = 0.0;
  double insertTopMeters = 0.0;
  double insertWidthMeters = 0.0;
  double insertThicknessMeters = 0.0;
  std::string_view reasonCode =
      "creative_world_layout_dimensions_not_requested";
};

// These pure measurements are the unit-conversion choke point for authored
// building geometry. They allocate no memory and never mutate layout state.
[[nodiscard]] CreativeWorldLayoutLevelDimensions
measureCreativeWorldLayoutLevelDimensions(
    const CreativeGridSettings& grid,
    const CreativeWorldLayout& layout,
    std::size_t levelIndex) noexcept;

// Uses an O(levels^2) nearest-higher scan to measure floor-to-floor ranges
// without allocating or imposing a second ordering on the authored level table.
[[nodiscard]] CreativeWorldLayoutBuildingDimensions
measureCreativeWorldLayoutBuildingDimensions(
    const CreativeGridSettings& grid,
    const CreativeWorldLayout& layout,
    std::size_t buildingIndex) noexcept;

// Accepts either source room-edge openings or normalized wall-host openings.
// The compiler uses the normalized form after room/facade expansion.
[[nodiscard]] CreativeWorldLayoutOpeningDimensions
measureCreativeWorldLayoutOpeningDimensions(
    const CreativeGridSettings& grid,
    const CreativeWorldLayout& layout,
    std::size_t openingIndex) noexcept;

}  // namespace iggy3d::creative
