#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/tools/RoomShell.hpp"
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

std::size_t countSurfaceNormal(
    std::span<const iggy3d::RoomSpatialSurface> surfaces,
    iggy3d::Vec3 normal) {
  std::size_t count = 0;
  for (const iggy3d::RoomSpatialSurface& surface : surfaces) {
    if (sameVec3(surface.normal, normal)) {
      ++count;
    }
  }
  return count;
}

std::size_t countSurfaceNormal(
    std::span<const iggy3d::CollisionSurfaceView> surfaces,
    iggy3d::Vec3 normal) {
  std::size_t count = 0;
  for (const iggy3d::CollisionSurfaceView& surface : surfaces) {
    if (sameVec3(surface.normal, normal)) {
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

cr::CreativeDocumentCreateReceipt createDefaultObject(
    cr::CreativeDocument& document,
    cr::CreativeObjectKind kind) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  return document.createObject(request);
}

cr::CreativeDocumentCreateReceipt createPoint(
    cr::CreativeDocument& document,
    cr::CreativeObjectKind kind,
    cr::CreativeVec3 position = {1.0, 0.0, 1.0},
    bool visible = true) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.transform.position = position;
  request.hasTransformOverride = true;
  request.visible = visible;
  request.hasVisibleOverride = true;
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
         expect(result.receipt.bakedAnchorCount == 0U,
                "empty baked anchor count") &&
         expect(result.room.id == "creative_test_room", "empty room id") &&
         expect(result.room.source == "iggy3d.creative_document",
                "empty room source") &&
         expect(result.room.units == "m", "empty room units");
}

bool generatedRoomShellBakesChildrenAndSkipsRoomMetadata() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Shell Bake");
  cr::CreativeDocumentCreateRequest roomRequest;
  roomRequest.kind = cr::CreativeObjectKind::Room;
  roomRequest.name = "Room";
  const cr::CreativeDocumentCreateReceipt roomReceipt =
      document.createObject(roomRequest);

  cr::CreativeRoomShellBuildRequest shellRequest;
  shellRequest.document = &document;
  shellRequest.roomObjectId = roomReceipt.objectId;
  const cr::CreativeRoomShellBuildResult shell =
      cr::buildCreativeRoomShellCreateRequests(shellRequest);
  bool shellCreated = shell.receipt.accepted;
  for (const cr::CreativeDocumentCreateRequest& createRequest :
       shell.createRequests) {
    const cr::CreativeDocumentCreateReceipt createReceipt =
        document.createObject(createRequest);
    shellCreated &= createReceipt.accepted;
  }

  const cr::CreativeRoomBakeResult result = bake(document);
  std::uint64_t floorRoleCount = 0;
  std::uint64_t wallRoleCount = 0;
  bool noRoomSource = true;
  for (const iggy3d::RoomStaticMeshAsset& mesh : result.room.staticMeshes) {
    floorRoleCount += mesh.role == "floor" ? 1U : 0U;
    wallRoleCount += mesh.role == "wall" ? 1U : 0U;
  }
  const std::uint64_t xNormalSurfaceCount =
      countSurfaceNormal(result.room.spatialSurfaces, {1.0F, 0.0F, 0.0F});
  const std::uint64_t zNormalSurfaceCount =
      countSurfaceNormal(result.room.spatialSurfaces, {0.0F, 0.0F, 1.0F});
  for (const cr::CreativeRoomBakeStaticMeshSource& source :
       result.staticMeshSources) {
    noRoomSource &= source.objectId != roomReceipt.objectId;
  }

  return expect(roomReceipt.accepted, "shell bake room created") &&
         expect(shellCreated, "shell bake requests created") &&
         expect(result.receipt.accepted, "shell bake accepted") &&
         expect(result.receipt.objectCount == 6U,
                "shell bake object count") &&
         expect(result.receipt.consideredObjectCount == 5U,
                "shell bake considered children") &&
         expect(result.receipt.skippedRoomMetadataCount == 1U,
                "shell bake skipped room metadata") &&
         expect(result.receipt.bakedStaticMeshCount == 5U,
                "shell bake mesh count") &&
         expect(result.receipt.bakedAnchorCount == 0U,
                "shell bake anchor count") &&
         expect(result.receipt.bakedSpatialSurfaceCount == 9U,
                "shell bake surface count") &&
         expect(result.room.staticMeshes.size() == 5U,
                "shell bake mesh vector") &&
         expect(result.room.spatialSurfaces.size() == 9U,
                "shell bake surface vector") &&
         expect(result.staticMeshSources.size() == 5U,
                "shell bake mesh sources") &&
         expect(result.spatialSurfaceSources.size() == 9U,
                "shell bake surface sources") &&
         expect(floorRoleCount == 1U, "shell bake floor role") &&
         expect(wallRoleCount == 4U, "shell bake wall roles") &&
         expect(xNormalSurfaceCount == 4U,
                "shell east/west wall blocker normals") &&
         expect(zNormalSurfaceCount == 4U,
                "shell north/south wall blocker normals") &&
         expect(noRoomSource, "shell bake sources are generated children");
}

bool floorWallCrateBakeToRoomAsset() {
  const cr::CreativeDocument document = buildFloorWallCrateDocument();
  const cr::CreativeRoomBakeResult result = bake(document);
  const std::vector<iggy3d::RoomStaticMeshAsset>& meshes =
      result.room.staticMeshes;
  const std::vector<cr::CreativeRoomBakeStaticMeshSource>& meshSources =
      result.staticMeshSources;
  const std::vector<cr::CreativeRoomBakeSpatialSurfaceSource>& surfaceSources =
      result.spatialSurfaceSources;

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
         expect(result.receipt.bakedAnchorCount == 0U,
                "bake anchor count") &&
         expect(result.receipt.bakedSpatialSurfaceCount == 5U,
                "bake surface count") &&
         expect(meshes.size() == 3U, "mesh vector count") &&
         expect(meshSources.size() == 3U, "mesh source count") &&
         expect(result.room.anchors.empty(), "no anchors for mesh bake") &&
         expect(result.anchorSources.empty(), "no anchor sources") &&
         expect(surfaceSources.size() == 5U, "surface source count") &&
         expect(countRole(meshes, "floor") == 1U, "floor role count") &&
         expect(countRole(meshes, "wall") == 1U, "wall role count") &&
         expect(countRole(meshes, "prop") == 1U, "prop role count") &&
         expect(meshes[0].id == "creative_object_1", "floor id") &&
         expect(meshSources[0].objectId == 1U, "floor source object") &&
         expect(meshSources[0].staticMeshId == meshes[0].id,
                "floor source mesh id") &&
         expect(meshes[0].meshId == "creative_floor_rect", "floor mesh id") &&
         expect(meshes[0].materialId == "creative_floor",
                "floor material") &&
         expect(sameVec3(meshes[0].positionMeters, {2.0F, 0.125F, 2.0F}),
                "floor center") &&
         expect(sameVec3(meshes[0].sizeMeters, {4.0F, 0.25F, 4.0F}),
                "floor size") &&
         expect(meshes[1].id == "creative_object_2", "wall id") &&
         expect(meshSources[1].objectId == 2U, "wall source object") &&
         expect(meshSources[1].staticMeshId == meshes[1].id,
                "wall source mesh id") &&
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
         expect(meshSources[2].objectId == 3U, "prop source object") &&
         expect(meshSources[2].staticMeshId == meshes[2].id,
                "prop source mesh id") &&
         expect(meshes[2].meshId == "creative_box_proxy", "prop mesh id") &&
         expect(meshes[2].materialId == "creative_prop", "prop material") &&
         expect(sameVec3(meshes[2].positionMeters, {1.5F, 0.5F, 5.5F}),
                "prop center") &&
         expect(sameVec3(meshes[2].sizeMeters, {1.0F, 1.0F, 1.0F}),
                "prop size") &&
         expect(surfaceSources[0].objectId == 1U, "floor surface object") &&
         expect(surfaceSources[0].surfaceId == "creative_object_1_walkable",
                "floor surface id") &&
         expect(surfaceSources[0].sourceStaticMeshId == "creative_object_1",
                "floor surface mesh id") &&
         expect(surfaceSources[1].objectId == 2U,
                "wall actor surface object") &&
         expect(surfaceSources[1].surfaceId ==
                    "creative_object_2_actor_blocker",
                "wall actor surface id") &&
         expect(surfaceSources[1].sourceStaticMeshId == "creative_object_2",
                "wall actor surface mesh id") &&
         expect(surfaceSources[2].objectId == 2U,
                "wall projectile surface object") &&
         expect(surfaceSources[2].surfaceId ==
                    "creative_object_2_projectile_blocker",
                "wall projectile surface id") &&
         expect(surfaceSources[2].sourceStaticMeshId == "creative_object_2",
                "wall projectile surface mesh id") &&
         expect(surfaceSources[3].objectId == 3U,
                "prop actor surface object") &&
         expect(surfaceSources[3].surfaceId ==
                    "creative_object_3_actor_blocker",
                "prop actor surface id") &&
         expect(surfaceSources[3].sourceStaticMeshId == "creative_object_3",
                "prop actor surface mesh id") &&
         expect(surfaceSources[4].objectId == 3U,
                "prop projectile surface object") &&
         expect(surfaceSources[4].surfaceId ==
                    "creative_object_3_projectile_blocker",
                "prop projectile surface id") &&
         expect(surfaceSources[4].sourceStaticMeshId == "creative_object_3",
                "prop projectile surface mesh id");
}

bool perpendicularWallsBakeTruthfulBlockerNormals() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Wall Normal Bake Test");
  const cr::CreativeDocumentCreateReceipt xWall =
      createObject(document,
                   cr::CreativeObjectKind::Wall,
                   {{0.0, 0.0, 0.0}, {4.0, 2.5, 0.25}});
  const cr::CreativeDocumentCreateReceipt zWall =
      createObject(document,
                   cr::CreativeObjectKind::Wall,
                   {{10.0, 0.0, 0.0}, {10.25, 2.5, 4.0}});

  const cr::CreativeRoomBakeResult result = bake(document);
  const iggy3d::ProductActiveRoomState active =
      iggy3d::buildProductActiveRoomFromPackageRoom(
          result.room, "iggy3d.creative", "creative.document");
  const iggy3d::ProductActiveRoomCollisionState collision =
      iggy3d::buildProductActiveRoomCollision(active);

  const iggy3d::RoomSpatialSurface* xWallActor =
      result.room.spatialSurfaces.size() > 0U ? &result.room.spatialSurfaces[0]
                                               : nullptr;
  const iggy3d::RoomSpatialSurface* xWallProjectile =
      result.room.spatialSurfaces.size() > 1U ? &result.room.spatialSurfaces[1]
                                               : nullptr;
  const iggy3d::RoomSpatialSurface* zWallActor =
      result.room.spatialSurfaces.size() > 2U ? &result.room.spatialSurfaces[2]
                                               : nullptr;
  const iggy3d::RoomSpatialSurface* zWallProjectile =
      result.room.spatialSurfaces.size() > 3U ? &result.room.spatialSurfaces[3]
                                               : nullptr;

  return expect(xWall.accepted, "x wall created") &&
         expect(zWall.accepted, "z wall created") &&
         expect(result.receipt.accepted, "wall normal bake accepted") &&
         expect(result.room.staticMeshes.size() == 2U,
                "wall normal mesh count") &&
         expect(result.room.spatialSurfaces.size() == 4U,
                "wall normal surface count") &&
         expect(xWallActor != nullptr, "x wall actor surface exists") &&
         expect(xWallActor != nullptr &&
                    sameVec3(xWallActor->normal, {0.0F, 0.0F, 1.0F}),
                "x-running wall actor normal +z") &&
         expect(xWallProjectile != nullptr,
                "x wall projectile surface exists") &&
         expect(xWallProjectile != nullptr &&
                    sameVec3(xWallProjectile->normal, {0.0F, 0.0F, 1.0F}),
                "x-running wall projectile normal +z") &&
         expect(zWallActor != nullptr, "z wall actor surface exists") &&
         expect(zWallActor != nullptr &&
                    sameVec3(zWallActor->normal, {1.0F, 0.0F, 0.0F}),
                "z-running wall actor normal +x") &&
         expect(zWallProjectile != nullptr,
                "z wall projectile surface exists") &&
         expect(zWallProjectile != nullptr &&
                    sameVec3(zWallProjectile->normal, {1.0F, 0.0F, 0.0F}),
                "z-running wall projectile normal +x") &&
         expect(collision.ready, "wall normal collision ready") &&
         expect(collision.querySurfaceCount == 4U,
                "wall normal collision query count") &&
         expect(countSurfaceNormal(collision.surfaces.surfaces(),
                                   {0.0F, 0.0F, 1.0F}) == 2U,
                "collision x-wall normals") &&
         expect(countSurfaceNormal(collision.surfaces.surfaces(),
                                   {1.0F, 0.0F, 0.0F}) == 2U,
                "collision z-wall normals");
}

bool boundsBackedLineBakesAsPropGeometry() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Line Geometry");
  const cr::CreativeDocumentCreateReceipt created =
      createObject(document,
                   cr::CreativeObjectKind::Beam,
                   {{0.0, 1.0, 0.0}, {4.0, 1.35, 0.35}});
  const cr::CreativeRoomBakeResult result = bake(document);

  const iggy3d::RoomStaticMeshAsset* mesh =
      result.room.staticMeshes.empty() ? nullptr : &result.room.staticMeshes[0];
  const iggy3d::RoomSpatialSurface* actorSurface =
      result.room.spatialSurfaces.empty() ? nullptr
                                          : &result.room.spatialSurfaces[0];
  const iggy3d::RoomSpatialSurface* projectileSurface =
      result.room.spatialSurfaces.size() < 2U
          ? nullptr
          : &result.room.spatialSurfaces[1];
  const cr::CreativeRoomBakeStaticMeshSource* meshSource =
      result.staticMeshSources.empty() ? nullptr : &result.staticMeshSources[0];
  const cr::CreativeRoomBakeSpatialSurfaceSource* actorSource =
      result.spatialSurfaceSources.empty() ? nullptr
                                           : &result.spatialSurfaceSources[0];
  const cr::CreativeRoomBakeSpatialSurfaceSource* projectileSource =
      result.spatialSurfaceSources.size() < 2U
          ? nullptr
          : &result.spatialSurfaceSources[1];

  return expect(created.accepted, "line create accepted") &&
         expect(result.receipt.accepted, "line bake accepted") &&
         expect(result.receipt.status == cr::CreativeRoomBakeStatus::Baked,
                "line bake status") &&
         expect(result.receipt.objectCount == 1U, "line object count") &&
         expect(result.receipt.consideredObjectCount == 1U,
                "line considered count") &&
         expect(result.receipt.bakedStaticMeshCount == 1U,
                "line mesh count") &&
         expect(result.receipt.bakedSpatialSurfaceCount == 2U,
                "line surface count") &&
         expect(result.receipt.bakedAnchorCount == 0U,
                "line anchor count") &&
         expect(result.room.staticMeshes.size() == 1U,
                "line mesh vector count") &&
         expect(result.staticMeshSources.size() == 1U,
                "line mesh source count") &&
         expect(result.spatialSurfaceSources.size() == 2U,
                "line surface source count") &&
         expect(result.room.anchors.empty(), "line no anchors") &&
         expect(result.anchorSources.empty(), "line no anchor sources") &&
         expect(mesh != nullptr, "line mesh exists") &&
         expect(mesh->id == "creative_object_1", "line mesh id") &&
         expect(meshSource != nullptr, "line mesh source exists") &&
         expect(meshSource->objectId == 1U, "line mesh source object") &&
         expect(meshSource->staticMeshId == mesh->id,
                "line mesh source id") &&
         expect(mesh->role == "prop", "line mesh role") &&
         expect(mesh->meshId == "creative_box_proxy", "line mesh id stable") &&
         expect(mesh->materialId == "creative_prop", "line material stable") &&
         expect(sameVec3(mesh->positionMeters, {2.0F, 1.175F, 0.175F}),
                "line center") &&
         expect(sameVec3(mesh->sizeMeters, {4.0F, 0.35F, 0.35F}),
                "line size") &&
         expect(actorSurface != nullptr, "line actor surface exists") &&
         expect(actorSource != nullptr, "line actor source exists") &&
         expect(actorSurface->sourceStaticMeshId == "creative_object_1",
                "line actor surface source") &&
         expect(actorSource->objectId == 1U, "line actor source object") &&
         expect(actorSource->surfaceId == actorSurface->id,
                "line actor source surface id") &&
         expect(actorSource->sourceStaticMeshId ==
                    actorSurface->sourceStaticMeshId,
                "line actor source mesh id") &&
         expect(actorSurface->role == iggy3d::RoomSpatialSurfaceRole::Blocker,
                "line actor surface role") &&
         expect(actorSurface->blocksActor, "line actor blocks actor") &&
         expect(!actorSurface->blocksProjectile,
                "line actor does not block projectile") &&
         expect(projectileSurface != nullptr,
                "line projectile surface exists") &&
         expect(projectileSource != nullptr,
                "line projectile source exists") &&
         expect(projectileSurface->sourceStaticMeshId == "creative_object_1",
                "line projectile surface source") &&
         expect(projectileSource->objectId == 1U,
                "line projectile source object") &&
         expect(projectileSource->surfaceId == projectileSurface->id,
                "line projectile source surface id") &&
         expect(projectileSource->sourceStaticMeshId ==
                    projectileSurface->sourceStaticMeshId,
                "line projectile source mesh id") &&
         expect(projectileSurface->role ==
                    iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker,
                "line projectile surface role") &&
         expect(!projectileSurface->blocksActor,
                "line projectile does not block actor") &&
         expect(projectileSurface->blocksProjectile,
                "line projectile blocks projectile");
}

bool endpointLineDescriptorStaysOutOfBake() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Line Link");
  const cr::CreativeDocumentCreateReceipt created =
      createDefaultObject(document, cr::CreativeObjectKind::NavLink);
  const cr::CreativeRoomBakeResult result = bake(document);

  return expect(created.accepted, "endpoint line create accepted") &&
         expect(!result.receipt.accepted, "endpoint line not accepted") &&
         expect(result.receipt.status ==
                    cr::CreativeRoomBakeStatus::NoRenderableObjects,
                "endpoint line status") &&
         expect(result.receipt.objectCount == 1U,
                "endpoint line object count") &&
         expect(result.receipt.consideredObjectCount == 1U,
                "endpoint line considered count") &&
         expect(result.receipt.skippedUnsupportedShapeCount == 1U,
                "endpoint line unsupported shape") &&
         expect(result.receipt.skippedNoBoundsCount == 0U,
                "endpoint line no bounds count") &&
         expect(result.receipt.bakedStaticMeshCount == 0U,
                "endpoint line mesh count") &&
         expect(result.receipt.bakedAnchorCount == 0U,
                "endpoint line anchor count") &&
         expect(result.room.staticMeshes.empty(), "endpoint line no meshes") &&
         expect(result.staticMeshSources.empty(),
                "endpoint line no mesh sources") &&
         expect(result.anchorSources.empty(),
                "endpoint line no anchor sources") &&
         expect(result.spatialSurfaceSources.empty(),
                "endpoint line no surface sources") &&
         expect(result.room.anchors.empty(), "endpoint line no anchors");
}

bool semanticVolumeProjectionObjectsDoNotBakeStaticGeometry() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Volumes");
  const cr::CreativeDocumentCreateReceipt water =
      createObject(document,
                   cr::CreativeObjectKind::WaterVolume,
                   {{0.0, 0.0, 0.0}, {4.0, 1.0, 4.0}});
  const cr::CreativeDocumentCreateReceipt trigger =
      createObject(document,
                   cr::CreativeObjectKind::TriggerZone,
                   {{5.0, 0.0, 0.0}, {7.0, 2.0, 2.0}});
  const cr::CreativeDocumentCreateReceipt boundary =
      createObject(document,
                   cr::CreativeObjectKind::BoundaryVolume,
                   {{-5.0, 0.0, -5.0}, {5.0, 4.0, 5.0}});
  const cr::CreativeDocumentCreateReceipt alert =
      createObject(document,
                   cr::CreativeObjectKind::AlertZone,
                   {{8.0, 0.0, 0.0}, {12.0, 2.0, 4.0}});

  const cr::CreativeRoomBakeResult result = bake(document);

  return expect(water.accepted, "water volume create accepted") &&
         expect(trigger.accepted, "trigger volume create accepted") &&
         expect(boundary.accepted, "boundary volume create accepted") &&
         expect(alert.accepted, "alert volume create accepted") &&
         expect(!result.receipt.accepted, "volumes not accepted") &&
         expect(result.receipt.status ==
                    cr::CreativeRoomBakeStatus::NoRenderableObjects,
                "volumes no-renderable status") &&
         expect(result.receipt.reasonCode ==
                    "creative_room_bake_no_renderable_objects",
                "volumes no-renderable reason") &&
         expect(result.receipt.objectCount == 4U, "volumes object count") &&
         expect(result.receipt.consideredObjectCount == 4U,
                "volumes considered count") &&
         expect(result.receipt.skippedUnsupportedShapeCount == 4U,
                "volumes unsupported shape count") &&
         expect(result.receipt.skippedNoBoundsCount == 0U,
                "volumes no-bounds count") &&
         expect(result.receipt.bakedStaticMeshCount == 0U,
                "volumes static mesh count") &&
         expect(result.receipt.bakedSpatialSurfaceCount == 0U,
                "volumes spatial surface count") &&
         expect(result.receipt.bakedAnchorCount == 0U,
                "volumes anchor count") &&
         expect(result.room.staticMeshes.empty(), "volumes no meshes") &&
         expect(result.room.spatialSurfaces.empty(), "volumes no surfaces") &&
         expect(result.room.anchors.empty(), "volumes no anchors") &&
         expect(result.staticMeshSources.empty(), "volumes no mesh sources") &&
         expect(result.spatialSurfaceSources.empty(),
                "volumes no surface sources") &&
         expect(result.anchorSources.empty(), "volumes no anchor sources");
}

bool boxProjectionTestingVolumesDoNotBakeStaticGeometry() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Testing Volumes");
  const cr::CreativeDocumentCreateReceipt testLane =
      createObject(document,
                   cr::CreativeObjectKind::TestLane,
                   {{0.0, 0.0, 0.0}, {3.0, 0.2, 12.0}});
  const cr::CreativeDocumentCreateReceipt fallShaft =
      createObject(document,
                   cr::CreativeObjectKind::FallShaft,
                   {{4.0, 0.0, 0.0}, {6.0, 8.0, 2.0}});
  const cr::CreativeDocumentCreateReceipt timingGate =
      createObject(document,
                   cr::CreativeObjectKind::TimingGate,
                   {{8.0, 0.0, 0.0}, {10.0, 2.0, 0.2}});

  const cr::CreativeRoomBakeResult result = bake(document);

  return expect(testLane.accepted, "test lane create accepted") &&
         expect(fallShaft.accepted, "fall shaft create accepted") &&
         expect(timingGate.accepted, "timing gate create accepted") &&
         expect(!result.receipt.accepted, "testing volumes not accepted") &&
         expect(result.receipt.status ==
                    cr::CreativeRoomBakeStatus::NoRenderableObjects,
                "testing volumes no-renderable status") &&
         expect(result.receipt.reasonCode ==
                    "creative_room_bake_no_renderable_objects",
                "testing volumes no-renderable reason") &&
         expect(result.receipt.objectCount == 3U,
                "testing volumes object count") &&
         expect(result.receipt.consideredObjectCount == 3U,
                "testing volumes considered count") &&
         expect(result.receipt.skippedUnsupportedShapeCount == 3U,
                "testing volumes unsupported shape count") &&
         expect(result.receipt.skippedNoBoundsCount == 0U,
                "testing volumes no-bounds count") &&
         expect(result.receipt.bakedStaticMeshCount == 0U,
                "testing volumes static mesh count") &&
         expect(result.receipt.bakedSpatialSurfaceCount == 0U,
                "testing volumes spatial surface count") &&
         expect(result.receipt.bakedAnchorCount == 0U,
                "testing volumes anchor count") &&
         expect(result.room.staticMeshes.empty(),
                "testing volumes no static meshes") &&
         expect(result.room.spatialSurfaces.empty(),
                "testing volumes no spatial surfaces") &&
         expect(result.staticMeshSources.empty(),
                "testing volumes no mesh sources") &&
         expect(result.spatialSurfaceSources.empty(),
                "testing volumes no surface sources");
}

bool pointObjectBakesToAnchorOnly() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Point Anchor");
  const cr::CreativeVec3 position = {6.25, 1.5, -2.75};
  const cr::CreativeDocumentCreateReceipt created =
      createPoint(document, cr::CreativeObjectKind::PointLight, position);
  const cr::CreativeRoomBakeResult result = bake(document);

  const iggy3d::RoomAnchorAsset* anchor =
      result.room.anchors.empty() ? nullptr : &result.room.anchors.front();
  const cr::CreativeRoomBakeAnchorSource* anchorSource =
      result.anchorSources.empty() ? nullptr : &result.anchorSources.front();
  return expect(created.accepted, "point create accepted") &&
         expect(result.receipt.accepted, "point anchor bake accepted") &&
         expect(result.receipt.status == cr::CreativeRoomBakeStatus::Baked,
                "point anchor status") &&
         expect(result.receipt.objectCount == 1U, "point object count") &&
         expect(result.receipt.consideredObjectCount == 1U,
                "point considered count") &&
         expect(result.receipt.bakedAnchorCount == 1U,
                "point baked anchor count") &&
         expect(result.receipt.bakedStaticMeshCount == 0U,
                "point no mesh count") &&
         expect(result.receipt.bakedSpatialSurfaceCount == 0U,
                "point no surface count") &&
         expect(result.room.staticMeshes.empty(), "point no meshes") &&
         expect(result.staticMeshSources.empty(), "point no mesh sources") &&
         expect(result.room.spatialSurfaces.empty(), "point no surfaces") &&
         expect(result.spatialSurfaceSources.empty(),
                "point no surface sources") &&
         expect(result.room.anchors.size() == 1U, "point anchor vector count") &&
         expect(result.anchorSources.size() == 1U,
                "point anchor source count") &&
         expect(anchor != nullptr, "point anchor exists") &&
         expect(anchorSource != nullptr, "point anchor source exists") &&
         expect(anchor->id == "creative_object_1_anchor", "point anchor id") &&
         expect(anchorSource->objectId == 1U, "point source object") &&
         expect(anchorSource->anchorId == anchor->id, "point source anchor id") &&
         expect(anchor->kind == "light", "point anchor role") &&
         expect(anchor->runtimeStableName == "creative_object_1",
                "point anchor stable name") &&
         expect(sameVec3(anchor->positionMeters, {6.25F, 1.5F, -2.75F}),
                "point anchor position");
}

bool productMeaningfulPointAnchorsUseDescriptorSemantics() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Point Anchor Semantics");
  const cr::CreativeDocumentCreateReceipt spawn =
      createPoint(document, cr::CreativeObjectKind::SpawnPoint);
  const cr::CreativeDocumentCreateReceipt exit =
      createPoint(document, cr::CreativeObjectKind::ExitPoint, {2.0, 0.0, 1.0});
  const cr::CreativeDocumentCreateReceipt npc =
      createPoint(document, cr::CreativeObjectKind::NpcSpawn, {3.0, 0.0, 1.0});
  const cr::CreativeDocumentCreateReceipt loot =
      createPoint(document, cr::CreativeObjectKind::LootPoint, {4.0, 0.0, 1.0});
  const cr::CreativeDocumentCreateReceipt enemy =
      createPoint(document, cr::CreativeObjectKind::EnemySpawn, {5.0, 0.0, 1.0});

  const cr::CreativeRoomBakeResult result = bake(document);

  return expect(spawn.accepted, "semantic spawn create") &&
         expect(exit.accepted, "semantic exit create") &&
         expect(npc.accepted, "semantic npc create") &&
         expect(loot.accepted, "semantic loot create") &&
         expect(enemy.accepted, "semantic enemy create") &&
         expect(result.receipt.accepted, "semantic point bake accepted") &&
         expect(result.receipt.bakedAnchorCount == 5U,
                "semantic point anchor count") &&
         expect(result.receipt.bakedStaticMeshCount == 0U,
                "semantic point no static meshes") &&
         expect(result.room.anchors.size() == 5U,
                "semantic point anchor vector count") &&
         expect(result.anchorSources.size() == 5U,
                "semantic point source count") &&
         expect(result.room.anchors[0].kind == "spawn",
                "spawn point anchor kind") &&
         expect(result.room.anchors[1].kind == "exit",
                "exit point anchor kind") &&
         expect(result.room.anchors[2].kind == "npc",
                "npc spawn anchor kind") &&
         expect(result.room.anchors[3].kind == "pickup",
                "loot point anchor kind") &&
         expect(result.room.anchors[4].kind == "monster",
                "enemy spawn anchor kind") &&
         expect(result.anchorSources[0].objectId == spawn.objectId,
                "spawn source object") &&
         expect(result.anchorSources[1].objectId == exit.objectId,
                "exit source object") &&
         expect(result.anchorSources[2].objectId == npc.objectId,
                "npc source object") &&
         expect(result.anchorSources[3].objectId == loot.objectId,
                "loot source object") &&
         expect(result.anchorSources[4].objectId == enemy.objectId,
                "enemy source object");
}

bool broadOccupancyPointsWithoutAnchorSemanticsDoNotBakeAnchors() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Broad Point Anchor Semantics");
  const cr::CreativeDocumentCreateReceipt entrance =
      createPoint(document, cr::CreativeObjectKind::EntrancePoint);
  const cr::CreativeDocumentCreateReceipt quest =
      createPoint(document, cr::CreativeObjectKind::QuestMarker, {2.0, 0.0, 1.0});
  const cr::CreativeDocumentCreateReceipt dialogue =
      createPoint(document,
                  cr::CreativeObjectKind::DialogueMarker,
                  {3.0, 0.0, 1.0});

  const cr::CreativeRoomBakeResult result = bake(document);

  return expect(entrance.accepted, "broad entrance create") &&
         expect(quest.accepted, "broad quest create") &&
         expect(dialogue.accepted, "broad dialogue create") &&
         expect(!result.receipt.accepted, "broad point bake not accepted") &&
         expect(result.receipt.status ==
                    cr::CreativeRoomBakeStatus::NoRenderableObjects,
                "broad point no renderable status") &&
         expect(result.receipt.skippedUnsupportedAnchorCount == 3U,
                "broad point unsupported anchor count") &&
         expect(result.receipt.bakedAnchorCount == 0U,
                "broad point no baked anchors") &&
         expect(result.room.anchors.empty(), "broad point no anchors") &&
         expect(result.anchorSources.empty(),
                "broad point no anchor sources");
}

bool hiddenPointAnchorsAreSkippedUnlessIncluded() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Hidden Point");
  (void)createPoint(document,
                    cr::CreativeObjectKind::PointLight,
                    {3.0, 2.0, 1.0},
                    false);

  const cr::CreativeRoomBakeResult skipped = bake(document);
  const cr::CreativeRoomBakeResult included = bake(document, true);

  return expect(!skipped.receipt.accepted, "hidden point skipped not accepted") &&
         expect(skipped.receipt.skippedHiddenCount == 1U,
                "hidden point skipped count") &&
         expect(skipped.receipt.bakedAnchorCount == 0U,
                "hidden point skipped anchor count") &&
         expect(skipped.room.anchors.empty(), "hidden point no anchors") &&
         expect(skipped.anchorSources.empty(),
                "hidden point no anchor sources") &&
         expect(included.receipt.accepted, "hidden point included accepted") &&
         expect(included.receipt.skippedHiddenCount == 0U,
                "hidden point included skipped count") &&
         expect(included.receipt.bakedAnchorCount == 1U,
                "hidden point included anchor count") &&
         expect(included.room.anchors.size() == 1U,
                "hidden point included anchor vector count") &&
         expect(included.anchorSources.size() == 1U,
                "hidden point included source count") &&
         expect(included.room.anchors[0].kind == "light",
                "hidden point included role");
}

bool editorOnlyPointIsSkipped() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Editor Point");
  (void)createPoint(document, cr::CreativeObjectKind::Note);

  const cr::CreativeRoomBakeResult result = bake(document);

  return expect(!result.receipt.accepted, "editor point not accepted") &&
         expect(result.receipt.status ==
                    cr::CreativeRoomBakeStatus::NoRenderableObjects,
                "editor point status") &&
         expect(result.receipt.skippedEditorOnlyCount == 1U,
                "editor point skipped count") &&
         expect(result.receipt.bakedAnchorCount == 0U,
                "editor point no anchor count") &&
         expect(result.staticMeshSources.empty(), "editor point no mesh sources") &&
         expect(result.anchorSources.empty(), "editor point no anchor sources") &&
         expect(result.spatialSurfaceSources.empty(),
                "editor point no surface sources") &&
         expect(result.room.anchors.empty(), "editor point no anchors");
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
  (void)createPoint(document, cr::CreativeObjectKind::Socket);
  (void)createDefaultObject(document, cr::CreativeObjectKind::NavLink);
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
         expect(result.receipt.skippedUnsupportedAnchorCount == 1U,
                "unsupported anchor count") &&
         expect(result.receipt.skippedUnsupportedShapeCount == 2U,
                "unsupported shape count") &&
         expect(result.receipt.skippedNoBoundsCount == 0U,
                "unsupported no bounds count") &&
         expect(result.receipt.bakedStaticMeshCount == 0U,
                "unsupported mesh count") &&
         expect(result.receipt.bakedAnchorCount == 0U,
                "unsupported anchor bake count") &&
         expect(result.staticMeshSources.empty(), "unsupported no mesh sources") &&
         expect(result.anchorSources.empty(), "unsupported no anchor sources") &&
         expect(result.spatialSurfaceSources.empty(),
                "unsupported no surface sources");
}

bool representativeBakeClassificationsRemainStable() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Bake Classification");
  const cr::CreativeDocumentCreateReceipt hidden =
      createObject(document,
                   cr::CreativeObjectKind::Floor,
                   {{-5.0, 0.0, -5.0}, {-1.0, 0.25, -1.0}},
                   false);
  const cr::CreativeDocumentCreateReceipt editorOnly =
      createPoint(document, cr::CreativeObjectKind::Note);
  const cr::CreativeDocumentCreateReceipt roomMetadata =
      createObject(document,
                   cr::CreativeObjectKind::Room,
                   {{0.0, 0.0, 0.0}, {10.0, 4.0, 10.0}});
  const cr::CreativeDocumentCreateReceipt anchor =
      createPoint(document,
                  cr::CreativeObjectKind::PointLight,
                  {1.0, 2.0, 3.0});
  const cr::CreativeDocumentCreateReceipt unsupportedPoint =
      createPoint(document, cr::CreativeObjectKind::Socket);
  const cr::CreativeDocumentCreateReceipt safeLine =
      createObject(document,
                   cr::CreativeObjectKind::Beam,
                   {{0.0, 1.0, 0.0}, {4.0, 1.35, 0.35}});
  const cr::CreativeDocumentCreateReceipt endpointLine =
      createDefaultObject(document, cr::CreativeObjectKind::NavLink);
  const cr::CreativeDocumentCreateReceipt boxVolume =
      createObject(document,
                   cr::CreativeObjectKind::WaterVolume,
                   {{0.0, 0.0, 5.0}, {4.0, 1.0, 9.0}});
  const cr::CreativeDocumentCreateReceipt floor =
      createObject(document,
                   cr::CreativeObjectKind::Floor,
                   {{0.0, 0.0, 10.0}, {4.0, 0.25, 14.0}});
  const cr::CreativeDocumentCreateReceipt wall =
      createObject(document,
                   cr::CreativeObjectKind::Wall,
                   {{5.0, 0.0, 10.0}, {9.0, 2.5, 10.25}});
  const cr::CreativeDocumentCreateReceipt prop =
      createObject(document,
                   cr::CreativeObjectKind::Crate,
                   {{1.0, 0.0, 15.0}, {2.0, 1.0, 16.0}});

  const cr::CreativeRoomBakeResult result = bake(document);
  const std::vector<iggy3d::RoomStaticMeshAsset>& meshes =
      result.room.staticMeshes;

  return expect(hidden.accepted, "classification hidden create") &&
         expect(editorOnly.accepted, "classification editor create") &&
         expect(roomMetadata.accepted, "classification room create") &&
         expect(anchor.accepted, "classification anchor create") &&
         expect(unsupportedPoint.accepted,
                "classification unsupported point create") &&
         expect(safeLine.accepted, "classification safe line create") &&
         expect(endpointLine.accepted,
                "classification endpoint line create") &&
         expect(boxVolume.accepted, "classification box volume create") &&
         expect(floor.accepted, "classification floor create") &&
         expect(wall.accepted, "classification wall create") &&
         expect(prop.accepted, "classification prop create") &&
         expect(result.receipt.accepted, "classification bake accepted") &&
         expect(result.receipt.objectCount == 11U,
                "classification object count") &&
         expect(result.receipt.consideredObjectCount == 8U,
                "classification considered count") &&
         expect(result.receipt.skippedHiddenCount == 1U,
                "classification hidden count") &&
         expect(result.receipt.skippedEditorOnlyCount == 1U,
                "classification editor-only count") &&
         expect(result.receipt.skippedRoomMetadataCount == 1U,
                "classification room metadata count") &&
         expect(result.receipt.skippedUnsupportedAnchorCount == 1U,
                "classification unsupported point count") &&
         expect(result.receipt.skippedUnsupportedShapeCount == 2U,
                "classification unsupported shape count") &&
         expect(result.receipt.skippedNoBoundsCount == 0U,
                "classification no-bounds count") &&
         expect(result.receipt.bakedAnchorCount == 1U,
                "classification anchor count") &&
         expect(result.receipt.bakedStaticMeshCount == 4U,
                "classification mesh count") &&
         expect(result.receipt.bakedSpatialSurfaceCount == 7U,
                "classification surface count") &&
         expect(result.anchorSources.size() == 1U,
                "classification anchor source count") &&
         expect(result.anchorSources[0].objectId == anchor.objectId,
                "classification anchor source object") &&
         expect(result.staticMeshSources.size() == 4U,
                "classification mesh source count") &&
         expect(result.staticMeshSources[0].objectId == safeLine.objectId,
                "classification line source") &&
         expect(result.staticMeshSources[1].objectId == floor.objectId,
                "classification floor source") &&
         expect(result.staticMeshSources[2].objectId == wall.objectId,
                "classification wall source") &&
         expect(result.staticMeshSources[3].objectId == prop.objectId,
                "classification prop source") &&
         expect(meshes.size() == 4U, "classification mesh vector count") &&
         expect(countRole(meshes, "floor") == 1U,
                "classification floor role") &&
         expect(countRole(meshes, "wall") == 1U,
                "classification wall role") &&
         expect(countRole(meshes, "prop") == 2U,
                "classification prop roles");
}

}  // namespace

int main() {
  const bool ok = nullDocumentRejects() &&
                  emptyDocumentHasNoRenderableObjects() &&
                  generatedRoomShellBakesChildrenAndSkipsRoomMetadata() &&
                  floorWallCrateBakeToRoomAsset() &&
                  perpendicularWallsBakeTruthfulBlockerNormals() &&
                  boundsBackedLineBakesAsPropGeometry() &&
                  endpointLineDescriptorStaysOutOfBake() &&
                  semanticVolumeProjectionObjectsDoNotBakeStaticGeometry() &&
                  boxProjectionTestingVolumesDoNotBakeStaticGeometry() &&
                  pointObjectBakesToAnchorOnly() &&
                  productMeaningfulPointAnchorsUseDescriptorSemantics() &&
                  broadOccupancyPointsWithoutAnchorSemanticsDoNotBakeAnchors() &&
                  bakedRoomProjectsAndLoadsIntoActiveRoom() &&
                  hiddenObjectsAreSkippedUnlessIncluded() &&
                  hiddenPointAnchorsAreSkippedUnlessIncluded() &&
                  editorOnlyPointIsSkipped() &&
                  unsupportedAndMetadataObjectsAreSkipped() &&
                  representativeBakeClassificationsRemainStable();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
