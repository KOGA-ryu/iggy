#pragma once

#include <cstdint>

#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"

namespace iggy3d {

struct ProductAppWindowState;

struct ProductRoomStore {
  ProductActiveRoomState activeRoom;
  // Window-owned monotonic room generation; whole activeRoom copies cannot stomp it.
  std::uint64_t activeRoomRevision = 0;
  ProductActiveRoomCollisionState activeRoomCollision;
  ProductActiveRoomCollisionFreshnessResult activeRoomCollisionFreshness;
};

ProductActiveRoomState& activeRoom(ProductAppWindowState& window) noexcept;
const ProductActiveRoomState& activeRoom(
    const ProductAppWindowState& window) noexcept;

std::uint64_t& activeRoomRevision(ProductAppWindowState& window) noexcept;
const std::uint64_t& activeRoomRevision(
    const ProductAppWindowState& window) noexcept;

ProductActiveRoomCollisionState& activeRoomCollision(
    ProductAppWindowState& window) noexcept;
const ProductActiveRoomCollisionState& activeRoomCollision(
    const ProductAppWindowState& window) noexcept;

ProductActiveRoomCollisionFreshnessResult& activeRoomCollisionFreshness(
    ProductAppWindowState& window) noexcept;
const ProductActiveRoomCollisionFreshnessResult& activeRoomCollisionFreshness(
    const ProductAppWindowState& window) noexcept;

}  // namespace iggy3d
