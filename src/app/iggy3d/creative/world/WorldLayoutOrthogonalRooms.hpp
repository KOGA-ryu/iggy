#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::size_t
    kCreativeWorldLayoutMaximumRoomBoundaryEdgeCount = 256U;

enum class CreativeWorldLayoutRoomGraphStatus : std::uint8_t {
  NotRequested,
  InvalidLayout,
  InvalidRoom,
  InvalidVertex,
  InvalidEdge,
  InvalidBoundary,
  BoundaryCapacityExceeded,
  NonOrthogonal,
  OpenBoundary,
  SelfIntersection,
  DisconnectedRoom,
  OverlappingRooms,
  OpeningHostInvalid,
  Ready,
};

struct CreativeWorldLayoutRoomGraphRange {
  std::size_t firstBoundary = 0U;
  std::size_t boundaryCount = 0U;
  std::size_t firstSurfaceRect = 0U;
  std::size_t surfaceRectCount = 0U;
};

// Canonical control-path view consumed by projection and compilation. Boundary
// rows are grouped by room and ordered counter-clockwise. Surface rectangles
// are exact, non-overlapping decompositions of the same room polygons.
struct CreativeWorldLayoutRoomGraph {
  bool requested = false;
  bool accepted = false;
  bool sourceWasExplicit = false;
  CreativeWorldLayoutRoomGraphStatus status =
      CreativeWorldLayoutRoomGraphStatus::NotRequested;
  std::size_t failedRoomIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t failedVertexIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t failedEdgeIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t failedBoundaryIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t failedOpeningIndex = kInvalidCreativeWorldLayoutIndex;
  std::vector<CreativeWorldLayoutTopologyVertex> vertices;
  std::vector<CreativeWorldLayoutTopologyEdge> edges;
  std::vector<CreativeWorldLayoutRoomBoundary> boundaries;
  std::vector<CreativeWorldLayoutRoomGraphRange> rooms;
  std::vector<CreativeWorldLayoutRect> roomBounds;
  std::vector<CreativeWorldLayoutRect> surfaceRects;
  std::string reasonCode = "creative_world_layout_room_graph_not_requested";
};

struct CreativeWorldLayoutRoomGraphMaterializeResult {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeWorldLayoutRoomGraphStatus status =
      CreativeWorldLayoutRoomGraphStatus::NotRequested;
  std::size_t failedOpeningIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayout edited;
  std::string reasonCode =
      "creative_world_layout_room_graph_materialize_not_requested";
};

[[nodiscard]] CreativeWorldLayoutRoomGraph
buildCreativeWorldLayoutRoomGraph(const CreativeWorldLayout& layout);

[[nodiscard]] std::span<const CreativeWorldLayoutRoomBoundary>
creativeWorldLayoutRoomBoundaries(
    const CreativeWorldLayoutRoomGraph& graph,
    std::size_t roomIndex) noexcept;

[[nodiscard]] std::span<const CreativeWorldLayoutRect>
creativeWorldLayoutRoomSurfaceRects(
    const CreativeWorldLayoutRoomGraph& graph,
    std::size_t roomIndex) noexcept;

[[nodiscard]] bool creativeWorldLayoutRoomContainsPoint(
    const CreativeWorldLayoutRoomGraph& graph,
    std::size_t roomIndex,
    CreativeTerrainCoord2 point) noexcept;

[[nodiscard]] bool creativeWorldLayoutRoomContainsRect(
    const CreativeWorldLayoutRoomGraph& graph,
    std::size_t roomIndex,
    CreativeWorldLayoutRect rect) noexcept;

// Converts a legacy rectangle layout to explicit schema-12 graph tables and
// upgrades RoomEdge openings to stable shared-edge hosts. Explicit valid input
// is returned unchanged.
[[nodiscard]] CreativeWorldLayoutRoomGraphMaterializeResult
materializeCreativeWorldLayoutRoomGraph(const CreativeWorldLayout& source);

}  // namespace iggy3d::creative
