#pragma once

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeStructuralSurfaceRecipeStatus : std::uint8_t {
  NotRequested,
  InvalidFootprint,
  InvalidAnchorPlane,
  InvalidLayerCount,
  UnsupportedKind,
  UnrepresentableBounds,
  Ready,
};

struct CreativeStructuralSurfaceRecipeRequest {
  CreativeObjectKind kind = CreativeObjectKind::Unknown;
  double minimumX = 0.0;
  double maximumX = 0.0;
  double minimumZ = 0.0;
  double maximumZ = 0.0;
  double anchorPlaneMeters = 0.0;
  std::uint16_t layerCount = 1U;
};

struct CreativeStructuralSurfaceRecipeResult {
  bool accepted = false;
  CreativeStructuralSurfaceRecipeStatus status =
      CreativeStructuralSurfaceRecipeStatus::NotRequested;
  CreativeStructuralSurfaceAnchor anchor =
      CreativeStructuralSurfaceAnchor::None;
  CreativeBounds bounds;
  double totalThicknessMeters = 0.0;
  std::string_view reasonCode =
      "creative_structural_surface_not_requested";
};

[[nodiscard]] CreativeStructuralSurfaceRecipeResult
planCreativeStructuralSurface(
    const CreativeStructuralSurfaceRecipeRequest& request) noexcept;

}  // namespace iggy3d::creative
