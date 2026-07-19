#include "EditorTerrainGeneration.hpp"

#include "EditorEdits.hpp"

#include <limits>
#include <string>
#include <utility>

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

cr::CreativeTerrainGeneratorRecipe
makeDefaultCreativeEditorTerrainGeneratorRecipe() noexcept {
  cr::CreativeTerrainGeneratorRecipe recipe;
  recipe.bounds = {{-32, -32}, 64U, 64U};
  return recipe;
}

void resetCreativeEditorTerrainGeneration(
    CreativeEditorTerrainGenerationState& state) noexcept {
  state = {};
}

bool creativeEditorTerrainGenerationPreviewMatches(
    const CreativeEditorTerrainGenerationState& state,
    const cr::CreativeDocument& document) noexcept {
  return state.previewActive &&
         state.sourceDocumentId == document.id() &&
         state.sourceDocumentRevision == document.revision() &&
         state.generation.receipt.accepted &&
         state.composition.receipt.accepted &&
         state.composition.receipt.status ==
             cr::CreativeTerrainCompositionStatus::Ready;
}

bool synchronizeCreativeEditorTerrainGeneration(
    CreativeEditorTerrainGenerationState& state,
    const cr::CreativeDocument& document) {
  if (!state.previewActive ||
      (state.sourceDocumentId == document.id() &&
       state.sourceDocumentRevision == document.revision())) {
    return false;
  }
  state.previewActive = false;
  state.generation = {};
  state.composition = {};
  state.sourceDocumentId = cr::kInvalidDocumentId;
  state.sourceDocumentRevision = 0U;
  state.statusMessage = "Preview canceled: document changed";
  return true;
}

CreativeEditorTerrainGenerationPreviewReceipt
previewCreativeEditorTerrainGeneration(
    CreativeEditorTerrainGenerationState& state,
    const cr::CreativeDocument& document,
    bool advanceSeed) {
  CreativeEditorTerrainGenerationPreviewReceipt receipt;
  receipt.requested = true;
  receipt.regenerated = advanceSeed;
  if (advanceSeed) {
    state.recipe.seed = state.recipe.seed ==
                                std::numeric_limits<std::uint64_t>::max()
                            ? 0U
                            : state.recipe.seed + 1U;
  }

  state.generation = cr::buildCreativeTerrainGenerationPlan(state.recipe);
  state.composition = {};
  if (state.generation.receipt.accepted) {
    const cr::CreativeTerrainSurfacePlan canonicalSource =
        cr::buildCreativeComposedTerrainSurfacePlan(
            document.terrainField(), document.terrainHeightField());
    state.composition = cr::composeCreativeTerrainGeneration(
        document.terrainHeightField(), canonicalSource, state.generation,
        state.compositionRecipe);
  }
  state.sourceDocumentId = document.id();
  state.sourceDocumentRevision = document.revision();
  ++state.generationCount;
  state.previewActive = true;

  receipt.accepted = state.composition.receipt.accepted;
  receipt.previewActive = true;
  receipt.seed = state.recipe.seed;
  receipt.generationCount = state.generationCount;
  receipt.reasonCode = !state.generation.receipt.accepted
                           ? state.generation.receipt.reasonCode
                           : state.composition.receipt.reasonCode;
  if (receipt.accepted) {
    state.statusMessage =
        advanceSeed ? "Terrain regenerated" : "Terrain preview ready";
  } else {
    state.statusMessage =
        "Preview failed: " + std::string(receipt.reasonCode);
  }
  return receipt;
}

bool cancelCreativeEditorTerrainGeneration(
    CreativeEditorTerrainGenerationState& state,
    std::string_view reasonCode) {
  const bool changed = state.previewActive || state.generation.receipt.requested;
  state.previewActive = false;
  state.generation = {};
  state.composition = {};
  state.sourceDocumentId = cr::kInvalidDocumentId;
  state.sourceDocumentRevision = 0U;
  state.statusMessage = reasonCode.empty() ? "Terrain preview canceled"
                                           : std::string(reasonCode);
  return changed;
}

CreativeEditorTerrainGenerationApplyReceipt
applyCreativeEditorTerrainGeneration(
    cr::CreativeAppState& appState,
    CreativeEditorTerrainGenerationState& state) {
  CreativeEditorTerrainGenerationApplyReceipt receipt;
  receipt.requested = true;
  const cr::CreativeDocument& document = appState.facade.document();
  if (!creativeEditorTerrainGenerationPreviewMatches(state, document)) {
    receipt.reasonCode = state.previewActive
                             ? "creative_editor_terrain_generation_preview_stale"
                             : "creative_editor_terrain_generation_preview_inactive";
    state.statusMessage = state.previewActive
                              ? "Apply failed: preview is stale"
                              : "Apply failed: no active preview";
    return receipt;
  }

  StandaloneEditTransaction transaction = beginEditTransaction(
      appState.facade, "desktop_terrain_generation_apply");
  const cr::CreativeTerrainHeightField& heightField =
      state.composition.heightField;
  receipt.replacement = appState.facade.replaceTerrainHeightField(
      heightField.bounds(), heightField.heights());
  receipt.history = completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.replacement.accepted && receipt.replacement.changed,
      receipt.replacement.reasonCode);

  receipt.changed = receipt.replacement.changed;
  receipt.accepted = receipt.replacement.accepted &&
                     (!receipt.changed || receipt.history.accepted);
  receipt.reasonCode = !receipt.replacement.accepted
                           ? receipt.replacement.reasonCode
                       : receipt.changed && !receipt.history.accepted
                           ? receipt.history.reasonCode
                           : "creative_editor_terrain_generation_applied";
  if (receipt.replacement.accepted) {
    state.previewActive = false;
    state.sourceDocumentId = document.id();
    state.sourceDocumentRevision = document.revision();
  }
  state.statusMessage = receipt.accepted
                            ? (receipt.changed ? "Terrain applied"
                                               : "Terrain already applied")
                            : "Apply failed: " +
                                  std::string(receipt.reasonCode);
  return receipt;
}

}  // namespace iggy3d_creative_app
