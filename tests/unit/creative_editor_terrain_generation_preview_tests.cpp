#include "EditorPreviewFrame.hpp"
#include "EditorDesktopCommands.hpp"
#include "EditorEdits.hpp"
#include "EditorState.hpp"
#include "EditorTerrainGeneration.hpp"

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/recipes/TerrainGeneration.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace {
namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeTerrainGenerationResult generationFor(std::uint64_t seed) {
  cr::CreativeTerrainGeneratorRecipe recipe;
  recipe.seed = seed;
  recipe.bounds = {{-1, -1}, 3U, 3U};
  recipe.baseHeightCells = 10U;
  recipe.reliefCells = 6U;
  recipe.horizontalScaleCells = 3.0;
  recipe.octaveCount = 5U;
  recipe.persistence = 0.5;
  recipe.lacunarity = 2.0;
  recipe.slopeDamping = 0.8;
  return cr::buildCreativeTerrainGenerationPlan(recipe);
}

bool surfaceMatchesHeightField(
    const cr::CreativeTerrainSurfacePlan& surface,
    const cr::CreativeTerrainHeightField& field) {
  if (!surface.accepted || !field.validateInvariants()) {
    return false;
  }
  const cr::CreativeTerrainHeightFieldBounds bounds = field.bounds();
  std::size_t heightIndex = 0U;
  for (std::uint16_t z = 0U; z < bounds.depthCells; ++z) {
    for (std::uint16_t x = 0U; x < bounds.widthCells; ++x) {
      const cr::CreativeTerrainCoord2 coord{
          bounds.minimum.x + static_cast<std::int32_t>(x),
          bounds.minimum.z + static_cast<std::int32_t>(z)};
      const auto found = std::lower_bound(
          surface.columns.begin(), surface.columns.end(), coord,
          [](const cr::CreativeTerrainColumn& column,
             cr::CreativeTerrainCoord2 candidate) {
            return column.coord.z != candidate.z
                       ? column.coord.z < candidate.z
                       : column.coord.x < candidate.x;
          });
      const bool hasColumn =
          found != surface.columns.end() && found->coord == coord;
      const std::uint16_t expectedHeight = field.heights()[heightIndex++];
      if ((expectedHeight == cr::kCreativeTerrainEmptyHeightCells &&
           hasColumn) ||
          (expectedHeight != cr::kCreativeTerrainEmptyHeightCells &&
           (!hasColumn || found->heightCells != expectedHeight))) {
        return false;
      }
    }
  }
  return true;
}

bool generatedPreviewReplacesTerrainAndCachesByHeightHash() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Generated Terrain Preview");
  static_cast<void>(document.assignId(901U));
  constexpr std::array controls{
      cr::CreativeTerrainControlEdit{
          cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 4U, 1U}},
      cr::CreativeTerrainControlEdit{
          cr::CreativeTerrainEditKind::Upsert, {{10, 0}, 4U, 1U}},
  };
  const cr::CreativeTerrainMutationReceipt terrainApplied =
      document.applyTerrainControlEdits(controls);
  const cr::CreativeTerrainGenerationResult firstGeneration =
      generationFor(1001U);
  const cr::CreativeTerrainHeightFieldReplaceReceipt authoredApplied =
      document.replaceTerrainHeightField(
          firstGeneration.plan.heightField.bounds(),
          firstGeneration.plan.heightField.heights());
  const std::uint64_t documentRevision = document.revision();

  app::CreativeEditorSceneCache sourceCache;
  const bool sourceBuilt =
      app::refreshCreativeEditorSceneCache(sourceCache, document);
  const bool sourceContainsAuthoredAndLegacy =
      sourceCache.terrainCollisionPatches.size() == 14U &&
      std::any_of(sourceCache.terrainCollisionPatches.begin(),
                  sourceCache.terrainCollisionPatches.end(),
                  [](const cr::CreativeTerrainSurfacePatch& patch) {
                    return patch.coord == cr::CreativeTerrainCoord2{-1, -1};
                  }) &&
      std::any_of(sourceCache.terrainCollisionPatches.begin(),
                  sourceCache.terrainCollisionPatches.end(),
                  [](const cr::CreativeTerrainSurfacePatch& patch) {
                    return patch.coord == cr::CreativeTerrainCoord2{10, 0};
                  });
  app::CreativeEditorGeneratedTerrainPreviewCache previewCache;
  const bool firstRefresh =
      app::refreshCreativeEditorGeneratedTerrainPreview(
          previewCache, sourceCache, document,
          firstGeneration.plan.heightField,
          firstGeneration.receipt.heightHash);

  bool reused = true;
  for (std::uint32_t frame = 0U; frame < 300U; ++frame) {
    reused = !app::refreshCreativeEditorGeneratedTerrainPreview(
                 previewCache, sourceCache, document,
                 firstGeneration.plan.heightField,
                 firstGeneration.receipt.heightHash) &&
             reused;
  }

  app::invalidateCreativeEditorSceneCache(sourceCache);
  const bool sourceRebuilt =
      app::refreshCreativeEditorSceneCache(sourceCache, document);
  const bool sourceRefresh =
      app::refreshCreativeEditorGeneratedTerrainPreview(
          previewCache, sourceCache, document,
          firstGeneration.plan.heightField,
          firstGeneration.receipt.heightHash);

  const cr::CreativeTerrainGenerationResult secondGeneration =
      generationFor(2002U);
  const bool secondRefresh =
      app::refreshCreativeEditorGeneratedTerrainPreview(
          previewCache, sourceCache, document,
          secondGeneration.plan.heightField,
          secondGeneration.receipt.heightHash);
  const bool containsOutsideSource = std::any_of(
      previewCache.composedSurface.columns.begin(),
      previewCache.composedSurface.columns.end(),
      [](const cr::CreativeTerrainColumn& column) {
        return column.coord == cr::CreativeTerrainCoord2{10, 0} &&
               column.heightCells == 4U;
      });

  const bool generatedRegionMatches = surfaceMatchesHeightField(
      previewCache.composedSurface, secondGeneration.plan.heightField);

  cr::CreativeTerrainHeightField invalidCandidate;
  const bool invalidRefresh =
      app::refreshCreativeEditorGeneratedTerrainPreview(
          previewCache, sourceCache, document, invalidCandidate, 0U);

  return expect(terrainApplied.accepted && firstGeneration.receipt.accepted &&
                    authoredApplied.accepted && authoredApplied.changed &&
                    sourceBuilt && firstRefresh,
                "valid generation builds a transient preview scene") &&
         expect(sourceContainsAuthoredAndLegacy &&
                    sourceCache.terrainHeightRevision ==
                        document.terrainHeightField().revision() &&
                    sourceCache.terrainHeightCellCount == 9U,
                "normal scene cache composes authored and legacy terrain") &&
         expect(previewCache.refreshCount == 3U && sourceRebuilt &&
                    sourceRefresh && secondRefresh && reused,
                "idle frames reuse output while source and height changes rebuild") &&
         expect(firstGeneration.receipt.heightHash !=
                    secondGeneration.receipt.heightHash,
                "test seeds produce distinct generated terrain") &&
         expect(containsOutsideSource && generatedRegionMatches,
                "generated region replaces source while outside terrain remains") &&
         expect(previewCache.composedSurface.columns.empty() &&
                    previewCache.terrainCollisionPatches.empty() &&
                    previewCache.terrainSurfacePatches.empty() &&
                    !previewCache.valid && !invalidRefresh,
                "invalid generation clears transient output atomically") &&
         expect(document.revision() == documentRevision &&
                    document.terrainField().controlCount() == 2U &&
                    document.terrainHeightField().bounds() ==
                        firstGeneration.plan.heightField.bounds(),
                "preview never mutates document or legacy terrain controls");
}

bool authoredEmptyRegionSuppressesLegacyTerrain() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Authored Empty Terrain");
  static_cast<void>(document.assignId(902U));
  const cr::CreativeTerrainControlEdit control{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 4U, 1U}};
  const cr::CreativeTerrainMutationReceipt terrainApplied =
      document.applyTerrainControlEdits(std::span{&control, 1U});
  constexpr cr::CreativeTerrainHeightFieldBounds bounds{
      {-1, -1}, 3U, 3U};
  constexpr std::array<std::uint16_t, 9U> emptyHeights{};
  const cr::CreativeTerrainHeightFieldReplaceReceipt authoredApplied =
      document.replaceTerrainHeightField(bounds, emptyHeights);

  app::CreativeEditorSceneCache cache;
  const bool refreshed =
      app::refreshCreativeEditorSceneCache(cache, document);
  const bool roomHasTerrain = std::any_of(
      cache.preview.roomBake.room.staticMeshes.begin(),
      cache.preview.roomBake.room.staticMeshes.end(),
      [](const iggy3d::RoomStaticMeshAsset& mesh) {
        return mesh.role == "terrain";
      });

  return expect(terrainApplied.accepted && authoredApplied.accepted &&
                    authoredApplied.changed && refreshed,
                "empty authored terrain fixture refreshes") &&
         expect(cache.terrainCuboids.empty() &&
                    cache.terrainCollisionPatches.empty() &&
                    cache.terrainSurfacePatches.empty(),
                "authored zero cells erase legacy terrain in their bounds") &&
         expect(!roomHasTerrain &&
                    !cache.preview.roomBake.receipt
                         .usedSmoothTerrainCollision &&
                    cache.preview.roomBake.receipt
                            .bakedTerrainSurfacePatchCount == 0U,
                "precomputed empty terrain cannot resurrect source collision");
}

void installDocument(cr::CreativeAppState& appState,
                     cr::CreativeDocumentId id) {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Terrain Generator Workflow");
  static_cast<void>(document.assignId(id));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
}

bool terrainGenerationWorkflowIsAtomicAndUndoable() {
  cr::CreativeAppState appState;
  installDocument(appState, 903U);
  app::CreativeEditorTerrainGenerationState state;
  state.recipe.bounds = {{-4, -3}, 8U, 6U};
  state.recipe.seed = 77U;
  const std::uint64_t revisionBefore = appState.facade.document().revision();

  const app::CreativeEditorTerrainGenerationPreviewReceipt preview =
      app::previewCreativeEditorTerrainGeneration(
          state, appState.facade.document(), false);
  const std::uint64_t firstHash = state.generation.receipt.heightHash;
  const app::CreativeEditorTerrainGenerationPreviewReceipt regenerated =
      app::previewCreativeEditorTerrainGeneration(
          state, appState.facade.document(), true);
  const std::uint64_t secondHash = state.generation.receipt.heightHash;
  const cr::CreativeTerrainHeightFieldBounds expectedBounds =
      state.operationPreview.heightField.bounds();
  const std::vector<std::uint16_t> expectedHeights(
      state.operationPreview.heightField.heights().begin(),
      state.operationPreview.heightField.heights().end());
  app::CreativeEditorSceneCache sourceCache;
  const bool sourceBuilt = app::refreshCreativeEditorSceneCache(
      sourceCache, appState.facade.document());
  app::CreativeEditorGeneratedTerrainPreviewCache renderedPreview;
  const bool candidateRendered =
      app::refreshCreativeEditorGeneratedTerrainPreview(
          renderedPreview, sourceCache, appState.facade.document(),
          state.operationPreview.heightField,
          state.operationPreview.receipt.replay.heightHash);
  const bool renderedCandidateMatches = surfaceMatchesHeightField(
      renderedPreview.composedSurface, state.operationPreview.heightField);

  const app::CreativeEditorTerrainGenerationApplyReceipt applied =
      app::applyCreativeEditorTerrainGeneration(appState, state);
  const std::uint64_t revisionAfterApply =
      appState.facade.document().revision();
  const bool appliedExactField =
      appState.facade.document().terrainHeightField().bounds() ==
          expectedBounds &&
      std::equal(
          expectedHeights.begin(), expectedHeights.end(),
          appState.facade.document().terrainHeightField().heights().begin(),
          appState.facade.document().terrainHeightField().heights().end());
  const std::uint64_t undoDepthAfterApply =
      cr::creativeUndoDepth(appState.history);
  const bool undone = app::undoLastEdit(appState, "terrain_generation_undo");

  return expect(preview.accepted && regenerated.accepted &&
                    preview.seed == 77U && regenerated.seed == 78U &&
                    firstHash != secondHash,
                "preview is deterministic and regenerate advances the seed") &&
         expect(revisionAfterApply != revisionBefore && applied.accepted &&
                    applied.changed && !state.previewActive &&
                    appliedExactField,
                "apply commits the exact active preview") &&
         expect(sourceBuilt && candidateRendered && renderedCandidateMatches,
                "rendered preview consumes the exact composed candidate") &&
         expect(applied.operation.replay.outputCellCount ==
                        expectedHeights.size() &&
                    undoDepthAfterApply == 1U &&
                    cr::creativeUndoDepth(appState.history) == 0U && undone,
                "apply creates exactly one undo record") &&
         expect(appState.facade.document().terrainHeightField().cellCount() ==
                    0U &&
                    expectedBounds.widthCells == 8U &&
                    expectedBounds.depthCells == 6U,
                "undo restores the terrain field before generation");
}

bool terrainGenerationRejectsStalePreviewAndCancelDoesNotMutate() {
  cr::CreativeAppState appState;
  installDocument(appState, 904U);
  app::CreativeEditorTerrainGenerationState state;
  state.recipe.bounds = {{0, 0}, 4U, 4U};
  const app::CreativeEditorTerrainGenerationPreviewReceipt preview =
      app::previewCreativeEditorTerrainGeneration(
          state, appState.facade.document(), false);
  const std::uint64_t revisionBeforeMutation =
      appState.facade.document().revision();
  constexpr cr::CreativeTerrainControlEdit edit{
      cr::CreativeTerrainEditKind::Upsert, {{10, 10}, 4U, 1U}};
  const cr::CreativeTerrainMutationReceipt mutation =
      appState.facade.applyTerrainControlEdits(std::span{&edit, 1U});
  const app::CreativeEditorTerrainGenerationApplyReceipt staleApply =
      app::applyCreativeEditorTerrainGeneration(appState, state);
  const bool synchronized = app::synchronizeCreativeEditorTerrainGeneration(
      state, appState.facade.document());
  const std::uint64_t revisionBeforeCancel =
      appState.facade.document().revision();
  static_cast<void>(app::previewCreativeEditorTerrainGeneration(
      state, appState.facade.document(), false));
  const bool canceled = app::cancelCreativeEditorTerrainGeneration(state);

  return expect(preview.accepted && mutation.accepted &&
                    appState.facade.document().revision() >
                        revisionBeforeMutation,
                "fixture changes document truth after preview") &&
         expect(!staleApply.accepted && !staleApply.changed && synchronized &&
                    !state.previewActive,
                "stale preview cannot overwrite newer document truth") &&
         expect(canceled && !state.previewActive &&
                    appState.facade.document().revision() ==
                        revisionBeforeCancel &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "cancel clears only transient state");
}

bool desktopCommandsRouteTerrainPreviewAndApply() {
  cr::CreativeAppState appState;
  installDocument(appState, 905U);
  app::CreativeEditorState editor;
  editor.terrainGeneration.recipe.bounds = {{-2, -2}, 5U, 5U};
  app::CreativeDesktopCommandFrame frame;
  frame.push(app::CreativeDesktopCommandId::TerrainGenerationPreview);
  frame.push(app::CreativeDesktopCommandId::TerrainGenerationApply);
  const app::CreativeDesktopCommandResult result =
      app::dispatchCreativeDesktopCommands(
          frame, {appState, editor, {}, nullptr, nullptr, nullptr});

  return expect(result.accepted && result.changed && result.sceneChanged &&
                    result.lastCommand ==
                        app::CreativeDesktopCommandId::TerrainGenerationApply,
                "desktop semantic commands preview then apply") &&
         expect(appState.facade.document()
                            .terrainHeightField()
                            .bounds() == editor.terrainGeneration.recipe.bounds &&
                    cr::creativeUndoDepth(appState.history) == 1U,
                "desktop apply uses the shared workflow and one history entry");
}

bool sequentialGenerationPreservesEarlierRegion() {
  cr::CreativeAppState appState;
  installDocument(appState, 906U);
  app::CreativeEditorTerrainGenerationState state;
  state.compositionRecipe.featherCells = 0U;
  state.recipe.bounds = {{0, 0}, 3U, 3U};
  state.recipe.seed = 100U;
  const app::CreativeEditorTerrainGenerationPreviewReceipt firstPreview =
      app::previewCreativeEditorTerrainGeneration(
          state, appState.facade.document(), false);
  const app::CreativeEditorTerrainGenerationApplyReceipt firstApply =
      app::applyCreativeEditorTerrainGeneration(appState, state);
  const std::vector<std::uint16_t> firstHeights(
      appState.facade.document().terrainHeightField().heights().begin(),
      appState.facade.document().terrainHeightField().heights().end());

  static_cast<void>(app::beginNewCreativeEditorTerrainOperation(state));
  state.recipe.bounds = {{4, 0}, 3U, 3U};
  state.recipe.seed = 200U;
  const app::CreativeEditorTerrainGenerationPreviewReceipt secondPreview =
      app::previewCreativeEditorTerrainGeneration(
          state, appState.facade.document(), false);
  const app::CreativeEditorTerrainGenerationApplyReceipt secondApply =
      app::applyCreativeEditorTerrainGeneration(appState, state);
  const cr::CreativeTerrainHeightField& combined =
      appState.facade.document().terrainHeightField();
  const cr::CreativeTerrainHeightFieldBounds combinedBounds =
      combined.bounds();
  bool firstRegionPreserved = true;
  for (std::int32_t z = 0; z < 3; ++z) {
    for (std::int32_t x = 0; x < 3; ++x) {
      const std::size_t firstIndex = static_cast<std::size_t>(z * 3 + x);
      firstRegionPreserved =
          firstRegionPreserved && combined.heightAt({x, z}).has_value() &&
          *combined.heightAt({x, z}) == firstHeights[firstIndex];
    }
  }
  const bool secondRegionPresent = combined.heightAt({4, 0}).has_value();
  const bool undone = app::undoLastEdit(
      appState, "terrain_generation_second_region_undo");

  return expect(firstPreview.accepted && firstApply.accepted &&
                    secondPreview.accepted && secondApply.accepted,
                "two disjoint generator operations apply") &&
         expect(combinedBounds ==
                        cr::CreativeTerrainHeightFieldBounds{{0, 0}, 7U, 3U} &&
                    firstRegionPreserved && secondRegionPresent,
                "second operation preserves the first generated region") &&
         expect(cr::creativeUndoDepth(appState.history) == 1U && undone &&
                    appState.facade.document().terrainHeightField().bounds() ==
                        cr::CreativeTerrainHeightFieldBounds{{0, 0}, 3U, 3U},
                "one undo removes only the second composed operation");
}

bool desktopCommandsEditOrderedTerrainOperationsWithHistory() {
  cr::CreativeAppState appState;
  installDocument(appState, 907U);
  app::CreativeEditorState editor;
  editor.terrainGeneration.recipe.bounds = {{0, 0}, 2U, 2U};
  editor.terrainGeneration.recipe.seed = 11U;
  static_cast<void>(app::previewCreativeEditorTerrainGeneration(
      editor.terrainGeneration, appState.facade.document(), false));
  const app::CreativeEditorTerrainGenerationApplyReceipt first =
      app::applyCreativeEditorTerrainGeneration(appState,
                                                editor.terrainGeneration);
  const cr::CreativeTerrainOperationId firstId = first.operation.operationId;

  static_cast<void>(
      app::beginNewCreativeEditorTerrainOperation(editor.terrainGeneration));
  editor.terrainGeneration.recipe.bounds = {{3, 0}, 2U, 2U};
  editor.terrainGeneration.recipe.seed = 22U;
  static_cast<void>(app::previewCreativeEditorTerrainGeneration(
      editor.terrainGeneration, appState.facade.document(), false));
  const app::CreativeEditorTerrainGenerationApplyReceipt second =
      app::applyCreativeEditorTerrainGeneration(appState,
                                                editor.terrainGeneration);
  const cr::CreativeTerrainOperationId secondId = second.operation.operationId;

  const auto dispatch = [&](app::CreativeDesktopCommandId id,
                            app::CreativeDesktopTerrainOperationPayload payload) {
    app::CreativeDesktopCommandFrame frame;
    frame.push(id, payload);
    return app::dispatchCreativeDesktopCommands(
        frame, {appState, editor, {}, nullptr, nullptr, nullptr});
  };
  const app::CreativeDesktopCommandResult disabled = dispatch(
      app::CreativeDesktopCommandId::TerrainOperationSetEnabled,
      {firstId, false, 0U});
  const app::CreativeDesktopCommandResult moved = dispatch(
      app::CreativeDesktopCommandId::TerrainOperationMove,
      {secondId, true, 0U});
  const app::CreativeDesktopCommandResult duplicated = dispatch(
      app::CreativeDesktopCommandId::TerrainOperationDuplicate,
      {firstId, false, 0U});
  const cr::CreativeTerrainOperationId duplicateId =
      editor.terrainGeneration.editingOperationId;
  const app::CreativeDesktopCommandResult selected = dispatch(
      app::CreativeDesktopCommandId::TerrainOperationSelect,
      {secondId, true, 0U});
  const app::CreativeDesktopCommandResult removed = dispatch(
      app::CreativeDesktopCommandId::TerrainOperationDelete,
      {secondId, true, 0U});
  const bool undone = app::undoLastEdit(
      appState, "terrain_operation_delete_undo");

  const cr::CreativeTerrainOperationStack& stack =
      appState.facade.document().terrainOperationStack();
  return expect(first.accepted && second.accepted && firstId == 1U &&
                    secondId == 2U,
                "two explicit operation adds establish stable ids") &&
         expect(disabled.accepted && disabled.changed &&
                    moved.accepted && moved.changed &&
                    duplicated.accepted && duplicated.changed &&
                    duplicateId == 3U,
                "enable reorder and duplicate commands mutate through dispatcher") &&
         expect(selected.accepted &&
                    editor.terrainGeneration.recipe.seed == 22U,
                "select command loads the durable recipe into the editor") &&
         expect(removed.accepted && removed.changed && undone &&
                    stack.operations.size() == 3U &&
                    stack.operations.front().id == secondId &&
                    stack.operations[1U].id == firstId &&
                    stack.operations.back().id == duplicateId,
                "delete is one history edit and undo restores operation order") &&
         expect(cr::creativeUndoDepth(appState.history) == 5U,
                "each document operation records exactly one undo snapshot");
}

bool terrainOperationDraftSurvivesUnrelatedDocumentRevision() {
  cr::CreativeAppState appState;
  installDocument(appState, 908U);
  app::CreativeEditorTerrainGenerationState state;
  state.recipe.bounds = {{0, 0}, 3U, 3U};
  state.recipe.seed = 31U;
  static_cast<void>(app::previewCreativeEditorTerrainGeneration(
      state, appState.facade.document(), false));
  const app::CreativeEditorTerrainGenerationApplyReceipt applied =
      app::applyCreativeEditorTerrainGeneration(appState, state);
  const cr::CreativeTerrainGeneratorRecipe durableRecipe = state.recipe;

  state.recipe.seed = 99U;
  state.draftDirty = true;
  const cr::CreativeDocumentCreateReceipt created =
      appState.facade.createDocumentObject(cr::CreativeObjectKind::Crate);
  const bool dirtySynchronized =
      app::synchronizeCreativeEditorTerrainGeneration(
          state, appState.facade.document());
  const bool dirtyDraftPreserved =
      state.recipe.seed == 99U && state.draftDirty &&
      state.sourceDocumentRevision != appState.facade.document().revision();

  state.draftDirty = false;
  const bool cleanSynchronized =
      app::synchronizeCreativeEditorTerrainGeneration(
          state, appState.facade.document());

  return expect(applied.accepted && created.accepted,
                "terrain draft synchronization fixture applies") &&
         expect(!dirtySynchronized && dirtyDraftPreserved,
                "unrelated revision preserves an unsaved terrain draft") &&
         expect(cleanSynchronized && state.recipe == durableRecipe &&
                    state.sourceDocumentRevision ==
                        appState.facade.document().revision(),
                "clean selected draft reloads durable operation truth");
}

bool terrainOperationFootprintTracksSelectedAndPreviewMasks() {
  cr::CreativeAppState appState;
  installDocument(appState, 909U);
  app::CreativeEditorState editor;
  editor.terrainGeneration.recipe.bounds = {{-2, 3}, 6U, 4U};
  editor.terrainGeneration.recipe.baseHeightCells = 5U;
  editor.terrainGeneration.recipe.reliefCells = 2U;
  static_cast<void>(app::previewCreativeEditorTerrainGeneration(
      editor.terrainGeneration, appState.facade.document(), false));
  const app::CreativeEditorTerrainGenerationApplyReceipt applied =
      app::applyCreativeEditorTerrainGeneration(appState,
                                                editor.terrainGeneration);

  std::vector<iggy3d::RenderCreativeWireframeDebugLine> selectedLines;
  app::appendCreativeEditorTerrainOverlay(appState.facade.document(), editor,
                                          0.08F, selectedLines, false);

  editor.terrainGeneration.compositionRecipe.mask =
      cr::CreativeTerrainCompositionMask::Ellipse;
  const app::CreativeEditorTerrainGenerationPreviewReceipt preview =
      app::previewCreativeEditorTerrainGeneration(
          editor.terrainGeneration, appState.facade.document(), false);
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> previewLines;
  app::appendCreativeEditorTerrainOverlay(appState.facade.document(), editor,
                                          0.08F, previewLines, false);
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> captureLines;
  app::appendCreativeEditorTerrainOverlay(appState.facade.document(), editor,
                                          0.08F, captureLines, true);

  return expect(applied.accepted && selectedLines.size() == 4U,
                "selected rectangle operation emits four footprint edges") &&
         expect(preview.accepted && previewLines.size() == 32U,
                "ellipse preview emits one bounded segmented footprint") &&
         expect(captureLines.empty(),
                "capture mode hides terrain operation footprints");
}

}  // namespace

int main() {
  return generatedPreviewReplacesTerrainAndCachesByHeightHash() &&
                 authoredEmptyRegionSuppressesLegacyTerrain() &&
                 terrainGenerationWorkflowIsAtomicAndUndoable() &&
                 terrainGenerationRejectsStalePreviewAndCancelDoesNotMutate() &&
                 desktopCommandsRouteTerrainPreviewAndApply() &&
                 sequentialGenerationPreservesEarlierRegion() &&
                 desktopCommandsEditOrderedTerrainOperationsWithHistory() &&
                 terrainOperationDraftSurvivesUnrelatedDocumentRevision() &&
                 terrainOperationFootprintTracksSelectedAndPreviewMasks()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
