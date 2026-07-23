#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/spatial/SpatialProjectionInternal.hpp"

#include "app/iggy3d/creative/document/Hierarchy.hpp"

#include <span>
#include <utility>
#include <vector>

namespace iggy3d::creative {

using spatial_projection_internal::appendSampledLineCells;
using spatial_projection_internal::CreativeSpatialProjectionPlan;
using spatial_projection_internal::fillBoundsCells;
using spatial_projection_internal::isValidRequest;
using spatial_projection_internal::makeBoundsPlan;
using spatial_projection_internal::makeLinkPlan;
using spatial_projection_internal::makeLinePlan;
using spatial_projection_internal::makePathPlan;
using spatial_projection_internal::makePointPlan;
using spatial_projection_internal::makeProjectionPlanBase;
using spatial_projection_internal::planIsReady;
using spatial_projection_internal::setPlanStatus;

namespace {

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
    const CreativeObjectHierarchyState hierarchyState =
        resolveCreativeObjectHierarchyState(objects, object.id);
    CreativeSpatialProjectionReceipt receipt;
    if (!hierarchyState.resolved || !hierarchyState.effectivelyVisible) {
      receipt.status = CreativeSpatialProjectionStatus::NoProjection;
      receipt.message = "object_hidden";
    } else {
      receipt = projectObjectToGrid(object, request);
    }
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
