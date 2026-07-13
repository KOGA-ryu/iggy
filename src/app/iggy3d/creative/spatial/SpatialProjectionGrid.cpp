#include "app/iggy3d/creative/spatial/SpatialProjectionInternal.hpp"

#include "app/iggy3d/creative/Geometry.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <span>
#include <vector>

namespace iggy3d::creative::spatial_projection_internal {

[[nodiscard]] bool validCellSize(double cellSize) noexcept {
  return std::isfinite(cellSize) && cellSize > 0.0;
}

[[nodiscard]] bool isValidRequest(
    const CreativeSpatialProjectionRequest& request) noexcept {
  return isValidGridSize(request.gridSize) && validCellSize(request.cellSize);
}

[[nodiscard]] bool boundsOutsideGrid(CreativeGridBounds3 bounds,
                                     CreativeGridSize3 size) noexcept {
  return bounds.min.x < 0 || bounds.min.y < 0 || bounds.min.z < 0 ||
         bounds.max.x > size.width || bounds.max.y > size.height ||
         bounds.max.z > size.depth;
}

[[nodiscard]] std::uint64_t cellCount(CreativeGridBounds3 bounds) noexcept {
  const auto width = static_cast<std::uint64_t>(bounds.max.x - bounds.min.x);
  const auto height = static_cast<std::uint64_t>(bounds.max.y - bounds.min.y);
  const auto depth = static_cast<std::uint64_t>(bounds.max.z - bounds.min.z);
  return width * height * depth;
}

void fillBoundsCells(std::vector<CreativeSpatialCell>& cells,
                     CreativeGridSize3 size,
                     CreativeGridBounds3 bounds,
                     const CreativeObject& object,
                     CreativeSpatialOccupancyKind occupancyKind) {
  for (std::int32_t z = bounds.min.z; z < bounds.max.z; ++z) {
    for (std::int32_t y = bounds.min.y; y < bounds.max.y; ++y) {
      for (std::int32_t x = bounds.min.x; x < bounds.max.x; ++x) {
        const CreativeGridCoord3 coord{x, y, z};
        cells.push_back(CreativeSpatialCell{toGridIndex(coord, size),
                                            coord,
                                            object.id,
                                            object.kind,
                                            occupancyKind});
      }
    }
  }
}

[[nodiscard]] CreativeGridBounds3 pointBounds(CreativeGridCoord3 coord) noexcept {
  return CreativeGridBounds3{
      coord,
      CreativeGridCoord3{coord.x + 1, coord.y + 1, coord.z + 1},
  };
}

[[nodiscard]] bool canExpandCell(CreativeGridCoord3 coord) noexcept {
  constexpr std::int32_t maxCell = std::numeric_limits<std::int32_t>::max();
  return coord.x < maxCell && coord.y < maxCell && coord.z < maxCell;
}

[[nodiscard]] CreativeGridBounds3 pointBoundsOrDefault(
    CreativeGridCoord3 coord) noexcept {
  return canExpandCell(coord) ? pointBounds(coord) : CreativeGridBounds3{};
}

[[nodiscard]] CreativeGridBounds3 lineBounds(CreativeGridCoord3 start,
                                             CreativeGridCoord3 end) noexcept {
  return CreativeGridBounds3{
      CreativeGridCoord3{
          std::min(start.x, end.x),
          std::min(start.y, end.y),
          std::min(start.z, end.z),
      },
      CreativeGridCoord3{
          std::max(start.x, end.x) + 1,
          std::max(start.y, end.y) + 1,
          std::max(start.z, end.z) + 1,
      },
  };
}

[[nodiscard]] CreativeGridBounds3 lineBoundsOrDefault(
    CreativeGridCoord3 start,
    CreativeGridCoord3 end) noexcept {
  const CreativeGridCoord3 maxCoord{
      std::max(start.x, end.x),
      std::max(start.y, end.y),
      std::max(start.z, end.z),
  };
  return canExpandCell(maxCoord) ? lineBounds(start, end)
                                 : CreativeGridBounds3{};
}

[[nodiscard]] CreativeGridBounds3 mergeBounds(CreativeGridBounds3 lhs,
                                              CreativeGridBounds3 rhs) noexcept {
  return CreativeGridBounds3{
      CreativeGridCoord3{
          std::min(lhs.min.x, rhs.min.x),
          std::min(lhs.min.y, rhs.min.y),
          std::min(lhs.min.z, rhs.min.z),
      },
      CreativeGridCoord3{
          std::max(lhs.max.x, rhs.max.x),
          std::max(lhs.max.y, rhs.max.y),
          std::max(lhs.max.z, rhs.max.z),
      },
  };
}

[[nodiscard]] bool checkedInt32(double value, std::int32_t& out) noexcept {
  if (!std::isfinite(value) ||
      value < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      value > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
    return false;
  }
  out = static_cast<std::int32_t>(value);
  return true;
}

[[nodiscard]] bool tryWorldToGridCoord(CreativeVec3 position,
                                       double cellSize,
                                       CreativeGridCoord3& out) noexcept {
  if (!validCellSize(cellSize) || !isFiniteCreativeVec3(position)) {
    return false;
  }

  CreativeGridCoord3 coord;
  if (!checkedInt32(std::floor(position.x / cellSize), coord.x) ||
      !checkedInt32(std::floor(position.y / cellSize), coord.y) ||
      !checkedInt32(std::floor(position.z / cellSize), coord.z)) {
    return false;
  }

  out = coord;
  return true;
}

[[nodiscard]] bool tryWorldBoundsToGridBounds(
    CreativeBounds bounds,
    double cellSize,
    CreativeGridBounds3& out) noexcept {
  if (!validCellSize(cellSize) || !isFiniteCreativeVec3(bounds.min) ||
      !isFiniteCreativeVec3(bounds.max)) {
    return false;
  }

  CreativeGridBounds3 gridBounds;
  if (!checkedInt32(std::floor(bounds.min.x / cellSize), gridBounds.min.x) ||
      !checkedInt32(std::floor(bounds.min.y / cellSize), gridBounds.min.y) ||
      !checkedInt32(std::floor(bounds.min.z / cellSize), gridBounds.min.z) ||
      !checkedInt32(std::ceil(bounds.max.x / cellSize), gridBounds.max.x) ||
      !checkedInt32(std::ceil(bounds.max.y / cellSize), gridBounds.max.y) ||
      !checkedInt32(std::ceil(bounds.max.z / cellSize), gridBounds.max.z)) {
    return false;
  }

  out = gridBounds;
  return true;
}

[[nodiscard]] bool containsCoord(std::span<const CreativeSpatialCell> cells,
                                 CreativeGridCoord3 coord) noexcept {
  return std::any_of(cells.begin(),
                     cells.end(),
                     [coord](const CreativeSpatialCell& cell) {
                       return cell.coord == coord;
                     });
}

void appendSampledLineCells(std::vector<CreativeSpatialCell>& cells,
                            CreativeGridSize3 size,
                            CreativeGridCoord3 start,
                            CreativeGridCoord3 end,
                            const CreativeObject& object,
                            CreativeSpatialOccupancyKind occupancyKind,
                            bool dedupeAgainstExisting) {
  forEachSampledLineCoord(
      start,
      end,
      [&](CreativeGridCoord3 coord, std::int32_t) {
        if (dedupeAgainstExisting && containsCoord(cells, coord)) {
          return;
        }
        cells.push_back(CreativeSpatialCell{toGridIndex(coord, size),
                                            coord,
                                            object.id,
                                            object.kind,
                                            occupancyKind});
      });
}

}  // namespace iggy3d::creative::spatial_projection_internal

namespace iggy3d::creative {

using spatial_projection_internal::tryWorldBoundsToGridBounds;
using spatial_projection_internal::tryWorldToGridCoord;

bool isValidGridSize(CreativeGridSize3 size) noexcept {
  return size.width > 0 && size.height > 0 && size.depth > 0;
}

bool isInsideGrid(CreativeGridCoord3 coord, CreativeGridSize3 size) noexcept {
  return coord.x >= 0 && coord.x < size.width && coord.y >= 0 &&
         coord.y < size.height && coord.z >= 0 && coord.z < size.depth;
}

CreativeGridIndex toGridIndex(CreativeGridCoord3 coord,
                              CreativeGridSize3 size) noexcept {
  const auto width = static_cast<CreativeGridIndex>(size.width);
  const auto height = static_cast<CreativeGridIndex>(size.height);
  return static_cast<CreativeGridIndex>(coord.z) * width * height +
         static_cast<CreativeGridIndex>(coord.y) * width +
         static_cast<CreativeGridIndex>(coord.x);
}

CreativeGridCoord3 toGridCoord(CreativeGridIndex index,
                               CreativeGridSize3 size) noexcept {
  if (size.width <= 0 || size.height <= 0) {
    return {};
  }
  const auto width = static_cast<CreativeGridIndex>(size.width);
  const auto height = static_cast<CreativeGridIndex>(size.height);
  const auto layer = width * height;
  return CreativeGridCoord3{
      static_cast<std::int32_t>(index % width),
      static_cast<std::int32_t>((index / width) % height),
      static_cast<std::int32_t>(index / layer),
  };
}

CreativeGridCoord3 worldToGridCoord(CreativeVec3 position,
                                    double cellSize) noexcept {
  CreativeGridCoord3 coord;
  return tryWorldToGridCoord(position, cellSize, coord) ? coord
                                                       : CreativeGridCoord3{};
}

CreativeGridBounds3 worldBoundsToGridBounds(CreativeBounds bounds,
                                            double cellSize) noexcept {
  CreativeGridBounds3 gridBounds;
  return tryWorldBoundsToGridBounds(bounds, cellSize, gridBounds)
             ? gridBounds
             : CreativeGridBounds3{};
}

CreativeGridBounds3 clampGridBounds(CreativeGridBounds3 bounds,
                                    CreativeGridSize3 size) noexcept {
  return CreativeGridBounds3{
      CreativeGridCoord3{
          std::clamp(bounds.min.x, std::int32_t{0}, size.width),
          std::clamp(bounds.min.y, std::int32_t{0}, size.height),
          std::clamp(bounds.min.z, std::int32_t{0}, size.depth),
      },
      CreativeGridCoord3{
          std::clamp(bounds.max.x, std::int32_t{0}, size.width),
          std::clamp(bounds.max.y, std::int32_t{0}, size.height),
          std::clamp(bounds.max.z, std::int32_t{0}, size.depth),
      },
  };
}

bool isEmptyGridBounds(CreativeGridBounds3 bounds) noexcept {
  return bounds.min.x >= bounds.max.x || bounds.min.y >= bounds.max.y ||
         bounds.min.z >= bounds.max.z;
}

}  // namespace iggy3d::creative
