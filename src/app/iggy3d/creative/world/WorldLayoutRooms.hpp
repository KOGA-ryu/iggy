#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace iggy3d::creative {

enum class CreativeWorldLayoutRoomCompileStatus : std::uint8_t {
  NotRequested,
  Ready,
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

[[nodiscard]] CreativeWorldLayoutRoomCompileResult
expandCreativeWorldLayoutRooms(const CreativeWorldLayout& layout);

[[nodiscard]] CreativeRectangularRoomGeometryPlan
planCreativeWorldLayoutRoomGeometry(
    const CreativeGridSettings& grid,
    const CreativeWorldLayoutRoom& room) noexcept;

}  // namespace iggy3d::creative
