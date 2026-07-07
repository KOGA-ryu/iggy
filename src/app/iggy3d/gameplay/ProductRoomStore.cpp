#include "app/iggy3d/gameplay/ProductRoomStore.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"

namespace iggy3d {

ProductActiveRoomState& activeRoom(ProductAppWindowState& window) noexcept {
  return window.room.activeRoom;
}

const ProductActiveRoomState& activeRoom(
    const ProductAppWindowState& window) noexcept {
  return window.room.activeRoom;
}

std::uint64_t& activeRoomRevision(ProductAppWindowState& window) noexcept {
  return window.room.activeRoomRevision;
}

const std::uint64_t& activeRoomRevision(
    const ProductAppWindowState& window) noexcept {
  return window.room.activeRoomRevision;
}

ProductActiveRoomCollisionState& activeRoomCollision(
    ProductAppWindowState& window) noexcept {
  return window.room.activeRoomCollision;
}

const ProductActiveRoomCollisionState& activeRoomCollision(
    const ProductAppWindowState& window) noexcept {
  return window.room.activeRoomCollision;
}

ProductActiveRoomCollisionFreshnessResult& activeRoomCollisionFreshness(
    ProductAppWindowState& window) noexcept {
  return window.room.activeRoomCollisionFreshness;
}

const ProductActiveRoomCollisionFreshnessResult& activeRoomCollisionFreshness(
    const ProductAppWindowState& window) noexcept {
  return window.room.activeRoomCollisionFreshness;
}

}  // namespace iggy3d
