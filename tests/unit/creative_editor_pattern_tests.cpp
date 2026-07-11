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

bool clipboardPastePreviewIsTransientAndConfirmable() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Clipboard Preview", 74U),
              "clipboard preview document installed") ||
      !expect(createAndSelectRoom(appState) != cr::kInvalidObjectId,
              "clipboard preview source selected") ||
      !expect(appState.facade.copySelectedObjectsToClipboard(appState.clipboard)
                  .accepted,
              "clipboard preview source copied")) {
    return false;
  }
  app::CreativeEditorState editor;
  const std::size_t objectCountBefore = appState.facade.document().objectCount();
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  bool ok = expect(app::beginCreativeEditorClipboardPastePreview(
                       appState.clipboard, editor.clipboardPaste,
                       "test_clipboard_preview"),
                   "clipboard preview begins") &&
            expect(editor.clipboardPaste.active &&
                       !editor.clipboardPaste.targetValid,
                   "clipboard preview begins without stale target");

  static_cast<void>(app::processCreativeEditorClipboardPastePreview(
      appState, editor.clipboardPaste, true, {5.5, 0.0, 7.5}, false,
      "test_clipboard_preview_update"));
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> lines;
  const std::size_t edgeCount = app::appendCreativeEditorClipboardPastePreview(
      appState.clipboard, editor.clipboardPaste, 0.05F, lines);
  ok = expect(editor.clipboardPaste.targetValid &&
                  editor.clipboardPaste.request.offset.x == 5.0 &&
                  editor.clipboardPaste.request.offset.y == 0.0 &&
                  editor.clipboardPaste.request.offset.z == 7.0,
              "clipboard preview derives target-minus-source offset") &&
       expect(edgeCount == 12U && lines.size() == 12U &&
                  lines[0].start.x == 5.0F && lines[0].end.x == 6.0F &&
                  lines[0].start.z == 7.0F,
              "clipboard preview keeps bounds aligned to target grid cell") &&
       expect(appState.facade.document().objectCount() == objectCountBefore &&
                  appState.facade.document().revision() == revisionBefore &&
                  cr::creativeUndoDepth(appState.history) == 0U,
              "clipboard preview is non-mutating") &&
       ok;

  static_cast<void>(app::requestCreativeEditorClipboardPasteCommit(
      editor.clipboardPaste));
  const cr::CreativeClipboardPasteReceipt committed =
      app::processCreativeEditorClipboardPastePreview(
          appState, editor.clipboardPaste, true, {5.5, 0.0, 7.5}, false,
          "test_clipboard_preview_confirm");
  ok = expect(committed.accepted && committed.changed &&
                  !editor.clipboardPaste.active,
              "clipboard preview confirm commits and closes") &&
       expect(appState.facade.document().objectCount() == 2U &&
                  appState.facade.selectionState().selectedTarget.value == 2U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "clipboard preview commit selects paste and records one undo") &&
       ok;

  const cr::CreativeHistoryApplyReceipt undo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  const cr::CreativeHistoryApplyReceipt redo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Redo);
  return expect(undo.accepted && undo.objectCountAfter == 1U,
                "clipboard preview undo removes full paste") &&
         expect(redo.accepted && redo.objectCountAfter == 2U,
                "clipboard preview redo restores full paste") &&
         ok;
}

bool clipboardPasteSecondaryCommitsAndCancelIsNonMutating() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Clipboard Secondary", 75U),
              "clipboard secondary document installed") ||
      !expect(createAndSelectRoom(appState) != cr::kInvalidObjectId,
              "clipboard secondary source selected") ||
      !expect(appState.facade.copySelectedObjectsToClipboard(appState.clipboard)
                  .accepted,
              "clipboard secondary source copied")) {
    return false;
  }
  app::CreativeEditorState editor;
  static_cast<void>(app::beginCreativeEditorClipboardPastePreview(
      appState.clipboard, editor.clipboardPaste, "test_secondary_begin"));
  const cr::CreativeClipboardPasteReceipt committed =
      app::processCreativeEditorClipboardPastePreview(
          appState, editor.clipboardPaste, true, {2.5, 0.0, 0.5}, true,
          "test_secondary_commit");
  bool ok = expect(committed.accepted && committed.changed &&
                       appState.facade.document().objectCount() == 2U,
                   "clipboard Secondary commits preview") &&
            expect(cr::creativeUndoDepth(appState.history) == 1U,
                   "clipboard Secondary records one undo step");

  static_cast<void>(app::beginCreativeEditorClipboardPastePreview(
      appState.clipboard, editor.clipboardPaste, "test_cancel_begin"));
  static_cast<void>(app::requestCreativeEditorClipboardPasteCommit(
      editor.clipboardPaste));
  static_cast<void>(app::processCreativeEditorClipboardPastePreview(
      appState, editor.clipboardPaste, false, {}, false,
      "test_invalid_confirm"));
  static_cast<void>(app::processCreativeEditorClipboardPastePreview(
      appState, editor.clipboardPaste, true, {9.0, 0.0, 0.0}, false,
      "test_no_delayed_confirm"));
  const std::size_t countBeforeCancel =
      appState.facade.document().objectCount();
  const std::uint64_t revisionBeforeCancel =
      appState.facade.document().revision();
  ok = expect(editor.clipboardPaste.active &&
                  !editor.clipboardPaste.commitRequested &&
                  countBeforeCancel == 2U,
              "invalid confirm does not commit later on a valid target") &&
       expect(app::cancelCreativeEditorClipboardPastePreview(
                  editor.clipboardPaste, "test_cancel"),
              "clipboard preview cancel accepted") &&
       expect(!editor.clipboardPaste.active &&
                  appState.facade.document().objectCount() == countBeforeCancel &&
                  appState.facade.document().revision() == revisionBeforeCancel &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "clipboard cancel leaves document and history unchanged") &&
       ok;
  return ok;
}

bool largeClipboardPreviewUsesOneAggregateBox() {
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
  app::CreativeEditorClipboardPasteState state;
  state.active = true;
  state.targetValid = true;
  state.request.offset = {2.0, 0.0, 0.0};
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> lines;

  const std::size_t edgeCount = app::appendCreativeEditorClipboardPastePreview(
      clipboard, state, 0.05F, lines);

  return expect(edgeCount == 12U && lines.size() == 12U,
                "large clipboard preview uses one bounded extent box") &&
         expect(lines[0].start.x == 2.0F && lines[0].end.x == 515.0F,
                "aggregate preview covers every clipboard object");
}

}  // namespace

int main() {
  const bool ok = requestMappingIsExplicit() &&
                  previewIsTransientAndOrdinalDerived() &&
                  commitRecordsOneUndoStep() &&
                  rejectedCommitDoesNotRecordHistory() &&
                  clipboardPastePreviewIsTransientAndConfirmable() &&
                  clipboardPasteSecondaryCommitsAndCancelIsNonMutating() &&
                  largeClipboardPreviewUsesOneAggregateBox();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
