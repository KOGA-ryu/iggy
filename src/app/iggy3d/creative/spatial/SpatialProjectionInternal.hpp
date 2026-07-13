#pragma once

#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace iggy3d::creative::spatial_projection_internal {

struct CreativeSpatialProjectionPlan {
  CreativeSpatialProjectionSummary summary;
  std::string_view message = "projected";
};

[[nodiscard]] bool validCellSize(double cellSize) noexcept;
[[nodiscard]] bool isValidRequest(
    const CreativeSpatialProjectionRequest& request) noexcept;
[[nodiscard]] bool boundsOutsideGrid(CreativeGridBounds3 bounds,
                                     CreativeGridSize3 size) noexcept;
[[nodiscard]] std::uint64_t cellCount(
    CreativeGridBounds3 bounds) noexcept;
void fillBoundsCells(std::vector<CreativeSpatialCell>& cells,
                     CreativeGridSize3 size,
                     CreativeGridBounds3 bounds,
                     const CreativeObject& object,
                     CreativeSpatialOccupancyKind occupancyKind);
[[nodiscard]] CreativeGridBounds3 pointBounds(
    CreativeGridCoord3 coord) noexcept;
[[nodiscard]] CreativeGridBounds3 pointBoundsOrDefault(
    CreativeGridCoord3 coord) noexcept;
[[nodiscard]] CreativeGridBounds3 lineBounds(
    CreativeGridCoord3 start,
    CreativeGridCoord3 end) noexcept;
[[nodiscard]] CreativeGridBounds3 lineBoundsOrDefault(
    CreativeGridCoord3 start,
    CreativeGridCoord3 end) noexcept;
[[nodiscard]] CreativeGridBounds3 mergeBounds(
    CreativeGridBounds3 lhs,
    CreativeGridBounds3 rhs) noexcept;
[[nodiscard]] bool tryWorldToGridCoord(
    CreativeVec3 position,
    double cellSize,
    CreativeGridCoord3& out) noexcept;
[[nodiscard]] bool tryWorldBoundsToGridBounds(
    CreativeBounds bounds,
    double cellSize,
    CreativeGridBounds3& out) noexcept;

template <typename Callback>
void forEachSampledLineCoord(CreativeGridCoord3 start,
                             CreativeGridCoord3 end,
                             Callback&& callback) {
  const std::int32_t dx = end.x - start.x;
  const std::int32_t dy = end.y - start.y;
  const std::int32_t dz = end.z - start.z;
  const std::int32_t steps = std::max({std::abs(dx), std::abs(dy), std::abs(dz)});

  CreativeGridCoord3 previous{-1, -1, -1};
  for (std::int32_t step = 0; step <= steps; ++step) {
    const double t = steps == 0 ? 0.0 : static_cast<double>(step) / steps;
    const CreativeGridCoord3 coord{
        start.x + static_cast<std::int32_t>(std::round(dx * t)),
        start.y + static_cast<std::int32_t>(std::round(dy * t)),
        start.z + static_cast<std::int32_t>(std::round(dz * t)),
    };
    if (coord == previous) {
      continue;
    }
    previous = coord;
    callback(coord, step);
  }
}

void appendSampledLineCells(
    std::vector<CreativeSpatialCell>& cells,
    CreativeGridSize3 size,
    CreativeGridCoord3 start,
    CreativeGridCoord3 end,
    const CreativeObject& object,
    CreativeSpatialOccupancyKind occupancyKind,
    bool dedupeAgainstExisting);

[[nodiscard]] CreativeSpatialProjectionPlan makeProjectionPlanBase(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request,
    CreativeSpatialProjectionProfile profile,
    bool applyAuthoringPolicy);
[[nodiscard]] bool planIsReady(
    const CreativeSpatialProjectionPlan& plan) noexcept;
void setPlanStatus(CreativeSpatialProjectionPlan& plan,
                   CreativeSpatialProjectionStatus status,
                   CreativeGridBounds3 bounds,
                   std::string_view message) noexcept;
[[nodiscard]] CreativeSpatialProjectionPlan makeBoundsPlan(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request,
    CreativeSpatialProjectionProfile profile,
    bool applyAuthoringPolicy);
[[nodiscard]] CreativeSpatialProjectionPlan makePointPlan(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request,
    bool applyAuthoringPolicy);
[[nodiscard]] CreativeSpatialProjectionPlan makeLinePlan(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request,
    CreativeSpatialProjectionProfile profile,
    bool applyAuthoringPolicy);
[[nodiscard]] CreativeSpatialProjectionPlan makePathPlan(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request,
    bool applyAuthoringPolicy);
[[nodiscard]] CreativeSpatialProjectionPlan makeLinkPlan(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request,
    bool applyAuthoringPolicy);

}  // namespace iggy3d::creative::spatial_projection_internal
