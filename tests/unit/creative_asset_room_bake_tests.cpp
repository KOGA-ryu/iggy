#include "app/iggy3d/creative/adapters/RoomBake.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

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
    bool walkable = false) {
  iggy3d::StaticMeshAssetCatalogEntry entry;
  entry.assetId = std::move(assetId);
  entry.label = entry.assetId;
  entry.boundsMin = {-0.5F, -0.5F, -0.5F};
  entry.boundsMax = {0.5F, 0.5F, 0.5F};
  entry.authoringMetadata.collisionMode = mode;
  entry.authoringMetadata.status = status;
  entry.authoringMetadata.walkable = walkable;
  entry.authoringMetadata.collisionSpecified =
      status !=
      iggy3d::StaticMeshAuthoringMetadataStatus::DefaultsApplied;
  entry.authoringMetadata.walkableSpecified = walkable;
  return entry;
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

}  // namespace

int main() {
  const bool ok = fixtureMetadataProducesHonestPhysicsSurfaces() &&
                  renderOnlyAndUnsafeMetadataStayVisibleWithoutPhysics() &&
                  defaultMetadataUsesBoundsButNeverInventsWalkability() &&
                  tiltedWalkableAssetDoesNotFabricateAHorizontalTop();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
