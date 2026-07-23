#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace iggy3d::creative {

enum class CreativeWorldLayoutWallOperationStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  InvalidSourceTopology,
  DuplicateStableKey,
  SplitOutsideInterior,
  OpeningConflict,
  BoundaryCapacityExceeded,
  EdgesNotMergeable,
  AttributesDiffer,
  JunctionInUse,
  ResultingTopologyInvalid,
  Ready,
};

struct CreativeWorldLayoutWallSplitRequest {
  std::size_t topologyEdgeIndex = kInvalidCreativeWorldLayoutIndex;
  // Integer cells measured from the edge's canonical minimum endpoint.
  std::uint32_t offsetCells = 0U;
  std::string newVertexStableKey;
  std::string newEdgeStableKey;
};

struct CreativeWorldLayoutWallMergeRequest {
  // The primary edge retains its stable identity and wall attributes.
  std::size_t primaryTopologyEdgeIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t secondaryTopologyEdgeIndex = kInvalidCreativeWorldLayoutIndex;
};

struct CreativeWorldLayoutWallOperationResult {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeWorldLayoutWallOperationStatus status =
      CreativeWorldLayoutWallOperationStatus::NotRequested;
  std::size_t topologyEdgeIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t newTopologyEdgeIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t failedOpeningIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayout edited;
  std::vector<std::size_t> sourceToEditedVertexIndices;
  std::vector<std::size_t> sourceToEditedEdgeIndices;
  std::string reasonCode =
      "creative_world_layout_wall_operation_not_requested";
};

// The source edge keeps its identity and owns the canonical-minimum span. The
// appended edge owns the canonical-maximum span. Hosted openings stay on their
// physical side; a split through an opening rejects without partial output.
[[nodiscard]] CreativeWorldLayoutWallOperationResult
splitCreativeWorldLayoutWall(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutWallSplitRequest& request);

// Merges two adjacent collinear edges with equal ownership and attributes.
// The primary edge survives. A junction used by any third wall is structural
// and therefore cannot be removed by this operation.
[[nodiscard]] CreativeWorldLayoutWallOperationResult
mergeCreativeWorldLayoutWalls(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutWallMergeRequest& request);

}  // namespace iggy3d::creative
