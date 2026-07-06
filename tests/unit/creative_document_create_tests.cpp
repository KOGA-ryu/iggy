#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/Facade.hpp"

#include <array>
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

bool authoredStateOverridesAreUniversalForRepresentativeDescriptors() {
  cr::CreativeDocument document;

  cr::CreativeDocumentCreateRequest lightRequest;
  lightRequest.kind = cr::CreativeObjectKind::PointLight;
  lightRequest.visible = false;
  lightRequest.hasVisibleOverride = true;
  lightRequest.locked = true;
  lightRequest.hasLockedOverride = true;
  lightRequest.tags = {"lighting", "hidden"};
  const cr::CreativeDocumentCreateReceipt light =
      document.createObject(lightRequest);

  cr::CreativeDocumentCreateRequest noteRequest;
  noteRequest.kind = cr::CreativeObjectKind::Note;
  noteRequest.visible = false;
  noteRequest.hasVisibleOverride = true;
  noteRequest.locked = true;
  noteRequest.hasLockedOverride = true;
  noteRequest.tags = {"authoring"};
  const cr::CreativeDocumentCreateReceipt note =
      document.createObject(noteRequest);

  cr::CreativeDocumentCreateRequest routeRequest;
  routeRequest.kind = cr::CreativeObjectKind::PatrolRoute;
  routeRequest.hasPathOverride = true;
  routeRequest.pathPoints = {
      cr::CreativePathPoint{{0.0, 0.0, 0.0}},
      cr::CreativePathPoint{{1.0, 0.0, 1.0}},
  };
  routeRequest.visible = false;
  routeRequest.hasVisibleOverride = true;
  routeRequest.locked = true;
  routeRequest.hasLockedOverride = true;
  routeRequest.tags = {"route"};
  const cr::CreativeDocumentCreateReceipt route =
      document.createObject(routeRequest);

  const cr::CreativeObject* lightObject = document.findObject(light.objectId);
  const cr::CreativeObject* noteObject = document.findObject(note.objectId);
  const cr::CreativeObject* routeObject = document.findObject(route.objectId);

  return expect(light.accepted, "point light state override accepted") &&
         expect(lightObject != nullptr, "point light override findable") &&
         expect(lightObject != nullptr && !lightObject->visible,
                "point light override hidden") &&
         expect(lightObject != nullptr && lightObject->locked,
                "point light override locked") &&
         expect(lightObject != nullptr && lightObject->tags.size() == 2U,
                "point light override tags") &&
         expect(note.accepted, "note state override accepted") &&
         expect(noteObject != nullptr, "note override findable") &&
         expect(noteObject != nullptr && !noteObject->visible,
                "note override hidden") &&
         expect(noteObject != nullptr && noteObject->locked,
                "note override locked") &&
         expect(noteObject != nullptr && noteObject->tags.size() == 1U,
                "note override tags") &&
         expect(route.accepted, "patrol route state override accepted") &&
         expect(routeObject != nullptr, "patrol route override findable") &&
         expect(routeObject != nullptr && !routeObject->visible,
                "patrol route override hidden") &&
         expect(routeObject != nullptr && routeObject->locked,
                "patrol route override locked") &&
         expect(routeObject != nullptr && routeObject->tags.size() == 1U,
                "patrol route override tags");
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

bool parentOwnerUnsupportedRejectsWithoutMutation() {
  cr::CreativeDocument document;

  cr::CreativeDocumentCreateRequest crateRequest;
  crateRequest.kind = cr::CreativeObjectKind::Crate;
  const cr::CreativeDocumentCreateReceipt crate =
      document.createObject(crateRequest);

  cr::CreativeDocumentCreateRequest wallRequest;
  wallRequest.kind = cr::CreativeObjectKind::Wall;
  wallRequest.parentId = crate.objectId;
  const std::uint64_t revisionBeforeReject = document.revision();
  const cr::CreativeDocumentCreateReceipt rejected =
      document.createObject(wallRequest);

  return expect(crate.accepted, "parent owner setup crate accepted") &&
         expect(!rejected.accepted, "parent owner reject not accepted") &&
         expect(rejected.status == cr::CreativeDocumentCreateStatus::Rejected,
                "parent owner reject status") &&
         expect(rejected.reasonCode == "parent_owner_unsupported",
                "parent owner reject reason") &&
         expect(rejected.revisionBefore == revisionBeforeReject,
                "parent owner reject revision before") &&
         expect(rejected.revisionAfter == revisionBeforeReject,
                "parent owner reject revision after") &&
         expect(document.objectCount() == 1U,
                "parent owner reject object count stable") &&
         expect(document.findObject(crate.objectId) != nullptr,
                "parent owner reject parent remains");
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

bool facadeBatchCreateAppliesAllRequestsAtomically() {
  cr::Facade facade;
  static_cast<void>(facade.documentForPersistence().assignId(7U));

  std::array<cr::CreativeDocumentCreateRequest, 2> requests{};
  requests[0].kind = cr::CreativeObjectKind::Room;
  requests[0].name = "Batch Room";
  requests[1].kind = cr::CreativeObjectKind::Crate;
  requests[1].name = "Batch Crate";

  const cr::CreativeFacadeDocumentBatchCreateReceipt receipt =
      facade.createDocumentObjectsAtomically(requests);

  return expect(receipt.requested, "batch requested") &&
         expect(receipt.accepted, "batch accepted") &&
         expect(receipt.changed, "batch changed") &&
         expect(receipt.status ==
                    cr::CreativeFacadeDocumentBatchCreateStatus::Applied,
                "batch applied status") &&
         expect(receipt.reasonCode == "creative_facade_batch_create_applied",
                "batch applied reason") &&
         expect(receipt.revisionBefore == 0U, "batch revision before") &&
         expect(receipt.revisionAfter == 2U, "batch revision after") &&
         expect(receipt.attemptedCreateCount == 2U,
                "batch attempted count") &&
         expect(receipt.appliedCreateCount == 2U,
                "batch applied count") &&
         expect(!receipt.hasFailedCreate, "batch no failed create") &&
         expect(receipt.installAttempted, "batch install attempted") &&
         expect(receipt.installReceipt.accepted, "batch install accepted") &&
         expect(receipt.installReceipt.changed, "batch install changed") &&
         expect(facade.document().objectCount() == 2U,
                "batch object count") &&
         expect(facade.document().revision() == 2U,
                "batch document revision") &&
         expect(facade.document().findObject(1U) != nullptr,
                "batch first object installed") &&
         expect(facade.document().findObject(2U) != nullptr,
                "batch second object installed");
}

bool facadeBatchCreateRejectionPreservesLiveDocument() {
  cr::Facade facade;
  static_cast<void>(facade.documentForPersistence().assignId(8U));

  std::array<cr::CreativeDocumentCreateRequest, 2> requests{};
  requests[0].kind = cr::CreativeObjectKind::Room;
  requests[0].name = "Staged Only Room";
  requests[1].kind = cr::CreativeObjectKind::Wall;
  requests[1].parentId = 999U;

  const cr::CreativeFacadeDocumentBatchCreateReceipt receipt =
      facade.createDocumentObjectsAtomically(requests);

  return expect(receipt.requested, "batch reject requested") &&
         expect(!receipt.accepted, "batch reject not accepted") &&
         expect(!receipt.changed, "batch reject unchanged") &&
         expect(receipt.status ==
                    cr::CreativeFacadeDocumentBatchCreateStatus::
                        CreateRejected,
                "batch create rejected status") &&
         expect(receipt.reasonCode ==
                    "creative_facade_batch_create_create_rejected",
                "batch create rejected reason") &&
         expect(receipt.revisionBefore == 0U,
                "batch reject revision before") &&
         expect(receipt.revisionAfter == 0U,
                "batch reject revision after") &&
         expect(receipt.attemptedCreateCount == 2U,
                "batch reject attempted count") &&
         expect(receipt.appliedCreateCount == 1U,
                "batch reject staged applied count") &&
         expect(receipt.hasFailedCreate, "batch failed create") &&
         expect(receipt.firstFailedCreateIndex == 1U,
                "batch failed create index") &&
         expect(receipt.firstFailedCreateStatus ==
                    cr::CreativeDocumentCreateStatus::Rejected,
                "batch failed create status") &&
         expect(receipt.firstFailedCreateReasonCode == "missing_parent",
                "batch failed create reason") &&
         expect(!receipt.installAttempted, "batch reject no install") &&
         expect(facade.document().objectCount() == 0U,
                "batch reject live object count") &&
         expect(facade.document().revision() == 0U,
                "batch reject live revision");
}

bool facadeBatchCreateInstallFailurePreservesLiveDocument() {
  cr::Facade facade;

  std::array<cr::CreativeDocumentCreateRequest, 1> requests{};
  requests[0].kind = cr::CreativeObjectKind::Room;
  requests[0].name = "Missing Id Room";

  const cr::CreativeFacadeDocumentBatchCreateReceipt receipt =
      facade.createDocumentObjectsAtomically(requests);

  return expect(receipt.requested, "batch install fail requested") &&
         expect(!receipt.accepted, "batch install fail not accepted") &&
         expect(!receipt.changed, "batch install fail unchanged") &&
         expect(receipt.status ==
                    cr::CreativeFacadeDocumentBatchCreateStatus::
                        InstallRejected,
                "batch install rejected status") &&
         expect(receipt.reasonCode ==
                    "creative_facade_batch_create_install_rejected",
                "batch install rejected reason") &&
         expect(receipt.revisionBefore == 0U,
                "batch install fail revision before") &&
         expect(receipt.revisionAfter == 0U,
                "batch install fail revision after") &&
         expect(receipt.attemptedCreateCount == 1U,
                "batch install fail attempted count") &&
         expect(receipt.appliedCreateCount == 1U,
                "batch install fail applied count") &&
         expect(!receipt.hasFailedCreate,
                "batch install fail no create failure") &&
         expect(receipt.installAttempted,
                "batch install fail attempted install") &&
         expect(!receipt.installReceipt.accepted,
                "batch install fail rejected install") &&
         expect(receipt.installReceipt.reasonCode ==
                    "creative_facade_document_id_missing",
                "batch install fail reason") &&
         expect(facade.document().objectCount() == 0U,
                "batch install fail live object count") &&
         expect(facade.document().revision() == 0U,
                "batch install fail live revision");
}

}  // namespace

int main() {
  const bool ok = defaultDocumentIsEmptyAndStable() &&
                  genericRoomCreateCopiesDescriptorDefaults() &&
                  secondGenericCreateGetsNewIdAndRevision() &&
                  unknownKindRejectsWithoutRevisionChange() &&
                  nameAndBoundsOverridesRespectDescriptorPolicy() &&
                  authoredStateOverridesAreUniversalForRepresentativeDescriptors() &&
                  unsupportedOverridesRejectWithoutMutation() &&
                  parentOwnerUnsupportedRejectsWithoutMutation() &&
                  receiptedCreateWithOverridesSharesAllocator() &&
                  facadeGenericCreateWrapsDocumentAndPreservesInteractionState() &&
                  facadeGenericCreateFailureRecordsFailureOnly() &&
                  facadeBatchCreateAppliesAllRequestsAtomically() &&
                  facadeBatchCreateRejectionPreservesLiveDocument() &&
                  facadeBatchCreateInstallFailurePreservesLiveDocument();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
