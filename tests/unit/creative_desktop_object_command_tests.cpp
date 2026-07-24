#include "creative_desktop_command_test_runners.hpp"
#include "creative_desktop_command_test_support.hpp"

namespace {

bool selectCommandsRoundTripAndRespectIdBoundary() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Select");
  static_cast<void>(document.assignId(415U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId a = createCrate(appState.facade, 0.0);
  const cr::CreativeObjectId b = createCrate(appState.facade, 2.0);
  const cr::CreativeObjectId c = createCrate(appState.facade, 4.0);

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};

  const app::CreativeDesktopCommandResult single = dispatchPayload(
      app::CreativeDesktopCommandId::SelectObjects, context,
      app::CreativeDesktopSelectPayload{{a}, a});
  const bool singleOk =
      single.accepted && single.affectedObjectCount == 1U &&
      appState.facade.selectionState().selectedTarget.value ==
          static_cast<cr::Id>(a);

  const app::CreativeDesktopCommandResult multi = dispatchPayload(
      app::CreativeDesktopCommandId::SelectObjects, context,
      app::CreativeDesktopSelectPayload{{a, b, c}, b});
  const bool multiOk =
      multi.accepted && multi.affectedObjectCount == 3U &&
      appState.facade.selectionState().selectedTarget.value ==
          static_cast<cr::Id>(b);

  const app::CreativeDesktopCommandResult cleared =
      dispatchPayload(app::CreativeDesktopCommandId::SelectObjects, context,
                      app::CreativeDesktopSelectPayload{});
  const bool clearOk =
      cleared.accepted && cleared.affectedObjectCount == 0U &&
      appState.facade.selectionState().selectedTarget.value == cr::kInvalidId;

  // uint64 CreativeObjectId -> uint32 TargetRef boundary: an id beyond the
  // 32-bit target space is dropped, never truncated into a bogus selection.
  const cr::CreativeObjectId hugeId =
      static_cast<cr::CreativeObjectId>(0x1'0000'0000ULL);
  const app::CreativeDesktopCommandResult dropped = dispatchPayload(
      app::CreativeDesktopCommandId::SelectObjects, context,
      app::CreativeDesktopSelectPayload{{hugeId}, hugeId});
  const app::CreativeDesktopCommandResult missing = dispatchPayload(
      app::CreativeDesktopCommandId::SelectObjects, context,
      app::CreativeDesktopSelectPayload{{999999U}, 999999U});
  const bool boundaryOk =
      dropped.accepted && dropped.affectedObjectCount == 0U &&
      missing.accepted && missing.affectedObjectCount == 0U &&
      appState.facade.selectionState().selectedTarget.value == cr::kInvalidId;

  return expect(singleOk, "SelectObjects selects a single primary") &&
         expect(multiOk,
                "SelectObjects replaces with a multi-selection + primary") &&
         expect(clearOk, "empty SelectObjects clears the selection") &&
         expect(boundaryOk,
                "SelectObjects drops missing and out-of-range object ids");
}

bool focusObjectSelectsAndFramesThroughDispatcher() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Focus");
  static_cast<void>(document.assignId(422U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId target = createCrate(appState.facade, 6.0);

  app::CreativeEditorState editor;
  editor.flyPos = {40.0F, 20.0F, 40.0F};
  editor.yawDegrees = 0.0F;
  editor.pitchDegrees = 0.0F;
  const iggy3d::Vec3 before = editor.flyPos;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult focused = dispatchPayload(
      app::CreativeDesktopCommandId::FocusObject, context,
      app::CreativeDesktopSelectPayload{{target}, target});
  const app::CreativeDesktopCommandResult mismatch = dispatchPayload(
      app::CreativeDesktopCommandId::FocusObject, context,
      std::monostate{});

  return expect(focused.accepted && focused.changed &&
                    focused.affectedObjectCount == 1U &&
                    appState.facade.selectionState().selectedTarget.value ==
                        static_cast<cr::Id>(target),
                "focus command selects exactly one authored object") &&
         expect(editor.flyPos.x != before.x || editor.flyPos.y != before.y ||
                    editor.flyPos.z != before.z,
                "focus command frames through the editor camera anchor") &&
         expect(!mismatch.accepted &&
                    mismatch.message == "focus: payload mismatch",
                "focus command rejects a mismatched payload");
}

bool frameSelectionAndSceneUseVisibleDocumentBounds() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Frame");
  static_cast<void>(document.assignId(424U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId left = createCrate(appState.facade, -12.0);
  const cr::CreativeObjectId right = createCrate(appState.facade, 12.0);
  static_cast<void>(right);
  static_cast<void>(appState.facade.selectTargets(
      std::span<const cr::CreativeObjectId>{&left, 1U}, left));

  app::CreativeEditorState editor;
  editor.flyPos = {50.0F, 20.0F, 50.0F};
  editor.yawDegrees = 0.0F;
  editor.pitchDegrees = -20.0F;
  const iggy3d::Vec3 arbitraryCamera = editor.flyPos;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult selection = dispatchOne(
      app::CreativeDesktopCommandId::FrameSelection3D, context);
  const iggy3d::Vec3 selectionCamera = editor.flyPos;
  const iggy3d::ProductCreativeViewportFocus selectionFocus =
      editor.viewportFocus;

  editor.flyPos = arbitraryCamera;
  const app::CreativeDesktopCommandResult all =
      dispatchOne(app::CreativeDesktopCommandId::FrameAll3D, context);
  const iggy3d::Vec3 allCamera = editor.flyPos;
  const iggy3d::ProductCreativeViewportFocus allFocus = editor.viewportFocus;

  static_cast<void>(appState.facade.selectTargets(
      std::span<const cr::CreativeObjectId>{}, cr::kInvalidObjectId));
  const app::CreativeDesktopCommandResult emptySelection = dispatchOne(
      app::CreativeDesktopCommandId::FrameSelection3D, context);

  cr::CreativeAppState emptyState;
  cr::CreativeDocument emptyDocument =
      cr::CreativeDocument::create("Cmd Empty Frame");
  static_cast<void>(emptyDocument.assignId(425U));
  static_cast<void>(emptyState.facade.installDocument(
      std::move(emptyDocument)));
  app::CreativeEditorState emptyEditor;
  const app::CreativeDesktopCommandContext emptyContext{
      emptyState, emptyEditor, std::filesystem::path{}, &saveId};
  const app::CreativeDesktopCommandResult emptyAll =
      dispatchOne(app::CreativeDesktopCommandId::FrameAll3D, emptyContext);

  return expect(selection.accepted && selection.changed &&
                    selection.affectedObjectCount == 1U,
                "frame selection uses the current visible selection") &&
         expect(all.accepted && all.changed && all.affectedObjectCount == 2U,
                "frame all uses every visible document object") &&
         expect(selectionCamera.x != arbitraryCamera.x ||
                    selectionCamera.y != arbitraryCamera.y ||
                    selectionCamera.z != arbitraryCamera.z,
                "frame selection moves the camera from arbitrary state") &&
         expect(allCamera.x != arbitraryCamera.x ||
                    allCamera.y != arbitraryCamera.y ||
                    allCamera.z != arbitraryCamera.z,
                "frame all moves the camera from arbitrary state") &&
         expect(selectionFocus.valid && allFocus.valid &&
                    selectionFocus.distanceMeters > 0.0F &&
                    allFocus.distanceMeters > 0.0F &&
                    allFocus.distanceMeters >=
                        selectionFocus.distanceMeters,
                "framing establishes the exact orbit focus for its scope") &&
         expect(!emptySelection.accepted &&
                    emptySelection.message ==
                        "frame selection: no visible selection",
                "frame selection fails visibly when nothing is selected") &&
         expect(!emptyAll.accepted &&
                    emptyAll.message == "frame all: no visible objects",
                "frame all fails visibly for an empty document");
}

bool logicCommandsRouteThroughTypedHistoryKernel() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Logic");
  static_cast<void>(document.assignId(423U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  cr::CreativeDocumentCreateRequest sourceRequest;
  sourceRequest.kind = cr::CreativeObjectKind::TriggerZone;
  sourceRequest.name = "Trigger";
  const cr::CreativeObjectId source =
      appState.facade.createDocumentObject(sourceRequest).objectId;
  cr::CreativeDocumentCreateRequest targetRequest;
  targetRequest.kind = cr::CreativeObjectKind::Door;
  targetRequest.name = "Door";
  const cr::CreativeObjectId target =
      appState.facade.createDocumentObject(targetRequest).objectId;
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult sourceResult = dispatchPayload(
      app::CreativeDesktopCommandId::SetLogicSource, context,
      app::CreativeDesktopLogicLinkPayload{source});
  const app::CreativeDesktopCommandResult added = dispatchPayload(
      app::CreativeDesktopCommandId::SetLogicLink, context,
      app::CreativeDesktopLogicLinkPayload{
          source, target, cr::CreativeLogicLinkAction::Toggle});
  const app::CreativeDesktopCommandResult updated = dispatchPayload(
      app::CreativeDesktopCommandId::SetLogicLink, context,
      app::CreativeDesktopLogicLinkPayload{
          source, target, cr::CreativeLogicLinkAction::Open});
  const app::CreativeDesktopCommandResult unchanged = dispatchPayload(
      app::CreativeDesktopCommandId::SetLogicLink, context,
      app::CreativeDesktopLogicLinkPayload{
          source, target, cr::CreativeLogicLinkAction::Open});
  const std::size_t depthBeforeRemove =
      cr::creativeUndoDepth(appState.history);
  const app::CreativeDesktopCommandResult removed = dispatchPayload(
      app::CreativeDesktopCommandId::RemoveLogicLink, context,
      app::CreativeDesktopLogicLinkPayload{source, target});
  const app::CreativeDesktopCommandResult cleared = dispatchPayload(
      app::CreativeDesktopCommandId::ClearLogicSource, context,
      std::monostate{});

  return expect(sourceResult.accepted && sourceResult.changed &&
                    editor.logicLinks.sourceObjectId ==
                        cr::kInvalidObjectId,
                "desktop command selects then clears a logic source") &&
         expect(added.accepted && added.changed && updated.accepted &&
                    updated.changed,
                "desktop commands add and update a typed logic link") &&
         expect(unchanged.accepted && !unchanged.changed &&
                    depthBeforeRemove == 2U,
                "unchanged desktop action adds no history") &&
         expect(removed.accepted && removed.changed && cleared.accepted &&
                    cleared.changed &&
                    cr::creativeUndoDepth(appState.history) == 3U &&
                    appState.facade.document().findLogicLink(source, target) ==
                        nullptr,
                "remove and source-clear finish through semantic commands");
}

bool deleteSelectionCommandRemovesGroupHierarchy() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd DeleteMulti");
  static_cast<void>(document.assignId(416U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId a = createCrate(appState.facade, 0.0);
  const cr::CreativeObjectId b = createCrate(appState.facade, 2.0);
  const std::array<cr::CreativeObjectId, 2U> members{a, b};
  static_cast<void>(appState.facade.selectTargets(members, b));
  const cr::CreativeGroupCommandReceipt grouped =
      appState.facade.groupSelectedObjects();
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};

  // Select the group root, then delete the current selection: the root would
  // trip ParentHasChildren on its own, so the whole hierarchy must go.
  selectPrimary(appState.facade, grouped.groupObjectId);
  const std::uint64_t before = appState.facade.document().objectCount();
  const app::CreativeDesktopCommandResult del =
      dispatchOne(app::CreativeDesktopCommandId::DeleteSelection, context);

  return expect(grouped.accepted && before >= 3U,
                "group creates a root over both crates") &&
         expect(del.accepted && del.changed &&
                    appState.facade.document().objectCount() == 0U,
                "DeleteSelection removes the whole group hierarchy") &&
         expect(cr::creativeUndoDepth(appState.history) == 1U,
                "multi-delete records exactly one undo step") &&
         expect(appState.facade.findObject(a) == nullptr &&
                    appState.facade.findObject(b) == nullptr &&
                    appState.facade.findObject(grouped.groupObjectId) == nullptr,
                "no group member survives the delete");
}

bool explicitObjectDeleteRejectsWithoutPartialHierarchy() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Delete Atomic");
  static_cast<void>(document.assignId(422U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const AttachedPair pair = createAttachedPair(appState.facade);
  static_cast<void>(appState.facade.mutateObject(
      pair.childId, cr::CreativeMutationKind::SetLocked,
      cr::makeLockPayload(true)));
  appState.history = {};

  app::CreativeEditorState editor;
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  const std::array lockedIds{pair.parentId};
  const app::CreativeEditorDeleteReceipt locked =
      app::deleteCreativeEditorObjectsWithUndo(
          appState, lockedIds, "desktop_delete_objects", &appState.history,
          &editor.worldLayout);
  const bool lockedRollback =
      !locked.accepted && !locked.changed &&
      appState.facade.findObject(pair.parentId) != nullptr &&
      appState.facade.findObject(pair.childId) != nullptr &&
      appState.facade.document().revision() == revisionBefore &&
      cr::creativeUndoDepth(appState.history) == 0U;

  static_cast<void>(appState.facade.mutateObject(
      pair.childId, cr::CreativeMutationKind::SetLocked,
      cr::makeLockPayload(false)));
  appState.history = {};
  const std::uint64_t missingRevisionBefore =
      appState.facade.document().revision();
  const std::array missingIds{pair.parentId, cr::CreativeObjectId{999999U}};
  const app::CreativeEditorDeleteReceipt missing =
      app::deleteCreativeEditorObjectsWithUndo(
          appState, missingIds, "desktop_delete_objects", &appState.history,
          &editor.worldLayout);
  const bool missingRollback =
      !missing.accepted && !missing.changed &&
      appState.facade.findObject(pair.parentId) != nullptr &&
      appState.facade.findObject(pair.childId) != nullptr &&
      appState.facade.document().revision() == missingRevisionBefore &&
      cr::creativeUndoDepth(appState.history) == 0U;

  return expect(lockedRollback,
                "locked descendants reject multi-delete atomically") &&
         expect(missingRollback,
                "missing ids reject multi-delete without partial removal");
}

bool renameObjectCommandChangesNameWithHistory() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Rename");
  static_cast<void>(document.assignId(417U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId a = createCrate(appState.facade, 0.0);
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};

  const app::CreativeDesktopCommandResult renamed = dispatchPayload(
      app::CreativeDesktopCommandId::RenameObject, context,
      app::CreativeDesktopRenamePayload{a, "Renamed Crate"});
  const cr::CreativeObject* object = appState.facade.findObject(a);
  const bool nameOk =
      object != nullptr && object->name == "Renamed Crate";

  const app::CreativeDesktopCommandResult empty = dispatchPayload(
      app::CreativeDesktopCommandId::RenameObject, context,
      app::CreativeDesktopRenamePayload{a, ""});

  return expect(
             renamed.accepted && renamed.changed && nameOk &&
                 renamed.objectAction.status ==
                     app::CreativeEditorObjectActionOutcomeStatus::Applied &&
                 renamed.objectAction.action ==
                     cr::CreativeSemanticObjectAction::Rename,
                "RenameObject sets the object name") &&
         expect(cr::creativeUndoDepth(appState.history) == 1U,
                "rename records exactly one undo step") &&
         expect(!empty.accepted &&
                    empty.objectAction.status ==
                        app::CreativeEditorObjectActionOutcomeStatus::
                            InvalidRequest,
                "rename with an empty name is rejected");
}

bool visibilityAndLockCommandsSetAbsoluteState() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Flags");
  static_cast<void>(document.assignId(418U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId a = createCrate(appState.facade, 0.0);
  const cr::CreativeObjectId b = createCrate(appState.facade, 2.0);
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};

  const app::CreativeDesktopCommandResult hidden = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectsVisible, context,
      app::CreativeDesktopObjectFlagPayload{{a, b}, false});
  const cr::CreativeObject* oa = appState.facade.findObject(a);
  const cr::CreativeObject* ob = appState.facade.findObject(b);
  const bool hiddenOk = hidden.accepted && hidden.changed && oa != nullptr &&
                        !oa->visible && ob != nullptr && !ob->visible;

  // Lock the current selection (empty id list drives from selection).
  selectPrimary(appState.facade, a);
  const app::CreativeDesktopCommandResult locked = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectsLocked, context,
      app::CreativeDesktopObjectFlagPayload{{}, true});
  const cr::CreativeObject* la = appState.facade.findObject(a);
  const bool lockedOk =
      locked.accepted && locked.changed && la != nullptr && la->locked;

  return expect(
             hiddenOk &&
                 hidden.objectAction.status ==
                     app::CreativeEditorObjectActionOutcomeStatus::Applied &&
                 hidden.objectAction.action ==
                     cr::CreativeSemanticObjectAction::SetVisible,
             "SetObjectsVisible hides both listed objects") &&
         expect(
             lockedOk &&
                 locked.objectAction.status ==
                     app::CreativeEditorObjectActionOutcomeStatus::Applied &&
                 locked.objectAction.action ==
                     cr::CreativeSemanticObjectAction::SetLocked,
             "SetObjectsLocked locks the current selection") &&
         expect(cr::creativeUndoDepth(appState.history) == 2U,
                "visibility + lock each record one undo step");
}

bool visibilityAndLockBatchesRollBackOnFailure() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Flags Atomic");
  static_cast<void>(document.assignId(423U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId a = createCrate(appState.facade, 0.0);
  const cr::CreativeObjectId b = createCrate(appState.facade, 2.0);
  static_cast<void>(appState.facade.mutateObject(
      b, cr::CreativeMutationKind::SetLocked, cr::makeLockPayload(true)));
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult visibility = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectsVisible, context,
      app::CreativeDesktopObjectFlagPayload{{a, b}, false});
  const bool visibilityRolledBack =
      !visibility.accepted && !visibility.changed &&
      appState.facade.findObject(a)->visible &&
      appState.facade.findObject(b)->visible &&
      cr::creativeUndoDepth(appState.history) == 0U;

  static_cast<void>(appState.facade.mutateObject(
      b, cr::CreativeMutationKind::SetLocked, cr::makeLockPayload(false)));
  appState.history = {};
  const app::CreativeDesktopCommandResult locking = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectsLocked, context,
      app::CreativeDesktopObjectFlagPayload{{a, 999999U}, true});
  const bool lockingRolledBack =
      !locking.accepted && !locking.changed &&
      !appState.facade.findObject(a)->locked &&
      cr::creativeUndoDepth(appState.history) == 0U;

  return expect(visibilityRolledBack,
                "locked members roll back an absolute visibility batch") &&
         expect(lockingRolledBack,
                "missing members roll back an absolute lock batch");
}

bool transformCommandSetsAbsoluteWithMask() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Transform");
  static_cast<void>(document.assignId(419U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId a = createCrate(appState.facade, 0.0);
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};

  cr::CreativeTransform target;
  target.position = {7.0, 8.0, 9.0};
  target.rotationEulerRadians = {0.5, 0.0, 0.0};
  target.scale = {2.0, 2.0, 2.0};

  // Position-only: the scale/rotation components in the payload are ignored.
  const app::CreativeDesktopCommandResult posOnly = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectTransform, context,
      app::CreativeDesktopTransformPayload{a, target, true, false, false});
  const cr::CreativeObject* afterPos = appState.facade.findObject(a);
  const bool posOk =
      posOnly.accepted && posOnly.changed && afterPos != nullptr &&
      afterPos->transform.position.x == 7.0 &&
      afterPos->transform.position.y == 8.0 &&
      afterPos->transform.scale.x == 1.0;  // scale mask was off.

  // Scale-only afterwards.
  const app::CreativeDesktopCommandResult scaleOnly = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectTransform, context,
      app::CreativeDesktopTransformPayload{a, target, false, false, true});
  const cr::CreativeObject* afterScale = appState.facade.findObject(a);
  const bool scaleOk =
      scaleOnly.accepted && scaleOnly.changed && afterScale != nullptr &&
      afterScale->transform.scale.x == 2.0 &&
      afterScale->transform.scale.y == 2.0;

  const app::CreativeDesktopCommandResult missing = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectTransform, context,
      app::CreativeDesktopTransformPayload{999999U, target, true, true, true});

  return expect(posOk, "position-only transform moves without scaling") &&
         expect(scaleOk, "scale-only transform scales after the fact") &&
         expect(cr::creativeUndoDepth(appState.history) == 2U,
                "two masked transforms record two undo steps") &&
         expect(!missing.accepted, "transform on a missing object is rejected");
}

bool transformCommandIsAtomicAndAttachmentAware() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Transform Hierarchy");
  static_cast<void>(document.assignId(424U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const AttachedPair pair = createAttachedPair(appState.facade);
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const cr::CreativeTransform parentBefore =
      appState.facade.findObject(pair.parentId)->transform;
  const cr::CreativeTransform childBefore =
      appState.facade.findObject(pair.childId)->transform;
  cr::CreativeTransform target = parentBefore;
  target.position = {5.0, 1.0, 6.0};
  target.rotationEulerRadians.y = std::numbers::pi * 0.5;
  target.scale = {2.0, 2.0, 2.0};
  const app::CreativeDesktopCommandResult transformed = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectTransform, context,
      app::CreativeDesktopTransformPayload{pair.parentId, target, true, true,
                                           true});
  const cr::CreativeObject* parentAfter =
      appState.facade.findObject(pair.parentId);
  const cr::CreativeObject* childAfter =
      appState.facade.findObject(pair.childId);
  const bool hierarchyChanged =
      transformed.accepted && transformed.changed &&
      transformed.affectedObjectCount == 2U && parentAfter != nullptr &&
      childAfter != nullptr &&
      cr::creativeVec3ExactlyEqual(parentAfter->transform.position,
                                   target.position) &&
      cr::creativeVec3ExactlyEqual(parentAfter->transform.scale,
                                   target.scale) &&
      !cr::creativeVec3ExactlyEqual(childAfter->transform.position,
                                    childBefore.position) &&
      cr::creativeVec3ExactlyEqual(childAfter->transform.scale,
                                   {2.0, 2.0, 2.0}) &&
      childAfter->parentId == pair.parentId &&
      cr::creativeUndoDepth(appState.history) == 1U;
  const bool undone = app::undoLastEdit(appState, "desktop_transform_undo");
  const cr::CreativeObject* parentUndone =
      appState.facade.findObject(pair.parentId);
  const cr::CreativeObject* childUndone =
      appState.facade.findObject(pair.childId);
  const bool hierarchyUndone =
      undone && parentUndone != nullptr && childUndone != nullptr &&
      cr::creativeVec3ExactlyEqual(parentUndone->transform.position,
                                   parentBefore.position) &&
      cr::creativeVec3ExactlyEqual(childUndone->transform.position,
                                   childBefore.position);

  cr::CreativeTransform fullRotation = parentBefore;
  fullRotation.rotationEulerRadians = {0.25, -0.35, 0.2};
  const app::CreativeDesktopCommandResult rotated = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectTransform, context,
      app::CreativeDesktopTransformPayload{pair.parentId, fullRotation, false,
                                           true, false});
  const cr::CreativeObject* fullyRotatedParent =
      appState.facade.findObject(pair.parentId);
  const cr::CreativeObject* fullyRotatedChild =
      appState.facade.findObject(pair.childId);
  const bool fullRotationApplied =
      rotated.accepted && rotated.changed &&
      rotated.affectedObjectCount == 2U && fullyRotatedParent != nullptr &&
      fullyRotatedChild != nullptr &&
      vecNear(fullyRotatedParent->transform.rotationEulerRadians,
              fullRotation.rotationEulerRadians) &&
      !vecNear(fullyRotatedChild->transform.position, childBefore.position) &&
      cr::creativeUndoDepth(appState.history) == 1U;
  const bool fullRotationUndone =
      app::undoLastEdit(appState, "desktop_full_rotation_undo") &&
      vecNear(appState.facade.findObject(pair.parentId)->transform.position,
              parentBefore.position) &&
      vecNear(appState.facade.findObject(pair.childId)->transform.position,
              childBefore.position);

  const cr::CreativeObjectId lone = createCrate(appState.facade, 9.0);
  appState.history = {};
  const cr::CreativeTransform loneBefore =
      appState.facade.findObject(lone)->transform;
  cr::CreativeTransform invalid = loneBefore;
  invalid.position = {12.0, 0.0, 0.0};
  invalid.rotationEulerRadians.y =
      std::numeric_limits<double>::quiet_NaN();
  const app::CreativeDesktopCommandResult invalidBatch = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectTransform, context,
      app::CreativeDesktopTransformPayload{lone, invalid, true, true, false});
  const bool invalidRolledBack =
      !invalidBatch.accepted && !invalidBatch.changed &&
      cr::creativeVec3ExactlyEqual(
          appState.facade.findObject(lone)->transform.position,
          loneBefore.position) &&
      cr::creativeUndoDepth(appState.history) == 0U;

  return expect(hierarchyChanged && hierarchyUndone,
                "absolute parent transform carries attachments in one edit") &&
         expect(fullRotationApplied && fullRotationUndone,
                "absolute pitch, yaw, and roll propagate as one undoable edit") &&
         expect(invalidRolledBack,
                "invalid masked transforms do not partially apply");
}

bool groupPivotUsesItsDedicatedTypedCommand() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Group Pivot");
  static_cast<void>(document.assignId(435U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  cr::CreativeDocumentCreateRequest groupRequest;
  groupRequest.kind = cr::CreativeObjectKind::Group;
  groupRequest.name = "Assembly";
  groupRequest.transform.position = {1.0, 0.5, 2.0};
  groupRequest.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt group =
      appState.facade.createDocumentObject(groupRequest);
  cr::CreativeDocumentCreateRequest childRequest;
  childRequest.kind = cr::CreativeObjectKind::Crate;
  childRequest.name = "Assembly Member";
  childRequest.transform.position = {3.0, 0.5, 2.0};
  childRequest.hasTransformOverride = true;
  childRequest.parentId = group.objectId;
  const cr::CreativeDocumentCreateReceipt child =
      appState.facade.createDocumentObject(childRequest);
  if (!group.accepted || !child.accepted) {
    return expect(false, "group pivot command fixture created");
  }
  const cr::CreativeVec3 originalPivot =
      appState.facade.findObject(group.objectId)->transform.position;
  const cr::CreativeTransform childBefore =
      appState.facade.findObject(child.objectId)->transform;
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const cr::CreativeVec3 newPivot{6.0, 1.25, -3.0};
  const app::CreativeDesktopCommandResult pivoted = dispatchPayload(
      app::CreativeDesktopCommandId::SetGroupPivot, context,
      app::CreativeDesktopGroupPivotPayload{group.objectId, newPivot});
  const cr::CreativeObject* groupAfter =
      appState.facade.findObject(group.objectId);
  const cr::CreativeObject* childAfter =
      appState.facade.findObject(child.objectId);
  const bool pivotApplied =
      pivoted.accepted && pivoted.changed && pivoted.affectedObjectCount == 1U &&
      groupAfter != nullptr && childAfter != nullptr &&
      cr::creativeVec3ExactlyEqual(groupAfter->transform.position, newPivot) &&
      cr::creativeVec3ExactlyEqual(childAfter->transform.position,
                                   childBefore.position) &&
      cr::creativeUndoDepth(appState.history) == 1U;

  cr::CreativeTransform forbiddenTransform = groupAfter->transform;
  forbiddenTransform.position = {12.0, 2.0, 4.0};
  const app::CreativeDesktopCommandResult forbidden = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectTransform, context,
      app::CreativeDesktopTransformPayload{group.objectId, forbiddenTransform,
                                           true, false, false});
  const bool genericRejected =
      !forbidden.accepted && !forbidden.changed &&
      appState.facade.findObject(group.objectId) != nullptr &&
      cr::creativeVec3ExactlyEqual(
          appState.facade.findObject(group.objectId)->transform.position,
          newPivot) &&
      cr::creativeUndoDepth(appState.history) == 1U;

  const bool undone = app::undoLastEdit(appState, "desktop_group_pivot_undo");
  const cr::CreativeObject* restoredGroup =
      appState.facade.findObject(group.objectId);
  return expect(pivotApplied,
                "SetGroupPivot changes only the persistent pivot in one edit") &&
         expect(genericRejected,
                "generic object transform cannot split a hierarchy container") &&
         expect(undone && restoredGroup != nullptr &&
                    cr::creativeVec3ExactlyEqual(
                        restoredGroup->transform.position, originalPivot),
                "group pivot command is undoable");
}

bool movingPlatformSettingsUseTypedCommandAndOneUndoStep() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Moving Platform");
  static_cast<void>(document.assignId(426U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId platformId =
      createMovingPlatform(appState.facade);
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  cr::CreativeMovingPlatformSettings settings;
  settings.speedMetersPerSecond = 3.25;
  settings.traversalMode = cr::CreativeMovingPlatformTraversalMode::Loop;
  settings.startsActive = false;
  const app::CreativeDesktopCommandResult changed = dispatchPayload(
      app::CreativeDesktopCommandId::SetMovingPlatformSettings, context,
      app::CreativeDesktopMovingPlatformPayload{platformId, settings});
  const cr::CreativeObject* afterChange =
      appState.facade.findObject(platformId);
  const bool settingsApplied =
      afterChange != nullptr && afterChange->movingPlatform == settings;
  const std::size_t depthAfterChange =
      cr::creativeUndoDepth(appState.history);
  const app::CreativeDesktopCommandResult unchanged = dispatchPayload(
      app::CreativeDesktopCommandId::SetMovingPlatformSettings, context,
      app::CreativeDesktopMovingPlatformPayload{platformId, settings});

  cr::CreativeMovingPlatformSettings invalid = settings;
  invalid.speedMetersPerSecond = 0.0;
  const app::CreativeDesktopCommandResult rejected = dispatchPayload(
      app::CreativeDesktopCommandId::SetMovingPlatformSettings, context,
      app::CreativeDesktopMovingPlatformPayload{platformId, invalid});
  const app::CreativeDesktopCommandResult undo =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const cr::CreativeObject* afterUndo = appState.facade.findObject(platformId);
  const bool restoredDefaults =
      afterUndo != nullptr &&
      afterUndo->movingPlatform == cr::CreativeMovingPlatformSettings{};
  const app::CreativeDesktopCommandResult redo =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  const cr::CreativeObject* afterRedo = appState.facade.findObject(platformId);

  return expect(changed.accepted && changed.changed &&
                    changed.affectedObjectCount == 1U &&
                    settingsApplied,
                "moving platform settings command applies typed values") &&
         expect(depthAfterChange == 1U && unchanged.accepted &&
                    !unchanged.changed &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "unchanged moving platform settings add no undo entry") &&
         expect(!rejected.accepted && !rejected.changed,
                "invalid moving platform settings are rejected") &&
         expect(undo.accepted && undo.changed && restoredDefaults,
                "moving platform settings undo restores defaults") &&
         expect(redo.accepted && redo.changed && afterRedo != nullptr &&
                    afterRedo->movingPlatform == settings,
                "moving platform settings redo restores edited values");
}

bool playerSpawnSettingsUseTypedCommandAndOneUndoStep() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Player Spawn");
  static_cast<void>(document.assignId(427U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId spawnId = createPlayerSpawn(appState.facade);
  const cr::CreativeObjectId crateId = createCrate(appState.facade, 4.0);
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  cr::CreativePlayerSpawnSettings settings;
  settings.playerProfileId = "future_player";
  settings.spawnGroup = "north_entry";
  settings.validationRadiusMeters = 0.75;
  settings.fallbackPriority = 4U;
  const app::CreativeDesktopCommandResult changed = dispatchPayload(
      app::CreativeDesktopCommandId::SetPlayerSpawnSettings, context,
      app::CreativeDesktopPlayerSpawnPayload{spawnId, settings});
  const cr::CreativeObject* afterChange = appState.facade.findObject(spawnId);
  const bool settingsApplied =
      afterChange != nullptr && afterChange->playerSpawn == settings;
  const std::size_t depthAfterChange =
      cr::creativeUndoDepth(appState.history);
  const app::CreativeDesktopCommandResult unchanged = dispatchPayload(
      app::CreativeDesktopCommandId::SetPlayerSpawnSettings, context,
      app::CreativeDesktopPlayerSpawnPayload{spawnId, settings});

  cr::CreativePlayerSpawnSettings invalid = settings;
  invalid.validationRadiusMeters = 0.0;
  const app::CreativeDesktopCommandResult rejected = dispatchPayload(
      app::CreativeDesktopCommandId::SetPlayerSpawnSettings, context,
      app::CreativeDesktopPlayerSpawnPayload{spawnId, invalid});
  const app::CreativeDesktopCommandResult wrongKind = dispatchPayload(
      app::CreativeDesktopCommandId::SetPlayerSpawnSettings, context,
      app::CreativeDesktopPlayerSpawnPayload{crateId, settings});
  const app::CreativeDesktopCommandResult mismatch = dispatchPayload(
      app::CreativeDesktopCommandId::SetPlayerSpawnSettings, context,
      std::monostate{});
  const app::CreativeDesktopCommandResult undo =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const cr::CreativeObject* afterUndo = appState.facade.findObject(spawnId);
  const bool restoredDefaults =
      afterUndo != nullptr &&
      afterUndo->playerSpawn == cr::CreativePlayerSpawnSettings{};
  const app::CreativeDesktopCommandResult redo =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  const cr::CreativeObject* afterRedo = appState.facade.findObject(spawnId);

  return expect(changed.accepted && changed.changed &&
                    changed.affectedObjectCount == 1U && settingsApplied,
                "player spawn settings command applies typed values") &&
         expect(depthAfterChange == 1U && unchanged.accepted &&
                    !unchanged.changed &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "unchanged player spawn settings add no undo entry") &&
         expect(!rejected.accepted && !rejected.changed,
                "invalid player spawn settings are rejected") &&
         expect(!wrongKind.accepted && !wrongKind.changed,
                "player spawn settings reject non-spawn objects") &&
         expect(!mismatch.accepted && !mismatch.changed &&
                    mismatch.message ==
                        "player spawn settings: payload mismatch",
                "player spawn settings reject a mismatched payload") &&
         expect(undo.accepted && undo.changed && restoredDefaults,
                "player spawn settings undo restores defaults") &&
         expect(redo.accepted && redo.changed && afterRedo != nullptr &&
                    afterRedo->playerSpawn == settings,
                "player spawn settings redo restores edited values");
}

bool npcSpawnSettingsUseTypedCommandAndOneUndoStep() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd NPC Spawn");
  static_cast<void>(document.assignId(428U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId npcId = createNpcSpawn(appState.facade);
  const cr::CreativeObjectId crateId = createCrate(appState.facade, 4.0);
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  cr::CreativeNpcSpawnSettings settings;
  settings.behaviorProfileId = "default";
  settings.team = cr::CreativeNpcTeam::Hostile;
  settings.hitPoints = 37U;
  settings.initialAlertLevel = 0.6;
  settings.spawnPolicy = cr::CreativeNpcSpawnPolicy::Disabled;
  const app::CreativeDesktopCommandResult changed = dispatchPayload(
      app::CreativeDesktopCommandId::SetNpcSpawnSettings, context,
      app::CreativeDesktopNpcSpawnPayload{npcId, settings});
  const cr::CreativeObject* afterChange = appState.facade.findObject(npcId);
  const bool settingsApplied =
      afterChange != nullptr && afterChange->npcSpawn == settings;
  const std::size_t depthAfterChange =
      cr::creativeUndoDepth(appState.history);
  const app::CreativeDesktopCommandResult unchanged = dispatchPayload(
      app::CreativeDesktopCommandId::SetNpcSpawnSettings, context,
      app::CreativeDesktopNpcSpawnPayload{npcId, settings});

  cr::CreativeNpcSpawnSettings invalid = settings;
  invalid.initialAlertLevel = 2.0;
  const app::CreativeDesktopCommandResult rejected = dispatchPayload(
      app::CreativeDesktopCommandId::SetNpcSpawnSettings, context,
      app::CreativeDesktopNpcSpawnPayload{npcId, invalid});
  const app::CreativeDesktopCommandResult wrongKind = dispatchPayload(
      app::CreativeDesktopCommandId::SetNpcSpawnSettings, context,
      app::CreativeDesktopNpcSpawnPayload{crateId, settings});
  const app::CreativeDesktopCommandResult mismatch = dispatchPayload(
      app::CreativeDesktopCommandId::SetNpcSpawnSettings, context,
      std::monostate{});
  const app::CreativeDesktopCommandResult undo =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const cr::CreativeObject* afterUndo = appState.facade.findObject(npcId);
  const bool restoredDefaults =
      afterUndo != nullptr &&
      afterUndo->npcSpawn == cr::CreativeNpcSpawnSettings{};
  const app::CreativeDesktopCommandResult redo =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  const cr::CreativeObject* afterRedo = appState.facade.findObject(npcId);

  return expect(changed.accepted && changed.changed &&
                    changed.affectedObjectCount == 1U && settingsApplied,
                "npc spawn settings command applies typed values") &&
         expect(depthAfterChange == 1U && unchanged.accepted &&
                    !unchanged.changed &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "unchanged npc spawn settings add no undo entry") &&
         expect(!rejected.accepted && !rejected.changed,
                "invalid npc spawn settings are rejected") &&
         expect(!wrongKind.accepted && !wrongKind.changed,
                "npc spawn settings reject non-actor objects") &&
         expect(!mismatch.accepted && !mismatch.changed &&
                    mismatch.message == "npc spawn settings: payload mismatch",
                "npc spawn settings reject a mismatched payload") &&
         expect(undo.accepted && undo.changed && restoredDefaults,
                "npc spawn settings undo restores defaults") &&
         expect(redo.accepted && redo.changed && afterRedo != nullptr &&
                    afterRedo->npcSpawn == settings,
                "npc spawn settings redo restores edited values");
}

bool objectiveSettingsUseTypedCommandsAndOneUndoStepEach() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Objectives");
  static_cast<void>(document.assignId(429U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  cr::CreativeDocumentCreateRequest lootCreate;
  lootCreate.kind = cr::CreativeObjectKind::LootPoint;
  lootCreate.name = "Estate Key";
  const cr::CreativeObjectId lootId =
      appState.facade.createDocumentObject(lootCreate).objectId;
  cr::CreativeDocumentCreateRequest exitCreate;
  exitCreate.kind = cr::CreativeObjectKind::ExitPoint;
  exitCreate.name = "Estate Exit";
  const cr::CreativeObjectId exitId =
      appState.facade.createDocumentObject(exitCreate).objectId;
  const cr::CreativeObjectId crateId = createCrate(appState.facade, 4.0);
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const cr::CreativeLootPointSettings lootSettings{
      .itemId = "estate_key",
      .itemCount = 2U,
      .deactivateOnCollect = false,
  };
  const cr::CreativeExitPointSettings exitSettings{
      .requiredItemId = "estate_key",
      .requiredItemCount = 2U,
  };
  const app::CreativeDesktopCommandResult lootChanged = dispatchPayload(
      app::CreativeDesktopCommandId::SetLootPointSettings, context,
      app::CreativeDesktopLootPointPayload{lootId, lootSettings});
  const app::CreativeDesktopCommandResult exitChanged = dispatchPayload(
      app::CreativeDesktopCommandId::SetExitPointSettings, context,
      app::CreativeDesktopExitPointPayload{exitId, exitSettings});
  const cr::CreativeObject* changedLoot =
      appState.facade.findObject(lootId);
  const cr::CreativeObject* changedExit =
      appState.facade.findObject(exitId);
  const bool settingsApplied =
      changedLoot != nullptr && changedLoot->lootPoint == lootSettings &&
      changedExit != nullptr && changedExit->exitPoint == exitSettings;
  const std::size_t depthAfterChanges =
      cr::creativeUndoDepth(appState.history);

  const app::CreativeDesktopCommandResult unchanged = dispatchPayload(
      app::CreativeDesktopCommandId::SetExitPointSettings, context,
      app::CreativeDesktopExitPointPayload{exitId, exitSettings});
  cr::CreativeLootPointSettings invalidLoot = lootSettings;
  invalidLoot.itemCount = 0U;
  const app::CreativeDesktopCommandResult rejected = dispatchPayload(
      app::CreativeDesktopCommandId::SetLootPointSettings, context,
      app::CreativeDesktopLootPointPayload{lootId, invalidLoot});
  const app::CreativeDesktopCommandResult wrongKind = dispatchPayload(
      app::CreativeDesktopCommandId::SetExitPointSettings, context,
      app::CreativeDesktopExitPointPayload{crateId, exitSettings});
  const app::CreativeDesktopCommandResult mismatch = dispatchPayload(
      app::CreativeDesktopCommandId::SetLootPointSettings, context,
      std::monostate{});

  const app::CreativeDesktopCommandResult undoExit =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const cr::CreativeObject* afterUndoExit =
      appState.facade.findObject(exitId);
  const cr::CreativeObject* lootAfterUndoExit =
      appState.facade.findObject(lootId);
  const bool exitRestored =
      afterUndoExit != nullptr &&
      afterUndoExit->exitPoint == cr::CreativeExitPointSettings{} &&
      lootAfterUndoExit != nullptr &&
      lootAfterUndoExit->lootPoint == lootSettings;
  const app::CreativeDesktopCommandResult undoLoot =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const cr::CreativeObject* afterUndoLoot =
      appState.facade.findObject(lootId);
  const bool lootRestored =
      afterUndoLoot != nullptr &&
      afterUndoLoot->lootPoint == cr::CreativeLootPointSettings{};
  const app::CreativeDesktopCommandResult redoLoot =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  const app::CreativeDesktopCommandResult redoExit =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  const cr::CreativeObject* afterRedoLoot =
      appState.facade.findObject(lootId);
  const cr::CreativeObject* afterRedoExit =
      appState.facade.findObject(exitId);

  return expect(lootChanged.accepted && lootChanged.changed &&
                    exitChanged.accepted && exitChanged.changed &&
                    settingsApplied && depthAfterChanges == 2U,
                "objective settings commands apply one typed edit each") &&
         expect(unchanged.accepted && !unchanged.changed &&
                    cr::creativeUndoDepth(appState.history) == 2U,
                "unchanged objective settings add no undo entry") &&
         expect(!rejected.accepted && !rejected.changed,
                "invalid objective settings are rejected") &&
         expect(!wrongKind.accepted && !wrongKind.changed,
                "objective settings reject nonmatching object kinds") &&
         expect(!mismatch.accepted && !mismatch.changed &&
                    mismatch.message ==
                        "loot point settings: payload mismatch",
                "objective settings reject a mismatched payload") &&
         expect(undoExit.accepted && undoExit.changed && exitRestored &&
                    undoLoot.accepted && undoLoot.changed && lootRestored,
                "objective settings undo independently in command order") &&
         expect(redoLoot.accepted && redoLoot.changed &&
                    redoExit.accepted && redoExit.changed &&
                    afterRedoLoot != nullptr &&
                    afterRedoLoot->lootPoint == lootSettings &&
                    afterRedoExit != nullptr &&
                    afterRedoExit->exitPoint == exitSettings,
                "objective settings redo exact typed values");
}

bool movingPlatformWaypointCommandsSelectEditAndUndo() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Moving Platform Waypoint");
  static_cast<void>(document.assignId(428U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId platformId =
      createMovingPlatform(appState.facade);
  selectPrimary(appState.facade, platformId);
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const std::uint64_t revisionBeforeSelection =
      appState.facade.document().revision();
  const app::CreativeDesktopCommandResult selected = dispatchPayload(
      app::CreativeDesktopCommandId::SelectMovingPlatformWaypoint, context,
      app::CreativeDesktopMovingPlatformWaypointPayload{platformId, 1U, 0.0});
  const bool pointSelected =
      editor.interaction.movingPlatformPathEdit.pointSelected &&
      editor.interaction.movingPlatformPathEdit.selectedPointIndex == 1U &&
      appState.facade.document().revision() == revisionBeforeSelection;
  const app::CreativeDesktopCommandResult changed = dispatchPayload(
      app::CreativeDesktopCommandId::SetMovingPlatformWaypointDwell, context,
      app::CreativeDesktopMovingPlatformWaypointPayload{platformId, 1U, 1.25});
  const cr::CreativeObject* edited = appState.facade.findObject(platformId);
  const bool valueStored = edited != nullptr &&
                           edited->pathPoints[1].dwellSeconds == 1.25;
  const app::CreativeDesktopCommandResult unchanged = dispatchPayload(
      app::CreativeDesktopCommandId::SetMovingPlatformWaypointDwell, context,
      app::CreativeDesktopMovingPlatformWaypointPayload{platformId, 1U, 1.25});
  const app::CreativeDesktopCommandResult invalid = dispatchPayload(
      app::CreativeDesktopCommandId::SetMovingPlatformWaypointDwell, context,
      app::CreativeDesktopMovingPlatformWaypointPayload{platformId, 1U, 60.25});
  const app::CreativeDesktopCommandResult undo =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const cr::CreativeObject* undone = appState.facade.findObject(platformId);
  const bool restoredZero = undone != nullptr &&
                            undone->pathPoints[1].dwellSeconds == 0.0;
  const app::CreativeDesktopCommandResult redo =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  const cr::CreativeObject* redone = appState.facade.findObject(platformId);

  return expect(selected.accepted && selected.changed && pointSelected,
                "waypoint selection is transient and targets one point") &&
         expect(changed.accepted && changed.changed &&
                    changed.affectedObjectCount == 1U && valueStored,
                "waypoint dwell command applies the typed value") &&
         expect(unchanged.accepted && !unchanged.changed &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "unchanged waypoint dwell records no extra history") &&
         expect(!invalid.accepted && !invalid.changed,
                "out-of-range waypoint dwell is rejected") &&
         expect(undo.accepted && undo.changed && restoredZero &&
                    redo.accepted && redo.changed && redone != nullptr &&
                    redone->pathPoints[1].dwellSeconds == 1.25,
                "waypoint dwell is one undoable desktop edit");
}

bool movingPlatformPreviewCommandsStayTransient() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Moving Platform Preview");
  static_cast<void>(document.assignId(427U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId platformId =
      createMovingPlatform(appState.facade);
  const cr::CreativeObject* platform = appState.facade.findObject(platformId);
  appState.history = {};

  app::CreativeEditorState editor;
  if (platform == nullptr ||
      !app::syncCreativeMovingPlatformPreview(
           editor.movingPlatformPreview, appState.facade.document().id(),
           platform)
           .accepted) {
    return expect(false, "desktop preview state synchronizes");
  }
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  const app::CreativeDesktopCommandResult play = dispatchPayload(
      app::CreativeDesktopCommandId::ToggleMovingPlatformPreview, context,
      app::CreativeDesktopMovingPlatformPreviewPayload{platformId, 0.0});
  const app::CreativeDesktopCommandResult seek = dispatchPayload(
      app::CreativeDesktopCommandId::SeekMovingPlatformPreview, context,
      app::CreativeDesktopMovingPlatformPreviewPayload{platformId, 0.5});
  const bool soughtToMidpoint =
      editor.movingPlatformPreview.normalizedProgress == 0.5 &&
      editor.movingPlatformPreview.runtimeState.positionMeters.y == 2.0F &&
      !editor.movingPlatformPreview.playing;
  const app::CreativeDesktopCommandResult restart = dispatchPayload(
      app::CreativeDesktopCommandId::RestartMovingPlatformPreview, context,
      app::CreativeDesktopMovingPlatformPreviewPayload{platformId, 0.0});

  return expect(play.accepted && play.changed,
                "desktop command starts route preview") &&
         expect(seek.accepted && seek.changed && soughtToMidpoint,
                "desktop command scrubs and pauses route preview") &&
         expect(restart.accepted && restart.changed &&
                    editor.movingPlatformPreview.normalizedProgress == 0.0,
                "desktop command restarts route preview") &&
         expect(appState.facade.document().revision() == revisionBefore &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "preview commands write no document or history state");
}

bool mismatchedPayloadsAreNoOpFailures() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Mismatch");
  static_cast<void>(document.assignId(421U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId a = createCrate(appState.facade, 0.0);
  static_cast<void>(createCrate(appState.facade, 2.0));
  selectPrimary(appState.facade, a);
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const std::uint64_t before = appState.facade.document().objectCount();

  const app::CreativeDesktopCommandResult badMapRegeneration = dispatchPayload(
      app::CreativeDesktopCommandId::RegenerateMapTemplate, context,
      std::monostate{});
  const app::CreativeDesktopCommandResult badRename = dispatchPayload(
      app::CreativeDesktopCommandId::RenameObject, context, std::monostate{});
  const app::CreativeDesktopCommandResult badTransform = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectTransform, context,
      std::monostate{});
  const app::CreativeDesktopCommandResult badLogic = dispatchPayload(
      app::CreativeDesktopCommandId::SetLogicLink, context,
      app::CreativeDesktopSelectPayload{{a}, a});
  const app::CreativeDesktopCommandResult badMovingPlatform = dispatchPayload(
      app::CreativeDesktopCommandId::SetMovingPlatformSettings, context,
      std::monostate{});
  const app::CreativeDesktopCommandResult badMovingPlatformPreview =
      dispatchPayload(
          app::CreativeDesktopCommandId::SeekMovingPlatformPreview, context,
          std::monostate{});
  const app::CreativeDesktopCommandResult badMovingPlatformWaypoint =
      dispatchPayload(
          app::CreativeDesktopCommandId::SetMovingPlatformWaypointDwell,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutManipulation =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutManipulateRoom, context,
          std::monostate{});
  const app::CreativeDesktopCommandResult badRoofApertureManipulation =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutManipulateRoofAperture,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badGeneratedRoomPreview =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutPreviewGeneratedRoomSettings,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badGeneratedRoomApply =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutApplyGeneratedRoomSettings,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badGeneratedLevelPreview =
      dispatchPayload(
          app::CreativeDesktopCommandId::
              WorldLayoutPreviewGeneratedLevelSettings,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badGeneratedLevelApply =
      dispatchPayload(
          app::CreativeDesktopCommandId::
              WorldLayoutApplyGeneratedLevelSettings,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutLevelOperation =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutLevelOperation, context,
          std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutBuildingSelection =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutSelectBuilding, context,
          std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutSourceFocus =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutFocusSource, context,
          std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutSourceScope =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutSelectSourceScope,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutObjectSourceFocus =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutFocusObjectSource,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutObjectSourceAdoption =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutAdoptObjectSource,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutSourceRename =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutRenameSource, context,
          std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutSourceDuplicate =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutDuplicateSource, context,
          std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutSourceDelete =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutDeleteSource, context,
          std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutBuildingManipulation =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutManipulateBuilding,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutBuildingDuplicate =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutDuplicateBuilding,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutBuildingTransform =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutTransformBuilding, context,
          std::monostate{});
  const app::CreativeDesktopCommandResult badGeneratedBuildingPreview =
      dispatchPayload(
          app::CreativeDesktopCommandId::
              WorldLayoutPreviewGeneratedBuildingOperation,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badGeneratedBuildingApply =
      dispatchPayload(
          app::CreativeDesktopCommandId::
              WorldLayoutApplyGeneratedBuildingOperation,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutTemplateCapture =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutCaptureBuildingTemplate,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutTemplateUpdate =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutUpdateBuildingTemplate,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutTemplateDetach =
      dispatchPayload(
          app::CreativeDesktopCommandId::
              WorldLayoutDetachBuildingTemplateInstance,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutTemplateRefresh =
      dispatchPayload(
          app::CreativeDesktopCommandId::
              WorldLayoutRefreshBuildingTemplateInstances,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutTemplateSelection =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutSelectBuildingTemplate,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutTemplatePlacement =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutPlaceBuildingTemplate,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutBoxSettings =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutSetBoxSettings, context,
          std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutBoxManipulation =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutManipulateBox, context,
          std::monostate{});
  const app::CreativeDesktopCommandResult
      badGeneratedVerticalConnectorPreview = dispatchPayload(
          app::CreativeDesktopCommandId::
              WorldLayoutPreviewGeneratedVerticalConnectorSettings,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult
      badGeneratedVerticalConnectorApply = dispatchPayload(
          app::CreativeDesktopCommandId::
              WorldLayoutApplyGeneratedVerticalConnectorSettings,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult
      badWorldLayoutVerticalConnectorManipulation = dispatchPayload(
          app::CreativeDesktopCommandId::
              WorldLayoutManipulateVerticalConnector,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutWallSettings =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutSetWallSettings, context,
          std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutWallManipulation =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutManipulateWall, context,
          std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutOpeningSettings =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutSetOpeningSettings, context,
          std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutOpeningInsert =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutSetOpeningInsert, context,
          std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutOpeningManipulation =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutManipulateOpening, context,
          std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutLevelSettings =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutSetLevelSettings, context,
          std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutBuildingGrounding =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutSetBuildingGrounding,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badGeneratedBuildingGrounding =
      dispatchPayload(
          app::CreativeDesktopCommandId::
              WorldLayoutApplyGeneratedBuildingGrounding,
          context, std::monostate{});
  const app::CreativeDesktopCommandResult badWorldLayoutObjectSettings =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutSetObjectSettings, context,
          std::monostate{});

  return expect(!badMapRegeneration.accepted &&
                    badMapRegeneration.message ==
                        "map regeneration: payload mismatch",
                "map regeneration rejects a mismatched payload") &&
         expect(!badRename.accepted &&
                    badRename.message == "rename: payload mismatch" &&
                    badRename.objectAction.status ==
                        app::CreativeEditorObjectActionOutcomeStatus::
                            PayloadMismatch,
                "RenameObject with no payload is a no-op failure") &&
         expect(!badTransform.accepted &&
                    badTransform.message == "transform: payload mismatch" &&
                    badTransform.objectAction.status ==
                        app::CreativeEditorObjectActionOutcomeStatus::
                            PayloadMismatch,
                "SetObjectTransform with the wrong payload is a no-op failure") &&
         expect(!badLogic.accepted &&
                    badLogic.message == "set logic link: payload mismatch",
                "SetLogicLink with the wrong payload is a no-op failure") &&
         expect(!badMovingPlatform.accepted &&
                    badMovingPlatform.message ==
                        "moving platform settings: payload mismatch",
                "SetMovingPlatformSettings rejects a mismatched payload") &&
         expect(!badMovingPlatformPreview.accepted &&
                    badMovingPlatformPreview.message ==
                        "moving platform preview: payload mismatch",
                "moving platform preview rejects a mismatched payload") &&
         expect(!badMovingPlatformWaypoint.accepted &&
                    badMovingPlatformWaypoint.message ==
                        "moving platform waypoint: payload mismatch",
                "moving platform waypoint rejects a mismatched payload") &&
         expect(!badWorldLayoutManipulation.accepted &&
                    badWorldLayoutManipulation.message ==
                        "layout room manipulation: payload mismatch",
                "room manipulation rejects a mismatched payload") &&
         expect(!badRoofApertureManipulation.accepted &&
                    badRoofApertureManipulation.message ==
                        "layout roof aperture manipulation: payload mismatch",
                "roof aperture manipulation rejects a mismatched payload") &&
         expect(!badGeneratedRoomPreview.accepted &&
                    badGeneratedRoomPreview.message ==
                        "generated room preview: payload mismatch" &&
                    !badGeneratedRoomApply.accepted &&
                    badGeneratedRoomApply.message ==
                        "generated room settings: payload mismatch",
                "generated room commands reject mismatched payloads") &&
         expect(!badGeneratedLevelPreview.accepted &&
                    badGeneratedLevelPreview.message ==
                        "generated level preview: payload mismatch" &&
                    !badGeneratedLevelApply.accepted &&
                    badGeneratedLevelApply.message ==
                        "generated level settings: payload mismatch",
                "generated level commands reject mismatched payloads") &&
         expect(!badWorldLayoutLevelOperation.accepted &&
                    badWorldLayoutLevelOperation.message ==
                        "layout level operation: payload mismatch",
                "level operation rejects a mismatched payload") &&
         expect(!badWorldLayoutBuildingSelection.accepted &&
                    badWorldLayoutBuildingSelection.message ==
                        "layout building selection: payload mismatch",
                "building selection rejects a mismatched payload") &&
         expect(!badWorldLayoutSourceFocus.accepted &&
                    badWorldLayoutSourceFocus.message ==
                        "layout source focus: payload mismatch",
                "source focus rejects a mismatched payload") &&
         expect(!badWorldLayoutSourceScope.accepted &&
                    badWorldLayoutSourceScope.message ==
                        "layout source scope: payload mismatch",
                "source scope rejects a mismatched payload") &&
         expect(!badWorldLayoutObjectSourceFocus.accepted &&
                    badWorldLayoutObjectSourceFocus.message ==
                        "layout object source focus: payload mismatch" &&
                    !badWorldLayoutObjectSourceAdoption.accepted &&
                    badWorldLayoutObjectSourceAdoption.message ==
                        "layout object adoption: payload mismatch",
                "object source commands reject mismatched payloads") &&
         expect(!badWorldLayoutSourceRename.accepted &&
                    badWorldLayoutSourceRename.message ==
                        "layout source rename: payload mismatch",
                "source rename rejects a mismatched payload") &&
         expect(!badWorldLayoutSourceDuplicate.accepted &&
                    badWorldLayoutSourceDuplicate.message ==
                        "layout source duplicate: payload mismatch",
                "source duplicate rejects a mismatched payload") &&
         expect(!badWorldLayoutSourceDelete.accepted &&
                    badWorldLayoutSourceDelete.message ==
                        "layout source delete: payload mismatch",
                "source delete rejects a mismatched payload") &&
         expect(!badWorldLayoutBuildingManipulation.accepted &&
                    badWorldLayoutBuildingManipulation.message ==
                        "layout building manipulation: payload mismatch",
                "building manipulation rejects a mismatched payload") &&
         expect(!badWorldLayoutBuildingDuplicate.accepted &&
                    badWorldLayoutBuildingDuplicate.message ==
                        "layout building duplicate: payload mismatch",
                "building duplication rejects a mismatched payload") &&
         expect(!badWorldLayoutBuildingTransform.accepted &&
                    badWorldLayoutBuildingTransform.message ==
                        "layout building transform: payload mismatch",
                "building transform rejects a mismatched payload") &&
         expect(!badGeneratedBuildingPreview.accepted &&
                    badGeneratedBuildingPreview.message ==
                        "generated building preview: payload mismatch" &&
                    !badGeneratedBuildingApply.accepted &&
                    badGeneratedBuildingApply.message ==
                        "generated building operation: payload mismatch",
                "generated building commands reject mismatched payloads") &&
         expect(!badWorldLayoutTemplateCapture.accepted &&
                    badWorldLayoutTemplateCapture.message ==
                        "layout template capture: payload mismatch",
                "building template capture rejects a mismatched payload") &&
         expect(!badWorldLayoutTemplateUpdate.accepted &&
                    badWorldLayoutTemplateUpdate.message ==
                        "layout template update: payload mismatch",
                "building template update rejects a mismatched payload") &&
         expect(!badWorldLayoutTemplateDetach.accepted &&
                    badWorldLayoutTemplateDetach.message ==
                        "layout template detach: payload mismatch",
                "building template detach rejects a mismatched payload") &&
         expect(!badWorldLayoutTemplateRefresh.accepted &&
                    badWorldLayoutTemplateRefresh.message ==
                        "layout template refresh: payload mismatch",
                "building template refresh rejects a mismatched payload") &&
         expect(!badWorldLayoutTemplateSelection.accepted &&
                    badWorldLayoutTemplateSelection.message ==
                        "layout template selection: payload mismatch",
                "building template selection rejects a mismatched payload") &&
         expect(!badWorldLayoutTemplatePlacement.accepted &&
                    badWorldLayoutTemplatePlacement.message ==
                        "layout template placement: payload mismatch",
                "building template placement rejects a mismatched payload") &&
         expect(!badWorldLayoutBoxSettings.accepted &&
                    badWorldLayoutBoxSettings.message ==
                        "layout floor settings: payload mismatch",
                "floor settings reject a mismatched payload") &&
         expect(!badWorldLayoutBoxManipulation.accepted &&
                    badWorldLayoutBoxManipulation.message ==
                        "layout floor manipulation: payload mismatch",
                "floor manipulation rejects a mismatched payload") &&
         expect(!badGeneratedVerticalConnectorPreview.accepted &&
                    badGeneratedVerticalConnectorPreview.message ==
                        "generated vertical connector preview: payload mismatch" &&
                    !badGeneratedVerticalConnectorApply.accepted &&
                    badGeneratedVerticalConnectorApply.message ==
                        "generated vertical connector settings: payload mismatch",
                "generated connector commands reject mismatched payloads") &&
         expect(!badWorldLayoutVerticalConnectorManipulation.accepted &&
                    badWorldLayoutVerticalConnectorManipulation.message ==
                        "layout vertical connector manipulation: payload mismatch",
                "vertical connector manipulation rejects a mismatched payload") &&
         expect(!badWorldLayoutWallSettings.accepted &&
                    badWorldLayoutWallSettings.message ==
                        "layout partition settings: payload mismatch",
                "partition settings reject a mismatched payload") &&
         expect(!badWorldLayoutWallManipulation.accepted &&
                    badWorldLayoutWallManipulation.message ==
                        "layout partition manipulation: payload mismatch",
                "partition manipulation rejects a mismatched payload") &&
         expect(!badWorldLayoutOpeningSettings.accepted &&
                    badWorldLayoutOpeningSettings.message ==
                        "layout opening settings: payload mismatch",
                "opening settings reject a mismatched payload") &&
         expect(!badWorldLayoutOpeningInsert.accepted &&
                    badWorldLayoutOpeningInsert.message ==
                        "layout opening insert: payload mismatch",
                "opening insert replacement rejects a mismatched payload") &&
         expect(!badWorldLayoutOpeningManipulation.accepted &&
                    badWorldLayoutOpeningManipulation.message ==
                        "layout opening manipulation: payload mismatch",
                "opening manipulation rejects a mismatched payload") &&
         expect(!badWorldLayoutLevelSettings.accepted &&
                    badWorldLayoutLevelSettings.message ==
                        "layout level settings: payload mismatch",
                "level settings reject a mismatched payload") &&
         expect(!badWorldLayoutBuildingGrounding.accepted &&
                    badWorldLayoutBuildingGrounding.message ==
                        "layout building grounding: payload mismatch" &&
                    !badGeneratedBuildingGrounding.accepted &&
                    badGeneratedBuildingGrounding.message ==
                        "generated building grounding: payload mismatch",
                "building grounding commands reject mismatched payloads") &&
         expect(!badWorldLayoutObjectSettings.accepted &&
                    badWorldLayoutObjectSettings.message ==
                        "layout object settings: payload mismatch",
                "object settings reject a mismatched payload") &&
         expect(appState.facade.document().objectCount() == before &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "mismatched payloads mutate nothing and record no history");
}


}  // namespace

bool runCreativeDesktopObjectCommandTests() {
  bool ok = true;
  ok = selectCommandsRoundTripAndRespectIdBoundary() && ok;
  ok = focusObjectSelectsAndFramesThroughDispatcher() && ok;
  ok = frameSelectionAndSceneUseVisibleDocumentBounds() && ok;
  ok = logicCommandsRouteThroughTypedHistoryKernel() && ok;
  ok = deleteSelectionCommandRemovesGroupHierarchy() && ok;
  ok = explicitObjectDeleteRejectsWithoutPartialHierarchy() && ok;
  ok = renameObjectCommandChangesNameWithHistory() && ok;
  ok = visibilityAndLockCommandsSetAbsoluteState() && ok;
  ok = visibilityAndLockBatchesRollBackOnFailure() && ok;
  ok = transformCommandSetsAbsoluteWithMask() && ok;
  ok = transformCommandIsAtomicAndAttachmentAware() && ok;
  ok = groupPivotUsesItsDedicatedTypedCommand() && ok;
  ok = movingPlatformSettingsUseTypedCommandAndOneUndoStep() && ok;
  ok = playerSpawnSettingsUseTypedCommandAndOneUndoStep() && ok;
  ok = npcSpawnSettingsUseTypedCommandAndOneUndoStep() && ok;
  ok = objectiveSettingsUseTypedCommandsAndOneUndoStepEach() && ok;
  ok = movingPlatformWaypointCommandsSelectEditAndUndo() && ok;
  ok = movingPlatformPreviewCommandsStayTransient() && ok;
  ok = mismatchedPayloadsAreNoOpFailures() && ok;
  return ok;
}
