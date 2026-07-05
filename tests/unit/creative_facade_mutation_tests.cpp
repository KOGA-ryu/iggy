#include "app/iggy3d/creative/Facade.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
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

void inspectTarget(cr::Facade& facade, cr::Id targetId) {
  static_cast<void>(facade.setActiveTool(cr::Tool::Inspect));
  static_cast<void>(facade.dispatchToolInput(pointerPress(targetId)));
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

bool inspectionTargetRemainsUnchanged() {
  cr::Facade facade;
  const cr::CreativeObjectId roomId = createRoom(facade);
  selectTarget(facade, roomId);
  inspectTarget(facade, 77);
  const cr::TargetRef inspectedBefore = facade.inspectionState().inspectedTarget;

  const cr::CreativeFacadeMutationReceipt receipt =
      facade.toggleSelectedObjectVisibility();

  return expect(receipt.accepted, "inspection toggle accepted") &&
         expect(facade.selectionState().selectedTarget.value == roomId,
                "inspection selection preserved") &&
         expect(facade.inspectionState().inspectedTarget.value ==
                    inspectedBefore.value,
                "inspection target preserved");
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

}  // namespace

int main() {
  const bool ok = defaultNoSelectionRejects() &&
                  selectedMissingTargetRejectsAndPreservesSelection() &&
                  selectedRoomTogglesVisibleFalseAndPreservesState() &&
                  secondToggleRestoresVisibilityAndIncrementsAgain() &&
                  inspectionTargetRemainsUnchanged() &&
                  lockToggleWithoutSelectionRejects() &&
                  lockToggleLocksSelectedRoomWithReceipt() &&
                  lockToggleUnlocksLockedRoomDespiteLockGate() &&
                  visibilityToggleRejectsWhileLocked() &&
                  lockedObjectRemovalRefusedThroughFacade() &&
                  unlockedObjectRemovalSucceedsAfterUnlock() &&
                  facadeMutationStatusStringsAreStable();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
