#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/recipes/StructuralRoofRecipe.hpp"
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
    iggy3d::Vec3 position = {},
    std::string compatibility = "door.frame") {
  return {std::move(name), std::move(compatibility), role, position,
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

bool aimedSocketSelectionIsDeliberateAndSelfOccupancyCanBeIgnored() {
  cr::CreativeDocument document = cr::CreativeDocument::create("aimed socket");
  const cr::CreativeDocumentCreateReceipt target = createTarget(document);
  iggy3d::StaticMeshAssetCatalog catalog = catalogWith({
      socket("near_window", iggy3d::StaticMeshAttachmentSocketRole::Receiver,
             {0.0F, 0.0F, 0.0F}, "window.frame"),
      socket("far_door", iggy3d::StaticMeshAttachmentSocketRole::Receiver,
             {0.5F, 0.0F, 0.0F}),
  });

  const cr::CreativeAttachmentSnapResult automatic =
      cr::resolveCreativeAttachmentSnap(
          snapRequest(document, catalog, target.objectId));
  cr::CreativeAttachmentSnapRequest aimedRequest =
      snapRequest(document, catalog, target.objectId);
  aimedRequest.selectionMode =
      cr::CreativeAttachmentSnapSelectionMode::AimedSocket;
  const cr::CreativeAttachmentSnapResult incompatible =
      cr::resolveCreativeAttachmentSnap(aimedRequest);
  cr::CreativeAttachmentSocketMarkerRequest markerRequest;
  markerRequest.document = &document;
  markerRequest.assetCatalog = &catalog;
  markerRequest.sourceAssetId = "door_leaf";
  markerRequest.targetObjectId = target.objectId;
  markerRequest.aimPoint = aimedRequest.aimPoint;
  markerRequest.selectionMode =
      cr::CreativeAttachmentSnapSelectionMode::AimedSocket;
  const cr::CreativeAttachmentSocketMarkerFrame aimedMarkers =
      cr::buildCreativeAttachmentSocketMarkers(markerRequest);
  aimedRequest.aimPoint = {8.0, 1.0, -2.0};
  const cr::CreativeAttachmentSnapResult noAimedSocket =
      cr::resolveCreativeAttachmentSnap(aimedRequest);

  cr::CreativeDocumentCreateRequest childRequest;
  childRequest.kind = cr::CreativeObjectKind::Door;
  childRequest.name = "Attached Door";
  childRequest.parentId = target.objectId;
  childRequest.attachmentSocket = "far_door";
  const cr::CreativeDocumentCreateReceipt child =
      document.createObject(childRequest);
  cr::CreativeAttachmentSnapRequest occupiedRequest =
      snapRequest(document, catalog, target.objectId, {4.5, 1.0, -2.0});
  occupiedRequest.selectionMode =
      cr::CreativeAttachmentSnapSelectionMode::AimedSocket;
  const cr::CreativeAttachmentSnapResult occupied =
      cr::resolveCreativeAttachmentSnap(occupiedRequest);
  occupiedRequest.ignoredOccupantObjectId = child.objectId;
  const cr::CreativeAttachmentSnapResult ignoredSelf =
      cr::resolveCreativeAttachmentSnap(occupiedRequest);

  return expect(target.accepted && child.accepted,
                "aimed socket objects created") &&
         expect(automatic.status == cr::CreativeAttachmentSnapStatus::Ready &&
                    automatic.targetSocket == "far_door",
                "automatic mode may choose a farther compatible receiver") &&
         expect(incompatible.status ==
                        cr::CreativeAttachmentSnapStatus::NoCompatibleSocket &&
                    incompatible.receiverSelected &&
                    incompatible.targetSocket == "near_window" &&
                    !incompatible.positioned,
                "aim mode reports the incompatible receiver under the cursor") &&
         expect(aimedMarkers.accepted && aimedMarkers.markerCount == 2U &&
                    aimedMarkers.markers[0].selected &&
                    !aimedMarkers.markers[1].selected,
                "aimed receiver is explicit in the marker frame") &&
         expect(noAimedSocket.status ==
                        cr::CreativeAttachmentSnapStatus::AimedSocketUnavailable &&
                    !noAimedSocket.receiverSelected,
                "aim mode rejects when no receiver is inside its aperture") &&
         expect(occupied.status == cr::CreativeAttachmentSnapStatus::Occupied &&
                    occupied.targetSocket == "far_door",
                "aim mode reports occupied receiver") &&
         expect(ignoredSelf.status == cr::CreativeAttachmentSnapStatus::Ready &&
                    ignoredSelf.targetSocket == "far_door",
                "moving object does not occupy its own receiver");
}

bool markerFrameIsWorldSpaceBoundedAndUsesSnapCompatibility() {
  cr::CreativeDocument document = cr::CreativeDocument::create("markers");
  const cr::CreativeDocumentCreateReceipt target =
      createTarget(document, std::numbers::pi * 0.5);
  const cr::CreativeDocumentMutationReceipt scaled = cr::applyDocumentMutation(
      document, target.objectId, cr::CreativeMutationKind::Scale,
      cr::makeScalePayload({2.0, 1.0, 1.0}));
  iggy3d::StaticMeshAssetCatalog catalog = catalogWith({
      socket("available", iggy3d::StaticMeshAttachmentSocketRole::Receiver,
             {1.0F, 0.0F, 0.0F}),
      socket("incompatible",
             iggy3d::StaticMeshAttachmentSocketRole::Receiver,
             {2.0F, 0.0F, 0.0F}),
      socket("occupied", iggy3d::StaticMeshAttachmentSocketRole::Receiver,
             {3.0F, 0.0F, 0.0F}),
  });
  catalog.entries.front().attachmentSockets[1].compatibility = "window.frame";
  cr::CreativeDocumentCreateRequest childRequest;
  childRequest.kind = cr::CreativeObjectKind::Door;
  childRequest.name = "Occupied Door";
  childRequest.parentId = target.objectId;
  childRequest.attachmentSocket = "occupied";
  const cr::CreativeDocumentCreateReceipt child =
      document.createObject(childRequest);

  cr::CreativeAttachmentSocketMarkerRequest request;
  request.document = &document;
  request.assetCatalog = &catalog;
  request.sourceAssetId = "door_leaf";
  request.targetObjectId = target.objectId;
  request.aimPoint = {4.0, 1.0, -4.0};
  request.maxDistanceMeters = 10.0;
  const cr::CreativeAttachmentSocketMarkerFrame markers =
      cr::buildCreativeAttachmentSocketMarkers(request);
  request.maxDistanceMeters = 0.5;
  const cr::CreativeAttachmentSocketMarkerFrame tightRadius =
      cr::buildCreativeAttachmentSocketMarkers(request);

  iggy3d::StaticMeshAssetCatalog oversized = catalogWith({});
  for (std::size_t index = 0U;
       index < iggy3d::kMaxStaticMeshAttachmentSocketCount + 6U; ++index) {
    oversized.entries.front().attachmentSockets.push_back(socket(
        "receiver_" + std::to_string(index),
        iggy3d::StaticMeshAttachmentSocketRole::Receiver));
  }
  request.assetCatalog = &oversized;
  request.maxDistanceMeters = 10.0;
  const cr::CreativeAttachmentSocketMarkerFrame bounded =
      cr::buildCreativeAttachmentSocketMarkers(request);

  return expect(target.accepted &&
                    cr::documentMutationSucceeded(scaled.status) &&
                    child.accepted,
                "marker state setup accepted") &&
         expect(markers.accepted &&
                    markers.status ==
                        cr::CreativeAttachmentSocketMarkerStatus::Built &&
                    markers.markerCount == 3U &&
                    markers.receiverCount == 3U,
                "marker frame contains every receiver") &&
         expect(markers.markers[0].state ==
                        cr::CreativeAttachmentSocketMarkerState::Available &&
                    markers.markers[1].state ==
                        cr::CreativeAttachmentSocketMarkerState::Incompatible &&
                    markers.markers[2].state ==
                        cr::CreativeAttachmentSocketMarkerState::Occupied,
                "markers distinguish free compatible incompatible and occupied") &&
         expect(tightRadius.accepted &&
                    tightRadius.markers[0].state ==
                        cr::CreativeAttachmentSocketMarkerState::Available &&
                    tightRadius.markers[2].state ==
                        cr::CreativeAttachmentSocketMarkerState::OutOfRange,
                "marker colors do not advertise sockets outside snap radius") &&
         expect(near(markers.markers[0].worldPosition.x, 4.0) &&
                    near(markers.markers[0].worldPosition.y, 1.0) &&
                    near(markers.markers[0].worldPosition.z, -4.0),
                "marker position applies target scale rotation and translation") &&
         expect(near(markers.markers[0].worldForward.x, 1.0) &&
                    near(markers.markers[0].worldForward.y, 0.0) &&
                    near(markers.markers[0].worldForward.z, 0.0) &&
                    near(markers.markers[0].worldUp.x, 0.0) &&
                    near(markers.markers[0].worldUp.y, 1.0) &&
                    near(markers.markers[0].worldUp.z, 0.0),
                "marker exposes rotated forward and up axes") &&
         expect(bounded.accepted && bounded.truncated &&
                    bounded.markerCount ==
                        iggy3d::kMaxStaticMeshAttachmentSocketCount &&
                    bounded.receiverCount ==
                        iggy3d::kMaxStaticMeshAttachmentSocketCount + 6U &&
                    bounded.skippedCount == 6U,
                "marker frame stays within the asset socket capacity");
}

bool generatedStairPublishesRailAndStringerReceivers() {
  cr::CreativeDocument document = cr::CreativeDocument::create("stair sockets");
  cr::CreativeDocumentCreateRequest targetRequest;
  targetRequest.kind = cr::CreativeObjectKind::Stair;
  targetRequest.name = "Generated Stair";
  targetRequest.transform.position = {4.0, 1.5, -2.0};
  targetRequest.hasTransformOverride = true;
  targetRequest.bounds = {{3.0, 0.0, -4.0}, {5.0, 3.0, 0.0}};
  targetRequest.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt target =
      document.createObject(targetRequest);

  iggy3d::StaticMeshAssetCatalog catalog;
  iggy3d::StaticMeshAssetCatalogEntry rail;
  rail.assetId = "stair_rail_asset";
  rail.attachmentSockets.push_back(socket(
      "stair_rail_plug", iggy3d::StaticMeshAttachmentSocketRole::Plug,
      {}, "stair.rail"));
  catalog.entries.push_back(std::move(rail));

  cr::CreativeAttachmentSnapRequest request;
  request.document = &document;
  request.assetCatalog = &catalog;
  request.sourceAssetId = "stair_rail_asset";
  request.targetObjectId = target.objectId;
  request.aimPoint = {3.0, 1.5, -2.0};
  const cr::CreativeAttachmentSnapResult snapped =
      cr::resolveCreativeAttachmentSnap(request);

  cr::CreativeAttachmentSocketMarkerRequest markerRequest;
  markerRequest.document = &document;
  markerRequest.assetCatalog = &catalog;
  markerRequest.sourceAssetId = "stair_rail_asset";
  markerRequest.targetObjectId = target.objectId;
  markerRequest.aimPoint = request.aimPoint;
  const cr::CreativeAttachmentSocketMarkerFrame markers =
      cr::buildCreativeAttachmentSocketMarkers(markerRequest);

  return expect(target.accepted, "generated stair target created") &&
         expect(snapped.status == cr::CreativeAttachmentSnapStatus::Ready &&
                    snapped.targetReceiverCount == 4U &&
                    snapped.compatiblePairCount == 2U &&
                    snapped.targetSocket == "stair_rail_left" &&
                    snapped.compatibility == "stair.rail",
                "generated stair resolves canonical rail receiver") &&
         expect(near(snapped.transform.position.x, 3.0) &&
                    near(snapped.transform.position.y, 1.5) &&
                    near(snapped.transform.position.z, -2.0),
                "generated stair receiver uses authored world transform") &&
         expect(markers.accepted && markers.receiverCount == 4U &&
                    markers.markerCount == 4U &&
                    markers.markers[0].targetSocket == "stair_rail_left" &&
                    markers.markers[0].state ==
                        cr::CreativeAttachmentSocketMarkerState::Available &&
                    markers.markers[2].state ==
                        cr::CreativeAttachmentSocketMarkerState::Incompatible,
                "generated stair publishes bounded rail and stringer markers");
}

bool generatedRampPublishesSideEdgeReceivers() {
  cr::CreativeDocument document = cr::CreativeDocument::create("ramp sockets");
  cr::CreativeDocumentCreateRequest targetRequest;
  targetRequest.kind = cr::CreativeObjectKind::Ramp;
  targetRequest.name = "Generated Ramp";
  targetRequest.transform.position = {4.0, 1.5, -2.0};
  targetRequest.hasTransformOverride = true;
  targetRequest.bounds = {{3.0, 0.0, -4.0}, {5.0, 3.0, 0.0}};
  targetRequest.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt target =
      document.createObject(targetRequest);

  iggy3d::StaticMeshAssetCatalog catalog;
  iggy3d::StaticMeshAssetCatalogEntry edge;
  edge.assetId = "ramp_edge_asset";
  edge.attachmentSockets.push_back(socket(
      "ramp_edge_plug", iggy3d::StaticMeshAttachmentSocketRole::Plug,
      {}, "ramp.edge"));
  catalog.entries.push_back(std::move(edge));

  cr::CreativeAttachmentSnapRequest request;
  request.document = &document;
  request.assetCatalog = &catalog;
  request.sourceAssetId = "ramp_edge_asset";
  request.targetObjectId = target.objectId;
  request.aimPoint = {3.0, 1.5, -2.0};
  const cr::CreativeAttachmentSnapResult snapped =
      cr::resolveCreativeAttachmentSnap(request);

  cr::CreativeAttachmentSocketMarkerRequest markerRequest;
  markerRequest.document = &document;
  markerRequest.assetCatalog = &catalog;
  markerRequest.sourceAssetId = "ramp_edge_asset";
  markerRequest.targetObjectId = target.objectId;
  markerRequest.aimPoint = request.aimPoint;
  markerRequest.maxDistanceMeters = 3.0;
  const cr::CreativeAttachmentSocketMarkerFrame markers =
      cr::buildCreativeAttachmentSocketMarkers(markerRequest);

  return expect(target.accepted, "generated ramp target created") &&
         expect(snapped.status == cr::CreativeAttachmentSnapStatus::Ready &&
                    snapped.targetReceiverCount == 2U &&
                    snapped.compatiblePairCount == 2U &&
                    snapped.targetSocket == "ramp_edge_left" &&
                    snapped.compatibility == "ramp.edge",
                "generated ramp resolves canonical side-edge receiver") &&
         expect(near(snapped.transform.position.x, 3.0) &&
                    near(snapped.transform.position.y, 1.5) &&
                    near(snapped.transform.position.z, -2.0),
                "generated ramp receiver uses authored world transform") &&
         expect(markers.accepted && markers.receiverCount == 2U &&
                    markers.markerCount == 2U &&
                    markers.markers[0].targetSocket == "ramp_edge_left" &&
                    markers.markers[0].state ==
                        cr::CreativeAttachmentSocketMarkerState::Available &&
                    markers.markers[1].state ==
                        cr::CreativeAttachmentSocketMarkerState::Available,
                "generated ramp publishes bounded side-edge markers");
}

bool generatedRoofsPublishDrainageReceivers() {
  cr::CreativeDocument document = cr::CreativeDocument::create("roof sockets");
  cr::CreativeDocumentCreateRequest flatRequest;
  flatRequest.kind = cr::CreativeObjectKind::Roof;
  flatRequest.name = "Flat Roof";
  flatRequest.bounds = {{-4.0, 3.0, -3.0}, {4.0, 3.25, 3.0}};
  flatRequest.hasBoundsOverride = true;
  flatRequest.transform.position =
      cr::measureCreativeBounds(flatRequest.bounds).center;
  flatRequest.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt flat =
      document.createObject(flatRequest);

  cr::CreativeStructuralRoofRecipeRequest roofRequest;
  roofRequest.style = cr::CreativeStructuralRoofStyle::Gable;
  roofRequest.ridgeAxis = cr::CreativeStructuralRoofRidgeAxis::X;
  roofRequest.minimumX = -4.0;
  roofRequest.maximumX = 4.0;
  roofRequest.minimumZ = -3.0;
  roofRequest.maximumZ = 3.0;
  roofRequest.supportPlaneMeters = 3.0;
  roofRequest.pitchDegrees = 45.0;
  const cr::CreativeStructuralRoofRecipeResult roof =
      cr::planCreativeStructuralRoof(roofRequest);
  cr::CreativeDocumentCreateReceipt slope;
  cr::CreativeTransform slopeTransform;
  if (roof.accepted) {
    const cr::CreativeStructuralRoofPart& part = roof.parts[0];
    cr::CreativeDocumentCreateRequest slopeRequest;
    slopeRequest.kind = part.kind;
    slopeRequest.name = "North Roof Slope";
    slopeRequest.bounds = part.bounds;
    slopeRequest.hasBoundsOverride = true;
    slopeTransform.position = cr::measureCreativeBounds(part.bounds).center;
    slopeTransform.rotationEulerRadians = part.rotationEulerRadians;
    slopeRequest.transform = slopeTransform;
    slopeRequest.hasTransformOverride = true;
    slope = document.createObject(slopeRequest);
  }

  iggy3d::StaticMeshAssetCatalog catalog;
  iggy3d::StaticMeshAssetCatalogEntry gutter;
  gutter.assetId = "gutter_asset";
  gutter.attachmentSockets.push_back(socket(
      "gutter_plug", iggy3d::StaticMeshAttachmentSocketRole::Plug, {},
      "roof.drainage"));
  catalog.entries.push_back(std::move(gutter));

  cr::CreativeAttachmentSnapRequest flatSnapRequest;
  flatSnapRequest.document = &document;
  flatSnapRequest.assetCatalog = &catalog;
  flatSnapRequest.sourceAssetId = "gutter_asset";
  flatSnapRequest.targetObjectId = flat.objectId;
  flatSnapRequest.aimPoint = {0.0, 3.25, -3.0};
  const cr::CreativeAttachmentSnapResult flatSnap =
      cr::resolveCreativeAttachmentSnap(flatSnapRequest);

  cr::CreativeAttachmentSnapResult slopeSnap;
  cr::CreativeVec3 slopeEave;
  if (slope.accepted) {
    const cr::CreativeStructuralRoofPartSocketResult sockets =
        cr::planCreativeStructuralRoofPartSockets(
            roof.parts[0].kind, roof.parts[0].bounds, slopeTransform);
    if (sockets.accepted) {
      const cr::CreativeVec3 rotated = cr::rotateCreativeVectorEulerXyz(
          sockets.sockets[0].localPosition,
          slopeTransform.rotationEulerRadians);
      slopeEave = {slopeTransform.position.x + rotated.x,
                   slopeTransform.position.y + rotated.y,
                   slopeTransform.position.z + rotated.z};
      cr::CreativeAttachmentSnapRequest slopeSnapRequest = flatSnapRequest;
      slopeSnapRequest.targetObjectId = slope.objectId;
      slopeSnapRequest.aimPoint = slopeEave;
      slopeSnap = cr::resolveCreativeAttachmentSnap(slopeSnapRequest);
    }
  }

  return expect(flat.accepted && roof.accepted && slope.accepted,
                "generated roof targets created") &&
         expect(flatSnap.status == cr::CreativeAttachmentSnapStatus::Ready &&
                    flatSnap.targetReceiverCount == 4U &&
                    flatSnap.compatiblePairCount == 4U &&
                    flatSnap.targetSocket == "roof_drainage_north" &&
                    flatSnap.compatibility == "roof.drainage",
                "flat roof routes one plug across four drainage receivers") &&
         expect(slopeSnap.status == cr::CreativeAttachmentSnapStatus::Ready &&
                    slopeSnap.targetReceiverCount == 1U &&
                    slopeSnap.compatiblePairCount == 1U &&
                    slopeSnap.targetSocket == "roof_drainage_north" &&
                    near(slopeSnap.transform.position.x, slopeEave.x) &&
                    near(slopeSnap.transform.position.y, slopeEave.y) &&
                    near(slopeSnap.transform.position.z, slopeEave.z),
                "sloped roof snaps drainage to the visible low eave");
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
                  aimedSocketSelectionIsDeliberateAndSelfOccupancyCanBeIgnored() &&
                  markerFrameIsWorldSpaceBoundedAndUsesSnapCompatibility() &&
                  generatedStairPublishesRailAndStringerReceivers() &&
                  generatedRampPublishesSideEdgeReceivers() &&
                  generatedRoofsPublishDrainageReceivers() &&
                  attachMutationStoresSocketAndOrdinaryReparentingClearsIt();
  if (!ok) {
    return 1;
  }
  std::cout << "PASS: creative attachment snap\n";
  return 0;
}
