#include "app/iggy3d/creative/tools/RoomShell.hpp"

#include "app/iggy3d/creative/document/DocumentMutation.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

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

bool requestHasTag(const cr::CreativeDocumentCreateRequest& request,
                   std::string_view tag) {
  for (const std::string& value : request.tags) {
    if (value == tag) {
      return true;
    }
  }
  return false;
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

  const std::string sourceTag =
      "source_room_" + std::to_string(roomReceipt.objectId);

  bool tagsOk = true;
  bool anchorsOk = true;
  for (const cr::CreativeDocumentCreateRequest& createRequest :
       result.createRequests) {
    tagsOk &= requestHasTag(createRequest, "generated_room_shell") &&
              requestHasTag(createRequest, sourceTag) &&
              createRequest.parentId.has_value() &&
              createRequest.parentId.value() == roomReceipt.objectId &&
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
                    cr::CreativeRoomShellBuildStatus::Generated,
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

bool missingDocumentRejects() {
  cr::CreativeRoomShellBuildRequest request;
  request.roomObjectId = 1;

  const cr::CreativeRoomShellBuildResult result =
      cr::buildCreativeRoomShellCreateRequests(request);

  return expect(!result.receipt.accepted, "missing document not accepted") &&
         expect(result.receipt.status ==
                    cr::CreativeRoomShellBuildStatus::DocumentMissing,
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
                    cr::CreativeRoomShellBuildStatus::NoRoomSelected,
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
                    cr::CreativeRoomShellBuildStatus::SelectedNotRoom,
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
                    cr::CreativeRoomShellBuildStatus::InvalidBounds,
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
  generated.tags = {"generated_room_shell",
                    "source_room_" + std::to_string(roomReceipt.objectId)};
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
                    cr::CreativeRoomShellBuildStatus::AlreadyExists,
                "duplicate status") &&
         expect(result.receipt.reasonCode ==
                    "creative_room_shell_already_exists",
                "duplicate reason") &&
         expect(result.createRequests.empty(), "duplicate no requests");
}

}  // namespace

int main() {
  const bool ok = defaultRoomBuildsFiveShellRequests() &&
                  nonOriginGeneratedChildMoveTranslatesFromCenterAnchor() &&
                  missingDocumentRejects() &&
                  noSelectionRejects() &&
                  nonRoomSelectionRejects() &&
                  invalidBoundsReject() &&
                  duplicateShellTagsReject();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
