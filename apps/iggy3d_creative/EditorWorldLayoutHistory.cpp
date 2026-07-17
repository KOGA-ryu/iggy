#include "EditorWorldLayoutHistory.hpp"

#include "EditorWorldLayoutInternal.hpp"

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
  state.statusMessage = "layout restored from history";
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
  cr::CreativeDocumentHistoryTransaction transaction =
      cr::beginCreativeHistoryTransaction(appState.facade, source,
                                          std::move(beforeSidecar));
  receipt = cr::applyCreativeWorldLayoutPlan(appState.facade, plan);
  if (!receipt.accepted) {
    cr::cancelCreativeHistoryTransaction(transaction);
    return receipt;
  }

  committedSnapshot.generatedRevision = committedSnapshot.revision;
  installCreativeEditorWorldLayoutSnapshot(state, std::move(committedSnapshot));
  state.generatedBaseline = captureCreativeEditorWorldLayoutSnapshot(state);
  if (!receipt.changed) {
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

  CreativeEditorWorldLayoutSnapshot targetSnapshot;
  const cr::CreativeHistorySidecar* targetSidecar =
      cr::creativeHistoryTargetSidecar(appState.history, direction);
  if (targetSidecar == nullptr) {
    return cr::applyCreativeHistory(appState.facade, appState.history,
                                    direction);
  }
  if (state.generatedRevision != state.revision) {
    rejected.reasonCode =
        "creative_world_layout_history_unsynchronized_source";
    return rejected;
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
    installCreativeEditorWorldLayoutSnapshot(state, std::move(targetSnapshot));
  }
  return receipt;
}

}  // namespace iggy3d_creative_app
