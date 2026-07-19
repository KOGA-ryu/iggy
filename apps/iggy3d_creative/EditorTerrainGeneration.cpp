#include "EditorTerrainGeneration.hpp"

#include "EditorEdits.hpp"

#include <limits>
#include <string>
#include <utility>

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

namespace {

void clearPreviewFacts(CreativeEditorTerrainGenerationState& state) noexcept {
  state.previewActive = false;
  state.generation = {};
  state.operationPreview = {};
  state.sourceDocumentId = cr::kInvalidDocumentId;
  state.sourceDocumentRevision = 0U;
}

[[nodiscard]] cr::CreativeTerrainOperationMutationRequest operationRequest(
    const CreativeEditorTerrainGenerationState& state,
    const cr::CreativeDocument& document) {
  cr::CreativeTerrainOperationMutationRequest request;
  request.kind = state.editingOperationId ==
                         cr::kInvalidCreativeTerrainOperationId
                     ? cr::CreativeTerrainOperationMutationKind::Add
                     : cr::CreativeTerrainOperationMutationKind::Update;
  request.operationId = state.editingOperationId;
  request.generation = state.recipe;
  request.composition = state.compositionRecipe;
  const cr::CreativeTerrainOperation* existing =
      cr::findCreativeTerrainOperation(document.terrainOperationStack(),
                                       state.editingOperationId);
  request.enabled = existing == nullptr ? true : existing->enabled;
  return request;
}

}  // namespace

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
         state.operationPreview.receipt.accepted;
}

bool synchronizeCreativeEditorTerrainGeneration(
    CreativeEditorTerrainGenerationState& state,
    const cr::CreativeDocument& document) {
  bool changed = false;
  const cr::CreativeTerrainOperation* selected =
      cr::findCreativeTerrainOperation(document.terrainOperationStack(),
                                       state.editingOperationId);
  if (state.editingOperationId != cr::kInvalidCreativeTerrainOperationId &&
      selected == nullptr) {
    state.editingOperationId = cr::kInvalidCreativeTerrainOperationId;
    changed = true;
  }
  if (state.previewActive &&
      (state.sourceDocumentId != document.id() ||
       state.sourceDocumentRevision != document.revision())) {
    clearPreviewFacts(state);
    state.statusMessage = "Preview canceled: document changed";
    changed = true;
  }
  if (selected != nullptr && !state.previewActive && !state.draftDirty &&
      (state.sourceDocumentId != document.id() ||
       state.sourceDocumentRevision != document.revision())) {
    state.recipe = selected->generation;
    state.compositionRecipe = selected->composition;
    state.sourceDocumentId = document.id();
    state.sourceDocumentRevision = document.revision();
    changed = true;
  }
  return changed;
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
    state.draftDirty = true;
  }

  state.generation = cr::buildCreativeTerrainGenerationPlan(state.recipe);
  state.operationPreview = cr::planCreativeTerrainOperationMutation(
      document.terrainField(), document.terrainHeightField(),
      document.terrainOperationStack(), operationRequest(state, document));
  state.sourceDocumentId = document.id();
  state.sourceDocumentRevision = document.revision();
  ++state.generationCount;
  state.previewActive = true;

  receipt.accepted = state.operationPreview.receipt.accepted;
  receipt.previewActive = true;
  receipt.seed = state.recipe.seed;
  receipt.generationCount = state.generationCount;
  receipt.reasonCode = !state.generation.receipt.accepted
                           ? state.generation.receipt.reasonCode
                           : state.operationPreview.receipt.reasonCode;
  if (receipt.accepted) {
    state.statusMessage =
        advanceSeed ? "Terrain operation regenerated"
                    : "Terrain operation preview ready";
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
  clearPreviewFacts(state);
  state.statusMessage = reasonCode.empty() ? "Terrain preview canceled"
                                           : std::string(reasonCode);
  return changed;
}

bool beginNewCreativeEditorTerrainOperation(
    CreativeEditorTerrainGenerationState& state) {
  const bool changed =
      state.editingOperationId != cr::kInvalidCreativeTerrainOperationId ||
      state.previewActive;
  clearPreviewFacts(state);
  state.editingOperationId = cr::kInvalidCreativeTerrainOperationId;
  state.draftDirty = false;
  state.statusMessage = "New terrain operation";
  return changed;
}

bool selectCreativeEditorTerrainOperation(
    CreativeEditorTerrainGenerationState& state,
    const cr::CreativeDocument& document,
    cr::CreativeTerrainOperationId operationId) {
  const cr::CreativeTerrainOperation* operation =
      cr::findCreativeTerrainOperation(document.terrainOperationStack(),
                                       operationId);
  if (operation == nullptr) {
    state.statusMessage = "Terrain operation unavailable";
    return false;
  }
  const bool changed = state.editingOperationId != operationId ||
                       state.recipe != operation->generation ||
                       state.compositionRecipe != operation->composition ||
                       state.previewActive;
  clearPreviewFacts(state);
  state.editingOperationId = operationId;
  state.recipe = operation->generation;
  state.compositionRecipe = operation->composition;
  state.sourceDocumentId = document.id();
  state.sourceDocumentRevision = document.revision();
  state.draftDirty = false;
  state.statusMessage = "Terrain operation selected";
  return changed;
}

CreativeEditorTerrainOperationEditReceipt editCreativeEditorTerrainOperation(
    cr::CreativeAppState& appState,
    CreativeEditorTerrainGenerationState& state,
    const cr::CreativeTerrainOperationMutationRequest& request,
    std::string_view source) {
  CreativeEditorTerrainOperationEditReceipt receipt;
  receipt.requested = true;
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  receipt.operation = appState.facade.applyTerrainOperationMutation(request);
  receipt.history = completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.operation.accepted && receipt.operation.changed,
      receipt.operation.reasonCode);
  receipt.changed = receipt.operation.changed;
  receipt.accepted = receipt.operation.accepted &&
                     (!receipt.changed || receipt.history.accepted);
  receipt.reasonCode = !receipt.operation.accepted
                           ? receipt.operation.reasonCode
                       : receipt.changed && !receipt.history.accepted
                           ? receipt.history.reasonCode
                           : "creative_editor_terrain_operation_edited";
  if (receipt.operation.accepted) {
    clearPreviewFacts(state);
    if (request.kind == cr::CreativeTerrainOperationMutationKind::Remove &&
        state.editingOperationId == request.operationId) {
      state.editingOperationId = cr::kInvalidCreativeTerrainOperationId;
      state.draftDirty = false;
    } else if (request.kind ==
               cr::CreativeTerrainOperationMutationKind::Add) {
      state.editingOperationId = receipt.operation.operationId;
      state.recipe = request.generation;
      state.compositionRecipe = request.composition;
      state.draftDirty = false;
    }
    state.sourceDocumentId = appState.facade.document().id();
    state.sourceDocumentRevision = appState.facade.document().revision();
  }
  state.statusMessage = receipt.accepted
                            ? (receipt.changed ? "Terrain operation updated"
                                               : "Terrain operation unchanged")
                            : "Terrain operation failed: " +
                                  std::string(receipt.reasonCode);
  return receipt;
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
  receipt.operation = appState.facade.applyTerrainOperationMutation(
      operationRequest(state, document));
  receipt.history = completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.operation.accepted && receipt.operation.changed,
      receipt.operation.reasonCode);

  receipt.changed = receipt.operation.changed;
  receipt.accepted = receipt.operation.accepted &&
                     (!receipt.changed || receipt.history.accepted);
  receipt.reasonCode = !receipt.operation.accepted
                           ? receipt.operation.reasonCode
                       : receipt.changed && !receipt.history.accepted
                           ? receipt.history.reasonCode
                           : "creative_editor_terrain_generation_applied";
  if (receipt.operation.accepted) {
    clearPreviewFacts(state);
    state.editingOperationId = receipt.operation.operationId;
    state.draftDirty = false;
    state.sourceDocumentId = appState.facade.document().id();
    state.sourceDocumentRevision = appState.facade.document().revision();
  }
  state.statusMessage = receipt.accepted
                            ? (receipt.changed ? "Terrain operation applied"
                                               : "Terrain operation unchanged")
                            : "Apply failed: " +
                                  std::string(receipt.reasonCode);
  return receipt;
}

}  // namespace iggy3d_creative_app
