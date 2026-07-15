#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/tools/AttachmentSnap.hpp"

#include <cmath>
#include <iostream>
#include <numbers>
#include <string>
#include <utility>
#include <vector>

namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs, double epsilon = 0.000001) {
  return std::fabs(lhs - rhs) <= epsilon;
}

iggy3d::StaticMeshAttachmentSocket socket(
    std::string name,
    iggy3d::StaticMeshAttachmentSocketRole role,
    iggy3d::Vec3 position = {}) {
  return {std::move(name), "door.frame", role, position,
          {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F}};
}

iggy3d::StaticMeshAssetCatalog catalogWith(
    std::vector<iggy3d::StaticMeshAttachmentSocket> receivers) {
  iggy3d::StaticMeshAssetCatalog catalog;
  iggy3d::StaticMeshAssetCatalogEntry target;
  target.assetId = "door_frame";
  target.attachmentSockets = std::move(receivers);
  iggy3d::StaticMeshAssetCatalogEntry source;
  source.assetId = "door_leaf";
  source.attachmentSockets.push_back(socket(
      "door_leaf", iggy3d::StaticMeshAttachmentSocketRole::Plug,
      {0.25F, 0.0F, 0.0F}));
  catalog.entries.push_back(std::move(target));
  catalog.entries.push_back(std::move(source));
  return catalog;
}

cr::CreativeDocumentCreateReceipt createTarget(cr::CreativeDocument& document,
                                                double yawRadians = 0.0) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Prop;
  request.name = "Door Frame";
  request.assetId = "door_frame";
  request.transform.position = {4.0, 1.0, -2.0};
  request.transform.rotationEulerRadians.y = yawRadians;
  request.hasTransformOverride = true;
  request.bounds = {{3.25, 1.0, -2.2}, {4.75, 3.5, -1.8}};
  request.hasBoundsOverride = true;
  return document.createObject(request);
}

cr::CreativeAttachmentSnapRequest snapRequest(
    const cr::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog& catalog,
    cr::CreativeObjectId targetId,
    cr::CreativeVec3 aim = {4.0, 1.0, -2.0}) {
  cr::CreativeAttachmentSnapRequest request;
  request.document = &document;
  request.assetCatalog = &catalog;
  request.sourceAssetId = "door_leaf";
  request.targetObjectId = targetId;
  request.aimPoint = aim;
  return request;
}

bool alignsPlugToReceiverAndResolvesTiesDeterministically() {
  cr::CreativeDocument document = cr::CreativeDocument::create("snap");
  const cr::CreativeDocumentCreateReceipt target =
      createTarget(document, std::numbers::pi * 0.5);
  iggy3d::StaticMeshAssetCatalog catalog = catalogWith({
      socket("z_socket", iggy3d::StaticMeshAttachmentSocketRole::Receiver),
      socket("a_socket", iggy3d::StaticMeshAttachmentSocketRole::Receiver),
  });
  const cr::CreativeAttachmentSnapResult result =
      cr::resolveCreativeAttachmentSnap(
          snapRequest(document, catalog, target.objectId));
  const cr::CreativeVec3 sourceOffset = cr::rotateCreativeVectorEulerXyz(
      {0.25, 0.0, 0.0}, result.transform.rotationEulerRadians);
  const cr::CreativeVec3 resolvedSocket{
      result.transform.position.x + sourceOffset.x,
      result.transform.position.y + sourceOffset.y,
      result.transform.position.z + sourceOffset.z};

  return expect(target.accepted, "snap target created") &&
         expect(result.status == cr::CreativeAttachmentSnapStatus::Ready &&
                    result.snapped && result.positioned,
                "compatible socket resolves") &&
         expect(result.targetSocket == "a_socket" &&
                    result.sourceSocket == "door_leaf" &&
                    result.compatibility == "door.frame",
                "equal-distance socket tie uses stable names") &&
         expect(near(result.transform.rotationEulerRadians.y,
                     std::numbers::pi * 0.5),
                "source yaw aligns to receiver") &&
         expect(near(resolvedSocket.x, 4.0) && near(resolvedSocket.y, 1.0) &&
                    near(resolvedSocket.z, -2.0),
                "plug origin lands exactly on receiver origin");
}

bool reportsOutsideIncompatibleAndOccupiedWithoutInventingPlacement() {
  cr::CreativeDocument document = cr::CreativeDocument::create("snap states");
  const cr::CreativeDocumentCreateReceipt target = createTarget(document);
  iggy3d::StaticMeshAssetCatalog catalog = catalogWith({
      socket("door_frame", iggy3d::StaticMeshAttachmentSocketRole::Receiver),
  });
  const cr::CreativeAttachmentSnapResult outside =
      cr::resolveCreativeAttachmentSnap(
          snapRequest(document, catalog, target.objectId, {10.0, 1.0, -2.0}));

  iggy3d::StaticMeshAssetCatalog incompatible = catalog;
  incompatible.entries.front().attachmentSockets.front().compatibility =
      "window.frame";
  const cr::CreativeAttachmentSnapResult noMatch =
      cr::resolveCreativeAttachmentSnap(
          snapRequest(document, incompatible, target.objectId));

  cr::CreativeDocumentCreateRequest childRequest;
  childRequest.kind = cr::CreativeObjectKind::Door;
  childRequest.name = "Attached Door";
  childRequest.parentId = target.objectId;
  childRequest.attachmentSocket = "door_frame";
  const cr::CreativeDocumentCreateReceipt child =
      document.createObject(childRequest);
  const cr::CreativeAttachmentSnapResult occupied =
      cr::resolveCreativeAttachmentSnap(
          snapRequest(document, catalog, target.objectId));

  return expect(target.accepted && child.accepted,
                "snap state objects created") &&
         expect(outside.status ==
                    cr::CreativeAttachmentSnapStatus::OutsideRadius &&
                    !outside.positioned,
                "distant receiver does not pull placement") &&
         expect(noMatch.status ==
                    cr::CreativeAttachmentSnapStatus::NoCompatibleSocket &&
                    !noMatch.positioned,
                "incompatible receiver does not invent placement") &&
         expect(occupied.status ==
                    cr::CreativeAttachmentSnapStatus::Occupied &&
                    occupied.positioned && !occupied.snapped &&
                    occupied.targetSocket == "door_frame",
                "occupied receiver retains a red-preview transform");
}

bool attachMutationStoresSocketAndOrdinaryReparentingClearsIt() {
  cr::CreativeDocument document = cr::CreativeDocument::create("relationship");
  const cr::CreativeDocumentCreateReceipt target = createTarget(document);
  cr::CreativeDocumentCreateRequest childRequest;
  childRequest.kind = cr::CreativeObjectKind::Door;
  childRequest.name = "Door";
  const cr::CreativeDocumentCreateReceipt child =
      document.createObject(childRequest);
  const cr::CreativeDocumentMutationReceipt attached =
      cr::applyDocumentMutation(
          document, child.objectId, cr::CreativeMutationKind::AttachTo,
          cr::makeAttachPayload(target.objectId, "door_frame"));
  const cr::CreativeObject* attachedObject = document.findObject(child.objectId);
  const bool stored = attachedObject != nullptr &&
                      attachedObject->parentId == target.objectId &&
                      attachedObject->attachmentSocket == "door_frame";
  cr::CreativeDocumentCreateRequest secondRequest;
  secondRequest.kind = cr::CreativeObjectKind::Door;
  secondRequest.name = "Second Door";
  const cr::CreativeDocumentCreateReceipt second =
      document.createObject(secondRequest);
  const std::uint64_t revisionBeforeDuplicate = document.revision();
  const cr::CreativeDocumentMutationReceipt duplicateAttach =
      cr::applyDocumentMutation(
          document, second.objectId, cr::CreativeMutationKind::AttachTo,
          cr::makeAttachPayload(target.objectId, "door_frame"));
  cr::CreativeDocumentCreateRequest duplicateCreateRequest = secondRequest;
  duplicateCreateRequest.name = "Third Door";
  duplicateCreateRequest.parentId = target.objectId;
  duplicateCreateRequest.attachmentSocket = "door_frame";
  const cr::CreativeDocumentCreateReceipt duplicateCreate =
      document.createObject(duplicateCreateRequest);
  const cr::CreativeDocumentMutationReceipt reparented =
      cr::applyDocumentMutation(
          document, child.objectId, cr::CreativeMutationKind::SetParent,
          cr::makeParentPayload(target.objectId));
  const cr::CreativeObject* reparentedObject =
      document.findObject(child.objectId);
  const cr::CreativeDocumentMutationReceipt invalid =
      cr::applyDocumentMutation(
          document, child.objectId, cr::CreativeMutationKind::AttachTo,
          cr::makeAttachPayload(target.objectId, "bad socket"));

  return expect(target.accepted && child.accepted,
                "relationship objects created") &&
         expect(cr::documentMutationSucceeded(attached.status) && stored,
                "attach mutation stores parent and receiver name") &&
         expect(second.accepted &&
                    cr::documentMutationFailed(duplicateAttach.status) &&
                    duplicateAttach.objectReceipt.message ==
                        "attachment_socket_occupied" &&
                    !duplicateCreate.accepted &&
                    duplicateCreate.reasonCode ==
                        "attachment_socket_occupied" &&
                    document.findObject(second.objectId) != nullptr &&
                    document.findObject(second.objectId)->parentId ==
                        std::nullopt &&
                    duplicateAttach.revisionAfter == revisionBeforeDuplicate &&
                    duplicateCreate.revisionAfter == revisionBeforeDuplicate,
                "document boundary enforces one child per receiver") &&
         expect(cr::documentMutationSucceeded(reparented.status) &&
                    reparented.changed && reparentedObject != nullptr &&
                    reparentedObject->attachmentSocket.empty(),
                "ordinary parent mutation clears receiver identity") &&
         expect(cr::documentMutationFailed(invalid.status),
                "invalid receiver name fails closed");
}

}  // namespace

int main() {
  const bool ok = alignsPlugToReceiverAndResolvesTiesDeterministically() &&
                  reportsOutsideIncompatibleAndOccupiedWithoutInventingPlacement() &&
                  attachMutationStoresSocketAndOrdinaryReparentingClearsIt();
  if (!ok) {
    return 1;
  }
  std::cout << "PASS: creative attachment snap\n";
  return 0;
}
