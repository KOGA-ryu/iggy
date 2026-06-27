#pragma once

#include <cstdint>
#include <string>

#include "app/iggy3d/gameplay/ProductActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/ascii_room/ProductAsciiRoomEditing.hpp"
#include "app/iggy3d/room_editor/AuthoringController.hpp"

namespace iggy3d {

struct ProductRoomEditingState {
  bool ready = false;
  std::string status = "not_started";
  std::string reasonCode = "not_started";
  ProductRoomAuthoringController controller;
  ProductRoomAuthoringSnapshot authoringSnapshot;
  ProductActiveRoomState activeRoom;
  ProductActiveRoomCollisionState activeRoomCollision;
  std::uint64_t documentFloorCount = 0;
  std::uint64_t documentWallCount = 0;
  std::uint64_t activeRoomStaticMeshCount = 0;
  std::uint64_t activeRoomSpatialSurfaceCount = 0;
  std::uint64_t activeRoomAuthoredFloorCount = 0;
  std::uint64_t activeRoomAuthoredWallCount = 0;
  std::uint64_t collisionQuerySurfaceCount = 0;
  std::uint64_t collisionWalkableSurfaceCount = 0;
  std::uint64_t collisionActorBlockerSurfaceCount = 0;
  std::uint64_t collisionProjectileBlockerSurfaceCount = 0;
  std::uint64_t undoDepth = 0;
  std::uint64_t redoDepth = 0;
};

struct ProductRoomEditingStartResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::string failedStage = "not_started";
  ProductAsciiRoomEditingResult asciiRoom;
  ProductRoomEditingState state;
};

struct ProductRoomEditingOperationResult {
  bool accepted = false;
  ProductRoomAuthoringInputSource inputSource = ProductRoomAuthoringInputSource::Script;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  RoomEditResult edit;
  ProductRoomAuthoringCommandResult command;
  ProductRoomEditingState state;
};

ProductRoomEditingState buildProductRoomEditingState(
    ProductRoomAuthoringController controller);

ProductRoomEditingStartResult startProductRoomEditingFromAscii(
    const ProductAsciiRoomAuthoringRequest& request);

ProductRoomEditingStartResult startProductRoomEditingFromActiveRoom(
    const ProductActiveRoomState& activeRoom);

ProductRoomEditingOperationResult applyProductRoomEditingCommand(
    ProductRoomEditingState& state,
    ProductRoomAuthoringInputSource source,
    const RoomEditCommand& command);

ProductRoomEditingOperationResult undoProductRoomEditing(
    ProductRoomEditingState& state,
    ProductRoomAuthoringInputSource source);

ProductRoomEditingOperationResult redoProductRoomEditing(
    ProductRoomEditingState& state,
    ProductRoomAuthoringInputSource source);

}  // namespace iggy3d
