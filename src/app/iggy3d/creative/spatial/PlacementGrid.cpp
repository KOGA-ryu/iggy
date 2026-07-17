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

[[nodiscard]] constexpr std::uint8_t placementAnchorCandidateCount(
    CreativePlacementAnchorKind kind) noexcept {
  switch (kind) {
    case CreativePlacementAnchorKind::BaseCenter:
      return 1U;
    case CreativePlacementAnchorKind::FaceCenter:
      return 6U;
    case CreativePlacementAnchorKind::EdgeMidpoint:
      return 12U;
    case CreativePlacementAnchorKind::Corner:
      return 8U;
    case CreativePlacementAnchorKind::Count:
      return 0U;
  }
  return 0U;
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

[[nodiscard]] bool validDepthAxisLock(
    CreativePlacementGridAxisMask axis) noexcept {
  return axis == 0U || axis == kCreativePlacementGridAxisX ||
         axis == kCreativePlacementGridAxisY ||
         axis == kCreativePlacementGridAxisZ;
}

[[nodiscard]] CreativePlacementGridAxisMask axisMask(
    CreativeVec3 axis) noexcept {
  const CreativeVec3 canonical = dominantAxisNormal(axis);
  if (canonical.x != 0.0) {
    return kCreativePlacementGridAxisX;
  }
  if (canonical.y != 0.0) {
    return kCreativePlacementGridAxisY;
  }
  return canonical.z != 0.0 ? kCreativePlacementGridAxisZ : 0U;
}

[[nodiscard]] double axisComponent(
    CreativeVec3 value,
    CreativePlacementGridAxisMask axis) noexcept {
  if (axis == kCreativePlacementGridAxisX) {
    return value.x;
  }
  if (axis == kCreativePlacementGridAxisY) {
    return value.y;
  }
  return axis == kCreativePlacementGridAxisZ ? value.z : 0.0;
}

[[nodiscard]] CreativeVec3 signedAxis(
    CreativePlacementGridAxisMask axis,
    CreativeVec3 direction,
    CreativeVec3 previous) noexcept {
  double component = axisComponent(direction, axis);
  if (std::fabs(component) <= 1.0e-12 && axisMask(previous) == axis) {
    component = axisComponent(previous, axis);
  }
  const double sign = component < 0.0 ? -1.0 : 1.0;
  if (axis == kCreativePlacementGridAxisX) {
    return {sign, 0.0, 0.0};
  }
  if (axis == kCreativePlacementGridAxisY) {
    return {0.0, sign, 0.0};
  }
  return axis == kCreativePlacementGridAxisZ
             ? CreativeVec3{0.0, 0.0, sign}
             : CreativeVec3{};
}

[[nodiscard]] CreativeVec3 resolveDepthAxis(
    CreativeVec3 direction,
    const CreativePlacementGridFrame& frame) noexcept {
  if (frame.depthAxisLock != 0U) {
    return signedAxis(frame.depthAxisLock, direction,
                      frame.previousDepthAxis);
  }
  const CreativeVec3 candidate = dominantAxisNormal(direction);
  const CreativePlacementGridAxisMask candidateMask = axisMask(candidate);
  const CreativePlacementGridAxisMask previousMask =
      axisMask(frame.previousDepthAxis);
  if (candidateMask == 0U || previousMask == 0U) {
    return candidate;
  }

  constexpr double kAutoAxisRetainRatio = 0.85;
  const double candidateMagnitude =
      std::max({std::fabs(direction.x), std::fabs(direction.y),
                std::fabs(direction.z)});
  const double previousMagnitude =
      std::fabs(axisComponent(direction, previousMask));
  return previousMagnitude >= candidateMagnitude * kAutoAxisRetainRatio
             ? signedAxis(previousMask, direction, frame.previousDepthAxis)
             : candidate;
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

[[nodiscard]] bool tryOffsetCell(CreativeGridCoord3 cell,
                                 CreativeVec3 direction,
                                 std::uint8_t steps,
                                 CreativeGridCoord3& output) noexcept {
  const std::int64_t x = static_cast<std::int64_t>(cell.x) +
                         static_cast<std::int64_t>(direction.x) * steps;
  const std::int64_t y = static_cast<std::int64_t>(cell.y) +
                         static_cast<std::int64_t>(direction.y) * steps;
  const std::int64_t z = static_cast<std::int64_t>(cell.z) +
                         static_cast<std::int64_t>(direction.z) * steps;
  constexpr std::int64_t kMin = std::numeric_limits<std::int32_t>::min();
  constexpr std::int64_t kMax = std::numeric_limits<std::int32_t>::max();
  if (x < kMin || x > kMax || y < kMin || y > kMax || z < kMin ||
      z > kMax) {
    return false;
  }
  output = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(y),
            static_cast<std::int32_t>(z)};
  return true;
}

[[nodiscard]] bool tryAdjacentCell(CreativeGridCoord3 cell,
                                   CreativeVec3 faceNormal,
                                   CreativeGridCoord3& adjacent) noexcept {
  return tryOffsetCell(cell, faceNormal, 1U, adjacent);
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

[[nodiscard]] CreativeVec3 cellPlacementAnchor(
    CreativeGridCoord3 cell,
    const CreativePlacementGridFrame& frame) noexcept {
  const CreativeBounds bounds = cellBounds(cell, frame);
  const CreativeVec3 center = measureCreativeBounds(bounds).center;
  return {center.x, bounds.min.y, center.z};
}

[[nodiscard]] CreativePlacementAnchorCandidatePlan placementAnchorCandidates(
    CreativePlacementAnchorKind kind,
    CreativeBounds bounds) noexcept {
  constexpr double kInverseSqrtTwo = 0.70710678118654752440;
  constexpr double kInverseSqrtThree = 0.57735026918962576451;
  CreativePlacementAnchorCandidatePlan result;
  const CreativeBoundsMetrics metrics = measureCreativeBounds(bounds);
  result.count = placementAnchorCandidateCount(kind);
  if (!metrics.valid || !isPositiveCreativeVec3(metrics.size) ||
      result.count == 0U) {
    return result;
  }

  switch (kind) {
    case CreativePlacementAnchorKind::BaseCenter:
      result.positions[0] = {metrics.center.x, bounds.min.y,
                             metrics.center.z};
      break;
    case CreativePlacementAnchorKind::FaceCenter:
      result.positions[0] = {bounds.min.x, metrics.center.y,
                             metrics.center.z};
      result.outwardNormals[0] = {-1.0, 0.0, 0.0};
      result.positions[1] = {bounds.max.x, metrics.center.y,
                             metrics.center.z};
      result.outwardNormals[1] = {1.0, 0.0, 0.0};
      result.positions[2] = {metrics.center.x, bounds.min.y,
                             metrics.center.z};
      result.outwardNormals[2] = {0.0, -1.0, 0.0};
      result.positions[3] = {metrics.center.x, bounds.max.y,
                             metrics.center.z};
      result.outwardNormals[3] = {0.0, 1.0, 0.0};
      result.positions[4] = {metrics.center.x, metrics.center.y,
                             bounds.min.z};
      result.outwardNormals[4] = {0.0, 0.0, -1.0};
      result.positions[5] = {metrics.center.x, metrics.center.y,
                             bounds.max.z};
      result.outwardNormals[5] = {0.0, 0.0, 1.0};
      break;
    case CreativePlacementAnchorKind::EdgeMidpoint: {
      std::size_t index = 0U;
      for (std::size_t y = 0U; y < 2U; ++y) {
        for (std::size_t z = 0U; z < 2U; ++z) {
          result.positions[index++] = {
              metrics.center.x, y == 0U ? bounds.min.y : bounds.max.y,
              z == 0U ? bounds.min.z : bounds.max.z};
          result.outwardNormals[index - 1U] = {
              0.0, y == 0U ? -kInverseSqrtTwo : kInverseSqrtTwo,
              z == 0U ? -kInverseSqrtTwo : kInverseSqrtTwo};
        }
      }
      for (std::size_t x = 0U; x < 2U; ++x) {
        for (std::size_t z = 0U; z < 2U; ++z) {
          result.positions[index++] = {
              x == 0U ? bounds.min.x : bounds.max.x, metrics.center.y,
              z == 0U ? bounds.min.z : bounds.max.z};
          result.outwardNormals[index - 1U] = {
              x == 0U ? -kInverseSqrtTwo : kInverseSqrtTwo, 0.0,
              z == 0U ? -kInverseSqrtTwo : kInverseSqrtTwo};
        }
      }
      for (std::size_t x = 0U; x < 2U; ++x) {
        for (std::size_t y = 0U; y < 2U; ++y) {
          result.positions[index++] = {
              x == 0U ? bounds.min.x : bounds.max.x,
              y == 0U ? bounds.min.y : bounds.max.y, metrics.center.z};
          result.outwardNormals[index - 1U] = {
              x == 0U ? -kInverseSqrtTwo : kInverseSqrtTwo,
              y == 0U ? -kInverseSqrtTwo : kInverseSqrtTwo, 0.0};
        }
      }
      break;
    }
    case CreativePlacementAnchorKind::Corner: {
      std::size_t index = 0U;
      for (std::size_t x = 0U; x < 2U; ++x) {
        for (std::size_t y = 0U; y < 2U; ++y) {
          for (std::size_t z = 0U; z < 2U; ++z) {
            result.positions[index++] = {
                x == 0U ? bounds.min.x : bounds.max.x,
                y == 0U ? bounds.min.y : bounds.max.y,
                z == 0U ? bounds.min.z : bounds.max.z};
            result.outwardNormals[index - 1U] = {
                x == 0U ? -kInverseSqrtThree : kInverseSqrtThree,
                y == 0U ? -kInverseSqrtThree : kInverseSqrtThree,
                z == 0U ? -kInverseSqrtThree : kInverseSqrtThree};
          }
        }
      }
      break;
    }
    case CreativePlacementAnchorKind::Count:
      return {};
  }
  result.valid =
      std::all_of(result.positions.begin(),
                  result.positions.begin() + result.count,
                  isFiniteCreativeVec3) &&
      std::all_of(result.outwardNormals.begin(),
                  result.outwardNormals.begin() + result.count,
                  isFiniteCreativeVec3);
  return result;
}

[[nodiscard]] double squaredDistance(CreativeVec3 a,
                                     CreativeVec3 b) noexcept {
  const double x = a.x - b.x;
  const double y = a.y - b.y;
  const double z = a.z - b.z;
  return x * x + y * y + z * z;
}

[[nodiscard]] CreativePlacementAnchorSelection selectPlacementAnchor(
    const CreativePlacementAnchorCandidatePlan& candidates,
    CreativeVec3 hitPoint,
    CreativeGridCoord3 cell,
    const CreativePlacementGridFrame& frame) noexcept {
  return selectCreativePlacementAnchor(
      candidates,
      {hitPoint,
       std::min({frame.stepMeters.x, frame.stepMeters.y,
                 frame.stepMeters.z}) *
           kCreativePlacementAnchorRetainStepFraction,
       frame.previousAnchorIndex,
       frame.hasPreviousAnchor && frame.previousAnchorCell == cell});
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

[[nodiscard]] bool tryOffsetCoordinate(std::int32_t value,
                                       std::int32_t offset,
                                       std::int32_t& output) noexcept {
  const std::int64_t candidate = static_cast<std::int64_t>(value) + offset;
  if (candidate < std::numeric_limits<std::int32_t>::min() ||
      candidate > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  output = static_cast<std::int32_t>(candidate);
  return true;
}

[[nodiscard]] bool cellTouchesAxisBoundary(
    const CreativeBounds& bounds,
    const CreativePlacementGridFrame& frame,
    CreativePlacementGridAxisMask axis) noexcept {
  if ((frame.boundedAxes & axis) == 0U) {
    return false;
  }
  double cellMin = 0.0;
  double cellMax = 0.0;
  double documentMin = 0.0;
  double documentMax = 0.0;
  double step = 1.0;
  if (axis == kCreativePlacementGridAxisX) {
    cellMin = bounds.min.x;
    cellMax = bounds.max.x;
    documentMin = frame.documentBounds.min.x;
    documentMax = frame.documentBounds.max.x;
    step = frame.stepMeters.x;
  } else if (axis == kCreativePlacementGridAxisY) {
    cellMin = bounds.min.y;
    cellMax = bounds.max.y;
    documentMin = frame.documentBounds.min.y;
    documentMax = frame.documentBounds.max.y;
    step = frame.stepMeters.y;
  } else {
    cellMin = bounds.min.z;
    cellMax = bounds.max.z;
    documentMin = frame.documentBounds.min.z;
    documentMax = frame.documentBounds.max.z;
    step = frame.stepMeters.z;
  }
  const double epsilon = step * 1.0e-6;
  return std::fabs(cellMin - documentMin) <= epsilon ||
         std::fabs(cellMax - documentMax) <= epsilon;
}

[[nodiscard]] CreativePlacementGridLineRole dotRole(
    CreativeGridCoord3 cell,
    const CreativeBounds& bounds,
    const CreativePlacementGridFrame& frame,
    CreativePlacementGridAxisMask firstAxis,
    CreativePlacementGridAxisMask secondAxis) noexcept {
  if (cellTouchesAxisBoundary(bounds, frame, firstAxis) ||
      cellTouchesAxisBoundary(bounds, frame, secondAxis)) {
    return CreativePlacementGridLineRole::Boundary;
  }
  const auto major = [majorEvery = frame.majorEvery](std::int32_t value) {
    return majorEvery > 0U &&
           value % static_cast<std::int32_t>(majorEvery) == 0;
  };
  const std::int32_t first = firstAxis == kCreativePlacementGridAxisX
                                 ? cell.x
                             : firstAxis == kCreativePlacementGridAxisY
                                 ? cell.y
                                 : cell.z;
  const std::int32_t second = secondAxis == kCreativePlacementGridAxisX
                                  ? cell.x
                              : secondAxis == kCreativePlacementGridAxisY
                                  ? cell.y
                                  : cell.z;
  return major(first) || major(second)
             ? CreativePlacementGridLineRole::Major
             : CreativePlacementGridLineRole::Minor;
}

}  // namespace

CreativePlacementAnchorCandidatePlan
buildCreativePlacementAnchorCandidatePlan(
    CreativePlacementAnchorKind kind,
    CreativeBounds bounds) noexcept {
  return placementAnchorCandidates(kind, bounds);
}

CreativePlacementAnchorSelection selectCreativePlacementAnchor(
    const CreativePlacementAnchorCandidatePlan& candidates,
    const CreativePlacementAnchorSelectionRequest& request) noexcept {
  CreativePlacementAnchorSelection result;
  if (!candidates.valid || candidates.count == 0U ||
      candidates.count > candidates.positions.size() ||
      !isFiniteCreativeVec3(request.hitPoint) ||
      !std::isfinite(request.retainDistanceMeters) ||
      request.retainDistanceMeters < 0.0 ||
      (request.hasPrevious && request.previousIndex >= candidates.count)) {
    return result;
  }
  for (std::uint8_t index = 0U; index < candidates.count; ++index) {
    if (!isFiniteCreativeVec3(candidates.positions[index]) ||
        !isFiniteCreativeVec3(candidates.outwardNormals[index])) {
      return result;
    }
  }

  std::uint8_t bestIndex = 0U;
  double bestDistanceSquared =
      squaredDistance(request.hitPoint, candidates.positions[0]);
  for (std::uint8_t index = 1U; index < candidates.count; ++index) {
    const double distanceSquared =
        squaredDistance(request.hitPoint, candidates.positions[index]);
    if (distanceSquared < bestDistanceSquared) {
      bestDistanceSquared = distanceSquared;
      bestIndex = index;
    }
  }

  if (request.hasPrevious) {
    const double previousDistance = std::sqrt(squaredDistance(
        request.hitPoint, candidates.positions[request.previousIndex]));
    const double bestDistance = std::sqrt(bestDistanceSquared);
    if (previousDistance <=
        bestDistance + request.retainDistanceMeters) {
      bestIndex = request.previousIndex;
    }
  }

  result.position = candidates.positions[bestIndex];
  result.outwardNormal = candidates.outwardNormals[bestIndex];
  result.index = bestIndex;
  result.valid = true;
  return result;
}

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
       !std::isfinite(request.activePlaneY)) ||
      !validDepthAxisLock(request.depthAxisLock) ||
      !isFiniteCreativeVec3(request.previousDepthAxis) ||
      placementAnchorCandidateCount(request.anchorKind) == 0U ||
      (request.hasPreviousAnchor &&
       request.previousAnchorIndex >=
           placementAnchorCandidateCount(request.anchorKind))) {
    return frame;
  }

  frame.storageOrigin = grid.origin;
  frame.storageCellSizeMeters = grid.cellSizeMeters;
  frame.storageSize = grid.size;
  frame.storageAligned = request.storageAligned;
  frame.depthOffsetSteps = request.depthOffsetSteps;
  frame.depthAxisLock = request.depthAxisLock;
  frame.previousDepthAxis =
      dominantAxisNormal(request.previousDepthAxis);
  frame.anchorKind = request.anchorKind;
  frame.previousAnchorCell = request.previousAnchorCell;
  frame.previousAnchorIndex = request.previousAnchorIndex;
  frame.hasPreviousAnchor = request.hasPreviousAnchor;
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
  const CreativeVec3 viewDepthAxis = resolveDepthAxis(placerForward, frame);
  if (!tryCellFromWorld(inside, frame, target.targetCell) ||
      !tryAdjacentCell(target.targetCell, snappedNormal,
                       target.surfaceAdjacentCell) ||
      (frame.depthOffsetSteps > 0U && viewDepthAxis.x == 0.0 &&
       viewDepthAxis.y == 0.0 && viewDepthAxis.z == 0.0) ||
      !tryOffsetCell(target.surfaceAdjacentCell, viewDepthAxis,
                     frame.depthOffsetSteps, target.adjacentCell)) {
    return target;
  }

  target.hitPoint = hitPoint;
  target.faceNormal = snappedNormal;
  target.placerForward = dominantHorizontalNormal(placerForward);
  target.viewDepthAxis = viewDepthAxis;
  target.targetCellBounds = cellBounds(target.targetCell, frame);
  target.surfaceAdjacentCellBounds =
      cellBounds(target.surfaceAdjacentCell, frame);
  target.adjacentCellBounds = cellBounds(target.adjacentCell, frame);
  target.surfacePlacementAnchor =
      cellPlacementAnchor(target.surfaceAdjacentCell, frame);
  target.basePlacementAnchor =
      cellPlacementAnchor(target.adjacentCell, frame);
  target.anchorCandidates = placementAnchorCandidates(
      frame.anchorKind, target.adjacentCellBounds);
  const CreativePlacementAnchorSelection anchor = selectPlacementAnchor(
      target.anchorCandidates, hitPoint, target.adjacentCell, frame);
  if (!anchor.valid) {
    return {};
  }
  target.placementAnchor = anchor.position;
  target.placementNormal = snappedNormal;
  target.anchorKind = frame.anchorKind;
  target.anchorIndex = anchor.index;
  target.anchorSnapped =
      frame.anchorKind != CreativePlacementAnchorKind::BaseCenter;
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

CreativeVec3 creativeGridTargetSurfaceNormal(
    const CreativeGridTarget& target) noexcept {
  if (target.anchorSnapped && isFiniteCreativeVec3(target.placementNormal)) {
    const double lengthSquared =
        target.placementNormal.x * target.placementNormal.x +
        target.placementNormal.y * target.placementNormal.y +
        target.placementNormal.z * target.placementNormal.z;
    if (std::isfinite(lengthSquared) && lengthSquared > 1.0e-24) {
      return target.placementNormal;
    }
  }
  return target.faceNormal;
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

CreativePlacementGridDotLayerPlan buildCreativePlacementGridDotLayerPlan(
    const CreativePlacementGridDotLayerRequest& request) noexcept {
  CreativePlacementGridDotLayerPlan plan;
  const CreativePlacementGridFrame& frame = request.frame;
  const CreativeGridTarget& target = request.target;
  if (!frame.valid || frame.status != CreativePlacementGridStatus::Ready ||
      !target.valid || !target.resolved || !target.adjacentInBounds ||
      target.status != CreativeGridTargetStatus::Ready) {
    return plan;
  }

  const CreativeVec3 fixedAxis = dominantAxisNormal(target.viewDepthAxis);
  CreativePlacementGridAxisMask firstAxis = kCreativePlacementGridAxisX;
  CreativePlacementGridAxisMask secondAxis = kCreativePlacementGridAxisY;
  if (fixedAxis.x != 0.0) {
    firstAxis = kCreativePlacementGridAxisY;
    secondAxis = kCreativePlacementGridAxisZ;
  } else if (fixedAxis.y != 0.0) {
    firstAxis = kCreativePlacementGridAxisX;
    secondAxis = kCreativePlacementGridAxisZ;
  } else if (fixedAxis.z == 0.0) {
    return plan;
  }

  constexpr std::int32_t kHalf =
      static_cast<std::int32_t>(kCreativePlacementGridDotsPerAxis / 2U);
  for (std::int32_t firstOffset = -kHalf; firstOffset <= kHalf;
       ++firstOffset) {
    for (std::int32_t secondOffset = -kHalf; secondOffset <= kHalf;
         ++secondOffset) {
      CreativeGridCoord3 cell = target.adjacentCell;
      std::int32_t* firstCoordinate =
          firstAxis == kCreativePlacementGridAxisX
              ? &cell.x
          : firstAxis == kCreativePlacementGridAxisY ? &cell.y
                                                     : &cell.z;
      std::int32_t* secondCoordinate =
          secondAxis == kCreativePlacementGridAxisX
              ? &cell.x
          : secondAxis == kCreativePlacementGridAxisY ? &cell.y
                                                       : &cell.z;
      if (!tryOffsetCoordinate(*firstCoordinate, firstOffset,
                               *firstCoordinate) ||
          !tryOffsetCoordinate(*secondCoordinate, secondOffset,
                               *secondCoordinate)) {
        continue;
      }
      const CreativeBounds bounds = cellBounds(cell, frame);
      if (!cellInsideDocumentBounds(bounds, frame)) {
        continue;
      }
      if (plan.dotCount >= plan.dots.size()) {
        return plan;
      }
      plan.dots[plan.dotCount++] = {
          cellPlacementAnchor(cell, frame),
          dotRole(cell, bounds, frame, firstAxis, secondAxis)};
    }
  }
  plan.valid = plan.dotCount > 0U;
  return plan;
}

}  // namespace iggy3d::creative
