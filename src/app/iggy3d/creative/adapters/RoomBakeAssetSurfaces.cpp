#include "app/iggy3d/creative/adapters/RoomBakeAssetSurfaces.hpp"

#include "app/iggy3d/creative/Geometry.hpp"

#include <cmath>
#include <string>
#include <utility>

namespace iggy3d::creative::room_bake_internal {
namespace {

[[nodiscard]] bool nearZero(double value) noexcept {
  return std::fabs(value) <= 1.0e-9;
}

[[nodiscard]] bool uprightWalkableRotation(CreativeVec3 rotation) noexcept {
  return isFiniteCreativeVec3(rotation) && nearZero(rotation.x) &&
         nearZero(rotation.z);
}

[[nodiscard]] std::string stableObjectId(CreativeObjectId objectId) {
  return "creative_object_" + std::to_string(objectId);
}

}  // namespace

RoomBakeAssetSurfaceClassification classifyRoomBakeAssetSurfaces(
    const CreativeObject& object,
    const StaticMeshAssetCatalog* catalog) noexcept {
  if (object.assetId.empty()) {
    return {};
  }
  const StaticMeshAssetCatalogEntry* entry =
      catalog != nullptr ? catalog->find(object.assetId) : nullptr;
  if (entry == nullptr) {
    return {RoomBakeAssetSurfacePolicy::MissingMetadata};
  }
  const StaticMeshAuthoringMetadata& metadata = entry->authoringMetadata;
  if (metadata.status == StaticMeshAuthoringMetadataStatus::Invalid) {
    return {RoomBakeAssetSurfacePolicy::InvalidMetadata};
  }
  if (metadata.status ==
          StaticMeshAuthoringMetadataStatus::UnsupportedCollision ||
      metadata.collisionMode == StaticMeshCollisionMode::Convex ||
      metadata.collisionMode == StaticMeshCollisionMode::Mesh) {
    return {RoomBakeAssetSurfacePolicy::UnsupportedCollision};
  }
  if (metadata.collisionMode == StaticMeshCollisionMode::None) {
    return {RoomBakeAssetSurfacePolicy::None};
  }
  if (metadata.collisionMode != StaticMeshCollisionMode::Bounds) {
    return {RoomBakeAssetSurfacePolicy::InvalidMetadata};
  }
  const bool walkable =
      metadata.walkable &&
      uprightWalkableRotation(object.transform.rotationEulerRadians);
  return {RoomBakeAssetSurfacePolicy::Bounds, walkable,
          metadata.walkable && !walkable};
}

bool appendRoomBakeAssetSpatialSurfaces(
    RoomAsset& room,
    std::vector<CreativeRoomBakeSpatialSurfaceSource>& sources,
    CreativeRoomBakeReceipt& receipt,
    const CreativeObject& object,
    BakeBounds bounds,
    BakedRoomRole role,
    const RoomBakeAssetSurfaceClassification& classification) {
  if (classification.policy == RoomBakeAssetSurfacePolicy::Descriptor) {
    return false;
  }
  if (classification.policy == RoomBakeAssetSurfacePolicy::Bounds) {
    const std::string stableId = stableObjectId(object.id);
    const Vec3 normal = blockerNormalForRole(bounds, role);
    RoomSpatialSurface actorSurface =
        blockerSurfaceForStableId(stableId, bounds, normal, false);
    appendSpatialSurfaceSource(sources, object.id, actorSurface);
    room.spatialSurfaces.push_back(std::move(actorSurface));

    RoomSpatialSurface projectileSurface =
        blockerSurfaceForStableId(stableId, bounds, normal, true);
    appendSpatialSurfaceSource(sources, object.id, projectileSurface);
    room.spatialSurfaces.push_back(std::move(projectileSurface));
    ++receipt.bakedAssetBoundsCollisionCount;

    if (classification.walkable) {
      RoomSpatialSurface walkable =
          walkableSurfaceForStableId(stableId, bounds);
      appendSpatialSurfaceSource(sources, object.id, walkable);
      room.spatialSurfaces.push_back(std::move(walkable));
      ++receipt.bakedAssetWalkableSurfaceCount;
    }
    if (classification.walkableTransformUnsupported) {
      ++receipt.skippedAssetWalkableTransformCount;
    }
    return true;
  }

  switch (classification.policy) {
    case RoomBakeAssetSurfacePolicy::None:
      ++receipt.skippedAssetNoCollisionCount;
      return true;
    case RoomBakeAssetSurfacePolicy::MissingMetadata:
      ++receipt.skippedMissingAssetMetadataCount;
      return true;
    case RoomBakeAssetSurfacePolicy::UnsupportedCollision:
      ++receipt.skippedUnsupportedAssetCollisionCount;
      return true;
    case RoomBakeAssetSurfacePolicy::InvalidMetadata:
      ++receipt.skippedInvalidAssetMetadataCount;
      return true;
    case RoomBakeAssetSurfacePolicy::Descriptor:
    case RoomBakeAssetSurfacePolicy::Bounds:
      return true;
  }
  return true;
}

}  // namespace iggy3d::creative::room_bake_internal
