#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutHistory.hpp"
#include "EditorWorldLayoutInternal.hpp"

#include <string>
#include <string_view>
#include <utility>

namespace iggy3d_creative_app {
namespace {

CreativeEditorWorldLayoutState makeWorldLayoutSettingsCandidate(
    const CreativeEditorWorldLayoutState& state) {
  CreativeEditorWorldLayoutState candidate;
  candidate.source = state.source;
  candidate.revision = state.revision;
  candidate.savedRevision = state.savedRevision;
  candidate.generatedRevision = state.generatedRevision;
  candidate.nextStableOrdinal = state.nextStableOrdinal;
  candidate.activeLevelIndex = state.activeLevelIndex;
  candidate.selection = state.selection;
  candidate.sourceHistory.maxDepth = 0U;
  return candidate;
}

CreativeEditorWorldLayoutPreviewReceipt previewWorldLayoutSettingsCandidate(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document,
    CreativeEditorWorldLayoutState candidate,
    CreativeEditorWorldLayoutEditReceipt editReceipt,
    std::string_view successMessage) {
  CreativeEditorWorldLayoutPreviewReceipt result;
  const bool previewWasActive = creativeEditorWorldLayoutPreviewActive(state);
  if (state.generatedRevision != state.revision) {
    result.reasonCode =
        "creative_editor_world_layout_generated_edit_unsynchronized_source";
    state.statusMessage = result.reasonCode;
    return result;
  }
  if (!editReceipt.accepted) {
    result.reasonCode = editReceipt.reasonCode;
    result.changed = previewWasActive;
    state.statusMessage = candidate.statusMessage;
    detail::invalidateWorldLayoutPreview(state);
    return result;
  }
  if (!editReceipt.changed) {
    result.accepted = true;
    result.reasonCode = editReceipt.reasonCode;
    state.statusMessage = "generated source preview already current";
    return result;
  }

  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, candidate.source);
  result.status = compiled.receipt.status;
  if (!compiled.receipt.accepted) {
    result.reasonCode = compiled.receipt.reasonCode;
    result.changed = previewWasActive;
    state.statusMessage = result.reasonCode;
    detail::invalidateWorldLayoutPreview(state);
    return result;
  }
  cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, compiled.plan);
  result.status = preview.status;
  result.accepted = preview.accepted;
  result.changed = preview.accepted || previewWasActive;
  result.reasonCode = preview.reasonCode;
  if (!preview.accepted) {
    state.statusMessage = result.reasonCode;
    detail::invalidateWorldLayoutPreview(state);
    return result;
  }
  state.preview = std::move(preview);
  state.previewVisible = true;
  state.previewLayoutRevision = state.revision;
  state.statusMessage = std::string(successMessage);
  return result;
}

CreativeEditorWorldLayoutApplyReceipt applyWorldLayoutSettingsCandidate(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    CreativeEditorWorldLayoutState candidate,
    CreativeEditorWorldLayoutEditReceipt editReceipt,
    std::string_view historySource, std::string_view successMessage) {
  CreativeEditorWorldLayoutApplyReceipt result;
  if (state.generatedRevision != state.revision) {
    result.reasonCode =
        "creative_editor_world_layout_generated_edit_unsynchronized_source";
    state.statusMessage = result.reasonCode;
    return result;
  }
  if (!editReceipt.accepted) {
    result.reasonCode = editReceipt.reasonCode;
    state.statusMessage = candidate.statusMessage;
    return result;
  }
  if (!editReceipt.changed) {
    result.accepted = true;
    result.reasonCode = editReceipt.reasonCode;
    state.statusMessage = "generated source already current";
    return result;
  }

  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       candidate.source);
  if (!compiled.receipt.accepted) {
    result.reasonCode = compiled.receipt.reasonCode;
    state.statusMessage = result.reasonCode;
    return result;
  }

  CreativeEditorWorldLayoutSourceHistoryEntry sourceOnlyUndo =
      detail::captureWorldLayoutSourceHistoryEntry(state);
  sourceOnlyUndo.source = std::string(historySource);
  result.apply = applyCreativeEditorWorldLayoutPlanWithHistory(
      state, appState, compiled.plan,
      captureCreativeEditorWorldLayoutSnapshot(candidate), historySource);
  result.accepted = result.apply.accepted;
  result.changed = result.accepted;
  result.reasonCode = result.apply.reasonCode;
  if (!result.accepted) {
    state.statusMessage = result.reasonCode;
    return result;
  }

  // A source setting can be meaningful even when its compiled geometry is
  // identical (for example, a closed pose on an omitted insert). Preserve one
  // source-history entry instead of manufacturing a document revision.
  if (!result.apply.changed) {
    detail::appendWorldLayoutSourceHistoryEntry(
        state.sourceHistory.undoEntries, std::move(sourceOnlyUndo),
        state.sourceHistory.maxDepth);
    state.sourceHistory.redoEntries.clear();
  }
  state.statusMessage = std::string(successMessage);
  return result;
}

}  // namespace

CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutRoomSettings(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t roomIndex,
    CreativeEditorWorldLayoutRoomSettings settings) {
  CreativeEditorWorldLayoutState candidate =
      makeWorldLayoutSettingsCandidate(state);
  const CreativeEditorWorldLayoutEditReceipt editReceipt =
      setCreativeEditorWorldLayoutRoomSettings(candidate, roomIndex, settings);
  return previewWorldLayoutSettingsCandidate(
      state, document, std::move(candidate), editReceipt,
      "room shell preview ready");
}

CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutRoomSettingsToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t roomIndex, CreativeEditorWorldLayoutRoomSettings settings) {
  CreativeEditorWorldLayoutState candidate =
      makeWorldLayoutSettingsCandidate(state);
  const CreativeEditorWorldLayoutEditReceipt editReceipt =
      setCreativeEditorWorldLayoutRoomSettings(candidate, roomIndex, settings);
  return applyWorldLayoutSettingsCandidate(
      state, appState, std::move(candidate), editReceipt,
      "desktop_generated_room_settings", "room shell updated in 3D");
}

CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutVerticalConnectorSettings(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t connectorIndex,
    CreativeEditorWorldLayoutVerticalConnectorSettings settings) {
  CreativeEditorWorldLayoutState candidate =
      makeWorldLayoutSettingsCandidate(state);
  const CreativeEditorWorldLayoutEditReceipt editReceipt =
      setCreativeEditorWorldLayoutVerticalConnectorSettings(
          candidate, connectorIndex, settings);
  return previewWorldLayoutSettingsCandidate(
      state, document, std::move(candidate), editReceipt,
      "vertical connector preview ready");
}

CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutVerticalConnectorSettingsToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t connectorIndex,
    CreativeEditorWorldLayoutVerticalConnectorSettings settings) {
  CreativeEditorWorldLayoutState candidate =
      makeWorldLayoutSettingsCandidate(state);
  const CreativeEditorWorldLayoutEditReceipt editReceipt =
      setCreativeEditorWorldLayoutVerticalConnectorSettings(
          candidate, connectorIndex, settings);
  return applyWorldLayoutSettingsCandidate(
      state, appState, std::move(candidate), editReceipt,
      "desktop_generated_vertical_connector_settings",
      "vertical connector updated in 3D");
}

CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutWallSettings(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t wallIndex,
    CreativeEditorWorldLayoutWallSettings settings) {
  CreativeEditorWorldLayoutState candidate =
      makeWorldLayoutSettingsCandidate(state);
  const CreativeEditorWorldLayoutEditReceipt editReceipt =
      setCreativeEditorWorldLayoutWallSettings(candidate, wallIndex, settings);
  return previewWorldLayoutSettingsCandidate(
      state, document, std::move(candidate), editReceipt,
      "partition preview ready");
}

CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutWallSettingsToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t wallIndex, CreativeEditorWorldLayoutWallSettings settings) {
  CreativeEditorWorldLayoutState candidate =
      makeWorldLayoutSettingsCandidate(state);
  const CreativeEditorWorldLayoutEditReceipt editReceipt =
      setCreativeEditorWorldLayoutWallSettings(candidate, wallIndex, settings);
  return applyWorldLayoutSettingsCandidate(
      state, appState, std::move(candidate), editReceipt,
      "desktop_generated_partition_settings", "partition updated in 3D");
}

CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayoutOpeningSettings(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings settings) {
  CreativeEditorWorldLayoutState candidate =
      makeWorldLayoutSettingsCandidate(state);
  const CreativeEditorWorldLayoutEditReceipt editReceipt =
      setCreativeEditorWorldLayoutOpeningSettings(candidate, openingIndex,
                                                  settings);
  return previewWorldLayoutSettingsCandidate(
      state, document, std::move(candidate), editReceipt,
      "opening preview ready");
}

CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutOpeningSettingsToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings settings) {
  CreativeEditorWorldLayoutState candidate =
      makeWorldLayoutSettingsCandidate(state);
  const CreativeEditorWorldLayoutEditReceipt editReceipt =
      setCreativeEditorWorldLayoutOpeningSettings(candidate, openingIndex,
                                                  settings);
  return applyWorldLayoutSettingsCandidate(
      state, appState, std::move(candidate), editReceipt,
      "desktop_generated_opening_settings", "opening updated in 3D");
}

}  // namespace iggy3d_creative_app
