#include "app/iggy3d/creative/world/WorldLayoutPlanProjectionInternal.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <span>

namespace iggy3d::creative::plan_projection_internal {
namespace {

bool finitePositiveScale(CreativeVec3 scale) noexcept {
  return isFiniteCreativeVec3(scale) && scale.x > 0.0 && scale.y > 0.0 &&
         scale.z > 0.0;
}

bool objectFootprint(const CreativeWorldLayoutObject& object,
                     CreativeGridSettings grid,
                     std::array<Point, 4U>& corners,
                     CreativeBounds& resolvedBounds) noexcept {
  if (object.mode == CreativeObjectLibraryPlacementMode::Bounds) {
    const CreativeBoundsMetrics measured =
        measureCreativeBounds(object.boundsCells);
    if (!measured.valid || !isPositiveCreativeVec3(measured.size)) {
      return false;
    }
    resolvedBounds = object.boundsCells;
    corners = {{{object.boundsCells.min.x, object.boundsCells.min.z},
                {object.boundsCells.max.x, object.boundsCells.min.z},
                {object.boundsCells.max.x, object.boundsCells.max.z},
                {object.boundsCells.min.x, object.boundsCells.max.z}}};
    return true;
  }
  if (object.mode != CreativeObjectLibraryPlacementMode::Point ||
      !object.hasAssetSourceBounds || !std::isfinite(grid.cellSizeMeters) ||
      grid.cellSizeMeters <= 0.0 || !isFiniteCreativeVec3(object.pointCells) ||
      !std::isfinite(object.yawRadians) || !finitePositiveScale(object.scale)) {
    return false;
  }
  const CreativeBoundsMetrics sourceBounds =
      measureCreativeBounds(object.assetSourceBoundsMeters);
  if (!sourceBounds.valid || !isPositiveCreativeVec3(sourceBounds.size)) {
    return false;
  }
  const double inverseCell = 1.0 / grid.cellSizeMeters;
  const CreativeBounds authoredCells{
      {object.pointCells.x + object.assetSourceBoundsMeters.min.x * inverseCell,
       object.pointCells.y + object.assetSourceBoundsMeters.min.y * inverseCell,
       object.pointCells.z + object.assetSourceBoundsMeters.min.z * inverseCell},
      {object.pointCells.x + object.assetSourceBoundsMeters.max.x * inverseCell,
       object.pointCells.y + object.assetSourceBoundsMeters.max.y * inverseCell,
       object.pointCells.z + object.assetSourceBoundsMeters.max.z * inverseCell},
  };
  CreativeTransform transform;
  transform.position = object.pointCells;
  transform.rotationEulerRadians.y = object.yawRadians;
  transform.scale = object.scale;
  const CreativeTransformedBounds transformed =
      resolveCreativeTransformedBounds(authoredCells, transform);
  if (!transformed.valid) {
    return false;
  }
  constexpr std::array<std::size_t, 4U> kPlanCornerIndices{0U, 1U, 5U, 4U};
  for (std::size_t index = 0U; index < corners.size(); ++index) {
    corners[index] = {transformed.corners[kPlanCornerIndices[index]].x,
                      transformed.corners[kPlanCornerIndices[index]].z};
  }
  resolvedBounds = transformed.worldBounds;
  return true;
}

Role objectRole(CreativeObjectKind kind) noexcept {
  if (kind == CreativeObjectKind::Bridge) {
    return Role::Bridge;
  }
  if (kind == CreativeObjectKind::SpawnPoint) {
    return Role::PlayerSpawn;
  }
  if (kind == CreativeObjectKind::NpcSpawn) {
    return Role::NpcSpawn;
  }
  return Role::Object;
}

bool objectInVerticalBand(const CreativeWorldLayoutObject& object,
                          const CreativeBounds* resolvedBounds,
                          bool haveActiveLevels, double bottom,
                          double top) noexcept {
  if (!haveActiveLevels) {
    return true;
  }
  if (resolvedBounds != nullptr) {
    return resolvedBounds->max.y >= bottom - kGeometryEpsilon &&
           resolvedBounds->min.y < top - kGeometryEpsilon;
  }
  return object.pointCells.y >= bottom - kGeometryEpsilon &&
         object.pointCells.y < top - kGeometryEpsilon;
}

}  // namespace

bool projectTerrain(Projection& projection,
                    const CreativeWorldLayout& layout,
                    std::span<const CreativeTerrainContourSegment> contours) {
  for (std::size_t index = 0U; index < layout.terrainProfiles.size(); ++index) {
    const CreativeWorldLayoutTerrainProfile& profile =
        layout.terrainProfiles[index];
    if (profile.kind >= CreativeTerrainRecipeKind::Count ||
        profile.radiusCells == 0U) {
      return false;
    }
    Primitive primitive;
    primitive.role = Role::TerrainProfile;
    primitive.kind = PrimitiveKind::Circle;
    primitive.layer = Layer::Active;
    primitive.source = source(CreativeWorldLayoutTable::TerrainProfile, index);
    primitive.points[0] = point(profile.center);
    primitive.pointCount = 1U;
    primitive.radiusCells = static_cast<double>(profile.radiusCells);
    primitive.terrainKind = profile.kind;
    if (!appendPrimitive(projection, primitive)) {
      return false;
    }
    ++projection.receipt.terrainPrimitiveCount;
  }

  for (std::size_t pathIndex = 0U; pathIndex < layout.terrainPaths.size();
       ++pathIndex) {
    const CreativeWorldLayoutTerrainPath& path = layout.terrainPaths[pathIndex];
    if (path.kind >= CreativeTerrainRecipeKind::Count || path.pointCount < 2U ||
        path.firstPointIndex > layout.terrainPathPoints.size() ||
        path.pointCount >
            layout.terrainPathPoints.size() - path.firstPointIndex) {
      return false;
    }
    const double widthCells =
        static_cast<double>(path.halfWidthCells) * 2.0 + 1.0;
    for (std::size_t offset = 1U; offset < path.pointCount; ++offset) {
      const std::size_t pointIndex = path.firstPointIndex + offset;
      Primitive primitive = segment(
          Role::TerrainPath, Layer::Active,
          source(CreativeWorldLayoutTable::TerrainPath, pathIndex),
          point(layout.terrainPathPoints[pointIndex - 1U].coord),
          point(layout.terrainPathPoints[pointIndex].coord));
      primitive.widthCells = widthCells;
      primitive.terrainKind = path.kind;
      if (!appendPrimitive(projection, primitive)) {
        return false;
      }
      ++projection.receipt.terrainPrimitiveCount;
    }
  }

  for (std::size_t index = 0U; index < contours.size(); ++index) {
    const CreativeTerrainContourSegment& contour = contours[index];
    Primitive primitive = segment(
        Role::Contour, Layer::Active,
        source(CreativeWorldLayoutTable::None, index),
        {contour.start.x, contour.start.z}, {contour.end.x, contour.end.z});
    primitive.contourMajor = contour.major;
    if (!appendPrimitive(projection, primitive)) {
      return false;
    }
    ++projection.receipt.contourPrimitiveCount;
  }
  return true;
}

bool projectObjects(Projection& projection, const CreativeWorldLayout& layout,
                    CreativeGridSettings grid, bool haveActiveLevels,
                    double activeFloorTop, double activeBandTop) {
  for (std::size_t index = 0U; index < layout.objects.size(); ++index) {
    const CreativeWorldLayoutObject& object = layout.objects[index];
    if (!object.visible) {
      continue;
    }
    if (object.kind <= CreativeObjectKind::Unknown ||
        object.kind >= CreativeObjectKind::Count ||
        object.mode >= CreativeObjectLibraryPlacementMode::Count ||
        !isFiniteCreativeVec3(object.pointCells)) {
      return false;
    }

    std::array<Point, 4U> corners{};
    CreativeBounds resolvedBounds;
    const bool hasFootprint =
        objectFootprint(object, grid, corners, resolvedBounds);
    const CreativeBounds* verticalBounds =
        hasFootprint ? &resolvedBounds : nullptr;
    if (!objectInVerticalBand(object, verticalBounds, haveActiveLevels,
                              activeFloorTop, activeBandTop)) {
      continue;
    }
    const Role role = objectRole(object.kind);
    const SourceRef objectSource =
        source(CreativeWorldLayoutTable::Object, index);
    Primitive primitive;
    if (hasFootprint) {
      primitive = polygon(role, Layer::Active, objectSource, corners);
    } else {
      if (object.mode != CreativeObjectLibraryPlacementMode::Point ||
          object.hasAssetSourceBounds) {
        return false;
      }
      primitive.role = role;
      primitive.kind = PrimitiveKind::Point;
      primitive.layer = Layer::Active;
      primitive.source = objectSource;
      primitive.points[0] = {object.pointCells.x, object.pointCells.z};
      primitive.pointCount = 1U;
    }
    primitive.objectKind = object.kind;
    if (!appendPrimitive(projection, primitive)) {
      return false;
    }
    ++projection.receipt.objectPrimitiveCount;
  }
  return true;
}

}  // namespace iggy3d::creative::plan_projection_internal
