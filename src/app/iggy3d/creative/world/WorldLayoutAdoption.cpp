#include "app/iggy3d/creative/world/WorldLayoutAdoption.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/recipes/CreativeRecipe.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include <cmath>
#include <limits>
#include <string>

namespace iggy3d::creative {
namespace {

constexpr double kInverseEpsilon = 1.0e-6;
constexpr std::string_view kSourceTagPrefix =
    "creative_world_layout_source:";
constexpr std::string_view kRoomEdgeTagPrefix =
    "creative_world_layout_room_edge:";

void reject(CreativeWorldLayoutAdoptionResult& result,
            CreativeWorldLayoutAdoptionStatus status,
            std::string_view reasonCode) noexcept {
  result.status = status;
  result.reasonCode = reasonCode;
}

[[nodiscard]] bool finiteGrid(CreativeGridSettings grid) noexcept {
  return std::isfinite(grid.origin.x) && std::isfinite(grid.origin.y) &&
         std::isfinite(grid.origin.z) &&
         std::isfinite(grid.cellSizeMeters) && grid.cellSizeMeters > 0.0;
}

[[nodiscard]] bool nearZero(double value) noexcept {
  return std::isfinite(value) && std::fabs(value) <= kInverseEpsilon;
}

[[nodiscard]] bool worldToCell(double world, double origin, double cellSize,
                               double& output) noexcept {
  output = (world - origin) / cellSize;
  return std::isfinite(output);
}

[[nodiscard]] bool worldToIntegerCell(double world, double origin,
                                      double cellSize,
                                      std::int32_t& output) noexcept {
  double cells = 0.0;
  if (!worldToCell(world, origin, cellSize, cells)) {
    return false;
  }
  const double rounded = std::round(cells);
  if (std::fabs(cells - rounded) > kInverseEpsilon ||
      rounded < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      rounded > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
    return false;
  }
  output = static_cast<std::int32_t>(rounded);
  return true;
}

[[nodiscard]] bool thicknessToLayerCount(double thickness,
                                         double layerThickness,
                                         std::uint16_t& output) noexcept {
  if (!std::isfinite(thickness) || thickness <= 0.0 ||
      !std::isfinite(layerThickness) || layerThickness <= 0.0) {
    return false;
  }
  const double layers = thickness / layerThickness;
  const double rounded = std::round(layers);
  if (std::fabs(layers - rounded) > kInverseEpsilon || rounded < 1.0 ||
      rounded > static_cast<double>(
                    std::numeric_limits<std::uint16_t>::max())) {
    return false;
  }
  output = static_cast<std::uint16_t>(rounded);
  return true;
}

[[nodiscard]] bool objectStateIsRepresentable(
    const CreativeObject& object, bool expectsParent) noexcept {
  return object.id != kInvalidObjectId && !object.locked &&
         object.parentId.has_value() == expectsParent &&
         object.attachmentSocket.empty() &&
         object.pathPoints.empty() &&
         object.movingPlatform == CreativeMovingPlatformSettings{};
}

[[nodiscard]] bool sourceTagsMatch(const CreativeWorldLayout& layout,
                                   std::span<const std::string> sourceTags,
                                   std::span<const std::string> objectTags) {
  const std::string layoutTag = creativeWorldLayoutTag(layout.stableKey);
  std::size_t sourceIndex = 0U;
  for (const std::string& tag : objectTags) {
    if (tag == layoutTag || isCreativeRecipeManagementTag(tag) ||
        tag.starts_with(kSourceTagPrefix) ||
        tag.starts_with(kRoomEdgeTagPrefix)) {
      continue;
    }
    if (sourceIndex >= sourceTags.size() || sourceTags[sourceIndex] != tag) {
      return false;
    }
    ++sourceIndex;
  }
  return sourceIndex == sourceTags.size();
}

[[nodiscard]] bool sameVec(CreativeVec3 lhs, CreativeVec3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

[[nodiscard]] bool sameBounds(CreativeBounds lhs,
                              CreativeBounds rhs) noexcept {
  return sameVec(lhs.min, rhs.min) && sameVec(lhs.max, rhs.max);
}

[[nodiscard]] CreativeBounds offsetBounds(CreativeBounds bounds,
                                          CreativeVec3 offset) noexcept {
  bounds.min.x += offset.x;
  bounds.min.y += offset.y;
  bounds.min.z += offset.z;
  bounds.max.x += offset.x;
  bounds.max.y += offset.y;
  bounds.max.z += offset.z;
  return bounds;
}

[[nodiscard]] CreativeBounds localizeBounds(CreativeBounds bounds,
                                            CreativeVec3 origin) noexcept {
  return offsetBounds(bounds, {-origin.x, -origin.y, -origin.z});
}

[[nodiscard]] bool adoptDirectObject(
    CreativeWorldLayoutAdoptionResult& result,
    const CreativeObject& object,
    CreativeGridSettings grid) {
  CreativeWorldLayoutObject& source = result.candidate.objects[result.index];
  if (source.kind != object.kind || source.assetId != object.assetId ||
      object.layerId != describeObject(source.kind).defaults.layerId ||
      !objectStateIsRepresentable(object, false) ||
      !sourceTagsMatch(result.candidate, source.tags, object.tags)) {
    reject(result, CreativeWorldLayoutAdoptionStatus::ObjectMismatch,
           "creative_world_layout_adoption_object_state_unsupported");
    return false;
  }

  const CreativeWorldLayoutObject previous = source;
  source.name = object.name;
  source.visible = object.visible;
  source.playerSpawn = object.playerSpawn;
  source.npcSpawn = object.npcSpawn;
  source.lootPoint = object.lootPoint;
  source.exitPoint = object.exitPoint;
  if (source.mode == CreativeObjectLibraryPlacementMode::Point) {
    if (!nearZero(object.transform.rotationEulerRadians.x) ||
        !nearZero(object.transform.rotationEulerRadians.z) ||
        !std::isfinite(object.transform.rotationEulerRadians.y) ||
        !isPositiveCreativeVec3(object.transform.scale) ||
        !worldToCell(object.transform.position.x, grid.origin.x,
                     grid.cellSizeMeters, source.pointCells.x) ||
        !worldToCell(object.transform.position.y, grid.origin.y,
                     grid.cellSizeMeters, source.pointCells.y) ||
        !worldToCell(object.transform.position.z, grid.origin.z,
                     grid.cellSizeMeters, source.pointCells.z)) {
      reject(result,
             CreativeWorldLayoutAdoptionStatus::NonInvertibleTransform,
             "creative_world_layout_adoption_point_transform_unsupported");
      return false;
    }
    source.yawRadians = object.transform.rotationEulerRadians.y;
    source.scale = object.transform.scale;
    const CreativeBounds generatedBounds =
        source.hasAssetSourceBounds
            ? offsetBounds(source.assetSourceBoundsMeters,
                           object.transform.position)
            : describeObject(source.kind).defaults.bounds;
    if (!sameBounds(generatedBounds, object.bounds)) {
      const CreativeBoundsMetrics liveBounds = measureCreativeBounds(object.bounds);
      if (!objectHasBounds(source.kind) || !liveBounds.valid) {
        reject(result,
               CreativeWorldLayoutAdoptionStatus::UnrepresentableGeometry,
               "creative_world_layout_adoption_point_bounds_unsupported");
        return false;
      }
      source.assetSourceBoundsMeters =
          localizeBounds(object.bounds, object.transform.position);
      source.hasAssetSourceBounds = true;
    }
    result.changed = previous.name != source.name ||
                     previous.visible != source.visible ||
                     !(previous.playerSpawn == source.playerSpawn) ||
                     !(previous.npcSpawn == source.npcSpawn) ||
                     !(previous.lootPoint == source.lootPoint) ||
                     !(previous.exitPoint == source.exitPoint) ||
                     !sameVec(previous.pointCells, source.pointCells) ||
                     previous.yawRadians != source.yawRadians ||
                     !sameVec(previous.scale, source.scale) ||
                     previous.hasAssetSourceBounds !=
                         source.hasAssetSourceBounds ||
                     !sameBounds(previous.assetSourceBoundsMeters,
                                 source.assetSourceBoundsMeters);
    result.mode = CreativeWorldLayoutAdoptionMode::Exact;
    return true;
  }

  if (source.mode != CreativeObjectLibraryPlacementMode::Bounds ||
      !nearZero(object.transform.rotationEulerRadians.x) ||
      !nearZero(object.transform.rotationEulerRadians.y) ||
      !nearZero(object.transform.rotationEulerRadians.z)) {
    reject(result,
           CreativeWorldLayoutAdoptionStatus::NonInvertibleTransform,
           "creative_world_layout_adoption_bounds_transform_unsupported");
    return false;
  }
  const CreativeTransformedBounds resolved = resolveCreativeObjectBounds(object);
  if (!resolved.valid ||
      !worldToCell(resolved.worldBounds.min.x, grid.origin.x,
                   grid.cellSizeMeters, source.boundsCells.min.x) ||
      !worldToCell(resolved.worldBounds.min.y, grid.origin.y,
                   grid.cellSizeMeters, source.boundsCells.min.y) ||
      !worldToCell(resolved.worldBounds.min.z, grid.origin.z,
                   grid.cellSizeMeters, source.boundsCells.min.z) ||
      !worldToCell(resolved.worldBounds.max.x, grid.origin.x,
                   grid.cellSizeMeters, source.boundsCells.max.x) ||
      !worldToCell(resolved.worldBounds.max.y, grid.origin.y,
                   grid.cellSizeMeters, source.boundsCells.max.y) ||
      !worldToCell(resolved.worldBounds.max.z, grid.origin.z,
                   grid.cellSizeMeters, source.boundsCells.max.z)) {
    reject(result,
           CreativeWorldLayoutAdoptionStatus::UnrepresentableGeometry,
           "creative_world_layout_adoption_bounds_unrepresentable");
    return false;
  }
  result.changed = previous.name != source.name ||
                   previous.visible != source.visible ||
                   !(previous.playerSpawn == source.playerSpawn) ||
                   !(previous.npcSpawn == source.npcSpawn) ||
                   !(previous.lootPoint == source.lootPoint) ||
                   !(previous.exitPoint == source.exitPoint) ||
                   !sameBounds(previous.boundsCells, source.boundsCells);
  result.mode = CreativeWorldLayoutAdoptionMode::Canonicalized;
  return true;
}

[[nodiscard]] bool adoptBox(CreativeWorldLayoutAdoptionResult& result,
                            const CreativeObject& object,
                            CreativeGridSettings grid) {
  CreativeWorldLayoutBox& source = result.candidate.boxes[result.index];
  if (source.buildingIndex >= result.candidate.buildings.size()) {
    reject(result, CreativeWorldLayoutAdoptionStatus::ObjectMismatch,
           "creative_world_layout_adoption_box_owner_invalid");
    return false;
  }
  const CreativeWorldLayoutBuilding& building =
      result.candidate.buildings[source.buildingIndex];
  const bool expectsParent =
      building.rootMode == CreativeBuildingRootMode::CreateRoom;
  if (source.kind != object.kind || !object.assetId.empty() ||
      object.visible != building.visible ||
      object.layerId != describeObject(source.kind).defaults.layerId ||
      !objectStateIsRepresentable(object, expectsParent) ||
      !sourceTagsMatch(result.candidate, building.tags, object.tags) ||
      (!objectHasTransform(source.kind) &&
       (!sameVec(object.transform.position, {}) ||
        !sameVec(object.transform.scale, {1.0, 1.0, 1.0}))) ||
      !nearZero(object.transform.rotationEulerRadians.x) ||
      !nearZero(object.transform.rotationEulerRadians.y) ||
      !nearZero(object.transform.rotationEulerRadians.z)) {
    reject(result, CreativeWorldLayoutAdoptionStatus::ObjectMismatch,
           "creative_world_layout_adoption_box_state_unsupported");
    return false;
  }

  const CreativeTransformedBounds resolved = resolveCreativeObjectBounds(object);
  CreativeWorldLayoutRect footprint;
  if (!resolved.valid ||
      !worldToIntegerCell(resolved.worldBounds.min.x, grid.origin.x,
                          grid.cellSizeMeters, footprint.minimum.x) ||
      !worldToIntegerCell(resolved.worldBounds.min.z, grid.origin.z,
                          grid.cellSizeMeters, footprint.minimum.z) ||
      !worldToIntegerCell(resolved.worldBounds.max.x, grid.origin.x,
                          grid.cellSizeMeters, footprint.maximum.x) ||
      !worldToIntegerCell(resolved.worldBounds.max.z, grid.origin.z,
                          grid.cellSizeMeters, footprint.maximum.z) ||
      footprint.minimum.x >= footprint.maximum.x ||
      footprint.minimum.z >= footprint.maximum.z) {
    reject(result,
           CreativeWorldLayoutAdoptionStatus::UnrepresentableGeometry,
           "creative_world_layout_adoption_box_footprint_unrepresentable");
    return false;
  }

  const CreativeStructuralSurfaceAnchor anchor =
      creativeStructuralSurfaceAnchor(source.kind);
  const double anchorMeters =
      anchor == CreativeStructuralSurfaceAnchor::TopPlane
          ? resolved.worldBounds.max.y
          : resolved.worldBounds.min.y;
  const double layerThickness =
      anchor == CreativeStructuralSurfaceAnchor::None
          ? grid.cellSizeMeters
          : defaultCreativeStructuralLayerThicknessMeters(source.kind);
  double anchorLayer = 0.0;
  std::uint16_t layerCount = 0U;
  if (!worldToCell(anchorMeters, grid.origin.y, grid.cellSizeMeters,
                   anchorLayer) ||
      !thicknessToLayerCount(resolved.size.y, layerThickness, layerCount)) {
    reject(result,
           CreativeWorldLayoutAdoptionStatus::UnrepresentableGeometry,
           "creative_world_layout_adoption_box_height_unrepresentable");
    return false;
  }

  result.changed = source.name != object.name ||
                   source.footprint.minimum != footprint.minimum ||
                   source.footprint.maximum != footprint.maximum ||
                   source.anchorLayer != anchorLayer ||
                   source.layerCount != layerCount;
  source.name = object.name;
  source.footprint = footprint;
  source.anchorLayer = anchorLayer;
  source.layerCount = layerCount;
  result.mode = CreativeWorldLayoutAdoptionMode::Canonicalized;
  return true;
}

}  // namespace

CreativeWorldLayoutAdoptionResult planCreativeWorldLayoutObjectAdoption(
    const CreativeWorldLayout& layout,
    const CreativeObject& object,
    CreativeGridSettings grid) {
  CreativeWorldLayoutAdoptionResult result;
  if (!finiteGrid(grid)) {
    reject(result, CreativeWorldLayoutAdoptionStatus::InvalidGrid,
           "creative_world_layout_adoption_grid_invalid");
    return result;
  }

  const CreativeWorldLayoutObjectProvenance provenance =
      resolveCreativeWorldLayoutObjectProvenance(layout, object);
  result.table = provenance.table;
  result.index = provenance.index;
  if (!provenance.owned ||
      provenance.index == kInvalidCreativeWorldLayoutIndex) {
    reject(result, CreativeWorldLayoutAdoptionStatus::SourceMissing,
           "creative_world_layout_adoption_source_missing");
    return result;
  }
  if (provenance.contributorCount != 1U ||
      (provenance.table != CreativeWorldLayoutTable::Object &&
       provenance.table != CreativeWorldLayoutTable::Box)) {
    reject(result, CreativeWorldLayoutAdoptionStatus::UnsupportedSource,
           "creative_world_layout_adoption_source_unsupported");
    return result;
  }

  result.candidate = layout;
  const bool planned = provenance.table == CreativeWorldLayoutTable::Object
                           ? adoptDirectObject(result, object, grid)
                           : adoptBox(result, object, grid);
  if (!planned) {
    result.candidate = {};
    return result;
  }
  result.accepted = true;
  result.status = CreativeWorldLayoutAdoptionStatus::Ready;
  result.reasonCode = result.changed
                          ? "creative_world_layout_adoption_ready"
                          : "creative_world_layout_adoption_no_change";
  return result;
}

}  // namespace iggy3d::creative
