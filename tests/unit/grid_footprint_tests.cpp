#include "core/grid/GridFootprint.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool sameFootprint(const iggy3d::GridFootprint& footprint,
                   std::int32_t minX,
                   std::int32_t minZ,
                   std::int32_t maxX,
                   std::int32_t maxZ) {
  return footprint.minCellX == minX && footprint.minCellZ == minZ &&
         footprint.maxCellXExclusive == maxX &&
         footprint.maxCellZExclusive == maxZ;
}

bool containingBoundsUseHalfOpenFloorCeilCells() {
  const iggy3d::GridFootprintResult result =
      iggy3d::gridFootprintForContainingBounds(0.25F,
                                               1.0F,
                                               2.0F,
                                               3.25F,
                                               1.0F);
  return expect(result.ok, "containing ok") &&
         expect(result.status == iggy3d::GridFootprintStatus::Ok,
                "containing status") &&
         expect(result.reasonCode == "grid_footprint_ok",
                "containing reason") &&
         expect(sameFootprint(result.footprint, 0, 1, 2, 4),
                "containing footprint") &&
         expect(result.footprint.width() == 2, "containing width") &&
         expect(result.footprint.depth() == 3, "containing depth");
}

bool containingBoundsHandleNegativeAndExactBoundaries() {
  const iggy3d::GridFootprintResult result =
      iggy3d::gridFootprintForContainingBounds(-2.0F,
                                               -0.01F,
                                               0.0F,
                                               1.0F,
                                               1.0F);
  return expect(result.ok, "negative containing ok") &&
         expect(sameFootprint(result.footprint, -2, -1, 0, 1),
                "negative containing footprint");
}

bool alignedBoundsRoundNearGridLines() {
  const iggy3d::GridFootprintResult result =
      iggy3d::gridFootprintForAlignedBounds(0.99995F,
                                            0.0F,
                                            1.99995F,
                                            1.0F,
                                            1.0F);
  return expect(result.ok, "aligned near boundary ok") &&
         expect(sameFootprint(result.footprint, 1, 0, 2, 1),
                "aligned near boundary footprint");
}

bool alignedBoundsRejectMisalignedValues() {
  const iggy3d::GridFootprintResult result =
      iggy3d::gridFootprintForAlignedBounds(0.25F,
                                            0.0F,
                                            1.25F,
                                            1.0F,
                                            1.0F);
  return expect(!result.ok, "misaligned rejected") &&
         expect(result.status == iggy3d::GridFootprintStatus::Misaligned,
                "misaligned status") &&
         expect(result.reasonCode == "grid_footprint_misaligned",
                "misaligned reason");
}

bool pointCellsUseContainingSemantics() {
  const iggy3d::GridCellCoordResult zero =
      iggy3d::gridCellForPoint(0.0F, 0.0F, 1.0F);
  const iggy3d::GridCellCoordResult negative =
      iggy3d::gridCellForPoint(-0.01F, -1.0F, 1.0F);
  return expect(zero.ok, "zero point ok") &&
         expect(zero.cell.x == 0 && zero.cell.z == 0, "zero point cell") &&
         expect(negative.ok, "negative point ok") &&
         expect(negative.cell.x == -1 && negative.cell.z == -1,
                "negative point cell");
}

bool invalidCellSizeAndNonFiniteInputsAreDeterministic() {
  const iggy3d::GridFootprintResult badCell =
      iggy3d::gridFootprintForContainingBounds(0.0F,
                                               0.0F,
                                               1.0F,
                                               1.0F,
                                               0.0F);
  const iggy3d::GridFootprintResult nonFinite =
      iggy3d::gridFootprintForContainingBounds(
          0.0F,
          0.0F,
          std::numeric_limits<float>::infinity(),
          1.0F,
          1.0F);
  return expect(!badCell.ok, "bad cell size rejected") &&
         expect(badCell.status ==
                    iggy3d::GridFootprintStatus::InvalidCellSize,
                "bad cell size status") &&
         expect(badCell.reasonCode == "grid_footprint_invalid_cell_size",
                "bad cell size reason") &&
         expect(!nonFinite.ok, "non-finite rejected") &&
         expect(nonFinite.status ==
                    iggy3d::GridFootprintStatus::NonFiniteInput,
                "non-finite status") &&
         expect(nonFinite.reasonCode == "grid_footprint_non_finite_input",
                "non-finite reason");
}

bool emptyAndOutOfRangeFootprintsAreDeterministic() {
  const iggy3d::GridFootprintResult empty =
      iggy3d::gridFootprintForContainingBounds(0.0F,
                                               0.0F,
                                               0.0F,
                                               1.0F,
                                               1.0F);
  const iggy3d::GridFootprintResult outOfRange =
      iggy3d::gridFootprintForContainingBounds(
          -3.0E10F,
          0.0F,
          -2.0E10F,
          1.0F,
          1.0F);
  return expect(!empty.ok, "empty rejected") &&
         expect(empty.status == iggy3d::GridFootprintStatus::EmptyFootprint,
                "empty status") &&
         expect(empty.reasonCode == "grid_footprint_empty",
                "empty reason") &&
         expect(!outOfRange.ok, "out-of-range rejected") &&
         expect(outOfRange.status == iggy3d::GridFootprintStatus::OutOfRange,
                "out-of-range status") &&
         expect(outOfRange.reasonCode == "grid_footprint_out_of_range",
                "out-of-range reason");
}

bool footprintDimensionsDoNotOverflowInt32Range() {
  const iggy3d::GridFootprint footprint{
      std::numeric_limits<std::int32_t>::min(),
      -2,
      std::numeric_limits<std::int32_t>::max(),
      3,
  };
  return expect(footprint.width() == 4294967295LL,
                "wide footprint width") &&
         expect(footprint.depth() == 5LL, "wide footprint depth");
}

bool toStringValuesAreStable() {
  return expect(iggy3d::toString(iggy3d::GridFootprintStatus::Unknown) ==
                    "unknown",
                "unknown string") &&
         expect(iggy3d::toString(iggy3d::GridFootprintStatus::Ok) == "ok",
                "ok string") &&
         expect(iggy3d::toString(
                    iggy3d::GridFootprintStatus::InvalidCellSize) ==
                    "invalid_cell_size",
                "invalid cell string") &&
         expect(iggy3d::toString(
                    iggy3d::GridFootprintStatus::NonFiniteInput) ==
                    "non_finite_input",
                "non-finite string") &&
         expect(iggy3d::toString(
                    iggy3d::GridFootprintStatus::OutOfRange) ==
                    "out_of_range",
                "out-of-range string") &&
         expect(iggy3d::toString(
                    iggy3d::GridFootprintStatus::EmptyFootprint) ==
                    "empty_footprint",
                "empty string") &&
         expect(iggy3d::toString(
                    iggy3d::GridFootprintStatus::Misaligned) ==
                    "misaligned",
                "misaligned string");
}

}  // namespace

int main() {
  const bool ok = containingBoundsUseHalfOpenFloorCeilCells() &&
                  containingBoundsHandleNegativeAndExactBoundaries() &&
                  alignedBoundsRoundNearGridLines() &&
                  alignedBoundsRejectMisalignedValues() &&
                  pointCellsUseContainingSemantics() &&
                  invalidCellSizeAndNonFiniteInputsAreDeterministic() &&
                  emptyAndOutOfRangeFootprintsAreDeterministic() &&
                  footprintDimensionsDoNotOverflowInt32Range() &&
                  toStringValuesAreStable();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
