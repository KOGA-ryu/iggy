#include "app/iggy3d/ProductActiveRoomCollision.hpp"

#include <cstddef>
#include <cstdint>

#include "runtime/collision/CollisionTypes.hpp"
#include "runtime/world/EntityState.hpp"

namespace iggy3d {
namespace {

std::uint64_t sizeReceiptValue(std::size_t value) {
  return static_cast<std::uint64_t>(value);
}

void copyActiveRoomCounts(const ProductActiveRoomState& activeRoom,
                          ProductActiveRoomCollisionState& collision) {
  collision.roomId = activeRoom.roomId;
  collision.spatialSurfaceCount = activeRoom.spatialSurfaceCount;
  collision.walkableSurfaceCount = activeRoom.walkableSurfaceCount;
  collision.actorBlockerSurfaceCount = activeRoom.actorBlockerSurfaceCount;
  collision.projectileBlockerSurfaceCount = activeRoom.projectileBlockerSurfaceCount;
}

void fillQueryCounts(ProductActiveRoomCollisionState& collision) {
  collision.querySurfaceCount = sizeReceiptValue(collision.surfaces.size());
  collision.walkableSurfaceCount = 0;
  collision.actorBlockerSurfaceCount = 0;
  collision.projectileBlockerSurfaceCount = 0;
  for (const CollisionSurfaceView& surface : collision.surfaces.surfaces()) {
    if (surface.role == CollisionSurfaceRole::Walkable) {
      ++collision.walkableSurfaceCount;
    }
    if (surface.blocksActor || surface.role == CollisionSurfaceRole::Blocker) {
      ++collision.actorBlockerSurfaceCount;
    }
    if (surface.blocksProjectile ||
        surface.role == CollisionSurfaceRole::ProjectileBlocker) {
      ++collision.projectileBlockerSurfaceCount;
    }
  }
}

bool surfaceBlocksDoor(const RoomSpatialSurface& surface,
                       const EntityState* owner) {
  return owner != nullptr && owner->kind == EntityKind::Door &&
         (surface.blocksActor || surface.blocksProjectile ||
          surface.role == RoomSpatialSurfaceRole::Blocker ||
          surface.role == RoomSpatialSurfaceRole::ProjectileBlocker);
}

RoomAsset roomWithRuntimeFilteredSurfaces(
    const ProductActiveRoomState& activeRoom,
    const SessionState* runtimeState,
    ProductActiveRoomCollisionState& collision) {
  RoomAsset room = activeRoom.room;
  room.spatialSurfaces.clear();
  room.spatialSurfaces.reserve(activeRoom.room.spatialSurfaces.size());

  for (const RoomSpatialSurface& surface : activeRoom.room.spatialSurfaces) {
    const EntityState* owner = nullptr;
    if (!surface.runtimeOwnerStableName.empty()) {
      ++collision.runtimeOwnedSurfaceCount;
      if (runtimeState != nullptr) {
        owner = runtimeState->world.findByStableName(surface.runtimeOwnerStableName);
      }
    }

    const bool doorBlocker = surfaceBlocksDoor(surface, owner);
    if (doorBlocker) {
      ++collision.doorBlockerSurfaceCount;
    }
    if (owner != nullptr && !owner->active) {
      ++collision.runtimeFilteredSurfaceCount;
      continue;
    }
    if (doorBlocker) {
      ++collision.activeDoorBlockerSurfaceCount;
    }
    room.spatialSurfaces.push_back(surface);
  }
  return room;
}

ProductActiveRoomCollisionState buildProductActiveRoomCollisionImpl(
    const ProductActiveRoomState& activeRoom,
    const SessionState* runtimeState) {
  ProductActiveRoomCollisionState collision;
  copyActiveRoomCounts(activeRoom, collision);
  if (!activeRoom.loaded) {
    collision.ready = false;
    collision.status = "active_room_collision_unavailable";
    collision.reasonCode =
        activeRoom.reasonCode.empty() ? "active_room_not_loaded" : activeRoom.reasonCode;
    return collision;
  }

  const RoomAsset filteredRoom =
      roomWithRuntimeFilteredSurfaces(activeRoom, runtimeState, collision);
  collision.surfaces = buildSpatialSurfaceSet(filteredRoom);
  fillQueryCounts(collision);
  if (collision.surfaces.empty()) {
    collision.ready = false;
    collision.status = "active_room_collision_unavailable";
    collision.reasonCode = "active_room_collision_missing_surfaces";
    return collision;
  }

  collision.ready = true;
  collision.status = "active_room_collision_ready";
  collision.reasonCode = "active_room_collision_ready";
  return collision;
}

}  // namespace

ProductActiveRoomCollisionState buildProductActiveRoomCollision(
    const ProductActiveRoomState& activeRoom) {
  return buildProductActiveRoomCollisionImpl(activeRoom, nullptr);
}

ProductActiveRoomCollisionState buildProductActiveRoomCollision(
    const ProductActiveRoomState& activeRoom,
    const SessionState& runtimeState) {
  return buildProductActiveRoomCollisionImpl(activeRoom, &runtimeState);
}

const SpatialSurfaceSet* productActiveRoomCollisionSurfaces(
    const ProductActiveRoomCollisionState& collision) {
  if (!collision.ready || collision.surfaces.empty()) {
    return nullptr;
  }
  return &collision.surfaces;
}

}  // namespace iggy3d
