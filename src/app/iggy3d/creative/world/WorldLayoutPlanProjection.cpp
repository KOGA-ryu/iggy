#include "app/iggy3d/creative/world/WorldLayoutPlanProjectionInternal.hpp"

#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>
#include <vector>

namespace iggy3d::creative::plan_projection_internal {

SourceRef source(CreativeWorldLayoutTable table, std::size_t index) noexcept {
  return {table, index, CreativeWorldLayoutTable::None,
          kInvalidCreativeWorldLayoutIndex};
}

void reject(Projection& projection,
            CreativeWorldLayoutPlanProjectionStatus status,
            std::string_view reasonCode) noexcept {
  projection.accepted = false;
  projection.status = status;
  projection.activeFloorTopLayer = 0.0;
  projection.bounds = {};
  projection.receipt = {};
  projection.primitives.clear();
  projection.reasonCode = reasonCode;
}

namespace {

bool finite(Point value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.z);
}

void includePoint(CreativeWorldLayoutPlanBounds& bounds, Point value) noexcept {
  if (!bounds.valid) {
    bounds.valid = true;
    bounds.minimum = value;
    bounds.maximum = value;
    return;
  }
  bounds.minimum.x = std::min(bounds.minimum.x, value.x);
  bounds.minimum.z = std::min(bounds.minimum.z, value.z);
  bounds.maximum.x = std::max(bounds.maximum.x, value.x);
  bounds.maximum.z = std::max(bounds.maximum.z, value.z);
}

std::vector<std::size_t> activeLevelsAtDatum(
    const CreativeWorldLayout& layout, double datum) {
  std::vector<std::size_t> result;
  result.reserve(layout.levels.size());
  for (std::size_t levelIndex = 0U; levelIndex < layout.levels.size();
       ++levelIndex) {
    const CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
    if (near(level.floorTopLayer, datum) &&
        level.buildingIndex < layout.buildings.size() &&
        layout.buildings[level.buildingIndex].visible) {
      result.push_back(levelIndex);
    }
  }
  return result;
}

enum class ContextDirection : std::uint8_t {
  Lower,
  Higher,
};

std::vector<std::size_t> nearestContextLevels(
    const CreativeWorldLayout& layout,
    std::span<const std::size_t> activeLevels, double datum,
    ContextDirection direction) {
  std::vector<std::size_t> result;
  std::vector<bool> visitedBuilding(layout.buildings.size(), false);
  for (const std::size_t activeLevelIndex : activeLevels) {
    const std::size_t buildingIndex =
        layout.levels[activeLevelIndex].buildingIndex;
    if (visitedBuilding[buildingIndex]) {
      continue;
    }
    visitedBuilding[buildingIndex] = true;
    std::size_t best = kInvalidCreativeWorldLayoutIndex;
    double bestDistance = std::numeric_limits<double>::infinity();
    for (std::size_t levelIndex = 0U; levelIndex < layout.levels.size();
         ++levelIndex) {
      const CreativeWorldLayoutLevel& candidate = layout.levels[levelIndex];
      const double distance =
          direction == ContextDirection::Lower
              ? datum - candidate.floorTopLayer
              : candidate.floorTopLayer - datum;
      if (candidate.buildingIndex == buildingIndex &&
          distance > kGeometryEpsilon && distance < bestDistance &&
          creativeWorldLayoutLevelHasRooms(layout, levelIndex)) {
        best = levelIndex;
        bestDistance = distance;
      }
    }
    if (best != kInvalidCreativeWorldLayoutIndex) {
      result.push_back(best);
    }
  }
  return result;
}

void makeMasks(const CreativeWorldLayout& layout,
               std::span<const std::size_t> levels,
               std::vector<std::uint8_t>& levelMask,
               std::vector<std::uint8_t>& buildingMask) {
  levelMask.assign(layout.levels.size(), 0U);
  buildingMask.assign(layout.buildings.size(), 0U);
  for (const std::size_t levelIndex : levels) {
    levelMask[levelIndex] = 1U;
    buildingMask[layout.levels[levelIndex].buildingIndex] = 1U;
  }
}

}  // namespace

bool appendPrimitive(Projection& projection, Primitive primitive) {
  if (primitive.role >= Role::Count || primitive.kind >= PrimitiveKind::Count ||
      primitive.layer >= Layer::Count || !std::isfinite(primitive.widthCells) ||
      primitive.widthCells < 0.0) {
    return false;
  }

  std::size_t requiredPoints = 0U;
  switch (primitive.kind) {
    case PrimitiveKind::Segment:
      requiredPoints = 2U;
      break;
    case PrimitiveKind::Polygon:
      if (primitive.pointCount < 3U ||
          primitive.pointCount > primitive.points.size()) {
        return false;
      }
      requiredPoints = primitive.pointCount;
      break;
    case PrimitiveKind::Circle:
      requiredPoints = 1U;
      if (!std::isfinite(primitive.radiusCells) ||
          primitive.radiusCells <= 0.0) {
        return false;
      }
      break;
    case PrimitiveKind::Arc:
      requiredPoints = 1U;
      if (!std::isfinite(primitive.radiusCells) ||
          primitive.radiusCells <= 0.0 ||
          !std::isfinite(primitive.startRadians) ||
          !std::isfinite(primitive.sweepRadians) ||
          std::abs(primitive.sweepRadians) <= kGeometryEpsilon) {
        return false;
      }
      break;
    case PrimitiveKind::Point:
      requiredPoints = 1U;
      break;
    case PrimitiveKind::Count:
      return false;
  }
  if (primitive.pointCount != requiredPoints) {
    return false;
  }
  for (std::size_t index = 0U; index < requiredPoints; ++index) {
    if (!finite(primitive.points[index])) {
      return false;
    }
  }
  if (primitive.kind == PrimitiveKind::Segment &&
      std::hypot(primitive.points[1].x - primitive.points[0].x,
                 primitive.points[1].z - primitive.points[0].z) <=
          kGeometryEpsilon) {
    return false;
  }

  if (primitive.kind == PrimitiveKind::Circle ||
      primitive.kind == PrimitiveKind::Arc) {
    const Point center = primitive.points[0];
    includePoint(projection.bounds,
                 {center.x - primitive.radiusCells,
                  center.z - primitive.radiusCells});
    includePoint(projection.bounds,
                 {center.x + primitive.radiusCells,
                  center.z + primitive.radiusCells});
  } else {
    for (std::size_t index = 0U; index < requiredPoints; ++index) {
      includePoint(projection.bounds, primitive.points[index]);
    }
  }
  projection.primitives.push_back(std::move(primitive));
  return true;
}

Primitive segment(Role role, Layer layer, SourceRef sourceRef, Point start,
                  Point end) noexcept {
  Primitive primitive;
  primitive.role = role;
  primitive.kind = PrimitiveKind::Segment;
  primitive.layer = layer;
  primitive.source = sourceRef;
  primitive.points[0] = start;
  primitive.points[1] = end;
  primitive.pointCount = 2U;
  return primitive;
}

Primitive polygon(Role role, Layer layer, SourceRef sourceRef,
                  std::array<Point, 4U> points) noexcept {
  Primitive primitive;
  primitive.role = role;
  primitive.kind = PrimitiveKind::Polygon;
  primitive.layer = layer;
  primitive.source = sourceRef;
  primitive.points = points;
  primitive.pointCount = 4U;
  return primitive;
}

Primitive rectPrimitive(Role role, Layer layer, SourceRef sourceRef,
                        CreativeWorldLayoutRect rect) noexcept {
  return polygon(role, layer, sourceRef,
                 {{{static_cast<double>(rect.minimum.x),
                     static_cast<double>(rect.minimum.z)},
                    {static_cast<double>(rect.maximum.x),
                     static_cast<double>(rect.minimum.z)},
                    {static_cast<double>(rect.maximum.x),
                     static_cast<double>(rect.maximum.z)},
                    {static_cast<double>(rect.minimum.x),
                     static_cast<double>(rect.maximum.z)}}});
}

}  // namespace iggy3d::creative::plan_projection_internal

namespace iggy3d::creative {

std::string_view toString(
    CreativeWorldLayoutPlanProjectionStatus status) noexcept {
  switch (status) {
    case CreativeWorldLayoutPlanProjectionStatus::NotRequested:
      return "NotRequested";
    case CreativeWorldLayoutPlanProjectionStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeWorldLayoutPlanProjectionStatus::InvalidActiveLevel:
      return "InvalidActiveLevel";
    case CreativeWorldLayoutPlanProjectionStatus::InvalidLayout:
      return "InvalidLayout";
    case CreativeWorldLayoutPlanProjectionStatus::Ready:
      return "Ready";
  }
  return "Unknown";
}

std::string_view toString(CreativeWorldLayoutPlanLayer layer) noexcept {
  switch (layer) {
    case CreativeWorldLayoutPlanLayer::LowerContext:
      return "LowerContext";
    case CreativeWorldLayoutPlanLayer::Active:
      return "Active";
    case CreativeWorldLayoutPlanLayer::UpperContext:
      return "UpperContext";
    case CreativeWorldLayoutPlanLayer::Overhead:
      return "Overhead";
    case CreativeWorldLayoutPlanLayer::Count:
      break;
  }
  return "Unknown";
}

std::string_view toString(CreativeWorldLayoutPlanRole role) noexcept {
  switch (role) {
    case CreativeWorldLayoutPlanRole::RoomFloor:
      return "RoomFloor";
    case CreativeWorldLayoutPlanRole::ExteriorWall:
      return "ExteriorWall";
    case CreativeWorldLayoutPlanRole::InteriorPartition:
      return "InteriorPartition";
    case CreativeWorldLayoutPlanRole::SharedBoundary:
      return "SharedBoundary";
    case CreativeWorldLayoutPlanRole::Door:
      return "Door";
    case CreativeWorldLayoutPlanRole::DoorSwing:
      return "DoorSwing";
    case CreativeWorldLayoutPlanRole::OpeningFacing:
      return "OpeningFacing";
    case CreativeWorldLayoutPlanRole::Window:
      return "Window";
    case CreativeWorldLayoutPlanRole::WindowShutter:
      return "WindowShutter";
    case CreativeWorldLayoutPlanRole::Stair:
      return "Stair";
    case CreativeWorldLayoutPlanRole::Ramp:
      return "Ramp";
    case CreativeWorldLayoutPlanRole::RoofOutline:
      return "RoofOutline";
    case CreativeWorldLayoutPlanRole::RoofRidge:
      return "RoofRidge";
    case CreativeWorldLayoutPlanRole::RoofSkylight:
      return "RoofSkylight";
    case CreativeWorldLayoutPlanRole::RoofClearance:
      return "RoofClearance";
    case CreativeWorldLayoutPlanRole::TerrainProfile:
      return "TerrainProfile";
    case CreativeWorldLayoutPlanRole::TerrainPath:
      return "TerrainPath";
    case CreativeWorldLayoutPlanRole::Contour:
      return "Contour";
    case CreativeWorldLayoutPlanRole::Object:
      return "Object";
    case CreativeWorldLayoutPlanRole::Bridge:
      return "Bridge";
    case CreativeWorldLayoutPlanRole::PlayerSpawn:
      return "PlayerSpawn";
    case CreativeWorldLayoutPlanRole::NpcSpawn:
      return "NpcSpawn";
    case CreativeWorldLayoutPlanRole::Count:
      break;
  }
  return "Unknown";
}

CreativeWorldLayoutPlanProjection projectCreativeWorldLayoutPlan(
    const CreativeWorldLayoutPlanProjectionRequest& request) {
  using namespace plan_projection_internal;

  Projection projection;
  if (request.layout == nullptr) {
    return projection;
  }
  if (!isFiniteCreativeVec3(request.grid.origin) ||
      !std::isfinite(request.grid.cellSizeMeters) ||
      request.grid.cellSizeMeters <= 0.0 ||
      !std::isfinite(request.cutPlaneHeightMeters) ||
      request.cutPlaneHeightMeters <= 0.0) {
    reject(projection, CreativeWorldLayoutPlanProjectionStatus::InvalidRequest,
           "creative_world_layout_plan_projection_request_invalid");
    return projection;
  }

  const CreativeWorldLayout& layout = *request.layout;
  const bool haveLevels = !layout.levels.empty();
  if ((haveLevels && request.activeLevelIndex >= layout.levels.size()) ||
      (!haveLevels &&
       request.activeLevelIndex != kInvalidCreativeWorldLayoutIndex)) {
    reject(
        projection,
        CreativeWorldLayoutPlanProjectionStatus::InvalidActiveLevel,
        "creative_world_layout_plan_projection_active_level_invalid");
    return projection;
  }
  const CreativeWorldLayoutRoomCompileResult compiled =
      expandCreativeWorldLayoutRooms(layout);
  if (!compiled.accepted) {
    reject(projection, CreativeWorldLayoutPlanProjectionStatus::InvalidLayout,
           "creative_world_layout_plan_projection_layout_invalid");
    return projection;
  }

  const double activeDatum =
      haveLevels ? layout.levels[request.activeLevelIndex].floorTopLayer : 0.0;
  const double cutPlaneHeightCells =
      request.cutPlaneHeightMeters / request.grid.cellSizeMeters;
  if (!std::isfinite(activeDatum) || !std::isfinite(cutPlaneHeightCells)) {
    reject(projection, CreativeWorldLayoutPlanProjectionStatus::InvalidRequest,
           "creative_world_layout_plan_projection_cut_plane_invalid");
    return projection;
  }
  projection.activeFloorTopLayer = activeDatum;

  const std::vector<std::size_t> activeLevels =
      haveLevels ? activeLevelsAtDatum(layout, activeDatum)
                 : std::vector<std::size_t>{};
  if (haveLevels && activeLevels.empty()) {
    reject(
        projection,
        CreativeWorldLayoutPlanProjectionStatus::InvalidActiveLevel,
        "creative_world_layout_plan_projection_active_datum_unavailable");
    return projection;
  }
  projection.receipt.activeLevelCount = activeLevels.size();

  std::vector<std::uint8_t> activeLevelMask;
  std::vector<std::uint8_t> activeBuildingMask;
  makeMasks(layout, activeLevels, activeLevelMask, activeBuildingMask);

  std::size_t terrainPathPointCount = 0U;
  std::size_t retainingTransitionCount = 0U;
  for (const CreativeWorldLayoutTerrainProfile& profile :
       layout.terrainProfiles) {
    retainingTransitionCount +=
        profile.usesRetainingEdgeRecipe
            ? profile.retainingEdge.settings.transitionCount
            : 0U;
  }
  for (const CreativeWorldLayoutTerrainPath& path : layout.terrainPaths) {
    terrainPathPointCount += path.recipe.points.size();
  }
  projection.primitives.reserve(
      layout.rooms.size() + layout.walls.size() + layout.openings.size() * 3U +
      layout.verticalConnectors.size() * 9U + layout.objects.size() +
      layout.terrainProfiles.size() + retainingTransitionCount +
      terrainPathPointCount +
      request.contours.size());

  if (!projectTerrain(projection, layout, request.contours)) {
    reject(projection, CreativeWorldLayoutPlanProjectionStatus::InvalidLayout,
           "creative_world_layout_plan_projection_terrain_invalid");
    return projection;
  }

  if (request.includeLowerLevelContext && haveLevels) {
    const std::vector<std::size_t> contextLevels =
        nearestContextLevels(layout, activeLevels, activeDatum,
                             ContextDirection::Lower);
    projection.receipt.lowerContextLevelCount = contextLevels.size();
    projection.receipt.contextLevelCount += contextLevels.size();
    for (const std::size_t contextLevelIndex : contextLevels) {
      const std::array<std::size_t, 1U> oneLevel{contextLevelIndex};
      std::vector<std::uint8_t> contextLevelMask;
      std::vector<std::uint8_t> contextBuildingMask;
      makeMasks(layout, oneLevel, contextLevelMask, contextBuildingMask);
      if (!projectLevelArchitecture(
              projection, layout, compiled, contextLevelMask,
              contextBuildingMask,
              layout.levels[contextLevelIndex].floorTopLayer,
              cutPlaneHeightCells, Layer::LowerContext)) {
        reject(
            projection,
            CreativeWorldLayoutPlanProjectionStatus::InvalidLayout,
            "creative_world_layout_plan_projection_context_invalid");
        return projection;
      }
    }
  }

  if (haveLevels &&
      !projectLevelArchitecture(projection, layout, compiled, activeLevelMask,
                                activeBuildingMask, activeDatum,
                                cutPlaneHeightCells, Layer::Active)) {
    reject(projection, CreativeWorldLayoutPlanProjectionStatus::InvalidLayout,
           "creative_world_layout_plan_projection_architecture_invalid");
    return projection;
  }

  if (request.includeUpperLevelContext && haveLevels) {
    const std::vector<std::size_t> contextLevels =
        nearestContextLevels(layout, activeLevels, activeDatum,
                             ContextDirection::Higher);
    projection.receipt.upperContextLevelCount = contextLevels.size();
    projection.receipt.contextLevelCount += contextLevels.size();
    for (const std::size_t contextLevelIndex : contextLevels) {
      const std::array<std::size_t, 1U> oneLevel{contextLevelIndex};
      std::vector<std::uint8_t> contextLevelMask;
      std::vector<std::uint8_t> contextBuildingMask;
      makeMasks(layout, oneLevel, contextLevelMask, contextBuildingMask);
      if (!projectLevelArchitecture(
              projection, layout, compiled, contextLevelMask,
              contextBuildingMask,
              layout.levels[contextLevelIndex].floorTopLayer,
              cutPlaneHeightCells, Layer::UpperContext)) {
        reject(
            projection,
            CreativeWorldLayoutPlanProjectionStatus::InvalidLayout,
            "creative_world_layout_plan_projection_upper_context_invalid");
        return projection;
      }
    }
  }

  double activeBandTop = std::numeric_limits<double>::infinity();
  if (haveLevels) {
    activeBandTop = activeDatum;
    for (const std::size_t levelIndex : activeLevels) {
      const CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
      const double facadeHeight =
          creativeWorldLayoutLevelFacadeHeightCells(layout, levelIndex);
      activeBandTop = std::max(
          activeBandTop,
          level.floorTopLayer +
              (std::isfinite(facadeHeight)
                   ? facadeHeight
                   : static_cast<double>(level.wallHeightCells)));
    }
  }
  if (!projectObjects(projection, layout, request.grid, haveLevels, activeDatum,
                      activeBandTop)) {
    reject(projection, CreativeWorldLayoutPlanProjectionStatus::InvalidLayout,
           "creative_world_layout_plan_projection_objects_invalid");
    return projection;
  }

  if (request.includeRoofOverhead && haveLevels &&
      !projectRoofs(projection, layout, request.grid, activeLevelMask)) {
    reject(projection, CreativeWorldLayoutPlanProjectionStatus::InvalidLayout,
           "creative_world_layout_plan_projection_roof_invalid");
    return projection;
  }

  projection.accepted = true;
  projection.status = CreativeWorldLayoutPlanProjectionStatus::Ready;
  projection.reasonCode = "creative_world_layout_plan_projection_ready";
  return projection;
}

}  // namespace iggy3d::creative
