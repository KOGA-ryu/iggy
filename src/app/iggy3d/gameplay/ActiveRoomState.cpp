#include "app/iggy3d/gameplay/ActiveRoomState.hpp"

#include <cstdint>
#include <string>
#include <string_view>

#include "app/iggy3d/ascii_room/AsciiRoomToRoomAsset.hpp"
#include "app/iggy3d/EditableRoomToAuthoredRoom.hpp"
#include "app/iggy3d/ascii_room/Authoring.hpp"
#include "app/iggy3d/room_editor/AuthoringController.hpp"

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
  state.authoredMarkerCount = sizeReceiptValue(state.authoredRoom.markers.size());
}

std::string fallbackString(std::string_view value, std::string fallback) {
  return value.empty() ? std::move(fallback) : std::string(value);
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

ProductActiveRoomState buildProductActiveRoomFromPackageRoom(
    const RoomAsset& room,
    std::string_view packageId,
    std::string_view scenarioId) {
  ProductActiveRoomState state;
  state.loaded = true;
  state.status = "active_room_loaded";
  state.reasonCode = "active_room_loaded";
  state.source = "package_room";
  state.roomId = fallbackString(room.id, "package_room");
  state.sourceName =
      room.sourceFile.empty() ? fallbackString(packageId, "package") : room.sourceFile;
  state.sourceSubset =
      room.sourceSubset.empty() ? fallbackString(scenarioId, "scenario") : room.sourceSubset;
  state.room = room;
  fillRoomCounts(state);
  fillAuthoredCounts(state);
  return state;
}

ProductActiveRoomState buildProductActiveRoomFromSavedAuthoredRoom(
    const SaveAuthoredRoomSection& authoredRoom) {
  ProductActiveRoomState state;
  state.source = "saved_authored_room";
  state.roomId = fallbackString(authoredRoom.id, "saved_authored_room");
  state.sourceName =
      fallbackString(authoredRoom.sourceFile, "saved_authored_room");
  state.sourceSubset =
      fallbackString(authoredRoom.sourceSubset, "save_authored_room");
  state.authoredRoom = authoredRoom;
  fillAuthoredCounts(state);

  if (!authoredRoom.present) {
    state.loaded = false;
    state.status = "saved_authored_room_missing";
    state.reasonCode = "saved_authored_room_missing";
    return state;
  }

  AsciiRoomAuthoredRoomResult authored;
  authored.ok = true;
  authored.status = "ascii_room_ok";
  authored.reasonCode = "ascii_room_ok";
  authored.authoredRoom = authoredRoom;
  authored.floorCount = authoredRoom.floors.size();
  authored.wallCount = authoredRoom.walls.size();

  AsciiRoomToRoomAssetConfig config;
  config.roomId = state.roomId;
  config.sourceName = state.sourceName;
  config.source =
      fallbackString(authoredRoom.source, "iggy3d.saved_authored_room");
  config.sourceSubset = state.sourceSubset;
  const AsciiRoomToRoomAssetResult roomAsset =
      buildRoomAssetFromAsciiRoom(authored, config);
  if (!roomAsset.ok) {
    state.loaded = false;
    state.status = roomAsset.status;
    state.reasonCode = roomAsset.reasonCode;
    return state;
  }

  state.loaded = true;
  state.status = "active_room_loaded";
  state.reasonCode = "active_room_loaded";
  state.room = roomAsset.room;
  fillRoomCounts(state);
  return state;
}

ProductActiveRoomState buildProductActiveRoomFromRoomAuthoringSnapshot(
    const ProductRoomAuthoringSnapshot& snapshot) {
  ProductActiveRoomState state;
  state.source = "editable_room";
  state.roomId = fallbackString(snapshot.document.id, "editable_room");
  state.sourceName =
      fallbackString(snapshot.document.sourceFile, "editable_room");
  state.sourceSubset =
      fallbackString(snapshot.document.sourceSubset, "editable_room_authoring");
  state.status = snapshot.status.empty() ? "not_ready" : snapshot.status;
  state.reasonCode =
      snapshot.reasonCode.empty() ? "not_ready" : snapshot.reasonCode;

  if (!snapshot.ready) {
    state.loaded = false;
    return state;
  }

  state.loaded = true;
  state.status = "active_room_loaded";
  state.reasonCode = "active_room_loaded";
  state.room = snapshot.room;
  const EditableRoomToAuthoredRoomResult authored =
      buildAuthoredRoomFromEditableRoomDocument(snapshot.document);
  if (authored.ok) {
    state.authoredRoom = authored.authoredRoom;
  }
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
