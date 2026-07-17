#include "app/iggy3d/creative/recipes/StructuralRoofRecipe.hpp"

#include "app/iggy3d/creative/recipes/StructuralSurfaceRecipe.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace iggy3d::creative {
namespace {

void reject(CreativeStructuralRoofRecipeResult& result,
            CreativeStructuralRoofRecipeStatus status,
            std::string_view reasonCode) noexcept {
  result.status = status;
  result.reasonCode = reasonCode;
}

[[nodiscard]] CreativeBounds mergedBounds(CreativeBounds lhs,
                                          CreativeBounds rhs) noexcept {
  return {{std::min(lhs.min.x, rhs.min.x), std::min(lhs.min.y, rhs.min.y),
           std::min(lhs.min.z, rhs.min.z)},
          {std::max(lhs.max.x, rhs.max.x), std::max(lhs.max.y, rhs.max.y),
           std::max(lhs.max.z, rhs.max.z)}};
}

}  // namespace

bool validCreativeStructuralRoofSettings(
    CreativeStructuralRoofStyle style,
    CreativeStructuralRoofRidgeAxis ridgeAxis,
    double pitchDegrees,
    double overhangMeters) noexcept {
  return style < CreativeStructuralRoofStyle::Count &&
         ridgeAxis < CreativeStructuralRoofRidgeAxis::Count &&
         std::isfinite(pitchDegrees) &&
         pitchDegrees >= kMinimumCreativeStructuralRoofPitchDegrees &&
         pitchDegrees <= kMaximumCreativeStructuralRoofPitchDegrees &&
         std::isfinite(overhangMeters) && overhangMeters >= 0.0;
}

CreativeStructuralRoofRecipeResult planCreativeStructuralRoof(
    const CreativeStructuralRoofRecipeRequest& request) noexcept {
  CreativeStructuralRoofRecipeResult result;
  if (!std::isfinite(request.minimumX) ||
      !std::isfinite(request.maximumX) ||
      !std::isfinite(request.minimumZ) ||
      !std::isfinite(request.maximumZ) ||
      request.minimumX >= request.maximumX ||
      request.minimumZ >= request.maximumZ) {
    reject(result, CreativeStructuralRoofRecipeStatus::InvalidFootprint,
           "creative_structural_roof_footprint_invalid");
    return result;
  }
  if (!std::isfinite(request.supportPlaneMeters)) {
    reject(result, CreativeStructuralRoofRecipeStatus::InvalidSupportPlane,
           "creative_structural_roof_support_invalid");
    return result;
  }
  if (request.layerCount == 0U) {
    reject(result, CreativeStructuralRoofRecipeStatus::InvalidLayerCount,
           "creative_structural_roof_layers_invalid");
    return result;
  }
  if (request.style >= CreativeStructuralRoofStyle::Count) {
    reject(result, CreativeStructuralRoofRecipeStatus::InvalidStyle,
           "creative_structural_roof_style_invalid");
    return result;
  }
  if (request.ridgeAxis >= CreativeStructuralRoofRidgeAxis::Count) {
    reject(result, CreativeStructuralRoofRecipeStatus::InvalidRidgeAxis,
           "creative_structural_roof_ridge_axis_invalid");
    return result;
  }
  if (!std::isfinite(request.pitchDegrees) ||
      request.pitchDegrees < kMinimumCreativeStructuralRoofPitchDegrees ||
      request.pitchDegrees > kMaximumCreativeStructuralRoofPitchDegrees) {
    reject(result, CreativeStructuralRoofRecipeStatus::InvalidPitch,
           "creative_structural_roof_pitch_invalid");
    return result;
  }
  if (!std::isfinite(request.overhangMeters) ||
      request.overhangMeters < 0.0) {
    reject(result, CreativeStructuralRoofRecipeStatus::InvalidOverhang,
           "creative_structural_roof_overhang_invalid");
    return result;
  }

  const double minimumX = request.minimumX - request.overhangMeters;
  const double maximumX = request.maximumX + request.overhangMeters;
  const double minimumZ = request.minimumZ - request.overhangMeters;
  const double maximumZ = request.maximumZ + request.overhangMeters;
  if (!std::isfinite(minimumX) || !std::isfinite(maximumX) ||
      !std::isfinite(minimumZ) || !std::isfinite(maximumZ) ||
      minimumX >= maximumX || minimumZ >= maximumZ) {
    reject(result,
           CreativeStructuralRoofRecipeStatus::UnrepresentableGeometry,
           "creative_structural_roof_overhang_unrepresentable");
    return result;
  }

  const CreativeStructuralSurfaceRecipeResult base =
      planCreativeStructuralSurface(
          {CreativeObjectKind::Roof, minimumX, maximumX, minimumZ, maximumZ,
           request.supportPlaneMeters, request.layerCount});
  if (!base.accepted) {
    reject(result,
           CreativeStructuralRoofRecipeStatus::UnrepresentableGeometry,
           base.reasonCode);
    return result;
  }
  result.parts[0] = {CreativeObjectKind::Roof, base.bounds, {}};
  result.partCount = 1U;
  result.worldBounds = base.bounds;
  result.baseThicknessMeters = base.totalThicknessMeters;

  if (request.style == CreativeStructuralRoofStyle::Flat) {
    result.accepted = true;
    result.status = CreativeStructuralRoofRecipeStatus::Ready;
    result.reasonCode = "creative_structural_roof_flat_ready";
    return result;
  }

  const double width = maximumX - minimumX;
  const double depth = maximumZ - minimumZ;
  const double span =
      request.ridgeAxis == CreativeStructuralRoofRidgeAxis::X ? depth : width;
  const double ridgeLength =
      request.ridgeAxis == CreativeStructuralRoofRidgeAxis::X ? width : depth;
  const double halfSpan = span * 0.5;
  const double pitchRadians =
      request.pitchDegrees * std::numbers::pi / 180.0;
  result.riseMeters = std::tan(pitchRadians) * halfSpan;
  const double slopeBottom = base.bounds.max.y;
  const double slopeTop = slopeBottom + result.riseMeters;
  if (!std::isfinite(result.riseMeters) || result.riseMeters <= 0.0 ||
      !std::isfinite(slopeTop) || slopeBottom >= slopeTop ||
      !std::isfinite(ridgeLength) || ridgeLength <= 0.0) {
    result = {};
    reject(result,
           CreativeStructuralRoofRecipeStatus::UnrepresentableGeometry,
           "creative_structural_roof_slope_unrepresentable");
    return result;
  }

  const double centerX = (minimumX + maximumX) * 0.5;
  const double centerZ = (minimumZ + maximumZ) * 0.5;
  // RampWedge rises along its local +Z axis. Keep each authored wedge X-long;
  // the Z-ridge branch rotates that local shape into its world footprint.
  const auto authoredWedgeBounds = [&](double pivotX, double pivotZ) {
    CreativeBounds bounds;
    bounds.min.x = pivotX - ridgeLength * 0.5;
    bounds.max.x = pivotX + ridgeLength * 0.5;
    bounds.min.y = slopeBottom;
    bounds.max.y = slopeTop;
    bounds.min.z = pivotZ - halfSpan * 0.5;
    bounds.max.z = pivotZ + halfSpan * 0.5;
    return bounds;
  };

  if (request.ridgeAxis == CreativeStructuralRoofRidgeAxis::X) {
    const double northCenterZ = minimumZ + halfSpan * 0.5;
    const double southCenterZ = maximumZ - halfSpan * 0.5;
    result.parts[1] = {CreativeObjectKind::GableRoof,
                       authoredWedgeBounds(centerX, northCenterZ), {}};
    result.parts[2] = {
        CreativeObjectKind::GableRoof,
        authoredWedgeBounds(centerX, southCenterZ),
        {0.0, std::numbers::pi, 0.0}};
    result.ridgeStart = {minimumX, slopeTop, centerZ};
    result.ridgeEnd = {maximumX, slopeTop, centerZ};
  } else {
    const double westCenterX = minimumX + halfSpan * 0.5;
    const double eastCenterX = maximumX - halfSpan * 0.5;
    result.parts[1] = {
        CreativeObjectKind::GableRoof,
        authoredWedgeBounds(westCenterX, centerZ),
        {0.0, std::numbers::pi * 0.5, 0.0}};
    result.parts[2] = {
        CreativeObjectKind::GableRoof,
        authoredWedgeBounds(eastCenterX, centerZ),
        {0.0, -std::numbers::pi * 0.5, 0.0}};
    result.ridgeStart = {centerX, slopeTop, minimumZ};
    result.ridgeEnd = {centerX, slopeTop, maximumZ};
  }
  result.partCount = 3U;
  result.worldBounds = mergedBounds(
      result.worldBounds,
      {{minimumX, slopeBottom, minimumZ}, {maximumX, slopeTop, maximumZ}});
  result.accepted = true;
  result.status = CreativeStructuralRoofRecipeStatus::Ready;
  result.reasonCode = "creative_structural_roof_gable_ready";
  return result;
}

}  // namespace iggy3d::creative
