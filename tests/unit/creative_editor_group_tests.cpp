#include "EditorGroup.hpp"
#include "EditorEdits.hpp"
#include "EditorFrame.hpp"
#include "EditorInteraction.hpp"
#include "EditorPlacement.hpp"
#include "EditorState.hpp"
#include "EditorToolOptions.hpp"
#include "app/iggy3d/creative/Facade.hpp"

#include <cstdlib>
#include <iostream>
#include <span>
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

cr::CreativeObjectId createCrate(cr::Facade& facade, double x) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.transform.position = {x, 0, 0};
  request.hasTransformOverride = true;
  return facade.createDocumentObject(request).objectId;
}

void select(cr::Facade& facade,
            cr::CreativeObjectId objectId,
            bool additive) {
  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerPress;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = static_cast<cr::Id>(objectId);
  input.pointer.modifiers =
      additive ? cr::kCreativeToolModifierShift
               : cr::kCreativeToolModifierNone;
  static_cast<void>(facade.dispatchToolInput(input));
}

bool editorCommandGroupsUngroupsAndRecordsOneStepEach() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Groups");
  static_cast<void>(document.assignId(301U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId first = createCrate(appState.facade, 0.0);
  const cr::CreativeObjectId second = createCrate(appState.facade, 2.0);
  appState.history = {};
  select(appState.facade, first, false);
  select(appState.facade, second, true);

  const cr::CreativeGroupCommandReceipt grouped =
      app::applyCreativeEditorGroupCommandWithHistory(
          appState, "test_group");
  const cr::CreativeObjectId groupId = grouped.groupObjectId;
  const app::CreativeEditorSelectionFrame selectionFrame =
      app::resolveCreativeEditorSelectionFrame(appState.facade);
  bool ok = expect(grouped.accepted && grouped.changed &&
                       cr::creativeUndoDepth(appState.history) == 1U &&
                       appState.facade.selectionState().selectedTarget.value ==
                           groupId,
                   "editor group command records one undo and selects group") &&
            expect(selectionFrame.hasSelection &&
                       selectionFrame.selectionCount == 1U &&
                       selectionFrame.selectedObjectIds.size() == 3U &&
                       selectionFrame.boxMax.x > selectionFrame.boxMin.x,
                   "one group selection outlines all visible descendants");

  const cr::CreativeGroupCommandReceipt ungrouped =
      app::applyCreativeEditorGroupCommandWithHistory(
          appState, "test_ungroup");
  ok = expect(ungrouped.accepted && ungrouped.changed &&
                  cr::creativeUndoDepth(appState.history) == 2U &&
                  cr::selectedTargetCount(
                      appState.facade.selectionState()) == 2U &&
                  appState.facade.findObject(groupId) == nullptr,
              "same X command ungroups one selected group in one undo") &&
       ok;

  const bool undone = app::undoLastEdit(appState, "test_undo_ungroup");
  return expect(undone && appState.facade.findObject(groupId) != nullptr,
                "undo restores the complete group hierarchy") &&
         ok;
}

bool heldGroupToolRoutesContextualXWithoutASecondBinding() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Group Tool");
  static_cast<void>(document.assignId(302U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId first = createCrate(appState.facade, 0.0);
  const cr::CreativeObjectId second = createCrate(appState.facade, 2.0);
  appState.history = {};
  select(appState.facade, first, false);
  select(appState.facade, second, true);
  app::CreativeEditorState editor;
  editor.interaction.hotbar.selectedSlot = 0U;
  editor.interaction.hotbar.entries[0] = {
      cr::CreativeHeldItemKind::ObjectGroup,
      cr::CreativeObjectKind::Unknown};

  const bool grouped = app::confirmCreativeEditorHeldItem(
      appState, editor, "test_group_tool_confirm");
  const cr::CreativeObjectId groupId = static_cast<cr::CreativeObjectId>(
      appState.facade.selectionState().selectedTarget.value);
  const bool cancelled = app::cancelCreativeEditorHeldItem(appState, editor);
  select(appState.facade, first, false);
  const bool ungrouped = app::confirmCreativeEditorHeldItem(
      appState, editor, "test_group_tool_confirm");
  return expect(grouped && groupId != cr::kInvalidObjectId &&
                    appState.facade.findObject(groupId) == nullptr &&
                    !cancelled && ungrouped &&
                    cr::creativeUndoDepth(appState.history) == 2U,
                "held Group routes X from a child while Circle stays inert");
}

bool deletingASelectedGroupRemovesAndRestoresItsHierarchy() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Delete Group");
  static_cast<void>(document.assignId(303U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId first = createCrate(appState.facade, 0.0);
  const cr::CreativeObjectId second = createCrate(appState.facade, 2.0);
  appState.history = {};
  select(appState.facade, first, false);
  select(appState.facade, second, true);
  const cr::CreativeGroupCommandReceipt grouped =
      app::applyCreativeEditorGroupCommandWithHistory(
          appState, "test_group_before_delete");
  const cr::CreativeDocumentRemoveReceipt removed = app::deleteSelectedObject(
      appState, "test_delete_group", &appState.history);
  const bool undone = app::undoLastEdit(appState, "test_undo_group_delete");
  return expect(grouped.accepted && removed.accepted &&
                    removed.objectRemoved &&
                    removed.reasonCode == "object_hierarchy_removed" &&
                    undone && appState.facade.document().objectCount() == 3U &&
                    appState.facade.findObject(grouped.groupObjectId) != nullptr,
                "delete and undo treat a group hierarchy as one history step");
}

bool arrayCopiesACompleteGroupAndSelectsOnlyTheNewRoot() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Array Group");
  static_cast<void>(document.assignId(304U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId first = createCrate(appState.facade, 0.0);
  const cr::CreativeObjectId second = createCrate(appState.facade, 2.0);
  select(appState.facade, first, false);
  select(appState.facade, second, true);
  const cr::CreativeGroupCommandReceipt grouped =
      appState.facade.groupSelectedObjects();
  cr::CreativeLinearArrayRequest request;
  request.copyCount = cr::CreativeLinearArrayCopyCount::One;
  request.spacing = cr::CreativeLinearArraySpacing::TwoCells;
  request.cellSize = 1.0;
  const cr::CreativeLinearArrayReceipt array =
      appState.facade.createLinearArrayFromSelection(request);
  const cr::CreativeSelectionState& selection =
      appState.facade.selectionState();
  const cr::CreativeObjectId selectedRoot =
      static_cast<cr::CreativeObjectId>(selection.selectedTarget.value);
  const cr::CreativeHierarchySelection hierarchy =
      cr::resolveCreativeObjectHierarchy(
          appState.facade.document(), std::span{&selectedRoot, 1U});
  const cr::CreativeObject* selectedObject =
      appState.facade.findObject(selectedRoot);
  return expect(grouped.accepted && array.accepted &&
                    array.sourceObjectCount == 3U &&
                    array.generatedObjectCount == 3U &&
                    cr::selectedTargetCount(selection) == 1U &&
                    selectedObject != nullptr &&
                    selectedObject->kind ==
                        cr::CreativeObjectKind::Group &&
                    hierarchy.accepted && hierarchy.objectIds.size() == 3U,
                "array copies group descendants and selects the new root");
}

bool moveDragMovesTheCompleteGroupHierarchy() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Drag Group");
  static_cast<void>(document.assignId(305U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId first = createCrate(appState.facade, 0.0);
  const cr::CreativeObjectId second = createCrate(appState.facade, 2.0);
  select(appState.facade, first, false);
  select(appState.facade, second, true);
  const cr::CreativeGroupCommandReceipt grouped =
      appState.facade.groupSelectedObjects();
  const cr::CreativeObjectId groupId = grouped.groupObjectId;
  const double groupX = appState.facade.findObject(groupId)
                            ->transform.position.x;
  const double firstX = appState.facade.findObject(first)->transform.position.x;
  const double secondX =
      appState.facade.findObject(second)->transform.position.x;

  static_cast<void>(appState.facade.setActiveTool(cr::Tool::Move));
  cr::CreativeToolInputPacket press;
  press.kind = cr::CreativeToolInputKind::PointerPress;
  press.pointer.button = cr::CreativeToolPointerButton::Primary;
  press.pointer.target.value = static_cast<cr::Id>(groupId);
  const cr::CreativeFacadeToolDispatchReceipt begun =
      appState.facade.dispatchToolInput(press);
  cr::CreativeToolInputPacket release;
  release.kind = cr::CreativeToolInputKind::PointerRelease;
  release.pointer.button = cr::CreativeToolPointerButton::Primary;
  release.pointer.hasWorldDestination = true;
  release.pointer.worldDestination = {groupX + 3.0, 0.0, 0.0};
  release.pointer.moveHeldAxis = cr::CreativeToolMoveHeldAxis::Y;
  const cr::CreativeFacadeToolDispatchReceipt committed =
      appState.facade.dispatchToolInput(release);
  const double committedDeltaX = committed.moveDrag.snappedAnchor.x -
                                 committed.moveDrag.startAnchor.x;

  return expect(grouped.accepted, "pointer drag group setup") &&
         expect(begun.moveDrag.accepted && begun.moveDrag.objectCount == 3U,
                "pointer drag expands the selected group at begin") &&
         expect(committed.moveDrag.committed &&
                    committed.moveDrag.objectCount == 3U,
                "pointer drag commits the complete group") &&
         expect(appState.facade.findObject(groupId)->transform.position.x ==
                    groupX + committedDeltaX,
                "pointer drag moves the group root") &&
         expect(appState.facade.findObject(first)->transform.position.x ==
                    firstX + committedDeltaX,
                "pointer drag moves the first child") &&
         expect(appState.facade.findObject(second)->transform.position.x ==
                    secondX + committedDeltaX,
                "pointer drag moves the second child");
}

bool clipboardCommandsPreserveAndSelectGroupRoots() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Clipboard Group");
  static_cast<void>(document.assignId(306U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId first = createCrate(appState.facade, 0.0);
  const cr::CreativeObjectId second = createCrate(appState.facade, 2.0);
  select(appState.facade, first, false);
  select(appState.facade, second, true);
  const cr::CreativeGroupCommandReceipt grouped =
      appState.facade.groupSelectedObjects();
  cr::CreativeClipboard clipboard;
  const cr::CreativeClipboardCopyReceipt copied =
      appState.facade.copySelectedObjectsToClipboard(clipboard);
  cr::CreativeClipboardPasteRequest pasteRequest;
  pasteRequest.offset = {4.0, 0.0, 0.0};
  const cr::CreativeClipboardPasteReceipt pasted =
      appState.facade.pasteClipboard(clipboard, pasteRequest);
  const cr::CreativeSelectionState& selectionAfterPaste =
      appState.facade.selectionState();
  const std::size_t selectedCountAfterPaste =
      cr::selectedTargetCount(selectionAfterPaste);
  const cr::CreativeObjectId pastedRoot =
      static_cast<cr::CreativeObjectId>(
          selectionAfterPaste.selectedTarget.value);
  const cr::CreativeHierarchySelection pastedHierarchy =
      cr::resolveCreativeObjectHierarchy(
          appState.facade.document(), std::span{&pastedRoot, 1U});
  const cr::CreativeClipboardCutReceipt cut =
      appState.facade.cutSelectedObjectsToClipboard(clipboard);

  return expect(grouped.accepted && copied.accepted &&
                    copied.copiedObjectCount == 3U && pasted.accepted &&
                    pasted.pastedObjectCount == 3U &&
                    selectedCountAfterPaste == 1U &&
                    pastedHierarchy.accepted &&
                    pastedHierarchy.rootObjectIds.size() == 1U &&
                    pastedHierarchy.objectIds.size() == 3U && cut.accepted &&
                    cut.cutObjectCount == 3U &&
                    cr::selectedTargetCount(
                        appState.facade.selectionState()) == 0U &&
                    appState.facade.document().objectCount() == 3U,
                "copy paste and cut preserve complete group hierarchies");
}

bool focusResolvesNestedGroupsAtTheCurrentEditingLevel() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Focus Group");
  static_cast<void>(document.assignId(307U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId first = createCrate(appState.facade, 0.0);
  const cr::CreativeObjectId second = createCrate(appState.facade, 2.0);
  select(appState.facade, first, false);
  select(appState.facade, second, true);
  const cr::CreativeGroupCommandReceipt inner =
      appState.facade.groupSelectedObjects();
  const cr::CreativeObjectId third = createCrate(appState.facade, 4.0);
  select(appState.facade, inner.groupObjectId, false);
  select(appState.facade, third, true);
  const cr::CreativeGroupCommandReceipt outer =
      appState.facade.groupSelectedObjects();
  const cr::CreativeObjectId outside = createCrate(appState.facade, 8.0);
  app::CreativeEditorGroupFocusState focus;

  bool ok = expect(
      inner.accepted && outer.accepted &&
          app::resolveCreativeEditorGroupSelectionTarget(
              appState.facade.document(), focus, first) ==
              outer.groupObjectId,
      "outside focus a descendant resolves to the outermost group");
  const app::CreativeEditorGroupFocusReceipt enteredOuter =
      app::enterCreativeEditorGroupFocus(appState, focus,
                                         outer.groupObjectId);
  ok = expect(enteredOuter.accepted && focus.depth == 1U &&
                  cr::selectedTargetCount(
                      appState.facade.selectionState()) == 0U &&
                  app::resolveCreativeEditorGroupSelectionTarget(
                      appState.facade.document(), focus, first) ==
                      inner.groupObjectId &&
                  app::resolveCreativeEditorGroupSelectionTarget(
                      appState.facade.document(), focus, outside) ==
                      cr::kInvalidObjectId,
              "outer focus exposes immediate children and rejects outsiders") &&
       ok;
  const app::CreativeEditorGroupFocusReceipt enteredInner =
      app::enterCreativeEditorGroupFocus(appState, focus,
                                         inner.groupObjectId);
  ok = expect(enteredInner.accepted && focus.depth == 2U &&
                  app::resolveCreativeEditorGroupSelectionTarget(
                      appState.facade.document(), focus, first) == first,
              "nested focus exposes the inner group's direct children") &&
       ok;
  const app::CreativeEditorGroupFocusReceipt exitedInner =
      app::exitCreativeEditorGroupFocus(appState, focus);
  const cr::CreativeObjectId selectedAfterExit =
      static_cast<cr::CreativeObjectId>(
          appState.facade.selectionState().selectedTarget.value);
  return expect(exitedInner.accepted && focus.depth == 1U &&
                    selectedAfterExit == inner.groupObjectId,
                "exiting focus selects the group at the parent level") &&
         ok;
}

bool focusedPlacementParentsAuthoredObjectsOnly() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Focused Placement");
  static_cast<void>(document.assignId(308U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId first = createCrate(appState.facade, 0.0);
  const cr::CreativeObjectId second = createCrate(appState.facade, 2.0);
  select(appState.facade, first, false);
  select(appState.facade, second, true);
  const cr::CreativeGroupCommandReceipt grouped =
      appState.facade.groupSelectedObjects();
  const app::CreativeBrushPlacementPlan plan =
      app::planBrushPlacement(cr::CreativeObjectKind::Crate,
                              iggy3d::Vec3{10.0F, 0.5F, 0.0F});
  const app::CreativeBrushPlacementMutationReceipt placed =
      app::applyBrushPlacement(appState.facade, plan, 1U,
                               grouped.groupObjectId);
  const cr::CreativeObject* child =
      appState.facade.findObject(placed.objectId);
  return expect(grouped.accepted && plan.valid && placed.accepted &&
                    placed.objectCreated && child != nullptr &&
                    child->parentId == grouped.groupObjectId,
                "authored placement inside focus joins the active group");
}

bool groupToolOptionsExposeEditAndUngroupCommands() {
  cr::CreativeHotbarEntry entry;
  entry.kind = cr::CreativeHeldItemKind::ObjectGroup;
  const app::CreativeEditorToolOptionsCommandList commands =
      app::creativeEditorToolOptionCommandsForEntry(entry);
  return expect(
      commands.count == 2U &&
          commands.ids[0] ==
              app::CreativeEditorToolOptionsCommandId::EditGroupContents &&
          commands.ids[1] ==
              app::CreativeEditorToolOptionsCommandId::UngroupSelection,
      "Group options present edit contents before destructive ungroup");
}

bool groupToolOptionsEnterFocusAndUngroupWithHistory() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Group Options");
  static_cast<void>(document.assignId(309U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId first = createCrate(appState.facade, 0.0);
  const cr::CreativeObjectId second = createCrate(appState.facade, 2.0);
  select(appState.facade, first, false);
  select(appState.facade, second, true);
  const cr::CreativeGroupCommandReceipt grouped =
      appState.facade.groupSelectedObjects();
  appState.history = {};
  app::CreativeEditorState editor;
  editor.toolOptions.open = true;
  editor.toolOptions.commands =
      app::creativeEditorToolOptionCommandsForEntry(
          {cr::CreativeHeldItemKind::ObjectGroup,
           cr::CreativeObjectKind::Unknown});
  editor.toolOptions.contextGroupId = grouped.groupObjectId;
  editor.toolOptions.selectedIndex = 0U;
  editor.toolOptions.draft = editor.toolSettings;
  const bool entered = app::activateCreativeEditorToolOptionsSelection(
      appState, editor);
  const bool exited =
      app::exitCreativeEditorGroupFocus(appState, editor.groupFocus).accepted;
  editor.toolOptions.open = true;
  editor.toolOptions.contextGroupId = grouped.groupObjectId;
  editor.toolOptions.selectedIndex = 1U;
  const bool ungrouped = app::activateCreativeEditorToolOptionsSelection(
      appState, editor);
  return expect(entered && exited && ungrouped &&
                    appState.facade.findObject(grouped.groupObjectId) ==
                        nullptr &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "Group options enter focus and ungroup through one history step");
}

}  // namespace

int main() {
  return editorCommandGroupsUngroupsAndRecordsOneStepEach() &&
                 heldGroupToolRoutesContextualXWithoutASecondBinding() &&
                 deletingASelectedGroupRemovesAndRestoresItsHierarchy() &&
                 arrayCopiesACompleteGroupAndSelectsOnlyTheNewRoot() &&
                 moveDragMovesTheCompleteGroupHierarchy() &&
                 clipboardCommandsPreserveAndSelectGroupRoots() &&
                 focusResolvesNestedGroupsAtTheCurrentEditingLevel() &&
                 focusedPlacementParentsAuthoredObjectsOnly() &&
                 groupToolOptionsExposeEditAndUngroupCommands() &&
                 groupToolOptionsEnterFocusAndUngroupWithHistory()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
