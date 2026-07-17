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

CreativeStructuralSurfaceCutoutResult planCreativeStructuralSurfaceCutout(
    const CreativeStructuralSurfaceCutoutRequest& request) noexcept {
  CreativeStructuralSurfaceCutoutResult result;
  const CreativeStructuralSurfaceRecipeResult full =
      planCreativeStructuralSurface(request.surface);
  if (!full.accepted) {
    result.status = CreativeStructuralSurfaceCutoutStatus::InvalidSurface;
    result.reasonCode = full.reasonCode;
    return result;
  }

  if (!std::isfinite(request.cutoutMinimumX) ||
      !std::isfinite(request.cutoutMaximumX) ||
      !std::isfinite(request.cutoutMinimumZ) ||
      !std::isfinite(request.cutoutMaximumZ) ||
      request.cutoutMinimumX >= request.cutoutMaximumX ||
      request.cutoutMinimumZ >= request.cutoutMaximumZ) {
    result.status = CreativeStructuralSurfaceCutoutStatus::InvalidCutout;
    result.reasonCode = "creative_structural_surface_cutout_invalid";
    return result;
  }
  if (request.cutoutMinimumX < request.surface.minimumX ||
      request.cutoutMaximumX > request.surface.maximumX ||
      request.cutoutMinimumZ < request.surface.minimumZ ||
      request.cutoutMaximumZ > request.surface.maximumZ) {
    result.status = CreativeStructuralSurfaceCutoutStatus::CutoutOutsideSurface;
    result.reasonCode = "creative_structural_surface_cutout_outside_surface";
    return result;
  }

  const auto appendPiece = [&](double minimumX, double maximumX,
                               double minimumZ, double maximumZ) {
    if (minimumX >= maximumX || minimumZ >= maximumZ) {
      return true;
    }
    CreativeStructuralSurfaceRecipeRequest piece = request.surface;
    piece.minimumX = minimumX;
    piece.maximumX = maximumX;
    piece.minimumZ = minimumZ;
    piece.maximumZ = maximumZ;
    const CreativeStructuralSurfaceRecipeResult planned =
        planCreativeStructuralSurface(piece);
    if (!planned.accepted || result.pieceCount >= result.pieces.size()) {
      return false;
    }
    result.pieces[result.pieceCount++] = planned;
    return true;
  };

  const bool planned =
      appendPiece(request.surface.minimumX, request.cutoutMinimumX,
                  request.surface.minimumZ, request.surface.maximumZ) &&
      appendPiece(request.cutoutMaximumX, request.surface.maximumX,
                  request.surface.minimumZ, request.surface.maximumZ) &&
      appendPiece(request.cutoutMinimumX, request.cutoutMaximumX,
                  request.surface.minimumZ, request.cutoutMinimumZ) &&
      appendPiece(request.cutoutMinimumX, request.cutoutMaximumX,
                  request.cutoutMaximumZ, request.surface.maximumZ);
  if (!planned) {
    result.pieces = {};
    result.pieceCount = 0U;
    result.status = CreativeStructuralSurfaceCutoutStatus::InvalidSurface;
    result.reasonCode = "creative_structural_surface_cutout_piece_rejected";
    return result;
  }
  if (result.pieceCount == 0U) {
    result.status =
        CreativeStructuralSurfaceCutoutStatus::CutoutConsumesSurface;
    result.reasonCode = "creative_structural_surface_cutout_consumes_surface";
    return result;
  }

  result.accepted = true;
  result.status = CreativeStructuralSurfaceCutoutStatus::Ready;
  result.reasonCode = "creative_structural_surface_cutout_ready";
  return result;
}

}  // namespace iggy3d::creative
