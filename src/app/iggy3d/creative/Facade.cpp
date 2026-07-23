#include "app/iggy3d/creative/Facade.hpp"

#include "app/iggy3d/creative/FacadeInternal.hpp"

#include <span>
#include <utility>

namespace iggy3d::creative {
namespace facade_internal {

void resetStats(Stats& stats) noexcept {
  stats = Stats{};
}

void recordCommandAttempt(Stats& stats) noexcept {
  ++stats.commandAttempts;
}

void recordCommandSuccess(Stats& stats) noexcept {
  ++stats.commandSuccesses;
}

void recordCommandFailure(Stats& stats) noexcept {
  ++stats.commandFailures;
}

void recordObjectCreated(Stats& stats) noexcept {
  ++stats.objectsCreated;
}

void recordRoomCreated(Stats& stats) noexcept {
  ++stats.roomsCreated;
}

[[nodiscard]] bool targetRefMatchesObject(TargetRef target,
                                          CreativeObjectId objectId) noexcept {
  CreativeObjectId targetObjectId = kInvalidObjectId;
  return targetRefToObjectId(target, targetObjectId) &&
         targetObjectId == objectId;
}

void clearTargetRefIfMatches(TargetRef& target,
                             CreativeObjectId objectId) noexcept {
  if (targetRefMatchesObject(target, objectId)) {
    target = {};
  }
}

void invalidateRemovedObjectEditorState(
    CreativeObjectId objectId,
    CreativeToolState& toolState,
    CreativeSelectionState& selectionState,
    CreativeMeasurementState& measurementState,
    CreativeGhostState& ghostState) noexcept {
  static_cast<void>(removeSelectedTarget(
      selectionState, objectIdToTargetRef(objectId)));
  if (targetRefMatchesObject(selectionState.candidateTarget, objectId)) {
    static_cast<void>(updateSelectionCandidate(selectionState, {}));
  }

  clearTargetRefIfMatches(toolState.pointer.target, objectId);

  if (ghostState.visible && targetRefMatchesObject(ghostState.target, objectId)) {
    static_cast<void>(hideGhost(ghostState));
  }

  if (targetRefMatchesObject(measurementState.startPoint.target, objectId) ||
      targetRefMatchesObject(measurementState.currentPoint.target, objectId)) {
    static_cast<void>(clearMeasurement(measurementState));
    toolState.measurementActive = false;
  }
}

}  // namespace facade_internal
namespace {

using facade_internal::resetStats;

[[nodiscard]] CreativeGhostReceipt applyFacadeGhostToolIntent(
    CreativeGhostState& state,
    const CreativeToolIntent& intent,
    CreativeSnapSettings snapSettings) noexcept {
  if (intent.kind == CreativeToolIntentKind::CancelToolAction) {
    return hideGhost(state);
  }

  return applyGhostToolIntent(state, intent, snapSettings);
}

void resetTransientFacadeState(
    Stats& stats,
    CreativeToolState& toolState,
    CreativeSelectionState& selectionState,
    CreativeMeasurementState& measurementState,
    CreativeSnapSettings& snapSettings,
    CreativeGhostState& ghostState) noexcept {
  resetStats(stats);
  toolState = makeDefaultCreativeToolState();
  selectionState = makeDefaultCreativeSelectionState();
  measurementState = makeDefaultCreativeMeasurementState();
  snapSettings = makeDefaultCreativeSnapSettings();
  ghostState = makeDefaultCreativeGhostState();
}

[[nodiscard]] bool hasSelectionState(
    const CreativeSelectionState& selectionState) noexcept {
  return selectedTargetCount(selectionState) > 0 ||
         selectionState.candidateTarget.value != kInvalidId;
}

[[nodiscard]] bool hasMeasurementState(
    const CreativeMeasurementState& measurementState,
    const CreativeToolState& toolState) noexcept {
  return measurementState.active || measurementState.hasMeasurement ||
         toolState.measurementActive;
}

[[nodiscard]] bool hasToolPointerState(
    const CreativeToolState& toolState) noexcept {
  return toolState.pointer.target.value != kInvalidId ||
         toolState.pointer.x != 0.0 || toolState.pointer.y != 0.0 ||
         toolState.pointer.button != CreativeToolPointerButton::None ||
         toolState.pointer.modifiers != kCreativeToolModifierNone;
}

void setInstallStatus(CreativeFacadeDocumentInstallReceipt& receipt,
                      std::string_view status) noexcept {
  receipt.status = status;
  receipt.reasonCode = status;
  receipt.message = status;
}

void setBatchCreateStatus(CreativeFacadeDocumentBatchCreateReceipt& receipt,
                          CreativeFacadeDocumentBatchCreateStatus status,
                          std::string_view reason) noexcept {
  receipt.status = status;
  receipt.reasonCode = reason;
  receipt.message = reason;
}

}  // namespace

std::string_view toString(CreativeFacadeMutationStatus status) noexcept {
  switch (status) {
    case CreativeFacadeMutationStatus::Unknown:
      return "Unknown";
    case CreativeFacadeMutationStatus::NoSelection:
      return "NoSelection";
    case CreativeFacadeMutationStatus::MissingObject:
      return "MissingObject";
    case CreativeFacadeMutationStatus::Applied:
      return "Applied";
    case CreativeFacadeMutationStatus::NoChange:
      return "NoChange";
    case CreativeFacadeMutationStatus::Rejected:
      return "Rejected";
  }
  return "Unknown";
}

std::string_view toString(CreativeFacadeDocumentBatchCreateStatus status)
    noexcept {
  switch (status) {
    case CreativeFacadeDocumentBatchCreateStatus::Unknown:
      return "Unknown";
    case CreativeFacadeDocumentBatchCreateStatus::Empty:
      return "Empty";
    case CreativeFacadeDocumentBatchCreateStatus::CreateRejected:
      return "CreateRejected";
    case CreativeFacadeDocumentBatchCreateStatus::InstallRejected:
      return "InstallRejected";
    case CreativeFacadeDocumentBatchCreateStatus::Applied:
      return "Applied";
  }
  return "Unknown";
}

void Facade::reset() noexcept {
  document_.reset();
  resetTransientFacadeState(stats_,
                            toolState_,
                            selectionState_,
                            measurementState_,
                            snapSettings_,
                            ghostState_);
  moveDragActive_ = false;
  moveDragTarget_ = {};
  moveDragObjectId_ = kInvalidObjectId;
  moveDragStartAnchor_ = {};
  moveDragObjectIds_.clear();
  moveDragReceipt_ = {};
}

const CreativeToolState& Facade::toolState() const noexcept {
  return toolState_;
}

const CreativeSelectionState& Facade::selectionState() const noexcept {
  return selectionState_;
}

const CreativeMeasurementState& Facade::measurementState() const noexcept {
  return measurementState_;
}

const CreativeSnapSettings& Facade::snapSettings() const noexcept {
  return snapSettings_;
}

const CreativeGhostState& Facade::ghostState() const noexcept {
  return ghostState_;
}

const CreativeFacadeMoveDragReceipt& Facade::moveDragReceipt() const noexcept {
  return moveDragReceipt_;
}

bool Facade::setActiveTool(Tool tool) noexcept {
  const Tool activeToolBefore = toolState_.activeTool;
  const bool changed = iggy3d::creative::setActiveTool(toolState_, tool);
  if (changed && activeToolBefore == Tool::Measure &&
      toolState_.activeTool != Tool::Measure && measurementState_.active) {
    static_cast<void>(
        iggy3d::creative::cancelMeasurement(measurementState_));
  }
  if (changed && ghostState_.visible) {
    static_cast<void>(hideGhost(ghostState_));
  }
  if (changed) {
    // A tool switch abandons any Move drag in flight (mirrors the tool core).
    moveDragActive_ = false;
    moveDragTarget_ = {};
    moveDragObjectId_ = kInvalidObjectId;
    moveDragStartAnchor_ = {};
    moveDragObjectIds_.clear();
  }
  return changed;
}

void Facade::setSnapSettings(CreativeSnapSettings settings) noexcept {
  snapSettings_ = settings;
}

CreativeFacadeToolDispatchReceipt Facade::dispatchToolInput(
    const CreativeToolInputPacket& input) {
  CreativeFacadeToolDispatchReceipt receipt;
  receipt.inputKind = input.kind;
  receipt.activeToolBefore = toolState_.activeTool;

  const CreativeToolDispatchReceipt toolReceipt =
      iggy3d::creative::dispatchToolInput(toolState_, input);
  receipt.activeToolAfter = toolState_.activeTool;
  receipt.emittedIntentCount = toolReceipt.emittedIntentCount;
  receipt.toolAccepted = toolReceipt.accepted;
  receipt.accepted = toolReceipt.accepted;
  receipt.changed = toolReceipt.changedState;
  receipt.message = toolReceipt.message;

  for (const CreativeToolIntent& intent : toolReceipt.intents) {
    const CreativeSelectionReceipt selectionReceipt =
        applySelectionToolIntent(selectionState_, intent);
    const CreativeMeasurementReceipt measurementReceipt =
        applyMeasurementToolIntent(measurementState_, intent);
    const CreativeGhostReceipt ghostReceipt =
        applyFacadeGhostToolIntent(ghostState_, intent, snapSettings_);

    receipt.selectionChanged = receipt.selectionChanged ||
                               selectionReceipt.changed;
    receipt.measurementChanged = receipt.measurementChanged ||
                                 measurementReceipt.changed;
    receipt.ghostChanged = receipt.ghostChanged || ghostReceipt.changed;

    switch (intent.kind) {
      case CreativeToolIntentKind::BeginMove:
      case CreativeToolIntentKind::PreviewMove:
      case CreativeToolIntentKind::CommitMove:
      case CreativeToolIntentKind::CancelMove: {
        receipt.moveDrag = applyMoveDragIntent(intent);
        receipt.moveDragChanged =
            receipt.moveDragChanged || receipt.moveDrag.changed ||
            receipt.moveDrag.stage == CreativeFacadeMoveDragStage::Begin ||
            receipt.moveDrag.stage == CreativeFacadeMoveDragStage::Cancelled;
        // TD-6: preview during the drag reuses the existing ghost to indicate
        // the destination; commit/cancel clear it. A dedicated wireframe box is
        // deferred to the render slice (TV1-J).
        if (intent.kind == CreativeToolIntentKind::BeginMove ||
            intent.kind == CreativeToolIntentKind::PreviewMove) {
          const CreativeGhostReceipt moveGhost = updateGhostPreview(
              ghostState_, intent.pointer, Tool::Move, snapSettings_);
          receipt.ghostChanged = receipt.ghostChanged || moveGhost.changed;
        } else if (ghostState_.visible) {
          const CreativeGhostReceipt moveGhost = hideGhost(ghostState_);
          receipt.ghostChanged = receipt.ghostChanged || moveGhost.changed;
        }
        break;
      }
      default:
        break;
    }
  }

  if (input.kind == CreativeToolInputKind::Cancel &&
      toolReceipt.intents.empty() && ghostState_.visible) {
    const CreativeGhostReceipt ghostReceipt = hideGhost(ghostState_);
    receipt.ghostChanged = receipt.ghostChanged || ghostReceipt.changed;
  }

  receipt.changed = receipt.changed || receipt.selectionChanged ||
                    receipt.measurementChanged || receipt.ghostChanged ||
                    receipt.moveDragChanged;
  if (receipt.moveDrag.stage != CreativeFacadeMoveDragStage::None) {
    moveDragReceipt_ = receipt.moveDrag;
  }
  toolState_.measurementActive = measurementState_.active;
  return receipt;
}

CreativeMeasurementReceipt Facade::configureMeasurement(
    CreativeMeasurementMode mode,
    CreativeMeasurementAxis axis,
    bool closePath) noexcept {
  const CreativeMeasurementReceipt receipt =
      iggy3d::creative::configureMeasurement(measurementState_, mode, axis,
                                             closePath);
  toolState_.measurementActive = measurementState_.active;
  return receipt;
}

CreativeMeasurementReceipt Facade::appendMeasurementPoint(
    CreativeMeasurementPoint point) noexcept {
  const CreativeMeasurementReceipt receipt =
      iggy3d::creative::appendMeasurementPoint(measurementState_, point);
  toolState_.measurementActive = measurementState_.active;
  return receipt;
}

CreativeMeasurementReceipt Facade::previewMeasurementPoint(
    CreativeMeasurementPoint point) noexcept {
  const CreativeMeasurementReceipt receipt =
      iggy3d::creative::previewMeasurementPoint(measurementState_, point);
  toolState_.measurementActive = measurementState_.active;
  return receipt;
}

CreativeMeasurementReceipt Facade::completeMeasurement() noexcept {
  const CreativeMeasurementReceipt receipt =
      iggy3d::creative::completeMeasurement(measurementState_);
  toolState_.measurementActive = measurementState_.active;
  return receipt;
}

CreativeMeasurementReceipt Facade::cancelMeasurement() noexcept {
  const CreativeMeasurementReceipt receipt =
      iggy3d::creative::cancelMeasurement(measurementState_);
  toolState_.measurementActive = measurementState_.active;
  return receipt;
}

CreativeMeasurementReceipt Facade::clearMeasurement() noexcept {
  const CreativeMeasurementReceipt receipt =
      iggy3d::creative::clearMeasurement(measurementState_);
  toolState_.measurementActive = measurementState_.active;
  return receipt;
}

CreativeFacadeDocumentInstallReceipt Facade::installDocument(
    CreativeDocument document) {
  CreativeFacadeDocumentInstallReceipt receipt;
  receipt.requested = true;
  receipt.hadPreviousDocument = document_.isValid();
  receipt.previousDocumentId = document_.id();
  receipt.nextDocumentId = document.id();
  receipt.previousObjectCount = document_.objectCount();
  receipt.nextObjectCount = document.objectCount();
  receipt.previousDirtyFlags = document_.dirtyFlags();
  receipt.nextDirtyFlags = document.dirtyFlags();
  receipt.activeToolBefore = toolState_.activeTool;
  receipt.activeToolAfter = toolState_.activeTool;

  if (!document.isValid()) {
    setInstallStatus(receipt, "creative_facade_document_invalid");
    return receipt;
  }

  if (document.id() == kInvalidDocumentId) {
    setInstallStatus(receipt, "creative_facade_document_id_missing");
    return receipt;
  }

  receipt.selectionCleared = hasSelectionState(selectionState_);
  receipt.measurementCleared = hasMeasurementState(measurementState_,
                                                   toolState_);
  receipt.ghostCleared = ghostState_.visible;
  receipt.toolPointerCleared = hasToolPointerState(toolState_);

  document_ = std::move(document);
  resetTransientFacadeState(stats_,
                            toolState_,
                            selectionState_,
                            measurementState_,
                            snapSettings_,
                            ghostState_);
  moveDragActive_ = false;
  moveDragTarget_ = {};
  moveDragObjectId_ = kInvalidObjectId;
  moveDragStartAnchor_ = {};
  moveDragObjectIds_.clear();
  moveDragReceipt_ = {};

  receipt.accepted = true;
  receipt.changed = true;
  receipt.nextDocumentId = document_.id();
  receipt.nextObjectCount = document_.objectCount();
  receipt.nextDirtyFlags = document_.dirtyFlags();
  receipt.activeToolAfter = toolState_.activeTool;
  setInstallStatus(receipt, "creative_facade_document_installed");
  return receipt;
}

CreativeFacadeDocumentBatchCreateReceipt Facade::createDocumentObjectsAtomically(
    std::span<const CreativeDocumentCreateRequest> requests) {
  CreativeFacadeDocumentBatchCreateReceipt receipt;
  receipt.requested = true;
  receipt.revisionBefore = document_.revision();
  receipt.revisionAfter = receipt.revisionBefore;
  receipt.attemptedCreateCount = requests.size();

  if (requests.empty()) {
    setBatchCreateStatus(receipt,
                         CreativeFacadeDocumentBatchCreateStatus::Empty,
                         "creative_facade_batch_create_empty");
    return receipt;
  }

  CreativeDocument stagedDocument = document_;
  for (std::size_t index = 0; index < requests.size(); ++index) {
    const CreativeDocumentCreateReceipt createReceipt =
        stagedDocument.createObject(requests[index]);
    if (createReceipt.accepted && createReceipt.objectCreated &&
        createReceipt.changed) {
      ++receipt.appliedCreateCount;
      continue;
    }

    receipt.hasFailedCreate = true;
    receipt.firstFailedCreateIndex = index;
    receipt.firstFailedCreateStatus = createReceipt.status;
    receipt.firstFailedCreateReasonCode = createReceipt.reasonCode;
    receipt.firstFailedCreateMessage = createReceipt.message;
    setBatchCreateStatus(receipt,
                         CreativeFacadeDocumentBatchCreateStatus::
                             CreateRejected,
                         "creative_facade_batch_create_create_rejected");
    return receipt;
  }

  receipt.installAttempted = true;
  receipt.installReceipt = installDocument(std::move(stagedDocument));
  receipt.revisionAfter = document_.revision();
  if (!receipt.installReceipt.accepted || !receipt.installReceipt.changed) {
    setBatchCreateStatus(receipt,
                         CreativeFacadeDocumentBatchCreateStatus::
                             InstallRejected,
                         "creative_facade_batch_create_install_rejected");
    return receipt;
  }

  receipt.accepted = true;
  receipt.changed = true;
  receipt.revisionAfter = document_.revision();
  setBatchCreateStatus(receipt,
                       CreativeFacadeDocumentBatchCreateStatus::Applied,
                       "creative_facade_batch_create_applied");
  return receipt;
}

const CreativeObject* Facade::findObject(CreativeObjectId id) const noexcept {
  return document_.findObject(id);
}

const CreativeDocument& Facade::document() const noexcept {
  return document_;
}

CreativeDocument& Facade::documentForPersistence() noexcept {
  return document_;
}

const Stats& Facade::stats() const noexcept {
  return stats_;
}

}  // namespace iggy3d::creative
