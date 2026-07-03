#include "app/iggy3d/creative/DocumentMutation.hpp"

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

cr::CreativeDocument makeDocumentWithRoom(cr::CreativeObjectId& roomId) {
  cr::CreativeDocument document = cr::CreativeDocument::create("Document");
  roomId = document.createRoom(
      "Room",
      cr::CreativeTransform{},
      cr::CreativeBounds{cr::CreativeVec3{0.0, 0.0, 0.0},
                         cr::CreativeVec3{1.0, 1.0, 1.0}});
  return document;
}

bool setVisibleMutatesThroughDocumentGateway() {
  cr::CreativeObjectId roomId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithRoom(roomId);
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt =
      cr::setDocumentObjectVisible(document, roomId, false);
  const cr::CreativeObject* room = document.findObject(roomId);

  return expect(room != nullptr, "visibility room exists") &&
         expect(!room->visible, "visibility changed to false") &&
         expect(receipt.status == cr::CreativeDocumentMutationStatus::Applied,
                "visibility document status applied") &&
         expect(cr::documentMutationSucceeded(receipt.status),
                "visibility status succeeded") &&
         expect(cr::documentMutationChanged(receipt.status),
                "visibility status changed") &&
         expect(receipt.objectId == roomId, "visibility object id") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Room,
                "visibility object kind") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::SetVisible,
                "visibility mutation kind") &&
         expect(receipt.revisionBefore == revisionBefore,
                "visibility revision before") &&
         expect(receipt.revisionAfter == revisionBefore + 1U,
                "visibility revision after") &&
         expect(document.revision() == revisionBefore + 1U,
                "visibility document revision advanced") &&
         expect(receipt.changed, "visibility changed flag") &&
         expect(receipt.allowed, "visibility allowed") &&
         expect(receipt.dirtyFlags ==
                    cr::dirtyFlagsForMutation(cr::CreativeObjectKind::Room,
                                              cr::CreativeMutationKind::SetVisible),
                "visibility dirty flags") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::Applied,
                "visibility object status applied") &&
         expect(receipt.objectReceipt.changed, "visibility object changed") &&
         expect(receipt.objectReceipt.allowed, "visibility object allowed") &&
         expect(receipt.objectReceipt.message == "object visibility changed",
                "visibility object message") &&
         expect(receipt.message ==
                    "document mutation applied through object mutation pipeline",
                "visibility document message");
}

bool settingAlreadyCurrentVisibilityIsNoChange() {
  cr::CreativeObjectId roomId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithRoom(roomId);
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt =
      cr::setDocumentObjectVisible(document, roomId, true);
  const cr::CreativeObject* room = document.findObject(roomId);

  return expect(room != nullptr, "no-change room exists") &&
         expect(room->visible, "no-change room remains visible") &&
         expect(receipt.status == cr::CreativeDocumentMutationStatus::NoChange,
                "no-change status") &&
         expect(cr::documentMutationSucceeded(receipt.status),
                "no-change status succeeded") &&
         expect(!cr::documentMutationChanged(receipt.status),
                "no-change status not changed") &&
         expect(!receipt.changed, "no-change changed flag") &&
         expect(receipt.allowed, "no-change allowed") &&
         expect(receipt.revisionBefore == revisionBefore,
                "no-change revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "no-change revision after") &&
         expect(document.revision() == revisionBefore,
                "no-change document revision unchanged") &&
         expect(receipt.dirtyFlags == 0U, "no-change dirty flags") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::NoChange,
                "no-change object status") &&
         expect(!receipt.objectReceipt.changed, "no-change object changed") &&
         expect(receipt.objectReceipt.allowed, "no-change object allowed") &&
         expect(receipt.objectReceipt.message ==
                    "object visibility already matches requested value",
                "no-change object message");
}

bool setVisibleFalseThenTrueIncrementsForEachRealChange() {
  cr::CreativeObjectId roomId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithRoom(roomId);
  const cr::CreativeDocumentMutationReceipt first =
      cr::setDocumentObjectVisible(document, roomId, false);
  const std::uint64_t revisionBeforeSecond = document.revision();

  const cr::CreativeDocumentMutationReceipt second =
      cr::setDocumentObjectVisible(document, roomId, true);
  const cr::CreativeObject* room = document.findObject(roomId);

  return expect(first.changed, "second setup changed first") &&
         expect(room != nullptr, "second room exists") &&
         expect(room->visible, "second room visible") &&
         expect(second.status == cr::CreativeDocumentMutationStatus::Applied,
                "second status applied") &&
         expect(second.changed, "second changed") &&
         expect(second.allowed, "second allowed") &&
         expect(second.revisionBefore == revisionBeforeSecond,
                "second revision before") &&
         expect(second.revisionAfter == revisionBeforeSecond + 1U,
                "second revision after") &&
         expect(document.revision() == revisionBeforeSecond + 1U,
                "second document revision advanced") &&
         expect(second.objectReceipt.message == "object visibility changed",
                "second object message");
}

bool missingObjectRejectsWithoutRevisionAdvance() {
  cr::CreativeObjectId roomId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithRoom(roomId);
  const std::uint64_t revisionBefore = document.revision();
  constexpr cr::CreativeObjectId missingId = 9999;

  const cr::CreativeDocumentMutationReceipt receipt =
      cr::setDocumentObjectVisible(document, missingId, false);

  return expect(receipt.status ==
                    cr::CreativeDocumentMutationStatus::MissingObject,
                "missing status") &&
         expect(cr::documentMutationFailed(receipt.status),
                "missing status failed") &&
         expect(!cr::documentMutationChanged(receipt.status),
                "missing status not changed") &&
         expect(receipt.objectId == missingId, "missing object id") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Unknown,
                "missing object kind") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::SetVisible,
                "missing mutation kind") &&
         expect(receipt.revisionBefore == revisionBefore,
                "missing revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "missing revision after") &&
         expect(document.revision() == revisionBefore,
                "missing document revision unchanged") &&
         expect(!receipt.changed, "missing changed flag") &&
         expect(!receipt.allowed, "missing allowed") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::Rejected,
                "missing object status rejected") &&
         expect(receipt.message == "document does not contain requested object",
                "missing document message") &&
         expect(receipt.objectReceipt.message ==
                    "document does not contain requested object",
                "missing object message");
}

bool unsupportedMutationDoesNotFalselyReportApplied() {
  cr::CreativeObjectId roomId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithRoom(roomId);
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt = cr::applyDocumentMutation(
      document, roomId, cr::CreativeMutationKind::Move,
      cr::makeMovePayload(cr::CreativeVec3{1.0, 2.0, 3.0}));

  return expect(receipt.status == cr::CreativeDocumentMutationStatus::ApplyFailed,
                "unsupported status failed") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::UnsupportedMutation,
                "unsupported object status") &&
         expect(!receipt.changed, "unsupported changed flag") &&
         expect(!receipt.allowed, "unsupported allowed") &&
         expect(receipt.revisionBefore == revisionBefore,
                "unsupported revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "unsupported revision after") &&
         expect(document.revision() == revisionBefore,
                "unsupported document revision unchanged") &&
         expect(receipt.objectReceipt.message ==
                    "object kind does not allow this mutation",
                "unsupported object message");
}

bool invalidMutationRequestRejectsBeforeApply() {
  cr::CreativeObjectId roomId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithRoom(roomId);
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt = cr::applyDocumentMutation(
      document, roomId, cr::CreativeMutationKind::Unknown,
      cr::CreativeMutationPayload{});

  return expect(receipt.status ==
                    cr::CreativeDocumentMutationStatus::InvalidRequest,
                "invalid status") &&
         expect(cr::documentMutationFailed(receipt.status),
                "invalid status failed") &&
         expect(!receipt.changed, "invalid changed flag") &&
         expect(!receipt.allowed, "invalid allowed") &&
         expect(receipt.revisionBefore == revisionBefore,
                "invalid revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "invalid revision after") &&
         expect(document.revision() == revisionBefore,
                "invalid document revision unchanged") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::Rejected,
                "invalid object status rejected") &&
         expect(receipt.message == "document mutation request is invalid",
                "invalid message");
}

}  // namespace

int main() {
  const bool ok = setVisibleMutatesThroughDocumentGateway() &&
                  settingAlreadyCurrentVisibilityIsNoChange() &&
                  setVisibleFalseThenTrueIncrementsForEachRealChange() &&
                  missingObjectRejectsWithoutRevisionAdvance() &&
                  unsupportedMutationDoesNotFalselyReportApplied() &&
                  invalidMutationRequestRejectsBeforeApply();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
