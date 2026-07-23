#include "EditorPlacementClearance.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "app/iggy3d/creative/spatial/SurfacePose.hpp"
#include "app/iggy3d/creative/tools/Volume.hpp"
#include "content/assets/StaticMeshAsset.hpp"
#include "core/math/OrientedBox.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

constexpr float kClearanceEpsilonMeters = 1.0e-5F;
constexpr std::uint64_t kClearanceCellVisitCapacity = 65'536U;
constexpr std::uint64_t kClearanceTerrainCellCapacity = 4'096U;

struct ClearanceBox {
  iggy3d::OrientedBox oriented;
  iggy3d::Aabb3 worldAabb;
  cr::CreativeTransformedBounds transformed;
  bool valid = false;
};

struct CollisionBoxSet {
  std::array<ClearanceBox, iggy3d::kMaxStaticMeshCollisionPartCount> boxes{};
  std::size_t count = 0U;
  bool valid = false;
};

[[nodiscard]] bool descriptorBlocksPlacement(
    const cr::CreativeObjectDescriptor& descriptor) noexcept {
  return descriptor.hasBounds &&
         descriptor.profile != cr::CreativeObjectProfile::RoomContainer &&
         cr::isSolidCreativeSpatialOccupancy(descriptor.occupancyKind);
}

[[nodiscard]] bool placementPlanBlocksPlacement(
    const CreativeBrushPlacementPlan& plan) noexcept {
  return plan.brush == cr::CreativeObjectKind::PrefabInstance ||
         descriptorBlocksPlacement(cr::describeObject(plan.brush));
}

[[nodiscard]] ClearanceBox clearanceBox(
    cr::CreativeBounds bounds,
    cr::CreativeTransform transform) noexcept {
  ClearanceBox result;
  result.transformed = cr::resolveCreativeTransformedBounds(bounds, transform);
  if (!result.transformed.valid) {
    return result;
  }
  const cr::CreativeCoreVec3Conversion center =
      cr::creativeVec3ToCoreChecked(result.transformed.center);
  const cr::CreativeCoreVec3Conversion size =
      cr::creativeVec3ToCoreChecked(result.transformed.size);
  const cr::CreativeCoreVec3Conversion rotation =
      cr::creativeVec3ToCoreChecked(result.transformed.rotationEulerRadians);
  const cr::CreativeCoreVec3Conversion worldMin =
      cr::creativeVec3ToCoreChecked(result.transformed.worldBounds.min);
  const cr::CreativeCoreVec3Conversion worldMax =
      cr::creativeVec3ToCoreChecked(result.transformed.worldBounds.max);
  if (!center.converted || !size.converted || !rotation.converted ||
      !worldMin.converted || !worldMax.converted) {
    return result;
  }
  const iggy3d::Vec3 half = size.value * 0.5F;
  result.oriented = iggy3d::makeOrientedBox(
      {center.value, rotation.value, {1.0F, 1.0F, 1.0F}},
      iggy3d::makeAabb3(half * -1.0F, half));
  result.worldAabb = iggy3d::makeAabb3(worldMin.value, worldMax.value);
  result.valid = iggy3d::isValid(result.worldAabb);
  return result;
}

[[nodiscard]] ClearanceBox localClearanceBox(
    cr::CreativeBounds localBounds,
    cr::CreativeTransform transform) noexcept {
  ClearanceBox result;
  const cr::CreativeBoundsMetrics local = cr::measureCreativeBounds(localBounds);
  if (!local.valid || !cr::isFiniteCreativeVec3(transform.position) ||
      !cr::isFiniteCreativeVec3(transform.rotationEulerRadians) ||
      !cr::isPositiveCreativeVec3(transform.scale) ||
      !cr::isPositiveCreativeVec3(local.size)) {
    return result;
  }

  result.transformed.size = {
      local.size.x * transform.scale.x,
      local.size.y * transform.scale.y,
      local.size.z * transform.scale.z,
  };
  result.transformed.rotationEulerRadians = transform.rotationEulerRadians;
  const cr::CreativeVec3 scaledCenter{
      local.center.x * transform.scale.x,
      local.center.y * transform.scale.y,
      local.center.z * transform.scale.z,
  };
  const cr::CreativeVec3 rotatedCenter = cr::rotateCreativeVectorEulerXyz(
      scaledCenter, transform.rotationEulerRadians);
  result.transformed.center = {
      transform.position.x + rotatedCenter.x,
      transform.position.y + rotatedCenter.y,
      transform.position.z + rotatedCenter.z,
  };
  if (!cr::isFiniteCreativeVec3(result.transformed.center) ||
      !cr::isFiniteCreativeVec3(result.transformed.size)) {
    return {};
  }

  const cr::CreativeVec3 half{
      result.transformed.size.x * 0.5,
      result.transformed.size.y * 0.5,
      result.transformed.size.z * 0.5,
  };
  for (std::size_t index = 0U; index < result.transformed.corners.size();
       ++index) {
    const cr::CreativeVec3 cornerOffset{
        (index & 1U) != 0U ? half.x : -half.x,
        (index & 2U) != 0U ? half.y : -half.y,
        (index & 4U) != 0U ? half.z : -half.z,
    };
    const cr::CreativeVec3 rotated = cr::rotateCreativeVectorEulerXyz(
        cornerOffset, transform.rotationEulerRadians);
    result.transformed.corners[index] = {
        result.transformed.center.x + rotated.x,
        result.transformed.center.y + rotated.y,
        result.transformed.center.z + rotated.z,
    };
    if (!cr::isFiniteCreativeVec3(result.transformed.corners[index])) {
      return {};
    }
  }
  result.transformed.worldBounds = {
      result.transformed.corners.front(), result.transformed.corners.front()};
  for (std::size_t index = 1U; index < result.transformed.corners.size();
       ++index) {
    const cr::CreativeVec3& corner = result.transformed.corners[index];
    result.transformed.worldBounds.min.x =
        std::min(result.transformed.worldBounds.min.x, corner.x);
    result.transformed.worldBounds.min.y =
        std::min(result.transformed.worldBounds.min.y, corner.y);
    result.transformed.worldBounds.min.z =
        std::min(result.transformed.worldBounds.min.z, corner.z);
    result.transformed.worldBounds.max.x =
        std::max(result.transformed.worldBounds.max.x, corner.x);
    result.transformed.worldBounds.max.y =
        std::max(result.transformed.worldBounds.max.y, corner.y);
    result.transformed.worldBounds.max.z =
        std::max(result.transformed.worldBounds.max.z, corner.z);
  }
  result.transformed.valid = true;

  const cr::CreativeCoreVec3Conversion center =
      cr::creativeVec3ToCoreChecked(result.transformed.center);
  const cr::CreativeCoreVec3Conversion size =
      cr::creativeVec3ToCoreChecked(result.transformed.size);
  const cr::CreativeCoreVec3Conversion rotation =
      cr::creativeVec3ToCoreChecked(result.transformed.rotationEulerRadians);
  const cr::CreativeCoreVec3Conversion worldMin = cr::creativeVec3ToCoreChecked(
      result.transformed.worldBounds.min);
  const cr::CreativeCoreVec3Conversion worldMax = cr::creativeVec3ToCoreChecked(
      result.transformed.worldBounds.max);
  if (!center.converted || !size.converted || !rotation.converted ||
      !worldMin.converted || !worldMax.converted) {
    return {};
  }
  const iggy3d::Vec3 coreHalf = size.value * 0.5F;
  result.oriented = iggy3d::makeOrientedBox(
      {center.value, rotation.value, {1.0F, 1.0F, 1.0F}},
      iggy3d::makeAabb3(coreHalf * -1.0F, coreHalf));
  result.worldAabb = iggy3d::makeAabb3(worldMin.value, worldMax.value);
  result.valid = iggy3d::isValid(result.worldAabb);
  return result;
}

[[nodiscard]] ClearanceBox objectClearanceBox(
    const cr::CreativeObject& object) noexcept {
  cr::CreativeTransform transform = object.transform;
  if (!cr::objectHasTransform(object.kind)) {
    transform = {};
  }
  return clearanceBox(object.bounds, transform);
}

[[nodiscard]] cr::CreativeBounds creativeBoundsFor(
    const iggy3d::StaticMeshCollisionPart& part) noexcept {
  return {{part.boundsMin.x, part.boundsMin.y, part.boundsMin.z},
          {part.boundsMax.x, part.boundsMax.y, part.boundsMax.z}};
}

[[nodiscard]] cr::CreativeBounds creativeBoundsFor(
    const iggy3d::StaticMeshAssetCatalogEntry& asset) noexcept {
  return {{asset.boundsMin.x, asset.boundsMin.y, asset.boundsMin.z},
          {asset.boundsMax.x, asset.boundsMax.y, asset.boundsMax.z}};
}

[[nodiscard]] CollisionBoxSet assetCollisionBoxes(
    const iggy3d::StaticMeshAssetCatalogEntry& asset,
    const cr::CreativeTransform& transform) noexcept {
  CollisionBoxSet result;
  if (asset.collisionParts.size() > result.boxes.size()) {
    return result;
  }
  if (asset.collisionParts.empty()) {
    result.boxes[0] = localClearanceBox(creativeBoundsFor(asset), transform);
    result.count = result.boxes[0].valid ? 1U : 0U;
    result.valid = result.count == 1U;
    return result;
  }
  for (const iggy3d::StaticMeshCollisionPart& part : asset.collisionParts) {
    ClearanceBox box = localClearanceBox(creativeBoundsFor(part), transform);
    if (!box.valid) {
      return {};
    }
    result.boxes[result.count++] = box;
  }
  result.valid = result.count > 0U;
  return result;
}

[[nodiscard]] CollisionBoxSet objectCollisionBoxes(
    const cr::CreativeObject& object,
    const iggy3d::StaticMeshAssetCatalog& catalog,
    bool requireCatalogAsset) noexcept {
  if (!object.assetId.empty()) {
    if (const iggy3d::StaticMeshAssetCatalogEntry* asset =
            catalog.find(object.assetId);
        asset != nullptr) {
      return assetCollisionBoxes(*asset, object.transform);
    }
    if (requireCatalogAsset) {
      return {};
    }
  }
  CollisionBoxSet result;
  result.boxes[0] = objectClearanceBox(object);
  result.count = result.boxes[0].valid ? 1U : 0U;
  result.valid = result.count == 1U;
  return result;
}

[[nodiscard]] ClearanceBox axisAlignedClearanceBox(
    cr::CreativeBounds bounds) noexcept {
  const cr::CreativeBoundsMetrics metrics = cr::measureCreativeBounds(bounds);
  if (!metrics.valid || !cr::isPositiveCreativeVec3(metrics.size)) {
    return {};
  }
  cr::CreativeTransform transform;
  transform.position = metrics.center;
  return clearanceBox(bounds, transform);
}

[[nodiscard]] bool configuredWorldBoundsContain(
    cr::CreativeBounds world,
    cr::CreativeBounds candidate) noexcept {
  const cr::CreativeBoundsMetrics worldMetrics = cr::measureCreativeBounds(world);
  if (!worldMetrics.valid || !cr::isPositiveCreativeVec3(worldMetrics.size)) {
    return true;
  }
  const double epsilon = static_cast<double>(kClearanceEpsilonMeters);
  return candidate.min.x >= world.min.x - epsilon &&
         candidate.min.y >= world.min.y - epsilon &&
         candidate.min.z >= world.min.z - epsilon &&
         candidate.max.x <= world.max.x + epsilon &&
         candidate.max.y <= world.max.y + epsilon &&
         candidate.max.z <= world.max.z + epsilon;
}

[[nodiscard]] bool checkedCellCoordinate(double value,
                                         std::int32_t& output) noexcept {
  const double floored = std::floor(value);
  if (!std::isfinite(floored) ||
      floored < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      floored > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
    return false;
  }
  output = static_cast<std::int32_t>(floored);
  return true;
}

[[nodiscard]] bool candidateCellBounds(
    const ClearanceBox& candidate,
    cr::CreativeGridSettings grid,
    cr::CreativeGridCoord3& minimum,
    cr::CreativeGridCoord3& maximum,
    std::uint64_t& cellCount) noexcept {
  if (!cr::isFiniteCreativeVec3(grid.origin) ||
      !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0) {
    return false;
  }
  const cr::CreativeBounds& bounds = candidate.transformed.worldBounds;
  if (!checkedCellCoordinate((bounds.min.x - grid.origin.x) /
                                 grid.cellSizeMeters,
                             minimum.x) ||
      !checkedCellCoordinate((bounds.min.y - grid.origin.y) /
                                 grid.cellSizeMeters,
                             minimum.y) ||
      !checkedCellCoordinate((bounds.min.z - grid.origin.z) /
                                 grid.cellSizeMeters,
                             minimum.z) ||
      !checkedCellCoordinate((bounds.max.x - grid.origin.x) /
                                 grid.cellSizeMeters,
                             maximum.x) ||
      !checkedCellCoordinate((bounds.max.y - grid.origin.y) /
                                 grid.cellSizeMeters,
                             maximum.y) ||
      !checkedCellCoordinate((bounds.max.z - grid.origin.z) /
                                 grid.cellSizeMeters,
                             maximum.z)) {
    return false;
  }
  const std::uint64_t sizeX =
      static_cast<std::uint64_t>(static_cast<std::int64_t>(maximum.x) -
                                 minimum.x + 1);
  const std::uint64_t sizeY =
      static_cast<std::uint64_t>(static_cast<std::int64_t>(maximum.y) -
                                 minimum.y + 1);
  const std::uint64_t sizeZ =
      static_cast<std::uint64_t>(static_cast<std::int64_t>(maximum.z) -
                                 minimum.z + 1);
  if (sizeX == 0U || sizeY == 0U || sizeZ == 0U ||
      sizeX > kClearanceCellVisitCapacity ||
      sizeY > kClearanceCellVisitCapacity / sizeX ||
      sizeZ > kClearanceCellVisitCapacity / (sizeX * sizeY)) {
    cellCount = kClearanceCellVisitCapacity + 1U;
    return true;
  }
  cellCount = sizeX * sizeY * sizeZ;
  return true;
}

[[nodiscard]] bool overlapsObject(
    const cr::CreativeDocument& document,
    std::span<const cr::CreativeObjectId> ignoredObjectIds,
    const ClearanceBox& candidate,
    const cr::CreativeObject& object,
    cr::CreativePlacementClearanceResult& result) noexcept {
  if (!cr::creativeObjectEffectivelyVisible(document, object.id) ||
      !creativeObjectBlocksPlacementClearance(object) ||
      std::find(ignoredObjectIds.begin(), ignoredObjectIds.end(), object.id) !=
          ignoredObjectIds.end()) {
    return false;
  }
  ++result.testedAuthoredObjectCount;
  const ClearanceBox obstacle = objectClearanceBox(object);
  if (!obstacle.valid) {
    result.status = cr::CreativePlacementClearanceStatus::InvalidRequest;
    return true;
  }
  if (!iggy3d::intersects(candidate.worldAabb, obstacle.worldAabb) ||
      !iggy3d::strictlyOverlaps(candidate.oriented, obstacle.oriented,
                                kClearanceEpsilonMeters)) {
    return false;
  }
  result.status =
      cr::CreativePlacementClearanceStatus::AuthoredObjectBlocked;
  result.blockingObjectId = object.id;
  return true;
}

[[nodiscard]] bool authoredObjectBlocks(
    const cr::CreativeDocument& document,
    std::span<const cr::CreativeObjectId> ignoredObjectIds,
    const ClearanceBox& candidate,
    const CreativePlacementClearanceCache* cache,
    cr::CreativePlacementClearanceResult& result) noexcept {
  const bool useCache = cache != nullptr && cache->valid && cache->complete &&
                        cache->documentId == document.id() &&
                        cache->documentRevision == document.revision();
  if (useCache) {
    try {
      const iggy3d::AabbGridQueryResult query =
          cache->authoredObstacleIndex.queryChecked(candidate.worldAabb);
      if (query.queried()) {
        for (const std::uint64_t id : query.candidates) {
          const cr::CreativeObject* object = document.findObject(id);
          if (object == nullptr ||
              overlapsObject(document, ignoredObjectIds, candidate, *object,
                             result)) {
            if (object == nullptr) {
              result.status =
                  cr::CreativePlacementClearanceStatus::InvalidRequest;
            }
            return true;
          }
        }
        return false;
      }
    } catch (...) {
      result.status = cr::CreativePlacementClearanceStatus::InvalidRequest;
      return true;
    }
  }
  for (const cr::CreativeObject& object : document.objects()) {
    if (overlapsObject(document, ignoredObjectIds, candidate, object, result)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool voxelBlocks(
    const cr::CreativeDocument& document,
    const ClearanceBox& candidate,
    cr::CreativePlacementClearanceResult& result) noexcept {
  if (document.voxelField().occupiedCellCount() == 0U) {
    return false;
  }
  const cr::CreativeGridSettings grid = document.gridSettings();
  cr::CreativeGridCoord3 minimum;
  cr::CreativeGridCoord3 maximum;
  std::uint64_t cellCount = 0U;
  if (!candidateCellBounds(candidate, grid, minimum, maximum, cellCount)) {
    result.status = cr::CreativePlacementClearanceStatus::InvalidRequest;
    return true;
  }
  if (cellCount > kClearanceCellVisitCapacity) {
    result.status =
        cr::CreativePlacementClearanceStatus::TraversalLimitExceeded;
    return true;
  }
  for (std::int64_t z = minimum.z; z <= maximum.z; ++z) {
    for (std::int64_t y = minimum.y; y <= maximum.y; ++y) {
      for (std::int64_t x = minimum.x; x <= maximum.x; ++x) {
        const cr::CreativeGridCoord3 cell{static_cast<std::int32_t>(x),
                                          static_cast<std::int32_t>(y),
                                          static_cast<std::int32_t>(z)};
        ++result.testedVoxelCellCount;
        if (!document.voxelField().occupied(cell)) {
          continue;
        }
        const ClearanceBox obstacle = axisAlignedClearanceBox(
            cr::creativeVolumeCellBounds(cell, grid.cellSizeMeters,
                                         grid.origin));
        if (!obstacle.valid) {
          result.status =
              cr::CreativePlacementClearanceStatus::InvalidRequest;
          return true;
        }
        if (iggy3d::strictlyOverlaps(candidate.oriented, obstacle.oriented,
                                     kClearanceEpsilonMeters)) {
          result.status = cr::CreativePlacementClearanceStatus::VoxelBlocked;
          result.blockingVoxelCell = cell;
          return true;
        }
      }
    }
  }
  return false;
}

[[nodiscard]] bool terrainSampleBlocks(
    const cr::CreativeDocument& document,
    const ClearanceBox& candidate,
    cr::CreativeVec3 samplePoint,
    cr::CreativePlacementClearanceResult& result) noexcept {
  const cr::CreativeGridSettings grid = document.gridSettings();
  const cr::CreativeTerrainSurfacePose pose =
      cr::sampleCreativeTerrainSurfacePose(
          {&document.terrainField(), samplePoint, grid.origin,
           grid.cellSizeMeters});
  ++result.testedTerrainSampleCount;
  if (!pose.accepted) {
    result.status = cr::CreativePlacementClearanceStatus::InvalidRequest;
    return true;
  }
  if (!pose.present) {
    return false;
  }
  const cr::CreativeCoreVec3Conversion rayOrigin =
      cr::creativeVec3ToCoreChecked(
          {samplePoint.x,
           candidate.transformed.worldBounds.min.y -
               std::max(1.0, candidate.transformed.size.y),
           samplePoint.z});
  if (!rayOrigin.converted) {
    result.status = cr::CreativePlacementClearanceStatus::InvalidRequest;
    return true;
  }
  const double maxDistanceDouble = candidate.transformed.size.y * 3.0 + 2.0;
  if (!std::isfinite(maxDistanceDouble) ||
      maxDistanceDouble > std::numeric_limits<float>::max()) {
    result.status = cr::CreativePlacementClearanceStatus::InvalidRequest;
    return true;
  }
  const float maxDistance = static_cast<float>(maxDistanceDouble);
  const iggy3d::OrientedBoxRayHit hit = iggy3d::intersectsRay(
      candidate.oriented, rayOrigin.value, iggy3d::vec3UnitY(), maxDistance);
  if (!hit.hit) {
    return false;
  }
  const double entryY = static_cast<double>(hit.pointMeters.y);
  if (pose.position.y <=
      entryY + static_cast<double>(kClearanceEpsilonMeters)) {
    return false;
  }
  result.status = cr::CreativePlacementClearanceStatus::TerrainBlocked;
  result.blockingTerrainCell = pose.cell;
  return true;
}

[[nodiscard]] bool terrainBlocks(
    const cr::CreativeDocument& document,
    const ClearanceBox& candidate,
    cr::CreativePlacementClearanceResult& result) noexcept {
  if (document.terrainField().controlCount() == 0U) {
    return false;
  }
  const cr::CreativeGridSettings grid = document.gridSettings();
  if (!cr::isFiniteCreativeVec3(grid.origin) ||
      !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0) {
    result.status = cr::CreativePlacementClearanceStatus::InvalidRequest;
    return true;
  }

  const std::array<cr::CreativeVec3, 9U> candidateSamples{{
      candidate.transformed.center,
      candidate.transformed.corners[0], candidate.transformed.corners[1],
      candidate.transformed.corners[2], candidate.transformed.corners[3],
      candidate.transformed.corners[4], candidate.transformed.corners[5],
      candidate.transformed.corners[6], candidate.transformed.corners[7],
  }};
  for (const cr::CreativeVec3 sample : candidateSamples) {
    if (terrainSampleBlocks(document, candidate, sample, result)) {
      return true;
    }
  }

  std::int32_t minX = 0;
  std::int32_t maxX = 0;
  std::int32_t minZ = 0;
  std::int32_t maxZ = 0;
  const cr::CreativeBounds& bounds = candidate.transformed.worldBounds;
  if (!checkedCellCoordinate((bounds.min.x - grid.origin.x) /
                                 grid.cellSizeMeters,
                             minX) ||
      !checkedCellCoordinate((bounds.max.x - grid.origin.x) /
                                 grid.cellSizeMeters,
                             maxX) ||
      !checkedCellCoordinate((bounds.min.z - grid.origin.z) /
                                 grid.cellSizeMeters,
                             minZ) ||
      !checkedCellCoordinate((bounds.max.z - grid.origin.z) /
                                 grid.cellSizeMeters,
                             maxZ)) {
    result.status = cr::CreativePlacementClearanceStatus::InvalidRequest;
    return true;
  }
  const std::uint64_t sizeX =
      static_cast<std::uint64_t>(static_cast<std::int64_t>(maxX) - minX + 1);
  const std::uint64_t sizeZ =
      static_cast<std::uint64_t>(static_cast<std::int64_t>(maxZ) - minZ + 1);
  if (sizeX == 0U || sizeZ == 0U ||
      sizeX > kClearanceTerrainCellCapacity ||
      sizeZ > kClearanceTerrainCellCapacity / sizeX) {
    result.status =
        cr::CreativePlacementClearanceStatus::TraversalLimitExceeded;
    return true;
  }

  constexpr std::array<std::array<double, 2U>, 5U> offsets{{
      {0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}, {0.5, 0.5},
  }};
  for (std::int64_t z = minZ; z <= maxZ; ++z) {
    for (std::int64_t x = minX; x <= maxX; ++x) {
      for (const auto& offset : offsets) {
        const cr::CreativeVec3 sample{
            grid.origin.x +
                (static_cast<double>(x) + offset[0]) * grid.cellSizeMeters,
            candidate.transformed.center.y,
            grid.origin.z +
                (static_cast<double>(z) + offset[1]) * grid.cellSizeMeters,
        };
        if (terrainSampleBlocks(document, candidate, sample, result)) {
          return true;
        }
      }
    }
  }
  return false;
}

}  // namespace

bool creativeObjectBlocksPlacementClearance(
    const cr::CreativeObject& object) noexcept {
  return object.visible &&
         descriptorBlocksPlacement(cr::describeObject(object.kind));
}

bool refreshCreativePlacementClearanceCache(
    CreativePlacementClearanceCache& cache,
    const cr::CreativeDocument& document) {
  if (cache.valid && cache.documentId == document.id() &&
      cache.documentRevision == document.revision()) {
    return false;
  }
  cache.authoredObstacleIndex.clear();
  cache.documentId = document.id();
  cache.documentRevision = document.revision();
  cache.indexedObjectCount = 0U;
  cache.complete = document.isValid();
  for (const cr::CreativeObject& object : document.objects()) {
    if (!cr::creativeObjectEffectivelyVisible(document, object.id) ||
        !creativeObjectBlocksPlacementClearance(object)) {
      continue;
    }
    const ClearanceBox obstacle = objectClearanceBox(object);
    if (!obstacle.valid ||
        !cache.authoredObstacleIndex.insert(object.id,
                                            obstacle.worldAabb)) {
      cache.complete = false;
      continue;
    }
    ++cache.indexedObjectCount;
  }
  ++cache.rebuildCount;
  cache.valid = document.isValid();
  return true;
}

void invalidateCreativePlacementClearanceCache(
    CreativePlacementClearanceCache& cache) noexcept {
  cache.authoredObstacleIndex.clear();
  cache.documentId = cr::kInvalidDocumentId;
  cache.documentRevision = 0U;
  cache.indexedObjectCount = 0U;
  cache.complete = false;
  cache.valid = false;
}

cr::CreativePlacementClearanceResult evaluateCreativeBrushPlacementClearance(
    const cr::CreativeDocument& document,
    const CreativeBrushPlacementPlan& plan,
    const CreativePlacementClearanceCache* cache,
    std::span<const cr::CreativeObjectId> ignoredObjectIds) noexcept {
  cr::CreativePlacementClearanceResult result;
  result.evaluated = true;
  result.status = cr::CreativePlacementClearanceStatus::InvalidRequest;
  if (!document.isValid() || !plan.valid ||
      plan.status != CreativeBrushPlacementPlanStatus::Ready) {
    return result;
  }
  const ClearanceBox candidate =
      clearanceBox(plan.authoredBounds, plan.transform);
  if (!candidate.valid) {
    return result;
  }

  if (!configuredWorldBoundsContain(document.worldBounds(),
                                    candidate.transformed.worldBounds)) {
    result.status =
        cr::CreativePlacementClearanceStatus::OutsideWorldBounds;
    return result;
  }

  if (placementPlanBlocksPlacement(plan)) {
    std::vector<cr::CreativeObjectId> combinedIgnored;
    std::span<const cr::CreativeObjectId> effectiveIgnored = ignoredObjectIds;
    if (plan.hasAttachment &&
        std::find(ignoredObjectIds.begin(), ignoredObjectIds.end(),
                  plan.attachmentTargetId) == ignoredObjectIds.end()) {
      combinedIgnored.assign(ignoredObjectIds.begin(), ignoredObjectIds.end());
      combinedIgnored.push_back(plan.attachmentTargetId);
      effectiveIgnored = combinedIgnored;
    }
    if (authoredObjectBlocks(document, effectiveIgnored, candidate, cache,
                             result) ||
        voxelBlocks(document, candidate, result) ||
        terrainBlocks(document, candidate, result)) {
      return result;
    }
  }

  result.status = cr::CreativePlacementClearanceStatus::Ready;
  result.allowed = true;
  return result;
}

CreativeObjectSetClearanceResult evaluateCreativeObjectSetPlacementClearance(
    const cr::CreativeDocument& document,
    std::span<const cr::CreativeObject> candidates,
    std::span<const cr::CreativeObjectId> ignoredObjectIds,
    const CreativePlacementClearanceCache* cache) noexcept {
  CreativeObjectSetClearanceResult result;
  result.clearance.evaluated = true;
  result.clearance.status = cr::CreativePlacementClearanceStatus::InvalidRequest;
  if (!document.isValid() || candidates.empty()) {
    return result;
  }

  for (const cr::CreativeObject& object : candidates) {
    ++result.candidateObjectCount;
    if (!creativeObjectBlocksPlacementClearance(object)) {
      continue;
    }
    result.candidateObjectId = object.id;
    const ClearanceBox candidate = objectClearanceBox(object);
    if (!candidate.valid) {
      return result;
    }
    if (!configuredWorldBoundsContain(document.worldBounds(),
                                      candidate.transformed.worldBounds)) {
      result.clearance.status =
          cr::CreativePlacementClearanceStatus::OutsideWorldBounds;
      return result;
    }
    if (authoredObjectBlocks(document, ignoredObjectIds, candidate, cache,
                             result.clearance) ||
        voxelBlocks(document, candidate, result.clearance) ||
        terrainBlocks(document, candidate, result.clearance)) {
      return result;
    }
  }

  result.candidateObjectId = cr::kInvalidObjectId;
  result.clearance.status = cr::CreativePlacementClearanceStatus::Ready;
  result.clearance.allowed = true;
  return result;
}

CreativeObjectSetClearanceResult evaluateCreativeAttachmentPlacementClearance(
    const cr::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog& assetCatalog,
    std::span<const cr::CreativeObject> candidates,
    std::span<const cr::CreativeObjectId> sourceHierarchyIds,
    cr::CreativeObjectId sourceObjectId,
    cr::CreativeObjectId targetObjectId,
    const CreativePlacementClearanceCache* cache) {
  CreativeObjectSetClearanceResult result;
  result.clearance.evaluated = true;
  result.clearance.status = cr::CreativePlacementClearanceStatus::InvalidRequest;
  if (!document.isValid() || candidates.empty() ||
      targetObjectId == cr::kInvalidObjectId ||
      (sourceObjectId != cr::kInvalidObjectId &&
       sourceObjectId == targetObjectId)) {
    return result;
  }
  const cr::CreativeObject* target = document.findObject(targetObjectId);
  if (target == nullptr) {
    return result;
  }
  const CollisionBoxSet targetBoxes =
      objectCollisionBoxes(*target, assetCatalog, false);
  if (!targetBoxes.valid) {
    return result;
  }

  std::vector<cr::CreativeObjectId> ignored(sourceHierarchyIds.begin(),
                                            sourceHierarchyIds.end());
  ignored.push_back(targetObjectId);
  const std::span<const cr::CreativeObjectId> ignoredObjects{ignored};

  for (const cr::CreativeObject& candidateObject : candidates) {
    ++result.candidateObjectCount;
    if (candidateObject.id != sourceObjectId &&
        !creativeObjectBlocksPlacementClearance(candidateObject)) {
      continue;
    }
    result.candidateObjectId = candidateObject.id;
    const CollisionBoxSet candidateBoxes = objectCollisionBoxes(
        candidateObject, assetCatalog, candidateObject.id == sourceObjectId);
    if (!candidateBoxes.valid) {
      return result;
    }
    for (std::size_t candidateIndex = 0U;
         candidateIndex < candidateBoxes.count; ++candidateIndex) {
      const ClearanceBox& candidate = candidateBoxes.boxes[candidateIndex];
      if (!configuredWorldBoundsContain(document.worldBounds(),
                                        candidate.transformed.worldBounds)) {
        result.clearance.status =
            cr::CreativePlacementClearanceStatus::OutsideWorldBounds;
        return result;
      }
      if (authoredObjectBlocks(document, ignoredObjects, candidate, cache,
                               result.clearance) ||
          voxelBlocks(document, candidate, result.clearance) ||
          terrainBlocks(document, candidate, result.clearance)) {
        return result;
      }
      for (std::size_t targetIndex = 0U; targetIndex < targetBoxes.count;
           ++targetIndex) {
        ++result.clearance.testedAuthoredObjectCount;
        const ClearanceBox& obstacle = targetBoxes.boxes[targetIndex];
        if (!iggy3d::intersects(candidate.worldAabb, obstacle.worldAabb) ||
            !iggy3d::strictlyOverlaps(candidate.oriented, obstacle.oriented,
                                      kClearanceEpsilonMeters)) {
          continue;
        }
        result.clearance.status =
            cr::CreativePlacementClearanceStatus::AuthoredObjectBlocked;
        result.clearance.blockingObjectId = targetObjectId;
        return result;
      }
    }
  }

  result.candidateObjectId = cr::kInvalidObjectId;
  result.clearance.status = cr::CreativePlacementClearanceStatus::Ready;
  result.clearance.allowed = true;
  return result;
}

void applyCreativeBrushPlacementClearance(
    CreativeBrushPlacementAdmission& admission,
    const cr::CreativeDocument& document,
    const CreativePlacementClearanceCache* cache,
    std::span<const cr::CreativeObjectId> ignoredObjectIds) noexcept {
  if (!admission.allowed || !admission.plan.valid) {
    return;
  }
  admission.plan.clearance = evaluateCreativeBrushPlacementClearance(
      document, admission.plan, cache, ignoredObjectIds);
  if (!admission.plan.clearance.allowed) {
    admission.allowed = false;
    admission.status =
        CreativeBrushPlacementAdmissionStatus::ClearanceBlocked;
  }
}

}  // namespace iggy3d_creative_app
