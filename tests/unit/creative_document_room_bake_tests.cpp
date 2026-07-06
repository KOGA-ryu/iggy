#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "runtime/session/SessionState.hpp"

#include <cstdlib>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool sameVec3(iggy3d::Vec3 lhs, iggy3d::Vec3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

std::size_t countRole(std::span<const iggy3d::RoomStaticMeshAsset> meshes,
                      std::string_view role) {
  std::size_t count = 0;
  for (const iggy3d::RoomStaticMeshAsset& mesh : meshes) {
    if (mesh.role == role) {
      ++count;
    }
  }
  return count;
}

std::size_t countProjectedRole(const iggy3d::SceneRoomProjection& room,
                               std::string_view role) {
  std::size_t count = 0;
  for (const iggy3d::SceneRoomMeshItem& mesh : room.meshes) {
    if (mesh.role == role) {
      ++count;
    }
  }
  return count;
}

cr::CreativeDocumentCreateReceipt createObject(
    cr::CreativeDocument& document,
    cr::CreativeObjectKind kind,
    cr::CreativeBounds bounds,
    bool visible = true) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.bounds = bounds;
  request.hasBoundsOverride = true;
  request.visible = visible;
  request.hasVisibleOverride = true;
  return document.createObject(request);
}

cr::CreativeDocumentCreateReceipt createPoint(
    cr::CreativeDocument& document,
    cr::CreativeObjectKind kind) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.transform.position = {1.0, 0.0, 1.0};
  request.hasTransformOverride = true;
  return document.createObject(request);
}

cr::CreativeDocumentCreateReceipt createPath(cr::CreativeDocument& document) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::PatrolRoute;
  request.hasPathOverride = true;
  request.pathPoints = {
      cr::CreativePathPoint{{0.0, 0.0, 0.0}},
      cr::CreativePathPoint{{2.0, 0.0, 0.0}},
      cr::CreativePathPoint{{2.0, 0.0, 2.0}},
  };
  return document.createObject(request);
}

cr::CreativeDocument buildFloorWallCrateDocument() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Bake Test");
  (void)createObject(document,
                     cr::CreativeObjectKind::Floor,
                     {{0.0, 0.0, 0.0}, {4.0, 0.25, 4.0}});
  (void)createObject(document,
                     cr::CreativeObjectKind::Wall,
                     {{5.0, 0.0, 0.0}, {9.0, 2.5, 0.25}});
  (void)createObject(document,
                     cr::CreativeObjectKind::Crate,
                     {{1.0, 0.0, 5.0}, {2.0, 1.0, 6.0}});
  return document;
}

cr::CreativeRoomBakeResult bake(const cr::CreativeDocument& document,
                                bool includeHidden = false) {
  cr::CreativeRoomBakeRequest request;
  request.document = &document;
  request.roomId = "creative_test_room";
  request.sourceName = "tests/creative_document_room_bake";
  request.sourceSubset = "unit";
  request.includeHidden = includeHidden;
  return cr::buildRoomAssetFromCreativeDocument(request);
}

bool nullDocumentRejects() {
  cr::CreativeRoomBakeRequest request;
  request.roomId = "null_room";

  const cr::CreativeRoomBakeResult result =
      cr::buildRoomAssetFromCreativeDocument(request);

  return expect(!result.receipt.accepted, "null not accepted") &&
         expect(result.receipt.requested, "null requested") &&
         expect(result.receipt.status ==
                    cr::CreativeRoomBakeStatus::MissingDocument,
                "null status") &&
         expect(result.receipt.reasonCode ==
                    "creative_room_bake_document_missing",
                "null reason") &&
         expect(result.room.staticMeshes.empty(), "null no meshes");
}

bool emptyDocumentHasNoRenderableObjects() {
  const cr::CreativeDocument document = cr::CreativeDocument::create("Empty");
  const cr::CreativeRoomBakeResult result = bake(document);

  return expect(!result.receipt.accepted, "empty not accepted") &&
         expect(result.receipt.status ==
                    cr::CreativeRoomBakeStatus::NoRenderableObjects,
                "empty status") &&
         expect(result.receipt.reasonCode ==
                    "creative_room_bake_no_renderable_objects",
                "empty reason") &&
         expect(result.receipt.objectCount == 0U, "empty object count") &&
         expect(result.receipt.bakedStaticMeshCount == 0U,
                "empty baked mesh count") &&
         expect(result.room.id == "creative_test_room", "empty room id") &&
         expect(result.room.source == "iggy3d.creative_document",
                "empty room source") &&
         expect(result.room.units == "m", "empty room units");
}

bool floorWallCrateBakeToRoomAsset() {
  const cr::CreativeDocument document = buildFloorWallCrateDocument();
  const cr::CreativeRoomBakeResult result = bake(document);
  const std::vector<iggy3d::RoomStaticMeshAsset>& meshes =
      result.room.staticMeshes;

  return expect(result.receipt.accepted, "bake accepted") &&
         expect(result.receipt.status == cr::CreativeRoomBakeStatus::Baked,
                "bake status") &&
         expect(result.receipt.reasonCode == "creative_room_baked",
                "bake reason") &&
         expect(result.receipt.objectCount == 3U, "bake object count") &&
         expect(result.receipt.consideredObjectCount == 3U,
                "bake considered count") &&
         expect(result.receipt.bakedStaticMeshCount == 3U,
                "bake mesh count") &&
         expect(result.receipt.bakedSpatialSurfaceCount == 5U,
                "bake surface count") &&
         expect(meshes.size() == 3U, "mesh vector count") &&
         expect(countRole(meshes, "floor") == 1U, "floor role count") &&
         expect(countRole(meshes, "wall") == 1U, "wall role count") &&
         expect(countRole(meshes, "prop") == 1U, "prop role count") &&
         expect(meshes[0].id == "creative_object_1", "floor id") &&
         expect(meshes[0].meshId == "creative_floor_rect", "floor mesh id") &&
         expect(meshes[0].materialId == "creative_floor",
                "floor material") &&
         expect(sameVec3(meshes[0].positionMeters, {2.0F, 0.125F, 2.0F}),
                "floor center") &&
         expect(sameVec3(meshes[0].sizeMeters, {4.0F, 0.25F, 4.0F}),
                "floor size") &&
         expect(meshes[1].id == "creative_object_2", "wall id") &&
         expect(meshes[1].meshId == "creative_wall_segment", "wall mesh id") &&
         expect(meshes[1].materialId == "creative_wall", "wall material") &&
         expect(meshes[1].hasWallSegment, "wall segment present") &&
         expect(sameVec3(meshes[1].positionMeters,
                         {7.0F, 1.25F, 0.125F}),
                "wall center") &&
         expect(sameVec3(meshes[1].sizeMeters, {4.0F, 2.5F, 0.25F}),
                "wall size") &&
         expect(sameVec3(meshes[1].wallStartMeters,
                         {5.0F, 0.0F, 0.125F}),
                "wall segment start") &&
         expect(sameVec3(meshes[1].wallEndMeters,
                         {9.0F, 0.0F, 0.125F}),
                "wall segment end") &&
         expect(meshes[1].wallBottomY == 0.0F, "wall bottom") &&
         expect(meshes[1].wallHeightMeters == 2.5F, "wall height") &&
         expect(meshes[1].wallThicknessMeters == 0.25F,
                "wall thickness") &&
         expect(meshes[2].id == "creative_object_3", "prop id") &&
         expect(meshes[2].meshId == "creative_box_proxy", "prop mesh id") &&
         expect(meshes[2].materialId == "creative_prop", "prop material") &&
         expect(sameVec3(meshes[2].positionMeters, {1.5F, 0.5F, 5.5F}),
                "prop center") &&
         expect(sameVec3(meshes[2].sizeMeters, {1.0F, 1.0F, 1.0F}),
                "prop size");
}

bool bakedRoomProjectsAndLoadsIntoActiveRoom() {
  const cr::CreativeDocument document = buildFloorWallCrateDocument();
  const cr::CreativeRoomBakeResult bakeResult = bake(document);
  iggy3d::SessionState session;
  const iggy3d::SceneProjectionResult projection =
      iggy3d::buildSceneProjection(session, &bakeResult.room);
  const iggy3d::ProductActiveRoomState active =
      iggy3d::buildProductActiveRoomFromPackageRoom(
          bakeResult.room, "iggy3d.creative", "creative.document");
  const iggy3d::ProductActiveRoomCollisionState collision =
      iggy3d::buildProductActiveRoomCollision(active);

  return expect(bakeResult.receipt.accepted, "projection bake accepted") &&
         expect(projection.room.loaded, "projection room loaded") &&
         expect(projection.room.assetId == "creative_test_room",
                "projection asset id") &&
         expect(projection.room.floorVisible, "projection floor visible") &&
         expect(projection.room.wallVisible, "projection wall visible") &&
         expect(projection.room.propVisible, "projection prop visible") &&
         expect(projection.room.meshes.size() == 3U,
                "projection mesh count") &&
         expect(countProjectedRole(projection.room, "floor") == 1U,
                "projection floor count") &&
         expect(countProjectedRole(projection.room, "wall") == 1U,
                "projection wall count") &&
         expect(countProjectedRole(projection.room, "prop") == 1U,
                "projection prop count") &&
         expect(active.loaded, "active room loaded") &&
         expect(active.status == "active_room_loaded",
                "active room status") &&
         expect(active.staticMeshCount == 3U, "active mesh count") &&
         expect(active.spatialSurfaceCount == 5U,
                "active surface count") &&
         expect(active.walkableSurfaceCount == 1U,
                "active walkable count") &&
         expect(active.actorBlockerSurfaceCount == 2U,
                "active actor blocker count") &&
         expect(active.projectileBlockerSurfaceCount == 2U,
                "active projectile blocker count") &&
         expect(collision.ready, "collision ready") &&
         expect(collision.status == "active_room_collision_ready",
                "collision status") &&
         expect(collision.spatialSurfaceCount == 5U,
                "collision source surface count") &&
         expect(collision.querySurfaceCount == 5U,
                "collision query surface count") &&
         expect(collision.surfaces.size() == 5U,
                "collision surface set count");
}

bool hiddenObjectsAreSkippedUnlessIncluded() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Hidden");
  (void)createObject(document,
                     cr::CreativeObjectKind::Floor,
                     {{0.0, 0.0, 0.0}, {4.0, 0.25, 4.0}},
                     false);

  const cr::CreativeRoomBakeResult skipped = bake(document);
  const cr::CreativeRoomBakeResult included = bake(document, true);

  return expect(!skipped.receipt.accepted, "hidden skipped not accepted") &&
         expect(skipped.receipt.skippedHiddenCount == 1U,
                "hidden skipped count") &&
         expect(skipped.receipt.bakedStaticMeshCount == 0U,
                "hidden skipped mesh count") &&
         expect(included.receipt.accepted, "hidden included accepted") &&
         expect(included.receipt.skippedHiddenCount == 0U,
                "hidden included skipped count") &&
         expect(included.receipt.bakedStaticMeshCount == 1U,
                "hidden included mesh count") &&
         expect(included.room.staticMeshes[0].role == "floor",
                "hidden included role");
}

bool unsupportedAndMetadataObjectsAreSkipped() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Skipped");
  (void)createObject(document,
                     cr::CreativeObjectKind::Room,
                     {{0.0, 0.0, 0.0}, {10.0, 4.0, 10.0}});
  (void)createPoint(document, cr::CreativeObjectKind::Note);
  (void)createPoint(document, cr::CreativeObjectKind::PointLight);
  (void)createObject(document,
                     cr::CreativeObjectKind::Beam,
                     {{0.0, 0.0, 0.0}, {4.0, 0.35, 0.35}});
  (void)createPath(document);

  const cr::CreativeRoomBakeResult result = bake(document);

  return expect(!result.receipt.accepted, "unsupported not accepted") &&
         expect(result.receipt.status ==
                    cr::CreativeRoomBakeStatus::NoRenderableObjects,
                "unsupported status") &&
         expect(result.receipt.objectCount == 5U,
                "unsupported object count") &&
         expect(result.receipt.skippedRoomMetadataCount == 1U,
                "room metadata count") &&
         expect(result.receipt.skippedEditorOnlyCount == 1U,
                "editor-only count") &&
         expect(result.receipt.skippedUnsupportedShapeCount == 3U,
                "unsupported shape count") &&
         expect(result.receipt.skippedNoBoundsCount == 0U,
                "unsupported no bounds count") &&
         expect(result.receipt.bakedStaticMeshCount == 0U,
                "unsupported mesh count");
}

}  // namespace

int main() {
  const bool ok = nullDocumentRejects() &&
                  emptyDocumentHasNoRenderableObjects() &&
                  floorWallCrateBakeToRoomAsset() &&
                  bakedRoomProjectsAndLoadsIntoActiveRoom() &&
                  hiddenObjectsAreSkippedUnlessIncluded() &&
                  unsupportedAndMetadataObjectsAreSkipped();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
