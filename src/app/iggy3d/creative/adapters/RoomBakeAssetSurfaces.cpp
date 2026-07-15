#include "app/iggy3d/creative/adapters/RoomBakeAssetSurfaces.hpp"

#include "app/iggy3d/creative/Geometry.hpp"

#include <array>
#include <cmath>
#include <cstddef>
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

[[nodiscard]] std::string stableCollisionPartId(CreativeObjectId objectId,
                                                std::size_t partIndex) {
  return stableObjectId(objectId) + "_collision_part_" +
         std::to_string(partIndex);
}

[[nodiscard]] bool finiteBounds(Vec3 min, Vec3 max) noexcept {
  return std::isfinite(min.x) && std::isfinite(min.y) && std::isfinite(min.z) &&
         std::isfinite(max.x) && std::isfinite(max.y) && std::isfinite(max.z) &&
         min.x < max.x && min.y < max.y && min.z < max.z;
}

[[nodiscard]] bool validCollisionPart(
    const StaticMeshAssetCatalogEntry& entry,
    const StaticMeshCollisionPart& part) noexcept {
  constexpr float kContainmentEpsilon = 1.0e-4F;
  return finiteBounds(entry.boundsMin, entry.boundsMax) &&
         finiteBounds(part.boundsMin, part.boundsMax) &&
         part.boundsMin.x >= entry.boundsMin.x - kContainmentEpsilon &&
         part.boundsMin.y >= entry.boundsMin.y - kContainmentEpsilon &&
         part.boundsMin.z >= entry.boundsMin.z - kContainmentEpsilon &&
         part.boundsMax.x <= entry.boundsMax.x + kContainmentEpsilon &&
         part.boundsMax.y <= entry.boundsMax.y + kContainmentEpsilon &&
         part.boundsMax.z <= entry.boundsMax.z + kContainmentEpsilon;
}

[[nodiscard]] double mapCollisionPartAxis(double objectMin, double objectSize,
                                          float sourceMin, float sourceSize,
                                          float partValue) noexcept {
  return objectMin + (static_cast<double>(partValue - sourceMin) /
                      static_cast<double>(sourceSize)) *
                         objectSize;
}

[[nodiscard]] bool resolveCollisionPartBounds(
    const CreativeObject& object, const StaticMeshAssetCatalogEntry& entry,
    const StaticMeshCollisionPart& part, BakeBounds& output) noexcept {
  const CreativeBoundsMetrics objectBounds =
      measureCreativeBounds(object.bounds);
  const Vec3 sourceSize = entry.boundsMax - entry.boundsMin;
  if (!objectBounds.valid || !isPositiveCreativeVec3(objectBounds.size) ||
      !finiteBounds(entry.boundsMin, entry.boundsMax) ||
      !validCollisionPart(entry, part)) {
    return false;
  }
  const CreativeBounds authoredPart{
      {mapCollisionPartAxis(object.bounds.min.x, objectBounds.size.x,
                            entry.boundsMin.x, sourceSize.x, part.boundsMin.x),
       mapCollisionPartAxis(object.bounds.min.y, objectBounds.size.y,
                            entry.boundsMin.y, sourceSize.y, part.boundsMin.y),
       mapCollisionPartAxis(object.bounds.min.z, objectBounds.size.z,
                            entry.boundsMin.z, sourceSize.z, part.boundsMin.z)},
      {mapCollisionPartAxis(object.bounds.min.x, objectBounds.size.x,
                            entry.boundsMin.x, sourceSize.x, part.boundsMax.x),
       mapCollisionPartAxis(object.bounds.min.y, objectBounds.size.y,
                            entry.boundsMin.y, sourceSize.y, part.boundsMax.y),
       mapCollisionPartAxis(object.bounds.min.z, objectBounds.size.z,
                            entry.boundsMin.z, sourceSize.z,
                            part.boundsMax.z)}};
  const CreativeTransformedBounds transformed =
      resolveCreativeTransformedBounds(authoredPart, object.transform);
  return transformed.valid && validBakeBounds(transformed.worldBounds, output);
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
  if (metadata.collisionMode == StaticMeshCollisionMode::CompoundBounds) {
    if (entry->collisionParts.empty() ||
        entry->collisionParts.size() > kMaxStaticMeshCollisionPartCount) {
      return {RoomBakeAssetSurfacePolicy::InvalidMetadata};
    }
    for (const StaticMeshCollisionPart& part : entry->collisionParts) {
      if (!validCollisionPart(*entry, part)) {
        return {RoomBakeAssetSurfacePolicy::InvalidMetadata};
      }
    }
    RoomBakeAssetSurfaceClassification classification;
    classification.policy = RoomBakeAssetSurfacePolicy::CompoundBounds;
    classification.catalogEntry = entry;
    return classification;
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
  if (classification.policy == RoomBakeAssetSurfacePolicy::CompoundBounds) {
    const StaticMeshAssetCatalogEntry* entry = classification.catalogEntry;
    if (entry == nullptr) {
      ++receipt.skippedInvalidAssetMetadataCount;
      return true;
    }
    std::array<BakeBounds, kMaxStaticMeshCollisionPartCount> resolvedParts{};
    for (std::size_t partIndex = 0; partIndex < entry->collisionParts.size();
         ++partIndex) {
      if (!resolveCollisionPartBounds(object, *entry,
                                      entry->collisionParts[partIndex],
                                      resolvedParts[partIndex])) {
        ++receipt.skippedInvalidAssetMetadataCount;
        return true;
      }
    }

    const bool walkableRotation =
        uprightWalkableRotation(object.transform.rotationEulerRadians);
    bool skippedWalkableTransform = false;
    for (std::size_t partIndex = 0; partIndex < entry->collisionParts.size();
         ++partIndex) {
      const StaticMeshCollisionPart& part = entry->collisionParts[partIndex];
      const BakeBounds& resolved = resolvedParts[partIndex];
      const std::string stableId = stableCollisionPartId(object.id, partIndex);
      const Vec3 normal = blockerNormalForRole(resolved, role);
      RoomSpatialSurface actorSurface =
          blockerSurfaceForStableId(stableId, resolved, normal, false);
      appendSpatialSurfaceSource(sources, object.id, actorSurface);
      room.spatialSurfaces.push_back(std::move(actorSurface));

      RoomSpatialSurface projectileSurface =
          blockerSurfaceForStableId(stableId, resolved, normal, true);
      appendSpatialSurfaceSource(sources, object.id, projectileSurface);
      room.spatialSurfaces.push_back(std::move(projectileSurface));
      ++receipt.bakedAssetBoundsCollisionCount;

      if (part.walkable && walkableRotation) {
        RoomSpatialSurface walkable =
            walkableSurfaceForStableId(stableId, resolved);
        appendSpatialSurfaceSource(sources, object.id, walkable);
        room.spatialSurfaces.push_back(std::move(walkable));
        ++receipt.bakedAssetWalkableSurfaceCount;
      } else if (part.walkable) {
        skippedWalkableTransform = true;
      }
    }
    if (skippedWalkableTransform) {
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
    case RoomBakeAssetSurfacePolicy::CompoundBounds:
      return true;
  }
  return true;
}

}  // namespace iggy3d::creative::room_bake_internal
