#include "app/iggy3d/creative/Facade.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeToolInputPacket pointerPress(cr::Id targetId) {
  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerPress;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = targetId;
  return input;
}

cr::CreativeObjectId createRoom(cr::Facade& facade) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  request.name = "Room";
  return facade.createDocumentObject(request).objectId;
}

void selectTarget(cr::Facade& facade, cr::CreativeObjectId objectId) {
  static_cast<void>(facade.setActiveTool(cr::Tool::Select));
  static_cast<void>(facade.dispatchToolInput(
      pointerPress(static_cast<cr::Id>(objectId))));
}

bool defaultNoSelectionRejects() {
  cr::Facade facade;
  const std::uint64_t revisionBefore = facade.document().revision();
  const cr::CreativeFacadeMutationReceipt receipt =
      facade.toggleSelectedObjectVisibility();

  return expect(receipt.requested, "no selection requested") &&
         expect(!receipt.accepted, "no selection not accepted") &&
         expect(!receipt.changed, "no selection unchanged") &&
         expect(!receipt.hadSelection, "no selection flag") &&
         expect(receipt.target.value == cr::kInvalidId,
                "no selection target invalid") &&
         expect(receipt.objectId == cr::kInvalidObjectId,
                "no selection object invalid") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Unknown,
                "no selection kind unknown") &&
         expect(receipt.revisionBefore == revisionBefore,
                "no selection revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "no selection revision after") &&
         expect(receipt.status == cr::CreativeFacadeMutationStatus::NoSelection,
                "no selection status") &&
         expect(receipt.documentStatus ==
                    cr::CreativeDocumentMutationStatus::Unknown,
                "no selection document status") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::Unknown,
                "no selection mutation kind") &&
         expect(receipt.message == "no_selection", "no selection message") &&
         expect(facade.document().revision() == revisionBefore,
                "no selection facade revision unchanged");
}

bool selectedMissingTargetRejectsAndPreservesSelection() {
  cr::Facade facade;
  constexpr cr::Id missingTarget = 999;
  static_cast<void>(facade.dispatchToolInput(pointerPress(missingTarget)));
  const std::uint64_t revisionBefore = facade.document().revision();

  const cr::CreativeFacadeMutationReceipt receipt =
      facade.toggleSelectedObjectVisibility();

  return expect(receipt.requested, "missing requested") &&
         expect(!receipt.accepted, "missing not accepted") &&
         expect(!receipt.changed, "missing unchanged") &&
         expect(receipt.hadSelection, "missing had selection") &&
         expect(receipt.target.value == missingTarget, "missing target") &&
         expect(receipt.objectId == missingTarget, "missing object id") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Unknown,
                "missing kind unknown") &&
         expect(receipt.revisionBefore == revisionBefore,
                "missing revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "missing revision after") &&
         expect(receipt.status == cr::CreativeFacadeMutationStatus::MissingObject,
                "missing status") &&
         expect(receipt.documentStatus ==
                    cr::CreativeDocumentMutationStatus::MissingObject,
                "missing document status") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::SetVisible,
                "missing mutation kind") &&
         expect(receipt.message == "missing_object", "missing message") &&
         expect(facade.selectionState().selectedTarget.value == missingTarget,
                "missing selection preserved") &&
         expect(facade.document().revision() == revisionBefore,
                "missing revision unchanged");
}

bool selectedRoomTogglesVisibleFalseAndPreservesState() {
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  const std::uint64_t objectCountBefore = facade.document().objectCount();
  const std::uint64_t revisionBefore = facade.document().revision();

  const cr::CreativeFacadeMutationReceipt receipt =
      facade.toggleSelectedObjectVisibility();
  const cr::CreativeObject* room = facade.findObject(roomId);

  return expect(room != nullptr, "first room exists") &&
         expect(receipt.requested, "first requested") &&
         expect(receipt.accepted, "first accepted") &&
         expect(receipt.changed, "first changed") &&
         expect(receipt.hadSelection, "first had selection") &&
         expect(receipt.target.value == roomId, "first target") &&
         expect(receipt.objectId == roomId, "first object id") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Room,
                "first object kind") &&
         expect(receipt.visibleBefore, "first visible before") &&
         expect(!receipt.visibleAfter, "first visible after") &&
         expect(!room->visible, "first room now hidden") &&
         expect(receipt.revisionBefore == revisionBefore,
                "first revision before") &&
         expect(receipt.revisionAfter == revisionBefore + 1U,
                "first revision after") &&
         expect(facade.document().revision() == revisionBefore + 1U,
                "first document revision") &&
         expect(facade.document().objectCount() == objectCountBefore,
                "first object count unchanged") &&
         expect(receipt.status == cr::CreativeFacadeMutationStatus::Applied,
                "first status") &&
         expect(receipt.documentStatus ==
                    cr::CreativeDocumentMutationStatus::Applied,
                "first document status") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::SetVisible,
                "first mutation kind") &&
         expect(receipt.message ==
                    "document mutation applied through object mutation pipeline",
                "first message") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "first selection preserved");
}

bool secondToggleRestoresVisibilityAndIncrementsAgain() {
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  const cr::CreativeFacadeMutationReceipt first =
      facade.toggleSelectedObjectVisibility();
  const std::uint64_t revisionBeforeSecond = facade.document().revision();

  const cr::CreativeFacadeMutationReceipt second =
      facade.toggleSelectedObjectVisibility();
  const cr::CreativeObject* room = facade.findObject(roomId);

  return expect(first.changed, "second setup first changed") &&
         expect(room != nullptr, "second room exists") &&
         expect(second.accepted, "second accepted") &&
         expect(second.changed, "second changed") &&
         expect(!second.visibleBefore, "second visible before") &&
         expect(second.visibleAfter, "second visible after") &&
         expect(room->visible, "second room visible") &&
         expect(second.revisionBefore == revisionBeforeSecond,
                "second revision before") &&
         expect(second.revisionAfter == revisionBeforeSecond + 1U,
                "second revision after") &&
         expect(second.status == cr::CreativeFacadeMutationStatus::Applied,
                "second status") &&
         expect(second.documentStatus ==
                    cr::CreativeDocumentMutationStatus::Applied,
                "second document status") &&
         expect(second.mutationKind == cr::CreativeMutationKind::SetVisible,
                "second mutation kind") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "second selection preserved");
}

bool activeToolRemainsUnchangedByToggle() {
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));

  const cr::CreativeFacadeMutationReceipt receipt =
      facade.toggleSelectedObjectVisibility();

  return expect(receipt.accepted, "tool toggle accepted") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "tool toggle selection preserved") &&
         expect(facade.toolState().activeTool == cr::Tool::Move,
                "tool toggle active tool preserved");
}

bool lockToggleWithoutSelectionRejects() {
  cr::Facade facade;
  const std::uint64_t revisionBefore = facade.document().revision();
  const cr::CreativeFacadeMutationReceipt receipt =
      facade.toggleSelectedObjectLocked();

  return expect(receipt.requested, "lock no selection requested") &&
         expect(!receipt.accepted, "lock no selection not accepted") &&
         expect(!receipt.changed, "lock no selection unchanged") &&
         expect(!receipt.hadSelection, "lock no selection flag") &&
         expect(receipt.objectId == cr::kInvalidObjectId,
                "lock no selection object invalid") &&
         expect(receipt.status == cr::CreativeFacadeMutationStatus::NoSelection,
                "lock no selection status") &&
         expect(receipt.documentStatus ==
                    cr::CreativeDocumentMutationStatus::Unknown,
                "lock no selection document status") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::Unknown,
                "lock no selection mutation kind") &&
         expect(receipt.message == "no_selection",
                "lock no selection message") &&
         expect(facade.document().revision() == revisionBefore,
                "lock no selection revision unchanged");
}

bool lockToggleLocksSelectedRoomWithReceipt() {
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  const std::uint64_t revisionBefore = facade.document().revision();

  const cr::CreativeFacadeMutationReceipt receipt =
      facade.toggleSelectedObjectLocked();
  const cr::CreativeObject* room = facade.findObject(roomId);

  return expect(room != nullptr, "lock on room exists") &&
         expect(receipt.requested, "lock on requested") &&
         expect(receipt.accepted, "lock on accepted") &&
         expect(receipt.changed, "lock on changed") &&
         expect(receipt.hadSelection, "lock on had selection") &&
         expect(receipt.target.value == roomId, "lock on target") &&
         expect(receipt.objectId == roomId, "lock on object id") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Room,
                "lock on object kind") &&
         expect(!receipt.lockedBefore, "lock on locked before") &&
         expect(receipt.lockedAfter, "lock on locked after") &&
         expect(receipt.visibleBefore, "lock on visible before untouched") &&
         expect(receipt.visibleAfter, "lock on visible after untouched") &&
         expect(room->locked, "lock on room now locked") &&
         expect(room->visible, "lock on room still visible") &&
         expect(receipt.revisionBefore == revisionBefore,
                "lock on revision before") &&
         expect(receipt.revisionAfter == revisionBefore + 1U,
                "lock on revision after") &&
         expect(receipt.status == cr::CreativeFacadeMutationStatus::Applied,
                "lock on status") &&
         expect(receipt.documentStatus ==
                    cr::CreativeDocumentMutationStatus::Applied,
                "lock on document status") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::SetLocked,
                "lock on mutation kind") &&
         expect(receipt.message ==
                    "document mutation applied through object mutation pipeline",
                "lock on message") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "lock on selection preserved");
}

bool lockToggleUnlocksLockedRoomDespiteLockGate() {
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  const cr::CreativeFacadeMutationReceipt first =
      facade.toggleSelectedObjectLocked();
  const std::uint64_t revisionBeforeSecond = facade.document().revision();

  const cr::CreativeFacadeMutationReceipt second =
      facade.toggleSelectedObjectLocked();
  const cr::CreativeObject* room = facade.findObject(roomId);

  return expect(first.changed, "unlock setup locked") &&
         expect(room != nullptr, "unlock room exists") &&
         expect(second.accepted, "unlock accepted while locked") &&
         expect(second.changed, "unlock changed") &&
         expect(second.lockedBefore, "unlock locked before") &&
         expect(!second.lockedAfter, "unlock locked after") &&
         expect(!room->locked, "unlock room unlocked") &&
         expect(second.revisionBefore == revisionBeforeSecond,
                "unlock revision before") &&
         expect(second.revisionAfter == revisionBeforeSecond + 1U,
                "unlock revision after") &&
         expect(second.status == cr::CreativeFacadeMutationStatus::Applied,
                "unlock status") &&
         expect(second.documentStatus ==
                    cr::CreativeDocumentMutationStatus::Applied,
                "unlock document status") &&
         expect(second.mutationKind == cr::CreativeMutationKind::SetLocked,
                "unlock mutation kind") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "unlock selection preserved");
}

bool visibilityToggleRejectsWhileLocked() {
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  const cr::CreativeFacadeMutationReceipt lockReceipt =
      facade.toggleSelectedObjectLocked();
  const std::uint64_t revisionBefore = facade.document().revision();

  const cr::CreativeFacadeMutationReceipt receipt =
      facade.toggleSelectedObjectVisibility();
  const cr::CreativeObject* room = facade.findObject(roomId);

  return expect(lockReceipt.changed, "locked visibility setup locked") &&
         expect(room != nullptr, "locked visibility room exists") &&
         expect(receipt.requested, "locked visibility requested") &&
         expect(!receipt.accepted, "locked visibility not accepted") &&
         expect(!receipt.changed, "locked visibility unchanged") &&
         expect(receipt.status == cr::CreativeFacadeMutationStatus::Rejected,
                "locked visibility status") &&
         expect(receipt.documentStatus ==
                    cr::CreativeDocumentMutationStatus::ApplyFailed,
                "locked visibility document status") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::SetVisible,
                "locked visibility mutation kind") &&
         expect(receipt.visibleBefore, "locked visibility before") &&
         expect(receipt.visibleAfter, "locked visibility after unchanged") &&
         expect(receipt.lockedBefore, "locked visibility locked before") &&
         expect(receipt.lockedAfter, "locked visibility locked after") &&
         expect(room->visible, "locked visibility room still visible") &&
         expect(receipt.revisionBefore == revisionBefore,
                "locked visibility revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "locked visibility revision after") &&
         expect(facade.document().revision() == revisionBefore,
                "locked visibility document revision unchanged");
}

bool lockedObjectRemovalRefusedThroughFacade() {
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  static_cast<void>(facade.toggleSelectedObjectLocked());
  const std::uint64_t revisionBefore = facade.document().revision();
  const std::uint64_t failuresBefore = facade.stats().commandFailures;

  const cr::CreativeDocumentRemoveReceipt receipt =
      facade.removeDocumentObject(roomId);
  const cr::CreativeObject* room = facade.findObject(roomId);

  return expect(receipt.requested, "locked remove requested") &&
         expect(!receipt.accepted, "locked remove not accepted") &&
         expect(!receipt.changed, "locked remove unchanged") &&
         expect(!receipt.objectRemoved, "locked remove no removal") &&
         expect(receipt.status == cr::CreativeDocumentRemoveStatus::LockedObject,
                "locked remove status") &&
         expect(receipt.objectId == roomId, "locked remove object id") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Room,
                "locked remove object kind") &&
         expect(receipt.objectName == "Room", "locked remove object name") &&
         expect(receipt.removalDirtyFlags == 0U,
                "locked remove no dirty flags") &&
         expect(receipt.message == "object is locked",
                "locked remove message") &&
         expect(receipt.revisionBefore == revisionBefore,
                "locked remove revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "locked remove revision after") &&
         expect(room != nullptr, "locked remove object survives") &&
         expect(facade.document().objectCount() == 1U,
                "locked remove object count stable") &&
         expect(facade.document().revision() == revisionBefore,
                "locked remove document revision unchanged") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "locked remove selection preserved") &&
         expect(facade.stats().commandFailures == failuresBefore + 1U,
                "locked remove counted as failure");
}

bool unlockedObjectRemovalSucceedsAfterUnlock() {
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  static_cast<void>(facade.toggleSelectedObjectLocked());
  const cr::CreativeDocumentRemoveReceipt refused =
      facade.removeDocumentObject(roomId);
  static_cast<void>(facade.toggleSelectedObjectLocked());

  const cr::CreativeDocumentRemoveReceipt removed =
      facade.removeDocumentObject(roomId);

  return expect(!refused.objectRemoved, "unlock remove setup refused") &&
         expect(removed.accepted, "unlock remove accepted") &&
         expect(removed.objectRemoved, "unlock remove removed") &&
         expect(removed.status == cr::CreativeDocumentRemoveStatus::Removed,
                "unlock remove status") &&
         expect(facade.findObject(roomId) == nullptr,
                "unlock remove object gone") &&
         expect(facade.document().objectCount() == 0U,
                "unlock remove document empty") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "unlock remove selection cleared");
}

bool facadeMutationStatusStringsAreStable() {
  return expect(cr::toString(cr::CreativeFacadeMutationStatus::Unknown) ==
                    "Unknown",
                "status unknown string") &&
         expect(cr::toString(cr::CreativeFacadeMutationStatus::NoSelection) ==
                    "NoSelection",
                "status no selection string") &&
         expect(cr::toString(cr::CreativeFacadeMutationStatus::MissingObject) ==
                    "MissingObject",
                "status missing object string") &&
         expect(cr::toString(cr::CreativeFacadeMutationStatus::Applied) ==
                    "Applied",
                "status applied string") &&
         expect(cr::toString(cr::CreativeFacadeMutationStatus::NoChange) ==
                    "NoChange",
                "status no change string") &&
         expect(cr::toString(cr::CreativeFacadeMutationStatus::Rejected) ==
                    "Rejected",
                "status rejected string");
}

cr::CreativeObjectId createRoomAt(cr::Facade& facade,
                                  cr::CreativeVec3 corner,
                                  bool locked = false) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  request.name = "Room";
  request.bounds.min = corner;
  request.bounds.max = {corner.x + 2.0, corner.y + 2.0, corner.z + 2.0};
  request.hasBoundsOverride = true;
  request.locked = locked;
  request.hasLockedOverride = locked;
  return facade.createDocumentObject(request).objectId;
}

cr::CreativeToolInputPacket movePress(cr::Id targetId) {
  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerPress;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = targetId;
  return input;
}

// TD-7: the creative viewport is a FRONT view (screen = world XY). The window
// fills worldDestination from the pointer as {worldX, worldY, <placeholder>}:
// screen-horizontal -> world X, screen-vertical -> world Y. The DEPTH axis
// (world Z) is the held axis the facade overrides with the start anchor.
cr::CreativeToolInputPacket moveDrag(cr::CreativeToolInputKind kind,
                                     double worldX,
                                     double worldY) {
  cr::CreativeToolInputPacket input;
  input.kind = kind;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.hasWorldDestination = true;
  input.pointer.worldDestination = {worldX, worldY, 0.0};
  return input;
}

cr::CreativeToolInputPacket moveDragToWorld(
    cr::CreativeToolInputKind kind,
    cr::CreativeToolWorldPoint worldDestination,
    cr::CreativeToolMoveHeldAxis heldAxis) {
  cr::CreativeToolInputPacket input;
  input.kind = kind;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.hasWorldDestination = true;
  input.pointer.worldDestination = worldDestination;
  input.pointer.moveHeldAxis = heldAxis;
  return input;
}

cr::CreativeToolInputPacket moveDragWithOptions(
    cr::CreativeToolInputKind kind,
    cr::CreativeToolWorldPoint worldDestination,
    cr::CreativeMoveConstraint constraint,
    double snapStep) {
  cr::CreativeToolInputPacket input = moveDragToWorld(
      kind, worldDestination, cr::CreativeToolMoveHeldAxis::Y);
  input.pointer.moveConstraint = constraint;
  input.pointer.hasMoveSnapStepOverride = true;
  input.pointer.moveSnapStepOverride = snapStep;
  return input;
}

bool dragCommitMovesRoomByCornerAnchor() {
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoomAt(facade, {1.0, 2.0, 1.0});
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  static_cast<void>(facade.dispatchToolInput(
      movePress(static_cast<cr::Id>(roomId))));
  const std::uint64_t revisionBefore = facade.document().revision();

  static_cast<void>(facade.dispatchToolInput(
      moveDrag(cr::CreativeToolInputKind::PointerMove, 5.0, 6.0)));
  const cr::CreativeFacadeToolDispatchReceipt commit = facade.dispatchToolInput(
      moveDrag(cr::CreativeToolInputKind::PointerRelease, 5.0, 6.0));

  const cr::CreativeObject* room = facade.findObject(roomId);
  const cr::CreativeFacadeMoveDragReceipt& drag = commit.moveDrag;
  return expect(room != nullptr, "drag commit room exists") &&
         // TD-2 corner anchor + TD-7 screen=XY drag: bounds.min tracks the
         // cursor in world X (horizontal) and world Y (vertical); the DEPTH
         // axis (world Z) holds the start anchor Z == 1.0.
         expect(room->bounds.min.x == 5.0, "drag commit corner x") &&
         expect(room->bounds.min.y == 6.0, "drag commit corner y tracks cursor") &&
         expect(room->bounds.min.z == 1.0, "drag commit corner z unchanged") &&
         expect(room->bounds.max.x == 7.0, "drag commit corner max x") &&
         expect(drag.stage == cr::CreativeFacadeMoveDragStage::Commit,
                "drag commit stage") &&
         expect(drag.outcome == cr::CreativeFacadeMoveDragOutcome::Applied,
                "drag commit applied") &&
         expect(drag.documentStatus ==
                    cr::CreativeDocumentMutationStatus::BatchApplied,
                "drag commit preserves applied document status") &&
         expect(drag.committed, "drag commit committed flag") &&
         expect(drag.changed, "drag commit changed") &&
         expect(drag.snappedAnchor.x == 5.0 && drag.snappedAnchor.y == 6.0,
                "drag commit snapped anchor") &&
         expect(drag.snappedAnchor.z == 1.0,
                "drag commit snapped anchor holds start z") &&
         expect(drag.startAnchor.x == 1.0 && drag.startAnchor.z == 1.0,
                "drag commit start anchor") &&
         expect(facade.document().revision() == revisionBefore + 1U,
                "drag commit bumps revision") &&
         expect(facade.moveDragReceipt().outcome ==
                    cr::CreativeFacadeMoveDragOutcome::Applied,
                "stored drag receipt applied");
}

bool dragCommitHeldYAxisSnapsOnlyXZWithCoreMask() {
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoomAt(facade, {0.0, 5.0, 0.0});
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  static_cast<void>(facade.dispatchToolInput(
      movePress(static_cast<cr::Id>(roomId))));
  const std::uint64_t revisionBefore = facade.document().revision();

  const cr::CreativeToolWorldPoint destination{10.4, 100.0, 10.6};
  const cr::CreativeFacadeToolDispatchReceipt preview =
      facade.dispatchToolInput(moveDragToWorld(
          cr::CreativeToolInputKind::PointerMove,
          destination,
          cr::CreativeToolMoveHeldAxis::Y));
  cr::CreativeSelectionPlacementRequest expectedRequest;
  expectedRequest.mode = cr::CreativeSelectionPlacementMode::Move;
  expectedRequest.sourceAnchor = preview.moveDrag.startAnchor;
  expectedRequest.targetAnchor = preview.moveDrag.snappedAnchor;
  const cr::CreativeSelectionPlacementPlan expectedPlan =
      cr::planCreativeSelectionPlacement(facade.document().objects(),
                                         expectedRequest);
  const cr::CreativeFacadeToolDispatchReceipt commit =
      facade.dispatchToolInput(moveDragToWorld(
          cr::CreativeToolInputKind::PointerRelease,
          destination,
          cr::CreativeToolMoveHeldAxis::Y));

  const cr::CreativeObject* room = facade.findObject(roomId);
  const cr::CreativeFacadeMoveDragReceipt& drag = commit.moveDrag;
  return expect(room != nullptr, "held-y room exists") &&
         expect(preview.moveDrag.requestedAnchor.y == 5.0,
                "held-y preview requested y anchored") &&
         expect(preview.moveDrag.snappedAnchor.x == 10.0,
                "held-y preview snapped x") &&
         expect(preview.moveDrag.snappedAnchor.y == 5.0,
                "held-y preview y preserved") &&
         expect(preview.moveDrag.snappedAnchor.z == 11.0,
                "held-y preview snapped z") &&
         expect(room->bounds.min.x == 10.0, "held-y commit bounds x") &&
         expect(room->bounds.min.y == 5.0, "held-y commit bounds y held") &&
         expect(room->bounds.min.z == 11.0, "held-y commit bounds z") &&
         expect(expectedPlan.accepted && expectedPlan.objects.size() == 1U &&
                    cr::creativeBoundsExactlyEqual(
                        room->bounds, expectedPlan.objects[0].bounds),
                "move drag commits the exact shared placement plan") &&
         expect(drag.snappedAnchor.x == 10.0, "held-y commit snapped x") &&
         expect(drag.snappedAnchor.y == 5.0,
                "held-y commit snapped y held") &&
         expect(drag.snappedAnchor.z == 11.0, "held-y commit snapped z") &&
         expect(drag.outcome == cr::CreativeFacadeMoveDragOutcome::Applied,
                "held-y commit applied") &&
         expect(facade.document().revision() == revisionBefore + 1U,
                "held-y commit bumps revision");
}

bool dragConstraintAndSnapOptionsApplyFromStartAnchor() {
  bool ok = true;
  {
    cr::Facade facade;
    const cr::CreativeObjectId roomId =
        createRoomAt(facade, {1.0, 2.0, 3.0});
    static_cast<void>(facade.setActiveTool(cr::Tool::Move));
    static_cast<void>(facade.dispatchToolInput(
        movePress(static_cast<cr::Id>(roomId))));

    const cr::CreativeToolWorldPoint destination{4.24, 99.0, 8.76};
    const cr::CreativeFacadeToolDispatchReceipt preview =
        facade.dispatchToolInput(moveDragWithOptions(
            cr::CreativeToolInputKind::PointerMove, destination,
            cr::CreativeMoveConstraint::X, 0.5));
    const cr::CreativeFacadeToolDispatchReceipt commit =
        facade.dispatchToolInput(moveDragWithOptions(
            cr::CreativeToolInputKind::PointerRelease, destination,
            cr::CreativeMoveConstraint::X, 0.5));
    const cr::CreativeObject* room = facade.findObject(roomId);

    ok = expect(preview.moveDrag.requestedAnchor.x == 4.24 &&
                    preview.moveDrag.requestedAnchor.y == 2.0 &&
                    preview.moveDrag.requestedAnchor.z == 3.0,
                "x constraint holds y and start z before snap") &&
         expect(preview.moveDrag.snappedAnchor.x == 4.0 &&
                    preview.moveDrag.snappedAnchor.z == 3.0,
                "x constraint uses half-meter snap") &&
         expect(room != nullptr && room->bounds.min.x == 4.0 &&
                    room->bounds.min.y == 2.0 && room->bounds.min.z == 3.0,
                "x constraint commits only x") &&
         expect(commit.moveDrag.outcome ==
                    cr::CreativeFacadeMoveDragOutcome::Applied,
                "x constraint commit applied") &&
         ok;
  }

  {
    cr::Facade facade;
    const cr::CreativeObjectId roomId =
        createRoomAt(facade, {1.0, 2.0, 3.0});
    static_cast<void>(facade.setActiveTool(cr::Tool::Move));
    static_cast<void>(facade.dispatchToolInput(
        movePress(static_cast<cr::Id>(roomId))));

    const cr::CreativeToolWorldPoint destination{4.24, 99.0, 8.76};
    const cr::CreativeFacadeToolDispatchReceipt commit =
        facade.dispatchToolInput(moveDragWithOptions(
            cr::CreativeToolInputKind::PointerRelease, destination,
            cr::CreativeMoveConstraint::Z, 0.5));
    const cr::CreativeObject* room = facade.findObject(roomId);

    ok = expect(commit.moveDrag.requestedAnchor.x == 1.0 &&
                    commit.moveDrag.requestedAnchor.y == 2.0 &&
                    commit.moveDrag.requestedAnchor.z == 8.76,
                "z constraint holds start x and y before snap") &&
         expect(commit.moveDrag.snappedAnchor.x == 1.0 &&
                    commit.moveDrag.snappedAnchor.z == 9.0,
                "z constraint uses half-meter snap") &&
         expect(room != nullptr && room->bounds.min.x == 1.0 &&
                    room->bounds.min.y == 2.0 && room->bounds.min.z == 9.0,
                "z constraint commits only z") &&
         expect(commit.moveDrag.outcome ==
                    cr::CreativeFacadeMoveDragOutcome::Applied,
                "z constraint commit applied") &&
         ok;
  }
  return ok;
}

bool dragRejectsInvalidOptionsWithoutMutation() {
  const auto rejected = [](cr::CreativeMoveConstraint constraint,
                           bool hasSnapOverride,
                           double snapStep) {
    cr::Facade facade;
    const cr::CreativeObjectId roomId =
        createRoomAt(facade, {1.0, 2.0, 3.0});
    static_cast<void>(facade.setActiveTool(cr::Tool::Move));
    static_cast<void>(facade.dispatchToolInput(
        movePress(static_cast<cr::Id>(roomId))));
    const std::uint64_t revisionBefore = facade.document().revision();

    cr::CreativeToolInputPacket input = moveDragToWorld(
        cr::CreativeToolInputKind::PointerRelease,
        {8.0, 9.0, 10.0}, cr::CreativeToolMoveHeldAxis::Y);
    input.pointer.moveConstraint = constraint;
    input.pointer.hasMoveSnapStepOverride = hasSnapOverride;
    input.pointer.moveSnapStepOverride = snapStep;
    const cr::CreativeFacadeToolDispatchReceipt receipt =
        facade.dispatchToolInput(input);
    const cr::CreativeObject* room = facade.findObject(roomId);

    return expect(receipt.moveDrag.outcome ==
                      cr::CreativeFacadeMoveDragOutcome::Rejected,
                  "invalid move option rejected") &&
           expect(receipt.moveDrag.message == "move_drag_options_invalid",
                  "invalid move option reason") &&
           expect(!receipt.moveDrag.changed && !receipt.moveDrag.committed,
                  "invalid move option does not commit") &&
           expect(room != nullptr && room->bounds.min.x == 1.0 &&
                      room->bounds.min.y == 2.0 && room->bounds.min.z == 3.0,
                  "invalid move option leaves room unchanged") &&
           expect(facade.document().revision() == revisionBefore,
                  "invalid move option preserves revision");
  };

  bool ok = rejected(cr::CreativeMoveConstraint::Count, false, 1.0);
  ok = rejected(cr::CreativeMoveConstraint::Free, true, -0.5) && ok;
  ok = rejected(cr::CreativeMoveConstraint::Free, true,
                std::numeric_limits<double>::quiet_NaN()) &&
       ok;
  ok = rejected(cr::CreativeMoveConstraint::Free, true,
                std::numeric_limits<double>::infinity()) &&
       ok;
  return ok;
}

bool dragCommitToStartAnchorIsNoChange() {
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoomAt(facade, {3.0, 0.0, 4.0});
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  static_cast<void>(facade.dispatchToolInput(
      movePress(static_cast<cr::Id>(roomId))));
  const std::uint64_t revisionBefore = facade.document().revision();

  // Release on the start anchor's own screen cell (world X=3, world Y=0): the
  // destination equals the start anchor, so it must be a no-change.
  const cr::CreativeFacadeToolDispatchReceipt commit = facade.dispatchToolInput(
      moveDrag(cr::CreativeToolInputKind::PointerRelease, 3.0, 0.0));

  const cr::CreativeObject* room = facade.findObject(roomId);
  return expect(room != nullptr && room->bounds.min.x == 3.0,
                "no-change room unmoved") &&
         expect(commit.moveDrag.outcome ==
                    cr::CreativeFacadeMoveDragOutcome::NoChange,
                "no-change outcome") &&
         expect(!commit.moveDrag.changed, "no-change not changed") &&
         expect(facade.document().revision() == revisionBefore,
                "no-change no revision bump");
}

bool dragCommitOnLockedRoomRefused() {
  cr::Facade facade;
  const cr::CreativeObjectId roomId =
      createRoomAt(facade, {1.0, 0.0, 1.0}, /*locked=*/true);
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  static_cast<void>(facade.dispatchToolInput(
      movePress(static_cast<cr::Id>(roomId))));
  const std::uint64_t revisionBefore = facade.document().revision();

  const cr::CreativeFacadeToolDispatchReceipt commit = facade.dispatchToolInput(
      moveDrag(cr::CreativeToolInputKind::PointerRelease, 7.0, 8.0));

  const cr::CreativeObject* room = facade.findObject(roomId);
  const cr::CreativeFacadeMoveDragReceipt& drag = commit.moveDrag;
  return expect(room != nullptr && room->bounds.min.x == 1.0,
                "locked drag room unmoved") &&
         expect(drag.outcome ==
                    cr::CreativeFacadeMoveDragOutcome::RejectedLocked,
                "locked drag outcome") &&
         expect(drag.locked, "locked drag locked flag") &&
         expect(!drag.changed, "locked drag not changed") &&
         expect(drag.documentStatus ==
                    cr::CreativeDocumentMutationStatus::ApplyFailed,
                "locked drag preserves failed document status") &&
         // TV1-A: the lock-refusal truth is observable in the receipt message.
         expect(!drag.message.empty(), "locked drag message present") &&
         expect(facade.document().revision() == revisionBefore,
                "locked drag no revision bump");
}

bool dragCancelDiscardsWithoutMutation() {
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoomAt(facade, {1.0, 0.0, 1.0});
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  static_cast<void>(facade.dispatchToolInput(
      movePress(static_cast<cr::Id>(roomId))));
  static_cast<void>(facade.dispatchToolInput(
      moveDrag(cr::CreativeToolInputKind::PointerMove, 9.0, 9.0)));
  const std::uint64_t revisionBefore = facade.document().revision();

  cr::CreativeToolInputPacket cancel;
  cancel.kind = cr::CreativeToolInputKind::Cancel;
  const cr::CreativeFacadeToolDispatchReceipt cancelReceipt =
      facade.dispatchToolInput(cancel);

  const cr::CreativeObject* room = facade.findObject(roomId);
  return expect(room != nullptr && room->bounds.min.x == 1.0,
                "cancel room unmoved") &&
         expect(cancelReceipt.moveDrag.outcome ==
                    cr::CreativeFacadeMoveDragOutcome::Cancelled,
                "cancel outcome") &&
         expect(cancelReceipt.moveDrag.message == "move_cancelled",
                "cancel message") &&
         expect(!facade.ghostState().visible, "cancel hides ghost") &&
         expect(facade.document().revision() == revisionBefore,
                "cancel no revision bump");
}

bool dragPressOnObjectSelectsAndTargetsIt() {
  // Move-press directly on an object selects it (TV1-C) and seeds the drag
  // with that same object (TV1-G).
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoomAt(facade, {2.0, 0.0, 2.0});
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));

  const cr::CreativeFacadeToolDispatchReceipt begin =
      facade.dispatchToolInput(movePress(static_cast<cr::Id>(roomId)));

  return expect(begin.moveDrag.stage == cr::CreativeFacadeMoveDragStage::Begin,
                "press begin stage") &&
         expect(begin.moveDrag.objectId == roomId, "press begin targets room") &&
         expect(begin.moveDrag.outcome ==
                    cr::CreativeFacadeMoveDragOutcome::Begun,
                "press begin outcome") &&
         expect(begin.moveDrag.hasStartAnchor, "press begin has start anchor") &&
         expect(begin.moveDrag.startAnchor.x == 2.0,
                "press begin start anchor x") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "press selects the object");
}

bool orphanReleaseThroughFacadeIsNoOp() {
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoomAt(facade, {1.0, 0.0, 1.0});
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  const std::uint64_t revisionBefore = facade.document().revision();

  // Release with NO preceding press (drag never began): harmless no-op.
  const cr::CreativeFacadeToolDispatchReceipt commit = facade.dispatchToolInput(
      moveDrag(cr::CreativeToolInputKind::PointerRelease, 7.0, 7.0));

  const cr::CreativeObject* room = facade.findObject(roomId);
  return expect(room != nullptr && room->bounds.min.x == 1.0,
                "orphan release room unmoved") &&
         expect(!commit.moveDrag.committed, "orphan release not committed") &&
         expect(facade.document().revision() == revisionBefore,
                "orphan release no revision bump");
}

}  // namespace

int main() {
  const bool ok = defaultNoSelectionRejects() &&
                  selectedMissingTargetRejectsAndPreservesSelection() &&
                  selectedRoomTogglesVisibleFalseAndPreservesState() &&
                  secondToggleRestoresVisibilityAndIncrementsAgain() &&
                  activeToolRemainsUnchangedByToggle() &&
                  lockToggleWithoutSelectionRejects() &&
                  lockToggleLocksSelectedRoomWithReceipt() &&
                  lockToggleUnlocksLockedRoomDespiteLockGate() &&
                  visibilityToggleRejectsWhileLocked() &&
                  lockedObjectRemovalRefusedThroughFacade() &&
                  unlockedObjectRemovalSucceedsAfterUnlock() &&
                  dragCommitMovesRoomByCornerAnchor() &&
                  dragCommitHeldYAxisSnapsOnlyXZWithCoreMask() &&
                  dragConstraintAndSnapOptionsApplyFromStartAnchor() &&
                  dragRejectsInvalidOptionsWithoutMutation() &&
                  dragCommitToStartAnchorIsNoChange() &&
                  dragCommitOnLockedRoomRefused() &&
                  dragCancelDiscardsWithoutMutation() &&
                  dragPressOnObjectSelectsAndTargetsIt() &&
                  orphanReleaseThroughFacadeIsNoOp() &&
                  facadeMutationStatusStringsAreStable();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
