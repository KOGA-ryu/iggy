#include "EditorDesktopCommands.hpp"

#include "EditorAssetLibrary.hpp"
#include "EditorAuthoredAssets.hpp"
#include "EditorEdits.hpp"
#include "EditorFrame.hpp"
#include "EditorPersistence.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <numbers>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

// UI-2a (docs/creative_desktop_ui_plan.md DD-7): the desktop UI emits semantic
// command IDs into a bounded frame; EditorDesktopCommands.cpp is the sole
// dispatcher, reusing the existing kernels with the same history discipline as
// the keyboard path. Fully headless — no ImGui, no window.

namespace {
namespace cr = iggy3d::creative;
namespace app = iggy3d_creative_app;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool vecNear(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return std::fabs(lhs.x - rhs.x) < 1.0e-8 &&
         std::fabs(lhs.y - rhs.y) < 1.0e-8 &&
         std::fabs(lhs.z - rhs.z) < 1.0e-8;
}

cr::CreativeObjectId createCrate(cr::Facade& facade, double x) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.transform.position = {x, 0, 0};
  request.hasTransformOverride = true;
  return facade.createDocumentObject(request).objectId;
}

cr::CreativeObjectId createMovingPlatform(cr::Facade& facade) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::MovingPlatform;
  request.name = "Command Lift";
  request.transform.position = {1.0, 0.5, 2.0};
  request.hasTransformOverride = true;
  request.bounds = {{0.0, 0.25, 1.0}, {2.0, 0.75, 3.0}};
  request.hasBoundsOverride = true;
  request.pathPoints = {{{1.0, 0.5, 2.0}}, {{1.0, 3.5, 2.0}}};
  request.hasPathOverride = true;
  return facade.createDocumentObject(request).objectId;
}

struct AttachedPair {
  cr::CreativeObjectId parentId = cr::kInvalidObjectId;
  cr::CreativeObjectId childId = cr::kInvalidObjectId;
};

AttachedPair createAttachedPair(cr::Facade& facade) {
  cr::CreativeDocumentCreateRequest parent;
  parent.kind = cr::CreativeObjectKind::Prop;
  parent.name = "Attachment Parent";
  parent.transform.position = {2.0, 0.0, 3.0};
  parent.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt parentReceipt =
      facade.createDocumentObject(parent);

  cr::CreativeDocumentCreateRequest child;
  child.kind = cr::CreativeObjectKind::Door;
  child.name = "Attachment Child";
  child.transform.position = {2.5, 0.0, 3.0};
  child.hasTransformOverride = true;
  child.parentId = parentReceipt.objectId;
  child.attachmentSocket = "door_frame";
  const cr::CreativeDocumentCreateReceipt childReceipt =
      facade.createDocumentObject(child);
  return {parentReceipt.objectId, childReceipt.objectId};
}

void selectPrimary(cr::Facade& facade, cr::CreativeObjectId objectId) {
  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerPress;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = static_cast<cr::Id>(objectId);
  input.pointer.modifiers = cr::kCreativeToolModifierNone;
  static_cast<void>(facade.dispatchToolInput(input));
}

app::CreativeDesktopCommandResult dispatchOne(
    app::CreativeDesktopCommandId id,
    const app::CreativeDesktopCommandContext& context,
    std::string arg = {}) {
  app::CreativeDesktopCommandFrame frame;
  frame.push(id, std::move(arg));
  return app::dispatchCreativeDesktopCommands(frame, context);
}

// Step 3: push a single typed-payload command and dispatch it.
app::CreativeDesktopCommandResult dispatchPayload(
    app::CreativeDesktopCommandId id,
    const app::CreativeDesktopCommandContext& context,
    app::CreativeDesktopCommandPayload payload) {
  app::CreativeDesktopCommandFrame frame;
  frame.push(id, std::move(payload));
  return app::dispatchCreativeDesktopCommands(frame, context);
}

bool newDocumentReplacesAndClearsHistory() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd New");
  static_cast<void>(document.assignId(410U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  static_cast<void>(createCrate(appState.facade, 0.0));
  static_cast<void>(createCrate(appState.facade, 2.0));
  selectPrimary(appState.facade,
                createCrate(appState.facade, 4.0));
  static_cast<void>(app::duplicateSelectedObjectsWithUndo(
      appState, appState.history, cr::CreativeDuplicateCommandRequest{}, "seed"));

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const app::CreativeDesktopCommandResult result =
      dispatchOne(app::CreativeDesktopCommandId::NewDocument, context);

  return expect(result.accepted && result.documentReplaced,
                "new document is accepted and replaces the document") &&
         expect(appState.facade.document().objectCount() == 0U,
                "new document is blank") &&
         expect(cr::creativeUndoDepth(appState.history) == 0U,
                "new document clears undo history");
}

bool deleteAndDuplicateHitTheKernels() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Edit");
  static_cast<void>(document.assignId(411U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  static_cast<void>(createCrate(appState.facade, 0.0));
  const cr::CreativeObjectId target = createCrate(appState.facade, 2.0);
  selectPrimary(appState.facade, target);
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};

  const std::uint64_t before = appState.facade.document().objectCount();
  const app::CreativeDesktopCommandResult dup =
      dispatchOne(app::CreativeDesktopCommandId::DuplicateSelection, context);
  const bool grew = appState.facade.document().objectCount() == before + 1U;
  const app::CreativeDesktopCommandResult del =
      dispatchOne(app::CreativeDesktopCommandId::DeleteSelection, context);

  return expect(dup.accepted && dup.changed && grew,
                "duplicate adds one object via the kernel") &&
         expect(del.accepted && del.changed &&
                    appState.facade.document().objectCount() == before,
                "delete removes the duplicated object via the kernel");
}

bool undoRedoMoveTheHistoryRings() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Undo");
  static_cast<void>(document.assignId(412U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  selectPrimary(appState.facade, createCrate(appState.facade, 0.0));
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  static_cast<void>(
      dispatchOne(app::CreativeDesktopCommandId::DuplicateSelection, context));
  const bool recorded = cr::creativeUndoDepth(appState.history) == 1U;
  const app::CreativeDesktopCommandResult undo =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const bool undone = cr::creativeUndoDepth(appState.history) == 0U;
  const app::CreativeDesktopCommandResult redo =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  const bool redone = cr::creativeUndoDepth(appState.history) == 1U;

  return expect(recorded, "duplicate records one undo step") &&
         expect(undo.accepted && undone, "undo command pops the ring") &&
         expect(redo.accepted && redone, "redo command restores the ring");
}

bool saveAsRebindsTheActiveSlotAndClearsHistory() {
  const std::filesystem::path saveRoot =
      std::filesystem::temp_directory_path() / "iggy3d_desktop_cmd_tests";
  std::filesystem::remove_all(saveRoot);
  std::filesystem::create_directories(saveRoot);

  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Save");
  static_cast<void>(document.assignId(413U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  selectPrimary(appState.facade, createCrate(appState.facade, 0.0));
  static_cast<void>(app::duplicateSelectedObjectsWithUndo(
      appState, appState.history, cr::CreativeDuplicateCommandRequest{}, "seed"));

  app::CreativeEditorState editor;
  std::string saveId = "world_start";
  const app::CreativeDesktopCommandContext context{appState, editor, saveRoot,
                                                    &saveId};

  const app::CreativeDesktopCommandResult saveAs =
      dispatchOne(app::CreativeDesktopCommandId::SaveDocumentAs, context,
                  "world_named");
  const bool rebounded = saveId == "world_named";
  const bool historyCleared = cr::creativeUndoDepth(appState.history) == 0U;

  const app::CreativeDesktopCommandResult emptyName =
      dispatchOne(app::CreativeDesktopCommandId::SaveDocumentAs, context,
                  std::string{});

  std::filesystem::remove_all(saveRoot);
  return expect(saveAs.accepted && rebounded,
                "save as accepts and rebinds the active save id") &&
         expect(historyCleared, "save clears undo history like the keyboard path") &&
         expect(!emptyName.accepted,
                "save as with an empty name is rejected");
}

bool playIsUnsupportedAndFrameIsBounded() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Misc");
  static_cast<void>(document.assignId(414U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};

  const app::CreativeDesktopCommandResult play =
      dispatchOne(app::CreativeDesktopCommandId::Play, context);

  app::CreativeDesktopCommandFrame frame;
  for (std::size_t index = 0U;
       index < app::kCreativeDesktopCommandCapacity + 4U; ++index) {
    frame.push(app::CreativeDesktopCommandId::Undo);
  }

  return expect(!play.accepted && !play.message.empty(),
                "play returns an unsupported result with a message") &&
         expect(frame.overflowed &&
                    frame.count == app::kCreativeDesktopCommandCapacity,
                "the command frame is bounded and records overflow");
}

// --- Step 3: Desktop Command Expansion -------------------------------------

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
      dispatchPayload(app::CreativeDesktopCommandId::ClearSelection, context,
                      std::monostate{});
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
         expect(clearOk, "ClearSelection empties the selection") &&
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
      app::CreativeDesktopDeletePayload{{target}});

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

bool deleteObjectsCommandRemovesGroupHierarchy() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd DeleteMulti");
  static_cast<void>(document.assignId(416U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId a = createCrate(appState.facade, 0.0);
  const cr::CreativeObjectId b = createCrate(appState.facade, 2.0);
  const std::array<cr::CreativeObjectId, 2U> members{a, b};
  const cr::CreativeGroupCommandReceipt grouped =
      cr::groupDocumentObjectsAtomically(
          appState.facade.documentForPersistence(), members);
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
      dispatchPayload(app::CreativeDesktopCommandId::DeleteObjects, context,
                      app::CreativeDesktopDeletePayload{});

  return expect(grouped.accepted && before >= 3U,
                "group creates a root over both crates") &&
         expect(del.accepted && del.changed &&
                    appState.facade.document().objectCount() == 0U,
                "DeleteObjects removes the whole group hierarchy") &&
         expect(cr::creativeUndoDepth(appState.history) == 1U,
                "multi-delete records exactly one undo step") &&
         expect(appState.facade.findObject(a) == nullptr &&
                    appState.facade.findObject(b) == nullptr &&
                    appState.facade.findObject(grouped.groupObjectId) == nullptr,
                "no group member survives the delete");
}

bool deleteObjectsRejectsWithoutPartialHierarchy() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Delete Atomic");
  static_cast<void>(document.assignId(422U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const AttachedPair pair = createAttachedPair(appState.facade);
  static_cast<void>(cr::setDocumentObjectLocked(
      appState.facade.documentForPersistence(), pair.childId, true));
  appState.history = {};

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  const app::CreativeDesktopCommandResult locked = dispatchPayload(
      app::CreativeDesktopCommandId::DeleteObjects, context,
      app::CreativeDesktopDeletePayload{{pair.parentId}});
  const bool lockedRollback =
      !locked.accepted && !locked.changed &&
      appState.facade.findObject(pair.parentId) != nullptr &&
      appState.facade.findObject(pair.childId) != nullptr &&
      appState.facade.document().revision() == revisionBefore &&
      cr::creativeUndoDepth(appState.history) == 0U;

  static_cast<void>(cr::setDocumentObjectLocked(
      appState.facade.documentForPersistence(), pair.childId, false));
  appState.history = {};
  const std::uint64_t missingRevisionBefore =
      appState.facade.document().revision();
  const app::CreativeDesktopCommandResult missing = dispatchPayload(
      app::CreativeDesktopCommandId::DeleteObjects, context,
      app::CreativeDesktopDeletePayload{{pair.parentId, 999999U}});
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

  return expect(renamed.accepted && renamed.changed && nameOk,
                "RenameObject sets the object name") &&
         expect(cr::creativeUndoDepth(appState.history) == 1U,
                "rename records exactly one undo step") &&
         expect(!empty.accepted,
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

  return expect(hiddenOk, "SetObjectsVisible hides both listed objects") &&
         expect(lockedOk, "SetObjectsLocked locks the current selection") &&
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
  static_cast<void>(cr::setDocumentObjectLocked(
      appState.facade.documentForPersistence(), b, true));
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

  static_cast<void>(cr::setDocumentObjectLocked(
      appState.facade.documentForPersistence(), b, false));
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

bool assetAndInstanceCommandsRouteAndRejectCleanly() {
  // Verifies the asset/instance families route to the right kernels and honor
  // payload typing. The success paths reuse existing kernels covered by
  // creative_authored_asset_tests (K-6 equip is exercised there via the
  // refactored SaveSelectionAsAsset action), so they are not re-fixtured here.
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Cmd Assets");
  static_cast<void>(document.assignId(420U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId a = createCrate(appState.facade, 0.0);

  app::CreativeEditorState editor;  // empty authored-asset library.
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};

  const app::CreativeDesktopCommandResult equip = dispatchPayload(
      app::CreativeDesktopCommandId::EquipAsset, context,
      app::CreativeDesktopAssetOpPayload{"missing_asset", "",
                                         app::CreativeDesktopAssetEditPhase::None});
  const app::CreativeDesktopCommandResult renameAsset = dispatchPayload(
      app::CreativeDesktopCommandId::RenameAsset, context,
      app::CreativeDesktopAssetOpPayload{"missing_asset", "New Label",
                                         app::CreativeDesktopAssetEditPhase::None});
  const app::CreativeDesktopCommandResult dupAsset = dispatchPayload(
      app::CreativeDesktopCommandId::DuplicateAsset, context,
      app::CreativeDesktopAssetOpPayload{"missing_asset", "",
                                         app::CreativeDesktopAssetEditPhase::None});
  const app::CreativeDesktopCommandResult delAsset = dispatchPayload(
      app::CreativeDesktopCommandId::DeleteAsset, context,
      app::CreativeDesktopAssetOpPayload{"missing_asset", "",
                                         app::CreativeDesktopAssetEditPhase::None});
  const app::CreativeDesktopCommandResult editNone = dispatchPayload(
      app::CreativeDesktopCommandId::EditAssetSource, context,
      app::CreativeDesktopAssetOpPayload{"missing_asset", "",
                                         app::CreativeDesktopAssetEditPhase::None});
  const app::CreativeDesktopCommandResult editCancel = dispatchPayload(
      app::CreativeDesktopCommandId::EditAssetSource, context,
      app::CreativeDesktopAssetOpPayload{
          "missing_asset", "", app::CreativeDesktopAssetEditPhase::Cancel});
  const app::CreativeDesktopCommandResult refresh = dispatchPayload(
      app::CreativeDesktopCommandId::RefreshInstances, context,
      app::CreativeDesktopInstanceRefreshPayload{
          cr::kInvalidObjectId,
          cr::CreativeAuthoredAssetRefreshMode::ForceAll});
  const app::CreativeDesktopCommandResult update = dispatchPayload(
      app::CreativeDesktopCommandId::UpdateAssetFromInstance, context,
      app::CreativeDesktopInstanceRefreshPayload{
          cr::kInvalidObjectId,
          cr::CreativeAuthoredAssetRefreshMode::ForceAll});
  const app::CreativeDesktopCommandResult mismatch = dispatchPayload(
      app::CreativeDesktopCommandId::EquipAsset, context,
      app::CreativeDesktopSelectPayload{{a}, a});

  return expect(!equip.accepted && equip.message == "equip: unknown asset",
                "EquipAsset routes to the library and rejects an unknown id") &&
         expect(!renameAsset.accepted && !dupAsset.accepted && !delAsset.accepted,
                "rename/duplicate/delete of an unknown asset reject cleanly") &&
         expect(!editNone.accepted && !editCancel.accepted,
                "edit-source with no phase / no session rejects") &&
         expect(!refresh.accepted && !update.accepted,
                "instance ops on an invalid root reject") &&
         expect(!mismatch.accepted && mismatch.message == "equip: payload mismatch",
                "an asset command fed the wrong payload is a no-op failure");
}

bool assetAndInstanceCommandsCompleteSuccessPaths() {
  const auto nonce =
      std::chrono::steady_clock::now().time_since_epoch().count();
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      ("iggy3d_desktop_asset_commands_" + std::to_string(nonce));

  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Asset Success");
  static_cast<void>(document.assignId(425U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId sourceId = createCrate(appState.facade, 0.0);
  selectPrimary(appState.facade, sourceId);

  app::CreativeEditorState editor;
  const app::CreativeEditorAuthoredAssetLoadReceipt loaded =
      app::loadCreativeEditorAuthoredAssetLibrary(editor.authoredAssets, root);
  const app::CreativeEditorAuthoredAssetSaveReceipt saved =
      app::saveCreativeEditorSelectionAsAuthoredAsset(
          appState, editor.authoredAssets, "Desktop Asset");
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor, root,
                                                    &saveId};

  const app::CreativeDesktopCommandResult equipped = dispatchPayload(
      app::CreativeDesktopCommandId::EquipAsset, context,
      app::CreativeDesktopAssetOpPayload{
          saved.assetId, "", app::CreativeDesktopAssetEditPhase::None});
  const bool equipOk =
      equipped.accepted &&
      cr::creativeHotbarAssetId(cr::selectedCreativeHotbarEntry(
          editor.interaction.hotbar)) == saved.assetId;
  const app::CreativeDesktopCommandResult renamed = dispatchPayload(
      app::CreativeDesktopCommandId::RenameAsset, context,
      app::CreativeDesktopAssetOpPayload{
          saved.assetId, "Desktop Asset Renamed",
          app::CreativeDesktopAssetEditPhase::None});
  const cr::CreativeAuthoredAssetDefinition* renamedDefinition =
      app::findCreativeEditorAuthoredAsset(editor.authoredAssets,
                                           saved.assetId);
  const bool renameOk =
      renamed.accepted && renamedDefinition != nullptr &&
      renamedDefinition->label == "Desktop Asset Renamed";

  const app::CreativeDesktopCommandResult duplicated = dispatchPayload(
      app::CreativeDesktopCommandId::DuplicateAsset, context,
      app::CreativeDesktopAssetOpPayload{
          saved.assetId, "", app::CreativeDesktopAssetEditPhase::None});
  std::string duplicateId;
  for (const cr::CreativeAuthoredAssetDefinition& definition :
       editor.authoredAssets.definitions) {
    if (definition.assetId != saved.assetId) {
      duplicateId = definition.assetId;
      break;
    }
  }
  const app::CreativeDesktopCommandResult deletedDuplicate = dispatchPayload(
      app::CreativeDesktopCommandId::DeleteAsset, context,
      app::CreativeDesktopAssetOpPayload{
          duplicateId, "", app::CreativeDesktopAssetEditPhase::None});
  const bool duplicateDeleteOk =
      duplicated.accepted && !duplicateId.empty() &&
      deletedDuplicate.accepted &&
      app::findCreativeEditorAuthoredAsset(editor.authoredAssets,
                                           duplicateId) == nullptr;

  const app::CreativeDesktopCommandResult editBegun = dispatchPayload(
      app::CreativeDesktopCommandId::EditAssetSource, context,
      app::CreativeDesktopAssetOpPayload{
          saved.assetId, "", app::CreativeDesktopAssetEditPhase::Begin});
  const app::CreativeDesktopCommandResult editCancelled = dispatchPayload(
      app::CreativeDesktopCommandId::EditAssetSource, context,
      app::CreativeDesktopAssetOpPayload{
          saved.assetId, "", app::CreativeDesktopAssetEditPhase::Cancel});
  const app::CreativeDesktopCommandResult editBegunAgain = dispatchPayload(
      app::CreativeDesktopCommandId::EditAssetSource, context,
      app::CreativeDesktopAssetOpPayload{
          saved.assetId, "", app::CreativeDesktopAssetEditPhase::Begin});
  const app::CreativeDesktopCommandResult editSaved = dispatchPayload(
      app::CreativeDesktopCommandId::EditAssetSource, context,
      app::CreativeDesktopAssetOpPayload{
          saved.assetId, "", app::CreativeDesktopAssetEditPhase::Save});
  const bool editLifecycleOk =
      editBegun.accepted && editCancelled.accepted &&
      editBegunAgain.accepted && editSaved.accepted;

  renamedDefinition = app::findCreativeEditorAuthoredAsset(
      editor.authoredAssets, saved.assetId);
  cr::CreativeAuthoredAssetPlacementRequest firstPlacement;
  firstPlacement.definition = renamedDefinition;
  firstPlacement.instanceTransform.position = {10.0, 0.0, 10.0};
  const cr::CreativeAuthoredAssetInstanceReceipt first =
      cr::instantiateCreativeAuthoredAssetAtomically(
          appState.facade.documentForPersistence(), firstPlacement);
  renamedDefinition = app::findCreativeEditorAuthoredAsset(
      editor.authoredAssets, saved.assetId);
  cr::CreativeAuthoredAssetPlacementRequest secondPlacement;
  secondPlacement.definition = renamedDefinition;
  secondPlacement.instanceTransform.position = {20.0, 0.0, 20.0};
  const cr::CreativeAuthoredAssetInstanceReceipt second =
      cr::instantiateCreativeAuthoredAssetAtomically(
          appState.facade.documentForPersistence(), secondPlacement);
  cr::CreativeDocumentCreateRequest detail;
  detail.kind = cr::CreativeObjectKind::Crate;
  detail.name = "Instance Detail";
  detail.transform.position = {10.0, 1.0, 10.0};
  detail.hasTransformOverride = true;
  detail.parentId = first.instanceRootObjectId;
  const cr::CreativeDocumentCreateReceipt detailCreated =
      appState.facade.createDocumentObject(detail);
  const app::CreativeDesktopCommandResult updated = dispatchPayload(
      app::CreativeDesktopCommandId::UpdateAssetFromInstance, context,
      app::CreativeDesktopInstanceRefreshPayload{
          first.instanceRootObjectId,
          cr::CreativeAuthoredAssetRefreshMode::ForceAll});
  const app::CreativeDesktopCommandResult refreshed = dispatchPayload(
      app::CreativeDesktopCommandId::RefreshInstances, context,
      app::CreativeDesktopInstanceRefreshPayload{
          second.instanceRootObjectId,
          cr::CreativeAuthoredAssetRefreshMode::SelectedInstance});
  const bool instanceOk =
      first.accepted && second.accepted && detailCreated.accepted &&
      updated.accepted && refreshed.accepted;

  std::error_code ignored;
  std::filesystem::remove_all(root, ignored);
  return expect(loaded.accepted && saved.accepted && equipOk,
                "EquipAsset succeeds for a durable authored asset") &&
         expect(renameOk && duplicateDeleteOk,
                "asset rename, duplicate, and unreferenced delete succeed") &&
         expect(editLifecycleOk,
                "asset edit begin, save, and cancel route through dispatcher") &&
         expect(instanceOk,
                "instance update and selected refresh succeed through payloads");
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

  const app::CreativeDesktopCommandResult badDelete = dispatchPayload(
      app::CreativeDesktopCommandId::DeleteObjects, context,
      app::CreativeDesktopSelectPayload{{a}, a});
  const app::CreativeDesktopCommandResult badRename = dispatchPayload(
      app::CreativeDesktopCommandId::RenameObject, context, std::monostate{});
  const app::CreativeDesktopCommandResult badTransform = dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectTransform, context,
      app::CreativeDesktopDeletePayload{{a}});
  const app::CreativeDesktopCommandResult badLogic = dispatchPayload(
      app::CreativeDesktopCommandId::SetLogicLink, context,
      app::CreativeDesktopSelectPayload{{a}, a});
  const app::CreativeDesktopCommandResult badMovingPlatform = dispatchPayload(
      app::CreativeDesktopCommandId::SetMovingPlatformSettings, context,
      app::CreativeDesktopDeletePayload{{a}});
  const app::CreativeDesktopCommandResult badMovingPlatformPreview =
      dispatchPayload(
          app::CreativeDesktopCommandId::SeekMovingPlatformPreview, context,
          app::CreativeDesktopDeletePayload{{a}});
  const app::CreativeDesktopCommandResult badMovingPlatformWaypoint =
      dispatchPayload(
          app::CreativeDesktopCommandId::SetMovingPlatformWaypointDwell,
          context, app::CreativeDesktopDeletePayload{{a}});
  const app::CreativeDesktopCommandResult badWorldLayoutManipulation =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutManipulateRoom, context,
          app::CreativeDesktopDeletePayload{{a}});
  const app::CreativeDesktopCommandResult badWorldLayoutBuildingSelection =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutSelectBuilding, context,
          app::CreativeDesktopDeletePayload{{a}});
  const app::CreativeDesktopCommandResult badWorldLayoutBuildingManipulation =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutManipulateBuilding,
          context, app::CreativeDesktopDeletePayload{{a}});
  const app::CreativeDesktopCommandResult badWorldLayoutBuildingDuplicate =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutDuplicateBuilding,
          context, app::CreativeDesktopDeletePayload{{a}});
  const app::CreativeDesktopCommandResult badWorldLayoutBuildingTransform =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutTransformBuilding, context,
          app::CreativeDesktopDeletePayload{{a}});
  const app::CreativeDesktopCommandResult badWorldLayoutBoxSettings =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutSetBoxSettings, context,
          app::CreativeDesktopDeletePayload{{a}});
  const app::CreativeDesktopCommandResult badWorldLayoutBoxManipulation =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutManipulateBox, context,
          app::CreativeDesktopDeletePayload{{a}});
  const app::CreativeDesktopCommandResult badWorldLayoutWallSettings =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutSetWallSettings, context,
          app::CreativeDesktopDeletePayload{{a}});
  const app::CreativeDesktopCommandResult badWorldLayoutWallManipulation =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutManipulateWall, context,
          app::CreativeDesktopDeletePayload{{a}});
  const app::CreativeDesktopCommandResult badWorldLayoutOpeningSettings =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutSetOpeningSettings, context,
          app::CreativeDesktopDeletePayload{{a}});
  const app::CreativeDesktopCommandResult badWorldLayoutOpeningManipulation =
      dispatchPayload(
          app::CreativeDesktopCommandId::WorldLayoutManipulateOpening, context,
          app::CreativeDesktopDeletePayload{{a}});

  return expect(!badDelete.accepted &&
                    badDelete.message == "delete objects: payload mismatch",
                "DeleteObjects with the wrong payload is a no-op failure") &&
         expect(!badRename.accepted &&
                    badRename.message == "rename: payload mismatch",
                "RenameObject with no payload is a no-op failure") &&
         expect(!badTransform.accepted &&
                    badTransform.message == "transform: payload mismatch",
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
         expect(!badWorldLayoutBuildingSelection.accepted &&
                    badWorldLayoutBuildingSelection.message ==
                        "layout building selection: payload mismatch",
                "building selection rejects a mismatched payload") &&
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
         expect(!badWorldLayoutBoxSettings.accepted &&
                    badWorldLayoutBoxSettings.message ==
                        "layout floor settings: payload mismatch",
                "floor settings reject a mismatched payload") &&
         expect(!badWorldLayoutBoxManipulation.accepted &&
                    badWorldLayoutBoxManipulation.message ==
                        "layout floor manipulation: payload mismatch",
                "floor manipulation rejects a mismatched payload") &&
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
         expect(!badWorldLayoutOpeningManipulation.accepted &&
                    badWorldLayoutOpeningManipulation.message ==
                        "layout opening manipulation: payload mismatch",
                "opening manipulation rejects a mismatched payload") &&
         expect(appState.facade.document().objectCount() == before &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "mismatched payloads mutate nothing and record no history");
}

bool worldLayoutStructuralCommandsRouteThroughDispatcher() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd Structural Layout");
  static_cast<void>(document.assignId(423U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};

  const auto setFloorTool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Floor});
  const auto floorBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Begin, {0.0, 0.0}});
  const auto floorCreate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Commit, {4.0, 3.0}});
  const std::uint64_t revisionBeforeFloorSettings = editor.worldLayout.revision;
  const auto floorSettings = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetBoxSettings, context,
      app::CreativeDesktopWorldLayoutBoxSettingsPayload{
          0U, {{{0, 0}, {4, 3}}, 1, 2U}});
  const bool floorSettingsCommittedOnce =
      editor.worldLayout.revision == revisionBeforeFloorSettings + 1U;
  static_cast<void>(dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Select}));
  const std::uint64_t revisionBeforeFloorMove = editor.worldLayout.revision;
  const auto floorMoveBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateBox, context,
      app::CreativeDesktopWorldLayoutBoxManipulationPayload{
          app::CreativeEditorWorldLayoutBoxManipulationPhase::Begin,
          {2.0, 1.5}, 0.2});
  const auto floorMoveUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateBox, context,
      app::CreativeDesktopWorldLayoutBoxManipulationPayload{
          app::CreativeEditorWorldLayoutBoxManipulationPhase::Update,
          {4.2, 2.6}, 0.2});
  const bool floorMovePreviewOnly =
      editor.worldLayout.revision == revisionBeforeFloorMove &&
      editor.worldLayout.source.boxes[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{0, 0} &&
      editor.worldLayout.boxManipulation.previewFootprint.minimum ==
          cr::CreativeTerrainCoord2{2, 1};
  const auto floorMoveCommit = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateBox, context,
      app::CreativeDesktopWorldLayoutBoxManipulationPayload{
          app::CreativeEditorWorldLayoutBoxManipulationPhase::Commit,
          {4.2, 2.6}, 0.2});
  const bool floorMoveCommittedOnce =
      editor.worldLayout.revision == revisionBeforeFloorMove + 1U;

  const auto setWallTool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Wall});
  const auto wallBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Begin, {0.0, 0.0}});
  const auto wallCreate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Commit, {8.0, 0.0}});
  const std::uint64_t revisionBeforeWallSettings = editor.worldLayout.revision;
  const auto wallSettings = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetWallSettings, context,
      app::CreativeDesktopWorldLayoutWallSettingsPayload{
          0U, {{0, 0}, {8, 0}, 0.5, 4U, 0.5}});
  const bool wallSettingsCommittedOnce =
      editor.worldLayout.revision == revisionBeforeWallSettings + 1U;
  static_cast<void>(dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Select}));
  const std::uint64_t revisionBeforeWallMove = editor.worldLayout.revision;
  const auto wallMoveBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateWall, context,
      app::CreativeDesktopWorldLayoutWallManipulationPayload{
          app::CreativeEditorWorldLayoutWallManipulationPhase::Begin,
          {4.0, 0.0}, 0.2});
  const auto wallMoveUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateWall, context,
      app::CreativeDesktopWorldLayoutWallManipulationPayload{
          app::CreativeEditorWorldLayoutWallManipulationPhase::Update,
          {5.2, 2.1}, 0.2});
  const bool wallMovePreviewOnly =
      editor.worldLayout.revision == revisionBeforeWallMove &&
      editor.worldLayout.source.walls[0].start ==
          cr::CreativeTerrainCoord2{0, 0} &&
      editor.worldLayout.wallManipulation.previewStart ==
          cr::CreativeTerrainCoord2{1, 2};
  const auto wallMoveCommit = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateWall, context,
      app::CreativeDesktopWorldLayoutWallManipulationPayload{
          app::CreativeEditorWorldLayoutWallManipulationPhase::Commit,
          {5.2, 2.1}, 0.2});
  const cr::CreativeWorldLayoutRect floorAfterMove =
      editor.worldLayout.source.boxes[0].footprint;
  const cr::CreativeWorldLayoutWall wallAfterMove =
      editor.worldLayout.source.walls[0];
  const std::uint64_t revisionAfterWallMove = editor.worldLayout.revision;

  const auto buildingSelected = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSelectBuilding, context,
      app::CreativeDesktopWorldLayoutBuildingSelectionPayload{0U});
  const std::uint64_t revisionBeforeBuildingMove =
      editor.worldLayout.revision;
  const auto buildingMoveBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateBuilding, context,
      app::CreativeDesktopWorldLayoutBuildingManipulationPayload{
          app::CreativeEditorWorldLayoutBuildingManipulationPhase::Begin,
          {4.0, 2.0}, 0.2});
  const auto buildingMoveUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateBuilding, context,
      app::CreativeDesktopWorldLayoutBuildingManipulationPayload{
          app::CreativeEditorWorldLayoutBuildingManipulationPhase::Update,
          {6.2, 3.1}, 0.2});
  const bool buildingMovePreviewOnly =
      editor.worldLayout.revision == revisionBeforeBuildingMove &&
      editor.worldLayout.source.walls[0].start ==
          cr::CreativeTerrainCoord2{1, 2} &&
      editor.worldLayout.buildingManipulation.previewDeltaXCells == 2 &&
      editor.worldLayout.buildingManipulation.previewDeltaZCells == 1;
  const auto buildingMoveCommit = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateBuilding, context,
      app::CreativeDesktopWorldLayoutBuildingManipulationPayload{
          app::CreativeEditorWorldLayoutBuildingManipulationPhase::Commit,
          {6.2, 3.1}, 0.2});
  const bool buildingMoveCommittedOnce =
      editor.worldLayout.revision == revisionBeforeBuildingMove + 1U;
  std::int64_t duplicateDeltaX = 0;
  std::int64_t duplicateDeltaZ = 0;
  const bool hasDuplicateOffset =
      app::defaultCreativeEditorWorldLayoutBuildingDuplicateOffset(
          editor.worldLayout, 0U, duplicateDeltaX, duplicateDeltaZ);
  const std::uint64_t revisionBeforeBuildingDuplicate =
      editor.worldLayout.revision;
  const auto buildingDuplicated = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutDuplicateBuilding, context,
      app::CreativeDesktopWorldLayoutBuildingDuplicatePayload{
          0U, duplicateDeltaX, duplicateDeltaZ});
  const bool buildingDuplicatedOnce =
      editor.worldLayout.revision == revisionBeforeBuildingDuplicate + 1U;
  const std::uint64_t revisionBeforeBuildingTransform =
      editor.worldLayout.revision;
  const auto buildingTransformPreview = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutTransformBuilding, context,
      app::CreativeDesktopWorldLayoutBuildingTransformPayload{
          app::CreativeEditorWorldLayoutBuildingTransformPhase::Preview,
          cr::CreativeWorldLayoutBuildingTransformOperation::MirrorZ});
  const bool buildingTransformPreviewOnly =
      buildingTransformPreview.accepted && buildingTransformPreview.changed &&
      !buildingTransformPreview.worldLayoutChanged &&
      editor.worldLayout.revision == revisionBeforeBuildingTransform &&
      editor.worldLayout.source.walls[1].start ==
          cr::CreativeTerrainCoord2{13, 3} &&
      app::creativeEditorWorldLayoutDisplaySource(editor.worldLayout)
              .walls[1]
              .start == cr::CreativeTerrainCoord2{13, 4};
  const auto buildingTransformCommit = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutTransformBuilding, context,
      app::CreativeDesktopWorldLayoutBuildingTransformPayload{
          app::CreativeEditorWorldLayoutBuildingTransformPhase::Commit,
          cr::CreativeWorldLayoutBuildingTransformOperation::MirrorZ});
  const bool buildingTransformCommittedOnce =
      buildingTransformCommit.accepted && buildingTransformCommit.changed &&
      buildingTransformCommit.worldLayoutChanged &&
      editor.worldLayout.revision == revisionBeforeBuildingTransform + 1U &&
      editor.worldLayout.source.walls[1].start ==
          cr::CreativeTerrainCoord2{13, 4};
  const auto groupModeCleared = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutClearSelection, context);

  return expect(setFloorTool.accepted && floorBegin.accepted &&
                    floorCreate.accepted && floorCreate.worldLayoutChanged &&
                    floorSettings.accepted && floorSettings.changed &&
                    floorSettings.worldLayoutChanged &&
                    floorSettingsCommittedOnce && floorMoveBegin.accepted &&
                    floorMoveBegin.changed &&
                    !floorMoveBegin.worldLayoutChanged &&
                    floorMoveUpdate.accepted && floorMoveUpdate.changed &&
                    !floorMoveUpdate.worldLayoutChanged &&
                    floorMovePreviewOnly && floorMoveCommit.accepted &&
                    floorMoveCommit.changed &&
                    floorMoveCommit.worldLayoutChanged &&
                    floorMoveCommittedOnce &&
                    floorAfterMove.minimum ==
                        cr::CreativeTerrainCoord2{2, 1} &&
                    editor.worldLayout.source.boxes[0].baseLayer == 1 &&
                    editor.worldLayout.source.boxes[0].heightCells == 2U,
                "floor settings and manipulation use typed dispatcher commands") &&
         expect(setWallTool.accepted && wallBegin.accepted &&
                    wallCreate.accepted && wallCreate.worldLayoutChanged &&
                    wallSettings.accepted && wallSettings.changed &&
                    wallSettings.worldLayoutChanged &&
                    wallSettingsCommittedOnce && wallMoveBegin.accepted &&
                    wallMoveBegin.changed &&
                    !wallMoveBegin.worldLayoutChanged &&
                    wallMoveUpdate.accepted && wallMoveUpdate.changed &&
                    !wallMoveUpdate.worldLayoutChanged &&
                    wallMovePreviewOnly && wallMoveCommit.accepted &&
                    wallMoveCommit.changed &&
                    wallMoveCommit.worldLayoutChanged &&
                    revisionAfterWallMove == revisionBeforeWallMove + 1U &&
                    wallAfterMove.start ==
                        cr::CreativeTerrainCoord2{1, 2} &&
                    wallAfterMove.end ==
                        cr::CreativeTerrainCoord2{9, 2} &&
                    wallAfterMove.baseLayer == 0.5 &&
                    wallAfterMove.heightCells == 4U &&
                    wallAfterMove.thicknessCells == 0.5,
                "partition settings and manipulation use typed dispatcher commands") &&
         expect(buildingSelected.accepted && buildingSelected.changed &&
                    buildingMoveBegin.accepted && buildingMoveBegin.changed &&
                    !buildingMoveBegin.worldLayoutChanged &&
                    buildingMoveUpdate.accepted &&
                    buildingMoveUpdate.changed &&
                    !buildingMoveUpdate.worldLayoutChanged &&
                    buildingMovePreviewOnly && buildingMoveCommit.accepted &&
                    buildingMoveCommit.changed &&
                    buildingMoveCommit.worldLayoutChanged &&
                    buildingMoveCommittedOnce && hasDuplicateOffset &&
                    duplicateDeltaX == 10 && duplicateDeltaZ == 0 &&
                    buildingDuplicated.accepted &&
                    buildingDuplicated.changed &&
                    buildingDuplicated.worldLayoutChanged &&
                    buildingDuplicatedOnce &&
                    editor.worldLayout.source.buildings.size() == 2U &&
                    editor.worldLayout.source.boxes.size() == 2U &&
                    editor.worldLayout.source.walls.size() == 2U &&
                    editor.worldLayout.source.boxes[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{4, 2} &&
                    editor.worldLayout.source.boxes[1].footprint.minimum ==
                        cr::CreativeTerrainCoord2{14, 2} &&
                    editor.worldLayout.source.walls[0].start ==
                        cr::CreativeTerrainCoord2{3, 3} &&
                    editor.worldLayout.source.walls[1].start ==
                        cr::CreativeTerrainCoord2{13, 4} &&
                    buildingTransformPreviewOnly &&
                    buildingTransformCommittedOnce &&
                    groupModeCleared.accepted && groupModeCleared.changed &&
                    editor.worldLayout.selection.kind ==
                        app::CreativeEditorWorldLayoutSelectionKind::None,
                "building group commands preview, commit, duplicate, and clear semantically");
}

bool worldLayoutCommandsPreviewAndGenerateThroughDispatcher() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Cmd World Layout");
  static_cast<void>(document.assignId(422U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{appState, editor,
                                                    std::filesystem::path{},
                                                    &saveId};

  const app::CreativeDesktopCommandResult tool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Room});
  const app::CreativeDesktopCommandResult anchor = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Begin, {0.0, 0.0}});
  const app::CreativeDesktopCommandResult room = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Commit, {6.0, 5.0}});
  const app::CreativeDesktopCommandResult resized = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetRoomSettings, context,
      app::CreativeDesktopWorldLayoutRoomSettingsPayload{
          0U, {{{0, 0}, {8, 6}}, 1, 4U, 0.5, 1U}});
  const app::CreativeDesktopCommandResult selectTool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Select});
  const std::uint64_t revisionBeforeMove = editor.worldLayout.revision;
  const app::CreativeDesktopCommandResult moveBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoom, context,
      app::CreativeDesktopWorldLayoutRoomManipulationPayload{
          app::CreativeEditorWorldLayoutRoomManipulationPhase::Begin,
          {3.0, 3.0}, 0.3});
  const app::CreativeDesktopCommandResult moveUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoom, context,
      app::CreativeDesktopWorldLayoutRoomManipulationPayload{
          app::CreativeEditorWorldLayoutRoomManipulationPhase::Update,
          {5.0, 4.0}, 0.3});
  const bool moveWasPreviewOnly =
      editor.worldLayout.revision == revisionBeforeMove &&
      editor.worldLayout.source.rooms[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{0, 0} &&
      editor.worldLayout.source.rooms[0].footprint.maximum ==
          cr::CreativeTerrainCoord2{8, 6} &&
      editor.worldLayout.roomManipulation.previewFootprint.minimum ==
          cr::CreativeTerrainCoord2{2, 1} &&
      editor.worldLayout.roomManipulation.previewFootprint.maximum ==
          cr::CreativeTerrainCoord2{10, 7};
  const app::CreativeDesktopCommandResult moved = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateRoom, context,
      app::CreativeDesktopWorldLayoutRoomManipulationPayload{
          app::CreativeEditorWorldLayoutRoomManipulationPhase::Commit,
          {5.0, 4.0}, 0.3});
  const bool roomMoveCommittedOnce =
      editor.worldLayout.revision == revisionBeforeMove + 1U;
  const app::CreativeDesktopCommandResult doorTool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Door});
  const app::CreativeDesktopCommandResult door = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasPoint, context,
      app::CreativeDesktopWorldLayoutPointPayload{{5.0, 1.0}});
  const std::uint64_t revisionBeforeOpeningSettings =
      editor.worldLayout.revision;
  const app::CreativeDesktopCommandResult openingSettings = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetOpeningSettings, context,
      app::CreativeDesktopWorldLayoutOpeningSettingsPayload{
          0U,
          {3.0, 1.5, 0.0, 2.5,
           cr::CreativeBuildingOpeningPose::OpenFromStartPositiveNormal,
           true}});
  const bool openingSettingsCommittedOnce =
      editor.worldLayout.revision == revisionBeforeOpeningSettings + 1U;
  const app::CreativeDesktopCommandResult selectOpeningTool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Select});
  const std::uint64_t revisionBeforeOpeningMove = editor.worldLayout.revision;
  const app::CreativeDesktopCommandResult openingMoveBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateOpening, context,
      app::CreativeDesktopWorldLayoutOpeningManipulationPayload{
          app::CreativeEditorWorldLayoutOpeningManipulationPhase::Begin,
          {5.0, 1.0}, 0.2});
  const app::CreativeDesktopCommandResult openingMoveUpdate = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateOpening, context,
      app::CreativeDesktopWorldLayoutOpeningManipulationPayload{
          app::CreativeEditorWorldLayoutOpeningManipulationPhase::Update,
          {6.12, 1.0}, 0.2});
  const bool openingMoveWasPreviewOnly =
      editor.worldLayout.revision == revisionBeforeOpeningMove &&
      editor.worldLayout.source.openings[0].centerOffsetCells == 3.0 &&
      editor.worldLayout.openingManipulation.previewCenterOffsetCells == 4.0;
  const app::CreativeDesktopCommandResult openingMoved = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutManipulateOpening, context,
      app::CreativeDesktopWorldLayoutOpeningManipulationPayload{
          app::CreativeEditorWorldLayoutOpeningManipulationPhase::Commit,
          {6.12, 1.0}, 0.2});

  const std::uint64_t liveCountBefore =
      appState.facade.document().objectCount();
  const app::CreativeDesktopCommandResult preview = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutPreview, context);
  const bool exactPreviewVisible =
      app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout) &&
      app::creativeEditorWorldLayoutRenderDocument(
          editor.worldLayout, appState.facade.document())
              .objectCount() > liveCountBefore;
  const app::CreativeDesktopCommandResult cancelled = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutCancelPreview, context);
  const bool canvasRestoredAfterCancel =
      editor.desktopUi.showWorldLayout &&
      !app::creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
  const app::CreativeDesktopCommandResult previewAgain = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutPreview, context);
  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);

  return expect(tool.accepted && anchor.accepted && !anchor.changed &&
                    room.accepted && room.worldLayoutChanged &&
                    resized.accepted && resized.worldLayoutChanged &&
                    selectTool.accepted && moveBegin.accepted &&
                    moveBegin.changed && !moveBegin.worldLayoutChanged &&
                    moveUpdate.accepted && moveUpdate.changed &&
                    !moveUpdate.worldLayoutChanged && moveWasPreviewOnly &&
                    moved.accepted && moved.changed &&
                    moved.worldLayoutChanged && roomMoveCommittedOnce &&
                    editor.worldLayout.source.rooms[0].footprint.minimum ==
                        cr::CreativeTerrainCoord2{2, 1} &&
                    editor.worldLayout.source.rooms[0].footprint.maximum ==
                        cr::CreativeTerrainCoord2{10, 7} &&
                    editor.worldLayout.source.rooms[0].baseLayer == 1 &&
                    editor.worldLayout.source.rooms[0].wallHeightCells == 4U &&
                    editor.worldLayout.source.rooms[0].wallThicknessCells ==
                        0.5,
                "room settings and preview-only manipulation route through typed payloads") &&
         expect(doorTool.accepted && door.accepted && door.worldLayoutChanged &&
                    openingSettings.accepted && openingSettings.changed &&
                    openingSettings.worldLayoutChanged &&
                    openingSettingsCommittedOnce &&
                    selectOpeningTool.accepted && openingMoveBegin.accepted &&
                    openingMoveBegin.changed &&
                    !openingMoveBegin.worldLayoutChanged &&
                    openingMoveUpdate.accepted && openingMoveUpdate.changed &&
                    !openingMoveUpdate.worldLayoutChanged &&
                    openingMoveWasPreviewOnly && openingMoved.accepted &&
                    openingMoved.changed && openingMoved.worldLayoutChanged &&
                    editor.worldLayout.revision ==
                        revisionBeforeOpeningMove + 1U &&
                    editor.worldLayout.source.openings[0].centerOffsetCells ==
                        4.0 &&
                    editor.worldLayout.source.openings[0].widthCells == 1.5,
                "opening settings and preview-only movement route through typed payloads") &&
         expect(preview.accepted && preview.sceneChanged &&
                    exactPreviewVisible && !editor.desktopUi.showWorldLayout,
                "layout preview is exact, transient, and closes the canvas") &&
         expect(cancelled.accepted && cancelled.sceneChanged &&
                    canvasRestoredAfterCancel,
                "layout preview cancel restores the canvas") &&
         expect(previewAgain.accepted && generated.accepted &&
                    generated.changed && generated.sceneChanged,
                "layout confirm publishes through the semantic dispatcher") &&
         expect(appState.facade.document().objectCount() > liveCountBefore &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "layout generation installs objects with one undo entry");
}

}  // namespace

int main() {
  bool ok = true;
  ok = newDocumentReplacesAndClearsHistory() && ok;
  ok = deleteAndDuplicateHitTheKernels() && ok;
  ok = undoRedoMoveTheHistoryRings() && ok;
  ok = saveAsRebindsTheActiveSlotAndClearsHistory() && ok;
  ok = playIsUnsupportedAndFrameIsBounded() && ok;
  // Step 3 — Desktop Command Expansion.
  ok = selectCommandsRoundTripAndRespectIdBoundary() && ok;
  ok = focusObjectSelectsAndFramesThroughDispatcher() && ok;
  ok = logicCommandsRouteThroughTypedHistoryKernel() && ok;
  ok = deleteObjectsCommandRemovesGroupHierarchy() && ok;
  ok = deleteObjectsRejectsWithoutPartialHierarchy() && ok;
  ok = renameObjectCommandChangesNameWithHistory() && ok;
  ok = visibilityAndLockCommandsSetAbsoluteState() && ok;
  ok = visibilityAndLockBatchesRollBackOnFailure() && ok;
  ok = transformCommandSetsAbsoluteWithMask() && ok;
  ok = transformCommandIsAtomicAndAttachmentAware() && ok;
  ok = movingPlatformSettingsUseTypedCommandAndOneUndoStep() && ok;
  ok = movingPlatformWaypointCommandsSelectEditAndUndo() && ok;
  ok = movingPlatformPreviewCommandsStayTransient() && ok;
  ok = assetAndInstanceCommandsRouteAndRejectCleanly() && ok;
  ok = assetAndInstanceCommandsCompleteSuccessPaths() && ok;
  ok = mismatchedPayloadsAreNoOpFailures() && ok;
  ok = worldLayoutStructuralCommandsRouteThroughDispatcher() && ok;
  ok = worldLayoutCommandsPreviewAndGenerateThroughDispatcher() && ok;
  return ok ? 0 : 1;
}
