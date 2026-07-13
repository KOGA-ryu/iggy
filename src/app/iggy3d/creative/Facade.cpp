#include "app/iggy3d/creative/Facade.hpp"

#include "app/iggy3d/creative/FacadeInternal.hpp"
#include "app/iggy3d/creative/tools/RoomShell.hpp"

#include <span>
#include <limits>
#include <utility>

namespace iggy3d::creative {
namespace {

using facade_internal::objectIdToTargetRef;
using facade_internal::targetRefToObjectId;

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

[[nodiscard]] CreativeGhostReceipt applyFacadeGhostToolIntent(
    CreativeGhostState& state,
    const CreativeToolIntent& intent,
    CreativeSnapSettings snapSettings) noexcept {
  if (intent.kind == CreativeToolIntentKind::CancelToolAction) {
    return hideGhost(state);
  }

  return applyGhostToolIntent(state, intent, snapSettings);
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
    State& state,
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
  state.selected = selectionState.selectedTarget;
  clearTargetRefIfMatches(state.hovered, objectId);

  if (ghostState.visible && targetRefMatchesObject(ghostState.target, objectId)) {
    static_cast<void>(hideGhost(ghostState));
  }

  if (targetRefMatchesObject(measurementState.startPoint.target, objectId) ||
      targetRefMatchesObject(measurementState.currentPoint.target, objectId)) {
    static_cast<void>(clearMeasurement(measurementState));
    toolState.measurementActive = false;
  }
}

void resetTransientFacadeState(
    State& state,
    Stats& stats,
    CreativeToolState& toolState,
    CreativeSelectionState& selectionState,
    CreativeMeasurementState& measurementState,
    CreativeSnapSettings& snapSettings,
    CreativeGhostState& ghostState) noexcept {
  state = State{};
  resetStats(stats);
  toolState = makeDefaultCreativeToolState();
  selectionState = makeDefaultCreativeSelectionState();
  measurementState = makeDefaultCreativeMeasurementState();
  snapSettings = makeDefaultCreativeSnapSettings();
  ghostState = makeDefaultCreativeGhostState();
}

[[nodiscard]] bool hasSelectionState(
    const State& state,
    const CreativeSelectionState& selectionState) noexcept {
  return state.selected.value != kInvalidId ||
         selectedTargetCount(selectionState) > 0 ||
         selectionState.candidateTarget.value != kInvalidId;
}

[[nodiscard]] std::vector<CreativeObjectId> selectedObjectIds(
    const CreativeSelectionState& selectionState) {
  std::vector<CreativeObjectId> objectIds;
  const std::span<const TargetRef> targets = selectedTargetList(selectionState);
  objectIds.reserve(targets.empty() ? 1U : targets.size());
  if (targets.empty()) {
    CreativeObjectId objectId = kInvalidObjectId;
    if (targetRefToObjectId(selectionState.selectedTarget, objectId)) {
      objectIds.push_back(objectId);
    }
    return objectIds;
  }
  for (TargetRef target : targets) {
    CreativeObjectId objectId = kInvalidObjectId;
    if (targetRefToObjectId(target, objectId)) {
      objectIds.push_back(objectId);
    }
  }
  return objectIds;
}

[[nodiscard]] bool hasMeasurementState(
    const CreativeMeasurementState& measurementState,
    const CreativeToolState& toolState) noexcept {
  return measurementState.active || measurementState.hasMeasurement ||
         toolState.measurementActive;
}

[[nodiscard]] bool hasToolPointerState(
    const State& state,
    const CreativeToolState& toolState) noexcept {
  return state.hovered.value != kInvalidId ||
         toolState.pointer.target.value != kInvalidId ||
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

[[nodiscard]] CreativeFacadeMutationReceipt toggleSelectedObjectMutation(
    CreativeDocument& document,
    TargetRef selectedTarget,
    CreativeMutationKind mutationKind) {
  CreativeFacadeMutationReceipt receipt;
  receipt.requested = true;
  receipt.target = selectedTarget;
  receipt.revisionBefore = document.revision();
  receipt.revisionAfter = receipt.revisionBefore;

  if (receipt.target.value == kInvalidId) {
    receipt.status = CreativeFacadeMutationStatus::NoSelection;
    receipt.message = "no_selection";
    return receipt;
  }

  receipt.hadSelection = true;
  receipt.mutationKind = mutationKind;

  CreativeObjectId objectId = kInvalidObjectId;
  if (!targetRefToObjectId(receipt.target, objectId)) {
    receipt.status = CreativeFacadeMutationStatus::MissingObject;
    receipt.documentStatus = CreativeDocumentMutationStatus::MissingObject;
    receipt.message = "missing_object";
    return receipt;
  }
  receipt.objectId = objectId;

  const CreativeObject* object = document.findObject(objectId);
  if (object == nullptr) {
    receipt.status = CreativeFacadeMutationStatus::MissingObject;
    receipt.documentStatus = CreativeDocumentMutationStatus::MissingObject;
    receipt.message = "missing_object";
    return receipt;
  }

  receipt.objectKind = object->kind;
  receipt.visibleBefore = object->visible;
  receipt.lockedBefore = object->locked;

  const CreativeDocumentMutationReceipt documentReceipt =
      mutationKind == CreativeMutationKind::SetLocked
          ? setDocumentObjectLocked(document, objectId, !receipt.lockedBefore)
          : setDocumentObjectVisible(document, objectId,
                                     !receipt.visibleBefore);

  receipt.accepted = documentMutationSucceeded(documentReceipt.status);
  receipt.changed = documentReceipt.changed &&
                    documentReceipt.revisionAfter !=
                        documentReceipt.revisionBefore;
  receipt.documentStatus = documentReceipt.status;
  receipt.mutationKind = documentReceipt.mutationKind;
  receipt.revisionBefore = documentReceipt.revisionBefore;
  receipt.revisionAfter = documentReceipt.revisionAfter;
  receipt.message = documentReceipt.message;

  const CreativeObject* objectAfter = document.findObject(objectId);
  if (objectAfter != nullptr) {
    receipt.objectKind = objectAfter->kind;
    receipt.visibleAfter = objectAfter->visible;
    receipt.lockedAfter = objectAfter->locked;
  } else {
    receipt.visibleAfter = receipt.visibleBefore;
    receipt.lockedAfter = receipt.lockedBefore;
  }

  if (documentReceipt.status == CreativeDocumentMutationStatus::Applied &&
      receipt.changed) {
    receipt.status = CreativeFacadeMutationStatus::Applied;
  } else if (documentReceipt.status == CreativeDocumentMutationStatus::NoChange) {
    receipt.status = CreativeFacadeMutationStatus::NoChange;
  } else {
    receipt.status = CreativeFacadeMutationStatus::Rejected;
  }

  return receipt;
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
  resetTransientFacadeState(state_,
                            stats_,
                            toolState_,
                            selectionState_,
                            measurementState_,
                            snapSettings_,
                            ghostState_);
  moveDragActive_ = false;
  moveDragTarget_ = {};
  moveDragObjectId_ = kInvalidObjectId;
  moveDragStartAnchor_ = {};
  moveDragObjects_.clear();
  moveDragReceipt_ = {};
}

void Facade::beginFrame(const FramePacket& packet) noexcept {
  state_.frame = packet.frame;
  state_.flags = packet.flags;
}

void Facade::handle(const Packet& packet) noexcept {
  state_.frame = packet.frame;
  state_.hovered = packet.target;
  state_.flags = packet.flags;
}

const State& Facade::state() const noexcept {
  return state_;
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
    static_cast<void>(cancelMeasurement(measurementState_));
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
    moveDragObjects_.clear();
  }
  state_.tool = toolState_.activeTool;
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
  state_.tool = toolState_.activeTool;
  state_.selected = selectionState_.selectedTarget;
  state_.hovered = toolState_.pointer.target;
  return receipt;
}

CreativeFacadeMutationReceipt Facade::toggleSelectedObjectVisibility() {
  return toggleSelectedObjectMutation(document_,
                                      selectionState_.selectedTarget,
                                      CreativeMutationKind::SetVisible);
}

CreativeFacadeMutationReceipt Facade::toggleSelectedObjectLocked() {
  return toggleSelectedObjectMutation(document_,
                                      selectionState_.selectedTarget,
                                      CreativeMutationKind::SetLocked);
}

CreativeTransformCommandReceipt Facade::transformSelectedObjects(
    const CreativeTransformCommandRequest& request) {
  recordCommandAttempt(stats_);
  const std::vector<CreativeObjectId> objectIds =
      selectedObjectIds(selectionState_);
  CreativeTransformCommandReceipt receipt =
      transformDocumentObjectsAtomically(document_, objectIds, request);
  if (receipt.accepted) {
    recordCommandSuccess(stats_);
  } else {
    recordCommandFailure(stats_);
  }
  return receipt;
}

CreativeSelectionPlacementReceipt Facade::placeObjects(
    std::span<const CreativeObjectId> objectIds,
    const CreativeSelectionPlacementRequest& request) {
  recordCommandAttempt(stats_);
  CreativeSelectionPlacementReceipt receipt =
      placeDocumentObjectsAtomically(document_, objectIds, request);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }

  std::vector<TargetRef> targets;
  targets.reserve(objectIds.size());
  for (CreativeObjectId objectId : objectIds) {
    const TargetRef target = objectIdToTargetRef(objectId);
    if (target.value != kInvalidId) {
      targets.push_back(target);
    }
  }
  const TargetRef primary = targets.empty() ? TargetRef{} : targets.back();
  static_cast<void>(setSelectedTargets(selectionState_, targets, primary));
  state_.selected = selectionState_.selectedTarget;
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeDuplicateCommandReceipt Facade::duplicateSelectedObjects(
    const CreativeDuplicateCommandRequest& request) {
  recordCommandAttempt(stats_);
  const std::vector<CreativeObjectId> objectIds =
      selectedObjectIds(selectionState_);
  if (!objectIds.empty()) {
    const CreativeObjectId nextObjectId = document_.nextObjectId();
    const CreativeObjectId maxTargetId = std::numeric_limits<Id>::max();
    if (nextObjectId > maxTargetId ||
        objectIds.size() - 1U > maxTargetId - nextObjectId) {
      CreativeDuplicateCommandReceipt receipt;
      receipt.requested = true;
      receipt.requestedObjectCount = objectIds.size();
      receipt.status = CreativeDuplicateCommandStatus::InvalidRequest;
      receipt.revisionBefore = document_.revision();
      receipt.revisionAfter = receipt.revisionBefore;
      receipt.message = "duplicate_target_id_exhausted";
      recordCommandFailure(stats_);
      return receipt;
    }
  }
  CreativeDuplicateCommandReceipt receipt =
      duplicateDocumentObjectsAtomically(document_, objectIds, request);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }

  std::vector<TargetRef> duplicateTargets;
  duplicateTargets.reserve(receipt.duplicatedSelectionObjectIds.size());
  for (CreativeObjectId objectId : receipt.duplicatedSelectionObjectIds) {
    const TargetRef target = objectIdToTargetRef(objectId);
    if (target.value != kInvalidId) {
      duplicateTargets.push_back(target);
    }
  }
  for (CreativeObjectId objectId : receipt.duplicatedObjectIds) {
    recordObjectCreated(stats_);
    const CreativeObject* object = document_.findObject(objectId);
    if (object != nullptr && object->kind == CreativeObjectKind::Room) {
      recordRoomCreated(stats_);
    }
  }
  const TargetRef primaryTarget = duplicateTargets.empty()
                                      ? TargetRef{}
                                      : duplicateTargets.back();
  static_cast<void>(setSelectedTargets(selectionState_, duplicateTargets,
                                       primaryTarget));
  state_.selected = selectionState_.selectedTarget;
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeGroupCommandReceipt Facade::groupSelectedObjects() {
  recordCommandAttempt(stats_);
  const std::vector<CreativeObjectId> objectIds =
      selectedObjectIds(selectionState_);
  CreativeGroupCommandReceipt receipt =
      groupDocumentObjectsAtomically(document_, objectIds);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  std::vector<TargetRef> targets;
  targets.reserve(receipt.selectionObjectIds.size());
  for (CreativeObjectId objectId : receipt.selectionObjectIds) {
    const TargetRef target = objectIdToTargetRef(objectId);
    if (target.value != kInvalidId) {
      targets.push_back(target);
    }
  }
  const TargetRef primary = targets.empty() ? TargetRef{} : targets.back();
  static_cast<void>(setSelectedTargets(selectionState_, targets, primary));
  state_.selected = selectionState_.selectedTarget;
  recordObjectCreated(stats_);
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeGroupCommandReceipt Facade::ungroupSelectedObject() {
  CreativeObjectId groupObjectId = kInvalidObjectId;
  if (!targetRefToObjectId(selectionState_.selectedTarget, groupObjectId)) {
    recordCommandAttempt(stats_);
    CreativeGroupCommandReceipt receipt;
    receipt.requested = true;
    receipt.kind = CreativeGroupCommandKind::Ungroup;
    receipt.status = CreativeGroupCommandStatus::EmptySelection;
    receipt.revisionBefore = document_.revision();
    receipt.revisionAfter = receipt.revisionBefore;
    receipt.reasonCode = "creative_ungroup_selection_empty";
    recordCommandFailure(stats_);
    return receipt;
  }
  return ungroupObject(groupObjectId);
}

CreativeGroupCommandReceipt Facade::ungroupObject(
    CreativeObjectId groupObjectId) {
  recordCommandAttempt(stats_);
  CreativeGroupCommandReceipt receipt =
      ungroupDocumentObjectAtomically(document_, groupObjectId);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  std::vector<TargetRef> targets;
  targets.reserve(receipt.selectionObjectIds.size());
  for (CreativeObjectId objectId : receipt.selectionObjectIds) {
    const TargetRef target = objectIdToTargetRef(objectId);
    if (target.value != kInvalidId) {
      targets.push_back(target);
    }
  }
  const TargetRef primary = targets.empty() ? TargetRef{} : targets.back();
  static_cast<void>(setSelectedTargets(selectionState_, targets, primary));
  state_.selected = selectionState_.selectedTarget;
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeLinearArrayReceipt Facade::createLinearArrayFromSelection(
    const CreativeLinearArrayRequest& request) {
  recordCommandAttempt(stats_);
  const std::vector<CreativeObjectId> objectIds =
      selectedObjectIds(selectionState_);
  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(document_, objectIds);
  const std::span<const CreativeObjectId> sourceObjectIds =
      hierarchy.accepted
          ? std::span<const CreativeObjectId>{hierarchy.objectIds}
          : std::span<const CreativeObjectId>{objectIds};
  if (!sourceObjectIds.empty()) {
    CreativeLinearArrayPlanRequest planRequest;
    planRequest.sourceObjectCount = sourceObjectIds.size();
    planRequest.direction = request.direction;
    planRequest.copyCount = request.copyCount;
    planRequest.spacing = request.spacing;
    planRequest.cellSize = request.cellSize;
    planRequest.maxGeneratedObjects = request.maxGeneratedObjects;
    const CreativeLinearArrayPlanReceipt plan =
        planCreativeLinearArray(planRequest);
    if (plan.accepted) {
      const CreativeObjectId nextObjectId = document_.nextObjectId();
      const CreativeObjectId maxTargetId = std::numeric_limits<Id>::max();
      if (nextObjectId > maxTargetId ||
          plan.generatedObjectCount - 1U > maxTargetId - nextObjectId) {
        CreativeLinearArrayReceipt receipt;
        receipt.requested = true;
        receipt.requestedObjectCount = objectIds.size();
        receipt.sourceObjectCount = sourceObjectIds.size();
        receipt.generatedObjectCount = plan.generatedObjectCount;
        receipt.status = CreativeLinearArrayStatus::ObjectIdExhausted;
        receipt.revisionBefore = document_.revision();
        receipt.revisionAfter = receipt.revisionBefore;
        receipt.plan = plan;
        receipt.message = "creative_linear_array_target_id_exhausted";
        recordCommandFailure(stats_);
        return receipt;
      }
    }
  }

  CreativeLinearArrayReceipt receipt =
      createCreativeLinearArrayAtomically(document_, sourceObjectIds, request);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }

  for (CreativeObjectId objectId : receipt.generatedObjectIds()) {
    recordObjectCreated(stats_);
    const CreativeObject* object = document_.findObject(objectId);
    if (object != nullptr && object->kind == CreativeObjectKind::Room) {
      recordRoomCreated(stats_);
    }
  }

  std::vector<TargetRef> finalCopyTargets;
  const CreativeHierarchySelection finalCopyHierarchy =
      resolveCreativeObjectHierarchy(document_, receipt.finalCopyObjectIds());
  const std::span<const CreativeObjectId> finalCopySelectionIds =
      finalCopyHierarchy.accepted
          ? std::span<const CreativeObjectId>{
                finalCopyHierarchy.rootObjectIds}
          : receipt.finalCopyObjectIds();
  finalCopyTargets.reserve(finalCopySelectionIds.size());
  for (CreativeObjectId objectId : finalCopySelectionIds) {
    const TargetRef target = objectIdToTargetRef(objectId);
    if (target.value != kInvalidId) {
      finalCopyTargets.push_back(target);
    }
  }
  const TargetRef primaryTarget = finalCopyTargets.empty()
                                      ? TargetRef{}
                                      : finalCopyTargets.back();
  static_cast<void>(setSelectedTargets(selectionState_, finalCopyTargets,
                                       primaryTarget));
  state_.selected = selectionState_.selectedTarget;
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeRadialArrayReceipt Facade::createRadialArrayFromSelection(
    const CreativeRadialArrayRequest& request) {
  recordCommandAttempt(stats_);
  const std::vector<CreativeObjectId> objectIds =
      selectedObjectIds(selectionState_);
  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(document_, objectIds);
  const std::span<const CreativeObjectId> sourceObjectIds =
      hierarchy.accepted
          ? std::span<const CreativeObjectId>{hierarchy.objectIds}
          : std::span<const CreativeObjectId>{objectIds};
  if (!sourceObjectIds.empty()) {
    CreativeRadialArrayPlanRequest planRequest;
    planRequest.sourceObjectCount = sourceObjectIds.size();
    planRequest.pivot = request.pivot;
    planRequest.axis = request.axis;
    planRequest.instanceCount = request.instanceCount;
    planRequest.sweep = request.sweep;
    planRequest.maxGeneratedObjects = request.maxGeneratedObjects;
    const CreativeRadialArrayPlanReceipt plan =
        planCreativeRadialArray(planRequest);
    if (plan.accepted) {
      const CreativeObjectId nextObjectId = document_.nextObjectId();
      const CreativeObjectId maxTargetId = std::numeric_limits<Id>::max();
      if (nextObjectId > maxTargetId ||
          plan.generatedObjectCount - 1U > maxTargetId - nextObjectId) {
        CreativeRadialArrayReceipt receipt;
        receipt.requested = true;
        receipt.requestedObjectCount = objectIds.size();
        receipt.sourceObjectCount = sourceObjectIds.size();
        receipt.generatedObjectCount = plan.generatedObjectCount;
        receipt.status = CreativeRadialArrayStatus::ObjectIdExhausted;
        receipt.revisionBefore = document_.revision();
        receipt.revisionAfter = receipt.revisionBefore;
        receipt.plan = plan;
        receipt.message = "creative_radial_array_target_id_exhausted";
        recordCommandFailure(stats_);
        return receipt;
      }
    }
  }

  CreativeRadialArrayReceipt receipt =
      createCreativeRadialArrayAtomically(document_, sourceObjectIds, request);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }

  for (CreativeObjectId objectId : receipt.generatedObjectIds()) {
    recordObjectCreated(stats_);
    const CreativeObject* object = document_.findObject(objectId);
    if (object != nullptr && object->kind == CreativeObjectKind::Room) {
      recordRoomCreated(stats_);
    }
  }

  std::vector<TargetRef> finalCopyTargets;
  const CreativeHierarchySelection finalCopyHierarchy =
      resolveCreativeObjectHierarchy(document_, receipt.finalCopyObjectIds());
  const std::span<const CreativeObjectId> finalCopySelectionIds =
      finalCopyHierarchy.accepted
          ? std::span<const CreativeObjectId>{
                finalCopyHierarchy.rootObjectIds}
          : receipt.finalCopyObjectIds();
  finalCopyTargets.reserve(finalCopySelectionIds.size());
  for (CreativeObjectId objectId : finalCopySelectionIds) {
    const TargetRef target = objectIdToTargetRef(objectId);
    if (target.value != kInvalidId) {
      finalCopyTargets.push_back(target);
    }
  }
  const TargetRef primaryTarget = finalCopyTargets.empty()
                                      ? TargetRef{}
                                      : finalCopyTargets.back();
  static_cast<void>(setSelectedTargets(selectionState_, finalCopyTargets,
                                       primaryTarget));
  state_.selected = selectionState_.selectedTarget;
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeClipboardCopyReceipt Facade::copySelectedObjectsToClipboard(
    CreativeClipboard& outClipboard) {
  recordCommandAttempt(stats_);
  const std::vector<CreativeObjectId> objectIds =
      selectedObjectIds(selectionState_);
  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(document_, objectIds);
  CreativeClipboardCopyReceipt receipt = copyDocumentObjectsToClipboard(
      document_, hierarchy.accepted ? hierarchy.objectIds : objectIds,
      outClipboard);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeClipboardCutReceipt Facade::cutSelectedObjectsToClipboard(
    CreativeClipboard& outClipboard) {
  recordCommandAttempt(stats_);
  const std::vector<CreativeObjectId> objectIds =
      selectedObjectIds(selectionState_);
  const CreativeHierarchySelection hierarchy =
      resolveCreativeObjectHierarchy(document_, objectIds);
  const std::span<const CreativeObjectId> cutIds =
      hierarchy.accepted
          ? std::span<const CreativeObjectId>{hierarchy.objectIds}
          : std::span<const CreativeObjectId>{objectIds};
  CreativeClipboardCutReceipt receipt =
      cutDocumentObjectsAtomically(document_, cutIds, outClipboard);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  for (CreativeObjectId objectId : cutIds) {
    invalidateRemovedObjectEditorState(objectId, state_, toolState_,
                                       selectionState_, measurementState_,
                                       ghostState_);
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeClipboardPasteReceipt Facade::pasteClipboard(
    const CreativeClipboard& clipboard,
    const CreativeClipboardPasteRequest& request) {
  recordCommandAttempt(stats_);
  CreativeClipboardPasteReceipt receipt =
      pasteCreativeClipboardAtomically(document_, clipboard, request);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }

  std::vector<TargetRef> pastedTargets;
  const CreativeHierarchySelection pastedHierarchy =
      resolveCreativeObjectHierarchy(document_, receipt.pastedObjectIds);
  const std::span<const CreativeObjectId> pastedSelectionIds =
      pastedHierarchy.accepted
          ? std::span<const CreativeObjectId>{pastedHierarchy.rootObjectIds}
          : std::span<const CreativeObjectId>{receipt.pastedObjectIds};
  pastedTargets.reserve(pastedSelectionIds.size());
  for (CreativeObjectId objectId : pastedSelectionIds) {
    const TargetRef target = objectIdToTargetRef(objectId);
    if (target.value != kInvalidId) {
      pastedTargets.push_back(target);
    }
  }
  for (CreativeObjectId objectId : receipt.pastedObjectIds) {
    recordObjectCreated(stats_);
    const CreativeObject* object = document_.findObject(objectId);
    if (object != nullptr && object->kind == CreativeObjectKind::Room) {
      recordRoomCreated(stats_);
    }
  }
  const TargetRef primary =
      pastedTargets.empty() ? TargetRef{} : pastedTargets.back();
  static_cast<void>(setSelectedTargets(selectionState_, pastedTargets, primary));
  state_.selected = selectionState_.selectedTarget;
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeVolumeOperationReceipt Facade::applyVolumeOperation(
    const CreativeVolumeOperationRequest& request) {
  recordCommandAttempt(stats_);
  CreativeVolumeOperationReceipt receipt =
      executeCreativeVolumeOperation(document_, request);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }

  for (CreativeObjectId objectId : receipt.removedObjectIds) {
    invalidateRemovedObjectEditorState(objectId, state_, toolState_,
                                       selectionState_, measurementState_,
                                       ghostState_);
  }

  std::vector<TargetRef> createdTargets;
  createdTargets.reserve(receipt.createdObjectIds.size());
  for (CreativeObjectId objectId : receipt.createdObjectIds) {
    const TargetRef target = objectIdToTargetRef(objectId);
    if (target.value != kInvalidId) {
      createdTargets.push_back(target);
    }
    recordObjectCreated(stats_);
    const CreativeObject* object = document_.findObject(objectId);
    if (object != nullptr && object->kind == CreativeObjectKind::Room) {
      recordRoomCreated(stats_);
    }
  }
  if (!createdTargets.empty()) {
    const TargetRef primary = createdTargets.back();
    static_cast<void>(
        setSelectedTargets(selectionState_, createdTargets, primary));
    state_.selected = selectionState_.selectedTarget;
  }

  recordCommandSuccess(stats_);
  return receipt;
}

CreativeVoxelMutationReceipt Facade::applyVoxelEdits(
    std::span<const CreativeVoxelEdit> edits) {
  recordCommandAttempt(stats_);
  CreativeVoxelMutationReceipt receipt = document_.applyVoxelEdits(edits);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeTerrainMutationReceipt Facade::applyTerrainControlEdits(
    std::span<const CreativeTerrainControlEdit> edits) {
  recordCommandAttempt(stats_);
  CreativeTerrainMutationReceipt receipt =
      document_.applyTerrainControlEdits(edits);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeTerrainMaterialMutationReceipt Facade::applyTerrainMaterialEdits(
    std::span<const CreativeTerrainMaterialEdit> edits) {
  recordCommandAttempt(stats_);
  CreativeTerrainMaterialMutationReceipt receipt =
      document_.applyTerrainMaterialEdits(edits);
  if (!receipt.accepted) {
    recordCommandFailure(stats_);
    return receipt;
  }
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeDocumentCreateReceipt Facade::createDocumentObject(
    const CreativeDocumentCreateRequest& request) {
  recordCommandAttempt(stats_);
  CreativeDocumentCreateReceipt receipt = document_.createObject(request);
  if (!receipt.accepted || !receipt.objectCreated) {
    recordCommandFailure(stats_);
    return receipt;
  }

  recordCommandSuccess(stats_);
  recordObjectCreated(stats_);
  if (receipt.objectKind == CreativeObjectKind::Room) {
    recordRoomCreated(stats_);
  }
  return receipt;
}

CreativeDocumentCreateReceipt Facade::createDocumentObject(
    CreativeObjectKind kind) {
  CreativeDocumentCreateRequest request;
  request.kind = kind;
  return createDocumentObject(request);
}

CreativeDocumentRemoveReceipt Facade::removeDocumentObject(
    const CreativeDocumentRemoveRequest& request) {
  recordCommandAttempt(stats_);
  const CreativeObject* requestedObject =
      document_.findObject(request.objectId);
  if (requestedObject != nullptr &&
      requestedObject->kind == CreativeObjectKind::Group) {
    CreativeHierarchyRemoveReceipt hierarchy =
        removeCreativeObjectHierarchyAtomically(document_, request.objectId);
    if (!hierarchy.accepted) {
      CreativeDocumentRemoveReceipt rejected;
      rejected.requested = true;
      rejected.objectId = request.objectId;
      rejected.objectKind = requestedObject->kind;
      rejected.objectName = requestedObject->name;
      rejected.revisionBefore = hierarchy.revisionBefore;
      rejected.revisionAfter = hierarchy.revisionAfter;
      const CreativeObject* failedObject =
          document_.findObject(hierarchy.failedObjectId);
      rejected.status = failedObject != nullptr && failedObject->locked
                            ? CreativeDocumentRemoveStatus::LockedObject
                            : CreativeDocumentRemoveStatus::ParentHasChildren;
      rejected.message = hierarchy.reasonCode;
      rejected.reasonCode = hierarchy.reasonCode;
      recordCommandFailure(stats_);
      return rejected;
    }
    for (CreativeObjectId objectId : hierarchy.removedObjectIds) {
      invalidateRemovedObjectEditorState(objectId, state_, toolState_,
                                         selectionState_, measurementState_,
                                         ghostState_);
    }
    recordCommandSuccess(stats_);
    return hierarchy.rootReceipt;
  }
  CreativeDocumentRemoveReceipt receipt = document_.removeDocumentObject(request);
  if (!receipt.accepted || !receipt.objectRemoved) {
    recordCommandFailure(stats_);
    return receipt;
  }

  invalidateRemovedObjectEditorState(receipt.objectId,
                                     state_,
                                     toolState_,
                                     selectionState_,
                                     measurementState_,
                                     ghostState_);
  recordCommandSuccess(stats_);
  return receipt;
}

CreativeDocumentRemoveReceipt Facade::removeDocumentObject(
    CreativeObjectId id) {
  CreativeDocumentRemoveRequest request;
  request.objectId = id;
  return removeDocumentObject(request);
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

  receipt.selectionCleared = hasSelectionState(state_, selectionState_);
  receipt.measurementCleared = hasMeasurementState(measurementState_,
                                                   toolState_);
  receipt.ghostCleared = ghostState_.visible;
  receipt.toolPointerCleared = hasToolPointerState(state_, toolState_);

  document_ = std::move(document);
  resetTransientFacadeState(state_,
                            stats_,
                            toolState_,
                            selectionState_,
                            measurementState_,
                            snapSettings_,
                            ghostState_);
  moveDragActive_ = false;
  moveDragTarget_ = {};
  moveDragObjectId_ = kInvalidObjectId;
  moveDragStartAnchor_ = {};
  moveDragObjects_.clear();
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
