#include "app/iggy3d/creative/Facade.hpp"

#include <limits>
#include <span>
#include <utility>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool targetRefToObjectId(TargetRef target,
                                       CreativeObjectId& objectId) noexcept {
  if (target.value == kInvalidId) {
    return false;
  }

  if constexpr (std::numeric_limits<Id>::max() >
                std::numeric_limits<CreativeObjectId>::max()) {
    if (target.value > std::numeric_limits<CreativeObjectId>::max()) {
      return false;
    }
  }

  objectId = static_cast<CreativeObjectId>(target.value);
  return objectId != kInvalidObjectId;
}

[[nodiscard]] TargetRef objectIdToTargetRef(CreativeObjectId objectId) noexcept {
  if (objectId == kInvalidObjectId ||
      objectId > std::numeric_limits<Id>::max()) {
    return {};
  }

  return TargetRef{static_cast<Id>(objectId)};
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
    CreativeInspectionState& inspectionState,
    CreativeMeasurementState& measurementState,
    CreativeGhostState& ghostState) noexcept {
  if (targetRefMatchesObject(selectionState.selectedTarget, objectId)) {
    static_cast<void>(setSelectedTarget(selectionState, {}));
  }
  if (targetRefMatchesObject(selectionState.candidateTarget, objectId)) {
    static_cast<void>(updateSelectionCandidate(selectionState, {}));
  }

  if (targetRefMatchesObject(inspectionState.inspectedTarget, objectId)) {
    static_cast<void>(setInspectedTarget(inspectionState, {}));
  }
  if (targetRefMatchesObject(inspectionState.candidateTarget, objectId)) {
    static_cast<void>(updateInspectionCandidate(inspectionState, {}));
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
    CreativeInspectionState& inspectionState,
    CreativeMeasurementState& measurementState,
    CreativeSnapSettings& snapSettings,
    CreativeGhostState& ghostState) noexcept {
  state = State{};
  resetStats(stats);
  toolState = makeDefaultCreativeToolState();
  selectionState = makeDefaultCreativeSelectionState();
  inspectionState = makeDefaultCreativeInspectionState();
  measurementState = makeDefaultCreativeMeasurementState();
  snapSettings = makeDefaultCreativeSnapSettings();
  ghostState = makeDefaultCreativeGhostState();
}

[[nodiscard]] bool hasSelectionState(
    const State& state,
    const CreativeSelectionState& selectionState) noexcept {
  return state.selected.value != kInvalidId ||
         selectionState.selectedTarget.value != kInvalidId ||
         selectionState.candidateTarget.value != kInvalidId;
}

[[nodiscard]] bool hasInspectionState(
    const CreativeInspectionState& inspectionState) noexcept {
  return inspectionState.inspectedTarget.value != kInvalidId ||
         inspectionState.candidateTarget.value != kInvalidId;
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

void Facade::reset() noexcept {
  document_.reset();
  resetTransientFacadeState(state_,
                            stats_,
                            toolState_,
                            selectionState_,
                            inspectionState_,
                            measurementState_,
                            snapSettings_,
                            ghostState_);
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

const CreativeInspectionState& Facade::inspectionState() const noexcept {
  return inspectionState_;
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
    const CreativeInspectionReceipt inspectionReceipt =
        applyInspectionToolIntent(inspectionState_, intent);
    const CreativeMeasurementReceipt measurementReceipt =
        applyMeasurementToolIntent(measurementState_, intent);
    const CreativeGhostReceipt ghostReceipt =
        applyFacadeGhostToolIntent(ghostState_, intent, snapSettings_);

    receipt.selectionChanged = receipt.selectionChanged ||
                               selectionReceipt.changed;
    receipt.inspectionChanged = receipt.inspectionChanged ||
                                inspectionReceipt.changed;
    receipt.measurementChanged = receipt.measurementChanged ||
                                 measurementReceipt.changed;
    receipt.ghostChanged = receipt.ghostChanged || ghostReceipt.changed;
  }

  if (input.kind == CreativeToolInputKind::Cancel &&
      toolReceipt.intents.empty() && ghostState_.visible) {
    const CreativeGhostReceipt ghostReceipt = hideGhost(ghostState_);
    receipt.ghostChanged = receipt.ghostChanged || ghostReceipt.changed;
  }

  receipt.changed = receipt.changed || receipt.selectionChanged ||
                    receipt.inspectionChanged || receipt.measurementChanged ||
                    receipt.ghostChanged;
  state_.tool = toolState_.activeTool;
  state_.selected = selectionState_.selectedTarget;
  state_.hovered = toolState_.pointer.target;
  return receipt;
}

CreativeUiBuildReceipt Facade::buildUiModel() const {
  CreativeUiBuildRequest request;
  request.toolState = toolState_;
  request.selectionState = selectionState_;
  request.inspectionState = inspectionState_;
  request.measurementState = measurementState_;
  request.snapSettings = snapSettings_;
  request.ghostState = ghostState_;
  const std::span<const CreativeObject> objects = document_.objects();
  request.objectSummaries.reserve(objects.size());
  for (const CreativeObject& object : objects) {
    const TargetRef target = objectIdToTargetRef(object.id);
    if (target.value == kInvalidId) {
      continue;
    }

    CreativeUiObjectSummary summary;
    summary.target = target;
    summary.objectKind = object.kind;
    summary.exists = true;
    summary.visible = object.visible;
    request.objectSummaries.push_back(summary);
  }
  return buildCreativeUiModel(request);
}

CreativeFacadeMutationReceipt Facade::toggleSelectedObjectVisibility() {
  CreativeFacadeMutationReceipt receipt;
  receipt.requested = true;
  receipt.target = selectionState_.selectedTarget;
  receipt.revisionBefore = document_.revision();
  receipt.revisionAfter = receipt.revisionBefore;

  if (receipt.target.value == kInvalidId) {
    receipt.status = CreativeFacadeMutationStatus::NoSelection;
    receipt.message = "no_selection";
    return receipt;
  }

  receipt.hadSelection = true;
  receipt.mutationKind = CreativeMutationKind::SetVisible;

  CreativeObjectId objectId = kInvalidObjectId;
  if (!targetRefToObjectId(receipt.target, objectId)) {
    receipt.status = CreativeFacadeMutationStatus::MissingObject;
    receipt.documentStatus = CreativeDocumentMutationStatus::MissingObject;
    receipt.message = "missing_object";
    return receipt;
  }
  receipt.objectId = objectId;

  const CreativeObject* object = document_.findObject(objectId);
  if (object == nullptr) {
    receipt.status = CreativeFacadeMutationStatus::MissingObject;
    receipt.documentStatus = CreativeDocumentMutationStatus::MissingObject;
    receipt.message = "missing_object";
    return receipt;
  }

  receipt.objectKind = object->kind;
  receipt.visibleBefore = object->visible;

  const CreativeDocumentMutationReceipt documentReceipt =
      setDocumentObjectVisible(document_, objectId, !receipt.visibleBefore);

  receipt.accepted = documentMutationSucceeded(documentReceipt.status);
  receipt.changed = documentReceipt.changed &&
                    documentReceipt.revisionAfter !=
                        documentReceipt.revisionBefore;
  receipt.documentStatus = documentReceipt.status;
  receipt.mutationKind = documentReceipt.mutationKind;
  receipt.revisionBefore = documentReceipt.revisionBefore;
  receipt.revisionAfter = documentReceipt.revisionAfter;
  receipt.message = documentReceipt.message;

  const CreativeObject* objectAfter = document_.findObject(objectId);
  if (objectAfter != nullptr) {
    receipt.objectKind = objectAfter->kind;
    receipt.visibleAfter = objectAfter->visible;
  } else {
    receipt.visibleAfter = receipt.visibleBefore;
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
  CreativeDocumentRemoveReceipt receipt = document_.removeDocumentObject(request);
  if (!receipt.accepted || !receipt.objectRemoved) {
    recordCommandFailure(stats_);
    return receipt;
  }

  invalidateRemovedObjectEditorState(receipt.objectId,
                                     state_,
                                     toolState_,
                                     selectionState_,
                                     inspectionState_,
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
  receipt.inspectionCleared = hasInspectionState(inspectionState_);
  receipt.measurementCleared = hasMeasurementState(measurementState_,
                                                   toolState_);
  receipt.ghostCleared = ghostState_.visible;
  receipt.toolPointerCleared = hasToolPointerState(state_, toolState_);

  document_ = std::move(document);
  resetTransientFacadeState(state_,
                            stats_,
                            toolState_,
                            selectionState_,
                            inspectionState_,
                            measurementState_,
                            snapSettings_,
                            ghostState_);

  receipt.accepted = true;
  receipt.changed = true;
  receipt.nextDocumentId = document_.id();
  receipt.nextObjectCount = document_.objectCount();
  receipt.nextDirtyFlags = document_.dirtyFlags();
  receipt.activeToolAfter = toolState_.activeTool;
  setInstallStatus(receipt, "creative_facade_document_installed");
  return receipt;
}

CreativeObjectId Facade::createRoom(const CreateRoomCommand& command) {
  recordCommandAttempt(stats_);

  const CreativeObjectId id = document_.createRoom(command.name,
                                                   command.transform,
                                                   command.bounds,
                                                   command.layerId,
                                                   command.visible,
                                                   command.locked,
                                                   command.tags,
                                                   command.parentId);
  if (id == kInvalidObjectId) {
    recordCommandFailure(stats_);
    return kInvalidObjectId;
  }

  recordCommandSuccess(stats_);
  recordObjectCreated(stats_);
  recordRoomCreated(stats_);
  return id;
}

CreativeObjectId Facade::createRoom(std::string name) {
  return createRoom(makeCreateRoomCommand(std::move(name)));
}

bool Facade::renameObject(const RenameObjectCommand& command) {
  recordCommandAttempt(stats_);
  const bool renamed = document_.renameObject(command.id, command.name);
  if (renamed) {
    recordCommandSuccess(stats_);
  } else {
    recordCommandFailure(stats_);
  }
  return renamed;
}

bool Facade::renameObject(CreativeObjectId id, std::string nextName) {
  RenameObjectCommand command;
  command.id = id;
  command.name = std::move(nextName);
  return renameObject(command);
}

bool Facade::removeObject(const RemoveObjectCommand& command) {
  return removeDocumentObject(command.id).objectRemoved;
}

bool Facade::removeObject(CreativeObjectId id) {
  RemoveObjectCommand command;
  command.id = id;
  return removeObject(command);
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
