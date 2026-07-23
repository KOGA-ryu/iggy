#include "app/iggy3d/creative/adapters/RoomBakeGreedyFloors.hpp"

#include "core/grid/GridFootprint.hpp"
#include "core/grid/GreedyMesh.hpp"

#include <algorithm>
#include <iterator>
#include <limits>
#include <utility>

namespace iggy3d::creative {
namespace {

struct GreedyFloorCandidate {
  const RoomBakeGreedyFloorInput* input = nullptr;
  GridFootprint footprint;
};

struct GreedyFloorGroup {
  float minY = 0.0F;
  float maxY = 0.0F;
  std::string meshId;
  std::string materialId;
  std::string semanticRole;
  std::string role;
  std::vector<GreedyFloorCandidate> candidates;
};

[[nodiscard]] bool greedyFloorFootprintForBounds(
    const RoomBakeGreedyFloorInput& input,
    float cellSizeMeters,
    GridFootprint& out) noexcept {
  const GridFootprintResult result = gridFootprintForAlignedBounds(input.min.x,
                                                                   input.min.z,
                                                                   input.max.x,
                                                                   input.max.z,
                                                                   cellSizeMeters);
  if (!result.ok) {
    return false;
  }
  out = result.footprint;
  return true;
}

[[nodiscard]] RoomStaticMeshAsset meshForInput(
    const RoomBakeGreedyFloorInput& input,
    const RoomBakeGreedyFloorPolicy& policy) {
  RoomStaticMeshAsset mesh;
  mesh.id = input.singleMeshId;
  mesh.meshId = policy.meshId;
  mesh.materialId = policy.materialId;
  mesh.semanticRole = policy.semanticRole;
  mesh.role = policy.role;
  mesh.positionMeters = input.center;
  mesh.sizeMeters = input.size;
  return mesh;
}

[[nodiscard]] RoomStaticMeshAsset meshForBounds(
    std::string id,
    Vec3 center,
    Vec3 size,
    const RoomBakeGreedyFloorPolicy& policy) {
  RoomStaticMeshAsset mesh;
  mesh.id = std::move(id);
  mesh.meshId = policy.meshId;
  mesh.materialId = policy.materialId;
  mesh.semanticRole = policy.semanticRole;
  mesh.role = policy.role;
  mesh.positionMeters = center;
  mesh.sizeMeters = size;
  return mesh;
}

[[nodiscard]] RoomBakeGreedyFloorSource sourceForInput(
    const RoomBakeGreedyFloorInput& input) {
  return {input.objectId,
          input.documentIndex,
          input.min,
          input.max,
          input.center,
          input.size};
}

void appendGreedyFloorSource(
    std::vector<const RoomBakeGreedyFloorInput*>& sources,
    const RoomBakeGreedyFloorInput* input) {
  if (input == nullptr) {
    return;
  }
  if (std::find(sources.begin(), sources.end(), input) == sources.end()) {
    sources.push_back(input);
  }
}

[[nodiscard]] std::string greedyFloorMeshId(
    const std::vector<const RoomBakeGreedyFloorInput*>& sources,
    std::size_t meshIndex) {
  if (sources.size() == 1U && sources.front() != nullptr) {
    return sources.front()->singleMeshId;
  }

  CreativeObjectId firstId{kInvalidObjectId};
  if (!sources.empty() && sources.front() != nullptr) {
    firstId = sources.front()->objectId;
  }
  return "creative_floor_greedy_" + std::to_string(firstId) + "_" +
         std::to_string(meshIndex);
}

[[nodiscard]] RoomBakeGreedyFloorMeshPlan fallbackFloorMesh(
    const RoomBakeGreedyFloorInput& input,
    const RoomBakeGreedyFloorPolicy& policy) {
  RoomBakeGreedyFloorMeshPlan plan;
  plan.sources = {sourceForInput(input)};
  plan.firstDocumentIndex = input.documentIndex;
  plan.mesh = meshForInput(input, policy);
  return plan;
}

[[nodiscard]] std::vector<RoomBakeGreedyFloorMeshPlan> fallbackFloorMeshes(
    const GreedyFloorGroup& group,
    const RoomBakeGreedyFloorPolicy& policy) {
  std::vector<RoomBakeGreedyFloorMeshPlan> meshes;
  meshes.reserve(group.candidates.size());
  for (const GreedyFloorCandidate& candidate : group.candidates) {
    if (candidate.input != nullptr) {
      meshes.push_back(fallbackFloorMesh(*candidate.input, policy));
    }
  }
  return meshes;
}

[[nodiscard]] std::vector<RoomBakeGreedyFloorMeshPlan>
buildGreedyFloorMeshesForGroup(const GreedyFloorGroup& group,
                               const RoomBakeGreedyFloorPolicy& policy) {
  if (group.candidates.empty()) {
    return {};
  }

  std::int32_t minCellX = group.candidates.front().footprint.minCellX;
  std::int32_t minCellZ = group.candidates.front().footprint.minCellZ;
  std::int32_t maxCellX = group.candidates.front().footprint.maxCellXExclusive;
  std::int32_t maxCellZ = group.candidates.front().footprint.maxCellZExclusive;
  for (const GreedyFloorCandidate& candidate : group.candidates) {
    minCellX = std::min(minCellX, candidate.footprint.minCellX);
    minCellZ = std::min(minCellZ, candidate.footprint.minCellZ);
    maxCellX = std::max(maxCellX, candidate.footprint.maxCellXExclusive);
    maxCellZ = std::max(maxCellZ, candidate.footprint.maxCellZExclusive);
  }

  const std::int64_t width64 =
      static_cast<std::int64_t>(maxCellX) - static_cast<std::int64_t>(minCellX);
  const std::int64_t depth64 =
      static_cast<std::int64_t>(maxCellZ) - static_cast<std::int64_t>(minCellZ);
  if (policy.maxCells <= 0 || width64 <= 0 || depth64 <= 0 ||
      width64 >
          static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max()) ||
      depth64 >
          static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max()) ||
      width64 * depth64 > policy.maxCells) {
    return fallbackFloorMeshes(group, policy);
  }

  GreedyMeshGrid grid;
  grid.width = static_cast<std::int32_t>(width64);
  grid.depth = static_cast<std::int32_t>(depth64);
  const std::size_t cellCount =
      static_cast<std::size_t>(grid.width) *
      static_cast<std::size_t>(grid.depth);
  grid.keys.assign(cellCount, 0U);
  std::vector<const RoomBakeGreedyFloorInput*> cellOwners(cellCount, nullptr);
  const auto cellIndex = [&](std::int32_t localX,
                             std::int32_t localZ) -> std::size_t {
    return static_cast<std::size_t>(localZ) *
               static_cast<std::size_t>(grid.width) +
           static_cast<std::size_t>(localX);
  };

  for (const GreedyFloorCandidate& candidate : group.candidates) {
    if (candidate.input == nullptr) {
      return fallbackFloorMeshes(group, policy);
    }
    for (std::int32_t z = candidate.footprint.minCellZ;
         z < candidate.footprint.maxCellZExclusive; ++z) {
      for (std::int32_t x = candidate.footprint.minCellX;
           x < candidate.footprint.maxCellXExclusive; ++x) {
        const std::int32_t localX = x - minCellX;
        const std::int32_t localZ = z - minCellZ;
        const std::size_t index = cellIndex(localX, localZ);
        if (cellOwners[index] != nullptr) {
          return fallbackFloorMeshes(group, policy);
        }
        cellOwners[index] = candidate.input;
        grid.keys[index] = 1U;
      }
    }
  }

  const GreedyMeshReceipt receipt = greedyMeshGrid(grid);
  if (!receipt.ok) {
    return fallbackFloorMeshes(group, policy);
  }

  std::vector<RoomBakeGreedyFloorMeshPlan> meshes;
  meshes.reserve(receipt.quads.size());
  for (std::size_t quadIndex = 0; quadIndex < receipt.quads.size();
       ++quadIndex) {
    const GreedyQuad& quad = receipt.quads[quadIndex];
    std::vector<const RoomBakeGreedyFloorInput*> sourceInputs;
    for (std::int32_t dz = 0; dz < quad.depth; ++dz) {
      for (std::int32_t dx = 0; dx < quad.width; ++dx) {
        const RoomBakeGreedyFloorInput* source =
            cellOwners[cellIndex(quad.x + dx, quad.z + dz)];
        appendGreedyFloorSource(sourceInputs, source);
      }
    }
    std::sort(sourceInputs.begin(),
              sourceInputs.end(),
              [](const RoomBakeGreedyFloorInput* lhs,
                 const RoomBakeGreedyFloorInput* rhs) {
                return lhs->documentIndex < rhs->documentIndex;
              });

    RoomBakeGreedyFloorMeshPlan plan;
    plan.firstDocumentIndex = std::numeric_limits<std::size_t>::max();
    plan.sources.reserve(sourceInputs.size());
    for (const RoomBakeGreedyFloorInput* source : sourceInputs) {
      plan.sources.push_back(sourceForInput(*source));
      plan.firstDocumentIndex =
          std::min(plan.firstDocumentIndex, source->documentIndex);
    }

    Vec3 min;
    min.x = static_cast<float>(minCellX + quad.x) * policy.cellSizeMeters;
    min.y = group.minY;
    min.z = static_cast<float>(minCellZ + quad.z) * policy.cellSizeMeters;
    Vec3 max;
    max.x = static_cast<float>(minCellX + quad.x + quad.width) *
            policy.cellSizeMeters;
    max.y = group.maxY;
    max.z = static_cast<float>(minCellZ + quad.z + quad.depth) *
            policy.cellSizeMeters;
    Vec3 size{max.x - min.x, max.y - min.y, max.z - min.z};
    Vec3 center{min.x + size.x * 0.5F,
                min.y + size.y * 0.5F,
                min.z + size.z * 0.5F};
    plan.mesh = meshForBounds(greedyFloorMeshId(sourceInputs, quadIndex),
                              center,
                              size,
                              policy);
    meshes.push_back(std::move(plan));
  }

  return meshes;
}

void appendFloorGroup(std::vector<GreedyFloorGroup>& groups,
                      const RoomBakeGreedyFloorInput& input,
                      GridFootprint footprint,
                      const RoomBakeGreedyFloorPolicy& policy) {
  for (GreedyFloorGroup& group : groups) {
    if (group.minY == input.min.y && group.maxY == input.max.y &&
        group.meshId == policy.meshId && group.materialId == policy.materialId &&
        group.semanticRole == policy.semanticRole && group.role == policy.role) {
      group.candidates.push_back({&input, footprint});
      return;
    }
  }

  GreedyFloorGroup group;
  group.minY = input.min.y;
  group.maxY = input.max.y;
  group.meshId = policy.meshId;
  group.materialId = policy.materialId;
  group.semanticRole = policy.semanticRole;
  group.role = policy.role;
  group.candidates.push_back({&input, footprint});
  groups.push_back(std::move(group));
}

}  // namespace

std::vector<RoomBakeGreedyFloorMeshPlan> buildRoomBakeGreedyFloorPlan(
    std::span<const RoomBakeGreedyFloorInput> inputs,
    const RoomBakeGreedyFloorPolicy& policy) {
  std::vector<GreedyFloorGroup> groups;
  std::vector<RoomBakeGreedyFloorMeshPlan> fallbackMeshes;
  for (const RoomBakeGreedyFloorInput& input : inputs) {
    GridFootprint footprint;
    if (!greedyFloorFootprintForBounds(input, policy.cellSizeMeters, footprint)) {
      fallbackMeshes.push_back(fallbackFloorMesh(input, policy));
      continue;
    }
    appendFloorGroup(groups, input, footprint, policy);
  }

  std::vector<RoomBakeGreedyFloorMeshPlan> meshes = std::move(fallbackMeshes);
  for (const GreedyFloorGroup& group : groups) {
    std::vector<RoomBakeGreedyFloorMeshPlan> groupMeshes =
        buildGreedyFloorMeshesForGroup(group, policy);
    meshes.insert(meshes.end(),
                  std::make_move_iterator(groupMeshes.begin()),
                  std::make_move_iterator(groupMeshes.end()));
  }

  std::sort(meshes.begin(),
            meshes.end(),
            [](const RoomBakeGreedyFloorMeshPlan& lhs,
               const RoomBakeGreedyFloorMeshPlan& rhs) {
              if (lhs.firstDocumentIndex != rhs.firstDocumentIndex) {
                return lhs.firstDocumentIndex < rhs.firstDocumentIndex;
              }
              return lhs.mesh.id < rhs.mesh.id;
            });
  return meshes;
}

}  // namespace iggy3d::creative
