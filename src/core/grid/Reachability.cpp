#include "core/grid/Reachability.hpp"

#include <cstddef>

namespace iggy3d {

namespace {

bool inBounds(const ReachabilityGrid& grid, std::int32_t x, std::int32_t z) {
  return x >= 0 && z >= 0 && x < grid.width && z < grid.depth;
}

std::size_t indexOf(const ReachabilityGrid& grid, std::int32_t x, std::int32_t z) {
  return static_cast<std::size_t>(z) * static_cast<std::size_t>(grid.width) +
         static_cast<std::size_t>(x);
}

bool isWalkable(const ReachabilityGrid& grid, std::int32_t x, std::int32_t z) {
  return inBounds(grid, x, z) && grid.walkable[indexOf(grid, x, z)] != 0;
}

// First four are orthogonal (4-connected); all eight are 8-connected.
constexpr std::int32_t kOffsets[8][2] = {
    {1, 0}, {-1, 0}, {0, 1}, {0, -1},
    {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

}  // namespace

ReachabilityReceipt floodFillReachability(
    const ReachabilityGrid& grid,
    std::span<const ReachabilityCoord> seeds,
    ReachabilityConnectivity connectivity) {
  ReachabilityReceipt receipt;

  const std::size_t cellCount = static_cast<std::size_t>(grid.width) *
                                static_cast<std::size_t>(grid.depth);
  if (grid.width <= 0 || grid.depth <= 0 || grid.walkable.size() != cellCount) {
    receipt.reasonCode = "reachability_grid_invalid";
    return receipt;
  }

  receipt.reached.assign(cellCount, 0);
  for (std::uint8_t cell : grid.walkable) {
    if (cell != 0) {
      ++receipt.walkableCellCount;
    }
  }

  const std::int32_t neighbourCount =
      connectivity == ReachabilityConnectivity::EightWay ? 8 : 4;

  // Deterministic frontier: a vector consumed via a read cursor (FIFO), never a hashed set.
  std::vector<ReachabilityCoord> frontier;
  const auto enqueue = [&](std::int32_t x, std::int32_t z) {
    if (!isWalkable(grid, x, z)) {
      return;
    }
    std::uint8_t& mark = receipt.reached[indexOf(grid, x, z)];
    if (mark == 0) {
      mark = 1;
      frontier.push_back({x, z});
    }
  };

  for (const ReachabilityCoord& seed : seeds) {
    enqueue(seed.x, seed.z);
  }

  std::size_t cursor = 0;
  while (cursor < frontier.size()) {
    const ReachabilityCoord cell = frontier[cursor];
    ++cursor;
    for (std::int32_t n = 0; n < neighbourCount; ++n) {
      enqueue(cell.x + kOffsets[n][0], cell.z + kOffsets[n][1]);
    }
  }

  receipt.reachedCellCount = static_cast<std::uint32_t>(frontier.size());
  receipt.strandedWalkableCount =
      receipt.walkableCellCount - receipt.reachedCellCount;
  receipt.allWalkableReached =
      receipt.strandedWalkableCount == 0 && receipt.walkableCellCount > 0;
  receipt.ok = true;
  receipt.reasonCode = "reachability_ok";
  return receipt;
}

}  // namespace iggy3d
