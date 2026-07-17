#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <string>

namespace iggy3d::creative {

struct CreativeWorldLayoutObjectProvenance {
  bool owned = false;
  CreativeWorldLayoutTable table = CreativeWorldLayoutTable::None;
  std::size_t index = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRoomEdge roomEdge = CreativeWorldLayoutRoomEdge::Count;
  std::size_t contributorCount = 0U;
};

// Produces a stable source tag from the symbol's authored key rather than its
// current vector index. Empty means the table/index cannot own a 3D object.
[[nodiscard]] std::string creativeWorldLayoutProvenanceTag(
    const CreativeWorldLayout& layout,
    CreativeWorldLayoutTable table,
    std::size_t index);

[[nodiscard]] std::string creativeWorldLayoutRoomEdgeProvenanceTag(
    const CreativeWorldLayout& layout,
    std::size_t roomIndex,
    CreativeWorldLayoutRoomEdge edge);

// Resolves generated document geometry back to its authored layout symbol.
// Shared room walls choose the lowest-index contributor deterministically and
// report the complete contributor count.
[[nodiscard]] CreativeWorldLayoutObjectProvenance
resolveCreativeWorldLayoutObjectProvenance(
    const CreativeWorldLayout& layout,
    const CreativeObject& object);

// Uses a point expressed in layout grid cells to distinguish contiguous room
// edge contributors that were condensed into one generated wall. Overlapping
// contributors, such as a shared internal wall, retain deterministic ordering.
[[nodiscard]] CreativeWorldLayoutObjectProvenance
resolveCreativeWorldLayoutObjectProvenance(
    const CreativeWorldLayout& layout,
    const CreativeObject& object,
    CreativeVec3 sourcePointCells);

}  // namespace iggy3d::creative
