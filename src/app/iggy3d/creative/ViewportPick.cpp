#include "app/iggy3d/creative/ViewportPick.hpp"

#include <limits>
#include <utility>

namespace iggy3d::creative {

namespace {

[[nodiscard]] bool containsPointer(CreativeViewportPickViewport viewport,
                                   float pointerX,
                                   float pointerY) noexcept {
  return pointerX >= viewport.x && pointerY >= viewport.y &&
         pointerX < viewport.x + viewport.width &&
         pointerY < viewport.y + viewport.height;
}

[[nodiscard]] bool sameCoord(CreativeGridCoord3 lhs,
                             CreativeGridCoord3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

[[nodiscard]] CreativeViewportPickReceipt makeReceipt(
    CreativeViewportPickStatus status,
    std::string message) {
  CreativeViewportPickReceipt receipt;
  receipt.requested = true;
  receipt.status = status;
  receipt.message = std::move(message);
  return receipt;
}

void copyHitCell(CreativeViewportPickReceipt& receipt,
                 const CreativeSpatialCell& cell,
                 std::size_t cellIndex) noexcept {
  receipt.hit = true;
  receipt.status = CreativeViewportPickStatus::Hit;
  receipt.coord = cell.coord;
  receipt.index = cell.index;
  receipt.objectId = cell.objectId;
  receipt.objectKind = cell.objectKind;
  receipt.occupancyKind = cell.occupancyKind;
  receipt.cellIndex = cellIndex;

  if (cell.objectId != kInvalidObjectId &&
      cell.objectId <= std::numeric_limits<Id>::max()) {
    receipt.target.value = static_cast<Id>(cell.objectId);
  }
}

}  // namespace

std::string_view toString(CreativeViewportPickStatus status) noexcept {
  switch (status) {
    case CreativeViewportPickStatus::Unknown:
      return "Unknown";
    case CreativeViewportPickStatus::InvalidGrid:
      return "InvalidGrid";
    case CreativeViewportPickStatus::InvalidViewport:
      return "InvalidViewport";
    case CreativeViewportPickStatus::NoCells:
      return "NoCells";
    case CreativeViewportPickStatus::OutOfViewport:
      return "OutOfViewport";
    case CreativeViewportPickStatus::OutOfGrid:
      return "OutOfGrid";
    case CreativeViewportPickStatus::Miss:
      return "Miss";
    case CreativeViewportPickStatus::Hit:
      return "Hit";
  }

  return "Unknown";
}

bool isValidCreativeViewportPickViewport(
    CreativeViewportPickViewport viewport) noexcept {
  return viewport.width > 0.0F && viewport.height > 0.0F;
}

CreativeGridCoord3 pointerToCreativeGridCoord(
    CreativeViewportPickViewport viewport,
    CreativeGridSize3 gridSize,
    float pointerX,
    float pointerY,
    std::int32_t z) noexcept {
  const float localX = (pointerX - viewport.x) / viewport.width;
  const float localY = (pointerY - viewport.y) / viewport.height;
  return CreativeGridCoord3{
      static_cast<std::int32_t>(localX * static_cast<float>(gridSize.width)),
      static_cast<std::int32_t>(localY * static_cast<float>(gridSize.height)),
      z,
  };
}

CreativeViewportPickReceipt pickCreativeViewportCell(
    const CreativeViewportPickRequest& request) {
  if (!isValidGridSize(request.gridSize)) {
    return makeReceipt(CreativeViewportPickStatus::InvalidGrid,
                       "invalid_grid");
  }
  if (!isValidCreativeViewportPickViewport(request.viewport)) {
    return makeReceipt(CreativeViewportPickStatus::InvalidViewport,
                       "invalid_viewport");
  }
  if (request.cells == nullptr || request.cellCount == 0U) {
    return makeReceipt(CreativeViewportPickStatus::NoCells, "no_cells");
  }
  if (!containsPointer(request.viewport, request.pointerX, request.pointerY)) {
    return makeReceipt(CreativeViewportPickStatus::OutOfViewport,
                       "out_of_viewport");
  }

  const CreativeGridCoord3 coord = pointerToCreativeGridCoord(
      request.viewport, request.gridSize, request.pointerX, request.pointerY,
      request.z);
  if (!isInsideGrid(coord, request.gridSize)) {
    CreativeViewportPickReceipt receipt =
        makeReceipt(CreativeViewportPickStatus::OutOfGrid, "out_of_grid");
    receipt.coord = coord;
    return receipt;
  }

  const CreativeGridIndex index = toGridIndex(coord, request.gridSize);
  const CreativeSpatialCell* matchedCell = nullptr;
  std::size_t matchedCellIndex = 0U;
  for (std::size_t cellIndex = 0U; cellIndex < request.cellCount; ++cellIndex) {
    const CreativeSpatialCell& cell = request.cells[cellIndex];
    if (cell.index == index && sameCoord(cell.coord, coord)) {
      matchedCell = &cell;
      matchedCellIndex = cellIndex;
    }
  }

  if (matchedCell == nullptr) {
    CreativeViewportPickReceipt receipt =
        makeReceipt(CreativeViewportPickStatus::Miss, "miss");
    receipt.coord = coord;
    receipt.index = index;
    return receipt;
  }

  CreativeViewportPickReceipt receipt =
      makeReceipt(CreativeViewportPickStatus::Hit, "hit");
  copyHitCell(receipt, *matchedCell, matchedCellIndex);
  if (receipt.target.value == kInvalidId) {
    receipt.message = matchedCell->objectId == kInvalidObjectId
                          ? "hit_target_invalid"
                          : "hit_target_out_of_range";
  }
  return receipt;
}

}  // namespace iggy3d::creative
