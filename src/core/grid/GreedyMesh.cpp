#include "core/grid/GreedyMesh.hpp"

#include <cstddef>

namespace iggy3d {

GreedyMeshReceipt greedyMeshGrid(const GreedyMeshGrid& grid) {
  GreedyMeshReceipt receipt;

  const std::size_t cellCount = static_cast<std::size_t>(grid.width) *
                                static_cast<std::size_t>(grid.depth);
  if (grid.width <= 0 || grid.depth <= 0 || grid.keys.size() != cellCount) {
    receipt.reasonCode = "greedy_mesh_grid_invalid";
    return receipt;
  }

  const auto index = [&](std::int32_t x, std::int32_t z) -> std::size_t {
    return static_cast<std::size_t>(z) * static_cast<std::size_t>(grid.width) +
           static_cast<std::size_t>(x);
  };

  std::vector<std::uint8_t> consumed(cellCount, 0);
  for (const std::uint32_t key : grid.keys) {
    if (key != 0) {
      ++receipt.filledCellCount;
    }
  }

  for (std::int32_t z = 0; z < grid.depth; ++z) {
    for (std::int32_t x = 0; x < grid.width; ++x) {
      const std::uint32_t key = grid.keys[index(x, z)];
      if (key == 0 || consumed[index(x, z)] != 0) {
        continue;
      }

      // Extend the run to the right while the key matches and cells are free.
      std::int32_t width = 1;
      while (x + width < grid.width && grid.keys[index(x + width, z)] == key &&
             consumed[index(x + width, z)] == 0) {
        ++width;
      }

      // Extend downward while every cell of the full width-run matches and is free.
      std::int32_t depth = 1;
      bool grow = true;
      while (grow && z + depth < grid.depth) {
        for (std::int32_t dx = 0; dx < width; ++dx) {
          const std::size_t below = index(x + dx, z + depth);
          if (grid.keys[below] != key || consumed[below] != 0) {
            grow = false;
            break;
          }
        }
        if (grow) {
          ++depth;
        }
      }

      for (std::int32_t dz = 0; dz < depth; ++dz) {
        for (std::int32_t dx = 0; dx < width; ++dx) {
          consumed[index(x + dx, z + dz)] = 1;
        }
      }
      receipt.quads.push_back({x, z, width, depth, key});
    }
  }

  receipt.quadCount = static_cast<std::uint32_t>(receipt.quads.size());
  receipt.ok = true;
  receipt.reasonCode = "greedy_mesh_ok";
  return receipt;
}

}  // namespace iggy3d
