#include "EditorPattern.hpp"
#include "EditorState.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"

namespace {
namespace cr = iggy3d::creative;
namespace app = iggy3d_creative_app;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool installDocument(cr::CreativeAppState& appState,
                     std::string_view name,
                     cr::CreativeDocumentId id) {
  cr::CreativeDocument document =
      cr::CreativeDocument::create(std::string{name});
  static_cast<void>(document.assignId(id));
  return appState.facade.installDocument(std::move(document)).accepted;
}

cr::CreativeObjectId createAndSelectRoom(cr::CreativeAppState& appState) {
  cr::CreativeDocumentCreateRequest create;
  create.kind = cr::CreativeObjectKind::Room;
  create.name = "Array Source";
  create.bounds = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}};
  create.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt created =
      appState.facade.createDocumentObject(create);
  if (!created.accepted) {
    return cr::kInvalidObjectId;
  }

  cr::CreativeToolInputPacket select;
  select.kind = cr::CreativeToolInputKind::PointerPress;
  select.pointer.button = cr::CreativeToolPointerButton::Primary;
  select.pointer.target.value = static_cast<cr::Id>(created.objectId);
  static_cast<void>(appState.facade.dispatchToolInput(select));
  return created.objectId;
}

app::CreativeEditorState editorForArray() {
  app::CreativeEditorState editor;
  editor.interaction.hotbar.selectedSlot = 0U;
  editor.interaction.hotbar.entries[0] = {
      cr::CreativeHeldItemKind::LinearArray, cr::CreativeObjectKind::Unknown};
  editor.toolSettings.arrayDirection =
      cr::CreativeLinearArrayDirection::PositiveX;
  editor.toolSettings.arrayCopyCount = cr::CreativeLinearArrayCopyCount::Two;
  editor.toolSettings.arraySpacing = cr::CreativeLinearArraySpacing::TwoCells;
  editor.placeCellSize = 1.0;
  return editor;
}

bool requestMappingIsExplicit() {
  const app::CreativeEditorState editor = editorForArray();
  const cr::CreativeLinearArrayRequest request =
      app::creativeEditorLinearArrayRequest(editor.toolSettings, 0.5);
  return expect(request.direction ==
                    cr::CreativeLinearArrayDirection::PositiveX,
                "adapter maps direction") &&
         expect(request.copyCount == cr::CreativeLinearArrayCopyCount::Two,
                "adapter maps copy count") &&
         expect(request.spacing == cr::CreativeLinearArraySpacing::TwoCells,
                "adapter maps spacing") &&
         expect(request.cellSize == 0.5, "adapter maps cell size");
}

bool previewIsTransientAndOrdinalDerived() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Preview", 71U),
              "preview document installed") ||
      !expect(createAndSelectRoom(appState) != cr::kInvalidObjectId,
              "preview source selected")) {
    return false;
  }
  const app::CreativeEditorState editor = editorForArray();
  const std::size_t objectCountBefore = appState.facade.document().objectCount();
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> lines;

  const std::size_t appended = app::appendCreativeEditorLinearArrayPreview(
      appState, editor, 0.05F, lines);

  return expect(appended == 24U && lines.size() == 24U,
                "two copies preview as two twelve-edge boxes") &&
         expect(lines[0].start.x == 2.0F && lines[0].end.x == 3.0F &&
                    lines[12].start.x == 4.0F && lines[12].end.x == 5.0F,
                "preview offsets derive from copy ordinals") &&
         expect(appState.facade.document().objectCount() == objectCountBefore &&
                    appState.facade.document().revision() == revisionBefore,
                "preview does not mutate document") &&
         expect(cr::creativeUndoDepth(appState.history) == 0U,
                "preview does not record history");
}

bool commitRecordsOneUndoStep() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Commit", 72U),
              "commit document installed") ||
      !expect(createAndSelectRoom(appState) != cr::kInvalidObjectId,
              "commit source selected")) {
    return false;
  }
  app::CreativeEditorState editor = editorForArray();

  const cr::CreativeLinearArrayReceipt applied =
      app::applyCreativeEditorLinearArrayWithHistory(
          appState, editor.pattern, editor.toolSettings, editor.placeCellSize,
          "test_linear_array");
  bool ok = expect(applied.accepted && applied.changed &&
                       appState.facade.document().objectCount() == 3U,
                   "adapter commits two new copies") &&
            expect(cr::creativeUndoDepth(appState.history) == 1U &&
                       cr::creativeRedoDepth(appState.history) == 0U,
                   "array commit records exactly one undo step") &&
            expect(appState.facade.selectionState().selectedTarget.value == 3U,
                   "array commit selects the final copy");

  const cr::CreativeHistoryApplyReceipt undo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  const cr::CreativeHistoryApplyReceipt redo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Redo);
  ok = expect(undo.accepted && undo.objectCountAfter == 1U,
              "single undo removes the full array batch") &&
       expect(redo.accepted && redo.objectCountAfter == 3U,
              "single redo restores the full array batch") &&
       ok;
  return ok;
}

bool rejectedCommitDoesNotRecordHistory() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Rejected", 73U),
              "rejected document installed")) {
    return false;
  }
  app::CreativeEditorState editor = editorForArray();
  const cr::CreativeLinearArrayReceipt rejected =
      app::applyCreativeEditorLinearArrayWithHistory(
          appState, editor.pattern, editor.toolSettings, editor.placeCellSize,
          "test_empty_linear_array");
  return expect(!rejected.accepted && !rejected.changed,
                "empty selection is rejected") &&
         expect(cr::creativeUndoDepth(appState.history) == 0U,
                "rejected array does not record history");
}

bool transformCopyPreviewIsTransientAndConfirmable() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Transform Copy", 74U),
              "transform copy document installed") ||
      !expect(createAndSelectRoom(appState) != cr::kInvalidObjectId,
              "transform copy source selected") ||
      !expect(appState.facade.copySelectedObjectsToClipboard(appState.clipboard)
                  .accepted,
              "transform copy source copied")) {
    return false;
  }
  app::CreativeEditorState editor;
  const std::size_t objectCountBefore = appState.facade.document().objectCount();
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  bool ok = expect(app::beginCreativeEditorClipboardTransformPreview(
                       appState, appState.clipboard, editor.transform,
                       "test_transform_copy"),
                   "transform copy begins") &&
            expect(editor.transform.active &&
                       !editor.transform.targetPositionable &&
                       editor.transform.mode ==
                           cr::CreativeSelectionPlacementMode::Copy,
                   "transform copy begins without stale target");

  static_cast<void>(app::processCreativeEditorSelectionTransformPreview(
      appState, editor.transform, true, {5.5, 0.0, 7.5}, false,
      "test_transform_copy_update"));
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> lines;
  const std::size_t edgeCount =
      app::appendCreativeEditorSelectionTransformPreview(
          editor.transform, 0.05F, lines);
  ok = expect(editor.transform.targetPositionable &&
                  editor.transform.plan.accepted &&
                  editor.transform.request.targetAnchor.x == 5.5 &&
                  editor.transform.request.targetAnchor.z == 7.5,
              "transform copy records shared source and target anchors") &&
       expect(edgeCount == 24U && lines.size() == 24U &&
                  lines[0].start.x == 5.0F && lines[0].end.x == 6.0F &&
                  lines[0].start.z == 7.0F,
              "transform copy draws planned object and target pivot") &&
       expect(appState.facade.document().objectCount() == objectCountBefore &&
                  appState.facade.document().revision() == revisionBefore &&
                  cr::creativeUndoDepth(appState.history) == 0U,
              "transform preview is non-mutating") &&
       ok;

  static_cast<void>(app::requestCreativeEditorSelectionTransformCommit(
      editor.transform));
  const app::CreativeEditorTransformCommitReceipt committed =
      app::processCreativeEditorSelectionTransformPreview(
          appState, editor.transform, true, {5.5, 0.0, 7.5}, false,
          "test_transform_copy_confirm");
  ok = expect(committed.accepted && committed.changed &&
                  !editor.transform.active,
              "transform copy confirm commits and closes") &&
       expect(appState.facade.document().objectCount() == 2U &&
                  appState.facade.selectionState().selectedTarget.value == 2U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "transform copy selects paste and records one undo") &&
       ok;

  const cr::CreativeHistoryApplyReceipt undo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  const cr::CreativeHistoryApplyReceipt redo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Redo);
  return expect(undo.accepted && undo.objectCountAfter == 1U,
                "transform copy undo removes full paste") &&
         expect(redo.accepted && redo.objectCountAfter == 2U,
                "transform copy redo restores full paste") &&
         ok;
}

bool selectionTransformMoveUsesControlsAndOneUndo() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Transform Move", 75U),
              "transform move document installed") ||
      !expect(createAndSelectRoom(appState) != cr::kInvalidObjectId,
              "transform move source selected")) {
    return false;
  }
  app::CreativeEditorState editor;
  bool ok = expect(app::beginCreativeEditorSelectionTransformPreview(
                       appState, editor.transform, "test_move_begin"),
                   "selection transform move begins") &&
            expect(app::applyCreativeEditorTransformControl(
                       appState, editor.transform,
                       app::CreativeEditorTransformControl::RotatePositive) &&
                       app::applyCreativeEditorTransformControl(
                           appState, editor.transform,
                           app::CreativeEditorTransformControl::MirrorX),
                   "selection transform controls update the shared request");
  const app::CreativeEditorTransformCommitReceipt committed =
      app::processCreativeEditorSelectionTransformPreview(
          appState, editor.transform, true, {2.5, 0.0, 0.5}, true,
          "test_move_commit");
  const cr::CreativeObject* moved = appState.facade.findObject(1U);
  ok = expect(committed.accepted && committed.changed &&
                  appState.facade.document().objectCount() == 1U &&
                  moved != nullptr,
              "selection transform move mutates originals without copying") &&
            expect(cr::creativeUndoDepth(appState.history) == 1U,
                   "selection transform move records one undo step") &&
       ok;

  static_cast<void>(app::beginCreativeEditorSelectionTransformPreview(
      appState, editor.transform, "test_cancel_begin"));
  static_cast<void>(app::requestCreativeEditorSelectionTransformCommit(
      editor.transform));
  static_cast<void>(app::processCreativeEditorSelectionTransformPreview(
      appState, editor.transform, false, {}, false,
      "test_invalid_confirm"));
  static_cast<void>(app::processCreativeEditorSelectionTransformPreview(
      appState, editor.transform, true, {9.0, 0.0, 0.0}, false,
      "test_no_delayed_confirm"));
  const std::size_t countBeforeCancel =
      appState.facade.document().objectCount();
  const std::uint64_t revisionBeforeCancel =
      appState.facade.document().revision();
  ok = expect(editor.transform.active &&
                  !editor.transform.commitRequested && countBeforeCancel == 1U,
              "invalid confirm does not commit later on a valid target") &&
       expect(app::cancelCreativeEditorSelectionTransformPreview(
                  editor.transform, "test_cancel"),
              "selection transform cancel accepted") &&
       expect(!editor.transform.active &&
                  appState.facade.document().objectCount() == countBeforeCancel &&
                  appState.facade.document().revision() == revisionBeforeCancel &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "selection transform cancel leaves document and history unchanged") &&
       ok;
  const cr::CreativeHistoryApplyReceipt undo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  return expect(undo.accepted && undo.objectCountAfter == 1U &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "selection transform move undoes atomically") &&
         ok;
}

bool precisionTransformConstrainsNudgesAndCommitsOnce() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Transform Precision", 77U),
              "precision transform document installed") ||
      !expect(createAndSelectRoom(appState) != cr::kInvalidObjectId,
              "precision transform source selected")) {
    return false;
  }
  app::CreativeEditorState editor;
  if (!expect(app::beginCreativeEditorSelectionTransformPreview(
                  appState, editor.transform, "test_precision_begin"),
              "precision transform begins")) {
    return false;
  }
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  static_cast<void>(app::processCreativeEditorSelectionTransformPreview(
      appState, editor.transform, true, {3.4, 2.2, -4.7}, false,
      "test_precision_aim", 0.5));
  bool ok = expect(app::setCreativeEditorTransformConstraint(
                       appState, editor.transform,
                       cr::CreativeSelectionPlacementAxis::X) &&
                       editor.transform.request.targetAnchor.x == 3.5 &&
                       editor.transform.request.targetAnchor.y == 0.0 &&
                       editor.transform.request.targetAnchor.z == 0.5,
                   "X lock projects and snaps aim relative to source") &&
            expect(app::nudgeCreativeEditorSelectionTransform(
                       appState, editor.transform, 1, false) &&
                       editor.transform.request.targetAnchor.x == 4.0 &&
                       app::nudgeCreativeEditorSelectionTransform(
                           appState, editor.transform, -1, true) &&
                       editor.transform.request.targetAnchor.x == 3.875,
                   "regular and fine nudges update transient target exactly");

  std::vector<iggy3d::RenderCreativeWireframeDebugLine> lines;
  const std::size_t lineCount =
      app::appendCreativeEditorSelectionTransformPreview(
          editor.transform, 0.05F, lines);
  ok = expect(lineCount >= 50U && !lines.empty() &&
                  lines.back().color.r == 1.0F &&
                  lines.back().color.g == 0.24F &&
                  lines.back().start.y == lines.back().end.y &&
                  lines.back().start.z == lines.back().end.z,
              "X lock renders a red axis-aligned guide") &&
       expect(appState.facade.document().revision() == revisionBefore &&
                  cr::creativeUndoDepth(appState.history) == 0U,
              "precision preview remains non-mutating") &&
       ok;

  ok = expect(app::applyCreativeEditorTransformControl(
                       appState, editor.transform,
                       app::CreativeEditorTransformControl::CycleConstraint) &&
                   editor.transform.constraint ==
                       cr::CreativeSelectionPlacementAxis::Y &&
                   editor.transform.request.targetAnchor.y == 2.0,
              "controller axis sector cycles X to Y") &&
       expect(app::nudgeCreativeEditorSelectionTransform(
                  appState, editor.transform, 2, false) &&
                  editor.transform.request.targetAnchor.y == 3.0,
              "repeated Y nudge accumulates by snap steps") &&
       ok;

  static_cast<void>(app::requestCreativeEditorSelectionTransformCommit(
      editor.transform));
  const app::CreativeEditorTransformCommitReceipt committed =
      app::processCreativeEditorSelectionTransformPreview(
          appState, editor.transform, true, {3.4, 2.2, -4.7}, false,
          "test_precision_commit", 0.5);
  const cr::CreativeObject* moved = appState.facade.findObject(1U);
  return expect(committed.accepted && committed.changed && moved != nullptr &&
                    moved->bounds.min.y == 3.0 &&
                    moved->bounds.max.y == 4.0,
                "precision target commits through shared placement plan") &&
         expect(appState.facade.document().revision() == revisionBefore + 1U &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "precision move advances once and records one undo") &&
         ok;
}

bool lockedSelectionTransformStaysRedAndNonMutating() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Transform Locked", 76U),
              "locked transform document installed") ||
      !expect(createAndSelectRoom(appState) != cr::kInvalidObjectId,
              "locked transform source selected") ||
      !expect(appState.facade.toggleSelectedObjectLocked().accepted,
              "locked transform source locked")) {
    return false;
  }
  app::CreativeEditorState editor;
  if (!expect(app::beginCreativeEditorSelectionTransformPreview(
                  appState, editor.transform, "test_locked_begin"),
              "locked transform preview begins")) {
    return false;
  }
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  static_cast<void>(app::processCreativeEditorSelectionTransformPreview(
      appState, editor.transform, true, {3.5, 0.0, 0.5}, false,
      "test_locked_update"));
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> lines;
  const std::size_t edgeCount =
      app::appendCreativeEditorSelectionTransformPreview(
          editor.transform, 0.05F, lines);
  static_cast<void>(app::requestCreativeEditorSelectionTransformCommit(
      editor.transform));
  const app::CreativeEditorTransformCommitReceipt rejected =
      app::processCreativeEditorSelectionTransformPreview(
          appState, editor.transform, true, {3.5, 0.0, 0.5}, false,
          "test_locked_commit");

  return expect(!editor.transform.plan.accepted &&
                    editor.transform.plan.status ==
                        cr::CreativeSelectionPlacementStatus::LockedObject,
                "locked move plan rejects before mutation") &&
         expect(edgeCount >= 36U && lines.size() >= 36U &&
                    lines[24].color.r == 1.0F &&
                    lines[24].color.g == 0.24F,
                "locked destination remains visibly red") &&
         expect(!rejected.accepted && !rejected.changed &&
                    appState.facade.document().revision() == revisionBefore &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "locked confirm cannot mutate or record history");
}

bool largeTransformPreviewUsesOneAggregateBox() {
  cr::CreativeClipboard clipboard;
  clipboard.hasPlacementAnchor = true;
  clipboard.placementAnchor = {};
  clipboard.objects.reserve(513U);
  for (std::size_t index = 0; index < 513U; ++index) {
    cr::CreativeObject object;
    object.id = index + 1U;
    object.kind = cr::CreativeObjectKind::Room;
    object.bounds = {{static_cast<double>(index), 0.0, 0.0},
                     {static_cast<double>(index + 1U), 1.0, 1.0}};
    clipboard.objects.push_back(std::move(object));
  }
  app::CreativeEditorSelectionTransformState state;
  state.active = true;
  state.targetPositionable = true;
  state.sourceClipboard = clipboard;
  state.request.mode = cr::CreativeSelectionPlacementMode::Copy;
  state.request.sourceAnchor = clipboard.placementAnchor;
  state.request.targetAnchor = {2.0, 0.0, 0.0};
  state.plan = cr::planCreativeSelectionPlacement(clipboard.objects,
                                                   state.request);
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> lines;

  const std::size_t edgeCount =
      app::appendCreativeEditorSelectionTransformPreview(state, 0.05F, lines);

  return expect(edgeCount == 24U && lines.size() == 24U,
                "large transform preview uses one extent and one pivot box") &&
         expect(lines[0].start.x == 2.0F && lines[0].end.x == 515.0F,
                "aggregate transform preview covers every source object");
}

}  // namespace

int main() {
  const bool ok = requestMappingIsExplicit() &&
                  previewIsTransientAndOrdinalDerived() &&
                  commitRecordsOneUndoStep() &&
                  rejectedCommitDoesNotRecordHistory() &&
                  transformCopyPreviewIsTransientAndConfirmable() &&
                  selectionTransformMoveUsesControlsAndOneUndo() &&
                  precisionTransformConstrainsNudgesAndCommitsOnce() &&
                  lockedSelectionTransformStaysRedAndNonMutating() &&
                  largeTransformPreviewUsesOneAggregateBox();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
