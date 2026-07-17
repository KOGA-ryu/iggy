#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace iggy3d::creative {

enum class CreativeWorldLayoutRoomCompileStatus : std::uint8_t {
  NotRequested,
  Ready,
  InvalidLevel,
  InvalidRoom,
  OverlappingRooms,
  InvalidOpeningHost,
  CapacityExceeded,
};

struct CreativeWorldLayoutRoomCompileResult {
  bool accepted = false;
  CreativeWorldLayoutRoomCompileStatus status =
      CreativeWorldLayoutRoomCompileStatus::NotRequested;
  CreativeWorldLayout expanded;
  struct WallContributor {
    std::size_t roomIndex = kInvalidCreativeWorldLayoutIndex;
    CreativeWorldLayoutRoomEdge roomEdge = CreativeWorldLayoutRoomEdge::Count;
  };
  struct WallProvenance {
    std::vector<WallContributor> contributors;
  };
  // Aligned one-for-one with expanded.walls. Explicit walls have no room
  // contributors; merged room-shell walls retain every authored edge.
  std::vector<WallProvenance> wallProvenance;
  std::size_t failedIndex = kInvalidCreativeWorldLayoutIndex;
  std::string reasonCode = "creative_world_layout_rooms_not_requested";
};

struct CreativeWorldLayoutSharedRoomEdgeSpan {
  std::size_t firstRoomIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRoomEdge firstRoomEdge =
      CreativeWorldLayoutRoomEdge::Count;
  std::size_t secondRoomIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRoomEdge secondRoomEdge =
      CreativeWorldLayoutRoomEdge::Count;
  CreativeTerrainCoord2 start;
  CreativeTerrainCoord2 end;
};

[[nodiscard]] CreativeWorldLayoutRoomCompileResult
expandCreativeWorldLayoutRooms(const CreativeWorldLayout& layout);

// O(room_count^2) control-path inspection. Spans are emitted once in stable
// room-index order and only when the room compiler would merge both edges.
[[nodiscard]] std::vector<CreativeWorldLayoutSharedRoomEdgeSpan>
inspectCreativeWorldLayoutSharedRoomEdges(
    const CreativeWorldLayout& layout);

[[nodiscard]] bool creativeWorldLayoutRoomEdgeIntervalIsShared(
    const CreativeWorldLayout& layout, std::size_t roomIndex,
    CreativeWorldLayoutRoomEdge roomEdge, double centerOffsetCells,
    double widthCells);

[[nodiscard]] bool creativeWorldLayoutHasInteriorRoomWindow(
    const CreativeWorldLayout& layout);

[[nodiscard]] CreativeRectangularRoomGeometryPlan
planCreativeWorldLayoutRoomGeometry(
    const CreativeGridSettings& grid, const CreativeWorldLayout& layout,
    std::size_t roomIndex) noexcept;

}  // namespace iggy3d::creative
