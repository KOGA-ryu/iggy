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

cr::CreativeDocument makeDocumentWithObjectKind(
    cr::CreativeObjectKind kind,
    cr::CreativeObjectId& objectId) {
  cr::CreativeDocument document = makeDocumentWithRoom(objectId);
  cr::CreativeObject* object = document.findObject(objectId);
  if (object != nullptr) {
    object->kind = kind;
    object->name = cr::toString(kind);
    object->transform = {};
    object->bounds = cr::CreativeBounds{cr::CreativeVec3{0.0, 0.0, 0.0},
                                        cr::CreativeVec3{2.0, 3.0, 4.0}};
    object->layerId = cr::kDefaultLayerId;
    object->visible = true;
    object->locked = false;
    object->tags.clear();
    object->parentId.reset();
  }
  return document;
}

cr::CreativeVec3 boundsSize(const cr::CreativeObject& object) {
  return cr::CreativeVec3{
      object.bounds.max.x - object.bounds.min.x,
      object.bounds.max.y - object.bounds.min.y,
      object.bounds.max.z - object.bounds.min.z,
  };
}

bool sameVec3(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
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

bool scalarAxisMutationChangesOnlyExpectedExtent(
    cr::CreativeMutationKind mutationKind,
    double value,
    cr::CreativeVec3 expectedSize,
    std::string_view label) {
  cr::CreativeObjectId roomId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithObjectKind(
      cr::CreativeObjectKind::Room, roomId);
  const cr::CreativeObject* before = document.findObject(roomId);
  const cr::CreativeVec3 minBefore = before != nullptr
                                         ? before->bounds.min
                                         : cr::CreativeVec3{};
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt = cr::applyDocumentMutation(
      document, roomId, mutationKind, cr::makeScalarPayload(value));
  const cr::CreativeObject* after = document.findObject(roomId);

  return expect(before != nullptr, "scalar setup room exists") &&
         expect(after != nullptr, "scalar room exists after") &&
         expect(receipt.status == cr::CreativeDocumentMutationStatus::Applied,
                "scalar status applied") &&
         expect(receipt.changed, "scalar changed") &&
         expect(receipt.allowed, "scalar allowed") &&
         expect(receipt.mutationKind == mutationKind,
                "scalar document mutation kind") &&
         expect(receipt.objectReceipt.mutationKind == mutationKind,
                "scalar object mutation kind") &&
         expect(receipt.revisionBefore == revisionBefore,
                "scalar revision before") &&
         expect(receipt.revisionAfter == revisionBefore + 1U,
                "scalar revision after") &&
         expect(document.revision() == revisionBefore + 1U,
                "scalar document revision advanced") &&
         expect(sameVec3(after->bounds.min, minBefore),
                "scalar preserves bounds min") &&
         expect(sameVec3(boundsSize(*after), expectedSize), label) &&
         expect(receipt.objectReceipt.message == "object resized",
                "scalar object message");
}

bool scalarDimensionAxisPolicyIsStable() {
  return scalarAxisMutationChangesOnlyExpectedExtent(
             cr::CreativeMutationKind::SetWidth, 5.0,
             cr::CreativeVec3{5.0, 3.0, 4.0}, "set width changes x only") &&
         scalarAxisMutationChangesOnlyExpectedExtent(
             cr::CreativeMutationKind::SetHeight, 6.0,
             cr::CreativeVec3{2.0, 6.0, 4.0}, "set height changes y only") &&
         scalarAxisMutationChangesOnlyExpectedExtent(
             cr::CreativeMutationKind::SetDepth, 7.0,
             cr::CreativeVec3{2.0, 3.0, 7.0}, "set depth changes z only") &&
         scalarAxisMutationChangesOnlyExpectedExtent(
             cr::CreativeMutationKind::SetLength, 8.0,
             cr::CreativeVec3{8.0, 3.0, 4.0},
             "set length changes x and not z");
}

bool alreadyCurrentScalarDimensionIsNoChange() {
  cr::CreativeObjectId roomId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithObjectKind(
      cr::CreativeObjectKind::Room, roomId);
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt = cr::applyDocumentMutation(
      document, roomId, cr::CreativeMutationKind::SetDepth,
      cr::makeScalarPayload(4.0));
  const cr::CreativeObject* room = document.findObject(roomId);

  return expect(room != nullptr, "current scalar room exists") &&
         expect(sameVec3(boundsSize(*room), cr::CreativeVec3{2.0, 3.0, 4.0}),
                "current scalar size unchanged") &&
         expect(receipt.status == cr::CreativeDocumentMutationStatus::NoChange,
                "current scalar status") &&
         expect(!receipt.changed, "current scalar changed false") &&
         expect(receipt.allowed, "current scalar allowed") &&
         expect(receipt.dirtyFlags == 0U, "current scalar dirty flags") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::SetDepth,
                "current scalar document kind") &&
         expect(receipt.objectReceipt.mutationKind ==
                    cr::CreativeMutationKind::SetDepth,
                "current scalar object kind") &&
         expect(receipt.revisionBefore == revisionBefore,
                "current scalar revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "current scalar revision after") &&
         expect(document.revision() == revisionBefore,
                "current scalar revision unchanged") &&
         expect(receipt.objectReceipt.message ==
                    "object bounds size already matches requested value",
                "current scalar object message");
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

bool descriptorAllowedTextSleeperVerbIsDocumentNoChange() {
  cr::CreativeObjectId noteId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithObjectKind(
      cr::CreativeObjectKind::Note, noteId);
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt = cr::applyDocumentMutation(
      document, noteId, cr::CreativeMutationKind::EditText,
      cr::makeTextPayload("hello"));

  return expect(receipt.status == cr::CreativeDocumentMutationStatus::NoChange,
                "text sleeper document no change") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Note,
                "text sleeper object kind") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::EditText,
                "text sleeper mutation kind") &&
         expect(receipt.objectReceipt.mutationKind ==
                    cr::CreativeMutationKind::EditText,
                "text sleeper object mutation kind") &&
         expect(!receipt.changed, "text sleeper changed false") &&
         expect(receipt.allowed, "text sleeper allowed") &&
         expect(receipt.dirtyFlags == 0U, "text sleeper dirty flags") &&
         expect(receipt.revisionBefore == revisionBefore,
                "text sleeper revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "text sleeper revision after") &&
         expect(document.revision() == revisionBefore,
                "text sleeper document revision unchanged") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::NoChange,
                "text sleeper object no change") &&
         expect(receipt.objectReceipt.message ==
                    "mutation has no stored object field yet",
                "text sleeper message");
}

bool descriptorAllowedScalarSleeperVerbIsDocumentNoChange() {
  cr::CreativeObjectId lightId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithObjectKind(
      cr::CreativeObjectKind::PointLight, lightId);
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt = cr::applyDocumentMutation(
      document, lightId, cr::CreativeMutationKind::SetLightIntensity,
      cr::makeScalarPayload(4.0));

  return expect(receipt.status == cr::CreativeDocumentMutationStatus::NoChange,
                "scalar sleeper document no change") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::PointLight,
                "scalar sleeper object kind") &&
         expect(receipt.mutationKind ==
                    cr::CreativeMutationKind::SetLightIntensity,
                "scalar sleeper mutation kind") &&
         expect(!receipt.changed, "scalar sleeper changed false") &&
         expect(receipt.allowed, "scalar sleeper allowed") &&
         expect(receipt.dirtyFlags == 0U, "scalar sleeper dirty flags") &&
         expect(receipt.revisionBefore == revisionBefore,
                "scalar sleeper revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "scalar sleeper revision after") &&
         expect(document.revision() == revisionBefore,
                "scalar sleeper document revision unchanged") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::NoChange,
                "scalar sleeper object no change") &&
         expect(receipt.objectReceipt.message ==
                    "mutation has no stored object field yet",
                "scalar sleeper message");
}

bool descriptorAllowedLinkSleeperVerbIsDocumentNoChange() {
  cr::CreativeObjectId linkId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithObjectKind(
      cr::CreativeObjectKind::NavLink, linkId);
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt = cr::applyDocumentMutation(
      document, linkId, cr::CreativeMutationKind::LinkTarget,
      cr::makeLinkPayload(77));

  return expect(receipt.status == cr::CreativeDocumentMutationStatus::NoChange,
                "link sleeper document no change") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::NavLink,
                "link sleeper object kind") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::LinkTarget,
                "link sleeper mutation kind") &&
         expect(!receipt.changed, "link sleeper changed false") &&
         expect(receipt.allowed, "link sleeper allowed") &&
         expect(receipt.dirtyFlags == 0U, "link sleeper dirty flags") &&
         expect(receipt.revisionBefore == revisionBefore,
                "link sleeper revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "link sleeper revision after") &&
         expect(document.revision() == revisionBefore,
                "link sleeper document revision unchanged") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::NoChange,
                "link sleeper object no change") &&
         expect(receipt.objectReceipt.message ==
                    "mutation has no stored object field yet",
                "link sleeper message");
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
                  scalarDimensionAxisPolicyIsStable() &&
                  alreadyCurrentScalarDimensionIsNoChange() &&
                  missingObjectRejectsWithoutRevisionAdvance() &&
                  descriptorAllowedTextSleeperVerbIsDocumentNoChange() &&
                  descriptorAllowedScalarSleeperVerbIsDocumentNoChange() &&
                  descriptorAllowedLinkSleeperVerbIsDocumentNoChange() &&
                  unsupportedMutationDoesNotFalselyReportApplied() &&
                  invalidMutationRequestRejectsBeforeApply();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
