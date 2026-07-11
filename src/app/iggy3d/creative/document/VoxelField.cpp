#include "app/iggy3d/creative/document/VoxelField.hpp"

#include <algorithm>
#include <bitset>
#include <cmath>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool validMaterial(CreativeObjectKind material) noexcept {
  return material >= CreativeObjectKind::Unknown &&
         material < CreativeObjectKind::Count;
}

[[nodiscard]] bool validCell(CreativeGridCoord3 cell) noexcept {
  constexpr std::int32_t maxCell =
      std::numeric_limits<std::int32_t>::max() - 1;
  return cell.x <= maxCell && cell.y <= maxCell && cell.z <= maxCell;
}

[[nodiscard]] bool chunkCoordLess(CreativeVoxelChunkCoord lhs,
                                  CreativeVoxelChunkCoord rhs) noexcept {
  if (lhs.z != rhs.z) {
    return lhs.z < rhs.z;
  }
  if (lhs.y != rhs.y) {
    return lhs.y < rhs.y;
  }
  return lhs.x < rhs.x;
}

[[nodiscard]] bool cellLess(CreativeGridCoord3 lhs,
                            CreativeGridCoord3 rhs) noexcept {
  if (lhs.z != rhs.z) {
    return lhs.z < rhs.z;
  }
  if (lhs.y != rhs.y) {
    return lhs.y < rhs.y;
  }
  return lhs.x < rhs.x;
}

[[nodiscard]] std::int32_t floorDivByChunk(std::int32_t value) noexcept {
  std::int32_t quotient = value / kCreativeVoxelChunkEdge;
  if (value % kCreativeVoxelChunkEdge < 0) {
    --quotient;
  }
  return quotient;
}

[[nodiscard]] CreativeVoxelChunkCoord chunkCoordForCell(
    CreativeGridCoord3 cell) noexcept {
  return {floorDivByChunk(cell.x), floorDivByChunk(cell.y),
          floorDivByChunk(cell.z)};
}

[[nodiscard]] std::int32_t localCoord(std::int32_t cell,
                                      std::int32_t chunk) noexcept {
  return cell - chunk * kCreativeVoxelChunkEdge;
}

[[nodiscard]] std::size_t localIndex(std::int32_t x,
                                     std::int32_t y,
                                     std::int32_t z) noexcept {
  return (static_cast<std::size_t>(z) * kCreativeVoxelChunkEdge +
          static_cast<std::size_t>(y)) *
             kCreativeVoxelChunkEdge +
         static_cast<std::size_t>(x);
}

[[nodiscard]] std::size_t localIndex(CreativeGridCoord3 cell,
                                     CreativeVoxelChunkCoord chunk) noexcept {
  return localIndex(localCoord(cell.x, chunk.x),
                    localCoord(cell.y, chunk.y),
                    localCoord(cell.z, chunk.z));
}

using ChunkIterator = std::vector<CreativeVoxelChunk>::iterator;
using ConstChunkIterator = std::vector<CreativeVoxelChunk>::const_iterator;

static_assert(std::is_nothrow_move_constructible_v<CreativeVoxelChunk>);
static_assert(std::is_nothrow_move_assignable_v<CreativeVoxelChunk>);

[[nodiscard]] ChunkIterator findChunk(
    std::vector<CreativeVoxelChunk>& chunks,
    CreativeVoxelChunkCoord coord) noexcept {
  const ChunkIterator found = std::lower_bound(
      chunks.begin(), chunks.end(), coord,
      [](const CreativeVoxelChunk& chunk, CreativeVoxelChunkCoord value) {
        return chunkCoordLess(chunk.coord, value);
      });
  return found != chunks.end() && found->coord == coord ? found : chunks.end();
}

[[nodiscard]] ConstChunkIterator findChunk(
    const std::vector<CreativeVoxelChunk>& chunks,
    CreativeVoxelChunkCoord coord) noexcept {
  const ConstChunkIterator found = std::lower_bound(
      chunks.begin(), chunks.end(), coord,
      [](const CreativeVoxelChunk& chunk, CreativeVoxelChunkCoord value) {
        return chunkCoordLess(chunk.coord, value);
      });
  return found != chunks.end() && found->coord == coord ? found : chunks.end();
}

[[nodiscard]] bool finiteVec3(CreativeVec3 value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

[[nodiscard]] bool tryWorldCell(CreativeVec3 point,
                                CreativeVec3 origin,
                                double cellSize,
                                CreativeGridCoord3& out) noexcept {
  const double x = std::floor((point.x - origin.x) / cellSize);
  const double y = std::floor((point.y - origin.y) / cellSize);
  const double z = std::floor((point.z - origin.z) / cellSize);
  constexpr double minimum =
      static_cast<double>(std::numeric_limits<std::int32_t>::min());
  constexpr double maximum =
      static_cast<double>(std::numeric_limits<std::int32_t>::max() - 1);
  if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) ||
      x < minimum || y < minimum || z < minimum || x > maximum ||
      y > maximum || z > maximum) {
    return false;
  }
  out = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(y),
         static_cast<std::int32_t>(z)};
  return true;
}

[[nodiscard]] double axisBoundaryDistance(double rayOrigin,
                                          double direction,
                                          double gridOrigin,
                                          double cellSize,
                                          std::int32_t cell,
                                          std::int32_t step) noexcept {
  if (step == 0) {
    return std::numeric_limits<double>::infinity();
  }
  const double boundary =
      gridOrigin +
      (static_cast<double>(cell) + (step > 0 ? 1.0 : 0.0)) * cellSize;
  return (boundary - rayOrigin) / direction;
}

[[nodiscard]] CreativeGridCoord3 globalCell(CreativeVoxelChunkCoord chunk,
                                            std::int32_t x,
                                            std::int32_t y,
                                            std::int32_t z) noexcept {
  return {static_cast<std::int32_t>(static_cast<std::int64_t>(chunk.x) *
                                        kCreativeVoxelChunkEdge +
                                    x),
          static_cast<std::int32_t>(static_cast<std::int64_t>(chunk.y) *
                                        kCreativeVoxelChunkEdge +
                                    y),
          static_cast<std::int32_t>(static_cast<std::int64_t>(chunk.z) *
                                        kCreativeVoxelChunkEdge +
                                    z)};
}

}  // namespace

std::string_view toString(CreativeVoxelMutationStatus status) noexcept {
  switch (status) {
    case CreativeVoxelMutationStatus::NotRequested:
      return "NotRequested";
    case CreativeVoxelMutationStatus::InvalidField:
      return "InvalidField";
    case CreativeVoxelMutationStatus::InvalidCell:
      return "InvalidCell";
    case CreativeVoxelMutationStatus::InvalidMaterial:
      return "InvalidMaterial";
    case CreativeVoxelMutationStatus::DuplicateCell:
      return "DuplicateCell";
    case CreativeVoxelMutationStatus::NoChange:
      return "NoChange";
    case CreativeVoxelMutationStatus::Applied:
      return "Applied";
  }
  return "Unknown";
}

bool CreativeVoxelField::isValid() const noexcept {
  return valid_;
}

bool CreativeVoxelField::validateInvariants() const noexcept {
  if (!valid_) {
    return false;
  }
  std::uint64_t occupiedCount = 0;
  CreativeVoxelChunkCoord previous{};
  bool hasPrevious = false;
  for (const CreativeVoxelChunk& chunk : chunks_) {
    if (hasPrevious && !chunkCoordLess(previous, chunk.coord)) {
      return false;
    }
    hasPrevious = true;
    previous = chunk.coord;

    std::uint64_t chunkOccupied = 0;
    for (CreativeObjectKind material : chunk.materials) {
      if (!validMaterial(material)) {
        return false;
      }
      chunkOccupied += material != CreativeObjectKind::Unknown ? 1U : 0U;
    }
    if (chunkOccupied == 0U || chunkOccupied != chunk.occupiedCellCount) {
      return false;
    }
    occupiedCount += chunkOccupied;
  }
  return occupiedCount == occupiedCellCount_;
}

std::uint64_t CreativeVoxelField::revision() const noexcept {
  return revision_;
}

std::uint64_t CreativeVoxelField::occupiedCellCount() const noexcept {
  return occupiedCellCount_;
}

std::uint64_t CreativeVoxelField::chunkCount() const noexcept {
  return chunks_.size();
}

std::span<const CreativeVoxelChunk> CreativeVoxelField::chunks() const noexcept {
  return chunks_;
}

CreativeObjectKind CreativeVoxelField::materialAt(
    CreativeGridCoord3 cell) const noexcept {
  const CreativeVoxelChunkCoord coord = chunkCoordForCell(cell);
  const ConstChunkIterator chunk = findChunk(chunks_, coord);
  return chunk == chunks_.end()
             ? CreativeObjectKind::Unknown
             : chunk->materials[localIndex(cell, coord)];
}

bool CreativeVoxelField::occupied(CreativeGridCoord3 cell) const noexcept {
  return materialAt(cell) != CreativeObjectKind::Unknown;
}

CreativeVoxelMutationReceipt CreativeVoxelField::apply(
    std::span<const CreativeVoxelEdit> edits) {
  CreativeVoxelMutationReceipt receipt;
  receipt.requested = true;
  receipt.revisionBefore = revision_;
  receipt.revisionAfter = revision_;
  receipt.attemptedCellCount = edits.size();
  receipt.chunkCountBefore = chunks_.size();
  receipt.chunkCountAfter = chunks_.size();

  if (!isValid()) {
    receipt.status = CreativeVoxelMutationStatus::InvalidField;
    receipt.reasonCode = "creative_voxel_field_invalid";
    return receipt;
  }

  std::vector<CreativeVoxelEdit> ordered{edits.begin(), edits.end()};
  for (const CreativeVoxelEdit& edit : ordered) {
    if (!validCell(edit.cell)) {
      receipt.status = CreativeVoxelMutationStatus::InvalidCell;
      receipt.reasonCode = "creative_voxel_cell_invalid";
      return receipt;
    }
    if (!validMaterial(edit.material)) {
      receipt.status = CreativeVoxelMutationStatus::InvalidMaterial;
      receipt.reasonCode = "creative_voxel_material_invalid";
      return receipt;
    }
  }
  std::sort(ordered.begin(), ordered.end(),
            [](const CreativeVoxelEdit& lhs, const CreativeVoxelEdit& rhs) {
              return cellLess(lhs.cell, rhs.cell);
            });
  for (std::size_t index = 1; index < ordered.size(); ++index) {
    if (!cellLess(ordered[index - 1].cell, ordered[index].cell) &&
        !cellLess(ordered[index].cell, ordered[index - 1].cell)) {
      receipt.status = CreativeVoxelMutationStatus::DuplicateCell;
      receipt.reasonCode = "creative_voxel_duplicate_cell";
      return receipt;
    }
  }

  std::vector<CreativeVoxelChunk> stagedChunks;
  std::size_t insertedChunkCount = 0;
  for (std::size_t begin = 0; begin < ordered.size();) {
    const CreativeVoxelChunkCoord coord = chunkCoordForCell(ordered[begin].cell);
    std::size_t end = begin + 1U;
    while (end < ordered.size() &&
           chunkCoordForCell(ordered[end].cell) == coord) {
      ++end;
    }

    const ConstChunkIterator existing = findChunk(chunks_, coord);
    const bool existed = existing != chunks_.end();
    bool chunkChanges = false;
    for (std::size_t index = begin; index < end; ++index) {
      const CreativeObjectKind oldMaterial =
          existed ? existing->materials[localIndex(ordered[index].cell, coord)]
                  : CreativeObjectKind::Unknown;
      chunkChanges |= oldMaterial != ordered[index].material;
    }
    if (!chunkChanges) {
      begin = end;
      continue;
    }

    CreativeVoxelChunk staged;
    if (existed) {
      staged = *existing;
    } else {
      staged.coord = coord;
    }

    for (std::size_t index = begin; index < end; ++index) {
      const CreativeVoxelEdit& edit = ordered[index];
      const std::size_t materialIndex = localIndex(edit.cell, coord);
      const CreativeObjectKind oldMaterial = staged.materials[materialIndex];
      if (oldMaterial == edit.material) {
        continue;
      }

      staged.materials[materialIndex] = edit.material;
      if (edit.material == CreativeObjectKind::Unknown) {
        --staged.occupiedCellCount;
        ++receipt.removedCellCount;
      } else if (oldMaterial == CreativeObjectKind::Unknown) {
        ++staged.occupiedCellCount;
        ++receipt.createdCellCount;
      } else {
        ++receipt.replacedCellCount;
      }
      ++receipt.changedCellCount;
    }

    staged.revision = revision_ + 1U;
    insertedChunkCount +=
        !existed && staged.occupiedCellCount != 0U ? 1U : 0U;
    receipt.dirtyChunks.push_back(coord);
    stagedChunks.push_back(std::move(staged));
    begin = end;
  }
  receipt.stagedChunkCount = stagedChunks.size();

  if (receipt.changedCellCount == 0U) {
    receipt.accepted = true;
    receipt.status = CreativeVoxelMutationStatus::NoChange;
    receipt.reasonCode = "creative_voxel_no_change";
    return receipt;
  }

  // All allocation completes before mutation. With the no-throw chunk moves
  // asserted above, the sorted-vector commit cannot leave a partial batch.
  chunks_.reserve(chunks_.size() + insertedChunkCount);
  for (CreativeVoxelChunk& staged : stagedChunks) {
    ChunkIterator existing = findChunk(chunks_, staged.coord);
    if (staged.occupiedCellCount == 0U) {
      if (existing != chunks_.end()) {
        chunks_.erase(existing);
      }
    } else if (existing != chunks_.end()) {
      *existing = std::move(staged);
    } else {
      ChunkIterator insertion = std::lower_bound(
          chunks_.begin(), chunks_.end(), staged.coord,
          [](const CreativeVoxelChunk& chunk, CreativeVoxelChunkCoord coord) {
            return chunkCoordLess(chunk.coord, coord);
          });
      chunks_.insert(insertion, std::move(staged));
    }
  }
  occupiedCellCount_ -= receipt.removedCellCount;
  occupiedCellCount_ += receipt.createdCellCount;
  ++revision_;

  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = CreativeVoxelMutationStatus::Applied;
  receipt.revisionAfter = revision_;
  receipt.chunkCountAfter = chunks_.size();
  receipt.reasonCode = "creative_voxel_applied";
  return receipt;
}

void CreativeVoxelField::clear() noexcept {
  chunks_.clear();
  occupiedCellCount_ = 0;
  revision_ = 0;
  valid_ = true;
}

std::vector<CreativeVoxelCuboid> buildCreativeVoxelCuboids(
    const CreativeVoxelField& field) {
  std::vector<CreativeVoxelCuboid> cuboids;
  for (const CreativeVoxelChunk& chunk : field.chunks()) {
    std::vector<CreativeVoxelCuboid> chunkCuboids =
        buildCreativeVoxelCuboids(chunk);
    cuboids.insert(cuboids.end(),
                   std::make_move_iterator(chunkCuboids.begin()),
                   std::make_move_iterator(chunkCuboids.end()));
  }
  return cuboids;
}

std::vector<CreativeVoxelCuboid> buildCreativeVoxelCuboids(
    const CreativeVoxelChunk& chunk) {
  std::vector<CreativeVoxelCuboid> cuboids;
  std::bitset<kCreativeVoxelChunkCellCount> consumed;
  for (std::int32_t z = 0; z < kCreativeVoxelChunkEdge; ++z) {
    for (std::int32_t y = 0; y < kCreativeVoxelChunkEdge; ++y) {
      for (std::int32_t x = 0; x < kCreativeVoxelChunkEdge; ++x) {
        const std::size_t seedIndex = localIndex(x, y, z);
        const CreativeObjectKind material = chunk.materials[seedIndex];
        if (material == CreativeObjectKind::Unknown ||
            consumed.test(seedIndex)) {
          continue;
        }

        std::int32_t maxX = x + 1;
        while (maxX < kCreativeVoxelChunkEdge) {
          const std::size_t index = localIndex(maxX, y, z);
          if (consumed.test(index) || chunk.materials[index] != material) {
            break;
          }
          ++maxX;
        }

        std::int32_t maxZ = z + 1;
        while (maxZ < kCreativeVoxelChunkEdge) {
          bool rowMatches = true;
          for (std::int32_t testX = x; testX < maxX; ++testX) {
            const std::size_t index = localIndex(testX, y, maxZ);
            if (consumed.test(index) || chunk.materials[index] != material) {
              rowMatches = false;
              break;
            }
          }
          if (!rowMatches) {
            break;
          }
          ++maxZ;
        }

        std::int32_t maxY = y + 1;
        while (maxY < kCreativeVoxelChunkEdge) {
          bool layerMatches = true;
          for (std::int32_t testZ = z; testZ < maxZ && layerMatches;
               ++testZ) {
            for (std::int32_t testX = x; testX < maxX; ++testX) {
              const std::size_t index = localIndex(testX, maxY, testZ);
              if (consumed.test(index) || chunk.materials[index] != material) {
                layerMatches = false;
                break;
              }
            }
          }
          if (!layerMatches) {
            break;
          }
          ++maxY;
        }

        for (std::int32_t fillY = y; fillY < maxY; ++fillY) {
          for (std::int32_t fillZ = z; fillZ < maxZ; ++fillZ) {
            for (std::int32_t fillX = x; fillX < maxX; ++fillX) {
              consumed.set(localIndex(fillX, fillY, fillZ));
            }
          }
        }

        cuboids.push_back({chunk.coord,
                           globalCell(chunk.coord, x, y, z),
                           globalCell(chunk.coord, maxX, maxY, maxZ),
                           material});
      }
    }
  }
  return cuboids;
}

std::vector<CreativeVoxelCell> collectCreativeVoxelCells(
    const CreativeVoxelField& field) {
  const CreativeGridBounds3 allCells{
      {std::numeric_limits<std::int32_t>::min(),
       std::numeric_limits<std::int32_t>::min(),
       std::numeric_limits<std::int32_t>::min()},
      {std::numeric_limits<std::int32_t>::max(),
       std::numeric_limits<std::int32_t>::max(),
       std::numeric_limits<std::int32_t>::max()}};
  return collectCreativeVoxelCells(field, allCells);
}

std::vector<CreativeVoxelCell> collectCreativeVoxelCells(
    const CreativeVoxelField& field,
    CreativeGridBounds3 inclusiveBounds) {
  std::vector<CreativeVoxelCell> cells;
  cells.reserve(static_cast<std::size_t>(
      std::min<std::uint64_t>(field.occupiedCellCount(), 4096U)));
  for (const CreativeVoxelChunk& chunk : field.chunks()) {
    for (std::int32_t z = 0; z < kCreativeVoxelChunkEdge; ++z) {
      for (std::int32_t y = 0; y < kCreativeVoxelChunkEdge; ++y) {
        for (std::int32_t x = 0; x < kCreativeVoxelChunkEdge; ++x) {
          const CreativeObjectKind material =
              chunk.materials[localIndex(x, y, z)];
          if (material == CreativeObjectKind::Unknown) {
            continue;
          }
          const CreativeGridCoord3 cell = globalCell(chunk.coord, x, y, z);
          if (cell.x < inclusiveBounds.min.x ||
              cell.y < inclusiveBounds.min.y ||
              cell.z < inclusiveBounds.min.z ||
              cell.x > inclusiveBounds.max.x ||
              cell.y > inclusiveBounds.max.y ||
              cell.z > inclusiveBounds.max.z) {
            continue;
          }
          cells.push_back({cell, material});
        }
      }
    }
  }
  return cells;
}

CreativeVoxelRaycastReceipt raycastCreativeVoxelField(
    const CreativeVoxelField& field,
    const CreativeVoxelRaycastRequest& request) noexcept {
  CreativeVoxelRaycastReceipt receipt;
  receipt.requested = true;
  if (!field.isValid() || !finiteVec3(request.rayOrigin) ||
      !finiteVec3(request.rayDirection) || !finiteVec3(request.gridOrigin) ||
      !std::isfinite(request.cellSize) || request.cellSize <= 0.0 ||
      !std::isfinite(request.maxDistance) || request.maxDistance < 0.0) {
    receipt.reasonCode = "creative_voxel_raycast_invalid";
    return receipt;
  }

  const double directionLength =
      std::sqrt(request.rayDirection.x * request.rayDirection.x +
                request.rayDirection.y * request.rayDirection.y +
                request.rayDirection.z * request.rayDirection.z);
  if (!std::isfinite(directionLength) || directionLength <= 1.0e-12) {
    receipt.reasonCode = "creative_voxel_raycast_degenerate";
    return receipt;
  }

  const CreativeVec3 direction{request.rayDirection.x / directionLength,
                               request.rayDirection.y / directionLength,
                               request.rayDirection.z / directionLength};
  CreativeGridCoord3 cell{};
  if (!tryWorldCell(request.rayOrigin, request.gridOrigin, request.cellSize,
                    cell)) {
    receipt.reasonCode = "creative_voxel_raycast_origin_out_of_range";
    return receipt;
  }
  const CreativeObjectKind startMaterial = field.materialAt(cell);
  receipt.accepted = true;
  if (startMaterial != CreativeObjectKind::Unknown) {
    receipt.hit = true;
    receipt.startInside = true;
    receipt.cell = cell;
    receipt.material = startMaterial;
    receipt.hitPoint = request.rayOrigin;
    receipt.reasonCode = "creative_voxel_raycast_start_inside";
    return receipt;
  }

  const std::int32_t stepX = (direction.x > 0.0) - (direction.x < 0.0);
  const std::int32_t stepY = (direction.y > 0.0) - (direction.y < 0.0);
  const std::int32_t stepZ = (direction.z > 0.0) - (direction.z < 0.0);
  double tMaxX = axisBoundaryDistance(request.rayOrigin.x, direction.x,
                                      request.gridOrigin.x, request.cellSize,
                                      cell.x, stepX);
  double tMaxY = axisBoundaryDistance(request.rayOrigin.y, direction.y,
                                      request.gridOrigin.y, request.cellSize,
                                      cell.y, stepY);
  double tMaxZ = axisBoundaryDistance(request.rayOrigin.z, direction.z,
                                      request.gridOrigin.z, request.cellSize,
                                      cell.z, stepZ);
  const double tDeltaX = stepX == 0
                             ? std::numeric_limits<double>::infinity()
                             : request.cellSize / std::fabs(direction.x);
  const double tDeltaY = stepY == 0
                             ? std::numeric_limits<double>::infinity()
                             : request.cellSize / std::fabs(direction.y);
  const double tDeltaZ = stepZ == 0
                             ? std::numeric_limits<double>::infinity()
                             : request.cellSize / std::fabs(direction.z);

  const double estimatedSteps =
      std::ceil(request.maxDistance / request.cellSize) * 3.0 + 3.0;
  const std::uint64_t maxSteps = static_cast<std::uint64_t>(
      std::min(estimatedSteps, 10'000'000.0));
  CreativeVec3 faceNormal{};
  double distance = 0.0;
  for (std::uint64_t step = 0; step < maxSteps; ++step) {
    if (tMaxX <= tMaxY && tMaxX <= tMaxZ) {
      if ((stepX > 0 &&
           cell.x >= std::numeric_limits<std::int32_t>::max() - 1) ||
          (stepX < 0 && cell.x == std::numeric_limits<std::int32_t>::min())) {
        break;
      }
      distance = tMaxX;
      tMaxX += tDeltaX;
      cell.x += stepX;
      faceNormal = {static_cast<double>(-stepX), 0.0, 0.0};
    } else if (tMaxY <= tMaxZ) {
      if ((stepY > 0 &&
           cell.y >= std::numeric_limits<std::int32_t>::max() - 1) ||
          (stepY < 0 && cell.y == std::numeric_limits<std::int32_t>::min())) {
        break;
      }
      distance = tMaxY;
      tMaxY += tDeltaY;
      cell.y += stepY;
      faceNormal = {0.0, static_cast<double>(-stepY), 0.0};
    } else {
      if ((stepZ > 0 &&
           cell.z >= std::numeric_limits<std::int32_t>::max() - 1) ||
          (stepZ < 0 && cell.z == std::numeric_limits<std::int32_t>::min())) {
        break;
      }
      distance = tMaxZ;
      tMaxZ += tDeltaZ;
      cell.z += stepZ;
      faceNormal = {0.0, 0.0, static_cast<double>(-stepZ)};
    }

    if (!std::isfinite(distance) || distance > request.maxDistance) {
      break;
    }
    const CreativeObjectKind material = field.materialAt(cell);
    if (material == CreativeObjectKind::Unknown) {
      continue;
    }
    receipt.hit = true;
    receipt.cell = cell;
    receipt.material = material;
    receipt.distance = distance;
    receipt.hitPoint = {request.rayOrigin.x + direction.x * distance,
                        request.rayOrigin.y + direction.y * distance,
                        request.rayOrigin.z + direction.z * distance};
    receipt.faceNormal = faceNormal;
    receipt.reasonCode = "creative_voxel_raycast_hit";
    return receipt;
  }

  receipt.reasonCode = "creative_voxel_raycast_miss";
  return receipt;
}

}  // namespace iggy3d::creative
