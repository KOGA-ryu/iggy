#include "app/iggy3d/ProductActiveRoomState.hpp"

#include <cstdint>
#include <string>

#include "app/iggy3d/ProductAsciiRoomAuthoring.hpp"

namespace iggy3d {
namespace {

std::uint64_t sizeReceiptValue(std::size_t value) {
  return static_cast<std::uint64_t>(value);
}

bool isActorBlocker(const RoomSpatialSurface& surface) {
  return surface.blocksActor || surface.role == RoomSpatialSurfaceRole::Blocker;
}

bool isProjectileBlocker(const RoomSpatialSurface& surface) {
  return surface.blocksProjectile ||
         surface.role == RoomSpatialSurfaceRole::ProjectileBlocker;
}

void fillRoomCounts(ProductActiveRoomState& state) {
  state.staticMeshCount = sizeReceiptValue(state.room.staticMeshes.size());
  state.anchorCount = sizeReceiptValue(state.room.anchors.size());
  state.openingCount = sizeReceiptValue(state.room.openings.size());
  state.spatialSurfaceCount = sizeReceiptValue(state.room.spatialSurfaces.size());
  state.walkableSurfaceCount = 0;
  state.actorBlockerSurfaceCount = 0;
  state.projectileBlockerSurfaceCount = 0;
  for (const RoomSpatialSurface& surface : state.room.spatialSurfaces) {
    if (surface.role == RoomSpatialSurfaceRole::Walkable) {
      ++state.walkableSurfaceCount;
    }
    if (isActorBlocker(surface)) {
      ++state.actorBlockerSurfaceCount;
    }
    if (isProjectileBlocker(surface)) {
      ++state.projectileBlockerSurfaceCount;
    }
  }
}

void fillAuthoredCounts(ProductActiveRoomState& state) {
  state.hasAuthoredRoom = state.authoredRoom.present;
  state.authoredFloorCount = sizeReceiptValue(state.authoredRoom.floors.size());
  state.authoredWallCount = sizeReceiptValue(state.authoredRoom.walls.size());
}

}  // namespace

ProductActiveRoomState buildProductActiveRoomFromAsciiAuthoring(
    const ProductAsciiRoomAuthoringRequest& request,
    const ProductAsciiRoomAuthoringResult& authoring) {
  ProductActiveRoomState state;
  state.source = "ascii_room";
  state.roomId = request.roomId.empty() ? std::string{"ascii_room"} : request.roomId;
  state.sourceName =
      request.sourceName.empty() ? std::string{"inline_ascii_room"} : request.sourceName;
  state.sourceSubset =
      request.sourceSubset.empty() ? std::string{"ascii_room_authoring"}
                                   : request.sourceSubset;
  state.status = authoring.status;
  state.reasonCode = authoring.reasonCode;

  if (!authoring.ok) {
    state.loaded = false;
    return state;
  }

  state.loaded = true;
  state.status = "active_room_loaded";
  state.reasonCode = "active_room_loaded";
  state.room = authoring.roomAsset.room;
  state.authoredRoom = authoring.authoredRoom.authoredRoom;
  if (!state.room.id.empty()) {
    state.roomId = state.room.id;
  }
  if (!state.room.sourceFile.empty()) {
    state.sourceName = state.room.sourceFile;
  }
  if (!state.room.sourceSubset.empty()) {
    state.sourceSubset = state.room.sourceSubset;
  }
  fillRoomCounts(state);
  fillAuthoredCounts(state);
  return state;
}

}  // namespace iggy3d
