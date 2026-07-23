#include "app/iggy3d/creative/world/WorldLayoutArchitecture.hpp"

#include "app/iggy3d/creative/world/WorldLayoutBlockout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"
#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace iggy3d::creative {
namespace {

constexpr double kArchitectureEpsilon = 1.0e-9;

void reject(CreativeWorldLayoutArchitectureResult& result,
            CreativeWorldLayoutArchitectureStatus status,
            std::string_view reasonCode) {
  result.receipt.accepted = false;
  result.receipt.changed = false;
  result.receipt.status = status;
  result.receipt.reasonCode = reasonCode;
  result.edited = {};
}

bool validGrid(const CreativeGridSettings& grid) noexcept {
  return std::isfinite(grid.origin.x) && std::isfinite(grid.origin.y) &&
         std::isfinite(grid.origin.z) &&
         std::isfinite(grid.cellSizeMeters) && grid.cellSizeMeters > 0.0;
}

bool nearlyEqual(double lhs, double rhs) noexcept {
  return std::fabs(lhs - rhs) <= kArchitectureEpsilon;
}

bool resolveFloorToFloorCells(
    const CreativeGridSettings& grid,
    const CreativeWorldLayoutArchitecturalProfile& profile,
    std::uint16_t& output) noexcept {
  if (!validGrid(grid) ||
      !validCreativeWorldLayoutArchitecturalProfile(profile)) {
    return false;
  }
  const long double cells =
      static_cast<long double>(profile.floorToFloorMeters) /
      grid.cellSizeMeters;
  const long double rounded = std::round(cells);
  if (!std::isfinite(cells) || !std::isfinite(rounded) ||
      std::fabs(cells - rounded) > kArchitectureEpsilon || rounded < 1.0L ||
      rounded > std::numeric_limits<std::uint16_t>::max()) {
    return false;
  }
  output = static_cast<std::uint16_t>(rounded);
  return true;
}

bool buildingLevelFacts(const CreativeWorldLayout& source,
                        std::size_t buildingIndex,
                        double& lowestFloorTop,
                        std::size_t& levelCount) noexcept {
  lowestFloorTop = std::numeric_limits<double>::infinity();
  levelCount = 0U;
  for (std::size_t index = 0U; index < source.levels.size(); ++index) {
    const CreativeWorldLayoutLevel& level = source.levels[index];
    if (level.buildingIndex != buildingIndex) {
      continue;
    }
    if (!std::isfinite(level.floorTopLayer) || level.wallHeightCells == 0U) {
      return false;
    }
    for (std::size_t prior = 0U; prior < index; ++prior) {
      if (source.levels[prior].buildingIndex == buildingIndex &&
          nearlyEqual(source.levels[prior].floorTopLayer,
                      level.floorTopLayer)) {
        return false;
      }
    }
    lowestFloorTop = std::min(lowestFloorTop, level.floorTopLayer);
    ++levelCount;
  }
  return levelCount > 0U && std::isfinite(lowestFloorTop);
}

bool normalizedFloorTop(const CreativeWorldLayout& source,
                        std::size_t buildingIndex,
                        std::size_t levelIndex,
                        double lowestFloorTop,
                        std::uint16_t wallHeightCells,
                        double& output) noexcept {
  if (levelIndex >= source.levels.size() ||
      source.levels[levelIndex].buildingIndex != buildingIndex) {
    return false;
  }
  std::size_t rank = 0U;
  const double sourceFloorTop = source.levels[levelIndex].floorTopLayer;
  for (const CreativeWorldLayoutLevel& candidate : source.levels) {
    if (candidate.buildingIndex == buildingIndex &&
        candidate.floorTopLayer < sourceFloorTop - kArchitectureEpsilon) {
      ++rank;
    }
  }
  const long double value =
      static_cast<long double>(lowestFloorTop) +
      static_cast<long double>(rank) * wallHeightCells;
  if (!std::isfinite(value) ||
      value < -static_cast<long double>(std::numeric_limits<double>::max()) ||
      value > static_cast<long double>(std::numeric_limits<double>::max())) {
    return false;
  }
  output = static_cast<double>(value);
  return std::isfinite(output);
}

bool mapVerticalPlane(const CreativeWorldLayout& source,
                      std::size_t buildingIndex,
                      double lowestFloorTop,
                      std::uint16_t wallHeightCells,
                      double sourcePlane,
                      double& output) noexcept {
  bool found = false;
  double mapped = 0.0;
  for (std::size_t index = 0U; index < source.levels.size(); ++index) {
    const CreativeWorldLayoutLevel& level = source.levels[index];
    if (level.buildingIndex != buildingIndex) {
      continue;
    }
    double normalizedFloor = 0.0;
    if (!normalizedFloorTop(source, buildingIndex, index, lowestFloorTop,
                            wallHeightCells, normalizedFloor)) {
      return false;
    }
    const auto accept = [&](double candidate) {
      if (found && !nearlyEqual(mapped, candidate)) {
        return false;
      }
      found = true;
      mapped = candidate;
      return true;
    };
    if (nearlyEqual(sourcePlane, level.floorTopLayer) &&
        !accept(normalizedFloor)) {
      return false;
    }
    const double sourceWallTop =
        level.floorTopLayer + static_cast<double>(level.wallHeightCells);
    if (nearlyEqual(sourcePlane, sourceWallTop) &&
        !accept(normalizedFloor + wallHeightCells)) {
      return false;
    }
  }
  output = mapped;
  return found;
}

bool openingOwnedByBuilding(const CreativeWorldLayout& layout,
                            const CreativeWorldLayoutOpening& opening,
                            std::size_t buildingIndex) noexcept {
  if (opening.hostKind == CreativeWorldLayoutOpeningHostKind::Wall) {
    return opening.wallIndex < layout.walls.size() &&
           layout.walls[opening.wallIndex].buildingIndex == buildingIndex;
  }
  return opening.hostKind == CreativeWorldLayoutOpeningHostKind::RoomEdge &&
         opening.roomIndex < layout.rooms.size() &&
         layout.rooms[opening.roomIndex].buildingIndex == buildingIndex;
}

bool connectorRunsAlongX(
    CreativeWorldLayoutVerticalDirection direction) noexcept {
  return direction == CreativeWorldLayoutVerticalDirection::PositiveX ||
         direction == CreativeWorldLayoutVerticalDirection::NegativeX;
}

bool centeredSpan(std::int64_t allowedMinimum,
                  std::int64_t allowedMaximum,
                  std::int64_t sourceMinimum,
                  std::int64_t sourceMaximum,
                  std::int64_t requiredLength,
                  std::int32_t& outputMinimum,
                  std::int32_t& outputMaximum) noexcept {
  if (requiredLength <= 0 || allowedMaximum - allowedMinimum < requiredLength) {
    return false;
  }
  const std::int64_t centeredMinimum =
      sourceMinimum + (sourceMaximum - sourceMinimum - requiredLength) / 2;
  const std::int64_t resolvedMinimum = std::clamp(
      centeredMinimum, allowedMinimum, allowedMaximum - requiredLength);
  const std::int64_t resolvedMaximum = resolvedMinimum + requiredLength;
  if (resolvedMinimum < std::numeric_limits<std::int32_t>::min() ||
      resolvedMaximum > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  outputMinimum = static_cast<std::int32_t>(resolvedMinimum);
  outputMaximum = static_cast<std::int32_t>(resolvedMaximum);
  return true;
}

bool resizeVerticalConnector(
    const CreativeWorldLayout& layout,
    std::size_t connectorIndex,
    std::uint16_t riseCells,
    bool matchRiseExactly,
    CreativeWorldLayoutRect& output) noexcept {
  if (connectorIndex >= layout.verticalConnectors.size()) {
    return false;
  }
  const CreativeWorldLayoutVerticalConnector& connector =
      layout.verticalConnectors[connectorIndex];
  if (connector.lowerRoomIndex >= layout.rooms.size() ||
      connector.upperRoomIndex >= layout.rooms.size()) {
    return false;
  }
  const CreativeWorldLayoutRect lower =
      layout.rooms[connector.lowerRoomIndex].footprint;
  const CreativeWorldLayoutRect upper =
      layout.rooms[connector.upperRoomIndex].footprint;
  const std::int64_t minimumX =
      std::max<std::int64_t>(lower.minimum.x, upper.minimum.x) + 1;
  const std::int64_t maximumX =
      std::min<std::int64_t>(lower.maximum.x, upper.maximum.x) - 1;
  const std::int64_t minimumZ =
      std::max<std::int64_t>(lower.minimum.z, upper.minimum.z) + 1;
  const std::int64_t maximumZ =
      std::min<std::int64_t>(lower.maximum.z, upper.maximum.z) - 1;
  const std::int64_t sourceWidth =
      static_cast<std::int64_t>(connector.footprint.maximum.x) -
      connector.footprint.minimum.x;
  const std::int64_t sourceDepth =
      static_cast<std::int64_t>(connector.footprint.maximum.z) -
      connector.footprint.minimum.z;
  const bool alongX = connectorRunsAlongX(connector.direction);
  const std::int64_t sourceRun = alongX ? sourceWidth : sourceDepth;
  const std::int64_t requiredRun =
      matchRiseExactly ? riseCells : std::max<std::int64_t>(sourceRun,
                                                            riseCells);
  const std::int64_t requiredX = alongX ? requiredRun : sourceWidth;
  const std::int64_t requiredZ = alongX ? sourceDepth : requiredRun;
  return centeredSpan(minimumX, maximumX, connector.footprint.minimum.x,
                      connector.footprint.maximum.x, requiredX,
                      output.minimum.x, output.maximum.x) &&
         centeredSpan(minimumZ, maximumZ, connector.footprint.minimum.z,
                      connector.footprint.maximum.z, requiredZ,
                      output.minimum.z, output.maximum.z);
}

template <typename T>
bool assignChanged(T& destination, const T& value) {
  if (destination == value) {
    return false;
  }
  destination = value;
  return true;
}

}  // namespace

std::string_view toString(
    CreativeWorldLayoutArchitecturalProfileKind kind) noexcept {
  switch (kind) {
    case CreativeWorldLayoutArchitecturalProfileKind::Residential:
      return "Residential";
    case CreativeWorldLayoutArchitecturalProfileKind::Grand:
      return "Grand";
    case CreativeWorldLayoutArchitecturalProfileKind::Custom:
      return "Custom";
    case CreativeWorldLayoutArchitecturalProfileKind::Count:
      break;
  }
  return "Invalid";
}

std::string_view toString(
    CreativeWorldLayoutArchitectureStatus status) noexcept {
  switch (status) {
    case CreativeWorldLayoutArchitectureStatus::NotRequested:
      return "NotRequested";
    case CreativeWorldLayoutArchitectureStatus::InvalidGrid:
      return "InvalidGrid";
    case CreativeWorldLayoutArchitectureStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeWorldLayoutArchitectureStatus::InvalidOwnership:
      return "InvalidOwnership";
    case CreativeWorldLayoutArchitectureStatus::EmptyBuilding:
      return "EmptyBuilding";
    case CreativeWorldLayoutArchitectureStatus::DuplicateLevelElevation:
      return "DuplicateLevelElevation";
    case CreativeWorldLayoutArchitectureStatus::UnrepresentableProfile:
      return "UnrepresentableProfile";
    case CreativeWorldLayoutArchitectureStatus::UnalignedStructure:
      return "UnalignedStructure";
    case CreativeWorldLayoutArchitectureStatus::OpeningDoesNotFit:
      return "OpeningDoesNotFit";
    case CreativeWorldLayoutArchitectureStatus::VerticalConnectorInvalid:
      return "VerticalConnectorInvalid";
    case CreativeWorldLayoutArchitectureStatus::NoChange:
      return "NoChange";
    case CreativeWorldLayoutArchitectureStatus::Ready:
      return "Ready";
  }
  return "Invalid";
}

CreativeWorldLayoutArchitecturalProfile
defaultCreativeWorldLayoutArchitecturalProfile(
    CreativeWorldLayoutArchitecturalProfileKind kind) noexcept {
  CreativeWorldLayoutArchitecturalProfile profile;
  profile.kind = kind;
  switch (kind) {
    case CreativeWorldLayoutArchitecturalProfileKind::Residential:
    case CreativeWorldLayoutArchitecturalProfileKind::Custom:
      return profile;
    case CreativeWorldLayoutArchitecturalProfileKind::Grand:
      profile.floorToFloorMeters = 5.0;
      profile.floorThicknessLayers = 6U;
      profile.ceilingThicknessLayers = 1U;
      profile.roofThicknessLayers = 1U;
      return profile;
    case CreativeWorldLayoutArchitecturalProfileKind::Count:
      break;
  }
  return {};
}

bool validCreativeWorldLayoutArchitecturalProfile(
    const CreativeWorldLayoutArchitecturalProfile& profile) noexcept {
  return profile.kind < CreativeWorldLayoutArchitecturalProfileKind::Count &&
         std::isfinite(profile.floorToFloorMeters) &&
         profile.floorToFloorMeters > 0.0 &&
         profile.floorThicknessLayers > 0U &&
         profile.ceilingThicknessLayers > 0U &&
         profile.roofThicknessLayers > 0U;
}

bool resolveCreativeWorldLayoutArchitecturalProfileFloorToFloorCells(
    const CreativeGridSettings& grid,
    const CreativeWorldLayoutArchitecturalProfile& profile,
    std::uint16_t& output) noexcept {
  return resolveFloorToFloorCells(grid, profile, output);
}

CreativeWorldLayoutArchitectureResult
normalizeCreativeWorldLayoutBuildingArchitecture(
    const CreativeGridSettings& grid,
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutArchitectureRequest& request) {
  CreativeWorldLayoutArchitectureResult result;
  result.receipt.requested = true;
  result.receipt.buildingIndex = request.buildingIndex;
  if (!validGrid(grid)) {
    reject(result, CreativeWorldLayoutArchitectureStatus::InvalidGrid,
           "creative_world_layout_architecture_grid_invalid");
    return result;
  }
  if (request.buildingIndex >= source.buildings.size() ||
      !validCreativeWorldLayoutArchitecturalProfile(request.profile)) {
    reject(result, CreativeWorldLayoutArchitectureStatus::InvalidRequest,
           "creative_world_layout_architecture_request_invalid");
    return result;
  }
  if (!validCreativeWorldLayoutBuildingOwnership(source)) {
    reject(result, CreativeWorldLayoutArchitectureStatus::InvalidOwnership,
           "creative_world_layout_architecture_ownership_invalid");
    return result;
  }

  result.receipt.before = measureCreativeWorldLayoutBuildingDimensions(
      grid, source, request.buildingIndex);
  if (!result.receipt.before.accepted) {
    reject(result, CreativeWorldLayoutArchitectureStatus::EmptyBuilding,
           "creative_world_layout_architecture_building_empty");
    return result;
  }

  double lowestFloorTop = 0.0;
  if (!buildingLevelFacts(source, request.buildingIndex, lowestFloorTop,
                          result.receipt.updatedLevelCount)) {
    reject(result,
           CreativeWorldLayoutArchitectureStatus::DuplicateLevelElevation,
           "creative_world_layout_architecture_levels_invalid");
    return result;
  }
  if (!resolveFloorToFloorCells(
          grid, request.profile,
          result.receipt.resolvedFloorToFloorCells)) {
    reject(result,
           CreativeWorldLayoutArchitectureStatus::UnrepresentableProfile,
           "creative_world_layout_architecture_profile_unrepresentable");
    return result;
  }
  result.receipt.resolvedFloorToFloorMeters =
      result.receipt.resolvedFloorToFloorCells * grid.cellSizeMeters;

  const CreativeWorldLayoutBuildingBlockoutSyncReceipt blockoutSync =
      inspectCreativeWorldLayoutBuildingBlockoutSync(source,
                                                     request.buildingIndex);
  result.receipt.blockoutWasCurrent =
      blockoutSync.accepted &&
      blockoutSync.state ==
          CreativeWorldLayoutBuildingBlockoutSyncState::Current;

  CreativeWorldLayout edited = source;
  bool changed = false;
  for (std::size_t index = 0U; index < source.levels.size(); ++index) {
    if (source.levels[index].buildingIndex != request.buildingIndex) {
      continue;
    }
    double floorTop = 0.0;
    if (!normalizedFloorTop(source, request.buildingIndex, index,
                            lowestFloorTop,
                            result.receipt.resolvedFloorToFloorCells,
                            floorTop)) {
      reject(result, CreativeWorldLayoutArchitectureStatus::UnrepresentableProfile,
             "creative_world_layout_architecture_elevation_unrepresentable");
      return result;
    }
    CreativeWorldLayoutLevel& level = edited.levels[index];
    changed |= assignChanged(level.floorTopLayer, floorTop);
    changed |= assignChanged(level.wallHeightCells,
                             result.receipt.resolvedFloorToFloorCells);
    changed |= assignChanged(level.floorThicknessLayers,
                             request.profile.floorThicknessLayers);
    changed |= assignChanged(level.ceilingThicknessLayers,
                             request.profile.ceilingThicknessLayers);
    changed |= assignChanged(level.roofThicknessLayers,
                             request.profile.roofThicknessLayers);
  }

  CreativeWorldLayoutBuilding& building =
      edited.buildings[request.buildingIndex];
  changed |= assignChanged(building.rootHeightCells,
                           result.receipt.resolvedFloorToFloorCells);

  for (std::size_t index = 0U; index < source.walls.size(); ++index) {
    const CreativeWorldLayoutWall& sourceWall = source.walls[index];
    if (sourceWall.buildingIndex != request.buildingIndex) {
      continue;
    }
    double base = 0.0;
    double top = 0.0;
    if (!mapVerticalPlane(source, request.buildingIndex, lowestFloorTop,
                          result.receipt.resolvedFloorToFloorCells,
                          sourceWall.baseLayer, base) ||
        !mapVerticalPlane(
            source, request.buildingIndex, lowestFloorTop,
            result.receipt.resolvedFloorToFloorCells,
            sourceWall.baseLayer + sourceWall.heightCells, top)) {
      reject(result, CreativeWorldLayoutArchitectureStatus::UnalignedStructure,
             "creative_world_layout_architecture_wall_unaligned");
      return result;
    }
    const double height = top - base;
    const double rounded = std::round(height);
    if (!std::isfinite(height) || height <= 0.0 ||
        !nearlyEqual(height, rounded) ||
        rounded > std::numeric_limits<std::uint16_t>::max()) {
      reject(result, CreativeWorldLayoutArchitectureStatus::UnalignedStructure,
             "creative_world_layout_architecture_wall_unrepresentable");
      return result;
    }
    CreativeWorldLayoutWall& wall = edited.walls[index];
    changed |= assignChanged(wall.baseLayer, base);
    changed |= assignChanged(wall.heightCells,
                             static_cast<std::uint16_t>(rounded));
    ++result.receipt.updatedWallCount;
  }

  for (std::size_t index = 0U; index < source.boxes.size(); ++index) {
    const CreativeWorldLayoutBox& sourceBox = source.boxes[index];
    if (sourceBox.buildingIndex != request.buildingIndex) {
      continue;
    }
    const bool structural = sourceBox.kind == CreativeObjectKind::Floor ||
                            sourceBox.kind == CreativeObjectKind::Ceiling ||
                            sourceBox.kind == CreativeObjectKind::Roof;
    if (!structural) {
      continue;
    }
    double anchor = 0.0;
    if (!mapVerticalPlane(source, request.buildingIndex, lowestFloorTop,
                          result.receipt.resolvedFloorToFloorCells,
                          sourceBox.anchorLayer, anchor)) {
      reject(result,
             CreativeWorldLayoutArchitectureStatus::UnalignedStructure,
             "creative_world_layout_architecture_surface_unaligned");
      return result;
    }
    CreativeWorldLayoutBox& box = edited.boxes[index];
    changed |= assignChanged(box.anchorLayer, anchor);
    if (box.kind == CreativeObjectKind::Floor) {
      changed |= assignChanged(box.layerCount,
                               request.profile.floorThicknessLayers);
    } else if (box.kind == CreativeObjectKind::Ceiling) {
      changed |= assignChanged(box.layerCount,
                               request.profile.ceilingThicknessLayers);
    } else if (box.kind == CreativeObjectKind::Roof) {
      changed |= assignChanged(box.layerCount,
                               request.profile.roofThicknessLayers);
    }
    ++result.receipt.updatedStructuralBoxCount;
  }

  for (std::size_t index = 0U; index < edited.openings.size(); ++index) {
    if (!openingOwnedByBuilding(edited, edited.openings[index],
                                request.buildingIndex)) {
      continue;
    }
    const CreativeWorldLayoutOpeningDimensions opening =
        measureCreativeWorldLayoutOpeningDimensions(grid, edited, index);
    if (!opening.accepted) {
      reject(result, CreativeWorldLayoutArchitectureStatus::OpeningDoesNotFit,
             "creative_world_layout_architecture_opening_does_not_fit");
      return result;
    }
    ++result.receipt.validatedOpeningCount;
  }

  for (std::size_t index = 0U; index < edited.verticalConnectors.size();
       ++index) {
    if (edited.verticalConnectors[index].buildingIndex !=
        request.buildingIndex) {
      continue;
    }
    CreativeWorldLayoutRect footprint;
    if (!resizeVerticalConnector(
            edited, index, result.receipt.resolvedFloorToFloorCells,
            result.receipt.blockoutWasCurrent, footprint)) {
      reject(
          result,
          CreativeWorldLayoutArchitectureStatus::VerticalConnectorInvalid,
          "creative_world_layout_architecture_vertical_connector_does_not_fit");
      return result;
    }
    CreativeWorldLayoutRect& destination =
        edited.verticalConnectors[index].footprint;
    if (destination.minimum != footprint.minimum ||
        destination.maximum != footprint.maximum) {
      destination = footprint;
      changed = true;
      ++result.receipt.updatedVerticalConnectorCount;
    }
    if (!planCreativeWorldLayoutVerticalConnector(grid, edited, index)
             .accepted) {
      reject(
          result,
          CreativeWorldLayoutArchitectureStatus::VerticalConnectorInvalid,
          "creative_world_layout_architecture_vertical_connector_invalid");
      return result;
    }
    ++result.receipt.validatedVerticalConnectorCount;
  }

  result.receipt.after = measureCreativeWorldLayoutBuildingDimensions(
      grid, edited, request.buildingIndex);
  if (!result.receipt.after.accepted) {
    reject(result, CreativeWorldLayoutArchitectureStatus::UnrepresentableProfile,
           "creative_world_layout_architecture_result_invalid");
    return result;
  }

  if (changed && result.receipt.blockoutWasCurrent) {
    CreativeWorldLayoutBuildingBlockoutProvenance provenance =
        blockoutSync.provenance;
    provenance.recipe.request.floorToFloorCells =
        result.receipt.resolvedFloorToFloorCells;
    provenance.recipe.floorTopLayer = lowestFloorTop;
    provenance.recipe.floorThicknessLayers =
        request.profile.floorThicknessLayers;
    provenance.recipe.ceilingThicknessLayers =
        request.profile.ceilingThicknessLayers;
    provenance.recipe.roofThicknessLayers =
        request.profile.roofThicknessLayers;
    provenance.recipe.architecturalProfileKind = request.profile.kind;
    const CreativeWorldLayoutBuildingBlockoutFingerprint baseline =
        fingerprintCreativeWorldLayoutBuildingBlockout(
            edited, request.buildingIndex);
    provenance.instanceBaselineFingerprint = baseline.value;
    if (baseline.valid && provenance.valid &&
        validCreativeWorldLayoutBuildingBlockoutRecipe(provenance.recipe) &&
        setCreativeWorldLayoutBuildingBlockoutProvenance(
            edited, request.buildingIndex, provenance)) {
      result.receipt.preservedBlockoutLink = true;
    }
  } else if (result.receipt.blockoutWasCurrent && !changed) {
    result.receipt.preservedBlockoutLink = true;
  }

  result.receipt.accepted = true;
  result.receipt.changed = changed;
  result.receipt.status =
      changed ? CreativeWorldLayoutArchitectureStatus::Ready
              : CreativeWorldLayoutArchitectureStatus::NoChange;
  result.receipt.reasonCode =
      changed ? "creative_world_layout_architecture_ready"
              : "creative_world_layout_architecture_no_change";
  result.edited = std::move(edited);
  return result;
}

}  // namespace iggy3d::creative
