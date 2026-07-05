#include "app/iggy3d/creative/Document.hpp"
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

bool sameVec3(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool defaultDocumentIsEmptyAndStable() {
  const cr::CreativeDocument document;

  return expect(document.isValid(), "default document valid") &&
         expect(document.objectCount() == 0U, "default object count") &&
         expect(document.revision() == 0U, "default revision");
}

bool genericRoomCreateCopiesDescriptorDefaults() {
  cr::CreativeDocument document;
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  const cr::CreativeObjectDescriptor& descriptor =
      cr::describeObject(cr::CreativeObjectKind::Room);

  const cr::CreativeDocumentCreateReceipt receipt =
      document.createObject(request);
  const cr::CreativeObject* object = document.findObject(receipt.objectId);

  return expect(receipt.requested, "room create requested") &&
         expect(receipt.accepted, "room create accepted") &&
         expect(receipt.changed, "room create changed") &&
         expect(receipt.objectCreated, "room object created") &&
         expect(receipt.status == cr::CreativeDocumentCreateStatus::Created,
                "room create status") &&
         expect(receipt.message == "object_created", "room create message") &&
         expect(receipt.reasonCode == "object_created",
                "room create reason") &&
         expect(receipt.objectId != cr::kInvalidObjectId,
                "room create id") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Room,
                "room create kind") &&
         expect(receipt.objectName == "Room", "room create default name") &&
         expect(receipt.revisionBefore == 0U, "room revision before") &&
         expect(receipt.revisionAfter == 1U, "room revision after") &&
         expect(receipt.creationDirtyFlags == descriptor.creationDirtyFlags,
                "room create dirty flags") &&
         expect(document.objectCount() == 1U, "room document count") &&
         expect(document.revision() == 1U, "room document revision") &&
         expect(document.containsObject(receipt.objectId),
                "room index contains object") &&
         expect(object != nullptr, "room object findable") &&
         expect(object->id == receipt.objectId, "room object id") &&
         expect(object->kind == cr::CreativeObjectKind::Room,
                "room object kind") &&
         expect(object->name == "Room", "room object default name") &&
         expect(sameVec3(object->transform.position,
                         descriptor.defaults.transform.position),
                "room default transform position") &&
         expect(sameVec3(object->transform.rotation,
                         descriptor.defaults.transform.rotation),
                "room default transform rotation") &&
         expect(sameVec3(object->transform.scale,
                         descriptor.defaults.transform.scale),
                "room default transform scale") &&
         expect(sameVec3(object->bounds.min, {0.0, 0.0, 0.0}),
                "room default bounds min") &&
         expect(sameVec3(object->bounds.max, {10.0, 4.0, 10.0}),
                "room default bounds max") &&
         expect(object->layerId == descriptor.defaults.layerId,
                "room default layer") &&
         expect(object->visible, "room default visible") &&
         expect(!object->locked, "room default unlocked") &&
         expect(object->tags.empty(), "room default tags") &&
         expect(!object->parentId.has_value(), "room default no parent");
}

bool secondGenericCreateGetsNewIdAndRevision() {
  cr::CreativeDocument document;
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;

  const cr::CreativeDocumentCreateReceipt first =
      document.createObject(request);
  const cr::CreativeDocumentCreateReceipt second =
      document.createObject(request);

  return expect(first.accepted, "first accepted") &&
         expect(second.accepted, "second accepted") &&
         expect(first.objectId != second.objectId, "ids differ") &&
         expect(second.objectId > first.objectId, "ids increase") &&
         expect(document.objectCount() == 2U, "two objects") &&
         expect(first.revisionAfter == 1U, "first revision") &&
         expect(second.revisionBefore == 1U, "second revision before") &&
         expect(second.revisionAfter == 2U, "second revision after") &&
         expect(document.revision() == 2U, "document revision two");
}

bool unknownKindRejectsWithoutRevisionChange() {
  cr::CreativeDocument document;
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Unknown;

  const cr::CreativeDocumentCreateReceipt receipt =
      document.createObject(request);

  return expect(receipt.requested, "unknown requested") &&
         expect(!receipt.accepted, "unknown not accepted") &&
         expect(!receipt.changed, "unknown unchanged") &&
         expect(!receipt.objectCreated, "unknown no object") &&
         expect(receipt.status == cr::CreativeDocumentCreateStatus::InvalidKind,
                "unknown status") &&
         expect(receipt.objectId == cr::kInvalidObjectId,
                "unknown invalid id") &&
         expect(receipt.revisionBefore == 0U, "unknown revision before") &&
         expect(receipt.revisionAfter == 0U, "unknown revision after") &&
         expect(receipt.reasonCode == "invalid_kind",
                "unknown reason") &&
         expect(document.objectCount() == 0U, "unknown document empty") &&
         expect(document.revision() == 0U, "unknown revision stable");
}

bool nameAndBoundsOverridesRespectDescriptorPolicy() {
  cr::CreativeDocument document;
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  request.name = "Atrium";
  request.bounds = {{-1.0, 0.0, -2.0}, {6.0, 5.0, 7.0}};
  request.hasBoundsOverride = true;
  request.visible = false;
  request.hasVisibleOverride = true;
  request.locked = true;
  request.hasLockedOverride = true;
  request.layerId = 9;
  request.hasLayerOverride = true;
  request.tags = {"blockout", "entry"};

  const cr::CreativeDocumentCreateReceipt receipt =
      document.createObject(request);
  const cr::CreativeObject* object = document.findObject(receipt.objectId);

  return expect(receipt.accepted, "override accepted") &&
         expect(object != nullptr, "override object") &&
         expect(object->name == "Atrium", "override name") &&
         expect(sameVec3(object->bounds.min, {-1.0, 0.0, -2.0}),
                "override bounds min") &&
         expect(sameVec3(object->bounds.max, {6.0, 5.0, 7.0}),
                "override bounds max") &&
         expect(!object->visible, "override visible") &&
         expect(object->locked, "override locked") &&
         expect(object->layerId == 9U, "override layer") &&
         expect(object->tags.size() == 2U, "override tags") &&
         expect(object->tags[0] == "blockout", "override first tag") &&
         expect(object->tags[1] == "entry", "override second tag");
}

bool unsupportedOverridesRejectWithoutMutation() {
  cr::CreativeDocument document;
  cr::CreativeDocumentCreateRequest parentRequest;
  parentRequest.kind = cr::CreativeObjectKind::Room;
  const cr::CreativeDocumentCreateReceipt parent =
      document.createObject(parentRequest);

  cr::CreativeDocumentCreateRequest childRequest;
  childRequest.kind = cr::CreativeObjectKind::Room;
  childRequest.parentId = parent.objectId;
  const std::uint64_t revisionBeforeParentReject = document.revision();
  const cr::CreativeDocumentCreateReceipt parentReject =
      document.createObject(childRequest);

  cr::CreativeDocumentCreateRequest transformRequest;
  transformRequest.kind = cr::CreativeObjectKind::Room;
  transformRequest.transform.position = {1.0, 2.0, 3.0};
  transformRequest.hasTransformOverride = true;
  const std::uint64_t revisionBeforeTransformReject = document.revision();
  const cr::CreativeDocumentCreateReceipt transformReject =
      document.createObject(transformRequest);

  return expect(parent.accepted, "parent setup accepted") &&
         expect(!parentReject.accepted, "parent reject not accepted") &&
         expect(parentReject.status == cr::CreativeDocumentCreateStatus::Rejected,
                "parent reject status") &&
         expect(parentReject.reasonCode == "parent_unsupported",
                "parent reject reason") &&
         expect(parentReject.revisionBefore == revisionBeforeParentReject,
                "parent reject revision before") &&
         expect(parentReject.revisionAfter == revisionBeforeParentReject,
                "parent reject revision after") &&
         expect(document.objectCount() == 1U,
                "parent reject object count stable") &&
         expect(!transformReject.accepted,
                "transform reject not accepted") &&
         expect(transformReject.reasonCode ==
                    "transform_override_unsupported",
                "transform reject reason") &&
         expect(transformReject.revisionBefore ==
                    revisionBeforeTransformReject,
                "transform reject revision before") &&
         expect(transformReject.revisionAfter ==
                    revisionBeforeTransformReject,
                "transform reject revision after") &&
         expect(document.objectCount() == 1U,
                "transform reject object count stable");
}

bool receiptedCreateWithOverridesSharesAllocator() {
  cr::CreativeDocument document;
  cr::CreativeDocumentCreateRequest genericRequest;
  genericRequest.kind = cr::CreativeObjectKind::Room;
  const cr::CreativeDocumentCreateReceipt generic =
      document.createObject(genericRequest);

  cr::CreativeDocumentCreateRequest overrideRequest;
  overrideRequest.kind = cr::CreativeObjectKind::Room;
  overrideRequest.name = "Override Room";
  overrideRequest.bounds = cr::CreativeBounds{{-5.0, 0.0, -6.0},
                                              {5.0, 4.0, 6.0}};
  overrideRequest.hasBoundsOverride = true;
  overrideRequest.layerId = 7;
  overrideRequest.hasLayerOverride = true;
  overrideRequest.visible = false;
  overrideRequest.hasVisibleOverride = true;
  overrideRequest.locked = true;
  overrideRequest.hasLockedOverride = true;
  overrideRequest.tags = {"override"};
  const cr::CreativeDocumentCreateReceipt overridden =
      document.createObject(overrideRequest);
  const cr::CreativeObject* object = document.findObject(overridden.objectId);

  return expect(generic.accepted, "generic setup accepted") &&
         expect(overridden.accepted, "override create accepted") &&
         expect(overridden.objectId != cr::kInvalidObjectId, "override id") &&
         expect(overridden.objectId > generic.objectId,
                "override shares allocator") &&
         expect(document.revision() == 2U, "override revision") &&
         expect(document.objectCount() == 2U, "override object count") &&
         expect(object != nullptr, "override findable") &&
         expect(object->name == "Override Room", "override name") &&
         expect(sameVec3(object->bounds.max, {5.0, 4.0, 6.0}),
                "override bounds") &&
         expect(object->layerId == 7U, "override layer") &&
         expect(!object->visible, "override visible") &&
         expect(object->locked, "override locked") &&
         expect(object->tags.size() == 1U, "override tags") &&
         expect(!object->parentId.has_value(), "override no parent");
}

bool facadeGenericCreateWrapsDocumentAndPreservesInteractionState() {
  cr::Facade facade;
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  request.name = "Facade Generic Room";

  const cr::CreativeDocumentCreateReceipt receipt =
      facade.createDocumentObject(request);
  const cr::CreativeObject* object = facade.document().findObject(receipt.objectId);

  return expect(receipt.accepted, "facade create accepted") &&
         expect(receipt.objectCreated, "facade object created") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Room,
                "facade create kind") &&
         expect(receipt.objectName == "Facade Generic Room",
                "facade receipt name") &&
         expect(object != nullptr, "facade object findable") &&
         expect(object->name == "Facade Generic Room", "facade object name") &&
         expect(facade.document().objectCount() == 1U,
                "facade document count") &&
         expect(facade.document().revision() == 1U,
                "facade document revision") &&
         expect(facade.stats().commandAttempts == 1U,
                "facade attempts") &&
         expect(facade.stats().commandSuccesses == 1U,
                "facade successes") &&
         expect(facade.stats().commandFailures == 0U,
                "facade failures") &&
         expect(facade.stats().objectsCreated == 1U,
                "facade objects created") &&
         expect(facade.stats().roomsCreated == 1U,
                "facade rooms created") &&
         expect(facade.toolState().activeTool == cr::Tool::Move,
                "facade tool preserved") &&
         expect(facade.state().tool == cr::Tool::Move,
                "facade old state tool preserved") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "facade selection preserved");
}

bool facadeGenericCreateFailureRecordsFailureOnly() {
  cr::Facade facade;
  const cr::CreativeDocumentCreateReceipt receipt =
      facade.createDocumentObject(cr::CreativeObjectKind::Unknown);

  return expect(!receipt.accepted, "facade invalid not accepted") &&
         expect(receipt.status == cr::CreativeDocumentCreateStatus::InvalidKind,
                "facade invalid status") &&
         expect(facade.document().objectCount() == 0U,
                "facade invalid object count") &&
         expect(facade.document().revision() == 0U,
                "facade invalid revision") &&
         expect(facade.stats().commandAttempts == 1U,
                "facade invalid attempts") &&
         expect(facade.stats().commandSuccesses == 0U,
                "facade invalid successes") &&
         expect(facade.stats().commandFailures == 1U,
                "facade invalid failures") &&
         expect(facade.stats().objectsCreated == 0U,
                "facade invalid objects created") &&
         expect(facade.stats().roomsCreated == 0U,
                "facade invalid rooms created");
}

}  // namespace

int main() {
  const bool ok = defaultDocumentIsEmptyAndStable() &&
                  genericRoomCreateCopiesDescriptorDefaults() &&
                  secondGenericCreateGetsNewIdAndRevision() &&
                  unknownKindRejectsWithoutRevisionChange() &&
                  nameAndBoundsOverridesRespectDescriptorPolicy() &&
                  unsupportedOverridesRejectWithoutMutation() &&
                  receiptedCreateWithOverridesSharesAllocator() &&
                  facadeGenericCreateWrapsDocumentAndPreservesInteractionState() &&
                  facadeGenericCreateFailureRecordsFailureOnly();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
