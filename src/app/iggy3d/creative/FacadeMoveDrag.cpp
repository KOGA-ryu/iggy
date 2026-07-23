#include "app/iggy3d/creative/Facade.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/FacadeInternal.hpp"
#include "app/iggy3d/creative/document/DocumentSnap.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"
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
[[nodiscard]] bool snapMoveAnchor(
    CreativeVec3 anchor,
    CreativeDocumentSnapSettings settings,
    const CreativeToolPointerPacket& pointer,
    CreativeVec3& snapped) noexcept {
  if (pointer.hasMoveSnapStepOverride) {
    settings.stepX = pointer.moveSnapStepOverride;
    settings.stepY = pointer.moveSnapStepOverride;
    settings.stepZ = pointer.moveSnapStepOverride;
  }
  const CreativeCoreVec3Conversion coreAnchor =
      creativeVec3ToCoreChecked(anchor);
  const CreativeCoreVec3Conversion coreStep = creativeVec3ToCoreChecked(
      {settings.stepX, settings.stepY, settings.stepZ});
  const CreativeCoreVec3Conversion coreOrigin = creativeVec3ToCoreChecked(
      {settings.originX, settings.originY, settings.originZ});
  if (!coreAnchor.converted || !coreStep.converted ||
      !coreOrigin.converted) {
    return false;
  }
  const unsigned axisMask =
      moveSnapAxisMask(pointer.moveHeldAxis) & documentSnapAxisMask(settings);
  const iggy3d::Vec3 coreSnapped = iggy3d::snapVec3ToGrid(
      coreAnchor.value, coreStep.value, coreOrigin.value, axisMask);
  if (!iggy3d::isFinite(coreSnapped)) {
    return false;
  }
  snapped = creativeVec3FromCore(coreSnapped);
  return true;
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

[[nodiscard]] CreativeSelectionPlacementRequest makeMovePlacementRequest(
    CreativeVec3 startAnchor,
    CreativeVec3 destinationAnchor) noexcept {
  CreativeSelectionPlacementRequest request;
  request.mode = CreativeSelectionPlacementMode::Move;
  request.sourceAnchor = startAnchor;
  request.targetAnchor = destinationAnchor;
  return request;
}

[[nodiscard]] CreativeDocumentMutationStatus documentStatusForPlacement(
    const CreativeSelectionPlacementReceipt& placement) noexcept {
  if (placement.mutationReceipt.status !=
      CreativeDocumentMutationStatus::Unknown) {
    return placement.mutationReceipt.status;
  }
  switch (placement.status) {
    case CreativeSelectionPlacementStatus::Applied:
      return CreativeDocumentMutationStatus::BatchApplied;
    case CreativeSelectionPlacementStatus::NoChange:
      return CreativeDocumentMutationStatus::BatchNoChange;
    case CreativeSelectionPlacementStatus::EmptySource:
    case CreativeSelectionPlacementStatus::InvalidRequest:
      return CreativeDocumentMutationStatus::InvalidRequest;
    case CreativeSelectionPlacementStatus::MissingObject:
    case CreativeSelectionPlacementStatus::InvalidSource:
    case CreativeSelectionPlacementStatus::LockedObject:
    case CreativeSelectionPlacementStatus::UnsupportedObject:
    case CreativeSelectionPlacementStatus::Rejected:
      return CreativeDocumentMutationStatus::ApplyFailed;
    case CreativeSelectionPlacementStatus::NotRequested:
    case CreativeSelectionPlacementStatus::Planned:
      return CreativeDocumentMutationStatus::Unknown;
  }
  return CreativeDocumentMutationStatus::Unknown;
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
        moveDragObjectIds_.clear();
        receipt.outcome = CreativeFacadeMoveDragOutcome::NoTarget;
        receipt.message = "move_drag_no_target";
        return receipt;
      }

      moveDragActive_ = true;
      moveDragTarget_ = dragTarget;
      moveDragObjectId_ = objectId;
      moveDragStartAnchor_ = objectCornerAnchor(*object);
      moveDragObjectIds_.clear();
      const std::span<const TargetRef> selectedTargets =
          selectedTargetList(selectionState_);
      std::vector<CreativeObjectId> selectedObjectIds;
      selectedObjectIds.reserve(selectedTargets.empty()
                                    ? 1U
                                    : selectedTargets.size());
      for (TargetRef selectedTarget : selectedTargets) {
        CreativeObjectId selectedObjectId = kInvalidObjectId;
        if (targetRefToObjectId(selectedTarget, selectedObjectId)) {
          selectedObjectIds.push_back(selectedObjectId);
        }
      }
      if (selectedObjectIds.empty()) {
        selectedObjectIds.push_back(objectId);
      }
      const CreativeHierarchySelection hierarchy =
          resolveCreativeObjectHierarchy(document_, selectedObjectIds);
      const std::span<const CreativeObjectId> dragObjectIds =
          hierarchy.accepted
              ? std::span<const CreativeObjectId>{hierarchy.objectIds}
              : std::span<const CreativeObjectId>{selectedObjectIds};
      for (CreativeObjectId selectedObjectId : dragObjectIds) {
        const CreativeObject* selectedObject = document_.findObject(
            selectedObjectId);
        if (selectedObject != nullptr) {
          moveDragObjectIds_.push_back(selectedObjectId);
        }
      }
      if (moveDragObjectIds_.empty()) {
        moveDragObjectIds_.push_back(objectId);
      }
      receipt.accepted = true;
      receipt.objectId = objectId;
      receipt.objectKind = object->kind;
      receipt.objectCount = moveDragObjectIds_.size();
      receipt.locked = std::any_of(
          moveDragObjectIds_.begin(), moveDragObjectIds_.end(),
          [this](CreativeObjectId dragObjectId) {
            return creativeObjectEffectivelyLocked(document_, dragObjectId);
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
      receipt.objectCount = moveDragObjectIds_.size();
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
        receipt.locked =
            creativeObjectEffectivelyLocked(document_, object->id);
      }
      if (intent.pointer.hasWorldDestination) {
        // View-agnostic Move (supersedes TD-7's hardcoded screen=XY hold): the
        // caller's moveHeldAxis picks which axis stays put; the other two follow
        // the pointer. Front view holds Z, ground-plane editor holds Y.
        const CreativeVec3 requested =
            resolveMoveAnchor(intent.pointer, moveDragStartAnchor_);
        CreativeVec3 snapped;
        if (!snapMoveAnchor(requested, document_.documentSnapSettings(),
                            intent.pointer, snapped)) {
          receipt.outcome = CreativeFacadeMoveDragOutcome::Rejected;
          receipt.message = "move_drag_geometry_invalid";
          return receipt;
        }
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
      const std::vector<CreativeObjectId> dragObjectIds =
          moveDragObjectIds_;
      // The drag ends here regardless of outcome.
      moveDragActive_ = false;
      moveDragTarget_ = {};
      moveDragObjectId_ = kInvalidObjectId;
      moveDragStartAnchor_ = {};
      moveDragObjectIds_.clear();

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
      receipt.objectCount = dragObjectIds.size();
      receipt.locked = std::any_of(
          dragObjectIds.begin(), dragObjectIds.end(),
          [this](CreativeObjectId dragObjectId) {
            return creativeObjectEffectivelyLocked(document_, dragObjectId);
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
      CreativeVec3 snapped;
      if (!snapMoveAnchor(requested, document_.documentSnapSettings(),
                          intent.pointer, snapped)) {
        receipt.outcome = CreativeFacadeMoveDragOutcome::Rejected;
        receipt.message = "move_drag_geometry_invalid";
        return receipt;
      }
      receipt.hasDestinationAnchor = true;
      receipt.requestedAnchor = requested;
      receipt.snappedAnchor = snapped;

      if (creativeVec3ExactlyEqual(snapped, startAnchor)) {
        // TD-6: destination equals start anchor — no revision bump.
        receipt.outcome = CreativeFacadeMoveDragOutcome::NoChange;
        receipt.documentStatus = CreativeDocumentMutationStatus::NoChange;
        receipt.message = "move_drag_no_change";
        return receipt;
      }

      const CreativeSelectionPlacementReceipt placement =
          placeDocumentObjectsAtomically(
              document_, dragObjectIds,
              makeMovePlacementRequest(startAnchor, snapped));
      receipt.documentStatus = documentStatusForPlacement(placement);
      receipt.revisionAfter = placement.revisionAfter;
      receipt.changed = placement.changed && placement.accepted &&
                        placement.revisionAfter != placement.revisionBefore;
      receipt.committed = true;

      if (placement.status == CreativeSelectionPlacementStatus::Applied &&
          receipt.changed) {
        receipt.accepted = true;
        receipt.outcome = CreativeFacadeMoveDragOutcome::Applied;
        receipt.message = "move_drag_applied";
      } else if (placement.status ==
                 CreativeSelectionPlacementStatus::NoChange) {
        receipt.outcome = CreativeFacadeMoveDragOutcome::NoChange;
        receipt.message = "move_drag_no_change";
      } else {
        receipt.outcome =
            placement.status == CreativeSelectionPlacementStatus::LockedObject
                              ? CreativeFacadeMoveDragOutcome::RejectedLocked
                              : CreativeFacadeMoveDragOutcome::Rejected;
        receipt.message = placement.reasonCode;
        for (const CreativeDocumentMutationReceipt& item :
             placement.mutationReceipt.receipts) {
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
      receipt.objectCount = moveDragObjectIds_.size();
      if (moveDragActive_) {
        receipt.hasStartAnchor = true;
        receipt.startAnchor = moveDragStartAnchor_;
      }
      moveDragActive_ = false;
      moveDragTarget_ = {};
      moveDragObjectId_ = kInvalidObjectId;
      moveDragStartAnchor_ = {};
      moveDragObjectIds_.clear();
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
