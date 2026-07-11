#pragma once

#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::int32_t kCreativeVoxelChunkEdge = 16;
inline constexpr std::size_t kCreativeVoxelChunkCellCount =
    static_cast<std::size_t>(kCreativeVoxelChunkEdge) *
    static_cast<std::size_t>(kCreativeVoxelChunkEdge) *
    static_cast<std::size_t>(kCreativeVoxelChunkEdge);

struct CreativeVoxelChunkCoord {
  std::int32_t x = 0;
  std::int32_t y = 0;
  std::int32_t z = 0;

  [[nodiscard]] bool operator==(const CreativeVoxelChunkCoord&) const noexcept =
      default;
};

struct CreativeVoxelEdit {
  CreativeGridCoord3 cell{};
  // Unknown erases the cell. Count is always invalid.
  CreativeObjectKind material = CreativeObjectKind::Unknown;
};

enum class CreativeVoxelMutationStatus : std::uint8_t {
  NotRequested,
  InvalidField,
  InvalidCell,
  InvalidMaterial,
  DuplicateCell,
  NoChange,
  Applied,
};

struct CreativeVoxelMutationReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeVoxelMutationStatus status =
      CreativeVoxelMutationStatus::NotRequested;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::uint64_t attemptedCellCount = 0;
  std::uint64_t changedCellCount = 0;
  std::uint64_t createdCellCount = 0;
  std::uint64_t removedCellCount = 0;
  std::uint64_t replacedCellCount = 0;
  std::uint64_t chunkCountBefore = 0;
  std::uint64_t chunkCountAfter = 0;
  // Deterministic work metric: only changed chunks may be staged.
  std::uint64_t stagedChunkCount = 0;
  std::vector<CreativeVoxelChunkCoord> dirtyChunks;
  std::string_view reasonCode = "creative_voxel_mutation_not_requested";
};

struct CreativeVoxelChunk {
  CreativeVoxelChunkCoord coord{};
  std::array<CreativeObjectKind, kCreativeVoxelChunkCellCount> materials{};
  std::uint16_t occupiedCellCount = 0;
  std::uint64_t revision = 0;
};

struct CreativeVoxelCuboid {
  CreativeVoxelChunkCoord chunk{};
  CreativeGridCoord3 minCell{};
  CreativeGridCoord3 maxCellExclusive{};
  CreativeObjectKind material = CreativeObjectKind::Unknown;
};

struct CreativeVoxelCell {
  CreativeGridCoord3 cell{};
  CreativeObjectKind material = CreativeObjectKind::Unknown;
};

class CreativeVoxelField {
 public:
  [[nodiscard]] bool isValid() const noexcept;
  [[nodiscard]] bool validateInvariants() const noexcept;
  [[nodiscard]] std::uint64_t revision() const noexcept;
  [[nodiscard]] std::uint64_t occupiedCellCount() const noexcept;
  [[nodiscard]] std::uint64_t chunkCount() const noexcept;
  [[nodiscard]] std::span<const CreativeVoxelChunk> chunks() const noexcept;
  [[nodiscard]] CreativeObjectKind materialAt(
      CreativeGridCoord3 cell) const noexcept;
  [[nodiscard]] bool occupied(CreativeGridCoord3 cell) const noexcept;

  [[nodiscard]] CreativeVoxelMutationReceipt apply(
      std::span<const CreativeVoxelEdit> edits);
  void clear() noexcept;

 private:
  std::vector<CreativeVoxelChunk> chunks_;
  std::uint64_t occupiedCellCount_ = 0;
  std::uint64_t revision_ = 0;
  bool valid_ = true;
};

struct CreativeVoxelRaycastRequest {
  CreativeVec3 rayOrigin{};
  CreativeVec3 rayDirection{};
  CreativeVec3 gridOrigin{};
  double cellSize = 1.0;
  double maxDistance = 256.0;
};

struct CreativeVoxelRaycastReceipt {
  bool requested = false;
  bool accepted = false;
  bool hit = false;
  bool startInside = false;
  CreativeGridCoord3 cell{};
  CreativeObjectKind material = CreativeObjectKind::Unknown;
  CreativeVec3 hitPoint{};
  CreativeVec3 faceNormal{};
  double distance = 0.0;
  std::string_view reasonCode = "creative_voxel_raycast_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeVoxelMutationStatus status) noexcept;

// Produces deterministic, chunk-local greedy boxes. Cuboids never cross a
// chunk boundary, so a renderer can cache and rebuild them per chunk revision.
[[nodiscard]] std::vector<CreativeVoxelCuboid> buildCreativeVoxelCuboids(
    const CreativeVoxelField& field);
[[nodiscard]] std::vector<CreativeVoxelCuboid> buildCreativeVoxelCuboids(
    const CreativeVoxelChunk& chunk);
[[nodiscard]] std::vector<CreativeVoxelCell> collectCreativeVoxelCells(
    const CreativeVoxelField& field);
[[nodiscard]] std::vector<CreativeVoxelCell> collectCreativeVoxelCells(
    const CreativeVoxelField& field,
    CreativeGridBounds3 inclusiveBounds);

// Grid DDA: O(number of crossed cells), no candidate list or per-cell heap
// allocation. The direction need not be normalized.
[[nodiscard]] CreativeVoxelRaycastReceipt raycastCreativeVoxelField(
    const CreativeVoxelField& field,
    const CreativeVoxelRaycastRequest& request) noexcept;

}  // namespace iggy3d::creative
