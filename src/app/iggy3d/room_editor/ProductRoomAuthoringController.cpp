#include "app/iggy3d/room_editor/ProductRoomAuthoringController.hpp"

#include <utility>

#include "app/iggy3d/room_editor/ProductRoomEditorActionController.hpp"
#include "app/iggy3d/room_editor/ProductRoomEditingState.hpp"
#include "app/input/ActionState.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "runtime/session/SessionState.hpp"

namespace iggy3d {
namespace {

std::uint64_t countProjectedMeshesWithRole(const SceneRoomProjection& room,
                                           const char* role) {
  std::uint64_t count = 0;
  for (const SceneRoomMeshItem& mesh : room.meshes) {
    if (mesh.role == role) {
      ++count;
    }
  }
  return count;
}

ProductRoomAuthoringCommandResult makeRejectedResult(
    ProductRoomAuthoringInputSource source,
    const char* status,
    const char* reasonCode,
    RoomEditResult edit,
    const ProductRoomAuthoringSnapshot& snapshot) {
  ProductRoomAuthoringCommandResult result;
  result.accepted = false;
  result.inputSource = source;
  result.status = status;
  result.reasonCode = reasonCode;
  result.edit = std::move(edit);
  result.snapshot = snapshot;
  return result;
}

ProductRoomAuthoringCommandResult makeAcceptedResult(
    ProductRoomAuthoringInputSource source,
    const char* status,
    RoomEditResult edit,
    const ProductRoomAuthoringSnapshot& snapshot) {
  ProductRoomAuthoringCommandResult result;
  result.accepted = true;
  result.inputSource = source;
  result.status = status;
  result.reasonCode = status;
  result.edit = std::move(edit);
  result.snapshot = snapshot;
  return result;
}

}  // namespace

const char* productRoomAuthoringInputSourceName(
    ProductRoomAuthoringInputSource source) {
  switch (source) {
    case ProductRoomAuthoringInputSource::Ai:
      return "ai";
    case ProductRoomAuthoringInputSource::Hotkey:
      return "hotkey";
    case ProductRoomAuthoringInputSource::Mouse:
      return "mouse";
    case ProductRoomAuthoringInputSource::Script:
      return "script";
  }
  return "unknown";
}

ProductRoomAuthoringSnapshot buildProductRoomAuthoringSnapshot(
    const EditableRoomSession& session) {
  ProductRoomAuthoringSnapshot snapshot;
  snapshot.document = session.document();
  snapshot.documentFloorCount = snapshot.document.floors.size();
  snapshot.documentWallCount = snapshot.document.walls.size();
  snapshot.undoDepth = session.undoDepth();
  snapshot.redoDepth = session.redoDepth();

  const RoomBakeResult bake = bakeEditableRoomDocument(snapshot.document);
  if (!bake.ok) {
    snapshot.ready = false;
    snapshot.status = "product_room_authoring_bake_failed";
    snapshot.reasonCode = bake.reasonCode;
    return snapshot;
  }

  snapshot.room = bake.room;
  snapshot.roomStaticMeshCount = snapshot.room.staticMeshes.size();
  snapshot.roomSpatialSurfaceCount = snapshot.room.spatialSurfaces.size();

  SessionState emptyState;
  const SceneProjectionResult projection = buildSceneProjection(emptyState, &snapshot.room);
  snapshot.roomProjection = projection.room;
  snapshot.projectedMeshCount = snapshot.roomProjection.meshes.size();
  snapshot.projectedFloorMeshCount =
      countProjectedMeshesWithRole(snapshot.roomProjection, "floor");
  snapshot.projectedWallMeshCount =
      countProjectedMeshesWithRole(snapshot.roomProjection, "wall");

  if (!snapshot.roomProjection.loaded) {
    snapshot.ready = false;
    snapshot.status = "product_room_authoring_projection_failed";
    snapshot.reasonCode = "product_room_authoring_projection_failed";
    return snapshot;
  }

  snapshot.ready = true;
  snapshot.status = "product_room_authoring_ready";
  snapshot.reasonCode = "product_room_authoring_ready";
  return snapshot;
}

ProductRoomAuthoringController::ProductRoomAuthoringController(
    EditableRoomDocument document)
    : session_(std::move(document)),
      snapshot_(buildProductRoomAuthoringSnapshot(session_)) {}

const ProductRoomAuthoringSnapshot& ProductRoomAuthoringController::snapshot() const {
  return snapshot_;
}

ProductRoomAuthoringCommandResult ProductRoomAuthoringController::submit(
    ProductRoomAuthoringInputSource source,
    const RoomEditCommand& command) {
  EditableRoomSession candidate = session_;
  RoomEditResult edit = candidate.submit(command);
  if (edit.status != RoomEditStatus::Applied) {
    return makeRejectedResult(source,
                              "product_room_authoring_edit_rejected",
                              edit.reasonCode,
                              std::move(edit),
                              snapshot_);
  }

  ProductRoomAuthoringSnapshot candidateSnapshot =
      buildProductRoomAuthoringSnapshot(candidate);
  if (!candidateSnapshot.ready) {
    return makeRejectedResult(source,
                              "product_room_authoring_rebuild_failed",
                              candidateSnapshot.reasonCode.c_str(),
                              std::move(edit),
                              snapshot_);
  }

  session_ = std::move(candidate);
  snapshot_ = std::move(candidateSnapshot);
  return makeAcceptedResult(source,
                            "product_room_authoring_edit_applied",
                            std::move(edit),
                            snapshot_);
}

ProductRoomAuthoringCommandResult ProductRoomAuthoringController::undo(
    ProductRoomAuthoringInputSource source) {
  EditableRoomSession candidate = session_;
  RoomEditResult edit = candidate.undo();
  if (edit.status != RoomEditStatus::UndoApplied) {
    return makeRejectedResult(source,
                              "product_room_authoring_undo_rejected",
                              edit.reasonCode,
                              std::move(edit),
                              snapshot_);
  }

  ProductRoomAuthoringSnapshot candidateSnapshot =
      buildProductRoomAuthoringSnapshot(candidate);
  if (!candidateSnapshot.ready) {
    return makeRejectedResult(source,
                              "product_room_authoring_rebuild_failed",
                              candidateSnapshot.reasonCode.c_str(),
                              std::move(edit),
                              snapshot_);
  }

  session_ = std::move(candidate);
  snapshot_ = std::move(candidateSnapshot);
  return makeAcceptedResult(source,
                            "product_room_authoring_undo_applied",
                            std::move(edit),
                            snapshot_);
}

ProductRoomAuthoringCommandResult ProductRoomAuthoringController::redo(
    ProductRoomAuthoringInputSource source) {
  EditableRoomSession candidate = session_;
  RoomEditResult edit = candidate.redo();
  if (edit.status != RoomEditStatus::RedoApplied) {
    return makeRejectedResult(source,
                              "product_room_authoring_redo_rejected",
                              edit.reasonCode,
                              std::move(edit),
                              snapshot_);
  }

  ProductRoomAuthoringSnapshot candidateSnapshot =
      buildProductRoomAuthoringSnapshot(candidate);
  if (!candidateSnapshot.ready) {
    return makeRejectedResult(source,
                              "product_room_authoring_rebuild_failed",
                              candidateSnapshot.reasonCode.c_str(),
                              std::move(edit),
                              snapshot_);
  }

  session_ = std::move(candidate);
  snapshot_ = std::move(candidateSnapshot);
  return makeAcceptedResult(source,
                            "product_room_authoring_redo_applied",
                            std::move(edit),
                            snapshot_);
}

ProductRoomEditingStartResult startProductRoomAuthoringFromAsciiDraft(
    ProductRoomAuthoringStartFromAsciiRequest request) {
  return startProductRoomEditingFromAscii(request.request);
}

ProductRoomEditingStartResult startProductRoomAuthoringFromActiveRoom(
    ProductRoomAuthoringStartFromActiveRoomRequest request) {
  return startProductRoomEditingFromActiveRoom(request.activeRoom);
}

ProductRoomEditingOperationResult applyProductRoomAuthoringEditCommand(
    ProductRoomAuthoringEditCommandRequest request) {
  return applyProductRoomEditingCommand(request.state, request.source, request.command);
}

ProductRoomEditingOperationResult undoProductRoomAuthoringEdit(
    ProductRoomAuthoringUndoRedoRequest request) {
  return undoProductRoomEditing(request.state, request.source);
}

ProductRoomEditingOperationResult redoProductRoomAuthoringEdit(
    ProductRoomAuthoringUndoRedoRequest request) {
  return redoProductRoomEditing(request.state, request.source);
}

ProductRoomEditorActionResult applyProductRoomAuthoringCursorPlace(
    ProductRoomAuthoringCursorPlaceRequest request) {
  ActionState actions;
  recordAction(actions, InputAction::EditorPlace, true, true, false, 1.0F);
  return applyProductRoomEditorActions(request.editing,
                                       request.cursor,
                                       actions,
                                       request.source);
}

}  // namespace iggy3d
