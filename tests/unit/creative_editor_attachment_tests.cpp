#include "EditorEdits.hpp"
#include "EditorAttachmentPlacement.hpp"
#include "EditorObjectActions.hpp"
#include "EditorState.hpp"
#include "EditorToolOptions.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "content/assets/StaticMeshAsset.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <optional>
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

iggy3d::StaticMeshAttachmentSocket attachmentSocket(
    std::string name,
    iggy3d::StaticMeshAttachmentSocketRole role,
    iggy3d::Vec3 position = {}) {
  return {std::move(name), "door.frame", role, position,
          {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F}};
}

iggy3d::StaticMeshAssetCatalog attachmentCatalog() {
  iggy3d::StaticMeshAssetCatalog catalog;
  iggy3d::StaticMeshAssetCatalogEntry frame;
  frame.assetId = "door_frame";
  frame.boundsMin = {-1.0F, -1.0F, -0.2F};
  frame.boundsMax = {1.0F, 1.0F, 0.0F};
  frame.collisionParts.push_back(
      {{-1.0F, -1.0F, -0.2F}, {1.0F, 1.0F, 0.0F}, false});
  frame.attachmentSockets.push_back(attachmentSocket(
      "door_frame", iggy3d::StaticMeshAttachmentSocketRole::Receiver));
  catalog.entries.push_back(frame);

  iggy3d::StaticMeshAssetCatalogEntry penetratingFrame = frame;
  penetratingFrame.assetId = "door_frame_penetrating";
  penetratingFrame.attachmentSockets.front().position.z = -0.1F;
  catalog.entries.push_back(std::move(penetratingFrame));

  iggy3d::StaticMeshAssetCatalogEntry leaf;
  leaf.assetId = "door_leaf";
  leaf.boundsMin = {-0.5F, -0.5F, 0.0F};
  leaf.boundsMax = {0.5F, 0.5F, 0.2F};
  leaf.collisionParts.push_back(
      {{-0.5F, -0.5F, 0.0F}, {0.5F, 0.5F, 0.2F}, false});
  leaf.attachmentSockets.push_back(attachmentSocket(
      "door_leaf", iggy3d::StaticMeshAttachmentSocketRole::Plug));
  catalog.entries.push_back(std::move(leaf));
  return catalog;
}

cr::CreativeDocumentCreateReceipt createAttachmentObject(
    cr::Facade& facade,
    cr::CreativeObjectKind kind,
    std::string name,
    std::string assetId,
    cr::CreativeVec3 position,
    std::optional<cr::CreativeObjectId> parentId = std::nullopt,
    std::string attachmentSocketName = {}) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
  request.assetId = std::move(assetId);
  request.transform.position = position;
  request.hasTransformOverride = true;
  request.parentId = parentId;
  request.attachmentSocket = std::move(attachmentSocketName);
  return facade.createDocumentObject(request);
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

bool reattachMovesHierarchyAndIsOneUndoableEdit() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, 320U, "Reattach"),
              "reattach document installed")) {
    return false;
  }
  const iggy3d::StaticMeshAssetCatalog catalog = attachmentCatalog();
  const cr::CreativeDocumentCreateReceipt firstFrame = createAttachmentObject(
      appState.facade, cr::CreativeObjectKind::Prop, "First Frame",
      "door_frame", {0.0, 0.0, 0.0});
  const cr::CreativeDocumentCreateReceipt secondFrame = createAttachmentObject(
      appState.facade, cr::CreativeObjectKind::Prop, "Second Frame",
      "door_frame", {4.0, 0.0, 0.0});
  const cr::CreativeDocumentCreateReceipt leaf = createAttachmentObject(
      appState.facade, cr::CreativeObjectKind::Door, "Door Leaf", "door_leaf",
      {0.0, 0.0, 0.0}, firstFrame.objectId, "door_frame");
  const cr::CreativeDocumentCreateReceipt handle = createAttachmentObject(
      appState.facade, cr::CreativeObjectKind::Prop, "Door Handle", {},
      {0.0, 1.0, 1.0}, leaf.objectId);
  if (!expect(firstFrame.accepted && secondFrame.accepted && leaf.accepted &&
                  handle.accepted,
              "reattach hierarchy created")) {
    return false;
  }
  const cr::CreativeTransform leafBefore =
      appState.facade.findObject(leaf.objectId)->transform;
  const cr::CreativeTransform handleBefore =
      appState.facade.findObject(handle.objectId)->transform;
  appState.history = {};
  const std::uint64_t revisionBefore = appState.facade.document().revision();

  const app::CreativeEditorObjectReattachmentPlan plan =
      app::planCreativeEditorObjectReattachment(
          appState.facade.document(), catalog, leaf.objectId,
          secondFrame.objectId, {4.0, 0.0, 0.0});
  const app::CreativeEditorObjectReattachmentReceipt applied =
      app::reattachObjectWithUndo(appState, appState.history, plan,
                                  "test_reattach");
  const cr::CreativeObject* movedLeaf =
      appState.facade.findObject(leaf.objectId);
  const cr::CreativeObject* movedHandle =
      appState.facade.findObject(handle.objectId);
  const bool moved =
      plan.accepted && plan.hierarchyObjectCount == 2U &&
      plan.clearance.allowed && applied.accepted && applied.changed &&
      movedLeaf != nullptr && movedHandle != nullptr &&
      movedLeaf->parentId == secondFrame.objectId &&
      movedLeaf->attachmentSocket == "door_frame" &&
      cr::creativeVec3ExactlyEqual(movedLeaf->transform.position,
                                   {4.0, 0.0, 0.0}) &&
      cr::creativeVec3ExactlyEqual(movedHandle->transform.position,
                                   {4.0, 1.0, 1.0}) &&
      movedHandle->parentId == leaf.objectId &&
      cr::creativeUndoDepth(appState.history) == 1U &&
      appState.facade.document().revision() == revisionBefore + 1U;
  const cr::CreativeTransform movedLeafTransform =
      movedLeaf != nullptr ? movedLeaf->transform : cr::CreativeTransform{};
  const cr::CreativeTransform movedHandleTransform =
      movedHandle != nullptr ? movedHandle->transform : cr::CreativeTransform{};

  const bool undone = app::undoLastEdit(appState, "test_undo_reattach");
  const cr::CreativeObject* restoredLeaf =
      appState.facade.findObject(leaf.objectId);
  const cr::CreativeObject* restoredHandle =
      appState.facade.findObject(handle.objectId);
  const bool restored =
      undone && restoredLeaf != nullptr && restoredHandle != nullptr &&
      restoredLeaf->parentId == firstFrame.objectId &&
      restoredLeaf->attachmentSocket == "door_frame" &&
      sameTransform(restoredLeaf->transform, leafBefore) &&
      sameTransform(restoredHandle->transform, handleBefore);
  const bool redone = app::redoLastEdit(appState, "test_redo_reattach");
  const cr::CreativeObject* redoneLeaf =
      appState.facade.findObject(leaf.objectId);
  const cr::CreativeObject* redoneHandle =
      appState.facade.findObject(handle.objectId);

  return expect(moved,
                "reattach moves the full hierarchy onto exact face contact") &&
         expect(restored && redone && redoneLeaf != nullptr &&
                    redoneHandle != nullptr &&
                    redoneLeaf->parentId == secondFrame.objectId &&
                    redoneLeaf->attachmentSocket == "door_frame" &&
                    sameTransform(redoneLeaf->transform,
                                  movedLeafTransform) &&
                    sameTransform(redoneHandle->transform,
                                  movedHandleTransform),
                "reattach is one undoable and redoable hierarchy edit");
}

bool branchRevisionPreventsFrozenReattachmentPlanReuse() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, 323U, "Branch-Safe Reattach"),
              "branch-safe reattach document installed")) {
    return false;
  }
  const iggy3d::StaticMeshAssetCatalog catalog = attachmentCatalog();
  const cr::CreativeDocumentCreateReceipt firstFrame = createAttachmentObject(
      appState.facade, cr::CreativeObjectKind::Prop, "First Frame",
      "door_frame", {0.0, 0.0, 0.0});
  const cr::CreativeDocumentCreateReceipt secondFrame = createAttachmentObject(
      appState.facade, cr::CreativeObjectKind::Prop, "Second Frame",
      "door_frame", {4.0, 0.0, 0.0});
  const cr::CreativeDocumentCreateReceipt leaf = createAttachmentObject(
      appState.facade, cr::CreativeObjectKind::Door, "Door Leaf", "door_leaf",
      {0.0, 0.0, 0.0}, firstFrame.objectId, "door_frame");
  if (!expect(firstFrame.accepted && secondFrame.accepted && leaf.accepted,
              "branch-safe reattach fixture created")) {
    return false;
  }
  appState.history = {};

  cr::CreativeDocumentHistoryTransaction editA =
      cr::beginCreativeHistoryTransaction(
          appState.facade, "reattach_branch_a");
  const cr::CreativeDocumentMutationReceipt renamedA =
      appState.facade.mutateObject(
          secondFrame.objectId, cr::CreativeMutationKind::Rename,
          cr::makeRenamePayload("Second Frame A"));
  const cr::CreativeHistoryRecordReceipt recordedA =
      cr::commitCreativeHistoryTransaction(
          appState.history, std::move(editA), appState.facade);
  const app::CreativeEditorObjectReattachmentPlan planA =
      app::planCreativeEditorObjectReattachment(
          appState.facade.document(), catalog, leaf.objectId,
          secondFrame.objectId, {4.0, 0.0, 0.0});
  const std::uint64_t revisionA = appState.facade.document().revision();

  const cr::CreativeHistoryApplyReceipt undone =
      cr::applyCreativeHistory(
          appState.facade, appState.history,
          cr::CreativeHistoryDirection::Undo);
  const std::uint64_t revisionAfterUndo =
      appState.facade.document().revision();

  cr::CreativeDocumentHistoryTransaction editB =
      cr::beginCreativeHistoryTransaction(
          appState.facade, "reattach_branch_b");
  const cr::CreativeDocumentMutationReceipt renamedB =
      appState.facade.mutateObject(
          secondFrame.objectId, cr::CreativeMutationKind::Rename,
          cr::makeRenamePayload("Second Frame B"));
  const cr::CreativeHistoryRecordReceipt recordedB =
      cr::commitCreativeHistoryTransaction(
          appState.history, std::move(editB), appState.facade);
  const std::uint64_t revisionB = appState.facade.document().revision();
  const cr::CreativeObject* leafBefore =
      appState.facade.findObject(leaf.objectId);
  const cr::CreativeTransform transformBefore =
      leafBefore != nullptr ? leafBefore->transform : cr::CreativeTransform{};
  const app::CreativeEditorObjectReattachmentReceipt stale =
      app::applyCreativeEditorObjectReattachment(appState.facade, planA);
  const cr::CreativeObject* leafAfter =
      appState.facade.findObject(leaf.objectId);

  return expect(renamedA.changed && recordedA.recorded &&
                    planA.accepted && planA.documentRevision == revisionA,
                "edit A produces one accepted frozen reattachment plan") &&
         expect(undone.accepted && revisionAfterUndo > revisionA,
                "undo rebases beyond the frozen plan revision") &&
         expect(renamedB.changed && recordedB.recorded &&
                    recordedB.clearedRedoCount == 1U &&
                    revisionB > revisionAfterUndo &&
                    revisionB != planA.documentRevision,
                "alternate branch cannot alias the frozen plan revision") &&
         expect(!stale.accepted && !stale.changed &&
                    stale.status ==
                        app::CreativeEditorObjectReattachmentStatus::
                            InvalidRequest &&
                    stale.revisionBefore == revisionB &&
                    stale.revisionAfter == revisionB &&
                    leafAfter != nullptr &&
                    leafAfter->parentId == firstFrame.objectId &&
                    sameTransform(leafAfter->transform, transformBefore),
                "branch-stale reattachment plan publishes nothing");
}

bool reattachRejectsStaleAndInsideHierarchyWithoutMutation() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, 322U, "Reattach Contract Rejections"),
              "reattach contract document installed")) {
    return false;
  }
  const cr::CreativeDocumentCreateReceipt source = createAttachmentObject(
      appState.facade, cr::CreativeObjectKind::Prop, "Source", "door_frame",
      {0.0, 0.0, 0.0});
  const cr::CreativeDocumentCreateReceipt child = createAttachmentObject(
      appState.facade, cr::CreativeObjectKind::Door, "Child", "door_leaf",
      {0.5, 0.0, 0.0}, source.objectId, "door_frame");
  const cr::CreativeDocumentCreateReceipt target = createAttachmentObject(
      appState.facade, cr::CreativeObjectKind::Prop, "Target", "door_frame",
      {4.0, 0.0, 0.0});
  if (!expect(source.accepted && child.accepted && target.accepted,
              "reattach contract rejection objects created")) {
    return false;
  }

  const cr::CreativeHierarchyReattachmentRequest stale{
      appState.facade.document().id(), appState.facade.document().revision(),
      child.objectId, target.objectId, "door_frame",
      {{4.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}}};
  static_cast<void>(appState.facade.mutateObject(
      target.objectId, cr::CreativeMutationKind::Rename,
      cr::makeRenamePayload("Changed Before Commit")));
  const std::uint64_t staleRevision = appState.facade.document().revision();
  const cr::CreativeTransform staleTransform =
      appState.facade.findObject(child.objectId)->transform;
  const cr::CreativeHierarchyReattachmentReceipt staleReceipt =
      appState.facade.reattachObjectHierarchyAtomically(stale);

  const cr::CreativeTransform insideTransform =
      appState.facade.findObject(source.objectId)->transform;
  const std::uint64_t insideRevision = appState.facade.document().revision();
  const cr::CreativeHierarchyReattachmentReceipt insideReceipt =
      appState.facade.reattachObjectHierarchyAtomically(
          {appState.facade.document().id(), insideRevision, source.objectId,
           child.objectId, "door_frame", insideTransform});

  return expect(staleReceipt.status == cr::CreativeHierarchyTransformStatus::StalePlan &&
                    staleReceipt.revisionAfter == staleRevision &&
                    appState.facade.findObject(child.objectId)->parentId ==
                        source.objectId &&
                    cr::creativeVec3ExactlyEqual(
                        appState.facade.findObject(child.objectId)->transform.position,
                        staleTransform.position) &&
                    cr::creativeVec3ExactlyEqual(
                        appState.facade.findObject(child.objectId)->transform.rotationEulerRadians,
                        staleTransform.rotationEulerRadians) &&
                    cr::creativeVec3ExactlyEqual(
                        appState.facade.findObject(child.objectId)->transform.scale,
                        staleTransform.scale),
                "stale reattachment publishes nothing") &&
         expect(insideReceipt.status ==
                    cr::CreativeHierarchyTransformStatus::TargetInsideSourceHierarchy &&
                    insideReceipt.revisionAfter == insideRevision &&
                    appState.facade.findObject(child.objectId)->parentId ==
                        source.objectId,
                "inside-hierarchy reattachment publishes nothing");
}

bool reattachRejectsOccupiedAndPenetratingHosts() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, 321U, "Reattach Rejections"),
              "reattach rejection document installed")) {
    return false;
  }
  const iggy3d::StaticMeshAssetCatalog catalog = attachmentCatalog();
  const cr::CreativeDocumentCreateReceipt source = createAttachmentObject(
      appState.facade, cr::CreativeObjectKind::Door, "Source", "door_leaf",
      {-4.0, 0.0, 0.0});
  const cr::CreativeDocumentCreateReceipt occupiedFrame =
      createAttachmentObject(appState.facade, cr::CreativeObjectKind::Prop,
                             "Occupied Frame", "door_frame",
                             {0.0, 0.0, 0.0});
  const cr::CreativeDocumentCreateReceipt occupant = createAttachmentObject(
      appState.facade, cr::CreativeObjectKind::Door, "Occupant", "door_leaf",
      {0.0, 0.0, 0.0}, occupiedFrame.objectId, "door_frame");
  const cr::CreativeDocumentCreateReceipt penetratingFrame =
      createAttachmentObject(appState.facade, cr::CreativeObjectKind::Prop,
                             "Penetrating Frame", "door_frame_penetrating",
                             {4.0, 0.0, 0.0});
  if (!expect(source.accepted && occupiedFrame.accepted && occupant.accepted &&
                  penetratingFrame.accepted,
              "reattach rejection objects created")) {
    return false;
  }
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  const app::CreativeEditorObjectReattachmentPlan occupied =
      app::planCreativeEditorObjectReattachment(
          appState.facade.document(), catalog, source.objectId,
          occupiedFrame.objectId, {0.0, 0.0, 0.0});
  const app::CreativeEditorObjectReattachmentPlan penetrating =
      app::planCreativeEditorObjectReattachment(
          appState.facade.document(), catalog, source.objectId,
          penetratingFrame.objectId, {4.0, 0.0, -0.1});

  return expect(!occupied.accepted &&
                    occupied.status ==
                        app::CreativeEditorObjectReattachmentStatus::SnapRejected &&
                    occupied.snap.status ==
                        cr::CreativeAttachmentSnapStatus::Occupied,
                "another child occupying the receiver rejects reattach") &&
         expect(!penetrating.accepted &&
                    penetrating.status ==
                        app::CreativeEditorObjectReattachmentStatus::ClearanceBlocked &&
                    penetrating.clearance.blockingObjectId ==
                        penetratingFrame.objectId &&
                    appState.facade.document().revision() == revisionBefore,
                "socket inside host collision rejects without mutation");
}

bool reattachObjectActionUsesFrozenAimTarget() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, 321U, "Reattach Object Action"),
              "reattach object-action document installed")) {
    return false;
  }
  const iggy3d::StaticMeshAssetCatalog catalog = attachmentCatalog();
  const cr::CreativeDocumentCreateReceipt firstFrame = createAttachmentObject(
      appState.facade, cr::CreativeObjectKind::Prop, "First Frame",
      "door_frame", {0.0, 0.0, 0.0});
  const cr::CreativeDocumentCreateReceipt secondFrame = createAttachmentObject(
      appState.facade, cr::CreativeObjectKind::Prop, "Second Frame",
      "door_frame", {4.0, 0.0, 0.0});
  const cr::CreativeDocumentCreateReceipt leaf = createAttachmentObject(
      appState.facade, cr::CreativeObjectKind::Door, "Door Leaf", "door_leaf",
      {0.0, 0.0, 0.0}, firstFrame.objectId, "door_frame");
  if (!expect(firstFrame.accepted && secondFrame.accepted && leaf.accepted,
              "reattach object-action fixtures created")) {
    return false;
  }
  appState.history = {};
  select(appState.facade, leaf.objectId);

  app::CreativeEditorState editor;
  editor.toolOptions.targetEntry = {cr::CreativeHeldItemKind::ObjectMove,
                                    cr::CreativeObjectKind::Unknown};
  editor.toolOptions.draft = editor.toolSettings;
  editor.toolOptions.commands = app::creativeEditorToolOptionCommandsForEntry(
      editor.toolOptions.targetEntry);
  editor.toolOptions.open = true;
  app::refreshCreativeEditorObjectActionContext(
      appState, editor.authoredAssets, editor.toolOptions);
  editor.toolOptions.contextAttachmentAimTargetId = secondFrame.objectId;
  editor.toolOptions.contextAttachmentAimPoint = {4.0, 0.0, 0.0};
  editor.toolOptions.contextAttachmentAimAvailable = true;

  const auto end = editor.toolOptions.commands.ids.begin() +
                   static_cast<std::ptrdiff_t>(
                       editor.toolOptions.commands.count);
  const auto reattach = std::find(
      editor.toolOptions.commands.ids.begin(), end,
      app::CreativeEditorToolOptionsCommandId::ReattachAttachment);
  if (!expect(reattach != end, "object actions expose reattach")) {
    return false;
  }
  const bool enabled = app::creativeEditorObjectActionEnabled(
      editor, editor.toolOptions,
      app::CreativeEditorToolOptionsCommandId::ReattachAttachment);
  const std::string value = app::creativeEditorObjectActionValueLabel(
      editor.toolOptions,
      app::CreativeEditorToolOptionsCommandId::ReattachAttachment);
  const bool activated = app::activateCreativeEditorObjectAction(
      appState, editor,
      app::CreativeEditorToolOptionsCommandId::ReattachAttachment, &catalog);
  const cr::CreativeObject* movedLeaf =
      appState.facade.findObject(leaf.objectId);

  return expect(
      enabled && value ==
                     "TO OBJECT #" + std::to_string(secondFrame.objectId) &&
          activated && !editor.toolOptions.open && movedLeaf != nullptr &&
          movedLeaf->parentId == secondFrame.objectId &&
          movedLeaf->attachmentSocket == "door_frame" &&
          cr::creativeVec3ExactlyEqual(movedLeaf->transform.position,
                                       {4.0, 0.0, 0.0}) &&
          editor.catalog.statusLabel == "REATTACHED TO door_frame" &&
          cr::creativeUndoDepth(appState.history) == 1U,
      "reattach action consumes the frozen aim and reports one accepted edit");
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

  const cr::CreativeSemanticDeleteReceipt removed =
      app::deleteSelectedObjectsWithUndo(
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

  return expect(removed.accepted && removed.changed &&
                    removed.removedObjectCount == 2U &&
                    removed.reasonCode == "creative_semantic_delete_applied" &&
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
      appState.facade.mutateObject(pair.doorId,
                                   cr::CreativeMutationKind::SetLocked,
                                   cr::makeLockPayload(true));
  appState.history = {};
  select(appState.facade, pair.frameId);

  const cr::CreativeSemanticDeleteReceipt removed =
      app::deleteSelectedObjectsWithUndo(
          appState, "test_reject_locked_attachment_delete", &appState.history);
  const cr::CreativeObject* frame = appState.facade.findObject(pair.frameId);
  const cr::CreativeObject* door = appState.facade.findObject(pair.doorId);
  return expect(
      cr::documentMutationSucceeded(locked.status) && !removed.accepted &&
          !removed.changed &&
          removed.status == cr::CreativeSemanticDeleteStatus::RemoveRejected &&
          frame != nullptr && door != nullptr &&
          door->parentId == pair.frameId &&
          cr::creativeUndoDepth(appState.history) == 0U,
      "locked attachment child rejects cascade without partial deletion");
}

}  // namespace

int main() {
  return attachedChildrenFollowParentTransformsAsOneHistoryStep() &&
                 detachActionPreservesWorldPoseAndRestoresRelationship() &&
                 reattachMovesHierarchyAndIsOneUndoableEdit() &&
                 branchRevisionPreventsFrozenReattachmentPlanReuse() &&
                 reattachRejectsOccupiedAndPenetratingHosts() &&
                 reattachRejectsStaleAndInsideHierarchyWithoutMutation() &&
                 reattachObjectActionUsesFrozenAimTarget() &&
                 deletingAttachmentParentCascadesAsOneUndoableEdit() &&
                 lockedAttachmentChildRejectsCascadeWithoutPartialDelete()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
