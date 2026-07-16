#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <string>

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
  std::size_t failedIndex = kInvalidCreativeWorldLayoutIndex;
  std::string reasonCode = "creative_world_layout_rooms_not_requested";
};

[[nodiscard]] CreativeWorldLayoutRoomCompileResult
expandCreativeWorldLayoutRooms(const CreativeWorldLayout& layout);

}  // namespace iggy3d::creative
