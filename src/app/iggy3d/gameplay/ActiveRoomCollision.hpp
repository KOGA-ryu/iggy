#pragma once

#include <cstdint>
#include <string>

#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/session/SessionState.hpp"

namespace iggy3d {

struct ProductActiveRoomCollisionState {
  bool ready = false;
  std::string status = "not_ready";
  std::string reasonCode = "not_loaded";
  std::string roomId = "none";
  std::uint64_t spatialSurfaceCount = 0;
  std::uint64_t walkableSurfaceCount = 0;
  std::uint64_t actorBlockerSurfaceCount = 0;
  std::uint64_t projectileBlockerSurfaceCount = 0;
  std::uint64_t querySurfaceCount = 0;
  std::uint64_t runtimeOwnedSurfaceCount = 0;
  std::uint64_t runtimeFilteredSurfaceCount = 0;
  std::uint64_t doorBlockerSurfaceCount = 0;
  std::uint64_t activeDoorBlockerSurfaceCount = 0;
  std::uint64_t bakedFromRoomRevision = 0;
  std::uint64_t bakedFromSessionHash = 0;
  SpatialSurfaceSet surfaces;
};

ProductActiveRoomCollisionState buildProductActiveRoomCollision(
    const ProductActiveRoomState& activeRoom);

ProductActiveRoomCollisionState buildProductActiveRoomCollision(
    const ProductActiveRoomState& activeRoom,
    const SessionState& runtimeState);

const SpatialSurfaceSet* productActiveRoomCollisionSurfaces(
    const ProductActiveRoomCollisionState& collision);

}  // namespace iggy3d
