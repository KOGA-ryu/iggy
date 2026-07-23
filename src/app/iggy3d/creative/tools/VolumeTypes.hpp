#pragma once

#include <cstdint>

namespace iggy3d::creative {

// Controls how Volume Fill treats cells that already contain authored volume
// material. Preserve is the non-destructive default; Replace makes the planned
// shape authoritative inside its generated cells.
enum class CreativeVolumeFillOverlapPolicy : std::uint8_t {
  PreserveExisting,
  ReplaceExisting,
  Count,
};

enum class CreativeVolumeHollowThickness : std::uint8_t {
  OneCell,
  TwoCells,
  FourCells,
  Count,
};

// Inward treats the selected bounds as the outside of the shell. Outward
// treats them as the cavity and grows the shell around them.
enum class CreativeVolumeHollowAlignment : std::uint8_t {
  Inward,
  Outward,
  Count,
};

// Openings are relative to the selected shape axis. For a Y cylinder these
// are its bottom/top caps; for X or Z they are the corresponding end caps.
enum class CreativeVolumeHollowOpening : std::uint8_t {
  Closed,
  NegativeEnd,
  PositiveEnd,
  BothEnds,
  Count,
};

// KeepEdges leaves cells that are also owned by a closed face, producing a
// framed opening. CutThrough removes the full open-face slab, including shared
// edge and corner cells.
enum class CreativeVolumeHollowCornerRule : std::uint8_t {
  KeepEdges,
  CutThrough,
  Count,
};

// Volume operations can address sparse voxel cells, compatible document
// objects, or both. Replace only treats legacy tagged volume-cell objects as
// compatible document objects; it never changes arbitrary authored objects.
enum class CreativeVolumeMemberMask : std::uint8_t {
  VoxelCells,
  DocumentObjects,
  Both,
  Count,
};

enum class CreativeCloneRotation : std::uint8_t {
  Degrees0,
  Degrees90,
  Degrees180,
  Degrees270,
  Count,
};

enum class CreativeCloneMirror : std::uint8_t {
  None,
  X,
  Z,
  XAndZ,
  Count,
};

// Authored objects retain their normal semantic placement rules. This policy
// explicitly owns only sparse voxel occupancy at the transformed destination.
enum class CreativeVolumeCloneVoxelOverlapPolicy : std::uint8_t {
  RejectOccupied,
  PreserveExisting,
  ReplaceExisting,
  Count,
};

[[nodiscard]] constexpr std::uint8_t creativeVolumeHollowThicknessCells(
    CreativeVolumeHollowThickness thickness) noexcept {
  switch (thickness) {
    case CreativeVolumeHollowThickness::OneCell: return 1U;
    case CreativeVolumeHollowThickness::TwoCells: return 2U;
    case CreativeVolumeHollowThickness::FourCells: return 4U;
    case CreativeVolumeHollowThickness::Count: break;
  }
  return 0U;
}

[[nodiscard]] constexpr bool creativeVolumeMemberMaskIncludesVoxels(
    CreativeVolumeMemberMask mask) noexcept {
  return mask == CreativeVolumeMemberMask::VoxelCells ||
         mask == CreativeVolumeMemberMask::Both;
}

[[nodiscard]] constexpr bool creativeVolumeMemberMaskIncludesObjects(
    CreativeVolumeMemberMask mask) noexcept {
  return mask == CreativeVolumeMemberMask::DocumentObjects ||
         mask == CreativeVolumeMemberMask::Both;
}

}  // namespace iggy3d::creative
