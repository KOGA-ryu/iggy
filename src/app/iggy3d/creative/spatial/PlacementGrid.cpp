#include "app/iggy3d/creative/spatial/PlacementGrid.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "app/iggy3d/creative/Geometry.hpp"

namespace iggy3d::creative {
namespace {

struct AxisBounds {
  double min = 0.0;
  double max = 0.0;
  bool bounded = false;
  bool valid = true;
};

struct OverlayAxisRange {
  std::int64_t firstLine = 0;
  std::int64_t lastLine = -1;
  double visibleMin = 0.0;
  double visibleMax = 0.0;
  bool clipped = false;
  bool valid = false;
};

[[nodiscard]] bool finitePositive(double value) noexcept {
  return std::isfinite(value) && value > 0.0;
}

[[nodiscard]] CreativeVec3 dominantAxisNormal(CreativeVec3 normal) noexcept {
  const double ax = std::fabs(normal.x);
  const double ay = std::fabs(normal.y);
  const double az = std::fabs(normal.z);
  if (!std::isfinite(ax) || !std::isfinite(ay) || !std::isfinite(az) ||
      std::max({ax, ay, az}) <= 1.0e-12) {
    return {};
  }
  if (ax >= ay && ax >= az) {
    return {std::copysign(1.0, normal.x), 0.0, 0.0};
  }
  if (ay >= az) {
    return {0.0, std::copysign(1.0, normal.y), 0.0};
  }
  return {0.0, 0.0, std::copysign(1.0, normal.z)};
}

[[nodiscard]] CreativeVec3 dominantHorizontalNormal(
    CreativeVec3 direction) noexcept {
  if (!isFiniteCreativeVec3(direction)) {
    return {};
  }
  const double ax = std::fabs(direction.x);
  const double az = std::fabs(direction.z);
  if (std::max(ax, az) <= 1.0e-12) {
    return {};
  }
  return ax >= az ? CreativeVec3{std::copysign(1.0, direction.x), 0.0, 0.0}
                  : CreativeVec3{0.0, 0.0,
                                 std::copysign(1.0, direction.z)};
}

[[nodiscard]] bool tryCellCoordinate(double value,
                                     double origin,
                                     double step,
                                     std::int32_t& output) noexcept {
  if (!std::isfinite(value) || !std::isfinite(origin) ||
      !finitePositive(step)) {
    return false;
  }
  const double coordinate = std::floor((value - origin) / step);
  if (!std::isfinite(coordinate) ||
      coordinate < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      coordinate > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
    return false;
  }
  output = static_cast<std::int32_t>(coordinate);
  return true;
}

[[nodiscard]] bool tryCellFromWorld(CreativeVec3 point,
                                    const CreativePlacementGridFrame& frame,
                                    CreativeGridCoord3& output) noexcept {
  return tryCellCoordinate(point.x, frame.latticeOrigin.x,
                           frame.stepMeters.x, output.x) &&
         tryCellCoordinate(point.y, frame.latticeOrigin.y,
                           frame.stepMeters.y, output.y) &&
         tryCellCoordinate(point.z, frame.latticeOrigin.z,
                           frame.stepMeters.z, output.z);
}

[[nodiscard]] bool tryAdjacentCell(CreativeGridCoord3 cell,
                                   CreativeVec3 faceNormal,
                                   CreativeGridCoord3& adjacent) noexcept {
  const std::int64_t x = static_cast<std::int64_t>(cell.x) +
                         static_cast<std::int64_t>(faceNormal.x);
  const std::int64_t y = static_cast<std::int64_t>(cell.y) +
                         static_cast<std::int64_t>(faceNormal.y);
  const std::int64_t z = static_cast<std::int64_t>(cell.z) +
                         static_cast<std::int64_t>(faceNormal.z);
  constexpr std::int64_t kMin = std::numeric_limits<std::int32_t>::min();
  constexpr std::int64_t kMax = std::numeric_limits<std::int32_t>::max();
  if (x < kMin || x > kMax || y < kMin || y > kMax || z < kMin ||
      z > kMax) {
    return false;
  }
  adjacent = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(y),
              static_cast<std::int32_t>(z)};
  return true;
}

[[nodiscard]] CreativeBounds cellBounds(
    CreativeGridCoord3 cell,
    const CreativePlacementGridFrame& frame) noexcept {
  const CreativeVec3 min{
      frame.latticeOrigin.x + static_cast<double>(cell.x) * frame.stepMeters.x,
      frame.latticeOrigin.y + static_cast<double>(cell.y) * frame.stepMeters.y,
      frame.latticeOrigin.z + static_cast<double>(cell.z) * frame.stepMeters.z};
  return {min,
          {min.x + frame.stepMeters.x, min.y + frame.stepMeters.y,
           min.z + frame.stepMeters.z}};
}

[[nodiscard]] bool cellInsideDocumentBounds(
    CreativeBounds cell,
    const CreativePlacementGridFrame& frame) noexcept {
  const double epsilon = std::max(
      {frame.stepMeters.x, frame.stepMeters.y, frame.stepMeters.z}) * 1.0e-8;
  if ((frame.boundedAxes & kCreativePlacementGridAxisX) != 0U &&
      (cell.min.x < frame.documentBounds.min.x - epsilon ||
       cell.max.x > frame.documentBounds.max.x + epsilon)) {
    return false;
  }
  if ((frame.boundedAxes & kCreativePlacementGridAxisY) != 0U &&
      (cell.min.y < frame.documentBounds.min.y - epsilon ||
       cell.max.y > frame.documentBounds.max.y + epsilon)) {
    return false;
  }
  if ((frame.boundedAxes & kCreativePlacementGridAxisZ) != 0U &&
      (cell.min.z < frame.documentBounds.min.z - epsilon ||
       cell.max.z > frame.documentBounds.max.z + epsilon)) {
    return false;
  }
  return true;
}

[[nodiscard]] AxisBounds combineAxisBounds(double gridMin,
                                           double gridMax,
                                           bool gridBounded,
                                           double worldMin,
                                           double worldMax) noexcept {
  AxisBounds result;
  if (!std::isfinite(gridMin) || !std::isfinite(gridMax) ||
      !std::isfinite(worldMin) || !std::isfinite(worldMax) ||
      worldMax < worldMin) {
    result.valid = false;
    return result;
  }
  const bool worldBounded = worldMax > worldMin;
  if (!gridBounded && !worldBounded) {
    return result;
  }
  result.min = gridBounded ? gridMin : worldMin;
  result.max = gridBounded ? gridMax : worldMax;
  if (gridBounded && worldBounded) {
    result.min = std::max(result.min, worldMin);
    result.max = std::min(result.max, worldMax);
  }
  result.bounded = true;
  result.valid = result.max > result.min;
  return result;
}

[[nodiscard]] OverlayAxisRange overlayAxisRange(
    double latticeOrigin,
    double step,
    double focus,
    double boundMin,
    double boundMax,
    bool bounded,
    std::size_t maximumLines) noexcept {
  OverlayAxisRange range;
  if (!std::isfinite(latticeOrigin) || !finitePositive(step) ||
      !std::isfinite(focus) || !std::isfinite(boundMin) ||
      !std::isfinite(boundMax)) {
    return range;
  }
  const std::int64_t halfVisible =
      static_cast<std::int64_t>(maximumLines / 2U);
  const double epsilon = step * 1.0e-8;
  const std::int64_t center = static_cast<std::int64_t>(
      std::floor((focus - latticeOrigin) / step));
  std::int64_t first = center - halfVisible;
  std::int64_t last = center + halfVisible;
  if (bounded) {
    const std::int64_t boundedFirst = static_cast<std::int64_t>(
        std::ceil((boundMin - latticeOrigin - epsilon) / step));
    const std::int64_t boundedLast = static_cast<std::int64_t>(
        std::floor((boundMax - latticeOrigin + epsilon) / step));
    if (boundedLast < boundedFirst) {
      return range;
    }
    const std::int64_t boundedCount = boundedLast - boundedFirst + 1;
    if (boundedCount <= static_cast<std::int64_t>(maximumLines)) {
      first = boundedFirst;
      last = boundedLast;
    } else {
      first = std::clamp(first, boundedFirst,
                         boundedLast - 2 * halfVisible);
      last = first + 2 * halfVisible;
      range.clipped = true;
    }
    range.visibleMin = std::max(
        boundMin, latticeOrigin + static_cast<double>(first) * step);
    range.visibleMax = std::min(
        boundMax, latticeOrigin + static_cast<double>(last) * step);
  } else {
    range.visibleMin = latticeOrigin + static_cast<double>(first) * step;
    range.visibleMax = latticeOrigin + static_cast<double>(last) * step;
    range.clipped = true;
  }
  range.firstLine = first;
  range.lastLine = last;
  range.valid = range.lastLine >= range.firstLine &&
                range.visibleMax >= range.visibleMin;
  return range;
}

[[nodiscard]] CreativePlacementGridLineRole lineRole(
    std::int64_t index,
    double position,
    double boundMin,
    double boundMax,
    bool bounded,
    std::uint32_t majorEvery,
    double step) noexcept {
  const double epsilon = step * 1.0e-6;
  if (bounded && (std::fabs(position - boundMin) <= epsilon ||
                  std::fabs(position - boundMax) <= epsilon)) {
    return CreativePlacementGridLineRole::Boundary;
  }
  if (majorEvery > 0U &&
      index % static_cast<std::int64_t>(majorEvery) == 0) {
    return CreativePlacementGridLineRole::Major;
  }
  return CreativePlacementGridLineRole::Minor;
}

}  // namespace

CreativePlacementGridFrame makeCreativePlacementGridFrame(
    const CreativePlacementGridFrameRequest& request) noexcept {
  CreativePlacementGridFrame frame;
  const CreativeGridSettings& grid = request.documentGrid;
  if (!isFiniteCreativeVec3(grid.origin) ||
      !finitePositive(grid.cellSizeMeters) || grid.size.width < 0 ||
      grid.size.height < 0 || grid.size.depth < 0 ||
      !isValidCreativeDocumentSnapSettings(request.documentSnap) ||
      !isFiniteCreativeVec3(request.documentWorldBounds.min) ||
      !isFiniteCreativeVec3(request.documentWorldBounds.max) ||
      (request.useStepOverride &&
       !finitePositive(request.stepOverrideMeters)) ||
      (request.useActivePlaneOverride &&
       !std::isfinite(request.activePlaneY))) {
    return frame;
  }

  frame.storageOrigin = grid.origin;
  frame.storageCellSizeMeters = grid.cellSizeMeters;
  frame.storageSize = grid.size;
  frame.storageAligned = request.storageAligned;
  frame.activePlaneY = request.useActivePlaneOverride
                           ? request.activePlaneY
                           : grid.origin.y;
  if (request.storageAligned) {
    frame.latticeOrigin = grid.origin;
    frame.stepMeters = {grid.cellSizeMeters, grid.cellSizeMeters,
                        grid.cellSizeMeters};
  } else {
    frame.latticeOrigin =
        request.documentSnap.mode == CreativeDocumentSnapMode::Grid
            ? CreativeVec3{request.documentSnap.originX,
                           request.documentSnap.originY,
                           request.documentSnap.originZ}
            : grid.origin;
    frame.stepMeters = request.useStepOverride
                           ? CreativeVec3{request.stepOverrideMeters,
                                          request.stepOverrideMeters,
                                          request.stepOverrideMeters}
                           : request.documentSnap.mode ==
                                     CreativeDocumentSnapMode::Grid
                                 ? CreativeVec3{request.documentSnap.stepX,
                                                request.documentSnap.stepY,
                                                request.documentSnap.stepZ}
                                 : CreativeVec3{grid.cellSizeMeters,
                                                grid.cellSizeMeters,
                                                grid.cellSizeMeters};
  }
  if (!isFiniteCreativeVec3(frame.latticeOrigin) ||
      !finitePositive(frame.stepMeters.x) ||
      !finitePositive(frame.stepMeters.y) ||
      !finitePositive(frame.stepMeters.z)) {
    return {};
  }

  const AxisBounds x = combineAxisBounds(
      grid.origin.x,
      grid.origin.x +
          static_cast<double>(grid.size.width) * grid.cellSizeMeters,
      grid.size.width > 0, request.documentWorldBounds.min.x,
      request.documentWorldBounds.max.x);
  const AxisBounds y = combineAxisBounds(
      grid.origin.y,
      grid.origin.y +
          static_cast<double>(grid.size.height) * grid.cellSizeMeters,
      grid.size.height > 0, request.documentWorldBounds.min.y,
      request.documentWorldBounds.max.y);
  const AxisBounds z = combineAxisBounds(
      grid.origin.z,
      grid.origin.z +
          static_cast<double>(grid.size.depth) * grid.cellSizeMeters,
      grid.size.depth > 0, request.documentWorldBounds.min.z,
      request.documentWorldBounds.max.z);
  if (!x.valid || !y.valid || !z.valid) {
    return {};
  }
  if (x.bounded) {
    frame.boundedAxes |= kCreativePlacementGridAxisX;
    frame.documentBounds.min.x = x.min;
    frame.documentBounds.max.x = x.max;
  }
  if (y.bounded) {
    frame.boundedAxes |= kCreativePlacementGridAxisY;
    frame.documentBounds.min.y = y.min;
    frame.documentBounds.max.y = y.max;
  }
  if (z.bounded) {
    frame.boundedAxes |= kCreativePlacementGridAxisZ;
    frame.documentBounds.min.z = z.min;
    frame.documentBounds.max.z = z.max;
  }
  frame.status = CreativePlacementGridStatus::Ready;
  frame.valid = true;
  return frame;
}

CreativeGridTarget resolveCreativeGridTargetFromHit(
    CreativeVec3 hitPoint,
    CreativeVec3 faceNormal,
    const CreativePlacementGridFrame& frame,
    CreativeVec3 placerForward) noexcept {
  CreativeGridTarget target;
  if (!frame.valid || frame.status != CreativePlacementGridStatus::Ready ||
      !isFiniteCreativeVec3(hitPoint) ||
      !isFiniteCreativeVec3(faceNormal) ||
      !isFiniteCreativeVec3(placerForward)) {
    return target;
  }
  const CreativeVec3 snappedNormal = dominantAxisNormal(faceNormal);
  if (snappedNormal.x == 0.0 && snappedNormal.y == 0.0 &&
      snappedNormal.z == 0.0) {
    return target;
  }
  const double epsilon =
      std::max({frame.stepMeters.x, frame.stepMeters.y,
                frame.stepMeters.z}) *
      1.0e-4;
  const CreativeVec3 inside{hitPoint.x - snappedNormal.x * epsilon,
                            hitPoint.y - snappedNormal.y * epsilon,
                            hitPoint.z - snappedNormal.z * epsilon};
  if (!tryCellFromWorld(inside, frame, target.targetCell) ||
      !tryAdjacentCell(target.targetCell, snappedNormal,
                       target.adjacentCell)) {
    return target;
  }

  target.hitPoint = hitPoint;
  target.faceNormal = snappedNormal;
  target.placerForward = dominantHorizontalNormal(placerForward);
  target.targetCellBounds = cellBounds(target.targetCell, frame);
  target.adjacentCellBounds = cellBounds(target.adjacentCell, frame);
  const CreativeVec3 adjacentCenter =
      measureCreativeBounds(target.adjacentCellBounds).center;
  target.placementAnchor = {adjacentCenter.x,
                            target.adjacentCellBounds.min.y,
                            adjacentCenter.z};
  target.resolved = true;
  target.targetInBounds = cellInsideDocumentBounds(target.targetCellBounds,
                                                   frame);
  target.adjacentInBounds = cellInsideDocumentBounds(
      target.adjacentCellBounds, frame);
  target.valid = target.targetInBounds;
  target.status = !target.targetInBounds
                      ? CreativeGridTargetStatus::TargetOutOfBounds
                  : !target.adjacentInBounds
                      ? CreativeGridTargetStatus::AdjacentOutOfBounds
                      : CreativeGridTargetStatus::Ready;
  return target;
}

CreativeGridTarget resolveCreativeGridTargetFromHit(
    CreativeVec3 hitPoint,
    CreativeVec3 faceNormal,
    double cellSize,
    CreativeVec3 origin,
    CreativeVec3 placerForward) noexcept {
  CreativePlacementGridFrameRequest request;
  request.documentGrid.origin = origin;
  request.documentGrid.cellSizeMeters = cellSize;
  request.documentSnap.originX = origin.x;
  request.documentSnap.originY = origin.y;
  request.documentSnap.originZ = origin.z;
  request.documentSnap.stepX = cellSize;
  request.documentSnap.stepY = cellSize;
  request.documentSnap.stepZ = cellSize;
  request.storageAligned = true;
  const CreativePlacementGridFrame frame =
      makeCreativePlacementGridFrame(request);
  return resolveCreativeGridTargetFromHit(hitPoint, faceNormal, frame,
                                          placerForward);
}

CreativePlacementGridOverlayPlan buildCreativePlacementGridOverlayPlan(
    const CreativePlacementGridOverlayRequest& request) noexcept {
  CreativePlacementGridOverlayPlan plan;
  const CreativePlacementGridFrame& frame = request.frame;
  if (!frame.valid || !isFiniteCreativeVec3(request.focus) ||
      !std::isfinite(frame.activePlaneY)) {
    return plan;
  }
  const bool boundedX =
      (frame.boundedAxes & kCreativePlacementGridAxisX) != 0U;
  const bool boundedZ =
      (frame.boundedAxes & kCreativePlacementGridAxisZ) != 0U;
  const OverlayAxisRange x = overlayAxisRange(
      frame.latticeOrigin.x, frame.stepMeters.x, request.focus.x,
      frame.documentBounds.min.x, frame.documentBounds.max.x, boundedX,
      boundedX ? kCreativePlacementGridMaximumLinesPerAxis
               : kCreativePlacementGridUnboundedLinesPerAxis);
  const OverlayAxisRange z = overlayAxisRange(
      frame.latticeOrigin.z, frame.stepMeters.z, request.focus.z,
      frame.documentBounds.min.z, frame.documentBounds.max.z, boundedZ,
      boundedZ ? kCreativePlacementGridMaximumLinesPerAxis
               : kCreativePlacementGridUnboundedLinesPerAxis);
  if (!x.valid || !z.valid) {
    return plan;
  }

  plan.visibleBounds = {{x.visibleMin, frame.activePlaneY, z.visibleMin},
                        {x.visibleMax, frame.activePlaneY, z.visibleMax}};
  plan.clippedX = x.clipped;
  plan.clippedZ = z.clipped;
  for (std::int64_t index = x.firstLine; index <= x.lastLine; ++index) {
    if (plan.lineCount >= plan.lines.size()) {
      return plan;
    }
    const double position =
        frame.latticeOrigin.x + static_cast<double>(index) * frame.stepMeters.x;
    plan.lines[plan.lineCount++] = {
        {position, frame.activePlaneY, z.visibleMin},
        {position, frame.activePlaneY, z.visibleMax},
        lineRole(index, position, frame.documentBounds.min.x,
                 frame.documentBounds.max.x, boundedX, frame.majorEvery,
                 frame.stepMeters.x)};
  }
  for (std::int64_t index = z.firstLine; index <= z.lastLine; ++index) {
    if (plan.lineCount >= plan.lines.size()) {
      return plan;
    }
    const double position =
        frame.latticeOrigin.z + static_cast<double>(index) * frame.stepMeters.z;
    plan.lines[plan.lineCount++] = {
        {x.visibleMin, frame.activePlaneY, position},
        {x.visibleMax, frame.activePlaneY, position},
        lineRole(index, position, frame.documentBounds.min.z,
                 frame.documentBounds.max.z, boundedZ, frame.majorEvery,
                 frame.stepMeters.z)};
  }
  plan.valid = plan.lineCount > 0U;
  return plan;
}

}  // namespace iggy3d::creative
