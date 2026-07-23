#include "app/iggy3d/creative/recipes/DoorRecipe.hpp"

#include "app/iggy3d/creative/Geometry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace iggy3d::creative {
namespace {

constexpr double kGeometryEpsilon = 1.0e-9;
constexpr double kPi = 3.14159265358979323846;

[[nodiscard]] bool positiveFinite(double value) noexcept {
  return std::isfinite(value) && value > kGeometryEpsilon;
}

[[nodiscard]] bool nonNegativeFinite(double value) noexcept {
  return std::isfinite(value) && value >= 0.0;
}

[[nodiscard]] bool validBounds(CreativeBounds bounds) noexcept {
  const CreativeBoundsMetrics metrics = measureCreativeBounds(bounds);
  return metrics.valid && isPositiveCreativeVec3(metrics.size);
}

[[nodiscard]] double axisMinimum(const CreativeStructuralWallFrame& frame,
                                 CreativeBounds bounds) noexcept {
  return frame.axis == CreativeStructuralWallAxis::X ? bounds.min.x
                                                     : bounds.min.z;
}

[[nodiscard]] double axisMaximum(const CreativeStructuralWallFrame& frame,
                                 CreativeBounds bounds) noexcept {
  return frame.axis == CreativeStructuralWallAxis::X ? bounds.max.x
                                                     : bounds.max.z;
}

[[nodiscard]] double normalCenter(const CreativeStructuralWallFrame& frame,
                                  CreativeBounds bounds) noexcept {
  return frame.axis == CreativeStructuralWallAxis::X
             ? (bounds.min.z + bounds.max.z) * 0.5
             : (bounds.min.x + bounds.max.x) * 0.5;
}

[[nodiscard]] CreativeBounds boundsForAxes(
    const CreativeStructuralWallFrame& frame, double axisMin, double axisMax,
    double bottom, double top, double normalMin, double normalMax) noexcept {
  if (frame.axis == CreativeStructuralWallAxis::X) {
    return {{axisMin, bottom, normalMin}, {axisMax, top, normalMax}};
  }
  return {{normalMin, bottom, axisMin}, {normalMax, top, axisMax}};
}

[[nodiscard]] CreativeVec3 axisNormalPoint(
    const CreativeStructuralWallFrame& frame, double axis, double y,
    double normal) noexcept {
  return frame.axis == CreativeStructuralWallAxis::X
             ? CreativeVec3{axis, y, normal}
             : CreativeVec3{normal, y, axis};
}

[[nodiscard]] CreativeVec3 rotateAroundY(CreativeVec3 point,
                                         CreativeVec3 pivot,
                                         double radians) noexcept {
  const double dx = point.x - pivot.x;
  const double dz = point.z - pivot.z;
  const double cosine = std::cos(radians);
  const double sine = std::sin(radians);
  return {pivot.x + dx * cosine + dz * sine, point.y,
          pivot.z - dx * sine + dz * cosine};
}

void expand(CreativeBounds& bounds, CreativeVec3 point,
            bool& initialized) noexcept {
  if (!initialized) {
    bounds = {point, point};
    initialized = true;
    return;
  }
  bounds.min.x = std::min(bounds.min.x, point.x);
  bounds.min.y = std::min(bounds.min.y, point.y);
  bounds.min.z = std::min(bounds.min.z, point.z);
  bounds.max.x = std::max(bounds.max.x, point.x);
  bounds.max.y = std::max(bounds.max.y, point.y);
  bounds.max.z = std::max(bounds.max.z, point.z);
}

void expand(CreativeBounds& target, CreativeBounds source,
            bool& initialized) noexcept {
  expand(target, source.min, initialized);
  expand(target, source.max, initialized);
}

[[nodiscard]] bool angleInSweep(double candidate, double angle) noexcept {
  const double minimum = std::min(0.0, angle) - kGeometryEpsilon;
  const double maximum = std::max(0.0, angle) + kGeometryEpsilon;
  return candidate >= minimum && candidate <= maximum;
}

[[nodiscard]] CreativeBounds sweptBounds(CreativeBounds closedBounds,
                                         CreativeVec3 pivot,
                                         double angle) noexcept {
  CreativeBounds result{};
  bool initialized = false;
  const std::array<CreativeVec3, 4U> corners{{
      {closedBounds.min.x, closedBounds.min.y, closedBounds.min.z},
      {closedBounds.min.x, closedBounds.min.y, closedBounds.max.z},
      {closedBounds.max.x, closedBounds.max.y, closedBounds.min.z},
      {closedBounds.max.x, closedBounds.max.y, closedBounds.max.z},
  }};
  for (const CreativeVec3 corner : corners) {
    expand(result, rotateAroundY(corner, pivot, 0.0), initialized);
    expand(result, rotateAroundY(corner, pivot, angle), initialized);
    const double dx = corner.x - pivot.x;
    const double dz = corner.z - pivot.z;
    const std::array<double, 2U> bases{
        std::atan2(dz, dx), std::atan2(-dx, dz)};
    for (const double base : bases) {
      for (int turn = -2; turn <= 2; ++turn) {
        const double candidate = base + static_cast<double>(turn) * kPi;
        if (angleInSweep(candidate, angle)) {
          expand(result, rotateAroundY(corner, pivot, candidate), initialized);
        }
      }
    }
  }
  return result;
}

[[nodiscard]] double openAngle(CreativeVec3 away,
                               CreativeVec3 desiredNormal) noexcept {
  return std::atan2(away.z * desiredNormal.x - away.x * desiredNormal.z,
                    away.x * desiredNormal.x + away.z * desiredNormal.z);
}

void reject(CreativeDoorRecipeResult& result,
            CreativeDoorRecipeStatus status,
            std::string_view reasonCode) noexcept {
  result = {};
  result.status = status;
  result.reasonCode = reasonCode;
}

[[nodiscard]] bool appendPart(CreativeDoorRecipeResult& result,
                              CreativeDoorPartKind kind,
                              CreativeObjectKind objectKind,
                              CreativeBounds bounds,
                              std::uint8_t leafIndex,
                              CreativeVec3 pivot) noexcept {
  if (result.partCount >= result.parts.size() || !validBounds(bounds)) {
    return false;
  }
  CreativeDoorPartPlan& part = result.parts[result.partCount++];
  part.kind = kind;
  part.objectKind = objectKind;
  part.closedBounds = bounds;
  part.closedTransform.position =
      leafIndex == kCreativeDoorNoLeaf ? measureCreativeBounds(bounds).center
                                       : pivot;
  part.leafIndex = leafIndex;
  return true;
}

[[nodiscard]] bool appendLeafAssembly(
    CreativeDoorRecipeResult& result,
    const CreativeStructuralWallFrame& frame, double leafMinimum,
    double leafMaximum, double bottom, double top, double normal,
    double leafThickness, bool hingeAtMinimum, std::uint8_t leafIndex,
    CreativeDoorPartKind leafKind, CreativeDoorPartKind lowerHingeKind,
    CreativeDoorPartKind upperHingeKind,
    CreativeDoorPartKind handleKind,
    CreativeDoorSwingSide swingSide) noexcept {
  if (result.leafCount >= result.leaves.size()) {
    return false;
  }
  const double hingeAxis = hingeAtMinimum ? leafMinimum : leafMaximum;
  const double freeAxis = hingeAtMinimum ? leafMaximum : leafMinimum;
  const CreativeVec3 pivot =
      axisNormalPoint(frame, hingeAxis, bottom, normal);
  CreativeDoorLeafPlan& leaf = result.leaves[result.leafCount++];
  leaf.firstPartIndex = result.partCount;
  leaf.hingePivotMeters = pivot;
  const CreativeVec3 away = frame.axis == CreativeStructuralWallAxis::X
                                ? CreativeVec3{freeAxis - hingeAxis, 0.0, 0.0}
                                : CreativeVec3{0.0, 0.0,
                                               freeAxis - hingeAxis};
  const double normalSign =
      swingSide == CreativeDoorSwingSide::PositiveNormal ? 1.0 : -1.0;
  leaf.openAngleRadians = openAngle(
      away, {frame.normal.x * normalSign, 0.0, frame.normal.z * normalSign});

  const double halfThickness = leafThickness * 0.5;
  const CreativeBounds leafBounds =
      boundsForAxes(frame, leafMinimum, leafMaximum, bottom, top,
                    normal - halfThickness, normal + halfThickness);
  if (!appendPart(result, leafKind,
                  leafIndex == 0U ? CreativeObjectKind::Door
                                  : CreativeObjectKind::Prop,
                  leafBounds, leafIndex, pivot)) {
    return false;
  }

  const double leafWidth = leafMaximum - leafMinimum;
  const double leafHeight = top - bottom;
  const double hingeWidth = std::min(0.04, leafWidth * 0.08);
  const double hingeHeight = std::min(0.10, leafHeight * 0.08);
  const double hingeDepth = leafThickness + std::min(0.03, leafThickness);
  const double hingeMinimum = hingeAxis - hingeWidth * 0.5;
  const double hingeMaximum = hingeAxis + hingeWidth * 0.5;
  const double hingeNormalMinimum = normal - hingeDepth * 0.5;
  const double hingeNormalMaximum = normal + hingeDepth * 0.5;
  const std::array<double, 2U> hingeCenters{
      bottom + leafHeight * 0.20, bottom + leafHeight * 0.80};
  if (!appendPart(
          result, lowerHingeKind, CreativeObjectKind::Prop,
          boundsForAxes(frame, hingeMinimum, hingeMaximum,
                        hingeCenters[0] - hingeHeight * 0.5,
                        hingeCenters[0] + hingeHeight * 0.5,
                        hingeNormalMinimum, hingeNormalMaximum),
          leafIndex, pivot) ||
      !appendPart(
          result, upperHingeKind, CreativeObjectKind::Prop,
          boundsForAxes(frame, hingeMinimum, hingeMaximum,
                        hingeCenters[1] - hingeHeight * 0.5,
                        hingeCenters[1] + hingeHeight * 0.5,
                        hingeNormalMinimum, hingeNormalMaximum),
          leafIndex, pivot)) {
    return false;
  }

  const double handleWidth = std::min(0.06, leafWidth * 0.12);
  const double handleHeight = std::min(0.12, leafHeight * 0.08);
  const double handleInset = std::min(0.12, leafWidth * 0.20);
  const double handleAxis = hingeAtMinimum ? leafMaximum - handleInset
                                           : leafMinimum + handleInset;
  const double handleDepth = leafThickness + std::min(0.08, leafThickness);
  if (!appendPart(
          result, handleKind, CreativeObjectKind::Prop,
          boundsForAxes(frame, handleAxis - handleWidth * 0.5,
                        handleAxis + handleWidth * 0.5,
                        bottom + leafHeight * 0.45 - handleHeight * 0.5,
                        bottom + leafHeight * 0.45 + handleHeight * 0.5,
                        normal - handleDepth * 0.5,
                        normal + handleDepth * 0.5),
          leafIndex, pivot)) {
    return false;
  }

  bool closedInitialized = false;
  bool sweepInitialized = false;
  for (std::size_t index = leaf.firstPartIndex; index < result.partCount;
       ++index) {
    const CreativeBounds partBounds = result.parts[index].closedBounds;
    expand(leaf.closedAssemblyBounds, partBounds, closedInitialized);
    expand(leaf.sweepBounds,
           sweptBounds(partBounds, pivot, leaf.openAngleRadians),
           sweepInitialized);
  }
  leaf.partCount = result.partCount - leaf.firstPartIndex;
  return closedInitialized && sweepInitialized;
}

}  // namespace

CreativeDoorRecipeResult planCreativeDoor(
    const CreativeDoorRecipeRequest& request) noexcept {
  CreativeDoorRecipeResult result;
  if (request.wallFrame.axis >= CreativeStructuralWallAxis::Count ||
      !validBounds(request.wallFrame.bounds) ||
      !positiveFinite(request.wallFrame.heightMeters) ||
      !positiveFinite(request.wallFrame.thicknessMeters) ||
      !isFiniteCreativeVec3(request.wallFrame.normal)) {
    reject(result, CreativeDoorRecipeStatus::InvalidWallFrame,
           "creative_door_wall_frame_invalid");
    return result;
  }
  const CreativeBoundsMetrics cutout =
      measureCreativeBounds(request.cutoutBounds);
  if (!cutout.valid || !isPositiveCreativeVec3(cutout.size) ||
      request.cutoutBounds.min.x < request.wallFrame.bounds.min.x -
                                      kGeometryEpsilon ||
      request.cutoutBounds.min.y < request.wallFrame.bounds.min.y -
                                      kGeometryEpsilon ||
      request.cutoutBounds.min.z < request.wallFrame.bounds.min.z -
                                      kGeometryEpsilon ||
      request.cutoutBounds.max.x > request.wallFrame.bounds.max.x +
                                      kGeometryEpsilon ||
      request.cutoutBounds.max.y > request.wallFrame.bounds.max.y +
                                      kGeometryEpsilon ||
      request.cutoutBounds.max.z > request.wallFrame.bounds.max.z +
                                      kGeometryEpsilon) {
    reject(result, CreativeDoorRecipeStatus::InvalidCutout,
           "creative_door_cutout_invalid");
    return result;
  }
  if (!isValidCreativeDoorSettings(request.settings)) {
    reject(result, CreativeDoorRecipeStatus::InvalidSettings,
           "creative_door_settings_invalid");
    return result;
  }
  if (!positiveFinite(request.frameWidthMeters) ||
      !nonNegativeFinite(request.floorGapMeters) ||
      !positiveFinite(request.leafGapMeters) ||
      !nonNegativeFinite(request.leafThicknessMeters)) {
    reject(result, CreativeDoorRecipeStatus::InvalidDimensions,
           "creative_door_dimensions_invalid");
    return result;
  }

  const double minimum = axisMinimum(request.wallFrame, request.cutoutBounds);
  const double maximum = axisMaximum(request.wallFrame, request.cutoutBounds);
  const double width = maximum - minimum;
  const double height = cutout.size.y;
  const double leafThickness =
      request.leafThicknessMeters > 0.0
          ? request.leafThicknessMeters
          : std::min(0.05, request.wallFrame.thicknessMeters * 0.60);
  const double innerMinimum = minimum + request.frameWidthMeters;
  const double innerMaximum = maximum - request.frameWidthMeters;
  const double bottom = request.cutoutBounds.min.y + request.floorGapMeters;
  const double top = request.cutoutBounds.max.y - request.frameWidthMeters -
                     request.leafGapMeters * 0.5;
  const double availableWidth = innerMaximum - innerMinimum;
  const double requiredGaps =
      request.settings.leafArrangement == CreativeDoorLeafArrangement::Double
          ? request.leafGapMeters * 2.0
          : request.leafGapMeters;
  if (!positiveFinite(width) || !positiveFinite(height) ||
      !positiveFinite(leafThickness) ||
      leafThickness > request.wallFrame.thicknessMeters + kGeometryEpsilon ||
      availableWidth <= requiredGaps + kGeometryEpsilon ||
      top <= bottom + kGeometryEpsilon ||
      request.frameWidthMeters * 2.0 >= width - kGeometryEpsilon ||
      request.frameWidthMeters >= height - kGeometryEpsilon) {
    reject(result, CreativeDoorRecipeStatus::InvalidDimensions,
           "creative_door_dimensions_unrepresentable");
    return result;
  }

  const double normal = normalCenter(request.wallFrame, request.cutoutBounds);
  const double normalMinimum =
      normal - request.wallFrame.thicknessMeters * 0.5;
  const double normalMaximum =
      normal + request.wallFrame.thicknessMeters * 0.5;
  if (!appendPart(
          result, CreativeDoorPartKind::MinimumJamb, CreativeObjectKind::Beam,
          boundsForAxes(request.wallFrame, minimum, innerMinimum,
                        request.cutoutBounds.min.y, request.cutoutBounds.max.y,
                        normalMinimum, normalMaximum),
          kCreativeDoorNoLeaf, {}) ||
      !appendPart(
          result, CreativeDoorPartKind::MaximumJamb, CreativeObjectKind::Beam,
          boundsForAxes(request.wallFrame, innerMaximum, maximum,
                        request.cutoutBounds.min.y, request.cutoutBounds.max.y,
                        normalMinimum, normalMaximum),
          kCreativeDoorNoLeaf, {}) ||
      !appendPart(
          result, CreativeDoorPartKind::Header, CreativeObjectKind::Beam,
          boundsForAxes(request.wallFrame, innerMinimum, innerMaximum,
                        request.cutoutBounds.max.y - request.frameWidthMeters,
                        request.cutoutBounds.max.y, normalMinimum,
                        normalMaximum),
          kCreativeDoorNoLeaf, {})) {
    reject(result, CreativeDoorRecipeStatus::CapacityExceeded,
           "creative_door_part_capacity_exceeded");
    return result;
  }

  const double edgeGap = request.leafGapMeters * 0.5;
  if (request.settings.leafArrangement == CreativeDoorLeafArrangement::Single) {
    const bool hingeAtMinimum =
        request.settings.hingeSide == CreativeDoorHingeSide::MinimumEdge;
    if (!appendLeafAssembly(
            result, request.wallFrame, innerMinimum + edgeGap,
            innerMaximum - edgeGap, bottom, top, normal, leafThickness,
            hingeAtMinimum, 0U, CreativeDoorPartKind::PrimaryLeaf,
            CreativeDoorPartKind::PrimaryLowerHinge,
            CreativeDoorPartKind::PrimaryUpperHinge,
            CreativeDoorPartKind::PrimaryHandle, request.settings.swingSide)) {
      reject(result, CreativeDoorRecipeStatus::CapacityExceeded,
             "creative_door_part_capacity_exceeded");
      return result;
    }
  } else {
    const double center = (innerMinimum + innerMaximum) * 0.5;
    const double primaryMinimum = innerMinimum + edgeGap;
    const double primaryMaximum = center - request.leafGapMeters * 0.5;
    const double secondaryMinimum = center + request.leafGapMeters * 0.5;
    const double secondaryMaximum = innerMaximum - edgeGap;
    const bool primaryAtMinimum =
        request.settings.hingeSide == CreativeDoorHingeSide::MinimumEdge;
    const double firstMinimum = primaryAtMinimum ? primaryMinimum
                                                 : secondaryMinimum;
    const double firstMaximum = primaryAtMinimum ? primaryMaximum
                                                 : secondaryMaximum;
    const double secondMinimum = primaryAtMinimum ? secondaryMinimum
                                                  : primaryMinimum;
    const double secondMaximum = primaryAtMinimum ? secondaryMaximum
                                                  : primaryMaximum;
    if (!appendLeafAssembly(
            result, request.wallFrame, firstMinimum, firstMaximum, bottom, top,
            normal, leafThickness, primaryAtMinimum, 0U,
            CreativeDoorPartKind::PrimaryLeaf,
            CreativeDoorPartKind::PrimaryLowerHinge,
            CreativeDoorPartKind::PrimaryUpperHinge,
            CreativeDoorPartKind::PrimaryHandle, request.settings.swingSide) ||
        !appendLeafAssembly(
            result, request.wallFrame, secondMinimum, secondMaximum, bottom,
            top, normal, leafThickness, !primaryAtMinimum, 1U,
            CreativeDoorPartKind::SecondaryLeaf,
            CreativeDoorPartKind::SecondaryLowerHinge,
            CreativeDoorPartKind::SecondaryUpperHinge,
            CreativeDoorPartKind::SecondaryHandle,
            request.settings.swingSide)) {
      reject(result, CreativeDoorRecipeStatus::CapacityExceeded,
             "creative_door_part_capacity_exceeded");
      return result;
    }
  }

  bool fullSweepInitialized = false;
  for (std::size_t index = 0U; index < result.leafCount; ++index) {
    expand(result.fullSweepBounds, result.leaves[index].sweepBounds,
           fullSweepInitialized);
  }
  if (!fullSweepInitialized) {
    reject(result, CreativeDoorRecipeStatus::InvalidDimensions,
           "creative_door_sweep_unrepresentable");
    return result;
  }
  result.accepted = true;
  result.status = CreativeDoorRecipeStatus::Ready;
  result.reasonCode = "creative_door_ready";
  return result;
}

}  // namespace iggy3d::creative
