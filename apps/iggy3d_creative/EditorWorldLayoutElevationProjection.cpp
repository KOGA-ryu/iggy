#include "EditorWorldLayoutElevation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

#include "app/iggy3d/creative/recipes/StructuralSurfaceRecipe.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"
#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

namespace iggy3d_creative_app {
namespace {

struct LevelProjectionFacts {
  bool found = false;
  std::size_t representativeRoomIndex = cr::kInvalidCreativeWorldLayoutIndex;
  double minimumHorizontal = 0.0;
  double maximumHorizontal = 0.0;
};

struct OpeningHostFacts {
  bool valid = false;
  std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::size_t levelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  cr::CreativeTerrainCoord2 start;
  cr::CreativeTerrainCoord2 end;
  double lengthCells = 0.0;
  double baseLayer = 0.0;
  double heightCells = 0.0;
};

[[nodiscard]] bool finiteGrid(const cr::CreativeGridSettings& grid) noexcept {
  return std::isfinite(grid.origin.x) && std::isfinite(grid.origin.y) &&
         std::isfinite(grid.origin.z) &&
         std::isfinite(grid.cellSizeMeters) && grid.cellSizeMeters > 0.0;
}

[[nodiscard]] double horizontalCoordinate(
    CreativeEditorWorldLayoutElevationAxis axis,
    double x,
    double z) noexcept {
  return axis == CreativeEditorWorldLayoutElevationAxis::X ? x : z;
}

[[nodiscard]] double horizontalCoordinate(
    CreativeEditorWorldLayoutElevationAxis axis,
    cr::CreativeTerrainCoord2 point) noexcept {
  return horizontalCoordinate(axis, static_cast<double>(point.x),
                              static_cast<double>(point.z));
}

[[nodiscard]] double worldHorizontalToCells(
    const cr::CreativeGridSettings& grid,
    CreativeEditorWorldLayoutElevationAxis axis,
    double worldCoordinate) noexcept {
  const double origin =
      axis == CreativeEditorWorldLayoutElevationAxis::X ? grid.origin.x
                                                        : grid.origin.z;
  return (worldCoordinate - origin) / grid.cellSizeMeters;
}

[[nodiscard]] double worldVerticalToCells(
    const cr::CreativeGridSettings& grid, double worldCoordinate) noexcept {
  return (worldCoordinate - grid.origin.y) / grid.cellSizeMeters;
}

[[nodiscard]] double cellsToWorld(double origin,
                                  double cellSize,
                                  double coordinate) noexcept {
  return origin + coordinate * cellSize;
}

void extendBounds(CreativeEditorWorldLayoutElevationBounds& bounds,
                  double minimumHorizontal,
                  double maximumHorizontal,
                  double minimumVertical,
                  double maximumVertical) noexcept {
  if (!std::isfinite(minimumHorizontal) ||
      !std::isfinite(maximumHorizontal) ||
      !std::isfinite(minimumVertical) || !std::isfinite(maximumVertical)) {
    return;
  }
  if (minimumHorizontal > maximumHorizontal) {
    std::swap(minimumHorizontal, maximumHorizontal);
  }
  if (minimumVertical > maximumVertical) {
    std::swap(minimumVertical, maximumVertical);
  }
  if (!bounds.valid) {
    bounds = {true, minimumHorizontal, maximumHorizontal, minimumVertical,
              maximumVertical};
    return;
  }
  bounds.minimumHorizontal =
      std::min(bounds.minimumHorizontal, minimumHorizontal);
  bounds.maximumHorizontal =
      std::max(bounds.maximumHorizontal, maximumHorizontal);
  bounds.minimumVertical = std::min(bounds.minimumVertical, minimumVertical);
  bounds.maximumVertical = std::max(bounds.maximumVertical, maximumVertical);
}

void appendItem(CreativeEditorWorldLayoutElevationProjection& projection,
                CreativeEditorWorldLayoutElevationItem item) {
  extendBounds(projection.bounds, item.minimumHorizontal,
               item.maximumHorizontal, item.minimumVertical,
               item.maximumVertical);
  projection.items.push_back(item);
}

void appendLine(CreativeEditorWorldLayoutElevationProjection& projection,
                CreativeEditorWorldLayoutElevationLine line) {
  extendBounds(projection.bounds, line.start.horizontal, line.end.horizontal,
               line.start.vertical, line.end.vertical);
  projection.lines.push_back(line);
}

void reject(CreativeEditorWorldLayoutElevationProjection& projection,
            CreativeEditorWorldLayoutElevationStatus status,
            std::string_view reasonCode) noexcept {
  projection.accepted = false;
  projection.status = status;
  projection.reasonCode = reasonCode;
}

[[nodiscard]] bool roomWorldBounds(
    const cr::CreativeGridSettings& grid,
    const cr::CreativeWorldLayoutRoom& room,
    double& minimumX,
    double& maximumX,
    double& minimumZ,
    double& maximumZ) noexcept {
  minimumX = cellsToWorld(grid.origin.x, grid.cellSizeMeters,
                          static_cast<double>(room.footprint.minimum.x));
  maximumX = cellsToWorld(grid.origin.x, grid.cellSizeMeters,
                          static_cast<double>(room.footprint.maximum.x));
  minimumZ = cellsToWorld(grid.origin.z, grid.cellSizeMeters,
                          static_cast<double>(room.footprint.minimum.z));
  maximumZ = cellsToWorld(grid.origin.z, grid.cellSizeMeters,
                          static_cast<double>(room.footprint.maximum.z));
  return std::isfinite(minimumX) && std::isfinite(maximumX) &&
         std::isfinite(minimumZ) && std::isfinite(maximumZ) &&
         minimumX < maximumX && minimumZ < maximumZ;
}

[[nodiscard]] bool appendSurface(
    CreativeEditorWorldLayoutElevationProjection& projection,
    const cr::CreativeGridSettings& grid,
    const cr::CreativeWorldLayoutRoom& room,
    std::size_t roomIndex,
    std::size_t levelIndex,
    cr::CreativeObjectKind objectKind,
    CreativeEditorWorldLayoutElevationItemKind itemKind,
    double anchorLayer,
    std::uint16_t layerCount,
    std::string_view& reasonCode) {
  double minimumX = 0.0;
  double maximumX = 0.0;
  double minimumZ = 0.0;
  double maximumZ = 0.0;
  if (!roomWorldBounds(grid, room, minimumX, maximumX, minimumZ, maximumZ)) {
    reasonCode = "creative_editor_world_layout_elevation_room_bounds_invalid";
    return false;
  }
  const cr::CreativeStructuralSurfaceRecipeResult surface =
      cr::planCreativeStructuralSurface(
          {objectKind, minimumX, maximumX, minimumZ, maximumZ,
           cellsToWorld(grid.origin.y, grid.cellSizeMeters, anchorLayer),
           layerCount});
  if (!surface.accepted) {
    reasonCode = surface.reasonCode;
    return false;
  }
  const double minimumHorizontal = worldHorizontalToCells(
      grid, projection.axis,
      horizontalCoordinate(projection.axis, surface.bounds.min.x,
                           surface.bounds.min.z));
  const double maximumHorizontal = worldHorizontalToCells(
      grid, projection.axis,
      horizontalCoordinate(projection.axis, surface.bounds.max.x,
                           surface.bounds.max.z));
  appendItem(projection,
             {itemKind,
              CreativeEditorWorldLayoutElevationSourceKind::Room,
              roomIndex,
              levelIndex,
              minimumHorizontal,
              maximumHorizontal,
              worldVerticalToCells(grid, surface.bounds.min.y),
              worldVerticalToCells(grid, surface.bounds.max.y)});
  return true;
}

[[nodiscard]] bool appendBoxSurface(
    CreativeEditorWorldLayoutElevationProjection& projection,
    const cr::CreativeGridSettings& grid,
    const cr::CreativeWorldLayoutBox& box,
    std::size_t boxIndex,
    std::string_view& reasonCode) {
  const double minimumX = cellsToWorld(
      grid.origin.x, grid.cellSizeMeters, box.footprint.minimum.x);
  const double maximumX = cellsToWorld(
      grid.origin.x, grid.cellSizeMeters, box.footprint.maximum.x);
  const double minimumZ = cellsToWorld(
      grid.origin.z, grid.cellSizeMeters, box.footprint.minimum.z);
  const double maximumZ = cellsToWorld(
      grid.origin.z, grid.cellSizeMeters, box.footprint.maximum.z);
  const bool structural = box.kind == cr::CreativeObjectKind::Floor ||
                          box.kind == cr::CreativeObjectKind::Ceiling ||
                          box.kind == cr::CreativeObjectKind::Roof;
  if (!structural) {
    if (!std::isfinite(minimumX) || !std::isfinite(maximumX) ||
        !std::isfinite(minimumZ) || !std::isfinite(maximumZ) ||
        !std::isfinite(box.anchorLayer) || minimumX >= maximumX ||
        minimumZ >= maximumZ || box.layerCount == 0U) {
      reasonCode =
          "creative_editor_world_layout_elevation_box_bounds_invalid";
      return false;
    }
    appendItem(
        projection,
        {CreativeEditorWorldLayoutElevationItemKind::Volume,
         CreativeEditorWorldLayoutElevationSourceKind::Box,
         boxIndex,
         cr::kInvalidCreativeWorldLayoutIndex,
         horizontalCoordinate(projection.axis, box.footprint.minimum),
         horizontalCoordinate(projection.axis, box.footprint.maximum),
         box.anchorLayer,
         box.anchorLayer + static_cast<double>(box.layerCount)});
    return true;
  }
  const cr::CreativeStructuralSurfaceRecipeResult surface =
      cr::planCreativeStructuralSurface(
          {box.kind, minimumX, maximumX, minimumZ, maximumZ,
           cellsToWorld(grid.origin.y, grid.cellSizeMeters, box.anchorLayer),
           box.layerCount});
  if (!surface.accepted) {
    reasonCode = surface.reasonCode;
    return false;
  }
  CreativeEditorWorldLayoutElevationItemKind itemKind =
      CreativeEditorWorldLayoutElevationItemKind::FloorSlab;
  if (box.kind == cr::CreativeObjectKind::Ceiling) {
    itemKind = CreativeEditorWorldLayoutElevationItemKind::CeilingSlab;
  } else if (box.kind == cr::CreativeObjectKind::Roof) {
    itemKind = CreativeEditorWorldLayoutElevationItemKind::RoofBase;
  }
  appendItem(
      projection,
      {itemKind,
       CreativeEditorWorldLayoutElevationSourceKind::Box,
       boxIndex,
       cr::kInvalidCreativeWorldLayoutIndex,
       worldHorizontalToCells(
           grid, projection.axis,
           horizontalCoordinate(projection.axis, surface.bounds.min.x,
                                surface.bounds.min.z)),
       worldHorizontalToCells(
           grid, projection.axis,
           horizontalCoordinate(projection.axis, surface.bounds.max.x,
                                surface.bounds.max.z)),
       worldVerticalToCells(grid, surface.bounds.min.y),
       worldVerticalToCells(grid, surface.bounds.max.y)});
  return true;
}

[[nodiscard]] OpeningHostFacts resolveOpeningHost(
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeWorldLayoutOpening& opening) noexcept {
  OpeningHostFacts host;
  if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall) {
    if (opening.wallIndex >= layout.walls.size()) {
      return host;
    }
    const cr::CreativeWorldLayoutWall& wall = layout.walls[opening.wallIndex];
    host.buildingIndex = wall.buildingIndex;
    host.start = wall.start;
    host.end = wall.end;
    host.baseLayer = wall.baseLayer;
    host.heightCells = wall.heightCells;
  } else if (opening.hostKind ==
             cr::CreativeWorldLayoutOpeningHostKind::RoomEdge) {
    if (opening.roomIndex >= layout.rooms.size() ||
        opening.roomEdge >= cr::CreativeWorldLayoutRoomEdge::Count) {
      return host;
    }
    const cr::CreativeWorldLayoutRoom& room = layout.rooms[opening.roomIndex];
    if (room.levelIndex >= layout.levels.size()) {
      return host;
    }
    const cr::CreativeWorldLayoutLevel& level = layout.levels[room.levelIndex];
    if (level.buildingIndex != room.buildingIndex) {
      return host;
    }
    host.buildingIndex = room.buildingIndex;
    host.levelIndex = room.levelIndex;
    host.baseLayer = level.floorTopLayer;
    host.heightCells = level.wallHeightCells;
    switch (opening.roomEdge) {
      case cr::CreativeWorldLayoutRoomEdge::North:
        host.start = room.footprint.minimum;
        host.end = {room.footprint.maximum.x, room.footprint.minimum.z};
        break;
      case cr::CreativeWorldLayoutRoomEdge::East:
        host.start = {room.footprint.maximum.x, room.footprint.minimum.z};
        host.end = room.footprint.maximum;
        break;
      case cr::CreativeWorldLayoutRoomEdge::South:
        host.start = {room.footprint.minimum.x, room.footprint.maximum.z};
        host.end = room.footprint.maximum;
        break;
      case cr::CreativeWorldLayoutRoomEdge::West:
        host.start = room.footprint.minimum;
        host.end = {room.footprint.minimum.x, room.footprint.maximum.z};
        break;
      case cr::CreativeWorldLayoutRoomEdge::Count:
        return {};
    }
  } else {
    return host;
  }
  const double deltaX = static_cast<double>(host.end.x) - host.start.x;
  const double deltaZ = static_cast<double>(host.end.z) - host.start.z;
  host.lengthCells = std::hypot(deltaX, deltaZ);
  host.valid = host.buildingIndex < layout.buildings.size() &&
               std::isfinite(host.lengthCells) && host.lengthCells > 0.0 &&
               std::isfinite(host.baseLayer) &&
               std::isfinite(host.heightCells) && host.heightCells > 0.0;
  return host;
}

[[nodiscard]] bool roofApertureVerticalBounds(
    const cr::CreativeGridSettings& grid,
    const cr::CreativeWorldLayoutLevel& level,
    const cr::CreativeWorldLayoutRoofPlan& roof,
    const cr::CreativeWorldLayoutRoofAperture& aperture,
    std::size_t localApertureIndex,
    double& minimumVertical,
    double& maximumVertical) noexcept {
  if (aperture.kind == cr::CreativeStructuralRoofApertureKind::Skylight) {
    for (std::size_t insertIndex = 0U;
         insertIndex < roof.closure.insertCount; ++insertIndex) {
      const cr::CreativeStructuralRoofApertureInsertPlan& insert =
          roof.closure.inserts[insertIndex];
      if (insert.apertureIndex == localApertureIndex) {
        minimumVertical = worldVerticalToCells(grid, insert.bounds.min.y);
        maximumVertical = worldVerticalToCells(grid, insert.bounds.max.y);
        return std::isfinite(minimumVertical) &&
               std::isfinite(maximumVertical) &&
               minimumVertical < maximumVertical;
      }
    }
    return false;
  }
  if (level.roofStyle == cr::CreativeStructuralRoofStyle::Flat) {
    minimumVertical =
        worldVerticalToCells(grid, roof.geometry.worldBounds.min.y);
    maximumVertical =
        worldVerticalToCells(grid, roof.geometry.worldBounds.max.y);
    return std::isfinite(minimumVertical) &&
           std::isfinite(maximumVertical) &&
           minimumVertical < maximumVertical;
  }
  if (level.roofStyle == cr::CreativeStructuralRoofStyle::Hip ||
      level.roofStyle == cr::CreativeStructuralRoofStyle::Count ||
      roof.geometry.edgeCount == 0U) {
    return false;
  }

  const double minimumX = cellsToWorld(
      grid.origin.x, grid.cellSizeMeters,
      static_cast<double>(roof.footprint.minimum.x) -
          level.roofOverhangCells);
  const double maximumX = cellsToWorld(
      grid.origin.x, grid.cellSizeMeters,
      static_cast<double>(roof.footprint.maximum.x) +
          level.roofOverhangCells);
  const double minimumZ = cellsToWorld(
      grid.origin.z, grid.cellSizeMeters,
      static_cast<double>(roof.footprint.minimum.z) -
          level.roofOverhangCells);
  const double maximumZ = cellsToWorld(
      grid.origin.z, grid.cellSizeMeters,
      static_cast<double>(roof.footprint.maximum.z) +
          level.roofOverhangCells);
  const double apertureMinimumX = cellsToWorld(
      grid.origin.x, grid.cellSizeMeters, aperture.minimumXCells);
  const double apertureMaximumX = cellsToWorld(
      grid.origin.x, grid.cellSizeMeters, aperture.maximumXCells);
  const double apertureMinimumZ = cellsToWorld(
      grid.origin.z, grid.cellSizeMeters, aperture.minimumZCells);
  const double apertureMaximumZ = cellsToWorld(
      grid.origin.z, grid.cellSizeMeters, aperture.maximumZCells);
  const double support = roof.geometry.edges[0].startMeters.y;
  const double width = maximumX - minimumX;
  const double depth = maximumZ - minimumZ;
  const double run = level.roofStyle == cr::CreativeStructuralRoofStyle::Shed
                         ? (level.roofSlopeDirection ==
                                    cr::CreativeStructuralRoofSlopeDirection::
                                        PositiveX ||
                                level.roofSlopeDirection ==
                                    cr::CreativeStructuralRoofSlopeDirection::
                                        NegativeX
                                ? width
                                : depth)
                         : (level.roofRidgeAxis ==
                                    cr::CreativeStructuralRoofRidgeAxis::X
                                ? depth
                                : width) *
                               0.5;
  if (!std::isfinite(run) || run <= 0.0 ||
      !std::isfinite(roof.geometry.riseMeters)) {
    return false;
  }
  const double risePerMeter = roof.geometry.riseMeters / run;
  const auto roofHeight = [&](double x, double z) {
    if (level.roofStyle == cr::CreativeStructuralRoofStyle::Gable) {
      const double uphill =
          level.roofRidgeAxis == cr::CreativeStructuralRoofRidgeAxis::X
              ? std::min(z - minimumZ, maximumZ - z)
              : std::min(x - minimumX, maximumX - x);
      return support + uphill * risePerMeter;
    }
    switch (level.roofSlopeDirection) {
      case cr::CreativeStructuralRoofSlopeDirection::PositiveX:
        return support + (maximumX - x) * risePerMeter;
      case cr::CreativeStructuralRoofSlopeDirection::NegativeX:
        return support + (x - minimumX) * risePerMeter;
      case cr::CreativeStructuralRoofSlopeDirection::PositiveZ:
        return support + (maximumZ - z) * risePerMeter;
      case cr::CreativeStructuralRoofSlopeDirection::NegativeZ:
        return support + (z - minimumZ) * risePerMeter;
      case cr::CreativeStructuralRoofSlopeDirection::Count:
        break;
    }
    return std::numeric_limits<double>::quiet_NaN();
  };
  const std::array<double, 4U> heights{{
      roofHeight(apertureMinimumX, apertureMinimumZ),
      roofHeight(apertureMaximumX, apertureMinimumZ),
      roofHeight(apertureMaximumX, apertureMaximumZ),
      roofHeight(apertureMinimumX, apertureMaximumZ),
  }};
  if (!std::all_of(heights.begin(), heights.end(),
                   [](double value) { return std::isfinite(value); })) {
    return false;
  }
  const auto [minimum, maximum] =
      std::minmax_element(heights.begin(), heights.end());
  constexpr double kDegreesToRadians =
      0.01745329251994329576923690768489;
  const double verticalThickness =
      roof.geometry.thicknessMeters *
      std::cos(level.roofPitchDegrees * kDegreesToRadians);
  minimumVertical = worldVerticalToCells(grid, *minimum);
  maximumVertical =
      worldVerticalToCells(grid, *maximum + verticalThickness);
  return std::isfinite(minimumVertical) &&
         std::isfinite(maximumVertical) &&
         minimumVertical < maximumVertical;
}

[[nodiscard]] bool appendRoofApertures(
    CreativeEditorWorldLayoutElevationProjection& projection,
    const cr::CreativeGridSettings& grid,
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeWorldLayoutLevel& level,
    const cr::CreativeWorldLayoutRoofPlan& roof) {
  for (std::size_t localIndex = 0U;
       localIndex < roof.sourceApertureCount; ++localIndex) {
    const std::size_t sourceIndex = roof.sourceApertureIndices[localIndex];
    if (sourceIndex >= layout.roofApertures.size()) {
      return false;
    }
    const cr::CreativeWorldLayoutRoofAperture& aperture =
        layout.roofApertures[sourceIndex];
    double minimumVertical = 0.0;
    double maximumVertical = 0.0;
    if (!roofApertureVerticalBounds(grid, level, roof, aperture, localIndex,
                                    minimumVertical, maximumVertical)) {
      return false;
    }
    appendItem(
        projection,
        {aperture.kind == cr::CreativeStructuralRoofApertureKind::Skylight
             ? CreativeEditorWorldLayoutElevationItemKind::RoofSkylight
             : CreativeEditorWorldLayoutElevationItemKind::RoofClearance,
         CreativeEditorWorldLayoutElevationSourceKind::RoofAperture,
         sourceIndex,
         roof.levelIndex,
         horizontalCoordinate(projection.axis, aperture.minimumXCells,
                              aperture.minimumZCells),
         horizontalCoordinate(projection.axis, aperture.maximumXCells,
                              aperture.maximumZCells),
         minimumVertical,
         maximumVertical});
  }
  return true;
}

}  // namespace

CreativeEditorWorldLayoutElevationProjection
planCreativeEditorWorldLayoutElevation(
    const CreativeEditorWorldLayoutElevationRequest& request) {
  CreativeEditorWorldLayoutElevationProjection projection;
  projection.buildingIndex = request.buildingIndex;
  projection.axis = request.axis;
  if (!finiteGrid(request.grid)) {
    reject(projection, CreativeEditorWorldLayoutElevationStatus::InvalidGrid,
           "creative_editor_world_layout_elevation_grid_invalid");
    return projection;
  }
  if (request.axis >= CreativeEditorWorldLayoutElevationAxis::Count) {
    reject(projection, CreativeEditorWorldLayoutElevationStatus::InvalidGrid,
           "creative_editor_world_layout_elevation_axis_invalid");
    return projection;
  }
  if (request.layout == nullptr ||
      request.buildingIndex >= request.layout->buildings.size()) {
    reject(projection,
           CreativeEditorWorldLayoutElevationStatus::InvalidBuilding,
           "creative_editor_world_layout_elevation_building_invalid");
    return projection;
  }
  const cr::CreativeWorldLayout& layout = *request.layout;
  if (!cr::validCreativeWorldLayoutLevelOwnership(layout)) {
    reject(projection, CreativeEditorWorldLayoutElevationStatus::InvalidLayout,
           "creative_editor_world_layout_elevation_layout_invalid");
    return projection;
  }

  std::vector<LevelProjectionFacts> levelFacts(layout.levels.size());
  std::string_view recipeReason;
  for (std::size_t roomIndex = 0U; roomIndex < layout.rooms.size();
       ++roomIndex) {
    const cr::CreativeWorldLayoutRoom& room = layout.rooms[roomIndex];
    if (room.buildingIndex != request.buildingIndex ||
        room.levelIndex >= layout.levels.size()) {
      continue;
    }
    const cr::CreativeWorldLayoutLevel& level = layout.levels[room.levelIndex];
    LevelProjectionFacts& facts = levelFacts[room.levelIndex];
    const double minimumHorizontal = horizontalCoordinate(
        request.axis, room.footprint.minimum);
    const double maximumHorizontal = horizontalCoordinate(
        request.axis, room.footprint.maximum);
    if (!facts.found) {
      facts.found = true;
      facts.representativeRoomIndex = roomIndex;
      facts.minimumHorizontal = minimumHorizontal;
      facts.maximumHorizontal = maximumHorizontal;
    } else {
      facts.minimumHorizontal =
          std::min(facts.minimumHorizontal, minimumHorizontal);
      facts.maximumHorizontal =
          std::max(facts.maximumHorizontal, maximumHorizontal);
    }
    if (!appendSurface(projection, request.grid, room, roomIndex,
                       room.levelIndex, cr::CreativeObjectKind::Floor,
                       CreativeEditorWorldLayoutElevationItemKind::FloorSlab,
                       level.floorTopLayer, level.floorThicknessLayers,
                       recipeReason)) {
      reject(projection,
             CreativeEditorWorldLayoutElevationStatus::RecipeRejected,
             recipeReason);
      return projection;
    }
    appendItem(
        projection,
        {CreativeEditorWorldLayoutElevationItemKind::WallEnvelope,
         CreativeEditorWorldLayoutElevationSourceKind::Room,
         roomIndex,
         room.levelIndex,
         minimumHorizontal,
         maximumHorizontal,
         level.floorTopLayer,
         level.floorTopLayer + static_cast<double>(level.wallHeightCells)});
    if (!cr::creativeWorldLayoutLevelIsTopmostOccupied(layout,
                                                       room.levelIndex) &&
        !appendSurface(
            projection, request.grid, room, roomIndex, room.levelIndex,
            cr::CreativeObjectKind::Ceiling,
            CreativeEditorWorldLayoutElevationItemKind::CeilingSlab,
            level.floorTopLayer + static_cast<double>(level.wallHeightCells),
            level.ceilingThicknessLayers, recipeReason)) {
      reject(projection,
             CreativeEditorWorldLayoutElevationStatus::RecipeRejected,
             recipeReason);
      return projection;
    }
  }

  for (std::size_t levelIndex = 0U; levelIndex < layout.levels.size();
       ++levelIndex) {
    const LevelProjectionFacts& facts = levelFacts[levelIndex];
    if (!facts.found) {
      continue;
    }
    const cr::CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
    projection.handles.push_back(
        {CreativeEditorWorldLayoutElevationHandleKind::LevelFloor,
         CreativeEditorWorldLayoutElevationSourceKind::Room,
         facts.representativeRoomIndex,
         levelIndex,
         {facts.minimumHorizontal, level.floorTopLayer}});
    projection.handles.push_back(
        {CreativeEditorWorldLayoutElevationHandleKind::WallTop,
         CreativeEditorWorldLayoutElevationSourceKind::Room,
         facts.representativeRoomIndex,
         levelIndex,
         {facts.maximumHorizontal,
          level.floorTopLayer + static_cast<double>(level.wallHeightCells)}});
    if (!cr::creativeWorldLayoutLevelIsTopmostOccupied(layout, levelIndex)) {
      continue;
    }
    const cr::CreativeWorldLayoutRoofPlan roof =
        cr::planCreativeWorldLayoutRoof(request.grid, layout, levelIndex);
    if (!roof.accepted || roof.geometry.partCount == 0U) {
      reject(projection,
             CreativeEditorWorldLayoutElevationStatus::RecipeRejected,
             roof.reasonCode);
      return projection;
    }
    const cr::CreativeBounds& roofBounds = roof.geometry.worldBounds;
    appendItem(
        projection,
        {CreativeEditorWorldLayoutElevationItemKind::RoofBase,
         CreativeEditorWorldLayoutElevationSourceKind::Room,
         facts.representativeRoomIndex,
         levelIndex,
         worldHorizontalToCells(
             request.grid, request.axis,
             horizontalCoordinate(request.axis, roofBounds.min.x,
                                  roofBounds.min.z)),
         worldHorizontalToCells(
             request.grid, request.axis,
             horizontalCoordinate(request.axis, roofBounds.max.x,
                                  roofBounds.max.z)),
         worldVerticalToCells(request.grid, roofBounds.min.y),
         worldVerticalToCells(request.grid, roofBounds.max.y)});
    if (!appendRoofApertures(projection, request.grid, layout, level, roof)) {
      reject(projection,
             CreativeEditorWorldLayoutElevationStatus::RecipeRejected,
             "creative_editor_world_layout_elevation_roof_aperture_invalid");
      return projection;
    }
    if (level.roofStyle == cr::CreativeStructuralRoofStyle::Flat) {
      continue;
    }
    double minimumHorizontal = std::numeric_limits<double>::infinity();
    double maximumHorizontal = -std::numeric_limits<double>::infinity();
    for (std::size_t edgeIndex = 0U;
         edgeIndex < roof.geometry.edgeCount; ++edgeIndex) {
      const cr::CreativeStructuralRoofEdgePlan& edge =
          roof.geometry.edges[edgeIndex];
      const double start = worldHorizontalToCells(
          request.grid, request.axis,
          horizontalCoordinate(request.axis, edge.startMeters.x,
                               edge.startMeters.z));
      const double end = worldHorizontalToCells(
          request.grid, request.axis,
          horizontalCoordinate(request.axis, edge.endMeters.x,
                               edge.endMeters.z));
      minimumHorizontal = std::min({minimumHorizontal, start, end});
      maximumHorizontal = std::max({maximumHorizontal, start, end});
    }
    if (!std::isfinite(minimumHorizontal) ||
        !std::isfinite(maximumHorizontal) ||
        minimumHorizontal >= maximumHorizontal) {
      reject(projection,
             CreativeEditorWorldLayoutElevationStatus::RecipeRejected,
             "creative_editor_world_layout_elevation_roof_perimeter_invalid");
      return projection;
    }
    const double slopeBottom = worldVerticalToCells(
        request.grid,
        roof.geometry.ridgeStart.y - roof.geometry.riseMeters);
    const double ridgeVertical =
        worldVerticalToCells(request.grid, roof.geometry.ridgeStart.y);
    if (level.roofStyle == cr::CreativeStructuralRoofStyle::Shed) {
      const bool slopesOnX =
          level.roofSlopeDirection ==
              cr::CreativeStructuralRoofSlopeDirection::PositiveX ||
          level.roofSlopeDirection ==
              cr::CreativeStructuralRoofSlopeDirection::NegativeX;
      const bool profileVisible =
          (request.axis == CreativeEditorWorldLayoutElevationAxis::X) ==
          slopesOnX;
      if (!profileVisible) {
        appendLine(
            projection,
            {CreativeEditorWorldLayoutElevationLineKind::RoofRidge,
             CreativeEditorWorldLayoutElevationSourceKind::Room,
             facts.representativeRoomIndex, levelIndex,
             {minimumHorizontal, ridgeVertical},
             {maximumHorizontal, ridgeVertical}});
        continue;
      }
      const bool downhillPositive =
          level.roofSlopeDirection ==
              cr::CreativeStructuralRoofSlopeDirection::PositiveX ||
          level.roofSlopeDirection ==
              cr::CreativeStructuralRoofSlopeDirection::PositiveZ;
      const double highHorizontal =
          downhillPositive ? minimumHorizontal : maximumHorizontal;
      const double lowHorizontal =
          downhillPositive ? maximumHorizontal : minimumHorizontal;
      appendLine(
          projection,
          {CreativeEditorWorldLayoutElevationLineKind::RoofSlope,
           CreativeEditorWorldLayoutElevationSourceKind::Room,
           facts.representativeRoomIndex, levelIndex,
           {lowHorizontal, slopeBottom},
           {highHorizontal, ridgeVertical}});
      projection.handles.push_back(
          {CreativeEditorWorldLayoutElevationHandleKind::RoofRidge,
           CreativeEditorWorldLayoutElevationSourceKind::Room,
           facts.representativeRoomIndex, levelIndex,
           {highHorizontal, ridgeVertical}});
      continue;
    }

    const bool crossSection =
        (request.axis == CreativeEditorWorldLayoutElevationAxis::X &&
         level.roofRidgeAxis == cr::CreativeStructuralRoofRidgeAxis::Z) ||
        (request.axis == CreativeEditorWorldLayoutElevationAxis::Z &&
         level.roofRidgeAxis == cr::CreativeStructuralRoofRidgeAxis::X);
    if (crossSection) {
      const double ridgeHorizontal =
          (minimumHorizontal + maximumHorizontal) * 0.5;
      appendLine(
          projection,
          {CreativeEditorWorldLayoutElevationLineKind::RoofSlope,
           CreativeEditorWorldLayoutElevationSourceKind::Room,
           facts.representativeRoomIndex,
           levelIndex,
           {minimumHorizontal, slopeBottom},
           {ridgeHorizontal, ridgeVertical}});
      appendLine(
          projection,
          {CreativeEditorWorldLayoutElevationLineKind::RoofSlope,
           CreativeEditorWorldLayoutElevationSourceKind::Room,
           facts.representativeRoomIndex,
           levelIndex,
           {ridgeHorizontal, ridgeVertical},
           {maximumHorizontal, slopeBottom}});
      projection.handles.push_back(
          {CreativeEditorWorldLayoutElevationHandleKind::RoofRidge,
           CreativeEditorWorldLayoutElevationSourceKind::Room,
           facts.representativeRoomIndex,
           levelIndex,
           {ridgeHorizontal, ridgeVertical}});
    } else if (level.roofStyle == cr::CreativeStructuralRoofStyle::Gable) {
      appendLine(
          projection,
          {CreativeEditorWorldLayoutElevationLineKind::RoofRidge,
           CreativeEditorWorldLayoutElevationSourceKind::Room,
           facts.representativeRoomIndex,
           levelIndex,
           {minimumHorizontal, ridgeVertical},
           {maximumHorizontal, ridgeVertical}});
    } else {
      const double ridgeStartHorizontal = worldHorizontalToCells(
          request.grid, request.axis,
          horizontalCoordinate(request.axis, roof.geometry.ridgeStart.x,
                               roof.geometry.ridgeStart.z));
      const double ridgeEndHorizontal = worldHorizontalToCells(
          request.grid, request.axis,
          horizontalCoordinate(request.axis, roof.geometry.ridgeEnd.x,
                               roof.geometry.ridgeEnd.z));
      const double firstRidge =
          std::min(ridgeStartHorizontal, ridgeEndHorizontal);
      const double secondRidge =
          std::max(ridgeStartHorizontal, ridgeEndHorizontal);
      appendLine(
          projection,
          {CreativeEditorWorldLayoutElevationLineKind::RoofSlope,
           CreativeEditorWorldLayoutElevationSourceKind::Room,
           facts.representativeRoomIndex, levelIndex,
           {minimumHorizontal, slopeBottom},
           {firstRidge, ridgeVertical}});
      appendLine(
          projection,
          {CreativeEditorWorldLayoutElevationLineKind::RoofRidge,
           CreativeEditorWorldLayoutElevationSourceKind::Room,
           facts.representativeRoomIndex, levelIndex,
           {firstRidge, ridgeVertical},
           {secondRidge, ridgeVertical}});
      appendLine(
          projection,
          {CreativeEditorWorldLayoutElevationLineKind::RoofSlope,
           CreativeEditorWorldLayoutElevationSourceKind::Room,
           facts.representativeRoomIndex, levelIndex,
           {secondRidge, ridgeVertical},
           {maximumHorizontal, slopeBottom}});
      projection.handles.push_back(
          {CreativeEditorWorldLayoutElevationHandleKind::RoofRidge,
           CreativeEditorWorldLayoutElevationSourceKind::Room,
           facts.representativeRoomIndex, levelIndex,
           {firstRidge, ridgeVertical}});
    }
  }

  for (std::size_t boxIndex = 0U; boxIndex < layout.boxes.size(); ++boxIndex) {
    const cr::CreativeWorldLayoutBox& box = layout.boxes[boxIndex];
    if (box.buildingIndex != request.buildingIndex) {
      continue;
    }
    if (!appendBoxSurface(projection, request.grid, box, boxIndex,
                          recipeReason)) {
      reject(projection,
             CreativeEditorWorldLayoutElevationStatus::RecipeRejected,
             recipeReason);
      return projection;
    }
    if (box.kind == cr::CreativeObjectKind::Floor) {
      projection.handles.push_back(
          {CreativeEditorWorldLayoutElevationHandleKind::LevelFloor,
           CreativeEditorWorldLayoutElevationSourceKind::Box,
           boxIndex,
           cr::kInvalidCreativeWorldLayoutIndex,
           {horizontalCoordinate(request.axis, box.footprint.minimum),
            box.anchorLayer}});
    }
  }

  for (std::size_t wallIndex = 0U; wallIndex < layout.walls.size();
       ++wallIndex) {
    const cr::CreativeWorldLayoutWall& wall = layout.walls[wallIndex];
    if (wall.buildingIndex != request.buildingIndex) {
      continue;
    }
    const double minimumHorizontal =
        std::min(horizontalCoordinate(request.axis, wall.start),
                 horizontalCoordinate(request.axis, wall.end));
    const double maximumHorizontal =
        std::max(horizontalCoordinate(request.axis, wall.start),
                 horizontalCoordinate(request.axis, wall.end));
    appendItem(
        projection,
        {CreativeEditorWorldLayoutElevationItemKind::WallEnvelope,
         CreativeEditorWorldLayoutElevationSourceKind::Wall,
         wallIndex,
         cr::kInvalidCreativeWorldLayoutIndex,
         minimumHorizontal,
         maximumHorizontal,
         wall.baseLayer,
         wall.baseLayer + static_cast<double>(wall.heightCells)});
    projection.handles.push_back(
        {CreativeEditorWorldLayoutElevationHandleKind::WallTop,
         CreativeEditorWorldLayoutElevationSourceKind::Wall,
         wallIndex,
         cr::kInvalidCreativeWorldLayoutIndex,
         {maximumHorizontal,
          wall.baseLayer + static_cast<double>(wall.heightCells)}});
  }

  for (std::size_t connectorIndex = 0U;
       connectorIndex < layout.verticalConnectors.size(); ++connectorIndex) {
    const cr::CreativeWorldLayoutVerticalConnector& connector =
        layout.verticalConnectors[connectorIndex];
    if (connector.buildingIndex != request.buildingIndex) {
      continue;
    }
    const cr::CreativeWorldLayoutVerticalConnectorPlan connectorPlan =
        cr::planCreativeWorldLayoutVerticalConnector(request.grid, layout,
                                                     connectorIndex);
    if (!connectorPlan.accepted) {
      reject(projection,
             CreativeEditorWorldLayoutElevationStatus::RecipeRejected,
             connectorPlan.reasonCode);
      return projection;
    }
    const double minimumHorizontal = worldHorizontalToCells(
        request.grid, request.axis,
        horizontalCoordinate(request.axis, connectorPlan.authoredBounds.min.x,
                             connectorPlan.authoredBounds.min.z));
    const double maximumHorizontal = worldHorizontalToCells(
        request.grid, request.axis,
        horizontalCoordinate(request.axis, connectorPlan.authoredBounds.max.x,
                             connectorPlan.authoredBounds.max.z));
    const double minimumVertical =
        worldVerticalToCells(request.grid, connectorPlan.authoredBounds.min.y);
    const double maximumVertical =
        worldVerticalToCells(request.grid, connectorPlan.authoredBounds.max.y);
    const auto kind = connector.kind ==
                              cr::CreativeWorldLayoutVerticalConnectorKind::Stair
                          ? CreativeEditorWorldLayoutElevationItemKind::Stair
                          : CreativeEditorWorldLayoutElevationItemKind::Ramp;
    const std::size_t levelIndex =
        connector.lowerRoomIndex < layout.rooms.size()
            ? layout.rooms[connector.lowerRoomIndex].levelIndex
            : cr::kInvalidCreativeWorldLayoutIndex;
    appendItem(
        projection,
        {kind,
         CreativeEditorWorldLayoutElevationSourceKind::VerticalConnector,
         connectorIndex,
         levelIndex,
         minimumHorizontal,
         maximumHorizontal,
         minimumVertical,
         maximumVertical});

    double lowHorizontal = (minimumHorizontal + maximumHorizontal) * 0.5;
    double highHorizontal = lowHorizontal;
    if ((request.axis == CreativeEditorWorldLayoutElevationAxis::X &&
         connector.direction ==
             cr::CreativeWorldLayoutVerticalDirection::PositiveX) ||
        (request.axis == CreativeEditorWorldLayoutElevationAxis::Z &&
         connector.direction ==
             cr::CreativeWorldLayoutVerticalDirection::PositiveZ)) {
      lowHorizontal = minimumHorizontal;
      highHorizontal = maximumHorizontal;
    } else if ((request.axis == CreativeEditorWorldLayoutElevationAxis::X &&
                connector.direction ==
                    cr::CreativeWorldLayoutVerticalDirection::NegativeX) ||
               (request.axis ==
                    CreativeEditorWorldLayoutElevationAxis::Z &&
                connector.direction ==
                    cr::CreativeWorldLayoutVerticalDirection::NegativeZ)) {
      lowHorizontal = maximumHorizontal;
      highHorizontal = minimumHorizontal;
    }
    appendLine(
        projection,
        {CreativeEditorWorldLayoutElevationLineKind::ConnectorRise,
         CreativeEditorWorldLayoutElevationSourceKind::VerticalConnector,
         connectorIndex,
         levelIndex,
         {lowHorizontal, minimumVertical},
         {highHorizontal, maximumVertical}});
  }

  for (std::size_t openingIndex = 0U; openingIndex < layout.openings.size();
       ++openingIndex) {
    const cr::CreativeWorldLayoutOpening& opening =
        layout.openings[openingIndex];
    const OpeningHostFacts host = resolveOpeningHost(layout, opening);
    if (!host.valid || host.buildingIndex != request.buildingIndex) {
      continue;
    }
    const double deltaX = static_cast<double>(host.end.x) - host.start.x;
    const double deltaZ = static_cast<double>(host.end.z) - host.start.z;
    const double unitX = deltaX / host.lengthCells;
    const double unitZ = deltaZ / host.lengthCells;
    const double centerX = host.start.x + unitX * opening.centerOffsetCells;
    const double centerZ = host.start.z + unitZ * opening.centerOffsetCells;
    const double projectedUnit =
        request.axis == CreativeEditorWorldLayoutElevationAxis::X
            ? std::abs(unitX)
            : std::abs(unitZ);
    const double centerHorizontal =
        horizontalCoordinate(request.axis, centerX, centerZ);
    const double halfWidth = opening.widthCells * projectedUnit * 0.5;
    const double minimumVertical =
        host.baseLayer + opening.cutoutBottomCells;
    const double maximumVertical =
        minimumVertical + opening.cutoutHeightCells;
    appendItem(
        projection,
        {opening.kind == cr::CreativeBuildingOpeningKind::Door
             ? CreativeEditorWorldLayoutElevationItemKind::Door
             : CreativeEditorWorldLayoutElevationItemKind::Window,
         CreativeEditorWorldLayoutElevationSourceKind::Opening,
         openingIndex,
         host.levelIndex,
         centerHorizontal - halfWidth,
         centerHorizontal + halfWidth,
         minimumVertical,
         maximumVertical});
    if (opening.kind == cr::CreativeBuildingOpeningKind::Window) {
      projection.handles.push_back(
          {CreativeEditorWorldLayoutElevationHandleKind::OpeningBottom,
           CreativeEditorWorldLayoutElevationSourceKind::Opening,
           openingIndex,
           host.levelIndex,
           {centerHorizontal, minimumVertical}});
    }
    projection.handles.push_back(
        {CreativeEditorWorldLayoutElevationHandleKind::OpeningTop,
         CreativeEditorWorldLayoutElevationSourceKind::Opening,
         openingIndex,
         host.levelIndex,
         {centerHorizontal, maximumVertical}});
  }

  if (!projection.bounds.valid) {
    reject(projection,
           CreativeEditorWorldLayoutElevationStatus::EmptyBuilding,
           "creative_editor_world_layout_elevation_building_empty");
    return projection;
  }
  projection.accepted = true;
  projection.status = CreativeEditorWorldLayoutElevationStatus::Ready;
  projection.reasonCode = "creative_editor_world_layout_elevation_ready";
  return projection;
}

}  // namespace iggy3d_creative_app
