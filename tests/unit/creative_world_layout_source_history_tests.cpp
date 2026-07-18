#include "EditorEdits.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutHistory.hpp"

#include <cstdlib>
#include <iostream>
#include <utility>

namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeAppState makeApp(cr::CreativeDocumentId id) {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Layout History");
  static_cast<void>(document.assignId(id));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  return appState;
}

bool addFloor(app::CreativeEditorWorldLayoutState& state,
              app::CreativeEditorWorldLayoutPoint minimum,
              app::CreativeEditorWorldLayoutPoint maximum) {
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Floor));
  const auto begin = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, minimum);
  const auto commit = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, maximum);
  return begin.accepted && !begin.changed && commit.accepted && commit.changed;
}

bool gestureCommitsOneEntryAndBranchesClearRedo() {
  cr::CreativeAppState live = makeApp(9701U);
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "gesture_history");
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Floor));
  const auto begin = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Begin, {0, 0});
  const std::uint64_t depthDuringGesture =
      app::creativeEditorWorldLayoutSourceUndoDepth(state);
  const auto commit = app::applyCreativeEditorWorldLayoutGesture(
      state, app::CreativeEditorWorldLayoutGesturePhase::Commit, {2, 2});
  const bool committedOnce =
      begin.accepted && !begin.changed && commit.accepted && commit.changed &&
      depthDuringGesture == 0U &&
      app::creativeEditorWorldLayoutSourceUndoDepth(state) == 1U;

  const bool undone = app::undoLastEdit(live, "source-floor-undo", &state);
  const bool undoRestoredBlank =
      undone && state.source.boxes.empty() &&
      app::creativeEditorWorldLayoutSourceUndoDepth(state) == 0U &&
      app::creativeEditorWorldLayoutSourceRedoDepth(state) == 1U;
  const bool branch = addFloor(state, {4, 4}, {6, 6});
  const bool branchClearedRedo =
      branch && state.source.boxes.size() == 1U &&
      state.source.boxes[0].footprint.minimum ==
          cr::CreativeTerrainCoord2{4, 4} &&
      !app::creativeEditorWorldLayoutSourceRedoAvailable(state);
  const bool staleRedoRejected =
      !app::redoLastEdit(live, "source-floor-stale-redo", &state);

  return expect(committedOnce,
                "a begin/commit gesture records one source-history entry") &&
         expect(undoRestoredBlank,
                "source undo restores the exact pre-gesture layout") &&
         expect(branchClearedRedo && staleRedoRejected,
                "a new semantic branch clears source redo history");
}

bool sourceHistoryTrimsToItsConfiguredBound() {
  cr::CreativeAppState live = makeApp(9702U);
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "bounded_history");
  state.sourceHistory.maxDepth = 2U;
  const bool first = addFloor(state, {0, 0}, {1, 1});
  const bool second = addFloor(state, {2, 0}, {3, 1});
  const bool third = addFloor(state, {4, 0}, {5, 1});
  const bool bounded =
      first && second && third && state.source.boxes.size() == 3U &&
      app::creativeEditorWorldLayoutSourceUndoDepth(state) == 2U;
  const bool undoThird =
      app::undoLastEdit(live, "bounded-third-undo", &state);
  const bool undoSecond =
      app::undoLastEdit(live, "bounded-second-undo", &state);
  const bool oldestTrimmed =
      !app::undoLastEdit(live, "bounded-first-undo", &state);

  return expect(bounded, "source history never exceeds its configured bound") &&
         expect(undoThird && undoSecond && oldestTrimmed &&
                    state.source.boxes.size() == 1U &&
                    app::creativeEditorWorldLayoutSourceRedoDepth(state) == 2U,
                "the oldest source snapshot is trimmed deterministically");
}

bool confirmHandsHistoryBackToTheDocumentOwner() {
  cr::CreativeAppState live = makeApp(9703U);
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "confirm_handoff");
  const auto shell = app::createCreativeEditorWorldLayoutBuildingShell(
      state, {{{0, 0}, {5, 4}}, 0.0, 3U, 0.25, 1U});
  const bool sourceUndoBeforeConfirm =
      app::creativeEditorWorldLayoutSourceUndoAvailable(state);
  const auto confirmed = app::confirmCreativeEditorWorldLayout(state, live);
  const std::uint64_t generatedObjectCount =
      live.facade.document().objectCount();
  const bool handedOff =
      shell.accepted && shell.changed && sourceUndoBeforeConfirm &&
      confirmed.accepted && confirmed.changed && generatedObjectCount > 0U &&
      !app::creativeEditorWorldLayoutSourceUndoAvailable(state) &&
      cr::creativeUndoDepth(live.history) == 1U;

  const bool undone = app::undoLastEdit(live, "confirm-handoff-undo", &state);
  const bool documentUndoRestoredBoth =
      undone && state.source.buildings.empty() &&
      live.facade.document().objectCount() == 0U &&
      cr::creativeRedoDepth(live.history) == 1U;
  const bool redone = app::redoLastEdit(live, "confirm-handoff-redo", &state);

  return expect(handedOff,
                "Confirm clears local snapshots after recording document history") &&
         expect(documentUndoRestoredBoth && redone &&
                    state.source.buildings.size() == 1U &&
                    live.facade.document().objectCount() ==
                        generatedObjectCount,
                "document undo/redo owns source and generated output after Confirm");
}

bool sourceRedoSurvivesDocumentHistoryRoundTrip() {
  cr::CreativeAppState live = makeApp(9704U);
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "history_round_trip");
  const auto shell = app::createCreativeEditorWorldLayoutBuildingShell(
      state, {{{0, 0}, {5, 4}}, 0.0, 3U, 0.25, 1U});
  const auto confirmed = app::confirmCreativeEditorWorldLayout(state, live);
  const std::uint64_t generatedObjectCount =
      live.facade.document().objectCount();
  const auto edited = app::setCreativeEditorWorldLayoutRoomSettings(
      state, 0U, {{{0, 0}, {7, 4}}, 0.0, 3U, 0.25, 1U});

  const bool sourceUndone =
      app::undoLastEdit(live, "round-trip-source-undo", &state);
  const bool documentUndone =
      app::undoLastEdit(live, "round-trip-document-undo", &state);
  const bool crossedBoundary =
      sourceUndone && documentUndone && state.source.buildings.empty() &&
      live.facade.document().objectCount() == 0U;

  const bool documentRedone =
      app::redoLastEdit(live, "round-trip-document-redo", &state);
  const bool sourceFutureRestored =
      documentRedone && state.source.rooms.size() == 1U &&
      state.source.rooms[0].footprint.maximum.x == 5 &&
      app::creativeEditorWorldLayoutSourceRedoAvailable(state) &&
      live.facade.document().objectCount() == generatedObjectCount;
  const bool sourceRedone =
      app::redoLastEdit(live, "round-trip-source-redo", &state);

  return expect(shell.accepted && confirmed.accepted && confirmed.changed &&
                    edited.accepted && edited.changed,
                "cross-boundary source history fixture is valid") &&
         expect(crossedBoundary,
                "Undo crosses from source history into document history") &&
         expect(sourceFutureRestored,
                "document Redo restores the deferred source future") &&
         expect(sourceRedone && state.source.rooms.size() == 1U &&
                    state.source.rooms[0].footprint.maximum.x == 7 &&
                    live.facade.document().objectCount() ==
                        generatedObjectCount,
                "source Redo remains available after the document round trip");
}

bool dirtySourceProtectsUnrelatedDocumentHistoryWhenLocalHistoryIsDisabled() {
  cr::CreativeAppState live = makeApp(9705U);
  app::StandaloneEditTransaction transaction =
      app::beginEditTransaction(live.facade, "unrelated-crate");
  cr::CreativeDocumentCreateRequest crate;
  crate.kind = cr::CreativeObjectKind::Crate;
  const auto created = live.facade.createDocumentObject(crate);
  const auto recorded = app::completeEditTransaction(
      live.history, std::move(transaction), live.facade,
      created.accepted && created.changed, created.reasonCode);

  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "disabled_source_history");
  state.sourceHistory.maxDepth = 0U;
  const bool floor = addFloor(state, {0, 0}, {2, 2});
  const bool undone = app::undoLastEdit(live, "guard-unrelated-undo", &state);

  return expect(created.accepted && recorded.recorded && floor,
                "dirty-source guard fixture is valid") &&
         expect(!undone && state.source.boxes.size() == 1U &&
                    live.facade.document().objectCount() == 1U &&
                    cr::creativeUndoDepth(live.history) == 1U,
                "a dirty source cannot consume unrelated document history");
}

bool sourceReplacementClearsLocalHistory() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "replacement_history");
  const bool floor = addFloor(state, {0, 0}, {2, 2});
  const bool hadUndo = app::creativeEditorWorldLayoutSourceUndoAvailable(state);
  app::resetCreativeEditorWorldLayout(state, "replacement_history_reset");
  const bool resetCleared =
      !app::creativeEditorWorldLayoutSourceUndoAvailable(state) &&
      !app::creativeEditorWorldLayoutSourceRedoAvailable(state);

  cr::CreativeWorldLayout replacement;
  replacement.stableKey = "replacement_history_loaded";
  static_cast<void>(addFloor(state, {1, 1}, {3, 3}));
  app::installCreativeEditorWorldLayout(state, std::move(replacement));
  return expect(floor && hadUndo && resetCleared,
                "reset clears source undo and redo history") &&
         expect(!app::creativeEditorWorldLayoutSourceUndoAvailable(state) &&
                    !app::creativeEditorWorldLayoutSourceRedoAvailable(state) &&
                    state.source.stableKey == "replacement_history_loaded",
                "installing a source starts a new history epoch");
}

bool savedCheckpointTracksAcrossSourceHistory() {
  cr::CreativeAppState live = makeApp(9706U);
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "saved_checkpoint");
  const bool first = addFloor(state, {0, 0}, {2, 2});
  app::markCreativeEditorWorldLayoutSaved(state);
  const std::uint64_t savedRevision = state.savedRevision;
  const bool saved = first && !app::creativeEditorWorldLayoutDirty(state);

  const bool undone =
      app::undoLastEdit(live, "saved-checkpoint-undo", &state);
  const bool olderStateIsDirty =
      undone && state.savedRevision == savedRevision &&
      app::creativeEditorWorldLayoutDirty(state);
  const bool redone =
      app::redoLastEdit(live, "saved-checkpoint-redo", &state);
  const bool savedStateIsClean =
      redone && state.revision == savedRevision &&
      !app::creativeEditorWorldLayoutDirty(state);

  const bool second = addFloor(state, {4, 0}, {6, 2});
  const bool branchUndone =
      app::undoLastEdit(live, "saved-checkpoint-branch-undo", &state);
  return expect(saved, "the current source can establish a save checkpoint") &&
         expect(olderStateIsDirty,
                "undo before the save checkpoint reports dirty") &&
         expect(savedStateIsClean,
                "redo to the save checkpoint reports clean") &&
         expect(second && branchUndone && state.revision == savedRevision &&
                    !app::creativeEditorWorldLayoutDirty(state),
                "undoing a later edit returns to the clean checkpoint");
}

bool noChangeApplyPreservesSourceHistory() {
  cr::CreativeAppState live = makeApp(9707U);
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "no_change_apply");
  const bool floor = addFloor(state, {0, 0}, {2, 2});
  const auto removed = app::deleteCreativeEditorWorldLayoutSource(
      state, cr::CreativeWorldLayoutTable::Building, 0U);
  const std::uint64_t undoDepthBefore =
      app::creativeEditorWorldLayoutSourceUndoDepth(state);

  cr::CreativeWorldLayoutPlan noChangePlan;
  noChangePlan.layoutKey = state.source.stableKey;
  noChangePlan.sourceDocumentId = live.facade.document().id();
  noChangePlan.sourceDocumentRevision = live.facade.document().revision();
  noChangePlan.sourceTerrainRevision =
      live.facade.document().terrainField().revision();
  noChangePlan.sourceMaterialRevision =
      live.facade.document().terrainMaterialField().revision();
  const auto applied = app::applyCreativeEditorWorldLayoutPlanWithHistory(
      state, live, noChangePlan,
      app::captureCreativeEditorWorldLayoutSnapshot(state),
      "no_change_source_apply");
  const bool sourceRoundTrip =
      floor && removed.accepted && removed.changed &&
      state.source.buildings.empty() && state.source.boxes.empty() &&
      undoDepthBefore == 2U;
  const bool appliedWithoutDocumentChange =
      applied.accepted && !applied.changed;
  const bool synchronized = state.revision == state.generatedRevision;
  const bool documentHistoryUnchanged =
      cr::creativeUndoDepth(live.history) == 0U;
  const bool historyPreserved =
      app::creativeEditorWorldLayoutSourceUndoDepth(state) == undoDepthBefore;
  const bool undone =
      app::undoLastEdit(live, "no-change-apply-undo", &state);

  return expect(sourceRoundTrip,
                "the source returns to an output-equivalent state") &&
         expect(appliedWithoutDocumentChange,
                "the output-equivalent apply changes no document data") &&
         expect(synchronized,
                "the output-equivalent apply synchronizes the source") &&
         expect(documentHistoryUnchanged,
                "the output-equivalent apply records no document history") &&
         expect(historyPreserved,
                "a no-change apply preserves local source history") &&
         expect(undone && state.source.boxes.size() == 1U &&
                    state.revision != state.generatedRevision &&
                    live.facade.document().objectCount() == 0U,
                "preserved source history remains usable after apply");
}

}  // namespace

int main() {
  const bool ok = gestureCommitsOneEntryAndBranchesClearRedo() &&
                  sourceHistoryTrimsToItsConfiguredBound() &&
                  confirmHandsHistoryBackToTheDocumentOwner() &&
                  sourceRedoSurvivesDocumentHistoryRoundTrip() &&
                  dirtySourceProtectsUnrelatedDocumentHistoryWhenLocalHistoryIsDisabled() &&
                  sourceReplacementClearsLocalHistory() &&
                  savedCheckpointTracksAcrossSourceHistory() &&
                  noChangeApplyPreservesSourceHistory();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
