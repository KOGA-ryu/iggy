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

struct GeneratedBuildingCandidate {
  CreativeEditorWorldLayoutState state;
  CreativeEditorWorldLayoutEditReceipt edit;
};

GeneratedBuildingCandidate makeGeneratedBuildingCandidate(
    const CreativeEditorWorldLayoutState& source,
    std::size_t buildingIndex,
    CreativeEditorWorldLayoutGeneratedBuildingOperation operation,
    std::int64_t deltaXCells,
    std::int64_t deltaZCells) {
  GeneratedBuildingCandidate candidate;
  candidate.state = makeWorldLayoutSettingsCandidate(source);
  if (operation >= CreativeEditorWorldLayoutGeneratedBuildingOperation::Count ||
      buildingIndex >= source.source.buildings.size()) {
    candidate.state.statusMessage = "generated building operation is invalid";
    candidate.edit = {
        false, false,
        "creative_editor_world_layout_generated_building_operation_invalid"};
    return candidate;
  }

  if (operation ==
      CreativeEditorWorldLayoutGeneratedBuildingOperation::Move) {
    cr::CreativeWorldLayoutBuildingEditResult moved =
        cr::moveCreativeWorldLayoutBuilding(
            source.source, {buildingIndex, deltaXCells, deltaZCells});
    if (!moved.accepted) {
      candidate.state.statusMessage = moved.reasonCode;
      candidate.edit = {false, false, moved.reasonCode};
      return candidate;
    }
    candidate.state.source = std::move(moved.edited);
    candidate.state.selection = {
        CreativeEditorWorldLayoutSelectionKind::Building,
        moved.resultBuildingIndex};
    if (moved.changed) {
      ++candidate.state.revision;
    }
    candidate.edit = {true, moved.changed, moved.reasonCode};
    return candidate;
  }

  if (operation ==
      CreativeEditorWorldLayoutGeneratedBuildingOperation::Duplicate) {
    cr::CreativeWorldLayoutBuildingEditResult duplicated =
        cr::duplicateCreativeWorldLayoutBuilding(
            source.source,
            {buildingIndex, deltaXCells, deltaZCells,
             source.nextStableOrdinal});
    if (!duplicated.accepted) {
      candidate.state.statusMessage = duplicated.reasonCode;
      candidate.edit = {false, false, duplicated.reasonCode};
      return candidate;
    }
    candidate.state.source = std::move(duplicated.edited);
    candidate.state.nextStableOrdinal = duplicated.nextStableOrdinal;
    candidate.state.selection = {
        CreativeEditorWorldLayoutSelectionKind::Building,
        duplicated.resultBuildingIndex};
    repairCreativeEditorWorldLayoutActiveLevel(
        candidate.state, duplicated.resultBuildingIndex);
    if (duplicated.changed) {
      ++candidate.state.revision;
    }
    candidate.edit = {true, duplicated.changed, duplicated.reasonCode};
    return candidate;
  }

  cr::CreativeWorldLayoutBuildingTransformOperation transformOperation =
      cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90;
  switch (operation) {
    case CreativeEditorWorldLayoutGeneratedBuildingOperation::RotateLeft90:
      transformOperation =
          cr::CreativeWorldLayoutBuildingTransformOperation::RotateLeft90;
      break;
    case CreativeEditorWorldLayoutGeneratedBuildingOperation::RotateRight90:
      transformOperation =
          cr::CreativeWorldLayoutBuildingTransformOperation::RotateRight90;
      break;
    case CreativeEditorWorldLayoutGeneratedBuildingOperation::MirrorX:
      transformOperation =
          cr::CreativeWorldLayoutBuildingTransformOperation::MirrorX;
      break;
    case CreativeEditorWorldLayoutGeneratedBuildingOperation::MirrorZ:
      transformOperation =
          cr::CreativeWorldLayoutBuildingTransformOperation::MirrorZ;
      break;
    case CreativeEditorWorldLayoutGeneratedBuildingOperation::Move:
    case CreativeEditorWorldLayoutGeneratedBuildingOperation::Duplicate:
    case CreativeEditorWorldLayoutGeneratedBuildingOperation::Count:
      break;
  }
  cr::CreativeWorldLayoutBuildingTransformResult transformed =
      cr::transformCreativeWorldLayoutBuilding(
          source.source, {buildingIndex, transformOperation});
  if (!transformed.accepted) {
    candidate.state.statusMessage = transformed.reasonCode;
    candidate.edit = {false, false, transformed.reasonCode};
    return candidate;
  }
  candidate.state.source = std::move(transformed.transformed);
  candidate.state.selection = {
      CreativeEditorWorldLayoutSelectionKind::Building,
      transformed.buildingIndex};
  ++candidate.state.revision;
  candidate.edit = {true, true, transformed.reasonCode};
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
  const CreativeEditorWorldLayoutSelection committedSelection =
      candidate.selection;
  const std::size_t committedActiveLevelIndex = candidate.activeLevelIndex;
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
  state.selection = committedSelection;
  state.activeLevelIndex = committedActiveLevelIndex;

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
previewCreativeEditorWorldLayoutGeneratedBuildingOperation(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t buildingIndex,
    CreativeEditorWorldLayoutGeneratedBuildingOperation operation,
    std::int64_t deltaXCells, std::int64_t deltaZCells) {
  GeneratedBuildingCandidate candidate = makeGeneratedBuildingCandidate(
      state, buildingIndex, operation, deltaXCells, deltaZCells);
  return previewWorldLayoutSettingsCandidate(
      state, document, std::move(candidate.state), candidate.edit,
      "building operation preview ready");
}

CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutGeneratedBuildingOperationToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t buildingIndex,
    CreativeEditorWorldLayoutGeneratedBuildingOperation operation,
    std::int64_t deltaXCells, std::int64_t deltaZCells) {
  GeneratedBuildingCandidate candidate = makeGeneratedBuildingCandidate(
      state, buildingIndex, operation, deltaXCells, deltaZCells);
  return applyWorldLayoutSettingsCandidate(
      state, appState, std::move(candidate.state), candidate.edit,
      "desktop_generated_building_operation", "building updated in 3D");
}

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
previewCreativeEditorWorldLayoutLevelSettings(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document, std::size_t levelIndex,
    CreativeEditorWorldLayoutLevelSettings settings) {
  CreativeEditorWorldLayoutState candidate =
      makeWorldLayoutSettingsCandidate(state);
  const CreativeEditorWorldLayoutEditReceipt editReceipt =
      setCreativeEditorWorldLayoutLevelSettings(candidate, levelIndex,
                                                settings);
  return previewWorldLayoutSettingsCandidate(
      state, document, std::move(candidate), editReceipt,
      "level shell preview ready");
}

CreativeEditorWorldLayoutApplyReceipt
applyCreativeEditorWorldLayoutLevelSettingsToDocument(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState,
    std::size_t levelIndex, CreativeEditorWorldLayoutLevelSettings settings) {
  CreativeEditorWorldLayoutState candidate =
      makeWorldLayoutSettingsCandidate(state);
  const CreativeEditorWorldLayoutEditReceipt editReceipt =
      setCreativeEditorWorldLayoutLevelSettings(candidate, levelIndex,
                                                settings);
  return applyWorldLayoutSettingsCandidate(
      state, appState, std::move(candidate), editReceipt,
      "desktop_generated_level_settings", "level shell updated in 3D");
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
