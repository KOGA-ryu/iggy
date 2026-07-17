#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

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

[[nodiscard]] bool validRect(CreativeWorldLayoutRect rect) noexcept {
  return rect.minimum.x < rect.maximum.x && rect.minimum.z < rect.maximum.z;
}

[[nodiscard]] bool contains(CreativeWorldLayoutRect outer,
                            CreativeWorldLayoutRect inner) noexcept {
  return validRect(outer) && validRect(inner) &&
         inner.minimum.x >= outer.minimum.x &&
         inner.maximum.x <= outer.maximum.x &&
         inner.minimum.z >= outer.minimum.z &&
         inner.maximum.z <= outer.maximum.z;
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

[[nodiscard]] bool
hasLanding(const CreativeWorldLayoutRoom& lower,
           const CreativeWorldLayoutRoom& upper,
           CreativeWorldLayoutRect footprint,
           CreativeWorldLayoutVerticalDirection direction) noexcept {
  constexpr std::int64_t kLandingCells = 1;
  switch (direction) {
  case CreativeWorldLayoutVerticalDirection::PositiveX:
    return static_cast<std::int64_t>(footprint.minimum.x) -
                   lower.footprint.minimum.x >=
               kLandingCells &&
           static_cast<std::int64_t>(upper.footprint.maximum.x) -
                   footprint.maximum.x >=
               kLandingCells;
  case CreativeWorldLayoutVerticalDirection::NegativeX:
    return static_cast<std::int64_t>(lower.footprint.maximum.x) -
                   footprint.maximum.x >=
               kLandingCells &&
           static_cast<std::int64_t>(footprint.minimum.x) -
                   upper.footprint.minimum.x >=
               kLandingCells;
  case CreativeWorldLayoutVerticalDirection::PositiveZ:
    return static_cast<std::int64_t>(footprint.minimum.z) -
                   lower.footprint.minimum.z >=
               kLandingCells &&
           static_cast<std::int64_t>(upper.footprint.maximum.z) -
                   footprint.maximum.z >=
               kLandingCells;
  case CreativeWorldLayoutVerticalDirection::NegativeZ:
    return static_cast<std::int64_t>(lower.footprint.maximum.z) -
                   footprint.maximum.z >=
               kLandingCells &&
           static_cast<std::int64_t>(footprint.minimum.z) -
                   upper.footprint.minimum.z >=
               kLandingCells;
  case CreativeWorldLayoutVerticalDirection::Count:
    break;
  }
  return false;
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
                                         std::size_t connectorIndex) noexcept {
  CreativeWorldLayoutVerticalConnectorPlan plan;
  if (connectorIndex >= layout.verticalConnectors.size()) {
    reject(plan, CreativeWorldLayoutVerticalConnectorStatus::InvalidConnector,
           "creative_world_layout_vertical_connector_index_invalid");
    return plan;
  }
  const CreativeWorldLayoutVerticalConnector& connector =
      layout.verticalConnectors[connectorIndex];
  plan.buildingIndex = connector.buildingIndex;
  plan.lowerRoomIndex = connector.lowerRoomIndex;
  plan.upperRoomIndex = connector.upperRoomIndex;
  plan.openingFootprint = connector.footprint;

  if (connector.kind >= CreativeWorldLayoutVerticalConnectorKind::Count ||
      connector.direction >= CreativeWorldLayoutVerticalDirection::Count ||
      connector.name.empty() || connector.stableKey.empty()) {
    reject(plan, CreativeWorldLayoutVerticalConnectorStatus::InvalidConnector,
           "creative_world_layout_vertical_connector_invalid");
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
  if (!contains(lower.footprint, connector.footprint) ||
      !contains(upper.footprint, connector.footprint)) {
    reject(plan, CreativeWorldLayoutVerticalConnectorStatus::InvalidFootprint,
           "creative_world_layout_vertical_connector_footprint_invalid");
    return plan;
  }
  if (!hasLanding(lower, upper, connector.footprint, connector.direction)) {
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
      runCells < riseCells || widthCells < 1.0) {
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
    plan.stepCount = creativeGeneratedGeometrySegmentCount(
        describeObject(CreativeObjectKind::Stair),
        {plan.widthMeters, plan.riseMeters, plan.runMeters});
    if (plan.stepCount == 0U) {
      reject(
          plan,
          CreativeWorldLayoutVerticalConnectorStatus::UnrepresentableGeometry,
          "creative_world_layout_vertical_connector_steps_unrepresentable");
      return plan;
    }
  }

  plan.accepted = true;
  plan.status = CreativeWorldLayoutVerticalConnectorStatus::Ready;
  plan.reasonCode = "creative_world_layout_vertical_connector_ready";
  return plan;
}

} // namespace iggy3d::creative
