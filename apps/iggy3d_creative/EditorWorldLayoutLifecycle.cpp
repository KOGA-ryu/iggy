#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutHistory.hpp"
#include "EditorWorldLayoutInternal.hpp"

#include <span>
#include <string>
#include <utility>

namespace iggy3d_creative_app {

using detail::clearWorldLayoutInteraction;
using detail::invalidateWorldLayoutPreview;

void resetCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                    std::string layoutKey) {
  CreativeEditorWorldLayoutBuildingTemplateLibrary buildingTemplates =
      std::move(state.buildingTemplates);
  const std::uint64_t sourceEpoch =
      detail::nextWorldLayoutSourceEpoch(state.sourceEpoch);
  state = {};
  state.sourceEpoch = sourceEpoch;
  state.buildingTemplates = std::move(buildingTemplates);
  state.source.stableKey =
      layoutKey.empty() ? "world_layout" : std::move(layoutKey);
  state.generatedBaseline = {state.source, state.revision, state.savedRevision,
                             state.generatedRevision,
                             state.nextStableOrdinal};
  detail::resetWorldLayoutSourceHistory(state);
  state.statusMessage = "blank layout";
}

void installCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                      cr::CreativeWorldLayout layout) {
  CreativeEditorWorldLayoutBuildingTemplateLibrary buildingTemplates =
      std::move(state.buildingTemplates);
  const std::uint64_t sourceEpoch =
      detail::nextWorldLayoutSourceEpoch(state.sourceEpoch);
  state = {};
  state.sourceEpoch = sourceEpoch;
  state.buildingTemplates = std::move(buildingTemplates);
  state.source = std::move(layout);
  state.nextStableOrdinal =
      1U + state.source.buildings.size() + state.source.levels.size() +
      state.source.rooms.size() + state.source.verticalConnectors.size() +
      state.source.boxes.size() + state.source.walls.size() +
      state.source.openings.size() + state.source.objects.size() +
      state.source.terrainProfiles.size() + state.source.terrainPaths.size();
  state.generatedRevision = state.revision;
  repairCreativeEditorWorldLayoutActiveLevel(state);
  state.generatedBaseline = {state.source, state.revision, state.savedRevision,
                             state.generatedRevision,
                             state.nextStableOrdinal};
  detail::resetWorldLayoutSourceHistory(state);
  state.statusMessage = "layout loaded";
}

void markCreativeEditorWorldLayoutSaved(
    CreativeEditorWorldLayoutState& state) noexcept {
  state.deferredSourceHistory = {};
  state.savedRevision = state.revision;
  if (state.generatedRevision == state.revision) {
    state.generatedBaseline.savedRevision = state.savedRevision;
  }
  const auto markEntrySaved = [&state](
                                  CreativeEditorWorldLayoutSourceHistoryEntry&
                                      entry) {
    entry.snapshot.savedRevision = state.savedRevision;
  };
  for (CreativeEditorWorldLayoutSourceHistoryEntry& entry :
       state.sourceHistory.undoEntries) {
    markEntrySaved(entry);
  }
  for (CreativeEditorWorldLayoutSourceHistoryEntry& entry :
       state.sourceHistory.redoEntries) {
    markEntrySaved(entry);
  }
  markEntrySaved(state.sourceHistory.current);
}


bool creativeEditorWorldLayoutDirty(
    const CreativeEditorWorldLayoutState& state) noexcept {
  return state.revision != state.savedRevision;
}

bool creativeEditorWorldLayoutPreviewActive(
    const CreativeEditorWorldLayoutState& state) noexcept {
  return state.previewVisible && state.preview.accepted &&
         state.preview.document.isValid() &&
         state.previewLayoutRevision == state.revision;
}

const cr::CreativeDocument& creativeEditorWorldLayoutRenderDocument(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& liveDocument) noexcept {
  return creativeEditorWorldLayoutPreviewActive(state) ? state.preview.document
                                                       : liveDocument;
}

const cr::CreativeWorldLayout& creativeEditorWorldLayoutDisplaySource(
    const CreativeEditorWorldLayoutState& state) noexcept {
  if (state.buildingTemplatePlacement.active &&
      state.buildingTemplatePlacement.previewValid &&
      state.buildingTemplatePlacement.sourceRevision == state.revision &&
      state.buildingTemplatePlacement.resultBuildingIndex <
          state.buildingTemplatePlacement.candidate.buildings.size()) {
    return state.buildingTemplatePlacement.candidate;
  }
  return state.buildingTransform.active &&
                 state.buildingTransform.sourceRevision == state.revision &&
                 state.buildingTransform.buildingIndex <
                     state.buildingTransform.candidate.buildings.size()
             ? state.buildingTransform.candidate
             : state.source;
}

CreativeEditorWorldLayoutEditReceipt cancelCreativeEditorWorldLayoutPreview(
    CreativeEditorWorldLayoutState& state) noexcept {
  const bool changed = state.previewVisible;
  invalidateWorldLayoutPreview(state);
  state.statusMessage = "3D preview closed";
  return {true, changed, "creative_editor_world_layout_preview_cancelled"};
}

CreativeEditorWorldLayoutPreviewReceipt previewCreativeEditorWorldLayout(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document) {
  CreativeEditorWorldLayoutPreviewReceipt receipt;
  state.anchorActive = false;
  clearWorldLayoutInteraction(state);
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, state.source);
  receipt.status = compiled.receipt.status;
  if (!compiled.receipt.accepted) {
    state.statusMessage = compiled.receipt.reasonCode;
    receipt.reasonCode = compiled.receipt.reasonCode;
    invalidateWorldLayoutPreview(state);
    return receipt;
  }
  cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, compiled.plan);
  receipt.status = preview.status;
  receipt.accepted = preview.accepted;
  receipt.changed = preview.accepted;
  receipt.reasonCode = preview.reasonCode;
  if (!preview.accepted) {
    state.statusMessage = preview.reasonCode;
    invalidateWorldLayoutPreview(state);
    return receipt;
  }
  state.preview = std::move(preview);
  state.previewVisible = true;
  state.liveEditPreviewVisible = false;
  state.previewLayoutRevision = state.revision;
  state.previewContentRevision =
      detail::nextWorldLayoutSourceEpoch(state.previewContentRevision);
  state.statusMessage = "exact 3D preview ready";
  return receipt;
}

cr::CreativeWorldLayoutTerrainReconciliationResult
reconcileCreativeEditorWorldLayoutTerrain(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayout& desiredLayout,
    std::span<const cr::CreativeWorldLayoutTerrainConflictDecision> decisions) {
  return cr::reconcileCreativeWorldLayoutTerrain(
      {&document, &state.generatedBaseline.source, &desiredLayout, decisions});
}

CreativeEditorWorldLayoutApplyReceipt confirmCreativeEditorWorldLayout(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::span<const cr::CreativeWorldLayoutConflictDecision>
        conflictDecisions,
    std::span<const cr::CreativeWorldLayoutTerrainConflictDecision>
        terrainConflictDecisions) {
  CreativeEditorWorldLayoutApplyReceipt result;
  state.anchorActive = false;
  clearWorldLayoutInteraction(state);
  const cr::CreativeWorldLayoutTerrainReconciliationResult terrain =
      reconcileCreativeEditorWorldLayoutTerrain(
          state, appState.facade.document(), state.source,
          terrainConflictDecisions);
  if (!terrain.accepted) {
    result.reasonCode = terrain.reasonCode;
    state.statusMessage = terrain.blocked
                              ? "terrain changed in 3D; resolve before generating"
                              : terrain.reasonCode;
    return result;
  }
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       state.source,
                                       {conflictDecisions});
  if (!compiled.receipt.accepted) {
    result.reasonCode = compiled.receipt.reasonCode;
    state.statusMessage = result.reasonCode;
    return result;
  }
  result.apply = applyCreativeEditorWorldLayoutPlanWithHistory(
      state, appState, compiled.plan,
      captureCreativeEditorWorldLayoutSnapshot(state),
      "desktop_world_layout_confirm");
  result.accepted = result.apply.accepted;
  result.changed = result.apply.changed;
  result.reasonCode = result.apply.reasonCode;
  if (result.accepted) {
    invalidateWorldLayoutPreview(state);
    state.statusMessage =
        result.changed ? "layout generated in 3D" : "3D output already current";
  } else {
    state.statusMessage = result.reasonCode;
  }
  return result;
}

}  // namespace iggy3d_creative_app
