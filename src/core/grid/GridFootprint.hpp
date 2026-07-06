#pragma once

#include <cstdint>
#include <string_view>

namespace iggy3d {

struct GridFootprint {
  std::int32_t minCellX = 0;
  std::int32_t minCellZ = 0;
  std::int32_t maxCellXExclusive = 0;
  std::int32_t maxCellZExclusive = 0;

  [[nodiscard]] std::int64_t width() const noexcept;
  [[nodiscard]] std::int64_t depth() const noexcept;
};

struct GridCellCoord {
  std::int32_t x = 0;
  std::int32_t z = 0;
};

enum class GridFootprintStatus : std::uint8_t {
  Unknown,
  Ok,
  InvalidCellSize,
  NonFiniteInput,
  OutOfRange,
  EmptyFootprint,
  Misaligned,
};

struct GridFootprintResult {
  bool ok = false;
  GridFootprintStatus status = GridFootprintStatus::Unknown;
  std::string_view reasonCode = "grid_footprint_not_requested";
  GridFootprint footprint;
};

struct GridCellCoordResult {
  bool ok = false;
  GridFootprintStatus status = GridFootprintStatus::Unknown;
  std::string_view reasonCode = "grid_cell_not_requested";
  GridCellCoord cell;
};

[[nodiscard]] std::string_view toString(GridFootprintStatus status) noexcept;

// Half-open containing footprint: every cell intersecting [min,max) in XZ.
[[nodiscard]] GridFootprintResult gridFootprintForContainingBounds(
    float minX,
    float minZ,
    float maxX,
    float maxZ,
    float cellSizeMeters) noexcept;

// Grid-line aligned footprint: every boundary must be near an integer grid line.
[[nodiscard]] GridFootprintResult gridFootprintForAlignedBounds(
    float minX,
    float minZ,
    float maxX,
    float maxZ,
    float cellSizeMeters,
    float toleranceCells = 0.0001F) noexcept;

// Containing cell for a point in XZ.
[[nodiscard]] GridCellCoordResult gridCellForPoint(
    float x,
    float z,
    float cellSizeMeters) noexcept;

}  // namespace iggy3d
