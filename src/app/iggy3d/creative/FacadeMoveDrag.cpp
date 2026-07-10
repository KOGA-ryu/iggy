#include "app/iggy3d/creative/Facade.hpp"

#include "app/iggy3d/creative/FacadeInternal.hpp"
#include "app/iggy3d/creative/document/DocumentSnap.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "core/math/Snap.hpp"

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
    CreativeToolMoveHeldAxis heldAxis) noexcept {
  const iggy3d::Vec3 step{static_cast<float>(settings.stepX),
                          static_cast<float>(settings.stepY),
                          static_cast<float>(settings.stepZ)};
  const iggy3d::Vec3 origin{static_cast<float>(settings.originX),
                            static_cast<float>(settings.originY),
                            static_cast<float>(settings.originZ)};
  const unsigned axisMask =
      moveSnapAxisMask(heldAxis) & documentSnapAxisMask(settings);
  return toCreativeVec3(
      iggy3d::snapVec3ToGrid(toCoreVec3(anchor), step, origin, axisMask));
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
    CreativeToolWorldPoint destination,
    CreativeVec3 startAnchor,
    CreativeToolMoveHeldAxis heldAxis) noexcept {
  CreativeVec3 requested{destination.x, destination.y, destination.z};
  holdMoveAxis(requested, startAnchor, heldAxis);
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
        const CreativeVec3 snapped =
            snapMoveAnchor(requested, document_.documentSnapSettings(),
                           intent.pointer.moveHeldAxis);
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
      const CreativeVec3 snapped =
          snapMoveAnchor(requested, document_.documentSnapSettings(),
                         intent.pointer.moveHeldAxis);
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

}  // namespace iggy3d::creative
