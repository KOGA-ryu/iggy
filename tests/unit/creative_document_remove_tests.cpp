#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/Facade.hpp"

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

cr::CreativeDocumentCreateReceipt createRoom(cr::CreativeDocument& document,
                                             std::string_view name) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  request.name = std::string{name};
  return document.createObject(request);
}

cr::CreativeToolInputPacket pointerInput(cr::CreativeToolInputKind kind,
                                         cr::CreativeObjectId objectId) {
  cr::CreativeToolInputPacket input;
  input.kind = kind;
  input.pointer.x = 1.0;
  input.pointer.y = 2.0;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = static_cast<cr::Id>(objectId);
  return input;
}

bool documentRemoveSuccessCopiesMetadataAndReindexes() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt first =
      createRoom(document, "First Room");
  const cr::CreativeDocumentCreateReceipt second =
      createRoom(document, "Second Room");
  cr::CreativeDocumentRemoveRequest request;
  request.objectId = first.objectId;

  const cr::CreativeDocumentRemoveReceipt receipt =
      document.removeDocumentObject(request);
  const cr::CreativeObject* remaining = document.findObject(second.objectId);

  return expect(first.accepted, "remove setup first accepted") &&
         expect(second.accepted, "remove setup second accepted") &&
         expect(receipt.requested, "remove requested") &&
         expect(receipt.accepted, "remove accepted") &&
         expect(receipt.changed, "remove changed") &&
         expect(receipt.objectRemoved, "object removed") &&
         expect(receipt.status == cr::CreativeDocumentRemoveStatus::Removed,
                "remove status") &&
         expect(receipt.message == "object_removed", "remove message") &&
         expect(receipt.reasonCode == "object_removed", "remove reason") &&
         expect(receipt.objectId == first.objectId, "remove object id") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Room,
                "remove object kind") &&
         expect(receipt.objectName == "First Room", "remove object name") &&
         expect(receipt.revisionBefore == 2U, "remove revision before") &&
         expect(receipt.revisionAfter == 3U, "remove revision after") &&
         expect(document.revision() == 3U, "document revision after remove") &&
         expect(document.objectCount() == 1U, "document count after remove") &&
         expect(!document.containsObject(first.objectId),
                "removed object not contained") &&
         expect(document.findObject(first.objectId) == nullptr,
                "removed object not findable") &&
         expect(document.containsObject(second.objectId),
                "remaining object contained") &&
         expect(remaining != nullptr, "remaining object findable") &&
         expect(remaining != nullptr && remaining->name == "Second Room",
                "remaining object name");
}

bool documentRemoveMissingRejectsWithoutRevisionChange() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt existing =
      createRoom(document, "Existing Room");
  const std::uint64_t revisionBeforeRemove = document.revision();

  const cr::CreativeDocumentRemoveReceipt receipt =
      document.removeDocumentObject(9999);

  return expect(existing.accepted, "missing setup accepted") &&
         expect(receipt.requested, "missing remove requested") &&
         expect(!receipt.accepted, "missing remove not accepted") &&
         expect(!receipt.changed, "missing remove unchanged") &&
         expect(!receipt.objectRemoved, "missing remove no object") &&
         expect(receipt.status ==
                    cr::CreativeDocumentRemoveStatus::MissingObject,
                "missing remove status") &&
         expect(receipt.objectId == 9999U, "missing remove object id") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Unknown,
                "missing remove kind unknown") &&
         expect(receipt.objectName.empty(), "missing remove name empty") &&
         expect(receipt.revisionBefore == revisionBeforeRemove,
                "missing remove revision before") &&
         expect(receipt.revisionAfter == revisionBeforeRemove,
                "missing remove revision after") &&
         expect(receipt.message == "missing_object",
                "missing remove message") &&
         expect(receipt.reasonCode == "missing_object",
                "missing remove reason") &&
         expect(document.objectCount() == 1U, "missing remove count stable") &&
         expect(document.revision() == revisionBeforeRemove,
                "missing remove document revision stable") &&
         expect(document.findObject(existing.objectId) != nullptr,
                "missing remove keeps existing object");
}

bool documentRemoveInvalidIdRejectsDeterministically() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt existing =
      createRoom(document, "Existing Room");
  const std::uint64_t revisionBeforeRemove = document.revision();

  const cr::CreativeDocumentRemoveReceipt receipt =
      document.removeDocumentObject(cr::kInvalidObjectId);

  return expect(existing.accepted, "invalid setup accepted") &&
         expect(receipt.requested, "invalid remove requested") &&
         expect(!receipt.accepted, "invalid remove not accepted") &&
         expect(!receipt.changed, "invalid remove unchanged") &&
         expect(!receipt.objectRemoved, "invalid remove no object") &&
         expect(receipt.status ==
                    cr::CreativeDocumentRemoveStatus::InvalidObjectId,
                "invalid remove status") &&
         expect(receipt.objectId == cr::kInvalidObjectId,
                "invalid remove object id") &&
         expect(receipt.revisionBefore == revisionBeforeRemove,
                "invalid remove revision before") &&
         expect(receipt.revisionAfter == revisionBeforeRemove,
                "invalid remove revision after") &&
         expect(receipt.message == "invalid_object_id",
                "invalid remove message") &&
         expect(receipt.reasonCode == "invalid_object_id",
                "invalid remove reason") &&
         expect(document.objectCount() == 1U, "invalid remove count stable") &&
         expect(document.findObject(existing.objectId) != nullptr,
                "invalid remove keeps existing object");
}

bool receiptedRemoveKeepsMissingAndExistingSemantics() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt first =
      createRoom(document, "First Room");
  const cr::CreativeDocumentCreateReceipt second =
      createRoom(document, "Second Room");
  const std::uint64_t revisionBeforeMissing = document.revision();

  const cr::CreativeDocumentRemoveReceipt missing =
      document.removeDocumentObject(9999);
  const cr::CreativeDocumentRemoveReceipt existing =
      document.removeDocumentObject(first.objectId);

  return expect(first.accepted, "receipted first accepted") &&
         expect(second.accepted, "receipted second accepted") &&
         expect(!missing.objectRemoved, "receipted missing not removed") &&
         expect(document.revision() == revisionBeforeMissing + 1U,
                "receipted existing increments once") &&
         expect(existing.objectRemoved, "receipted existing removed") &&
         expect(document.objectCount() == 1U, "receipted count") &&
         expect(document.findObject(first.objectId) == nullptr,
                "receipted removed object gone") &&
         expect(document.findObject(second.objectId) != nullptr,
                "receipted remaining object findable");
}

bool documentRemoveRefusesLockedObject() {
  cr::CreativeDocument document;
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  request.name = "Locked Room";
  request.locked = true;
  request.hasLockedOverride = true;
  const cr::CreativeDocumentCreateReceipt created =
      document.createObject(request);
  const std::uint64_t revisionBeforeRemove = document.revision();
  const cr::CreativeObjectDirtyFlags dirtyBeforeRemove =
      document.dirtyFlags();

  const cr::CreativeDocumentRemoveReceipt receipt =
      document.removeDocumentObject(created.objectId);

  return expect(created.accepted, "locked doc remove setup accepted") &&
         expect(receipt.requested, "locked doc remove requested") &&
         expect(!receipt.accepted, "locked doc remove not accepted") &&
         expect(!receipt.changed, "locked doc remove unchanged") &&
         expect(!receipt.objectRemoved, "locked doc remove no removal") &&
         expect(receipt.status ==
                    cr::CreativeDocumentRemoveStatus::LockedObject,
                "locked doc remove status") &&
         expect(receipt.objectId == created.objectId,
                "locked doc remove object id") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Room,
                "locked doc remove object kind") &&
         expect(receipt.objectName == "Locked Room",
                "locked doc remove object name") &&
         expect(receipt.removalDirtyFlags == 0U,
                "locked doc remove no dirty flags") &&
         expect(receipt.message == "object is locked",
                "locked doc remove message") &&
         expect(receipt.reasonCode == "object is locked",
                "locked doc remove reason") &&
         expect(receipt.revisionBefore == revisionBeforeRemove,
                "locked doc remove revision before") &&
         expect(receipt.revisionAfter == revisionBeforeRemove,
                "locked doc remove revision after") &&
         expect(document.revision() == revisionBeforeRemove,
                "locked doc remove document revision stable") &&
         expect(document.dirtyFlags() == dirtyBeforeRemove,
                "locked doc remove dirty flags stable") &&
         expect(document.objectCount() == 1U,
                "locked doc remove object retained") &&
         expect(document.findObject(created.objectId) != nullptr,
                "locked doc remove object findable");
}

bool documentRemoveSucceedsAfterUnlockThroughPipeline() {
  cr::CreativeDocument document;
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  request.name = "Locked Room";
  request.locked = true;
  request.hasLockedOverride = true;
  const cr::CreativeDocumentCreateReceipt created =
      document.createObject(request);
  const cr::CreativeDocumentRemoveReceipt refused =
      document.removeDocumentObject(created.objectId);

  const cr::CreativeDocumentMutationReceipt unlocked =
      cr::setDocumentObjectLocked(document, created.objectId, false);
  const cr::CreativeDocumentRemoveReceipt removed =
      document.removeDocumentObject(created.objectId);

  return expect(created.accepted, "unlock doc remove setup accepted") &&
         expect(refused.status ==
                    cr::CreativeDocumentRemoveStatus::LockedObject,
                "unlock doc remove refused first") &&
         expect(unlocked.status ==
                    cr::CreativeDocumentMutationStatus::Applied,
                "unlock doc remove unlock applied") &&
         expect(removed.accepted, "unlock doc remove accepted") &&
         expect(removed.objectRemoved, "unlock doc remove removed") &&
         expect(removed.status == cr::CreativeDocumentRemoveStatus::Removed,
                "unlock doc remove status") &&
         expect(document.objectCount() == 0U,
                "unlock doc remove document empty");
}

bool toStringCoversRemoveStatuses() {
  return expect(cr::toString(cr::CreativeDocumentRemoveStatus::Unknown) ==
                    "Unknown",
                "remove status unknown string") &&
         expect(cr::toString(
                    cr::CreativeDocumentRemoveStatus::InvalidDocument) ==
                    "InvalidDocument",
                "remove status invalid document string") &&
         expect(cr::toString(
                    cr::CreativeDocumentRemoveStatus::InvalidObjectId) ==
                    "InvalidObjectId",
                "remove status invalid id string") &&
         expect(cr::toString(
                    cr::CreativeDocumentRemoveStatus::MissingObject) ==
                    "MissingObject",
                "remove status missing string") &&
         expect(cr::toString(
                    cr::CreativeDocumentRemoveStatus::LockedObject) ==
                    "LockedObject",
                "remove status locked string") &&
         expect(cr::toString(cr::CreativeDocumentRemoveStatus::Removed) ==
                    "Removed",
                "remove status removed string");
}

bool facadeReceiptedRemoveInvalidatesSelectedTarget() {
  cr::Facade facade;
  const cr::CreativeDocumentCreateReceipt created =
      facade.createDocumentObject(cr::CreativeObjectKind::Room);
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress, created.objectId)));
  const std::uint64_t revisionBeforeRemove = facade.document().revision();

  const cr::CreativeDocumentRemoveReceipt receipt =
      facade.removeDocumentObject(created.objectId);

  return expect(created.accepted, "facade remove setup accepted") &&
         expect(receipt.accepted, "facade remove accepted") &&
         expect(receipt.objectRemoved, "facade object removed") &&
         expect(receipt.objectId == created.objectId,
                "facade remove object id") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Room,
                "facade remove kind") &&
         expect(receipt.revisionBefore == revisionBeforeRemove,
                "facade remove revision before") &&
         expect(receipt.revisionAfter == revisionBeforeRemove + 1U,
                "facade remove revision after") &&
         expect(facade.document().objectCount() == 0U,
                "facade remove document empty") &&
         expect(facade.selectionState().selectedTarget.value ==
                    cr::kInvalidId,
                "facade remove clears selection") &&
         expect(facade.toolState().pointer.target.value == cr::kInvalidId,
                "facade remove clears pointer target") &&
         expect(facade.stats().commandAttempts == 2U,
                "facade remove attempts") &&
         expect(facade.stats().commandSuccesses == 2U,
                "facade remove successes") &&
         expect(facade.stats().commandFailures == 0U,
                "facade remove failures");
}

bool facadeReceiptedMissingRemovePreservesStateAndRecordsFailure() {
  cr::Facade facade;
  const cr::CreativeDocumentCreateReceipt created =
      facade.createDocumentObject(cr::CreativeObjectKind::Room);
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress, created.objectId)));
  const std::uint64_t revisionBeforeRemove = facade.document().revision();

  const cr::CreativeDocumentRemoveReceipt receipt =
      facade.removeDocumentObject(9999);

  return expect(created.accepted, "facade missing setup accepted") &&
         expect(!receipt.accepted, "facade missing not accepted") &&
         expect(!receipt.objectRemoved, "facade missing no object") &&
         expect(receipt.status == cr::CreativeDocumentRemoveStatus::MissingObject,
                "facade missing status") &&
         expect(receipt.revisionBefore == revisionBeforeRemove,
                "facade missing revision before") &&
         expect(receipt.revisionAfter == revisionBeforeRemove,
                "facade missing revision after") &&
         expect(facade.document().objectCount() == 1U,
                "facade missing keeps object") &&
         expect(facade.selectionState().selectedTarget.value ==
                    static_cast<cr::Id>(created.objectId),
                "facade missing preserves selection") &&
         expect(facade.toolState().pointer.target.value ==
                    static_cast<cr::Id>(created.objectId),
                "facade missing preserves pointer target") &&
         expect(facade.stats().commandAttempts == 2U,
                "facade missing attempts") &&
         expect(facade.stats().commandSuccesses == 1U,
                "facade missing successes") &&
         expect(facade.stats().commandFailures == 1U,
                "facade missing failures");
}

}  // namespace

int main() {
  const bool ok = documentRemoveSuccessCopiesMetadataAndReindexes() &&
                  documentRemoveMissingRejectsWithoutRevisionChange() &&
                  documentRemoveInvalidIdRejectsDeterministically() &&
                  receiptedRemoveKeepsMissingAndExistingSemantics() &&
                  documentRemoveRefusesLockedObject() &&
                  documentRemoveSucceedsAfterUnlockThroughPipeline() &&
                  toStringCoversRemoveStatuses() &&
                  facadeReceiptedRemoveInvalidatesSelectedTarget() &&
                  facadeReceiptedMissingRemovePreservesStateAndRecordsFailure();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
