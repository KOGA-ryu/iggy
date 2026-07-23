#include "app/iggy3d/creative/recipes/WindowRecipe.hpp"

#include "app/iggy3d/creative/Geometry.hpp"

#include <algorithm>
#include <cmath>

namespace iggy3d::creative {
namespace {

constexpr double kGeometryEpsilon = 1.0e-9;

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

void reject(CreativeWindowRecipeResult& result,
            CreativeWindowRecipeStatus status,
            std::string_view reasonCode) noexcept {
  result = {};
  result.status = status;
  result.reasonCode = reasonCode;
}

[[nodiscard]] bool appendPart(CreativeWindowRecipeResult& result,
                              CreativeWindowPartKind kind,
                              CreativeObjectKind objectKind,
                              CreativeBounds bounds) noexcept {
  if (result.partCount >= result.parts.size() || !validBounds(bounds)) {
    return false;
  }
  result.parts[result.partCount++] = {kind, objectKind, bounds};
  return true;
}

}  // namespace

CreativeWindowRecipeResult planCreativeWindow(
    const CreativeWindowRecipeRequest& request) noexcept {
  CreativeWindowRecipeResult result;
  if (request.wallFrame.axis >= CreativeStructuralWallAxis::Count ||
      !validBounds(request.wallFrame.bounds) ||
      !positiveFinite(request.wallFrame.heightMeters) ||
      !positiveFinite(request.wallFrame.thicknessMeters) ||
      !isFiniteCreativeVec3(request.wallFrame.normal)) {
    reject(result, CreativeWindowRecipeStatus::InvalidWallFrame,
           "creative_window_wall_frame_invalid");
    return result;
  }

  const CreativeBoundsMetrics cutout =
      measureCreativeBounds(request.cutoutBounds);
  if (!cutout.valid || !isPositiveCreativeVec3(cutout.size) ||
      request.cutoutBounds.min.x <
          request.wallFrame.bounds.min.x - kGeometryEpsilon ||
      request.cutoutBounds.min.y <
          request.wallFrame.bounds.min.y - kGeometryEpsilon ||
      request.cutoutBounds.min.z <
          request.wallFrame.bounds.min.z - kGeometryEpsilon ||
      request.cutoutBounds.max.x >
          request.wallFrame.bounds.max.x + kGeometryEpsilon ||
      request.cutoutBounds.max.y >
          request.wallFrame.bounds.max.y + kGeometryEpsilon ||
      request.cutoutBounds.max.z >
          request.wallFrame.bounds.max.z + kGeometryEpsilon) {
    reject(result, CreativeWindowRecipeStatus::InvalidCutout,
           "creative_window_cutout_invalid");
    return result;
  }
  if (!isValidCreativeWindowSettings(request.settings)) {
    reject(result, CreativeWindowRecipeStatus::InvalidSettings,
           "creative_window_settings_invalid");
    return result;
  }
  if (!positiveFinite(request.frameWidthMeters) ||
      !nonNegativeFinite(request.insertGapMeters) ||
      !nonNegativeFinite(request.insertThicknessMeters)) {
    reject(result, CreativeWindowRecipeStatus::InvalidDimensions,
           "creative_window_dimensions_invalid");
    return result;
  }

  const double minimum = axisMinimum(request.wallFrame, request.cutoutBounds);
  const double maximum = axisMaximum(request.wallFrame, request.cutoutBounds);
  const double innerMinimum = minimum + request.frameWidthMeters;
  const double innerMaximum = maximum - request.frameWidthMeters;
  const double innerBottom =
      request.cutoutBounds.min.y + request.frameWidthMeters;
  const double innerTop =
      request.cutoutBounds.max.y - request.frameWidthMeters;
  const double insertMinimum = innerMinimum + request.insertGapMeters;
  const double insertMaximum = innerMaximum - request.insertGapMeters;
  const double insertBottom = innerBottom + request.insertGapMeters;
  const double insertTop = innerTop - request.insertGapMeters;
  const double insertThickness =
      request.insertThicknessMeters > 0.0
          ? request.insertThicknessMeters
          : std::min(0.02, request.wallFrame.thicknessMeters * 0.25);
  if (!positiveFinite(insertThickness) ||
      insertThickness > request.wallFrame.thicknessMeters + kGeometryEpsilon ||
      insertMaximum <= insertMinimum + kGeometryEpsilon ||
      insertTop <= insertBottom + kGeometryEpsilon ||
      request.frameWidthMeters * 2.0 >=
          maximum - minimum - kGeometryEpsilon ||
      request.frameWidthMeters * 2.0 >= cutout.size.y - kGeometryEpsilon) {
    reject(result, CreativeWindowRecipeStatus::InvalidDimensions,
           "creative_window_dimensions_unrepresentable");
    return result;
  }

  const double normal = normalCenter(request.wallFrame, request.cutoutBounds);
  const double frameNormalMinimum =
      normal - request.wallFrame.thicknessMeters * 0.5;
  const double frameNormalMaximum =
      normal + request.wallFrame.thicknessMeters * 0.5;
  if (!appendPart(result, CreativeWindowPartKind::MinimumJamb,
                  CreativeObjectKind::Beam,
                  boundsForAxes(request.wallFrame, minimum, innerMinimum,
                                request.cutoutBounds.min.y,
                                request.cutoutBounds.max.y,
                                frameNormalMinimum, frameNormalMaximum)) ||
      !appendPart(result, CreativeWindowPartKind::MaximumJamb,
                  CreativeObjectKind::Beam,
                  boundsForAxes(request.wallFrame, innerMaximum, maximum,
                                request.cutoutBounds.min.y,
                                request.cutoutBounds.max.y,
                                frameNormalMinimum, frameNormalMaximum)) ||
      !appendPart(result, CreativeWindowPartKind::FrameSill,
                  CreativeObjectKind::Beam,
                  boundsForAxes(request.wallFrame, innerMinimum, innerMaximum,
                                request.cutoutBounds.min.y, innerBottom,
                                frameNormalMinimum, frameNormalMaximum)) ||
      !appendPart(result, CreativeWindowPartKind::FrameHeader,
                  CreativeObjectKind::Beam,
                  boundsForAxes(request.wallFrame, innerMinimum, innerMaximum,
                                innerTop, request.cutoutBounds.max.y,
                                frameNormalMinimum, frameNormalMaximum))) {
    reject(result, CreativeWindowRecipeStatus::CapacityExceeded,
           "creative_window_part_capacity_exceeded");
    return result;
  }

  const double insertHalfThickness = insertThickness * 0.5;
  result.framedOpeningBounds =
      boundsForAxes(request.wallFrame, insertMinimum, insertMaximum,
                    insertBottom, insertTop, normal - insertHalfThickness,
                    normal + insertHalfThickness);
  result.primaryInsertPartIndex = result.partCount;
  if (request.settings.insertKind == CreativeWindowInsertKind::Glazing ||
      !request.proceduralInsert) {
    if (!appendPart(result, CreativeWindowPartKind::PrimaryInsert,
                    CreativeObjectKind::Window,
                    result.framedOpeningBounds)) {
      reject(result, CreativeWindowRecipeStatus::CapacityExceeded,
             "creative_window_part_capacity_exceeded");
      return result;
    }
  } else {
    const double center = (insertMinimum + insertMaximum) * 0.5;
    const double halfGap = request.insertGapMeters * 0.5;
    if (center - halfGap <= insertMinimum + kGeometryEpsilon ||
        center + halfGap >= insertMaximum - kGeometryEpsilon ||
        !appendPart(
            result, CreativeWindowPartKind::PrimaryInsert,
            CreativeObjectKind::Window,
            boundsForAxes(request.wallFrame, insertMinimum, center - halfGap,
                          insertBottom, insertTop,
                          normal - insertHalfThickness,
                          normal + insertHalfThickness)) ||
        !appendPart(
            result, CreativeWindowPartKind::SecondaryShutter,
            CreativeObjectKind::Prop,
            boundsForAxes(request.wallFrame, center + halfGap, insertMaximum,
                          insertBottom, insertTop,
                          normal - insertHalfThickness,
                          normal + insertHalfThickness))) {
      reject(result, CreativeWindowRecipeStatus::CapacityExceeded,
             "creative_window_part_capacity_exceeded");
      return result;
    }
  }

  result.accepted = true;
  result.status = CreativeWindowRecipeStatus::Ready;
  result.reasonCode = "creative_window_ready";
  return result;
}

}  // namespace iggy3d::creative
