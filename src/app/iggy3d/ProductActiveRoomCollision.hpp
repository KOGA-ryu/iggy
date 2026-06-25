#pragma once

#include <cstdint>
#include <string>

#include "app/iggy3d/ProductActiveRoomState.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"

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
  SpatialSurfaceSet surfaces;
};

ProductActiveRoomCollisionState buildProductActiveRoomCollision(
    const ProductActiveRoomState& activeRoom);

const SpatialSurfaceSet* productActiveRoomCollisionSurfaces(
    const ProductActiveRoomCollisionState& collision);

}  // namespace iggy3d
