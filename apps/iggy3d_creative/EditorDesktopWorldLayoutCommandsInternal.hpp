#pragma once

#include "EditorDesktopCommandsInternal.hpp"
#include "EditorWorldLayout.hpp"

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace iggy3d_creative_app {

struct GeneratedSourceScopeResolution {
  bool ancestor = false;
  bool stable = false;
};

struct CreativeDesktopWorldLayoutLiveEditResult {
  bool accepted = false;
  bool changed = false;
  bool sceneChanged = false;
  bool worldLayoutChanged = false;
};

template <typename Phase, typename ApplyEdit>
[[nodiscard]] CreativeDesktopWorldLayoutLiveEditResult
dispatchCreativeDesktopWorldLayoutLiveEdit(
    CreativeEditorWorldLayoutState& state,
    iggy3d::creative::CreativeAppState& appState,
    Phase phase,
    Phase beginPhase,
    Phase updatePhase,
    Phase commitPhase,
    Phase cancelPhase,
    ApplyEdit applyEdit,
    std::string_view historySource,
    std::string_view previewMessage,
    std::string_view successMessage) {
  CreativeDesktopWorldLayoutLiveEditResult result;
  const bool previewWasActive = creativeEditorWorldLayoutPreviewActive(state);

  if (phase == beginPhase) {
    result.sceneChanged =
        clearCreativeEditorWorldLayoutLiveEditPreview(state);
    const CreativeEditorWorldLayoutEditReceipt edit =
        applyEdit(state, phase);
    result.accepted = edit.accepted;
    result.changed = edit.changed;
    return result;
  }

  if (phase == cancelPhase) {
    const CreativeEditorWorldLayoutEditReceipt edit =
        applyEdit(state, phase);
    result.accepted = edit.accepted;
    result.changed = edit.changed;
    result.sceneChanged =
        clearCreativeEditorWorldLayoutLiveEditPreview(state);
    return result;
  }

  const bool sourceSynchronized = state.generatedRevision == state.revision;
  if (!sourceSynchronized) {
    const CreativeEditorWorldLayoutEditReceipt edit =
        applyEdit(state, phase);
    result.accepted = edit.accepted;
    result.changed = edit.changed;
    result.worldLayoutChanged = phase == commitPhase && edit.changed;
    result.sceneChanged = previewWasActive && result.worldLayoutChanged;
    return result;
  }

  if (phase == updatePhase) {
    const CreativeEditorWorldLayoutEditReceipt edit =
        applyEdit(state, phase);
    result.accepted = edit.accepted;
    result.changed = edit.changed;
    if (!edit.accepted || !edit.changed) {
      return result;
    }

    CreativeEditorWorldLayoutState candidate =
        makeCreativeEditorWorldLayoutLiveEditCandidate(state);
    const CreativeEditorWorldLayoutEditReceipt candidateEdit =
        applyEdit(candidate, commitPhase);
    if (!candidateEdit.accepted || !candidateEdit.changed) {
      const std::string candidateMessage = candidate.statusMessage;
      result.sceneChanged =
          clearCreativeEditorWorldLayoutLiveEditPreview(state);
      state.statusMessage = candidateMessage;
      return result;
    }

    const CreativeEditorWorldLayoutPreviewReceipt preview =
        previewCreativeEditorWorldLayoutLiveEditCandidate(
            state, appState.facade.document(), std::move(candidate),
            candidateEdit, previewMessage);
    result.sceneChanged = preview.changed;
    return result;
  }

  if (phase == commitPhase) {
    CreativeEditorWorldLayoutState candidate =
        makeCreativeEditorWorldLayoutLiveEditCandidate(state);
    const CreativeEditorWorldLayoutEditReceipt candidateEdit =
        applyEdit(candidate, commitPhase);
    if (!candidateEdit.accepted || !candidateEdit.changed) {
      const std::string candidateMessage = candidate.statusMessage;
      static_cast<void>(applyEdit(state, cancelPhase));
      result.sceneChanged =
          clearCreativeEditorWorldLayoutLiveEditPreview(state);
      state.statusMessage = candidateMessage;
      result.accepted = candidateEdit.accepted;
      return result;
    }

    const CreativeEditorWorldLayoutApplyReceipt applied =
        applyCreativeEditorWorldLayoutLiveEditCandidate(
            state, appState, std::move(candidate), candidateEdit,
            historySource, successMessage);
    result.accepted = applied.accepted;
    result.changed = applied.changed;
    result.worldLayoutChanged = applied.changed;
    result.sceneChanged = previewWasActive || applied.apply.changed;
    if (!applied.accepted) {
      const std::string failureMessage = state.statusMessage;
      static_cast<void>(applyEdit(state, cancelPhase));
      static_cast<void>(
          clearCreativeEditorWorldLayoutLiveEditPreview(state));
      state.statusMessage = failureMessage;
    }
    return result;
  }

  const CreativeEditorWorldLayoutEditReceipt edit =
      applyEdit(state, phase);
  result.accepted = edit.accepted;
  result.changed = edit.changed;
  return result;
}

template <typename ApplyEdit>
[[nodiscard]] CreativeDesktopWorldLayoutLiveEditResult
dispatchCreativeDesktopWorldLayoutImmediateEdit(
    CreativeEditorWorldLayoutState& state,
    iggy3d::creative::CreativeAppState& appState,
    ApplyEdit applyEdit,
    std::string_view historySource) {
  CreativeDesktopWorldLayoutLiveEditResult result;
  const bool previewWasActive = creativeEditorWorldLayoutPreviewActive(state);
  const bool livePreviewCleared =
      clearCreativeEditorWorldLayoutLiveEditPreview(state);
  if (state.generatedRevision != state.revision) {
    const CreativeEditorWorldLayoutEditReceipt edit = applyEdit(state);
    result.accepted = edit.accepted;
    result.changed = edit.changed;
    result.worldLayoutChanged = edit.changed;
    result.sceneChanged =
        livePreviewCleared || (previewWasActive && edit.changed);
    return result;
  }

  CreativeEditorWorldLayoutState candidate =
      makeCreativeEditorWorldLayoutLiveEditCandidate(state);
  const CreativeEditorWorldLayoutEditReceipt candidateEdit =
      applyEdit(candidate);
  if (!candidateEdit.accepted) {
    state.statusMessage = candidate.statusMessage;
    result.sceneChanged = livePreviewCleared;
    return result;
  }

  if (!candidateEdit.changed || candidate.revision == state.revision) {
    const CreativeEditorWorldLayoutEditReceipt edit = applyEdit(state);
    result.accepted = edit.accepted;
    result.changed = edit.changed;
    result.sceneChanged = livePreviewCleared;
    return result;
  }

  const std::string successMessage = candidate.statusMessage;
  const CreativeEditorWorldLayoutApplyReceipt applied =
      applyCreativeEditorWorldLayoutLiveEditCandidate(
          state, appState, std::move(candidate), candidateEdit,
          historySource, successMessage);
  result.accepted = applied.accepted;
  result.changed = applied.changed;
  result.worldLayoutChanged = applied.changed;
  result.sceneChanged = applied.accepted &&
                        (previewWasActive || applied.apply.changed);
  return result;
}

[[nodiscard]] GeneratedSourceScopeResolution resolveGeneratedSourceScope(
    const iggy3d::creative::CreativeWorldLayout& layout,
    const iggy3d::creative::CreativeObject& object,
    iggy3d::creative::CreativeWorldLayoutTable table,
    std::size_t index, std::string_view stableKey);

[[nodiscard]] iggy3d::creative::CreativeObjectId
findGeneratedSourceScopeObject(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::creative::CreativeWorldLayout& layout,
    iggy3d::creative::CreativeWorldLayoutTable table,
    std::size_t index,
    iggy3d::creative::CreativeObjectKind preferredKind) noexcept;

[[nodiscard]] const iggy3d::creative::CreativeCatalogEntry* findCatalogAsset(
    const iggy3d::creative::CreativeCatalogState& catalog,
    std::string_view assetId) noexcept;

CreativeEditorWorldLayoutEditReceipt repairWorldLayoutAsset(
    CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeCatalogState& catalog,
    double gridCellSizeMeters,
    const CreativeDesktopWorldLayoutAssetRepairPayload& payload);

bool dispatchCreativeDesktopWorldLayoutSourceCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result);

bool dispatchCreativeDesktopWorldLayoutBuildingCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result);

bool dispatchCreativeDesktopWorldLayoutStructureCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result);

bool dispatchCreativeDesktopWorldLayoutLifecycleCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result);

}  // namespace iggy3d_creative_app
