#include "app/iggy3d/creative/ViewportPick.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeViewportPickViewport viewport() {
  return {0.0F, 0.0F, 400.0F, 400.0F};
}

cr::CreativeGridSize3 gridSize() {
  return {4, 4, 2};
}

cr::CreativeSpatialCell makeCell(cr::CreativeGridCoord3 coord,
                                 cr::CreativeObjectId objectId,
                                 cr::CreativeObjectKind objectKind,
                                 cr::CreativeSpatialOccupancyKind occupancy) {
  cr::CreativeSpatialCell cell;
  cell.coord = coord;
  cell.index = cr::toGridIndex(coord, gridSize());
  cell.objectId = objectId;
  cell.objectKind = objectKind;
  cell.occupancyKind = occupancy;
  return cell;
}

cr::CreativeViewportPickRequest requestWithCells(
    const std::vector<cr::CreativeSpatialCell>& cells,
    float pointerX = 150.0F,
    float pointerY = 250.0F,
    std::int32_t z = 1) {
  cr::CreativeViewportPickRequest request;
  request.viewport = viewport();
  request.gridSize = gridSize();
  request.cells = cells.data();
  request.cellCount = cells.size();
  request.pointerX = pointerX;
  request.pointerY = pointerY;
  request.z = z;
  return request;
}

bool invalidGridReturnsInvalidGrid() {
  const std::vector<cr::CreativeSpatialCell> cells{
      makeCell({1, 2, 1}, 9, cr::CreativeObjectKind::Room,
               cr::CreativeSpatialOccupancyKind::Structural),
  };
  cr::CreativeViewportPickRequest request = requestWithCells(cells);
  request.gridSize = {0, 4, 2};

  const cr::CreativeViewportPickReceipt receipt =
      cr::pickCreativeViewportCell(request);

  return expect(receipt.requested, "invalid grid requested") &&
         expect(!receipt.hit, "invalid grid no hit") &&
         expect(receipt.status == cr::CreativeViewportPickStatus::InvalidGrid,
                "invalid grid status") &&
         expect(receipt.message == "invalid_grid", "invalid grid message");
}

bool invalidViewportReturnsInvalidViewport() {
  const std::vector<cr::CreativeSpatialCell> cells{
      makeCell({1, 2, 1}, 9, cr::CreativeObjectKind::Room,
               cr::CreativeSpatialOccupancyKind::Structural),
  };
  cr::CreativeViewportPickRequest request = requestWithCells(cells);
  request.viewport.width = 0.0F;

  const cr::CreativeViewportPickReceipt receipt =
      cr::pickCreativeViewportCell(request);

  return expect(receipt.status ==
                    cr::CreativeViewportPickStatus::InvalidViewport,
                "invalid viewport status") &&
         expect(receipt.message == "invalid_viewport",
                "invalid viewport message");
}

bool noCellsReturnsNoCells() {
  cr::CreativeViewportPickRequest request;
  request.viewport = viewport();
  request.gridSize = gridSize();
  request.pointerX = 1.0F;
  request.pointerY = 1.0F;

  const cr::CreativeViewportPickReceipt receipt =
      cr::pickCreativeViewportCell(request);

  return expect(receipt.status == cr::CreativeViewportPickStatus::NoCells,
                "no cells status") &&
         expect(receipt.message == "no_cells", "no cells message");
}

bool pointerViewportEdgesAreLeftTopInclusiveRightBottomExclusive() {
  const std::vector<cr::CreativeSpatialCell> cells{
      makeCell({0, 0, 0}, 1, cr::CreativeObjectKind::Room,
               cr::CreativeSpatialOccupancyKind::Structural),
  };

  cr::CreativeViewportPickRequest leftTop = requestWithCells(cells, 0.0F,
                                                             0.0F, 0);
  const cr::CreativeViewportPickReceipt leftTopReceipt =
      cr::pickCreativeViewportCell(leftTop);

  cr::CreativeViewportPickRequest right = requestWithCells(cells, 400.0F,
                                                           0.0F, 0);
  const cr::CreativeViewportPickReceipt rightReceipt =
      cr::pickCreativeViewportCell(right);

  cr::CreativeViewportPickRequest bottom = requestWithCells(cells, 0.0F,
                                                            400.0F, 0);
  const cr::CreativeViewportPickReceipt bottomReceipt =
      cr::pickCreativeViewportCell(bottom);

  return expect(leftTopReceipt.status == cr::CreativeViewportPickStatus::Hit,
                "left top inclusive") &&
         expect(rightReceipt.status ==
                    cr::CreativeViewportPickStatus::OutOfViewport,
                "right exclusive") &&
         expect(bottomReceipt.status ==
                    cr::CreativeViewportPickStatus::OutOfViewport,
                "bottom exclusive");
}

bool pointerMapsToExpectedGridCoord() {
  const cr::CreativeGridCoord3 coord = cr::pointerToCreativeGridCoord(
      viewport(), gridSize(), 150.0F, 250.0F, 1);

  return expect(coord.x == 1, "mapped x") &&
         expect(coord.y == 2, "mapped y") &&
         expect(coord.z == 1, "mapped z");
}

bool zOutsideGridReturnsOutOfGrid() {
  const std::vector<cr::CreativeSpatialCell> cells{
      makeCell({1, 2, 1}, 9, cr::CreativeObjectKind::Room,
               cr::CreativeSpatialOccupancyKind::Structural),
  };
  const cr::CreativeViewportPickReceipt receipt =
      cr::pickCreativeViewportCell(requestWithCells(cells, 150.0F, 250.0F, 2));

  return expect(receipt.status == cr::CreativeViewportPickStatus::OutOfGrid,
                "out of grid status") &&
         expect(receipt.coord.x == 1, "out of grid x") &&
         expect(receipt.coord.y == 2, "out of grid y") &&
         expect(receipt.coord.z == 2, "out of grid z");
}

bool missWhenNoCellMatchesComputedCoord() {
  const std::vector<cr::CreativeSpatialCell> cells{
      makeCell({0, 0, 0}, 9, cr::CreativeObjectKind::Room,
               cr::CreativeSpatialOccupancyKind::Structural),
  };
  const cr::CreativeViewportPickReceipt receipt =
      cr::pickCreativeViewportCell(requestWithCells(cells));

  return expect(receipt.status == cr::CreativeViewportPickStatus::Miss,
                "miss status") &&
         expect(!receipt.hit, "miss no hit") &&
         expect(receipt.coord.x == 1, "miss coord x") &&
         expect(receipt.coord.y == 2, "miss coord y") &&
         expect(receipt.index == 25U, "miss index");
}

bool hitCopiesCellData() {
  const std::vector<cr::CreativeSpatialCell> cells{
      makeCell({1, 2, 1}, 42, cr::CreativeObjectKind::Door,
               cr::CreativeSpatialOccupancyKind::Navigation),
  };
  const cr::CreativeViewportPickReceipt receipt =
      cr::pickCreativeViewportCell(requestWithCells(cells));

  return expect(receipt.status == cr::CreativeViewportPickStatus::Hit,
                "hit status") &&
         expect(receipt.hit, "hit flag") &&
         expect(receipt.objectId == 42U, "hit object id") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Door,
                "hit object kind") &&
         expect(receipt.occupancyKind ==
                    cr::CreativeSpatialOccupancyKind::Navigation,
                "hit occupancy") &&
         expect(receipt.coord.x == 1, "hit coord x") &&
         expect(receipt.coord.y == 2, "hit coord y") &&
         expect(receipt.coord.z == 1, "hit coord z") &&
         expect(receipt.index == 25U, "hit index") &&
         expect(receipt.cellIndex == 0U, "hit cell index") &&
         expect(receipt.message == "hit", "hit message");
}

bool targetUsesObjectIdWhenInRange() {
  const std::vector<cr::CreativeSpatialCell> cells{
      makeCell({1, 2, 1}, 42, cr::CreativeObjectKind::Door,
               cr::CreativeSpatialOccupancyKind::Navigation),
  };
  const cr::CreativeViewportPickReceipt receipt =
      cr::pickCreativeViewportCell(requestWithCells(cells));

  return expect(receipt.target.value == 42U, "target object id");
}

bool objectIdZeroLeavesTargetInvalid() {
  const std::vector<cr::CreativeSpatialCell> cells{
      makeCell({1, 2, 1}, cr::kInvalidObjectId,
               cr::CreativeObjectKind::Door,
               cr::CreativeSpatialOccupancyKind::Navigation),
  };
  const cr::CreativeViewportPickReceipt receipt =
      cr::pickCreativeViewportCell(requestWithCells(cells));

  return expect(receipt.status == cr::CreativeViewportPickStatus::Hit,
                "zero id still hit") &&
         expect(receipt.objectId == cr::kInvalidObjectId,
                "zero id copied") &&
         expect(receipt.target.value == cr::kInvalidId,
                "zero id target invalid") &&
         expect(receipt.message == "hit_target_invalid",
                "zero id message");
}

bool objectIdOutsideTargetRangeLeavesTargetInvalid() {
  const cr::CreativeObjectId tooLarge =
      static_cast<cr::CreativeObjectId>(std::numeric_limits<cr::Id>::max()) +
      1U;
  const std::vector<cr::CreativeSpatialCell> cells{
      makeCell({1, 2, 1}, tooLarge, cr::CreativeObjectKind::Door,
               cr::CreativeSpatialOccupancyKind::Navigation),
  };
  const cr::CreativeViewportPickReceipt receipt =
      cr::pickCreativeViewportCell(requestWithCells(cells));

  return expect(receipt.status == cr::CreativeViewportPickStatus::Hit,
                "large id still hit") &&
         expect(receipt.objectId == tooLarge, "large id copied") &&
         expect(receipt.target.value == cr::kInvalidId,
                "large id target invalid") &&
         expect(receipt.message == "hit_target_out_of_range",
                "large id message");
}

bool laterMatchingCellWins() {
  const std::vector<cr::CreativeSpatialCell> cells{
      makeCell({1, 2, 1}, 11, cr::CreativeObjectKind::Room,
               cr::CreativeSpatialOccupancyKind::Structural),
      makeCell({1, 2, 1}, 12, cr::CreativeObjectKind::Door,
               cr::CreativeSpatialOccupancyKind::Navigation),
  };
  const cr::CreativeViewportPickReceipt receipt =
      cr::pickCreativeViewportCell(requestWithCells(cells));

  return expect(receipt.objectId == 12U, "later object id") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Door,
                "later object kind") &&
         expect(receipt.cellIndex == 1U, "later cell index") &&
         expect(receipt.target.value == 12U, "later target");
}

}  // namespace

int main() {
  const bool ok =
      invalidGridReturnsInvalidGrid() &&
      invalidViewportReturnsInvalidViewport() &&
      noCellsReturnsNoCells() &&
      pointerViewportEdgesAreLeftTopInclusiveRightBottomExclusive() &&
      pointerMapsToExpectedGridCoord() &&
      zOutsideGridReturnsOutOfGrid() &&
      missWhenNoCellMatchesComputedCoord() &&
      hitCopiesCellData() &&
      targetUsesObjectIdWhenInRange() &&
      objectIdZeroLeavesTargetInvalid() &&
      objectIdOutsideTargetRangeLeavesTargetInvalid() &&
      laterMatchingCellWins();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
