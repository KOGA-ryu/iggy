#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"

#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace iggy3d::creative {
namespace {

void reject(CreativeWorldLayoutRoofPlan& plan,
            CreativeWorldLayoutRoofStatus status,
            std::string_view reasonCode) noexcept {
  plan.status = status;
  plan.reasonCode = reasonCode;
}

[[nodiscard]] bool validRect(CreativeWorldLayoutRect rect) noexcept {
  return rect.minimum.x < rect.maximum.x &&
         rect.minimum.z < rect.maximum.z;
}

[[nodiscard]] bool rectArea(CreativeWorldLayoutRect rect,
                            std::uint64_t& output) noexcept {
  if (!validRect(rect)) {
    return false;
  }
  const std::uint64_t width = static_cast<std::uint64_t>(
      static_cast<std::int64_t>(rect.maximum.x) - rect.minimum.x);
  const std::uint64_t depth = static_cast<std::uint64_t>(
      static_cast<std::int64_t>(rect.maximum.z) - rect.minimum.z);
  if (width != 0U && depth > std::numeric_limits<std::uint64_t>::max() / width) {
    return false;
  }
  output = width * depth;
  return true;
}

[[nodiscard]] bool worldCoordinate(double origin,
                                   double cellSize,
                                   long double coordinate,
                                   double& output) noexcept {
  const long double value = static_cast<long double>(origin) +
                            static_cast<long double>(cellSize) * coordinate;
  if (!std::isfinite(value) ||
      value < -std::numeric_limits<double>::max() ||
      value > std::numeric_limits<double>::max()) {
    return false;
  }
  output = static_cast<double>(value);
  return std::isfinite(output);
}

}  // namespace

bool creativeWorldLayoutLevelRoofFootprint(
    const CreativeWorldLayout& layout,
    std::size_t levelIndex,
    CreativeWorldLayoutRect& output) {
  if (levelIndex >= layout.levels.size()) {
    return false;
  }
  const CreativeWorldLayoutRoomGraph graph =
      buildCreativeWorldLayoutRoomGraph(layout);
  if (!graph.accepted) {
    return false;
  }
  bool found = false;
  std::uint64_t coveredArea = 0U;
  for (std::size_t index = 0U; index < layout.rooms.size(); ++index) {
    const CreativeWorldLayoutRoom& room = layout.rooms[index];
    if (room.levelIndex != levelIndex) {
      continue;
    }
    if (room.buildingIndex != layout.levels[levelIndex].buildingIndex ||
        index >= graph.roomBounds.size()) {
      return false;
    }
    const CreativeWorldLayoutRect roomBounds = graph.roomBounds[index];
    const std::span<const CreativeWorldLayoutRect> surfaceRects =
        creativeWorldLayoutRoomSurfaceRects(graph, index);
    if (surfaceRects.empty()) {
      return false;
    }
    for (const CreativeWorldLayoutRect surfaceRect : surfaceRects) {
      std::uint64_t area = 0U;
      if (!rectArea(surfaceRect, area) ||
          area > std::numeric_limits<std::uint64_t>::max() - coveredArea) {
        return false;
      }
      coveredArea += area;
    }
    if (!found) {
      output = roomBounds;
      found = true;
    } else {
      output.minimum.x = std::min(output.minimum.x, roomBounds.minimum.x);
      output.minimum.z = std::min(output.minimum.z, roomBounds.minimum.z);
      output.maximum.x = std::max(output.maximum.x, roomBounds.maximum.x);
      output.maximum.z = std::max(output.maximum.z, roomBounds.maximum.z);
    }
  }
  std::uint64_t boundingArea = 0U;
  return found && rectArea(output, boundingArea) && coveredArea == boundingArea;
}

CreativeWorldLayoutRoofPlan planCreativeWorldLayoutRoof(
    const CreativeGridSettings& grid,
    const CreativeWorldLayout& layout,
    std::size_t levelIndex) {
  CreativeWorldLayoutRoofPlan plan;
  plan.levelIndex = levelIndex;
  if (levelIndex >= layout.levels.size()) {
    reject(plan, CreativeWorldLayoutRoofStatus::InvalidLevel,
           "creative_world_layout_roof_level_invalid");
    return plan;
  }
  const CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
  plan.buildingIndex = level.buildingIndex;
  if (level.buildingIndex >= layout.buildings.size()) {
    reject(plan, CreativeWorldLayoutRoofStatus::InvalidLevel,
           "creative_world_layout_roof_owner_invalid");
    return plan;
  }
  if (!creativeWorldLayoutLevelIsTopmostOccupied(layout, levelIndex)) {
    reject(plan, CreativeWorldLayoutRoofStatus::NotTopmost,
           "creative_world_layout_roof_level_not_topmost");
    return plan;
  }
  if (!creativeWorldLayoutLevelRoofFootprint(layout, levelIndex,
                                             plan.footprint)) {
    reject(plan, CreativeWorldLayoutRoofStatus::NonRectangularFootprint,
           "creative_world_layout_roof_footprint_not_rectangular");
    return plan;
  }
  if (!std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0) {
    reject(plan, CreativeWorldLayoutRoofStatus::InvalidGrid,
           "creative_world_layout_roof_grid_invalid");
    return plan;
  }
  if (!validCreativeStructuralRoofSettings(
          level.roofStyle, level.roofRidgeAxis, level.roofSlopeDirection,
          level.roofPitchDegrees, level.roofOverhangCells,
          level.roofMaterial) ||
      level.roofOverhangCells >
          kMaximumCreativeWorldLayoutRoofOverhangCells) {
    reject(plan, CreativeWorldLayoutRoofStatus::InvalidSettings,
           "creative_world_layout_roof_settings_invalid");
    return plan;
  }

  CreativeStructuralRoofRecipeRequest request;
  request.style = level.roofStyle;
  request.ridgeAxis = level.roofRidgeAxis;
  request.slopeDirection = level.roofSlopeDirection;
  request.layerCount = level.roofThicknessLayers;
  request.pitchDegrees = level.roofPitchDegrees;
  request.overhangMeters = level.roofOverhangCells * grid.cellSizeMeters;
  request.material = level.roofMaterial;
  if (!worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                       plan.footprint.minimum.x, request.minimumX) ||
      !worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                       plan.footprint.maximum.x, request.maximumX) ||
      !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                       plan.footprint.minimum.z, request.minimumZ) ||
      !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                       plan.footprint.maximum.z, request.maximumZ) ||
      !worldCoordinate(
          grid.origin.y, grid.cellSizeMeters,
          static_cast<long double>(level.floorTopLayer) +
              level.wallHeightCells,
          request.supportPlaneMeters)) {
    reject(plan, CreativeWorldLayoutRoofStatus::InvalidFootprint,
           "creative_world_layout_roof_coordinates_invalid");
    return plan;
  }
  CreativeStructuralRoofApertureRequest closureRequest;
  closureRequest.roof = request;
  closureRequest.minimumClearanceMeters = 0.1 * grid.cellSizeMeters;
  for (std::size_t apertureIndex = 0U;
       apertureIndex < layout.roofApertures.size(); ++apertureIndex) {
    const CreativeWorldLayoutRoofAperture& source =
        layout.roofApertures[apertureIndex];
    if (source.levelIndex != levelIndex) {
      continue;
    }
    if (plan.sourceApertureCount >= plan.sourceApertureIndices.size()) {
      reject(plan, CreativeWorldLayoutRoofStatus::RecipeRejected,
             "creative_world_layout_roof_aperture_capacity_exceeded");
      return plan;
    }
    CreativeStructuralRoofAperture& aperture =
        closureRequest.apertures[plan.sourceApertureCount];
    aperture.kind = source.kind;
    if (!worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                         source.minimumXCells, aperture.minimumX) ||
        !worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                         source.maximumXCells, aperture.maximumX) ||
        !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                         source.minimumZCells, aperture.minimumZ) ||
        !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                         source.maximumZCells, aperture.maximumZ)) {
      reject(plan, CreativeWorldLayoutRoofStatus::InvalidFootprint,
             "creative_world_layout_roof_aperture_coordinates_invalid");
      return plan;
    }
    plan.sourceApertureIndices[plan.sourceApertureCount] = apertureIndex;
    ++plan.sourceApertureCount;
  }
  closureRequest.apertureCount = plan.sourceApertureCount;
  plan.closure = planCreativeStructuralRoofApertures(closureRequest);
  plan.geometry = plan.closure.roof;
  if (!plan.closure.accepted) {
    reject(plan, CreativeWorldLayoutRoofStatus::RecipeRejected,
           plan.closure.reasonCode);
    return plan;
  }
  plan.accepted = true;
  plan.status = CreativeWorldLayoutRoofStatus::Ready;
  plan.reasonCode = "creative_world_layout_roof_ready";
  return plan;
}

}  // namespace iggy3d::creative
