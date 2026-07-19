#include "app/iggy3d/creative/recipes/TerrainGrounding.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <span>

namespace iggy3d::creative {
namespace {

[[nodiscard]] constexpr bool coordLess(CreativeTerrainCoord2 lhs,
                                       CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z != rhs.z ? lhs.z < rhs.z : lhs.x < rhs.x;
}

[[nodiscard]] bool validColumns(
    std::span<const CreativeTerrainColumn> columns) noexcept {
  for (std::size_t index = 0U; index < columns.size(); ++index) {
    if (columns[index].heightCells < kCreativeTerrainMinimumHeightCells ||
        columns[index].heightCells > kCreativeTerrainMaximumHeightCells ||
        (index > 0U &&
         !coordLess(columns[index - 1U].coord, columns[index].coord))) {
      return false;
    }
  }
  return true;
}

void reject(CreativeTerrainGroundingPlan& plan,
            CreativeTerrainGroundingStatus status,
            std::string_view reasonCode) noexcept {
  plan.status = status;
  plan.reasonCode = reasonCode;
}

}  // namespace

CreativeTerrainGroundingPlan planCreativeTerrainGrounding(
    const CreativeTerrainGroundingRequest& request) noexcept {
  CreativeTerrainGroundingPlan plan;
  plan.requested = true;

  const std::int64_t width =
      static_cast<std::int64_t>(request.maximum.x) - request.minimum.x;
  const std::int64_t depth =
      static_cast<std::int64_t>(request.maximum.z) - request.minimum.z;
  if (width <= 0 || depth <= 0 ||
      !std::isfinite(request.authoredGroundLayer)) {
    reject(plan, CreativeTerrainGroundingStatus::InvalidRequest,
           "creative_terrain_grounding_request_invalid");
    return plan;
  }
  if (width > static_cast<std::int64_t>(
                  kCreativeTerrainHeightFieldCellCapacity) ||
      depth > static_cast<std::int64_t>(
                  kCreativeTerrainHeightFieldCellCapacity) ||
      width > static_cast<std::int64_t>(
                  kCreativeTerrainHeightFieldCellCapacity) /
                  depth) {
    reject(plan, CreativeTerrainGroundingStatus::FootprintTooLarge,
           "creative_terrain_grounding_footprint_too_large");
    return plan;
  }

  const CreativeTerrainSurfacePlan* terrain = request.terrain;
  if (terrain == nullptr || !terrain->requested || !terrain->accepted ||
      (terrain->status != CreativeTerrainSurfacePlanStatus::Ready &&
       terrain->status != CreativeTerrainSurfacePlanStatus::Empty) ||
      (terrain->status == CreativeTerrainSurfacePlanStatus::Empty &&
       !terrain->columns.empty()) ||
      (terrain->status == CreativeTerrainSurfacePlanStatus::Ready &&
       terrain->columns.empty()) ||
      !validColumns(terrain->columns)) {
    reject(plan, CreativeTerrainGroundingStatus::InvalidTerrain,
           "creative_terrain_grounding_terrain_invalid");
    return plan;
  }

  if (terrain->status == CreativeTerrainSurfacePlanStatus::Empty) {
    plan.sampleCount = static_cast<std::size_t>(width * depth);
    plan.targetGroundLayer = 0.0;
    plan.verticalOffsetLayers = -request.authoredGroundLayer;
    plan.accepted = true;
    plan.status = CreativeTerrainGroundingStatus::Ready;
    plan.reasonCode = "creative_terrain_grounding_ready";
    return plan;
  }

  const auto findColumn = [&](CreativeTerrainCoord2 coord) {
    const auto found = std::lower_bound(
        terrain->columns.begin(), terrain->columns.end(), coord,
        [](const CreativeTerrainColumn& column, CreativeTerrainCoord2 value) {
          return coordLess(column.coord, value);
        });
    return found != terrain->columns.end() && found->coord == coord
               ? found
               : terrain->columns.end();
  };

  std::uint16_t minimumHeight =
      std::numeric_limits<std::uint16_t>::max();
  std::uint16_t maximumHeight = 0U;
  for (std::int64_t z = request.minimum.z; z < request.maximum.z; ++z) {
    for (std::int64_t x = request.minimum.x; x < request.maximum.x; ++x) {
      const auto found = findColumn(
          {static_cast<std::int32_t>(x), static_cast<std::int32_t>(z)});
      if (found == terrain->columns.end()) {
        reject(plan, CreativeTerrainGroundingStatus::MissingSurface,
               "creative_terrain_grounding_surface_missing");
        return plan;
      }
      minimumHeight = std::min(minimumHeight, found->heightCells);
      maximumHeight = std::max(maximumHeight, found->heightCells);
      ++plan.sampleCount;
    }
  }

  plan.minimumHeightCells = minimumHeight;
  plan.maximumHeightCells = maximumHeight;
  plan.reliefCells = static_cast<std::uint16_t>(maximumHeight - minimumHeight);
  if (plan.reliefCells > request.maximumReliefCells) {
    reject(plan, CreativeTerrainGroundingStatus::ReliefExceeded,
           "creative_terrain_grounding_relief_exceeded");
    return plan;
  }

  plan.targetGroundLayer = static_cast<double>(maximumHeight);
  plan.verticalOffsetLayers =
      plan.targetGroundLayer - request.authoredGroundLayer;
  plan.accepted = true;
  plan.status = CreativeTerrainGroundingStatus::Ready;
  plan.reasonCode = "creative_terrain_grounding_ready";
  return plan;
}

}  // namespace iggy3d::creative
