#include "app/iggy3d/creative/Facade.hpp"

#include "app/iggy3d/creative/FacadeInternal.hpp"
#include "app/iggy3d/creative/document/DocumentSnap.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "core/math/Snap.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

// branch-gate-relocation: BG-1227 from=src/app/iggy3d/creative/Facade.cpp
namespace iggy3d::creative {
namespace {

using facade_internal::targetRefToObjectId;

// TD-2 corner anchor: bounds-only kinds (Room) anchor at bounds.min; kinds with
// a transform anchor at transform.position.
[[nodiscard]] CreativeVec3 objectCornerAnchor(const CreativeObject& object) noexcept {
  if (!objectHasTransform(object.kind) && objectHasBounds(object.kind)) {
    return object.bounds.min;
  }
  return object.transform.position;
}

[[nodiscard]] iggy3d::Vec3 toCoreVec3(CreativeVec3 value) noexcept {
  return {static_cast<float>(value.x), static_cast<float>(value.y),
          static_cast<float>(value.z)};
}

[[nodiscard]] CreativeVec3 toCreativeVec3(iggy3d::Vec3 value) noexcept {
  return {static_cast<double>(value.x), static_cast<double>(value.y),
          static_cast<double>(value.z)};
}

[[nodiscard]] unsigned moveSnapAxisMask(
    CreativeToolMoveHeldAxis heldAxis) noexcept {
  switch (heldAxis) {
    case CreativeToolMoveHeldAxis::X:
      return 0x6u;
    case CreativeToolMoveHeldAxis::Y:
      return 0x5u;
    case CreativeToolMoveHeldAxis::Z:
      return 0x3u;
  }
  return 0x7u;
}

[[nodiscard]] unsigned documentSnapAxisMask(
    CreativeDocumentSnapSettings settings) noexcept {
  if (settings.mode == CreativeDocumentSnapMode::Disabled) {
    return 0x0u;
  }
  return static_cast<unsigned>(settings.axes) & 0x7u;
}

// Snap a Move anchor through the core float snap kernel. The held axis is
// excluded from the core axis mask, so no second post-snap hold pass is needed.
[[nodiscard]] CreativeVec3 snapMoveAnchor(
    CreativeVec3 anchor,
    CreativeDocumentSnapSettings settings,
    const CreativeToolPointerPacket& pointer) noexcept {
  if (pointer.hasMoveSnapStepOverride) {
    settings.stepX = pointer.moveSnapStepOverride;
    settings.stepY = pointer.moveSnapStepOverride;
    settings.stepZ = pointer.moveSnapStepOverride;
  }
  const iggy3d::Vec3 step{static_cast<float>(settings.stepX),
                          static_cast<float>(settings.stepY),
                          static_cast<float>(settings.stepZ)};
  const iggy3d::Vec3 origin{static_cast<float>(settings.originX),
                            static_cast<float>(settings.originY),
                            static_cast<float>(settings.originZ)};
  const unsigned axisMask =
      moveSnapAxisMask(pointer.moveHeldAxis) & documentSnapAxisMask(settings);
  return toCreativeVec3(
      iggy3d::snapVec3ToGrid(toCoreVec3(anchor), step, origin, axisMask));
}

[[nodiscard]] bool validMovePointerOptions(
    const CreativeToolPointerPacket& pointer) noexcept {
  const bool constraintValid =
      static_cast<std::size_t>(pointer.moveConstraint) <
      static_cast<std::size_t>(CreativeMoveConstraint::Count);
  const bool snapOverrideValid =
      !pointer.hasMoveSnapStepOverride ||
      (std::isfinite(pointer.moveSnapStepOverride) &&
       pointer.moveSnapStepOverride > 0.0);
  return constraintValid && snapOverrideValid;
}

void applyMoveConstraint(CreativeVec3& anchor,
                         CreativeVec3 startAnchor,
                         CreativeMoveConstraint constraint) noexcept {
  switch (constraint) {
    case CreativeMoveConstraint::Free:
      break;
    case CreativeMoveConstraint::X:
      anchor.z = startAnchor.z;
      break;
    case CreativeMoveConstraint::Z:
      anchor.x = startAnchor.x;
      break;
    case CreativeMoveConstraint::Count:
      break;
  }
}

[[nodiscard]] bool sameAnchor(CreativeVec3 lhs, CreativeVec3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

// Hold one axis of a Move anchor at the start-anchor value, leaving the other
// two. Applied BEFORE snapping so the requested anchor already holds the axis;
// core snap receives an axis mask that excludes the held axis. The caller
// chooses the held axis from its camera (front view holds Z, ground-plane
// editor holds Y), so the kernel stays view-agnostic.
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
    const CreativeToolPointerPacket& pointer,
    CreativeVec3 startAnchor) noexcept {
  CreativeVec3 requested{pointer.worldDestination.x,
                         pointer.worldDestination.y,
                         pointer.worldDestination.z};
  holdMoveAxis(requested, startAnchor, pointer.moveHeldAxis);
  applyMoveConstraint(requested, startAnchor, pointer.moveConstraint);
  return requested;
}

}  // namespace

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
        moveDragObjects_.clear();
        receipt.outcome = CreativeFacadeMoveDragOutcome::NoTarget;
        receipt.message = "move_drag_no_target";
        return receipt;
      }

      moveDragActive_ = true;
      moveDragTarget_ = dragTarget;
      moveDragObjectId_ = objectId;
      moveDragStartAnchor_ = objectCornerAnchor(*object);
      moveDragObjects_.clear();
      const std::span<const TargetRef> selectedTargets =
          selectedTargetList(selectionState_);
      for (TargetRef selectedTarget : selectedTargets) {
        CreativeObjectId selectedObjectId = kInvalidObjectId;
        if (!targetRefToObjectId(selectedTarget, selectedObjectId)) {
          continue;
        }
        const CreativeObject* selectedObject =
            document_.findObject(selectedObjectId);
        if (selectedObject != nullptr) {
          moveDragObjects_.push_back(
              MoveDragObject{selectedObjectId,
                             objectCornerAnchor(*selectedObject)});
        }
      }
      if (moveDragObjects_.empty()) {
        moveDragObjects_.push_back(
            MoveDragObject{objectId, moveDragStartAnchor_});
      }
      receipt.accepted = true;
      receipt.objectId = objectId;
      receipt.objectKind = object->kind;
      receipt.objectCount = moveDragObjects_.size();
      receipt.locked = std::any_of(
          moveDragObjects_.begin(), moveDragObjects_.end(),
          [this](const MoveDragObject& dragObject) {
            const CreativeObject* selected =
                document_.findObject(dragObject.objectId);
            return selected != nullptr && selected->locked;
          });
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
      receipt.objectCount = moveDragObjects_.size();
      if (!moveDragActive_) {
        receipt.outcome = CreativeFacadeMoveDragOutcome::None;
        receipt.message = "move_drag_inactive";
        return receipt;
      }
      if (!validMovePointerOptions(intent.pointer)) {
        receipt.outcome = CreativeFacadeMoveDragOutcome::Rejected;
        receipt.message = "move_drag_options_invalid";
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
        const CreativeVec3 requested =
            resolveMoveAnchor(intent.pointer, moveDragStartAnchor_);
        const CreativeVec3 snapped =
            snapMoveAnchor(requested, document_.documentSnapSettings(),
                           intent.pointer);
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
      const std::vector<MoveDragObject> dragObjects = moveDragObjects_;
      // The drag ends here regardless of outcome.
      moveDragActive_ = false;
      moveDragTarget_ = {};
      moveDragObjectId_ = kInvalidObjectId;
      moveDragStartAnchor_ = {};
      moveDragObjects_.clear();

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
      receipt.objectCount = dragObjects.size();
      receipt.locked = std::any_of(
          dragObjects.begin(), dragObjects.end(),
          [this](const MoveDragObject& dragObject) {
            const CreativeObject* selected =
                document_.findObject(dragObject.objectId);
            return selected != nullptr && selected->locked;
          });

      if (!intent.pointer.hasWorldDestination) {
        // No resolved destination (pointer never left the grid): nothing to do.
        receipt.outcome = CreativeFacadeMoveDragOutcome::NoChange;
        receipt.message = "move_drag_no_destination";
        return receipt;
      }
      if (!validMovePointerOptions(intent.pointer)) {
        receipt.outcome = CreativeFacadeMoveDragOutcome::Rejected;
        receipt.message = "move_drag_options_invalid";
        return receipt;
      }

      // View-agnostic Move (see the PreviewMove note above): moveHeldAxis picks
      // the axis to hold at the start anchor; the other two follow the pointer.
      const CreativeVec3 requested =
          resolveMoveAnchor(intent.pointer, startAnchor);
      const CreativeVec3 snapped =
          snapMoveAnchor(requested, document_.documentSnapSettings(),
                         intent.pointer);
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

      const CreativeVec3 delta{snapped.x - startAnchor.x,
                               snapped.y - startAnchor.y,
                               snapped.z - startAnchor.z};
      std::vector<CreativeMutationRequest> moveRequests;
      moveRequests.reserve(dragObjects.size());
      for (const MoveDragObject& dragObject : dragObjects) {
        moveRequests.push_back(
            CreativeMutationRequest{0,
                                    dragObject.objectId,
                                    CreativeMutationKind::Move,
                                    makeMovePayload(CreativeVec3{
                                        dragObject.startAnchor.x + delta.x,
                                        dragObject.startAnchor.y + delta.y,
                                        dragObject.startAnchor.z + delta.z})});
      }
      const CreativeDocumentBatchMutationReceipt moveReceipt =
          applyDocumentMutationsAtomically(document_, moveRequests);
      receipt.documentStatus = moveReceipt.status;
      receipt.revisionAfter = document_.revision();
      receipt.changed =
          moveReceipt.changed && moveReceipt.committed &&
          moveReceipt.revisionAfter != moveReceipt.revisionBefore;
      receipt.committed = true;

      if (moveReceipt.status == CreativeDocumentMutationStatus::BatchApplied &&
          receipt.changed) {
        receipt.accepted = true;
        receipt.outcome = CreativeFacadeMoveDragOutcome::Applied;
        receipt.message = "move_drag_applied";
      } else if (moveReceipt.status ==
                 CreativeDocumentMutationStatus::BatchNoChange) {
        receipt.outcome = CreativeFacadeMoveDragOutcome::NoChange;
        receipt.message = "move_drag_no_change";
      } else {
        // Lock refusal (TD-3) surfaces here: the pipeline rejects the Move on a
        // locked object; the lock-refusal truth lives in objectReceipt.message
        // (TV1-A). Report it truthfully so the status line can read it.
        receipt.outcome = receipt.locked
                              ? CreativeFacadeMoveDragOutcome::RejectedLocked
                              : CreativeFacadeMoveDragOutcome::Rejected;
        receipt.message = moveReceipt.message;
        for (const CreativeDocumentMutationReceipt& item : moveReceipt.receipts) {
          if (documentMutationFailed(item.status)) {
            receipt.message = item.objectReceipt.message.empty()
                                  ? item.message
                                  : item.objectReceipt.message;
            break;
          }
        }
        if (receipt.message.empty()) {
          receipt.message = receipt.locked ? "move_drag_rejected_locked"
                                           : "move_drag_rejected";
        }
      }
      return receipt;
    }

    case CreativeToolIntentKind::CancelMove: {
      receipt.stage = CreativeFacadeMoveDragStage::Cancelled;
      receipt.target = moveDragTarget_;
      receipt.objectId = moveDragObjectId_;
      receipt.objectCount = moveDragObjects_.size();
      if (moveDragActive_) {
        receipt.hasStartAnchor = true;
        receipt.startAnchor = moveDragStartAnchor_;
      }
      moveDragActive_ = false;
      moveDragTarget_ = {};
      moveDragObjectId_ = kInvalidObjectId;
      moveDragStartAnchor_ = {};
      moveDragObjects_.clear();
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

}  // namespace iggy3d::creative
