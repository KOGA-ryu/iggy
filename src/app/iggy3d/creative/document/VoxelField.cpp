#include "app/iggy3d/creative/document/VoxelField.hpp"
#include "app/iggy3d/creative/document/VoxelFieldInternal.hpp"

#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>

namespace iggy3d::creative::voxel_field_internal {

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

}  // namespace iggy3d::creative::voxel_field_internal

namespace iggy3d::creative {

using voxel_field_internal::localIndex;

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

[[nodiscard]] bool voxelEditLess(const CreativeVoxelEdit& lhs,
                                 const CreativeVoxelEdit& rhs) noexcept {
  const CreativeVoxelChunkCoord lhsChunk = chunkCoordForCell(lhs.cell);
  const CreativeVoxelChunkCoord rhsChunk = chunkCoordForCell(rhs.cell);
  if (!(lhsChunk == rhsChunk)) {
    return chunkCoordLess(lhsChunk, rhsChunk);
  }
  return cellLess(lhs.cell, rhs.cell);
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
  std::sort(ordered.begin(), ordered.end(), voxelEditLess);
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

}  // namespace iggy3d::creative
