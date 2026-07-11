#include "app/iggy3d/creative/tools/RoomShell.hpp"

#include "app/iggy3d/creative/document/DocumentMutation.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

bool near(double lhs, double rhs) {
  return std::abs(lhs - rhs) < 0.000001;
}

bool expectVec3(const cr::CreativeVec3& actual,
                const cr::CreativeVec3& expected,
                std::string_view message) {
  return expect(near(actual.x, expected.x) && near(actual.y, expected.y) &&
                    near(actual.z, expected.z),
                message);
}

bool expectBounds(const cr::CreativeBounds& actual,
                  const cr::CreativeBounds& expected,
                  std::string_view message) {
  return expectVec3(actual.min, expected.min, message) &&
         expectVec3(actual.max, expected.max, message);
}

cr::CreativeVec3 centerOfBounds(const cr::CreativeBounds& bounds) {
  return {
      bounds.min.x + (bounds.max.x - bounds.min.x) * 0.5,
      bounds.min.y + (bounds.max.y - bounds.min.y) * 0.5,
      bounds.min.z + (bounds.max.z - bounds.min.z) * 0.5,
  };
}

cr::CreativeBounds translateBounds(const cr::CreativeBounds& bounds,
                                   const cr::CreativeVec3& delta) {
  return {
      {bounds.min.x + delta.x, bounds.min.y + delta.y,
       bounds.min.z + delta.z},
      {bounds.max.x + delta.x, bounds.max.y + delta.y,
       bounds.max.z + delta.z},
  };
}

cr::CreativeDocumentCreateReceipt createObject(
    cr::CreativeDocument& document,
    cr::CreativeObjectKind kind,
    std::string_view name = "Object") {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::string(name);
  return document.createObject(request);
}

bool applyShellRequests(cr::CreativeDocument& document,
                        const cr::CreativeRoomShellBuildResult& shell) {
  bool accepted = shell.receipt.accepted;
  for (const cr::CreativeDocumentCreateRequest& createRequest :
       shell.createRequests) {
    const cr::CreativeDocumentCreateReceipt createReceipt =
        document.createObject(createRequest);
    accepted &= createReceipt.accepted;
  }
  return accepted;
}

bool defaultRoomBuildsFiveShellRequests() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Shell Test");
  const cr::CreativeDocumentCreateReceipt roomReceipt =
      createObject(document, cr::CreativeObjectKind::Room, "Room");

  cr::CreativeRoomShellBuildRequest request;
  request.document = &document;
  request.roomObjectId = roomReceipt.objectId;
  const cr::CreativeRoomShellBuildResult result =
      cr::buildCreativeRoomShellCreateRequests(request);

  bool tagsOk = true;
  bool anchorsOk = true;
  for (const cr::CreativeDocumentCreateRequest& createRequest :
       result.createRequests) {
    tagsOk &= cr::creativeRoomShellCreateRequestHasProvenance(
                  createRequest,
                  roomReceipt.objectId) &&
              createRequest.hasBoundsOverride && createRequest.visible;
    anchorsOk &= createRequest.hasTransformOverride &&
                 near(createRequest.transform.position.x,
                      centerOfBounds(createRequest.bounds).x) &&
                 near(createRequest.transform.position.y,
                      centerOfBounds(createRequest.bounds).y) &&
                 near(createRequest.transform.position.z,
                      centerOfBounds(createRequest.bounds).z);
  }

  return expect(result.receipt.accepted, "shell accepted") &&
         expect(result.receipt.status ==
                    cr::CreativeRoomShellStatus::Generated,
                "shell status") &&
         expect(result.receipt.reasonCode == "creative_room_shell_generated",
                "shell reason") &&
         expect(result.receipt.roomObjectId == roomReceipt.objectId,
                "shell room id") &&
         expect(result.receipt.generatedRequestCount == 5U,
                "generated count") &&
         expect(result.receipt.floorRequestCount == 1U, "floor count") &&
         expect(result.receipt.wallRequestCount == 4U, "wall count") &&
         expect(result.createRequests.size() == 5U, "request count") &&
         expect(tagsOk, "all requests carry parent and provenance tags") &&
         expect(anchorsOk, "all requests anchor at generated bounds center") &&
         expect(result.createRequests[0].kind == cr::CreativeObjectKind::Floor,
                "floor kind") &&
         expect(result.createRequests[0].name == "Room Shell Floor",
                "floor name") &&
         expectBounds(result.createRequests[0].bounds,
                      {{0.0, 0.0, 0.0}, {10.0, 0.25, 10.0}},
                      "floor bounds") &&
         expect(result.createRequests[1].kind == cr::CreativeObjectKind::Wall,
                "north kind") &&
         expectBounds(result.createRequests[1].bounds,
                      {{0.0, 0.0, 0.0}, {10.0, 4.0, 0.25}},
                      "north bounds") &&
         expectBounds(result.createRequests[2].bounds,
                      {{0.0, 0.0, 9.75}, {10.0, 4.0, 10.0}},
                      "south bounds") &&
         expectBounds(result.createRequests[3].bounds,
                      {{0.0, 0.0, 0.0}, {0.25, 4.0, 10.0}},
                      "west bounds") &&
         expectBounds(result.createRequests[4].bounds,
                      {{9.75, 0.0, 0.0}, {10.0, 4.0, 10.0}},
                      "east bounds");
}

bool provenanceHelpersRecognizeGeneratedRequestsAndObjects() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Provenance");
  const cr::CreativeDocumentCreateReceipt roomReceipt =
      createObject(document, cr::CreativeObjectKind::Room, "Room");

  cr::CreativeRoomShellBuildRequest shellRequest;
  shellRequest.document = &document;
  shellRequest.roomObjectId = roomReceipt.objectId;
  const cr::CreativeRoomShellBuildResult shell =
      cr::buildCreativeRoomShellCreateRequests(shellRequest);

  cr::CreativeDocumentCreateRequest hiddenGenerated =
      shell.createRequests.empty() ? cr::CreativeDocumentCreateRequest{}
                                   : shell.createRequests.front();
  hiddenGenerated.visible = false;
  hiddenGenerated.hasVisibleOverride = true;
  const cr::CreativeDocumentCreateReceipt hiddenReceipt =
      document.createObject(hiddenGenerated);

  cr::CreativeDocumentCreateRequest unrelatedTagged;
  unrelatedTagged.kind = cr::CreativeObjectKind::Floor;
  unrelatedTagged.name = "Tagged But Not Parent";
  unrelatedTagged.tags = {
      std::string(cr::generatedRoomShellTag()),
      cr::sourceRoomShellTag(roomReceipt.objectId),
  };
  const cr::CreativeDocumentCreateReceipt unrelatedReceipt =
      document.createObject(unrelatedTagged);

  const cr::CreativeObject* hiddenObject =
      document.findObject(hiddenReceipt.objectId);
  const cr::CreativeObject* unrelatedObject =
      document.findObject(unrelatedReceipt.objectId);
  const std::vector<cr::CreativeObjectId> collected =
      cr::collectCreativeRoomShellChildIds(document, roomReceipt.objectId);

  bool allRequestsHaveProvenance = shell.receipt.accepted;
  for (const cr::CreativeDocumentCreateRequest& createRequest :
       shell.createRequests) {
    allRequestsHaveProvenance &=
        cr::creativeRoomShellCreateRequestHasProvenance(
            createRequest,
            roomReceipt.objectId);
  }

  return expect(!cr::generatedRoomShellTag().empty(),
                "generated tag value present") &&
         expect(cr::sourceRoomShellTag(roomReceipt.objectId).find(
                    std::to_string(roomReceipt.objectId)) !=
                    std::string::npos,
                "source tag carries room id") &&
         expect(allRequestsHaveProvenance,
                "all generated requests have provenance") &&
         expect(hiddenReceipt.accepted, "hidden generated setup accepted") &&
         expect(hiddenObject != nullptr, "hidden generated findable") &&
         expect(hiddenObject != nullptr && !hiddenObject->visible,
                "hidden generated is hidden") &&
         expect(hiddenObject != nullptr &&
                    cr::creativeRoomShellObjectHasProvenance(
                        *hiddenObject,
                        roomReceipt.objectId),
                "hidden generated object provenance") &&
         expect(unrelatedReceipt.accepted, "unrelated tagged accepted") &&
         expect(unrelatedObject != nullptr, "unrelated tagged findable") &&
         expect(unrelatedObject != nullptr &&
                    !cr::creativeRoomShellObjectHasProvenance(
                        *unrelatedObject,
                        roomReceipt.objectId),
                "non-parented tagged object ignored") &&
         expect(collected.size() == 1U, "collected generated count") &&
         expect(!collected.empty() &&
                    collected.front() == hiddenReceipt.objectId,
                "collected hidden generated id") &&
         expect(cr::creativeRoomHasGeneratedShellChildren(
                    document,
                    roomReceipt.objectId),
                "room reports hidden generated child");
}

bool nonOriginGeneratedChildMoveTranslatesFromCenterAnchor() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Shell Move");
  cr::CreativeDocumentCreateRequest roomRequest;
  roomRequest.kind = cr::CreativeObjectKind::Room;
  roomRequest.name = "Offset Room";
  roomRequest.hasBoundsOverride = true;
  roomRequest.bounds = {{20.0, 2.0, -6.0}, {30.0, 8.0, 4.0}};
  const cr::CreativeDocumentCreateReceipt roomReceipt =
      document.createObject(roomRequest);

  cr::CreativeRoomShellBuildRequest shellRequest;
  shellRequest.document = &document;
  shellRequest.roomObjectId = roomReceipt.objectId;
  const cr::CreativeRoomShellBuildResult shell =
      cr::buildCreativeRoomShellCreateRequests(shellRequest);

  const cr::CreativeDocumentCreateReceipt floorReceipt =
      shell.createRequests.empty()
          ? cr::CreativeDocumentCreateReceipt{}
          : document.createObject(shell.createRequests.front());
  const cr::CreativeObject* before =
      document.findObject(floorReceipt.objectId);
  const cr::CreativeVec3 delta = {3.0, 0.5, -2.0};
  const cr::CreativeVec3 requestedAnchor =
      before == nullptr
          ? cr::CreativeVec3{}
          : cr::CreativeVec3{centerOfBounds(before->bounds).x + delta.x,
                             centerOfBounds(before->bounds).y + delta.y,
                             centerOfBounds(before->bounds).z + delta.z};
  const cr::CreativeBounds expectedBounds =
      before == nullptr ? cr::CreativeBounds{}
                        : translateBounds(before->bounds, delta);
  const cr::CreativeDocumentMutationReceipt moveReceipt =
      cr::moveDocumentObject(document, floorReceipt.objectId, requestedAnchor);
  const cr::CreativeObject* after = document.findObject(floorReceipt.objectId);

  return expect(roomReceipt.accepted, "offset room accepted") &&
         expect(shell.receipt.accepted, "offset shell accepted") &&
         expect(floorReceipt.accepted, "offset shell child accepted") &&
         expect(before != nullptr, "offset child before exists") &&
         expect(before == nullptr ||
                    expectVec3(before->transform.position,
                               centerOfBounds(before->bounds),
                               "offset child anchor is bounds center"),
                "offset child anchor checked") &&
         expect(moveReceipt.status == cr::CreativeDocumentMutationStatus::Applied,
                "offset child move applied") &&
         expect(after != nullptr, "offset child after exists") &&
         expect(after == nullptr ||
                    expectVec3(after->transform.position,
                               requestedAnchor,
                               "offset child moved anchor"),
                "offset child moved anchor checked") &&
         expect(after == nullptr ||
                    expectBounds(after->bounds,
                                 expectedBounds,
                                 "offset child bounds translated by delta"),
                "offset child bounds checked");
}

bool restoreRejectsGeneratedShellChildWithMissingParent() {
  cr::CreativeDocument source = cr::CreativeDocument::create("Shell Source");
  const cr::CreativeDocumentCreateReceipt roomReceipt =
      createObject(source, cr::CreativeObjectKind::Room, "Room");

  cr::CreativeRoomShellBuildRequest shellRequest;
  shellRequest.document = &source;
  shellRequest.roomObjectId = roomReceipt.objectId;
  const cr::CreativeRoomShellBuildResult shell =
      cr::buildCreativeRoomShellCreateRequests(shellRequest);
  const bool shellCreated = applyShellRequests(source, shell);

  cr::CreativeDocumentRestoreRequest request;
  request.documentId = 9001;
  request.name = "Dangling Shell";
  request.nextObjectId = source.nextObjectId();
  for (const cr::CreativeObject& object : source.objects()) {
    if (object.id != roomReceipt.objectId) {
      request.objects.push_back(object);
    }
  }

  cr::CreativeDocument target = cr::CreativeDocument::create("Target");
  const cr::CreativeDocumentCreateReceipt existing =
      createObject(target, cr::CreativeObjectKind::Crate, "Existing");
  const std::uint64_t revisionBefore = target.revision();
  const cr::CreativeDocumentRestoreReceipt restored =
      target.restoreForLoad(request);

  return expect(roomReceipt.accepted, "restore parent setup room") &&
         expect(shellCreated, "restore parent setup shell") &&
         expect(existing.accepted, "restore parent existing setup") &&
         expect(!restored.accepted, "restore missing parent rejected") &&
         expect(restored.status ==
                    cr::CreativeDocumentRestoreStatus::InvalidObject,
                "restore missing parent status") &&
         expect(restored.reasonCode == "missing_parent",
                "restore missing parent reason") &&
         expect(target.objectCount() == 1U,
                "restore missing parent object count unchanged") &&
         expect(target.containsObject(existing.objectId),
                "restore missing parent existing remains") &&
         expect(target.revision() == revisionBefore,
                "restore missing parent revision unchanged");
}

bool deletingRoomWithGeneratedShellChildrenRejects() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Shell Delete");
  const cr::CreativeDocumentCreateReceipt roomReceipt =
      createObject(document, cr::CreativeObjectKind::Room, "Room");

  cr::CreativeRoomShellBuildRequest shellRequest;
  shellRequest.document = &document;
  shellRequest.roomObjectId = roomReceipt.objectId;
  const cr::CreativeRoomShellBuildResult shell =
      cr::buildCreativeRoomShellCreateRequests(shellRequest);
  const bool shellCreated = applyShellRequests(document, shell);
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeObjectDirtyFlags dirtyBefore = document.dirtyFlags();

  const cr::CreativeDocumentRemoveReceipt removed =
      document.removeDocumentObject(roomReceipt.objectId);

  std::uint64_t childCount = 0;
  for (const cr::CreativeObject& object : document.objects()) {
    if (object.parentId.has_value() &&
        object.parentId.value() == roomReceipt.objectId) {
      ++childCount;
    }
  }

  return expect(roomReceipt.accepted, "parent delete room setup") &&
         expect(shellCreated, "parent delete shell setup") &&
         expect(!removed.accepted, "parent delete rejected") &&
         expect(!removed.changed, "parent delete unchanged") &&
         expect(!removed.objectRemoved, "parent delete not removed") &&
         expect(removed.status ==
                    cr::CreativeDocumentRemoveStatus::ParentHasChildren,
                "parent delete status") &&
         expect(removed.reasonCode == "parent_has_children",
                "parent delete reason") &&
         expect(document.containsObject(roomReceipt.objectId),
                "parent delete room remains") &&
         expect(childCount == 5U, "parent delete children remain") &&
         expect(document.objectCount() == 6U,
                "parent delete object count unchanged") &&
         expect(document.revision() == revisionBefore,
                "parent delete revision unchanged") &&
         expect(document.dirtyFlags() == dirtyBefore,
                "parent delete dirty unchanged");
}

bool missingDocumentRejects() {
  cr::CreativeRoomShellBuildRequest request;
  request.roomObjectId = 1;

  const cr::CreativeRoomShellBuildResult result =
      cr::buildCreativeRoomShellCreateRequests(request);

  return expect(!result.receipt.accepted, "missing document not accepted") &&
         expect(result.receipt.status ==
                    cr::CreativeRoomShellStatus::DocumentMissing,
                "missing document status") &&
         expect(result.receipt.reasonCode ==
                    "creative_room_shell_document_missing",
                "missing document reason") &&
         expect(result.createRequests.empty(), "missing document no requests");
}

bool noSelectionRejects() {
  const cr::CreativeDocument document =
      cr::CreativeDocument::create("Shell Test");
  cr::CreativeRoomShellBuildRequest request;
  request.document = &document;

  const cr::CreativeRoomShellBuildResult result =
      cr::buildCreativeRoomShellCreateRequests(request);

  return expect(!result.receipt.accepted, "no selection not accepted") &&
         expect(result.receipt.status ==
                    cr::CreativeRoomShellStatus::NoRoomSelected,
                "no selection status") &&
         expect(result.receipt.reasonCode ==
                    "creative_room_shell_no_room_selected",
                "no selection reason");
}

bool nonRoomSelectionRejects() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Shell Test");
  const cr::CreativeDocumentCreateReceipt crateReceipt =
      createObject(document, cr::CreativeObjectKind::Crate, "Crate");
  cr::CreativeRoomShellBuildRequest request;
  request.document = &document;
  request.roomObjectId = crateReceipt.objectId;

  const cr::CreativeRoomShellBuildResult result =
      cr::buildCreativeRoomShellCreateRequests(request);

  return expect(!result.receipt.accepted, "non-room not accepted") &&
         expect(result.receipt.status ==
                    cr::CreativeRoomShellStatus::SelectedNotRoom,
                "non-room status") &&
         expect(result.receipt.reasonCode ==
                    "creative_room_shell_selected_not_room",
                "non-room reason");
}

bool invalidBoundsReject() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Shell Test");
  const cr::CreativeDocumentCreateReceipt roomReceipt =
      createObject(document, cr::CreativeObjectKind::Room, "Room");
  cr::CreativeRoomShellBuildRequest request;
  request.document = &document;
  request.roomObjectId = roomReceipt.objectId;
  request.wallThickness = 20.0;

  const cr::CreativeRoomShellBuildResult result =
      cr::buildCreativeRoomShellCreateRequests(request);

  return expect(!result.receipt.accepted, "invalid bounds not accepted") &&
         expect(result.receipt.status ==
                    cr::CreativeRoomShellStatus::InvalidBounds,
                "invalid bounds status") &&
         expect(result.receipt.reasonCode ==
                    "creative_room_shell_invalid_bounds",
                "invalid bounds reason");
}

bool duplicateShellTagsReject() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Shell Test");
  const cr::CreativeDocumentCreateReceipt roomReceipt =
      createObject(document, cr::CreativeObjectKind::Room, "Room");

  cr::CreativeDocumentCreateRequest generated;
  generated.kind = cr::CreativeObjectKind::Floor;
  generated.name = "Existing Shell Floor";
  generated.parentId = roomReceipt.objectId;
  generated.tags = {std::string(cr::generatedRoomShellTag()),
                    cr::sourceRoomShellTag(roomReceipt.objectId)};
  const cr::CreativeDocumentCreateReceipt generatedReceipt =
      document.createObject(generated);

  cr::CreativeRoomShellBuildRequest request;
  request.document = &document;
  request.roomObjectId = roomReceipt.objectId;
  const cr::CreativeRoomShellBuildResult result =
      cr::buildCreativeRoomShellCreateRequests(request);

  return expect(generatedReceipt.accepted, "existing shell setup accepted") &&
         expect(!result.receipt.accepted, "duplicate not accepted") &&
         expect(result.receipt.status ==
                    cr::CreativeRoomShellStatus::AlreadyExists,
                "duplicate status") &&
         expect(result.receipt.reasonCode ==
                    "creative_room_shell_already_exists",
                "duplicate reason") &&
         expect(result.createRequests.empty(), "duplicate no requests");
}

bool findGeneratedShellChildrenUsesParentAndProvenanceTags() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Shell Remove");
  const cr::CreativeDocumentCreateReceipt roomReceipt =
      createObject(document, cr::CreativeObjectKind::Room, "Room");

  cr::CreativeRoomShellBuildRequest shellRequest;
  shellRequest.document = &document;
  shellRequest.roomObjectId = roomReceipt.objectId;
  const cr::CreativeRoomShellBuildResult shell =
      cr::buildCreativeRoomShellCreateRequests(shellRequest);
  const bool shellCreated = applyShellRequests(document, shell);

  cr::CreativeDocumentCreateRequest arbitraryChild;
  arbitraryChild.kind = cr::CreativeObjectKind::Floor;
  arbitraryChild.name = "Manual Child";
  arbitraryChild.parentId = roomReceipt.objectId;
  const cr::CreativeDocumentCreateReceipt arbitraryChildReceipt =
      document.createObject(arbitraryChild);

  cr::CreativeRoomShellRemoveRequest removeRequest;
  removeRequest.document = &document;
  removeRequest.roomObjectId = roomReceipt.objectId;
  const cr::CreativeRoomShellRemoveResult remove =
      cr::findCreativeRoomShellChildren(removeRequest);

  bool excludesArbitraryChild = true;
  for (const cr::CreativeObjectId objectId : remove.objectIds) {
    excludesArbitraryChild &= objectId != arbitraryChildReceipt.objectId;
  }

  return expect(roomReceipt.accepted, "remove finder room setup") &&
         expect(shellCreated, "remove finder shell setup") &&
         expect(arbitraryChildReceipt.accepted,
                "remove finder manual child setup") &&
         expect(cr::creativeRoomHasGeneratedShellChildren(
                    document,
                    roomReceipt.objectId),
                "room reports generated shell children") &&
         expect(remove.receipt.accepted, "remove finder accepted") &&
         expect(remove.receipt.status ==
                    cr::CreativeRoomShellStatus::Removed,
                "remove finder status") &&
         expect(remove.receipt.reasonCode == "creative_room_shell_removed",
                "remove finder reason") &&
         expect(remove.receipt.removedObjectCount == 5U,
                "remove finder removed count") &&
         expect(remove.receipt.floorObjectCount == 1U,
                "remove finder floor count") &&
         expect(remove.receipt.wallObjectCount == 4U,
                "remove finder wall count") &&
         expect(remove.objectIds.size() == 5U, "remove finder id count") &&
         expect(excludesArbitraryChild,
                "remove finder excludes arbitrary child");
}

bool findGeneratedShellChildrenRejectsWhenNoneExist() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Shell Empty");
  const cr::CreativeDocumentCreateReceipt roomReceipt =
      createObject(document, cr::CreativeObjectKind::Room, "Room");

  cr::CreativeRoomShellRemoveRequest request;
  request.document = &document;
  request.roomObjectId = roomReceipt.objectId;
  const cr::CreativeRoomShellRemoveResult result =
      cr::findCreativeRoomShellChildren(request);

  return expect(roomReceipt.accepted, "remove empty room setup") &&
         expect(!cr::creativeRoomHasGeneratedShellChildren(
                    document,
                    roomReceipt.objectId),
                "remove empty no generated children") &&
         expect(!result.receipt.accepted, "remove empty rejected") &&
         expect(result.receipt.status ==
                    cr::CreativeRoomShellStatus::NoGeneratedShell,
                "remove empty status") &&
         expect(result.receipt.reasonCode ==
                    "creative_room_shell_remove_not_found",
                "remove empty reason") &&
         expect(result.objectIds.empty(), "remove empty no ids");
}

}  // namespace

int main() {
  const bool ok = defaultRoomBuildsFiveShellRequests() &&
                  provenanceHelpersRecognizeGeneratedRequestsAndObjects() &&
                  nonOriginGeneratedChildMoveTranslatesFromCenterAnchor() &&
                  restoreRejectsGeneratedShellChildWithMissingParent() &&
                  deletingRoomWithGeneratedShellChildrenRejects() &&
                  missingDocumentRejects() &&
                  noSelectionRejects() &&
                  nonRoomSelectionRejects() &&
                  invalidBoundsReject() &&
                  duplicateShellTagsReject() &&
                  findGeneratedShellChildrenUsesParentAndProvenanceTags() &&
                  findGeneratedShellChildrenRejectsWhenNoneExist();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
