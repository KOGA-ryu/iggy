#include "app/iggy3d/creative/document/DocumentMutation.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
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
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  request.name = "Room";
  request.bounds = cr::CreativeBounds{cr::CreativeVec3{0.0, 0.0, 0.0},
                                      cr::CreativeVec3{1.0, 1.0, 1.0}};
  request.hasBoundsOverride = true;
  roomId = document.createObject(request).objectId;
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

cr::CreativeDocument makeDocumentWithDefaultRoom(cr::CreativeObjectId& roomId) {
  cr::CreativeDocument document = cr::CreativeDocument::create("Document");
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  request.name = "Room";
  roomId = document.createObject(request).objectId;
  return document;
}

cr::CreativeDocument makeDocumentWithCrate(cr::CreativeObjectId& crateId) {
  cr::CreativeDocument document = cr::CreativeDocument::create("Document");
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.name = "Crate";
  request.transform = cr::CreativeTransform{};
  request.hasTransformOverride = true;
  request.bounds = cr::CreativeBounds{cr::CreativeVec3{0.0, 0.0, 0.0},
                                     cr::CreativeVec3{1.0, 1.0, 1.0}};
  request.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt receipt =
      document.createObject(request);
  crateId = receipt.objectId;
  return document;
}

cr::CreativeDocumentCreateReceipt createObject(
    cr::CreativeDocument& document,
    cr::CreativeObjectKind kind,
    std::string_view name,
    std::optional<cr::CreativeObjectId> parentId = std::nullopt) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::string{name};
  request.parentId = parentId;
  return document.createObject(request);
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

bool sameBounds(const cr::CreativeBounds& lhs, const cr::CreativeBounds& rhs) {
  return sameVec3(lhs.min, rhs.min) && sameVec3(lhs.max, rhs.max);
}

constexpr std::array kAuthoredMutationKinds{
    cr::CreativeMutationKind::Rename,
    cr::CreativeMutationKind::SetVisible,
    cr::CreativeMutationKind::SetLocked,
    cr::CreativeMutationKind::Move,
    cr::CreativeMutationKind::Rotate,
    cr::CreativeMutationKind::Scale,
    cr::CreativeMutationKind::SetTransform,
    cr::CreativeMutationKind::Resize,
    cr::CreativeMutationKind::Stretch,
    cr::CreativeMutationKind::SetBounds,
    cr::CreativeMutationKind::SetHeight,
    cr::CreativeMutationKind::SetRadius,
    cr::CreativeMutationKind::SetThickness,
    cr::CreativeMutationKind::SetLength,
    cr::CreativeMutationKind::SetWidth,
    cr::CreativeMutationKind::SetDepth,
    cr::CreativeMutationKind::SetParent,
    cr::CreativeMutationKind::ClearParent,
    cr::CreativeMutationKind::AttachTo,
    cr::CreativeMutationKind::DetachFrom,
    cr::CreativeMutationKind::LinkTarget,
    cr::CreativeMutationKind::UnlinkTarget,
    cr::CreativeMutationKind::SetSocket,
    cr::CreativeMutationKind::ClearSocket,
    cr::CreativeMutationKind::AssignLayer,
    cr::CreativeMutationKind::AddTag,
    cr::CreativeMutationKind::RemoveTag,
    cr::CreativeMutationKind::ClearTags,
    cr::CreativeMutationKind::EditText,
    cr::CreativeMutationKind::SetLabel,
    cr::CreativeMutationKind::SetNotes,
    cr::CreativeMutationKind::SetAsset,
    cr::CreativeMutationKind::SetReferenceSource,
    cr::CreativeMutationKind::SetBlueprintOpacity,
    cr::CreativeMutationKind::SetTriggerShape,
    cr::CreativeMutationKind::SetTriggerEvent,
    cr::CreativeMutationKind::SetCondition,
    cr::CreativeMutationKind::SetEventRelayTarget,
    cr::CreativeMutationKind::SetSpawnerProfile,
    cr::CreativeMutationKind::SetDespawnRule,
    cr::CreativeMutationKind::SetSpawnFacing,
    cr::CreativeMutationKind::SetCheckpointId,
    cr::CreativeMutationKind::SetNavCost,
    cr::CreativeMutationKind::SetPatrolRoute,
    cr::CreativeMutationKind::SetJumpArc,
    cr::CreativeMutationKind::SetClimbRule,
    cr::CreativeMutationKind::SetWallRunRule,
    cr::CreativeMutationKind::SetSlideRule,
    cr::CreativeMutationKind::SetTestLaneKind,
    cr::CreativeMutationKind::SetDistanceValue,
    cr::CreativeMutationKind::SetSpeedValue,
    cr::CreativeMutationKind::SetTimingWindow,
    cr::CreativeMutationKind::SetProbeKind,
    cr::CreativeMutationKind::SetExpectedResult,
    cr::CreativeMutationKind::SetLightColor,
    cr::CreativeMutationKind::SetLightIntensity,
    cr::CreativeMutationKind::SetLightRadius,
    cr::CreativeMutationKind::SetLightConeAngle,
    cr::CreativeMutationKind::SetAudioRadius,
    cr::CreativeMutationKind::SetAudioSource,
    cr::CreativeMutationKind::SetMusicCue,
    cr::CreativeMutationKind::SetCameraTarget,
    cr::CreativeMutationKind::SetCameraRail,
    cr::CreativeMutationKind::SetEnemyProfile,
    cr::CreativeMutationKind::SetNpcProfile,
    cr::CreativeMutationKind::SetResourceKind,
    cr::CreativeMutationKind::SetLootTable,
    cr::CreativeMutationKind::SetQuestId,
    cr::CreativeMutationKind::SetDialogueId,
    cr::CreativeMutationKind::SetDangerLevel,
    cr::CreativeMutationKind::SetSafeZoneRule,
    cr::CreativeMutationKind::SetLootPointSettings,
    cr::CreativeMutationKind::SetExitPointSettings,
};

bool categoryPredicateMatches(cr::CreativeMutationKind kind,
                              cr::CreativeMutationCategory category) {
  switch (category) {
  case cr::CreativeMutationCategory::Identity:
    return cr::isIdentityMutation(kind);
  case cr::CreativeMutationCategory::Transform:
    return cr::isTransformMutation(kind);
  case cr::CreativeMutationCategory::Shape:
    return cr::isShapeMutation(kind);
  case cr::CreativeMutationCategory::Relationship:
    return cr::isRelationshipMutation(kind);
  case cr::CreativeMutationCategory::Organization:
    return cr::isOrganizationMutation(kind);
  case cr::CreativeMutationCategory::Content:
    return cr::isContentMutation(kind);
  case cr::CreativeMutationCategory::Logic:
    return cr::isLogicMutation(kind);
  case cr::CreativeMutationCategory::Navigation:
    return cr::isNavigationMutation(kind);
  case cr::CreativeMutationCategory::Testing:
    return cr::isTestingMutation(kind);
  case cr::CreativeMutationCategory::Sensory:
    return cr::isSensoryMutation(kind);
  case cr::CreativeMutationCategory::Gameplay:
    return cr::isGameplayMutation(kind);
  case cr::CreativeMutationCategory::Unknown:
    return false;
  }

  return false;
}

int categoryPredicateCount(cr::CreativeMutationKind kind) {
  int count = 0;
  count += cr::isIdentityMutation(kind) ? 1 : 0;
  count += cr::isTransformMutation(kind) ? 1 : 0;
  count += cr::isShapeMutation(kind) ? 1 : 0;
  count += cr::isRelationshipMutation(kind) ? 1 : 0;
  count += cr::isOrganizationMutation(kind) ? 1 : 0;
  count += cr::isContentMutation(kind) ? 1 : 0;
  count += cr::isLogicMutation(kind) ? 1 : 0;
  count += cr::isNavigationMutation(kind) ? 1 : 0;
  count += cr::isTestingMutation(kind) ? 1 : 0;
  count += cr::isSensoryMutation(kind) ? 1 : 0;
  count += cr::isGameplayMutation(kind) ? 1 : 0;
  return count;
}

bool mutationMetadataRegistryIsInternallyConsistent() {
  bool ok = expect(cr::toString(cr::CreativeMutationKind::Unknown) ==
                       std::string_view{"Unknown"},
                   "unknown mutation name") &&
            expect(cr::categoryOf(cr::CreativeMutationKind::Unknown) ==
                       cr::CreativeMutationCategory::Unknown,
                   "unknown mutation category") &&
            expect(!cr::requiresPayload(cr::CreativeMutationKind::Unknown),
                   "unknown mutation no payload required") &&
            expect(cr::payloadMatchesMutation(cr::CreativeMutationKind::Unknown,
                                             cr::CreativeMutationPayload{}),
                   "unknown mutation matches empty payload");

  for (const cr::CreativeMutationKind kind : kAuthoredMutationKinds) {
    const cr::CreativeMutationDescriptor descriptor = cr::describeMutation(kind);
    ok = expect(cr::toString(kind) != std::string_view{"Unknown"},
                "authored mutation has non-unknown name") &&
         expect(cr::categoryOf(kind) != cr::CreativeMutationCategory::Unknown,
                "authored mutation has non-unknown category") &&
         expect(descriptor.kind == kind, "descriptor kind mirrors request") &&
         expect(descriptor.name == cr::toString(kind),
                "descriptor name mirrors registry") &&
         expect(descriptor.category == cr::categoryOf(kind),
                "descriptor category mirrors registry") &&
         expect(descriptor.storagePolicy == cr::mutationStoragePolicy(kind),
                "descriptor storage policy mirrors registry") &&
         expect(descriptor.changesGeometry == cr::mutationChangesGeometry(kind),
                "descriptor geometry flag mirrors registry") &&
         expect(descriptor.changesRelationships ==
                    cr::mutationChangesRelationships(kind),
                "descriptor relationship flag mirrors registry") &&
         expect(descriptor.changesRuntimeMeaning ==
                    cr::mutationChangesRuntimeMeaning(kind),
                "descriptor runtime flag mirrors registry") &&
         expect(categoryPredicateCount(kind) == 1,
                "exactly one category predicate is true") &&
         expect(categoryPredicateMatches(kind, descriptor.category),
                "matching category predicate is true") && ok;
  }

  return ok;
}

bool mutationStoragePolicySignalDistinguishesStoredAndFuturePlaceholders() {
  const cr::CreativeMutationPayload boundsPayload = cr::makeBoundsPayload(
      cr::CreativeBounds{cr::CreativeVec3{0.0, 0.0, 0.0},
                         cr::CreativeVec3{1.0, 1.0, 1.0}});
  const cr::CreativeMutationPayload pathPayload = cr::makePathPointsPayload(
      {cr::CreativePathPoint{{0.0, 0.0, 0.0}},
       cr::CreativePathPoint{{1.0, 0.0, 1.0}}});

  return expect(cr::toString(cr::CreativeMutationStoragePolicy::Unknown) ==
                    std::string_view{"Unknown"},
                "storage policy unknown string") &&
         expect(cr::toString(cr::CreativeMutationStoragePolicy::StoredObject) ==
                    std::string_view{"StoredObject"},
                "storage policy stored string") &&
         expect(cr::toString(cr::CreativeMutationStoragePolicy::
                                 FutureStoragePlaceholder) ==
                    std::string_view{"FutureStoragePlaceholder"},
                "storage policy future string") &&
         expect(cr::toString(cr::CreativeMutationStoragePolicy::
                                 PayloadDependent) ==
                    std::string_view{"PayloadDependent"},
                "storage policy payload dependent string") &&
         expect(cr::mutationStoragePolicy(cr::CreativeMutationKind::Rename) ==
                    cr::CreativeMutationStoragePolicy::StoredObject,
                "rename storage policy") &&
         expect(cr::mutationStoragePolicy(
                    cr::CreativeMutationKind::SetTriggerShape) ==
                    cr::CreativeMutationStoragePolicy::StoredObject,
                "trigger shape storage policy") &&
         expect(cr::mutationStoragePolicy(
                    cr::CreativeMutationKind::SetLightIntensity) ==
                    cr::CreativeMutationStoragePolicy::FutureStoragePlaceholder,
                "light intensity future policy") &&
         expect(cr::mutationStoragePolicy(
                    cr::CreativeMutationKind::LinkTarget) ==
                    cr::CreativeMutationStoragePolicy::FutureStoragePlaceholder,
                "link target future policy") &&
         expect(cr::mutationStoragePolicy(
                    cr::CreativeMutationKind::ClearSocket) ==
                    cr::CreativeMutationStoragePolicy::FutureStoragePlaceholder,
                "clear socket future policy") &&
         expect(cr::mutationStoragePolicy(
                    cr::CreativeMutationKind::SetPatrolRoute) ==
                    cr::CreativeMutationStoragePolicy::PayloadDependent,
                "patrol route payload-dependent policy") &&
         expect(cr::mutationPayloadStoragePolicy(
                    cr::CreativeMutationKind::SetTriggerShape, boundsPayload) ==
                    cr::CreativeMutationStoragePolicy::StoredObject,
                "trigger shape payload stored policy") &&
         expect(cr::mutationPayloadStoragePolicy(
                    cr::CreativeMutationKind::SetLightIntensity,
                    cr::makeScalarPayload(4.0)) ==
                    cr::CreativeMutationStoragePolicy::FutureStoragePlaceholder,
                "light intensity payload future policy") &&
         expect(cr::mutationPayloadStoragePolicy(
                    cr::CreativeMutationKind::SetPatrolRoute, pathPayload) ==
                    cr::CreativeMutationStoragePolicy::StoredObject,
                "patrol route path payload stored policy") &&
         expect(cr::mutationPayloadStoragePolicy(
                    cr::CreativeMutationKind::SetPatrolRoute,
                    cr::makeTextPayload("legacy")) ==
                    cr::CreativeMutationStoragePolicy::FutureStoragePlaceholder,
                "patrol route text payload future policy") &&
         expect(cr::mutationPayloadStoragePolicy(
                    cr::CreativeMutationKind::SetPatrolRoute,
                    cr::makeStringIdPayload("legacy-id")) ==
                    cr::CreativeMutationStoragePolicy::FutureStoragePlaceholder,
                "patrol route string id payload future policy") &&
         expect(cr::mutationPayloadStoragePolicy(
                    cr::CreativeMutationKind::SetPatrolRoute,
                    cr::makeScalarPayload(1.0)) ==
                    cr::CreativeMutationStoragePolicy::Unknown,
                "patrol route wrong payload unknown policy") &&
         expect(cr::mutationHasStoredObjectEffect(
                    cr::CreativeMutationKind::SetPatrolRoute),
                "patrol route has stored payload option") &&
         expect(cr::mutationPayloadHasStoredObjectEffect(
                    cr::CreativeMutationKind::SetPatrolRoute, pathPayload),
                "patrol route path payload has stored effect") &&
         expect(!cr::mutationPayloadHasStoredObjectEffect(
                    cr::CreativeMutationKind::SetPatrolRoute,
                    cr::makeTextPayload("legacy")),
                "patrol route legacy payload has no stored effect") &&
         expect(!cr::mutationHasStoredObjectEffect(
                    cr::CreativeMutationKind::SetLightIntensity),
                "future light intensity has no stored effect");
}

bool mutationPayloadMetadataMatchesExpectedPayloadFamilies() {
  return expect(cr::requiresPayload(cr::CreativeMutationKind::Rename),
                "rename requires payload") &&
         expect(cr::payloadMatchesMutation(cr::CreativeMutationKind::Rename,
                                          cr::makeRenamePayload("name")),
                "rename payload matches") &&
         expect(!cr::payloadMatchesMutation(cr::CreativeMutationKind::Rename,
                                           cr::CreativeMutationPayload{}),
                "rename empty payload rejected") &&
         expect(!cr::requiresPayload(cr::CreativeMutationKind::ClearParent),
                "clear parent no payload required") &&
         expect(cr::payloadMatchesMutation(cr::CreativeMutationKind::ClearParent,
                                          cr::CreativeMutationPayload{}),
                "clear parent empty payload matches") &&
         expect(!cr::payloadMatchesMutation(cr::CreativeMutationKind::ClearParent,
                                           cr::makeRenamePayload("wrong")),
                "clear parent wrong payload rejected") &&
         expect(cr::payloadMatchesMutation(cr::CreativeMutationKind::SetBounds,
                                          cr::makeBoundsPayload(
                                              cr::CreativeBounds{
                                                  cr::CreativeVec3{0.0, 0.0, 0.0},
                                                  cr::CreativeVec3{1.0, 1.0, 1.0}})),
                "set bounds payload matches") &&
         expect(cr::payloadMatchesMutation(cr::CreativeMutationKind::SetTriggerShape,
                                          cr::makeBoundsPayload(
                                              cr::CreativeBounds{
                                                  cr::CreativeVec3{0.0, 0.0, 0.0},
                                                  cr::CreativeVec3{1.0, 1.0, 1.0}})),
                "trigger shape payload matches bounds") &&
         expect(cr::payloadMatchesMutation(
                    cr::CreativeMutationKind::SetAsset,
                    cr::makeAssetPayload(
                        cr::CreativeObjectKind::Rock, "asset_b",
                        cr::CreativeBounds{{-1.0, 0.0, -2.0},
                                           {1.0, 3.0, 2.0}})),
                "set asset payload matches") &&
         expect(!cr::payloadMatchesMutation(
                    cr::CreativeMutationKind::SetAsset,
                    cr::makeBoundsPayload(
                        cr::CreativeBounds{{-1.0, 0.0, -2.0},
                                           {1.0, 3.0, 2.0}})),
                "set asset rejects bounds-only payload") &&
         expect(cr::payloadMatchesMutation(cr::CreativeMutationKind::SetLightIntensity,
                                          cr::makeScalarPayload(3.0)),
                "scalar sensory payload matches") &&
         expect(cr::payloadMatchesMutation(cr::CreativeMutationKind::SetCameraTarget,
                                          cr::makeLinkPayload(42)),
                "camera target link payload matches") &&
         expect(cr::payloadMatchesMutation(
                    cr::CreativeMutationKind::SetPatrolRoute,
                    cr::makePathPointsPayload(
                        {cr::CreativePathPoint{{0.0, 0.0, 0.0}},
                         cr::CreativePathPoint{{1.0, 0.0, 1.0}}})),
                "patrol route path points payload matches") &&
         expect(cr::payloadMatchesMutation(cr::CreativeMutationKind::SetPatrolRoute,
                                          cr::makeTextPayload("legacy")),
                "patrol route legacy text payload matches") &&
         expect(cr::payloadMatchesMutation(cr::CreativeMutationKind::SetPatrolRoute,
                                          cr::makeStringIdPayload("legacy-id")),
                "patrol route legacy string id payload matches") &&
         expect(!cr::payloadMatchesMutation(cr::CreativeMutationKind::SetPatrolRoute,
                                           cr::makeScalarPayload(1.0)),
                "patrol route scalar payload rejected");
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

bool boundedMoveTranslatesCrateBoundsExactlyOnce() {
  cr::CreativeObjectId crateId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithCrate(crateId);
  const cr::CreativeObject* before = document.findObject(crateId);
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt = cr::moveDocumentObject(
      document, crateId, cr::CreativeVec3{2.0, 0.0, 3.0});
  const cr::CreativeObject* after = document.findObject(crateId);
  const std::uint64_t revisionBeforeNoChange = document.revision();

  const cr::CreativeDocumentMutationReceipt noChange = cr::moveDocumentObject(
      document, crateId, cr::CreativeVec3{2.0, 0.0, 3.0});
  const cr::CreativeObject* afterNoChange = document.findObject(crateId);

  return expect(before != nullptr, "crate move setup exists") &&
         expect(after != nullptr, "crate move after exists") &&
         expect(afterNoChange != nullptr, "crate move no-change exists") &&
         expect(receipt.status == cr::CreativeDocumentMutationStatus::Applied,
                "crate move status applied") &&
         expect(receipt.changed, "crate move changed") &&
         expect(receipt.allowed, "crate move allowed") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Crate,
                "crate move object kind") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::Move,
                "crate move mutation kind") &&
         expect(receipt.revisionBefore == revisionBefore,
                "crate move revision before") &&
         expect(receipt.revisionAfter == revisionBefore + 1U,
                "crate move revision after") &&
         expect(document.revision() == revisionBeforeNoChange,
                "crate move document revision advanced once") &&
         expect(sameVec3(after->transform.position,
                         cr::CreativeVec3{2.0, 0.0, 3.0}),
                "crate move position updated") &&
         expect(sameBounds(after->bounds,
                           cr::CreativeBounds{
                               cr::CreativeVec3{2.0, 0.0, 3.0},
                               cr::CreativeVec3{3.0, 1.0, 4.0}}),
                "crate move bounds translated") &&
         expect(cr::hasDirtyFlag(receipt.dirtyFlags,
                                 cr::CreativeObjectDirtyFlag::Transform),
                "crate move dirty transform") &&
         expect(cr::hasDirtyFlag(receipt.dirtyFlags,
                                 cr::CreativeObjectDirtyFlag::Bounds),
                "crate move dirty bounds") &&
         expect(receipt.objectReceipt.message == "object moved",
                "crate move object message") &&
         expect(noChange.status == cr::CreativeDocumentMutationStatus::NoChange,
                "crate repeated move no change") &&
         expect(!noChange.changed, "crate repeated move changed false") &&
         expect(noChange.revisionBefore == revisionBeforeNoChange,
                "crate repeated move revision before") &&
         expect(noChange.revisionAfter == revisionBeforeNoChange,
                "crate repeated move revision after") &&
         expect(document.revision() == revisionBeforeNoChange,
                "crate repeated move revision unchanged") &&
         expect(sameBounds(afterNoChange->bounds,
                           cr::CreativeBounds{
                               cr::CreativeVec3{2.0, 0.0, 3.0},
                               cr::CreativeVec3{3.0, 1.0, 4.0}}),
                "crate repeated move did not translate bounds again");
}

bool pointOnlyMovePreservesStoredBoundsField() {
  cr::CreativeObjectId pointId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithObjectKind(
      cr::CreativeObjectKind::SpawnPoint, pointId);
  const cr::CreativeObject* before = document.findObject(pointId);
  const cr::CreativeBounds boundsBefore =
      before != nullptr ? before->bounds : cr::CreativeBounds{};
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt = cr::moveDocumentObject(
      document, pointId, cr::CreativeVec3{1.0, 2.0, 3.0});
  const cr::CreativeObject* after = document.findObject(pointId);

  return expect(before != nullptr, "point move setup exists") &&
         expect(after != nullptr, "point move after exists") &&
         expect(receipt.status == cr::CreativeDocumentMutationStatus::Applied,
                "point move status applied") &&
         expect(receipt.changed, "point move changed") &&
         expect(receipt.revisionBefore == revisionBefore,
                "point move revision before") &&
         expect(receipt.revisionAfter == revisionBefore + 1U,
                "point move revision after") &&
         expect(sameVec3(after->transform.position,
                         cr::CreativeVec3{1.0, 2.0, 3.0}),
                "point move position updated") &&
         expect(sameBounds(after->bounds, boundsBefore),
                "point move bounds field unchanged");
}

bool setParentNoChangePreservesRevision() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Document");
  const cr::CreativeDocumentCreateReceipt parent =
      createObject(document, cr::CreativeObjectKind::Group, "Parent");
  const cr::CreativeDocumentCreateReceipt child =
      createObject(document, cr::CreativeObjectKind::Group, "Child",
                   parent.objectId);
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt = cr::applyDocumentMutation(
      document, child.objectId, cr::CreativeMutationKind::SetParent,
      cr::makeParentPayload(parent.objectId));
  const cr::CreativeObject* childObject = document.findObject(child.objectId);

  return expect(parent.accepted, "set parent no-change parent created") &&
         expect(child.accepted, "set parent no-change child created") &&
         expect(childObject != nullptr, "set parent no-change child exists") &&
         expect(childObject != nullptr && childObject->parentId.has_value() &&
                    *childObject->parentId == parent.objectId,
                "set parent no-change parent unchanged") &&
         expect(receipt.status == cr::CreativeDocumentMutationStatus::NoChange,
                "set parent no-change status") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::NoChange,
                "set parent no-change object status") &&
         expect(receipt.allowed, "set parent no-change allowed") &&
         expect(!receipt.changed, "set parent no-change changed false") &&
         expect(receipt.revisionBefore == revisionBefore,
                "set parent no-change revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "set parent no-change revision after") &&
         expect(document.revision() == revisionBefore,
                "set parent no-change document revision stable");
}

bool relationshipMutationsRejectInvalidParentTargets() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Document");
  const cr::CreativeDocumentCreateReceipt parent =
      createObject(document, cr::CreativeObjectKind::Group, "Parent");
  const cr::CreativeDocumentCreateReceipt child =
      createObject(document, cr::CreativeObjectKind::Group, "Child");
  const cr::CreativeDocumentCreateReceipt wallParent =
      createObject(document, cr::CreativeObjectKind::Wall, "Wall Parent");
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt missing = cr::applyDocumentMutation(
      document, child.objectId, cr::CreativeMutationKind::SetParent,
      cr::makeParentPayload(9999));
  const cr::CreativeDocumentMutationReceipt self = cr::applyDocumentMutation(
      document, child.objectId, cr::CreativeMutationKind::SetParent,
      cr::makeParentPayload(child.objectId));
  const cr::CreativeDocumentMutationReceipt ownerUnsupported =
      cr::applyDocumentMutation(
          document, child.objectId, cr::CreativeMutationKind::AttachTo,
          cr::makeAttachPayload(wallParent.objectId, "socket"));
  const cr::CreativeObject* childObject = document.findObject(child.objectId);

  return expect(parent.accepted, "relationship invalid parent setup parent") &&
         expect(child.accepted, "relationship invalid parent setup child") &&
         expect(wallParent.accepted,
                "relationship invalid parent setup wall parent") &&
         expect(childObject != nullptr,
                "relationship invalid parent child exists") &&
         expect(childObject != nullptr && !childObject->parentId.has_value(),
                "relationship invalid parent leaves child unparented") &&
         expect(missing.status ==
                    cr::CreativeDocumentMutationStatus::ApplyFailed,
                "missing relationship parent status") &&
         expect(missing.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::Rejected,
                "missing relationship parent object status") &&
         expect(missing.objectReceipt.message == "missing_parent",
                "missing relationship parent reason") &&
         expect(self.status == cr::CreativeDocumentMutationStatus::ApplyFailed,
                "self relationship parent status") &&
         expect(self.objectReceipt.message == "invalid_parent",
                "self relationship parent reason") &&
         expect(ownerUnsupported.status ==
                    cr::CreativeDocumentMutationStatus::ApplyFailed,
                "unsupported owner relationship status") &&
         expect(ownerUnsupported.objectReceipt.message ==
                    "parent_owner_unsupported",
                "unsupported owner relationship reason") &&
         expect(document.revision() == revisionBefore,
                "invalid relationship parent revisions stable");
}

bool relationshipMutationsRejectParentCycles() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Document");
  const cr::CreativeDocumentCreateReceipt root =
      createObject(document, cr::CreativeObjectKind::Group, "Root");
  const cr::CreativeDocumentCreateReceipt middle =
      createObject(document, cr::CreativeObjectKind::Group, "Middle",
                   root.objectId);
  const cr::CreativeDocumentCreateReceipt leaf =
      createObject(document, cr::CreativeObjectKind::Group, "Leaf",
                   middle.objectId);
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt directCycle =
      cr::applyDocumentMutation(
          document, root.objectId, cr::CreativeMutationKind::SetParent,
          cr::makeParentPayload(middle.objectId));
  const cr::CreativeDocumentMutationReceipt indirectCycle =
      cr::applyDocumentMutation(
          document, root.objectId, cr::CreativeMutationKind::AttachTo,
          cr::makeAttachPayload(leaf.objectId, "socket"));
  const cr::CreativeObject* rootObject = document.findObject(root.objectId);
  const cr::CreativeObject* middleObject =
      document.findObject(middle.objectId);
  const cr::CreativeObject* leafObject = document.findObject(leaf.objectId);

  return expect(root.accepted, "parent cycle root created") &&
         expect(middle.accepted, "parent cycle middle created") &&
         expect(leaf.accepted, "parent cycle leaf created") &&
         expect(rootObject != nullptr && !rootObject->parentId.has_value(),
                "parent cycle root remains unparented") &&
         expect(middleObject != nullptr && middleObject->parentId.has_value() &&
                    *middleObject->parentId == root.objectId,
                "parent cycle middle parent remains") &&
         expect(leafObject != nullptr && leafObject->parentId.has_value() &&
                    *leafObject->parentId == middle.objectId,
                "parent cycle leaf parent remains") &&
         expect(directCycle.status ==
                    cr::CreativeDocumentMutationStatus::ApplyFailed,
                "direct parent cycle status") &&
         expect(directCycle.objectReceipt.message == "parent_cycle",
                "direct parent cycle reason") &&
         expect(indirectCycle.status ==
                    cr::CreativeDocumentMutationStatus::ApplyFailed,
                "indirect parent cycle status") &&
         expect(indirectCycle.objectReceipt.message == "parent_cycle",
                "indirect parent cycle reason") &&
         expect(document.revision() == revisionBefore,
                "parent cycle revisions stable");
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
                "missing object message") &&
         expect(!receipt.message.empty(), "missing document message nonempty") &&
         expect(!receipt.objectReceipt.message.empty(),
                "missing object message nonempty");
}

bool descriptorAllowedTextSleeperVerbIsDocumentNoChange() {
  cr::CreativeObjectId noteId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithObjectKind(
      cr::CreativeObjectKind::Note, noteId);
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt = cr::applyDocumentMutation(
      document, noteId, cr::CreativeMutationKind::EditText,
      cr::makeTextPayload("hello"));
  const cr::CreativeMutationPayload payload = cr::makeTextPayload("hello");

  return expect(receipt.status == cr::CreativeDocumentMutationStatus::NoChange,
                "text sleeper document no change") &&
         expect(cr::mutationPayloadStoragePolicy(
                    cr::CreativeMutationKind::EditText, payload) ==
                    cr::CreativeMutationStoragePolicy::FutureStoragePlaceholder,
                "text sleeper future-storage policy") &&
         expect(!cr::mutationPayloadHasStoredObjectEffect(
                    cr::CreativeMutationKind::EditText, payload),
                "text sleeper no stored effect") &&
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
  const cr::CreativeMutationPayload payload = cr::makeScalarPayload(4.0);

  return expect(receipt.status == cr::CreativeDocumentMutationStatus::NoChange,
                "scalar sleeper document no change") &&
         expect(cr::mutationPayloadStoragePolicy(
                    cr::CreativeMutationKind::SetLightIntensity, payload) ==
                    cr::CreativeMutationStoragePolicy::FutureStoragePlaceholder,
                "scalar sleeper future-storage policy") &&
         expect(!cr::mutationPayloadHasStoredObjectEffect(
                    cr::CreativeMutationKind::SetLightIntensity, payload),
                "scalar sleeper no stored effect") &&
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
  const cr::CreativeMutationPayload payload = cr::makeLinkPayload(77);

  return expect(receipt.status == cr::CreativeDocumentMutationStatus::NoChange,
                "link sleeper document no change") &&
         expect(cr::mutationPayloadStoragePolicy(
                    cr::CreativeMutationKind::LinkTarget, payload) ==
                    cr::CreativeMutationStoragePolicy::FutureStoragePlaceholder,
                "link sleeper future-storage policy") &&
         expect(!cr::mutationPayloadHasStoredObjectEffect(
                    cr::CreativeMutationKind::LinkTarget, payload),
                "link sleeper no stored effect") &&
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
      document, roomId, cr::CreativeMutationKind::Rotate,
      cr::makeRotatePayload(cr::CreativeVec3{0.0, 90.0, 0.0}));

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
         expect(receipt.message ==
                    "document mutation applied through object mutation pipeline",
                "unsupported document message") &&
         expect(receipt.objectReceipt.message ==
                    "object kind does not allow this mutation",
                "unsupported object message") &&
         expect(!receipt.message.empty(),
                "unsupported document message nonempty") &&
         expect(!receipt.objectReceipt.message.empty(),
                "unsupported object message nonempty");
}

bool wrongPayloadFailureMessagesAreStable() {
  cr::CreativeObjectId roomId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithRoom(roomId);
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt = cr::applyDocumentMutation(
      document, roomId, cr::CreativeMutationKind::SetVisible,
      cr::makeMovePayload(cr::CreativeVec3{1.0, 2.0, 3.0}));

  return expect(receipt.status == cr::CreativeDocumentMutationStatus::ApplyFailed,
                "wrong payload status failed") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::WrongPayload,
                "wrong payload object status") &&
         expect(!receipt.changed, "wrong payload changed false") &&
         expect(!receipt.allowed, "wrong payload allowed false") &&
         expect(receipt.revisionBefore == revisionBefore,
                "wrong payload revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "wrong payload revision after") &&
         expect(document.revision() == revisionBefore,
                "wrong payload revision unchanged") &&
         expect(receipt.message ==
                    "document mutation applied through object mutation pipeline",
                "wrong payload document message") &&
         expect(receipt.objectReceipt.message ==
                    "mutation payload does not match mutation kind",
                "wrong payload object message") &&
         expect(!receipt.message.empty(),
                "wrong payload document message nonempty") &&
         expect(!receipt.objectReceipt.message.empty(),
                "wrong payload object message nonempty");
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
                "invalid message") &&
         expect(receipt.objectReceipt.message ==
                    "document mutation request is invalid",
                "invalid object message") &&
         expect(!receipt.message.empty(), "invalid document message nonempty") &&
         expect(!receipt.objectReceipt.message.empty(),
                "invalid object message nonempty");
}

bool cornerAnchorMoveTranslatesRoomBoundsAndKeepsIdentityTransform() {
  cr::CreativeObjectId roomId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithDefaultRoom(roomId);
  const cr::CreativeObject* before = document.findObject(roomId);
  const cr::CreativeBounds boundsBefore =
      before != nullptr ? before->bounds : cr::CreativeBounds{};
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeObjectDirtyFlags expectedDirtyFlags =
      cr::CreativeObjectDirtyFlag::Identity |
      cr::CreativeObjectDirtyFlag::Transform |
      cr::CreativeObjectDirtyFlag::Bounds |
      cr::CreativeObjectDirtyFlag::Geometry |
      cr::CreativeObjectDirtyFlag::Collision |
      cr::CreativeObjectDirtyFlag::Preview |
      cr::CreativeObjectDirtyFlag::Serialization;

  const cr::CreativeDocumentMutationReceipt receipt = cr::moveDocumentObject(
      document, roomId, cr::CreativeVec3{5.0, 0.0, 3.0});
  const cr::CreativeObject* after = document.findObject(roomId);

  return expect(before != nullptr, "room corner move setup exists") &&
         expect(sameBounds(boundsBefore,
                           cr::CreativeBounds{
                               cr::CreativeVec3{0.0, 0.0, 0.0},
                               cr::CreativeVec3{10.0, 4.0, 10.0}}),
                "room corner move default bounds") &&
         expect(after != nullptr, "room corner move after exists") &&
         expect(receipt.status == cr::CreativeDocumentMutationStatus::Applied,
                "room corner move status applied") &&
         expect(receipt.changed, "room corner move changed") &&
         expect(receipt.allowed, "room corner move allowed") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Room,
                "room corner move object kind") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::Move,
                "room corner move mutation kind") &&
         expect(receipt.revisionBefore == revisionBefore,
                "room corner move revision before") &&
         expect(receipt.revisionAfter == revisionBefore + 1U,
                "room corner move revision after") &&
         expect(document.revision() == revisionBefore + 1U,
                "room corner move document revision advanced") &&
         expect(sameBounds(after->bounds,
                           cr::CreativeBounds{
                               cr::CreativeVec3{5.0, 0.0, 3.0},
                               cr::CreativeVec3{15.0, 4.0, 13.0}}),
                "room corner move bounds anchored to position") &&
         expect(sameVec3(after->transform.position,
                         cr::CreativeVec3{0.0, 0.0, 0.0}),
                "room corner move transform position untouched") &&
         expect(sameVec3(after->transform.rotationEulerRadians,
                         cr::CreativeVec3{0.0, 0.0, 0.0}),
                "room corner move transform rotation untouched") &&
         expect(sameVec3(after->transform.scale,
                         cr::CreativeVec3{1.0, 1.0, 1.0}),
                "room corner move transform scale untouched") &&
         expect(receipt.dirtyFlags == expectedDirtyFlags,
                "room corner move exact dirty flags") &&
         expect(receipt.dirtyFlags ==
                    cr::dirtyFlagsForMutation(cr::CreativeObjectKind::Room,
                                              cr::CreativeMutationKind::Move),
                "room corner move dirty flags match descriptor derivation") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::Applied,
                "room corner move object status applied") &&
         expect(receipt.objectReceipt.message == "object moved",
                "room corner move object message");
}

bool cornerAnchorMoveToCurrentCornerIsNoChange() {
  cr::CreativeObjectId roomId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithDefaultRoom(roomId);
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt = cr::moveDocumentObject(
      document, roomId, cr::CreativeVec3{0.0, 0.0, 0.0});
  const cr::CreativeObject* room = document.findObject(roomId);

  return expect(room != nullptr, "room corner no-change exists") &&
         expect(receipt.status == cr::CreativeDocumentMutationStatus::NoChange,
                "room corner no-change status") &&
         expect(!receipt.changed, "room corner no-change changed false") &&
         expect(receipt.allowed, "room corner no-change allowed") &&
         expect(receipt.dirtyFlags == 0U, "room corner no-change dirty flags") &&
         expect(receipt.revisionBefore == revisionBefore,
                "room corner no-change revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "room corner no-change revision after") &&
         expect(document.revision() == revisionBefore,
                "room corner no-change document revision stable") &&
         expect(sameBounds(room->bounds,
                           cr::CreativeBounds{
                               cr::CreativeVec3{0.0, 0.0, 0.0},
                               cr::CreativeVec3{10.0, 4.0, 10.0}}),
                "room corner no-change bounds untouched") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::NoChange,
                "room corner no-change object status") &&
         expect(receipt.objectReceipt.message ==
                    "object position already matches requested value",
                "room corner no-change object message");
}

bool lockedRoomMoveRejectsWithoutBoundsChange() {
  cr::CreativeObjectId roomId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithDefaultRoom(roomId);
  const cr::CreativeDocumentMutationReceipt locked =
      cr::setDocumentObjectLocked(document, roomId, true);
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt = cr::moveDocumentObject(
      document, roomId, cr::CreativeVec3{5.0, 0.0, 3.0});
  const cr::CreativeObject* room = document.findObject(roomId);

  return expect(locked.status == cr::CreativeDocumentMutationStatus::Applied,
                "locked room move setup applied") &&
         expect(room != nullptr, "locked room move exists") &&
         expect(receipt.status ==
                    cr::CreativeDocumentMutationStatus::ApplyFailed,
                "locked room move document status") &&
         expect(!receipt.changed, "locked room move unchanged") &&
         expect(!receipt.allowed, "locked room move not allowed") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::Move,
                "locked room move mutation kind") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::LockedObject,
                "locked room move object status") &&
         expect(receipt.objectReceipt.message == "object is locked",
                "locked room move object message") &&
         expect(receipt.revisionBefore == revisionBefore,
                "locked room move revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "locked room move revision after") &&
         expect(document.revision() == revisionBefore,
                "locked room move document revision stable") &&
         expect(sameBounds(room->bounds,
                           cr::CreativeBounds{
                               cr::CreativeVec3{0.0, 0.0, 0.0},
                               cr::CreativeVec3{10.0, 4.0, 10.0}}),
                "locked room move bounds untouched");
}

bool documentSnapSettingsFeedTheToolBoundarySnapFunctions() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Document");
  const cr::CreativeDocumentSnapSettings defaults =
      document.documentSnapSettings();
  const cr::CreativeDocumentSnapReceipt defaultReceipt =
      cr::snapCreativeDocumentPoint({5.4, 0.0, 3.6}, defaults);

  cr::CreativeDocumentSnapSettings shifted = defaults;
  shifted.originX = 0.5;
  shifted.originZ = 0.5;
  const bool shiftedStored = document.setDocumentSnapSettings(shifted);
  const cr::CreativeDocumentSnapReceipt shiftedReceipt =
      cr::snapCreativeDocumentPoint({5.4, 0.0, 3.6},
                                    document.documentSnapSettings());

  cr::CreativeDocumentSnapSettings maskedXz = defaults;
  maskedXz.axes = cr::kCreativeDocumentSnapAxisXZ;
  const bool maskedStored = document.setDocumentSnapSettings(maskedXz);
  const cr::CreativeDocumentSnapReceipt maskedReceipt =
      cr::snapCreativeDocumentPoint({5.4, 2.7, 3.6},
                                    document.documentSnapSettings());

  cr::CreativeDocumentSnapSettings disabled = defaults;
  disabled.mode = cr::CreativeDocumentSnapMode::Disabled;
  const bool disabledStored = document.setDocumentSnapSettings(disabled);
  const cr::CreativeDocumentSnapReceipt disabledReceipt =
      cr::snapCreativeDocumentPoint({5.4, 0.0, 3.6},
                                    document.documentSnapSettings());

  return expect(defaults.mode == cr::CreativeDocumentSnapMode::Grid,
                "document snap default mode grid") &&
         expect(defaults.axes == cr::kCreativeDocumentSnapAxisXYZ,
                "document snap default axes xyz") &&
         expect(defaults.stepX == 1.0 && defaults.stepY == 1.0 &&
                    defaults.stepZ == 1.0,
                "document snap default unit steps") &&
         expect(defaults.originX == 0.0 && defaults.originY == 0.0 &&
                    defaults.originZ == 0.0,
                "document snap default zero origin") &&
         expect(defaultReceipt.snapped, "document snap default snapped") &&
         expect(sameVec3(cr::CreativeVec3{defaultReceipt.snappedPoint.x,
                                          defaultReceipt.snappedPoint.y,
                                          defaultReceipt.snappedPoint.z},
                         cr::CreativeVec3{5.0, 0.0, 4.0}),
                "document snap default grid point") &&
         expect(shiftedStored, "document snap shifted origin stored") &&
         expect(shiftedReceipt.snapped, "document snap shifted snapped") &&
         expect(sameVec3(cr::CreativeVec3{shiftedReceipt.snappedPoint.x,
                                          shiftedReceipt.snappedPoint.y,
                                          shiftedReceipt.snappedPoint.z},
                         cr::CreativeVec3{5.5, 0.0, 3.5}),
                "document snap shifted origin point") &&
         expect(maskedStored, "document snap masked axes stored") &&
         expect(maskedReceipt.snapped, "document snap masked snapped") &&
         expect(sameVec3(cr::CreativeVec3{maskedReceipt.snappedPoint.x,
                                          maskedReceipt.snappedPoint.y,
                                          maskedReceipt.snappedPoint.z},
                         cr::CreativeVec3{5.0, 2.7, 4.0}),
                "document snap masked leaves y unsnapped") &&
         expect(disabledStored, "document snap disabled stored") &&
         expect(!disabledReceipt.snapped, "document snap disabled not snapped") &&
         expect(!disabledReceipt.changed, "document snap disabled unchanged") &&
         expect(sameVec3(cr::CreativeVec3{disabledReceipt.snappedPoint.x,
                                          disabledReceipt.snappedPoint.y,
                                          disabledReceipt.snappedPoint.z},
                         cr::CreativeVec3{5.4, 0.0, 3.6}),
                "document snap disabled passthrough");
}

}  // namespace

bool lockedObjectRenameRejectsThroughPipeline() {
  cr::CreativeObjectId roomId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithRoom(roomId);
  const cr::CreativeDocumentMutationReceipt locked =
      cr::setDocumentObjectLocked(document, roomId, true);
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt =
      cr::renameDocumentObject(document, roomId, "Renamed Room");
  const cr::CreativeObject* room = document.findObject(roomId);

  return expect(locked.status == cr::CreativeDocumentMutationStatus::Applied,
                "locked rename setup applied") &&
         expect(room != nullptr, "locked rename room exists") &&
         expect(receipt.status ==
                    cr::CreativeDocumentMutationStatus::ApplyFailed,
                "locked rename document status") &&
         expect(cr::documentMutationFailed(receipt.status),
                "locked rename failed") &&
         expect(!receipt.changed, "locked rename unchanged") &&
         expect(!receipt.allowed, "locked rename not allowed") &&
         expect(receipt.mutationKind == cr::CreativeMutationKind::Rename,
                "locked rename mutation kind") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::LockedObject,
                "locked rename object status") &&
         expect(receipt.objectReceipt.message == "object is locked",
                "locked rename object message") &&
         expect(receipt.revisionBefore == revisionBefore,
                "locked rename revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "locked rename revision after") &&
         expect(document.revision() == revisionBefore,
                "locked rename document revision stable") &&
         expect(room->name == "Room", "locked rename name unchanged");
}

bool lockedObjectUnlockThenRenameApplies() {
  cr::CreativeObjectId roomId = cr::kInvalidObjectId;
  cr::CreativeDocument document = makeDocumentWithRoom(roomId);
  static_cast<void>(cr::setDocumentObjectLocked(document, roomId, true));
  const cr::CreativeDocumentMutationReceipt refused =
      cr::renameDocumentObject(document, roomId, "Renamed Room");

  const cr::CreativeDocumentMutationReceipt unlocked =
      cr::setDocumentObjectLocked(document, roomId, false);
  const cr::CreativeDocumentMutationReceipt renamed =
      cr::renameDocumentObject(document, roomId, "Renamed Room");
  const cr::CreativeObject* room = document.findObject(roomId);

  return expect(refused.status ==
                    cr::CreativeDocumentMutationStatus::ApplyFailed,
                "unlock rename setup refused") &&
         expect(unlocked.status == cr::CreativeDocumentMutationStatus::Applied,
                "unlock rename unlock applied") &&
         expect(unlocked.mutationKind == cr::CreativeMutationKind::SetLocked,
                "unlock rename unlock kind") &&
         expect(renamed.status == cr::CreativeDocumentMutationStatus::Applied,
                "unlock rename rename applied") &&
         expect(renamed.changed, "unlock rename changed") &&
         expect(room != nullptr && room->name == "Renamed Room",
                "unlock rename name updated") &&
         expect(room != nullptr && !room->locked,
                "unlock rename object unlocked");
}

bool setAssetAtomicallyChangesImportedRenderIdentity() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Assets");
  static_cast<void>(document.assignId(91U));
  cr::CreativeDocumentCreateRequest create;
  create.kind = cr::CreativeObjectKind::Prop;
  create.name = "Imported prop";
  create.assetId = "asset_a";
  create.transform.position = {4.0, 2.0, -3.0};
  create.transform.rotationEulerRadians = {0.1, 0.2, 0.3};
  create.transform.scale = {1.5, 2.0, 0.5};
  create.hasTransformOverride = true;
  create.bounds = {{3.0, 2.0, -4.0}, {5.0, 4.0, -2.0}};
  create.hasBoundsOverride = true;
  create.layerId = 7U;
  create.hasLayerOverride = true;
  create.tags = {"exterior", "stone"};
  const cr::CreativeDocumentCreateReceipt created =
      document.createObject(create);
  const cr::CreativeObject* createdObject =
      document.findObject(created.objectId);
  if (!expect(created.accepted && createdObject != nullptr,
              "set asset fixture created")) {
    return false;
  }
  const cr::CreativeObject before = *createdObject;
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeBounds replacementBounds{{3.5, 2.0, -5.0},
                                              {4.5, 6.0, -1.0}};
  const cr::CreativeDocumentMutationReceipt applied =
      cr::applyDocumentMutation(
          document, created.objectId, cr::CreativeMutationKind::SetAsset,
          cr::makeAssetPayload(cr::CreativeObjectKind::Rock, "asset_b",
                               replacementBounds));
  const cr::CreativeObject* after = document.findObject(created.objectId);

  bool ok = expect(applied.status ==
                           cr::CreativeDocumentMutationStatus::Applied &&
                       applied.changed && applied.allowed &&
                       applied.revisionBefore == revisionBefore &&
                       applied.revisionAfter == revisionBefore + 1U,
                   "set asset applies as one document revision") &&
            expect(after != nullptr &&
                       after->kind == cr::CreativeObjectKind::Rock &&
                       after->assetId == "asset_b" &&
                       sameBounds(after->bounds, replacementBounds),
                   "set asset changes kind id and bounds together") &&
            expect(after != nullptr && after->id == before.id &&
                       after->name == before.name &&
                       sameVec3(after->transform.position,
                                before.transform.position) &&
                       sameVec3(after->transform.rotationEulerRadians,
                                before.transform.rotationEulerRadians) &&
                       sameVec3(after->transform.scale,
                                before.transform.scale) &&
                       after->layerId == before.layerId &&
                       after->visible == before.visible &&
                       after->locked == before.locked &&
                       after->tags == before.tags &&
                       after->parentId == before.parentId,
                   "set asset preserves authored object identity and metadata") &&
            expect(cr::categoryOf(cr::CreativeMutationKind::SetAsset) ==
                           cr::CreativeMutationCategory::Content &&
                       cr::mutationChangesGeometry(
                           cr::CreativeMutationKind::SetAsset) &&
                       cr::mutationChangesRuntimeMeaning(
                           cr::CreativeMutationKind::SetAsset) &&
                       cr::mutationStoragePolicy(
                           cr::CreativeMutationKind::SetAsset) ==
                           cr::CreativeMutationStoragePolicy::StoredObject,
                   "set asset metadata marks durable geometry and runtime change") &&
            expect(cr::canMutate(cr::CreativeObjectKind::Prop,
                                 cr::CreativeMutationKind::SetAsset) &&
                       cr::canMutate(cr::CreativeObjectKind::Rock,
                                    cr::CreativeMutationKind::SetAsset) &&
                       cr::canMutate(cr::CreativeObjectKind::Bridge,
                                    cr::CreativeMutationKind::SetAsset) &&
                       !cr::canMutate(cr::CreativeObjectKind::Crate,
                                     cr::CreativeMutationKind::SetAsset),
                   "set asset is restricted to imported mesh object kinds");

  const cr::CreativeObject unchanged = *after;
  const std::uint64_t revisionBeforeInvalid = document.revision();
  const cr::CreativeDocumentMutationReceipt invalid =
      cr::applyDocumentMutation(
          document, created.objectId, cr::CreativeMutationKind::SetAsset,
          cr::makeAssetPayload(cr::CreativeObjectKind::Unknown, "",
                               replacementBounds));
  after = document.findObject(created.objectId);
  ok = expect(invalid.status ==
                      cr::CreativeDocumentMutationStatus::ApplyFailed &&
                  !invalid.changed &&
                  document.revision() == revisionBeforeInvalid,
              "invalid set asset fails without revision") &&
       expect(after != nullptr && after->kind == unchanged.kind &&
                  after->assetId == unchanged.assetId &&
                  sameBounds(after->bounds, unchanged.bounds),
              "invalid set asset cannot partially change object") &&
       ok;
  return ok;
}

bool lootAndExitSettingsAreDurableTypedMutations() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Objectives");

  cr::CreativeDocumentCreateRequest lootRequest;
  lootRequest.kind = cr::CreativeObjectKind::LootPoint;
  lootRequest.name = "Estate Key";
  const cr::CreativeDocumentCreateReceipt lootCreated =
      document.createObject(lootRequest);

  cr::CreativeDocumentCreateRequest exitRequest;
  exitRequest.kind = cr::CreativeObjectKind::ExitPoint;
  exitRequest.name = "Estate Exit";
  const cr::CreativeDocumentCreateReceipt exitCreated =
      document.createObject(exitRequest);

  const cr::CreativeObject* loot = document.findObject(lootCreated.objectId);
  const cr::CreativeObject* exit = document.findObject(exitCreated.objectId);
  bool ok =
      expect(lootCreated.accepted && exitCreated.accepted,
             "loot and exit objects create with valid defaults") &&
      expect(loot != nullptr && loot->lootPoint.itemId.empty() &&
                 loot->lootPoint.itemCount == 1U &&
                 loot->lootPoint.deactivateOnCollect,
             "loot point defaults are stable") &&
      expect(loot != nullptr &&
                 cr::effectiveCreativeLootPointItemId(*loot) ==
                     cr::makeCreativeAutomaticLootItemId(loot->id),
             "blank loot id resolves to deterministic object id") &&
      expect(exit != nullptr && exit->exitPoint.requiredItemId.empty() &&
                 exit->exitPoint.requiredItemCount == 0U,
             "exit point defaults do not invent a requirement") &&
      expect(exit != nullptr &&
                 cr::makeCreativeExitObjectiveId(exit->id) ==
                     "exit_creative_object_" + std::to_string(exit->id),
             "exit objective id is deterministic") &&
      expect(cr::canMutate(cr::CreativeObjectKind::LootPoint,
                           cr::CreativeMutationKind::SetLootPointSettings) &&
                 !cr::canMutate(
                     cr::CreativeObjectKind::ExitPoint,
                     cr::CreativeMutationKind::SetLootPointSettings) &&
                 cr::canMutate(
                     cr::CreativeObjectKind::ExitPoint,
                     cr::CreativeMutationKind::SetExitPointSettings) &&
                 !cr::canMutate(
                     cr::CreativeObjectKind::LootPoint,
                     cr::CreativeMutationKind::SetExitPointSettings),
             "loot and exit settings mutations have matching-kind ownership");

  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeLootPointSettings lootSettings{
      .itemId = "estate_key",
      .itemCount = 2U,
      .deactivateOnCollect = false,
  };
  const cr::CreativeDocumentMutationReceipt lootApplied =
      cr::applyDocumentMutation(
          document, lootCreated.objectId,
          cr::CreativeMutationKind::SetLootPointSettings,
          cr::makeLootPointSettingsPayload(lootSettings));
  const cr::CreativeExitPointSettings exitSettings{
      .requiredItemId = "estate_key",
      .requiredItemCount = 1U,
  };
  const cr::CreativeDocumentMutationReceipt exitApplied =
      cr::applyDocumentMutation(
          document, exitCreated.objectId,
          cr::CreativeMutationKind::SetExitPointSettings,
          cr::makeExitPointSettingsPayload(exitSettings));
  loot = document.findObject(lootCreated.objectId);
  exit = document.findObject(exitCreated.objectId);
  ok =
      expect(lootApplied.status ==
                     cr::CreativeDocumentMutationStatus::Applied &&
                 lootApplied.changed &&
                 lootApplied.revisionBefore == revisionBefore &&
                 lootApplied.revisionAfter == revisionBefore + 1U,
             "loot settings apply as one document revision") &&
      expect(exitApplied.status ==
                     cr::CreativeDocumentMutationStatus::Applied &&
                 exitApplied.changed &&
                 exitApplied.revisionBefore == revisionBefore + 1U &&
                 exitApplied.revisionAfter == revisionBefore + 2U,
             "exit settings apply as one document revision") &&
      expect(loot != nullptr && loot->lootPoint == lootSettings,
             "loot settings persist on the authored object") &&
      expect(exit != nullptr && exit->exitPoint == exitSettings,
             "exit settings persist on the authored object") &&
      ok;

  const std::uint64_t revisionBeforeInvalid = document.revision();
  cr::CreativeLootPointSettings invalidLoot = lootSettings;
  invalidLoot.itemCount = 0U;
  const cr::CreativeDocumentMutationReceipt invalidMutation =
      cr::applyDocumentMutation(
          document, lootCreated.objectId,
          cr::CreativeMutationKind::SetLootPointSettings,
          cr::makeLootPointSettingsPayload(invalidLoot));
  cr::CreativeDocumentCreateRequest invalidCreate;
  invalidCreate.kind = cr::CreativeObjectKind::ExitPoint;
  invalidCreate.name = "Invalid Exit";
  invalidCreate.hasExitPointSettingsOverride = true;
  invalidCreate.exitPoint.requiredItemId = "estate_key";
  invalidCreate.exitPoint.requiredItemCount = 0U;
  const cr::CreativeDocumentCreateReceipt rejectedCreate =
      document.createObject(invalidCreate);
  loot = document.findObject(lootCreated.objectId);
  return expect(invalidMutation.status ==
                    cr::CreativeDocumentMutationStatus::ApplyFailed &&
                !invalidMutation.changed &&
                document.revision() == revisionBeforeInvalid,
                "invalid loot mutation cannot advance document revision") &&
         expect(loot != nullptr && loot->lootPoint == lootSettings,
                "invalid loot mutation cannot partially change settings") &&
         expect(rejectedCreate.status ==
                    cr::CreativeDocumentCreateStatus::Rejected &&
                !rejectedCreate.accepted && !rejectedCreate.changed &&
                document.revision() == revisionBeforeInvalid,
                "invalid exit create is rejected without revision drift") &&
         ok;
}

int main() {
  const bool ok = mutationMetadataRegistryIsInternallyConsistent() &&
                  mutationStoragePolicySignalDistinguishesStoredAndFuturePlaceholders() &&
                  mutationPayloadMetadataMatchesExpectedPayloadFamilies() &&
                  setVisibleMutatesThroughDocumentGateway() &&
                  settingAlreadyCurrentVisibilityIsNoChange() &&
                  setVisibleFalseThenTrueIncrementsForEachRealChange() &&
                  scalarDimensionAxisPolicyIsStable() &&
                  alreadyCurrentScalarDimensionIsNoChange() &&
                  boundedMoveTranslatesCrateBoundsExactlyOnce() &&
                  pointOnlyMovePreservesStoredBoundsField() &&
                  setParentNoChangePreservesRevision() &&
                  relationshipMutationsRejectInvalidParentTargets() &&
                  relationshipMutationsRejectParentCycles() &&
                  cornerAnchorMoveTranslatesRoomBoundsAndKeepsIdentityTransform() &&
                  cornerAnchorMoveToCurrentCornerIsNoChange() &&
                  lockedRoomMoveRejectsWithoutBoundsChange() &&
                  documentSnapSettingsFeedTheToolBoundarySnapFunctions() &&
                  missingObjectRejectsWithoutRevisionAdvance() &&
                  descriptorAllowedTextSleeperVerbIsDocumentNoChange() &&
                  descriptorAllowedScalarSleeperVerbIsDocumentNoChange() &&
                  descriptorAllowedLinkSleeperVerbIsDocumentNoChange() &&
                  unsupportedMutationDoesNotFalselyReportApplied() &&
                  wrongPayloadFailureMessagesAreStable() &&
                  invalidMutationRequestRejectsBeforeApply() &&
                  lockedObjectRenameRejectsThroughPipeline() &&
                  lockedObjectUnlockThenRenameApplies() &&
                  setAssetAtomicallyChangesImportedRenderIdentity() &&
                  lootAndExitSettingsAreDurableTypedMutations();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
