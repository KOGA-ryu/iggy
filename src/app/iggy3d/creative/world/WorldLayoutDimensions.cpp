#include "app/iggy3d/creative/world/WorldLayoutDimensions.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOpenings.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace iggy3d::creative {
namespace {

constexpr double kDimensionEpsilonMeters = 1.0e-6;
constexpr double kDimensionEpsilonLayers = 1.0e-9;

bool validGrid(const CreativeGridSettings& grid) noexcept {
  return std::isfinite(grid.origin.x) && std::isfinite(grid.origin.y) &&
         std::isfinite(grid.origin.z) &&
         std::isfinite(grid.cellSizeMeters) && grid.cellSizeMeters > 0.0;
}

bool worldCoordinate(double origin,
                     double cellSizeMeters,
                     double cells,
                     double& output) noexcept {
  if (!std::isfinite(origin) || !std::isfinite(cellSizeMeters) ||
      !std::isfinite(cells)) {
    return false;
  }
  const long double value = static_cast<long double>(origin) +
                            static_cast<long double>(cellSizeMeters) * cells;
  if (!std::isfinite(value) ||
      value < -static_cast<long double>(std::numeric_limits<double>::max()) ||
      value > static_cast<long double>(std::numeric_limits<double>::max())) {
    return false;
  }
  output = static_cast<double>(value);
  return std::isfinite(output);
}

bool scaledLength(double cellSizeMeters,
                  double cells,
                  double& output) noexcept {
  return worldCoordinate(0.0, cellSizeMeters, cells, output);
}

bool levelOccupied(const CreativeWorldLayout& layout,
                   std::size_t levelIndex) noexcept {
  return creativeWorldLayoutLevelHasRooms(layout, levelIndex);
}

void reject(CreativeWorldLayoutLevelDimensions& result,
            CreativeWorldLayoutDimensionStatus status,
            std::string_view reasonCode) noexcept {
  result.status = status;
  result.reasonCode = reasonCode;
}

void reject(CreativeWorldLayoutBuildingDimensions& result,
            CreativeWorldLayoutDimensionStatus status,
            std::string_view reasonCode) noexcept {
  result.status = status;
  result.reasonCode = reasonCode;
}

void reject(CreativeWorldLayoutOpeningDimensions& result,
            CreativeWorldLayoutDimensionStatus status,
            std::string_view reasonCode) noexcept {
  result.status = status;
  result.reasonCode = reasonCode;
}

}  // namespace

CreativeWorldLayoutLevelDimensions measureCreativeWorldLayoutLevelDimensions(
    const CreativeGridSettings& grid,
    const CreativeWorldLayout& layout,
    std::size_t levelIndex) noexcept {
  CreativeWorldLayoutLevelDimensions result;
  result.levelIndex = levelIndex;
  if (!validGrid(grid)) {
    reject(result, CreativeWorldLayoutDimensionStatus::InvalidGrid,
           "creative_world_layout_dimensions_grid_invalid");
    return result;
  }
  if (levelIndex >= layout.levels.size() ||
      !levelOccupied(layout, levelIndex)) {
    reject(result, CreativeWorldLayoutDimensionStatus::InvalidLevel,
           "creative_world_layout_dimensions_level_invalid");
    return result;
  }

  const CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
  if (level.buildingIndex >= layout.buildings.size() ||
      !std::isfinite(level.floorTopLayer) || level.wallHeightCells == 0U ||
      level.floorThicknessLayers == 0U ||
      level.ceilingThicknessLayers == 0U ||
      level.roofThicknessLayers == 0U ||
      !validCreativeStructuralRoofSettings(
          level.roofStyle, level.roofRidgeAxis, level.roofSlopeDirection,
          level.roofPitchDegrees, level.roofOverhangCells,
          level.roofMaterial) ||
      level.roofOverhangCells >
          kMaximumCreativeWorldLayoutRoofOverhangCells) {
    reject(result, CreativeWorldLayoutDimensionStatus::InvalidLevel,
           "creative_world_layout_dimensions_level_invalid");
    return result;
  }

  result.buildingIndex = level.buildingIndex;
  result.topmostOccupied =
      creativeWorldLayoutLevelIsTopmostOccupied(layout, levelIndex);
  result.upperSurfaceKind = result.topmostOccupied
                                ? CreativeObjectKind::Roof
                                : CreativeObjectKind::Ceiling;
  result.floorThicknessLayers = level.floorThicknessLayers;
  result.upperSurfaceThicknessLayers =
      result.topmostOccupied ? level.roofThicknessLayers
                             : level.ceilingThicknessLayers;
  result.floorThicknessMeters =
      static_cast<double>(result.floorThicknessLayers) *
      defaultCreativeStructuralLayerThicknessMeters(CreativeObjectKind::Floor);
  if (!worldCoordinate(grid.origin.y, grid.cellSizeMeters,
                       level.floorTopLayer, result.floorTopMeters) ||
      !scaledLength(grid.cellSizeMeters,
                    static_cast<double>(level.wallHeightCells),
                    result.wallHeightMeters) ||
      !std::isfinite(result.floorThicknessMeters) ||
      result.floorThicknessMeters <= 0.0 ||
      result.wallHeightMeters <= 0.0) {
    reject(result, CreativeWorldLayoutDimensionStatus::Unrepresentable,
           "creative_world_layout_dimensions_level_unrepresentable");
    return result;
  }

  result.floorBottomMeters =
      result.floorTopMeters - result.floorThicknessMeters;
  result.wallBaseMeters = result.floorTopMeters;
  result.wallTopMeters = result.wallBaseMeters + result.wallHeightMeters;

  result.upperSurfaceThicknessMeters =
      static_cast<double>(result.upperSurfaceThicknessLayers) *
      defaultCreativeStructuralLayerThicknessMeters(result.upperSurfaceKind);
  if (!std::isfinite(result.upperSurfaceThicknessMeters) ||
      result.upperSurfaceThicknessMeters <= 0.0) {
    reject(result, CreativeWorldLayoutDimensionStatus::Unrepresentable,
           "creative_world_layout_dimensions_level_unrepresentable");
    return result;
  }

  if (result.topmostOccupied) {
    result.clearHeightMeters = result.wallHeightMeters;
    result.upperSurfaceSupportMeters = result.wallTopMeters;
    result.upperSurfaceTopMeters = result.upperSurfaceSupportMeters +
                                   result.upperSurfaceThicknessMeters;
  } else {
    double nearestHigherLayer = std::numeric_limits<double>::infinity();
    for (std::size_t candidateIndex = 0U;
         candidateIndex < layout.levels.size(); ++candidateIndex) {
      const CreativeWorldLayoutLevel& candidate =
          layout.levels[candidateIndex];
      if (candidate.buildingIndex != level.buildingIndex ||
          !levelOccupied(layout, candidateIndex) ||
          candidate.floorTopLayer <= level.floorTopLayer ||
          candidate.floorTopLayer >= nearestHigherLayer) {
        continue;
      }
      nearestHigherLayer = candidate.floorTopLayer;
      result.upperLevelIndex = candidateIndex;
    }
    if (result.upperLevelIndex == kInvalidCreativeWorldLayoutIndex) {
      reject(result, CreativeWorldLayoutDimensionStatus::InvalidLevel,
             "creative_world_layout_dimensions_upper_level_invalid");
      return result;
    }

    const CreativeWorldLayoutLevel& upperLevel =
        layout.levels[result.upperLevelIndex];
    const double upperFloorThicknessMeters =
        static_cast<double>(upperLevel.floorThicknessLayers) *
        defaultCreativeStructuralLayerThicknessMeters(
            CreativeObjectKind::Floor);
    if (upperLevel.floorThicknessLayers == 0U ||
        !worldCoordinate(grid.origin.y, grid.cellSizeMeters,
                         upperLevel.floorTopLayer,
                         result.nextFloorTopMeters) ||
        !std::isfinite(upperFloorThicknessMeters) ||
        upperFloorThicknessMeters <= 0.0) {
      reject(result, CreativeWorldLayoutDimensionStatus::InvalidLevel,
             "creative_world_layout_dimensions_upper_level_invalid");
      return result;
    }

    result.hasUpperLevel = true;
    result.nextFloorBottomMeters =
        result.nextFloorTopMeters - upperFloorThicknessMeters;
    result.floorToFloorMeters =
        result.nextFloorTopMeters - result.floorTopMeters;
    result.upperSurfaceTopMeters = result.nextFloorBottomMeters;
    result.upperSurfaceSupportMeters =
        result.upperSurfaceTopMeters - result.upperSurfaceThicknessMeters;
    result.clearHeightMeters =
        result.upperSurfaceSupportMeters - result.floorTopMeters;
  }

  if (!std::isfinite(result.floorBottomMeters) ||
      !std::isfinite(result.wallTopMeters) ||
      !std::isfinite(result.upperSurfaceTopMeters) ||
      !std::isfinite(result.upperSurfaceSupportMeters) ||
      !std::isfinite(result.clearHeightMeters) ||
      result.floorBottomMeters >= result.floorTopMeters ||
      result.wallTopMeters <= result.wallBaseMeters ||
      result.upperSurfaceTopMeters <= result.upperSurfaceSupportMeters ||
      result.clearHeightMeters <= 0.0 ||
      (result.hasUpperLevel &&
       (!std::isfinite(result.nextFloorBottomMeters) ||
        !std::isfinite(result.nextFloorTopMeters) ||
        !std::isfinite(result.floorToFloorMeters) ||
        result.nextFloorBottomMeters >= result.nextFloorTopMeters ||
        result.floorToFloorMeters <= 0.0))) {
    reject(result, CreativeWorldLayoutDimensionStatus::Unrepresentable,
           "creative_world_layout_dimensions_level_unrepresentable");
    return result;
  }

  result.accepted = true;
  result.status = CreativeWorldLayoutDimensionStatus::Ready;
  result.reasonCode = "creative_world_layout_dimensions_level_ready";
  return result;
}

CreativeWorldLayoutBuildingDimensions
measureCreativeWorldLayoutBuildingDimensions(
    const CreativeGridSettings& grid,
    const CreativeWorldLayout& layout,
    std::size_t buildingIndex) noexcept {
  CreativeWorldLayoutBuildingDimensions result;
  result.buildingIndex = buildingIndex;
  if (!validGrid(grid)) {
    reject(result, CreativeWorldLayoutDimensionStatus::InvalidGrid,
           "creative_world_layout_dimensions_grid_invalid");
    return result;
  }
  if (buildingIndex >= layout.buildings.size()) {
    reject(result, CreativeWorldLayoutDimensionStatus::InvalidBuilding,
           "creative_world_layout_dimensions_building_invalid");
    return result;
  }
  for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
    if (layout.levels[index].buildingIndex != buildingIndex ||
        !levelOccupied(layout, index)) {
      continue;
    }
    for (std::size_t prior = 0U; prior < index; ++prior) {
      if (layout.levels[prior].buildingIndex == buildingIndex &&
          levelOccupied(layout, prior) &&
          std::fabs(layout.levels[index].floorTopLayer -
                    layout.levels[prior].floorTopLayer) <=
              kDimensionEpsilonLayers) {
        reject(result, CreativeWorldLayoutDimensionStatus::InvalidLevel,
               "creative_world_layout_dimensions_level_elevation_duplicate");
        return result;
      }
    }
  }

  CreativeWorldLayoutBuildingBounds footprint;
  if (!measureCreativeWorldLayoutBuildingBounds(layout, buildingIndex,
                                                footprint) ||
      !footprint.valid ||
      !worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                       footprint.minimum.x,
                       result.footprintMinimumXMeters) ||
      !worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                       footprint.maximum.x,
                       result.footprintMaximumXMeters) ||
      !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                       footprint.minimum.z,
                       result.footprintMinimumZMeters) ||
      !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                       footprint.maximum.z,
                       result.footprintMaximumZMeters)) {
    reject(result, CreativeWorldLayoutDimensionStatus::InvalidFootprint,
           "creative_world_layout_dimensions_footprint_invalid");
    return result;
  }
  result.footprintWidthMeters =
      result.footprintMaximumXMeters - result.footprintMinimumXMeters;
  result.footprintDepthMeters =
      result.footprintMaximumZMeters - result.footprintMinimumZMeters;
  if (!std::isfinite(result.footprintWidthMeters) ||
      !std::isfinite(result.footprintDepthMeters) ||
      result.footprintWidthMeters <= 0.0 ||
      result.footprintDepthMeters <= 0.0) {
    reject(result, CreativeWorldLayoutDimensionStatus::InvalidFootprint,
           "creative_world_layout_dimensions_footprint_invalid");
    return result;
  }

  result.lowestFloorBottomMeters = std::numeric_limits<double>::infinity();
  result.lowestFloorTopMeters = std::numeric_limits<double>::infinity();
  result.exteriorFacadeBaseMeters = std::numeric_limits<double>::infinity();
  result.exteriorFacadeTopMeters =
      -std::numeric_limits<double>::infinity();
  result.roofBaseMeters = -std::numeric_limits<double>::infinity();
  result.roofTopMeters = -std::numeric_limits<double>::infinity();
  result.minimumWallHeightMeters = std::numeric_limits<double>::infinity();
  result.maximumWallHeightMeters = 0.0;
  result.minimumFloorThicknessMeters =
      std::numeric_limits<double>::infinity();
  result.maximumFloorThicknessMeters = 0.0;
  result.minimumFloorToFloorMeters =
      std::numeric_limits<double>::infinity();
  result.maximumFloorToFloorMeters = 0.0;

  for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
    if (layout.levels[index].buildingIndex != buildingIndex ||
        !levelOccupied(layout, index)) {
      continue;
    }
    const CreativeWorldLayoutLevelDimensions level =
        measureCreativeWorldLayoutLevelDimensions(grid, layout, index);
    if (!level.accepted) {
      reject(result, level.status, level.reasonCode);
      return result;
    }
    ++result.occupiedLevelCount;
    result.lowestFloorBottomMeters =
        std::min(result.lowestFloorBottomMeters, level.floorBottomMeters);
    result.lowestFloorTopMeters =
        std::min(result.lowestFloorTopMeters, level.floorTopMeters);
    result.exteriorFacadeBaseMeters =
        std::min(result.exteriorFacadeBaseMeters, level.wallBaseMeters);
    result.exteriorFacadeTopMeters =
        std::max(result.exteriorFacadeTopMeters, level.wallTopMeters);
    result.minimumWallHeightMeters =
        std::min(result.minimumWallHeightMeters, level.wallHeightMeters);
    result.maximumWallHeightMeters =
        std::max(result.maximumWallHeightMeters, level.wallHeightMeters);
    result.minimumFloorThicknessMeters = std::min(
        result.minimumFloorThicknessMeters, level.floorThicknessMeters);
    result.maximumFloorThicknessMeters = std::max(
        result.maximumFloorThicknessMeters, level.floorThicknessMeters);
    if (level.hasUpperLevel) {
      result.minimumFloorToFloorMeters =
          std::min(result.minimumFloorToFloorMeters,
                   level.floorToFloorMeters);
      result.maximumFloorToFloorMeters =
          std::max(result.maximumFloorToFloorMeters,
                   level.floorToFloorMeters);
    }
    if (level.topmostOccupied) {
      result.roofBaseMeters = level.upperSurfaceSupportMeters;
      const CreativeWorldLayoutLevel& sourceLevel = layout.levels[index];
      const bool authoredRoof =
          sourceLevel.roofStyle != CreativeStructuralRoofStyle::Flat ||
          sourceLevel.roofOverhangCells > 0.0;
      if (authoredRoof) {
        const CreativeWorldLayoutRoofPlan roof =
            planCreativeWorldLayoutRoof(grid, layout, index);
        if (!roof.accepted || !roof.geometry.accepted) {
          reject(result, CreativeWorldLayoutDimensionStatus::InvalidRoof,
                 "creative_world_layout_dimensions_roof_invalid");
          return result;
        }
        result.roofTopMeters = roof.geometry.worldBounds.max.y;
      } else {
        result.roofTopMeters = level.upperSurfaceTopMeters;
      }
    }
  }
  if (result.occupiedLevelCount == 0U ||
      !std::isfinite(result.roofBaseMeters) ||
      !std::isfinite(result.roofTopMeters)) {
    reject(result, CreativeWorldLayoutDimensionStatus::EmptyBuilding,
           "creative_world_layout_dimensions_building_empty");
    return result;
  }

  if (!std::isfinite(result.minimumFloorToFloorMeters)) {
    result.minimumFloorToFloorMeters = 0.0;
  }

  result.exteriorFacadeHeightMeters =
      result.exteriorFacadeTopMeters - result.exteriorFacadeBaseMeters;
  result.totalHeightMeters =
      std::max(result.exteriorFacadeTopMeters, result.roofTopMeters) -
      result.lowestFloorBottomMeters;
  result.uniformFloorToFloor =
      result.maximumFloorToFloorMeters - result.minimumFloorToFloorMeters <=
      kDimensionEpsilonMeters;
  result.uniformWallHeight =
      result.maximumWallHeightMeters - result.minimumWallHeightMeters <=
      kDimensionEpsilonMeters;
  result.uniformFloorThickness =
      result.maximumFloorThicknessMeters -
          result.minimumFloorThicknessMeters <=
      kDimensionEpsilonMeters;
  if (!std::isfinite(result.exteriorFacadeHeightMeters) ||
      !std::isfinite(result.totalHeightMeters) ||
      result.exteriorFacadeHeightMeters <= 0.0 ||
      result.totalHeightMeters <= 0.0) {
    reject(result, CreativeWorldLayoutDimensionStatus::Unrepresentable,
           "creative_world_layout_dimensions_building_unrepresentable");
    return result;
  }

  result.accepted = true;
  result.status = CreativeWorldLayoutDimensionStatus::Ready;
  result.reasonCode = "creative_world_layout_dimensions_building_ready";
  return result;
}

CreativeWorldLayoutOpeningDimensions
measureCreativeWorldLayoutOpeningDimensions(
    const CreativeGridSettings& grid,
    const CreativeWorldLayout& layout,
    std::size_t openingIndex) noexcept {
  CreativeWorldLayoutOpeningDimensions result;
  result.openingIndex = openingIndex;
  if (!validGrid(grid)) {
    reject(result, CreativeWorldLayoutDimensionStatus::InvalidGrid,
           "creative_world_layout_dimensions_grid_invalid");
    return result;
  }
  if (openingIndex >= layout.openings.size()) {
    reject(result, CreativeWorldLayoutDimensionStatus::InvalidOpening,
           "creative_world_layout_dimensions_opening_invalid");
    return result;
  }
  const CreativeWorldLayoutOpening& opening = layout.openings[openingIndex];
  const CreativeWorldLayoutOpeningHostFrame host =
      resolveCreativeWorldLayoutOpeningHost(layout, opening);
  if (!host.accepted || host.buildingIndex >= layout.buildings.size()) {
    reject(result, CreativeWorldLayoutDimensionStatus::InvalidOpening,
           "creative_world_layout_dimensions_opening_host_invalid");
    return result;
  }
  double wallHeightMeters = 0.0;
  double wallThicknessMeters = 0.0;
  result.buildingIndex = host.buildingIndex;
  if (!worldCoordinate(grid.origin.y, grid.cellSizeMeters, host.baseLayer,
                       result.wallBaseMeters) ||
      !scaledLength(grid.cellSizeMeters, host.wallHeightCells,
                    wallHeightMeters) ||
      !scaledLength(grid.cellSizeMeters, host.wallThicknessCells,
                    wallThicknessMeters)) {
    reject(result, CreativeWorldLayoutDimensionStatus::InvalidOpening,
           "creative_world_layout_dimensions_opening_host_invalid");
    return result;
  }
  if (!std::isfinite(wallHeightMeters) || wallHeightMeters <= 0.0 ||
      !std::isfinite(wallThicknessMeters) || wallThicknessMeters <= 0.0) {
    reject(result, CreativeWorldLayoutDimensionStatus::InvalidOpening,
           "creative_world_layout_dimensions_opening_host_invalid");
    return result;
  }

  const double insertBottomCells =
      opening.kind == CreativeBuildingOpeningKind::Window &&
              opening.insertBottomCells == 0.0
          ? opening.cutoutBottomCells
          : opening.insertBottomCells;
  const double insertHeightCells =
      opening.insertHeightCells == 0.0 ? opening.cutoutHeightCells
                                       : opening.insertHeightCells;
  const double insertWidthCells =
      opening.insertWidthCells == 0.0 ? opening.widthCells
                                      : opening.insertWidthCells;
  bool insertThicknessValid = true;
  if (opening.insertThicknessCells == 0.0) {
    result.insertThicknessMeters = wallThicknessMeters;
  } else {
    insertThicknessValid =
        scaledLength(grid.cellSizeMeters, opening.insertThicknessCells,
                     result.insertThicknessMeters);
  }
  if ((opening.kind != CreativeBuildingOpeningKind::Door &&
       opening.kind != CreativeBuildingOpeningKind::Window) ||
      !scaledLength(grid.cellSizeMeters, opening.centerOffsetCells,
                    result.centerOffsetMeters) ||
      !scaledLength(grid.cellSizeMeters, opening.widthCells,
                    result.widthMeters) ||
      !scaledLength(grid.cellSizeMeters, opening.cutoutBottomCells,
                    result.cutoutBottomOffsetMeters) ||
      !scaledLength(grid.cellSizeMeters, opening.cutoutHeightCells,
                    result.cutoutHeightMeters) ||
      !scaledLength(grid.cellSizeMeters, insertBottomCells,
                    result.insertBottomOffsetMeters) ||
      !scaledLength(grid.cellSizeMeters, insertHeightCells,
                    result.insertHeightMeters) ||
      !scaledLength(grid.cellSizeMeters, insertWidthCells,
                    result.insertWidthMeters) ||
      !insertThicknessValid) {
    reject(result, CreativeWorldLayoutDimensionStatus::Unrepresentable,
           "creative_world_layout_dimensions_opening_unrepresentable");
    return result;
  }

  result.wallTopMeters = result.wallBaseMeters + wallHeightMeters;
  result.cutoutBottomMeters =
      result.wallBaseMeters + result.cutoutBottomOffsetMeters;
  result.cutoutTopMeters =
      result.cutoutBottomMeters + result.cutoutHeightMeters;
  result.insertBottomMeters =
      result.wallBaseMeters + result.insertBottomOffsetMeters;
  result.insertTopMeters =
      result.insertBottomMeters + result.insertHeightMeters;
  const bool insertValid =
      !opening.includeInsert ||
      (result.insertHeightMeters > 0.0 && result.insertWidthMeters > 0.0 &&
       result.insertThicknessMeters > 0.0 &&
       result.insertBottomMeters >=
           result.cutoutBottomMeters - kDimensionEpsilonMeters &&
       result.insertTopMeters <=
           result.cutoutTopMeters + kDimensionEpsilonMeters);
  if (!std::isfinite(result.wallTopMeters) || result.widthMeters <= 0.0 ||
      result.cutoutBottomOffsetMeters < 0.0 ||
      result.cutoutHeightMeters <= 0.0 ||
      result.cutoutTopMeters >
          result.wallTopMeters + kDimensionEpsilonMeters ||
      !insertValid) {
    reject(result, CreativeWorldLayoutDimensionStatus::InvalidOpening,
           "creative_world_layout_dimensions_opening_invalid");
    return result;
  }

  result.accepted = true;
  result.status = CreativeWorldLayoutDimensionStatus::Ready;
  result.reasonCode = "creative_world_layout_dimensions_opening_ready";
  return result;
}

}  // namespace iggy3d::creative
