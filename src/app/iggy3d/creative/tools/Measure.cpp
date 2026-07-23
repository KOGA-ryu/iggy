#include "app/iggy3d/creative/tools/Measure.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace iggy3d::creative {
namespace {

constexpr double kMeasurementEpsilon = 1.0e-9;

[[nodiscard]] bool finitePoint(CreativeMeasurementPoint point) noexcept {
  return std::isfinite(point.x) && std::isfinite(point.y) &&
         std::isfinite(point.z) &&
         point.snapKind < CreativeMeasurementSnapKind::Count;
}

[[nodiscard]] double segmentLength(CreativeMeasurementPoint from,
                                   CreativeMeasurementPoint to) noexcept {
  return std::hypot(to.x - from.x, to.y - from.y, to.z - from.z);
}

[[nodiscard]] bool modeUsesTwoPoints(CreativeMeasurementMode mode) noexcept {
  return mode == CreativeMeasurementMode::Distance ||
         mode == CreativeMeasurementMode::AxisProjected ||
         mode == CreativeMeasurementMode::Vertical ||
         mode == CreativeMeasurementMode::Slope;
}

[[nodiscard]] double selectedAxisDistance(
    CreativeMeasurementAxis axis,
    double deltaX,
    double deltaY,
    double deltaZ) noexcept {
  switch (axis) {
    case CreativeMeasurementAxis::X:
      return std::abs(deltaX);
    case CreativeMeasurementAxis::Y:
      return std::abs(deltaY);
    case CreativeMeasurementAxis::Z:
      return std::abs(deltaZ);
    case CreativeMeasurementAxis::Count:
      break;
  }
  return 0.0;
}

[[nodiscard]] bool planArea(CreativeMeasurementPlan& result,
                            std::span<const CreativeMeasurementPoint> points)
    noexcept {
  CreativeVec3 normal;
  double minimumX = points.front().x;
  double maximumX = points.front().x;
  double minimumY = points.front().y;
  double maximumY = points.front().y;
  double minimumZ = points.front().z;
  double maximumZ = points.front().z;
  for (std::size_t index = 0U; index < points.size(); ++index) {
    const CreativeMeasurementPoint& current = points[index];
    const CreativeMeasurementPoint& next = points[(index + 1U) % points.size()];
    normal.x += (current.y - next.y) * (current.z + next.z);
    normal.y += (current.z - next.z) * (current.x + next.x);
    normal.z += (current.x - next.x) * (current.y + next.y);
    minimumX = std::min(minimumX, current.x);
    maximumX = std::max(maximumX, current.x);
    minimumY = std::min(minimumY, current.y);
    maximumY = std::max(maximumY, current.y);
    minimumZ = std::min(minimumZ, current.z);
    maximumZ = std::max(maximumZ, current.z);
  }
  const double doubledArea = std::hypot(normal.x, normal.y, normal.z);
  if (!std::isfinite(doubledArea) || doubledArea <= kMeasurementEpsilon) {
    return false;
  }
  result.areaSquareMeters = doubledArea * 0.5;
  result.areaNormal = {normal.x / doubledArea, normal.y / doubledArea,
                       normal.z / doubledArea};

  const double extent = std::max(
      {maximumX - minimumX, maximumY - minimumY, maximumZ - minimumZ, 1.0});
  const double planarTolerance = extent * 1.0e-6;
  const CreativeMeasurementPoint& origin = points.front();
  for (const CreativeMeasurementPoint& point : points) {
    const double planeDistance = std::abs(
        (point.x - origin.x) * result.areaNormal.x +
        (point.y - origin.y) * result.areaNormal.y +
        (point.z - origin.z) * result.areaNormal.z);
    if (!std::isfinite(planeDistance) || planeDistance > planarTolerance) {
      result.status = CreativeMeasurementPlanStatus::NonPlanar;
      result.reasonCode = "creative_measurement_area_non_planar";
      return false;
    }
  }
  return true;
}

[[nodiscard]] CreativeMeasurementPoint pointFromPointer(
    const CreativeToolPointerPacket& pointer) noexcept {
  CreativeMeasurementPoint point;
  point.x = pointer.hasWorldDestination ? pointer.worldDestination.x : pointer.x;
  point.y = pointer.hasWorldDestination ? pointer.worldDestination.y : pointer.y;
  point.z = pointer.hasWorldDestination ? pointer.worldDestination.z : 0.0;
  point.target = pointer.target;
  return point;
}

[[nodiscard]] bool samePoint(CreativeMeasurementPoint lhs,
                             CreativeMeasurementPoint rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y &&
         lhs.z == rhs.z && lhs.snapKind == rhs.snapKind &&
         lhs.target.value == rhs.target.value;
}

[[nodiscard]] CreativeMeasurementReceipt makeReceipt(
    const CreativeMeasurementState& state,
    CreativeMeasurementChangeKind requestedChange) noexcept {
  CreativeMeasurementReceipt receipt;
  receipt.requestedChange = requestedChange;
  receipt.activeBefore = state.active;
  receipt.activeAfter = state.active;
  receipt.hasMeasurementBefore = state.hasMeasurement;
  receipt.hasMeasurementAfter = state.hasMeasurement;
  receipt.startPointBefore = state.startPoint;
  receipt.startPointAfter = state.startPoint;
  receipt.currentPointBefore = state.currentPoint;
  receipt.currentPointAfter = state.currentPoint;
  receipt.sampleCountBefore = state.sampleCount;
  receipt.sampleCountAfter = state.sampleCount;
  receipt.pointCountBefore = state.pointCount;
  receipt.pointCountAfter = state.pointCount;
  receipt.planStatusBefore = state.plan.status;
  receipt.planStatusAfter = state.plan.status;
  return receipt;
}

void refreshAfter(CreativeMeasurementReceipt& receipt,
                  const CreativeMeasurementState& state) noexcept {
  receipt.activeAfter = state.active;
  receipt.hasMeasurementAfter = state.hasMeasurement;
  receipt.startPointAfter = state.startPoint;
  receipt.currentPointAfter = state.currentPoint;
  receipt.sampleCountAfter = state.sampleCount;
  receipt.pointCountAfter = state.pointCount;
  receipt.planStatusAfter = state.plan.status;
}

void resetMeasurementData(CreativeMeasurementState& state) noexcept {
  const CreativeMeasurementMode mode = state.mode;
  const CreativeMeasurementAxis axis = state.axis;
  const bool closePath = state.closePath;
  state = {};
  state.mode = mode;
  state.axis = axis;
  state.closePath = closePath;
}

[[nodiscard]] CreativeMeasurementPlan planCommittedMeasurement(
    const CreativeMeasurementState& state) noexcept {
  return planCreativeMeasurement(
      {state.mode, state.axis,
       std::span<const CreativeMeasurementPoint>{state.points.data(),
                                                 state.pointCount},
       state.closePath});
}

void refreshMeasurementPlan(CreativeMeasurementState& state) noexcept {
  const std::size_t count =
      state.pointCount + (state.hasPreviewPoint ? 1U : 0U);
  state.plan = planCreativeMeasurement(
      {state.mode, state.axis,
       std::span<const CreativeMeasurementPoint>{state.points.data(), count},
       state.closePath});
}

}  // namespace

CreativeMeasurementState makeDefaultCreativeMeasurementState() noexcept {
  return {};
}

CreativeMeasurementPlan planCreativeMeasurement(
    const CreativeMeasurementPlanRequest& request) noexcept {
  CreativeMeasurementPlan result;
  result.requested = true;
  result.mode = request.mode;
  result.axis = request.axis;
  result.pointCount = request.points.size();
  if (request.mode >= CreativeMeasurementMode::Count) {
    result.status = CreativeMeasurementPlanStatus::InvalidMode;
    result.reasonCode = "creative_measurement_mode_invalid";
    return result;
  }
  if (request.axis >= CreativeMeasurementAxis::Count) {
    result.status = CreativeMeasurementPlanStatus::InvalidAxis;
    result.reasonCode = "creative_measurement_axis_invalid";
    return result;
  }
  const std::size_t minimumPointCount =
      request.mode == CreativeMeasurementMode::Area ? 3U : 2U;
  if (request.points.size() < minimumPointCount) {
    result.status = CreativeMeasurementPlanStatus::TooFewPoints;
    result.reasonCode = "creative_measurement_too_few_points";
    return result;
  }
  if (request.points.size() > kCreativeMeasurementPointCapacity) {
    result.status = CreativeMeasurementPlanStatus::CapacityExceeded;
    result.reasonCode = "creative_measurement_capacity_exceeded";
    return result;
  }
  if (!std::all_of(request.points.begin(), request.points.end(), finitePoint)) {
    result.status = CreativeMeasurementPlanStatus::InvalidPoint;
    result.reasonCode = "creative_measurement_point_invalid";
    return result;
  }
  if (modeUsesTwoPoints(request.mode) && request.points.size() != 2U) {
    result.status = CreativeMeasurementPlanStatus::InvalidPoint;
    result.reasonCode = "creative_measurement_two_point_count_invalid";
    return result;
  }

  result.firstPoint = request.points.front();
  result.lastPoint = request.points.back();
  result.deltaX = result.lastPoint.x - result.firstPoint.x;
  result.deltaY = result.lastPoint.y - result.firstPoint.y;
  result.deltaZ = result.lastPoint.z - result.firstPoint.z;
  result.directDistanceMeters =
      segmentLength(result.firstPoint, result.lastPoint);
  result.horizontalDistanceMeters = std::hypot(result.deltaX, result.deltaZ);
  result.verticalDistanceMeters = std::abs(result.deltaY);
  result.signedRiseMeters = result.deltaY;
  result.axisDistanceMeters = selectedAxisDistance(
      request.axis, result.deltaX, result.deltaY, result.deltaZ);

  for (std::size_t index = 1U; index < request.points.size(); ++index) {
    result.perimeterMeters +=
        segmentLength(request.points[index - 1U], request.points[index]);
    ++result.segmentCount;
  }
  const bool closePath = request.mode == CreativeMeasurementMode::Area ||
                         (request.mode == CreativeMeasurementMode::Perimeter &&
                          request.closePath);
  if (closePath) {
    result.perimeterMeters +=
        segmentLength(request.points.back(), request.points.front());
    ++result.segmentCount;
  }

  if (result.directDistanceMeters <= kMeasurementEpsilon &&
      modeUsesTwoPoints(request.mode)) {
    result.status = CreativeMeasurementPlanStatus::Degenerate;
    result.reasonCode = "creative_measurement_degenerate";
    return result;
  }
  if (request.mode == CreativeMeasurementMode::Perimeter &&
      result.perimeterMeters <= kMeasurementEpsilon) {
    result.status = CreativeMeasurementPlanStatus::Degenerate;
    result.reasonCode = "creative_measurement_perimeter_degenerate";
    return result;
  }
  if (request.mode == CreativeMeasurementMode::Slope) {
    result.slopeDegrees =
        std::atan2(result.signedRiseMeters, result.horizontalDistanceMeters) *
        (180.0 / std::numbers::pi);
    result.gradeFinite = result.horizontalDistanceMeters > kMeasurementEpsilon;
    result.gradePercent = result.gradeFinite
                              ? (result.signedRiseMeters /
                                 result.horizontalDistanceMeters) *
                                    100.0
                              : 0.0;
  }
  if (request.mode == CreativeMeasurementMode::Area &&
      !planArea(result, request.points)) {
    if (result.status != CreativeMeasurementPlanStatus::NonPlanar) {
      result.status = CreativeMeasurementPlanStatus::Degenerate;
      result.reasonCode = "creative_measurement_area_degenerate";
    }
    return result;
  }

  result.accepted = true;
  result.status = CreativeMeasurementPlanStatus::Ready;
  result.reasonCode = "creative_measurement_ready";
  return result;
}

CreativeMeasurementReadout buildCreativeMeasurementReadout(
    const CreativeMeasurementState& state) noexcept {
  CreativeMeasurementReadout result;
  result.visible = state.hasMeasurement;
  result.valid = state.plan.accepted;
  result.completed = state.completed;
  result.mode = state.mode;
  result.axis = state.axis;
  result.snapKind = state.hasMeasurement ? state.currentPoint.snapKind
                                         : CreativeMeasurementSnapKind::None;
  result.pointCount = state.plan.pointCount;
  result.segmentCount = state.plan.segmentCount;
  switch (state.mode) {
    case CreativeMeasurementMode::Distance:
      result.primaryValue = state.plan.directDistanceMeters;
      result.primaryLabel = "Distance";
      break;
    case CreativeMeasurementMode::AxisProjected:
      result.primaryValue = state.plan.axisDistanceMeters;
      result.primaryLabel = "Axis distance";
      break;
    case CreativeMeasurementMode::Vertical:
      result.primaryValue = state.plan.signedRiseMeters;
      result.primaryLabel = "Rise";
      break;
    case CreativeMeasurementMode::Slope:
      result.primaryValue = state.plan.slopeDegrees;
      result.primaryLabel = "Slope";
      result.primaryUnit = "deg";
      result.secondaryValue = state.plan.gradePercent;
      result.hasSecondaryValue = state.plan.gradeFinite;
      result.secondaryLabel = "Grade";
      result.secondaryUnit = "%";
      break;
    case CreativeMeasurementMode::Perimeter:
      result.primaryValue = state.plan.perimeterMeters;
      result.primaryLabel = "Perimeter";
      break;
    case CreativeMeasurementMode::Area:
      result.primaryValue = state.plan.areaSquareMeters;
      result.primaryLabel = "Area";
      result.primaryUnit = "m2";
      result.secondaryValue = state.plan.perimeterMeters;
      result.hasSecondaryValue = true;
      result.secondaryLabel = "Perimeter";
      result.secondaryUnit = "m";
      break;
    case CreativeMeasurementMode::Count:
      result.visible = false;
      result.valid = false;
      break;
  }
  return result;
}

CreativeMeasurementGeometry buildCreativeMeasurementGeometry(
    const CreativeMeasurementState& state) noexcept {
  CreativeMeasurementGeometry result;
  if (!state.hasMeasurement ||
      state.pointCount > kCreativeMeasurementPointCapacity) {
    return result;
  }

  const std::size_t visiblePointCount =
      state.pointCount + (state.hasPreviewPoint ? 1U : 0U);
  if (visiblePointCount == 0U ||
      visiblePointCount > kCreativeMeasurementPointCapacity) {
    return result;
  }
  for (std::size_t index = 0U; index < visiblePointCount; ++index) {
    if (!finitePoint(state.points[index])) {
      return {};
    }
    result.points[index] = state.points[index];
  }

  result.visible = true;
  result.completed = state.completed;
  result.pointCount = visiblePointCount;
  const auto appendSegment = [&](CreativeMeasurementPoint start,
                                 CreativeMeasurementPoint end) {
    if (result.segmentCount >= result.segments.size() ||
        segmentLength(start, end) <= kMeasurementEpsilon) {
      return;
    }
    result.segments[result.segmentCount++] = {start, end};
  };
  for (std::size_t index = 1U; index < visiblePointCount; ++index) {
    appendSegment(result.points[index - 1U], result.points[index]);
  }

  result.closed = visiblePointCount >= 3U &&
                  (state.mode == CreativeMeasurementMode::Area ||
                   (state.mode == CreativeMeasurementMode::Perimeter &&
                    state.closePath));
  if (result.closed) {
    appendSegment(result.points[visiblePointCount - 1U], result.points[0U]);
  }
  return result;
}

std::string_view toString(CreativeMeasurementSnapKind snapKind) noexcept {
  switch (snapKind) {
    case CreativeMeasurementSnapKind::None: return "Free";
    case CreativeMeasurementSnapKind::Grid: return "Grid";
    case CreativeMeasurementSnapKind::Surface: return "Surface";
    case CreativeMeasurementSnapKind::Vertex: return "Vertex";
    case CreativeMeasurementSnapKind::Opening: return "Opening";
    case CreativeMeasurementSnapKind::Level: return "Level";
    case CreativeMeasurementSnapKind::Count: break;
  }
  return "Invalid";
}

std::string_view toString(CreativeMeasurementPlanStatus status) noexcept {
  switch (status) {
    case CreativeMeasurementPlanStatus::NotRequested: return "NotRequested";
    case CreativeMeasurementPlanStatus::InvalidMode: return "InvalidMode";
    case CreativeMeasurementPlanStatus::InvalidAxis: return "InvalidAxis";
    case CreativeMeasurementPlanStatus::TooFewPoints: return "TooFewPoints";
    case CreativeMeasurementPlanStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeMeasurementPlanStatus::InvalidPoint: return "InvalidPoint";
    case CreativeMeasurementPlanStatus::Degenerate: return "Degenerate";
    case CreativeMeasurementPlanStatus::NonPlanar: return "NonPlanar";
    case CreativeMeasurementPlanStatus::Ready: return "Ready";
  }
  return "Invalid";
}

CreativeMeasurementReceipt configureMeasurement(
    CreativeMeasurementState& state,
    CreativeMeasurementMode mode,
    CreativeMeasurementAxis axis,
    bool closePath) noexcept {
  CreativeMeasurementReceipt receipt = makeReceipt(
      state, CreativeMeasurementChangeKind::ConfigureMeasurement);
  if (mode >= CreativeMeasurementMode::Count) {
    receipt.message = "measurement_mode_invalid";
    return receipt;
  }
  if (axis >= CreativeMeasurementAxis::Count) {
    receipt.message = "measurement_axis_invalid";
    return receipt;
  }
  receipt.accepted = true;
  if (state.mode == mode && state.axis == axis &&
      state.closePath == closePath) {
    receipt.message = "measurement_configuration_unchanged";
    return receipt;
  }
  resetMeasurementData(state);
  state.mode = mode;
  state.axis = axis;
  state.closePath = closePath;
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange =
      CreativeMeasurementChangeKind::ConfigureMeasurement;
  receipt.message = "measurement_configured";
  return receipt;
}

CreativeMeasurementReceipt appendMeasurementPoint(
    CreativeMeasurementState& state,
    CreativeMeasurementPoint point) noexcept {
  CreativeMeasurementReceipt receipt = makeReceipt(
      state, CreativeMeasurementChangeKind::AppendMeasurementPoint);
  if (!finitePoint(point)) {
    receipt.message = "measurement_point_invalid";
    return receipt;
  }
  if (!state.active) {
    resetMeasurementData(state);
    state.active = true;
    state.hasMeasurement = true;
    state.startPoint = point;
    state.currentPoint = point;
    state.points[0U] = point;
    state.pointCount = 1U;
    state.sampleCount = 1U;
    refreshMeasurementPlan(state);
    refreshAfter(receipt, state);
    receipt.accepted = true;
    receipt.changed = true;
    receipt.appliedChange =
        CreativeMeasurementChangeKind::AppendMeasurementPoint;
    receipt.message = "measurement_first_point_added";
    return receipt;
  }

  if (state.pointCount > 0U &&
      samePoint(state.points[state.pointCount - 1U], point)) {
    const CreativeMeasurementPlan committed = planCommittedMeasurement(state);
    if (committed.accepted && !modeUsesTwoPoints(state.mode)) {
      state.plan = committed;
      state.active = false;
      state.completed = true;
      state.hasPreviewPoint = false;
      state.currentPoint = state.points[state.pointCount - 1U];
      refreshAfter(receipt, state);
      receipt.accepted = true;
      receipt.changed = true;
      receipt.appliedChange =
          CreativeMeasurementChangeKind::CompleteMeasurement;
      receipt.message = "measurement_completed_by_repeated_point";
      return receipt;
    }
    receipt.accepted = true;
    receipt.message = "measurement_point_unchanged";
    return receipt;
  }
  if (state.pointCount >= kCreativeMeasurementPointCapacity) {
    receipt.message = "measurement_capacity_exceeded";
    return receipt;
  }

  state.points[state.pointCount] = point;
  const CreativeMeasurementPlan candidate = planCreativeMeasurement(
      {state.mode, state.axis,
       std::span<const CreativeMeasurementPoint>{state.points.data(),
                                                 state.pointCount + 1U},
       state.closePath});
  if (modeUsesTwoPoints(state.mode) && !candidate.accepted) {
    receipt.message = "measurement_endpoint_rejected";
    return receipt;
  }
  ++state.pointCount;
  state.currentPoint = point;
  state.hasPreviewPoint = false;
  ++state.sampleCount;
  state.plan = candidate;
  if (modeUsesTwoPoints(state.mode)) {
    state.active = false;
    state.completed = true;
  }
  refreshAfter(receipt, state);
  receipt.accepted = true;
  receipt.changed = true;
  receipt.appliedChange =
      CreativeMeasurementChangeKind::AppendMeasurementPoint;
  receipt.message = state.completed ? "measurement_completed"
                                    : "measurement_point_added";
  return receipt;
}

CreativeMeasurementReceipt previewMeasurementPoint(
    CreativeMeasurementState& state,
    CreativeMeasurementPoint point) noexcept {
  CreativeMeasurementReceipt receipt = makeReceipt(
      state, CreativeMeasurementChangeKind::UpdateMeasurement);
  if (!state.active) {
    receipt.message = "measurement_not_active";
    return receipt;
  }
  if (!finitePoint(point)) {
    receipt.message = "measurement_point_invalid";
    return receipt;
  }
  if (state.pointCount >= kCreativeMeasurementPointCapacity) {
    receipt.message = "measurement_capacity_exceeded";
    return receipt;
  }
  if (state.hasPreviewPoint && samePoint(state.currentPoint, point)) {
    receipt.accepted = true;
    receipt.message = "measurement_update_unchanged";
    return receipt;
  }
  state.points[state.pointCount] = point;
  state.currentPoint = point;
  state.hasPreviewPoint = true;
  ++state.sampleCount;
  refreshMeasurementPlan(state);
  refreshAfter(receipt, state);
  receipt.accepted = true;
  receipt.changed = true;
  receipt.appliedChange = CreativeMeasurementChangeKind::UpdateMeasurement;
  receipt.message = "measurement_updated";
  return receipt;
}

CreativeMeasurementReceipt completeMeasurement(
    CreativeMeasurementState& state) noexcept {
  CreativeMeasurementReceipt receipt = makeReceipt(
      state, CreativeMeasurementChangeKind::CompleteMeasurement);
  if (!state.active) {
    receipt.message = "measurement_not_active";
    return receipt;
  }
  const CreativeMeasurementPlan committed = planCommittedMeasurement(state);
  if (!committed.accepted) {
    receipt.message = committed.reasonCode;
    return receipt;
  }
  state.plan = committed;
  state.active = false;
  state.completed = true;
  state.hasPreviewPoint = false;
  state.currentPoint = state.points[state.pointCount - 1U];
  refreshAfter(receipt, state);
  receipt.accepted = true;
  receipt.changed = true;
  receipt.appliedChange = CreativeMeasurementChangeKind::CompleteMeasurement;
  receipt.message = "measurement_completed";
  return receipt;
}

CreativeMeasurementReceipt clearMeasurement(
    CreativeMeasurementState& state) noexcept {
  CreativeMeasurementReceipt receipt =
      makeReceipt(state, CreativeMeasurementChangeKind::ClearMeasurement);
  receipt.accepted = true;

  if (!state.active && !state.hasMeasurement) {
    receipt.message = "measurement_already_empty";
    return receipt;
  }

  resetMeasurementData(state);
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeMeasurementChangeKind::ClearMeasurement;
  receipt.message = "measurement_cleared";
  return receipt;
}

CreativeMeasurementReceipt beginMeasurement(
    CreativeMeasurementState& state,
    const CreativeToolPointerPacket& pointer) noexcept {
  CreativeMeasurementReceipt receipt =
      makeReceipt(state, CreativeMeasurementChangeKind::BeginMeasurement);
  const CreativeMeasurementPoint point = pointFromPointer(pointer);
  if (!finitePoint(point)) {
    receipt.message = "measurement_point_invalid";
    return receipt;
  }
  resetMeasurementData(state);
  state.active = true;
  state.hasMeasurement = true;
  state.startPoint = point;
  state.currentPoint = point;
  state.points[0U] = point;
  state.pointCount = 1U;
  state.sampleCount = 1U;
  refreshMeasurementPlan(state);
  refreshAfter(receipt, state);
  receipt.accepted = true;
  receipt.changed = true;
  receipt.appliedChange = CreativeMeasurementChangeKind::BeginMeasurement;
  receipt.message = "measurement_began";
  return receipt;
}

CreativeMeasurementReceipt updateMeasurement(
    CreativeMeasurementState& state,
    const CreativeToolPointerPacket& pointer) noexcept {
  CreativeMeasurementReceipt receipt =
      makeReceipt(state, CreativeMeasurementChangeKind::UpdateMeasurement);

  if (!state.active) {
    receipt.message = "measurement_not_active";
    return receipt;
  }

  const CreativeMeasurementPoint point = pointFromPointer(pointer);
  receipt = previewMeasurementPoint(state, point);
  receipt.requestedChange = CreativeMeasurementChangeKind::UpdateMeasurement;
  return receipt;
}

CreativeMeasurementReceipt endMeasurement(
    CreativeMeasurementState& state,
    const CreativeToolPointerPacket& pointer) noexcept {
  CreativeMeasurementReceipt receipt =
      makeReceipt(state, CreativeMeasurementChangeKind::EndMeasurement);

  if (!state.active) {
    receipt.message = "measurement_not_active";
    return receipt;
  }

  const CreativeMeasurementPoint point = pointFromPointer(pointer);
  receipt = appendMeasurementPoint(state, point);
  receipt.requestedChange = CreativeMeasurementChangeKind::EndMeasurement;
  if (receipt.accepted && !state.completed) {
    receipt = completeMeasurement(state);
    receipt.requestedChange = CreativeMeasurementChangeKind::EndMeasurement;
  }
  if (receipt.accepted) {
    receipt.appliedChange = CreativeMeasurementChangeKind::EndMeasurement;
    receipt.message = "measurement_ended";
  }
  return receipt;
}

CreativeMeasurementReceipt cancelMeasurement(
    CreativeMeasurementState& state) noexcept {
  CreativeMeasurementReceipt receipt =
      makeReceipt(state, CreativeMeasurementChangeKind::CancelMeasurement);

  if (!state.active) {
    receipt.message = "measurement_not_active";
    return receipt;
  }

  receipt.accepted = true;
  resetMeasurementData(state);
  refreshAfter(receipt, state);
  receipt.changed = true;
  receipt.appliedChange = CreativeMeasurementChangeKind::CancelMeasurement;
  receipt.message = "measurement_cancelled";
  return receipt;
}

CreativeMeasurementReceipt applyMeasurementToolIntent(
    CreativeMeasurementState& state,
    const CreativeToolIntent& intent) noexcept {
  switch (intent.kind) {
    case CreativeToolIntentKind::BeginMeasurement:
      return beginMeasurement(state, intent.pointer);
    case CreativeToolIntentKind::UpdateMeasurement:
      return updateMeasurement(state, intent.pointer);
    case CreativeToolIntentKind::EndMeasurement:
      return endMeasurement(state, intent.pointer);
    case CreativeToolIntentKind::AppendMeasurementPoint:
      return appendMeasurementPoint(state, pointFromPointer(intent.pointer));
    case CreativeToolIntentKind::CompleteMeasurement:
      return completeMeasurement(state);
    case CreativeToolIntentKind::CancelToolAction:
      return cancelMeasurement(state);
    case CreativeToolIntentKind::NoIntent:
    case CreativeToolIntentKind::SelectObjectCandidate:
    case CreativeToolIntentKind::PreviewPointer:
    case CreativeToolIntentKind::BeginMove:
    case CreativeToolIntentKind::PreviewMove:
    case CreativeToolIntentKind::CommitMove:
    case CreativeToolIntentKind::CancelMove:
      break;
  }

  CreativeMeasurementReceipt receipt =
      makeReceipt(state, CreativeMeasurementChangeKind::None);
  receipt.message = "non_measurement_intent";
  return receipt;
}

}  // namespace iggy3d::creative
