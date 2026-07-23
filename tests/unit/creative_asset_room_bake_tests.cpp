#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "runtime/player/PlayerPhysicsMovePlanner.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/collision/CollisionQuery.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"
#include "runtime/physics/PhysicsCollisionQueries.hpp"
#include "runtime/ai/SegmentOcclusion.hpp"
#include "runtime/ai/ReasoningGraph.hpp"

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool addAsset(cr::CreativeDocument& document,
              cr::CreativeObjectKind kind,
              std::string_view assetId,
              double x,
              double pitchRadians = 0.0) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::string(assetId);
  request.assetId = std::string(assetId);
  request.transform.position = {x, 0.5, 0.0};
  request.transform.rotationEulerRadians.x = pitchRadians;
  request.hasTransformOverride = true;
  request.bounds = {{x - 0.5, 0.0, -0.5}, {x + 0.5, 1.0, 0.5}};
  request.hasBoundsOverride = true;
  return document.createObject(request).accepted;
}

iggy3d::StaticMeshAssetCatalogEntry catalogEntry(
    std::string assetId,
    iggy3d::StaticMeshCollisionMode mode,
    iggy3d::StaticMeshAuthoringMetadataStatus status,
    bool walkable = false,
    iggy3d::Vec3 boundsMin = {-0.5F, -0.5F, -0.5F},
    iggy3d::Vec3 boundsMax = {0.5F, 0.5F, 0.5F},
    std::span<const iggy3d::StaticMeshCollisionPart> collisionParts = {}) {
  iggy3d::StaticMeshAssetCatalogEntry entry;
  entry.assetId = std::move(assetId);
  entry.label = entry.assetId;
  entry.boundsMin = boundsMin;
  entry.boundsMax = boundsMax;
  entry.authoringMetadata.collisionMode = mode;
  entry.authoringMetadata.status = status;
  entry.authoringMetadata.walkable = walkable;
  entry.authoringMetadata.collisionSpecified =
      status !=
      iggy3d::StaticMeshAuthoringMetadataStatus::DefaultsApplied;
  entry.authoringMetadata.walkableSpecified = walkable;
  entry.collisionParts.assign(collisionParts.begin(), collisionParts.end());
  return entry;
}

cr::CreativeDocumentCreateReceipt addAssetWithTransform(
    cr::CreativeDocument& document,
    cr::CreativeObjectKind kind,
    std::string_view assetId,
    cr::CreativeVec3 pivot,
    cr::CreativeBounds bounds,
    cr::CreativeVec3 rotation = {},
    cr::CreativeVec3 scale = {1.0, 1.0, 1.0}) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::string(assetId);
  request.assetId = std::string(assetId);
  request.transform.position = pivot;
  request.transform.rotationEulerRadians = rotation;
  request.transform.scale = scale;
  request.hasTransformOverride = true;
  request.bounds = bounds;
  request.hasBoundsOverride = true;
  return document.createObject(request);
}

cr::CreativeDocumentCreateReceipt addCatalogAsset(
    cr::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalogEntry& entry,
    double x,
    cr::CreativeObjectKind kind = cr::CreativeObjectKind::Bridge) {
  const cr::CreativeVec3 pivot{x, 0.0, 0.0};
  return addAssetWithTransform(
      document, kind, entry.assetId, pivot,
      {{pivot.x + entry.boundsMin.x, pivot.y + entry.boundsMin.y,
        pivot.z + entry.boundsMin.z},
       {pivot.x + entry.boundsMax.x, pivot.y + entry.boundsMax.y,
        pivot.z + entry.boundsMax.z}});
}

cr::CreativeDocumentCreateReceipt addGenerated(
    cr::CreativeDocument& document,
    cr::CreativeObjectKind kind,
    double x,
    cr::CreativeVec3 scale = {1.0, 1.0, 1.0},
    cr::CreativeVec3 rotation = {}) {
  const cr::CreativeVec3 size = cr::defaultCreativeObjectSize(kind);
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::string(cr::toString(kind));
  request.transform.position = {x, 0.0, 0.0};
  request.transform.rotationEulerRadians = rotation;
  request.transform.scale = scale;
  request.hasTransformOverride = true;
  request.bounds = {{x - size.x * 0.5, 0.0, -size.z * 0.5},
                    {x + size.x * 0.5, size.y, size.z * 0.5}};
  request.hasBoundsOverride = true;
  return document.createObject(request);
}

cr::CreativeRoomBakeResult bake(
    const cr::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog* catalog) {
  cr::CreativeRoomBakeRequest request;
  request.document = &document;
  request.validateReachability = false;
  request.staticMeshAssetCatalog = catalog;
  return cr::buildRoomAssetFromCreativeDocument(request);
}

std::size_t countRole(const iggy3d::RoomAsset& room,
                      iggy3d::RoomSpatialSurfaceRole role) {
  return static_cast<std::size_t>(std::count_if(
      room.spatialSurfaces.begin(), room.spatialSurfaces.end(),
      [role](const iggy3d::RoomSpatialSurface& surface) {
        return surface.role == role;
      }));
}

const iggy3d::RoomSpatialSurface* findSurface(const iggy3d::RoomAsset& room,
                                              std::string_view id) {
  const auto found =
      std::find_if(room.spatialSurfaces.begin(), room.spatialSurfaces.end(),
                   [id](const iggy3d::RoomSpatialSurface& surface) {
                     return surface.id == id;
                   });
  return found == room.spatialSurfaces.end() ? nullptr : &*found;
}

const iggy3d::RoomStaticMeshAsset* findMesh(
    const iggy3d::RoomAsset& room,
    cr::CreativeObjectId objectId) {
  const std::string id = "creative_object_" + std::to_string(objectId);
  const auto found = std::find_if(
      room.staticMeshes.begin(), room.staticMeshes.end(),
      [&id](const iggy3d::RoomStaticMeshAsset& mesh) {
        return mesh.id == id;
      });
  return found == room.staticMeshes.end() ? nullptr : &*found;
}

const iggy3d::RoomAnchorAsset* findAnchor(const iggy3d::RoomAsset& room,
                                          std::string_view id) {
  const auto found = std::find_if(
      room.anchors.begin(), room.anchors.end(),
      [id](const iggy3d::RoomAnchorAsset& anchor) { return anchor.id == id; });
  return found == room.anchors.end() ? nullptr : &*found;
}

bool hasWalkableEdgeBetweenKind(const iggy3d::ReasoningGraph& graph,
                                iggy3d::ReasoningNodeKind kind) {
  return std::any_of(
      graph.edges.begin(), graph.edges.end(),
      [&graph, kind](const iggy3d::ReasoningEdge& edge) {
        return edge.kind == iggy3d::ReasoningEdgeKind::walkable &&
               edge.from < graph.nodes.size() && edge.to < graph.nodes.size() &&
               graph.nodes[edge.from].kind == kind &&
               graph.nodes[edge.to].kind == kind;
      });
}

iggy3d::RoomSpatialSurface traversalFloorSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "traversal_floor";
  surface.sourceStaticMeshId = "traversal_floor_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {
      {-4.0F, 0.0F, -4.0F},
      {4.0F, 0.0F, -4.0F},
      {4.0F, 0.0F, 4.0F},
      {-4.0F, 0.0F, 4.0F},
  };
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {"walkable"};
  surface.collisionMask = {"actor"};
  surface.runtimeOwnerStableName = "owner.traversal_floor";
  return surface;
}

bool voxelBakePreservesMaterialIdentityAndGeometryRole() {
  cr::CreativeDocument document = cr::CreativeDocument::create("voxel materials");
  const std::array edits{
      cr::CreativeVoxelEdit{{0, 0, 0}, cr::CreativeObjectKind::Floor},
      cr::CreativeVoxelEdit{{2, 0, 0}, cr::CreativeObjectKind::Roof},
      cr::CreativeVoxelEdit{{4, 0, 0}, cr::CreativeObjectKind::Wall},
  };
  const cr::CreativeVoxelMutationReceipt applied = document.applyVoxelEdits(edits);
  const cr::CreativeRoomBakeResult result = bake(document, nullptr);
  const auto hasMaterialAndRole = [&](std::string_view material,
                                      std::string_view semanticRole,
                                      std::string_view role) {
    return std::any_of(
        result.room.staticMeshes.begin(), result.room.staticMeshes.end(),
        [&](const iggy3d::RoomStaticMeshAsset& mesh) {
          return mesh.materialId == material &&
                 mesh.semanticRole == semanticRole && mesh.role == role;
        });
  };

  return expect(applied.accepted && applied.changed &&
                    result.receipt.accepted &&
                    result.receipt.bakedVoxelCuboidCount == edits.size(),
                "voxel material bake fixture is accepted") &&
         expect(hasMaterialAndRole("creative_voxel_material_Floor", "Floor",
                                   "floor") &&
                    hasMaterialAndRole("creative_voxel_material_Roof", "Roof",
                                       "floor"),
                "same-role voxel surfaces retain material and semantic identity") &&
         expect(hasMaterialAndRole("creative_voxel_material_Wall", "Wall",
                                   "wall"),
                "voxel meaning does not replace its coarse geometry role");
}

bool fixtureMetadataProducesHonestPhysicsSurfaces() {
  const iggy3d::StaticMeshAssetCatalog catalog =
      iggy3d::discoverStaticMeshAssetCatalog("assets/creative");
  cr::CreativeDocument document = cr::CreativeDocument::create("assets");
  const bool created =
      addAsset(document, cr::CreativeObjectKind::Rock, "boulder_01", -2.0) &&
      addAsset(document, cr::CreativeObjectKind::Bridge,
               "walkway_stone_01", 2.0);
  const cr::CreativeRoomBakeResult result = bake(document, &catalog);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(result.room);
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult physics =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&surfaces, {}});

  return expect(created, "fixture asset objects created") &&
         expect(result.receipt.accepted &&
                    result.receipt.bakedStaticMeshCount == 2U,
                "both imported assets remain renderable") &&
         expect(result.receipt.bakedAssetBoundsCollisionCount == 2U &&
                    result.receipt.bakedAssetWalkableSurfaceCount == 1U &&
                    result.room.spatialSurfaces.size() == 5U,
                "fixture metadata emits two bounds and one walkable top") &&
         expect(countRole(result.room,
                          iggy3d::RoomSpatialSurfaceRole::Blocker) == 2U &&
                    countRole(result.room,
                              iggy3d::RoomSpatialSurfaceRole::
                                  ProjectileBlocker) == 2U &&
                    countRole(result.room,
                              iggy3d::RoomSpatialSurfaceRole::Walkable) == 1U,
                "surface roles match authored collision intent") &&
         expect(physics.ok && physics.colliderCount == 3U,
                "physics consumes two actor bounds and the walkable top");
}

bool compoundFixtureAssetsReachRuntimePhysics() {
  const iggy3d::StaticMeshAssetCatalog catalog =
      iggy3d::discoverStaticMeshAssetCatalog("assets/creative");
  const iggy3d::StaticMeshAssetCatalogEntry* stairs =
      catalog.find("homestead/modular/stair_straight_2x3x1p5");
  const iggy3d::StaticMeshAssetCatalogEntry* porch =
      catalog.find("homestead/modular/porch_4x2x0p5");
  const iggy3d::StaticMeshAssetCatalogEntry* bridge =
      catalog.find("homestead/modular/bridge_4x2");
  if (stairs == nullptr || porch == nullptr || bridge == nullptr) {
    return expect(false, "compound fixture catalog entries exist");
  }
  cr::CreativeDocument document =
      cr::CreativeDocument::create("compound fixtures");
  const bool created = addCatalogAsset(document, *stairs, -8.0).accepted &&
                       addCatalogAsset(document, *porch, 0.0).accepted &&
                       addCatalogAsset(document, *bridge, 8.0).accepted;
  const cr::CreativeRoomBakeResult result = bake(document, &catalog);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(result.room);
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult physics =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&surfaces, {}});

  return expect(created && result.receipt.accepted &&
                    result.receipt.bakedStaticMeshCount == 3U,
                "compound fixtures remain renderable room meshes") &&
         expect(result.receipt.bakedAssetBoundsCollisionCount == 12U &&
                    result.receipt.bakedAssetWalkableSurfaceCount == 10U &&
                    result.room.spatialSurfaces.size() == 34U,
                "fixture part counts reach RoomBake unchanged") &&
         expect(
             countRole(result.room, iggy3d::RoomSpatialSurfaceRole::Blocker) ==
                     12U &&
                 countRole(result.room,
                           iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker) ==
                     12U &&
                 countRole(result.room,
                           iggy3d::RoomSpatialSurfaceRole::Walkable) == 10U,
             "fixture surface roles preserve per-part intent") &&
         expect(physics.ok && physics.colliderCount == 22U,
                "runtime physics receives compound boxes and walkable tops");
}

bool importedStairSupportsFullBoundedRuntimeTraversal() {
  const iggy3d::StaticMeshAssetCatalog catalog =
      iggy3d::discoverStaticMeshAssetCatalog("assets/creative");
  const iggy3d::StaticMeshAssetCatalogEntry* stairs =
      catalog.find("homestead/modular/stair_straight_2x3x1p5");
  if (stairs == nullptr) {
    return expect(false, "runtime stair fixture exists");
  }

  cr::CreativeDocument document = cr::CreativeDocument::create("stair traversal");
  const cr::CreativeDocumentCreateReceipt created = addCatalogAsset(
      document, *stairs, 0.0, cr::CreativeObjectKind::Stair);
  const cr::CreativeRoomBakeResult baked = bake(document, &catalog);
  const iggy3d::RoomStaticMeshAsset* bakedMesh =
      findMesh(baked.room, created.objectId);
  iggy3d::RoomAsset traversalRoom = baked.room;
  traversalRoom.spatialSurfaces.push_back(traversalFloorSurface());
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(traversalRoom);

  iggy3d::PlayerPhysicsMovePlannerConfig config;
  config.motor.skinMeters = 0.02F;
  config.motor.groundProbeDistanceMeters = 0.10F;
  config.motor.groundSnapDistanceMeters = 0.10F;
  config.maxStepHeightMeters = 0.35F;
  iggy3d::Vec3 center{0.0F, 0.90F, 2.0F};
  bool everyStepAccepted = true;
  bool everyObstacleMatched = true;
  for (std::size_t stepIndex = 0U; stepIndex < 6U; ++stepIndex) {
    iggy3d::PlayerPhysicsMovePlannerRequest request;
    request.collisionSurfaces = &surfaces;
    request.startCenterMeters = center;
    request.bodyHalfExtentsMeters = {0.30F, 0.90F, 0.30F};
    request.desiredDisplacementMeters = {0.0F, 0.0F, -0.50F};
    request.config = config;
    const iggy3d::PlayerPhysicsMovePlannerResult planned =
        iggy3d::planPlayerPhysicsMove(request);
    const std::string expectedObstacle =
        "creative_object_" + std::to_string(created.objectId) +
        "_collision_part_" + std::to_string(stepIndex) +
        "_actor_blocker";
    everyStepAccepted = everyStepAccepted && planned.ok &&
                        planned.stepAttempted && planned.stepAccepted &&
                        std::fabs(planned.stepHeightMetersApplied - 0.25F) <=
                            0.001F &&
                        planned.grounded;
    everyObstacleMatched =
        everyObstacleMatched &&
        planned.stepObstacleSourceSurfaceId == expectedObstacle;
    center = planned.finalCenterMeters;
  }

  return expect(created.accepted && baked.receipt.accepted &&
                    bakedMesh != nullptr &&
                    bakedMesh->meshId ==
                        "asset:homestead/modular/stair_straight_2x3x1p5" &&
                    bakedMesh->proceduralSegmentCount == 0U,
                "runtime stair room bake accepted") &&
         expect(everyStepAccepted,
                "runtime stair accepts all six bounded steps") &&
         expect(everyObstacleMatched,
                "runtime stair reports each imported collision part") &&
         expect(iggy3d::nearlyEqual(center, {0.0F, 2.40F, -1.0F}),
                "runtime stair reaches the sixth authored tread");
}

bool generatedTraversalGeometryStaysInRenderCollisionParity() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("generated traversal");
  const cr::CreativeDocumentCreateReceipt platform = addGenerated(
      document, cr::CreativeObjectKind::Platform, -6.0);
  const cr::CreativeDocumentCreateReceipt ramp =
      addGenerated(document, cr::CreativeObjectKind::Ramp, 0.0);
  const cr::CreativeDocumentCreateReceipt stair =
      addGenerated(document, cr::CreativeObjectKind::Stair, 6.0);
  const cr::CreativeRoomBakeResult baked = bake(document, nullptr);
  const iggy3d::RoomStaticMeshAsset* platformMesh =
      findMesh(baked.room, platform.objectId);
  const iggy3d::RoomStaticMeshAsset* rampMesh =
      findMesh(baked.room, ramp.objectId);
  const iggy3d::RoomStaticMeshAsset* stairMesh =
      findMesh(baked.room, stair.objectId);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(baked.room);
  const iggy3d::CollisionQueryResult rampLow =
      iggy3d::sampleSurfaceHeight(surfaces, {0.0F, 0.0F, -1.0F});
  const iggy3d::CollisionQueryResult rampMiddle =
      iggy3d::sampleSurfaceHeight(surfaces, {0.0F, 0.0F, 0.0F});
  const iggy3d::CollisionQueryResult rampHigh =
      iggy3d::sampleSurfaceHeight(surfaces, {0.0F, 0.0F, 1.0F});
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult physics =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&surfaces, {}});
  const std::vector<iggy3d::Vec3> noWaypoints;
  const iggy3d::ReasoningGraph reasoning =
      iggy3d::buildReasoningGraph(baked.room, noWaypoints);
  const std::size_t stairNodeCount = static_cast<std::size_t>(std::count_if(
      reasoning.nodes.begin(), reasoning.nodes.end(),
      [](const iggy3d::ReasoningNode& node) {
        return node.kind == iggy3d::ReasoningNodeKind::stair;
      }));
  const std::size_t rampNodeCount = static_cast<std::size_t>(std::count_if(
      reasoning.nodes.begin(), reasoning.nodes.end(),
      [](const iggy3d::ReasoningNode& node) {
        return node.kind == iggy3d::ReasoningNodeKind::ramp;
      }));
  const std::string stairLowId =
      "creative_object_" + std::to_string(stair.objectId) +
      "_stair_low_anchor";
  const std::string stairHighId =
      "creative_object_" + std::to_string(stair.objectId) +
      "_stair_high_anchor";
  const std::string rampLowId =
      "creative_object_" + std::to_string(ramp.objectId) +
      "_ramp_low_anchor";
  const std::string rampHighId =
      "creative_object_" + std::to_string(ramp.objectId) +
      "_ramp_high_anchor";
  const iggy3d::RoomAnchorAsset* stairLow = findAnchor(baked.room, stairLowId);
  const iggy3d::RoomAnchorAsset* stairHigh =
      findAnchor(baked.room, stairHighId);
  const iggy3d::RoomAnchorAsset* rampLowAnchor =
      findAnchor(baked.room, rampLowId);
  const iggy3d::RoomAnchorAsset* rampHighAnchor =
      findAnchor(baked.room, rampHighId);

  return expect(platform.accepted && ramp.accepted && stair.accepted &&
                    baked.receipt.accepted,
                "generated traversal objects bake") &&
         expect(platformMesh != nullptr &&
                    platformMesh->meshId == "creative_walkable_slab" &&
                    platformMesh->semanticRole == "Platform" &&
                    platformMesh->proceduralSegmentCount == 0U &&
                    rampMesh != nullptr &&
                    rampMesh->meshId == "creative_ramp_wedge" &&
                    rampMesh->semanticRole == "Ramp" &&
                    rampMesh->proceduralSegmentCount == 0U &&
                    stairMesh != nullptr &&
                    stairMesh->meshId == "creative_stair_steps" &&
                    stairMesh->semanticRole == "Stair" &&
                    stairMesh->proceduralSegmentCount == 4U,
                "room meshes carry exact traversal meaning and generated profiles") &&
         expect(baked.room.spatialSurfaces.size() == 14U &&
                    countRole(baked.room,
                              iggy3d::RoomSpatialSurfaceRole::Walkable) == 6U &&
                    countRole(baked.room,
                              iggy3d::RoomSpatialSurfaceRole::Blocker) == 4U &&
                    countRole(
                        baked.room,
                        iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker) == 4U,
                "platform ramp and four stairs emit matching surface facts") &&
         expect(rampLow.status == iggy3d::CollisionQueryStatus::Hit &&
                    rampMiddle.status == iggy3d::CollisionQueryStatus::Hit &&
                    rampHigh.status == iggy3d::CollisionQueryStatus::Hit &&
                    std::fabs(rampLow.heightMeters - 0.25F) <= 0.001F &&
                    std::fabs(rampMiddle.heightMeters - 0.5F) <= 0.001F &&
                    std::fabs(rampHigh.heightMeters - 0.75F) <= 0.001F &&
                    rampMiddle.shape ==
                        iggy3d::CollisionSurfaceShape::HeightPatch &&
                    rampMiddle.normal.y > 0.0F && rampMiddle.normal.z < 0.0F,
                "generated ramp exposes exact sloped height and normal") &&
         expect(physics.ok && physics.colliderCount == 9U,
                "physics consumes slab and stair boxes but skips height patch") &&
         expect(baked.room.anchors.size() == 4U &&
                    baked.anchorSources.size() == 4U && stairLow != nullptr &&
                    stairHigh != nullptr && stairLow->kind == "stair" &&
                    stairHigh->kind == "stair" &&
                    rampLowAnchor != nullptr && rampHighAnchor != nullptr &&
                    rampLowAnchor->kind == "ramp" &&
                    rampHighAnchor->kind == "ramp",
                "generated traversal objects publish stable endpoint anchors") &&
         expect(stairNodeCount == 2U && rampNodeCount == 2U &&
                    hasWalkableEdgeBetweenKind(
                        reasoning, iggy3d::ReasoningNodeKind::stair) &&
                    hasWalkableEdgeBetweenKind(
                        reasoning, iggy3d::ReasoningNodeKind::ramp),
                "generated stair and ramp endpoints form traversable edges");
}

bool generatedStructuralGeometryStaysInRenderCollisionParity() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("generated structures");
  const cr::CreativeDocumentCreateReceipt column =
      addGenerated(document, cr::CreativeObjectKind::Column, -12.0);
  const cr::CreativeDocumentCreateReceipt beam =
      addGenerated(document, cr::CreativeObjectKind::Beam, -6.0);
  const cr::CreativeDocumentCreateReceipt bridge =
      addGenerated(document, cr::CreativeObjectKind::Bridge, 0.0);
  const cr::CreativeDocumentCreateReceipt arch =
      addGenerated(document, cr::CreativeObjectKind::Arch, 8.0);
  const cr::CreativeRoomBakeResult baked = bake(document, nullptr);
  const iggy3d::RoomStaticMeshAsset* columnMesh =
      findMesh(baked.room, column.objectId);
  const iggy3d::RoomStaticMeshAsset* beamMesh =
      findMesh(baked.room, beam.objectId);
  const iggy3d::RoomStaticMeshAsset* bridgeMesh =
      findMesh(baked.room, bridge.objectId);
  const iggy3d::RoomStaticMeshAsset* archMesh =
      findMesh(baked.room, arch.objectId);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(baked.room);
  const iggy3d::CollisionQueryResult bridgeTop =
      iggy3d::sampleSurfaceHeight(surfaces, {0.0F, 0.0F, 0.0F});
  const iggy3d::CollisionQueryResult archOpening = iggy3d::queryPointOverlap(
      surfaces, {8.0F, 1.0F, 0.0F}, iggy3d::CollisionQueryKind::Actor);
  const iggy3d::CollisionQueryResult archPier = iggy3d::queryPointOverlap(
      surfaces, {6.8F, 1.0F, 0.0F}, iggy3d::CollisionQueryKind::Actor);
  const iggy3d::CollisionQueryResult archLintel = iggy3d::queryPointOverlap(
      surfaces, {8.0F, 2.75F, 0.0F}, iggy3d::CollisionQueryKind::Actor);
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult physics =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&surfaces, {}});

  return expect(column.accepted && beam.accepted && bridge.accepted &&
                    arch.accepted && baked.receipt.accepted,
                "generated structural objects bake") &&
         expect(columnMesh != nullptr &&
                    columnMesh->meshId == "creative_solid_prism" &&
                    columnMesh->semanticRole == "Column" &&
                    beamMesh != nullptr &&
                    beamMesh->meshId == "creative_solid_prism" &&
                    beamMesh->semanticRole == "Beam" &&
                    bridgeMesh != nullptr &&
                    bridgeMesh->meshId == "creative_walkable_slab" &&
                    bridgeMesh->semanticRole == "Bridge" &&
                    archMesh != nullptr &&
                    archMesh->meshId == "creative_open_frame" &&
                    archMesh->semanticRole == "Arch" &&
                    archMesh->proceduralSegmentCount == 0U,
                "room meshes carry exact structural meaning and generated profiles") &&
         expect(baked.room.spatialSurfaces.size() == 11U &&
                    countRole(baked.room,
                              iggy3d::RoomSpatialSurfaceRole::Walkable) == 1U &&
                    countRole(baked.room,
                              iggy3d::RoomSpatialSurfaceRole::Blocker) == 5U &&
                    countRole(
                        baked.room,
                        iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker) == 5U,
                "solid prisms bridge and open frame emit exact surface facts") &&
         expect(bridgeTop.status == iggy3d::CollisionQueryStatus::Hit &&
                    std::fabs(bridgeTop.heightMeters - 0.35F) <= 0.001F &&
                    bridgeTop.role == iggy3d::CollisionSurfaceRole::Walkable,
                "generated bridge exposes a walkable top") &&
         expect(archOpening.status == iggy3d::CollisionQueryStatus::NoHit &&
                    archPier.status == iggy3d::CollisionQueryStatus::Hit &&
                    archLintel.status == iggy3d::CollisionQueryStatus::Hit,
                "generated arch keeps its opening clear and frame solid") &&
         expect(physics.ok && physics.colliderCount == 6U,
                "physics consumes solid prisms bridge and three arch parts");
}

bool generatedHipRoofCollisionMatchesTaperedWeatherFace() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("generated hip roof collision");
  constexpr double kHalfRun = 2.8284271247461903;
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::HipRoof;
  request.name = "Hip roof panel";
  request.transform.position = {0.0, 3.0, 0.0};
  request.transform.rotationEulerRadians.x =
      -std::numbers::pi_v<double> / 4.0;
  request.hasTransformOverride = true;
  request.bounds = {{-5.0, 2.875, -kHalfRun},
                    {5.0, 3.125, kHalfRun}};
  request.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt created =
      document.createObject(request);
  const cr::CreativeRoomBakeResult baked = bake(document, nullptr);
  const std::string surfaceId =
      "creative_object_" + std::to_string(created.objectId) +
      "_hip_roof_walkable";
  const iggy3d::RoomSpatialSurface* surface =
      findSurface(baked.room, surfaceId);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(baked.room);
  const iggy3d::CollisionQueryResult ridgeHit = iggy3d::querySegment(
      surfaces, {0.0F, 7.0F, 1.8F}, {0.0F, 0.0F, 1.8F},
      iggy3d::CollisionQueryKind::Actor);
  const iggy3d::CollisionQueryResult oldEnvelopeMiss = iggy3d::querySegment(
      surfaces, {4.0F, 7.0F, 1.8F}, {4.0F, 0.0F, 1.8F},
      iggy3d::CollisionQueryKind::Actor);
  const iggy3d::CollisionQueryResult sampledRidge =
      iggy3d::sampleSurfaceHeight(surfaces, {0.0F, 0.0F, 1.8F});
  const iggy3d::CollisionQueryResult sampledOutside =
      iggy3d::sampleSurfaceHeight(surfaces, {4.0F, 0.0F, 1.8F});
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult physics =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&surfaces, {}});

  return expect(created.accepted && baked.receipt.accepted &&
                    surface != nullptr &&
                    surface->shape ==
                        iggy3d::RoomSpatialSurfaceShape::HeightPatch &&
                    surface->role ==
                        iggy3d::RoomSpatialSurfaceRole::Walkable &&
                    surface->pointsMeters.size() == 5U,
                "hip roof bakes one exact walkable height patch") &&
         expect(std::fabs(surface->pointsMeters[1].x + 5.0F) <= 0.001F &&
                    std::fabs(surface->pointsMeters[2].x - 5.0F) <= 0.001F &&
                    std::fabs(surface->pointsMeters[3].x - 1.0F) <= 0.001F &&
                    std::fabs(surface->pointsMeters[4].x + 1.0F) <= 0.001F,
                "hip collision narrows from eave width to canonical ridge") &&
         expect(ridgeHit.status == iggy3d::CollisionQueryStatus::Hit &&
                    sampledRidge.status ==
                        iggy3d::CollisionQueryStatus::Hit &&
                    ridgeHit.shape ==
                        iggy3d::CollisionSurfaceShape::HeightPatch &&
                    ridgeHit.normal.y > 0.0F,
                "hip weather face supports exact segment and height queries") &&
         expect(oldEnvelopeMiss.status ==
                        iggy3d::CollisionQueryStatus::NoHit &&
                    sampledOutside.status ==
                        iggy3d::CollisionQueryStatus::NoHit,
                "hip collision rejects empty space inside the old box envelope") &&
         expect(physics.ok && physics.surfaceCount == 1U &&
                    physics.skippedSurfaceCount == 1U &&
                    physics.colliderCount == 0U,
                "hip height patch never regresses into a phantom AABB");
}

bool generatedTraversalTransformsFailClosedAndStayBounded() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("generated traversal transforms");
  const cr::CreativeDocumentCreateReceipt stair = addGenerated(
      document, cr::CreativeObjectKind::Stair, 0.0, {1.0, 2.0, 1.0});
  const cr::CreativeDocumentCreateReceipt ramp = addGenerated(
      document, cr::CreativeObjectKind::Ramp, 6.0, {1.0, 1.0, 1.0},
      {0.2, 0.0, 0.0});
  const cr::CreativeDocumentCreateReceipt platform = addGenerated(
      document, cr::CreativeObjectKind::Platform, -6.0, {1.0, 1.0, 1.0},
      {0.2, 0.0, 0.0});
  const cr::CreativeRoomBakeResult baked = bake(document, nullptr);
  const iggy3d::RoomStaticMeshAsset* stairMesh =
      findMesh(baked.room, stair.objectId);

  return expect(stair.accepted && ramp.accepted && platform.accepted &&
                    baked.receipt.accepted && stairMesh != nullptr &&
                    stairMesh->proceduralSegmentCount == 8U,
                "scaled generated stair resolves eight bounded steps") &&
         expect(baked.room.spatialSurfaces.size() == 28U &&
                    countRole(baked.room,
                              iggy3d::RoomSpatialSurfaceRole::Walkable) == 8U &&
                    countRole(baked.room,
                              iggy3d::RoomSpatialSurfaceRole::Blocker) == 10U &&
                    countRole(
                        baked.room,
                        iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker) == 10U,
                "tilted slab and ramp retain conservative blockers only");
}

bool renderOnlyAndUnsafeMetadataStayVisibleWithoutPhysics() {
  iggy3d::StaticMeshAssetCatalog catalog;
  catalog.entries.push_back(catalogEntry(
      "decor", iggy3d::StaticMeshCollisionMode::None,
      iggy3d::StaticMeshAuthoringMetadataStatus::Authored));
  catalog.entries.push_back(catalogEntry(
      "invalid", iggy3d::StaticMeshCollisionMode::Invalid,
      iggy3d::StaticMeshAuthoringMetadataStatus::Invalid));
  catalog.entries.push_back(catalogEntry(
      "unsupported", iggy3d::StaticMeshCollisionMode::Convex,
      iggy3d::StaticMeshAuthoringMetadataStatus::UnsupportedCollision));

  cr::CreativeDocument document = cr::CreativeDocument::create("fail closed");
  const bool created = addAsset(document, cr::CreativeObjectKind::Prop,
                                "decor", -3.0) &&
                       addAsset(document, cr::CreativeObjectKind::Prop,
                                "invalid", -1.0) &&
                       addAsset(document, cr::CreativeObjectKind::Prop,
                                "missing", 1.0) &&
                       addAsset(document, cr::CreativeObjectKind::Prop,
                                "unsupported", 3.0);
  const cr::CreativeRoomBakeResult result = bake(document, &catalog);
  const auto missingObject = std::find_if(
      document.objects().begin(), document.objects().end(),
      [](const cr::CreativeObject& object) {
        return object.assetId == "missing";
      });
  const iggy3d::RoomStaticMeshAsset* missingMesh =
      missingObject == document.objects().end()
          ? nullptr
          : findMesh(result.room, missingObject->id);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(result.room);
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult physics =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&surfaces, {}});

  return expect(created, "fail-closed asset objects created") &&
         expect(result.receipt.accepted &&
                    result.receipt.bakedStaticMeshCount == 4U &&
                    result.room.staticMeshes.size() == 4U,
                "unsafe physics metadata never hides render geometry") &&
         expect(result.room.spatialSurfaces.empty() && physics.ok &&
                    physics.colliderCount == 0U,
                "unsafe physics metadata emits no collision") &&
         expect(missingMesh != nullptr &&
                    missingMesh->meshId == "creative_box_proxy" &&
                    missingMesh->proceduralSegmentCount == 0U,
                "missing asset renders its descriptor-backed procedural fallback") &&
         expect(result.receipt.skippedAssetNoCollisionCount == 1U &&
                    result.receipt.skippedInvalidAssetMetadataCount == 1U &&
                    result.receipt.skippedMissingAssetMetadataCount == 1U &&
                    result.receipt.skippedUnsupportedAssetCollisionCount == 1U,
                "receipt identifies every no-physics reason");
}

bool defaultMetadataUsesBoundsButNeverInventsWalkability() {
  iggy3d::StaticMeshAssetCatalog catalog;
  catalog.entries.push_back(catalogEntry(
      "legacy", iggy3d::StaticMeshCollisionMode::Bounds,
      iggy3d::StaticMeshAuthoringMetadataStatus::DefaultsApplied));
  cr::CreativeDocument document = cr::CreativeDocument::create("defaults");
  const bool created = addAsset(document, cr::CreativeObjectKind::Prop,
                                "legacy", 0.0);
  const cr::CreativeRoomBakeResult result = bake(document, &catalog);

  return expect(created && result.receipt.accepted,
                "default metadata bake accepted") &&
         expect(result.receipt.bakedAssetBoundsCollisionCount == 1U &&
                    result.receipt.bakedAssetWalkableSurfaceCount == 0U &&
                    countRole(result.room,
                              iggy3d::RoomSpatialSurfaceRole::Blocker) == 1U &&
                    countRole(result.room,
                              iggy3d::RoomSpatialSurfaceRole::Walkable) == 0U,
                "legacy defaults are solid but not fabricated walkable");
}

bool tiltedWalkableAssetDoesNotFabricateAHorizontalTop() {
  iggy3d::StaticMeshAssetCatalog catalog;
  catalog.entries.push_back(catalogEntry(
      "tilted_walkway", iggy3d::StaticMeshCollisionMode::Bounds,
      iggy3d::StaticMeshAuthoringMetadataStatus::Authored, true));
  cr::CreativeDocument document = cr::CreativeDocument::create("tilted");
  const bool created = addAsset(document, cr::CreativeObjectKind::Bridge,
                                "tilted_walkway", 0.0, 0.25);
  const cr::CreativeRoomBakeResult result = bake(document, &catalog);

  return expect(created && result.receipt.accepted,
                "tilted walkable asset bake accepted") &&
         expect(result.receipt.bakedAssetBoundsCollisionCount == 1U &&
                    result.receipt.bakedAssetWalkableSurfaceCount == 0U &&
                    result.receipt.skippedAssetWalkableTransformCount == 1U &&
                    result.room.spatialSurfaces.size() == 2U,
                "tilted asset keeps bounds but skips false walkable top");
}

bool compoundBoundsMapResizeAndEmitIndependentWalkableTops() {
  constexpr std::array parts{
      iggy3d::StaticMeshCollisionPart{
          {-1.0F, 0.0F, -1.0F}, {1.0F, 0.25F, 0.0F}, true},
      iggy3d::StaticMeshCollisionPart{
          {-1.0F, 0.0F, 0.0F}, {1.0F, 0.5F, 1.0F}, true},
  };
  iggy3d::StaticMeshAssetCatalog catalog;
  catalog.entries.push_back(catalogEntry(
      "compound_steps", iggy3d::StaticMeshCollisionMode::CompoundBounds,
      iggy3d::StaticMeshAuthoringMetadataStatus::Authored, true,
      {-1.0F, 0.0F, -1.0F}, {1.0F, 1.0F, 1.0F}, parts));
  cr::CreativeDocument document = cr::CreativeDocument::create("compound");
  const cr::CreativeDocumentCreateReceipt created = addAssetWithTransform(
      document, cr::CreativeObjectKind::Bridge, "compound_steps",
      {10.0, 0.0, 4.0}, {{8.0, 0.0, 3.0}, {12.0, 2.0, 5.0}});
  const cr::CreativeRoomBakeResult result = bake(document, &catalog);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(result.room);
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult physics =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&surfaces, {}});
  const std::string stable =
      "creative_object_" + std::to_string(created.objectId);
  const iggy3d::RoomSpatialSurface* lower =
      findSurface(result.room, stable + "_collision_part_0_actor_blocker");
  const iggy3d::RoomSpatialSurface* upper =
      findSurface(result.room, stable + "_collision_part_1_actor_blocker");

  return expect(created.accepted && result.receipt.accepted,
                "compound asset bake accepted") &&
         expect(result.receipt.bakedAssetBoundsCollisionCount == 2U &&
                    result.receipt.bakedAssetWalkableSurfaceCount == 2U &&
                    result.room.spatialSurfaces.size() == 6U,
                "each compound part emits blockers and a walkable top") &&
         expect(lower != nullptr && upper != nullptr &&
                    lower->pointsMeters.size() == 4U &&
                    upper->pointsMeters.size() == 4U &&
                    iggy3d::nearlyEqual(lower->pointsMeters[0],
                                        {8.0F, 0.0F, 3.0F}) &&
                    iggy3d::nearlyEqual(lower->pointsMeters[2],
                                        {12.0F, 0.5F, 4.0F}) &&
                    iggy3d::nearlyEqual(upper->pointsMeters[0],
                                        {8.0F, 0.0F, 4.0F}) &&
                    iggy3d::nearlyEqual(upper->pointsMeters[2],
                                        {12.0F, 1.0F, 5.0F}),
                "source parts map through custom authored bounds") &&
         expect(physics.ok && physics.colliderCount == 4U,
                "physics consumes both boxes and both walkable tops");
}

bool compoundBoundsFollowNonuniformScaleAndYaw() {
  constexpr std::array parts{
      iggy3d::StaticMeshCollisionPart{
          {-1.0F, 0.0F, -1.0F}, {0.0F, 1.0F, 1.0F}, false},
  };
  iggy3d::StaticMeshAssetCatalog catalog;
  catalog.entries.push_back(catalogEntry(
      "compound_rotated", iggy3d::StaticMeshCollisionMode::CompoundBounds,
      iggy3d::StaticMeshAuthoringMetadataStatus::Authored, false,
      {-1.0F, 0.0F, -1.0F}, {1.0F, 1.0F, 1.0F}, parts));
  cr::CreativeDocument document = cr::CreativeDocument::create("transform");
  const cr::CreativeDocumentCreateReceipt created = addAssetWithTransform(
      document, cr::CreativeObjectKind::Bridge, "compound_rotated",
      {2.0, 0.0, 3.0}, {{1.0, 0.0, 2.0}, {3.0, 1.0, 4.0}},
      {0.0, std::numbers::pi * 0.5, 0.0}, {2.0, 1.0, 1.0});
  const cr::CreativeRoomBakeResult result = bake(document, &catalog);
  const std::string surfaceId = "creative_object_" +
                                std::to_string(created.objectId) +
                                "_collision_part_0_actor_blocker";
  const iggy3d::RoomSpatialSurface* surface =
      findSurface(result.room, surfaceId);

  return expect(created.accepted && result.receipt.accepted &&
                    result.receipt.bakedAssetBoundsCollisionCount == 1U,
                "scaled and rotated compound asset bakes") &&
         expect(surface != nullptr && surface->pointsMeters.size() == 4U &&
                    iggy3d::nearlyEqual(surface->pointsMeters[0],
                                        {1.0F, 0.0F, 3.0F}) &&
                    iggy3d::nearlyEqual(surface->pointsMeters[2],
                                        {3.0F, 1.0F, 5.0F}),
                "compound AABB follows nonuniform scale and quarter-turn yaw");
}

bool invalidAndTiltedCompoundContractsFailClosed() {
  constexpr std::array walkableParts{
      iggy3d::StaticMeshCollisionPart{
          {-1.0F, 0.0F, -1.0F}, {1.0F, 0.25F, 0.0F}, true},
      iggy3d::StaticMeshCollisionPart{
          {-1.0F, 0.0F, 0.0F}, {1.0F, 0.5F, 1.0F}, true},
  };
  constexpr std::array outsidePart{
      iggy3d::StaticMeshCollisionPart{
          {-2.0F, 0.0F, -1.0F}, {1.0F, 0.5F, 1.0F}, true},
  };
  std::array<iggy3d::StaticMeshCollisionPart,
             iggy3d::kMaxStaticMeshCollisionPartCount + 1U>
      excessiveParts;
  excessiveParts.fill(walkableParts.front());
  iggy3d::StaticMeshAssetCatalog invalidCatalog;
  invalidCatalog.entries.push_back(catalogEntry(
      "empty_compound", iggy3d::StaticMeshCollisionMode::CompoundBounds,
      iggy3d::StaticMeshAuthoringMetadataStatus::Authored));
  invalidCatalog.entries.push_back(catalogEntry(
      "excessive_compound", iggy3d::StaticMeshCollisionMode::CompoundBounds,
      iggy3d::StaticMeshAuthoringMetadataStatus::Authored, true,
      {-1.0F, 0.0F, -1.0F}, {1.0F, 1.0F, 1.0F}, excessiveParts));
  invalidCatalog.entries.push_back(catalogEntry(
      "outside_compound", iggy3d::StaticMeshCollisionMode::CompoundBounds,
      iggy3d::StaticMeshAuthoringMetadataStatus::Authored, true,
      {-1.0F, 0.0F, -1.0F}, {1.0F, 1.0F, 1.0F}, outsidePart));
  cr::CreativeDocument invalidDocument =
      cr::CreativeDocument::create("invalid compound");
  const bool invalidCreated =
      addAsset(invalidDocument, cr::CreativeObjectKind::Bridge,
               "empty_compound", -2.0) &&
      addAsset(invalidDocument, cr::CreativeObjectKind::Bridge,
               "outside_compound", 0.0) &&
      addAsset(invalidDocument, cr::CreativeObjectKind::Bridge,
               "excessive_compound", 2.0);
  const cr::CreativeRoomBakeResult invalidResult =
      bake(invalidDocument, &invalidCatalog);

  iggy3d::StaticMeshAssetCatalog tiltedCatalog;
  tiltedCatalog.entries.push_back(catalogEntry(
      "tilted_compound", iggy3d::StaticMeshCollisionMode::CompoundBounds,
      iggy3d::StaticMeshAuthoringMetadataStatus::Authored, true,
      {-1.0F, 0.0F, -1.0F}, {1.0F, 1.0F, 1.0F}, walkableParts));
  cr::CreativeDocument tiltedDocument =
      cr::CreativeDocument::create("tilted compound");
  const cr::CreativeDocumentCreateReceipt tiltedCreated = addAssetWithTransform(
      tiltedDocument, cr::CreativeObjectKind::Bridge, "tilted_compound",
      {0.0, 0.0, 0.0}, {{-1.0, 0.0, -1.0}, {1.0, 1.0, 1.0}}, {0.25, 0.0, 0.0});
  const cr::CreativeRoomBakeResult tiltedResult =
      bake(tiltedDocument, &tiltedCatalog);

  return expect(invalidCreated, "invalid compound fixtures create") &&
         expect(invalidResult.receipt.accepted,
                "invalid compound fixture room bake accepted") &&
         expect(invalidResult.room.spatialSurfaces.empty(),
                "invalid compound fixtures emit no physics") &&
         expect(invalidResult.receipt.skippedInvalidAssetMetadataCount == 3U,
                "every invalid compound contract is counted") &&
         expect(tiltedCreated.accepted && tiltedResult.receipt.accepted &&
                    tiltedResult.receipt.bakedAssetBoundsCollisionCount == 2U &&
                    tiltedResult.receipt.bakedAssetWalkableSurfaceCount == 0U &&
                    tiltedResult.receipt.skippedAssetWalkableTransformCount ==
                        1U &&
                    tiltedResult.room.spatialSurfaces.size() == 4U,
                "tilted compound keeps blockers but fabricates no flat tops");
}

bool hiddenHierarchyIsExcludedUnlessExplicitlyRequested() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("hidden hierarchy");
  cr::CreativeDocumentCreateRequest parentRequest;
  parentRequest.kind = cr::CreativeObjectKind::Group;
  parentRequest.name = "Hidden Assembly";
  parentRequest.visible = false;
  parentRequest.hasVisibleOverride = true;
  const cr::CreativeDocumentCreateReceipt parent =
      document.createObject(parentRequest);

  cr::CreativeDocumentCreateRequest childRequest;
  childRequest.kind = cr::CreativeObjectKind::Crate;
  childRequest.name = "Locally Visible Member";
  childRequest.parentId = parent.objectId;
  const cr::CreativeDocumentCreateReceipt child =
      document.createObject(childRequest);
  const cr::CreativeRoomBakeResult hidden = bake(document, nullptr);

  cr::CreativeRoomBakeRequest includeHiddenRequest;
  includeHiddenRequest.document = &document;
  includeHiddenRequest.validateReachability = false;
  includeHiddenRequest.includeHidden = true;
  const cr::CreativeRoomBakeResult included =
      cr::buildRoomAssetFromCreativeDocument(includeHiddenRequest);

  return expect(parent.accepted && child.accepted,
                "hidden room-bake hierarchy fixture created") &&
         expect(!hidden.receipt.accepted &&
                    hidden.receipt.status ==
                        cr::CreativeRoomBakeStatus::NoRenderableObjects &&
                    hidden.receipt.skippedHiddenCount == 2U &&
                    hidden.room.staticMeshes.empty(),
                "hidden parent excludes every descendant from normal bake") &&
         expect(included.receipt.accepted &&
                    findMesh(included.room, child.objectId) != nullptr,
                "includeHidden explicitly admits the locally visible child");
}

bool windowTreatmentSeparatesCollisionFromVision() {
  cr::CreativeDocument document = cr::CreativeDocument::create("windows");
  const auto addWindow = [&](double centerX,
                             cr::CreativeWindowInsertKind treatment) {
    cr::CreativeDocumentCreateRequest request;
    request.kind = cr::CreativeObjectKind::Window;
    request.name = treatment == cr::CreativeWindowInsertKind::Glazing
                       ? "Glazing"
                       : "Shutters";
    request.bounds = {{centerX - 0.5, 1.0, -0.05},
                      {centerX + 0.5, 2.0, 0.05}};
    request.hasBoundsOverride = true;
    request.transform.position = {centerX, 1.5, 0.0};
    request.hasTransformOverride = true;
    request.window.insertKind = treatment;
    request.hasWindowSettingsOverride = true;
    return document.createObject(request);
  };
  const cr::CreativeDocumentCreateReceipt glazing =
      addWindow(0.0, cr::CreativeWindowInsertKind::Glazing);
  const cr::CreativeDocumentCreateReceipt shutters =
      addWindow(3.0, cr::CreativeWindowInsertKind::PairedShutters);
  cr::CreativeDocumentCreateRequest missingAssetRequest;
  missingAssetRequest.kind = cr::CreativeObjectKind::Window;
  missingAssetRequest.name = "Missing Asset Shutters";
  missingAssetRequest.assetId = "missing/window_assembly";
  missingAssetRequest.bounds = {{5.5, 1.0, -0.05}, {6.5, 2.0, 0.05}};
  missingAssetRequest.hasBoundsOverride = true;
  missingAssetRequest.transform.position = {6.0, 1.5, 0.0};
  missingAssetRequest.hasTransformOverride = true;
  missingAssetRequest.window.insertKind =
      cr::CreativeWindowInsertKind::PairedShutters;
  missingAssetRequest.hasWindowSettingsOverride = true;
  const cr::CreativeDocumentCreateReceipt missingAsset =
      document.createObject(missingAssetRequest);
  const cr::CreativeRoomBakeResult result = bake(document, nullptr);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(result.room);
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult physics =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&surfaces, {}});
  const std::string glazingSurfaceId =
      "creative_object_" + std::to_string(glazing.objectId) +
      "_actor_blocker";
  const std::string shutterSurfaceId =
      "creative_object_" + std::to_string(shutters.objectId) +
      "_actor_blocker";
  const iggy3d::RoomSpatialSurface* glazingSurface =
      findSurface(result.room, glazingSurfaceId);
  const iggy3d::RoomSpatialSurface* shutterSurface =
      findSurface(result.room, shutterSurfaceId);
  const iggy3d::RoomStaticMeshAsset* glazingMesh =
      findMesh(result.room, glazing.objectId);
  const iggy3d::RoomStaticMeshAsset* shutterMesh =
      findMesh(result.room, shutters.objectId);
  const iggy3d::RoomStaticMeshAsset* missingAssetMesh =
      findMesh(result.room, missingAsset.objectId);
  bool glazingStartsInside = false;
  const bool glazingPhysicallyHit = iggy3d::segmentHitsAnyPhysicsAabb(
      physics.colliders, {0.0F, 1.5F, -1.0F}, {0.0F, 1.5F, 1.0F}, 0.0F,
      &glazingStartsInside);
  const iggy3d::SegmentOcclusionVerdict glazingVision =
      iggy3d::segmentOcclusion(physics.colliders, {0.0F, 1.5F, -1.0F},
                               {0.0F, 1.5F, 1.0F}, 0.0F);
  const iggy3d::SegmentOcclusionVerdict shutterVision =
      iggy3d::segmentOcclusion(physics.colliders, {3.0F, 1.5F, -1.0F},
                               {3.0F, 1.5F, 1.0F}, 0.0F);
  const iggy3d::SegmentOcclusionVerdict missingAssetVision =
      iggy3d::segmentOcclusion(physics.colliders, {6.0F, 1.5F, -1.0F},
                               {6.0F, 1.5F, 1.0F}, 0.0F);

  return expect(glazing.accepted && shutters.accepted && missingAsset.accepted &&
                    result.receipt.accepted && physics.ok,
                "window treatments reach room and physics bake") &&
         expect(glazingSurface != nullptr &&
                    glazingSurface->blocksActor &&
                    !glazingSurface->blocksVision,
                "glazing blocks movement without claiming opacity") &&
         expect(shutterSurface != nullptr && shutterSurface->blocksActor &&
                    shutterSurface->blocksVision,
                "shutters block movement and vision") &&
         expect(glazingMesh != nullptr && shutterMesh != nullptr &&
                    glazingMesh->materialId == "creative_window_glass" &&
                    shutterMesh->materialId == "creative_window_shutter" &&
                    glazingMesh->semanticRole == "Window" &&
                    shutterMesh->semanticRole == "Window",
                "window treatment owns distinct render material semantics") &&
         expect(glazingPhysicallyHit && !glazingStartsInside &&
                    glazingVision == iggy3d::SegmentOcclusionVerdict::Clear,
                "glazing physical collider remains transparent to sight") &&
         expect(shutterVision ==
                    iggy3d::SegmentOcclusionVerdict::Blocked,
                "closed shutters occlude sight") &&
         expect(missingAssetMesh != nullptr &&
                    missingAssetMesh->meshId == "creative_box_proxy" &&
                    result.receipt.skippedMissingAssetMetadataCount == 1U &&
                    missingAssetVision ==
                        iggy3d::SegmentOcclusionVerdict::Blocked,
                "missing shutter asset keeps a visible and semantic fallback");
}

bool authoredTerrainHardEdgeAddsOnlyTheInternalCliffBlocker() {
  cr::CreativeTerrainHeightField field;
  const std::array<std::uint16_t, 2U> heights{2U, 6U};
  const cr::CreativeTerrainHeightFieldReplaceReceipt replaced =
      field.replace({{0, 0}, 2U, 1U}, heights);
  cr::CreativeTerrainField legacy;
  const cr::CreativeTerrainSurfacePlan smoothSurface =
      cr::buildCreativeComposedTerrainSurfacePlan(legacy, field);
  const std::array hardEdges{cr::canonicalCreativeTerrainHardEdge({0, 0},
                                                                  {1, 0})};
  const cr::CreativeTerrainSurfacePlan hardSurface =
      cr::buildCreativeComposedTerrainSurfacePlan(legacy, field, hardEdges);
  const cr::CreativeTerrainRenderPlan smoothRender =
      cr::buildCreativeTerrainRenderPlan(smoothSurface, {}, 1.0);
  const cr::CreativeTerrainRenderPlan hardRender =
      cr::buildCreativeTerrainRenderPlan(hardSurface, {}, 1.0);

  cr::CreativeDocument document = cr::CreativeDocument::create("hard edge bake");
  const auto bakePatches = [&](const cr::CreativeTerrainRenderPlan& render) {
    cr::CreativeRoomBakeRequest request;
    request.document = &document;
    request.validateReachability = false;
    request.usePrecomputedVoxelCuboids = true;
    request.precomputedVoxelCuboids = smoothSurface.cuboids;
    request.usePrecomputedTerrainSurfacePatches = true;
    request.precomputedTerrainSurfacePatches = render.patches;
    return cr::buildRoomAssetFromCreativeDocument(request);
  };
  const cr::CreativeRoomBakeResult smooth = bakePatches(smoothRender);
  const cr::CreativeRoomBakeResult hard = bakePatches(hardRender);
  const iggy3d::RoomSpatialSurface* hardActor = findSurface(
      hard.room,
      "creative_terrain_1_0_hard_edge_west_actor_blocker");
  const iggy3d::RoomSpatialSurface* hardProjectile = findSurface(
      hard.room,
      "creative_terrain_1_0_hard_edge_west_projectile_blocker");
  const iggy3d::SpatialSurfaceSet smoothSurfaces =
      iggy3d::buildSpatialSurfaceSet(smooth.room);
  const iggy3d::SpatialSurfaceSet hardSurfaces =
      iggy3d::buildSpatialSurfaceSet(hard.room);
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult smoothPhysics =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces(
          {&smoothSurfaces, {}});
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult hardPhysics =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces(
          {&hardSurfaces, {}});
  const bool smoothHit = iggy3d::segmentHitsAnyPhysicsAabb(
      smoothPhysics.colliders, {0.5F, 4.0F, 0.5F},
      {1.5F, 4.0F, 0.5F}, 0.0F, nullptr);
  const bool hardHit = iggy3d::segmentHitsAnyPhysicsAabb(
      hardPhysics.colliders, {0.5F, 4.0F, 0.5F},
      {1.5F, 4.0F, 0.5F}, 0.0F, nullptr);
  const std::array<iggy3d::Vec3, 2U> waypoints{
      iggy3d::Vec3{0.5F, 2.0F, 0.5F},
      iggy3d::Vec3{1.5F, 6.0F, 0.5F},
  };
  const iggy3d::ReasoningGraph smoothReasoning =
      iggy3d::buildReasoningGraph(smooth.room, waypoints);
  const iggy3d::ReasoningGraph hardReasoning =
      iggy3d::buildReasoningGraph(hard.room, waypoints);

  return expect(replaced.accepted && smoothRender.accepted &&
                    hardRender.accepted && smooth.receipt.accepted &&
                    hard.receipt.accepted,
                "smooth and hard-edge terrain fixtures bake") &&
         expect(smooth.receipt.bakedTerrainSurfacePatchCount == 2U &&
                    hard.receipt.bakedTerrainSurfacePatchCount == 2U,
                "hard topology does not duplicate walkable top patches") &&
         expect(hard.receipt.bakedTerrainCliffBlockerCount ==
                    smooth.receipt.bakedTerrainCliffBlockerCount + 2U &&
                    hardActor != nullptr && hardActor->blocksActor &&
                    hardProjectile != nullptr &&
                    hardProjectile->blocksProjectile,
                "one internal hard edge adds exactly actor and projectile blockers") &&
         expect(smoothPhysics.ok && hardPhysics.ok && !smoothHit && hardHit,
                "runtime segment crosses smooth join but stops at authored cliff") &&
         expect(smoothReasoning.nodes.size() == 2U &&
                    smoothReasoning.edges.size() == 1U &&
                    hardReasoning.nodes.size() == 2U &&
                    hardReasoning.edges.empty(),
                "authored cliff removes the cross-edge reasoning connection");
}

}  // namespace

int main() {
  const bool ok = voxelBakePreservesMaterialIdentityAndGeometryRole() &&
                  fixtureMetadataProducesHonestPhysicsSurfaces() &&
                  compoundFixtureAssetsReachRuntimePhysics() &&
                  importedStairSupportsFullBoundedRuntimeTraversal() &&
                  generatedTraversalGeometryStaysInRenderCollisionParity() &&
                  generatedStructuralGeometryStaysInRenderCollisionParity() &&
                  generatedHipRoofCollisionMatchesTaperedWeatherFace() &&
                  generatedTraversalTransformsFailClosedAndStayBounded() &&
                  renderOnlyAndUnsafeMetadataStayVisibleWithoutPhysics() &&
                  defaultMetadataUsesBoundsButNeverInventsWalkability() &&
                  tiltedWalkableAssetDoesNotFabricateAHorizontalTop() &&
                  compoundBoundsMapResizeAndEmitIndependentWalkableTops() &&
                  compoundBoundsFollowNonuniformScaleAndYaw() &&
                  invalidAndTiltedCompoundContractsFailClosed() &&
                  hiddenHierarchyIsExcludedUnlessExplicitlyRequested() &&
                  windowTreatmentSeparatesCollisionFromVision() &&
                  authoredTerrainHardEdgeAddsOnlyTheInternalCliffBlocker();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
