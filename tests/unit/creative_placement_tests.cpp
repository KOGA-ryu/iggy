#include "app/iggy3d/creative/Placement.hpp"

#include "app/iggy3d/creative/ObjectDescriptor.hpp"

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

cr::CreativeDocument documentWithObjects(std::size_t roomCount) {
  cr::CreativeDocument document = cr::CreativeDocument::create("Placement");
  for (std::size_t index = 0; index < roomCount; ++index) {
    cr::CreativeDocumentCreateRequest request;
    request.kind = cr::CreativeObjectKind::Room;
    static_cast<void>(document.createObject(request));
  }
  return document;
}

bool emptyDocumentKeepsDescriptorDefaults() {
  const cr::CreativeDocument document = documentWithObjects(0);
  const cr::CreativePlacedCreateRequest placed =
      cr::buildPlacedCreateRequest(document, cr::CreativeObjectKind::Room);

  return expect(placed.createRequest.kind == cr::CreativeObjectKind::Room,
                "empty kind copied") &&
         expect(placed.existingObjectCount == 0U, "empty object count") &&
         expect(placed.stepX == 1.0, "empty default snap step") &&
         expect(placed.offsetX == 0.0, "empty offset zero") &&
         expect(!placed.offsetApplied, "empty offset not applied") &&
         expect(!placed.boundsOffsetApplied, "empty no bounds offset") &&
         expect(!placed.transformOffsetApplied, "empty no transform offset") &&
         expect(!placed.createRequest.hasBoundsOverride,
                "empty no bounds override") &&
         expect(!placed.createRequest.hasTransformOverride,
                "empty no transform override");
}

bool roomOffsetsBoundsOnlyByCornerTranslation() {
  const cr::CreativeDocument document = documentWithObjects(1);
  const cr::CreativePlacedCreateRequest placed =
      cr::buildPlacedCreateRequest(document, cr::CreativeObjectKind::Room);

  return expect(placed.existingObjectCount == 1U, "room count one") &&
         expect(placed.offsetX == 1.0, "room offset one step") &&
         expect(placed.offsetApplied, "room offset applied") &&
         expect(placed.boundsOffsetApplied, "room bounds offset applied") &&
         expect(!placed.transformOffsetApplied,
                "room no transform offset (hasTransform=false)") &&
         expect(placed.createRequest.hasBoundsOverride,
                "room bounds override set") &&
         expect(!placed.createRequest.hasTransformOverride,
                "room transform override not set") &&
         expect(placed.createRequest.bounds.min.x == 1.0 &&
                    placed.createRequest.bounds.min.y == 0.0 &&
                    placed.createRequest.bounds.min.z == 0.0,
                "room bounds min translated") &&
         expect(placed.createRequest.bounds.max.x == 11.0 &&
                    placed.createRequest.bounds.max.y == 4.0 &&
                    placed.createRequest.bounds.max.z == 10.0,
                "room bounds max translated");
}

bool crateOffsetsTransformAndBoundsTogether() {
  const cr::CreativeDocument document = documentWithObjects(1);
  const cr::CreativeObjectDescriptor& descriptor =
      cr::describeObject(cr::CreativeObjectKind::Crate);
  const cr::CreativePlacedCreateRequest placed =
      cr::buildPlacedCreateRequest(document, cr::CreativeObjectKind::Crate);

  return expect(descriptor.hasTransform, "crate descriptor has transform") &&
         expect(descriptor.hasBounds, "crate descriptor has bounds") &&
         expect(descriptor.defaults.bounds.max.x == 1.0 &&
                    descriptor.defaults.bounds.max.y == 1.0 &&
                    descriptor.defaults.bounds.max.z == 1.0,
                "crate descriptor unit cube") &&
         expect(placed.offsetX == 1.0, "crate offset one step") &&
         expect(placed.offsetApplied, "crate offset applied") &&
         expect(placed.boundsOffsetApplied, "crate bounds offset applied") &&
         expect(placed.transformOffsetApplied,
                "crate transform offset applied") &&
         expect(placed.createRequest.hasBoundsOverride,
                "crate bounds override set") &&
         expect(placed.createRequest.hasTransformOverride,
                "crate transform override set") &&
         expect(placed.createRequest.transform.position.x == 1.0 &&
                    placed.createRequest.transform.position.y == 0.0 &&
                    placed.createRequest.transform.position.z == 0.0,
                "crate transform position translated") &&
         expect(placed.createRequest.bounds.min.x == 1.0 &&
                    placed.createRequest.bounds.min.y == 0.0 &&
                    placed.createRequest.bounds.min.z == 0.0,
                "crate bounds min translated") &&
         expect(placed.createRequest.bounds.max.x == 2.0 &&
                    placed.createRequest.bounds.max.y == 1.0 &&
                    placed.createRequest.bounds.max.z == 1.0,
                "crate bounds max translated") &&
         expect(placed.createRequest.transform.position.x ==
                    placed.createRequest.bounds.min.x,
                "crate position and bounds move together");
}

bool offsetScalesWithObjectCountAndSnapStep() {
  cr::CreativeDocument document = documentWithObjects(2);
  cr::CreativeDocumentSnapSettings settings =
      document.documentSnapSettings();
  settings.stepX = 2.5;
  static_cast<void>(document.setDocumentSnapSettings(settings));

  const cr::CreativePlacedCreateRequest placed =
      cr::buildPlacedCreateRequest(document, cr::CreativeObjectKind::Room);

  return expect(placed.existingObjectCount == 2U, "scaled count two") &&
         expect(placed.stepX == 2.5, "scaled step copied") &&
         expect(placed.offsetX == 5.0, "scaled offset count*step") &&
         expect(placed.createRequest.bounds.min.x == 5.0,
                "scaled bounds min") &&
         expect(placed.createRequest.bounds.max.x == 15.0,
                "scaled bounds max");
}

bool unknownKindGetsNoOverrides() {
  const cr::CreativeDocument document = documentWithObjects(3);
  const cr::CreativePlacedCreateRequest placed =
      cr::buildPlacedCreateRequest(document, cr::CreativeObjectKind::Unknown);

  return expect(placed.createRequest.kind == cr::CreativeObjectKind::Unknown,
                "unknown kind copied") &&
         expect(placed.offsetX == 3.0, "unknown offset computed") &&
         expect(!placed.offsetApplied, "unknown offset not applied") &&
         expect(!placed.createRequest.hasBoundsOverride,
                "unknown no bounds override") &&
         expect(!placed.createRequest.hasTransformOverride,
                "unknown no transform override");
}

bool placedRequestsRoundTripThroughDocumentCreate() {
  cr::CreativeDocument document = documentWithObjects(0);

  const cr::CreativePlacedCreateRequest firstPlaced =
      cr::buildPlacedCreateRequest(document, cr::CreativeObjectKind::Room);
  const cr::CreativeDocumentCreateReceipt first =
      document.createObject(firstPlaced.createRequest);
  const cr::CreativePlacedCreateRequest secondPlaced =
      cr::buildPlacedCreateRequest(document, cr::CreativeObjectKind::Room);
  const cr::CreativeDocumentCreateReceipt second =
      document.createObject(secondPlaced.createRequest);
  const cr::CreativePlacedCreateRequest cratePlaced =
      cr::buildPlacedCreateRequest(document, cr::CreativeObjectKind::Crate);
  const cr::CreativeDocumentCreateReceipt crate =
      document.createObject(cratePlaced.createRequest);

  const cr::CreativeObject* firstRoom = document.findObject(first.objectId);
  const cr::CreativeObject* secondRoom = document.findObject(second.objectId);
  const cr::CreativeObject* crateObject = document.findObject(crate.objectId);

  return expect(first.status == cr::CreativeDocumentCreateStatus::Created,
                "round trip first created") &&
         expect(second.status == cr::CreativeDocumentCreateStatus::Created,
                "round trip second created") &&
         expect(crate.status == cr::CreativeDocumentCreateStatus::Created,
                "round trip crate created") &&
         expect(firstRoom != nullptr && firstRoom->bounds.min.x == 0.0 &&
                    firstRoom->bounds.max.x == 10.0,
                "round trip first room at origin") &&
         expect(secondRoom != nullptr && secondRoom->bounds.min.x == 1.0 &&
                    secondRoom->bounds.max.x == 11.0,
                "round trip second room offset one") &&
         expect(firstRoom != nullptr && secondRoom != nullptr &&
                    firstRoom->bounds.min.x != secondRoom->bounds.min.x,
                "round trip rooms at different x") &&
         expect(crateObject != nullptr &&
                    crateObject->transform.position.x == 2.0 &&
                    crateObject->bounds.min.x == 2.0 &&
                    crateObject->bounds.max.x == 3.0,
                "round trip crate offset two with coherent bounds");
}

}  // namespace

int main() {
  const bool ok = emptyDocumentKeepsDescriptorDefaults() &&
                  roomOffsetsBoundsOnlyByCornerTranslation() &&
                  crateOffsetsTransformAndBoundsTogether() &&
                  offsetScalesWithObjectCountAndSnapStep() &&
                  unknownKindGetsNoOverrides() &&
                  placedRequestsRoundTripThroughDocumentCreate();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
