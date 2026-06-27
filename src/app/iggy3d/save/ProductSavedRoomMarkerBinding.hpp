#pragma once

#include <cstdint>
#include <string>

#include "app/iggy3d/ProductActiveRoomState.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

struct ProductSavedRoomMarkerBindingResult {
  bool ok = false;
  bool requested = false;
  bool sessionReplaced = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string roomId = "none";
  std::uint64_t markerCount = 0;
  std::uint64_t seedEntityCount = 0;
  std::uint64_t addedEntityCount = 0;
  std::uint64_t existingEntityCount = 0;
  std::uint64_t addedObjectiveCount = 0;
  std::uint64_t existingObjectiveCount = 0;
  std::uint64_t addedCombatantCount = 0;
  std::uint64_t existingCombatantCount = 0;
  std::uint64_t pickupCount = 0;
  std::uint64_t doorCount = 0;
  std::uint64_t markerEntityCount = 0;
  std::uint64_t npcCount = 0;
  std::uint64_t previousHash = 0;
  std::uint64_t boundHash = 0;
};

ProductSavedRoomMarkerBindingResult bindSavedRoomMarkersToSession(
    const ProductActiveRoomState& activeRoom,
    Session& session);

}  // namespace iggy3d
