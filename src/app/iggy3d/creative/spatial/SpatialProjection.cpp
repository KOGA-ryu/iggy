#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <span>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool validCellSize(double cellSize) noexcept {
  return std::isfinite(cellSize) && cellSize > 0.0;
}

[[nodiscard]] bool isValidRequest(
    const CreativeSpatialProjectionRequest& request) noexcept {
  return isValidGridSize(request.gridSize) && validCellSize(request.cellSize);
}

[[nodiscard]] bool isValidObject(const CreativeObject& object) noexcept {
  return object.id != kInvalidObjectId &&
         object.kind != CreativeObjectKind::Unknown;
}

[[nodiscard]] bool resolvedWorldBounds(const CreativeObject& object,
                                       CreativeBounds& out) noexcept {
  const CreativeTransformedBounds resolved =
      resolveCreativeObjectBounds(object);
  if (!resolved.valid) {
    return false;
  }
  out = resolved.worldBounds;
  return true;
}

struct CreativeSpatialProjectionPlan {
  CreativeSpatialProjectionSummary summary;
  std::string_view message = "projected";
};

[[nodiscard]] CreativeSpatialProjectionReceipt makeReceipt(
    CreativeSpatialProjectionStatus status,
    const CreativeObject& object,
    CreativeSpatialProjectionProfile profile,
    CreativeSpatialOccupancyKind occupancyKind,
    CreativeGridBounds3 projectedBounds,
    std::string message) {
  CreativeSpatialProjectionReceipt receipt;
  receipt.status = status;
  receipt.objectId = object.id;
  receipt.objectKind = object.kind;
  receipt.profile = profile;
  receipt.occupancyKind = occupancyKind;
  receipt.projectedBounds = projectedBounds;
  receipt.message = std::move(message);
  return receipt;
}

[[nodiscard]] CreativeSpatialProjectionReceipt makeAggregateReceipt(
    CreativeSpatialProjectionStatus status,
    std::string message) {
  CreativeObject object;
  return makeReceipt(status,
                     object,
                     CreativeSpatialProjectionProfile::Unknown,
                     CreativeSpatialOccupancyKind::Unknown,
                     {},
                     std::move(message));
}

[[nodiscard]] CreativeSpatialProjectionPlan makeProjectionPlanBase(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request,
    CreativeSpatialProjectionProfile profile,
    bool applyAuthoringPolicy) {
  CreativeSpatialProjectionPlan plan;
  plan.summary.objectId = object.id;
  plan.summary.objectKind = object.kind;
  plan.summary.profile = profile;
  plan.summary.occupancyKind = occupancyKindForObject(object.kind);

  if (!isValidRequest(request)) {
    plan.summary.status = CreativeSpatialProjectionStatus::InvalidGrid;
    plan.message = "invalid_grid";
    return plan;
  }
  if (!isValidObject(object)) {
    plan.summary.status = CreativeSpatialProjectionStatus::InvalidObject;
    plan.message = "invalid_object";
    return plan;
  }
  if (!object.visible) {
    plan.summary.status = CreativeSpatialProjectionStatus::NoProjection;
    plan.message = "object_hidden";
    return plan;
  }
  if (applyAuthoringPolicy && !request.includeAuthoringOnly &&
      plan.summary.occupancyKind == CreativeSpatialOccupancyKind::Authoring) {
    plan.summary.status = CreativeSpatialProjectionStatus::NoProjection;
    plan.message = "authoring_excluded";
    return plan;
  }

  return plan;
}

[[nodiscard]] bool planIsReady(
    const CreativeSpatialProjectionPlan& plan) noexcept {
  return plan.summary.status == CreativeSpatialProjectionStatus::Unknown;
}

void setPlanStatus(CreativeSpatialProjectionPlan& plan,
                   CreativeSpatialProjectionStatus status,
                   CreativeGridBounds3 bounds,
                   std::string_view message) noexcept {
  plan.summary.status = status;
  plan.summary.projectedBounds = bounds;
  plan.message = message;
}

[[nodiscard]] bool boundsOutsideGrid(CreativeGridBounds3 bounds,
                                     CreativeGridSize3 size) noexcept {
  return bounds.min.x < 0 || bounds.min.y < 0 || bounds.min.z < 0 ||
         bounds.max.x > size.width || bounds.max.y > size.height ||
         bounds.max.z > size.depth;
}

[[nodiscard]] std::uint64_t cellCount(CreativeGridBounds3 bounds) noexcept {
  const auto width = static_cast<std::uint64_t>(bounds.max.x - bounds.min.x);
  const auto height = static_cast<std::uint64_t>(bounds.max.y - bounds.min.y);
  const auto depth = static_cast<std::uint64_t>(bounds.max.z - bounds.min.z);
  return width * height * depth;
}

void fillBoundsCells(std::vector<CreativeSpatialCell>& cells,
                     CreativeGridSize3 size,
                     CreativeGridBounds3 bounds,
                     const CreativeObject& object,
                     CreativeSpatialOccupancyKind occupancyKind) {
  for (std::int32_t z = bounds.min.z; z < bounds.max.z; ++z) {
    for (std::int32_t y = bounds.min.y; y < bounds.max.y; ++y) {
      for (std::int32_t x = bounds.min.x; x < bounds.max.x; ++x) {
        const CreativeGridCoord3 coord{x, y, z};
        cells.push_back(CreativeSpatialCell{toGridIndex(coord, size),
                                            coord,
                                            object.id,
                                            object.kind,
                                            occupancyKind});
      }
    }
  }
}

[[nodiscard]] CreativeGridBounds3 pointBounds(CreativeGridCoord3 coord) noexcept {
  return CreativeGridBounds3{
      coord,
      CreativeGridCoord3{coord.x + 1, coord.y + 1, coord.z + 1},
  };
}

[[nodiscard]] bool canExpandCell(CreativeGridCoord3 coord) noexcept {
  constexpr std::int32_t maxCell = std::numeric_limits<std::int32_t>::max();
  return coord.x < maxCell && coord.y < maxCell && coord.z < maxCell;
}

[[nodiscard]] CreativeGridBounds3 pointBoundsOrDefault(
    CreativeGridCoord3 coord) noexcept {
  return canExpandCell(coord) ? pointBounds(coord) : CreativeGridBounds3{};
}

[[nodiscard]] CreativeGridBounds3 lineBounds(CreativeGridCoord3 start,
                                             CreativeGridCoord3 end) noexcept {
  return CreativeGridBounds3{
      CreativeGridCoord3{
          std::min(start.x, end.x),
          std::min(start.y, end.y),
          std::min(start.z, end.z),
      },
      CreativeGridCoord3{
          std::max(start.x, end.x) + 1,
          std::max(start.y, end.y) + 1,
          std::max(start.z, end.z) + 1,
      },
  };
}

[[nodiscard]] CreativeGridBounds3 lineBoundsOrDefault(
    CreativeGridCoord3 start,
    CreativeGridCoord3 end) noexcept {
  const CreativeGridCoord3 maxCoord{
      std::max(start.x, end.x),
      std::max(start.y, end.y),
      std::max(start.z, end.z),
  };
  return canExpandCell(maxCoord) ? lineBounds(start, end)
                                 : CreativeGridBounds3{};
}

[[nodiscard]] CreativeGridBounds3 mergeBounds(CreativeGridBounds3 lhs,
                                              CreativeGridBounds3 rhs) noexcept {
  return CreativeGridBounds3{
      CreativeGridCoord3{
          std::min(lhs.min.x, rhs.min.x),
          std::min(lhs.min.y, rhs.min.y),
          std::min(lhs.min.z, rhs.min.z),
      },
      CreativeGridCoord3{
          std::max(lhs.max.x, rhs.max.x),
          std::max(lhs.max.y, rhs.max.y),
          std::max(lhs.max.z, rhs.max.z),
      },
  };
}

[[nodiscard]] bool finiteVec3(CreativeVec3 value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

[[nodiscard]] bool checkedInt32(double value, std::int32_t& out) noexcept {
  if (!std::isfinite(value) ||
      value < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      value > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
    return false;
  }
  out = static_cast<std::int32_t>(value);
  return true;
}

[[nodiscard]] bool tryWorldToGridCoord(CreativeVec3 position,
                                       double cellSize,
                                       CreativeGridCoord3& out) noexcept {
  if (!validCellSize(cellSize) || !finiteVec3(position)) {
    return false;
  }

  CreativeGridCoord3 coord;
  if (!checkedInt32(std::floor(position.x / cellSize), coord.x) ||
      !checkedInt32(std::floor(position.y / cellSize), coord.y) ||
      !checkedInt32(std::floor(position.z / cellSize), coord.z)) {
    return false;
  }

  out = coord;
  return true;
}

[[nodiscard]] bool tryWorldBoundsToGridBounds(
    CreativeBounds bounds,
    double cellSize,
    CreativeGridBounds3& out) noexcept {
  if (!validCellSize(cellSize) || !finiteVec3(bounds.min) ||
      !finiteVec3(bounds.max)) {
    return false;
  }

  CreativeGridBounds3 gridBounds;
  if (!checkedInt32(std::floor(bounds.min.x / cellSize), gridBounds.min.x) ||
      !checkedInt32(std::floor(bounds.min.y / cellSize), gridBounds.min.y) ||
      !checkedInt32(std::floor(bounds.min.z / cellSize), gridBounds.min.z) ||
      !checkedInt32(std::ceil(bounds.max.x / cellSize), gridBounds.max.x) ||
      !checkedInt32(std::ceil(bounds.max.y / cellSize), gridBounds.max.y) ||
      !checkedInt32(std::ceil(bounds.max.z / cellSize), gridBounds.max.z)) {
    return false;
  }

  out = gridBounds;
  return true;
}

[[nodiscard]] bool pathPointsAreValid(
    std::span<const CreativePathPoint> pathPoints) noexcept {
  if (pathPoints.size() < 2U) {
    return false;
  }

  return std::all_of(pathPoints.begin(),
                     pathPoints.end(),
                     [](const CreativePathPoint& point) {
                       return finiteVec3(point.position);
                     });
}

[[nodiscard]] bool lineEndpointsAreValid(
    std::span<const CreativePathPoint> pathPoints) noexcept {
  return pathPoints.size() == 2U &&
         std::all_of(pathPoints.begin(),
                     pathPoints.end(),
                     [](const CreativePathPoint& point) {
                       return finiteVec3(point.position);
                     });
}

[[nodiscard]] bool sameCoord(CreativeGridCoord3 lhs,
                             CreativeGridCoord3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

[[nodiscard]] bool containsCoord(std::span<const CreativeSpatialCell> cells,
                                 CreativeGridCoord3 coord) noexcept {
  return std::any_of(cells.begin(),
                     cells.end(),
                     [coord](const CreativeSpatialCell& cell) {
                       return sameCoord(cell.coord, coord);
                     });
}

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
    if (sameCoord(coord, previous)) {
      continue;
    }
    previous = coord;
    callback(coord, step);
  }
}

void appendSampledLineCells(std::vector<CreativeSpatialCell>& cells,
                            CreativeGridSize3 size,
                            CreativeGridCoord3 start,
                            CreativeGridCoord3 end,
                            const CreativeObject& object,
                            CreativeSpatialOccupancyKind occupancyKind,
                            bool dedupeAgainstExisting) {
  forEachSampledLineCoord(
      start,
      end,
      [&](CreativeGridCoord3 coord, std::int32_t) {
        if (dedupeAgainstExisting && containsCoord(cells, coord)) {
          return;
        }
        cells.push_back(CreativeSpatialCell{toGridIndex(coord, size),
                                            coord,
                                            object.id,
                                            object.kind,
                                            occupancyKind});
      });
}

[[nodiscard]] bool pathPointGridCoord(
    std::span<const CreativePathPoint> pathPoints,
    std::size_t index,
    double cellSize,
    CreativeGridCoord3& coord) noexcept {
  return tryWorldToGridCoord(pathPoints[index].position, cellSize, coord);
}

[[nodiscard]] bool pathCoordSeenBefore(
    std::span<const CreativePathPoint> pathPoints,
    double cellSize,
    std::size_t segmentIndex,
    std::int32_t sampleStep,
    CreativeGridCoord3 candidate) noexcept {
  for (std::size_t priorSegment = 0; priorSegment <= segmentIndex;
       ++priorSegment) {
    CreativeGridCoord3 start;
    CreativeGridCoord3 end;
    if (!pathPointGridCoord(pathPoints, priorSegment, cellSize, start) ||
        !pathPointGridCoord(pathPoints, priorSegment + 1U, cellSize, end)) {
      return false;
    }
    bool found = false;
    forEachSampledLineCoord(
        start,
        end,
        [&](CreativeGridCoord3 coord, std::int32_t priorStep) {
          if (priorSegment < segmentIndex || priorStep < sampleStep) {
            found = found || sameCoord(coord, candidate);
          }
        });
    if (found) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::uint64_t countPathCells(
    std::span<const CreativePathPoint> pathPoints,
    double cellSize) noexcept {
  std::uint64_t count = 0;
  for (std::size_t segmentIndex = 0; segmentIndex + 1U < pathPoints.size();
       ++segmentIndex) {
    CreativeGridCoord3 start;
    CreativeGridCoord3 end;
    if (!pathPointGridCoord(pathPoints, segmentIndex, cellSize, start) ||
        !pathPointGridCoord(pathPoints, segmentIndex + 1U, cellSize, end)) {
      return 0;
    }
    forEachSampledLineCoord(
        start,
        end,
        [&](CreativeGridCoord3 coord, std::int32_t sampleStep) {
          if (!pathCoordSeenBefore(pathPoints,
                                   cellSize,
                                   segmentIndex,
                                   sampleStep,
                                   coord)) {
            ++count;
          }
        });
  }
  return count;
}

[[nodiscard]] CreativeSpatialProjectionPlan makeBoundsPlan(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request,
    CreativeSpatialProjectionProfile profile,
    bool applyAuthoringPolicy) {
  CreativeSpatialProjectionPlan plan =
      makeProjectionPlanBase(object, request, profile, applyAuthoringPolicy);
  if (!planIsReady(plan)) {
    return plan;
  }

  CreativeGridBounds3 bounds{};
  CreativeBounds worldBounds{};
  if (!resolvedWorldBounds(object, worldBounds) ||
      !tryWorldBoundsToGridBounds(worldBounds, request.cellSize, bounds)) {
    setPlanStatus(plan,
                  CreativeSpatialProjectionStatus::OutOfBounds,
                  {},
                  "out_of_bounds");
    return plan;
  }
  if (!request.clampToGrid && boundsOutsideGrid(bounds, request.gridSize)) {
    setPlanStatus(plan,
                  CreativeSpatialProjectionStatus::OutOfBounds,
                  bounds,
                  "out_of_bounds");
    return plan;
  }

  bounds = clampGridBounds(bounds, request.gridSize);
  if (isEmptyGridBounds(bounds)) {
    setPlanStatus(plan,
                  CreativeSpatialProjectionStatus::EmptyProjection,
                  bounds,
                  "empty_projection");
    return plan;
  }
  plan.summary.cellCount = cellCount(bounds);
  setPlanStatus(plan,
                CreativeSpatialProjectionStatus::Projected,
                bounds,
                "projected");
  return plan;
}

[[nodiscard]] CreativeSpatialProjectionPlan makePointPlan(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request,
    bool applyAuthoringPolicy) {
  CreativeSpatialProjectionPlan plan = makeProjectionPlanBase(
      object,
      request,
      CreativeSpatialProjectionProfile::PointProjection,
      applyAuthoringPolicy);
  if (!planIsReady(plan)) {
    return plan;
  }

  CreativeGridCoord3 coord;
  if (!tryWorldToGridCoord(object.transform.position, request.cellSize, coord)) {
    setPlanStatus(plan,
                  CreativeSpatialProjectionStatus::OutOfBounds,
                  {},
                  "out_of_bounds");
    return plan;
  }
  if (!isInsideGrid(coord, request.gridSize)) {
    setPlanStatus(plan,
                  CreativeSpatialProjectionStatus::OutOfBounds,
                  pointBoundsOrDefault(coord),
                  "out_of_bounds");
    return plan;
  }
  plan.summary.cellCount = 1;
  setPlanStatus(plan,
                CreativeSpatialProjectionStatus::Projected,
                pointBounds(coord),
                "projected");
  return plan;
}

[[nodiscard]] CreativeSpatialProjectionPlan makeLinePlan(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request,
    CreativeSpatialProjectionProfile profile,
    bool applyAuthoringPolicy) {
  CreativeSpatialProjectionPlan plan =
      makeProjectionPlanBase(object, request, profile, applyAuthoringPolicy);
  if (!planIsReady(plan)) {
    return plan;
  }

  CreativeGridCoord3 start;
  CreativeGridCoord3 end;
  CreativeBounds worldBounds{};
  if (!resolvedWorldBounds(object, worldBounds) ||
      !tryWorldToGridCoord(worldBounds.min, request.cellSize, start) ||
      !tryWorldToGridCoord(worldBounds.max, request.cellSize, end)) {
    setPlanStatus(plan,
                  CreativeSpatialProjectionStatus::OutOfBounds,
                  {},
                  "out_of_bounds");
    return plan;
  }
  if (!isInsideGrid(start, request.gridSize) ||
      !isInsideGrid(end, request.gridSize)) {
    setPlanStatus(plan,
                  CreativeSpatialProjectionStatus::OutOfBounds,
                  lineBoundsOrDefault(start, end),
                  "out_of_bounds");
    return plan;
  }

  std::uint64_t count = 0;
  forEachSampledLineCoord(start, end, [&](CreativeGridCoord3, std::int32_t) {
    ++count;
  });
  setPlanStatus(plan,
                count == 0 ? CreativeSpatialProjectionStatus::EmptyProjection
                           : CreativeSpatialProjectionStatus::Projected,
                lineBounds(start, end),
                count == 0 ? "empty_projection" : "projected");
  plan.summary.cellCount = count;
  return plan;
}

[[nodiscard]] CreativeSpatialProjectionPlan makePathPlan(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request,
    bool applyAuthoringPolicy) {
  CreativeSpatialProjectionPlan plan = makeProjectionPlanBase(
      object,
      request,
      CreativeSpatialProjectionProfile::PathProjection,
      applyAuthoringPolicy);
  if (!planIsReady(plan)) {
    return plan;
  }
  if (!pathPointsAreValid(object.pathPoints)) {
    setPlanStatus(plan,
                  CreativeSpatialProjectionStatus::EmptyProjection,
                  {},
                  "invalid_path_points");
    return plan;
  }

  CreativeGridCoord3 firstStart;
  CreativeGridCoord3 firstEnd;
  if (!pathPointGridCoord(object.pathPoints, 0U, request.cellSize, firstStart) ||
      !pathPointGridCoord(object.pathPoints, 1U, request.cellSize, firstEnd)) {
    setPlanStatus(plan,
                  CreativeSpatialProjectionStatus::OutOfBounds,
                  {},
                  "out_of_bounds");
    return plan;
  }
  CreativeGridBounds3 projectedBounds = lineBounds(firstStart, firstEnd);
  for (std::size_t index = 1; index + 1U < object.pathPoints.size(); ++index) {
    CreativeGridCoord3 coord;
    CreativeGridCoord3 next;
    if (!pathPointGridCoord(object.pathPoints, index, request.cellSize, coord) ||
        !pathPointGridCoord(object.pathPoints, index + 1U, request.cellSize, next)) {
      setPlanStatus(plan,
                    CreativeSpatialProjectionStatus::OutOfBounds,
                    {},
                    "out_of_bounds");
      return plan;
    }
    projectedBounds = mergeBounds(projectedBounds, lineBounds(coord, next));
  }

  for (std::size_t index = 0; index < object.pathPoints.size(); ++index) {
    CreativeGridCoord3 coord;
    if (!pathPointGridCoord(object.pathPoints, index, request.cellSize, coord)) {
      setPlanStatus(plan,
                    CreativeSpatialProjectionStatus::OutOfBounds,
                    {},
                    "out_of_bounds");
      return plan;
    }
    if (!isInsideGrid(coord, request.gridSize)) {
      setPlanStatus(plan,
                    CreativeSpatialProjectionStatus::OutOfBounds,
                    pointBoundsOrDefault(coord),
                    "out_of_bounds");
      return plan;
    }
  }

  plan.summary.cellCount = countPathCells(object.pathPoints, request.cellSize);
  setPlanStatus(plan,
                plan.summary.cellCount == 0
                    ? CreativeSpatialProjectionStatus::EmptyProjection
                    : CreativeSpatialProjectionStatus::Projected,
                projectedBounds,
                plan.summary.cellCount == 0 ? "empty_projection" : "projected");
  return plan;
}

[[nodiscard]] CreativeSpatialProjectionPlan makeLinkPlan(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request,
    bool applyAuthoringPolicy) {
  CreativeSpatialProjectionPlan plan = makeProjectionPlanBase(
      object,
      request,
      CreativeSpatialProjectionProfile::LinkProjection,
      applyAuthoringPolicy);
  if (!planIsReady(plan)) {
    return plan;
  }
  if (!lineEndpointsAreValid(object.pathPoints)) {
    setPlanStatus(plan,
                  CreativeSpatialProjectionStatus::EmptyProjection,
                  {},
                  "invalid_line_endpoints");
    return plan;
  }

  CreativeGridCoord3 start;
  CreativeGridCoord3 end;
  if (!pathPointGridCoord(object.pathPoints, 0U, request.cellSize, start) ||
      !pathPointGridCoord(object.pathPoints, 1U, request.cellSize, end)) {
    setPlanStatus(plan,
                  CreativeSpatialProjectionStatus::OutOfBounds,
                  {},
                  "out_of_bounds");
    return plan;
  }
  if (!isInsideGrid(start, request.gridSize) ||
      !isInsideGrid(end, request.gridSize)) {
    setPlanStatus(plan,
                  CreativeSpatialProjectionStatus::OutOfBounds,
                  lineBoundsOrDefault(start, end),
                  "out_of_bounds");
    return plan;
  }

  std::uint64_t count = 0;
  forEachSampledLineCoord(start, end, [&](CreativeGridCoord3, std::int32_t) {
    ++count;
  });
  setPlanStatus(plan,
                count == 0 ? CreativeSpatialProjectionStatus::EmptyProjection
                           : CreativeSpatialProjectionStatus::Projected,
                lineBounds(start, end),
                count == 0 ? "empty_projection" : "projected");
  plan.summary.cellCount = count;
  return plan;
}

[[nodiscard]] CreativeSpatialProjectionReceipt materializePlan(
    const CreativeSpatialProjectionPlan& plan,
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request) {
  CreativeSpatialProjectionReceipt receipt;
  receipt.status = plan.summary.status;
  receipt.objectId = plan.summary.objectId;
  receipt.objectKind = plan.summary.objectKind;
  receipt.profile = plan.summary.profile;
  receipt.occupancyKind = plan.summary.occupancyKind;
  receipt.projectedBounds = plan.summary.projectedBounds;
  receipt.message = std::string(plan.message);
  if (plan.summary.status != CreativeSpatialProjectionStatus::Projected) {
    return receipt;
  }

  receipt.cells.reserve(static_cast<std::size_t>(plan.summary.cellCount));
  switch (plan.summary.profile) {
    case CreativeSpatialProjectionProfile::PointProjection: {
      const CreativeGridCoord3 coord = worldToGridCoord(
          object.transform.position, request.cellSize);
      receipt.cells.push_back(CreativeSpatialCell{toGridIndex(coord,
                                                               request.gridSize),
                                                  coord,
                                                  object.id,
                                                  object.kind,
                                                  plan.summary.occupancyKind});
      break;
    }
    case CreativeSpatialProjectionProfile::BoxProjection:
    case CreativeSpatialProjectionProfile::VolumeProjection:
      fillBoundsCells(receipt.cells,
                      request.gridSize,
                      plan.summary.projectedBounds,
                      object,
                      plan.summary.occupancyKind);
      break;
    case CreativeSpatialProjectionProfile::LineProjection: {
      const CreativeTransformedBounds resolved =
          resolveCreativeObjectBounds(object);
      if (!resolved.valid) {
        break;
      }
      const CreativeGridCoord3 start =
          worldToGridCoord(resolved.worldBounds.min, request.cellSize);
      const CreativeGridCoord3 end =
          worldToGridCoord(resolved.worldBounds.max, request.cellSize);
      appendSampledLineCells(receipt.cells,
                             request.gridSize,
                             start,
                             end,
                             object,
                             plan.summary.occupancyKind,
                             false);
      break;
    }
    case CreativeSpatialProjectionProfile::PathProjection:
      for (std::size_t index = 0; index + 1U < object.pathPoints.size(); ++index) {
        const CreativeGridCoord3 start =
            worldToGridCoord(object.pathPoints[index].position, request.cellSize);
        const CreativeGridCoord3 end = worldToGridCoord(
            object.pathPoints[index + 1U].position, request.cellSize);
        appendSampledLineCells(receipt.cells,
                               request.gridSize,
                               start,
                               end,
                               object,
                               plan.summary.occupancyKind,
                               true);
      }
      break;
    case CreativeSpatialProjectionProfile::LinkProjection: {
      const CreativeGridCoord3 start =
          worldToGridCoord(object.pathPoints[0].position, request.cellSize);
      const CreativeGridCoord3 end = worldToGridCoord(
          object.pathPoints[1].position, request.cellSize);
      appendSampledLineCells(receipt.cells,
                             request.gridSize,
                             start,
                             end,
                             object,
                             plan.summary.occupancyKind,
                             true);
      break;
    }
    case CreativeSpatialProjectionProfile::Unknown:
    case CreativeSpatialProjectionProfile::NoProjection:
      break;
  }
  return receipt;
}

[[nodiscard]] CreativeSpatialProjectionStatus mergeAggregateStatus(
    CreativeSpatialProjectionStatus current,
    CreativeSpatialProjectionStatus next) noexcept {
  if (current == CreativeSpatialProjectionStatus::Unknown) {
    return next;
  }
  if (current == CreativeSpatialProjectionStatus::InvalidGrid ||
      next == CreativeSpatialProjectionStatus::InvalidGrid) {
    return CreativeSpatialProjectionStatus::InvalidGrid;
  }
  if (current == CreativeSpatialProjectionStatus::InvalidObject ||
      next == CreativeSpatialProjectionStatus::InvalidObject) {
    return CreativeSpatialProjectionStatus::InvalidObject;
  }
  if (current == CreativeSpatialProjectionStatus::OutOfBounds ||
      next == CreativeSpatialProjectionStatus::OutOfBounds) {
    return CreativeSpatialProjectionStatus::OutOfBounds;
  }
  if (current == CreativeSpatialProjectionStatus::EmptyProjection ||
      next == CreativeSpatialProjectionStatus::EmptyProjection) {
    return CreativeSpatialProjectionStatus::EmptyProjection;
  }
  return CreativeSpatialProjectionStatus::NoProjection;
}

}  // namespace

std::string_view toString(CreativeSpatialProjectionProfile profile) noexcept {
  switch (profile) {
    case CreativeSpatialProjectionProfile::Unknown:
      return "Unknown";
    case CreativeSpatialProjectionProfile::NoProjection:
      return "NoProjection";
    case CreativeSpatialProjectionProfile::PointProjection:
      return "PointProjection";
    case CreativeSpatialProjectionProfile::BoxProjection:
      return "BoxProjection";
    case CreativeSpatialProjectionProfile::VolumeProjection:
      return "VolumeProjection";
    case CreativeSpatialProjectionProfile::LineProjection:
      return "LineProjection";
    case CreativeSpatialProjectionProfile::PathProjection:
      return "PathProjection";
    case CreativeSpatialProjectionProfile::LinkProjection:
      return "LinkProjection";
  }
  return "Unknown";
}

std::string_view toString(
    CreativeSpatialOccupancyKind occupancyKind) noexcept {
  switch (occupancyKind) {
    case CreativeSpatialOccupancyKind::Unknown:
      return "Unknown";
    case CreativeSpatialOccupancyKind::Structural:
      return "Structural";
    case CreativeSpatialOccupancyKind::Collision:
      return "Collision";
    case CreativeSpatialOccupancyKind::Navigation:
      return "Navigation";
    case CreativeSpatialOccupancyKind::Trigger:
      return "Trigger";
    case CreativeSpatialOccupancyKind::Gameplay:
      return "Gameplay";
    case CreativeSpatialOccupancyKind::Light:
      return "Light";
    case CreativeSpatialOccupancyKind::Audio:
      return "Audio";
    case CreativeSpatialOccupancyKind::Camera:
      return "Camera";
    case CreativeSpatialOccupancyKind::Testing:
      return "Testing";
    case CreativeSpatialOccupancyKind::Authoring:
      return "Authoring";
  }
  return "Unknown";
}

std::string_view toString(CreativeSpatialProjectionStatus status) noexcept {
  switch (status) {
    case CreativeSpatialProjectionStatus::Unknown:
      return "Unknown";
    case CreativeSpatialProjectionStatus::InvalidGrid:
      return "InvalidGrid";
    case CreativeSpatialProjectionStatus::InvalidObject:
      return "InvalidObject";
    case CreativeSpatialProjectionStatus::NoProjection:
      return "NoProjection";
    case CreativeSpatialProjectionStatus::EmptyProjection:
      return "EmptyProjection";
    case CreativeSpatialProjectionStatus::OutOfBounds:
      return "OutOfBounds";
    case CreativeSpatialProjectionStatus::Projected:
      return "Projected";
  }
  return "Unknown";
}

bool isValidGridSize(CreativeGridSize3 size) noexcept {
  return size.width > 0 && size.height > 0 && size.depth > 0;
}

bool isInsideGrid(CreativeGridCoord3 coord, CreativeGridSize3 size) noexcept {
  return coord.x >= 0 && coord.x < size.width && coord.y >= 0 &&
         coord.y < size.height && coord.z >= 0 && coord.z < size.depth;
}

CreativeGridIndex toGridIndex(CreativeGridCoord3 coord,
                              CreativeGridSize3 size) noexcept {
  const auto width = static_cast<CreativeGridIndex>(size.width);
  const auto height = static_cast<CreativeGridIndex>(size.height);
  return static_cast<CreativeGridIndex>(coord.z) * width * height +
         static_cast<CreativeGridIndex>(coord.y) * width +
         static_cast<CreativeGridIndex>(coord.x);
}

CreativeGridCoord3 toGridCoord(CreativeGridIndex index,
                               CreativeGridSize3 size) noexcept {
  if (size.width <= 0 || size.height <= 0) {
    return {};
  }
  const auto width = static_cast<CreativeGridIndex>(size.width);
  const auto height = static_cast<CreativeGridIndex>(size.height);
  const auto layer = width * height;
  return CreativeGridCoord3{
      static_cast<std::int32_t>(index % width),
      static_cast<std::int32_t>((index / width) % height),
      static_cast<std::int32_t>(index / layer),
  };
}

CreativeGridCoord3 worldToGridCoord(CreativeVec3 position,
                                    double cellSize) noexcept {
  CreativeGridCoord3 coord;
  return tryWorldToGridCoord(position, cellSize, coord) ? coord
                                                       : CreativeGridCoord3{};
}

CreativeGridBounds3 worldBoundsToGridBounds(CreativeBounds bounds,
                                            double cellSize) noexcept {
  CreativeGridBounds3 gridBounds;
  return tryWorldBoundsToGridBounds(bounds, cellSize, gridBounds)
             ? gridBounds
             : CreativeGridBounds3{};
}

CreativeGridBounds3 clampGridBounds(CreativeGridBounds3 bounds,
                                    CreativeGridSize3 size) noexcept {
  return CreativeGridBounds3{
      CreativeGridCoord3{
          std::clamp(bounds.min.x, std::int32_t{0}, size.width),
          std::clamp(bounds.min.y, std::int32_t{0}, size.height),
          std::clamp(bounds.min.z, std::int32_t{0}, size.depth),
      },
      CreativeGridCoord3{
          std::clamp(bounds.max.x, std::int32_t{0}, size.width),
          std::clamp(bounds.max.y, std::int32_t{0}, size.height),
          std::clamp(bounds.max.z, std::int32_t{0}, size.depth),
      },
  };
}

bool isEmptyGridBounds(CreativeGridBounds3 bounds) noexcept {
  return bounds.min.x >= bounds.max.x || bounds.min.y >= bounds.max.y ||
         bounds.min.z >= bounds.max.z;
}

CreativeSpatialProjectionProfile projectionProfileForObject(
    CreativeObjectKind kind) noexcept {
  return describeObject(kind).projectionProfile;
}

CreativeSpatialOccupancyKind occupancyKindForObject(
    CreativeObjectKind kind) noexcept {
  return describeObject(kind).occupancyKind;
}

CreativeSpatialProjectionReceipt projectObjectToGrid(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request) {
  const CreativeSpatialProjectionProfile profile =
      projectionProfileForObject(object.kind);
  const CreativeSpatialProjectionPlan plan =
      [&]() {
        switch (profile) {
          case CreativeSpatialProjectionProfile::PointProjection:
            return makePointPlan(object, request, true);
          case CreativeSpatialProjectionProfile::BoxProjection:
          case CreativeSpatialProjectionProfile::VolumeProjection:
            return makeBoundsPlan(object, request, profile, true);
          case CreativeSpatialProjectionProfile::LineProjection:
            return makeLinePlan(object, request, profile, true);
          case CreativeSpatialProjectionProfile::PathProjection:
            return makePathPlan(object, request, true);
          case CreativeSpatialProjectionProfile::LinkProjection:
            return makeLinkPlan(object, request, true);
          case CreativeSpatialProjectionProfile::Unknown:
          case CreativeSpatialProjectionProfile::NoProjection:
            break;
        }
        CreativeSpatialProjectionPlan noProjection =
            makeProjectionPlanBase(object, request, profile, true);
        if (planIsReady(noProjection)) {
          setPlanStatus(noProjection,
                        CreativeSpatialProjectionStatus::NoProjection,
                        {},
                        "no_projection");
        }
        return noProjection;
      }();
  return materializePlan(plan, object, request);
}

CreativeSpatialProjectionSummary projectObjectToGridSummary(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request) {
  const CreativeSpatialProjectionProfile profile =
      projectionProfileForObject(object.kind);
  switch (profile) {
    case CreativeSpatialProjectionProfile::PointProjection:
      return makePointPlan(object, request, true).summary;
    case CreativeSpatialProjectionProfile::BoxProjection:
    case CreativeSpatialProjectionProfile::VolumeProjection:
      return makeBoundsPlan(object, request, profile, true).summary;
    case CreativeSpatialProjectionProfile::LineProjection:
      return makeLinePlan(object, request, profile, true).summary;
    case CreativeSpatialProjectionProfile::PathProjection:
      return makePathPlan(object, request, true).summary;
    case CreativeSpatialProjectionProfile::LinkProjection:
      return makeLinkPlan(object, request, true).summary;
    case CreativeSpatialProjectionProfile::Unknown:
    case CreativeSpatialProjectionProfile::NoProjection:
      break;
  }
  CreativeSpatialProjectionPlan noProjection =
      makeProjectionPlanBase(object, request, profile, true);
  if (planIsReady(noProjection)) {
    setPlanStatus(noProjection,
                  CreativeSpatialProjectionStatus::NoProjection,
                  {},
                  "no_projection");
  }
  return noProjection.summary;
}

CreativeSpatialProjectionReceipt projectObjectsToGrid(
    std::span<const CreativeObject> objects,
    const CreativeSpatialProjectionRequest& request) {
  if (!isValidRequest(request)) {
    return makeAggregateReceipt(CreativeSpatialProjectionStatus::InvalidGrid,
                                "invalid_grid");
  }

  CreativeSpatialProjectionReceipt aggregate =
      makeAggregateReceipt(CreativeSpatialProjectionStatus::Unknown,
                           "aggregate_empty");

  for (const CreativeObject& object : objects) {
    CreativeSpatialProjectionReceipt receipt = projectObjectToGrid(object,
                                                                   request);
    if (!receipt.cells.empty()) {
      aggregate.cells.reserve(aggregate.cells.size() + receipt.cells.size());
      aggregate.cells.insert(aggregate.cells.end(),
                             receipt.cells.begin(),
                             receipt.cells.end());
    }
    aggregate.status = mergeAggregateStatus(aggregate.status, receipt.status);
  }

  if (!aggregate.cells.empty()) {
    aggregate.status = CreativeSpatialProjectionStatus::Projected;
    aggregate.message = "projected";
  } else if (aggregate.status == CreativeSpatialProjectionStatus::Unknown) {
    aggregate.status = CreativeSpatialProjectionStatus::EmptyProjection;
    aggregate.message = "empty_projection";
  } else {
    aggregate.message = toString(aggregate.status);
  }

  return aggregate;
}

}  // namespace iggy3d::creative
