#include "EditorWorldLayoutHistory.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include <optional>
#include <sstream>
#include <string>
#include <utility>

#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] bool validSnapshotMetadata(
    const CreativeEditorWorldLayoutSnapshot& snapshot) noexcept {
  return snapshot.revision > 0U && snapshot.savedRevision > 0U &&
         snapshot.generatedRevision <= snapshot.revision &&
         snapshot.nextStableOrdinal > 0U;
}

[[nodiscard]] bool validSelection(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutSelection selection) noexcept {
  switch (selection.kind) {
    case CreativeEditorWorldLayoutSelectionKind::None:
      return true;
    case CreativeEditorWorldLayoutSelectionKind::Level:
      return selection.index < state.source.levels.size();
    case CreativeEditorWorldLayoutSelectionKind::Room:
      return selection.index < state.source.rooms.size();
    case CreativeEditorWorldLayoutSelectionKind::TopologyEdge:
      return selection.index < state.source.topologyEdges.size();
    case CreativeEditorWorldLayoutSelectionKind::VerticalConnector:
      return selection.index < state.source.verticalConnectors.size();
    case CreativeEditorWorldLayoutSelectionKind::Box:
      return selection.index < state.source.boxes.size();
    case CreativeEditorWorldLayoutSelectionKind::Wall:
      return selection.index < state.source.walls.size();
    case CreativeEditorWorldLayoutSelectionKind::Opening:
      return selection.index < state.source.openings.size();
    case CreativeEditorWorldLayoutSelectionKind::RoofAperture:
      return selection.index < state.source.roofApertures.size();
    case CreativeEditorWorldLayoutSelectionKind::Building:
      return selection.index < state.source.buildings.size();
    case CreativeEditorWorldLayoutSelectionKind::TerrainProfile:
      return selection.index < state.source.terrainProfiles.size();
    case CreativeEditorWorldLayoutSelectionKind::TerrainPath:
      return selection.index < state.source.terrainPaths.size();
    case CreativeEditorWorldLayoutSelectionKind::Object:
      return selection.index < state.source.objects.size();
  }
  return false;
}

[[nodiscard]] bool equivalentSnapshot(
    const CreativeEditorWorldLayoutSnapshot& lhs,
    const CreativeEditorWorldLayoutSnapshot& rhs) {
  cr::CreativeHistorySidecar lhsSidecar;
  cr::CreativeHistorySidecar rhsSidecar;
  return encodeCreativeEditorWorldLayoutHistorySidecar(lhs, lhsSidecar) &&
         encodeCreativeEditorWorldLayoutHistorySidecar(rhs, rhsSidecar) &&
         lhsSidecar == rhsSidecar;
}

void installSourceHistoryEntry(
    CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutSourceHistoryEntry& entry) {
  state.sourceEpoch = detail::nextWorldLayoutSourceEpoch(state.sourceEpoch);
  state.source = entry.snapshot.source;
  state.revision = entry.snapshot.revision;
  state.savedRevision = entry.snapshot.savedRevision;
  state.generatedRevision = entry.snapshot.generatedRevision;
  state.nextStableOrdinal = entry.snapshot.nextStableOrdinal;
  if (state.revision == state.generatedRevision) {
    state.generatedBaseline = entry.snapshot;
  }
  state.selection = validSelection(state, entry.selection)
                        ? entry.selection
                        : CreativeEditorWorldLayoutSelection{};
  state.activeLevelIndex = entry.activeLevelIndex;
  repairCreativeEditorWorldLayoutActiveLevel(state);
  state.anchorActive = false;
  detail::clearWorldLayoutInteraction(state);
  detail::invalidateWorldLayoutPreview(state);
  state.elevationCache = {};
  state.diagnosticCache = {};
}

[[nodiscard]] cr::CreativeHistoryApplyReceipt applySourceHistory(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeAppState& appState,
    cr::CreativeHistoryDirection direction) {
  cr::CreativeHistoryApplyReceipt receipt;
  receipt.requested = true;
  receipt.direction = direction;
  receipt.documentId = appState.facade.document().id();
  receipt.revisionBefore = appState.facade.document().revision();
  receipt.revisionAfter = receipt.revisionBefore;
  receipt.objectCountBefore = appState.facade.document().objectCount();
  receipt.objectCountAfter = receipt.objectCountBefore;
  receipt.undoDepthBefore = state.sourceHistory.undoEntries.size();
  receipt.redoDepthBefore = state.sourceHistory.redoEntries.size();

  const bool undo = direction == cr::CreativeHistoryDirection::Undo;
  auto& sourceEntries = undo ? state.sourceHistory.undoEntries
                             : state.sourceHistory.redoEntries;
  auto& destinationEntries = undo ? state.sourceHistory.redoEntries
                                  : state.sourceHistory.undoEntries;
  if (sourceEntries.empty()) {
    receipt.status = cr::CreativeHistoryStatus::Empty;
    receipt.reasonCode = undo
                             ? "creative_world_layout_source_undo_empty"
                             : "creative_world_layout_source_redo_empty";
    receipt.undoDepthAfter = receipt.undoDepthBefore;
    receipt.redoDepthAfter = receipt.redoDepthBefore;
    return receipt;
  }

  CreativeEditorWorldLayoutSourceHistoryEntry target =
      std::move(sourceEntries.back());
  sourceEntries.pop_back();
  CreativeEditorWorldLayoutSourceHistoryEntry current =
      std::move(state.sourceHistory.current);
  current.selection = state.selection;
  current.activeLevelIndex = state.activeLevelIndex;
  current.source = target.source;
  detail::appendWorldLayoutSourceHistoryEntry(
      destinationEntries, std::move(current), state.sourceHistory.maxDepth);

  receipt.hadSnapshot = true;
  receipt.source = target.source;
  installSourceHistoryEntry(state, target);
  state.sourceHistory.current = std::move(target);
  state.sourceHistory.current.source.clear();
  state.statusMessage = undo ? "layout edit undone: " + receipt.source
                             : "layout edit redone: " + receipt.source;
  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = cr::CreativeHistoryStatus::Applied;
  receipt.reasonCode = undo
                           ? "creative_world_layout_source_undo_applied"
                           : "creative_world_layout_source_redo_applied";
  receipt.undoDepthAfter = state.sourceHistory.undoEntries.size();
  receipt.redoDepthAfter = state.sourceHistory.redoEntries.size();
  return receipt;
}

}  // namespace

CreativeEditorWorldLayoutSnapshot captureCreativeEditorWorldLayoutSnapshot(
    const CreativeEditorWorldLayoutState& state) {
  return {state.source, state.revision, state.savedRevision,
          state.generatedRevision, state.nextStableOrdinal};
}

bool encodeCreativeEditorWorldLayoutHistorySidecar(
    const CreativeEditorWorldLayoutSnapshot& snapshot,
    cr::CreativeHistorySidecar& output) {
  if (!validSnapshotMetadata(snapshot)) {
    return false;
  }
  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(snapshot.source);
  if (!encoded.accepted) {
    return false;
  }
  std::ostringstream payload;
  payload << snapshot.revision << ' ' << snapshot.savedRevision << ' '
          << snapshot.generatedRevision << ' ' << snapshot.nextStableOrdinal
          << '\n'
          << encoded.encodedText;
  output.type = kCreativeEditorWorldLayoutHistorySidecarType;
  output.version = kCreativeEditorWorldLayoutHistorySidecarVersion;
  output.payload = payload.str();
  return true;
}

bool decodeCreativeEditorWorldLayoutHistorySidecar(
    const cr::CreativeHistorySidecar& sidecar,
    CreativeEditorWorldLayoutSnapshot& output) {
  if (sidecar.type != kCreativeEditorWorldLayoutHistorySidecarType ||
      sidecar.version != kCreativeEditorWorldLayoutHistorySidecarVersion) {
    return false;
  }
  const std::size_t metadataEnd = sidecar.payload.find('\n');
  if (metadataEnd == std::string::npos) {
    return false;
  }
  CreativeEditorWorldLayoutSnapshot decoded;
  std::istringstream metadata(sidecar.payload.substr(0U, metadataEnd));
  if (!(metadata >> decoded.revision >> decoded.savedRevision >>
        decoded.generatedRevision >> decoded.nextStableOrdinal)) {
    return false;
  }
  metadata >> std::ws;
  if (!metadata.eof() || !validSnapshotMetadata(decoded)) {
    return false;
  }
  const cr::CreativeWorldLayoutDecodeResult layout =
      cr::decodeCreativeWorldLayout(
          std::string_view(sidecar.payload).substr(metadataEnd + 1U));
  if (!layout.accepted) {
    return false;
  }
  decoded.source = layout.layout;
  output = std::move(decoded);
  return true;
}

void installCreativeEditorWorldLayoutSnapshot(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutSnapshot snapshot) {
  CreativeEditorWorldLayoutBuildingTemplateLibrary buildingTemplates =
      std::move(state.buildingTemplates);
  const float canvasPixelsPerCell = state.canvasPixelsPerCell;
  const float canvasPanX = state.canvasPanX;
  const float canvasPanZ = state.canvasPanZ;
  const CreativeEditorWorldLayoutViewMode viewMode = state.viewMode;
  const CreativeEditorWorldLayoutElevationAxis elevationAxis =
      state.elevationAxis;
  const float elevationPixelsPerCell = state.elevationPixelsPerCell;
  const float elevationPanHorizontal = state.elevationPanHorizontal;
  const float elevationPanY = state.elevationPanY;
  const std::uint64_t sourceEpoch =
      detail::nextWorldLayoutSourceEpoch(state.sourceEpoch);
  state = {};
  state.sourceEpoch = sourceEpoch;
  state.buildingTemplates = std::move(buildingTemplates);
  state.canvasPixelsPerCell = canvasPixelsPerCell;
  state.canvasPanX = canvasPanX;
  state.canvasPanZ = canvasPanZ;
  state.viewMode = viewMode;
  state.elevationAxis = elevationAxis;
  state.elevationPixelsPerCell = elevationPixelsPerCell;
  state.elevationPanHorizontal = elevationPanHorizontal;
  state.elevationPanY = elevationPanY;
  state.source = std::move(snapshot.source);
  state.revision = snapshot.revision;
  state.savedRevision = snapshot.savedRevision;
  state.generatedRevision = snapshot.generatedRevision;
  state.nextStableOrdinal = snapshot.nextStableOrdinal;
  repairCreativeEditorWorldLayoutActiveLevel(state);
  state.generatedBaseline = captureCreativeEditorWorldLayoutSnapshot(state);
  detail::resetWorldLayoutSourceHistory(state);
  state.statusMessage = "layout restored from history";
}

bool creativeEditorWorldLayoutSourceUndoAvailable(
    const CreativeEditorWorldLayoutState& state) noexcept {
  return state.sourceHistory.maxDepth > 0U &&
         !state.sourceHistory.undoEntries.empty();
}

bool creativeEditorWorldLayoutSourceRedoAvailable(
    const CreativeEditorWorldLayoutState& state) noexcept {
  return state.sourceHistory.maxDepth > 0U &&
         !state.sourceHistory.redoEntries.empty();
}

std::uint64_t creativeEditorWorldLayoutSourceUndoDepth(
    const CreativeEditorWorldLayoutState& state) noexcept {
  return state.sourceHistory.undoEntries.size();
}

std::uint64_t creativeEditorWorldLayoutSourceRedoDepth(
    const CreativeEditorWorldLayoutState& state) noexcept {
  return state.sourceHistory.redoEntries.size();
}

cr::CreativeWorldLayoutApplyReceipt applyCreativeEditorWorldLayoutPlanWithHistory(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeAppState& appState,
    const cr::CreativeWorldLayoutPlan& plan,
    CreativeEditorWorldLayoutSnapshot committedSnapshot,
    std::string_view source) {
  cr::CreativeWorldLayoutApplyReceipt receipt;
  receipt.requested = true;
  cr::CreativeHistorySidecar beforeSidecar;
  if (!encodeCreativeEditorWorldLayoutHistorySidecar(state.generatedBaseline,
                                                      beforeSidecar) ||
      !validSnapshotMetadata(committedSnapshot)) {
    receipt.status = cr::CreativeWorldLayoutStatus::InvalidSchema;
    receipt.reasonCode = "creative_world_layout_history_sidecar_invalid";
    return receipt;
  }
  std::optional<cr::CreativeAuthoringOperationRecord> operation =
      cr::makeCreativeWorldLayoutOperationRecord(plan);
  if (!operation.has_value()) {
    receipt.status = cr::CreativeWorldLayoutStatus::InvalidSchema;
    receipt.reasonCode = "creative_world_layout_operation_invalid";
    return receipt;
  }
  cr::CreativeDocumentHistoryTransaction transaction =
      cr::beginCreativeHistoryTransaction(appState.facade, source,
                                          std::move(beforeSidecar),
                                          std::move(operation));
  receipt = cr::applyCreativeWorldLayoutPlan(appState.facade, plan);
  if (!receipt.accepted) {
    cr::cancelCreativeHistoryTransaction(transaction);
    return receipt;
  }

  CreativeEditorWorldLayoutSourceHistory sourceHistory =
      std::move(state.sourceHistory);
  committedSnapshot.generatedRevision = committedSnapshot.revision;
  installCreativeEditorWorldLayoutSnapshot(state, std::move(committedSnapshot));
  state.generatedBaseline = captureCreativeEditorWorldLayoutSnapshot(state);
  if (!receipt.changed) {
    state.sourceHistory = std::move(sourceHistory);
    state.sourceHistory.current =
        detail::captureWorldLayoutSourceHistoryEntry(state);
    cr::cancelCreativeHistoryTransaction(transaction);
    return receipt;
  }
  receipt.historyReceipt = cr::commitCreativeHistoryTransaction(
      appState.history, std::move(transaction), appState.facade);
  if (!receipt.historyReceipt.accepted || !receipt.historyReceipt.recorded) {
    receipt.reasonCode = std::string(receipt.historyReceipt.reasonCode);
  }
  return receipt;
}

cr::CreativeHistoryApplyReceipt applyCreativeEditorWorldLayoutHistory(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeAppState& appState,
    cr::CreativeHistoryDirection direction) {
  cr::CreativeHistoryApplyReceipt rejected;
  rejected.requested = true;
  rejected.direction = direction;
  rejected.status = cr::CreativeHistoryStatus::InvalidDocument;
  rejected.reasonCode = "creative_world_layout_history_sidecar_invalid";

  const bool sourceHistoryAvailable =
      direction == cr::CreativeHistoryDirection::Undo
          ? creativeEditorWorldLayoutSourceUndoAvailable(state)
          : creativeEditorWorldLayoutSourceRedoAvailable(state);
  if (sourceHistoryAvailable) {
    return applySourceHistory(state, appState, direction);
  }
  if (state.generatedRevision != state.revision) {
    rejected.reasonCode =
        "creative_world_layout_history_unsynchronized_source";
    return rejected;
  }

  CreativeEditorWorldLayoutSnapshot targetSnapshot;
  const cr::CreativeHistorySidecar* targetSidecar =
      cr::creativeHistoryTargetSidecar(appState.history, direction);
  if (targetSidecar == nullptr) {
    return cr::applyCreativeHistory(appState.facade, appState.history,
                                    direction);
  }
  if (!decodeCreativeEditorWorldLayoutHistorySidecar(*targetSidecar,
                                                     targetSnapshot)) {
    return rejected;
  }

  cr::CreativeHistorySidecar currentSidecar;
  if (!encodeCreativeEditorWorldLayoutHistorySidecar(state.generatedBaseline,
                                                      currentSidecar)) {
    return rejected;
  }

  cr::CreativeHistoryApplyReceipt receipt = cr::applyCreativeHistory(
      appState.facade, appState.history, direction, std::move(currentSidecar));
  if (receipt.accepted) {
    CreativeEditorWorldLayoutDeferredSourceHistory deferred =
        std::move(state.deferredSourceHistory);
    if (direction == cr::CreativeHistoryDirection::Undo &&
        !deferred.active &&
        creativeEditorWorldLayoutSourceRedoAvailable(state)) {
      deferred.active = true;
      deferred.history = std::move(state.sourceHistory);
    }
    installCreativeEditorWorldLayoutSnapshot(state, std::move(targetSnapshot));
    if (deferred.active &&
        direction == cr::CreativeHistoryDirection::Redo &&
        equivalentSnapshot(state.generatedBaseline,
                           deferred.history.current.snapshot)) {
      state.sourceHistory = std::move(deferred.history);
    } else {
      state.deferredSourceHistory = std::move(deferred);
    }
  }
  return receipt;
}

}  // namespace iggy3d_creative_app
