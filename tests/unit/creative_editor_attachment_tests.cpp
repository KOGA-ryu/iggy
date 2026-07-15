#include "EditorEdits.hpp"
#include "EditorObjectActions.hpp"
#include "EditorState.hpp"
#include "EditorToolOptions.hpp"
#include "app/iggy3d/creative/Facade.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {
namespace cr = iggy3d::creative;
namespace app = iggy3d_creative_app;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

struct AttachedDoorPair {
  cr::CreativeObjectId frameId = cr::kInvalidObjectId;
  cr::CreativeObjectId doorId = cr::kInvalidObjectId;
};

AttachedDoorPair createAttachedDoorPair(cr::Facade& facade) {
  cr::CreativeDocumentCreateRequest frameRequest;
  frameRequest.kind = cr::CreativeObjectKind::Prop;
  frameRequest.name = "Attached Door Frame";
  frameRequest.assetId = "door_frame";
  frameRequest.transform.position = {2.0, 0.0, 3.0};
  frameRequest.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt frame =
      facade.createDocumentObject(frameRequest);

  cr::CreativeDocumentCreateRequest doorRequest;
  doorRequest.kind = cr::CreativeObjectKind::Door;
  doorRequest.name = "Attached Door Leaf";
  doorRequest.assetId = "door_leaf";
  doorRequest.transform.position = {2.5, 0.0, 3.0};
  doorRequest.hasTransformOverride = true;
  doorRequest.parentId = frame.objectId;
  doorRequest.attachmentSocket = "door_frame";
  const cr::CreativeDocumentCreateReceipt door =
      facade.createDocumentObject(doorRequest);
  return {frame.accepted ? frame.objectId : cr::kInvalidObjectId,
          door.accepted ? door.objectId : cr::kInvalidObjectId};
}

bool sameTransform(const cr::CreativeTransform& lhs,
                   const cr::CreativeTransform& rhs) {
  return cr::creativeVec3ExactlyEqual(lhs.position, rhs.position) &&
         cr::creativeVec3ExactlyEqual(lhs.rotationEulerRadians,
                                      rhs.rotationEulerRadians) &&
         cr::creativeVec3ExactlyEqual(lhs.scale, rhs.scale);
}

void select(cr::Facade& facade, cr::CreativeObjectId objectId) {
  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerPress;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = static_cast<cr::Id>(objectId);
  static_cast<void>(facade.dispatchToolInput(input));
}

bool installDocument(cr::CreativeAppState& appState,
                     cr::CreativeDocumentId documentId,
                     std::string_view name) {
  cr::CreativeDocument document =
      cr::CreativeDocument::create(std::string(name));
  if (!document.assignId(documentId)) {
    return false;
  }
  return appState.facade.installDocument(std::move(document)).accepted;
}

bool attachedChildrenFollowParentTransformsAsOneHistoryStep() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, 316U, "Attachment Transform"),
              "attachment transform document installed")) {
    return false;
  }
  const AttachedDoorPair pair = createAttachedDoorPair(appState.facade);
  if (!expect(pair.frameId != cr::kInvalidObjectId &&
                  pair.doorId != cr::kInvalidObjectId,
              "attachment transform pair created")) {
    return false;
  }
  appState.history = {};
  select(appState.facade, pair.frameId);

  cr::CreativeTransformCommandRequest translate;
  translate.kind = cr::CreativeTransformCommandKind::Translate;
  translate.translation = {1.0, 2.0, -1.0};
  const cr::CreativeTransformCommandReceipt translated =
      app::transformSelectedObjectsWithUndo(
          appState, appState.history, translate,
          "test_attached_parent_translate");
  const cr::CreativeObject* translatedFrame =
      appState.facade.findObject(pair.frameId);
  const cr::CreativeObject* translatedDoor =
      appState.facade.findObject(pair.doorId);
  if (!expect(translated.accepted && translated.changed &&
                  translated.objectCount == 2U &&
                  translatedFrame != nullptr && translatedDoor != nullptr &&
                  cr::creativeVec3ExactlyEqual(
                      translatedFrame->transform.position, {3.0, 2.0, 2.0}) &&
                  cr::creativeVec3ExactlyEqual(
                      translatedDoor->transform.position, {3.5, 2.0, 2.0}) &&
                  translatedDoor->parentId == pair.frameId &&
                  translatedDoor->attachmentSocket == "door_frame" &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "moving an attachment parent carries its child in one edit")) {
    return false;
  }

  cr::CreativeTransformCommandRequest rotate;
  rotate.kind = cr::CreativeTransformCommandKind::RotateYaw;
  rotate.yawDegrees = 90.0;
  const cr::CreativeTransformCommandReceipt rotated =
      app::transformSelectedObjectsWithUndo(
          appState, appState.history, rotate,
          "test_attached_parent_rotate");
  cr::CreativeTransformCommandRequest scale;
  scale.kind = cr::CreativeTransformCommandKind::Scale;
  scale.scaleFactor = {2.0, 2.0, 2.0};
  const cr::CreativeTransformCommandReceipt scaled =
      app::transformSelectedObjectsWithUndo(
          appState, appState.history, scale,
          "test_attached_parent_scale");
  const cr::CreativeObject* finalFrame =
      appState.facade.findObject(pair.frameId);
  const cr::CreativeObject* finalDoor =
      appState.facade.findObject(pair.doorId);
  const cr::CreativeTransform finalFrameTransform =
      finalFrame != nullptr ? finalFrame->transform : cr::CreativeTransform{};
  const cr::CreativeTransform finalDoorTransform =
      finalDoor != nullptr ? finalDoor->transform : cr::CreativeTransform{};
  const bool finalHierarchyTransformed =
      rotated.accepted && rotated.objectCount == 2U && scaled.accepted &&
      scaled.objectCount == 2U && finalFrame != nullptr &&
      finalDoor != nullptr &&
      cr::creativeVec3ExactlyEqual(finalFrame->transform.scale,
                                   {2.0, 2.0, 2.0}) &&
      cr::creativeVec3ExactlyEqual(finalDoor->transform.scale,
                                   {2.0, 2.0, 2.0}) &&
      finalDoor->parentId == pair.frameId;
  const bool undone = app::undoLastEdit(appState, "test_undo_attached_scale");
  const cr::CreativeObject* unscaledFrame =
      appState.facade.findObject(pair.frameId);
  const cr::CreativeObject* unscaledDoor =
      appState.facade.findObject(pair.doorId);
  const bool hierarchyUnscaled =
      undone && unscaledFrame != nullptr && unscaledDoor != nullptr &&
      cr::creativeVec3ExactlyEqual(unscaledFrame->transform.scale,
                                   {1.0, 1.0, 1.0}) &&
      cr::creativeVec3ExactlyEqual(unscaledDoor->transform.scale,
                                   {1.0, 1.0, 1.0});
  const bool redone = app::redoLastEdit(appState, "test_redo_attached_scale");
  const cr::CreativeObject* redoneFrame =
      appState.facade.findObject(pair.frameId);
  const cr::CreativeObject* redoneDoor =
      appState.facade.findObject(pair.doorId);

  return expect(finalHierarchyTransformed,
                "rotation and scale expand through the attachment hierarchy") &&
         expect(hierarchyUnscaled && redone && redoneFrame != nullptr &&
                    redoneDoor != nullptr &&
                    sameTransform(redoneFrame->transform,
                                  finalFrameTransform) &&
                    sameTransform(redoneDoor->transform,
                                  finalDoorTransform),
                "undo and redo restore both attachment transforms atomically");
}

bool detachActionPreservesWorldPoseAndRestoresRelationship() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, 317U, "Detach"),
              "detach document installed")) {
    return false;
  }
  const AttachedDoorPair pair = createAttachedDoorPair(appState.facade);
  if (!expect(pair.frameId != cr::kInvalidObjectId &&
                  pair.doorId != cr::kInvalidObjectId,
              "detach pair created")) {
    return false;
  }
  appState.history = {};
  select(appState.facade, pair.doorId);
  const cr::CreativeObject* before = appState.facade.findObject(pair.doorId);
  const cr::CreativeTransform worldBefore =
      before != nullptr ? before->transform : cr::CreativeTransform{};

  app::CreativeEditorState editor;
  editor.toolOptions.targetEntry = {cr::CreativeHeldItemKind::ObjectMove,
                                    cr::CreativeObjectKind::Unknown};
  editor.toolOptions.draft = editor.toolSettings;
  editor.toolOptions.commands = app::creativeEditorToolOptionCommandsForEntry(
      editor.toolOptions.targetEntry);
  editor.toolOptions.open = true;
  app::refreshCreativeEditorObjectActionContext(
      appState, editor.authoredAssets, editor.toolOptions);
  const auto end = editor.toolOptions.commands.ids.begin() +
                   static_cast<std::ptrdiff_t>(
                       editor.toolOptions.commands.count);
  const auto detach = std::find(
      editor.toolOptions.commands.ids.begin(), end,
      app::CreativeEditorToolOptionsCommandId::DetachAttachment);
  if (!expect(detach != end, "object actions expose detach")) {
    return false;
  }
  editor.toolOptions.selectedIndex = static_cast<std::size_t>(
      detach - editor.toolOptions.commands.ids.begin());
  const bool enabled = app::creativeEditorObjectActionEnabled(
      editor, editor.toolOptions,
      app::CreativeEditorToolOptionsCommandId::DetachAttachment);
  const std::string value = app::creativeEditorObjectActionValueLabel(
      editor.toolOptions,
      app::CreativeEditorToolOptionsCommandId::DetachAttachment);
  const bool detached = app::activateCreativeEditorObjectAction(
      appState, editor,
      app::CreativeEditorToolOptionsCommandId::DetachAttachment);
  const cr::CreativeObject* after = appState.facade.findObject(pair.doorId);
  const bool detachedWorldPosePreserved =
      enabled && value == "FROM door_frame" && detached && after != nullptr &&
      !after->parentId.has_value() && after->attachmentSocket.empty() &&
      sameTransform(after->transform, worldBefore) &&
      cr::creativeUndoDepth(appState.history) == 1U;
  const bool undone = app::undoLastEdit(appState, "test_undo_detach");
  const cr::CreativeObject* restored = appState.facade.findObject(pair.doorId);
  const bool restoredRelationship =
      restored != nullptr && restored->parentId == pair.frameId &&
      restored->attachmentSocket == "door_frame" &&
      sameTransform(restored->transform, worldBefore);
  const bool redone = app::redoLastEdit(appState, "test_redo_detach");
  const cr::CreativeObject* redetached =
      appState.facade.findObject(pair.doorId);

  return expect(detachedWorldPosePreserved,
                "detach action frees the socket without moving the child") &&
         expect(undone && restoredRelationship && redone &&
                    redetached != nullptr &&
                    !redetached->parentId.has_value() &&
                    redetached->attachmentSocket.empty() &&
                    sameTransform(redetached->transform, worldBefore),
                "detach relationship is one undoable and redoable edit");
}

bool deletingAttachmentParentCascadesAsOneUndoableEdit() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, 318U, "Delete Attachment"),
              "delete attachment document installed")) {
    return false;
  }
  const AttachedDoorPair pair = createAttachedDoorPair(appState.facade);
  appState.history = {};
  select(appState.facade, pair.frameId);

  const cr::CreativeDocumentRemoveReceipt removed = app::deleteSelectedObject(
      appState, "test_delete_attachment_parent", &appState.history);
  const bool undone =
      app::undoLastEdit(appState, "test_undo_attachment_delete");
  const cr::CreativeObject* restoredDoor =
      appState.facade.findObject(pair.doorId);
  const bool restored = appState.facade.findObject(pair.frameId) != nullptr &&
                        restoredDoor != nullptr &&
                        restoredDoor->parentId == pair.frameId &&
                        restoredDoor->attachmentSocket == "door_frame";
  const bool redone =
      app::redoLastEdit(appState, "test_redo_attachment_delete");

  return expect(removed.accepted && removed.objectRemoved &&
                    removed.reasonCode == "object_hierarchy_removed" &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "deleting an attachment parent records one cascade edit") &&
         expect(undone && restored && redone &&
                    appState.facade.findObject(pair.frameId) == nullptr &&
                    appState.facade.findObject(pair.doorId) == nullptr,
                "cascade delete undo and redo restore or remove the full pair");
}

bool lockedAttachmentChildRejectsCascadeWithoutPartialDelete() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, 319U, "Locked Attachment Delete"),
              "locked attachment document installed")) {
    return false;
  }
  const AttachedDoorPair pair = createAttachedDoorPair(appState.facade);
  const cr::CreativeDocumentMutationReceipt locked =
      cr::setDocumentObjectLocked(
          appState.facade.documentForPersistence(), pair.doorId, true);
  appState.history = {};
  select(appState.facade, pair.frameId);

  const cr::CreativeDocumentRemoveReceipt removed = app::deleteSelectedObject(
      appState, "test_reject_locked_attachment_delete", &appState.history);
  const cr::CreativeObject* frame = appState.facade.findObject(pair.frameId);
  const cr::CreativeObject* door = appState.facade.findObject(pair.doorId);
  return expect(
      cr::documentMutationSucceeded(locked.status) && !removed.accepted &&
          !removed.changed &&
          removed.status == cr::CreativeDocumentRemoveStatus::LockedObject &&
          frame != nullptr && door != nullptr &&
          door->parentId == pair.frameId &&
          cr::creativeUndoDepth(appState.history) == 0U,
      "locked attachment child rejects cascade without partial deletion");
}

}  // namespace

int main() {
  return attachedChildrenFollowParentTransformsAsOneHistoryStep() &&
                 detachActionPreservesWorldPoseAndRestoresRelationship() &&
                 deletingAttachmentParentCascadesAsOneUndoableEdit() &&
                 lockedAttachmentChildRejectsCascadeWithoutPartialDelete()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
