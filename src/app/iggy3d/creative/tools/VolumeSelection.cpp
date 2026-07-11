#include "app/iggy3d/creative/tools/Volume.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <limits>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool finiteVec3(CreativeVec3 value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

[[nodiscard]] bool validCellSize(double cellSize) noexcept {
  return std::isfinite(cellSize) && cellSize > 0.0;
}

[[nodiscard]] bool checkedCoord(double value, std::int32_t& out) noexcept {
  if (!std::isfinite(value) ||
      value < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      value > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
    return false;
  }
  out = static_cast<std::int32_t>(value);
  return true;
}

}  // namespace

std::string_view toString(CreativeVolumeSelectionPhase phase) noexcept {
  switch (phase) {
    case CreativeVolumeSelectionPhase::Empty: return "Empty";
    case CreativeVolumeSelectionPhase::FirstCorner: return "FirstCorner";
    case CreativeVolumeSelectionPhase::Complete: return "Complete";
  }
  return "Unknown";
}

std::string_view toString(CreativeVolumeOperationKind operation) noexcept {
  switch (operation) {
    case CreativeVolumeOperationKind::Fill: return "Fill";
    case CreativeVolumeOperationKind::Hollow: return "Hollow";
    case CreativeVolumeOperationKind::Replace: return "Replace";
    case CreativeVolumeOperationKind::Erase: return "Erase";
    case CreativeVolumeOperationKind::Clone: return "Clone";
    case CreativeVolumeOperationKind::Count: break;
  }
  return "Unknown";
}

bool creativeVolumeSelectionHasFirstCorner(
    const CreativeVolumeSelection& selection) noexcept {
  return selection.phase == CreativeVolumeSelectionPhase::FirstCorner ||
         selection.phase == CreativeVolumeSelectionPhase::Complete;
}

bool creativeVolumeSelectionComplete(
    const CreativeVolumeSelection& selection) noexcept {
  return selection.phase == CreativeVolumeSelectionPhase::Complete;
}

bool creativeVolumeSelectionValid(
    const CreativeVolumeSelection& selection) noexcept {
  if (!creativeVolumeSelectionComplete(selection) ||
      !validCellSize(selection.cellSize) || !finiteVec3(selection.origin)) {
    return false;
  }
  constexpr std::int32_t maxCoord =
      std::numeric_limits<std::int32_t>::max();
  if (selection.firstCell.x == maxCoord || selection.firstCell.y == maxCoord ||
      selection.firstCell.z == maxCoord || selection.secondCell.x == maxCoord ||
      selection.secondCell.y == maxCoord || selection.secondCell.z == maxCoord) {
    return false;
  }
  const CreativeBounds bounds = creativeVolumeWorldBounds(selection);
  return finiteVec3(bounds.min) && finiteVec3(bounds.max) &&
         bounds.max.x > bounds.min.x && bounds.max.y > bounds.min.y &&
         bounds.max.z > bounds.min.z && creativeVolumeCellCount(selection) > 0U;
}

void clearCreativeVolumeSelection(CreativeVolumeSelection& selection) noexcept {
  const double cellSize = selection.cellSize;
  const CreativeVec3 origin = selection.origin;
  selection = {};
  selection.cellSize = cellSize;
  selection.origin = origin;
}

CreativeVolumeSelectionPhase advanceCreativeVolumeSelection(
    CreativeVolumeSelection& selection,
    CreativeGridCoord3 cell) noexcept {
  if (selection.phase == CreativeVolumeSelectionPhase::FirstCorner) {
    selection.secondCell = cell;
    selection.phase = CreativeVolumeSelectionPhase::Complete;
  } else {
    selection.firstCell = cell;
    selection.secondCell = cell;
    selection.phase = CreativeVolumeSelectionPhase::FirstCorner;
  }
  return selection.phase;
}

CreativeVolumeSelectionPhase setCreativeVolumeSelectionCorner(
    CreativeVolumeSelection& selection,
    CreativeVolumeCorner corner,
    CreativeGridCoord3 cell) noexcept {
  if (corner == CreativeVolumeCorner::First) {
    selection.firstCell = cell;
    selection.phase = creativeVolumeSelectionComplete(selection)
                          ? CreativeVolumeSelectionPhase::Complete
                          : CreativeVolumeSelectionPhase::FirstCorner;
    return selection.phase;
  }

  selection.secondCell = cell;
  if (!creativeVolumeSelectionHasFirstCorner(selection)) {
    selection.firstCell = cell;
  }
  selection.phase = CreativeVolumeSelectionPhase::Complete;
  return selection.phase;
}

bool expandCreativeVolumeSelectionToCell(
    CreativeVolumeSelection& selection,
    CreativeGridCoord3 cell) noexcept {
  if (selection.phase == CreativeVolumeSelectionPhase::Empty) {
    selection.firstCell = cell;
    selection.phase = CreativeVolumeSelectionPhase::FirstCorner;
    return true;
  }
  if (selection.phase == CreativeVolumeSelectionPhase::FirstCorner) {
    selection.secondCell = cell;
    selection.phase = CreativeVolumeSelectionPhase::Complete;
    return true;
  }

  const CreativeGridBounds3 bounds = creativeVolumeGridBounds(selection);
  const CreativeGridCoord3 nextMin{
      std::min(bounds.min.x, cell.x),
      std::min(bounds.min.y, cell.y),
      std::min(bounds.min.z, cell.z),
  };
  const CreativeGridCoord3 nextMaxInclusive{
      std::max(bounds.max.x - 1, cell.x),
      std::max(bounds.max.y - 1, cell.y),
      std::max(bounds.max.z - 1, cell.z),
  };
  if (nextMin.x == bounds.min.x && nextMin.y == bounds.min.y &&
      nextMin.z == bounds.min.z && nextMaxInclusive.x == bounds.max.x - 1 &&
      nextMaxInclusive.y == bounds.max.y - 1 &&
      nextMaxInclusive.z == bounds.max.z - 1) {
    return false;
  }
  selection.firstCell = nextMin;
  selection.secondCell = nextMaxInclusive;
  return true;
}

CreativeVolumeSelection previewCreativeVolumeSelection(
    const CreativeVolumeSelection& selection,
    CreativeGridCoord3 cursorCell) noexcept {
  CreativeVolumeSelection preview = selection;
  if (preview.phase == CreativeVolumeSelectionPhase::FirstCorner) {
    preview.secondCell = cursorCell;
    preview.phase = CreativeVolumeSelectionPhase::Complete;
  }
  return preview;
}

bool resizeCreativeVolumeSelectionHeight(CreativeVolumeSelection& selection,
                                         std::int32_t deltaCells) noexcept {
  if (!creativeVolumeSelectionComplete(selection) || deltaCells == 0) {
    return false;
  }
  const std::int64_t minY =
      std::min<std::int64_t>(selection.firstCell.y, selection.secondCell.y);
  const std::int64_t maxY =
      std::max<std::int64_t>(selection.firstCell.y, selection.secondCell.y);
  const std::int64_t height = maxY - minY + 1;
  const std::int64_t nextHeight = std::max<std::int64_t>(1, height + deltaCells);
  const std::int64_t nextMaxY = minY + nextHeight - 1;
  if (nextHeight == height ||
      nextMaxY >= std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  selection.firstCell.y = static_cast<std::int32_t>(minY);
  selection.secondCell.y = static_cast<std::int32_t>(nextMaxY);
  return true;
}

CreativeGridCoord3 creativeVolumeCellFromWorld(CreativeVec3 worldPosition,
                                               double cellSize,
                                               CreativeVec3 origin) noexcept {
  CreativeGridCoord3 cell;
  static_cast<void>(tryCreativeVolumeCellFromWorld(
      worldPosition, cellSize, origin, cell));
  return cell;
}

bool tryCreativeVolumeCellFromWorld(CreativeVec3 worldPosition,
                                    double cellSize,
                                    CreativeVec3 origin,
                                    CreativeGridCoord3& outCell) noexcept {
  if (!finiteVec3(worldPosition) || !finiteVec3(origin) ||
      !validCellSize(cellSize)) {
    return false;
  }
  CreativeGridCoord3 cell;
  if (!checkedCoord(std::floor((worldPosition.x - origin.x) / cellSize),
                    cell.x) ||
      !checkedCoord(std::floor((worldPosition.y - origin.y) / cellSize),
                    cell.y) ||
      !checkedCoord(std::floor((worldPosition.z - origin.z) / cellSize),
                    cell.z)) {
    return false;
  }
  outCell = cell;
  return true;
}

CreativeBounds creativeVolumeCellBounds(CreativeGridCoord3 cell,
                                        double cellSize,
                                        CreativeVec3 origin) noexcept {
  const CreativeVec3 min{
      origin.x + static_cast<double>(cell.x) * cellSize,
      origin.y + static_cast<double>(cell.y) * cellSize,
      origin.z + static_cast<double>(cell.z) * cellSize,
  };
  return {min,
          {min.x + cellSize, min.y + cellSize, min.z + cellSize}};
}

CreativeGridBounds3 creativeVolumeGridBounds(
    const CreativeVolumeSelection& selection) noexcept {
  if (!creativeVolumeSelectionComplete(selection)) {
    return {};
  }
  CreativeGridBounds3 bounds;
  bounds.min = {
      std::min(selection.firstCell.x, selection.secondCell.x),
      std::min(selection.firstCell.y, selection.secondCell.y),
      std::min(selection.firstCell.z, selection.secondCell.z),
  };
  bounds.max = {
      std::max(selection.firstCell.x, selection.secondCell.x) + 1,
      std::max(selection.firstCell.y, selection.secondCell.y) + 1,
      std::max(selection.firstCell.z, selection.secondCell.z) + 1,
  };
  return bounds;
}

CreativeBounds creativeVolumeWorldBounds(
    const CreativeVolumeSelection& selection) noexcept {
  const CreativeGridBounds3 gridBounds = creativeVolumeGridBounds(selection);
  const CreativeVec3 min{
      selection.origin.x +
          static_cast<double>(gridBounds.min.x) * selection.cellSize,
      selection.origin.y +
          static_cast<double>(gridBounds.min.y) * selection.cellSize,
      selection.origin.z +
          static_cast<double>(gridBounds.min.z) * selection.cellSize,
  };
  const CreativeVec3 max{
      selection.origin.x +
          static_cast<double>(gridBounds.max.x) * selection.cellSize,
      selection.origin.y +
          static_cast<double>(gridBounds.max.y) * selection.cellSize,
      selection.origin.z +
          static_cast<double>(gridBounds.max.z) * selection.cellSize,
  };
  return {min, max};
}

std::uint64_t creativeVolumeCellCount(
    const CreativeVolumeSelection& selection) noexcept {
  if (!creativeVolumeSelectionComplete(selection)) {
    return 0U;
  }
  const std::uint64_t width = static_cast<std::uint64_t>(
      std::llabs(static_cast<std::int64_t>(selection.secondCell.x) -
                 selection.firstCell.x) +
      1);
  const std::uint64_t height = static_cast<std::uint64_t>(
      std::llabs(static_cast<std::int64_t>(selection.secondCell.y) -
                 selection.firstCell.y) +
      1);
  const std::uint64_t depth = static_cast<std::uint64_t>(
      std::llabs(static_cast<std::int64_t>(selection.secondCell.z) -
                 selection.firstCell.z) +
      1);
  if (width > std::numeric_limits<std::uint64_t>::max() / height) {
    return 0U;
  }
  const std::uint64_t area = width * height;
  if (area > std::numeric_limits<std::uint64_t>::max() / depth) {
    return 0U;
  }
  return area * depth;
}

CreativeVolumeOperationKind nextCreativeVolumeOperation(
    CreativeVolumeOperationKind operation) noexcept {
  constexpr std::array operations{
      CreativeVolumeOperationKind::Fill,
      CreativeVolumeOperationKind::Hollow,
      CreativeVolumeOperationKind::Replace,
      CreativeVolumeOperationKind::Erase,
      CreativeVolumeOperationKind::Clone,
  };
  const auto current = std::find(operations.begin(), operations.end(), operation);
  if (current == operations.end() || std::next(current) == operations.end()) {
    return operations.front();
  }
  return *std::next(current);
}

CreativeObjectKind firstCreativeVolumeBrush(
    std::span<const CreativeObjectKind> palette) noexcept {
  const auto brush = std::find_if(
      palette.begin(), palette.end(), creativeVolumeBrushSupported);
  return brush == palette.end() ? CreativeObjectKind::Unknown : *brush;
}

CreativeObjectKind nextCreativeVolumeBrush(
    std::span<const CreativeObjectKind> palette,
    CreativeObjectKind current) noexcept {
  if (palette.empty()) {
    return CreativeObjectKind::Unknown;
  }
  const auto currentBrush = std::find(palette.begin(), palette.end(), current);
  const std::size_t start =
      currentBrush == palette.end()
          ? 0U
          : static_cast<std::size_t>(
                std::distance(palette.begin(), currentBrush) + 1);
  for (std::size_t offset = 0; offset < palette.size(); ++offset) {
    const CreativeObjectKind candidate =
        palette[(start + offset) % palette.size()];
    if (creativeVolumeBrushSupported(candidate)) {
      return candidate;
    }
  }
  return CreativeObjectKind::Unknown;
}

}  // namespace iggy3d::creative
