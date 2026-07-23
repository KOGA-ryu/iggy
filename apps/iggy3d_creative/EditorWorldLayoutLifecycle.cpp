#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutHistory.hpp"
#include "EditorWorldLayoutInternal.hpp"
#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"

#include <array>
#include <span>
#include <string>
#include <utility>

namespace iggy3d_creative_app {

using detail::clearWorldLayoutInteraction;
using detail::invalidateWorldLayoutPreview;

namespace {

struct InspectionPreviewProbe {
  bool requested = false;
  bool valid = false;
  std::string_view reasonCode;
};

bool exactPreviewActive(
    const CreativeEditorWorldLayoutState& state) noexcept {
  return state.previewVisible && state.preview.accepted &&
         state.preview.document.isValid() &&
         state.previewLayoutRevision == state.revision;
}

const cr::CreativeWorldLayout* inspectionDisplaySource(
    const CreativeEditorWorldLayoutState& state) noexcept {
  if (exactPreviewActive(state)) {
    return &state.previewSource;
  }
  if (state.roomCornerManipulation.active &&
      state.roomCornerManipulation.previewValid &&
      state.roomCornerManipulation.sourceRevision == state.revision &&
      state.roomCornerManipulation.previewEdit.accepted &&
      state.roomCornerManipulation.previewEdit.changed) {
    return &state.roomCornerManipulation.previewEdit.edited;
  }
  if (state.roomBoundaryManipulation.active &&
      state.roomBoundaryManipulation.previewValid &&
      state.roomBoundaryManipulation.sourceRevision == state.revision &&
      state.roomBoundaryManipulation.previewEdit.accepted &&
      state.roomBoundaryManipulation.previewEdit.changed) {
    return &state.roomBoundaryManipulation.previewEdit.edited;
  }
  if (state.roomManipulation.active &&
      state.roomManipulation.previewValid &&
      state.roomManipulation.sourceRevision == state.revision &&
      state.roomManipulation.previewEdit.accepted &&
      state.roomManipulation.target.roomIndex <
          state.roomManipulation.previewEdit.edited.rooms.size()) {
    return &state.roomManipulation.previewEdit.edited;
  }
  if (state.buildingTemplatePlacement.active &&
      state.buildingTemplatePlacement.previewPositioned &&
      state.buildingTemplatePlacement.sourceRevision == state.revision &&
      state.buildingTemplatePlacement.resultBuildingIndex <
          state.buildingTemplatePlacement.candidate.buildings.size()) {
    return &state.buildingTemplatePlacement.candidate;
  }
  if (state.buildingTransform.active &&
      state.buildingTransform.sourceRevision == state.revision &&
      state.buildingTransform.buildingIndex <
          state.buildingTransform.candidate.buildings.size()) {
    return &state.buildingTransform.candidate;
  }
  return &state.source;
}

std::uint64_t currentWorldLayoutFingerprint(
    const CreativeEditorWorldLayoutState& state) {
  if (!state.cachedFingerprintValid ||
      state.cachedFingerprintSourceEpoch != state.sourceEpoch ||
      state.cachedFingerprintRevision != state.revision) {
    state.cachedFingerprintSourceEpoch = state.sourceEpoch;
    state.cachedFingerprintRevision = state.revision;
    state.cachedFingerprint =
        cr::fingerprintCreativeWorldLayout(state.source);
    state.cachedFingerprintValid = true;
  }
  return state.cachedFingerprint;
}

InspectionPreviewProbe activePreviewProbe(
    const CreativeEditorWorldLayoutState& state) noexcept {
  const std::array probes{
      InspectionPreviewProbe{state.roomManipulation.active,
                             state.roomManipulation.previewValid,
                             state.roomManipulation.reasonCode},
      InspectionPreviewProbe{state.roomBoundaryManipulation.active,
                             state.roomBoundaryManipulation.previewValid,
                             state.roomBoundaryManipulation.reasonCode},
      InspectionPreviewProbe{state.roomCornerManipulation.active,
                             state.roomCornerManipulation.previewValid,
                             state.roomCornerManipulation.reasonCode},
      InspectionPreviewProbe{state.verticalConnectorManipulation.active,
                             state.verticalConnectorManipulation.previewValid,
                             state.verticalConnectorManipulation.reasonCode},
      InspectionPreviewProbe{state.boxManipulation.active,
                             state.boxManipulation.previewValid,
                             state.boxManipulation.reasonCode},
      InspectionPreviewProbe{state.wallManipulation.active,
                             state.wallManipulation.previewValid,
                             state.wallManipulation.reasonCode},
      InspectionPreviewProbe{state.buildingManipulation.active,
                             state.buildingManipulation.previewValid,
                             state.buildingManipulation.reasonCode},
      InspectionPreviewProbe{
          state.buildingTransform.active,
          state.buildingTransform.active &&
              state.buildingTransform.sourceRevision == state.revision &&
              state.buildingTransform.buildingIndex <
                  state.buildingTransform.candidate.buildings.size(),
          state.buildingTransform.reasonCode},
      InspectionPreviewProbe{
          state.buildingTemplatePlacement.active,
          state.buildingTemplatePlacement.previewValid,
          state.buildingTemplatePlacement.reasonCode},
      InspectionPreviewProbe{state.openingManipulation.active,
                             state.openingManipulation.previewValid,
                             state.openingManipulation.reasonCode},
      InspectionPreviewProbe{state.roofApertureManipulation.active,
                             state.roofApertureManipulation.previewValid,
                             state.roofApertureManipulation.reasonCode},
      InspectionPreviewProbe{state.roofManipulation.active,
                             state.roofManipulation.previewValid,
                             state.roofManipulation.reasonCode},
      InspectionPreviewProbe{state.objectManipulation.active,
                             state.objectManipulation.previewValid,
                             state.objectManipulation.reasonCode},
      InspectionPreviewProbe{state.elevationManipulation.active,
                             state.elevationManipulation.preview.accepted,
                             state.elevationManipulation.preview.reasonCode},
      InspectionPreviewProbe{
          state.terrainPathDraft.active,
          state.terrainPathDraft.path.recipe.points.size() >= 2U,
          "creative_editor_world_layout_path_draft_incomplete"},
      InspectionPreviewProbe{state.anchorActive,
                             state.gesturePreviewGridPointValid,
                             "creative_editor_world_layout_gesture_target_invalid"},
  };
  for (const InspectionPreviewProbe& probe : probes) {
    if (probe.requested) {
      return probe;
    }
  }
  return {};
}

}  // namespace

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
  state.savedFingerprint = currentWorldLayoutFingerprint(state);
  state.hasSavedFingerprint = true;
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
      state.source.roofApertures.size() +
      state.source.topologyVertices.size() +
      state.source.topologyEdges.size() +
      state.source.boxes.size() + state.source.walls.size() +
      state.source.openings.size() + state.source.objects.size() +
      state.source.terrainProfiles.size() + state.source.terrainPaths.size();
  state.generatedRevision = state.revision;
  state.savedFingerprint = currentWorldLayoutFingerprint(state);
  state.hasSavedFingerprint = true;
  repairCreativeEditorWorldLayoutActiveLevel(state);
  state.generatedBaseline = {state.source, state.revision, state.savedRevision,
                             state.generatedRevision,
                             state.nextStableOrdinal};
  detail::resetWorldLayoutSourceHistory(state);
  state.statusMessage = "layout loaded";
}

void markCreativeEditorWorldLayoutSaved(
    CreativeEditorWorldLayoutState& state) {
  state.deferredSourceHistory = {};
  state.savedRevision = state.revision;
  state.savedFingerprint =
      cr::fingerprintCreativeWorldLayout(state.source);
  state.hasSavedFingerprint = true;
  state.cachedFingerprintSourceEpoch = state.sourceEpoch;
  state.cachedFingerprintRevision = state.revision;
  state.cachedFingerprint = state.savedFingerprint;
  state.cachedFingerprintValid = true;
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
    const CreativeEditorWorldLayoutState& state) {
  if (!state.hasSavedFingerprint) {
    return state.revision != state.savedRevision;
  }
  const std::uint64_t fingerprint = currentWorldLayoutFingerprint(state);
  return fingerprint == 0U || fingerprint != state.savedFingerprint;
}

bool creativeEditorWorldLayoutPreviewActive(
    const CreativeEditorWorldLayoutState& state) noexcept {
  return exactPreviewActive(state);
}

const cr::CreativeDocument& creativeEditorWorldLayoutRenderDocument(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& liveDocument) noexcept {
  return creativeEditorWorldLayoutPreviewActive(state) ? state.preview.document
                                                       : liveDocument;
}

const cr::CreativeWorldLayout& creativeEditorWorldLayoutDisplaySource(
    const CreativeEditorWorldLayoutState& state) noexcept {
  return *inspectionDisplaySource(state);
}

CreativeEditorWorldLayoutInspection inspectCreativeEditorWorldLayout(
    const CreativeEditorWorldLayoutState& state) noexcept {
  CreativeEditorWorldLayoutInspection inspection;
  inspection.source = inspectionDisplaySource(state);
  const bool exactPreview = exactPreviewActive(state);
  const InspectionPreviewProbe probe = activePreviewProbe(state);
  const bool previewRequested = exactPreview || probe.requested;
  inspection.sourceKind =
      previewRequested || inspection.source != &state.source
          ? CreativeEditorWorldLayoutInspectionSourceKind::Preview
          : CreativeEditorWorldLayoutInspectionSourceKind::Authored;
  inspection.previewValidity =
      exactPreview
          ? CreativeEditorWorldLayoutPreviewValidity::Valid
          : probe.requested
                ? CreativeEditorWorldLayoutPreviewValidity::Invalid
                : CreativeEditorWorldLayoutPreviewValidity::None;
  inspection.contentRevision =
      exactPreview ? state.previewContentRevision : 0U;
  inspection.volatileSource = inspection.source != &state.source &&
                              inspection.source != &state.previewSource;
  inspection.reasonCode =
      exactPreview ? state.preview.reasonCode
                   : probe.requested
                         ? probe.valid
                               ? "creative_editor_world_layout_inspection_3d_preview_missing"
                               : probe.reasonCode
                         : "creative_editor_world_layout_inspection_authored";

  const cr::CreativeWorldLayoutTable table =
      creativeEditorWorldLayoutSelectionTable(state.selection.kind);
  if (table != cr::CreativeWorldLayoutTable::None &&
      !creativeEditorWorldLayoutSourceStableKey(
           state, table, state.selection.index)
           .empty()) {
    inspection.authoredSource = {table, state.selection.index};
  }
  return inspection;
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
  state.previewSource = state.source;
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
