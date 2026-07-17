#include "app/iggy3d/creative/recipes/StructuralSurfaceRecipe.hpp"

#include <cmath>

namespace iggy3d::creative {
namespace {

void reject(CreativeStructuralSurfaceRecipeResult& result,
            CreativeStructuralSurfaceRecipeStatus status,
            std::string_view reasonCode) noexcept {
  result.status = status;
  result.reasonCode = reasonCode;
}

}  // namespace

CreativeStructuralSurfaceRecipeResult planCreativeStructuralSurface(
    const CreativeStructuralSurfaceRecipeRequest& request) noexcept {
  CreativeStructuralSurfaceRecipeResult result;
  if (!std::isfinite(request.minimumX) ||
      !std::isfinite(request.maximumX) ||
      !std::isfinite(request.minimumZ) ||
      !std::isfinite(request.maximumZ) ||
      request.minimumX >= request.maximumX ||
      request.minimumZ >= request.maximumZ) {
    reject(result, CreativeStructuralSurfaceRecipeStatus::InvalidFootprint,
           "creative_structural_surface_footprint_invalid");
    return result;
  }
  if (!std::isfinite(request.anchorPlaneMeters)) {
    reject(result, CreativeStructuralSurfaceRecipeStatus::InvalidAnchorPlane,
           "creative_structural_surface_anchor_invalid");
    return result;
  }
  if (request.layerCount == 0U) {
    reject(result, CreativeStructuralSurfaceRecipeStatus::InvalidLayerCount,
           "creative_structural_surface_layer_count_invalid");
    return result;
  }

  result.anchor = creativeStructuralSurfaceAnchor(request.kind);
  const double layerThickness =
      defaultCreativeStructuralLayerThicknessMeters(request.kind);
  if (result.anchor == CreativeStructuralSurfaceAnchor::None ||
      !std::isfinite(layerThickness) || layerThickness <= 0.0) {
    reject(result, CreativeStructuralSurfaceRecipeStatus::UnsupportedKind,
           "creative_structural_surface_kind_unsupported");
    return result;
  }

  result.totalThicknessMeters =
      layerThickness * static_cast<double>(request.layerCount);
  if (!std::isfinite(result.totalThicknessMeters) ||
      result.totalThicknessMeters <= 0.0) {
    reject(result,
           CreativeStructuralSurfaceRecipeStatus::UnrepresentableBounds,
           "creative_structural_surface_thickness_unrepresentable");
    return result;
  }

  result.bounds.min.x = request.minimumX;
  result.bounds.max.x = request.maximumX;
  result.bounds.min.z = request.minimumZ;
  result.bounds.max.z = request.maximumZ;
  if (result.anchor == CreativeStructuralSurfaceAnchor::TopPlane) {
    result.bounds.min.y =
        request.anchorPlaneMeters - result.totalThicknessMeters;
    result.bounds.max.y = request.anchorPlaneMeters;
  } else {
    result.bounds.min.y = request.anchorPlaneMeters;
    result.bounds.max.y =
        request.anchorPlaneMeters + result.totalThicknessMeters;
  }
  if (!std::isfinite(result.bounds.min.y) ||
      !std::isfinite(result.bounds.max.y) ||
      result.bounds.min.y >= result.bounds.max.y) {
    result.bounds = {};
    result.totalThicknessMeters = 0.0;
    reject(result,
           CreativeStructuralSurfaceRecipeStatus::UnrepresentableBounds,
           "creative_structural_surface_bounds_unrepresentable");
    return result;
  }

  result.accepted = true;
  result.status = CreativeStructuralSurfaceRecipeStatus::Ready;
  result.reasonCode = "creative_structural_surface_ready";
  return result;
}

}  // namespace iggy3d::creative
