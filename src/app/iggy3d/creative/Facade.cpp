#include "app/iggy3d/creative/Facade.hpp"

#include "app/iggy3d/creative/document/DocumentSnap.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <limits>
#include <span>
#include <utility>

namespace iggy3d::creative {
namespace {

// TD-2 corner anchor: bounds-only kinds (Room) anchor at bounds.min; kinds with
// a transform anchor at transform.position.
[[nodiscard]] CreativeVec3 objectCornerAnchor(const CreativeObject& object) noexcept {
  if (!objectHasTransform(object.kind) && objectHasBounds(object.kind)) {
    return object.bounds.min;
  }
  return object.transform.position;
}

// Snap a world anchor via the document 3D snap contract (TL-4, D4). The tool
// boundary snaps BEFORE building the Move payload; snap is never auto-applied
// inside the mutation executor.
[[nodiscard]] CreativeVec3 snapWorldAnchor(
    CreativeVec3 anchor,
    CreativeDocumentSnapSettings settings) noexcept {
  const CreativeDocumentSnapReceipt snap = snapCreativeDocumentPoint(
      CreativeDocumentSnapPoint3{anchor.x, anchor.y, anchor.z}, settings);
  if (!snap.accepted) {
    return anchor;
  }
  return CreativeVec3{snap.snappedPoint.x, snap.snappedPoint.y,
                      snap.snappedPoint.z};
}

[[nodiscard]] bool sameAnchor(CreativeVec3 lhs, CreativeVec3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

// Hold one axis of a Move anchor at the start-anchor value, leaving the other
// two. Applied BEFORE snapping (so the requested anchor already holds the axis)
// AND AFTER snapping (so the grid snap can't lift/shift a HELD axis — e.g. a
// ground-plane editor holding Y keeps the object on the floor instead of the
// snap rounding Y up to the next grid line). The caller chooses the held axis
// from its camera (front view holds Z, ground plane holds Y), so the kernel
// stays view-agnostic.
void holdMoveAxis(CreativeVec3& anchor, CreativeVec3 startAnchor,
                  CreativeToolMoveHeldAxis heldAxis) noexcept {
  switch (heldAxis) {
    case CreativeToolMoveHeldAxis::X:
      anchor.x = startAnchor.x;
      break;
    case CreativeToolMoveHeldAxis::Y:
      anchor.y = startAnchor.y;
      break;
    case CreativeToolMoveHeldAxis::Z:
      anchor.z = startAnchor.z;
      break;
  }
}

[[nodiscard]] CreativeVec3 resolveMoveAnchor(
    CreativeToolWorldPoint destination,
    CreativeVec3 startAnchor,
    CreativeToolMoveHeldAxis heldAxis) noexcept {
  CreativeVec3 requested{destination.x, destination.y, destination.z};
  holdMoveAxis(requested, startAnchor, heldAxis);
  return requested;
}

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
    CreativeMeasurementState& measurementState,
    CreativeGhostState& ghostState) noexcept {
  if (targetRefMatchesObject(selectionState.selectedTarget, objectId)) {
    static_cast<void>(setSelectedTarget(selectionState, {}));
  }
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
         selectionState.selectedTarget.value != kInvalidId ||
         selectionState.candidateTarget.value != kInvalidId;
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

CreativeUiBuildReceipt Facade::buildUiModel() const {
  return buildUiModel(CreativeUiBuildOptions{});
}

CreativeUiBuildReceipt Facade::buildUiModel(
    CreativeUiBuildOptions options) const {
  CreativeUiBuildRequest request;
  request.toolState = toolState_;
  request.selectionState = selectionState_;
  request.measurementState = measurementState_;
  request.snapSettings = snapSettings_;
  request.ghostState = ghostState_;
  request.undoAvailable = options.undoAvailable;
  request.undoDepth = options.undoDepth;
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
    summary.locked = object.locked;
    summary.name = object.name;
    summary.objectId = object.id;
    summary.layerId = object.layerId;
    summary.bounds = object.bounds;
    summary.position = object.transform.position;
    request.objectSummaries.push_back(summary);
  }
  return buildCreativeUiModel(request);
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

CreativeFacadeMoveDragReceipt Facade::applyMoveDragIntent(
    const CreativeToolIntent& intent) {
  CreativeFacadeMoveDragReceipt receipt;
  receipt.requested = true;
  receipt.revisionBefore = document_.revision();
  receipt.revisionAfter = receipt.revisionBefore;

  switch (intent.kind) {
    case CreativeToolIntentKind::BeginMove: {
      receipt.stage = CreativeFacadeMoveDragStage::Begin;
      // The picked target seeds the drag; fall back to the current selection
      // when the press missed a specific object (TV1-G).
      TargetRef dragTarget = intent.pointer.target;
      if (dragTarget.value == kInvalidId) {
        dragTarget = selectionState_.selectedTarget;
      }
      receipt.target = dragTarget;

      CreativeObjectId objectId = kInvalidObjectId;
      const CreativeObject* object = nullptr;
      if (targetRefToObjectId(dragTarget, objectId)) {
        object = document_.findObject(objectId);
      }
      if (object == nullptr) {
        moveDragActive_ = false;
        moveDragTarget_ = {};
        moveDragObjectId_ = kInvalidObjectId;
        moveDragStartAnchor_ = {};
        receipt.outcome = CreativeFacadeMoveDragOutcome::NoTarget;
        receipt.message = "move_drag_no_target";
        return receipt;
      }

      moveDragActive_ = true;
      moveDragTarget_ = dragTarget;
      moveDragObjectId_ = objectId;
      moveDragStartAnchor_ = objectCornerAnchor(*object);
      receipt.accepted = true;
      receipt.objectId = objectId;
      receipt.objectKind = object->kind;
      receipt.locked = object->locked;
      receipt.hasStartAnchor = true;
      receipt.startAnchor = moveDragStartAnchor_;
      receipt.outcome = CreativeFacadeMoveDragOutcome::Begun;
      receipt.message = "move_drag_begin";
      return receipt;
    }

    case CreativeToolIntentKind::PreviewMove: {
      receipt.stage = CreativeFacadeMoveDragStage::Preview;
      receipt.target = moveDragTarget_;
      receipt.objectId = moveDragObjectId_;
      if (!moveDragActive_) {
        receipt.outcome = CreativeFacadeMoveDragOutcome::None;
        receipt.message = "move_drag_inactive";
        return receipt;
      }
      receipt.accepted = true;
      receipt.hasStartAnchor = true;
      receipt.startAnchor = moveDragStartAnchor_;
      const CreativeObject* object = document_.findObject(moveDragObjectId_);
      if (object != nullptr) {
        receipt.objectKind = object->kind;
        receipt.locked = object->locked;
      }
      if (intent.pointer.hasWorldDestination) {
        // View-agnostic Move (supersedes TD-7's hardcoded screen=XY hold): the
        // caller's moveHeldAxis picks which axis stays put; the other two follow
        // the pointer. Front view holds Z, ground-plane editor holds Y.
        const CreativeVec3 requested = resolveMoveAnchor(
            intent.pointer.worldDestination, moveDragStartAnchor_,
            intent.pointer.moveHeldAxis);
        CreativeVec3 snapped =
            snapWorldAnchor(requested, document_.documentSnapSettings());
        holdMoveAxis(snapped, moveDragStartAnchor_, intent.pointer.moveHeldAxis);
        receipt.hasDestinationAnchor = true;
        receipt.requestedAnchor = requested;
        receipt.snappedAnchor = snapped;
      }
      receipt.outcome = CreativeFacadeMoveDragOutcome::Previewing;
      receipt.message = "move_drag_preview";
      return receipt;
    }

    case CreativeToolIntentKind::CommitMove: {
      receipt.stage = CreativeFacadeMoveDragStage::Commit;
      receipt.target = moveDragTarget_;
      receipt.objectId = moveDragObjectId_;
      const bool wasActive = moveDragActive_;
      const CreativeVec3 startAnchor = moveDragStartAnchor_;
      const CreativeObjectId objectId = moveDragObjectId_;
      // The drag ends here regardless of outcome.
      moveDragActive_ = false;
      moveDragTarget_ = {};
      moveDragObjectId_ = kInvalidObjectId;
      moveDragStartAnchor_ = {};

      if (!wasActive) {
        receipt.outcome = CreativeFacadeMoveDragOutcome::None;
        receipt.message = "move_drag_inactive";
        return receipt;
      }
      receipt.hasStartAnchor = true;
      receipt.startAnchor = startAnchor;

      const CreativeObject* object = document_.findObject(objectId);
      if (object == nullptr) {
        receipt.outcome = CreativeFacadeMoveDragOutcome::NoTarget;
        receipt.message = "move_drag_no_target";
        return receipt;
      }
      receipt.objectKind = object->kind;
      receipt.locked = object->locked;

      if (!intent.pointer.hasWorldDestination) {
        // No resolved destination (pointer never left the grid): nothing to do.
        receipt.outcome = CreativeFacadeMoveDragOutcome::NoChange;
        receipt.message = "move_drag_no_destination";
        return receipt;
      }

      // View-agnostic Move (see the PreviewMove note above): moveHeldAxis picks
      // the axis to hold at the start anchor; the other two follow the pointer.
      const CreativeVec3 requested = resolveMoveAnchor(
          intent.pointer.worldDestination, startAnchor,
          intent.pointer.moveHeldAxis);
      CreativeVec3 snapped =
          snapWorldAnchor(requested, document_.documentSnapSettings());
      holdMoveAxis(snapped, startAnchor, intent.pointer.moveHeldAxis);
      receipt.hasDestinationAnchor = true;
      receipt.requestedAnchor = requested;
      receipt.snappedAnchor = snapped;

      if (sameAnchor(snapped, startAnchor)) {
        // TD-6: destination equals start anchor — no revision bump.
        receipt.outcome = CreativeFacadeMoveDragOutcome::NoChange;
        receipt.documentStatus = CreativeDocumentMutationStatus::NoChange;
        receipt.message = "move_drag_no_change";
        return receipt;
      }

      const CreativeDocumentMutationReceipt moveReceipt = moveDocumentObject(
          document_, objectId, snapped);
      receipt.documentStatus = moveReceipt.status;
      receipt.revisionAfter = document_.revision();
      receipt.changed =
          moveReceipt.changed &&
          moveReceipt.revisionAfter != moveReceipt.revisionBefore;
      receipt.committed = true;

      if (moveReceipt.status == CreativeDocumentMutationStatus::Applied &&
          receipt.changed) {
        receipt.accepted = true;
        receipt.outcome = CreativeFacadeMoveDragOutcome::Applied;
        receipt.message = "move_drag_applied";
      } else if (moveReceipt.status ==
                 CreativeDocumentMutationStatus::NoChange) {
        receipt.outcome = CreativeFacadeMoveDragOutcome::NoChange;
        receipt.message = "move_drag_no_change";
      } else {
        // Lock refusal (TD-3) surfaces here: the pipeline rejects the Move on a
        // locked object; the lock-refusal truth lives in objectReceipt.message
        // (TV1-A). Report it truthfully so the status line can read it.
        receipt.outcome = object->locked
                              ? CreativeFacadeMoveDragOutcome::RejectedLocked
                              : CreativeFacadeMoveDragOutcome::Rejected;
        receipt.message = moveReceipt.objectReceipt.message.empty()
                              ? moveReceipt.message
                              : moveReceipt.objectReceipt.message;
        if (receipt.message.empty()) {
          receipt.message = object->locked ? "move_drag_rejected_locked"
                                           : "move_drag_rejected";
        }
      }
      return receipt;
    }

    case CreativeToolIntentKind::CancelMove: {
      receipt.stage = CreativeFacadeMoveDragStage::Cancelled;
      receipt.target = moveDragTarget_;
      receipt.objectId = moveDragObjectId_;
      if (moveDragActive_) {
        receipt.hasStartAnchor = true;
        receipt.startAnchor = moveDragStartAnchor_;
      }
      moveDragActive_ = false;
      moveDragTarget_ = {};
      moveDragObjectId_ = kInvalidObjectId;
      moveDragStartAnchor_ = {};
      receipt.accepted = true;
      receipt.outcome = CreativeFacadeMoveDragOutcome::Cancelled;
      receipt.message = "move_cancelled";
      return receipt;
    }

    default:
      receipt.message = "move_drag_not_requested";
      return receipt;
  }
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
