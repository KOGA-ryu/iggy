#include "app/iggy3d/creative/spatial/SpatialProjectionInternal.hpp"

#include "app/iggy3d/creative/Geometry.hpp"

#include <algorithm>
#include <span>

namespace iggy3d::creative::spatial_projection_internal {

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

[[nodiscard]] bool pathPointsAreValid(
    std::span<const CreativePathPoint> pathPoints) noexcept {
  if (pathPoints.size() < 2U) {
    return false;
  }

  return std::all_of(pathPoints.begin(),
                     pathPoints.end(),
                     [](const CreativePathPoint& point) {
                       return isValidCreativePathPoint(point);
                     });
}

[[nodiscard]] bool lineEndpointsAreValid(
    std::span<const CreativePathPoint> pathPoints) noexcept {
  return pathPoints.size() == 2U &&
         std::all_of(pathPoints.begin(),
                     pathPoints.end(),
                     [](const CreativePathPoint& point) {
                       return isValidCreativePathPoint(point);
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
            found = found || coord == candidate;
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

}  // namespace iggy3d::creative::spatial_projection_internal
