#include "EditorEdits.hpp"
#include "EditorFrame.hpp"
#include "EditorGizmo.hpp"
#include "EditorInteractionInternal.hpp"
#include "EditorPattern.hpp"
#include "EditorPlacementClearance.hpp"
#include "EditorState.hpp"
#include "EditorToolOptions.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutHistory.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBlockoutMaterialization.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

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

cr::CreativeObjectId createAndSelectRoom(
    cr::CreativeAppState& appState,
    cr::CreativeBounds bounds = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}},
    cr::CreativeObjectKind kind = cr::CreativeObjectKind::Room) {
  cr::CreativeDocumentCreateRequest create;
  create.kind = kind;
  create.name = "Array Source";
  create.bounds = bounds;
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

app::CreativeEditorState editorForArray();

bool previewReportsCollisionAndFinalBounds() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Collision Preview", 80U),
              "collision preview document installed") ||
      !expect(createAndSelectRoom(
                  appState, {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}},
                  cr::CreativeObjectKind::Wall) != cr::kInvalidObjectId,
              "solid array source selected")) {
    return false;
  }
  cr::CreativeDocumentCreateRequest blockerRequest;
  blockerRequest.kind = cr::CreativeObjectKind::Wall;
  blockerRequest.name = "Array Blocker";
  blockerRequest.bounds = {{2.0, 0.0, 0.0}, {3.0, 1.0, 1.0}};
  blockerRequest.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt blocker =
      appState.facade.createDocumentObject(blockerRequest);
  if (!expect(blocker.accepted, "array blocker created")) {
    return false;
  }

  app::CreativeEditorState editor = editorForArray();
  app::CreativePlacementClearanceCache clearance;
  const bool cacheReady = app::refreshCreativePlacementClearanceCache(
      clearance, appState.facade.document());
  const app::CreativeEditorPatternPreviewReceipt preview =
      app::evaluateCreativeEditorArrayPreview(appState, editor, &clearance);
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> lines;
  const std::size_t lineCount = app::appendCreativeEditorArrayPreview(
      appState, editor, 0.05F, lines, &clearance);

  return expect(cacheReady && preview.accepted && !preview.collisionFree &&
                    preview.status ==
                        app::CreativeEditorPatternPreviewStatus::Collision,
                "valid array plan reports its blocked occupancy") &&
         expect(preview.linear.direction ==
                        cr::CreativeLinearArrayDirection::PositiveX &&
                    preview.linear.copyCount ==
                        cr::CreativeLinearArrayCopyCount::Two &&
                    preview.linear.spacing ==
                        cr::CreativeLinearArraySpacing::TwoCells &&
                    preview.linear.cellSize == 1.0,
                "preview receipt carries exact linear parameters") &&
         expect(preview.generatedObjectCount == 2U &&
                    preview.objectCount == 2U &&
                    preview.collidingObjectCount == 1U &&
                    preview.blockingObjectId == blocker.objectId &&
                    preview.objects[0].colliding &&
                    !preview.objects[1].colliding,
                "only the copy occupying the blocker is marked") &&
         expect(preview.hasFinalBounds && preview.finalBounds.min.x == 0.0 &&
                    preview.finalBounds.max.x == 5.0 &&
                    preview.finalBounds.min.y == 0.0 &&
                    preview.finalBounds.max.y == 1.0,
                "final bounds include the source and every planned copy") &&
         expect(lineCount == 24U && lines.size() == 24U &&
                    lines[0].color.r == 1.0F &&
                    lines[0].color.g == 0.18F &&
                    lines[12].color.r == 0.22F &&
                    lines[12].color.g == 0.88F,
                "collision renders red while clear copies remain cyan");
}

bool hiddenHierarchyObstaclesMatchColdAndWarmClearance() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Hierarchy Collision Preview");
  static_cast<void>(document.assignId(82U));

  cr::CreativeDocumentCreateRequest sourceRequest;
  sourceRequest.kind = cr::CreativeObjectKind::Wall;
  sourceRequest.name = "Array Source";
  sourceRequest.bounds = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}};
  sourceRequest.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt source =
      document.createObject(sourceRequest);

  cr::CreativeDocumentCreateRequest groupRequest;
  groupRequest.kind = cr::CreativeObjectKind::Group;
  groupRequest.name = "Hidden Blocker Group";
  const cr::CreativeDocumentCreateReceipt group =
      document.createObject(groupRequest);

  cr::CreativeDocumentCreateRequest blockerRequest;
  blockerRequest.kind = cr::CreativeObjectKind::Wall;
  blockerRequest.name = "Locally Visible Blocker";
  blockerRequest.bounds = {{2.0, 0.0, 0.0}, {3.0, 1.0, 1.0}};
  blockerRequest.hasBoundsOverride = true;
  blockerRequest.parentId = group.objectId;
  const cr::CreativeDocumentCreateReceipt blocker =
      document.createObject(blockerRequest);
  const cr::CreativeDocumentMutationReceipt hidden =
      cr::setDocumentObjectVisible(document, group.objectId, false);

  cr::CreativeAppState appState;
  if (!expect(source.accepted && group.accepted && blocker.accepted &&
                  hidden.changed &&
                  appState.facade.installDocument(std::move(document)).accepted,
              "hidden hierarchy collision fixture installed")) {
    return false;
  }
  cr::CreativeToolInputPacket select;
  select.kind = cr::CreativeToolInputKind::PointerPress;
  select.pointer.button = cr::CreativeToolPointerButton::Primary;
  select.pointer.target.value = static_cast<cr::Id>(source.objectId);
  static_cast<void>(appState.facade.dispatchToolInput(select));

  app::CreativeEditorState editor = editorForArray();
  const app::CreativeEditorPatternPreviewReceipt cold =
      app::evaluateCreativeEditorArrayPreview(appState, editor);
  app::CreativePlacementClearanceCache clearance;
  const bool cacheReady = app::refreshCreativePlacementClearanceCache(
      clearance, appState.facade.document());
  const app::CreativeEditorPatternPreviewReceipt warm =
      app::evaluateCreativeEditorArrayPreview(appState, editor, &clearance);
  const cr::CreativeObject* localBlocker =
      appState.facade.findObject(blocker.objectId);

  return expect(localBlocker != nullptr && localBlocker->visible,
                "hidden hierarchy keeps blocker locally visible") &&
         expect(cacheReady && cold.accepted && cold.collisionFree &&
                    warm.accepted && warm.collisionFree,
                "hidden hierarchy never blocks cold or warm preview") &&
         expect(cold.blockingObjectId == cr::kInvalidObjectId &&
                    warm.blockingObjectId == cr::kInvalidObjectId &&
                    cold.collidingObjectCount == 0U &&
                    warm.collidingObjectCount == 0U,
                "cold and warm hierarchy collision facts match");
}

bool liveOptionsDraftPreviewIsBoundedAndRevisionFree() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Draft Preview", 81U),
              "draft preview document installed") ||
      !expect(createAndSelectRoom(appState) != cr::kInvalidObjectId,
              "draft preview source selected")) {
    return false;
  }
  app::CreativeEditorState editor = editorForArray();
  editor.toolOptions.open = true;
  editor.toolOptions.targetEntry =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  editor.toolOptions.draft = editor.toolSettings;
  editor.toolOptions.draft.arrayCopyCount =
      cr::CreativeLinearArrayCopyCount::ThirtyTwo;
  editor.toolOptions.draft.arraySpacing =
      cr::CreativeLinearArraySpacing::OneCell;
  editor.toolOptions.placeCellSizeDraft = 0.5;
  app::CreativePlacementClearanceCache clearance;
  if (!expect(app::refreshCreativePlacementClearanceCache(
                  clearance, appState.facade.document()),
              "draft preview clearance cache ready")) {
    return false;
  }
  const std::uint64_t revisionBefore =
      appState.facade.document().revision();
  const std::uint64_t cacheBuildsBefore = clearance.rebuildCount;
  app::CreativeEditorPatternPreviewReceipt preview;
  for (std::size_t frame = 0; frame < 300U; ++frame) {
    preview = app::evaluateCreativeEditorArrayPreview(
        appState, editor, &clearance);
  }
  return expect(preview.accepted && preview.generatedObjectCount == 32U &&
                    preview.objectCount == 32U &&
                    preview.linear.copyCount ==
                        cr::CreativeLinearArrayCopyCount::ThirtyTwo &&
                    preview.linear.spacing ==
                        cr::CreativeLinearArraySpacing::OneCell &&
                    preview.linear.cellSize == 0.5,
                "open options preview reads the bounded live draft") &&
         expect(preview.hasFinalBounds &&
                    preview.finalBounds.min.x == 0.0 &&
                    preview.finalBounds.max.x == 17.0,
                "maximum draft preview reports its final extent") &&
         expect(appState.facade.document().revision() == revisionBefore &&
                    clearance.rebuildCount == cacheBuildsBefore &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "300 draft previews cause no document, cache, or history work");
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
  const cr::CreativeVec3 pivot{3.0, 1.0, -2.0};
  const cr::CreativeRadialArrayRequest radial =
      app::creativeEditorRadialArrayRequest(editor.toolSettings, pivot);
  return expect(request.direction ==
                    cr::CreativeLinearArrayDirection::PositiveX,
                "adapter maps direction") &&
         expect(request.copyCount == cr::CreativeLinearArrayCopyCount::Two,
                "adapter maps copy count") &&
         expect(request.spacing == cr::CreativeLinearArraySpacing::TwoCells,
                "adapter maps spacing") &&
         expect(request.cellSize == 0.5, "adapter maps cell size") &&
         expect(cr::creativeVec3ExactlyEqual(radial.pivot, pivot) &&
                    radial.axis == editor.toolSettings.radialArrayAxis &&
                    radial.instanceCount ==
                        editor.toolSettings.radialArrayInstanceCount &&
                    radial.sweep == editor.toolSettings.radialArraySweep,
                "adapter maps radial pivot and options explicitly");
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
  const cr::CreativePatternRecipe* recipe = cr::findCreativePatternRecipe(
      appState.facade.document().patternRecipeStore(), applied.patternRecipeId);
  const cr::CreativeAuthoringOperationRecord* operation =
      cr::creativeHistoryTargetOperation(
          appState.history, cr::CreativeHistoryDirection::Undo);
  const std::optional<cr::CreativeAuthoringOperationRecord> expectedOperation =
      operation != nullptr
          ? std::optional<cr::CreativeAuthoringOperationRecord>{*operation}
          : std::nullopt;
  bool ok = expect(applied.accepted && applied.changed &&
                       appState.facade.document().objectCount() == 3U,
                   "adapter commits two new copies") &&
            expect(cr::creativeUndoDepth(appState.history) == 1U &&
                       cr::creativeRedoDepth(appState.history) == 0U,
                   "array commit records exactly one undo step") &&
            expect(appState.facade.selectionState().selectedTarget.value == 3U,
                   "array commit selects the final copy") &&
            expect(recipe != nullptr && expectedOperation.has_value() &&
                       expectedOperation->family ==
                           cr::CreativeAuthoringFamily::Pattern &&
                       expectedOperation->kind ==
                           cr::CreativeAuthoringOperationKind::Apply &&
                       expectedOperation->lifecycle ==
                           cr::CreativeAuthoringLifecycle::Parametric &&
                       expectedOperation->action == "LinearArray.Create" &&
                       expectedOperation->requestFingerprint ==
                           cr::fingerprintCreativePatternRecipeSource(*recipe) &&
                       expectedOperation->affectedMemberCount == 2U,
                   "array commit records its exact durable source operation");

  const cr::CreativeHistoryApplyReceipt undo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  const cr::CreativeHistoryApplyReceipt redo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Redo);
  ok = expect(undo.accepted && undo.objectCountAfter == 1U,
              "single undo removes the full array batch") &&
       expect(redo.accepted && redo.objectCountAfter == 3U,
              "single redo restores the full array batch") &&
       expect(undo.targetOperation == expectedOperation &&
                  redo.targetOperation == expectedOperation,
              "array operation metadata survives both history directions") &&
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

bool radialPreviewUsesCrosshairPivotWithoutMutation() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Radial Preview", 77U),
              "radial preview document installed") ||
      !expect(createAndSelectRoom(appState) != cr::kInvalidObjectId,
              "radial preview source selected")) {
    return false;
  }
  app::CreativeEditorState editor = editorForArray();
  editor.toolSettings.arrayMode = cr::CreativeArrayMode::Radial;
  editor.toolSettings.radialArrayAxis = cr::CreativeAxis3::Y;
  editor.toolSettings.radialArrayInstanceCount =
      cr::CreativeRadialArrayInstanceCount::Four;
  editor.toolSettings.radialArraySweep =
      cr::CreativeRadialArraySweep::Degrees360;
  editor.interaction.target.grid.valid = true;
  editor.interaction.target.grid.placementAnchor = {0.5, 0.0, -1.5};
  const std::size_t objectCountBefore = appState.facade.document().objectCount();
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> lines;

  const app::CreativeEditorPatternPreviewReceipt preview =
      app::evaluateCreativeEditorArrayPreview(appState, editor);
  const std::size_t appended =
      app::appendCreativeEditorArrayPreview(appState, editor, 0.05F, lines);

  return expect(preview.accepted && preview.collisionFree &&
                    preview.kind ==
                        cr::CreativePatternRecipeKind::RadialArray &&
                    cr::creativeVec3ExactlyEqual(preview.radial.pivot,
                                                 {0.5, 0.0, -1.5}) &&
                    preview.radial.axis == cr::CreativeAxis3::Y &&
                    preview.radial.instanceCount ==
                        cr::CreativeRadialArrayInstanceCount::Four &&
                    preview.radial.sweep ==
                        cr::CreativeRadialArraySweep::Degrees360 &&
                    preview.generatedObjectCount == 3U,
                "radial preview receipt preserves every control") &&
         expect(preview.hasFinalBounds &&
                    preview.finalBounds.min.x == -2.0 &&
                    preview.finalBounds.max.x == 3.0 &&
                    preview.finalBounds.min.z == -4.0 &&
                    preview.finalBounds.max.z == 1.0,
                "radial final bounds include the source and full ring") &&
         expect(appended == 48U && lines.size() == 48U,
                "three radial copies and pivot use bounded wire boxes") &&
         expect(lines[0].start.x == 2.0F && lines[0].end.x == 3.0F &&
                    lines[0].start.z == -2.0F,
                "radial preview revolves source around crosshair pivot") &&
         expect(lines[36].color.r == 0.96F &&
                    lines[36].color.g == 0.74F,
                "radial preview marks pivot distinctly") &&
         expect(appState.facade.document().objectCount() == objectCountBefore &&
                    appState.facade.document().revision() == revisionBefore &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "radial preview is transient");
}

bool radialCommitRecordsOneUndoStepAndRequiresPivot() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Radial Commit", 78U),
              "radial commit document installed") ||
      !expect(createAndSelectRoom(appState) != cr::kInvalidObjectId,
              "radial commit source selected")) {
    return false;
  }
  app::CreativeEditorState editor = editorForArray();
  editor.toolSettings.arrayMode = cr::CreativeArrayMode::Radial;
  editor.toolSettings.radialArrayInstanceCount =
      cr::CreativeRadialArrayInstanceCount::Four;
  editor.toolSettings.radialArraySweep =
      cr::CreativeRadialArraySweep::Degrees360;
  const cr::CreativeVec3 pivot{0.5, 0.0, -1.5};

  const bool missingPivot = app::applyCreativeEditorArrayWithHistory(
      appState, editor.pattern, editor.toolSettings, editor.placeCellSize, false,
      pivot, "test_radial_missing_pivot");
  bool ok = expect(!missingPivot &&
                       appState.facade.document().objectCount() == 1U &&
                       cr::creativeUndoDepth(appState.history) == 0U,
                   "radial commit fails closed without a live pivot");

  const bool applied = app::applyCreativeEditorArrayWithHistory(
      appState, editor.pattern, editor.toolSettings, editor.placeCellSize, true,
      pivot, "test_radial_array");
  ok = expect(applied && editor.pattern.lastRadialReceipt.accepted &&
                  editor.pattern.lastRadialReceipt.changed &&
                  appState.facade.document().objectCount() == 4U,
              "radial adapter commits generated copies") &&
       expect(cr::creativeUndoDepth(appState.history) == 1U &&
                  appState.facade.selectionState().selectedTarget.value == 4U,
              "radial commit records one undo and selects final copy") &&
       ok;
  const cr::CreativeHistoryApplyReceipt undo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  return expect(undo.accepted && undo.objectCountAfter == 1U,
                "one undo removes complete radial batch") &&
         ok;
}

bool editableArrayReopensUpdatesAndDetachesWithHistory() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Editable Array", 79U),
              "editable array document installed") ||
      !expect(createAndSelectRoom(appState) != cr::kInvalidObjectId,
              "editable array source selected")) {
    return false;
  }
  app::CreativeEditorState editor = editorForArray();
  const cr::CreativeLinearArrayReceipt created =
      app::applyCreativeEditorLinearArrayWithHistory(
          appState, editor.pattern, editor.toolSettings,
          editor.placeCellSize, "test_editable_array_create");
  const cr::CreativePatternRecipe* selectedRecipe =
      app::creativeEditorSelectedPatternRecipe(appState);
  if (!expect(created.accepted && selectedRecipe != nullptr,
              "generated selection resolves its editable relation")) {
    return false;
  }
  const cr::CreativePatternRecipeId recipeId = selectedRecipe->id;

  cr::CreativeToolSettings reopenedSettings;
  reopenedSettings.arrayDirection =
      cr::CreativeLinearArrayDirection::PositiveY;
  reopenedSettings.arrayCopyCount =
      cr::CreativeLinearArrayCopyCount::ThirtyTwo;
  double reopenedCellSize = 9.0;
  const bool loaded = app::loadCreativeEditorPatternRecipeSettings(
      *selectedRecipe, reopenedSettings, reopenedCellSize);

  editor.toolSettings.arrayDirection =
      cr::CreativeLinearArrayDirection::NegativeZ;
  editor.toolSettings.arrayCopyCount =
      cr::CreativeLinearArrayCopyCount::Four;
  editor.toolSettings.arraySpacing =
      cr::CreativeLinearArraySpacing::OneCell;
  editor.placeCellSize = 0.5;
  const std::uint64_t previewRevision =
      appState.facade.document().revision();
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> previewLines;
  const std::size_t previewCount =
      app::appendCreativeEditorLinearArrayPreview(
          appState, editor, 0.05F, previewLines);
  const std::uint64_t revisionAfterPreview =
      appState.facade.document().revision();
  const cr::CreativeLinearArrayReceipt updated =
      app::applyCreativeEditorLinearArrayWithHistory(
          appState, editor.pattern, editor.toolSettings,
          editor.placeCellSize, "test_editable_array_update");
  const cr::CreativePatternRecipe* updatedRecipe =
      cr::findCreativePatternRecipe(
          appState.facade.document().patternRecipeStore(), recipeId);

  bool ok = expect(loaded &&
                       reopenedSettings.arrayMode ==
                           cr::CreativeArrayMode::Linear &&
                       reopenedSettings.arrayDirection ==
                           cr::CreativeLinearArrayDirection::PositiveX &&
                       reopenedSettings.arrayCopyCount ==
                           cr::CreativeLinearArrayCopyCount::Two &&
                       reopenedSettings.arraySpacing ==
                           cr::CreativeLinearArraySpacing::TwoCells &&
                       reopenedCellSize == 1.0,
                   "reopen loads every stored linear parameter") &&
            expect(previewCount == 48U && previewLines.size() == 48U &&
                       previewLines.front().start.z == -0.5F &&
                       revisionAfterPreview == previewRevision,
                   "editable preview uses original source before update") &&
            expect(updated.accepted && updated.updatedExistingRecipe &&
                       updated.patternRecipeId == recipeId &&
                       appState.facade.document().objectCount() == 5U &&
                       cr::creativeUndoDepth(appState.history) == 2U,
                   "update replaces relation outputs in one history step") &&
            expect(updatedRecipe != nullptr &&
                       updatedRecipe->linear.direction ==
                           cr::CreativeLinearArrayDirection::NegativeZ &&
                       updatedRecipe->linear.copyCount ==
                           cr::CreativeLinearArrayCopyCount::Four &&
                       updatedRecipe->linear.cellSize == 0.5,
                   "update stores the reopened relation parameters");

  const cr::CreativeHistoryApplyReceipt undoUpdate =
      cr::applyCreativeHistory(appState.facade, appState.history,
                               cr::CreativeHistoryDirection::Undo);
  const cr::CreativePatternRecipe* originalRecipe =
      cr::findCreativePatternRecipe(
          appState.facade.document().patternRecipeStore(), recipeId);
  const std::size_t undoObjectCount =
      appState.facade.document().objectCount();
  const bool originalParametersRestored =
      originalRecipe != nullptr &&
      originalRecipe->linear.copyCount ==
          cr::CreativeLinearArrayCopyCount::Two;
  const cr::CreativeHistoryApplyReceipt redoUpdate =
      cr::applyCreativeHistory(appState.facade, appState.history,
                               cr::CreativeHistoryDirection::Redo);
  ok = expect(undoUpdate.accepted && undoObjectCount == 3U,
              "update undo restores the original generated count") &&
       expect(undoUpdate.targetOperation.has_value() &&
                  undoUpdate.targetOperation->kind ==
                      cr::CreativeAuthoringOperationKind::Reconcile &&
                  undoUpdate.targetOperation->action == "LinearArray.Update",
              "array update records reconciliation rather than creation") &&
       expect(originalParametersRestored,
              "update undo restores original recipe parameters") &&
       expect(redoUpdate.accepted &&
                  appState.facade.document().objectCount() == 5U,
              "update redo restores replacement geometry") &&
       ok;

  editor.toolOptions.open = true;
  editor.toolOptions.targetEntry =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  editor.toolOptions.draft = editor.toolSettings;
  editor.toolOptions.contextPatternRecipeId = recipeId;
  editor.toolOptions.options = app::creativeEditorToolOptionsForEntry(
      editor.toolOptions.targetEntry, editor.toolOptions.draft);
  editor.toolOptions.commands = app::creativeEditorToolOptionCommandsForEntry(
      editor.toolOptions.targetEntry, cr::CreativeObjectKind::Room, recipeId);
  editor.toolOptions.selectedIndex = editor.toolOptions.options.count;
  const bool detachCommandPresent =
      editor.toolOptions.commands.count == 1U &&
      editor.toolOptions.commands.ids[0] ==
          app::CreativeEditorToolOptionsCommandId::DetachPatternRecipe;
  const std::size_t historyDepthBeforeDetach =
      cr::creativeUndoDepth(appState.history);
  const bool detached = app::activateCreativeEditorToolOptionsSelection(
      appState, editor);
  const std::size_t bakedObjectCount =
      appState.facade.document().objectCount();
  const bool detachedStoreEmpty =
      appState.facade.document().patternRecipeStore().recipes.empty();
  const std::size_t historyDepthAfterDetach =
      cr::creativeUndoDepth(appState.history);
  const cr::CreativeHistoryApplyReceipt undoDetach =
      cr::applyCreativeHistory(appState.facade, appState.history,
                               cr::CreativeHistoryDirection::Undo);
  const bool undoDetachRestored =
      cr::findCreativePatternRecipe(
          appState.facade.document().patternRecipeStore(), recipeId) !=
      nullptr;
  const cr::CreativeHistoryApplyReceipt redoDetach =
      cr::applyCreativeHistory(appState.facade, appState.history,
                               cr::CreativeHistoryDirection::Redo);
  const bool redoDetachRestored =
      appState.facade.document().patternRecipeStore().recipes.empty() &&
      appState.facade.document().objectCount() == bakedObjectCount;
  return expect(detachCommandPresent && detached && !editor.toolOptions.open &&
                    historyDepthAfterDetach ==
                        historyDepthBeforeDetach + 1U &&
                    detachedStoreEmpty &&
                    appState.facade.document().objectCount() ==
                        bakedObjectCount,
                "tool options detach keeps baked copies in one history step") &&
         expect(undoDetach.accepted &&
                    undoDetach.targetOperation.has_value() &&
                    undoDetach.targetOperation->kind ==
                        cr::CreativeAuthoringOperationKind::Destructive &&
                    undoDetach.targetOperation->lifecycle ==
                        cr::CreativeAuthoringLifecycle::Destructive &&
                    undoDetachRestored,
                "detach is independently undoable") &&
         expect(redoDetach.accepted &&
                    redoDetach.targetOperation ==
                        undoDetach.targetOperation &&
                    redoDetachRestored,
                "detach reapplies without deleting baked outputs") &&
         ok;
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
  ok = expect(app::cycleCreativeEditorTransformMode(
                  appState, editor.transform) &&
                  editor.transform.transformMode ==
                      app::CreativeEditorTransformMode::Rotate &&
                  app::cycleCreativeEditorTransformMode(
                      appState, editor.transform) &&
                  editor.transform.transformMode ==
                      app::CreativeEditorTransformMode::Scale,
              "clipboard transform exposes duplicate scaling") &&
       ok;

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

bool selectionTransformScalePreviewMatchesCommitAndOneUndo() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Transform Scale", 79U),
              "scale transform document installed") ||
      !expect(createAndSelectRoom(appState) != cr::kInvalidObjectId,
              "scale transform source selected")) {
    return false;
  }
  app::CreativeEditorState editor;
  if (!expect(app::beginCreativeEditorSelectionTransformPreview(
                  appState, editor.transform, "test_scale_begin"),
              "scale transform begins")) {
    return false;
  }
  const cr::CreativeVec3 anchor =
      editor.transform.sourceClipboard.placementAnchor;
  static_cast<void>(app::processCreativeEditorSelectionTransformPreview(
      appState, editor.transform, true, anchor, false, "test_scale_aim"));
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  bool ok = expect(app::cycleCreativeEditorTransformMode(
                       appState, editor.transform) &&
                       app::cycleCreativeEditorTransformMode(
                           appState, editor.transform) &&
                       editor.transform.transformMode ==
                           app::CreativeEditorTransformMode::Scale,
                   "Square mode cycle reaches scale for live selection") &&
            expect(app::adjustCreativeEditorTransformSetting(
                       appState, editor.transform, 1) &&
                       app::adjustCreativeEditorTransformSetting(
                           appState, editor.transform, 1) &&
                       cr::creativeVec3ExactlyEqual(
                           app::creativeEditorTransformScaleFactor(
                               editor.transform),
                           {1.5, 1.5, 1.5}) &&
                       editor.transform.plan.accepted,
                   "D-pad scale adjustment reaches bounded 1.5x preview");

  const cr::CreativeObject planned = editor.transform.plan.objects.front();
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> lines;
  const std::size_t lineCount =
      app::appendCreativeEditorSelectionTransformPreview(
          editor.transform, 0.05F, lines);
  ok = expect(lineCount >= 48U && lines.size() >= 48U &&
                  lines[24].color.g == 1.0F &&
                  lines[24].start.x == -0.25F &&
                  lines[24].end.x == 1.25F,
              "scale preview renders exact mint destination bounds") &&
       expect(appState.facade.document().revision() == revisionBefore &&
                  cr::creativeUndoDepth(appState.history) == 0U,
              "scale preview remains transient") &&
       ok;

  static_cast<void>(app::requestCreativeEditorSelectionTransformCommit(
      editor.transform));
  const app::CreativeEditorTransformCommitReceipt committed =
      app::processCreativeEditorSelectionTransformPreview(
          appState, editor.transform, true, anchor, false,
          "test_scale_commit");
  const cr::CreativeObject* scaled = appState.facade.findObject(1U);
  ok = expect(committed.accepted && committed.changed && scaled != nullptr &&
                  cr::creativeBoundsExactlyEqual(scaled->bounds,
                                                 planned.bounds) &&
                  cr::creativeVec3ExactlyEqual(scaled->transform.scale,
                                               planned.transform.scale),
              "scale commit matches preview geometry exactly") &&
       expect(appState.facade.document().revision() == revisionBefore + 1U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "scale commit advances once and records one undo") &&
       ok;

  const cr::CreativeHistoryApplyReceipt undo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  scaled = appState.facade.findObject(1U);
  return expect(undo.accepted && scaled != nullptr &&
                    scaled->bounds.min.x == 0.0 &&
                    scaled->bounds.max.x == 1.0,
                "one undo restores pre-scale geometry") &&
         ok;
}

bool selectionTransformUsesAllAxesAndNonUniformScale() {
  cr::CreativeAppState appState;
  const cr::CreativeBounds originalBounds{{0.0, 0.0, 0.0},
                                           {1.0, 2.0, 3.0}};
  if (!expect(installDocument(appState, "Axis Transform", 80U),
              "axis transform document installed") ||
      !expect(createAndSelectRoom(appState, originalBounds) !=
                  cr::kInvalidObjectId,
              "rectangular transform source selected")) {
    return false;
  }
  app::CreativeEditorState editor;
  if (!expect(app::beginCreativeEditorSelectionTransformPreview(
                  appState, editor.transform, "test_axis_transform_begin"),
              "axis transform begins")) {
    return false;
  }
  const cr::CreativeVec3 anchor =
      editor.transform.sourceClipboard.placementAnchor;
  static_cast<void>(app::processCreativeEditorSelectionTransformPreview(
      appState, editor.transform, true, anchor, false,
      "test_axis_transform_aim"));
  const auto planSize = [&editor]() {
    return cr::measureCreativeBounds(
               editor.transform.plan.objects.front().bounds)
        .size;
  };
  const auto near = [](double actual, double expected) {
    return std::fabs(actual - expected) <= 1.0e-9;
  };

  bool ok = expect(app::cycleCreativeEditorTransformMode(
                       appState, editor.transform) &&
                       app::adjustCreativeEditorTransformSetting(
                           appState, editor.transform, 1) &&
                       editor.transform.request.hasAxisAngleRotation &&
                       editor.transform.request.rotationAxis ==
                           cr::CreativeAxis3::Y &&
                       editor.transform.rotationQuarterSteps == 1,
                   "Free rotation uses the familiar Y axis") &&
            expect(near(planSize().x, 3.0) && near(planSize().y, 2.0) &&
                       near(planSize().z, 1.0),
                   "Y rotation swaps rectangular X and Z extents");

  ok = expect(app::setCreativeEditorTransformRotationDegrees(
                       appState, editor.transform, cr::CreativeAxis3::X,
                       editor.transform.rotationDegrees) &&
                       editor.transform.request.rotationAxis ==
                           cr::CreativeAxis3::X &&
                       near(planSize().x, 1.0) && near(planSize().y, 3.0) &&
                       near(planSize().z, 2.0),
                   "X rotation swaps rectangular Y and Z extents") &&
       expect(app::setCreativeEditorTransformRotationDegrees(
                  appState, editor.transform, cr::CreativeAxis3::Z,
                  editor.transform.rotationDegrees) &&
                  editor.transform.request.rotationAxis ==
                      cr::CreativeAxis3::Z &&
                  near(planSize().x, 2.0) && near(planSize().y, 1.0) &&
                  near(planSize().z, 3.0),
              "Z rotation swaps rectangular X and Y extents") &&
       ok;

  ok = expect(app::setCreativeEditorTransformRotationDegrees(
                       appState, editor.transform, cr::CreativeAxis3::X,
                       editor.transform.rotationDegrees) &&
                       app::setCreativeEditorTransformConstraint(
                           appState, editor.transform,
                           cr::CreativeSelectionPlacementAxis::X) &&
                       app::cycleCreativeEditorTransformMode(
                           appState, editor.transform) &&
                       editor.transform.transformMode ==
                           app::CreativeEditorTransformMode::Scale &&
                       app::adjustCreativeEditorTransformSetting(
                           appState, editor.transform, 1) &&
                       cr::creativeVec3ExactlyEqual(
                           app::creativeEditorTransformScaleFactor(
                               editor.transform),
                           {1.25, 1.0, 1.0}),
                   "Scale adjusts only the active X channel") &&
       expect(app::applyCreativeEditorTransformControl(
                  appState, editor.transform,
                  app::CreativeEditorTransformControl::CycleConstraint) &&
                  editor.transform.transformMode ==
                      app::CreativeEditorTransformMode::Scale &&
                  editor.transform.constraint ==
                      cr::CreativeSelectionPlacementAxis::Y &&
                  editor.transform.request.rotationAxis ==
                      cr::CreativeAxis3::X &&
                  app::adjustCreativeEditorTransformSetting(
                      appState, editor.transform, 1) &&
                  cr::creativeVec3ExactlyEqual(
                      app::creativeEditorTransformScaleFactor(editor.transform),
                      {1.25, 1.25, 1.0}),
              "Radial Axis preserves Scale mode and previous X rotation") &&
       expect(near(planSize().x, 1.25) && near(planSize().y, 3.0) &&
                  near(planSize().z, 2.5),
              "non-uniform scale composes before X rotation") &&
       ok;

  app::CreativeEditorSelectionTransformState resetProbe = editor.transform;
  ok = expect(app::applyCreativeEditorTransformControl(
                       appState, resetProbe,
                       app::CreativeEditorTransformControl::Reset) &&
                       resetProbe.transformMode ==
                           app::CreativeEditorTransformMode::Move &&
                       resetProbe.constraint ==
                           cr::CreativeSelectionPlacementAxis::Free &&
                       resetProbe.rotationAxis == cr::CreativeAxis3::Y &&
                       resetProbe.rotationQuarterSteps == 0 &&
                       !resetProbe.request.hasAxisAngleRotation &&
                       resetProbe.request.rotationAxis == cr::CreativeAxis3::Y &&
                       cr::creativeVec3ExactlyEqual(
                           app::creativeEditorTransformScaleFactor(resetProbe),
                           {1.0, 1.0, 1.0}),
                   "reset clears every axis-aware transform channel") &&
       ok;

  const cr::CreativeObject planned = editor.transform.plan.objects.front();
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  static_cast<void>(app::requestCreativeEditorSelectionTransformCommit(
      editor.transform));
  const app::CreativeEditorTransformCommitReceipt committed =
      app::processCreativeEditorSelectionTransformPreview(
          appState, editor.transform, true, anchor, false,
          "test_axis_transform_commit");
  const cr::CreativeObject* transformed = appState.facade.findObject(1U);
  ok = expect(committed.accepted && committed.changed &&
                  transformed != nullptr &&
                  cr::creativeBoundsExactlyEqual(transformed->bounds,
                                                 planned.bounds) &&
                  appState.facade.document().revision() == revisionBefore + 1U &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "axis transform commit exactly matches preview in one undo") &&
       ok;

  const cr::CreativeHistoryApplyReceipt undo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  transformed = appState.facade.findObject(1U);
  return expect(undo.accepted && transformed != nullptr &&
                    cr::creativeBoundsExactlyEqual(transformed->bounds,
                                                   originalBounds),
                "axis transform undo restores original bounds") &&
         ok;
}

bool exactTransformSupportsScaledDuplicateAndFixedNumericTarget() {
  cr::CreativeAppState appState;
  const cr::CreativeBounds originalBounds{{0.0, 0.0, 0.0},
                                           {1.0, 2.0, 3.0}};
  if (!expect(installDocument(appState, "Exact Transform", 801U),
              "exact transform document installed") ||
      !expect(createAndSelectRoom(appState, originalBounds,
                                  cr::CreativeObjectKind::Crate) !=
                  cr::kInvalidObjectId,
              "exact transform source selected")) {
    return false;
  }
  app::CreativeEditorState editor;
  if (!expect(app::beginCreativeEditorSelectionTransformPreview(
                  appState, editor.transform, "test_exact_transform_begin"),
              "exact transform begins")) {
    return false;
  }

  const cr::CreativeVec3 exactTarget{8.25, 1.5, -3.75};
  bool ok = expect(editor.transform.activeObjectId == 1U &&
                       app::setCreativeEditorTransformPivot(
                           appState, editor.transform,
                           app::CreativeEditorTransformPivot::ActiveObjectOrigin) &&
                       editor.transform.request.pivotMode ==
                           cr::CreativeSelectionPlacementPivotMode::SharedAnchor &&
                       cr::creativeVec3ExactlyEqual(
                           editor.transform.request.sourceAnchor,
                           {0.0, 0.0, 0.0}),
                   "active-object pivot resolves the primary source origin") &&
            expect(app::setCreativeEditorTransformPivot(
                       appState, editor.transform,
                       app::CreativeEditorTransformPivot::IndividualOrigins) &&
                       editor.transform.request.pivotMode ==
                           cr::CreativeSelectionPlacementPivotMode::
                               IndividualOrigins &&
                       cr::creativeVec3ExactlyEqual(
                           editor.transform.request.sourceAnchor,
                           editor.transform.sourceClipboard.placementAnchor) &&
                       app::setCreativeEditorTransformPivot(
                           appState, editor.transform,
                           app::CreativeEditorTransformPivot::ActiveObjectOrigin),
                   "editor pivot choice maps to the shared placement kernel") &&
            expect(app::setCreativeEditorTransformPlacementMode(
                       appState, editor.transform,
                       cr::CreativeSelectionPlacementMode::Copy) &&
                       editor.transform.mode ==
                           cr::CreativeSelectionPlacementMode::Copy,
                   "selection transform switches to duplicate mode") &&
            expect(app::setCreativeEditorTransformTargetAnchor(
                       appState, editor.transform, exactTarget) &&
                       editor.transform.anchorPolicy ==
                           app::CreativeEditorTransformAnchorPolicy::FixedTarget &&
                       cr::creativeVec3ExactlyEqual(
                           editor.transform.request.targetAnchor, exactTarget),
                   "numeric target becomes a stable fixed target") &&
            expect(app::setCreativeEditorTransformRotationDegrees(
                       appState, editor.transform, cr::CreativeAxis3::Z, 37.5) &&
                       editor.transform.request.rotationAxis ==
                           cr::CreativeAxis3::Z &&
                       editor.transform.rotationAxis == cr::CreativeAxis3::Z &&
                       editor.transform.constraint ==
                           cr::CreativeSelectionPlacementAxis::Free &&
                       std::abs(editor.transform.rotationDegrees - 37.5) <=
                           1.0e-9,
                   "numeric rotation accepts non-quarter-turn angles") &&
            expect(app::setCreativeEditorTransformScaleFactor(
                       appState, editor.transform, {1.2, 0.8, 1.6}) &&
                       cr::creativeVec3ExactlyEqual(
                           app::creativeEditorTransformScaleFactor(
                               editor.transform),
                           {1.2, 0.8, 1.6}) &&
                       editor.transform.plan.accepted,
                   "numeric scale accepts exact non-uniform factors");

  const cr::CreativeObject planned = editor.transform.plan.objects.front();
  static_cast<void>(app::processCreativeEditorSelectionTransformPreview(
      appState, editor.transform, true, {99.0, 99.0, 99.0}, false,
      "test_exact_transform_aim_overwrite"));
  ok = expect(cr::creativeVec3ExactlyEqual(
                  editor.transform.request.targetAnchor, exactTarget),
              "continued aim cannot overwrite a numeric target") &&
       ok;

  static_cast<void>(app::requestCreativeEditorSelectionTransformCommit(
      editor.transform));
  const app::CreativeEditorTransformCommitReceipt committed =
      app::processCreativeEditorSelectionTransformPreview(
          appState, editor.transform, true, {99.0, 99.0, 99.0}, false,
          "test_exact_transform_commit");
  const cr::CreativeObject* original = appState.facade.findObject(1U);
  const cr::CreativeObject* duplicate = appState.facade.findObject(2U);
  return expect(committed.accepted && committed.changed &&
                    committed.mode == cr::CreativeSelectionPlacementMode::Copy &&
                    original != nullptr && duplicate != nullptr,
                "exact duplicate transform commits once") &&
         expect(cr::creativeBoundsExactlyEqual(original->bounds,
                                               originalBounds),
                "duplicate transform leaves its source untouched") &&
         expect(cr::creativeBoundsExactlyEqual(duplicate->bounds,
                                               planned.bounds) &&
                    cr::creativeVec3ExactlyEqual(duplicate->transform.scale,
                                                 planned.transform.scale),
                "duplicate commit exactly matches its scaled preview") &&
         expect(cr::creativeUndoDepth(appState.history) == 1U,
                "exact duplicate transform records one undo step") &&
         ok;
}

bool transformCoordinateSpaceUsesTheActiveObjectBasis() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Transform Coordinates", 802U),
              "coordinate-space document installed")) {
    return false;
  }
  cr::CreativeDocumentCreateRequest create;
  create.kind = cr::CreativeObjectKind::Crate;
  create.name = "Rotated source";
  create.transform.position = {1.0, 0.5, 2.0};
  create.transform.rotationEulerRadians =
      {0.0, std::numbers::pi * 0.5, 0.0};
  create.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt created =
      appState.facade.createDocumentObject(create);
  const std::array selected{created.objectId};
  if (!expect(created.accepted &&
                  appState.facade
                      .selectTargets(selected, created.objectId)
                      .accepted,
              "rotated source becomes the primary selection")) {
    return false;
  }

  app::CreativeEditorState editor;
  if (!expect(app::beginCreativeEditorSelectionTransformPreview(
                  appState, editor.transform,
                  "test_transform_coordinate_space_begin"),
              "coordinate-space transform begins")) {
    return false;
  }
  const cr::CreativeVec3 source = editor.transform.request.sourceAnchor;
  const cr::CreativeVec3 aim{source.x + 2.0, source.y, source.z - 3.0};
  bool ok = expect(app::setCreativeEditorTransformCoordinateSpace(
                       appState, editor.transform,
                       cr::CreativeSelectionPlacementCoordinateSpace::Local) &&
                       editor.transform.request.coordinateSpace ==
                           cr::CreativeSelectionPlacementCoordinateSpace::Local &&
                       cr::creativeVec3ExactlyEqual(
                           editor.transform.request.coordinateBasisEulerRadians,
                           create.transform.rotationEulerRadians),
                   "Local captures the primary object's authored basis") &&
            expect(app::setCreativeEditorTransformConstraint(
                       appState, editor.transform,
                       cr::CreativeSelectionPlacementAxis::X),
                   "local X constraint selected");
  static_cast<void>(app::processCreativeEditorSelectionTransformPreview(
      appState, editor.transform, true, aim, false,
      "test_transform_coordinate_space_aim", 0.5));
  const auto near = [](double actual, double expected) {
    return std::fabs(actual - expected) <= 1.0e-9;
  };
  ok = expect(editor.transform.targetPositionable &&
                  near(editor.transform.request.targetAnchor.x, source.x) &&
                  near(editor.transform.request.targetAnchor.y, source.y) &&
                  near(editor.transform.request.targetAnchor.z,
                       source.z - 3.0),
              "local X follows the rotated object's negative world Z axis") &&
       expect(app::nudgeCreativeEditorSelectionTransform(
                  appState, editor.transform, 1, false) &&
                  near(editor.transform.request.targetAnchor.z,
                       source.z - 3.5),
              "local nudge follows the same rotated axis") &&
       ok;

  ok = expect(app::setCreativeEditorTransformCoordinateSpace(
                       appState, editor.transform,
                       cr::CreativeSelectionPlacementCoordinateSpace::World) &&
                       cr::creativeVec3ExactlyEqual(
                           editor.transform.nudgeOffset, {}) &&
                       near(editor.transform.request.targetAnchor.x,
                            source.x + 2.0) &&
                       near(editor.transform.request.targetAnchor.z, source.z),
                   "World resets local nudge and reprojects the live aim") &&
       expect(appState.facade.document().revision() == 1U &&
                  cr::creativeUndoDepth(appState.history) == 0U,
              "coordinate-space changes remain transient") &&
       ok;
  static_cast<void>(app::cancelCreativeEditorSelectionTransformPreview(
      editor.transform, "test_transform_coordinate_space_cancel"));
  return ok;
}

bool generatedBuildingTransformRoutesThroughWorldLayoutSource() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Generated Building Transform", 804U),
              "building transform document installed")) {
    return false;
  }

  cr::CreativeWorldLayout source;
  source.stableKey = "transform_building_layout";
  cr::CreativeWorldLayoutBuildingBlockoutRecipe recipe;
  recipe.request.footprint = {{0, 0}, {6, 4}};
  recipe.request.pattern =
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom;
  recipe.request.floorToFloorCells = 3U;
  recipe.request.wallThicknessCells = 0.25;
  const cr::CreativeWorldLayoutBuildingEditResult materialized =
      cr::materializeCreativeWorldLayoutBuildingBlockout(
          source, recipe, 1U);
  if (!expect(materialized.accepted && materialized.changed,
              "non-square building source materialized")) {
    return false;
  }

  app::CreativeEditorState editor;
  app::installCreativeEditorWorldLayout(
      editor.worldLayout, materialized.edited);
  editor.worldLayout.nextStableOrdinal = materialized.nextStableOrdinal;
  const app::CreativeEditorWorldLayoutApplyReceipt generated =
      app::confirmCreativeEditorWorldLayout(editor.worldLayout, appState);
  if (!expect(generated.accepted && generated.changed &&
                  editor.worldLayout.generatedRevision ==
                      editor.worldLayout.revision,
              "building source generated into the live document")) {
    return false;
  }

  std::vector<cr::CreativeObjectId> buildingObjectIds;
  for (const cr::CreativeObject& object : appState.facade.document().objects()) {
    if (cr::creativeWorldLayoutObjectBelongsToSource(
            editor.worldLayout.source, object,
            cr::CreativeWorldLayoutTable::Building, 0U)) {
      buildingObjectIds.push_back(object.id);
    }
  }
  if (!expect(!buildingObjectIds.empty() &&
                  appState.facade
                      .selectTargets(buildingObjectIds,
                                     buildingObjectIds.front())
                      .accepted,
              "complete generated building scope selected")) {
    return false;
  }

  cr::CreativeTransformCommandRequest directTransform;
  directTransform.kind = cr::CreativeTransformCommandKind::Translate;
  directTransform.translation = {1.0, 0.0, 0.0};
  const std::uint64_t admissionRevision =
      appState.facade.document().revision();
  const std::uint64_t admissionUndoDepth =
      cr::creativeUndoDepth(appState.history);
  const app::CreativeEditorSemanticEditReceipt direct =
      app::transformCreativeEditorSelectionWithUndo(
          appState, appState.history, directTransform,
          "test_world_layout_direct_transform", &editor.worldLayout);
  if (!expect(!direct.accepted && !direct.changed &&
                  direct.reasonCode ==
                      "creative_editor_transform_world_layout_use_transform_tool",
              "direct transform routes the building owner to its transform tool") ||
      !expect(appState.facade.document().revision() == admissionRevision &&
                  cr::creativeUndoDepth(appState.history) ==
                      admissionUndoDepth,
              "direct building admission does not mutate before the tool opens")) {
    return false;
  }

  ++editor.worldLayout.revision;
  const app::CreativeEditorSemanticEditReceipt staleDirect =
      app::transformCreativeEditorSelectionWithUndo(
          appState, appState.history, directTransform,
          "test_world_layout_stale_direct_transform", &editor.worldLayout);
  const bool stalePreview =
      app::beginCreativeEditorSelectionTransformPreview(
          appState, editor.transform,
          "test_world_layout_stale_transform",
          app::CreativeEditorTransformAnchorPolicy::FixedSource,
          &editor.worldLayout);
  if (!expect(!staleDirect.accepted &&
                  staleDirect.reasonCode ==
                      "creative_editor_object_action_world_layout_unsynchronized" &&
                  !stalePreview &&
                  editor.transform.preflight.reasonCode ==
                      "editor_transform_world_layout_source_unsynchronized",
              "direct and interactive transforms both reject stale sources") ||
      !expect(appState.facade.document().revision() == admissionRevision &&
                  cr::creativeUndoDepth(appState.history) ==
                      admissionUndoDepth,
              "stale source admission cannot mutate or record history")) {
    return false;
  }
  editor.worldLayout.generatedRevision = editor.worldLayout.revision;

  if (!expect(app::beginCreativeEditorSelectionTransformPreview(
                  appState, editor.transform,
                  "test_world_layout_building_transform",
                  app::CreativeEditorTransformAnchorPolicy::FixedSource,
                  &editor.worldLayout),
              "building source opens the shared transform session")) {
    return false;
  }
  const cr::CreativeVec3 sourceAnchor =
      editor.transform.request.sourceAnchor;
  bool ok = expect(
                editor.transform.preflight.ownershipRoute ==
                        app::CreativeEditorTransformOwnershipRoute::
                            WorldLayoutBuilding &&
                    editor.transform.preflight.worldLayoutSource.table ==
                        cr::CreativeWorldLayoutTable::Building &&
                    editor.transform.preflight.worldLayoutSource.index == 0U &&
                    editor.transform.preflight.capabilities.translate &&
                    editor.transform.preflight.capabilities.rotation ==
                        cr::CreativeObjectRotationSupport::QuarterTurns &&
                    editor.transform.preflight.capabilities.scale ==
                        cr::CreativeObjectScaleSupport::None,
                "building route publishes its source-level capabilities") &&
            expect(!app::setCreativeEditorTransformCoordinateSpace(
                       appState, editor.transform,
                       cr::CreativeSelectionPlacementCoordinateSpace::Local) &&
                       !app::setCreativeEditorTransformPivot(
                           appState, editor.transform,
                           app::CreativeEditorTransformPivot::IndividualOrigins) &&
                       !app::setCreativeEditorTransformConstraint(
                           appState, editor.transform,
                           cr::CreativeSelectionPlacementAxis::Y) &&
                       !app::setCreativeEditorTransformRotationDegrees(
                           appState, editor.transform, cr::CreativeAxis3::X,
                           90.0) &&
                       !app::setCreativeEditorTransformScaleFactor(
                           appState, editor.transform, {2.0, 2.0, 2.0}),
                   "building route rejects controls its source cannot represent") &&
            expect(app::setCreativeEditorTransformRotationDegrees(
                       appState, editor.transform, cr::CreativeAxis3::Y,
                       90.0) &&
                       editor.transform.plan.accepted &&
                       editor.transform.candidateWorldLayoutReady &&
                       editor.transform.candidateWorldLayoutChanged &&
                       editor.transform.plan.objectCount > 0U,
                   "quarter-turn building source produces an exact compiled preview");
  const cr::CreativeBounds previewBounds =
      editor.transform.plan.aggregateBounds;
  const std::uint64_t revisionBefore =
      appState.facade.document().revision();
  const std::uint64_t undoDepthBefore =
      cr::creativeUndoDepth(appState.history);
  static_cast<void>(app::requestCreativeEditorSelectionTransformCommit(
      editor.transform));
  const app::CreativeEditorTransformCommitReceipt committed =
      app::processCreativeEditorSelectionTransformPreview(
          appState, editor.transform, true, sourceAnchor, false,
          "test_world_layout_building_transform_commit", 1.0,
          &editor.worldLayout);

  cr::CreativeWorldLayoutBuildingBounds rotatedBounds;
  const bool measured = cr::measureCreativeWorldLayoutBuildingBounds(
      editor.worldLayout.source, 0U, rotatedBounds);
  cr::CreativeBounds actualBounds{};
  bool actualBoundsReady = false;
  for (const cr::CreativeObject& object : appState.facade.document().objects()) {
    if (!cr::creativeWorldLayoutObjectBelongsToSource(
            editor.worldLayout.source, object,
            cr::CreativeWorldLayoutTable::Building, 0U)) {
      continue;
    }
    const cr::CreativeTransformedBounds bounds =
        cr::resolveCreativeObjectBounds(object);
    if (!bounds.valid) {
      continue;
    }
    if (!actualBoundsReady) {
      actualBounds = bounds.worldBounds;
      actualBoundsReady = true;
      continue;
    }
    actualBounds.min.x = std::min(actualBounds.min.x, bounds.worldBounds.min.x);
    actualBounds.min.y = std::min(actualBounds.min.y, bounds.worldBounds.min.y);
    actualBounds.min.z = std::min(actualBounds.min.z, bounds.worldBounds.min.z);
    actualBounds.max.x = std::max(actualBounds.max.x, bounds.worldBounds.max.x);
    actualBounds.max.y = std::max(actualBounds.max.y, bounds.worldBounds.max.y);
    actualBounds.max.z = std::max(actualBounds.max.z, bounds.worldBounds.max.z);
  }
  ok = expect(committed.accepted && committed.changed &&
                  committed.worldLayoutReceipt.accepted &&
                  committed.worldLayoutReceipt.changed && measured &&
                  rotatedBounds.minimum == cr::CreativeTerrainCoord2{0, 0} &&
                  rotatedBounds.maximum == cr::CreativeTerrainCoord2{4, 6},
              "shared transform commits the canonical building source") &&
       expect(actualBoundsReady &&
                  cr::creativeBoundsExactlyEqual(actualBounds, previewBounds),
              "committed generated geometry exactly matches its preview") &&
       expect(appState.facade.document().revision() != revisionBefore &&
                  cr::creativeUndoDepth(appState.history) ==
                      undoDepthBefore + 1U,
              "source and generated geometry commit as one undo step") &&
       ok;

  const cr::CreativeHistoryApplyReceipt undone =
      app::applyCreativeEditorWorldLayoutHistory(
          editor.worldLayout, appState,
          cr::CreativeHistoryDirection::Undo);
  cr::CreativeWorldLayoutBuildingBounds restoredBounds;
  return expect(undone.accepted && undone.changed &&
                    cr::measureCreativeWorldLayoutBuildingBounds(
                        editor.worldLayout.source, 0U, restoredBounds) &&
                    restoredBounds.minimum == cr::CreativeTerrainCoord2{0, 0} &&
                    restoredBounds.maximum == cr::CreativeTerrainCoord2{6, 4},
                "one undo restores the canonical building source") &&
         ok;
}

bool generatedPatternOutputUsesSourceLevelTransform() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Generated Transform Ownership");
  static_cast<void>(document.assignId(803U));
  cr::CreativeDocumentCreateRequest create;
  create.kind = cr::CreativeObjectKind::Crate;
  create.name = "Pattern source";
  const cr::CreativeDocumentCreateReceipt source = document.createObject(create);
  create.name = "Pattern output";
  create.transform.position = {2.0, 0.0, 0.0};
  create.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt generated =
      document.createObject(create);
  cr::CreativePatternRecipeMutationRequest addRecipe;
  addRecipe.kind = cr::CreativePatternRecipeMutationKind::Add;
  addRecipe.recipe.kind = cr::CreativePatternRecipeKind::LinearArray;
  addRecipe.recipe.sourceObjectIds = {source.objectId};
  addRecipe.recipe.generatedObjectIds = {generated.objectId};
  const cr::CreativePatternRecipeMutationReceipt recipe =
      document.applyPatternRecipeMutation(addRecipe);

  cr::CreativeAppState appState;
  if (!expect(source.accepted && generated.accepted && recipe.accepted &&
                  appState.facade.installDocument(std::move(document)).accepted,
              "pattern-owned transform fixture installed")) {
    return false;
  }
  const std::array selected{generated.objectId};
  if (!expect(appState.facade
                  .selectTargets(selected, generated.objectId)
                  .accepted,
              "pattern output selected")) {
    return false;
  }

  app::CreativeEditorState editor;
  cr::CreativeTransformCommandRequest directTransform;
  directTransform.kind = cr::CreativeTransformCommandKind::Translate;
  directTransform.translation = {1.0, 0.0, 0.0};
  const std::uint64_t revisionBeforeAdmission =
      appState.facade.document().revision();
  const app::CreativeEditorSemanticEditReceipt direct =
      app::transformCreativeEditorSelectionWithUndo(
          appState, appState.history, directTransform,
          "test_pattern_direct_transform");
  if (!expect(!direct.accepted && !direct.changed &&
                  direct.reasonCode ==
                      "creative_editor_transform_pattern_requires_transform_tool",
              "direct transform routes Pattern output to its transform tool") ||
      !expect(appState.facade.document().revision() ==
                      revisionBeforeAdmission &&
                  cr::creativeUndoDepth(appState.history) == 0U,
              "Pattern transform admission cannot mutate or record history")) {
    return false;
  }
  const bool began = app::beginCreativeEditorSelectionTransformPreview(
      appState, editor.transform, "test_generated_transform_preflight",
      app::CreativeEditorTransformAnchorPolicy::FixedSource);
  if (!expect(began && editor.transform.active &&
                  editor.transform.preflight.accepted &&
                  editor.transform.preflight.ownershipRoute ==
                      app::CreativeEditorTransformOwnershipRoute::PatternRecipe &&
                  editor.transform.preflight.patternRecipeId == recipe.recipeId &&
                  editor.transform.sourceClipboard.objects.size() == 2U &&
                  editor.transform.sourceObjectIds.size() == 2U &&
                  editor.transform.plan.accepted &&
                  !editor.transform.preflight.capabilities.mirror &&
                  editor.transform.preflight.capabilities.rotation ==
                      cr::CreativeObjectRotationSupport::None &&
                  editor.transform.preflight.capabilities.scale ==
                      cr::CreativeObjectScaleSupport::None,
              "generated output opens one translation-only recipe session")) {
    return false;
  }
  const cr::CreativeVec3 sourceAnchor =
      editor.transform.request.sourceAnchor;
  const cr::CreativeVec3 targetAnchor{
      sourceAnchor.x + 3.0, sourceAnchor.y, sourceAnchor.z + 2.0};
  if (!expect(app::setCreativeEditorTransformTargetAnchor(
                  appState, editor.transform, targetAnchor),
              "recipe transform accepts an explicit target")) {
    return false;
  }
  const std::uint64_t revisionBefore =
      appState.facade.document().revision();
  const app::CreativeEditorTransformCommitReceipt committed =
      app::processCreativeEditorSelectionTransformPreview(
          appState, editor.transform, true, targetAnchor, true,
          "test_generated_transform_commit");
  const cr::CreativeObject* movedSource =
      appState.facade.findObject(source.objectId);
  const cr::CreativeObject* movedGenerated =
      appState.facade.findObject(generated.objectId);
  const cr::CreativePatternRecipe* movedRecipe =
      cr::findCreativePatternRecipe(
          appState.facade.document().patternRecipeStore(), recipe.recipeId);
  bool ok = expect(committed.accepted && committed.changed &&
                       committed.patternReceipt.accepted &&
                       appState.facade.document().revision() ==
                           revisionBefore + 1U &&
                       cr::creativeUndoDepth(appState.history) == 1U,
                   "recipe transform publishes once and records one undo") &&
            expect(movedSource != nullptr && movedGenerated != nullptr &&
                       movedRecipe != nullptr &&
                       cr::creativeVec3ExactlyEqual(
                           movedSource->transform.position, {3.0, 0.0, 2.0}) &&
                       cr::creativeVec3ExactlyEqual(
                           movedGenerated->transform.position,
                           {5.0, 0.0, 2.0}) &&
                       movedRecipe->sourceObjectIds ==
                           std::vector{source.objectId} &&
                       movedRecipe->generatedObjectIds ==
                           std::vector{generated.objectId},
                   "source generated output and durable membership stay aligned");
  const cr::CreativeHistoryApplyReceipt undo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  movedSource = appState.facade.findObject(source.objectId);
  movedGenerated = appState.facade.findObject(generated.objectId);
  return expect(undo.accepted && movedSource != nullptr &&
                    movedGenerated != nullptr &&
                    cr::creativeVec3ExactlyEqual(
                        movedSource->transform.position, {}) &&
                    cr::creativeVec3ExactlyEqual(
                        movedGenerated->transform.position,
                        {2.0, 0.0, 0.0}),
                "one undo restores the complete recipe relationship") &&
         ok;
}

bool gizmoHitTestingUsesTheVisibleContentViewport() {
  app::CreativeEditorSelectionFrame selection;
  selection.hasSelection = true;
  selection.boxMin = {-0.5F, -0.5F, -0.5F};
  selection.boxMax = {0.5F, 0.5F, 0.5F};
  app::CreativeMovingPlatformPathEditState pathEdit;
  iggy3d::RenderCameraFrame camera;
  camera.clipFromWorld = iggy3d::identityMat4();
  const iggy3d::RenderContentViewport content{100, 50, 400U, 300U};
  const app::CreativeEditorGizmoFrame gizmo =
      app::buildCreativeEditorGizmoFrame(
          selection, pathEdit, camera, 800U, 600U, 0.5F, content);

  const app::GizmoAxisPickResult x =
      app::pickCreativeEditorGizmoAxisAtPixel(gizmo.axisHandles, 360.0F,
                                               200.0F, 4.0F);
  const app::GizmoAxisPickResult y =
      app::pickCreativeEditorGizmoAxisAtPixel(gizmo.axisHandles, 300.0F,
                                               160.0F, 4.0F);
  const app::GizmoAxisPickResult outside =
      app::pickCreativeEditorGizmoAxisAtPixel(gizmo.axisHandles, 500.0F,
                                               350.0F, 4.0F);
  return expect(gizmo.axisHandles[0].valid && gizmo.axisHandles[1].valid &&
                    !gizmo.axisHandles[2].valid,
                "projected gizmo rejects a depth-collapsed axis") &&
         expect(x.hit && x.axis == app::GizmoAxis::X && y.hit &&
                    y.axis == app::GizmoAxis::Y,
                "gizmo shafts hit in offset content-viewport coordinates") &&
         expect(!outside.hit && outside.axis == app::GizmoAxis::None,
                "gizmo hit padding stays bounded to visible shafts");
}

bool pointerAxisDragUsesTheSharedTransformTransaction() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Transform Pointer", 804U),
              "pointer transform document installed")) {
    return false;
  }
  const cr::CreativeObjectId objectId = createAndSelectRoom(appState);
  if (!expect(objectId != cr::kInvalidObjectId,
              "pointer transform source selected")) {
    return false;
  }

  app::CreativeEditorState editor;
  if (!expect(app::beginCreativeEditorSelectionTransformPreview(
                  appState, editor.transform, "test_pointer_drag_begin",
                  app::CreativeEditorTransformAnchorPolicy::FixedSource),
              "pointer transform uses the canonical session")) {
    return false;
  }
  const cr::CreativeVec3 source = editor.transform.request.sourceAnchor;
  const cr::CreativeVec3 initialRayOrigin{source.x + 1.0, source.y + 2.0,
                                          source.z};
  const cr::CreativeVec3 movedRayOrigin{source.x + 3.0, source.y + 2.0,
                                        source.z};
  const cr::CreativeVec3 rayDirection{0.0, -1.0, 0.0};
  const app::CreativeEditorTransformAxisRaySample parallel =
      app::sampleCreativeEditorTransformAxisRay(
          initialRayOrigin, {1.0, 0.0, 0.0}, source, {1.0, 0.0, 0.0});
  bool ok = expect(!parallel.valid,
                   "parallel ray-axis drag fails closed") &&
            expect(app::beginCreativeEditorAxisTransformPointerGesture(
                       appState, editor.transform,
                       cr::CreativeSelectionPlacementAxis::X, source,
                       {1.0, 0.0, 0.0}, initialRayOrigin, rayDirection),
                   "X handle begins a constrained pointer gesture") &&
            expect(app::updateCreativeEditorTransformPointerGesture(
                       appState, editor.transform, false, {}, movedRayOrigin,
                       rayDirection) &&
                       editor.transform.targetPositionable &&
                       editor.transform.request.targetAnchor.x ==
                           source.x + 2.0 &&
                       editor.transform.request.targetAnchor.y == source.y &&
                       editor.transform.request.targetAnchor.z == source.z,
                   "axis drag computes a no-jump snapped displacement") &&
            expect(app::finishCreativeEditorTransformPointerGesture(
                       editor.transform, "test_pointer_drag_release") &&
                       editor.transform.commitRequested,
                   "release requests the shared transform commit");
  const app::CreativeEditorTransformCommitReceipt committed =
      app::processCreativeEditorSelectionTransformPreview(
          appState, editor.transform, false, {}, false,
          "test_pointer_drag_commit", 1.0);
  const cr::CreativeObject* moved = appState.facade.findObject(objectId);
  ok = expect(committed.accepted && committed.changed && moved != nullptr &&
                  moved->bounds.min.x == 2.0 &&
                  cr::creativeUndoDepth(appState.history) == 1U,
              "pointer drag commits once through SelectionPlacement") &&
       ok;

  const cr::CreativeHistoryApplyReceipt undone = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  if (!expect(undone.accepted && undone.changed,
              "pointer transform undo restores the source")) {
    return false;
  }
  const std::array reselected{objectId};
  if (!expect(appState.facade.selectTargets(reselected, objectId).accepted,
              "pointer source is reselected after history restore")) {
    return false;
  }
  if (!expect(app::beginCreativeEditorSelectionTransformPreview(
                  appState, editor.transform, "test_empty_pointer_begin",
                  app::CreativeEditorTransformAnchorPolicy::FixedSource) &&
                  app::beginCreativeEditorAxisTransformPointerGesture(
                      appState, editor.transform,
                      cr::CreativeSelectionPlacementAxis::X, source,
                      {1.0, 0.0, 0.0}, initialRayOrigin, rayDirection),
              "empty pointer gesture begins")) {
    return false;
  }
  const std::uint64_t revisionBeforeEmpty =
      appState.facade.document().revision();
  const std::uint64_t undoDepthBeforeEmpty =
      cr::creativeUndoDepth(appState.history);
  return expect(app::finishCreativeEditorTransformPointerGesture(
                    editor.transform, "test_empty_pointer_release") &&
                    !editor.transform.active,
                "empty pointer release cancels the preview") &&
         expect(appState.facade.document().revision() == revisionBeforeEmpty &&
                    cr::creativeUndoDepth(appState.history) ==
                        undoDepthBeforeEmpty,
                "empty pointer gesture records no mutation or history") &&
         ok;
}

bool objectMoveInputRoutesHandlesAndBodiesIntoTransform() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Transform Input Route", 805U),
              "transform input document installed")) {
    return false;
  }
  const cr::CreativeObjectId objectId = createAndSelectRoom(appState);
  if (!expect(objectId != cr::kInvalidObjectId,
              "transform input source selected")) {
    return false;
  }

  app::CreativeEditorState editor;
  editor.interaction.target.objectHit = true;
  editor.interaction.target.objectId = objectId;
  editor.interaction.target.grid.valid = true;
  editor.interaction.target.grid.placementAnchor = {0.5, 0.0, 0.5};
  editor.interaction.target.ray =
      {true, {0.0F, 2.0F, 0.0F}, {0.0F, -1.0F, 0.0F}};
  cr::CreativeWorldActionFrame press;
  const std::size_t primary =
      static_cast<std::size_t>(cr::CreativeWorldActionId::Primary);
  press.down[primary] = true;
  press.pressed[primary] = true;
  iggy3d::RenderCameraFrame camera;
  app::CreativeEditorPickFrame pickFrame;
  const iggy3d::RenderContentViewport viewport{0, 0, 800U, 600U};
  app::CreativeEditorGizmoFrame gizmo;
  gizmo.center = {};
  gizmo.shafts[0] = {app::GizmoAxis::X, {1.0F, 0.0F, 0.0F}, {}};
  gizmo.axisHandles[0] =
      {app::GizmoAxis::X, 320.0F, 300.0F, 460.0F, 300.0F, true};

  const std::uint64_t revisionBefore = appState.facade.document().revision();
  app::processCreativeEditorMoveInteraction(
      {appState, editor, press, cr::kCreativeInputModifierNone, camera,
       pickFrame, viewport, 0U, false, nullptr, nullptr, &gizmo});
  bool ok = expect(editor.transform.active &&
                       editor.transform.pointerGesture.kind ==
                           app::CreativeEditorTransformPointerGestureKind::Axis &&
                       editor.transform.pointerGesture.axis ==
                           cr::CreativeSelectionPlacementAxis::X,
                   "visible X handle starts the shared constrained session") &&
            expect(appState.facade.document().revision() == revisionBefore &&
                       cr::creativeUndoDepth(appState.history) == 0U,
                   "handle press is preview-only");
  static_cast<void>(app::cancelCreativeEditorSelectionTransformPreview(
      editor.transform, "test_axis_input_cancel"));

  app::processCreativeEditorMoveInteraction(
      {appState, editor, press, cr::kCreativeInputModifierNone, camera,
       pickFrame, viewport});
  ok = expect(editor.transform.active &&
                  editor.transform.pointerGesture.kind ==
                      app::CreativeEditorTransformPointerGestureKind::Free,
              "object body starts the same free transform session") &&
       expect(appState.facade.document().revision() == revisionBefore &&
                  cr::creativeUndoDepth(appState.history) == 0U,
              "object press remains preview-only in the shared session") &&
       ok;
  static_cast<void>(app::cancelCreativeEditorSelectionTransformPreview(
      editor.transform, "test_free_input_cancel"));
  return ok;
}

bool objectMovePreservesSelectedHierarchyRoot() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Transform Group Route", 806U),
              "transform group document installed")) {
    return false;
  }
  cr::CreativeDocumentCreateRequest create;
  create.kind = cr::CreativeObjectKind::Crate;
  create.name = "Group child A";
  create.bounds = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}};
  create.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt first =
      appState.facade.createDocumentObject(create);
  create.name = "Group child B";
  create.transform.position = {2.0, 0.0, 0.0};
  create.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt second =
      appState.facade.createDocumentObject(create);
  const std::array children{first.objectId, second.objectId};
  if (!expect(first.accepted && second.accepted &&
                  appState.facade
                      .selectTargets(children, first.objectId)
                      .accepted,
              "transform group children selected")) {
    return false;
  }
  const cr::CreativeGroupCommandReceipt grouped =
      appState.facade.groupSelectedObjects();
  if (!expect(grouped.accepted && grouped.changed,
              "transform input group created")) {
    return false;
  }

  app::CreativeEditorState editor;
  editor.interaction.target.objectHit = true;
  editor.interaction.target.objectId = first.objectId;
  editor.interaction.target.grid.valid = true;
  editor.interaction.target.grid.placementAnchor = {0.5, 0.0, 0.5};
  editor.interaction.target.ray =
      {true, {0.0F, 2.0F, 0.0F}, {0.0F, -1.0F, 0.0F}};
  cr::CreativeWorldActionFrame press;
  const std::size_t primary =
      static_cast<std::size_t>(cr::CreativeWorldActionId::Primary);
  press.down[primary] = true;
  press.pressed[primary] = true;
  iggy3d::RenderCameraFrame camera;
  app::CreativeEditorPickFrame pickFrame;
  const iggy3d::RenderContentViewport viewport{0, 0, 800U, 600U};
  app::processCreativeEditorMoveInteraction(
      {appState, editor, press, cr::kCreativeInputModifierNone, camera,
       pickFrame, viewport});

  const cr::CreativeSelectionState& selection =
      appState.facade.selectionState();
  const bool ok =
      expect(editor.transform.active &&
                 editor.transform.pointerGesture.kind ==
                     app::CreativeEditorTransformPointerGestureKind::Free,
             "group child hit starts the shared transform session") &&
      expect(selection.selectedTarget.value ==
                 static_cast<cr::Id>(grouped.groupObjectId) &&
                 cr::selectedTargetList(selection).size() == 1U,
             "selected hierarchy root remains the transform owner") &&
      expect(editor.transform.plan.accepted &&
                 editor.transform.plan.objects.size() == 3U,
             "group transform plan retains root and both children");
  static_cast<void>(app::cancelCreativeEditorSelectionTransformPreview(
      editor.transform, "test_group_input_cancel"));
  return ok;
}

bool transformClearanceBlocksMoveAndZeroOffsetCopy() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Transform Clearance", 807U),
              "transform clearance document installed")) {
    return false;
  }
  const cr::CreativeObjectId sourceId = createAndSelectRoom(
      appState, {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}},
      cr::CreativeObjectKind::Crate);
  cr::CreativeDocumentCreateRequest create;
  create.kind = cr::CreativeObjectKind::Crate;
  create.name = "Transform blocker";
  create.bounds = {{3.0, 0.0, 0.0}, {4.0, 1.0, 1.0}};
  create.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt blocker =
      appState.facade.createDocumentObject(create);
  app::CreativePlacementClearanceCache clearanceCache;
  if (!expect(sourceId != cr::kInvalidObjectId && blocker.accepted &&
                  app::refreshCreativePlacementClearanceCache(
                      clearanceCache, appState.facade.document()),
              "transform clearance fixture indexed")) {
    return false;
  }

  app::CreativeEditorState editor;
  if (!expect(app::beginCreativeEditorSelectionTransformPreview(
                  appState, editor.transform, "test_clearance_begin"),
              "transform clearance session begins")) {
    return false;
  }
  const cr::CreativeVec3 sourceAnchor = editor.transform.request.sourceAnchor;
  const cr::CreativeVec3 blockedAnchor{sourceAnchor.x + 3.0, sourceAnchor.y,
                                        sourceAnchor.z};
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  const app::CreativeEditorTransformCommitReceipt blocked =
      app::processCreativeEditorSelectionTransformPreview(
          appState, editor.transform, true, blockedAnchor, true,
          "test_clearance_blocked", 1.0, nullptr, &clearanceCache);
  bool ok =
      expect(editor.transform.clearance.evaluated &&
                 !editor.transform.clearance.allowed &&
                 editor.transform.clearance.status ==
                     cr::CreativePlacementClearanceStatus::AuthoredObjectBlocked &&
                 editor.transform.clearance.blockingObjectId ==
                     blocker.objectId &&
                 editor.transform.clearanceCandidateObjectId == sourceId,
             "move candidate reports its exact blocking object") &&
      expect(blocked.requested && !blocked.accepted && !blocked.changed &&
                 appState.facade.document().revision() == revisionBefore &&
                 cr::creativeUndoDepth(appState.history) == 0U,
             "blocked transform cannot mutate or record history");

  ok = expect(app::setCreativeEditorTransformPlacementMode(
                  appState, editor.transform,
                  cr::CreativeSelectionPlacementMode::Copy),
              "transform clearance switches to copy") &&
       ok;
  static_cast<void>(app::processCreativeEditorSelectionTransformPreview(
      appState, editor.transform, true, sourceAnchor, false,
      "test_clearance_copy_overlap", 1.0, nullptr, &clearanceCache));
  ok = expect(editor.transform.clearance.evaluated &&
                  !editor.transform.clearance.allowed &&
                  editor.transform.clearance.status ==
                      cr::CreativePlacementClearanceStatus::AuthoredObjectBlocked &&
                  editor.transform.clearance.blockingObjectId == sourceId,
              "zero-offset copy treats the source as an obstacle") &&
       ok;

  ok = expect(app::setCreativeEditorTransformPlacementMode(
                  appState, editor.transform,
                  cr::CreativeSelectionPlacementMode::Move),
              "transform clearance returns to move") &&
       ok;
  const cr::CreativeVec3 clearAnchor{sourceAnchor.x + 5.0, sourceAnchor.y,
                                      sourceAnchor.z};
  const app::CreativeEditorTransformCommitReceipt moved =
      app::processCreativeEditorSelectionTransformPreview(
          appState, editor.transform, true, clearAnchor, true,
          "test_clearance_move", 1.0, nullptr, &clearanceCache);
  const cr::CreativeObject* source = appState.facade.findObject(sourceId);
  const cr::CreativeTransformedBounds movedBounds =
      source == nullptr ? cr::CreativeTransformedBounds{}
                        : cr::resolveCreativeObjectBounds(*source);
  return expect(moved.accepted && moved.changed &&
                    editor.transform.clearance.allowed && movedBounds.valid &&
                    movedBounds.worldBounds.min.x == 5.0 &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "clear move ignores its original source and commits once") &&
         ok;
}

bool lockedSelectionTransformFailsBeforePreview() {
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
  const std::uint64_t revisionBefore = appState.facade.document().revision();
  const bool began = app::beginCreativeEditorSelectionTransformPreview(
      appState, editor.transform, "test_locked_begin");
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> lines;
  const std::size_t edgeCount =
      app::appendCreativeEditorSelectionTransformPreview(
          editor.transform, 0.05F, lines);

  return expect(!began && !editor.transform.active &&
                    editor.transform.preflight.requested &&
                    !editor.transform.preflight.accepted &&
                    editor.transform.preflight.reasonCode ==
                        "editor_transform_object_locked",
                "locked selection rejects before opening a transform session") &&
         expect(edgeCount == 0U && lines.empty(),
                "locked selection never advertises a mutable preview") &&
         expect(appState.facade.document().revision() == revisionBefore &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "locked preflight cannot mutate or record history");
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

bool unsupportedGeneratedTransformUsesSemanticPolicy() {
  cr::CreativeAppState appState;
  if (!expect(installDocument(appState, "Unsupported Generated Transform",
                              805U),
              "unsupported generated transform document installed")) {
    return false;
  }

  app::CreativeEditorState editor;
  editor.worldLayout.source.stableKey = "unsupported_transform_layout";
  cr::CreativeWorldLayoutObject source;
  source.kind = cr::CreativeObjectKind::Crate;
  source.stableKey = "generated_crate";
  source.name = "Generated crate";
  source.boundsCells = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}};
  editor.worldLayout.source.objects.push_back(source);

  cr::CreativeDocumentCreateRequest create;
  create.kind = cr::CreativeObjectKind::Crate;
  create.name = "Generated crate";
  create.tags = {
      cr::creativeWorldLayoutTag(editor.worldLayout.source.stableKey),
      cr::creativeWorldLayoutProvenanceTag(
          editor.worldLayout.source, cr::CreativeWorldLayoutTable::Object,
          0U)};
  const cr::CreativeDocumentCreateReceipt generated =
      appState.facade.createDocumentObject(create);
  const std::array selected{generated.objectId};
  if (!expect(generated.accepted &&
                  appState.facade
                      .selectTargets(selected, generated.objectId)
                      .accepted,
              "unsupported generated object selected")) {
    return false;
  }

  const cr::CreativeSemanticSelectionSetResolution semanticSelection =
      cr::resolveCreativeSemanticSelectionSet(
          appState.facade.document(), selected, generated.objectId,
          &editor.worldLayout.source);
  const cr::CreativeSemanticObjectActionPolicy expectedPolicy =
      cr::resolveCreativeSemanticObjectAction(
          semanticSelection,
          cr::CreativeSemanticObjectAction::TransformSelection);
  cr::CreativeTransformCommandRequest directTransform;
  directTransform.kind = cr::CreativeTransformCommandKind::Translate;
  directTransform.translation = {1.0, 0.0, 0.0};
  const std::uint64_t revisionBefore =
      appState.facade.document().revision();
  const app::CreativeEditorSemanticEditReceipt direct =
      app::transformCreativeEditorSelectionWithUndo(
          appState, appState.history, directTransform,
          "test_unsupported_generated_direct_transform",
          &editor.worldLayout);
  const bool preview = app::beginCreativeEditorSelectionTransformPreview(
      appState, editor.transform,
      "test_unsupported_generated_transform",
      app::CreativeEditorTransformAnchorPolicy::FixedSource,
      &editor.worldLayout);

  return expect(semanticSelection.primaryOwner ==
                        cr::CreativeSemanticSelectionOwner::
                            WorldLayoutSource &&
                    !expectedPolicy.allowed,
                "unsupported generated transform resolves a closed policy") &&
         expect(!direct.accepted && !direct.changed &&
                    direct.reasonCode == expectedPolicy.reasonCode,
                "direct transform reports the shared semantic rejection") &&
         expect(!preview && !editor.transform.active &&
                    editor.transform.preflight.blockedOwner ==
                        cr::CreativeSemanticSelectionOwner::
                            WorldLayoutSource &&
                    editor.transform.preflight.reasonCode ==
                        expectedPolicy.reasonCode,
                "interactive transform reports the shared semantic rejection") &&
         expect(appState.facade.document().revision() == revisionBefore &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "rejected generated transforms cannot mutate or record history");
}

}  // namespace

int main() {
  const bool ok = requestMappingIsExplicit() &&
                  previewIsTransientAndOrdinalDerived() &&
                  previewReportsCollisionAndFinalBounds() &&
                  hiddenHierarchyObstaclesMatchColdAndWarmClearance() &&
                  liveOptionsDraftPreviewIsBoundedAndRevisionFree() &&
                  commitRecordsOneUndoStep() &&
                  rejectedCommitDoesNotRecordHistory() &&
                  radialPreviewUsesCrosshairPivotWithoutMutation() &&
                  radialCommitRecordsOneUndoStepAndRequiresPivot() &&
                  editableArrayReopensUpdatesAndDetachesWithHistory() &&
                  transformCopyPreviewIsTransientAndConfirmable() &&
                  selectionTransformMoveUsesControlsAndOneUndo() &&
                  selectionTransformScalePreviewMatchesCommitAndOneUndo() &&
                  selectionTransformUsesAllAxesAndNonUniformScale() &&
                  exactTransformSupportsScaledDuplicateAndFixedNumericTarget() &&
                  transformCoordinateSpaceUsesTheActiveObjectBasis() &&
                  generatedBuildingTransformRoutesThroughWorldLayoutSource() &&
                  generatedPatternOutputUsesSourceLevelTransform() &&
                  gizmoHitTestingUsesTheVisibleContentViewport() &&
                  pointerAxisDragUsesTheSharedTransformTransaction() &&
                  objectMoveInputRoutesHandlesAndBodiesIntoTransform() &&
                  objectMovePreservesSelectedHierarchyRoot() &&
                  transformClearanceBlocksMoveAndZeroOffsetCopy() &&
                  precisionTransformConstrainsNudgesAndCommitsOnce() &&
                  lockedSelectionTransformFailsBeforePreview() &&
                  largeTransformPreviewUsesOneAggregateBox() &&
                  unsupportedGeneratedTransformUsesSemanticPolicy();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
