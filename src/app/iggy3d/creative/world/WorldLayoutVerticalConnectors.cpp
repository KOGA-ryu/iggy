#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <numbers>

namespace iggy3d::creative {
namespace {

void reject(CreativeWorldLayoutVerticalConnectorPlan& plan,
            CreativeWorldLayoutVerticalConnectorStatus status,
            std::string_view reasonCode) noexcept {
  plan.status = status;
  plan.reasonCode = reasonCode;
}

[[nodiscard]] bool worldCoordinate(double origin, double cellSize,
                                   long double coordinate,
                                   double& output) noexcept {
  const long double value = static_cast<long double>(origin) +
                            static_cast<long double>(cellSize) * coordinate;
  if (!std::isfinite(value) || value < -std::numeric_limits<double>::max() ||
      value > std::numeric_limits<double>::max()) {
    return false;
  }
  output = static_cast<double>(value);
  return std::isfinite(output);
}

[[nodiscard]] bool landingRects(
    CreativeWorldLayoutRect footprint,
    CreativeWorldLayoutVerticalDirection direction,
    CreativeWorldLayoutRect& lowerLanding,
    CreativeWorldLayoutRect& upperLanding) noexcept {
  switch (direction) {
  case CreativeWorldLayoutVerticalDirection::PositiveX:
    lowerLanding = {{footprint.minimum.x - 1, footprint.minimum.z},
                    {footprint.minimum.x, footprint.maximum.z}};
    upperLanding = {{footprint.maximum.x, footprint.minimum.z},
                    {footprint.maximum.x + 1, footprint.maximum.z}};
    break;
  case CreativeWorldLayoutVerticalDirection::NegativeX:
    lowerLanding = {{footprint.maximum.x, footprint.minimum.z},
                    {footprint.maximum.x + 1, footprint.maximum.z}};
    upperLanding = {{footprint.minimum.x - 1, footprint.minimum.z},
                    {footprint.minimum.x, footprint.maximum.z}};
    break;
  case CreativeWorldLayoutVerticalDirection::PositiveZ:
    lowerLanding = {{footprint.minimum.x, footprint.minimum.z - 1},
                    {footprint.maximum.x, footprint.minimum.z}};
    upperLanding = {{footprint.minimum.x, footprint.maximum.z},
                    {footprint.maximum.x, footprint.maximum.z + 1}};
    break;
  case CreativeWorldLayoutVerticalDirection::NegativeZ:
    lowerLanding = {{footprint.minimum.x, footprint.maximum.z},
                    {footprint.maximum.x, footprint.maximum.z + 1}};
    upperLanding = {{footprint.minimum.x, footprint.minimum.z - 1},
                    {footprint.maximum.x, footprint.minimum.z}};
    break;
  case CreativeWorldLayoutVerticalDirection::Count:
    return false;
  }
  return true;
}

[[nodiscard]] bool hasLanding(
    const CreativeWorldLayoutRoomGraph& graph,
    std::size_t lowerRoomIndex,
    std::size_t upperRoomIndex,
    CreativeWorldLayoutRect footprint,
    CreativeWorldLayoutVerticalDirection direction) noexcept {
  CreativeWorldLayoutRect lowerLanding;
  CreativeWorldLayoutRect upperLanding;
  if (!landingRects(footprint, direction, lowerLanding, upperLanding)) {
    return false;
  }
  return creativeWorldLayoutRoomContainsRect(graph, lowerRoomIndex,
                                              lowerLanding) &&
         creativeWorldLayoutRoomContainsRect(graph, upperRoomIndex,
                                              upperLanding);
}

[[nodiscard]] bool
sharesCutSurface(const CreativeWorldLayoutVerticalConnector& lhs,
                 const CreativeWorldLayoutVerticalConnector& rhs) noexcept {
  return lhs.lowerRoomIndex == rhs.lowerRoomIndex ||
         lhs.upperRoomIndex == rhs.upperRoomIndex;
}

} // namespace

CreativeWorldLayoutVerticalConnectorPlan
planCreativeWorldLayoutVerticalConnector(const CreativeGridSettings& grid,
                                         const CreativeWorldLayout& layout,
                                         std::size_t connectorIndex) {
  CreativeWorldLayoutVerticalConnectorPlan plan;
  if (connectorIndex >= layout.verticalConnectors.size()) {
    reject(plan, CreativeWorldLayoutVerticalConnectorStatus::InvalidConnector,
           "creative_world_layout_vertical_connector_index_invalid");
    return plan;
  }
  return planCreativeWorldLayoutVerticalConnector(
      grid, layout, connectorIndex, layout.verticalConnectors[connectorIndex]);
}

CreativeWorldLayoutVerticalConnectorPlan
planCreativeWorldLayoutVerticalConnector(
    const CreativeGridSettings& grid, const CreativeWorldLayout& layout,
    std::size_t connectorIndex,
    const CreativeWorldLayoutVerticalConnector& connector) {
  CreativeWorldLayoutVerticalConnectorPlan plan;
  if (connectorIndex >= layout.verticalConnectors.size()) {
    reject(plan, CreativeWorldLayoutVerticalConnectorStatus::InvalidConnector,
           "creative_world_layout_vertical_connector_index_invalid");
    return plan;
  }
  plan.buildingIndex = connector.buildingIndex;
  plan.lowerRoomIndex = connector.lowerRoomIndex;
  plan.upperRoomIndex = connector.upperRoomIndex;
  plan.openingFootprint = connector.footprint;
  plan.material = connector.material;

  if (connector.kind >= CreativeWorldLayoutVerticalConnectorKind::Count ||
      connector.direction >= CreativeWorldLayoutVerticalDirection::Count ||
      connector.material >= CreativeStructuralMaterial::Count ||
      connector.name.empty() || connector.stableKey.empty()) {
    const bool invalidMaterial =
        connector.material >= CreativeStructuralMaterial::Count;
    reject(plan, invalidMaterial
                     ? CreativeWorldLayoutVerticalConnectorStatus::InvalidMaterial
                     : CreativeWorldLayoutVerticalConnectorStatus::InvalidConnector,
           invalidMaterial
               ? "creative_world_layout_vertical_connector_material_invalid"
               : "creative_world_layout_vertical_connector_invalid");
    return plan;
  }
  if (connector.buildingIndex >= layout.buildings.size() ||
      connector.lowerRoomIndex >= layout.rooms.size() ||
      connector.upperRoomIndex >= layout.rooms.size() ||
      connector.lowerRoomIndex == connector.upperRoomIndex) {
    reject(plan, CreativeWorldLayoutVerticalConnectorStatus::InvalidOwner,
           "creative_world_layout_vertical_connector_owner_invalid");
    return plan;
  }
  const CreativeWorldLayoutRoom& lower = layout.rooms[connector.lowerRoomIndex];
  const CreativeWorldLayoutRoom& upper = layout.rooms[connector.upperRoomIndex];
  if (lower.buildingIndex != connector.buildingIndex ||
      upper.buildingIndex != connector.buildingIndex ||
      lower.levelIndex >= layout.levels.size() ||
      upper.levelIndex >= layout.levels.size()) {
    reject(plan, CreativeWorldLayoutVerticalConnectorStatus::InvalidOwner,
           "creative_world_layout_vertical_connector_owner_invalid");
    return plan;
  }
  const CreativeWorldLayoutLevel& lowerLevel = layout.levels[lower.levelIndex];
  const CreativeWorldLayoutLevel& upperLevel = layout.levels[upper.levelIndex];
  const double riseCells = upperLevel.floorTopLayer - lowerLevel.floorTopLayer;
  if (lowerLevel.buildingIndex != connector.buildingIndex ||
      upperLevel.buildingIndex != connector.buildingIndex ||
      !std::isfinite(riseCells) || riseCells <= 0.0 ||
      std::fabs(riseCells - static_cast<double>(lowerLevel.wallHeightCells)) >
          1.0e-9 ||
      lowerLevel.wallHeightCells < 2U) {
    reject(plan, CreativeWorldLayoutVerticalConnectorStatus::InvalidLevels,
           "creative_world_layout_vertical_connector_levels_invalid");
    return plan;
  }
  const CreativeWorldLayoutRoomGraph graph =
      buildCreativeWorldLayoutRoomGraph(layout);
  if (!graph.accepted ||
      !creativeWorldLayoutRoomContainsRect(
          graph, connector.lowerRoomIndex, connector.footprint) ||
      !creativeWorldLayoutRoomContainsRect(
          graph, connector.upperRoomIndex, connector.footprint)) {
    reject(plan, CreativeWorldLayoutVerticalConnectorStatus::InvalidFootprint,
           "creative_world_layout_vertical_connector_footprint_invalid");
    return plan;
  }
  if (!hasLanding(graph, connector.lowerRoomIndex, connector.upperRoomIndex,
                  connector.footprint, connector.direction)) {
    reject(plan, CreativeWorldLayoutVerticalConnectorStatus::InvalidLanding,
           "creative_world_layout_vertical_connector_landing_invalid");
    return plan;
  }
  for (std::size_t index = 0U; index < layout.verticalConnectors.size();
       ++index) {
    if (index != connectorIndex &&
        sharesCutSurface(connector, layout.verticalConnectors[index])) {
      reject(plan,
             CreativeWorldLayoutVerticalConnectorStatus::SurfaceAlreadyCut,
             "creative_world_layout_vertical_connector_surface_already_cut");
      return plan;
    }
  }

  const double extentX = static_cast<double>(connector.footprint.maximum.x) -
                         connector.footprint.minimum.x;
  const double extentZ = static_cast<double>(connector.footprint.maximum.z) -
                         connector.footprint.minimum.z;
  const bool alongX =
      connector.direction == CreativeWorldLayoutVerticalDirection::PositiveX ||
      connector.direction == CreativeWorldLayoutVerticalDirection::NegativeX;
  const double runCells = alongX ? extentX : extentZ;
  const double widthCells = alongX ? extentZ : extentX;
  if (!std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0 ||
      runCells <= 0.0 || widthCells < 1.0 ||
      (connector.kind == CreativeWorldLayoutVerticalConnectorKind::Stair &&
       runCells < riseCells)) {
    reject(plan, CreativeWorldLayoutVerticalConnectorStatus::InvalidSlope,
           "creative_world_layout_vertical_connector_slope_invalid");
    return plan;
  }

  plan.objectKind =
      connector.kind == CreativeWorldLayoutVerticalConnectorKind::Stair
          ? CreativeObjectKind::Stair
          : CreativeObjectKind::Ramp;
  plan.riseMeters = riseCells * grid.cellSizeMeters;
  plan.runMeters = runCells * grid.cellSizeMeters;
  plan.widthMeters = widthCells * grid.cellSizeMeters;
  if (!std::isfinite(plan.riseMeters) || !std::isfinite(plan.runMeters) ||
      !std::isfinite(plan.widthMeters) || plan.riseMeters <= 0.0 ||
      plan.runMeters <= 0.0 || plan.widthMeters <= 0.0) {
    reject(plan,
           CreativeWorldLayoutVerticalConnectorStatus::UnrepresentableGeometry,
           "creative_world_layout_vertical_connector_geometry_unrepresentable");
    return plan;
  }

  double centerX = 0.0;
  double centerZ = 0.0;
  if (!worldCoordinate(
          grid.origin.x, grid.cellSizeMeters,
          (static_cast<long double>(connector.footprint.minimum.x) +
           connector.footprint.maximum.x) *
              0.5L,
          centerX) ||
      !worldCoordinate(
          grid.origin.z, grid.cellSizeMeters,
          (static_cast<long double>(connector.footprint.minimum.z) +
           connector.footprint.maximum.z) *
              0.5L,
          centerZ) ||
      !worldCoordinate(grid.origin.y, grid.cellSizeMeters,
                       lowerLevel.floorTopLayer, plan.authoredBounds.min.y) ||
      !worldCoordinate(grid.origin.y, grid.cellSizeMeters,
                       upperLevel.floorTopLayer, plan.authoredBounds.max.y)) {
    reject(plan,
           CreativeWorldLayoutVerticalConnectorStatus::UnrepresentableGeometry,
           "creative_world_layout_vertical_connector_geometry_unrepresentable");
    return plan;
  }
  plan.authoredBounds.min.x = centerX - plan.widthMeters * 0.5;
  plan.authoredBounds.max.x = centerX + plan.widthMeters * 0.5;
  plan.authoredBounds.min.z = centerZ - plan.runMeters * 0.5;
  plan.authoredBounds.max.z = centerZ + plan.runMeters * 0.5;

  switch (connector.direction) {
  case CreativeWorldLayoutVerticalDirection::PositiveX:
    plan.rotationEulerRadians.y = std::numbers::pi * 0.5;
    break;
  case CreativeWorldLayoutVerticalDirection::NegativeX:
    plan.rotationEulerRadians.y = -std::numbers::pi * 0.5;
    break;
  case CreativeWorldLayoutVerticalDirection::PositiveZ:
    break;
  case CreativeWorldLayoutVerticalDirection::NegativeZ:
    plan.rotationEulerRadians.y = std::numbers::pi;
    break;
  case CreativeWorldLayoutVerticalDirection::Count:
    break;
  }

  if (plan.objectKind == CreativeObjectKind::Stair) {
    CreativeStairRecipeRequest stairRequest;
    stairRequest.authoredBounds = plan.authoredBounds;
    stairRequest.transform.position =
        measureCreativeBounds(plan.authoredBounds).center;
    stairRequest.transform.rotationEulerRadians =
        plan.rotationEulerRadians;
    stairRequest.maximumRiserHeightMeters =
        describeObject(CreativeObjectKind::Stair)
            .generatedGeometry.maximumStepRiseMeters;
    stairRequest.landingDepthMeters = grid.cellSizeMeters;
    stairRequest.availableHeadroomMeters =
        static_cast<double>(upperLevel.wallHeightCells) *
        grid.cellSizeMeters;
    plan.stair = planCreativeStair(stairRequest);
    if (!plan.stair.accepted) {
      const bool headroom =
          plan.stair.status == CreativeStairRecipeStatus::InvalidHeadroom;
      reject(plan,
             headroom
                 ? CreativeWorldLayoutVerticalConnectorStatus::InvalidHeadroom
                 : CreativeWorldLayoutVerticalConnectorStatus::
                       UnrepresentableGeometry,
             plan.stair.reasonCode);
      return plan;
    }
    plan.stepCount = plan.stair.stepCount;
  } else {
    CreativeRampRecipeRequest rampRequest;
    rampRequest.authoredBounds = plan.authoredBounds;
    rampRequest.transform.position =
        measureCreativeBounds(plan.authoredBounds).center;
    rampRequest.transform.rotationEulerRadians = plan.rotationEulerRadians;
    rampRequest.landingDepthMeters = grid.cellSizeMeters;
    rampRequest.availableHeadroomMeters =
        static_cast<double>(upperLevel.wallHeightCells) *
        grid.cellSizeMeters;
    rampRequest.material = connector.material;
    plan.ramp = planCreativeRamp(rampRequest);
    if (!plan.ramp.accepted) {
      CreativeWorldLayoutVerticalConnectorStatus status =
          CreativeWorldLayoutVerticalConnectorStatus::UnrepresentableGeometry;
      if (plan.ramp.status == CreativeRampRecipeStatus::InvalidHeadroom) {
        status = CreativeWorldLayoutVerticalConnectorStatus::InvalidHeadroom;
      } else if (plan.ramp.status == CreativeRampRecipeStatus::InvalidSlope) {
        status = CreativeWorldLayoutVerticalConnectorStatus::InvalidSlope;
      } else if (plan.ramp.status == CreativeRampRecipeStatus::InvalidMaterial) {
        status = CreativeWorldLayoutVerticalConnectorStatus::InvalidMaterial;
      }
      reject(plan, status, plan.ramp.reasonCode);
      return plan;
    }
  }

  plan.accepted = true;
  plan.status = CreativeWorldLayoutVerticalConnectorStatus::Ready;
  plan.reasonCode = "creative_world_layout_vertical_connector_ready";
  return plan;
}

} // namespace iggy3d::creative
