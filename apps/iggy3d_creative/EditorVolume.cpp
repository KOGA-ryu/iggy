#include "EditorVolume.hpp"

#include <SDL3/SDL_log.h>

#include <cmath>
#include <string>
#include <utility>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/history/History.hpp"

namespace iggy3d_creative_app {
namespace creative = iggy3d::creative;
namespace {

[[nodiscard]] bool sameGrid(CreativeEditorVolumeState& state,
                            double cellSize,
                            creative::CreativeVec3 origin) noexcept {
  return std::isfinite(cellSize) &&
         state.selection.cellSize == cellSize &&
         state.selection.origin.x == origin.x &&
         state.selection.origin.y == origin.y &&
         state.selection.origin.z == origin.z;
}

[[nodiscard]] bool preflightEditorShapeOperation(
    const creative::CreativeDocument& document,
    const creative::CreativeVolumeOperationRequest& request,
    creative::CreativeVolumeOperationReceipt& receipt) {
  const bool shapeOperation =
      request.operation == creative::CreativeVolumeOperationKind::Fill ||
      request.operation == creative::CreativeVolumeOperationKind::Hollow;
  if (!shapeOperation || !document.isValid() ||
      document.id() == creative::kInvalidDocumentId ||
      !creative::creativeVolumeSelectionValid(request.selection) ||
      request.maxAffectedObjects == 0U) {
    return false;
  }

  creative::CreativeShapeBrushPlanRequest planRequest;
  planRequest.kind = request.shapeKind;
  planRequest.axis = request.shapeAxis;
  planRequest.firstCell = request.selection.firstCell;
  planRequest.secondCell = request.selection.secondCell;
  planRequest.hollow =
      request.operation == creative::CreativeVolumeOperationKind::Hollow;
  planRequest.maxCandidateCellCount = request.maxAffectedObjects;
  planRequest.maxGeneratedCellCount = request.maxAffectedObjects;
  const creative::CreativeShapeBrushPlanReceipt plan =
      creative::planCreativeShapeBrush(planRequest);
  if (plan.accepted) {
    return false;
  }

  const bool limitExceeded =
      plan.status ==
          creative::CreativeShapeBrushPlanStatus::CandidateLimitExceeded ||
      plan.status ==
          creative::CreativeShapeBrushPlanStatus::GeneratedLimitExceeded;
  receipt = {};
  receipt.requested = true;
  receipt.operation = request.operation;
  receipt.shapeKind = request.shapeKind;
  receipt.shapeAxis = request.shapeAxis;
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = document.revision();
  receipt.volumeCellCount =
      creative::creativeVolumeCellCount(request.selection);
  receipt.shapeCandidateCellCount = plan.candidateCellCount;
  receipt.plannedCellCount = plan.generatedCellCount;
  receipt.status =
      limitExceeded
          ? creative::CreativeVolumeOperationStatus::OperationLimitExceeded
          : creative::CreativeVolumeOperationStatus::InvalidRequest;
  receipt.reasonCode = limitExceeded
                           ? "creative_volume_shape_limit_exceeded"
                           : "creative_volume_shape_plan_invalid";
  return true;
}

}  // namespace

void activateCreativeEditorVolumeMode(CreativeEditorVolumeState& state,
                                      double cellSize,
                                      creative::CreativeVec3 origin) {
  if (!sameGrid(state, cellSize, origin)) {
    creative::clearCreativeVolumeSelection(state.selection);
    state.selection.cellSize = cellSize;
    state.selection.origin = origin;
    state.cursorValid = false;
  }
  state.active = true;
  state.lastReceipt = {};
}

void deactivateCreativeEditorVolumeMode(
  CreativeEditorVolumeState& state) noexcept {
  state.active = false;
}

creative::CreativeVolumeSelection creativeEditorVolumePreviewSelection(
    const CreativeEditorVolumeState& state) noexcept {
  if (!state.active || !state.cursorValid) {
    return state.selection;
  }
  return creative::previewCreativeVolumeSelection(state.selection,
                                                   state.cursorCell);
}

CreativeEditorVolumeGestureReceipt stepCreativeEditorVolumeGesture(
    CreativeEditorVolumeState& state,
    CreativeEditorVolumeGestureAction action,
    bool targetValid,
    creative::CreativeGridCoord3 targetCell) noexcept {
  CreativeEditorVolumeGestureReceipt receipt;
  receipt.requested = true;
  receipt.action = action;
  receipt.phaseBefore = state.selection.phase;
  receipt.phaseAfter = state.selection.phase;
  if (!targetValid) {
    return receipt;
  }

  switch (action) {
    case CreativeEditorVolumeGestureAction::Begin:
      creative::clearCreativeVolumeSelection(state.selection);
      static_cast<void>(creative::setCreativeVolumeSelectionCorner(
          state.selection, creative::CreativeVolumeCorner::First, targetCell));
      state.lastReceipt = {};
      receipt.accepted = true;
      receipt.status = CreativeEditorVolumeGestureStatus::Began;
      receipt.reasonCode = "creative_volume_gesture_began";
      break;
    case CreativeEditorVolumeGestureAction::Commit:
      if (state.selection.phase !=
          creative::CreativeVolumeSelectionPhase::FirstCorner) {
        receipt.status = CreativeEditorVolumeGestureStatus::NotArmed;
        receipt.reasonCode = "creative_volume_gesture_not_armed";
        break;
      }
      static_cast<void>(creative::setCreativeVolumeSelectionCorner(
          state.selection, creative::CreativeVolumeCorner::Second, targetCell));
      state.lastReceipt = {};
      receipt.accepted = true;
      receipt.status = CreativeEditorVolumeGestureStatus::Completed;
      receipt.reasonCode = "creative_volume_gesture_completed";
      break;
  }
  receipt.phaseAfter = state.selection.phase;
  return receipt;
}

creative::CreativeVolumeOperationReceipt
applyCreativeEditorVolumeOperationWithHistory(
    creative::CreativeAppState& appState,
    CreativeEditorVolumeState& state,
    creative::CreativeObjectKind brushKind,
    creative::CreativeVolumeOperationKind operation,
    const creative::CreativeToolSettings& toolSettings,
    std::string_view source) {
  creative::CreativeVolumeOperationRequest request;
  request.operation = operation;
  request.selection = state.selection;
  request.objectKind = brushKind;
  request.maxAffectedObjects = kCreativeEditorVolumeCellLimit;
  if (!creative::applyCreativeToolSettingsToVolumeRequest(request,
                                                           toolSettings)) {
    // Preserve the Facade's single receipt/history path while forcing its
    // existing atomic request validator to reject malformed option state.
    request.maxAffectedObjects = 0U;
  }

  creative::CreativeHistoryRecordReceipt historyReceipt;
  if (!preflightEditorShapeOperation(appState.facade.document(), request,
                                     state.lastReceipt)) {
    creative::CreativeDocumentHistoryTransaction transaction =
        creative::beginCreativeHistoryTransaction(appState.facade, source);
    state.lastReceipt = appState.facade.applyVolumeOperation(request);
    historyReceipt = creative::commitCreativeHistoryTransaction(
        appState.history, std::move(transaction), appState.facade);
  }

  SDL_Log("iggy3d_creative: VOLUME operation='%s' shape='%s' axis='%s' "
          "status='%s' accepted=%d changed=%d candidates=%llu planned=%llu "
          "matchedObjects=%llu matchedVoxels=%llu createdObjects=%llu "
          "removedObjects=%llu createdVoxels=%llu removedVoxels=%llu "
          "replacedVoxels=%llu dirtyChunks=%llu undoRecorded=%d "
          "reasonCode='%s'",
          std::string(creative::toString(operation)).c_str(),
          std::string(creative::toString(state.lastReceipt.shapeKind)).c_str(),
          std::string(creative::toString(state.lastReceipt.shapeAxis)).c_str(),
          std::string(creative::toString(state.lastReceipt.status)).c_str(),
          state.lastReceipt.accepted ? 1 : 0,
          state.lastReceipt.changed ? 1 : 0,
          static_cast<unsigned long long>(
              state.lastReceipt.shapeCandidateCellCount),
          static_cast<unsigned long long>(state.lastReceipt.plannedCellCount),
          static_cast<unsigned long long>(state.lastReceipt.matchedObjectCount),
          static_cast<unsigned long long>(
              state.lastReceipt.matchedVoxelCellCount),
          static_cast<unsigned long long>(state.lastReceipt.createdObjectCount),
          static_cast<unsigned long long>(state.lastReceipt.removedObjectCount),
          static_cast<unsigned long long>(
              state.lastReceipt.createdVoxelCellCount),
          static_cast<unsigned long long>(
              state.lastReceipt.removedVoxelCellCount),
          static_cast<unsigned long long>(
              state.lastReceipt.replacedVoxelCellCount),
          static_cast<unsigned long long>(
              state.lastReceipt.dirtyVoxelChunkCount),
          historyReceipt.recorded ? 1 : 0,
          std::string(state.lastReceipt.reasonCode).c_str());
  return state.lastReceipt;
}

}  // namespace iggy3d_creative_app
