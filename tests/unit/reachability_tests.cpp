#include "core/grid/Reachability.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

// Row-major cell map: '.' = walkable, anything else = blocked.
iggy3d::ReachabilityGrid gridFrom(std::int32_t width, std::int32_t depth,
                                  std::string_view cells) {
  iggy3d::ReachabilityGrid grid;
  grid.width = width;
  grid.depth = depth;
  grid.walkable.assign(cells.size(), 0);
  for (std::size_t i = 0; i < cells.size(); ++i) {
    grid.walkable[i] = static_cast<std::uint8_t>(cells[i] == '.' ? 1 : 0);
  }
  return grid;
}

bool openGridIsOneRegion() {
  const iggy3d::ReachabilityGrid grid = gridFrom(3, 3, ".........");
  const std::vector<iggy3d::ReachabilityCoord> seeds = {{0, 0}};
  const iggy3d::ReachabilityReceipt r =
      iggy3d::floodFillReachability(grid, seeds);
  return expect(r.ok, "ok") && expect(r.walkableCellCount == 9U, "9 walkable") &&
         expect(r.reachedCellCount == 9U, "all reached") &&
         expect(r.strandedWalkableCount == 0U, "none stranded") &&
         expect(r.allWalkableReached, "one connected region");
}

bool wallStrandsAnIsland() {
  // A vertical wall at x=1 splits into two 3-cell columns.
  const iggy3d::ReachabilityGrid grid = gridFrom(3, 3, ".#..#..#.");
  const std::vector<iggy3d::ReachabilityCoord> seeds = {{0, 0}};
  const iggy3d::ReachabilityReceipt r =
      iggy3d::floodFillReachability(grid, seeds);
  return expect(r.walkableCellCount == 6U, "6 walkable") &&
         expect(r.reachedCellCount == 3U, "left column reached") &&
         expect(r.strandedWalkableCount == 3U, "right column stranded") &&
         expect(!r.allWalkableReached, "not one region (island present)");
}

bool seedOrderIsInvariant() {
  const iggy3d::ReachabilityGrid grid = gridFrom(3, 1, "...");
  const std::vector<iggy3d::ReachabilityCoord> ab = {{0, 0}, {2, 0}};
  const std::vector<iggy3d::ReachabilityCoord> ba = {{2, 0}, {0, 0}};
  const iggy3d::ReachabilityReceipt a = iggy3d::floodFillReachability(grid, ab);
  const iggy3d::ReachabilityReceipt b = iggy3d::floodFillReachability(grid, ba);
  return expect(a.reached == b.reached, "reordering seeds yields the same reached set") &&
         expect(a.reachedCellCount == 3U, "both reach all");
}

bool nonWalkableAndOffGridSeedsIgnored() {
  const iggy3d::ReachabilityGrid grid = gridFrom(3, 1, ".#.");
  const std::vector<iggy3d::ReachabilityCoord> seeds = {
      {1, 0},   // blocked cell
      {9, 9},   // off-grid
  };
  const iggy3d::ReachabilityReceipt r =
      iggy3d::floodFillReachability(grid, seeds);
  return expect(r.ok, "ok") &&
         expect(r.reachedCellCount == 0U, "invalid seeds reach nothing");
}

bool invalidGridIsRejected() {
  const iggy3d::ReachabilityGrid grid = gridFrom(0, 0, "");
  const iggy3d::ReachabilityReceipt r = iggy3d::floodFillReachability(grid, {});
  return expect(!r.ok, "empty grid rejected") &&
         expect(r.reasonCode == "reachability_grid_invalid", "reason code");
}

bool eightWayCrossesDiagonalGap() {
  // (0,0) and (1,1) touch only diagonally; (1,0) and (0,1) are blocked.
  const iggy3d::ReachabilityGrid grid = gridFrom(2, 2, ".##.");
  const std::vector<iggy3d::ReachabilityCoord> seeds = {{0, 0}};
  const iggy3d::ReachabilityReceipt four = iggy3d::floodFillReachability(
      grid, seeds, iggy3d::ReachabilityConnectivity::FourWay);
  const iggy3d::ReachabilityReceipt eight = iggy3d::floodFillReachability(
      grid, seeds, iggy3d::ReachabilityConnectivity::EightWay);
  return expect(four.reachedCellCount == 1U, "4-way stops at the diagonal gap") &&
         expect(eight.reachedCellCount == 2U, "8-way crosses the diagonal");
}

}  // namespace

int main() {
  const bool ok = openGridIsOneRegion() && wallStrandsAnIsland() &&
                  seedOrderIsInvariant() && nonWalkableAndOffGridSeedsIgnored() &&
                  invalidGridIsRejected() && eightWayCrossesDiagonalGap();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
