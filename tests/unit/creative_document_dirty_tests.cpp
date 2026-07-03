#include "app/iggy3d/creative/Document.hpp"
#include "app/iggy3d/creative/DocumentMutation.hpp"
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

constexpr cr::CreativeObjectDirtyFlags dirtyFlag(
    cr::CreativeObjectDirtyFlag flag) noexcept {
  return static_cast<cr::CreativeObjectDirtyFlags>(flag);
}

constexpr cr::CreativeObjectDirtyFlags documentIdentityDirtyFlags() noexcept {
  return dirtyFlag(cr::CreativeObjectDirtyFlag::Identity) |
         dirtyFlag(cr::CreativeObjectDirtyFlag::Preview) |
         dirtyFlag(cr::CreativeObjectDirtyFlag::Serialization);
}

cr::CreativeDocumentCreateReceipt createObject(
    cr::CreativeDocument& document,
    cr::CreativeObjectKind kind,
    std::string_view name = {}) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::string{name};
  return document.createObject(request);
}

bool defaultDocumentDirtyFlagsAreEmptyAndDrainable() {
  cr::CreativeDocument document;

  const cr::CreativeObjectDirtyFlags drained = document.drainDirtyFlags();

  return expect(document.dirtyFlags() == 0U, "default dirty flags") &&
         expect(drained == 0U, "default drain returns zero") &&
         expect(document.dirtyFlags() == 0U, "default drain leaves zero") &&
         expect(document.revision() == 0U, "default drain revision stable");
}

bool resetClearsAccumulatedDirtyFlags() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt created =
      createObject(document, cr::CreativeObjectKind::Room);

  document.reset();

  return expect(created.accepted, "reset setup create accepted") &&
         expect(document.dirtyFlags() == 0U, "reset clears dirty flags") &&
         expect(document.drainDirtyFlags() == 0U, "reset drain zero") &&
         expect(document.revision() == 0U, "reset revision zero");
}

bool genericRoomCreateAccumulatesCreationFlagsAndDrainClears() {
  cr::CreativeDocument document;
  const cr::CreativeObjectDirtyFlags expected =
      cr::dirtyFlagsForCreation(cr::CreativeObjectKind::Room);

  const cr::CreativeDocumentCreateReceipt receipt =
      createObject(document, cr::CreativeObjectKind::Room);
  const std::uint64_t revisionAfterCreate = document.revision();
  const cr::CreativeObjectDirtyFlags drained = document.drainDirtyFlags();

  return expect(receipt.accepted, "create dirty accepted") &&
         expect(receipt.creationDirtyFlags == expected,
                "create receipt dirty flags") &&
         expect(drained == expected, "create drain returns creation flags") &&
         expect(document.dirtyFlags() == 0U, "create drain clears") &&
         expect(document.revision() == revisionAfterCreate,
                "create drain revision stable");
}

bool rejectedCreateLeavesDirtyFlagsEmpty() {
  cr::CreativeDocument document;
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Unknown;

  const cr::CreativeDocumentCreateReceipt receipt = document.createObject(request);

  return expect(!receipt.accepted, "rejected create not accepted") &&
         expect(document.dirtyFlags() == 0U, "rejected create dirty zero") &&
         expect(document.drainDirtyFlags() == 0U,
                "rejected create drain zero") &&
         expect(document.revision() == 0U, "rejected create revision zero");
}

bool successfulRemoveAccumulatesRemovalFlags() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt created =
      createObject(document, cr::CreativeObjectKind::Room, "Room");
  static_cast<void>(document.drainDirtyFlags());
  const cr::CreativeObjectDirtyFlags expected =
      cr::dirtyFlagsForCreation(cr::CreativeObjectKind::Room) |
      documentIdentityDirtyFlags();

  const cr::CreativeDocumentRemoveReceipt removed =
      document.removeDocumentObject(created.objectId);
  const cr::CreativeObjectDirtyFlags drained = document.drainDirtyFlags();

  return expect(created.accepted, "remove dirty setup accepted") &&
         expect(removed.accepted, "remove dirty accepted") &&
         expect(removed.objectRemoved, "remove dirty object removed") &&
         expect(removed.removalDirtyFlags == expected,
                "remove receipt dirty flags") &&
         expect(drained == expected, "remove drain returns removal flags") &&
         expect(document.dirtyFlags() == 0U, "remove drain clears") &&
         expect(document.revision() == removed.revisionAfter,
                "remove drain revision stable");
}

bool missingAndInvalidRemoveLeaveDirtyFlagsUnchanged() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt created =
      createObject(document, cr::CreativeObjectKind::Room, "Room");
  static_cast<void>(document.drainDirtyFlags());

  const cr::CreativeDocumentRemoveReceipt missing =
      document.removeDocumentObject(9999);
  const cr::CreativeDocumentRemoveReceipt invalid =
      document.removeDocumentObject(cr::kInvalidObjectId);

  return expect(created.accepted, "remove unchanged setup accepted") &&
         expect(!missing.accepted, "missing remove not accepted") &&
         expect(!invalid.accepted, "invalid remove not accepted") &&
         expect(missing.removalDirtyFlags == 0U,
                "missing remove dirty receipt zero") &&
         expect(invalid.removalDirtyFlags == 0U,
                "invalid remove dirty receipt zero") &&
         expect(document.dirtyFlags() == 0U,
                "missing invalid remove dirty zero") &&
         expect(document.drainDirtyFlags() == 0U,
                "missing invalid remove drain zero") &&
         expect(document.objectCount() == 1U,
                "missing invalid remove object count stable");
}

bool setVisibleAppliedAccumulatesMutationFlags() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt created =
      createObject(document, cr::CreativeObjectKind::Room, "Room");
  static_cast<void>(document.drainDirtyFlags());
  const cr::CreativeObjectDirtyFlags expected =
      cr::dirtyFlagsForMutation(cr::CreativeObjectKind::Room,
                                cr::CreativeMutationKind::SetVisible);

  const cr::CreativeDocumentMutationReceipt receipt =
      cr::setDocumentObjectVisible(document, created.objectId, false);
  const cr::CreativeObjectDirtyFlags drained = document.drainDirtyFlags();

  return expect(created.accepted, "visible dirty setup accepted") &&
         expect(receipt.status == cr::CreativeDocumentMutationStatus::Applied,
                "visible dirty status applied") &&
         expect(receipt.changed, "visible dirty changed") &&
         expect(receipt.dirtyFlags == expected,
                "visible receipt dirty flags") &&
         expect(drained == expected,
                "visible drain returns mutation flags") &&
         expect(document.dirtyFlags() == 0U, "visible drain clears");
}

bool noChangeVisibilityLeavesAccumulatorEmpty() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt created =
      createObject(document, cr::CreativeObjectKind::Room, "Room");
  static_cast<void>(document.drainDirtyFlags());

  const cr::CreativeDocumentMutationReceipt receipt =
      cr::setDocumentObjectVisible(document, created.objectId, true);

  return expect(created.accepted, "visible no-change setup accepted") &&
         expect(receipt.status == cr::CreativeDocumentMutationStatus::NoChange,
                "visible no-change status") &&
         expect(!receipt.changed, "visible no-change changed false") &&
         expect(receipt.dirtyFlags == 0U, "visible no-change receipt dirty") &&
         expect(document.dirtyFlags() == 0U,
                "visible no-change dirty zero") &&
         expect(document.drainDirtyFlags() == 0U,
                "visible no-change drain zero");
}

bool futureStorageNoChangeLeavesAccumulatorEmpty() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt created =
      createObject(document, cr::CreativeObjectKind::Note, "Note");
  static_cast<void>(document.drainDirtyFlags());

  const cr::CreativeDocumentMutationReceipt receipt =
      cr::applyDocumentMutation(document,
                                created.objectId,
                                cr::CreativeMutationKind::EditText,
                                cr::makeTextPayload("hello"));

  return expect(created.accepted, "future no-change setup accepted") &&
         expect(receipt.status == cr::CreativeDocumentMutationStatus::NoChange,
                "future no-change status") &&
         expect(!receipt.changed, "future no-change changed false") &&
         expect(receipt.dirtyFlags == 0U, "future no-change receipt dirty") &&
         expect(receipt.objectReceipt.message ==
                    "mutation has no stored object field yet",
                "future no-change message") &&
         expect(document.dirtyFlags() == 0U,
                "future no-change dirty zero") &&
         expect(document.drainDirtyFlags() == 0U,
                "future no-change drain zero");
}

bool sequentialAppliedMutationsOrDirtyFlagsAndDrainOnce() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt created =
      createObject(document, cr::CreativeObjectKind::Room, "Room");
  static_cast<void>(document.drainDirtyFlags());
  const cr::CreativeObjectDirtyFlags expected =
      cr::dirtyFlagsForMutation(cr::CreativeObjectKind::Room,
                                cr::CreativeMutationKind::SetVisible) |
      cr::dirtyFlagsForMutation(cr::CreativeObjectKind::Room,
                                cr::CreativeMutationKind::SetLocked);

  const cr::CreativeDocumentMutationReceipt visible =
      cr::setDocumentObjectVisible(document, created.objectId, false);
  const cr::CreativeDocumentMutationReceipt locked =
      cr::setDocumentObjectLocked(document, created.objectId, true);
  const cr::CreativeObjectDirtyFlags drained = document.drainDirtyFlags();

  return expect(created.accepted, "sequential setup accepted") &&
         expect(visible.changed, "sequential visible changed") &&
         expect(locked.changed, "sequential locked changed") &&
         expect(drained == expected, "sequential drain returns or flags") &&
         expect(document.dirtyFlags() == 0U, "sequential drain clears") &&
         expect(document.revision() == locked.revisionAfter,
                "sequential drain revision stable");
}

bool facadePathsExposeDocumentAccumulator() {
  cr::Facade facade;

  const cr::CreativeDocumentCreateReceipt created =
      facade.createDocumentObject(cr::CreativeObjectKind::Room);
  const cr::CreativeObjectDirtyFlags afterCreate =
      cr::dirtyFlagsForCreation(cr::CreativeObjectKind::Room);

  return expect(created.accepted, "facade dirty create accepted") &&
         expect(facade.document().dirtyFlags() == afterCreate,
                "facade exposes document dirty flags");
}

}  // namespace

int main() {
  const bool ok = defaultDocumentDirtyFlagsAreEmptyAndDrainable() &&
                  resetClearsAccumulatedDirtyFlags() &&
                  genericRoomCreateAccumulatesCreationFlagsAndDrainClears() &&
                  rejectedCreateLeavesDirtyFlagsEmpty() &&
                  successfulRemoveAccumulatesRemovalFlags() &&
                  missingAndInvalidRemoveLeaveDirtyFlagsUnchanged() &&
                  setVisibleAppliedAccumulatesMutationFlags() &&
                  noChangeVisibilityLeavesAccumulatorEmpty() &&
                  futureStorageNoChangeLeavesAccumulatorEmpty() &&
                  sequentialAppliedMutationsOrDirtyFlagsAndDrainOnce() &&
                  facadePathsExposeDocumentAccumulator();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
