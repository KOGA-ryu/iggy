#include "core/grid/GreedyMesh.hpp"

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

// Row-major key map: '.' = empty (0); '1'..'9' = that key value.
iggy3d::GreedyMeshGrid gridFrom(std::int32_t width, std::int32_t depth,
                                std::string_view cells) {
  iggy3d::GreedyMeshGrid grid;
  grid.width = width;
  grid.depth = depth;
  grid.keys.assign(cells.size(), 0);
  for (std::size_t i = 0; i < cells.size(); ++i) {
    grid.keys[i] = cells[i] == '.'
                       ? 0U
                       : static_cast<std::uint32_t>(cells[i] - '0');
  }
  return grid;
}

// Every filled cell covered by exactly one quad, empty cells by none, every quad same-key + in
// bounds. This is the meshing invariant that must hold for any grid.
bool coverageIsExact(const iggy3d::GreedyMeshGrid& grid,
                     const iggy3d::GreedyMeshReceipt& r) {
  const std::size_t cells = static_cast<std::size_t>(grid.width) *
                            static_cast<std::size_t>(grid.depth);
  std::vector<int> cover(cells, 0);
  for (const iggy3d::GreedyQuad& q : r.quads) {
    for (std::int32_t dz = 0; dz < q.depth; ++dz) {
      for (std::int32_t dx = 0; dx < q.width; ++dx) {
        const std::int32_t x = q.x + dx;
        const std::int32_t z = q.z + dz;
        if (x < 0 || z < 0 || x >= grid.width || z >= grid.depth) {
          return false;  // out of bounds
        }
        const std::size_t i =
            static_cast<std::size_t>(z) * static_cast<std::size_t>(grid.width) +
            static_cast<std::size_t>(x);
        if (grid.keys[i] != q.key) {
          return false;  // quad covers a wrong-key cell
        }
        ++cover[i];
      }
    }
  }
  for (std::size_t i = 0; i < cells; ++i) {
    const bool filled = grid.keys[i] != 0;
    if (filled && cover[i] != 1) {
      return false;  // filled cell not covered exactly once
    }
    if (!filled && cover[i] != 0) {
      return false;  // empty cell covered
    }
  }
  return true;
}

bool solidGridIsOneQuad() {
  const iggy3d::GreedyMeshGrid grid = gridFrom(3, 3, "111111111");
  const iggy3d::GreedyMeshReceipt r = iggy3d::greedyMeshGrid(grid);
  return expect(r.ok, "ok") && expect(r.filledCellCount == 9U, "9 filled") &&
         expect(r.quadCount == 1U, "collapses to one quad") &&
         expect(coverageIsExact(grid, r), "coverage exact");
}

bool lShapeSplitsIntoTwoQuads() {
  // 2 wide, 3 deep: "11" / "11" / "1." -> a 2x2 block plus a single tail cell.
  const iggy3d::GreedyMeshGrid grid = gridFrom(2, 3, "11111.");
  const iggy3d::GreedyMeshReceipt r = iggy3d::greedyMeshGrid(grid);
  return expect(r.filledCellCount == 5U, "5 filled") &&
         expect(r.quadCount == 2U, "L-shape is two quads") &&
         expect(coverageIsExact(grid, r), "coverage exact");
}

bool differentKeysDoNotMerge() {
  const iggy3d::GreedyMeshGrid grid = gridFrom(2, 1, "12");
  const iggy3d::GreedyMeshReceipt r = iggy3d::greedyMeshGrid(grid);
  return expect(r.quadCount == 2U, "distinct keys stay distinct quads") &&
         expect(coverageIsExact(grid, r), "coverage exact");
}

bool holeIsExcluded() {
  // A ring with an empty centre.
  const iggy3d::GreedyMeshGrid grid = gridFrom(3, 3, "111101111");
  const iggy3d::GreedyMeshReceipt r = iggy3d::greedyMeshGrid(grid);
  return expect(r.filledCellCount == 8U, "8 filled (hole excluded)") &&
         expect(coverageIsExact(grid, r), "coverage exact, hole uncovered");
}

bool emptyGridIsZeroQuads() {
  const iggy3d::GreedyMeshGrid grid = gridFrom(2, 2, "....");
  const iggy3d::GreedyMeshReceipt r = iggy3d::greedyMeshGrid(grid);
  return expect(r.ok, "ok") && expect(r.quadCount == 0U, "no quads") &&
         expect(r.filledCellCount == 0U, "no filled cells");
}

bool invalidGridIsRejected() {
  iggy3d::GreedyMeshGrid grid;
  grid.width = 0;
  grid.depth = 0;
  const iggy3d::GreedyMeshReceipt r = iggy3d::greedyMeshGrid(grid);
  return expect(!r.ok, "empty grid rejected") &&
         expect(r.reasonCode == "greedy_mesh_grid_invalid", "reason code");
}

}  // namespace

int main() {
  const bool ok = solidGridIsOneQuad() && lShapeSplitsIntoTwoQuads() &&
                  differentKeysDoNotMerge() && holeIsExcluded() &&
                  emptyGridIsZeroQuads() && invalidGridIsRejected();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
