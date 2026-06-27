#include "app/iggy3d/room_editor/ProductRoomEditingState.hpp"

#include <utility>

namespace iggy3d {
namespace {

void fillCounts(ProductRoomEditingState& state) {
  state.documentFloorCount = state.authoringSnapshot.documentFloorCount;
  state.documentWallCount = state.authoringSnapshot.documentWallCount;
  state.activeRoomStaticMeshCount = state.activeRoom.staticMeshCount;
  state.activeRoomSpatialSurfaceCount = state.activeRoom.spatialSurfaceCount;
  state.activeRoomAuthoredFloorCount = state.activeRoom.authoredFloorCount;
  state.activeRoomAuthoredWallCount = state.activeRoom.authoredWallCount;
  state.collisionQuerySurfaceCount = state.activeRoomCollision.querySurfaceCount;
  state.collisionWalkableSurfaceCount = state.activeRoomCollision.walkableSurfaceCount;
  state.collisionActorBlockerSurfaceCount =
      state.activeRoomCollision.actorBlockerSurfaceCount;
  state.collisionProjectileBlockerSurfaceCount =
      state.activeRoomCollision.projectileBlockerSurfaceCount;
  state.undoDepth = state.authoringSnapshot.undoDepth;
  state.redoDepth = state.authoringSnapshot.redoDepth;
}

ProductRoomEditingOperationResult rejectedOperation(
    ProductRoomAuthoringInputSource source,
    std::string status,
    std::string reasonCode,
    ProductRoomEditingState state) {
  ProductRoomEditingOperationResult result;
  result.accepted = false;
  result.inputSource = source;
  result.status = std::move(status);
  result.reasonCode = std::move(reasonCode);
  result.state = std::move(state);
  return result;
}

ProductRoomEditingOperationResult rejectedCommandOperation(
    ProductRoomAuthoringInputSource source,
    ProductRoomAuthoringCommandResult command,
    ProductRoomEditingState state) {
  ProductRoomEditingOperationResult result;
  result.accepted = false;
  result.inputSource = source;
  result.status = command.status;
  result.reasonCode = command.reasonCode;
  result.edit = command.edit;
  result.command = std::move(command);
  result.state = std::move(state);
  return result;
}

ProductRoomEditingOperationResult acceptedOperation(
    ProductRoomAuthoringInputSource source,
    std::string status,
    ProductRoomAuthoringCommandResult command,
    ProductRoomEditingState state) {
  ProductRoomEditingOperationResult result;
  result.accepted = true;
  result.inputSource = source;
  result.status = std::move(status);
  result.reasonCode = result.status;
  result.edit = command.edit;
  result.command = std::move(command);
  result.state = std::move(state);
  return result;
}

ProductRoomEditingOperationResult finishCandidateOperation(
    ProductRoomEditingState& state,
    ProductRoomAuthoringInputSource source,
    std::string acceptedStatus,
    ProductRoomAuthoringController candidate,
    ProductRoomAuthoringCommandResult command) {
  if (!command.accepted) {
    return rejectedCommandOperation(source, std::move(command), state);
  }

  ProductRoomEditingState rebuilt =
      buildProductRoomEditingState(std::move(candidate));
  if (!rebuilt.ready) {
    ProductRoomEditingOperationResult result =
        rejectedOperation(source,
                          "product_room_editing_rebuild_failed",
                          rebuilt.reasonCode,
                          state);
    result.edit = command.edit;
    result.command = std::move(command);
    return result;
  }

  state = std::move(rebuilt);
  return acceptedOperation(source,
                           std::move(acceptedStatus),
                           std::move(command),
                           state);
}

}  // namespace

ProductRoomEditingState buildProductRoomEditingState(
    ProductRoomAuthoringController controller) {
  ProductRoomEditingState state;
  state.controller = std::move(controller);
  state.authoringSnapshot = state.controller.snapshot();
  fillCounts(state);

  if (!state.authoringSnapshot.ready) {
    state.ready = false;
    state.status = "product_room_editing_authoring_unavailable";
    state.reasonCode = state.authoringSnapshot.reasonCode;
    fillCounts(state);
    return state;
  }

  state.activeRoom =
      buildProductActiveRoomFromRoomAuthoringSnapshot(state.authoringSnapshot);
  fillCounts(state);
  if (!state.activeRoom.loaded) {
    state.ready = false;
    state.status = "product_room_editing_active_room_unavailable";
    state.reasonCode = state.activeRoom.reasonCode;
    return state;
  }

  state.activeRoomCollision = buildProductActiveRoomCollision(state.activeRoom);
  fillCounts(state);
  if (!state.activeRoomCollision.ready) {
    state.ready = false;
    state.status = "product_room_editing_collision_unavailable";
    state.reasonCode = state.activeRoomCollision.reasonCode;
    return state;
  }

  state.ready = true;
  state.status = "product_room_editing_ready";
  state.reasonCode = "product_room_editing_ready";
  fillCounts(state);
  return state;
}

ProductRoomEditingStartResult startProductRoomEditingFromAscii(
    const ProductAsciiRoomAuthoringRequest& request) {
  ProductRoomEditingStartResult result;
  result.asciiRoom = buildProductAsciiRoomEditing(request);
  result.failedStage = result.asciiRoom.failedStage;
  if (!result.asciiRoom.ok) {
    result.ok = false;
    result.status = result.asciiRoom.status;
    result.reasonCode = result.asciiRoom.reasonCode;
    result.state.status = result.status;
    result.state.reasonCode = result.reasonCode;
    return result;
  }

  result.state = buildProductRoomEditingState(result.asciiRoom.controller);
  if (!result.state.ready) {
    result.ok = false;
    result.failedStage = "room_editing_state";
    result.status = result.state.status;
    result.reasonCode = result.state.reasonCode;
    return result;
  }

  result.ok = true;
  result.status = "product_room_editing_started";
  result.reasonCode = "product_room_editing_started";
  result.failedStage = "none";
  return result;
}

ProductRoomEditingStartResult startProductRoomEditingFromActiveRoom(
    const ProductActiveRoomState& activeRoom) {
  ProductRoomEditingStartResult result;
  result.failedStage = "active_room";
  if (!activeRoom.loaded) {
    result.ok = false;
    result.status = "product_room_editing_active_room_unloaded";
    result.reasonCode = activeRoom.reasonCode.empty() ? result.status
                                                       : activeRoom.reasonCode;
    result.state.status = result.status;
    result.state.reasonCode = result.reasonCode;
    return result;
  }
  if (!activeRoom.hasAuthoredRoom || !activeRoom.authoredRoom.present) {
    result.ok = false;
    result.status = "product_room_editing_authored_room_missing";
    result.reasonCode = "product_room_editing_authored_room_missing";
    result.state.status = result.status;
    result.state.reasonCode = result.reasonCode;
    return result;
  }

  ProductRoomAuthoringController controller(
      buildEditableRoomDocumentFromAuthoredRoom(activeRoom.authoredRoom));
  result.state = buildProductRoomEditingState(std::move(controller));
  if (!result.state.ready) {
    result.ok = false;
    result.failedStage = "room_editing_state";
    result.status = result.state.status;
    result.reasonCode = result.state.reasonCode;
    return result;
  }

  result.ok = true;
  result.status = "product_room_editing_started_from_active_room";
  result.reasonCode = "product_room_editing_started_from_active_room";
  result.failedStage = "none";
  return result;
}

ProductRoomEditingOperationResult applyProductRoomEditingCommand(
    ProductRoomEditingState& state,
    ProductRoomAuthoringInputSource source,
    const RoomEditCommand& command) {
  if (!state.ready) {
    return rejectedOperation(source,
                             "product_room_editing_not_ready",
                             state.reasonCode,
                             state);
  }

  ProductRoomAuthoringController candidate = state.controller;
  ProductRoomAuthoringCommandResult applied = candidate.submit(source, command);
  return finishCandidateOperation(state,
                                  source,
                                  "product_room_editing_edit_applied",
                                  std::move(candidate),
                                  std::move(applied));
}

ProductRoomEditingOperationResult undoProductRoomEditing(
    ProductRoomEditingState& state,
    ProductRoomAuthoringInputSource source) {
  if (!state.ready) {
    return rejectedOperation(source,
                             "product_room_editing_not_ready",
                             state.reasonCode,
                             state);
  }

  ProductRoomAuthoringController candidate = state.controller;
  ProductRoomAuthoringCommandResult undone = candidate.undo(source);
  return finishCandidateOperation(state,
                                  source,
                                  "product_room_editing_undo_applied",
                                  std::move(candidate),
                                  std::move(undone));
}

ProductRoomEditingOperationResult redoProductRoomEditing(
    ProductRoomEditingState& state,
    ProductRoomAuthoringInputSource source) {
  if (!state.ready) {
    return rejectedOperation(source,
                             "product_room_editing_not_ready",
                             state.reasonCode,
                             state);
  }

  ProductRoomAuthoringController candidate = state.controller;
  ProductRoomAuthoringCommandResult redone = candidate.redo(source);
  return finishCandidateOperation(state,
                                  source,
                                  "product_room_editing_redo_applied",
                                  std::move(candidate),
                                  std::move(redone));
}

}  // namespace iggy3d
