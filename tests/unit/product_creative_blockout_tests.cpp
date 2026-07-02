#include "app/iggy3d/creative/Blockout.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool expectVec3(iggy3d::Vec3 value, iggy3d::Vec3 expected, std::string_view message) {
  if (!iggy3d::nearlyEqual(value, expected)) {
    std::cerr << "FAIL: " << message << " got (" << value.x << ", " << value.y
              << ", " << value.z << ") expected (" << expected.x << ", "
              << expected.y << ", " << expected.z << ")\n";
    return false;
  }
  return true;
}

iggy3d::EditableRoomFloor testFloor() {
  iggy3d::EditableRoomFloor floor;
  floor.id = "floor_001";
  floor.centerMeters = {1.0F, 0.0F, 2.0F};
  floor.sizeMeters = {3.0F, 0.2F, 2.0F};
  return floor;
}

iggy3d::EditableRoomWall testWall() {
  iggy3d::EditableRoomWall wall;
  wall.id = "wall_001";
  wall.startMeters = {0.0F, 0.0F, 0.0F};
  wall.endMeters = {4.0F, 0.0F, 0.0F};
  wall.bottomY = 0.25F;
  wall.heightMeters = 2.0F;
  wall.thicknessMeters = 0.5F;
  return wall;
}

iggy3d::EditableRoomObject testObject() {
  iggy3d::EditableRoomObject object;
  object.id = "object_001";
  object.positionMeters = {2.0F, 1.0F, 3.0F};
  object.sizeMeters = {1.0F, 2.0F, 3.0F};
  object.yawDegrees = 45.0F;
  return object;
}

bool namesAreStable() {
  return expect(iggy3d::productCreativeBlockoutPrimitiveKindName(
                    iggy3d::ProductCreativeBlockoutPrimitiveKind::Floor) == "floor",
                "floor kind name") &&
         expect(iggy3d::productCreativeBlockoutPrimitiveKindName(
                    iggy3d::ProductCreativeBlockoutPrimitiveKind::Wall) == "wall",
                "wall kind name") &&
         expect(iggy3d::productCreativeBlockoutPrimitiveKindName(
                    iggy3d::ProductCreativeBlockoutPrimitiveKind::Object) == "object",
                "object kind name") &&
         expect(iggy3d::productCreativeBlockoutFaceName(
                    iggy3d::ProductCreativeBlockoutFace::Top) == "top",
                "top face name") &&
         expect(iggy3d::productCreativeBlockoutFaceName(
                    iggy3d::ProductCreativeBlockoutFace::West) == "west",
                "west face name");
}

bool missingAndInvalidRequestsFailClosed() {
  const iggy3d::ProductCreativeBlockoutOverlay missing =
      iggy3d::buildProductCreativeBlockoutOverlay({});
  iggy3d::EditableRoomDocument document;
  const iggy3d::ProductCreativeBlockoutOverlay badStep =
      iggy3d::buildProductCreativeBlockoutOverlay({&document, 0.0F});
  return expect(!missing.ok, "missing document fails") &&
         expect(missing.reasonCode == "creative_blockout_missing_document",
                "missing reason") &&
         expect(missing.faces.empty(), "missing faces empty") &&
         expect(missing.labels.empty(), "missing labels empty") &&
         expect(!badStep.ok, "bad step fails") &&
         expect(badStep.reasonCode == "creative_blockout_invalid_grid_step",
                "bad step reason");
}

bool floorPacketContainsTopFaceAndLabels() {
  iggy3d::EditableRoomDocument document;
  document.floors.push_back(testFloor());
  const iggy3d::ProductCreativeBlockoutOverlay overlay =
      iggy3d::buildProductCreativeBlockoutOverlay({&document, 1.0F});
  const iggy3d::ProductCreativeBlockoutFaceOverlay& face = overlay.faces.front();
  return expect(overlay.ok, "floor overlay ok") &&
         expect(overlay.floorCount == 1U, "floor count") &&
         expect(overlay.faces.size() == 1U, "floor face count") &&
         expect(overlay.labels.size() == 2U, "floor label count") &&
         expect(face.primitive.kind ==
                    iggy3d::ProductCreativeBlockoutPrimitiveKind::Floor,
                "floor face kind") &&
         expect(face.primitive.id == "floor_001", "floor id") &&
         expect(face.face == iggy3d::ProductCreativeBlockoutFace::Top,
                "floor top face") &&
         expectVec3(face.centerMeters, {1.0F, 0.1F, 2.0F}, "floor face center") &&
         expect(face.widthMeters == 3.0F, "floor width") &&
         expect(face.heightMeters == 2.0F, "floor depth as face height") &&
         expect(face.gridLineCountU == 4U, "floor grid U") &&
         expect(face.gridLineCountV == 3U, "floor grid V") &&
         expect(overlay.labels[0].label == "W 3.0m", "floor width label") &&
         expect(overlay.labels[1].label == "D 2.0m", "floor depth label");
}

bool wallPacketContainsSixFacesAndLabels() {
  iggy3d::EditableRoomDocument document;
  document.walls.push_back(testWall());
  const iggy3d::ProductCreativeBlockoutOverlay overlay =
      iggy3d::buildProductCreativeBlockoutOverlay({&document, 1.0F});
  return expect(overlay.ok, "wall overlay ok") &&
         expect(overlay.wallCount == 1U, "wall count") &&
         expect(overlay.faces.size() == 6U, "wall face count") &&
         expect(overlay.labels.size() == 3U, "wall label count") &&
         expect(overlay.faces[0].face == iggy3d::ProductCreativeBlockoutFace::North,
                "wall first north") &&
         expect(overlay.faces[0].widthMeters == 4.0F, "wall length") &&
         expect(overlay.faces[0].heightMeters == 2.0F, "wall height") &&
         expect(overlay.faces[0].gridLineCountU == 5U, "wall grid U") &&
         expect(overlay.faces[0].gridLineCountV == 3U, "wall grid V") &&
         expect(overlay.faces[2].face == iggy3d::ProductCreativeBlockoutFace::Top,
                "wall top third") &&
         expect(overlay.faces[2].heightMeters == 0.5F, "wall thickness as top height") &&
         expect(overlay.labels[0].label == "W 4.0m", "wall width label") &&
         expect(overlay.labels[1].label == "H 2.0m", "wall height label") &&
         expect(overlay.labels[2].label == "D 0.5m", "wall depth label");
}

bool objectPacketContainsAxisAlignedFacesYawAndLabels() {
  iggy3d::EditableRoomDocument document;
  document.objects.push_back(testObject());
  const iggy3d::ProductCreativeBlockoutOverlay overlay =
      iggy3d::buildProductCreativeBlockoutOverlay({&document, 1.0F});
  return expect(overlay.ok, "object overlay ok") &&
         expect(overlay.objectCount == 1U, "object count") &&
         expect(overlay.faces.size() == 6U, "object face count") &&
         expect(overlay.labels.size() == 3U, "object label count") &&
         expect(overlay.faces[0].face == iggy3d::ProductCreativeBlockoutFace::Top,
                "object top first") &&
         expectVec3(overlay.faces[0].centerMeters, {2.0F, 2.0F, 3.0F},
                    "object top center") &&
         expect(overlay.faces[0].widthMeters == 1.0F, "object width") &&
         expect(overlay.faces[0].heightMeters == 3.0F, "object depth") &&
         expect(overlay.faces[0].yawDegrees == 45.0F, "object yaw copied") &&
         expect(overlay.labels[0].label == "W 1.0m", "object width label") &&
         expect(overlay.labels[1].label == "H 2.0m", "object height label") &&
         expect(overlay.labels[2].label == "D 3.0m", "object depth label");
}

bool selectionAndHoverFlagOnlyMatchingPrimitive() {
  iggy3d::EditableRoomDocument document;
  document.floors.push_back(testFloor());
  document.walls.push_back(testWall());
  document.objects.push_back(testObject());
  iggy3d::ProductCreativeBlockoutOverlayRequest request;
  request.document = &document;
  request.gridStepMeters = 1.0F;
  request.hasSelectedPrimitive = true;
  request.selectedPrimitive = {iggy3d::ProductCreativeBlockoutPrimitiveKind::Wall,
                               "wall_001",
                               0};
  request.hasHoveredPrimitive = true;
  request.hoveredPrimitive = {iggy3d::ProductCreativeBlockoutPrimitiveKind::Object,
                              "object_001",
                              0};
  const iggy3d::ProductCreativeBlockoutOverlay overlay =
      iggy3d::buildProductCreativeBlockoutOverlay(request);
  bool floorFlagged = false;
  bool wallSelected = false;
  bool objectHovered = false;
  for (const iggy3d::ProductCreativeBlockoutFaceOverlay& face : overlay.faces) {
    if (face.primitive.kind == iggy3d::ProductCreativeBlockoutPrimitiveKind::Floor) {
      floorFlagged = floorFlagged || face.selected || face.hovered;
    }
    if (face.primitive.kind == iggy3d::ProductCreativeBlockoutPrimitiveKind::Wall) {
      wallSelected = wallSelected || face.selected;
    }
    if (face.primitive.kind == iggy3d::ProductCreativeBlockoutPrimitiveKind::Object) {
      objectHovered = objectHovered || face.hovered;
    }
  }
  return expect(overlay.selectedFaceCount == 6U, "selected wall faces") &&
         expect(overlay.hoveredFaceCount == 6U, "hovered object faces") &&
         expect(!floorFlagged, "floor not flagged") &&
         expect(wallSelected, "wall selected") &&
         expect(objectHovered, "object hovered");
}

bool deterministicAndDoesNotMutateInput() {
  iggy3d::EditableRoomDocument document;
  document.floors.push_back(testFloor());
  document.walls.push_back(testWall());
  document.objects.push_back(testObject());
  const std::string floorId = document.floors.front().id;
  const std::string wallId = document.walls.front().id;
  const std::string objectId = document.objects.front().id;
  const iggy3d::ProductCreativeBlockoutOverlay first =
      iggy3d::buildProductCreativeBlockoutOverlay({&document, 1.0F});
  const iggy3d::ProductCreativeBlockoutOverlay second =
      iggy3d::buildProductCreativeBlockoutOverlay({&document, 1.0F});
  return expect(first.faces.size() == 13U, "combined face count") &&
         expect(first.labels.size() == 8U, "combined label count") &&
         expect(first.faces.size() == second.faces.size(), "repeat face count") &&
         expect(first.labels.size() == second.labels.size(), "repeat label count") &&
         expect(first.faces[0].primitive.id == second.faces[0].primitive.id,
                "repeat first face id") &&
         expect(first.faces[1].primitive.kind ==
                    iggy3d::ProductCreativeBlockoutPrimitiveKind::Wall,
                "wall after floor") &&
         expect(first.faces[7].primitive.kind ==
                    iggy3d::ProductCreativeBlockoutPrimitiveKind::Object,
                "object after wall") &&
         expect(document.floors.front().id == floorId, "floor id not mutated") &&
         expect(document.walls.front().id == wallId, "wall id not mutated") &&
         expect(document.objects.front().id == objectId, "object id not mutated");
}

}  // namespace

int main() {
  const bool ok = namesAreStable() &&
                  missingAndInvalidRequestsFailClosed() &&
                  floorPacketContainsTopFaceAndLabels() &&
                  wallPacketContainsSixFacesAndLabels() &&
                  objectPacketContainsAxisAlignedFacesYawAndLabels() &&
                  selectionAndHoverFlagOnlyMatchingPrimitive() &&
                  deterministicAndDoesNotMutateInput();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
