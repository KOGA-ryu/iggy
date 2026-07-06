#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace iggy3d {

// A dense grid of cell KEYS. key 0 = empty (no quad); a non-zero key merges only with the SAME key
// (same material / role / height layer). Row-major: index = z * width + x. The caller keys the
// cells (e.g. floor role + material id) before meshing.
struct GreedyMeshGrid {
  std::int32_t width = 0;
  std::int32_t depth = 0;
  std::vector<std::uint32_t> keys;  // size width*depth; 0 = empty
};

// One merged rectangle: the cells [x, x+width) x [z, z+depth), all sharing `key`.
struct GreedyQuad {
  std::int32_t x = 0;
  std::int32_t z = 0;
  std::int32_t width = 0;
  std::int32_t depth = 0;
  std::uint32_t key = 0;
};

struct GreedyMeshReceipt {
  bool ok = false;
  std::string reasonCode;
  std::vector<GreedyQuad> quads;
  std::uint32_t filledCellCount = 0;  // non-zero cells in the grid
  std::uint32_t quadCount = 0;        // == quads.size()
};

// Pure, deterministic greedy meshing of one keyed 2D grid into a minimal set of axis-aligned
// rectangles, each covering only same-key cells. Row-major scan, width-then-height extension (the
// Minecraft-derived 2D pass). Every filled cell is covered by exactly one quad; quads never
// overlap; empty (key 0) cells are never covered. Collapses a wall of N same-key tiles into
// O(surfaces) quads -- the bake draw-call / triangle win for every box/surface object.
GreedyMeshReceipt greedyMeshGrid(const GreedyMeshGrid& grid);

}  // namespace iggy3d
