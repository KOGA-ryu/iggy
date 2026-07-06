#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

// Owned ascii-room activation state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: ascii_room. Behavior-identical.
struct ProductAsciiRoomActivationState {
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string roomId = "none";
  std::string packageId = "none";
  std::string scenarioId = "none";
  bool sessionCreated = false;
  bool playerSpawned = false;
  std::uint64_t playerCount = 0;
  std::uint64_t entityCount = 0;
  std::uint64_t npcCount = 0;
  std::uint64_t pickupCount = 0;
  std::uint64_t doorCount = 0;
  std::uint64_t markerEntityCount = 0;
  std::uint64_t objectiveCount = 0;
  std::uint64_t wallCount = 0;
  std::uint64_t markerCount = 0;
  std::uint64_t runtimeHash = 0;
};

}  // namespace iggy3d
