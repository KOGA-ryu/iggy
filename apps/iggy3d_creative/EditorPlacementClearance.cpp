#include "EditorPlacementClearance.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/spatial/SurfacePose.hpp"
#include "app/iggy3d/creative/tools/Volume.hpp"
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

[[nodiscard]] bool descriptorBlocksPlacement(
    const cr::CreativeObjectDescriptor& descriptor) noexcept {
  return descriptor.hasBounds &&
         descriptor.profile != cr::CreativeObjectProfile::RoomContainer &&
         cr::isSolidCreativeSpatialOccupancy(descriptor.occupancyKind);
}

[[nodiscard]] bool objectBlocksPlacement(
    const cr::CreativeObject& object) noexcept {
  return object.visible &&
         descriptorBlocksPlacement(cr::describeObject(object.kind));
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

[[nodiscard]] ClearanceBox objectClearanceBox(
    const cr::CreativeObject& object) noexcept {
  cr::CreativeTransform transform = object.transform;
  if (!cr::objectHasTransform(object.kind)) {
    transform = {};
  }
  return clearanceBox(object.bounds, transform);
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
    const CreativeBrushPlacementPlan& plan,
    const ClearanceBox& candidate,
    const cr::CreativeObject& object,
    cr::CreativePlacementClearanceResult& result) noexcept {
  if (!objectBlocksPlacement(object) ||
      (plan.hasAttachment && object.id == plan.attachmentTargetId)) {
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
    const CreativeBrushPlacementPlan& plan,
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
              overlapsObject(plan, candidate, *object, result)) {
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
    if (overlapsObject(plan, candidate, object, result)) {
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
    if (!objectBlocksPlacement(object)) {
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
    const CreativePlacementClearanceCache* cache) noexcept {
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
    if (authoredObjectBlocks(document, plan, candidate, cache, result) ||
        voxelBlocks(document, candidate, result) ||
        terrainBlocks(document, candidate, result)) {
      return result;
    }
  }

  result.status = cr::CreativePlacementClearanceStatus::Ready;
  result.allowed = true;
  return result;
}

void applyCreativeBrushPlacementClearance(
    CreativeBrushPlacementAdmission& admission,
    const cr::CreativeDocument& document,
    const CreativePlacementClearanceCache* cache) noexcept {
  if (!admission.allowed || !admission.plan.valid) {
    return;
  }
  admission.plan.clearance = evaluateCreativeBrushPlacementClearance(
      document, admission.plan, cache);
  if (!admission.plan.clearance.allowed) {
    admission.allowed = false;
    admission.status =
        CreativeBrushPlacementAdmissionStatus::ClearanceBlocked;
  }
}

}  // namespace iggy3d_creative_app
