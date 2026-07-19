#include "app/iggy3d/creative/adapters/RoomBakeInternal.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "content/assets/TraversalTag.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace iggy3d::creative::room_bake_internal {

[[nodiscard]] std::string stableVoxelId(
    const CreativeVoxelCuboid& cuboid,
    std::string_view suffix = {}) {
  std::string id = "creative_voxel_" +
                   std::to_string(cuboid.minCell.x) + "_" +
                   std::to_string(cuboid.minCell.y) + "_" +
                   std::to_string(cuboid.minCell.z) + "_" +
                   std::to_string(cuboid.maxCellExclusive.x) + "_" +
                   std::to_string(cuboid.maxCellExclusive.y) + "_" +
                   std::to_string(cuboid.maxCellExclusive.z) + "_" +
                   std::string(serializedObjectKindId(cuboid.material));
  if (!suffix.empty()) {
    id += "_";
    id += suffix;
  }
  return id;
}

[[nodiscard]] BakedRoomRole roleForVoxelMaterial(
    const CreativeObjectDescriptor& descriptor) noexcept {
  if (!descriptorSupportsRuntimeRoomGeometry(descriptor)) {
    return BakedRoomRole::Unsupported;
  }
  if (descriptor.shapeKind == CreativeObjectShapeKind::MeshProxy) {
    return BakedRoomRole::Prop;
  }
  if (descriptor.shapeKind == CreativeObjectShapeKind::BoxVolume) {
    return occupancySupportsRuntimeRoomGeometry(descriptor.occupancyKind)
               ? BakedRoomRole::Prop
               : BakedRoomRole::Unsupported;
  }
  if (descriptor.shapeKind != CreativeObjectShapeKind::Surface) {
    return BakedRoomRole::Unsupported;
  }

  const CreativeBoundsMetrics metrics =
      measureCreativeBounds(descriptor.defaults.bounds);
  const CreativeCoreVec3Conversion semanticSize =
      creativeVec3ToCoreChecked(metrics.size);
  return metrics.valid && semanticSize.converted
             ? roleForObject(descriptor, semanticSize.value)
             : BakedRoomRole::Unsupported;
}

[[nodiscard]] bool bakeBoundsForVoxelCuboid(
    const CreativeVoxelCuboid& cuboid,
    CreativeGridSettings grid,
    BakeBounds& bounds) noexcept {
  const CreativeBounds worldBounds{
      {grid.origin.x + static_cast<double>(cuboid.minCell.x) *
                           grid.cellSizeMeters,
       grid.origin.y + static_cast<double>(cuboid.minCell.y) *
                           grid.cellSizeMeters,
       grid.origin.z + static_cast<double>(cuboid.minCell.z) *
                           grid.cellSizeMeters},
      {grid.origin.x + static_cast<double>(cuboid.maxCellExclusive.x) *
                           grid.cellSizeMeters,
       grid.origin.y + static_cast<double>(cuboid.maxCellExclusive.y) *
                           grid.cellSizeMeters,
       grid.origin.z + static_cast<double>(cuboid.maxCellExclusive.z) *
                           grid.cellSizeMeters}};
  return validBakeBounds(worldBounds, bounds);
}

void appendVoxelCuboid(CreativeRoomBakeResult& result,
                       const CreativeVoxelCuboid& cuboid,
                       CreativeGridSettings grid,
                       bool smoothTerrainCollision) {
  const CreativeObjectDescriptor& descriptor = describeObject(cuboid.material);
  const BakedRoomRole role = roleForVoxelMaterial(descriptor);
  BakeBounds bounds;
  if (role == BakedRoomRole::Unsupported ||
      !bakeBoundsForVoxelCuboid(cuboid, grid, bounds)) {
    ++result.receipt.skippedUnsupportedShapeCount;
    return;
  }

  const std::string stableId = stableVoxelId(cuboid);
  RoomStaticMeshAsset mesh;
  mesh.id = stableId;
  mesh.meshId = std::string(meshIdForRole(role));
  mesh.materialId = std::string(materialIdForRole(role));
  mesh.role = cuboid.material == CreativeObjectKind::TerrainPatch
                  ? "terrain"
                  : std::string(roleName(role));
  mesh.positionMeters = bounds.center;
  mesh.sizeMeters = bounds.size;
  if (role == BakedRoomRole::Wall) {
    setWallSegmentFields(mesh, bounds);
  }
  result.room.staticMeshes.push_back(std::move(mesh));
  ++result.receipt.bakedVoxelCuboidCount;

  if (cuboid.material == CreativeObjectKind::TerrainPatch &&
      smoothTerrainCollision) {
    return;
  }

  if (role == BakedRoomRole::Floor) {
    RoomSpatialSurface surface =
        walkableSurfaceForStableId(stableId, bounds);
    appendSpatialSurfaceSource(result.spatialSurfaceSources,
                               kInvalidObjectId, surface);
    result.room.spatialSurfaces.push_back(std::move(surface));
    if (cuboid.material == CreativeObjectKind::TerrainPatch) {
      const Vec3 normal = blockerNormalForRole(bounds, BakedRoomRole::Prop);
      RoomSpatialSurface actorSurface =
          blockerSurfaceForStableId(stableId, bounds, normal, false);
      appendSpatialSurfaceSource(result.spatialSurfaceSources,
                                 kInvalidObjectId, actorSurface);
      result.room.spatialSurfaces.push_back(std::move(actorSurface));
      RoomSpatialSurface projectileSurface =
          blockerSurfaceForStableId(stableId, bounds, normal, true);
      appendSpatialSurfaceSource(result.spatialSurfaceSources,
                                 kInvalidObjectId, projectileSurface);
      result.room.spatialSurfaces.push_back(std::move(projectileSurface));
    }
    return;
  }

  if (descriptor.occupancyKind == CreativeSpatialOccupancyKind::Structural ||
      descriptor.occupancyKind == CreativeSpatialOccupancyKind::Collision) {
    const Vec3 normal = blockerNormalForRole(bounds, role);
    RoomSpatialSurface actorSurface =
        blockerSurfaceForStableId(stableId, bounds, normal, false);
    appendSpatialSurfaceSource(result.spatialSurfaceSources,
                               kInvalidObjectId, actorSurface);
    result.room.spatialSurfaces.push_back(std::move(actorSurface));

    RoomSpatialSurface projectileSurface =
        blockerSurfaceForStableId(stableId, bounds, normal, true);
    appendSpatialSurfaceSource(result.spatialSurfaceSources,
                               kInvalidObjectId, projectileSurface);
    result.room.spatialSurfaces.push_back(std::move(projectileSurface));
  }
}

[[nodiscard]] bool terrainPatchCoordLess(CreativeTerrainCoord2 lhs,
                                         CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z < rhs.z || (lhs.z == rhs.z && lhs.x < rhs.x);
}

[[nodiscard]] bool validTerrainSurfacePatches(
    std::span<const CreativeTerrainSurfacePatch> patches) noexcept {
  if (patches.empty() ||
      patches.size() > kCreativeTerrainRenderPatchCapacity) {
    return false;
  }
  for (std::size_t index = 0U; index < patches.size(); ++index) {
    const CreativeTerrainSurfacePatch& patch = patches[index];
    const CreativeCoreVec3Conversion center =
        creativeVec3ToCoreChecked(patch.center);
    if (!center.converted) {
      return false;
    }
    std::array<Vec3, 4U> corners;
    for (std::size_t cornerIndex = 0U;
         cornerIndex < patch.corners.size(); ++cornerIndex) {
      const CreativeCoreVec3Conversion corner =
          creativeVec3ToCoreChecked(patch.corners[cornerIndex]);
      if (!corner.converted) {
        return false;
      }
      corners[cornerIndex] = corner.value;
    }
    float winding = 0.0F;
    for (std::size_t cornerIndex = 0U; cornerIndex < corners.size();
         ++cornerIndex) {
      const Vec3 edgeNormal =
          cross(corners[cornerIndex] - center.value,
                corners[(cornerIndex + 1U) % corners.size()] - center.value);
      if (!isFinite(edgeNormal) || std::fabs(edgeNormal.y) <= 0.0001F) {
        return false;
      }
      if (cornerIndex == 0U) {
        winding = edgeNormal.y;
      } else if ((winding < 0.0F) != (edgeNormal.y < 0.0F)) {
        return false;
      }
    }
    if (index > 0U &&
        !terrainPatchCoordLess(patches[index - 1U].coord, patch.coord)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] const CreativeTerrainSurfacePatch* findTerrainSurfacePatch(
    std::span<const CreativeTerrainSurfacePatch> patches,
    CreativeTerrainCoord2 coord) noexcept {
  const auto found = std::lower_bound(
      patches.begin(), patches.end(), coord,
      [](const CreativeTerrainSurfacePatch& patch,
         CreativeTerrainCoord2 candidate) {
        return terrainPatchCoordLess(patch.coord, candidate);
      });
  return found != patches.end() && found->coord == coord ? &*found : nullptr;
}

[[nodiscard]] std::string stableTerrainPatchId(
    CreativeTerrainCoord2 coord) {
  return "creative_terrain_" + std::to_string(coord.x) + "_" +
         std::to_string(coord.z);
}

void appendTerrainHeightPatch(
    CreativeRoomBakeResult& result,
    const CreativeTerrainSurfacePatch& patch) {
  RoomSpatialSurface surface;
  surface.id = stableTerrainPatchId(patch.coord) + "_height_patch";
  surface.shape = RoomSpatialSurfaceShape::HeightPatch;
  surface.role = RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters.reserve(5U);
  surface.pointsMeters.push_back(
      creativeVec3ToCoreChecked(patch.center).value);
  for (const CreativeVec3 corner : patch.corners) {
    surface.pointsMeters.push_back(creativeVec3ToCoreChecked(corner).value);
  }
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {
      std::string(traversalTagId(TraversalTag::Walkable))};
  surface.collisionMask = {"actor", "projectile"};
  appendSpatialSurfaceSource(result.spatialSurfaceSources, kInvalidObjectId,
                             surface);
  result.room.spatialSurfaces.push_back(std::move(surface));
  ++result.receipt.bakedTerrainSurfacePatchCount;
}

[[nodiscard]] RoomSpatialSurface terrainCliffSurface(
    std::string id,
    BakeBounds bounds,
    Vec3 normal,
    bool projectile) {
  RoomSpatialSurface surface;
  surface.id = std::move(id) +
               (projectile ? "_projectile_blocker" : "_actor_blocker");
  surface.shape = RoomSpatialSurfaceShape::Box;
  surface.role = projectile ? RoomSpatialSurfaceRole::ProjectileBlocker
                            : RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = boxExtentPoints(bounds);
  surface.normal = normal;
  surface.traversalTags = {
      std::string(traversalTagId(projectile ? TraversalTag::ProjectileBlocker
                                            : TraversalTag::Blocker))};
  surface.collisionMask = {projectile ? "projectile" : "actor"};
  surface.blocksActor = !projectile;
  surface.blocksProjectile = projectile;
  return surface;
}

void appendTerrainCliffBlocker(CreativeRoomBakeResult& result,
                               std::string id,
                               Vec3 first,
                               Vec3 second,
                               Vec3 normal,
                               float bottomY,
                               float thicknessMeters) {
  BakeBounds bounds;
  bounds.min = {std::min(first.x, second.x), bottomY,
                std::min(first.z, second.z)};
  bounds.max = {std::max(first.x, second.x),
                std::max(first.y, second.y),
                std::max(first.z, second.z)};
  if (std::fabs(first.x - second.x) <= 0.0001F) {
    bounds.min.x -= thicknessMeters * 0.5F;
    bounds.max.x += thicknessMeters * 0.5F;
  } else {
    bounds.min.z -= thicknessMeters * 0.5F;
    bounds.max.z += thicknessMeters * 0.5F;
  }
  bounds.center = (bounds.min + bounds.max) * 0.5F;
  bounds.size = bounds.max - bounds.min;

  RoomSpatialSurface actor =
      terrainCliffSurface(id, bounds, normal, false);
  appendSpatialSurfaceSource(result.spatialSurfaceSources, kInvalidObjectId,
                             actor);
  result.room.spatialSurfaces.push_back(std::move(actor));
  RoomSpatialSurface projectile =
      terrainCliffSurface(std::move(id), bounds, normal, true);
  appendSpatialSurfaceSource(result.spatialSurfaceSources, kInvalidObjectId,
                             projectile);
  result.room.spatialSurfaces.push_back(std::move(projectile));
  result.receipt.bakedTerrainCliffBlockerCount += 2U;
}

void appendSmoothTerrainCollision(
    CreativeRoomBakeResult& result,
    std::span<const CreativeTerrainSurfacePatch> patches,
    CreativeGridSettings grid) {
  struct EdgeSpec {
    std::int32_t neighborX = 0;
    std::int32_t neighborZ = 0;
    std::size_t firstCorner = 0U;
    std::size_t secondCorner = 0U;
    Vec3 normal;
    std::string_view suffix;
  };
  constexpr std::array edges{
      EdgeSpec{0, -1, 0U, 1U, {0.0F, 0.0F, -1.0F}, "south"},
      EdgeSpec{1, 0, 1U, 2U, {1.0F, 0.0F, 0.0F}, "east"},
      EdgeSpec{0, 1, 2U, 3U, {0.0F, 0.0F, 1.0F}, "north"},
      EdgeSpec{-1, 0, 3U, 0U, {-1.0F, 0.0F, 0.0F}, "west"},
  };
  const float bottomY = creativeVec3ToCoreChecked(grid.origin).value.y;
  const float thickness = static_cast<float>(
      std::max(0.02, grid.cellSizeMeters * 0.04));
  for (const CreativeTerrainSurfacePatch& patch : patches) {
    appendTerrainHeightPatch(result, patch);
    for (const EdgeSpec& edge : edges) {
      const std::int64_t neighborX =
          static_cast<std::int64_t>(patch.coord.x) + edge.neighborX;
      const std::int64_t neighborZ =
          static_cast<std::int64_t>(patch.coord.z) + edge.neighborZ;
      const bool neighborRepresentable =
          neighborX >= std::numeric_limits<std::int32_t>::min() &&
          neighborX <= std::numeric_limits<std::int32_t>::max() &&
          neighborZ >= std::numeric_limits<std::int32_t>::min() &&
          neighborZ <= std::numeric_limits<std::int32_t>::max();
      if (neighborRepresentable &&
          findTerrainSurfacePatch(
              patches,
              {static_cast<std::int32_t>(neighborX),
               static_cast<std::int32_t>(neighborZ)}) != nullptr) {
        continue;
      }
      const Vec3 first =
          creativeVec3ToCoreChecked(patch.corners[edge.firstCorner]).value;
      const Vec3 second =
          creativeVec3ToCoreChecked(patch.corners[edge.secondCorner]).value;
      appendTerrainCliffBlocker(
          result, stableTerrainPatchId(patch.coord) + "_cliff_" +
                      std::string(edge.suffix),
          first, second, edge.normal, bottomY, thickness);
    }
  }
  result.receipt.usedSmoothTerrainCollision = true;
}

void appendRoomBakeFields(CreativeRoomBakeResult& result,
                          const CreativeRoomBakeRequest& request,
                          const CreativeDocument& document) {
  std::vector<CreativeVoxelCuboid> ownedVoxelCuboids;
  std::span<const CreativeVoxelCuboid> voxelCuboids =
      request.precomputedVoxelCuboids;
  CreativeTerrainSurfacePlan ownedTerrainSurface;
  bool terrainSurfaceBuilt = false;
  const auto ensureTerrainSurface = [&]()
      -> const CreativeTerrainSurfacePlan& {
    if (!terrainSurfaceBuilt) {
      ownedTerrainSurface = buildCreativeComposedTerrainSurfacePlan(
          document.terrainField(), document.terrainHeightField());
      terrainSurfaceBuilt = true;
    }
    return ownedTerrainSurface;
  };
  if (!request.usePrecomputedVoxelCuboids) {
    ownedVoxelCuboids = buildCreativeVoxelCuboids(document.voxelField());
    const CreativeTerrainSurfacePlan& terrain = ensureTerrainSurface();
    if (terrain.accepted) {
      ownedVoxelCuboids.insert(ownedVoxelCuboids.end(),
                               terrain.cuboids.begin(),
                               terrain.cuboids.end());
    }
    voxelCuboids = ownedVoxelCuboids;
  }
  const CreativeGridSettings grid = document.gridSettings();
  const CreativeCoreVec3Conversion gridOrigin =
      creativeVec3ToCoreChecked(grid.origin);
  std::vector<CreativeTerrainSurfacePatch> ownedTerrainSurfacePatches;
  std::span<const CreativeTerrainSurfacePatch> terrainSurfacePatches =
      request.precomputedTerrainSurfacePatches;
  bool useTerrainSurfacePatches =
      request.usePrecomputedTerrainSurfacePatches;
  if (!useTerrainSurfacePatches) {
    const CreativeTerrainSurfacePlan& terrain = ensureTerrainSurface();
    if (terrain.accepted) {
      const CreativeTerrainRenderPlan render =
          buildCreativeTerrainRenderPlan(
              terrain, document.terrainMaterialField(), grid.origin,
              grid.cellSizeMeters);
      if (render.accepted) {
        ownedTerrainSurfacePatches = render.patches;
        terrainSurfacePatches = ownedTerrainSurfacePatches;
        useTerrainSurfacePatches = true;
      }
    }
  }
  const bool smoothTerrainCollision =
      useTerrainSurfacePatches && !terrainSurfacePatches.empty() &&
      gridOrigin.converted &&
      std::isfinite(static_cast<float>(grid.cellSizeMeters)) &&
      static_cast<float>(grid.cellSizeMeters) > 0.0F &&
      validTerrainSurfacePatches(terrainSurfacePatches);
  for (const CreativeVoxelCuboid& cuboid : voxelCuboids) {
    appendVoxelCuboid(result, cuboid, grid, smoothTerrainCollision);
  }
  if (smoothTerrainCollision) {
    appendSmoothTerrainCollision(result, terrainSurfacePatches, grid);
  }
}

}  // namespace iggy3d::creative::room_bake_internal
