#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace iggy3d {

// A dense rectangular grid of walkable/blocked cells. The CALLER fills it (e.g. from an occupancy
// projection); this kernel only FLOODS it. Row-major: index = z * width + x.
struct ReachabilityGrid {
  std::int32_t width = 0;
  std::int32_t depth = 0;
  std::vector<std::uint8_t> walkable;  // size width*depth; 1 = walkable, 0 = blocked
};

struct ReachabilityCoord {
  std::int32_t x = 0;
  std::int32_t z = 0;
};

enum class ReachabilityConnectivity : std::uint8_t { FourWay, EightWay };

// The report: which walkable cells are reachable from the seeds, and whether any walkable cell was
// stranded (an unreachable island -- the thing bake / nav / patrol / token-gen validation flags).
struct ReachabilityReceipt {
  bool ok = false;
  std::string reasonCode;
  std::vector<std::uint8_t> reached;         // size width*depth; 1 = reached from a seed
  std::uint32_t walkableCellCount = 0;
  std::uint32_t reachedCellCount = 0;
  std::uint32_t strandedWalkableCount = 0;   // walkable but NOT reached = islands
  bool allWalkableReached = false;           // no stranded cells: one connected region
};

// Pure, deterministic 4/8-connected flood from `seeds` over the walkable cells. Off-grid or
// non-walkable seeds are ignored (never fabricate reachability). The frontier is an index-keyed
// vector used as a FIFO -- NEVER an unordered_set, whose hash-iteration order would break
// determinism. Reordering the seeds yields the same reached set. Reused four ways: token-gen
// connectivity, navmesh island detection, patrol-loop validation, parkour reachability.
ReachabilityReceipt floodFillReachability(
    const ReachabilityGrid& grid,
    std::span<const ReachabilityCoord> seeds,
    ReachabilityConnectivity connectivity = ReachabilityConnectivity::FourWay);

}  // namespace iggy3d
